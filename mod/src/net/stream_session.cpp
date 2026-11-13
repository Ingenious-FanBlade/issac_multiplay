#include "net/stream_session.h"
#include "state/host_publish.h"
#include "replica/stream_e2e.h"
#include <algorithm>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace cs::net::live {
namespace s = replica::stream;
namespace r = replica;
namespace {
[[noreturn]] void Fail(const char *reason) { throw r::ContractError(reason); }
unsigned Lane(s::Lane lane) {
  return lane == s::Lane::PlayerFast                             ? 1
         : lane == s::Lane::WorldDynamic                         ? 2
         : lane == s::Lane::Baseline || lane == s::Lane::Context ? 3
                                                                 : 0;
}
std::uint64_t Read(std::span<const std::uint8_t> b, std::size_t at, unsigned width) {
  if (at + width > b.size())
    Fail("uplink_packet_length");
  std::uint64_t n = 0;
  for (unsigned i = 0; i < width; ++i)
    n |= std::uint64_t(b[at + i]) << (8 * i);
  return n;
}
bool Final(OperationStage s) {
  return s == OperationStage::Complete || s == OperationStage::Cancelled ||
         s == OperationStage::Failed;
}
void N(r::Value &v, const char *name, std::uint64_t n, r::Allocator &a) {
  r::Put(v, name, r::Decimal(n, a), a);
}
bool Same(const s::Message &a, const s::Message &b) {
  return a.kind == b.kind && a.sequence == b.sequence && a.connection == b.connection &&
         a.route == b.route && a.body == b.body;
}
} // namespace
void ObservePacket(s::Metrics &m,std::span<const std::uint8_t> b,const char *direction) noexcept {
  try {
    std::array<std::uint64_t,4> costs{};unsigned messages=1;
    if(s::IsBundle(b)){
      messages=unsigned(Read(b,24,2));std::size_t at=28;std::uint64_t assigned=0;
      if(!messages||messages>16||b.size()<=28)throw r::ContractError("metric_bundle");
      for(unsigned i=0;i<messages;++i){const auto n=Read(b,at,2),lane=Read(b,at+4,1);if(lane<1||lane>6||at+22+n>b.size())throw r::ContractError("metric_lane");const auto cost=i+1==messages?b.size()+28-assigned:(22+n)*(b.size()+28)/(b.size()-28);costs[Lane(s::Lane(lane))]+=cost;assigned+=cost;at+=22+std::size_t(n);}
      if(at!=b.size())throw r::ContractError("metric_length");
    }else costs[Lane(s::ReadHeader(b).lane)]=b.size()+28;
    ++m.packets;m.payload_bytes+=b.size();m.wire_bytes+=b.size()+28;m.bundle_messages+=messages;for(unsigned i=0;i<4;++i){m.lane_bytes[i]+=costs[i];if(costs[i])++m.lane_packets[i];}
#ifdef ISAACCS_E2E
    s::E2EPacket(Read(b,8,8),Read(b,16,8),b.size()+28,costs,direction);
#endif
  }catch(...){++m.missing;}
}
void Uplink::Bucket::Fill(std::uint64_t now) {
  if (!started) {
    tokens = burst * 1000;
    at = now;
    started = true;
    return;
  }
  if (now < at)
    return;
  const auto elapsed = (std::min)(now - at, 1000ull);
  tokens = (std::min)(burst * 1000, tokens + rate * elapsed);
  at = now;
}
void Uplink::Configure(const s::RuntimeConfig &c) noexcept {config_=c;total_={c.total_bytes_per_second,c.total_burst_bytes};for(auto &p:peers_){p.total={c.peer_bytes_per_second,c.peer_burst_bytes};p.lanes={{{c.control_bytes_per_second,c.control_burst_bytes},{c.player_bytes_per_second,c.player_burst_bytes},{c.world_bytes_per_second,c.world_burst_bytes},{c.reliable_bytes_per_second,c.reliable_burst_bytes}}};}}
s::SendResult Uplink::Transmit(unsigned peer, std::span<const std::uint8_t> b, bool initializing,
                               std::uint64_t now, const Send &send) {
  if (!peer || peer >= peers_.size() || b.empty() || b.size() > 1200)
    Fail("uplink_peer_or_packet");
  std::array<std::uint64_t, 4> costs{};
  if (s::IsBundle(b)) {
    // 物理包只收费一次；共同28字节容器头与28字节IP/UDP按各条目长度分摊。
    const auto count = Read(b, 24, 2);
    std::size_t at = 28;
    std::uint64_t assigned = 0;
    if (!count || count > 16 || b.size() <= 28)
      Fail("uplink_bundle_count");
    for (unsigned i = 0; i < count; ++i) {
      const auto n = Read(b, at, 2);
      const auto lane = Read(b, at + 4, 1);
      if (lane < 1 || lane > 6 || at + 22 + n > b.size())
        Fail("uplink_bundle_length");
      const auto cost =
          i + 1 == count ? b.size() + 28 - assigned : (22 + n) * (b.size() + 28) / (b.size() - 28);
      costs[Lane(s::Lane(lane))] += cost;
      assigned += cost;
      at += 22 + std::size_t(n);
    }
    if (at != b.size())
      Fail("uplink_bundle_trailing");
  } else
    costs[b.size() >= 4 && !std::memcmp(b.data(), "ICRF", 4) ? Lane(s::ReadHeader(b).lane) : 0] =
        b.size() + 28;
  auto &p = peers_[peer];
  auto &status = stats_[peer];
  total_.Fill(now);
  p.total.Fill(now);
  for (auto &bucket : p.lanes)
    bucket.Fill(now);
  const auto all = (b.size() + 28) * 1000;
  auto debit = costs;
  for (auto &n : debit)
    n *= 1000;
  if (initializing && config_.borrow_initial_lanes && debit[3] > p.lanes[3].tokens) {
    auto extra = debit[3] - p.lanes[3].tokens;
    debit[3] -= extra;
    for (unsigned i : {1u, 2u}) {
      const auto available = p.lanes[i].tokens > debit[i] ? p.lanes[i].tokens - debit[i] : 0;
      const auto take = (std::min)(extra, available);
      debit[i] += take;
      extra -= take;
    }
    if (extra) {
      ++status.blocked;
      return s::SendResult::WouldBlock;
    }
  }
  if (total_.tokens < all || p.total.tokens < all) {
    ++status.blocked;
    return s::SendResult::WouldBlock;
  }
  for (unsigned i = 0; i < 4; ++i)
    if (p.lanes[i].tokens < debit[i]) {
      ++status.blocked;
      return s::SendResult::WouldBlock;
    }
  const auto result = send(b);
  if (result != s::SendResult::Sent)
    return result;
  total_.tokens -= all;
  p.total.tokens -= all;
  for (unsigned i = 0; i < 4; ++i) {
    p.lanes[i].tokens -= debit[i];
    status.lanes[i] += costs[i];if(costs[i])++status.lane_packets[i];
  }
  ++status.packets;
  status.payload_bytes+=b.size();
  status.bundle_messages+=s::IsBundle(b)?Read(b,24,2):1;
  status.wire_bytes += b.size() + 28;
#ifdef ISAACCS_E2E
  s::E2EPacket(Read(b,8,8),Read(b,16,8),b.size()+28,costs,"host-send");
#endif
  return result;
}
bool Operations::Reliable(s::Kind k) noexcept {
  return k == s::Kind::ScenePrepare || k == s::Kind::Cancel || k == s::Kind::Retire ||
         k == s::Kind::Stop || k == s::Kind::Disconnect;
}
void Operations::Bind(std::uint64_t connection, std::uint64_t route) {
  if (connection_ == connection && route_ == route)
    return;
  sent_.clear();
  received_.clear();
  connection_ = connection;
  route_ = route;
  sent_floor_.clear();
  received_floor_.clear();
  result_sequence_ = 0;
  failure_.clear();
}
void Operations::Trim(std::map<OperationKey, Record> &records,
                      std::map<s::Kind, std::uint64_t> &floor) {
  while (records.size() >= 128) {
    auto i = std::find_if(records.begin(), records.end(),
                          [](const auto &v) { return Final(v.second.stage) && v.second.receipt; });
    if (i == records.end())
      Fail("reliable_operation_history_full");
    floor[i->first.kind] = (std::max)(floor[i->first.kind], i->first.sequence);
    records.erase(i);
  }
}
void Operations::Queue(std::shared_ptr<const s::Message> m, std::uint64_t now) {
  if (!m || !Reliable(m->kind) || m->connection != connection_ || m->route != route_)
    Fail("reliable_operation_route");
  s::ValidateMessage(*m);
  OperationKey key{m->kind, m->sequence};
  auto found = sent_.find(key);
  if (found != sent_.end()) {
    if (!Same(*m, *found->second.message))
      Fail("reliable_operation_conflict");
    return;
  }
  if (m->sequence <= sent_floor_[m->kind])
    Fail("reliable_operation_stale");
  const bool urgent = m->kind == s::Kind::Stop || m->kind == s::Kind::Disconnect ||
                      m->kind == s::Kind::Retire || m->kind == s::Kind::Cancel;
  if (std::count_if(sent_.begin(), sent_.end(),
                    [](const auto &v) { return !Final(v.second.stage); }) >= (urgent ? 36 : 32))
    Fail("reliable_operation_capacity");
  Trim(sent_, sent_floor_);
  Record v;
  v.memory = memory::Reserve(memory::Pool::Wire, sizeof(Record) + 256);
  v.stage = OperationStage::Queued;
  v.progress_sequence = 0;
  if (m->kind == s::Kind::Cancel)
    ++statistics_.cancellations;
  v.message = std::move(m);
  v.started = v.progress = v.due = now;
  sent_.emplace(key, std::move(v));
}
bool Operations::Admit(std::shared_ptr<const s::Message> m, std::uint64_t now) {
  if (!m || !Reliable(m->kind) || m->connection != connection_ || m->route != route_)
    Fail("reliable_operation_route");
  OperationKey key{m->kind, m->sequence};
  auto found = received_.find(key);
  if (found != received_.end()) {
    if (!Same(*m, *found->second.message))
      Fail("reliable_operation_conflict");
    found->second.due = now;
    return false;
  }
  if (m->sequence <= received_floor_[m->kind])
    return false;
  // 停止、取消、退休另有保留位置，不能被等待世界依赖占满。
  const bool urgent = m->kind == s::Kind::Stop || m->kind == s::Kind::Disconnect ||
                      m->kind == s::Kind::Cancel || m->kind == s::Kind::Retire;
  if (std::count_if(received_.begin(), received_.end(),
                    [](const auto &v) { return !Final(v.second.stage); }) >= (urgent ? 36 : 32))
    Fail("reliable_operation_capacity");
  Trim(received_, received_floor_);
  Record v;
  v.memory = memory::Reserve(memory::Pool::Wire, sizeof(Record) + 256);
  v.message = std::move(m);
  v.started = v.progress = v.due = now;
  received_.emplace(key, std::move(v));
  return true;
}
void Operations::Finish(OperationKey key, OperationStage stage, const char *reason,
                        std::uint64_t now) {
  auto i = received_.find(key);
  if (i == received_.end())
    Fail("reliable_operation_not_admitted");
  auto &v = i->second;
  if (Final(v.stage))
    return;
  if (stage == OperationStage::Accepted || stage == OperationStage::Receipt)
    Fail("reliable_operation_stage");
  if (stage == OperationStage::Complete)
    ++statistics_.completed;
  else if (stage == OperationStage::Failed)
    ++statistics_.failed;
  v.stage = stage;
  v.reason = reason;
  v.progress = v.due = now;
  if (v.progress_sequence == UINT64_MAX)
    Fail("operation_progress_exhausted");
  ++v.progress_sequence;
}
void Operations::Acknowledge(const s::Message &m, std::uint64_t now) {
  s::ValidateMessage(m);
  if (m.kind != s::Kind::OperationAck || m.connection != connection_ || m.route != route_)
    Fail("operation_ack_route");
  const OperationKey key{s::Kind(r::Unsigned(m.body["operation"])),
                         r::Unsigned(m.body["operation_sequence"])};
  const auto stage = OperationStage(r::Unsigned(m.body["stage"]));
  if (stage == OperationStage::Receipt) {
    auto i = received_.find(key);
    if (i != received_.end() && Final(i->second.stage))
      i->second.receipt = true;
    return;
  }
  auto i = sent_.find(key);
  if (i == sent_.end())
    return;
  auto &v = i->second;
  if (!v.retries)
    Fail("operation_ack_unsent");
  const auto progress_sequence = r::Unsigned(m.body["progress_sequence"]);
  if (!Final(v.stage) && unsigned(stage) >= unsigned(v.stage) &&
      progress_sequence > v.progress_sequence) {
    v.progress_sequence = progress_sequence;
    if (v.retries == 1 && now >= v.sent)
      rtt_.Observe(now - v.sent);
    v.stage = stage;
    v.reason.assign(m.body["reason"].GetString(), m.body["reason"].GetStringLength());
    v.progress = now;
  }
  if (Final(v.stage)) {
    v.receipt = false;
    v.due = now;
    if (v.stage == OperationStage::Failed && failure_.empty())
      failure_ = v.reason;
  }
}
void Operations::Pump(std::uint64_t now, const std::function<bool(const s::Message &)> &send) {
  const auto result = [&](OperationKey key, Record &v, OperationStage stage) {
    if (result_sequence_ == UINT64_MAX - 1)
      Fail("operation_result_sequence_exhausted");
    s::Message m;
    m.memory = memory::Reserve(memory::Pool::Values, 8192, "operation_control");
    m.kind = s::Kind::OperationAck;
    m.sequence = ++result_sequence_;
    m.connection = connection_;
    m.route = route_;
    m.body.SetObject();
    auto &a = m.body.GetAllocator();
    N(m.body, "operation", unsigned(key.kind), a);
    N(m.body, "operation_sequence", key.sequence, a);
    N(m.body, "stage", unsigned(stage), a);
    N(m.body, "progress_sequence", v.progress_sequence, a);
    r::Put(m.body, "reason", v.reason, a);
    return send(m);
  };
  for (auto &[key, v] : sent_) {
    if (!Final(v.stage) && (now < v.progress || now - v.progress >= config_.progress_timeout_ms || now < v.started ||
                            now - v.started >= config_.absolute_timeout_ms)) {
#ifdef ISAACCS_E2E
      // 首次超时保留真实操作kind/sequence及已取得进度，不以笼统连接错误丢掉定位依据。
      s::E2EStreamProgress("operation-sent-timeout",{key.sequence,unsigned(key.kind),unsigned(v.stage),v.progress_sequence,v.started,now,v.progress,now>=v.progress?now-v.progress:0});
#endif
      v.stage = OperationStage::Failed;
      ++statistics_.failed;
      v.reason = "operation_no_progress_or_deadline";
      if (failure_.empty())
        failure_ = v.reason;
      v.receipt = true;
    }
    if (now < v.due)
      continue;
    if (Final(v.stage)) {
      if (!v.receipt && result(key, v, OperationStage::Receipt))
        v.receipt = true;
    } else if (send(*v.message)) {
      v.sent = now;
      v.due = now + rtt_.Rto();
      if (v.retries)
        ++statistics_.retransmissions;
      ++v.retries;
    }
  }
  for (auto &[key, v] : received_) {
    if (!Final(v.stage) && (now < v.progress || now - v.progress >= config_.progress_timeout_ms || now < v.started ||
                            now - v.started >= config_.absolute_timeout_ms)) {
      v.stage = OperationStage::Failed;
      ++v.progress_sequence;
      ++statistics_.failed;
      v.reason = "operation_execution_no_progress";
      if (failure_.empty())
        failure_ = v.reason;
    }
    if (!v.receipt && now >= v.due && result(key, v, v.stage))
      v.due = now + rtt_.Rto();
  }
}
void Operations::CancelPending(std::uint64_t now) {
  for (auto *records : {&sent_, &received_})
    for (auto &[key, v] : *records)
      if (!Final(v.stage)) {
        v.stage = OperationStage::Cancelled;
        v.reason = "cancelled_by_stop";
        v.progress = v.due = now;
        ++v.progress_sequence;
        if (records == &sent_)
          v.receipt = true;
        ++statistics_.cancellations;
      }
}
bool Operations::Settled() const noexcept {
  for (const auto *records : {&sent_, &received_})
    for (const auto &[key, v] : *records)
      if (!Final(v.stage) || !v.receipt)
        return false;
  return true;
}
bool Operations::Pending() const noexcept {
  return std::any_of(sent_.begin(), sent_.end(),
                     [](const auto &v) { return !Final(v.second.stage); });
}
std::uint64_t Operations::NextWake(std::uint64_t now) const noexcept {
  auto next = now + 1000;
  for (const auto *records : {&sent_, &received_})
    for (const auto &[key, v] : *records)
      if (!Final(v.stage) || !v.receipt)
        next = (std::min)(next, (std::min)(v.due, v.progress + config_.progress_timeout_ms));
  return (std::max)(now, next);
}

