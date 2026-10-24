#pragma once
#include "isaac_cs_t0/minimal_protocol.hpp"

namespace isaac_cs_t0::protocol {
enum class Direction : std::uint32_t { ToAuthority,ToRemote };
enum class TraceAction : std::uint32_t { Send=1,Drop,Enqueue,Deliver,Receive,Rejected,Full,Admission,Closed };
struct TraceRecord {
    std::uint64_t tick=0,token=0,generation=0;
    TraceAction action=TraceAction::Send;Direction direction=Direction::ToAuthority;
    std::uint32_t decision=0;Packet packet{};
};
using TraceFunction=bool(*)(const TraceRecord&,void*) noexcept;
struct alignas(8) FaultConfig {
    std::uint32_t version=1,size=64;
    std::uint64_t seed=0x20008;
    std::uint32_t delay_ticks=1,jitter_ticks=1,loss_per_mille=50,duplicate_per_mille=150,reorder_per_mille=150,reserved=0;
    std::uint64_t outage_start=64,outage_ticks=20,reserved_tail=0;
};
static_assert(sizeof(FaultConfig)==64);
bool ValidFaultConfig(const FaultConfig& config) noexcept;
enum class TransportResult : std::uint32_t { Queued,Lost,Full,Invalid,Closed };
struct Delivery {Packet packet{};std::uint64_t generation=0,token=0;};
// Logical Authority ticks, xorshift64, stable (due, token, copy) ordering.
// Owned bounded storage, no OS/Steam/network/game calls. One owner only.
class FaultInjectionTransport final {
public:
    static constexpr std::size_t kCapacity=128;
    bool Initialize(const FaultConfig& config,TraceFunction trace,void* context) noexcept;
    TransportResult Send(Direction direction,const Packet& packet,std::uint64_t generation,
                         std::uint64_t now,bool reliable=false) noexcept;
    bool Pop(Direction direction,std::uint64_t now,Delivery& output) noexcept;
    void Clear(Direction direction,std::uint64_t now) noexcept;
    void Close(std::uint64_t now) noexcept;
    bool Healthy() const noexcept {return healthy_;}
private:
    struct Slot {Packet packet{};std::uint64_t due=0,generation=0,token=0;Direction direction{};std::uint32_t copy=0;bool used=false;};
    std::uint64_t Random() noexcept;
    bool Emit(TraceAction action,Direction direction,const Packet& packet,std::uint64_t now,
              std::uint64_t generation,std::uint64_t token,std::uint32_t decision=0) noexcept;
    FaultConfig config_{};TraceFunction trace_=nullptr;void* context_=nullptr;
    std::array<Slot,kCapacity> slots_{};std::uint64_t rng_=0,token_=0;
    std::size_t count_=0;bool initialized_=false,closed_=false,healthy_=true;
};
using AdmitFunction=std::uint32_t(*)(const coop_active::InputCmdDraftV2&,std::uint64_t,void*) noexcept;
class ProtocolLink final {
public:
    bool Initialize(const Config& authority,const FaultConfig& faults,TraceFunction trace,void* context) noexcept;
    bool SendInput(const coop_active::InputCmdDraftV2& command,std::uint64_t generation,std::uint64_t now) noexcept;
    bool PumpInputs(std::uint64_t now,std::uint32_t admission_budget,AdmitFunction admit,void* context,std::uint64_t expected_generation=0) noexcept;
    bool Publish(const Ack& ack,const StateObservation& state,std::uint64_t now) noexcept;
    void Retire(std::uint64_t now) noexcept;
    const Endpoint& Remote() const noexcept {return remote_;}
    bool Healthy() const noexcept {return healthy_ && transport_.Healthy();}
private:
    bool Send(Endpoint& from,Direction direction,const Message& message,std::uint64_t generation,std::uint64_t now,bool reliable=false) noexcept;
    bool SendPrepared(Endpoint& from,Direction direction,const Packet& packet,std::uint64_t generation,std::uint64_t now,bool reliable) noexcept;
    bool PumpRemote(std::uint64_t now) noexcept;
    bool Receive(Endpoint& endpoint,Direction direction,const Delivery& delivery,std::uint64_t now,Message& message,Error& result) noexcept;
    Endpoint authority_{},remote_{};FaultInjectionTransport transport_{};
    TraceFunction trace_=nullptr;void* context_=nullptr;
    std::array<coop_active::InputCmdDraftV2,kMaximumInputRecords-1> history_{};
    std::size_t history_count_=0;std::uint64_t generation_=0;
    bool healthy_=true,initialized_=false,closed_=false;
};
} // namespace isaac_cs_t0::protocol
