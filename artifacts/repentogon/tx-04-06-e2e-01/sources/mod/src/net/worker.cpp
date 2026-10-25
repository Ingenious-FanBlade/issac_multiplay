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
  bool wsa = false, locked = false;
  sockaddr_in peer{};
  Endpoint endpoint{};
  SpscQueue<ReceivedInput, kReceiveCapacity> incoming;
  SpscQueue<Ack, kSendCapacity> outgoing;
  std::atomic<std::uint32_t> stop{0};
  std::atomic<bool> running{false}, published_locked{false};
  std::atomic<PeerState> published_state{PeerState::Dormant};
  std::atomic<TerminalReason> terminal{TerminalReason::None};
  std::atomic<Error> protocol_error{Error::None};
  std::atomic<ByeReason> peer_bye{ByeReason::Stop};
  std::atomic<std::uint32_t> native_error{0}, last_send_error{0}, peer_ip{0},
      peer_port{0};
  std::atomic<std::uint64_t> received_packets{0}, sent_packets{0},
      received_bytes{0}, sent_bytes{0}, valid_packets{0}, duplicate_packets{0},
      stale_packets{0}, bye_send_attempts{0}, bye_sent_packets{0},
      foreign_packets{0}, receive_errors{0}, send_errors{0}, last_receive_ms{0},
      rtt_samples{0}, rtt_last_ms{0}, rtt_max_ms{0}, wait_timeouts{0};
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
    if (socket != INVALID_SOCKET) {
      closesocket(socket);
      socket = INVALID_SOCKET;
    }
    if (wsa) {
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
    terminal.store(TerminalFor(reason), std::memory_order_release);
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
  bool SendPacket(const Packet &packet, std::uint64_t now,
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
    SendMessage(m, now);
  }
  void BestEffortBye(ByeReason reason, std::uint64_t now) noexcept {
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

  // 已锁定地址之外的包一律在解析前丢弃，不能借错误报文结束合法对端的会话。
  void ReceiveOne(std::span<const std::uint8_t> bytes,
                  const sockaddr_in &source, std::uint64_t now) noexcept {
    if (locked && !SameAddress(source, peer)) {
      foreign_packets.fetch_add(1, std::memory_order_relaxed);
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
  void ReceiveBatch() noexcept {
    // 多留一个字节以识别略超上限的包；更大的 datagram 由 WSAEMSGSIZE 计错。
    std::array<std::uint8_t, kMaximumPacket + 1> bytes{};
    for (std::uint32_t i = 0;
         i < kReceiveBudget && !stop.load(std::memory_order_acquire); ++i) {
      sockaddr_in source{};
      int source_size = sizeof(source);
      const auto n =
          recvfrom(socket, reinterpret_cast<char *>(bytes.data()),
                   static_cast<int>(bytes.size()), 0,
                   reinterpret_cast<sockaddr *>(&source), &source_size);
      if (n == SOCKET_ERROR) {
        const auto error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
          break;
        receive_errors.fetch_add(1, std::memory_order_relaxed);
        // 对端 Bye 已终止会话；收尾 socket 错误不能覆盖原因或把 Retired 改成
        // Failed。
        if (terminal.load(std::memory_order_acquire) == TerminalReason::PeerBye)
          break;
        native_error.store(error, std::memory_order_relaxed);
        if (error == WSAEMSGSIZE) {
          received_packets.fetch_add(1, std::memory_order_relaxed);
          if (locked && !SameAddress(source, peer))
            foreign_packets.fetch_add(1, std::memory_order_relaxed);
          else {
            CountError(Error::Length);
            if (endpoint.State() == PeerState::Established) {
              Finish(ByeReason::PeerProtocol, Error::Length);
              BestEffortBye(ByeReason::PeerProtocol, GetTickCount64());
              endpoint.Fail();
            }
          }
        } else {
          // 不把 ICMP 等 socket 错误冒充合法 Bye；真实断线由存活超时处理。
          if (error != WSAECONNRESET && error != WSAENETRESET) {
            Finish(ByeReason::InternalFailure, Error::None, error);
            endpoint.Fail();
          }
        }
        break;
      }
      received_packets.fetch_add(1, std::memory_order_relaxed);
      received_bytes.fetch_add(static_cast<std::uint64_t>(n),
                               std::memory_order_relaxed);
      if (source_size != sizeof(source) || source.sin_family != AF_INET) {
        CountError(Error::Identity);
        continue;
      }
      ReceiveOne({bytes.data(), static_cast<std::size_t>(n)}, source,
                 GetTickCount64());
      const auto cause = terminal.load(std::memory_order_acquire);
      // 首份 Bye 已终止会话；本批剩余预算只收当前可读副本，不增加等待窗口。
      if (cause != TerminalReason::None && cause != TerminalReason::PeerBye)
        break;
    }
  }
  void SendBatch(std::uint64_t now) noexcept {
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

  // 不等待全员输入；select 最长 10ms，每轮收发工作量有界，可及时响应停止。
  void Run() noexcept {
    while (terminal.load(std::memory_order_acquire) == TerminalReason::None) {
      const auto requested = stop.load(std::memory_order_acquire);
      if (requested) {
        const auto reason = static_cast<ByeReason>(requested);
        Finish(reason);
        BestEffortBye(reason, GetTickCount64());
        break;
      }
      const auto now = GetTickCount64();
      // 存活判定共用 Endpoint；先尝试 Bye，再关闭状态。
      if (endpoint.TimedOut(now)) {
        Finish(ByeReason::PeerTimeout);
        BestEffortBye(ByeReason::PeerTimeout, now);
        endpoint.Retire();
        break;
      }
      SendBatch(now);
      if (terminal.load(std::memory_order_acquire) != TerminalReason::None)
        break;
      fd_set reads;
      FD_ZERO(&reads);
      FD_SET(socket, &reads);
      timeval timeout{0, kPollMs * 1000};
      const auto ready = select(0, &reads, nullptr, nullptr, &timeout);
      if (ready == SOCKET_ERROR) {
        const auto error = WSAGetLastError();
        Finish(ByeReason::InternalFailure, Error::None, error);
        endpoint.Fail();
        break;
      }
      if (ready > 0)
        ReceiveBatch();
    }
    const auto cause = terminal.load(std::memory_order_acquire);
    // 本地进度等内部失败仍可能保持已建立的传输；此时也按相同三副本规则通知对端。
    if (cause == TerminalReason::InternalFailure)
      BestEffortBye(ByeReason::InternalFailure, GetTickCount64());
    if (cause == TerminalReason::PeerProtocol ||
        cause == TerminalReason::InternalFailure)
      endpoint.Fail();
    else
      endpoint.Retire();
    CloseTransport();
    published_state.store(endpoint.State(), std::memory_order_release);
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
  if (!config.port || !config.peer_timeout_ms || !config.heartbeat_ms ||
      config.heartbeat_ms >= config.peer_timeout_ms ||
      config.bind_address.empty() || config.bind_address.size() > 15 ||
      config.bind_address.find('\0') != std::string::npos)
    return {StartError::Configuration};
  try {
    auto state = std::make_shared<State>();
    state->config = config;
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
    protocol::Config endpoint_config{};
    endpoint_config.session = state->identity.session;
    endpoint_config.local_nonce = state->identity.nonce;
    endpoint_config.build = config.build;
    endpoint_config.peer_timeout_ms = config.peer_timeout_ms;
    if (state->endpoint.Start(endpoint_config, GetTickCount64()) != Error::None)
      return {StartError::Configuration};
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
  return state_ && !state_->stop.load(std::memory_order_acquire) &&
         state_->running.load(std::memory_order_acquire) &&
         state_->terminal.load(std::memory_order_acquire) ==
             TerminalReason::None &&
         state_->published_state.load(std::memory_order_acquire) ==
             PeerState::Established &&
         state_->outgoing.TryPush(ack);
}
bool Worker::TryReceive(ReceivedInput &input) noexcept {
  return state_ && !state_->stop.load(std::memory_order_acquire) &&
         state_->terminal.load(std::memory_order_acquire) ==
             TerminalReason::None &&
         state_->incoming.TryPop(input);
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
WorkerStatus Worker::Status() const noexcept {
  WorkerStatus result{};
  if (!state_)
    return result;
  const auto &s = *state_;
  result.terminal = s.terminal.load(std::memory_order_acquire);
  result.running = s.running.load(std::memory_order_acquire);
  result.state = s.published_state.load(std::memory_order_acquire);
  result.peer_locked = s.published_locked.load(std::memory_order_acquire);
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
