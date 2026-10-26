#pragma once

#include "core/participant.h"
#include "core/session.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace cs::net::protocol {

inline constexpr std::array<std::uint8_t, 4> kMagic{'I', 'C', 'S', 'N'};
inline constexpr std::uint16_t kVersion = 1, kHeaderSize = 64;
inline constexpr std::size_t kMaximumPacket = 1200, kMaximumInputRecords = 4;
inline constexpr std::size_t kHelloSize = 104, kInputPrefixSize = 16,
                             kAckSize = 80 + kHelloSize, kByeSize = 16;
// V2 输入、整包 CRC、有界冗余；不包含原型的世界观测能力。
inline constexpr std::uint64_t kCapabilities = 1 | 4 | 8;
inline constexpr std::array<std::uint64_t, 2> kClients{1, 2};
using Id = std::array<std::uint8_t, 16>;
using Text = std::array<std::uint8_t, 32>;
constexpr Text TextId(const char *value) noexcept {
  Text result{};
  if (!value)
    return result;
  for (std::size_t i = 0; i < result.size(); ++i) {
    if (!value[i])
      return result;
    result[i] = static_cast<std::uint8_t>(value[i]);
  }
  return {}; // 超长身份无效，禁止截断后参与协商。
}

enum class Role : std::uint16_t { Authority = 1, Remote = 2 };
enum class Kind : std::uint16_t { Hello = 1, Input = 2, Ack = 3, Bye = 5 };
enum class PeerState : std::uint32_t {
  Dormant,
  Negotiating,
  Established,
  Retired,
  Failed
};
enum class Error : std::uint32_t {
  None,
  Duplicate,
  Configuration,
  Length,
  Magic,
  Version,
  Header,
  Flags,
  Reserved,
  Crc,
  Kind,
  Role,
  Identity,
  Sequence,
  Payload,
  Incompatible,
  Handshake,
  Phase,
  Terminal,
  Pending,
  PendingMismatch,
  Stale, // 合法但乱序/迟到的旧包；与 Duplicate 一样是接收结果，不是协议错误。
  Count
};
enum class AckPurpose : std::uint16_t { First = 1, Progress = 2 };
// 传输层结束原因独立于 core 原因码，后续由 game/peer 在主线程映射。
// tx-04-07 在末尾追加 PeerLost，既有数值不变；Rust 远端须接受该合法值。
enum class ByeReason : std::uint16_t {
  Stop = 1,
  PeerBye,
  PeerTimeout,
  PeerProtocol,
  RunTeardown,
  InternalFailure,
  PeerLost
};
const char *ErrorName(Error error) noexcept;
const char *PeerStateName(PeerState state) noexcept;
const char *ByeReasonName(ByeReason reason) noexcept;
bool ValidByeReason(ByeReason reason) noexcept;

// 64 字节包头只携带稳定身份；flags、reserved 与 CRC 由 codec 管理。
struct Header {
  Kind kind = Kind::Hello;
  Role role = Role::Remote;
  Id session{};
  std::uint64_t sender = kClients[1], sequence = 0;
};
// 初始 Hello 的 peer_nonce/session 为零；First Ack 才公布主机身份和双方 nonce。
struct Hello {
  Text build{};
  Id local_nonce{}, peer_nonce{};
  std::uint16_t input_schema = 2, protocol = kVersion;
  Role role = Role::Remote;
  std::uint64_t capabilities = kCapabilities;
  std::uint32_t maximum_packet = kMaximumPacket;
  std::array<std::uint64_t, 2> clients = kClients;
  bool operator==(const Hello &) const = default;
};
// 冗余命令从旧到新排列，command 是最新一条；仍由 Participant 做命令级去重。
struct Input {
  core::InputCmdDraftV2 command{};
  std::uint64_t last_ack = 0;
  std::uint16_t redundancy_count = 0;
  std::array<core::InputCmdDraftV2, kMaximumInputRecords - 1> redundant{};
};
// accepted/processed 为 sequence 高水位，processed_source
// 为本次实际来源，可为零。
struct Ack {
  std::uint64_t server_tick = 0, client_id = kClients[1], accepted = 0,
                processed = 0, processed_tick = 0, processed_source = 0,
                generation = 0, rejected_count = 0;
  std::uint32_t rejection_mask = 0;
  core::SessionState session_state = core::SessionState::Warmup;
  AckPurpose purpose = AckPurpose::Progress;
  std::uint64_t hello_sequence = 0;
  Hello agreement{};
};
struct Bye {
  ByeReason reason = ByeReason::Stop;
  std::uint64_t last_ack = 0;
};
// 固定大小值对象；网络层不携带对象指针或可变长分配。
struct Message {
  Header header{};
  Hello hello{};
  Input input{};
  Ack ack{};
  Bye bye{};
};
struct Packet {
  std::array<std::uint8_t, kMaximumPacket> bytes{};
  std::size_t size = 0;
  std::span<const std::uint8_t> View() const noexcept {
    return {bytes.data(), size <= bytes.size() ? size : 0};
  }
};

// CRC-32/ISO-HDLC：56..59
// 字节按零计算，仅用于报文损坏检测，不是认证或状态哈希。
std::uint32_t PacketCrc(std::span<const std::uint8_t> bytes) noexcept;
// 所有失败均不改变调用者的输出；字段显式小端，不能 memcpy 整个结构作为网络包。
Error Encode(const Message &message, Packet &output) noexcept;
Error Decode(std::span<const std::uint8_t> bytes, Message &output) noexcept;
// 协商字段在 codec 与 Endpoint 中共用，输入编码仍复用 core 的 V2 codec。
bool ValidHello(const Hello &hello, bool agreement) noexcept;
bool AckAdvances(const Ack &previous, const Ack &next) noexcept;
} // namespace cs::net::protocol
