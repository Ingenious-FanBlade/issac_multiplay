#pragma once
#include <Windows.h>
#include "isaac_cs_t0/coop_input.hpp"
#include "isaac_cs_t0/protocol_transport.hpp"
#include <array>

namespace isaac_cs_t0::coop_active {
inline constexpr std::uint64_t kMagic=0x315643414F4F4349ull; // ICOOACV1
inline constexpr std::size_t kEvents=32768,kScriptSteps=256;
enum class EventKind : std::uint32_t { Installed=1,DeviceState,Binding,Submit,Resolve,
    QueryResult,WorldWitness,AuthorityCommit,JoinPass,JoinQuery,CreateAttempt,
    Joined,Lifecycle,Retired,Restored,Failure,PhysicalSample,ScriptComplete,Diagnostic,
    NativePlayerCall,NativePlayerStack,StartupInput,PhysicalObserved,ShootingSource,
    ShootingDispatch,QueryRepeat,LifecycleWindow,JoinRoute,JoinOutcome,CharacterSelection,
    AuthoritySessionTransition,AuthorityTickBoundary,ClientInputProgress,AuthorityTickReceipt,
    ClientInputAvailability,InputRuntimeConfig };

enum class DiagnosticStage : std::uint32_t { Topology=1,Binding,DeviceCreate,DeviceRegister,Join,PairLookup };
enum class DiagnosticReason : std::uint32_t {
    Ready=0,RootAbsent,RunInactive,OnlineRoster,OnlineFrame,InputCallbackEnabled,
    CallbackMaskUnavailable,PlayerVector,PlayerEntry,PlayerVtable,PlayerOrdinal,
    TopologyChanged,ReadFault,
    KeyPending=20,PairVector,PairEntry,KeyNotUnique,PhysicalIdentity,OwnedIdentity,Dead,PlayerAbsent,OwnedPlayerKind,
    JoinNotEligible=40,JoinScopeExhausted
};
enum class Error : LONG { None,Arguments,Image,Hook,Thread,Topology,Device,
    Publication,Reentry,Overflow,Protocol,Restore,ReadFault,JoinUnconfirmed,DeadState,Character };
// Exactly one test invocation may include these native-observed phases.
enum class Lifecycle : std::uint32_t { Pause=1,Resume,Room,Loading,LoadingExit,
    Death,Respawn,Rebind,Teardown,RetireB };
struct ScriptStep {
    std::uint64_t tick=0;
    InputStateDraftV2 state{};
    std::uint32_t fault=0; // 1 duplicate,2 expired,3 invalid,4 old generation,5 conflict,6 wrong session
    // IPC v2 source metadata, outside the canonical input codec. Zero preserves
    // legacy vector scripts; bit4 means a raw left/right/up/down mask is present.
    std::uint32_t shoot_directions=0;
};
static_assert(sizeof(ScriptStep)==40);
struct alignas(8) Event {
    volatile LONG64 published=0;
    std::uint64_t tick=0,generation=0,command=0,target=0;
    EventKind kind{};
    std::uint32_t ordinal=UINT32_MAX,action=UINT32_MAX,query=0,decision=0,detail=0;
    float scalar=0;
    std::uint32_t boolean=0;
    InputStateDraftV2 state{};
    std::array<std::uint32_t,4> before{},after{};
};
static_assert(sizeof(Event)==128);
struct alignas(8) Shared {
    std::uint64_t magic=kMagic;
    std::uint32_t version=3,size=sizeof(Shared),pid=0,script_count=0;
    std::array<std::uint8_t,16> session{};
    std::array<std::uint8_t,32> nonce{};
    std::array<std::uint64_t,2> clients{};
    std::uint64_t creation_time=0;
    std::uint32_t controller_pid=0;
    std::int32_t b_character=-1; // -1 keeps native selection; nonnegative ID must be selectable.
    wchar_t loop_name[96]{};
    volatile LONG state=0,error=0,stop=0,retire_b=0; // control: 1 retire B, 2 fresh script, 3 conflict, 4 read-only query repetitions
    volatile LONG main_thread=0,active_callbacks=0,pinned=0,restored=0;
    volatile LONG64 produced=0,consumed=0,completed_tick=0,overwritten=0;
    std::array<ScriptStep,kScriptSteps> script{};
    std::array<Event,kEvents> events{};
};
static_assert(offsetof(Shared,script)==360);
static_assert(offsetof(Shared,b_character)==100);
static_assert(offsetof(Shared,events)==10600);
static_assert(sizeof(Shared)==4204904);
// Optional internal extension; the complete v3 prefix and every old offset stay
// intact. World values use their own mapping, never the input/event ring.
struct alignas(8) SharedV4 {Shared base{};wchar_t world_name[96]{};};
static_assert(sizeof(SharedV4)==4205096);
// Complete v5 prefix contains the v4 world slot and appends one frozen input
// configuration. Old v3/v4 mappings remain accepted and use default config.
struct alignas(8) SharedV5 {SharedV4 base{};InputRuntimeConfig config{};};
static_assert(offsetof(SharedV5,config)==4205096);
static_assert(sizeof(SharedV5)==4205144);
// T0-09 real Authority endpoint extension. Version 7 keeps the complete v5
// event/script/world/input-config prefix and appends only frozen T09 identity;
// packet data lives in the separate Wire IPC mapping.
struct alignas(8) T09AuthorityConfig {
    std::uint32_t version=1,size=sizeof(T09AuthorityConfig),role=1,flags=0;
    std::array<std::uint8_t,16> run_nonce{};
    std::uint64_t run_ticks=0;
    std::array<std::uint8_t,32> build{};
    std::array<std::uint8_t,24> reserved{};
};
static_assert(sizeof(T09AuthorityConfig)==96);
struct alignas(8) SharedV7 {SharedV5 base{};T09AuthorityConfig t09{};};
static_assert(offsetof(SharedV7,t09)==4205144);
static_assert(sizeof(SharedV7)==4205240);
inline constexpr std::size_t kProtocolTraceCapacity=256;
struct alignas(8) ProtocolTraceSlot {
    volatile LONG64 published=0;
    std::uint64_t tick=0,token=0,generation=0;
    std::uint32_t action=0,direction=0,decision=0,length=0;
    std::array<std::uint8_t,protocol::kMaximumPacket> bytes{};
};
static_assert(sizeof(ProtocolTraceSlot)==1248);
struct alignas(8) SharedV6 {
    SharedV5 base{};protocol::FaultConfig protocol_config{};
    volatile LONG64 protocol_produced=0,protocol_consumed=0,protocol_overwritten=0;
    std::array<ProtocolTraceSlot,kProtocolTraceCapacity> protocol_trace{};
};
static_assert(offsetof(SharedV6,protocol_config)==4205144);
static_assert(offsetof(SharedV6,protocol_trace)==4205232);
static_assert(sizeof(SharedV6)==4524720);
inline InputConfigDecision ReadInputRuntimeConfig(const Shared* shared,std::uint32_t version,
    std::uint32_t size,InputRuntimeConfig& output) noexcept
{
    if(!shared) return InputConfigDecision::InvalidMappingSize;
    if(version==3) {
        if(size!=sizeof(Shared)) return InputConfigDecision::InvalidMappingSize;
        output=InputRuntimeConfig{};return InputConfigDecision::Accepted;
    }
    if(version==4) {
        if(size!=sizeof(SharedV4)) return InputConfigDecision::InvalidMappingSize;
        output=InputRuntimeConfig{};return InputConfigDecision::Accepted;
    }
    if(version!=5 && version!=6 && version!=7) return InputConfigDecision::InvalidMappingVersion;
    if(size!=(version==6?sizeof(SharedV6):version==7?sizeof(SharedV7):sizeof(SharedV5)))
        return InputConfigDecision::InvalidMappingSize;
    const auto& config=reinterpret_cast<const SharedV5*>(shared)->config;
    const auto decision=ValidateInputRuntimeConfig(config);
    if(decision!=InputConfigDecision::Accepted) return decision;
    output=config;return InputConfigDecision::Accepted;
}
// Shared production packing used by Native initialization and the offline
// config scenario. The caller owns the surrounding Event publication/slot.
inline void FillInputRuntimeConfigEvent(Event& event,const InputRuntimeConfig& config,
    std::uint32_t mapping_version) noexcept
{
    event.tick=0;event.generation=config.max_input_age_ticks;
    event.command=config.input_lead_ticks;event.target=config.future_window_ticks;
    event.kind=EventKind::InputRuntimeConfig;event.ordinal=UINT32_MAX;
    event.action=config.script_drain_budget;event.query=mapping_version;
    event.decision=config.admission_attempt_budget;event.detail=config.version;
    event.scalar=0;event.boolean=1;event.state={};
    event.before={config.version,config.size,static_cast<std::uint32_t>(config.reserved),
        static_cast<std::uint32_t>(config.reserved>>32)};
    event.after={};
}
} // namespace isaac_cs_t0::coop_active
