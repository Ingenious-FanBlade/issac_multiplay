// Winsock必须先于Windows；网络线程只读取自有纯值。
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include "net/live.h"
#include "net/mod_manifest_wire.h"
#include "net/timing_clock.h"
#include "core/lobby.h"
#include "replica/stream_e2e.h"
#include "net/queue.h"
#include "replica/stream_clock_e2e.h"
#include "state/budget.h"
#include "state/client_validation.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <optional>

namespace cs::net::live {
namespace r = replica;
namespace s = replica::stream;
namespace {
[[noreturn]] void Fail(const char *reason) { throw r::ContractError(reason); }
const r::Value &At(const r::Value &v, const char *key) {
  if (!v.IsObject() || !v.HasMember(key))
    Fail("live_missing_field");
  return v[key];
}
std::uint64_t N(const r::Value &v, const char *key) { return r::Unsigned(At(v, key)); }
void N(r::Value &v, const char *key, std::uint64_t n, r::Allocator &a) {
  r::Put(v, key, r::Decimal(n, a), a);
}
std::string String(const r::Value &v) {
  if (!v.IsString())
    Fail("live_string");
  return {v.GetString(), v.GetStringLength()};
}
void Object(r::Value &v, const char *key, const r::Value &source, r::Allocator &a) {
  r::Value owned;
  owned.CopyFrom(source, a);
  r::Put(v, key, std::move(owned), a);
}
void Reason(Event &e, const char *reason) noexcept { strncpy_s(e.reason, reason, _TRUNCATE); }
s::Message Make(s::Kind kind, std::uint64_t sequence, std::uint64_t connection,
                std::uint64_t route) {
  s::Message m;
  m.memory = memory::Reserve(memory::Pool::Values, 8192, "live_control");
  m.kind = kind;
  m.sequence = sequence;
  m.connection = connection;
  m.route = route;
  m.body.SetObject();
  return m;
}
std::uint64_t Next(std::uint64_t &n) {
  if (n >= UINT64_MAX - 1)
    Fail("live_sequence_exhausted");
  return ++n;
}
r::Json Capabilities(const s::RuntimeConfig &config,bool binary_host=false,bool binary_client=false,bool lobby=false) {
  auto caps = s::Capabilities();
  // 真实DLL声明已接线的具名owner能力；纯模型Capabilities仍不冒充原生Client。
  auto &a=caps.GetAllocator();
  caps["required"].PushBack(r::Value(kModManifestCapability,a),a);
  if(!lobby) {
    auto &supported=caps["supported"];
    const auto it=std::find_if(supported.Begin(),supported.End(),[](const auto &item){return item==s::kLobbyCapability;});
    if(it!=supported.End())supported.Erase(it);
  }
  const auto binary=std::string("binary-state-v1-layout-")+std::to_string(state::LayoutVersion());
  // Host要求现行整数布局；Client完整接收／安装接通之前不声明此能力。
  if(binary_host)caps["required"].PushBack(r::Value(binary.c_str(),a),a);
  else if(!binary_client)for(auto i=caps["supported"].Begin();i!=caps["supported"].End();)if(i->IsString()&&binary==i->GetString())i=caps["supported"].Erase(i);else ++i;
  if(binary_host||binary_client)caps["supported"].PushBack(r::Value(s::kStateTableDeltaCapability,a),a);
  for(const char *name:{"client-scene-entity-v1","client-scene-grid-v1","client-scene-room-v1","client-scene-player-v2"})caps["native_appliers"].PushBack(r::Value(name,a),a);
  auto &l = caps["limits"];
  const auto local = Limits();
  l["bytes"].SetUint(local.bytes);
  l["datagram"].SetUint(local.datagram);
  l["candidates"].SetUint(local.candidates);
  l["fragments"].SetUint(local.fragments);
  l["timeout_ms"].SetUint(local.timeout_ms);
  const auto world=s::ConfiguredLimits(config);
  auto &w=caps["world_limits"];w["bytes"].SetUint(world.bytes);w["nodes"].SetUint(world.nodes);w["fragments"].SetUint(world.fragments);w["candidates"].SetUint(world.candidates);w["value_bytes"].SetUint64(world.value_bytes);
  auto &f=caps["fast_limits"];f["bytes"].SetUint64(config.fast_message_bytes);f["nodes"].SetUint64(config.fast_nodes);f["candidates"].SetUint64(config.dynamic_candidates);
  return caps;
}
r::Limits Agreed(const r::Value &v) {
  auto l = Limits();
  for(const char *name:{"client-scene-entity-v1","client-scene-grid-v1","client-scene-room-v1","client-scene-player-v2"}){bool found=false;for(const auto &applier:At(v,"native_appliers").GetArray())if(applier.IsString()&&String(applier)==name)found=true;if(!found)Fail("live_native_applier_capability");}
  const auto &a = v["limits"];
  if (N(a, "datagram") != l.datagram || N(a, "bytes") < l.bytes || N(a, "fragments") < l.fragments)
    Fail("live_transport_budget");
  l.candidates = unsigned((std::min)(std::uint64_t(l.candidates), N(a, "candidates")));
  return l;
}
bool Zstd(const r::Value &agreed) {
  for (const auto &v : agreed["compression"].GetArray())
    if (String(v) == "zstd-v1")
      return true;
  return false;
}
bool Supports(const r::Value &agreed,const char *name) {
  for(const auto &feature:agreed["supported"].GetArray())
    if(String(feature)==name)return true;
  return false;
}
bool WorldBodyZstd(const r::Value &agreed) {return Zstd(agreed)&&Supports(agreed,s::kWorldBodyZstdCapability);}
s::WorldScope ReadWorld(const r::Value &v) {
  return {N(v, "session"),   N(v, "run"),  N(v, "floor"),
          N(v, "dimension"), N(v, "room"), N(v, "scene")};
}
void PutWorld(r::Value &v, const s::WorldScope &w, r::Allocator &a) {
  r::Value world(rapidjson::kObjectType);
  N(world, "session", w.session, a);
  N(world, "run", w.run, a);
  N(world, "floor", w.floor, a);
  N(world, "dimension", w.dimension, a);
  N(world, "room", w.room, a);
  N(world, "scene", w.scene, a);
  r::Put(v, "world", std::move(world), a);
}
// 名单与高频时序独立；processed=0合法，不虚构启动ACK。
s::Message Roster(const protocol::Ack &p, unsigned ordinal, std::uint64_t sequence,
                  const s::Route &route, std::uint64_t echo, std::uint64_t received) {
  auto m = Make(s::Kind::Roster, sequence, route.connection, route.epoch);
  auto &v = m.body;
  auto &a = v.GetAllocator();
  N(v, "eligibility_revision", p.eligibility_revision, a);
  N(v, "server_tick", p.server_tick, a);
  N(v, "participant", ordinal, a);
  N(v, "accepted", p.accepted, a);
  N(v, "processed", p.processed, a);
  N(v, "processed_tick", p.processed_tick, a);
  N(v, "processed_source", p.processed_source, a);
  N(v, "generation", p.generation, a);
  N(v, "state", unsigned(p.session_state), a);
  N(v, "roster", p.roster_mask, a);
  N(v, "rejected", p.rejected_count, a);
  N(v, "rejection_mask", p.rejection_mask, a);
  r::Put(v, "input_enabled", r::Value(p.input_enabled), a);
  N(v, "server_sample_ms", p.server_sample_ms, a);
  N(v, "server_send_ms", TimingClockMs(), a);  // 输入时序链：与 server_sample_ms 同钟
  N(v, "echo_client_ms", echo, a);
  N(v, "echo_server_ms", received, a);
  return m;
}
void Timing(Event &e, const r::Value &v, const char *sample, const char *sent) {
  // 2026-11-20 起本链各时刻改用 TimingClockMs（net/timing_clock.h，毫秒截断误差 <1ms），
  // 原 GetTickCount64 以 10—16ms 量化。矛盾拒绝阈值仍保留原 32ms（只做健全性检查，
  // 收紧它没有收益，反而可能因两机时钟速率差误判）；不比较异机绝对时钟。
  constexpr std::uint64_t reject_tolerance_ms = 32;
  constexpr std::uint64_t quantum_ms = kTimingQuantumMs;
  const auto now = e.received_ms, send = N(v, sent), echo = N(v, "echo_client_ms"),
             received = N(v, "echo_server_ms");
  e.progress.server_sample_ms = N(v, sample);
  if (!echo || echo > now || e.progress.server_sample_ms > send || received > send ||
      ((send - received) > (now - echo) && (send - received) - (now - echo) > reject_tolerance_ms)) {
#ifdef ISAACCS_E2E
    s::E2EClockRejected(echo,now,received,send,e.progress.server_sample_ms);
#endif
    Fail("live_clock_range");
  }
  e.round_trip_ms = now - echo >= send - received ? (now - echo) - (send - received) : 0;
  e.publication_age_ms = send - e.progress.server_sample_ms;
  // RTT差的两次截断、发布区间及接收后区间各一次，合计误差上界 4×1ms=4ms（原 GetTickCount64 下为 64ms，
  // 约占输入提前量 4 tick）。不把负差夹零后当作无误差，也不伪增实测RTT用于N0网络判定。
  e.timing_uncertainty_ms = 4 * quantum_ms;
}
// Host→Client 丢包估计（只作显示）：Host 的 TickProgress 序号（HostPeer::timing_）每发一条加一、
// 跨路由不归零，按 Authority tick 约 30—60Hz 发送，暂停时按心跳发送。客机按“虚拟序号”记录最近
// kWindow 条的到达位，缺号即计为丢失；迟到的乱序包补回到达位。路由切换期间被路由过滤的包不是
// 网络丢包，Rebase 让下一条到达的序号紧接上一条虚拟序号，不把切换空档算进丢包。
// Host 侧发送背压导致的未发出也会表现为缺号，因此这是“未收到”比例，不区分链路或发送端原因。
class TickLossWindow final {
public:
  static constexpr std::uint64_t kWindow = 256;
  void Rebase() noexcept { rebase_ = true; }
  void Observe(std::uint64_t sequence) noexcept {
    if (!sequence) return;
    if (!span_ || rebase_) {
      // 首条或切换后首条：建立“真实序号→虚拟序号”的偏移，旧窗口的到达位原样保留。
      if (sequence <= floor_ && span_) return;
      const auto next = span_ ? highest_ + 1 : 1;
      offset_ = sequence - next;  // 无符号回绕相减，Virtual() 用同一偏移还原
      floor_ = sequence;
      rebase_ = false;
      Advance(next);
      return;
    }
    if (sequence < floor_) return;  // 切换前的迟到包，已不属于当前虚拟序号段
    const auto v = sequence - offset_;
    if (v > highest_) {
      Advance(v);
      return;
    }
    // 迟到或重复：仍在窗口内且原先未到达时补回。
    if (highest_ - v < span_ && !Got(v)) {
      Set(v, true);
      ++received_;
    }
  }
  std::uint64_t Span() const noexcept { return span_; }
  // 千分比，向上取整，避免少量丢失被显示成 0.0%。
  std::uint32_t LossPermille() const noexcept {
    if (!span_) return 0;
    const auto lost = span_ - received_;
    return std::uint32_t((lost * 1000 + span_ - 1) / span_);
  }

private:
  std::array<std::uint64_t, kWindow / 64> bits_{};
  std::uint64_t highest_ = 0, span_ = 0, received_ = 0, offset_ = 0, floor_ = 0;
  bool rebase_ = false;
  bool Got(std::uint64_t v) const noexcept { return (bits_[(v % kWindow) / 64] >> (v % 64)) & 1u; }
  void Set(std::uint64_t v, bool on) noexcept {
    auto &word = bits_[(v % kWindow) / 64];
    const auto mask = std::uint64_t(1) << (v % 64);
    word = on ? (word | mask) : (word & ~mask);
  }
  // 窗口前移到虚拟序号 v（v 本身已到达），中间的序号记为未到达；被挤出窗口的旧位同步扣除。
  void Advance(std::uint64_t v) noexcept {
    const auto from = span_ ? highest_ + 1 : v;
    if (v - from >= kWindow) {
      bits_ = {};
      span_ = kWindow;
      received_ = 0;
    } else {
      for (auto i = from; i < v; ++i) Push(i, false);
    }
    Push(v, true);
    highest_ = v;
  }
  void Push(std::uint64_t v, bool on) noexcept {
    if (span_ == kWindow) {
      if (Got(v)) --received_;  // 同一槽位原先存的是 v-kWindow
    } else
      ++span_;
    Set(v, on);
    if (on) ++received_;
  }
};
} // namespace
r::Limits Limits() noexcept { return s::HandshakeLimits(); }
#ifdef ISAACCS_E2E
r::Json E2EHandshakeCapabilities() { return Capabilities(s::RuntimeConfig{}); }
#endif
std::uint64_t Random() {
  std::uint64_t n = 0;
  if (BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&n), sizeof(n),
                      BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0 ||
      !n || n == UINT64_MAX)
    Fail("live_random");
  return n;
}
bool IsPacket(std::span<const std::uint8_t> b) noexcept {
  return b.size() >= 4 && !std::memcmp(b.data(), "ICR", 3) &&
         (b[3] == 'F' || b[3] == 'S' || b[3] == 'B');
}
// 每个Peer的二进制发送owner；可靠分片确认和原生AppliedAck分别驱动各自寿命。
struct HostPeer::Binary {
  state::HostStatePublisher publisher;
  state::HostStateSnapshotRef ordinary,prepared;
  state::StateRoute route;
  s::ReliableSender reliable;
  state::StateTransferEpisode episode;
  // 一张可靠目录可与一个动态候选同时在途。联合基线仍独占发送。
  std::shared_ptr<const state::HostStateMessage> message,context;
  s::SharedBlob blob;
  std::uint64_t transfer=0,context_transfer=0,sequence=0,message_sequence=0,message_at=0,last_state_ack=0,unmatched_acks=0;
  BinaryProgress cadence;
  std::optional<std::uint64_t> tables_sent_us;
  // World 30Hz 和 Body 60Hz 独立使用高精度本机单调时钟，共享总带宽额度。
  // GetTickCount64 的15/16ms步长会把17ms间隔量化成约31ms，不能用于这两条流的发帧节奏。
  std::array<std::uint64_t,2> next_dynamic_us{};
  std::uint32_t cursor=0;
  unsigned recovery_reason=0;
  std::array<std::uint64_t,2> dynamic_blocked{};
  bool opened=false,preparing=false,registered=false,context_registered=false;
  void AdvanceDynamic(unsigned stream,std::uint64_t now_us) noexcept {
    const auto hz=stream?state::StateBudgets().body_hz:state::StateBudgets().world_hz;
    const auto period=(1'000'000+hz-1)/hz;
    auto &due=next_dynamic_us[stream];
    // 完整发出或明确跳过才消费节拍。短误差不累积，错过整周期时从当前重新排。
    due=due&&now_us>=due&&now_us-due<period?due+period:now_us+period;
  }
  // 恢复撤销在途传输和该次目录计时；可靠重传仍保留首次完整发出的起点。
  void CancelPublication() noexcept {
    if(context){reliable.Cancel(context_transfer);context.reset();}
    if(message){reliable.Cancel(transfer);message.reset();blob.reset();}
    tables_sent_us.reset();
  }
  void Clear(){publisher.Stop();CancelPublication();ordinary.reset();prepared.reset();opened=preparing=registered=context_registered=false;recovery_reason=0;last_state_ack=0;next_dynamic_us={};dynamic_blocked={};reliable.Authorize({});episode.Finish();}
  // 可靠回执推进资源目录；状态 ACK 由客机保留解码快照后发送。
  unsigned PumpReliable(std::uint64_t now,unsigned budget,const s::ReliableSender::Send &send){
    const bool table=bool(context);
    if(!table&&(!message||!blob||!s::Reliable(blob->lane)))return 0;
    auto &current=table?context:message;
    auto &sent_complete=table?context_registered:registered;
    const auto id=table?context_transfer:transfer;
    const auto sent=reliable.Pump(now,budget,send);
    if(!sent_complete&&reliable.AllSent(id)){
      if(!publisher.Sent(current,now,0))Fail("binary_sent_ledger_budget");
      sent_complete=true;
      if(table){
        if(!tables_sent_us)tables_sent_us=s::MetricMicros();
      }else if(current->target.count==2)++cadence.joint_sent;
#ifdef ISAACCS_E2E
      if(!table&&current->target.count==2){
        const auto &target=current->target;
        s::E2EStreamProgress("binary_joint_sent",{route.route,target.sections[0].base_version,
          target.sections[0].target_version,target.sections[1].base_version,target.sections[1].target_version,
          id,target.sections[0].member_epoch,target.sections[0].resource_epoch});
        if(recovery_reason==unsigned(state::StateResyncReason::Apply))
          s::E2EStreamProgress("binary_apply_recovery_baseline",{route.route,target.count,
            target.sections[0].base_version,target.sections[0].target_version,
            target.sections[1].base_version,target.sections[1].target_version,id,0});
      }
#endif
    }
    s::ReliableSender::Result result;
    while(reliable.Poll(result)){
      if(result.transfer!=id)continue; // 已取消传输的迟到结果不归新候选。
      if(!result.complete)Fail(result.reason);
      if(!sent_complete)Fail("binary_receive_before_complete_send");
      if(table)publisher.TableReceived(*current);
      else {
        if(episode.Status(now)==state::StateEpisodeStatus::Active)episode.Received(now);
        blob.reset();
      }
      current.reset();
    }
    return sent;
  }
  unsigned Pump(std::uint64_t now,std::uint64_t &transfer_sequence,const s::Route &outbound,EncodingCache &cache,const s::ReliableSender::Send &send){
    if(!opened)return 0;
    if(publisher.RecoverExpired(now)){
      recovery_reason=unsigned(state::StateResyncReason::Base);
      CancelPublication();
      if(episode.Status(now)==state::StateEpisodeStatus::Inactive)episode.Begin(now);else if(!episode.Attempt(now))Fail("binary_recovery_attempts");
    }
    bool preparing_world=false;
    const auto refresh_episode=[&]{
      preparing_world=publisher.Preparing();
      if(!preparing_world){episode.Finish();return;}
      if(episode.Status(now)==state::StateEpisodeStatus::Inactive)episode.Begin(now);
      switch(episode.Status(now)){
        case state::StateEpisodeStatus::Active:break;
        case state::StateEpisodeStatus::SendDeadline:Fail("binary_prepare_receive_deadline");
        case state::StateEpisodeStatus::TotalDeadline:Fail("binary_prepare_install_deadline");
        case state::StateEpisodeStatus::NoProgress:{
          // 诊断记录已确认的帧和最近可用目录，资源目录单独传输。
          const auto confirmed=publisher.Confirmed();
          char detail[256];
          std::snprintf(detail,sizeof(detail),"binary_prepare_no_progress received=%u/%u offered_members=%u offered_resources=%u resync=%u lane=%u",
            confirmed[0].target,confirmed[1].target,ordinary?ordinary->member_epoch:0,ordinary?ordinary->resource_epoch:0,
            recovery_reason,context?unsigned(context->lane):message?unsigned(message->lane):0);
          Fail(detail);
        }
        default:Fail("binary_prepare_budget_or_deadline");
      }
    };
    refresh_episode();
    const auto transmit=[&](std::span<const std::uint8_t> bytes){
      const auto cost=state::MeasureStateDatagram(bytes.size());
      // 失败／WouldBlock没有上行，不计为发包；先核剩余额度再调用真实socket。
      if(preparing_world&&episode.Charged()+cost>state::StateBudgets().prepare_wire_bytes)Fail("binary_prepare_wire_budget");
      const auto result=send(bytes);
      if(result==s::SendResult::Sent){
        if(preparing_world&&!episode.Charge(cost,now))Fail("binary_prepare_episode");
      }
      return result;
    };
    unsigned sent=PumpReliable(now,64,transmit);
    if(message&&blob&&s::Reliable(blob->lane))return sent;
    if(message&&(message->lane==state::StateLane::World||message->lane==state::StateLane::Body)){
      const auto deadline=message->lane==state::StateLane::World?state::StateBudgets().world_send_ms:state::StateBudgets().body_send_ms;
      if(now<message_at||now-message_at>=deadline){
        // 未完整发送的动态候选没有ACK资格；只撤候选，事件和确认base继续保留。
        const auto n=message->lane==state::StateLane::World?0u:1u;if(!dynamic_blocked[n])dynamic_blocked[n]=now;
        publisher.AbandonPending(message);message.reset();blob.reset();next_dynamic_us[n]=s::MetricMicros()+1000;return sent;
      }
    }
    // 当目录已交给可靠 sender 时，允许同一轮再选择一个普通帧。
    std::uint8_t deferred_streams=0;
    for(unsigned attempt=0;attempt<2&&sent<64;++attempt){
      if(!message){
        const auto pacing_now=s::MetricMicros();
        const auto eligible=std::uint8_t(((pacing_now>=next_dynamic_us[0]?1:0)|(pacing_now>=next_dynamic_us[1]?2:0))&~deferred_streams);
        if(!eligible&&publisher.Active()&&!preparing_world)return sent;
        for(const auto at:dynamic_blocked)if(at&&now>=at&&now-at>=state::StateBudgets().world_stale_ms){
          if(publisher.RequireRecovery(state::StateResyncReason::Budget)){
            recovery_reason=unsigned(state::StateResyncReason::Budget);CancelPublication();
          }
          dynamic_blocked={};break;
        }
        state::HostWorldDeferral world_deferral{};
        message=publisher.Next(now,pacing_now,eligible,world_deferral);
        if(world_deferral!=state::HostWorldDeferral::None){
          AdvanceDynamic(0,pacing_now);deferred_streams|=1;
        }
        refresh_episode();
        if(!message){
          // World 事件预算不足时给 Body 保留本轮独立发送机会。
          if(world_deferral!=state::HostWorldDeferral::None&&(eligible&2))continue;
          return sent;
        }
        message_sequence=Next(sequence);message_at=now;
      }
      if(!blob){
        s::Message value;value.kind=s::Kind(message->kind);value.sequence=message_sequence;value.connection=outbound.connection;value.route=outbound.epoch;
        value.state_body=message->payload;value.state_lane=s::Lane(message->lane);
        const bool compressed=outbound.zstd&&(s::Reliable(value.state_lane)||outbound.world_body_zstd);
        blob=cache.Get(value,compressed,value.state_lane==s::Lane::PlayerFast||value.state_lane==s::Lane::WorldDynamic?outbound.fast_limits:outbound.world_limits);if(!blob)return sent;
        transfer=Next(transfer_sequence);cursor=0;registered=false;
        if(s::Reliable(blob->lane))reliable.Start(blob,transfer,now);
      }
      if(message->lane==state::StateLane::Context){
        if(context||!publisher.StartTable(message))Fail("binary_context_transfer_slot");
        context=std::move(message);context_transfer=transfer;context_registered=false;blob.reset();
        sent+=PumpReliable(now,64-sent,transmit);
        continue;
      }
      if(s::Reliable(blob->lane))return sent+PumpReliable(now,64-sent,transmit);
      {
        const auto count=std::uint32_t((blob->bytes.size()+outbound.fast_limits.datagram-65)/(outbound.fast_limits.datagram-64));
        while(cursor<count&&sent<64){const auto bytes=s::Datagram(*blob,{outbound.connection,outbound.epoch,transfer},cursor,outbound.fast_limits);
          const auto result=transmit(bytes);if(result==s::SendResult::Failed)Fail("binary_socket_send");if(result==s::SendResult::WouldBlock)break;++cursor;++sent;}
        if(cursor==count){
          const auto sent_us=s::MetricMicros();
          if(!publisher.Sent(message,now,sent_us))Fail("binary_sent_ledger_budget");
#ifdef ISAACCS_E2E
          s::E2ECompareSent(*message);
          // 显式诊断时确认真实单流发送及其目录；默认关闭，不在性能窗口逐帧记录。
          const auto &section=message->target.sections[0];
          s::E2EStreamProgress("binary_dynamic_sent",{route.route,unsigned(section.stream),section.base_version,
            section.target_version,section.member_epoch,section.resource_epoch,section.source_revision,transfer});
#endif
          const auto stream=message->lane==state::StateLane::World?0u:1u;dynamic_blocked[stream]=0;
          ++(stream?cadence.body_sent:cadence.world_sent);
          AdvanceDynamic(stream,sent_us);
          message.reset();blob.reset();}
      }
      break;
    }
    return sent;
  }
};
HostPeer::~HostPeer()=default;
const s::TransferStatistics &HostPeer::TransferStatus() const noexcept{return binary_->reliable.Statistics();}
bool HostPeer::Initializing() const noexcept{return binary_->publisher.Preparing();}
bool HostPeer::Stale() const noexcept{return binary_->opened&&binary_->last_state_ack&&GetTickCount64()-binary_->last_state_ack>state::StateBudgets().world_stale_ms;}
unsigned HostPeer::PublicationPhase() const noexcept{return unsigned(ending_||stopped_?PublicationPhaseValue::Stopped:binary_->publisher.Active()?PublicationPhaseValue::Active:binary_->preparing?PublicationPhaseValue::Preparing:PublicationPhaseValue::WaitingWorld);}
HostPeer::BinaryProgress HostPeer::StateProgress() const noexcept {
  const auto confirmed=binary_->publisher.Confirmed();const auto events=binary_->publisher.EventStatistics();
  auto result=binary_->cadence;
  result.world=confirmed[0].target;result.body=confirmed[1].target;result.events=confirmed[0].event_watermark;
  result.dropped_capacity=events.dropped_capacity;result.dropped_expired=events.dropped_expired;
  result.unmatched_acks=binary_->unmatched_acks;
  result.body_dependency_blocks=binary_->publisher.BodyDependencyBlocks();
  result.world_window=binary_->publisher.WorldProgress();return result;
}
HostPeer::HostPeer(std::uint64_t session, unsigned ordinal, s::RuntimeConfig config, bool lobby,ModManifestRef mods)
    : ordinal_(ordinal), config_(config), lobby_(lobby), mods_(std::move(mods)) {
  if(*config.Validate())Fail(config.Validate());
  operations_.Configure(config_);
  scope_ = {session, Random(), 1, 1, 1, 1, 1};
  server_nonce_ = Random();
  route_ = {scope_.connection, 1, false, false, false, s::Direction::ToAuthority};
  route_.config=config_;
  assembler_.Authorize(route_);
  operations_.Bind(route_.connection, route_.epoch);
  binary_=std::make_unique<Binary>();
}
s::Message HostPeer::Message(s::Kind kind, std::uint64_t sequence) {
  return Make(kind, sequence, route_.connection, route_.epoch);
}
Packets HostPeer::Pack(const s::Message &m) { return s::Fragment(m, Next(transfer_), Limits()); }
bool HostPeer::Receive(std::span<const std::uint8_t> bytes, std::uint64_t now, ReceivedInput &out,
                       const HostIdentity &identity,ModIntentInbox *intents) {
  const auto header = s::ReadHeader(bytes, s::WorldLimits());
  if(prepared_echo_&&header.kind==s::Kind::ScenePrepared&&header.connection==route_.connection&&header.route==prepared_echo_->route){
    s::Message replay;if(prepare_replay_.Receive(bytes,now,replay)){
      if(replay.sequence!=prepared_echo_->sequence||replay.body!=prepared_echo_->body)Fail("live_prepare_replay_conflict");echo_pending_=true;last_receive_=now;
    }return false;
  }
  if ((header.kind == s::Kind::Hello || header.kind == s::Kind::ModManifest) && route_.world)
    return false;
  if (header.kind != s::Kind::Hello &&
      (header.connection != route_.connection || header.route != route_.epoch))
    return false;
  // 旧世界已撤销但传输路由要等 ScenePrepared 才递增；期间到达的旧确认没有发布归属。
  // 首次准备前 route_.world 尚未建立，仍走原来的提前确认拒绝路径。
  if (route_.world && !binary_->opened &&
      (header.kind == s::Kind::AppliedAck || header.kind == s::Kind::Resync))
    return false;
  s::Message m;
  if (!assembler_.Receive(bytes, now, m))
    return false;
  if (m.kind == s::Kind::Hello) {
    const auto nonce = N(m.body, "nonce");
    if (nonce_ && nonce_ != nonce)
      Fail("live_nonce_conflict");
    if (nonce_) {
      reply_ = true;
      last_receive_ = now;
      return false;
    }
    auto agreed = s::Negotiate(Capabilities(config_,true,false,lobby_), At(m.body, "capabilities"));
    if(lobby_&&!Supports(agreed,s::kLobbyCapability))Fail("live_lobby_required");
    assembler_.Configure(Agreed(agreed));
    route_.negotiated = true;
    route_.zstd = Zstd(agreed);
    route_.world_body_zstd = WorldBodyZstd(agreed);
    route_.world_limits = s::NegotiatedWorldLimits(agreed);
    route_.fast_limits = s::NegotiatedFastLimits(agreed);
    assembler_.Authorize(route_);
    nonce_ = nonce;
    auto welcome = Message(s::Kind::Welcome, 1);
    auto &a = welcome.body.GetAllocator();
    Object(welcome.body, "capabilities", agreed, a);
    N(welcome.body, "session", scope_.session, a);
    N(welcome.body, "client_nonce", nonce_, a);
    N(welcome.body, "server_nonce", server_nonce_, a);
    N(welcome.body, "participant", ordinal_, a);
    r::Put(welcome.body, "role", std::string("Authority"), a);
    welcome_ = std::make_shared<const s::Message>(std::move(welcome));
    auto manifest=Message(s::Kind::ModManifest,1);
    WriteModManifest(manifest,mods_);
    manifest_sender_.Authorize(route_);
    manifest_sender_.GrantRate(config_.control_bytes_per_second);
    manifest_sender_.Start(s::EncodeBlob(manifest,false,Limits()),Next(transfer_),now);
    reply_ = true;
    last_receive_ = now;
    return false;
  }
  if (!nonce_ || m.connection != route_.connection || m.route != route_.epoch)
    Fail("live_stale_connection");
  last_receive_ = now;
  if (m.kind==s::Kind::ModManifest) {
    const auto difference=CompareModManifest(mods_,m.body["chunks"]);
    if(!difference.empty())Fail(difference.c_str());
    manifest_matches_=true;
    return false;
  }
  if (!route_.world && m.kind==s::Kind::ReceiveAck) {
    manifest_sender_.Acknowledge(m,now);
    return false;
  }
  // 只有收到对端清单并逐项通过比较，才能处理 Ready、选择角色或后续报文。
  if(!manifest_matches_)return false;
  if(m.kind==s::Kind::ModIntent){
    if(!intents||!established_||!route_.world||ending_||stopped_||!binary_->opened||
       !binary_->publisher.Active()||m.sequence<=last_mod_intent_||!mod_intent_rate_.Ready(now))return false;
    if(mod_intent_run_==route_.scope.run&&mod_intent_floor_==route_.scope.floor&&
       last_mod_intent_>binary_->publisher.ModResultReceipt())return false;
    ModIntent value;
    value.id=m.sequence;value.connection=route_.connection;value.ordinal=ordinal_;
    value.run=route_.scope.run;value.floor=route_.scope.floor;value.room=route_.scope.room;
    value.mod=std::uint16_t(N(m.body,"mod"));value.declaration=std::uint16_t(N(m.body,"declaration"));
    value.member_epoch=std::uint32_t(N(m.body,"member_epoch"));
    const auto &args=m.body["args"];
    value.size=std::uint16_t(args.GetStringLength());std::memcpy(value.bytes.data(),args.GetString(),value.size);
    if(intents->Push(value)){
      last_mod_intent_=value.id;mod_intent_run_=value.run;mod_intent_floor_=value.floor;
      mod_intent_rate_.Accept(now);
    }
    return false;
  }
  if(binary_->episode.Status(now)==state::StateEpisodeStatus::Active&&!binary_->episode.Charge(state::MeasureStateDatagram(bytes.size()),now))Fail("binary_prepare_receive_budget");
  if (m.kind == s::Kind::Ready || m.kind == s::Kind::LobbySelection) {
    if (lobby_ != (m.kind == s::Kind::LobbySelection)) Fail("live_lobby_mode");
    if (N(m.body, "client_nonce") != nonce_ || N(m.body, "server_nonce") != server_nonce_)
      Fail("live_ready_identity");
    if (m.sequence > client_control_) {
      client_control_ = m.sequence;
      echo_client_ms_ = N(m.body, "sent_ms");
      echo_server_ms_ = TimingClockMs();  // 输入时序链：与 server_send_ms 同钟，不用调用方的 GetTickCount64
      if(lobby_)lobby_selection_=unsigned(N(m.body,"selection"));
      established_ = true;
    }
    return false;
  }
  if(m.state_body){
    // 正常断开已撤销发布表；在途 UDP 确认没有新确认资格，不能再交给已清空的 owner。
    if(ending_||stopped_)return false;
    if(!binary_->opened)Fail("binary_confirmation_before_prepare");
    const auto &bytes=m.state_body->bytes;
    if(m.kind==s::Kind::AppliedAck){
      const auto ack=state::DecodeStateAppliedAck(binary_->route.scope,bytes);
      const auto result=binary_->publisher.Received(binary_->route,ack);
      // 未知／冲突ACK没有确认资格；旧UDP副本也可能已超出保留回执，不推进版本或事件水位。
      if(result==state::StateAckResult::UnknownOrConflict){++binary_->unmatched_acks;return false;}
      if(result==state::StateAckResult::Accepted){
#ifdef ISAACCS_E2E
        // 只采集正在处理 Apply 恢复时的真实已接受凭据，保留原联合条目数。
        if(binary_->recovery_reason==unsigned(state::StateResyncReason::Apply))s::E2EStreamProgress("binary_apply_recovery_ack",{
          binary_->route.route,ack.count,ack.entries[0].target,ack.entries[1].target,0,0,0,0});
#endif
        binary_->last_state_ack=now;if(binary_->episode.Status(now)==state::StateEpisodeStatus::Active)binary_->episode.Progress(now);
        // 精确匹配的基线接收回执结束传输计费，原生应用由客机独立推进。
        if(ack.count==2||!binary_->publisher.Preparing()){
          binary_->episode.Finish();binary_->recovery_reason=0;}}
    }else if(m.kind==s::Kind::Resync){
      const auto request=state::DecodeStateResync(binary_->route.scope,bytes);
      if(!binary_->publisher.Recover(binary_->route,request))return false;
      binary_->recovery_reason=unsigned(request.reason);
#ifdef ISAACCS_E2E
      // 使用接收端已解析并接受的原始恢复请求，历史版本只用于明示其来源。
      s::E2EStreamProgress(request.reason==state::StateResyncReason::Apply?
        "binary_apply_recovery_request":"binary_recovery_request",{
        binary_->route.route,request.streams,binary_->recovery_reason,request.installed_world,request.installed_body,0,0,0});
#endif
      binary_->CancelPublication();
      if(binary_->episode.Status(now)==state::StateEpisodeStatus::Inactive)binary_->episode.Begin(now);
      else if(!binary_->episode.Attempt(now))Fail("binary_recovery_attempts");
    }else Fail("binary_host_receive_kind");
    return false;
  }
  if (m.kind == s::Kind::OperationAck) {
    operations_.Acknowledge(m, now);
    return false;
  }
  if (m.kind == s::Kind::ReceiveAck) {
    // 切房已清空旧发送器、尚未激活新route时，旧分片ACK没有归属，直接丢弃。
    if(ending_||stopped_||!binary_->opened)return false;
    if(binary_->reliable.Acknowledge(m,now)&&binary_->episode.Status(now)==state::StateEpisodeStatus::Active)binary_->episode.Progress(now);
    return false;
  }
  if (m.kind == s::Kind::ScenePrepared) {
    if(ending_||stopped_)return false;
    const auto prepared=binary_->prepared;
    if(!prepared||!binary_->preparing||ReadWorld(m.body["world"])!=s::WorldScope{binary_->route.scope.session,binary_->route.scope.run,binary_->route.scope.floor,binary_->route.scope.dimension,binary_->route.scope.room,binary_->route.scope.scene}||
       N(m.body,"manifest")!=prepared->ordinary.revision||N(m.body,"generation")!=prepare_generation_||String(m.body["contract"])!="binary-state-v1"||m.sequence!=prepare_sequence_)
      Fail("binary_prepared_identity");
    prepare_replay_.Authorize(route_);prepared_echo_=std::make_shared<const s::Message>(std::move(m));echo_pending_=true;
    route_.epoch=Next(route_.epoch);route_.world=true;route_.scope=ReadWorld(prepared_echo_->body["world"]);
    scope_=s::Connected(route_.scope,route_.connection);assembler_.Authorize(route_);operations_.Bind(route_.connection,route_.epoch);
    manifest_sender_.Authorize(route_);
    auto outbound=route_;outbound.direction=s::Direction::ToClient;binary_->reliable.Authorize(outbound);binary_->opened=true;binary_->preparing=false;
    return false;
  }
  if(m.kind==s::Kind::AppliedAck||m.kind==s::Kind::Resync)
    Fail("binary_confirmation_requires_binary_body");
  if (Operations::Reliable(m.kind)) {
    const auto kind = m.kind;
    const auto sequence = m.sequence;
    const auto cancel = kind == s::Kind::Cancel ? N(m.body, "transfer") : 0;
    if (!operations_.Admit(std::make_shared<const s::Message>(std::move(m)), now))
      return false;
    if (cancel) {
      assembler_.Cancel(cancel);
      if((binary_->message&&binary_->transfer==cancel)||(binary_->context&&binary_->context_transfer==cancel))
        Fail("binary_cancel_requires_explicit_resync");
    }
    if (kind == s::Kind::Stop || kind == s::Kind::Disconnect || kind == s::Kind::Retire) {
      stopped_ = true;
      binary_->Clear();
    }
    operations_.Finish({kind, sequence}, OperationStage::Complete, "network_operation_complete",
                       now);
    return false;
  }
  if(m.kind==s::Kind::Input&&(!route_.world||!binary_->publisher.Active()))return false;
  if (!established_ || m.kind != s::Kind::Input || N(m.body, "participant") != ordinal_)
    Fail("live_input_identity");
  ReceivedInput value{};
  auto &cmd = value.input.command;
  cmd.session_id = identity.session;
  cmd.client_id = identity.clients[ordinal_];
  cmd.sequence = N(m.body, "input_sequence");
  cmd.target_tick = N(m.body, "target_tick");
  cmd.state.move_x = std::int16_t(m.body["movement"][0].GetInt());
  cmd.state.move_y = std::int16_t(m.body["movement"][1].GetInt());
  cmd.state.aim_x = std::int16_t(m.body["aim"][0].GetInt());
  cmd.state.aim_y = std::int16_t(m.body["aim"][1].GetInt());
  cmd.state.held_flags = unsigned(N(m.body, "held"));
  cmd.state.edge_flags = unsigned(N(m.body, "edge"));
  cmd.state.mod_held = N(m.body, "mod_held");
  cmd.state.mod_edge = N(m.body, "mod_edge");
  value.packet_sequence = m.sequence;
  value.received_ms = now;
  value.acknowledged_generation = N(m.body, "generation");
  value.acknowledged_state = sent_.session_state;
  value.ack_stamp_known =
      value.acknowledged_generation == sent_.generation && sent_.generation != 0;
  out = value;
  return true;
}
bool HostPeer::OwnsPacket(std::span<const std::uint8_t> packet) const noexcept {
  const auto read = [&](std::size_t at) {
    std::uint64_t n = 0;
    for (unsigned i = 0; i < 8; ++i)
      n |= std::uint64_t(packet[at + i]) << (8 * i);
    return n;
  };
  return packet.size() >= 64 && read(8) == route_.connection && (read(16) == route_.epoch||
      (prepared_echo_&&read(16)==prepared_echo_->route&&packet[48]==unsigned(s::Kind::ScenePrepared)&&packet[49]==0));
}
Packets HostPeer::Reply() {
  if (!reply_)
    return {};
  reply_ = false;
  return Pack(*welcome_);
}
void HostPeer::PumpManifest(std::uint64_t now,const s::ReliableSender::Send &send) {
  if (!nonce_ || route_.world) return;
  for (const auto &ack:assembler_.Acks(now)) {
    const auto packets=Pack(s::AckMessage(ack,Next(control_),route_));
    if(send(packets.front())==s::SendResult::Failed)Fail("live_manifest_send");
  }
  manifest_sender_.Pump(now,16,send);
  s::ReliableSender::Result result;
  while(manifest_sender_.Poll(result))
    if(!result.complete&&!established_)Fail("mod_manifest_timeout");
}
Packets HostPeer::Progress(const protocol::Ack &p) {
  const auto now = GetTickCount64();
  Packets packets;
  const bool enabled=p.input_enabled&&route_.world&&binary_->publisher.Active();
  const bool changed = p.authority_paused!=sent_.authority_paused || p.generation != sent_.generation || enabled != sent_.input_enabled ||
                       p.session_state != sent_.session_state || p.roster_mask != sent_.roster_mask;
  const bool tick = p.server_tick != sent_.server_tick;
  const bool lobby_changed = p.lobby_roster != sent_.lobby_roster || p.lobby_unlocked != sent_.lobby_unlocked;
  sent_ = p;
  sent_.input_enabled=enabled;
  if (changed || !eligibility_revision_)
    Next(eligibility_revision_);
  sent_.eligibility_revision = eligibility_revision_;
  const auto append = [&](Packets more) {
    packets.insert(packets.end(), std::make_move_iterator(more.begin()),
                   std::make_move_iterator(more.end()));
  };
  if (!roster_at_ || changed || lobby_changed || now - roster_at_ >= (1000+config_.roster_hz-1)/config_.roster_hz) {
    append(Pack(Roster(sent_, ordinal_, Next(roster_), route_, echo_client_ms_, echo_server_ms_)));
    if (lobby_ && p.lobby_roster) {
      auto lobby = Message(s::Kind::LobbyRoster, roster_);
      N(lobby.body, "roster", p.lobby_roster, lobby.body.GetAllocator());
      N(lobby.body, "unlocked", p.lobby_unlocked, lobby.body.GetAllocator());
      append(Pack(lobby));
    }
    roster_at_ = now;
  }
  if (p.server_tick && p.generation &&
      ((tick&&p.server_tick%(60/config_.timing_hz)==0) || changed || !timing_at_ || now - timing_at_ >= config_.heartbeat_ms)) {
    auto m = Message(s::Kind::TickProgress, Next(timing_));
    auto &a = m.body.GetAllocator();
    N(m.body, "eligibility_revision", eligibility_revision_, a);
    N(m.body, "server_tick", p.server_tick, a);
    N(m.body, "kind", p.authority_kind, a);
    r::Put(m.body,"paused",r::Value(p.authority_paused),a);
    N(m.body, "generation", p.generation, a);
    r::Put(m.body, "input_enabled", r::Value(enabled), a);
    N(m.body, "sample_ms", p.server_sample_ms, a);
    N(m.body, "send_ms", TimingClockMs(), a);  // 输入时序链：与 sample_ms 同钟；now 仍只管发送节奏
    N(m.body, "echo_client_ms", echo_client_ms_, a);
    N(m.body, "echo_server_ms", echo_server_ms_, a);
    append(Pack(m));
    timing_at_ = now;
    if (p.processed && p.processed_tick) {
      auto ack = Message(s::Kind::InputAck, p.processed);
      auto &v = ack.body;
      auto &b = v.GetAllocator();
      N(v, "participant", ordinal_, b);
      N(v, "generation", p.generation, b);
      N(v, "input_sequence", p.processed, b);
      N(v, "processed_tick", p.processed_tick, b);
      append(Pack(ack));
    }
  }
  return s::Bundle(packets);
}
bool HostPeer::QueueModResult(const ModIntentResult &result) noexcept {
  return binary_->publisher.QueueModResult(result);
}
Packets HostPeer::Bye(const char *reason) {
  if (!nonce_ || ending_)
    return {};
  ending_ = true;
  operations_.CancelPending(GetTickCount64());
  auto m = Message(s::Kind::Disconnect, Next(control_));
  r::Put(m.body, "reason", std::string(reason), m.body.GetAllocator());
  operations_.Queue(std::make_shared<const s::Message>(std::move(m)), GetTickCount64());
  binary_->Clear();
  return {};
}
Packets HostPeer::Reject(const char *reason) {
  auto m = Message(s::Kind::Error, Next(control_));
  auto &a = m.body.GetAllocator();
  const std::string_view text(reason);
  const bool manifest=text.rfind("mod_manifest: ",0)==0;
  r::Put(m.body, "code", manifest?std::string("mod_manifest_mismatch"):std::string(text.substr(0,128)), a);
  r::Put(m.body, "recoverable", r::Value(false), a);
  r::Put(m.body, "detail", manifest?std::string(text):std::string("Client negotiation or connection rejected"), a);
  return Pack(m);
}
bool HostPeer::Offer(const Publication &p,const state::HostStatePublication::View &sources,std::uint64_t now){
  if(ending_||stopped_)return true;
  nightmare_=p.nightmare;
  // 保留上一个已授权路由直至原生演出结束，后续楼层的完整基线再接管显示。
  if(p.nightmare_active)return false;
  const auto &ordinary=sources.ordinary;
  // 已发出的 Prepare 在客机回显之前独占本次路由迁移。网络线程会重新读取最新来源。
  // 单独发出的 Retire 也必须等客机完成，避免迟到的退休包撤销同一路由的新 Prepare。
  if(binary_->preparing||operations_.Pending())return false;
  if(!ordinary){
    if(p.stage_display_active)return false; // 原生换层短暂撤下采集源时保留客机画面。
    if(!p.source[0]&&binary_->ordinary){
      const auto old=binary_->ordinary->witness.scope;
      const bool same_mod_scene=p.mod_run&&p.mod_run==old.run&&p.mod_floor==old.floor;
      binary_->Clear();
      auto m=Message(s::Kind::Retire,Next(control_));
      auto &body=m.body.GetAllocator();
      r::Put(m.body,"reason",std::string(same_mod_scene?"binary_host_room_source_retired":"binary_host_source_retired"),body);
      const auto cutscene=!same_mod_scene&&p.cutscene_run==old.run?p.cutscene_serial:0;
      r::Put(m.body,"cutscene_serial",r::Decimal(cutscene,body),body);
      r::Put(m.body,"cutscene_id",r::Value(cutscene?p.cutscene_id:0u),body);
      operations_.Queue(std::make_shared<const s::Message>(std::move(m)),now);
    }
    return !p.source[0];
  }
  auto scope=ordinary->witness.scope;scope.session=scope_.session;
  if(!binary_->ordinary||binary_->route.scope!=scope||binary_->ordinary->witness.generation!=ordinary->witness.generation){
    // 切房前先让旧route上未确认事件走完；新候选及主线程副本继续保留，不丢必要事件。
    if(binary_->ordinary&&binary_->publisher.EventWatermark()<binary_->ordinary->event_watermark)return false;
    // 新来源已就绪时，由 ScenePrepare 在客机侧退休旧场景，再回显新身份。
    binary_->Clear();
    binary_->route={route_.connection,route_.epoch+1,scope};binary_->publisher.Open(binary_->route,ordinal_);binary_->episode.Begin(now);
  }
  if(!binary_->publisher.Offer(ordinary))return false;binary_->ordinary=ordinary;
  if(sources.latest&&sources.latest!=ordinary&&sources.latest->witness.scope==ordinary->witness.scope&&!binary_->publisher.Offer(sources.latest))return false;
  if(established_&&sent_.generation&&!binary_->opened&&!binary_->preparing){
    binary_->prepared=ordinary;binary_->preparing=true;
    auto m=Message(s::Kind::ScenePrepare,Next(control_));auto &a=m.body.GetAllocator();
    PutWorld(m.body,{scope.session,scope.run,scope.floor,scope.dimension,scope.room,scope.scene},a);
    N(m.body,"next_route",binary_->route.route,a);N(m.body,"manifest",ordinary->ordinary.revision,a);N(m.body,"generation",sent_.generation,a);
    N(m.body,"native_domains",2047,a);N(m.body,"retained_domains",0,a);r::Put(m.body,"contract","binary-state-v1",a);
    prepare_sequence_=m.sequence;prepare_generation_=sent_.generation;operations_.Queue(std::make_shared<const s::Message>(std::move(m)),now);
  }
  return true;
}
unsigned HostPeer::Pump(std::uint64_t now, EncodingCache &cache,
                        const s::ReliableSender::Send &send) {
  const auto transmit = [&](const s::Message &m) {
    auto packets = Pack(m);
    if (packets.size() != 1)
      Fail("live_control_single_datagram");
    if(binary_->episode.Status(now)==state::StateEpisodeStatus::Active&&binary_->episode.Charged()+state::MeasureStateDatagram(packets.front().size())>state::StateBudgets().prepare_wire_bytes)Fail("binary_prepare_control_budget");
    const auto result = send(packets.front());
    if (result == s::SendResult::Failed)
      Fail("live_socket_send");
    if(result==s::SendResult::Sent&&binary_->episode.Status(now)==state::StateEpisodeStatus::Active&&
       !binary_->episode.Charge(state::MeasureStateDatagram(packets.front().size()),now))Fail("binary_prepare_control_budget");
    return result == s::SendResult::Sent;
  };
  if(echo_pending_&&prepared_echo_&&transmit(*prepared_echo_))echo_pending_=false;
  operations_.Pump(now, transmit);
  if (!operations_.Failure().empty())
    Fail(operations_.Failure().c_str());
  unsigned nightmare_packets=0;
  if(route_.world&&!ending_){
    if(nightmare_route_!=route_.epoch){
      nightmare_route_=route_.epoch;
      nightmare_sent_.reset();nightmare_end_repeats_=0;
      nightmare_packets_.clear();
    }
    if(now>=nightmare_deadline_)nightmare_packets_.clear();
    if(nightmare_sent_&&!nightmare_){
      nightmare_sent_.reset();
      nightmare_end_repeats_=3;
    }
    if(nightmare_packets_.empty()&&now>=nightmare_at_&&
       ((nightmare_&&nightmare_!=nightmare_sent_)||nightmare_end_repeats_)){
      nightmare_at_=now+33;
      nightmare_sent_=nightmare_;
      if(nightmare_)nightmare_end_repeats_=0;
      else --nightmare_end_repeats_;
      try {
        auto message=Message(s::Kind::Nightmare,Next(nightmare_sequence_));
        s::WriteNightmare(message,nightmare_.get());
        if(s::MeasurePayload(message,Limits())+40<=r::kNightmareWireLimit){
          nightmare_packets_=Pack(message);
          nightmare_fragment_=0;
          nightmare_deadline_=now+150;
        }
      }catch(const r::ContractError &){/* 单帧超过显示预算时可由下一帧恢复。 */}
    }
    // 一帧可跨越多次令牌补充。只保留当前发送帧，超期后从最新画面重建。
    while(nightmare_fragment_<nightmare_packets_.size()&&
          send(nightmare_packets_[nightmare_fragment_])==s::SendResult::Sent){
      ++nightmare_fragment_;
      ++nightmare_packets;
    }
    if(nightmare_fragment_==nightmare_packets_.size())nightmare_packets_.clear();
  }
  auto outbound=route_;outbound.direction=s::Direction::ToClient;
  return nightmare_packets+binary_->Pump(now,transfer_,outbound,cache,send);
}
std::uint64_t HostPeer::NextWake(std::uint64_t now) const noexcept {
  return (std::min)(operations_.NextWake(now),binary_->opened?(std::min)(binary_->reliable.NextWake(now),now+2):now+2);
}

