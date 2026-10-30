#pragma once

#include "net/protocol.h"

namespace cs::net::protocol {

// 单次协商配置；Authority 在启动时已有随机 session/nonce，Remote 只有本地
// nonce。 build 必须相同；身份不提供认证。now_ms 只用于存活，不参与 Authority
// tick。
struct Config {
  Role role = Role::Authority;
  // 每个 Endpoint 仅属于一个远端 ordinal；网络线程不推断 Player 身份。
  std::uint32_t remote_ordinal = 1;
  Id session{}, local_nonce{};
  Text build{};
  std::uint32_t peer_timeout_ms = 3000;
};

// 单 owner、一次性 Endpoint；由网络线程独占，不能从主线程直接调用。
// Prepare 不推进序号；实际 sendto 成功后才 CommitSent，失败必须 CancelPending。
// Hello 重传使用相同字节，First Ack 丢失时可凭相同 Hello 重发而不再次建立会话。
class Endpoint final {
public:
  Error Start(const Config &config, std::uint64_t now_ms) noexcept;
  Error PrepareHello(Packet &output) noexcept;
  Error PrepareFirstAck(Packet &output) noexcept;
  Error Prepare(const Message &content, Packet &output) noexcept;
  Error CommitSent(const Packet &packet) noexcept;
  void CancelPending() noexcept { pending_valid_ = false; }
  Error RetryHandshake(Packet &output) const noexcept;
  // Duplicate 仅刷新存活；Stale 仅表示丢弃，均不修改 output 或重复交付内容。
  Error Receive(std::span<const std::uint8_t> bytes, std::uint64_t now_ms,
                Message &output) noexcept;
  bool TimedOut(std::uint64_t now_ms) const noexcept;
  bool CheckTimeout(std::uint64_t now_ms) noexcept;
  void Retire() noexcept;
  void Fail() noexcept;
  PeerState State() const noexcept { return state_; }
  const Id &SessionId() const noexcept { return config_.session; }
  Hello Agreement() const noexcept;
  std::uint64_t LastAckSequence() const noexcept {
    return received_sequences_[2];
  }
  std::uint64_t LastSentSequence() const noexcept { return next_send_ - 1; }
  std::uint64_t LastReceiveMs() const noexcept { return last_receive_ms_; }
  Error Latest(Kind kind, Message &output) const noexcept;

private:
  Hello LocalHello() const noexcept;
  Config config_{};
  Id peer_nonce_{};
  PeerState state_ = PeerState::Dormant;
  std::uint64_t next_send_ = 1, receive_high_ = 0, hello_sequence_ = 0,
                last_ack_sent_ = 0, last_receive_ms_ = 0;
  bool hello_sent_ = false, hello_received_ = false, pending_valid_ = false;
  bool ack_sent_ = false;
  Packet pending_{}, handshake_{};
  Ack last_ack_{};
  std::array<Packet, 5> received_{};
  std::array<std::uint64_t, 5> received_sequences_{};
};
} // namespace cs::net::protocol
