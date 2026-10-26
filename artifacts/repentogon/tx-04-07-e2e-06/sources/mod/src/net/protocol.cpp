#include "net/protocol.h"
#include <algorithm>
#include <bit>

namespace cs::net::protocol {
namespace {
bool Nonzero(const Id &id) noexcept {
  return std::any_of(id.begin(), id.end(), [](auto b) { return b != 0; });
}
bool ValidSequence(std::uint64_t value) noexcept {
  return value && value != UINT64_MAX;
}
bool ValidText(const Text &text) noexcept {
  bool tail = false, nonempty = false;
  for (const auto b : text) {
    if (!b) {
      tail = true;
      continue;
    }
    if (tail || b < 33 || b > 126)
      return false;
    nonempty = true;
  }
  return tail && nonempty;
}
bool ValidKind(Kind kind) noexcept {
  return kind == Kind::Hello || kind == Kind::Input || kind == Kind::Ack ||
         kind == Kind::Bye;
}
void Put(std::uint8_t *out, std::uint64_t value, unsigned width) noexcept {
  for (unsigned i = 0; i < width; ++i)
    out[i] = static_cast<std::uint8_t>(value >> (i * 8));
}
std::uint64_t Get(const std::uint8_t *in, unsigned width) noexcept {
  std::uint64_t value = 0;
  for (unsigned i = 0; i < width; ++i)
    value |= std::uint64_t(in[i]) << (i * 8);
  return value;
}
template <std::size_t N>
void Store(std::uint8_t *out, const std::array<std::uint8_t, N> &in) noexcept {
  std::copy(in.begin(), in.end(), out);
}
template <std::size_t N>
void Read(const std::uint8_t *in, std::array<std::uint8_t, N> &out) noexcept {
  std::copy_n(in, N, out.begin());
}
// Hello 和 Ack 中的 agreement 共用同一布局；已删除原型的 domain/state_schema。
void WriteHello(std::uint8_t *p, const Hello &h) noexcept {
  Store(p, h.build);
  Store(p + 32, h.local_nonce);
  Store(p + 48, h.peer_nonce);
  Put(p + 64, h.input_schema, 2);
  Put(p + 66, h.protocol, 2);
  Put(p + 68, static_cast<unsigned>(h.role), 2);
  Put(p + 72, h.capabilities, 8);
  Put(p + 80, h.maximum_packet, 4);
  Put(p + 88, h.clients[0], 8);
  Put(p + 96, h.clients[1], 8);
}
Error ReadHello(const std::uint8_t *p, Hello &h) noexcept {
  if (Get(p + 70, 2) || Get(p + 84, 4))
    return Error::Reserved;
  Read(p, h.build);
  Read(p + 32, h.local_nonce);
  Read(p + 48, h.peer_nonce);
  h.input_schema = static_cast<std::uint16_t>(Get(p + 64, 2));
  h.protocol = static_cast<std::uint16_t>(Get(p + 66, 2));
  h.role = static_cast<Role>(Get(p + 68, 2));
  h.capabilities = Get(p + 72, 8);
  h.maximum_packet = static_cast<std::uint32_t>(Get(p + 80, 4));
  h.clients = {Get(p + 88, 8), Get(p + 96, 8)};
  return Error::None;
}
bool ValidAck(const Ack &a) noexcept {
  if (a.client_id != kClients[1] || !ValidHello(a.agreement, true) ||
      a.server_tick == UINT64_MAX || a.accepted == UINT64_MAX ||
      a.processed > a.accepted || a.processed_source > a.processed ||
      a.processed_tick > a.server_tick ||
      a.session_state < core::SessionState::Warmup ||
      a.session_state > core::SessionState::Failed ||
      (a.rejection_mask & ~0x7ff8u) ||
      a.rejected_count <
          static_cast<std::uint64_t>(std::popcount(a.rejection_mask)) ||
      ((a.rejected_count == 0) != (a.rejection_mask == 0)))
    return false;
  // 中性 tick 也会推进 processed_tick，但 processed 高水位仍可能为零。
  if ((a.processed && (!a.processed_tick || !a.generation)) ||
      (a.processed_tick && !a.generation))
    return false;
  if (a.purpose == AckPurpose::First)
    return ValidSequence(a.hello_sequence) &&
           a.session_state == core::SessionState::Warmup && !a.server_tick &&
           !a.accepted && !a.processed && !a.processed_tick && !a.generation &&
           !a.rejected_count && !a.rejection_mask;
  // Warmup/menu/暂停时仍能发送 completed 不变的心跳。
  return a.purpose == AckPurpose::Progress && !a.hello_sequence;
}
Error Validate(const Message &m) noexcept {
  const auto &h = m.header;
  if (!ValidKind(h.kind))
    return Error::Kind;
  if (h.role != Role::Authority && h.role != Role::Remote)
    return Error::Role;
  if (!ValidSequence(h.sequence))
    return Error::Sequence;
  if (h.sender != kClients[h.role == Role::Authority ? 0 : 1])
    return Error::Identity;
  if (h.kind == Kind::Hello) {
    if (h.role != Role::Remote)
      return Error::Role;
    if (Nonzero(h.session))
      return Error::Identity;
    return ValidHello(m.hello, false) ? Error::None : Error::Payload;
  }
  if (!Nonzero(h.session))
    return Error::Identity;
  if (h.kind == Kind::Input) {
    if (h.role != Role::Remote)
      return Error::Role;
    if (m.input.redundancy_count >= kMaximumInputRecords)
      return Error::Payload;
    if (m.input.last_ack == UINT64_MAX)
      return Error::Sequence;
    std::uint64_t previous = 0;
    for (std::size_t i = 0; i <= m.input.redundancy_count; ++i) {
      const auto &c = i == m.input.redundancy_count ? m.input.command
                                                    : m.input.redundant[i];
      if (c.session_id != h.session || c.client_id != h.sender)
        return Error::Identity;
      if (!ValidSequence(c.sequence) || c.sequence <= previous)
        return Error::Sequence;
      if (!ValidSequence(c.target_tick) || !core::Valid(c.state))
        return Error::Payload;
      previous = c.sequence;
    }
  } else if (h.kind == Kind::Ack) {
    if (h.role != Role::Authority)
      return Error::Role;
    if (!ValidAck(m.ack))
      return Error::Payload;
  } else if (!ValidByeReason(m.bye.reason) || m.bye.last_ack == UINT64_MAX) {
    return Error::Payload;
  }
  return Error::None;
}
} // namespace

bool ValidHello(const Hello &h, bool agreement) noexcept {
  if (!ValidText(h.build) || !Nonzero(h.local_nonce) || h.input_schema != 2 ||
      h.protocol != kVersion || h.capabilities != kCapabilities ||
      h.maximum_packet != kMaximumPacket || h.clients != kClients)
    return false;
  return agreement ? h.role == Role::Authority && Nonzero(h.peer_nonce) &&
                         h.local_nonce != h.peer_nonce
                   : h.role == Role::Remote && !Nonzero(h.peer_nonce);
}
bool ValidByeReason(ByeReason reason) noexcept {
  // 追加 PeerLost 后仍按连续区间校验，旧的 1..6 保持合法。
  return reason >= ByeReason::Stop && reason <= ByeReason::PeerLost;
}
bool AckAdvances(const Ack &old, const Ack &next) noexcept {
  return next.server_tick >= old.server_tick && next.accepted >= old.accepted &&
         next.processed >= old.processed &&
         next.processed_tick >= old.processed_tick &&
         next.generation >= old.generation &&
         next.rejected_count >= old.rejected_count &&
         next.session_state >= old.session_state &&
         (next.rejection_mask & old.rejection_mask) == old.rejection_mask;
}
const char *ErrorName(Error error) noexcept {
  switch (error) {
#define CS_NET_ERROR(name)                                                     \
  case Error::name:                                                            \
    return #name
    CS_NET_ERROR(None);
    CS_NET_ERROR(Duplicate);
    CS_NET_ERROR(Configuration);
    CS_NET_ERROR(Length);
    CS_NET_ERROR(Magic);
    CS_NET_ERROR(Version);
    CS_NET_ERROR(Header);
    CS_NET_ERROR(Flags);
    CS_NET_ERROR(Reserved);
    CS_NET_ERROR(Crc);
    CS_NET_ERROR(Kind);
    CS_NET_ERROR(Role);
    CS_NET_ERROR(Identity);
    CS_NET_ERROR(Sequence);
    CS_NET_ERROR(Payload);
    CS_NET_ERROR(Incompatible);
    CS_NET_ERROR(Handshake);
    CS_NET_ERROR(Phase);
    CS_NET_ERROR(Terminal);
    CS_NET_ERROR(Pending);
    CS_NET_ERROR(PendingMismatch);
    CS_NET_ERROR(Stale);
#undef CS_NET_ERROR
  case Error::Count:
    break;
  }
  return "UNKNOWN";
}
const char *PeerStateName(PeerState state) noexcept {
  switch (state) {
  case PeerState::Dormant:
    return "Dormant";
  case PeerState::Negotiating:
    return "Negotiating";
  case PeerState::Established:
    return "Established";
  case PeerState::Retired:
    return "Retired";
  case PeerState::Failed:
    return "Failed";
  }
  return "UNKNOWN";
}
const char *ByeReasonName(ByeReason reason) noexcept {
  switch (reason) {
  case ByeReason::Stop:
    return "Stop";
  case ByeReason::PeerBye:
    return "PeerBye";
  case ByeReason::PeerTimeout:
    return "PeerTimeout";
  case ByeReason::PeerProtocol:
    return "PeerProtocol";
  case ByeReason::RunTeardown:
    return "RunTeardown";
  case ByeReason::InternalFailure:
    return "InternalFailure";
  case ByeReason::PeerLost:
    return "PeerLost";
  }
  return "UNKNOWN";
}
std::uint32_t PacketCrc(std::span<const std::uint8_t> bytes) noexcept {
  std::uint32_t crc = 0xffffffffu;
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    crc ^= i >= 56 && i < 60 ? 0 : bytes[i];
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320u : 0);
  }
  return crc ^ 0xffffffffu;
}
Error Encode(const Message &m, Packet &output) noexcept {
  const auto error = Validate(m);
  if (error != Error::None)
    return error;
  const auto size =
      m.header.kind == Kind::Hello ? kHelloSize
      : m.header.kind == Kind::Ack ? kAckSize
      : m.header.kind == Kind::Bye
          ? kByeSize
          : kInputPrefixSize + 64 * (1 + m.input.redundancy_count);
  Packet packet{};
  packet.size = kHeaderSize + size;
  auto *p = packet.bytes.data();
  Store(p, kMagic);
  Put(p + 4, kVersion, 2);
  Put(p + 6, static_cast<unsigned>(m.header.kind), 2);
  Put(p + 8, kHeaderSize, 2);
  Put(p + 10, static_cast<unsigned>(m.header.role), 2);
  Store(p + 16, m.header.session);
  Put(p + 32, m.header.sender, 8);
  Put(p + 40, m.header.sequence, 8);
  Put(p + 48, size, 4);
  p += kHeaderSize;
  if (m.header.kind == Kind::Hello) {
    WriteHello(p, m.hello);
  } else if (m.header.kind == Kind::Input) {
    Put(p, 1 + m.input.redundancy_count, 2);
    Put(p + 8, m.input.last_ack, 8);
    for (std::size_t i = 0; i <= m.input.redundancy_count; ++i) {
      core::EncodedInputCmdV2 command{};
      if (!core::Encode(i == m.input.redundancy_count ? m.input.command
                                                      : m.input.redundant[i],
                        command))
        return Error::Payload;
      Store(p + kInputPrefixSize + 64 * i, command);
    }
  } else if (m.header.kind == Kind::Ack) {
    const auto &a = m.ack;
    const std::array<std::uint64_t, 8> words{
        a.server_tick,    a.client_id,        a.accepted,   a.processed,
        a.processed_tick, a.processed_source, a.generation, a.rejected_count};
    for (std::size_t i = 0; i < words.size(); ++i)
      Put(p + i * 8, words[i], 8);
    Put(p + 64, a.rejection_mask, 4);
    Put(p + 68, static_cast<unsigned>(a.session_state), 2);
    Put(p + 70, static_cast<unsigned>(a.purpose), 2);
    Put(p + 72, a.hello_sequence, 8);
    WriteHello(p + 80, a.agreement);
  } else {
    Put(p, static_cast<unsigned>(m.bye.reason), 2);
    Put(p + 8, m.bye.last_ack, 8);
  }
  Put(packet.bytes.data() + 56, PacketCrc(packet.View()), 4);
  output = packet;
  return Error::None;
}
Error Decode(std::span<const std::uint8_t> bytes, Message &output) noexcept {
  if (bytes.size() < kHeaderSize || bytes.size() > kMaximumPacket ||
      !bytes.data())
    return Error::Length;
  const auto *p = bytes.data();
  if (!std::equal(kMagic.begin(), kMagic.end(), p))
    return Error::Magic;
  if (Get(p + 4, 2) != kVersion)
    return Error::Version;
  if (Get(p + 8, 2) != kHeaderSize)
    return Error::Header;
  if (Get(p + 12, 4))
    return Error::Flags;
  if (Get(p + 52, 4) || Get(p + 60, 4))
    return Error::Reserved;
  if (Get(p + 48, 4) != bytes.size() - kHeaderSize)
    return Error::Length;
  if (Get(p + 56, 4) != PacketCrc(bytes))
    return Error::Crc;
  Message m{};
  m.header.kind = static_cast<Kind>(Get(p + 6, 2));
  m.header.role = static_cast<Role>(Get(p + 10, 2));
  Read(p + 16, m.header.session);
  m.header.sender = Get(p + 32, 8);
  m.header.sequence = Get(p + 40, 8);
  if (!ValidKind(m.header.kind))
    return Error::Kind; // 4 永久保留，不接收旧 StateHash。
  const auto size = bytes.size() - kHeaderSize;
  p += kHeaderSize;
  if (m.header.kind == Kind::Hello) {
    if (size != kHelloSize)
      return Error::Length;
    const auto r = ReadHello(p, m.hello);
    if (r != Error::None)
      return r;
  } else if (m.header.kind == Kind::Input) {
    if (size < kInputPrefixSize)
      return Error::Length;
    const auto count = Get(p, 2);
    if (!count || count > kMaximumInputRecords ||
        size != kInputPrefixSize + 64 * count)
      return Error::Length;
    if (Get(p + 2, 2) || Get(p + 4, 4))
      return Error::Reserved;
    m.input.redundancy_count = static_cast<std::uint16_t>(count - 1);
    m.input.last_ack = Get(p + 8, 8);
    for (std::size_t i = 0; i < count; ++i) {
      core::EncodedInputCmdV2 command{};
      Read(p + kInputPrefixSize + 64 * i, command);
      auto &target = i == count - 1 ? m.input.command : m.input.redundant[i];
      if (!core::Decode(command, target))
        return Error::Payload;
    }
  } else if (m.header.kind == Kind::Ack) {
    if (size != kAckSize)
      return Error::Length;
    auto &a = m.ack;
    a.server_tick = Get(p, 8);
    a.client_id = Get(p + 8, 8);
    a.accepted = Get(p + 16, 8);
    a.processed = Get(p + 24, 8);
    a.processed_tick = Get(p + 32, 8);
    a.processed_source = Get(p + 40, 8);
    a.generation = Get(p + 48, 8);
    a.rejected_count = Get(p + 56, 8);
    a.rejection_mask = static_cast<std::uint32_t>(Get(p + 64, 4));
    a.session_state = static_cast<core::SessionState>(Get(p + 68, 2));
    a.purpose = static_cast<AckPurpose>(Get(p + 70, 2));
    a.hello_sequence = Get(p + 72, 8);
    const auto r = ReadHello(p + 80, a.agreement);
    if (r != Error::None)
      return r;
  } else {
    if (size != kByeSize)
      return Error::Length;
    if (Get(p + 2, 2) || Get(p + 4, 4))
      return Error::Reserved;
    m.bye.reason = static_cast<ByeReason>(Get(p, 2));
    m.bye.last_ack = Get(p + 8, 8);
  }
  const auto error = Validate(m);
  if (error != Error::None)
    return error;
  output = m;
  return Error::None;
}
} // namespace cs::net::protocol