// 每条连接只共享纯值邮箱；停止/失败用固定粘性终态发布，队列满或分配失败也必达。
struct Client::State {
  SOCKET socket = INVALID_SOCKET;
  bool wsa = false;
  std::atomic<bool> stop{false}, finished{false};
  s::RuntimeConfig config;
  s::Metrics metrics,transmit_metrics;
  s::MetricsMailbox metrics_mailbox,transmit_mailbox;
  std::mutex transfer_mutex;
  s::TransferStatistics transfer_status{};
  r::Limits effective_limits{},effective_fast_limits{};
  std::uint64_t metrics_at=0;
  bool accept_world=true;
  bool lobby=false;
  std::atomic<unsigned> lobby_selection{0};
  unsigned sent_lobby_selection=0;
  std::uint64_t lobby_sequence=0;
  std::atomic<std::uint64_t> lobby_roster{0};
  std::atomic<std::uint64_t> lobby_unlocked{0};
  SpscQueue<Intent, 256> intents;
  OwnedQueue<Event, 32> events, world_controls;
  OwnedQueue<Event, 4> urgent_controls;
  Latest<Event> timing_event, roster_event, input_ack_event, nightmare_event;
  // 连接质量显示：tick_loss／srtt_x8 只有网络线程读写；quality_* 打包发布给主线程。
  // quality_rtt：bit32 有效，低 32 位毫秒；quality_loss：bit32 有效，bit16—31 窗口长度，低 16 位千分比。
  TickLossWindow tick_loss;
  std::uint64_t srtt_x8 = 0;
  std::atomic<std::uint64_t> quality_rtt{0}, quality_loss{0};
  void ObserveRtt(std::uint64_t sample) noexcept {
    // 与 TCP SRTT 相同的 1/8 平滑；以 8 倍定点保存，首个样本直接作初值。
    const auto x8 = (std::min)(sample, std::uint64_t(0xFFFFFF)) * 8;
    srtt_x8 = srtt_x8 ? srtt_x8 - srtt_x8 / 8 + x8 / 8 : x8;
    quality_rtt.store((std::uint64_t(1) << 32) | ((srtt_x8 + 4) / 8), std::memory_order_relaxed);
  }
  void ObserveTickSequence(std::uint64_t sequence) noexcept {
    tick_loss.Observe(sequence);
    if (!tick_loss.Span()) return;
    quality_loss.store((std::uint64_t(1) << 32) | (tick_loss.Span() << 16) | tick_loss.LossPermille(),
                       std::memory_order_relaxed);
  }
  Event terminal_event;
  std::atomic<bool> terminal_ready{false};
  bool terminal_delivered = false;
  struct Command {
    enum class Type { Complete, Prepare } type = Type::Complete;
    std::shared_ptr<const s::Message> message;
    s::Route route;
    std::uint64_t generation = 0;
    OperationKey operation;
    OperationStage stage = OperationStage::Failed;
    std::array<char, 128> reason{};
  };
  OwnedQueue<Command, 32> commands;
  std::atomic<std::uint64_t> published_route{1}, published_connection{0}, input_epoch{1};
  std::uint64_t consumed_input_epoch = 1;
  s::Reassembler assembler{Limits()};
  Operations operations;
  s::Route route;
  // 工作与反馈各一个SPSC方向。接收owner只在Run线程构造/修改，Scene不跨线程调用它。
  std::shared_ptr<const state::ClientStatePreparation> binary_preparation;
  std::shared_ptr<state::ClientStateMemory> binary_memory;
  // 各通道独立交接一张在途票据，避免 World 收尾阻塞已经就绪的 Body。
  std::array<OwnedQueue<state::ClientStateApplyBatchRef,1>,2> binary_apply;
  OwnedQueue<state::ClientStateResourceJob,state::kClientResourceJobsInFlight> binary_resources;
  // 两张票据各可产生完成及实际排空回执。
  OwnedQueue<state::ClientStateApplyCompletion,4> binary_completions;
  struct ResourceCompletion {state::ClientResourcePreparationTicket ticket;state::ClientResourceState result;};
  OwnedQueue<ResourceCompletion,state::kClientResourceJobsInFlight> binary_resource_completions;
  struct BinaryRecovery {state::StateRoute route;state::StateResyncReason reason=state::StateResyncReason::Apply;};
  OwnedQueue<BinaryRecovery,1> binary_recovery;
  OwnedQueue<ModIntent,kModIntentInFlight> mod_requests;
  OwnedQueue<ModIntentResult,kModIntentInFlight> mod_results;
  std::optional<ModIntent> mod_inflight;
  std::atomic<std::uint64_t> mod_finished{0};
  std::uint64_t mod_sent_at=0,mod_result_received=0;
  memory::Lease binary_mailbox_memory=memory::Reserve(memory::Pool::Values,
      sizeof(binary_apply)+sizeof(binary_resources)+sizeof(binary_completions)+sizeof(binary_resource_completions)+sizeof(binary_recovery),"client_binary_mailboxes");
  struct Binary final:state::ClientStateReceiveDelivery {
    State &owner;
    std::unique_ptr<state::ClientStateReceiver> receiver;
    bool active=false;
    explicit Binary(State &s):owner(s) {
      const auto bindings=state::ClientStateRootBindings();
      receiver=std::make_unique<state::ClientStateReceiver>(s.binary_memory,bindings,*s.binary_preparation,*this);
    }
    bool Apply(state::ClientStateApplyBatchRef batch)override{
      const auto channel=state::StateApplyChannel(batch->target.candidate->Target());
      return owner.binary_apply[channel].TryPush(std::move(batch));
    }
    bool Resource(const state::ClientStateResourceJob &job)override{return owner.binary_resources.TryPush(job);}
    void ModResult(const ModIntentResult &value) override {
      // 私有事件在接收 ACK 前交给连接邮箱，切房不会使已确认结果丢失。
      const auto &inflight=owner.mod_inflight;
      if(!inflight||value.id!=inflight->id||value.id<=owner.mod_result_received)return;
      auto result=value;
      if(value.mod!=inflight->mod||value.declaration!=inflight->declaration){
        result.mod=inflight->mod;result.declaration=inflight->declaration;result.ok=false;result.size=0;
      }
      if(owner.mod_results.TryPush(result))owner.mod_result_received=result.id;
    }
    bool Control(state::StateMessageKind kind,std::span<const std::byte> bytes)override {
      auto m=Make(s::Kind(unsigned(kind)),Next(owner.control),owner.route.connection,owner.route.epoch);
      // Make 为旧控制消息创建对象正文；二进制确认只能保留 state_body，不能混入旧壳。
      m.body.SetNull();
      m.state_body=std::make_shared<const state::HostStateBytes>(bytes);m.state_lane=s::Lane::Control;
      // 二进制AppliedAck自身按规定周期补发；不能交给只理解compact OperationAck的旧控制事务。
      owner.SendMessage(m);return true;
    }
    void Close(){if(active){receiver->Close();active=false;}}
    // socket等待期间主线程可能已完成安装；处理新数据前先消费真实完成反馈，
    // 避免把仍未取出的Success误当成busy而丢弃零等待Body。
    void CollectFeedback() {
      state::ClientStateApplyCompletion completion;
      while(owner.binary_completions.TryPop(completion))receiver->Complete(completion);
      ResourceCompletion resource;while(owner.binary_resource_completions.TryPop(resource))receiver->CompleteResource(resource.ticket,resource.result);
      BinaryRecovery recovery;
      while(owner.binary_recovery.TryPop(recovery))if(active&&recovery.route.connection==owner.route.connection&&recovery.route.route==owner.route.epoch)
        receiver->RequireRecovery(3,recovery.reason,true);
    }
    void Pump(std::uint64_t now,bool allow_waiting=true) {
      CollectFeedback();
      // Close 后主线程仍会归还批次；Pump 的 Collect 必须继续回收这些退休租约。
      // receiver 自身在非 active 时只做 Collect，不再交付目标或发送网络确认。
      receiver->Pump(now,allow_waiting);
    }
  };
  std::unique_ptr<Binary> binary;
  r::Scope scope{};
  unsigned ordinal = 0;
  std::uint64_t nonce = 0, server_nonce = 0, transfer = 0, control = 0, last_receive = 0,
                last_send = 0, roster_sequence = 0, tick_sequence = 0;
  std::uint64_t input_ack_generation = 0, input_ack_sequence = 0, input_ack_tick = 0;
  std::uint64_t nightmare_sequence=0;
  std::uint32_t timeout = 3000;
  bool welcomed = false, have_progress = false;
  bool manifest_received=false,manifest_matches=false;
  ModManifestRef mods;
  state::ModSchemaRef mod_schema;
  s::ReliableSender manifest_sender;
  protocol::Ack progress{};
  std::shared_ptr<const s::Message> pending_prepare,prepared_response;
  std::uint64_t prepare_due=0,prepare_started=0;
  std::array<Intent, 4> history{};
  unsigned history_size = 0;
  struct Queued {
    memory::Lease memory;
    std::vector<std::uint8_t> bytes;
    std::uint64_t generation = 0, route = 0;
    bool input = false;
  };
  std::deque<Queued> pending_send;
  ~State() {
    if (socket != INVALID_SOCKET)
      closesocket(socket);
    if (wsa)
      WSACleanup();
  }
  void End(Event::Type type, const char *reason) noexcept {
    if (terminal_ready.load(std::memory_order_acquire))
      return;
    terminal_event.type = type;
    terminal_event.scope = scope;
    terminal_event.route = route.epoch;
    try{if(binary)binary->Close();}catch(...){}
    Reason(terminal_event, reason);
#ifdef ISAACCS_E2E
    // 即使主线程仍在预检，也记录worker自身的真实终态；诊断失败不改变终态发布。
    try{const auto site=std::string("client-worker-terminal:")+reason;
      s::E2EStreamProgress(site.c_str(),{0,0,unsigned(type),0,0,GetTickCount64(),0,0});}catch(...){}
#endif
    terminal_ready.store(true, std::memory_order_release);
  }
  void Emit(Event e) {
    if (!events.TryPush(std::move(e)))
      Fail("live_client_event_queue_full");
  }
  void Revoke() {
    history_size = 0;
    for (auto i = pending_send.begin(); i != pending_send.end();)
      if (i->input)
        i = pending_send.erase(i);
      else
        ++i;
  }
  s::SendResult SocketSend(std::span<const std::uint8_t> p) {
    const auto n = send(socket, reinterpret_cast<const char *>(p.data()), int(p.size()), 0);
    if (n == int(p.size())){ObservePacket(transmit_metrics,p,"client-send");return s::SendResult::Sent;}
    const auto error = WSAGetLastError();
    if (error == WSAEWOULDBLOCK || error == WSAENOBUFS || error == WSAECONNRESET ||
        error == WSAENETRESET)
      return s::SendResult::WouldBlock;
    return s::SendResult::Failed;
  }
  void Flush() {
    for (unsigned i = 0; i < 64 && !pending_send.empty(); ++i) {
      auto &p = pending_send.front();
      if (p.route != route.epoch ||
          (p.input && (p.generation != progress.generation || !progress.input_enabled))) {
        pending_send.pop_front();
        continue;
      }
      const auto result = SocketSend(p.bytes);
      if (result == s::SendResult::WouldBlock)
        return;
      if (result == s::SendResult::Failed)
        Fail("live_client_send");
      pending_send.pop_front();
    }
  }
  void SendMessage(const s::Message &m, bool input = false) {
    auto packets = s::Fragment(m, Next(transfer), Limits());
    if (packets.size() > 256 - pending_send.size())
      Fail("live_client_send_queue_full");
    for (auto &p : packets) {
      auto lease = memory::Reserve(memory::Pool::Wire, p.capacity() + sizeof(Queued) + 64);
      pending_send.push_back(
          {std::move(lease), std::move(p), progress.generation, route.epoch, input});
    }
    Flush();
  }
  void PumpModIntent(std::uint64_t now,bool ending) {
    const auto finished=mod_finished.load(std::memory_order_acquire);
    if(mod_inflight&&mod_inflight->id<=finished)mod_inflight.reset();
    if(!mod_inflight){
      ModIntent next;
      while(mod_requests.TryPop(next))if(next.id>finished){mod_inflight=next;mod_sent_at=0;break;}
    }
    // 队首重传直至收到私有结果，后续请求不能越过它。
    if(!mod_inflight||mod_result_received>=mod_inflight->id||ending||!route.world||
       mod_inflight->run!=route.scope.run||mod_inflight->floor!=route.scope.floor||
       (mod_sent_at&&now-mod_sent_at<120)||!pending_send.empty())return;
    const auto &intent=*mod_inflight;
    auto message=Make(s::Kind::ModIntent,intent.id,route.connection,route.epoch);
    auto &a=message.body.GetAllocator();
    N(message.body,"mod",intent.mod,a);N(message.body,"declaration",intent.declaration,a);
    N(message.body,"member_epoch",intent.member_epoch,a);
    r::Put(message.body,"args",r::Value(reinterpret_cast<const char *>(intent.bytes.data()),intent.size,a),a);
    SendMessage(message);mod_sent_at=now;
  }
  void Ready() {
    auto m = Make(lobby?s::Kind::LobbySelection:s::Kind::Ready, Next(control), route.connection, route.epoch);
    auto &a = m.body.GetAllocator();
    N(m.body, "client_nonce", nonce, a);
    N(m.body, "server_nonce", server_nonce, a);
    N(m.body, "sent_ms", TimingClockMs(), a);  // 输入时序链：Host 原样回显为 echo_client_ms
    if(lobby) {
      sent_lobby_selection=lobby_selection.load(std::memory_order_acquire);
      N(m.body,"selection",sent_lobby_selection,a);
    }
    SendMessage(m);
  }
  void Receive(s::Message m) {
    const auto now = GetTickCount64();
    if (m.kind == s::Kind::Error) {
      const auto code=String(m.body["code"]);
      Fail(code=="mod_manifest_mismatch"?ModManifestDetail(String(m.body["detail"])).c_str():code.c_str());
    }
    if (m.kind == s::Kind::Welcome) {
      if (N(m.body, "client_nonce") != nonce)
        Fail("live_welcome_nonce");
      const auto participant = N(m.body, "participant"), server = N(m.body, "server_nonce");
      if (welcomed &&
          (route.connection != m.connection || ordinal != participant || server_nonce != server))
        Fail("live_welcome_conflict");
      auto agreed = s::Negotiate(Capabilities(config,false,binary_preparation&&binary_preparation->Available(),lobby), m.body["capabilities"]);
      if(lobby&&!Supports(agreed,s::kLobbyCapability))Fail("live_lobby_required");
      assembler.Configure(Agreed(agreed));
      if (!welcomed) {
        scope = {N(m.body, "session"), m.connection, 1, 1, 1, 1, 1};
        route = {m.connection,
                 m.route,
                 true,
                 false,
                 Zstd(agreed),
                 s::Direction::ToClient,
                 s::NegotiatedWorldLimits(agreed)};
        ordinal = unsigned(participant);
        server_nonce = server;
        welcomed = true;
        route.config=config;route.fast_limits=s::NegotiatedFastLimits(agreed);route.world_body_zstd=WorldBodyZstd(agreed);
        assembler.Authorize(route);
        operations.Bind(route.connection, route.epoch);
        published_connection.store(route.connection, std::memory_order_release);
      }
      last_receive = now;
      return;
    }
    if (!welcomed || m.connection != route.connection || m.route != route.epoch)
      Fail("live_server_scope");
    if (m.kind==s::Kind::ModManifest) {
      const auto difference=CompareModManifest(mods,m.body["chunks"]);
      if(!difference.empty())Fail(difference.c_str());
      last_receive=now;
      if(manifest_received)return;
      auto response=Make(s::Kind::ModManifest,1,route.connection,route.epoch);
      WriteModManifest(response,mods);
      manifest_sender.Authorize(route);
      manifest_sender.Start(s::EncodeBlob(response,false,Limits()),Next(transfer),now);
      manifest_received=true;
      return;
    }
    if (m.kind==s::Kind::ReceiveAck) {
      if(!route.world&&manifest_received) {
        last_receive=now;
        manifest_sender.Acknowledge(m,now);
        s::ReliableSender::Result result;
        while(manifest_sender.Poll(result)) {
          if(!result.complete)Fail("mod_manifest_timeout");
          if(manifest_matches)continue;
          manifest_matches=true;
          Event e;
          e.type=Event::Type::Welcome;e.scope=scope;e.route=route.epoch;e.ordinal=ordinal;
          Emit(e);
          Ready();
        }
      }
      return;
    }
    if(!manifest_matches)return;
    if (terminal_ready.load(std::memory_order_acquire) && m.kind != s::Kind::OperationAck &&
        m.kind != s::Kind::Stop && m.kind != s::Kind::Disconnect && m.kind != s::Kind::Cancel)
      return;
    last_receive = now;
    if(m.kind==s::Kind::Nightmare){
      if(m.sequence<=nightmare_sequence)return;
      Event event;
      event.type=Event::Type::Nightmare;event.scope=scope;event.route=m.route;
      event.nightmare=s::ReadNightmare(m);event.received_ms=now;
      nightmare_sequence=m.sequence;
      nightmare_event.Put(std::make_shared<const Event>(std::move(event)));
      return;
    }
    if(m.kind==s::Kind::LobbyRoster) {
      if(!lobby)Fail("live_lobby_mode");
      const auto value=N(m.body,"roster");
      if(!(core::LobbyRoster::Unpack(value).present&(1u<<ordinal)))Fail("live_lobby_roster_identity");
      if(m.sequence>lobby_sequence) {
        lobby_sequence=m.sequence;
        // 先存解锁位图再存名单，读者看到新名单时位图不会更旧。
        lobby_unlocked.store(N(m.body,"unlocked"),std::memory_order_release);
        lobby_roster.store(value,std::memory_order_release);
      }
      return;
    }
    if (m.kind == s::Kind::OperationAck) {
      operations.Acknowledge(m, now);
      return;
    }
    if(m.kind==s::Kind::ScenePrepared){
      if(!pending_prepare||!prepared_response||m.sequence!=prepared_response->sequence||m.body!=prepared_response->body)Fail("live_prepare_confirmation");
      Revoke();Intent old;while(intents.TryPop(old)){};
      route.epoch=N(pending_prepare->body,"next_route");route.world=true;route.scope=ReadWorld(pending_prepare->body["world"]);scope=s::Connected(route.scope,route.connection);
      accept_world=true;metrics.Bind(route.scope,route.epoch);tick_loss.Rebase();
      assembler.Authorize(route);operations.Bind(route.connection,route.epoch);manifest_sender.Authorize(route);have_progress=false;progress={};roster_sequence=tick_sequence=0;input_ack_generation=input_ack_sequence=input_ack_tick=0;
      // 主线程是值队列唯一consumer；旧路由由Poll/PollWorld过滤并释放。
      published_route.store(route.epoch,std::memory_order_release);Event activated;activated.type=Event::Type::Prepared;activated.scope=scope;activated.route=route.epoch;activated.ordinal=ordinal;activated.world=pending_prepare;
      if(String(pending_prepare->body["contract"])=="binary-state-v1") {
        if(!binary_preparation||!binary_preparation->Available())Fail("binary_client_native_contract_unavailable");
        if(!binary)binary=std::make_unique<Binary>(*this);
        if(!binary->receiver->Drained())Fail("binary_client_previous_route_not_drained");
        binary->receiver->Open({route.connection,route.epoch,{route.scope.session,route.scope.run,route.scope.floor,route.scope.dimension,route.scope.room,route.scope.scene}},state::LocalStateAgreement(),route.epoch,now,mod_schema,ordinal);binary->active=true;
      }
      // 路由激活与退休共用有序控制队列，先于后续原生世界接纳；不能跨两个队列竞速。
      if(!urgent_controls.TryPush(std::move(activated)))Fail("live_prepare_activation_capacity");
      pending_prepare.reset();prepared_response.reset();return;
    }
    Event e;
    e.scope = scope;
    e.route = route.epoch;
    e.ordinal = ordinal;
    e.received_ms = TimingClockMs();  // 输入时序链：与 sent_ms 及客机计算目标时的“现在”同钟
    if(m.state_body) {
      if(!accept_world||!binary||!binary->active)return;
      if(m.kind!=s::Kind::StateFrame&&m.kind!=s::Kind::StateResources)Fail("binary_client_receive_kind");
      binary->CollectFeedback();
      binary->receiver->Receive(state::StateMessageKind(unsigned(m.kind)),state::StateLane(unsigned(m.state_lane)),
        m.state_body->bytes,m.sequence,now,true);
      return;
    }
    if (m.kind == s::Kind::Roster) {
      if (m.sequence <= roster_sequence ||
          N(m.body, "eligibility_revision") < progress.eligibility_revision)
        return;
      const auto &v = m.body;
      if (N(v, "participant") != ordinal)
        Fail("live_roster_identity");
      auto next = progress;
      next.eligibility_revision = N(v, "eligibility_revision");
      next.client_id = ordinal + 1;
      next.accepted = N(v, "accepted");
      next.processed = N(v, "processed");
      next.processed_tick = N(v, "processed_tick");
      next.processed_source = N(v, "processed_source");
      next.roster_mask = unsigned(N(v, "roster"));
      next.rejected_count = N(v, "rejected");
      next.rejection_mask = unsigned(N(v, "rejection_mask"));
      next.session_state = core::SessionState(N(v, "state"));
      next.generation = N(v, "generation");
      next.input_enabled = v["input_enabled"].GetBool();
      next.server_tick = N(v, "server_tick");
      if (have_progress && next.server_tick < progress.server_tick) {
        next.server_tick = progress.server_tick;
        next.server_sample_ms = progress.server_sample_ms;
      } else
        next.server_sample_ms = N(v, "server_sample_ms");
      if (next.generation != progress.generation || !next.input_enabled ||
          next.session_state != core::SessionState::Active)
        Revoke();
      if (have_progress && !protocol::AckAdvances(progress, next))
        Fail("live_progress_regression");
      progress = next;
      e.progress = progress;
      e.type = Event::Type::Progress;
      Timing(e, v, "server_sample_ms", "server_send_ms");
      ObserveRtt(e.round_trip_ms);
      roster_sequence = m.sequence;
      roster_event.Put(std::make_shared<const Event>(std::move(e)));
      return;
    }
    if (m.kind == s::Kind::TickProgress) {
      // 丢包统计看全部到达的序号（含随后因旧/重复被丢弃的），先于下方过滤。
      ObserveTickSequence(m.sequence);
      if (m.sequence <= tick_sequence ||
          N(m.body, "eligibility_revision") < progress.eligibility_revision)
        return;
      progress.eligibility_revision = N(m.body, "eligibility_revision");
      const auto tick = N(m.body, "server_tick"), generation = N(m.body, "generation");
      if (have_progress && tick < progress.server_tick)
        return;
      if (generation != progress.generation || !m.body["input_enabled"].GetBool())
        Revoke();
      progress.server_tick = tick;
      progress.generation = generation;
      progress.input_enabled = m.body["input_enabled"].GetBool();
      progress.server_sample_ms = N(m.body, "sample_ms");
      progress.authority_kind = unsigned(N(m.body, "kind"));
      progress.authority_paused=m.body["paused"].GetBool();
      have_progress = true;
      tick_sequence = m.sequence;
      e.type = Event::Type::TickProgress;
      e.progress = progress;
      Timing(e, m.body, "sample_ms", "send_ms");
      ObserveRtt(e.round_trip_ms);
      timing_event.Put(std::make_shared<const Event>(std::move(e)));
      return;
    }
    if (m.kind == s::Kind::InputAck) {
      if (N(m.body, "participant") != ordinal)
        Fail("live_ack_identity");
      const auto generation = N(m.body, "generation"), sequence = N(m.body, "input_sequence"),
                 tick = N(m.body, "processed_tick");
      if (generation < input_ack_generation ||
          (generation == input_ack_generation && sequence < input_ack_sequence))
        return;
      if(generation==input_ack_generation){
        // processed 是累计消费水位；中性帧仍推进 processed_tick，不要求同一序号永远绑定首次 tick。
        // 同水位的旧／重复回执可直接忽略，新水位却倒退 tick 才属于真实冲突。
        if(sequence==input_ack_sequence&&tick<=input_ack_tick)return;
        if(tick<input_ack_tick)Fail("live_input_ack_conflict");
      }
      input_ack_generation = generation;
      input_ack_sequence = sequence;
      input_ack_tick = tick;
      e.type = Event::Type::InputAck;
      e.progress.generation = N(m.body, "generation");
      e.progress.processed = N(m.body, "input_sequence");
      e.progress.processed_tick = N(m.body, "processed_tick");
      input_ack_event.Put(std::make_shared<const Event>(std::move(e)));
      return;
    }
    const auto kind = m.kind;
    const auto sequence = m.sequence;
    const auto cancel = kind == s::Kind::Cancel ? N(m.body, "transfer") : 0;
    e.world = std::make_shared<const s::Message>(std::move(m));
    if (Operations::Reliable(kind)) {
      if (!operations.Admit(e.world, now))
        return;
      e.type = Event::Type::Control;
      if(kind==s::Kind::ScenePrepare){Revoke();progress.input_enabled=false;have_progress=false;if(binary)binary->Close();}
      if (cancel)
        assembler.Cancel(cancel);
      if (kind == s::Kind::Stop || kind == s::Kind::Disconnect) {
        Revoke();
        End(Event::Type::Stopped, "server_disconnected");
        operations.Finish({kind, sequence}, OperationStage::Complete, "network_stopped", now);
      } else if (kind == s::Kind::Cancel)
        operations.Finish({kind, sequence}, OperationStage::Complete, "transfer_cancelled", now);
      else if (kind == s::Kind::Retire) {
        if(binary)binary->Close();
        pending_prepare.reset();prepared_response.reset();
        Revoke();
        progress.input_enabled = false;
        have_progress = false;
        if (!urgent_controls.TryPush(std::move(e)))
          Fail("live_retirement_control_capacity");
      } else if (!world_controls.TryPush(std::move(e)))
        Fail("live_world_control_capacity");
      return;
    }
    Fail("live_unexpected_world_message");
  }
  void ReceivePacket(std::span<const std::uint8_t> packet) {
    const auto h = s::ReadHeader(packet, s::WorldLimits());
    if (h.kind==s::Kind::ModManifest&&(!welcomed||route.world))return;
    if (welcomed && (h.connection != route.connection || h.route != route.epoch))
      return;
    if(h.kind==s::Kind::ModManifest)last_receive=GetTickCount64();
    if (welcomed && h.kind == s::Kind::Welcome) {
      last_receive = GetTickCount64();
      if(manifest_matches)Ready();
      return;
    }
    s::Message m;
    s::Measure decode_measure(metrics,s::CpuStage::Decode);
    if (assembler.Receive(packet, GetTickCount64(), m))
      Receive(std::move(m));
  }
  void Commands(std::uint64_t now) {
    Command c;
    for (unsigned n = 0; n < 32 && commands.TryPop(c); ++n) {
      if(stop.load(std::memory_order_acquire))continue;
      if (c.type == Command::Type::Complete) {
        if (c.route.connection != route.connection || c.route.epoch != route.epoch)
          continue;
        operations.Finish(c.operation, c.stage, c.reason.data(), now);
        continue;
      }
      if(c.type==Command::Type::Prepare){
        if(!c.message||c.message->kind!=s::Kind::ScenePrepare||c.message->connection!=route.connection||c.message->route!=route.epoch)continue;
        if(pending_prepare){if(pending_prepare->sequence!=c.message->sequence||pending_prepare->body!=c.message->body)Fail("live_pending_prepare_conflict");continue;}
        Revoke();Intent old;while(intents.TryPop(old)){};have_progress=false;progress={};pending_prepare=c.message;
        auto response=Make(s::Kind::ScenePrepared,c.message->sequence,route.connection,route.epoch);response.body=r::Copy(c.message->body);response.body.RemoveMember("next_route");
        prepared_response=std::make_shared<const s::Message>(std::move(response));prepare_due=now;prepare_started=now;continue;
      }
    }
  }
  void Run() noexcept {
    try {
      auto hello = Make(s::Kind::Hello, 1, 0, 0);
      auto &a = hello.body.GetAllocator();
      Object(hello.body, "capabilities", Capabilities(config,false,binary_preparation&&binary_preparation->Available(),lobby), a);
      N(hello.body, "nonce", nonce, a);
      r::Put(hello.body, "role", std::string("Client"), a);
      last_receive = GetTickCount64();
      std::uint64_t ending = 0;
      for (;;) {
        const auto now = GetTickCount64();
        s::Measure network_measure(metrics,s::CpuStage::Network);
        if (stop.load(std::memory_order_acquire) && !ending) {
          if (!manifest_matches)
            break;
          operations.CancelPending(now);
          auto bye = Make(s::Kind::Disconnect, Next(control), route.connection, route.epoch);
          r::Put(bye.body, "reason", std::string("client_disconnected"), bye.body.GetAllocator());
          operations.Queue(std::make_shared<const s::Message>(std::move(bye)), now);
          End(Event::Type::Stopped, "client_stopped");
          ending = now;
        }
        const auto epoch = input_epoch.load(std::memory_order_acquire);
        if (epoch != consumed_input_epoch) {
          Revoke();
          consumed_input_epoch = epoch;
        }
        Flush();
        Commands(now);
        // 先释放已完成的应用槽，再读本轮已到达的UDP；等待world不能在读取新body之前反复抢占。
        if(binary)binary->Pump(now,false);
        if(prepared_response&&now>=prepare_due){
          if(now<prepare_started||now-prepare_started>=config.progress_timeout_ms)Fail("live_prepare_no_confirmation");
          // 旧路由的本机回执和最后一份计划必须由 receiver 消费后才授权新路由。
          if(!binary||binary->receiver->Drained()){SendMessage(*prepared_response);prepare_due=now+config.final_retry_ms;}
        }
        if (terminal_ready.load(std::memory_order_acquire)) {
          if (!ending)
            ending = now;
          Revoke();
          if (now - ending >= config.progress_timeout_ms || (now - ending >= 100 && operations.Settled()))
            break;
        } else if (now - last_receive >= timeout)
          Fail("live_peer_timeout");
        const bool selection_changed=lobby&&manifest_matches&&sent_lobby_selection!=lobby_selection.load(std::memory_order_acquire);
        if (!ending && (selection_changed || now - last_send >= config.heartbeat_ms)) {
          if (manifest_matches)
            Ready();
          else
            SendMessage(hello);
          last_send = now;
        }
        if(!ending&&manifest_received&&!manifest_matches) {
          manifest_sender.Pump(now,16,[&](auto bytes){return SocketSend(bytes);});
          s::ReliableSender::Result result;
          while(manifest_sender.Poll(result))
            if(!result.complete)Fail("mod_manifest_timeout");
        }
        operations.Pump(now, [&](const s::Message &m) {
          const auto packets = s::Fragment(m, Next(transfer), Limits());
          if (packets.size() != 1)
            Fail("live_control_single_datagram");
          const auto result = SocketSend(packets.front());
          if (result == s::SendResult::Failed)
            Fail("live_client_send");
          return result == s::SendResult::Sent;
        });
        if (!operations.Failure().empty())
          Fail(operations.Failure().c_str());
        for (const auto &ack : assembler.Acks(now))
          SendMessage(s::AckMessage(ack, Next(control), route));
        PumpModIntent(now,ending);
        Intent intent;
        for (unsigned n = 0; n < 32 && intents.TryPop(intent); ++n) {
          if (intent.input_epoch != consumed_input_epoch || ending || !welcomed || !have_progress ||
              !progress.input_enabled || progress.session_state != core::SessionState::Active ||
              intent.scope != scope || intent.route != route.epoch || !intent.generation ||
              intent.generation != progress.generation)
            continue;
          if (history_size && history[history_size - 1].generation != intent.generation)
            Revoke();
          if (history_size == history.size()) {
            std::move(history.begin() + 1, history.end(), history.begin());
            --history_size;
          }
          history[history_size++] = intent;
          for (unsigned i = 0; i < history_size; ++i) {
            const auto &v = history[i];
            if (v.target < progress.server_tick && progress.server_tick - v.target > 8)
              continue;
            auto m = Make(s::Kind::Input, v.sequence, v.scope.connection, v.route);
            auto old =
                r::InputIntent(v.scope, ordinal, v.generation, v.sequence, v.target, v.state);
            m.body = std::move(old.body);
            m.body.RemoveMember("scope");
            SendMessage(m, true);
          }
        }
        metrics.Queue(pending_send.size()+world_controls.Size()+urgent_controls.Size()+commands.Size());metrics.candidates=assembler.Pending();
        if(now>=metrics_at){metrics_mailbox.Publish(metrics);transmit_mailbox.Publish(transmit_metrics);metrics_at=now+50;}
        if(transfer_mutex.try_lock()){transfer_status=assembler.Statistics();effective_limits=route.world_limits;effective_fast_limits=route.fast_limits;transfer_mutex.unlock();}
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket, &reads);
        const auto wake =
            (std::min)(now + config.bundle_wait_ms, (std::min)(operations.NextWake(now), assembler.NextWake(now)));
        // 重工作片已经让出。先轮询可达 Body／反馈，再续做，避免固定等待一个 bundle 周期。
        const bool resume=binary&&binary->receiver->HasRunnableSlice();
        timeval wait{0, resume?0:long((wake > now ? wake - now : 1) * 1000)};
        if (select(0, &reads, nullptr, nullptr, &wait) == SOCKET_ERROR)
          Fail("live_client_select");
        for (unsigned n = 0; n < 64; ++n) {
          std::array<std::uint8_t, 1201> bytes{};
          const auto count =
              recv(socket, reinterpret_cast<char *>(bytes.data()), int(bytes.size()), 0);
          if (count == SOCKET_ERROR) {
            const auto error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK)
              break;
            if (error == WSAECONNRESET || error == WSAENETRESET || error == WSAEMSGSIZE)
              continue;
            Fail("live_client_receive");
          }
          const std::span<const std::uint8_t> packet(bytes.data(), std::size_t(count));
          ObservePacket(metrics,packet);
          if (s::IsBundle(packet)) {
            auto workspace = memory::Reserve(memory::Pool::Workspace, 8192);
            for (const auto &part : s::Unbundle(packet)) {
              ReceivePacket(part);
            }
          } else {
            ReceivePacket(packet);
          }
        }
        if(binary)binary->Pump(GetTickCount64());
      }
      Revoke();
      pending_send.clear();
    } catch (const std::exception &error) {
      End(Event::Type::Failed, error.what());
    } catch (...) {
      End(Event::Type::Failed, "live_client_exception");
    }
    // socket终态不代表主线程已排空。继续只收真实完成/清理回执，不再接纳网络目标。
    // 保留原生在途存储到实际退休，不能在超时路径销毁receiver或强杀writer。
    if(binary) {
      try{binary->Close();}catch(...){}
      while(!binary->receiver->Drained()) {
        try{binary->Pump(GetTickCount64());}catch(...){}
        Sleep(1);
      }
      binary.reset();
    }
    Intent retired_intent;while(intents.TryPop(retired_intent)){}
    Command retired_command;while(commands.TryPop(retired_command)){}
    metrics_mailbox.Publish(metrics);transmit_mailbox.Publish(transmit_metrics);
    if(socket!=INVALID_SOCKET){closesocket(socket);socket=INVALID_SOCKET;}if(wsa){WSACleanup();wsa=false;}
    finished.store(true, std::memory_order_release);
  }
  static DWORD WINAPI Entry(void *arg) noexcept {
    std::unique_ptr<std::shared_ptr<State>> owner(static_cast<std::shared_ptr<State> *>(arg));
    auto self = std::move(*owner);
    owner.reset();
    self->Run();
    return 0;
  }
};
Client::~Client() {
  Stop();
  if (thread_)
    CloseHandle(thread_);
}
const char *Client::Start(const std::string &ipv4, std::uint16_t port,
                          std::uint32_t timeout_ms,s::RuntimeConfig config,std::shared_ptr<const state::ClientStatePreparation> binary,
                          std::shared_ptr<state::ClientStateMemory> binary_memory,bool lobby,ModManifestRef mods,state::ModSchemaRef mod_schema) noexcept {
  if (*config.Validate())return config.Validate();
  if(config.peer_timeout_ms!=timeout_ms)return "live_timeout_configuration_conflict";
  if (state_ || !port || timeout_ms < 500 || timeout_ms > 60000)
    return "live_configuration";
  try {
    auto state = std::make_shared<State>();
    state->lobby=lobby;
    state->mods=std::move(mods);
    state->mod_schema=std::move(mod_schema);
    state->binary_preparation=std::move(binary);
    state->binary_memory=binary_memory?std::move(binary_memory):
      state->binary_preparation?state::ClientStateMemory::Create():nullptr;
    state->config=config;state->route.config=config;state->operations.Configure(config);
    state->nonce = Random();
    state->timeout = timeout_ms;
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data))
      return "live_winsock";
    state->wsa = true;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (InetPtonA(AF_INET, ipv4.c_str(), &address.sin_addr) != 1 ||
        address.sin_addr.s_addr == INADDR_ANY || address.sin_addr.s_addr == INADDR_BROADCAST)
      return "live_ipv4";
    state->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (state->socket == INVALID_SOCKET)
      return "live_socket";
    u_long nonblocking = 1;
    if (ioctlsocket(state->socket, FIONBIO, &nonblocking))
      return "live_nonblocking";
    if (connect(state->socket, reinterpret_cast<const sockaddr *>(&address), sizeof(address)))
      return "live_connect";
    auto transfer = std::make_unique<std::shared_ptr<State>>(state);
    auto thread = CreateThread(nullptr, 0, State::Entry, transfer.get(), 0, nullptr);
    if (!thread)
      return "live_thread";
    transfer.release();
    thread_ = thread;
    state_ = std::move(state);
    return "";
  } catch (...) {
    return "live_allocation";
  }
}
r::Limits Client::EffectiveLimits() const noexcept {if(!state_)return {};std::lock_guard lock(state_->transfer_mutex);return state_->effective_limits;}
bool Client::SendModIntent(const ModIntent &value) noexcept {
  return state_&&!Finished()&&!state_->stop.load()&&state_->mod_requests.TryPush(value);
}
bool Client::PollModResult(ModIntentResult &value) noexcept {
  return state_&&state_->mod_results.TryPop(value);
}
void Client::FinishModIntent(std::uint64_t id) noexcept {
  if(state_)state_->mod_finished.store((std::max)(id,state_->mod_finished.load(std::memory_order_relaxed)),std::memory_order_release);
}
bool Client::SetLobbySelection(unsigned selection) noexcept {
  if(!state_||!state_->lobby||!core::ValidLobbySelection(selection)||Finished()||state_->stop.load())return false;
  state_->lobby_selection.store(selection,std::memory_order_release);
  return true;
}
std::uint64_t Client::LobbyRoster() const noexcept {
  return state_?state_->lobby_roster.load(std::memory_order_acquire):0;
}
std::uint64_t Client::LobbyUnlocked() const noexcept {
  return state_?state_->lobby_unlocked.load(std::memory_order_acquire):0;
}
r::Limits Client::EffectiveFastLimits() const noexcept {if(!state_)return {};std::lock_guard lock(state_->transfer_mutex);return state_->effective_fast_limits;}
s::Metrics Client::TransmitMetrics() const noexcept {return state_?state_->transmit_mailbox.Read():s::Metrics{};}
s::Metrics Client::Metrics() const noexcept {return state_?state_->metrics_mailbox.Read():s::Metrics{};}
LinkQuality Client::Quality() const noexcept {
  LinkQuality q;
  if (!state_) return q;
  const auto rtt = state_->quality_rtt.load(std::memory_order_relaxed),
             loss = state_->quality_loss.load(std::memory_order_relaxed);
  q.rtt_valid = (rtt >> 32) & 1;
  q.rtt_ms = std::uint32_t(rtt);
  q.loss_valid = (loss >> 32) & 1;
  q.loss_window = std::uint32_t((loss >> 16) & 0xFFFF);
  q.loss_permille = std::uint32_t(loss & 0xFFFF);
  return q;
}
s::TransferStatistics Client::TransferStatus() const noexcept {if(!state_)return {};std::lock_guard lock(state_->transfer_mutex);return state_->transfer_status;}
bool Client::Send(const Intent &value) noexcept {
  if (!state_ || state_->stop.load(std::memory_order_acquire) || Finished() ||
      value.scope.connection != state_->published_connection.load(std::memory_order_acquire) ||
      value.route != state_->published_route.load(std::memory_order_acquire))
    return false;
  auto intent = value;
  intent.input_epoch = state_->input_epoch.load(std::memory_order_acquire);
  return state_->intents.TryPush(intent);
}
void Client::RevokeInput() noexcept {
  if (!state_)
    return;
  auto epoch = state_->input_epoch.load(std::memory_order_relaxed);
  for (;;) {
    if (epoch == UINT64_MAX) {
      Stop();
      return;
    }
    if (state_->input_epoch.compare_exchange_weak(epoch, epoch + 1, std::memory_order_release,
                                                  std::memory_order_relaxed))
      return;
  }
}

