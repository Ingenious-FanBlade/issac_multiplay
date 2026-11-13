// Winsock 必须先于 Windows 头；网络库的宏和 include 不传播给 core/SDK。
// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
// clang-format on
#include "net/worker.h"
#include "net/endpoint.h"
#include "net/queue.h"
#include "net/live.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <new>

namespace cs::net {
namespace {
using namespace protocol;
constexpr std::uint32_t kPollMs = 10, kReceiveBudget = 64, kSendBudget = 64;
constexpr std::size_t kRttHistory = 64;
constexpr std::uint32_t kByeCopies = 3;
constexpr std::uint32_t kTerminalDrainMs = 100;
TerminalReason TerminalFor(ByeReason reason) noexcept {
  switch (reason) {
  case ByeReason::Stop:
    return TerminalReason::Stop;
  case ByeReason::PeerBye:
    return TerminalReason::PeerBye;
  case ByeReason::PeerTimeout:
    return TerminalReason::PeerTimeout;
  case ByeReason::PeerProtocol:
    return TerminalReason::PeerProtocol;
  case ByeReason::RunTeardown:
    return TerminalReason::RunTeardown;
  case ByeReason::InternalFailure:
    return TerminalReason::InternalFailure;
  case ByeReason::PeerLost:
    return TerminalReason::PeerLost;
  }
  return TerminalReason::InternalFailure;
}
bool SameAddress(const sockaddr_in &a, const sockaddr_in &b) noexcept {
  return a.sin_family == b.sin_family &&
         a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}
bool Nonzero(const Id &id) noexcept {
  return std::any_of(id.begin(), id.end(), [](auto b) { return b != 0; });
}
} // namespace

// socket、Endpoint 与 RTT 环只有网络线程使用；跨线程只共享队列和原子发布值。
struct Worker::State {
  WorkerConfig config{};
  HostIdentity identity{};
  SOCKET socket = INVALID_SOCKET;
  bool wsa = false, locked = false, owns_transport = true;
  unsigned ordinal = 0;
  // 父状态独占 socket 和线程；三个子状态各自独占协议、队列、nonce 和 ACK 环。
  std::array<std::unique_ptr<State>, core::kMaximumParticipants> peers{};
  sockaddr_in peer{};
  Endpoint endpoint{};
  // ICSR与历史输入工具共用监听socket，但每槽只选择一种协议与独立身份。
  std::unique_ptr<live::HostPeer> game_peer;
  struct QueuedGame final {memory::Lease memory;std::vector<std::uint8_t> bytes;};
  std::deque<QueuedGame> game_outgoing;
  live::Uplink uplink_owner;
  live::Uplink *uplink=nullptr;
  live::EncodingCache encoding_owner;
  live::EncodingCache *encoding_cache=nullptr;
  Latest<live::Publication> publications;
  std::shared_ptr<const live::Publication> current_publication;
  std::atomic<std::uint64_t> uplink_wire{0},uplink_blocks{0};
  std::atomic<unsigned> publication_phase{0};
  std::array<std::atomic<std::uint64_t>,6> binary_progress{};
  std::atomic<bool> stream_stale{false};
  replica::stream::MetricsMailbox stream_metrics,encoder_metrics;
  std::uint64_t metrics_at=0;
  replica::Limits stream_limits,stream_fast_limits;
  mutable std::mutex stream_status_mutex;
  replica::stream::TransferStatistics stream_transfer{};
  std::atomic<std::uint64_t> uplink_packets{0},uplink_payload{0},uplink_messages{0};
  std::array<std::atomic<std::uint64_t>,4> operation_statistics{};
  std::array<char,128> replication_first_failure{};
  std::atomic<bool> have_replication_failure{false};
  std::array<std::atomic<std::uint64_t>,4> uplink_lanes{},uplink_lane_packets{};
  bool game_candidate = false;
  std::uint64_t candidate_since = 0;
  SpscQueue<ReceivedInput, kReceiveCapacity> incoming;
  SpscQueue<Ack, kSendCapacity> outgoing;
  std::atomic<std::uint32_t> stop{0};
  std::atomic<bool> running{false}, published_locked{false};
  std::atomic<PeerState> published_state{PeerState::Dormant};
  std::atomic<TerminalReason> terminal{TerminalReason::None};
  std::atomic<Error> protocol_error{Error::None};
  // 先写诊断时刻、再 release 终态；只复制值给主线程。
  std::atomic<std::uint64_t> terminal_at_ms{0};
  std::atomic<ByeReason> peer_bye{ByeReason::Stop};
  std::atomic<std::uint32_t> native_error{0}, last_send_error{0}, peer_ip{0},
      peer_port{0};
  std::atomic<std::uint64_t> received_packets{0}, sent_packets{0},
      received_bytes{0}, sent_bytes{0}, valid_packets{0}, duplicate_packets{0},
      stale_packets{0}, bye_send_attempts{0}, bye_sent_packets{0},
      foreign_packets{0}, receive_errors{0}, send_errors{0}, last_receive_ms{0},
      rtt_samples{0}, rtt_last_ms{0}, rtt_max_ms{0}, wait_timeouts{0};
  std::atomic<std::uint64_t> terminal_received_packets{0};
  std::array<std::atomic<std::uint64_t>, static_cast<std::size_t>(Error::Count)>
      errors{};
  struct AckStamp {
    std::uint64_t sequence = 0, sent_ms = 0;
    std::uint64_t generation = 0;
    core::SessionState state = core::SessionState::Warmup;
  };
  std::array<AckStamp, kRttHistory> ack_stamps{};
  std::size_t stamp_cursor = 0;
  std::uint64_t echoed_ack = 0, last_sent_ms = 0;
  Ack heartbeat{};

