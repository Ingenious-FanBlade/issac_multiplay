#include "net/stream_session.h"
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
s::Version Version(const r::Value &v) {
  s::RangeId id;
  id.kind = s::RangeKind(r::Unsigned(v["kind"]));
  id.name.assign(v["name"].GetString(), v["name"].GetStringLength());
  if (!v["body"].IsNull()) {
    const auto &b = v["body"];
    id.body = {r::Unsigned(b["run"]), r::Unsigned(b["id"]), unsigned(r::Unsigned(b["space"]))};
  }
  return {id, r::Unsigned(v["revision"])};
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
  return k == s::Kind::ContextReady || k == s::Kind::AppliedAck || k == s::Kind::ScenePrepare ||
         k == s::Kind::Resync || k == s::Kind::Cancel || k == s::Kind::Retire ||
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
    m.memory = memory::Reserve(memory::Pool::Values, 8192);
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
    s::Range source;
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
          return !e->working && !e->blob && !e->error[0] && unsigned(e->source->id.kind) <= 4;
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
          m.kind = s::Kind::Range;
          m.sequence = work->source->revision;
          m.connection = m.route = 1;
          m.range = work->source;
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
          std::strncpy(error.data(), e.what(), error.size() - 1);
          break;
        } catch (...) {
          std::strncpy(error.data(), "publication_encoding_exception", error.size() - 1);
          break;
        }
      {
        std::lock_guard lock(mutex);
        if(blob){metrics.Bind(work->source->world);metrics.encoded_bytes+=blob->decoded_bytes;metrics.compressed_bytes+=blob->bytes.size();const auto lane=Lane(blob->lane);metrics.lane_encoded[lane]+=blob->decoded_bytes;metrics.lane_compressed[lane]+=blob->bytes.size();metrics.duration[unsigned(s::CpuStage::Encode)].Observe(blob->encode_us);if(blob->encode_cpu_us!=UINT64_MAX)metrics.cpu[unsigned(s::CpuStage::Encode)].Observe(blob->encode_cpu_us);else ++metrics.missing;if(work->compressed){metrics.duration[unsigned(s::CpuStage::Compress)].Observe(blob->compression_us);if(blob->compression_cpu_us!=UINT64_MAX)metrics.cpu[unsigned(s::CpuStage::Compress)].Observe(blob->compression_cpu_us);else ++metrics.missing;}}
        metrics.Queue(entries.size());
        work->blob = std::move(blob);
        work->error = std::move(error);
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
s::SharedBlob EncodingCache::Get(s::Range range, bool compressed, r::Limits limits) {
  if (!range)
    Fail("publication_missing_range");
  auto state = state_;
  std::lock_guard lock(state->mutex);
  if (state->stop)
    Fail("publication_encoder_stopped");
  for (auto &entry : state->entries)
    if (entry->source == range && entry->compressed == compressed &&
        entry->limits.bytes == limits.bytes && entry->limits.nodes == limits.nodes &&
        entry->limits.value_bytes == limits.value_bytes && entry->limits.depth == limits.depth &&
        entry->limits.string_bytes == limits.string_bytes &&
        entry->limits.datagram == limits.datagram && entry->limits.fragments == limits.fragments) {
      if (entry->error[0])
        Fail(entry->error.data());
      return entry->blob;
    }
  // 尚未编码的动态可以被同范围完整新值替换；可靠冷依赖与正在编码项不覆盖。
  for (auto i = state->entries.begin(); i != state->entries.end();) {
    const auto &e = *i;
    if (unsigned(e->source->id.kind) >= 5 && unsigned(range->id.kind) >= 5 && !e->working &&
        e->source->world == range->world && e->source->id == range->id)
      i = state->entries.erase(i);
    else
      ++i;
  }
  while (state->entries.size() >= 128) {
    auto i = std::find_if(state->entries.begin(), state->entries.end(),
                          [](const auto &e) { return e->blob || e->error[0]; });
    if (i == state->entries.end())
      Fail("publication_encoding_capacity");
    state->entries.erase(i);
  }
  auto entry = std::make_shared<State::Entry>();
  entry->memory = memory::Reserve(memory::Pool::Wire, sizeof(State::Entry) + 128);
  entry->source = std::move(range);
  entry->limits = limits;
  entry->compressed = compressed;
  state->entries.push_back(entry);
  if (!state->started) {
    std::thread([state] { state->Run(); }).detach();
    state->started = true;
  }
  state->wake.notify_one();
  return {};
}
void PublicationSender::AuthorityPaused(bool value,std::uint64_t now) noexcept {
  if(value&&!paused_)pause_started_=now;
  if(!value&&paused_){const auto elapsed=now>=pause_started_?now-pause_started_:0;for(auto &[id,at]:convergence_since_)at+=elapsed;}
  paused_=value;
}
void PublicationSender::Offer(std::shared_ptr<const s::Publication> baseline,
                              std::shared_ptr<const s::Publication> latest,
                              std::vector<std::shared_ptr<const s::Publication>> bodies) {
  if (!latest) {
    Stop();
    return;
  }
  if (!baseline_ || baseline_->world != latest->world ||
      (phase_ == Phase::Recovering && baseline && baseline->manifest && baseline_->manifest &&
       baseline->manifest->revision != baseline_->manifest->revision)) {
    Stop();
    phase_ = Phase::WaitingWorld;
    baseline_ = std::move(baseline);
  }
  // 有效冷目标在途时固定；最新普通/逐身体候选只替换尚未执行的完整集合。
  latest_ = std::move(latest);
  bodies_ = std::move(bodies);
  std::set<s::RangeId> keep;
  for (const auto &range : latest_->ranges)
    keep.insert(range->id);
  if (baseline_)
    for (const auto &range : baseline_->ranges)
      keep.insert(range->id);
  for (const auto &body : bodies_)
    if (body && body->player_fast)
      keep.insert(body->player_fast->id);
  for (const auto &[id, work] : fast_)
    keep.insert(id);
  for (const auto &[version, transfer] : pending_context_)
    keep.insert(version.id);
  const auto trim = [&](auto &values) {
    for (auto i = values.begin(); i != values.end();)
      if (!keep.contains(i->first))
        i = values.erase(i);
      else
        ++i;
  };
  trim(sent_history_);
  trim(sent_at_);
  trim(queued_);
  trim(applied_);
  trim(fully_sent_);
  trim(convergence_since_);
  if (bodies_.size() > 64)
    Fail("publication_body_capacity");
}
void PublicationSender::Authorize(s::Route route, std::uint64_t now) {
  if (!baseline_ || !baseline_->manifest || baseline_->world != route.scope || !route.world)
    Fail("publication_world_not_prepared");
  started_ = progress_ = now;
  native_progress_ = 0;
  route_ = route;
  reliable_.Authorize(route);
  reliable_.GrantRate(route_.config.borrow_initial_lanes?route_.config.peer_bytes_per_second-route_.config.control_bytes_per_second:route_.config.reliable_bytes_per_second);
  metadata_ = memory::Reserve(memory::Pool::Wire, 512ull << 10);
  cold_ = baseline_->ranges;
  cold_cursor_ = 0;
  queued_.clear();
  applied_.clear();
  transfers_.clear();
  pending_context_.clear();
  context_since_.clear();
  fully_sent_.clear();
  sent_history_.clear();
  sent_facts_.clear();confirmed_.clear();
  fast_ = {};
  sent_at_.clear();
  fast_cursor_ = {};
  convergence_since_.clear();stale_=false;active_at_=now;applied_world_tick_=0;
  metrics_.Bind(route.scope,route.epoch);metrics_.first_prepare_ms=prepare_started_?prepare_started_:now;
  phase_ = Phase::Baseline;
}
void PublicationSender::Remember(const s::Version &version) {
  if (!sent_history_.contains(version.id) && sent_history_.size() >= 128)
    Fail("publication_version_scope_capacity");
  auto &history = sent_history_[version.id];
  if (std::find(history.begin(), history.end(), version.revision) != history.end())
    return;
  if (history.size() == 128) {
    sent_facts_.erase({version.id,history.front()});confirmed_.erase({version.id,history.front()});
    history.pop_front();
  }
  history.push_back(version.revision);
}
void PublicationSender::Encoded(const s::Blob &b){if(metrics_.Note(b.range_id,b.sequence,b.sample,s::FlowStage::Encode,s::MetricMicros()/1000)){metrics_.encoded_bytes+=b.decoded_bytes;metrics_.compressed_bytes+=b.bytes.size();const auto lane=Lane(b.lane);metrics_.lane_encoded[lane]+=b.decoded_bytes;metrics_.lane_compressed[lane]+=b.bytes.size();}}
void PublicationSender::Track(const s::Range &r){const s::Version version{r->id,r->revision};if(sent_facts_.contains(version))return;metrics_.Note(*r,s::FlowStage::Publish,s::MetricMicros()/1000);
  if(sent_facts_.size()>=8192)Fail("publication_sent_fact_capacity");SentFact f;f.dependencies=r->dependencies;f.sample=r->sample;
  if(r->id.kind==s::RangeKind::Manifest){f.dependencies=r->required;f.native=r->native_domains;f.retained=r->retained_domains;f.watermark=r->event_watermark;}
  else if(r->id.kind==s::RangeKind::WorldDynamic)f.native=baseline_->manifest->native_domains;
  else if(r->id.kind==s::RangeKind::PlayerFast)f.native=1ull<<6;
  sent_facts_.emplace(version,std::move(f));if(metadata_){std::uint64_t bytes=512ull<<10;for(const auto &[v,fact]:sent_facts_)bytes+=sizeof(v)+sizeof(fact)+v.id.name.capacity()+fact.dependencies.size()*256;metadata_->Resize(bytes);}}
bool PublicationSender::WasSent(const s::Version &version) const noexcept {
  auto i = sent_history_.find(version.id);
  return i != sent_history_.end() &&
         std::find(i->second.begin(), i->second.end(), version.revision) != i->second.end();
}
bool PublicationSender::ContextReady(const s::Message &m, std::uint64_t now) {
  if (!baseline_ || m.connection != route_.connection || m.route != route_.epoch)
    return false;
  const auto &w = m.body["world"];
  const s::WorldScope world{r::Unsigned(w["session"]), r::Unsigned(w["run"]),
                            r::Unsigned(w["floor"]),   r::Unsigned(w["dimension"]),
                            r::Unsigned(w["room"]),    r::Unsigned(w["scene"])};
  if (world != route_.scope)
    return false;
  std::vector<s::Version> versions;
  for (const auto &v : m.body["versions"].GetArray())
    versions.push_back(Version(v));
  // 收齐不是主线程接纳；先检查整组精确版本，之后才统一释放至多两个信用。
  for (const auto &version : versions) {
    auto pending = pending_context_.find(version);
    if (!WasSent(version) &&
        (pending == pending_context_.end() || !reliable_.AllSent(pending->second)))
      return false;
  }
  for (const auto &version : versions) {
    auto i = pending_context_.find(version);
    if (i == pending_context_.end())
      continue;
    if (reliable_.AllSent(i->second))
      reliable_.Confirm(i->second);
    Remember(version);
    fully_sent_[version.id] = version.revision;
    pending_context_.erase(i);
    context_since_.erase(version);
    progress_ = now;
  }
  return true;
}
bool PublicationSender::Applied(const s::Message &m,std::uint64_t now) {
  const auto reject=[&](const char *reason){if(failure_.empty())failure_=reason;return false;};
  if ((phase_!=Phase::Baseline&&phase_!=Phase::WaitingApplied&&phase_!=Phase::Active)||!baseline_ || m.kind != s::Kind::AppliedAck || m.connection != route_.connection ||
      m.route != route_.epoch)
    return reject("applied_ack_route");
  const auto &w = m.body["world"];
  const s::WorldScope world{r::Unsigned(w["session"]), r::Unsigned(w["run"]),
                            r::Unsigned(w["floor"]),   r::Unsigned(w["dimension"]),
                            r::Unsigned(w["room"]),    r::Unsigned(w["scene"])};
  if (world != route_.scope)
    return reject("applied_ack_world");
  // 先复核整组不可变提交事实；任何一项失败都不推进其它版本或释放内容。
  std::vector<std::pair<s::Version,NativeFact>> facts;
  for(const auto &result:m.body["results"].GetArray()){const auto version=Version(result["version"]);auto sent=sent_facts_.find(version);auto transfer=transfers_.find(version);
    if(sent==sent_facts_.end()||(!WasSent(version)&&(transfer==transfers_.end()||!reliable_.AllSent(transfer->second))))return reject("applied_ack_unsent_version");
    const auto &expected=sent->second;if(r::Unsigned(result["native_domains"])!=expected.native||r::Unsigned(result["retained_domains"])!=expected.retained||r::Unsigned(result["event_watermark"])!=expected.watermark)return reject("applied_ack_precise_range_or_watermark");
    NativeFact fact{r::Unsigned(result["attempt"]),r::Unsigned(result["instance"]),r::Unsigned(result["resource"])};auto known=confirmed_.find(version);
    if(known!=confirmed_.end()){if(known->second!=fact)return reject("applied_ack_committed_fact_conflict");}
    else if(applied_.contains(version.id)&&applied_.at(version.id)>=version.revision)return reject("applied_ack_version_regression");
    // 依赖必须确曾送给此Peer；收到一个任意较新版本不能证明旧依赖已发送。
    for(const auto &d:expected.dependencies){auto t=transfers_.find(d);if(!WasSent(d)&&(t==transfers_.end()||!reliable_.AllSent(t->second)))return reject("applied_ack_unsent_dependency");}
    facts.emplace_back(version,fact);
  }
  if (phase_ == Phase::Active) {
    if ((r::Unsigned(m.body["native_domains"]) & ~baseline_->manifest->native_domains) ||
        (r::Unsigned(m.body["retained_domains"]) & ~baseline_->manifest->retained_domains))
      return false;
    std::vector<s::Version> versions;
    for (const auto &v : m.body["versions"].GetArray()) {
      auto version = Version(v);
      auto i = transfers_.find(version);
      if (!WasSent(version) && (i == transfers_.end() || !reliable_.AllSent(i->second)))
        return false;
      versions.push_back(std::move(version));
    }
    for (const auto &version : versions) {
      auto i = transfers_.find(version);
      if (i != transfers_.end() && reliable_.AllSent(i->second))
        reliable_.Confirm(i->second);
      Remember(version);
      pending_context_.erase(version);
      context_since_.erase(version);
      if(version.revision>applied_[version.id])convergence_since_.erase(version.id);
      applied_[version.id] = (std::max)(applied_[version.id], version.revision);
      if(version.id.kind==s::RangeKind::WorldDynamic)applied_world_tick_=(std::max)(applied_world_tick_,sent_facts_.at(version).sample.tick);
    }
    for(const auto &[v,f]:facts){confirmed_.insert_or_assign(v,f);auto sent=sent_facts_.find(v);if(sent!=sent_facts_.end()){metrics_.Note(v.id,v.revision,sent->second.sample,s::FlowStage::Ack,now);}}
    metrics_.display_ready_ms=now;return true;
  }
  bool manifest = false;
  for (const auto &v : m.body["versions"].GetArray()) {
    const auto kind = s::RangeKind(r::Unsigned(v["kind"]));
    const auto revision = r::Unsigned(v["revision"]);
    if (kind == s::RangeKind::Manifest && revision == baseline_->manifest->revision &&
        std::string(v["name"].GetString()) == baseline_->manifest->id.name)
      manifest = true;
  }
  // 只有已发送完整冷闭包与精确域位匹配的真实主线程ACK，才结束初建。
  if (!manifest || cold_cursor_ != cold_.size() ||
      r::Unsigned(m.body["native_domains"]) != baseline_->manifest->native_domains ||
      r::Unsigned(m.body["retained_domains"]) != baseline_->manifest->retained_domains ||
      r::Unsigned(m.body["event_watermark"]) != baseline_->manifest->event_watermark)
    return false;
  for (const auto &range : cold_) {
    const s::Version version{range->id, range->revision};
    auto i = transfers_.find(version);
    if (!WasSent(version) && (i == transfers_.end() || !reliable_.AllSent(i->second)))
      return false;
  }
  for (const auto &[version, id] : transfers_)
    if (reliable_.AllSent(id))
      reliable_.Confirm(id);
  pending_context_.clear();
  context_since_.clear();
  metrics_.display_ready_ms=now;
  phase_ = Phase::Active;
  active_at_=now;convergence_since_.clear();if(baseline_->world_dynamic)applied_world_tick_=baseline_->world_dynamic->sample.tick;
  for (const auto &range : baseline_->ranges) {
    Remember({range->id, range->revision});
    applied_[range->id] = range->revision;
  }
  for(const auto &[v,f]:facts){confirmed_.insert_or_assign(v,f);metrics_.Note(v.id,v.revision,sent_facts_.at(v).sample,s::FlowStage::Ack,now);}
  reliable_.GrantRate(route_.config.reliable_bytes_per_second);
  return true;
}
bool PublicationSender::Acknowledge(const s::Message &m, std::uint64_t now) {
  const auto transfer = r::Unsigned(m.body["transfer"]);
  const auto changed = reliable_.Acknowledge(m, now);
  if (changed) {
    progress_ = now;
    for (const auto &[version, id] : pending_context_)
      if (id == transfer)
        context_since_[version] = now;
  }
  return changed;
}
bool PublicationSender::NativeProgress(const s::Message &m, std::uint64_t now) {
  if (!baseline_ || !baseline_->manifest || m.connection != route_.connection ||
      m.route != route_.epoch || r::Unsigned(m.body["operation"]) != unsigned(s::Kind::Range) ||
      r::Unsigned(m.body["operation_sequence"]) != baseline_->manifest->revision ||
      r::Unsigned(m.body["stage"]) != unsigned(OperationStage::Executing))
    return false;
  const s::Version version{baseline_->manifest->id, baseline_->manifest->revision};
  auto i = transfers_.find(version);
  if (!WasSent(version) && (i == transfers_.end() || !reliable_.AllSent(i->second)))
    return false;
  const auto step = r::Unsigned(m.body["progress_sequence"]);
  if (step > native_progress_) {
    native_progress_ = step;
    progress_ = now;
  }
  return true;
}
void PublicationSender::FailPublication(const char *reason) {
  if (failure_.empty())
    failure_ = reason;
  phase_ = Phase::Recovering;
  reliable_.Authorize({});
  fast_.clear();
}
void PublicationSender::Stop() noexcept {
  phase_ = Phase::Stopped;
  reliable_.Authorize({});
  baseline_.reset();
  latest_.reset();
  metadata_.reset();
  bodies_.clear();
  cold_.clear();
  sent_facts_.clear();confirmed_.clear();
  cold_cursor_ = 0;
  queued_.clear();
  applied_.clear();
  transfers_.clear();
  pending_context_.clear();
  context_since_.clear();
  fully_sent_.clear();
  sent_history_.clear();
  fast_ = {};
  sent_at_.clear();
  convergence_since_.clear();stale_=false;
  fast_cursor_ = {};
}
unsigned PublicationSender::Pump(std::uint64_t now, std::uint64_t &transfer, EncodingCache &cache,
                                 const s::ReliableSender::Send &send) {
  s::Measure network_measure(metrics_,s::CpuStage::Network);
  if (phase_ == Phase::Stopped || phase_ == Phase::WaitingWorld || phase_ == Phase::Preparing ||
      !latest_)
    return 0;
  const auto next = [&] {
    if (transfer == UINT64_MAX - 1)
      Fail("publication_transfer_exhausted");
    return ++transfer;
  };
  s::ReliableSender::Result result;
  while (reliable_.Poll(result)) {
    if (!result.complete) {
      if(failure_.empty())failure_ = result.reason;
      phase_ = Phase::Recovering;
      return 0;
    }
    for (auto i = transfers_.begin(); i != transfers_.end();)
      if (i->second == result.transfer) {
        fully_sent_[i->first.id] = i->first.revision;
        Remember(i->first);
        if(auto fact=sent_facts_.find(i->first);fact!=sent_facts_.end())metrics_.Note(i->first.id,i->first.revision,fact->second.sample,s::FlowStage::Send,now);
        if (pending_context_.contains(i->first))
          context_since_[i->first] = now;
        i = transfers_.erase(i);
      } else
        ++i;
  }
  if (phase_ == Phase::Recovering)
    return 0;
  if ((phase_ != Phase::Active && (now < started_ || now - started_ >= route_.config.absolute_timeout_ms || now < progress_ ||
                                   now - progress_ >= route_.config.progress_timeout_ms)) ||
      std::any_of(context_since_.begin(), context_since_.end(),
                  [&](const auto &v) { return now < v.second || now - v.second >= route_.config.progress_timeout_ms; })) {
    FailPublication("publication_no_progress_or_deadline");
    return 0;
  }
  // 两个可靠活动槽：只移动游标，不为新普通帧撤销正在发送的Baseline。
  while (cold_cursor_ < cold_.size() && reliable_.Active() < 2 && pending_context_.size() < 2) {
    const auto range = cold_[cold_cursor_];
    if (range->id.kind == s::RangeKind::WorldDynamic ||
        range->id.kind == s::RangeKind::PlayerFast) {
      ++cold_cursor_;
      continue;
    }
    auto blob = cache.Get(range, route_.zstd, route_.world_limits);
    if (!blob)
      break;
    const auto id = next();
    Track(range);Encoded(*blob);
    reliable_.Start(std::move(blob), id, now);
    transfers_[{range->id, range->revision}] = id;
    pending_context_[{range->id, range->revision}] = id;
    context_since_[{range->id, range->revision}] = now;
    queued_[range->id] = range->revision;
    ++cold_cursor_;
  }
  unsigned sent = reliable_.Pump(now, 4, send);
  for(const auto &[version,id]:transfers_)if(reliable_.AllSent(id)){const auto fact=sent_facts_.find(version);if(fact!=sent_facts_.end())metrics_.Note(version.id,version.revision,fact->second.sample,s::FlowStage::Send,now);}
  if (phase_ == Phase::Baseline && cold_cursor_ == cold_.size() && reliable_.Empty())
    phase_ = Phase::WaitingApplied;
  // 每个完整范围各保留一个在途发送游标及来源中的最新候选；按范围轮转，
  // 大WorldDynamic不能挡住PlayerFast；未确认的最后静止后态每100ms补发。
  std::vector<s::Range> ranges;
  if (phase_ == Phase::Active) {
    for (const auto &range : latest_->ranges)
      if (range->id.kind == s::RangeKind::Context || range->id.kind == s::RangeKind::Structure ||
          range->id.kind == s::RangeKind::Resource) {
        if (queued_[range->id] != range->revision && reliable_.Active() < 2 &&
            pending_context_.size() < 2) {
          auto blob = cache.Get(range, route_.zstd, route_.world_limits);
          if (blob) {
            const auto id = next();
            Track(range);Encoded(*blob);
            reliable_.Start(std::move(blob), id, now);
            transfers_[{range->id, range->revision}] = id;
            pending_context_[{range->id, range->revision}] = id;
            context_since_[{range->id, range->revision}] = now;
            queued_[range->id] = range->revision;
          }
        }
      }
    if (latest_->world_dynamic)
      ranges.push_back(latest_->world_dynamic);
    for (const auto &body : bodies_)
      if (body && body->player_fast) {
        // 普通完整Player域已经实际提交时，旧快源不再需要发送；不伪造Fast ACK。
        if(body->player_fast->sample.tick>applied_world_tick_)ranges.push_back(body->player_fast);
        else convergence_since_.erase(body->player_fast->id);
      }
  } else if (baseline_->world_dynamic)
    ranges.push_back(baseline_->world_dynamic);
  stale_=false;
  for(const auto &range:ranges) {
    if(applied_[range->id]>=range->revision){convergence_since_.erase(range->id);continue;}
    auto [wait,inserted]=convergence_since_.try_emplace(range->id,now);
#ifdef ISAACCS_E2E
    // 显式、有时限的画面审阅段不当作真实慢端；采样/发送/接收仍正常并单列区间。
    if(s::E2EVisualNeutral()){wait->second=now;continue;}
#endif
    if(!paused_&&now>=wait->second&&now-wait->second>=route_.config.stale_ms)stale_=true;
    if(!paused_&&now>=wait->second&&now-wait->second>=route_.config.recovery_ms){FailPublication("publication_not_converged");return sent;}
  }
  metrics_.candidates=fast_.size()+pending_context_.size();metrics_.retained=sent_facts_.size();
  for (const auto &range : ranges)
    if (!fast_.contains(range->id) &&
        (queued_[range->id] != range->revision || !sent_at_.contains(range->id) ||
         (applied_[range->id]<range->revision&&now - sent_at_[range->id] >= route_.config.final_retry_ms))) {
      auto blob = cache.Get(range, false, route_.fast_limits);
      if (blob) {
        Track(range);Encoded(*blob);
        if(queued_[range->id]==range->revision&&sent_at_.contains(range->id))++metrics_.repeated_range_transfers;
        if (fast_.size() >= 65)
          Fail("publication_fast_capacity");
        fast_.emplace(range->id, Fast{std::move(blob)});
        queued_[range->id] = range->revision;
      }
    }
  unsigned remaining = unsigned(fast_.size()), fast_sent = 0;
  auto i = fast_.upper_bound(fast_cursor_);
  while (remaining-- && fast_sent < 4 && !fast_.empty()) {
    if (i == fast_.end())
      i = fast_.begin();
    auto current = i++;
    auto &work = current->second;
    fast_cursor_ = current->first;
    if (!work.transfer)
      work.transfer = next();
    auto lease = memory::Reserve(memory::Pool::Wire, 1200);
    const auto packet = s::Datagram(*work.blob, {route_.connection, route_.epoch, work.transfer},
                                    work.part, route_.fast_limits);
    const auto result = send(packet);
    if (result == s::SendResult::WouldBlock)
      continue;
    if (result == s::SendResult::Failed)
      Fail("publication_socket_send_failed");
    ++sent;
    ++fast_sent;
    if (++work.part == (work.blob->bytes.size() + 1135) / 1136) {
      fully_sent_[current->first] = work.blob->sequence;
      Remember({current->first, work.blob->sequence});
      sent_at_[current->first] = now;
      metrics_.Note(current->first,work.blob->sequence,work.blob->sample,s::FlowStage::Send,now);
      fast_.erase(current);
    }
  }
  return sent;
}
} // namespace cs::net::live