bool Client::Poll(Event &e) noexcept {
  if (!state_)
    return false;
  if (!state_->terminal_delivered && state_->terminal_ready.load(std::memory_order_acquire)) {
    e = state_->terminal_event;
    state_->terminal_delivered = true;
    return true;
  }
  if (state_->events.TryPop(e))
    return true;
  if(auto nightmare=state_->nightmare_event.Take()){
    e=*nightmare;
    return true;
  }
  if (auto roster = state_->roster_event.Take()) {
    e = *roster;
    return true;
  }
  if (auto timing = state_->timing_event.Take()) {
    e = *timing;
    return true;
  }
  if (auto ack = state_->input_ack_event.Take()) {
    e = *ack;
    return true;
  }
  return false;
}
bool Client::PollWorld(Event &e) noexcept {
  if (!state_)
    return false;
  for (unsigned n = 0; n < 68; ++n) {
    if(!state_->urgent_controls.TryPop(e)&&!state_->world_controls.TryPop(e))return false;
    if (e.scope.connection == state_->published_connection.load(std::memory_order_acquire) &&
        e.route == state_->published_route.load(std::memory_order_acquire))
      return true;
    e = {};
  }
  return false;
}
bool Client::PollBinary(unsigned channel,state::ClientStateApplyBatchRef &batch) noexcept {
  return state_&&channel<state_->binary_apply.size()&&state_->binary_apply[channel].TryPop(batch);
}
bool Client::PollBinaryResource(state::ClientStateResourceJob &job) noexcept {
  return state_&&state_->binary_resources.TryPop(job);
}
bool Client::CompleteBinary(const state::ClientStateApplyCompletion &completion) noexcept {
  return state_&&!Finished()&&state_->binary_completions.TryPush(completion);
}
bool Client::CompleteBinaryResource(const state::ClientResourcePreparationTicket &ticket,state::ClientResourceState result) noexcept {
  return state_&&!Finished()&&state_->binary_resource_completions.TryPush({ticket,result});
}
bool Client::RecoverBinary(const state::StateRoute &route,state::StateResyncReason reason) noexcept {
  return state_&&!Finished()&&state_->binary_recovery.TryPush({route,reason});
}
bool Client::Prepare(const Event &event) noexcept {if(!state_||Finished()||!event.world||event.world->kind!=s::Kind::ScenePrepare)return false;State::Command command;command.type=State::Command::Type::Prepare;command.message=event.world;return state_->commands.TryPush(std::move(command));}
bool Client::Complete(const Event &event, OperationStage stage, const char *reason) noexcept {
  if (!state_ || Finished() || !event.world || !Operations::Reliable(event.world->kind) ||
      unsigned(stage) < 2 || unsigned(stage) > 5)
    return false;
  State::Command c;
  c.type = State::Command::Type::Complete;
  c.operation = {event.world->kind, event.world->sequence};
  c.stage = stage;
  c.route.connection = event.scope.connection;
  c.route.epoch = event.route;
  strncpy_s(c.reason.data(), c.reason.size(), reason, _TRUNCATE);
  return state_->commands.TryPush(std::move(c));
}
void Client::Stop() noexcept {
  if (state_)
    state_->stop.store(true, std::memory_order_release);
}
bool Client::Finished() const noexcept {
  return !state_ || state_->finished.load(std::memory_order_acquire);
}
} // namespace cs::net::live

