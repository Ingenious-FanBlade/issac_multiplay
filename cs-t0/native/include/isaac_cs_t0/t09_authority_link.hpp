#pragma once
#include "isaac_cs_t0/minimal_protocol.hpp"
#include "isaac_cs_t0/t09_wire_ipc.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace isaac_cs_t0::t09 {
using AdmitFunction=std::uint32_t(*)(const coop_active::InputCmdDraftV2&,std::uint64_t,void*) noexcept;

// Authority-side production link for the real game. All Endpoint decode,
// canonical-admission handoff and publication happen on the caller/main thread.
// This class never touches Player, Lua, device or world memory.
class AuthorityWireLink final {
public:
    bool Initialize(const protocol::Config& config,const wchar_t* wire_name,
                    const std::array<std::uint8_t,16>& run_nonce,
                    std::uint64_t run_ticks) noexcept;
    bool PumpInputs(std::uint64_t now,std::uint32_t admission_budget,
                    AdmitFunction admit,void* context,
                    std::uint64_t expected_generation=0) noexcept;
    bool Publish(const protocol::Ack& ack,const protocol::StateObservation& state,
                 std::uint64_t tick) noexcept;
    void MarkGameReady() noexcept;
    void FailInitialization(std::uint32_t code) noexcept;
    void Retire(std::uint64_t now) noexcept;
    bool Healthy() const noexcept {return healthy_;}
    bool Established() const noexcept {return established_;}
    bool RunStarted() const noexcept {return run_started_;}
    std::uint64_t RunStartTick() const noexcept {return run_start_tick_;}
    std::uint64_t EndTick() const noexcept {return end_tick_;}
private:
    bool Send(const protocol::Message& content,std::uint64_t tick) noexcept;
    bool SendPrepared(const protocol::Packet& packet,std::uint64_t tick) noexcept;
    bool DrainInbound(std::uint64_t now,std::uint32_t admission_budget,
                      AdmitFunction admit,void* context,
                      std::uint64_t expected_generation) noexcept;
    protocol::Endpoint endpoint_{};
    WireIpc wire_{};
    std::uint64_t run_ticks_=0,run_start_tick_=0,end_tick_=0;
    std::uint64_t generation_=1,last_heartbeat_=0;
    bool initialized_=false,established_=false,ready_marked_=false;
    bool hello_sent_=false,first_ack_sent_=false;
    bool run_started_=false,publication_complete_=false,closed_=false,healthy_=true;
};
} // namespace isaac_cs_t0::t09