  ~State() { CloseTransport(); }
  void CloseTransport() noexcept {
    if (owns_transport && socket != INVALID_SOCKET) {
      closesocket(socket);
      socket = INVALID_SOCKET;
    }
    if (owns_transport && wsa) {
      WSACleanup();
      wsa = false;
    }
  }
  void CountError(Error error) noexcept {
    // 正常 UDP 重复与迟到不是错误；只进各自的独立计数，保留真正的协议错误统计。
    if (error == Error::None || error == Error::Duplicate ||
        error == Error::Stale)
      return;
    const auto index = static_cast<std::size_t>(error);
    if (index < errors.size())
      errors[index].fetch_add(1, std::memory_order_relaxed);
  }
  void Finish(ByeReason reason, Error error = Error::None,
              std::uint32_t native = 0) noexcept {
    // 只有工作线程发布 terminal；先发布详细原因，terminal 的 release
    // 是通知边界。
    if (terminal.load(std::memory_order_relaxed) != TerminalReason::None)
      return;
    protocol_error.store(error, std::memory_order_relaxed);
    native_error.store(native, std::memory_order_relaxed);
    terminal_at_ms.store(GetTickCount64(), std::memory_order_relaxed);
    terminal.store(TerminalFor(reason), std::memory_order_release);
  }
  // 首因一次发布，后续停止/重试只能增加计数，不能覆盖诊断原值。
  void ReplicationFailure(const char *reason) noexcept {
    if(have_replication_failure.load(std::memory_order_relaxed))return;
    strncpy_s(replication_first_failure.data(),replication_first_failure.size(),reason,_TRUNCATE);
    have_replication_failure.store(true,std::memory_order_release);
  }
  void StampAck(std::uint64_t sequence, std::uint64_t now,
                const Ack &ack) noexcept {
    ack_stamps[stamp_cursor] = {sequence, now, ack.generation,
                                ack.session_state};
    stamp_cursor = (stamp_cursor + 1) % ack_stamps.size();
  }
  // Ack 发出到 Input 回显之间的墙钟间隔仅是 RTT 估计，包含远端发送等待。
  void ObserveEcho(std::uint64_t sequence, std::uint64_t now) noexcept {
    if (!sequence || sequence <= echoed_ack)
      return;
    echoed_ack = sequence;
    for (const auto &stamp : ack_stamps) {
      if (stamp.sequence != sequence || now < stamp.sent_ms)
        continue;
      const auto elapsed = now - stamp.sent_ms;
      rtt_samples.fetch_add(1, std::memory_order_relaxed);
      rtt_last_ms.store(elapsed, std::memory_order_relaxed);
      rtt_max_ms.store(
          (std::max)(rtt_max_ms.load(std::memory_order_relaxed), elapsed),
          std::memory_order_relaxed);
      return;
    }
  }
  // 非阻塞 UDP；只在完整 datagram 发出后提交序号，不为失败 sendto
  // 伪造发送成功。
  bool RawSendPacket(const Packet &packet, std::uint64_t now,
                  bool ending = false) noexcept {
    const auto n =
        sendto(socket, reinterpret_cast<const char *>(packet.bytes.data()),
               static_cast<int>(packet.size), 0,
               reinterpret_cast<const sockaddr *>(&peer), sizeof(peer));
    if (n != static_cast<int>(packet.size)) {
      send_errors.fetch_add(1, std::memory_order_relaxed);
      const auto error = n == SOCKET_ERROR ? WSAGetLastError() : WSAEMSGSIZE;
      last_send_error.store(error, std::memory_order_relaxed);
      // Bye 副本仍须尝试余下发送；已记录的结束原因和 Endpoint
      // 待提交状态都保留。
      if (ending)
        return false;
      native_error.store(error, std::memory_order_relaxed);
      // 背压与 ICMP 只计数并等待下一次输入/心跳；失效 socket
      // 等内部错误结束线程。
      if (error != WSAEWOULDBLOCK && error != WSAENOBUFS &&
          error != WSAECONNRESET && error != WSAENETRESET) {
        Finish(ByeReason::InternalFailure, Error::None, error);
        endpoint.Fail();
      }
      return false;
    }
    sent_packets.fetch_add(1, std::memory_order_relaxed);
    sent_bytes.fetch_add(packet.size, std::memory_order_relaxed);
    last_sent_ms = now;
    return true;
  }
  // 所有数据报在真实sendto成功后扣同一套总/Peer/lane令牌，背压不忙等。
  bool SendPacket(const Packet &packet,std::uint64_t now,bool ending=false) noexcept {
    if(!uplink)return RawSendPacket(packet,now,ending);
    try {
      const auto result=uplink->Transmit(ordinal,packet.View(),game_peer&&game_peer->Initializing(),now,[&](auto){
        if(RawSendPacket(packet,now,ending))return replica::stream::SendResult::Sent;
        return terminal.load(std::memory_order_acquire)==TerminalReason::InternalFailure?replica::stream::SendResult::Failed:replica::stream::SendResult::WouldBlock;
      });
      const auto &stats=uplink->Status(ordinal);uplink_packets.store(stats.packets);uplink_payload.store(stats.payload_bytes);uplink_messages.store(stats.bundle_messages);uplink_wire.store(stats.wire_bytes,std::memory_order_relaxed);uplink_blocks.store(stats.blocked,std::memory_order_relaxed);for(unsigned i=0;i<4;++i){uplink_lanes[i].store(stats.lanes[i],std::memory_order_relaxed);uplink_lane_packets[i].store(stats.lane_packets[i]);}
      return result==replica::stream::SendResult::Sent;
    }catch(...){Finish(ByeReason::InternalFailure,Error::Payload);return false;}
  }
  bool SendMessage(const Message &message, std::uint64_t now) noexcept {
    Packet packet{};
    const auto prepared = endpoint.Prepare(message, packet);
    if (prepared != Error::None) {
      CountError(prepared);
      Finish(ByeReason::InternalFailure, prepared);
      return false;
    }
    if (!SendPacket(packet, now)) {
      endpoint.CancelPending();
      return false;
    }
    const auto committed = endpoint.CommitSent(packet);
    if (committed != Error::None) {
      CountError(committed);
      Finish(ByeReason::InternalFailure, committed);
      return false;
    }
    published_state.store(endpoint.State(), std::memory_order_release);
    if (message.header.kind == Kind::Ack)
      StampAck(endpoint.LastSentSequence(), now, message.ack);
    return true;
  }
  void FirstAck(std::uint64_t now) noexcept {
    Message m{};
    m.header.kind = Kind::Ack;
    m.ack.purpose = AckPurpose::First;
    m.ack.client_id = protocol::kClients[ordinal];
    SendMessage(m, now);
  }
  void BestEffortBye(ByeReason reason, std::uint64_t now) noexcept {
    if (game_peer) {
      try { game_outgoing.clear();SendGame(game_peer->Bye(ByeReasonName(reason)), now); }
      catch (...) { CountError(Error::Payload); }
      published_state.store(PeerState::Retired, std::memory_order_release);
      return;
    }
    if (endpoint.State() != PeerState::Established)
      return;
    Finish(reason); // 发送前固定真实原因，后续 sendto 失败只能增加发送诊断。
    Message m{};
    m.header.kind = Kind::Bye;
    m.bye.reason = reason;
    Packet packet{};
    const auto prepared = endpoint.Prepare(m, packet);
    if (prepared != Error::None) {
      CountError(prepared);
      return;
    }
    // 只编码一次；第一次成功发送才提交一次，另外两份不再进入已退休的 Endpoint。
    // 即使首发/前两次失败，仍使用同一字节、同一序号完成全部三次尝试。
    bool commit_attempted = false;
    for (std::uint32_t copy = 0; copy < kByeCopies; ++copy) {
      bye_send_attempts.fetch_add(1, std::memory_order_relaxed);
      if (!SendPacket(packet, now, true))
        continue;
      bye_sent_packets.fetch_add(1, std::memory_order_relaxed);
      if (!commit_attempted) {
        commit_attempted = true;
        const auto committed = endpoint.CommitSent(packet);
        if (committed != Error::None)
          CountError(committed);
      }
    }
    endpoint.CancelPending(); // 三次都失败时也不留下未提交的包。
    published_state.store(endpoint.State(), std::memory_order_release);
  }