// 单独的纯值编码线程最多处理一个任务；待执行同范围只保留最新版本。
// detached线程持有自己的State，Stop/析构不等待压缩，也不访问游戏或socket。
struct EncodingCache::State {
  struct Entry {
    memory::Lease memory;
    // 已编码缓存只保留弱身份，报文不再需要旧版本的完整DOM；编码期间source才持强引用。
    std::shared_ptr<const state::HostStateBytes> state_source;
    std::weak_ptr<const state::HostStateBytes> state_identity;
    s::Kind state_kind=s::Kind::Error;
    s::Lane state_lane=s::Lane::Control;
    std::uint64_t state_sequence=0;
    s::SharedBlob blob;
    r::Limits limits;
    bool compressed = false, working = false;
    std::uint64_t queued_us=s::MetricMicros();
    std::array<char, 128> error{};
  };
  std::mutex mutex;
  std::condition_variable wake;
  s::Metrics metrics;
  bool stop = false, started = false;
  std::deque<std::shared_ptr<Entry>> entries;
  void Run() noexcept {
    for (;;) {
      std::shared_ptr<Entry> work;
      {
        std::unique_lock lock(mutex);
        wake.wait(lock, [&] {
          return stop || std::any_of(entries.begin(), entries.end(), [](const auto &e) {
                   return !e->working && !e->blob && !e->error[0];
                 });
        });
        if (stop) {
          entries.clear();
          return;
        }
        auto i = std::find_if(entries.begin(), entries.end(), [](const auto &e) {
          return !e->working && !e->blob && !e->error[0] && s::Reliable(e->state_lane);
        });
        if (i == entries.end())
          i = std::find_if(entries.begin(), entries.end(),
                           [](const auto &e) { return !e->working && !e->blob && !e->error[0]; });
        work = *i;
        work->working = true;metrics.duration[unsigned(s::CpuStage::Queue)].Observe(s::MetricMicros()-work->queued_us);
      }
      s::SharedBlob blob;
      std::array<char, 128> error{};
      for (unsigned attempt = 0; attempt < 129; ++attempt)
        try {
          s::Message m;
          m.kind = work->state_kind;
          m.sequence = work->state_sequence;
          m.connection = m.route = 1;
          m.state_body=work->state_source;m.state_lane=work->state_lane;
          blob = s::EncodeBlob(m, work->compressed, work->limits);
          break;
        } catch (const std::exception &e) {
          if (!std::strcmp(e.what(), "replication_pool_exhausted")) {
            std::lock_guard lock(mutex);
            auto old = std::find_if(entries.begin(), entries.end(), [&](const auto &candidate) {
              return candidate != work && candidate->blob && candidate->blob.use_count() == 1;
            });
            if (old != entries.end()) {
              entries.erase(old);
              continue;
            }
          }
          // 保留实际失败范围和字节数，区分冷目标与快范围预算，不靠提高上限排错。
          try{
            auto detail=std::string(e.what())+":kind="+std::to_string(unsigned(work->state_kind));
            try{detail+="/bytes="+std::to_string(work->state_source->bytes.size()+40);}catch(...){}
            detail+="/limit="+std::to_string(work->limits.bytes);
            std::strncpy(error.data(),detail.c_str(),error.size()-1);
          }catch(...){std::strncpy(error.data(), e.what(), error.size() - 1);}
          break;
        } catch (...) {
          std::strncpy(error.data(), "publication_encoding_exception", error.size() - 1);
          break;
        }
      {
        std::lock_guard lock(mutex);
        if(!blob&&!error[0])std::strncpy(error.data(),"publication_encoding_attempts_exhausted",error.size()-1);
        if(blob){metrics.Bind(blob->world);metrics.encoded_bytes+=blob->decoded_bytes;metrics.compressed_bytes+=blob->bytes.size();const auto lane=Lane(blob->lane);metrics.lane_encoded[lane]+=blob->decoded_bytes;metrics.lane_compressed[lane]+=blob->bytes.size();metrics.duration[unsigned(s::CpuStage::Encode)].Observe(blob->encode_us);if(blob->encode_cpu_us!=UINT64_MAX)metrics.cpu[unsigned(s::CpuStage::Encode)].Observe(blob->encode_cpu_us);else ++metrics.missing;if(work->compressed){metrics.duration[unsigned(s::CpuStage::Compress)].Observe(blob->compression_us);if(blob->compression_cpu_us!=UINT64_MAX)metrics.cpu[unsigned(s::CpuStage::Compress)].Observe(blob->compression_cpu_us);else ++metrics.missing;}}
        metrics.Queue(entries.size());
        work->blob = std::move(blob);
        work->error = std::move(error);
        work->working = false;
        work->state_source.reset();
      }
    }
  }
};
EncodingCache::EncodingCache() : state_(std::make_shared<State>()) {}
EncodingCache::~EncodingCache() { Stop(); }
void EncodingCache::Stop() noexcept {
  std::lock_guard lock(state_->mutex);
  state_->stop = true;
  state_->wake.notify_one();
}
s::Metrics EncodingCache::Statistics() const noexcept {std::unique_lock lock(state_->mutex,std::try_to_lock);if(!lock.owns_lock()){s::Metrics missing;missing.missing=1;return missing;}return state_->metrics;}
s::SharedBlob EncodingCache::Get(const s::Message &message,bool compressed,r::Limits limits){
  if(!message.state_body||!message.sequence)Fail("binary_encoding_source");auto state=state_;
  std::unique_lock lock(state->mutex,std::try_to_lock);if(!lock)return {};if(state->stop)Fail("publication_encoder_stopped");
  for(const auto &entry:state->entries)if(entry->state_identity.lock()==message.state_body&&entry->state_kind==message.kind&&entry->state_lane==message.state_lane&&
    entry->state_sequence==message.sequence&&entry->compressed==compressed&&entry->limits.bytes==limits.bytes&&entry->limits.nodes==limits.nodes&&
    entry->limits.value_bytes==limits.value_bytes&&entry->limits.depth==limits.depth&&entry->limits.string_bytes==limits.string_bytes&&
    entry->limits.datagram==limits.datagram&&entry->limits.fragments==limits.fragments){if(entry->error[0])Fail(entry->error.data());return entry->blob;}
  for(auto i=state->entries.begin();i!=state->entries.end();)if((*i)->state_sequence&&(*i)->state_identity.expired()&&!(*i)->working)i=state->entries.erase(i);else ++i;
  while(state->entries.size()>=128){auto i=std::find_if(state->entries.begin(),state->entries.end(),[](const auto &e){return !e->working&&(e->blob||e->error[0]);});
    if(i==state->entries.end())return {};state->entries.erase(i);}
  auto entry=std::make_shared<State::Entry>();entry->memory=memory::Reserve(memory::Pool::Wire,sizeof(State::Entry)+128,"binary_encode_job");
  entry->state_source=message.state_body;entry->state_identity=message.state_body;entry->state_kind=message.kind;entry->state_lane=message.state_lane;entry->state_sequence=message.sequence;
  entry->limits=limits;entry->compressed=compressed;state->entries.push_back(std::move(entry));
  if(!state->started){std::thread([state]{state->Run();}).detach();state->started=true;}state->wake.notify_one();return {};
}
} // namespace cs::net::live
