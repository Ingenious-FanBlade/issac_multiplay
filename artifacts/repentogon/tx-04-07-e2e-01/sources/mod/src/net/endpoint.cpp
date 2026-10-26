#include "net/endpoint.h"
#include <algorithm>

namespace cs::net::protocol {
namespace {
bool Nonzero(const Id &id) noexcept {
  return std::any_of(id.begin(), id.end(), [](auto b) { return b != 0; });
}
bool Same(const Packet &a, std::span<const std::uint8_t> b) noexcept {
  return a.size == b.size() && a.size <= a.bytes.size() &&
         std::equal(b.begin(), b.end(), a.bytes.begin());
}
bool Terminal(PeerState state) noexcept {
  return state == PeerState::Retired || state == PeerState::Failed;
}
} // namespace

Hello Endpoint::LocalHello() const noexcept {
  Hello h{};
  h.build = config_.build;
  h.role = config_.role;
  h.local_nonce = config_.local_nonce;
  return h;
}
Hello Endpoint::Agreement() const noexcept {
  auto h = LocalHello();
  h.role = Role::Authority;
  if (config_.role == Role::Authority) {
    h.peer_nonce = peer_nonce_;
  } else {
    h.local_nonce = peer_nonce_;
    h.peer_nonce = config_.local_nonce;
  }
  return h;
}
Error Endpoint::Start(const Config &config, std::uint64_t now_ms) noexcept {
  if (state_ != PeerState::Dormant)
    return Error::Phase;
  auto hello = Hello{};
  hello.build = config.build;
  hello.local_nonce = config.local_nonce;
  if (!ValidHello(hello, false) || !config.peer_timeout_ms ||
      (config.role != Role::Authority && config.role != Role::Remote) ||
      (Nonzero(config.session) != (config.role == Role::Authority))) {
    Fail();
    return Error::Configuration;
  }
  config_ = config;
  last_receive_ms_ = now_ms;
  state_ = PeerState::Negotiating;
  return Error::None;
}
Error Endpoint::PrepareHello(Packet &output) noexcept {
  Message m{};
  m.header.kind = Kind::Hello;
  return Prepare(m, output);
}
Error Endpoint::PrepareFirstAck(Packet &output) noexcept {
  Message m{};
  m.header.kind = Kind::Ack;
  m.ack.purpose = AckPurpose::First;
  return Prepare(m, output);
}
Error Endpoint::Prepare(const Message &content, Packet &output) noexcept {
  if (Terminal(state_))
    return Error::Terminal;
  if (state_ == PeerState::Dormant)
    return Error::Phase;
  if (pending_valid_)
    return Error::Pending;
  if (!next_send_ || next_send_ == UINT64_MAX)
    return Error::Sequence;
  auto m = content;
  m.header.role = config_.role;
  m.header.sender = kClients[config_.role == Role::Authority ? 0 : 1];
  m.header.sequence = next_send_;
  m.header.session = config_.session;
  if (m.header.kind == Kind::Hello) {
    if (config_.role != Role::Remote || state_ != PeerState::Negotiating ||
        hello_sent_)
      return Error::Phase;
    m.hello = LocalHello();
  } else if (m.header.kind == Kind::Ack && m.ack.purpose == AckPurpose::First) {
    if (config_.role != Role::Authority || state_ != PeerState::Negotiating ||
        !hello_received_)
      return Error::Phase;
    m.ack = {};
    m.ack.purpose = AckPurpose::First;
    m.ack.hello_sequence = hello_sequence_;
    m.ack.agreement = Agreement();
  } else {
    if (state_ != PeerState::Established)
      return Error::Phase;
    if (m.header.kind == Kind::Ack) {
      m.ack.agreement = Agreement();
      if (ack_sent_ && !AckAdvances(last_ack_, m.ack))
        return Error::Sequence;
    }
    if (m.header.kind == Kind::Input && m.input.last_ack > LastAckSequence())
      return Error::Sequence;
    if (m.header.kind == Kind::Bye &&
        (m.bye.last_ack > LastAckSequence() ||
         (config_.role == Role::Authority && m.bye.last_ack)))
      return Error::Sequence;
  }
  Packet packet{};
  const auto error = Encode(m, packet);
  if (error != Error::None)
    return error;
  pending_ = packet;
  pending_valid_ = true;
  output = packet;
  return Error::None;
}
Error Endpoint::CommitSent(const Packet &packet) noexcept {
  if (Terminal(state_))
    return Error::Terminal;
  if (!pending_valid_ || packet.size > kMaximumPacket ||
      !Same(pending_, packet.View()))
    return Error::PendingMismatch;
  Message m{};
  const auto error = Decode(pending_.View(), m);
  if (error != Error::None)
    return error;
  if (m.header.kind == Kind::Hello) {
    hello_sent_ = true;
    hello_sequence_ = m.header.sequence;
    handshake_ = packet;
  } else if (m.header.kind == Kind::Ack) {
    last_ack_sent_ = m.header.sequence;
    ack_sent_ = true;
    last_ack_ = m.ack;
    if (m.ack.purpose == AckPurpose::First) {
      state_ = PeerState::Established;
      handshake_ = packet;
    }
  } else if (m.header.kind == Kind::Bye) {
    state_ = PeerState::Retired;
  }
  ++next_send_;
  pending_valid_ = false;
  return Error::None;
}
Error Endpoint::RetryHandshake(Packet &output) const noexcept {
  if (Terminal(state_))
    return Error::Terminal;
  if (!handshake_.size)
    return Error::Phase;
  output = handshake_;
  return Error::None;
}

// 先在局部完成 codec、身份、阶段与单调性校验，再一次性提交接收状态。
// Duplicate 只刷新存活时间，不重新投递输入，也不改变调用者的输出。
// Stale 不改输出、高水位或存活时间；Hello/First Ack 仍按各自握手规则处理。
Error Endpoint::Receive(std::span<const std::uint8_t> bytes,
                        std::uint64_t now_ms, Message &output) noexcept {
  // 第一份 Bye 已结束会话；只识别其原字节副本供有界收尾计数，不复活 Endpoint。
  if (state_ == PeerState::Retired && received_sequences_[4] &&
      Same(received_[4], bytes)) {
    last_receive_ms_ = now_ms;
    return Error::Duplicate;
  }
  if (Terminal(state_))
    return Error::Terminal;
  if (state_ == PeerState::Dormant)
    return Error::Phase;
  Message m{};
  const auto error = Decode(bytes, m);
  if (error != Error::None)
    return error;
  const auto other =
      config_.role == Role::Authority ? Role::Remote : Role::Authority;
  if (m.header.role != other)
    return Error::Role;
  const auto index = static_cast<std::size_t>(m.header.kind) - 1;
  if (received_sequences_[index] && Same(received_[index], bytes)) {
    last_receive_ms_ = now_ms;
    return Error::Duplicate;
  }
  if (m.header.kind == Kind::Hello) {
    if (config_.role != Role::Authority || state_ != PeerState::Negotiating ||
        hello_received_)
      return Error::Phase;
    if (m.header.sequence <= receive_high_)
      return Error::Sequence;
    if (m.hello.build != config_.build ||
        m.hello.local_nonce == config_.local_nonce)
      return Error::Incompatible;
  } else if (m.header.kind == Kind::Ack && m.ack.purpose == AckPurpose::First) {
    // Remote 的 First Ack 专用副本缓存由后续 Rust 客户端实现，本层不改其规则。
    if (m.header.sequence <= receive_high_)
      return Error::Sequence;
    if (config_.role != Role::Remote || state_ != PeerState::Negotiating ||
        !hello_sent_)
      return Error::Phase;
    auto expected = LocalHello();
    expected.role = Role::Authority;
    expected.local_nonce = m.ack.agreement.local_nonce;
    expected.peer_nonce = config_.local_nonce;
    if (m.ack.agreement != expected || m.ack.hello_sequence != hello_sequence_)
      return Error::Handshake;
  } else {
    if (state_ != PeerState::Established)
      return Error::Phase;
    if (m.header.session != config_.session)
      return Error::Identity;
    if (m.header.kind == Kind::Ack && m.ack.agreement != Agreement())
      return Error::Incompatible;
    // 只有通过格式和会话身份检查的旧包可以丢弃；更高序号的矛盾内容仍要报错。
    if (m.header.sequence <= receive_high_)
      return Error::Stale;
    if (m.header.kind == Kind::Ack) {
      Message previous{};
      if (Latest(Kind::Ack, previous) == Error::None &&
          !AckAdvances(previous.ack, m.ack))
        return Error::Sequence;
    }
    if (m.header.kind == Kind::Input && m.input.last_ack > last_ack_sent_)
      return Error::Sequence;
    if (m.header.kind == Kind::Bye &&
        ((other == Role::Authority && m.bye.last_ack) ||
         m.bye.last_ack > last_ack_sent_))
      return Error::Sequence;
  }
  Packet copy{};
  copy.size = bytes.size();
  std::copy(bytes.begin(), bytes.end(), copy.bytes.begin());
  received_[index] = copy;
  received_sequences_[index] = m.header.sequence;
  receive_high_ = m.header.sequence;
  last_receive_ms_ = now_ms;
  if (m.header.kind == Kind::Hello) {
    hello_received_ = true;
    hello_sequence_ = m.header.sequence;
    peer_nonce_ = m.hello.local_nonce;
  } else if (m.header.kind == Kind::Ack && m.ack.purpose == AckPurpose::First) {
    peer_nonce_ = m.ack.agreement.local_nonce;
    config_.session = m.header.session;
    state_ = PeerState::Established;
  } else if (m.header.kind == Kind::Bye) {
    Retire();
  }
  output = m;
  return Error::None;
}
bool Endpoint::TimedOut(std::uint64_t now_ms) const noexcept {
  if (Terminal(state_) || state_ == PeerState::Dormant ||
      (!hello_received_ && !hello_sent_))
    return false; // Authority 还没锁定对端时可以一直 Hosting。
  return now_ms >= last_receive_ms_ &&
         now_ms - last_receive_ms_ >= config_.peer_timeout_ms;
}
bool Endpoint::CheckTimeout(std::uint64_t now_ms) noexcept {
  if (!TimedOut(now_ms))
    return false;
  Retire();
  return true;
}
void Endpoint::Retire() noexcept {
  if (state_ != PeerState::Failed)
    state_ = PeerState::Retired;
  pending_valid_ = false;
}
void Endpoint::Fail() noexcept {
  state_ = PeerState::Failed;
  pending_valid_ = false;
}
Error Endpoint::Latest(Kind kind, Message &output) const noexcept {
  const auto index = static_cast<std::size_t>(kind) - 1;
  if (index >= received_.size() || kind == static_cast<Kind>(4))
    return Error::Kind;
  if (!received_sequences_[index])
    return Error::Phase;
  return Decode(received_[index].View(), output);
}
} // namespace cs::net::protocol
