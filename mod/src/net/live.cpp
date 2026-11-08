// Winsock必须先于Windows；网络线程只读取自有纯值。
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include "net/live.h"
#include "replica/stream_e2e.h"
#include "net/queue.h"
#include "replica/stream_clock_e2e.h"
#include <algorithm>
#include <cstring>

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
r::Json Capabilities(const s::RuntimeConfig &config) {
  auto caps = s::Capabilities();
  // 真实DLL声明已接线的具名owner能力；纯模型Capabilities仍不冒充原生Client。
  auto &a=caps.GetAllocator();for(const char *name:{"client-scene-entity-v1","client-scene-grid-v1","client-scene-room-v1","client-scene-player-v2"})caps["native_appliers"].PushBack(r::Value(name,a),a);
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
  N(v, "server_send_ms", GetTickCount64(), a);
  N(v, "echo_client_ms", echo, a);
  N(v, "echo_server_ms", received, a);
  return m;
}
void Timing(Event &e, const r::Value &v, const char *sample, const char *sent) {
  // GetTickCount64在受支持Windows环境通常以10—16ms量化。两台机器各自
  // 的区间差可相差两次量化，不能把正常15ms负RTT误认成时钟倒退。
  // 仍拒绝各机内部顺序错误与超出32ms的矛盾，不比较异机绝对时钟。
  constexpr std::uint64_t quantum_ms = 16;
  const auto now = e.received_ms, send = N(v, sent), echo = N(v, "echo_client_ms"),
             received = N(v, "echo_server_ms");
  e.progress.server_sample_ms = N(v, sample);
  if (!echo || echo > now || e.progress.server_sample_ms > send || received > send ||
      ((send - received) > (now - echo) && (send - received) - (now - echo) > 2 * quantum_ms)) {
#ifdef ISAACCS_E2E
    s::E2EClockRejected(echo,now,received,send,e.progress.server_sample_ms);
#endif
    Fail("live_clock_range");
  }
  e.round_trip_ms = now - echo >= send - received ? (now - echo) - (send - received) : 0;
  e.publication_age_ms = send - e.progress.server_sample_ms;
  // RTT差的两次量化、发布区间及接收后区间各一次，合计保守误差64ms。
  // 不把负差夹零后当作无误差，也不伪增实测RTT用于N0网络判定。
  e.timing_uncertainty_ms = 4 * quantum_ms;
}
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
HostPeer::HostPeer(std::uint64_t session, unsigned ordinal, s::RuntimeConfig config) : ordinal_(ordinal),config_(config) {
  if(*config.Validate())Fail(config.Validate());
  operations_.Configure(config_);
  scope_ = {session, Random(), 1, 1, 1, 1, 1};
  server_nonce_ = Random();
  route_ = {scope_.connection, 1, false, false, false, s::Direction::ToAuthority};
  route_.config=config_;
  assembler_.Authorize(route_);
  operations_.Bind(route_.connection, route_.epoch);
}
s::Message HostPeer::Message(s::Kind kind, std::uint64_t sequence) {
  return Make(kind, sequence, route_.connection, route_.epoch);
}
Packets HostPeer::Pack(const s::Message &m) { return s::Fragment(m, Next(transfer_), Limits()); }
bool HostPeer::Receive(std::span<const std::uint8_t> bytes, std::uint64_t now, ReceivedInput &out,
                       const HostIdentity &identity) {
  const auto header = s::ReadHeader(bytes, s::WorldLimits());
  if(prepared_echo_&&header.kind==s::Kind::ScenePrepared&&header.connection==route_.connection&&header.route==prepared_echo_->route){
    s::Message replay;if(prepare_replay_.Receive(bytes,now,replay)){
      if(replay.sequence!=prepared_echo_->sequence||replay.body!=prepared_echo_->body)Fail("live_prepare_replay_conflict");echo_pending_=true;last_receive_=now;
    }return false;
  }
  if (header.kind == s::Kind::Hello && route_.world)
    return false;
  if (header.kind != s::Kind::Hello &&
      (header.connection != route_.connection || header.route != route_.epoch))
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
    auto agreed = s::Negotiate(Capabilities(config_), At(m.body, "capabilities"));
    assembler_.Configure(Agreed(agreed));
    route_.negotiated = true;
    route_.zstd = Zstd(agreed);
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
    reply_ = true;
    last_receive_ = now;
    return false;
  }
  if (!nonce_ || m.connection != route_.connection || m.route != route_.epoch)
    Fail("live_stale_connection");
  last_receive_ = now;
  if (m.kind == s::Kind::Ready) {
    if (N(m.body, "client_nonce") != nonce_ || N(m.body, "server_nonce") != server_nonce_)
      Fail("live_ready_identity");
    if (m.sequence > client_control_) {
      client_control_ = m.sequence;
      echo_client_ms_ = N(m.body, "sent_ms");
      echo_server_ms_ = now;
      established_ = true;
    }
    return false;
  }
  if (m.kind == s::Kind::OperationAck) {
    if (N(m.body, "operation") == unsigned(s::Kind::Range)) {
      const auto phase=publication_.State();if(phase==PublicationSender::Phase::Recovering||phase==PublicationSender::Phase::Preparing||phase==PublicationSender::Phase::Stopped||phase==PublicationSender::Phase::WaitingWorld)return false;
      if (!publication_.NativeProgress(m, now))
        Fail("live_native_progress_scope");
    } else
      operations_.Acknowledge(m, now);
    return false;
  }
  if (m.kind == s::Kind::ReceiveAck) {
    publication_.Acknowledge(m, now);
    return false;
  }
  if (m.kind == s::Kind::ScenePrepared) {
    const auto &baseline = publication_.Baseline();
    if (!baseline || publication_.State() != PublicationSender::Phase::Preparing ||
        ReadWorld(m.body["world"]) != baseline->world ||
        N(m.body, "manifest") != baseline->manifest->revision ||
        N(m.body, "generation") != prepare_generation_||N(m.body,"native_domains")!=baseline->manifest->native_domains||N(m.body,"retained_domains")!=baseline->manifest->retained_domains)
      Fail("live_prepared_identity");
    // 确认在旧路由返回；Client收到同值echo才切换，丢包由小消息重复完成。
    if(m.sequence!=prepare_sequence_)Fail("live_prepared_sequence");
    prepare_replay_.Authorize(route_);prepared_echo_=std::make_shared<const s::Message>(std::move(m));echo_pending_=true;
    route_.epoch = Next(route_.epoch);
    route_.world = true;
    route_.scope = baseline->world;
    scope_ = s::Connected(route_.scope, route_.connection);
    assembler_.Authorize(route_);
    operations_.Bind(route_.connection, route_.epoch);
    auto outbound = route_;
    outbound.direction = s::Direction::ToClient;
    publication_.Authorize(outbound, now);
    return false;
  }
  if (m.kind == s::Kind::AppliedAck || m.kind == s::Kind::ContextReady) {
    auto message = std::make_shared<const s::Message>(std::move(m));
    if (!operations_.Admit(message, now))
      return false;
    const auto phase=publication_.State();
    if(phase==PublicationSender::Phase::Recovering||phase==PublicationSender::Phase::Preparing||phase==PublicationSender::Phase::Stopped||phase==PublicationSender::Phase::WaitingWorld){operations_.Finish({message->kind,message->sequence},OperationStage::Cancelled,"world_confirmation_retired",now);return false;}
    const bool accepted = message->kind == s::Kind::AppliedAck
                              ? publication_.Applied(*message,now)
                              : publication_.ContextReady(*message, now);
    operations_.Finish(
        {message->kind, message->sequence},
        accepted ? OperationStage::Complete : OperationStage::Failed,
        accepted ? "range_confirmation_recorded" : "range_confirmation_unsent_or_scope", now);
    return false;
  }
  if (Operations::Reliable(m.kind)) {
    const auto kind = m.kind;
    const auto sequence = m.sequence;
    const auto cancel = kind == s::Kind::Cancel ? N(m.body, "transfer") : 0;
    const auto recovery_reason=kind==s::Kind::Resync?String(m.body["reason"]):std::string{};
    if(kind==s::Kind::Resync&&ReadWorld(m.body["world"])!=route_.scope)Fail("live_resync_world");
    if (!operations_.Admit(std::make_shared<const s::Message>(std::move(m)), now))
      return false;
    if (cancel) {
      assembler_.Cancel(cancel);
      publication_.Cancel(cancel);
    }
    if (kind == s::Kind::Resync) {
      RequestRecovery(recovery_reason.c_str(),now,recovery_reason.rfind("native:",0)==0);
    } else if (kind == s::Kind::Stop || kind == s::Kind::Disconnect || kind == s::Kind::Retire) {
      stopped_ = true;
      publication_.Stop();
    }
    operations_.Finish({kind, sequence}, OperationStage::Complete, "network_operation_complete",
                       now);
    return false;
  }
  if(m.kind==s::Kind::Input&&(!route_.world||publication_.State()!=PublicationSender::Phase::Active))return false;
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
Packets HostPeer::Progress(const protocol::Ack &p) {
  const auto now = GetTickCount64();
  Packets packets;
  publication_.AuthorityPaused(p.authority_paused,now);
  const bool enabled=p.input_enabled&&route_.world&&publication_.State()==PublicationSender::Phase::Active;
  const bool changed = p.authority_paused!=sent_.authority_paused || p.generation != sent_.generation || enabled != sent_.input_enabled ||
                       p.session_state != sent_.session_state || p.roster_mask != sent_.roster_mask;
  const bool tick = p.server_tick != sent_.server_tick;
  sent_ = p;
  sent_.input_enabled=enabled;
  if (changed || !eligibility_revision_)
    Next(eligibility_revision_);
  sent_.eligibility_revision = eligibility_revision_;
  const auto append = [&](Packets more) {
    packets.insert(packets.end(), std::make_move_iterator(more.begin()),
                   std::make_move_iterator(more.end()));
  };
  if (!roster_at_ || changed || now - roster_at_ >= (1000+config_.roster_hz-1)/config_.roster_hz) {
    append(Pack(Roster(sent_, ordinal_, Next(roster_), route_, echo_client_ms_, echo_server_ms_)));
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
    N(m.body, "send_ms", now, a);
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
Packets HostPeer::Bye(const char *reason) {
  if (!nonce_ || ending_)
    return {};
  ending_ = true;
  operations_.CancelPending(GetTickCount64());
  auto m = Message(s::Kind::Disconnect, Next(control_));
  r::Put(m.body, "reason", std::string(reason), m.body.GetAllocator());
  operations_.Queue(std::make_shared<const s::Message>(std::move(m)), GetTickCount64());
  publication_.Stop();
  return {};
}
Packets HostPeer::Reject(const char *reason) {
  auto m = Message(s::Kind::Error, Next(control_));
  auto &a = m.body.GetAllocator();
  r::Put(m.body, "code", std::string(reason), a);
  r::Put(m.body, "recoverable", r::Value(false), a);
  r::Put(m.body, "detail", std::string("Client negotiation or connection rejected"), a);
  return Pack(m);
}
void HostPeer::RequestRecovery(const char *reason,std::uint64_t now,bool native) {
  if(first_failure_.empty())first_failure_=reason;
  if(native&&++native_failures_>1)Fail("live_native_recovery_exhausted");
  if(!recovering_){recovering_=true;recovery_started_=now;resync_=true;publication_.FailPublication(reason);}
}
void HostPeer::Offer(const Publication &p, std::uint64_t now) {
  if(ending_||stopped_)return;
  const bool had = bool(publication_.Baseline());
  // 来源退休/换场景必须撤销路由；普通更新期间暂缺新候选只撤新目标资格。
  // 网络已经持有的不可变冷目标继续按原sample传输，不能因每帧dirty反复取消。
  if (!p.source[0] || (source_[0] && source_!=p.source)) {
    publication_.Stop();
    if (had) {
      operations_.CancelPending(now);
      auto m = Message(s::Kind::Retire, Next(control_));
      r::Put(m.body, "reason", std::string("host_source_withdrawn"), m.body.GetAllocator());
      operations_.Queue(std::make_shared<const s::Message>(std::move(m)), now);
    }
  }
  source_=p.source;
  if(!source_[0])return;
  publication_.Offer(p.baseline, p.latest, p.bodies);
  if (established_ && sent_.generation &&
      publication_.State() == PublicationSender::Phase::WaitingWorld && publication_.Baseline() &&
      publication_.Baseline()->manifest) {
    auto m = Message(s::Kind::ScenePrepare, Next(control_));
    auto &a = m.body.GetAllocator();
    PutWorld(m.body, publication_.Baseline()->world, a);
    N(m.body, "next_route", route_.epoch + 1, a);
    N(m.body, "manifest", publication_.Baseline()->manifest->revision, a);
    N(m.body, "generation", sent_.generation, a);
    N(m.body,"native_domains",publication_.Baseline()->manifest->native_domains,a);N(m.body,"retained_domains",publication_.Baseline()->manifest->retained_domains,a);r::Put(m.body,"contract","complete-ranges-schema-v1",a);
    prepare_sequence_ = m.sequence;
    prepare_generation_=sent_.generation;
    operations_.Queue(std::make_shared<const s::Message>(std::move(m)), now);
    publication_.Preparing(now);
  }
}
unsigned HostPeer::Pump(std::uint64_t now, EncodingCache &cache,
                        const s::ReliableSender::Send &send) {
  const auto transmit = [&](const s::Message &m) {
    auto packets = Pack(m);
    if (packets.size() != 1)
      Fail("live_control_single_datagram");
    const auto result = send(packets.front());
    if (result == s::SendResult::Failed)
      Fail("live_socket_send");
    return result == s::SendResult::Sent;
  };
  if(echo_pending_&&prepared_echo_&&transmit(*prepared_echo_))echo_pending_=false;
  operations_.Pump(now, transmit);
  if (!operations_.Failure().empty())
    Fail(operations_.Failure().c_str());
  if(recovering_){if(publication_.State()==PublicationSender::Phase::Active)recovering_=false;else if(now<recovery_started_||now-recovery_started_>=config_.absolute_timeout_ms)Fail("live_recovery_deadline");}
  unsigned sent = 0;
  try {
    sent = publication_.Pump(now, transfer_, cache, send);
  } catch (const std::exception &error) {
    publication_.FailPublication(error.what());
  } catch (...) {
    publication_.FailPublication("publication_exception");
  }
  if (!publication_.Failure().empty() &&
      publication_.State() == PublicationSender::Phase::Recovering)
  {if(!recovering_){RequestRecovery(publication_.Failure().c_str(),now);
    auto retire=Message(s::Kind::Retire,Next(control_));r::Put(retire.body,"reason",std::string("host_publication_recovery"),retire.body.GetAllocator());operations_.Queue(std::make_shared<const s::Message>(std::move(retire)),now);
  }}
  return sent;
}
std::uint64_t HostPeer::NextWake(std::uint64_t now) const noexcept {
  return (std::min)(operations_.NextWake(now), publication_.NextWake(now));
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
  std::deque<Event> handoff;
  SpscQueue<Intent, 256> intents;
  OwnedQueue<Event, 32> events, world_controls;
  OwnedQueue<Event, 4> urgent_controls;
  OwnedQueue<Event, 2> reliable_world;
  Latest<Event> timing_event, roster_event, input_ack_event;
  std::mutex dynamic_mutex;
  std::map<s::RangeId, Event> dynamic;
  Event terminal_event;
  std::atomic<bool> terminal_ready{false};
  bool terminal_delivered = false;
  struct Command {
    enum class Type { Send, Complete, Prepare, Suspend } type = Type::Send;
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
  r::Scope scope{};
  unsigned ordinal = 0;
  std::uint64_t nonce = 0, server_nonce = 0, transfer = 0, control = 0, last_receive = 0,
                last_send = 0, roster_sequence = 0, tick_sequence = 0;
  std::uint64_t input_ack_generation = 0, input_ack_sequence = 0, input_ack_tick = 0;
  std::uint32_t timeout = 3000;
  bool welcomed = false, have_progress = false;
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
  void Ready() {
    auto m = Make(s::Kind::Ready, Next(control), route.connection, route.epoch);
    auto &a = m.body.GetAllocator();
    N(m.body, "client_nonce", nonce, a);
    N(m.body, "server_nonce", server_nonce, a);
    N(m.body, "sent_ms", GetTickCount64(), a);
    SendMessage(m);
  }
  void Receive(s::Message m) {
    const auto now = GetTickCount64();
    if (m.kind == s::Kind::Error)
      Fail(String(m.body["code"]).c_str());
    if (m.kind == s::Kind::Welcome) {
      if (N(m.body, "client_nonce") != nonce)
        Fail("live_welcome_nonce");
      const auto participant = N(m.body, "participant"), server = N(m.body, "server_nonce");
      if (welcomed &&
          (route.connection != m.connection || ordinal != participant || server_nonce != server))
        Fail("live_welcome_conflict");
      auto agreed = s::Negotiate(Capabilities(config), m.body["capabilities"]);
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
        route.config=config;route.fast_limits=s::NegotiatedFastLimits(agreed);
        assembler.Authorize(route);
        operations.Bind(route.connection, route.epoch);
        published_connection.store(route.connection, std::memory_order_release);
        Event e;
        e.type = Event::Type::Welcome;
        e.scope = scope;
        e.route = route.epoch;
        e.ordinal = ordinal;
        Emit(e);
      }
      last_receive = now;
      Ready();
      return;
    }
    if (!welcomed || m.connection != route.connection || m.route != route.epoch)
      Fail("live_server_scope");
    if (terminal_ready.load(std::memory_order_acquire) && m.kind != s::Kind::OperationAck &&
        m.kind != s::Kind::Stop && m.kind != s::Kind::Disconnect && m.kind != s::Kind::Cancel)
      return;
    last_receive = now;
    if (m.kind == s::Kind::OperationAck) {
      operations.Acknowledge(m, now);
      return;
    }
    if(m.kind==s::Kind::ScenePrepared){
      if(!pending_prepare||!prepared_response||m.sequence!=prepared_response->sequence||m.body!=prepared_response->body)Fail("live_prepare_confirmation");
      Revoke();Intent old;while(intents.TryPop(old)){};
      route.epoch=N(pending_prepare->body,"next_route");route.world=true;route.scope=ReadWorld(pending_prepare->body["world"]);scope=s::Connected(route.scope,route.connection);
      accept_world=true;metrics.Bind(route.scope,route.epoch);
      assembler.Authorize(route);operations.Bind(route.connection,route.epoch);have_progress=false;progress={};roster_sequence=tick_sequence=0;input_ack_generation=input_ack_sequence=input_ack_tick=0;
      // 主线程是值队列唯一consumer；旧路由由Poll/PollWorld过滤并释放。
      {std::lock_guard lock(dynamic_mutex);dynamic.clear();}
      published_route.store(route.epoch,std::memory_order_release);Event activated;activated.type=Event::Type::Prepared;activated.scope=scope;activated.route=route.epoch;activated.ordinal=ordinal;activated.world=pending_prepare;
      // 路由激活与退休共用有序控制队列，先于其后的Range交付；不能跨两个队列竞速。
      if(!urgent_controls.TryPush(std::move(activated)))Fail("live_prepare_activation_capacity");
      pending_prepare.reset();prepared_response.reset();return;
    }
    Event e;
    e.scope = scope;
    e.route = route.epoch;
    e.ordinal = ordinal;
    e.received_ms = now;
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
      roster_sequence = m.sequence;
      roster_event.Put(std::make_shared<const Event>(std::move(e)));
      return;
    }
    if (m.kind == s::Kind::TickProgress) {
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
      if (generation == input_ack_generation && sequence == input_ack_sequence &&
          tick != input_ack_tick)
        Fail("live_input_ack_conflict");
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
      if(kind==s::Kind::ScenePrepare){Revoke();progress.input_enabled=false;have_progress=false;}
      if (cancel)
        assembler.Cancel(cancel);
      if (kind == s::Kind::Stop || kind == s::Kind::Disconnect) {
        Revoke();
        End(Event::Type::Stopped, "server_disconnected");
        operations.Finish({kind, sequence}, OperationStage::Complete, "network_stopped", now);
      } else if (kind == s::Kind::Cancel)
        operations.Finish({kind, sequence}, OperationStage::Complete, "transfer_cancelled", now);
      else if (kind == s::Kind::Retire) {
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
    if (kind == s::Kind::Range) {
      if(!accept_world)return;
      if(e.world->range){metrics.Note(*e.world->range,s::FlowStage::Receive,now);if(!metrics.first_receive_ms)metrics.first_receive_ms=now;}
      e.type = Event::Type::World;
      if (s::Reliable(s::MessageLane(*e.world))) {
        if (!reliable_world.TryPush(e)){
          if(handoff.size()>=2)Fail("live_world_dependency_capacity");
          handoff.push_back(std::move(e));
        }
      } else {
        std::lock_guard lock(dynamic_mutex);
        if (!dynamic.contains(e.world->range->id) && dynamic.size() >= config.dynamic_candidates)
          Fail("live_dynamic_scope_capacity");
        if(dynamic.contains(e.world->range->id))++metrics.replacements;
        dynamic.insert_or_assign(e.world->range->id, std::move(e));
      }
      return;
    }
    Fail("live_unexpected_world_message");
  }
  void ReceivePacket(std::span<const std::uint8_t> packet) {
    const auto h = s::ReadHeader(packet, s::WorldLimits());
    if(!accept_world&&h.kind==s::Kind::Range)return;
    if (welcomed && (h.connection != route.connection || h.route != route.epoch))
      return;
    if (welcomed && h.kind == s::Kind::Welcome) {
      last_receive = GetTickCount64();
      Ready();
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
      if(c.type==Command::Type::Suspend){handoff.clear();Revoke();Intent old;while(intents.TryPop(old)){}accept_world=false;progress.input_enabled=false;have_progress=false;operations.CancelPending(now);pending_send.clear();auto closed=route;closed.world=false;assembler.Authorize(closed);{std::lock_guard lock(dynamic_mutex);dynamic.clear();}continue;}
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
      if (!c.message || c.message->connection != route.connection ||
          c.message->route != route.epoch)
        continue;
      if (Operations::Reliable(c.message->kind))
        operations.Queue(c.message, now);
      else
        SendMessage(*c.message);
    }
  }
  void Run() noexcept {
    try {
      auto hello = Make(s::Kind::Hello, 1, 0, 0);
      auto &a = hello.body.GetAllocator();
      Object(hello.body, "capabilities", Capabilities(config), a);
      N(hello.body, "nonce", nonce, a);
      r::Put(hello.body, "role", std::string("Client"), a);
      last_receive = GetTickCount64();
      std::uint64_t ending = 0;
      for (;;) {
        const auto now = GetTickCount64();
        s::Measure network_measure(metrics,s::CpuStage::Network);
        if (stop.load(std::memory_order_acquire) && !ending) {
          if (!welcomed)
            break;
          Revoke();accept_world=false;handoff.clear();auto closed=route;closed.world=false;assembler.Authorize(closed);
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
        while(!handoff.empty()){if(!reliable_world.TryPush(handoff.front()))break;handoff.pop_front();}
        Commands(now);
        if(prepared_response&&now>=prepare_due){if(now<prepare_started||now-prepare_started>=config.progress_timeout_ms)Fail("live_prepare_no_confirmation");SendMessage(*prepared_response);prepare_due=now+config.final_retry_ms;}
        if (terminal_ready.load(std::memory_order_acquire)) {
          if (!ending)
            ending = now;
          Revoke();
          if (now - ending >= config.progress_timeout_ms || (now - ending >= 100 && operations.Settled()))
            break;
        } else if (now - last_receive >= timeout)
          Fail("live_peer_timeout");
        if (now - last_send >= config.heartbeat_ms && !ending) {
          if (welcomed)
            Ready();
          else
            SendMessage(hello);
          last_send = now;
        }
        operations.Pump(now, [&](const s::Message &m) {
          const auto packets = s::Fragment(m, Next(transfer), Limits());
          if (packets.size() != 1)
            Fail("live_control_single_datagram");
          const auto result = SocketSend(packets.front());
          if (result == s::SendResult::Failed)
            Fail("live_client_send");
          if(result==s::SendResult::Sent&&m.kind==s::Kind::AppliedAck){try{for(const auto &v:m.body["versions"].GetArray()){s::RangeData stamp;stamp.id.kind=s::RangeKind(N(v,"kind"));stamp.id.name=String(v["name"]);if(!v["body"].IsNull())stamp.id.body={N(v["body"],"run"),N(v["body"],"id"),unsigned(N(v["body"],"space"))};stamp.revision=N(v,"revision");for(const auto &f:metrics.flows)if(f.used&&f.kind==unsigned(stamp.id.kind)&&f.body==stamp.id.body&&stamp.id.name==f.name.data()&&f.sequence[unsigned(s::FlowStage::Receive)]==stamp.revision){stamp.sample.tick=f.tick[unsigned(s::FlowStage::Receive)];stamp.sample.kind=s::CaptureKind(f.authority_kind[unsigned(s::FlowStage::Receive)]);break;}metrics.Note(stamp,s::FlowStage::Ack,now);}}catch(...){++metrics.missing;}}
          return result == s::SendResult::Sent;
        });
        if (!operations.Failure().empty())
          Fail(operations.Failure().c_str());
        for (const auto &ack : assembler.Acks(now))
          SendMessage(s::AckMessage(ack, Next(control), route));
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
        metrics.Queue(pending_send.size()+handoff.size()+world_controls.Size()+reliable_world.Size()+commands.Size());metrics.candidates=assembler.Pending();
        if(now>=metrics_at){metrics_mailbox.Publish(metrics);transmit_mailbox.Publish(transmit_metrics);metrics_at=now+50;}
        if(transfer_mutex.try_lock()){transfer_status=assembler.Statistics();effective_limits=route.world_limits;effective_fast_limits=route.fast_limits;transfer_mutex.unlock();}
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(socket, &reads);
        const auto wake =
            (std::min)(now + config.bundle_wait_ms, (std::min)(operations.NextWake(now), assembler.NextWake(now)));
        timeval wait{0, long((wake > now ? wake - now : 1) * 1000)};
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
      }
      Revoke();
      pending_send.clear();
    } catch (const std::exception &error) {
      End(Event::Type::Failed, error.what());
    } catch (...) {
      End(Event::Type::Failed, "live_client_exception");
    }
    Revoke();pending_send.clear();handoff.clear();assembler.Authorize({});operations.Bind(0,0);pending_prepare.reset();prepared_response.reset();
    {std::lock_guard lock(dynamic_mutex);dynamic.clear();}
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
                          std::uint32_t timeout_ms,s::RuntimeConfig config) noexcept {
  if (*config.Validate())return config.Validate();
  if(config.peer_timeout_ms!=timeout_ms)return "live_timeout_configuration_conflict";
  if (state_ || !port || timeout_ms < 500 || timeout_ms > 60000)
    return "live_configuration";
  try {
    auto state = std::make_shared<State>();
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
bool Client::SuspendWorld() noexcept {if(!state_||Finished())return false;RevokeInput();State::Command c;c.type=State::Command::Type::Suspend;return state_->commands.TryPush(std::move(c));}
r::Limits Client::EffectiveLimits() const noexcept {if(!state_)return {};std::lock_guard lock(state_->transfer_mutex);return state_->effective_limits;}
r::Limits Client::EffectiveFastLimits() const noexcept {if(!state_)return {};std::lock_guard lock(state_->transfer_mutex);return state_->effective_fast_limits;}
s::Metrics Client::TransmitMetrics() const noexcept {return state_?state_->transmit_mailbox.Read():s::Metrics{};}
s::Metrics Client::Metrics() const noexcept {return state_?state_->metrics_mailbox.Read():s::Metrics{};}
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
    bool have = state_->urgent_controls.TryPop(e) || state_->world_controls.TryPop(e) ||
                state_->reliable_world.TryPop(e);
    if (!have) {
      std::lock_guard lock(state_->dynamic_mutex);
      if (state_->dynamic.empty())
        return false;
      auto i = state_->dynamic.begin();
      e = std::move(i->second);
      state_->dynamic.erase(i);
    }
    if (e.scope.connection == state_->published_connection.load(std::memory_order_acquire) &&
        e.route == state_->published_route.load(std::memory_order_acquire))
      return true;
    e = {};
  }
  return false;
}
bool Client::SendControl(std::shared_ptr<const s::Message> m) noexcept {
  if (!state_ || state_->stop.load(std::memory_order_acquire) || Finished() || !m)
    return false;
  State::Command c;
  c.message = std::move(m);
  return state_->commands.TryPush(std::move(c));
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