  // 正式ICSR数据报仍受1200字节限制；sendto失败不触及主线程对象。
  void FlushGame(std::uint64_t now) noexcept {
    for(unsigned i=0;i<kSendBudget&&!game_outgoing.empty();++i){const auto &bytes=game_outgoing.front().bytes;if(game_peer&&!game_peer->OwnsPacket(bytes)){game_outgoing.pop_front();continue;}Packet packet{};packet.size=bytes.size();std::copy(bytes.begin(),bytes.end(),packet.bytes.begin());if(!SendPacket(packet,now))return;game_outgoing.pop_front();}
  }
  // 临时socket反压保留尚未发送的完整数据报；部分排队失败具名收口，不悄悄丢掉后半包。
  void SendGame(const live::Packets &packets, std::uint64_t now) noexcept {
    try {
      if(packets.size()>256-game_outgoing.size())throw replica::ContractError("live_send_queue_full");
      for(const auto &bytes:packets){if(bytes.empty()||bytes.size()>kMaximumPacket)throw replica::ContractError("live_datagram_budget");auto lease=memory::Reserve(memory::Pool::Wire,bytes.size()+sizeof(QueuedGame)+64);game_outgoing.push_back({std::move(lease),bytes});}
      FlushGame(now);
    }catch(...){Finish(ByeReason::InternalFailure,Error::Payload);}
  }
  void ReceiveGame(std::span<const std::uint8_t> bytes, const sockaddr_in &source,
                   std::uint64_t now) noexcept {
    if(!game_peer&&terminal.load(std::memory_order_acquire)!=TerminalReason::None)return;
    try {
      // 一次先检查完整容器，再逐条交已有owner；所有条目共用已经绑定的实际endpoint。
      if(replica::stream::IsBundle(bytes)){auto workspace=memory::Reserve(memory::Pool::Workspace,8192);const auto entries=replica::stream::Unbundle(bytes);for(const auto &entry:entries)ReceiveGame(entry,source,now);return;}
      if (!game_peer) {
        std::uint64_t session = 0; std::memcpy(&session, identity.session.data(), sizeof(session));
        if (!session) session = 1;
        game_peer = std::make_unique<live::HostPeer>(session, ordinal,config.stream);
        game_candidate = true; candidate_since = now; peer = source;
      }
      ReceivedInput input{};
      const bool delivered = game_peer->Receive(bytes, now, input, identity);
      SendGame(game_peer->Reply(), now);
      if (game_peer->Nonce()) {
        locked = true; game_candidate = false;
        peer_ip.store(ntohl(peer.sin_addr.s_addr), std::memory_order_relaxed);
        peer_port.store(ntohs(peer.sin_port), std::memory_order_relaxed);
        published_locked.store(true, std::memory_order_release);
      }
      if(game_peer->Established()&&terminal.load(std::memory_order_acquire)==TerminalReason::None)published_state.store(PeerState::Established,std::memory_order_release);
      if (delivered&&terminal.load(std::memory_order_acquire)==TerminalReason::None) incoming.TryPush(input);
      if (game_peer->Stopped()) {
        Finish(ByeReason::PeerBye);
        published_state.store(PeerState::Retired, std::memory_order_release);
      }
      last_receive_ms.store(game_peer->LastReceive(), std::memory_order_relaxed);
      valid_packets.fetch_add(1, std::memory_order_relaxed);
    } catch (const std::exception &error) {
      ReplicationFailure(error.what());CountError(Error::Payload);
      try { if(game_peer)SendGame(game_peer->Reject(error.what()),now); }catch(...){}
      if (locked) { Finish(ByeReason::PeerProtocol, Error::Payload); BestEffortBye(ByeReason::PeerProtocol, now); }
      else { game_peer.reset(); game_candidate = false; }
    } catch (...) {
      Finish(ByeReason::InternalFailure,Error::Payload);
    }
  }

