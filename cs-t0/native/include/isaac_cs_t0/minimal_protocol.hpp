#pragma once
#include "isaac_cs_t0/authority_session.hpp"
#include "isaac_cs_t0/coop_input.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

// T0-only experimental wire. No socket, game object, Hook, Lua or scheduler owner.
namespace isaac_cs_t0::protocol {
inline constexpr std::array<std::uint8_t,4> kMagic{'I','C','T','8'};
inline constexpr std::uint16_t kVersion=2,kHeaderSize=64;
inline constexpr std::uint32_t kMaximumPacket=1200,kMaximumValues=24;
inline constexpr std::uint64_t kCapabilities=15; // V2 input, observations, CRC, bounded redundancy
inline constexpr std::size_t kMaximumInputRecords=4;
inline constexpr std::uint16_t kValuesNotExported=19;
using Id=std::array<std::uint8_t,16>;
using Text=std::array<std::uint8_t,32>;
constexpr Text TextId(const char* value) noexcept {
    Text result{};if(!value) return result;
    for(std::size_t i=0;i<32;++i) {if(!value[i]) return result;result[i]=static_cast<std::uint8_t>(value[i]);}
    return {}; // An overlong label is invalid; never silently truncate identity.
}
inline constexpr Text kStateDomain=TextId("isaac.cs.t0.observation.v1");
enum class Role : std::uint16_t { Authority=1,Remote=2 };
enum class Kind : std::uint16_t { Hello=1,Input=2,Ack=3,StateHash=4 };
enum class PeerState : std::uint32_t { Dormant,Negotiating,Established,Retired,Failed };
enum class Error : std::uint32_t {
    None,Duplicate,Configuration,Length,Magic,Version,Header,Flags,Reserved,Crc,
    Kind,Role,Identity,Sequence,Payload,Incompatible,Handshake,Phase,Terminal,Pending,PendingMismatch
};
const char* ErrorName(Error error) noexcept;
struct Header {
    Kind kind=Kind::Hello;Role role=Role::Authority;Id session{};
    std::uint64_t sender=0,sequence=0;
};
struct Hello {
    Text build{},domain=kStateDomain;Id local_nonce{},peer_nonce{};
    std::uint16_t input_schema=2,state_schema=1,protocol=kVersion;Role role=Role::Authority;
    std::uint64_t capabilities=kCapabilities;
    std::uint32_t maximum_packet=kMaximumPacket;
    std::array<std::uint64_t,2> clients{};
    bool operator==(const Hello&) const = default;
};
struct Input {
    coop_active::InputCmdDraftV2 command{};std::uint64_t last_ack=0;
    std::uint16_t redundancy_count=0;
    std::array<coop_active::InputCmdDraftV2,kMaximumInputRecords-1> redundant{};
};
enum class AckPurpose : std::uint16_t { First=1,Progress=2 };
struct Ack {
    std::uint64_t server_tick=0,client_id=0,accepted=0,processed=0,processed_tick=0,
        processed_source=0,generation=0,rejected_count=0;
    std::uint32_t rejection_mask=0;
    coop_active::SessionState session_state=coop_active::SessionState::Warmup;
    AckPurpose purpose=AckPurpose::Progress;
    std::uint64_t hello_sequence=0;
    Hello agreement{}; // Full non-hash negotiation identity, including both nonces.
};
enum class CapturePhase : std::uint16_t { PostStandardPreTopExit=1,OddOnly=2 };
enum class StateStatus : std::uint16_t { PartialValues=1,Gap=2 };
enum class ValueEncoding : std::uint8_t { Raw=0,F32=1,Derived=3 };
struct RawValue {
    // Process-local observation identity; does not establish cross-process equality.
    std::uint64_t entity=0,use=0;std::uint16_t field=0;
    ValueEncoding encoding=ValueEncoding::Raw;std::uint16_t width=0;
    std::array<std::uint8_t,16> raw{};
};
struct StateObservation {
    Text domain=kStateDomain;
    std::uint64_t tick=0,run=0,room=0,attempt=0,capture=0;
    std::uint16_t schema=1;
    CapturePhase phase=CapturePhase::PostStandardPreTopExit;
    StateStatus status=StateStatus::PartialValues;
    std::uint16_t reason=15,count=0;
    std::uint64_t source_token=0;
    std::array<RawValue,kMaximumValues> values{};
};
struct Message {Header header{};Hello hello{};Input input{};Ack ack{};StateObservation state{};};
struct Packet {
    std::array<std::uint8_t,kMaximumPacket> bytes{};std::size_t size=0;
    std::span<const std::uint8_t> View() const noexcept {return {bytes.data(),size<=bytes.size()?size:0};}
};
// CRC-32/ISO-HDLC over the complete packet with bytes 56..59 treated as zero.
// This detects accidental packet damage; it is neither authentication nor state hashing.
std::uint32_t PacketCrc(std::span<const std::uint8_t> bytes) noexcept;
// On all errors, caller output is unchanged. Wire is explicit little endian.
Error Encode(const Message& message,Packet& output) noexcept;
Error Decode(std::span<const std::uint8_t> bytes,Message& output) noexcept;

struct Config {
    Role role=Role::Authority;Id session{};std::array<std::uint64_t,2> clients{};
    Text build{},domain=kStateDomain;Id local_nonce{},peer_nonce{};
    std::uint16_t input_schema=2,state_schema=1;
    std::uint64_t capabilities=kCapabilities;
    std::uint32_t maximum_packet=kMaximumPacket;
};
// Single owner, one-shot, bounded protocol state. Config is trusted experiment
// metadata; nonces must be fresh and distinct outside this class. No authentication.
// Established is transport negotiation only: it NEVER activates AuthoritySession.
// Prepare does not advance sequence/state. CommitSent belongs immediately after
// successful delivery into an owned bounded transport queue; failed sends cancel.
class Endpoint final {
public:
    Error Start(const Config& config) noexcept;
    Error PrepareHello(Packet& output) noexcept;
    Error PrepareFirstAck(Packet& output) noexcept;
    Error Prepare(const Message& content,Packet& output) noexcept;
    Error CommitSent(const Packet& packet) noexcept;
    void CancelPending() noexcept {pending_valid_=false;}
    Error Receive(std::span<const std::uint8_t> bytes,Message& output) noexcept;
    void Retire() noexcept {if(state_!=PeerState::Failed) state_=PeerState::Retired;pending_valid_=false;}
    PeerState State() const noexcept {return state_;}
    Error Latest(Kind kind,Message& output) const noexcept;
    std::uint64_t LastAckSequence() const noexcept {return received_sequences_[2];}
private:
    Hello LocalHello() const noexcept;
    Hello PeerHello() const noexcept;
    Error Fail(Error error) noexcept {state_=PeerState::Failed;pending_valid_=false;return error;}
    Config config_{};PeerState state_=PeerState::Dormant;
    std::uint64_t next_send_=1,local_hello_sequence_=0,peer_hello_sequence_=0,last_ack_sent_=0;
    bool pending_valid_=false,hello_sent_=false,hello_received_=false;
    Packet pending_{};
    std::array<Packet,4> received_{};
    std::array<std::uint64_t,4> received_sequences_{};
};
} // namespace isaac_cs_t0::protocol