  // 已锁定地址之外的包一律在解析前丢弃，不能借错误报文结束合法对端的会话。
  void ReceiveOne(std::span<const std::uint8_t> bytes,
                  const sockaddr_in &source, std::uint64_t now) noexcept {
    if (locked && !SameAddress(source, peer)) {
      foreign_packets.fetch_add(1, std::memory_order_relaxed);
      return;
    }
    if (terminal.load(std::memory_order_acquire) != TerminalReason::None) {
      terminal_received_packets.fetch_add(1, std::memory_order_relaxed);
      return;
    }
    Message m{};
    const auto after_bye =
        terminal.load(std::memory_order_acquire) == TerminalReason::PeerBye;
    const auto error = endpoint.Receive(bytes, now, m);
    if (error == Error::Duplicate) {
      duplicate_packets.fetch_add(1, std::memory_order_relaxed);
      valid_packets.fetch_add(1, std::memory_order_relaxed);
      last_receive_ms.store(endpoint.LastReceiveMs(),
                            std::memory_order_relaxed);
      // Receive 已确认这是字节完全相同的合法旧包；不能再次投递其中的输入。
      if (bytes[6] == static_cast<std::uint8_t>(Kind::Hello)) {
        if (endpoint.State() == PeerState::Negotiating)
          FirstAck(now);
        else {
          Packet retry{};
          if (endpoint.RetryHandshake(retry) == Error::None)
            SendPacket(retry, now);
        }
      }
      return;
    }
    // 收到 Bye 后仅计数当前可读的原字节副本，不处理新内容、不回 Bye。
    if (after_bye)
      return;
    if (error == Error::Stale) {
      stale_packets.fetch_add(1, std::memory_order_relaxed);
      return; // 不刷新 last_receive_ms、不提交输入、不修改连接状态。
    }
    if (error != Error::None) {
      CountError(error);
      // 握手前的畸形/不兼容包只丢弃，不改变 Endpoint 或锁定来源。
      if (endpoint.State() == PeerState::Established) {
        Finish(ByeReason::PeerProtocol, error);
        BestEffortBye(ByeReason::PeerProtocol, now);
        endpoint.Fail();
      }
      return;
    }
    valid_packets.fetch_add(1, std::memory_order_relaxed);
    last_receive_ms.store(endpoint.LastReceiveMs(), std::memory_order_relaxed);
    if (m.header.kind == Kind::Hello) {
      peer = source;
      locked = true;
      peer_ip.store(ntohl(peer.sin_addr.s_addr), std::memory_order_relaxed);
      peer_port.store(ntohs(peer.sin_port), std::memory_order_relaxed);
      published_locked.store(true, std::memory_order_release);
      FirstAck(now);
    } else if (m.header.kind == Kind::Input) {
      ObserveEcho(m.input.last_ack, now);
      ReceivedInput received{m.input, m.header.sequence, now};
      for (const auto &stamp : ack_stamps) {
        if (stamp.sequence && stamp.sequence == m.input.last_ack) {
          received.acknowledged_generation = stamp.generation;
          received.acknowledged_state = stamp.state;
          // 命中成功发送的 Ack 记录；generation=0 仍是合法值（Warmup/未公布）。
          received.ack_stamp_known = true;
          break;
        }
      }
      incoming.TryPush(received);
    } else if (m.header.kind == Kind::Bye) {
      peer_bye.store(m.bye.reason, std::memory_order_relaxed);
      Finish(ByeReason::PeerBye);
    }
    published_state.store(endpoint.State(), std::memory_order_release);
  }
  // 单 socket 按已锁定地址分流；首次 Hello 按请求 ordinal 选槽，旧槽永不复用。
  void ReceiveBatch() noexcept {
    std::array<std::uint8_t, kMaximumPacket + 1> bytes{};
    for (unsigned n = 0;
         n < kReceiveBudget && !stop.load(std::memory_order_acquire); ++n) {
      sockaddr_in source{};
      int size = sizeof(source);
      const int count = recvfrom(socket, reinterpret_cast<char *>(bytes.data()),
                                 static_cast<int>(bytes.size()), 0,
                                 reinterpret_cast<sockaddr *>(&source), &size);
      if (count == SOCKET_ERROR) {
        const auto error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
          break;
        if (error == WSAEMSGSIZE || error == WSAECONNRESET ||
            error == WSAENETRESET) {
          receive_errors.fetch_add(1, std::memory_order_relaxed);
          continue;
        }
        Finish(ByeReason::InternalFailure, Error::None, error);
        break;
      }
      if (size != sizeof(source) || source.sin_family != AF_INET)
        continue;
      const auto packet = std::span<const std::uint8_t>(bytes.data(), std::size_t(count));
      const auto now = GetTickCount64();
      // 未完成的分片候选最多三个、每个64KiB；超时释放，不永久占用参与者。
      for (unsigned i=1;i<peers.size();++i) {
        auto &p=*peers[i];
        if (p.game_candidate && now-p.candidate_since>=1000) {
          p.game_peer.reset();p.game_candidate=false;
        }
      }
      State *target = nullptr;
      for (unsigned i = 1; i < peers.size(); ++i)
        if ((peers[i]->locked || peers[i]->game_candidate) && SameAddress(peers[i]->peer, source)) {
          target = peers[i].get();
          break;
        }
      if (live::IsPacket(packet)) {
        if (!target) for (unsigned i=1;i<peers.size();++i) {
          auto &p=*peers[i];
          if (!p.locked && !p.game_candidate && p.terminal.load(std::memory_order_acquire)==TerminalReason::None) {
            target=&p;break;
          }
        }
        if (target && (!target->locked || target->game_peer)) {
          target->received_packets.fetch_add(1,std::memory_order_relaxed);
          target->received_bytes.fetch_add(count,std::memory_order_relaxed);
          target->ReceiveGame(packet,source,now);
          if (target->game_peer && target->game_peer->Nonce())
            for (unsigned i=1;i<peers.size();++i)
              if (peers[i].get()!=target && peers[i]->game_peer &&
                  peers[i]->game_peer->Nonce()==target->game_peer->Nonce()) {
                target->Finish(ByeReason::PeerProtocol,Error::Identity);
                target->BestEffortBye(ByeReason::PeerProtocol,now);
              }
          // 只有完整协商/nonce互斥完成后才向主线程发布，不能先Join再撤销。
          if(target->game_peer&&target->game_peer->Established()&&
             target->terminal.load(std::memory_order_acquire)==TerminalReason::None)
            target->published_state.store(PeerState::Established,std::memory_order_release);
        } else foreign_packets.fetch_add(1,std::memory_order_relaxed);
        continue;
      }
      if (target && target->game_peer) { foreign_packets.fetch_add(1,std::memory_order_relaxed);continue; }
      if (!target) {
        Message hello{};
        if (Decode({bytes.data(), static_cast<std::size_t>(count)}, hello) !=
                Error::None ||
            hello.header.kind != Kind::Hello || hello.header.sender < 2 ||
            hello.header.sender > core::kMaximumParticipants) {
          foreign_packets.fetch_add(1, std::memory_order_relaxed);
          continue;
        }
        auto &candidate =
            *peers[static_cast<std::size_t>(hello.header.sender - 1)];
        if (candidate.locked || candidate.game_candidate ||
            candidate.terminal.load(std::memory_order_acquire) !=
                TerminalReason::None) {
          foreign_packets.fetch_add(1, std::memory_order_relaxed);
          continue;
        }
        // 同 nonce 不得从第二地址占用另一个参与者。
        bool duplicate_nonce = false;
        for (unsigned i = 1; i < peers.size(); ++i)
          if (peers[i]->locked && peers[i]->endpoint.Agreement().peer_nonce ==
                                      hello.hello.local_nonce)
            duplicate_nonce = true;
        if (duplicate_nonce)
          continue;
        target = &candidate;
      }
      target->received_packets.fetch_add(1, std::memory_order_relaxed);
      target->received_bytes.fetch_add(count, std::memory_order_relaxed);
      target->ReceiveOne({bytes.data(), static_cast<std::size_t>(count)},
                         source, GetTickCount64());
    }
  }
  // 已终止Peer仍在有界窗口回复完成结果；首因/输入资格不因重发改变。
  void ServiceGameEnd(std::uint64_t now) noexcept {
    if(!game_peer)return;
    try {FlushGame(now);game_peer->Pump(now,*encoding_cache,[&](auto bytes){Packet packet{};packet.size=bytes.size();std::copy(bytes.begin(),bytes.end(),packet.bytes.begin());if(SendPacket(packet,now,true))return replica::stream::SendResult::Sent;return replica::stream::SendResult::WouldBlock;});}catch(...){CountError(Error::Payload);}
  }
  void SendBatch(std::uint64_t now) noexcept {
    if (game_peer) {
      FlushGame(now);
      if (!game_peer->Established()) return;
      try {
        Ack ack{};
        // 每轮只发布最新完整快照，避免高帧率主线程造成无界发送；输入ACK独立产生。
        for (unsigned i=0;i<kSendBudget && outgoing.TryPop(ack);++i) heartbeat=ack;
        if(!(heartbeat.roster_mask&(1u<<ordinal)))return;
        SendGame(game_peer->Progress(heartbeat),now);
        game_peer->Pump(now,*encoding_cache,[&](auto bytes){Packet packet{};packet.size=bytes.size();std::copy(bytes.begin(),bytes.end(),packet.bytes.begin());if(SendPacket(packet,now))return replica::stream::SendResult::Sent;return terminal.load(std::memory_order_acquire)==TerminalReason::InternalFailure?replica::stream::SendResult::Failed:replica::stream::SendResult::WouldBlock;});
        publication_phase.store(game_peer->PublicationPhase(),std::memory_order_relaxed);
        const auto binary=game_peer->StateProgress();const std::array<std::uint64_t,6> progress{binary.world,binary.body,binary.events,binary.dropped_capacity,binary.dropped_expired,binary.unmatched_acks};
        for(unsigned i=0;i<progress.size();++i)binary_progress[i].store(progress[i],std::memory_order_relaxed);
        stream_stale.store(game_peer->Stale());
        if(now>=metrics_at){stream_metrics.Publish(game_peer->Metrics());encoder_metrics.Publish(encoding_cache->Statistics());metrics_at=now+50;}
        if(stream_status_mutex.try_lock()){stream_transfer=game_peer->TransferStatus();stream_limits=game_peer->EffectiveLimits();stream_fast_limits=game_peer->EffectiveFastLimits();stream_status_mutex.unlock();}
        const auto &stats=game_peer->OperationStatistics();const std::array<std::uint64_t,4> counters{stats.retransmissions,stats.cancellations,stats.completed,stats.failed};for(unsigned i=0;i<4;++i)operation_statistics[i].store(counters[i],std::memory_order_relaxed);
        if(!game_peer->FirstFailure().empty())ReplicationFailure(game_peer->FirstFailure().c_str());
      } catch(const std::exception &error){ReplicationFailure(error.what());Finish(ByeReason::InternalFailure,Error::Payload);}
      catch (...) {ReplicationFailure("live_network_exception");Finish(ByeReason::InternalFailure,Error::Payload);}
      return;
    }
    if (endpoint.State() != PeerState::Established)
      return;
    // 每个主线程发布值都按顺序验证；socket 背压失败时保留最后的有效快照作心跳。
    Ack ack{};
    for (std::uint32_t i = 0;
         i < kSendBudget && !stop.load(std::memory_order_acquire) &&
         outgoing.TryPop(ack);
         ++i) {
      Message m{};
      m.header.kind = Kind::Ack;
      m.ack = ack;
      if (ack.purpose != AckPurpose::Progress) {
        CountError(Error::Phase);
        Finish(ByeReason::InternalFailure, Error::Phase);
        return;
      }
      // 先通过 Endpoint 的完整验证，避免把非法快照留作后续心跳。
      SendMessage(m, now);
      if (terminal.load(std::memory_order_acquire) != TerminalReason::None)
        return;
      heartbeat = ack; // 合法但 sendto 失败时，下一次心跳继续尝试此 completed。
    }
    if (!stop.load(std::memory_order_acquire) && now >= last_sent_ms &&
        now - last_sent_ms >= config.heartbeat_ms) {
      Message m{};
      m.header.kind = Kind::Ack;
      m.ack = heartbeat;
      SendMessage(m, now);
    }
  }

  // 逻辑终止后只保留短暂传输收包窗口，接收迟到 Bye/在途包供真实计数。
  // 不调用 Endpoint::Receive、不刷新存活时间、不入输入队列、不发布 Ack 或回
  // Bye。
  void DrainTerminal() noexcept {
    if (!locked || socket == INVALID_SOCKET)
      return;
    const auto end = GetTickCount64() + kTerminalDrainMs;
    std::array<std::uint8_t, kMaximumPacket + 1> bytes{};
    do {
      for (std::uint32_t i = 0; i < kReceiveBudget; ++i) {
        sockaddr_in source{};
        int size = sizeof(source);
        const int n = recvfrom(socket, reinterpret_cast<char *>(bytes.data()),
                               static_cast<int>(bytes.size()), 0,
                               reinterpret_cast<sockaddr *>(&source), &size);
        if (n == SOCKET_ERROR) {
          const auto error = WSAGetLastError();
          if (error != WSAEWOULDBLOCK) {
            receive_errors.fetch_add(1, std::memory_order_relaxed);
            native_error.store(error, std::memory_order_relaxed);
          }
          break;
        }
        received_packets.fetch_add(1, std::memory_order_relaxed);
        received_bytes.fetch_add(static_cast<std::uint64_t>(n),
                                 std::memory_order_relaxed);
        terminal_received_packets.fetch_add(1, std::memory_order_relaxed);
        if (size != sizeof(source) || !SameAddress(source, peer))
          foreign_packets.fetch_add(1, std::memory_order_relaxed);
      }
      if (GetTickCount64() < end)
        Sleep(2);
    } while (GetTickCount64() < end);
  }
  // 真实游戏按最近到期服务，最多2ms；历史工具仍保持10ms的有界轮询。
  void Run() noexcept {
    unsigned round_robin=0;
    while (!stop.load(std::memory_order_acquire) &&
           terminal.load(std::memory_order_acquire) == TerminalReason::None) {
      const auto now = GetTickCount64();
      if(auto update=publications.Take())current_publication=std::move(update);
      bool binary_transferred=true;
      for (unsigned offset = 0; offset < peers.size()-1; ++offset) {
        const unsigned i=1+(round_robin+offset)%(peers.size()-1);
        auto &p = *peers[i];
        if (p.terminal.load(std::memory_order_acquire) != TerminalReason::None){if(now-p.terminal_at_ms.load(std::memory_order_relaxed)<2000)p.ServiceGameEnd(now);continue;}
        if(p.game_peer&&current_publication){try{binary_transferred=p.game_peer->Offer(*current_publication,now)&&binary_transferred;}catch(...){binary_transferred=false;p.Finish(ByeReason::InternalFailure,Error::Payload);}}
        const auto requested = p.stop.load(std::memory_order_acquire);
        const bool timed_out = p.game_peer ? p.game_peer->LastReceive() && now-p.game_peer->LastReceive()>=config.peer_timeout_ms : p.endpoint.TimedOut(now);
        if (requested || timed_out) {
          const auto reason = requested ? static_cast<ByeReason>(requested)
                                        : ByeReason::PeerTimeout;
          p.Finish(reason);
          p.BestEffortBye(reason, now);
          p.endpoint.Retire();
        } else
          p.SendBatch(now);
        if (p.terminal.load(std::memory_order_acquire) !=
            TerminalReason::None) {
          if (p.terminal.load(std::memory_order_relaxed) ==
              TerminalReason::InternalFailure) {
            p.BestEffortBye(ByeReason::InternalFailure, now);
            p.endpoint.Fail();
          }
          p.published_state.store(p.game_peer ? PeerState::Retired : p.endpoint.State(),
                                  std::memory_order_release);
        }
      }
      // 当前全部存活游戏Peer均已复制，才归还ordinary采集票据；工具Peer不消费世界事件。
      if(binary_transferred&&current_publication&&current_publication->binary_ordinary)
        current_publication->binary_ordinary->events_transferred.store(1,std::memory_order_release);
      round_robin=(round_robin+1)%(peers.size()-1);
      fd_set reads;
      FD_ZERO(&reads);
      FD_SET(socket, &reads);
      unsigned wait_ms=kPollMs;for(unsigned i=1;i<peers.size();++i)if(peers[i]->game_peer){const auto wake=peers[i]->game_peer->NextWake(now);wait_ms=(std::min)(wait_ms,unsigned((std::min)(config.stream.bundle_wait_ms,wake>now?wake-now:1)));}else if(!peers[i]->game_outgoing.empty())wait_ms=2;
      timeval timeout{0, long(wait_ms * 1000)};
      const auto ready = select(0, &reads, nullptr, nullptr, &timeout);
      if (ready == SOCKET_ERROR) {
        Finish(ByeReason::InternalFailure, Error::None, WSAGetLastError());
        break;
      }
      if (ready > 0)
        ReceiveBatch();
    }
    const auto requested = stop.load(std::memory_order_acquire);
    const auto reason = requested ? static_cast<ByeReason>(requested)
                                  : ByeReason::InternalFailure;
    Finish(reason);
    for (unsigned i = 1; i < peers.size(); ++i) {
      auto &p = *peers[i];
      if (p.terminal.load(std::memory_order_acquire) == TerminalReason::None) {
        p.Finish(reason);
        p.BestEffortBye(reason, GetTickCount64());
        p.endpoint.Retire();
      }
      p.published_state.store(p.game_peer ? PeerState::Retired : p.endpoint.State(), std::memory_order_release);
      p.running.store(false, std::memory_order_release);
    }
    // 网络结束只排空可靠控制，不等待原生世界退休；主线程独立推进Scene生命周期。
    const auto drain_start=GetTickCount64(),drain_until=drain_start+2000;
    while(GetTickCount64()<drain_until){bool any=false,settled=true;const auto at=GetTickCount64();for(unsigned i=1;i<peers.size();++i)if(peers[i]->game_peer){any=true;peers[i]->ServiceGameEnd(at);settled=settled&&peers[i]->game_peer->NetworkSettled();}if(!any||(settled&&at-drain_start>=100))break;ReceiveBatch();Sleep(2);}
    current_publication.reset();publications.Take();encoding_owner.Stop();
    CloseTransport();
    running.store(false, std::memory_order_release);
  }
  static DWORD WINAPI Entry(void *context) noexcept {
    // 即使主线程 Stop 超时后释放 Worker，线程也拥有独立的状态强引用。
    std::unique_ptr<std::shared_ptr<State>> transfer(
        static_cast<std::shared_ptr<State> *>(context));
    auto state = std::move(*transfer);
    transfer.reset();
    state->Run();
    return 0;
  }
};

const char *StartErrorName(StartError error) noexcept {
  switch (error) {
#define CS_START_ERROR(name)                                                   \
  case StartError::name:                                                       \
    return #name
    CS_START_ERROR(None);
    CS_START_ERROR(AlreadyStarted);
    CS_START_ERROR(Configuration);
    CS_START_ERROR(Allocation);
    CS_START_ERROR(Random);
    CS_START_ERROR(Winsock);
    CS_START_ERROR(Socket);
    CS_START_ERROR(SocketOption);
    CS_START_ERROR(Bind);
    CS_START_ERROR(Thread);
#undef CS_START_ERROR
  }
  return "UNKNOWN";
}
const char *TerminalReasonName(TerminalReason reason) noexcept {
  switch (reason) {
  case TerminalReason::None:
    return "None";
  case TerminalReason::Stop:
    return "Stop";
  case TerminalReason::PeerBye:
    return "PeerBye";
  case TerminalReason::PeerTimeout:
    return "PeerTimeout";
  case TerminalReason::PeerProtocol:
    return "PeerProtocol";
  case TerminalReason::RunTeardown:
    return "RunTeardown";
  case TerminalReason::InternalFailure:
    return "InternalFailure";
  case TerminalReason::PeerLost:
    return "PeerLost";
  }
  return "UNKNOWN";
}
Worker::~Worker() {
  RequestStop();
  if (thread_)
    CloseHandle(thread_);
}
StartResult Worker::Start(const WorkerConfig &config) noexcept {
  if (started_)
    return {StartError::AlreadyStarted};
  started_ = true;
  if (*config.stream.Validate()||config.peer_timeout_ms!=config.stream.peer_timeout_ms||config.heartbeat_ms!=config.stream.heartbeat_ms)return {StartError::Configuration};
  if (!config.port || !config.peer_timeout_ms || !config.heartbeat_ms ||
      config.heartbeat_ms >= config.peer_timeout_ms ||
      config.bind_address.empty() || config.bind_address.size() > 15 ||
      config.bind_address.find('\0') != std::string::npos)
    return {StartError::Configuration};
  try {
    auto state = std::make_shared<State>();
    state->config = config;
    state->uplink_owner.Configure(config.stream);
    // 随机身份在创建线程前生成，成功返回后不再修改；没有文件/构建哈希。
    auto random =
        BCryptGenRandom(nullptr, state->identity.session.data(),
                        static_cast<ULONG>(state->identity.session.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (random < 0)
      return {StartError::Random, static_cast<std::uint32_t>(random)};
    random = BCryptGenRandom(nullptr, state->identity.nonce.data(),
                             static_cast<ULONG>(state->identity.nonce.size()),
                             BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (random < 0 || !Nonzero(state->identity.session) ||
        !Nonzero(state->identity.nonce))
      return {StartError::Random, static_cast<std::uint32_t>(random)};
    WSADATA data{};
    const auto wsa = WSAStartup(MAKEWORD(2, 2), &data);
    if (wsa)
      return {StartError::Winsock, static_cast<std::uint32_t>(wsa)};
    state->wsa = true;
    state->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (state->socket == INVALID_SOCKET)
      return {StartError::Socket,
              static_cast<std::uint32_t>(WSAGetLastError())};
    BOOL exclusive = TRUE;
    if (setsockopt(state->socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                   reinterpret_cast<const char *>(&exclusive),
                   sizeof(exclusive)))
      return {StartError::SocketOption,
              static_cast<std::uint32_t>(WSAGetLastError())};
    u_long nonblocking = 1;
    if (ioctlsocket(state->socket, FIONBIO, &nonblocking))
      return {StartError::SocketOption,
              static_cast<std::uint32_t>(WSAGetLastError())};
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(config.port);
    if (InetPtonA(AF_INET, config.bind_address.c_str(), &address.sin_addr) != 1)
      return {StartError::Configuration};
    if (bind(state->socket, reinterpret_cast<const sockaddr *>(&address),
             sizeof(address)))
      return {StartError::Bind, static_cast<std::uint32_t>(WSAGetLastError())};
    for (unsigned i = 1; i < core::kMaximumParticipants; ++i) {
      auto peer = std::make_unique<State>();
      peer->owns_transport = false;
      peer->socket = state->socket;
      peer->ordinal = i;
      peer->uplink=&state->uplink_owner;peer->encoding_cache=&state->encoding_owner;
      peer->config = config;
      peer->identity = state->identity;
      if (BCryptGenRandom(nullptr, peer->identity.nonce.data(),
                          static_cast<ULONG>(peer->identity.nonce.size()),
                          BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0 ||
          !Nonzero(peer->identity.nonce))
        return {StartError::Random};
      protocol::Config endpoint_config{};
      endpoint_config.session = peer->identity.session;
      endpoint_config.local_nonce = peer->identity.nonce;
      endpoint_config.remote_ordinal = i;
      endpoint_config.build = config.build;
      endpoint_config.peer_timeout_ms = config.peer_timeout_ms;
      if (peer->endpoint.Start(endpoint_config, GetTickCount64()) !=
          Error::None)
        return {StartError::Configuration};
      peer->heartbeat.client_id = protocol::kClients[i];
      peer->running.store(true, std::memory_order_relaxed);
      peer->published_state.store(PeerState::Negotiating,
                                  std::memory_order_relaxed);
      state->peers[i] = std::move(peer);
    }
    auto transfer = std::make_unique<std::shared_ptr<State>>(state);
    state->published_state.store(PeerState::Negotiating,
                                 std::memory_order_relaxed);
    state->running.store(true, std::memory_order_relaxed);
    const auto thread =
        CreateThread(nullptr, 0, State::Entry, transfer.get(), 0, nullptr);
    if (!thread)
      return {StartError::Thread, GetLastError()};
    transfer.release();
    thread_ = thread;
    state_ = std::move(state);
    return {StartError::None, 0, state_->identity};
  } catch (...) {
    return {StartError::Allocation};
  }
}
bool Worker::QueueAck(const protocol::Ack &ack) noexcept {
  if (!state_ || ack.client_id < 2 ||
      ack.client_id > core::kMaximumParticipants)
    return false;
  auto &p = *state_->peers[static_cast<std::size_t>(ack.client_id - 1)];
  return !state_->stop.load(std::memory_order_acquire) &&
         p.terminal.load(std::memory_order_acquire) == TerminalReason::None &&
         p.published_state.load(std::memory_order_acquire) ==
             PeerState::Established &&
         p.outgoing.TryPush(ack);
}
bool Worker::PublishWorld(std::shared_ptr<const live::Publication> publication) noexcept {
  if(!state_||state_->stop.load(std::memory_order_acquire))return false;
  try{if(!publication)publication=std::make_shared<const live::Publication>();state_->publications.Put(std::move(publication));return true;}catch(...){return false;}
}
bool Worker::TryReceive(ReceivedInput &input, unsigned ordinal) noexcept {
  if (!state_ || !ordinal || ordinal >= core::kMaximumParticipants)
    return false;
  auto &p = *state_->peers[ordinal];
  return !state_->stop.load(std::memory_order_acquire) &&
         p.terminal.load(std::memory_order_acquire) == TerminalReason::None &&
         p.incoming.TryPop(input);
}
void Worker::RequestPeerStop(unsigned ordinal,
                             protocol::ByeReason reason) noexcept {
  if (!state_ || !ordinal || ordinal >= core::kMaximumParticipants)
    return;
  if (!protocol::ValidByeReason(reason))
    reason = ByeReason::InternalFailure;
  std::uint32_t expected = 0;
  state_->peers[ordinal]->stop.compare_exchange_strong(
      expected, static_cast<std::uint32_t>(reason), std::memory_order_release,
      std::memory_order_relaxed);
}
void Worker::RequestStop(protocol::ByeReason reason) noexcept {
  if (!state_)
    return;
  if (!protocol::ValidByeReason(reason))
    reason = ByeReason::InternalFailure;
  std::uint32_t expected = 0;
  state_->stop.compare_exchange_strong(
      expected, static_cast<std::uint32_t>(reason), std::memory_order_release,
      std::memory_order_relaxed);
}
StopResult Worker::Stop(protocol::ByeReason reason,
                        std::uint32_t wait_ms) noexcept {
  RequestStop(reason);
  if (!thread_)
    return StopResult::Stopped;
  // 不允许 INFINITE；调用者即使传超大值也最多等待 5 秒。
  const auto waited = WaitForSingleObject(thread_, (std::min)(wait_ms, 5000u));
  if (waited == WAIT_OBJECT_0) {
    CloseHandle(thread_);
    thread_ = nullptr;
    return StopResult::Stopped;
  }
  if (waited == WAIT_TIMEOUT) {
    state_->wait_timeouts.fetch_add(1, std::memory_order_relaxed);
    return StopResult::TimedOut;
  }
  state_->native_error.store(GetLastError(), std::memory_order_relaxed);
  return StopResult::WaitFailed;
}
bool Worker::TransportFailed() const noexcept {
  return state_ && state_->terminal.load(std::memory_order_acquire) == TerminalReason::InternalFailure;
}
WorkerStatus Worker::Status(unsigned ordinal) const noexcept {
  WorkerStatus result{};
  result.replication_memory=memory::ReplicationBudget()->Snapshot();
  if (!state_ || !ordinal || ordinal >= core::kMaximumParticipants)
    return result;
  result.routing_rejected_packets=state_->foreign_packets.load(std::memory_order_relaxed);
  const auto &s = *state_->peers[ordinal];
  result.terminal = s.terminal.load(std::memory_order_acquire);
  result.running = s.running.load(std::memory_order_acquire);
  result.state = s.published_state.load(std::memory_order_acquire);
  result.peer_locked = s.published_locked.load(std::memory_order_acquire);
  result.stream_config=s.config.stream;result.stream_stale=s.stream_stale.load();result.stream_metrics=s.stream_metrics.Read();result.encoder_metrics=s.encoder_metrics.Read();
  {std::lock_guard lock(s.stream_status_mutex);result.stream_transfer=s.stream_transfer;result.stream_limits=s.stream_limits;result.stream_fast_limits=s.stream_fast_limits;}
  result.uplink_packets=s.uplink_packets.load();result.uplink_payload_bytes=s.uplink_payload.load();result.uplink_bundle_messages=s.uplink_messages.load();
  result.publication_phase=s.publication_phase.load(std::memory_order_relaxed);
  result.binary_world_ack=s.binary_progress[0].load();result.binary_body_ack=s.binary_progress[1].load();result.binary_event_ack=s.binary_progress[2].load();
  result.binary_event_dropped_capacity=s.binary_progress[3].load();result.binary_event_dropped_expired=s.binary_progress[4].load();result.binary_unmatched_acks=s.binary_progress[5].load();
  result.operation_retransmissions=s.operation_statistics[0].load(std::memory_order_relaxed);result.operation_cancellations=s.operation_statistics[1].load(std::memory_order_relaxed);result.operation_completed=s.operation_statistics[2].load(std::memory_order_relaxed);result.operation_failed=s.operation_statistics[3].load(std::memory_order_relaxed);
  if(s.have_replication_failure.load(std::memory_order_acquire))result.replication_first_failure=s.replication_first_failure;
  result.uplink_wire_bytes=s.uplink_wire.load(std::memory_order_relaxed);result.uplink_budget_blocks=s.uplink_blocks.load(std::memory_order_relaxed);for(unsigned i=0;i<4;++i){result.uplink_lane_bytes[i]=s.uplink_lanes[i].load(std::memory_order_relaxed);result.uplink_lane_packets[i]=s.uplink_lane_packets[i].load();}
  result.protocol_error = s.protocol_error.load(std::memory_order_relaxed);
  result.peer_bye = s.peer_bye.load(std::memory_order_relaxed);
  result.native_error = s.native_error.load(std::memory_order_relaxed);
  result.last_send_error = s.last_send_error.load(std::memory_order_relaxed);
  const auto ip = s.peer_ip.load(std::memory_order_relaxed);
  for (unsigned i = 0; i < result.peer_address.size(); ++i)
    result.peer_address[i] = static_cast<std::uint8_t>(ip >> (24 - 8 * i));
  result.peer_port =
      static_cast<std::uint16_t>(s.peer_port.load(std::memory_order_relaxed));
#define CS_NET_COUNTER(name)                                                   \
  result.name = s.name.load(std::memory_order_relaxed)
  CS_NET_COUNTER(received_packets);
  CS_NET_COUNTER(sent_packets);
  CS_NET_COUNTER(received_bytes);
  CS_NET_COUNTER(sent_bytes);
  CS_NET_COUNTER(valid_packets);
  CS_NET_COUNTER(duplicate_packets);
  CS_NET_COUNTER(stale_packets);
  CS_NET_COUNTER(bye_send_attempts);
  CS_NET_COUNTER(bye_sent_packets);
  CS_NET_COUNTER(foreign_packets);
  CS_NET_COUNTER(receive_errors);
  CS_NET_COUNTER(send_errors);
  CS_NET_COUNTER(last_receive_ms);
  CS_NET_COUNTER(terminal_at_ms);
  CS_NET_COUNTER(terminal_received_packets);
  CS_NET_COUNTER(rtt_samples);
  CS_NET_COUNTER(rtt_last_ms);
  CS_NET_COUNTER(rtt_max_ms);
  CS_NET_COUNTER(wait_timeouts);
#undef CS_NET_COUNTER
  result.receive_queue_dropped = s.incoming.Dropped();
  result.send_queue_dropped = s.outgoing.Dropped();
  for (std::size_t i = 0; i < result.errors.size(); ++i)
    result.errors[i] = s.errors[i].load(std::memory_order_relaxed);
  return result;
}
} // namespace cs::net
