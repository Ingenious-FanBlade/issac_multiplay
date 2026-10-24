#include "isaac_cs_t0/coop_active_runtime.hpp"
#include "isaac_cs_t0/authority_session.hpp"
#include "isaac_cs_t0/authority_tick.hpp"
#include "isaac_cs_t0/authority_tick_pipeline.hpp"
#include "isaac_cs_t0/coop_active_ipc.hpp"
#include "isaac_cs_t0/owned_virtual_device.hpp"
#include "isaac_cs_t0/coop_sampler.hpp"
#include "isaac_cs_t0/t09_authority_link.hpp"
#include "isaac_cs_t0/active_targets.hpp"
#include "isaac_cs_t0/active_runtime.hpp"
#include "isaac_cs_t0/target_image.hpp"
#include "isaac_cs_t0/hooks.hpp"
#include "isaac_cs_t0/world_record.hpp"
#include <intrin.h>
#include <algorithm>
#include <cstring>
#include <string>
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"user32.lib")

namespace isaac_cs_t0::coop_active {
namespace {
HMODULE g_module=nullptr;
HANDLE g_mapping=nullptr,g_controller=nullptr;
Shared* g_shared=nullptr;
SharedV6* g_protocol_shared=nullptr;
SharedV7* g_t09_shared=nullptr;
protocol::ProtocolLink g_protocol_link{};
t09::AuthorityWireLink g_t09_link{};
bool g_protocol_enabled=false,g_t09_enabled=false;
std::uint64_t g_protocol_startup_generation=0;
std::uint8_t* g_image=nullptr;
volatile LONG g_enabled=0,g_callbacks=0,g_stop_handled=0,g_failed=0,g_hook_owner=0,g_recovery_started=0,g_shutdown_owner=0;
std::array<Participant,2> g_participants{};
AuthoritySession g_authority_session{};
OwnedVirtualDevice g_device{};
std::array<mode::RelativeCallPatch,12> g_relative{};
std::array<mode::PointerPatch,7> g_pointers{};
std::array<std::uint32_t,12> g_relative_ordinals{32,33,34,35,36,37,38,39,40,41,42,43};
std::array<std::uint32_t,7> g_pointer_ordinals{44,45,46,47,48,49,50};
using ThisVoid=void(__thiscall*)(void*);
using VoidFn=void(__cdecl*)();
using ThisBool=bool(__thiscall*)(void*,std::uint32_t);
using ThisFloat=float(__thiscall*)(void*,std::uint32_t);
using ItemFn=void(__thiscall*)(void*,std::int32_t);
using JoinFn=std::uint32_t(__thiscall*)(void*);
using JoinModeFn=bool(__cdecl*)();
using ExitFn=void(__thiscall*)(void*,void*,std::uint32_t);
std::array<ThisVoid,6> g_consumers{};
std::array<ThisBool,4> g_bool_queries{};
std::array<ThisFloat,2> g_scalar_queries{};
ThisVoid g_player_update=nullptr;
VoidFn g_player_only_original=nullptr;
JoinFn g_join_original=nullptr;
JoinModeFn g_join_mode_original=nullptr;
extern "C" void* g_coop_create_original=nullptr;
extern "C" void* g_coop_no_create=nullptr;
AuthorityTickClock g_authority_clock{};
AuthorityTickPipeline g_tick_pipeline{g_authority_session,g_authority_clock,g_participants};
InputRuntimeConfig g_input_config{};
bool g_input_config_frozen=false;
bool ApplyInputConfig(const InputRuntimeConfig& config) noexcept
{
    if(g_input_config_frozen) return false;
    if(ValidateInputRuntimeConfig(config)!=InputConfigDecision::Accepted) return false;
    if(!g_authority_clock.ConfigureFutureWindow(config.future_window_ticks)) return false;
    if(!g_tick_pipeline.Configure(config)) return false;
    for(auto& participant:g_participants) if(!participant.Configure(config)) return false;
    g_input_config=config;g_input_config_frozen=true;return true;
}
bool g_conflict_pending=false;
std::uint64_t g_conflict_generation=0;
struct TickCallState {TickBoundary boundary{};bool observed=false;};
TickCallState g_tick_call{}; // Main-thread native pre/post, including known skipped scopes.
std::uint64_t g_script_origin=0,g_pass=0;
std::uint32_t g_join_phase=0,g_attempt=0,g_script_index=0;
bool g_join_scope=false,g_authority_seen=false,g_session_retired=false,g_ever_registered=false,g_b_retired_done=false;
bool g_b_join_attempted=false;
// Copied once before Hook installation; never reread mutable IPC as Join config.
std::int32_t g_b_character=-1,g_b_selected_character=-1;
void* g_join_root=nullptr;
void* g_join_manager=nullptr;
void* g_join_host=nullptr;
bool g_player_only_scope=false,g_player_only_started=false;
void* g_player_only_return_address=nullptr;
void* g_player_only_owner=nullptr; // The first witnessed Main Player; never dereferenced after the helper.
std::array<std::uint32_t,5> g_join_queries{};
std::array<bool,2> g_dead{};
std::array<raw_apply::detail::ProjectionHistory,2> g_physical_history{};
std::array<ShootingDirections,2> g_shoot_directions{};
std::array<std::uint64_t,2> g_shoot_samples{};
bool g_physical_shoot_reset=true,g_script_shoot_active=false;
std::uint64_t g_script_shoot_generation=0;
InputStateDraftV2 g_last_physical{};
std::uint64_t g_last_physical_tick=UINT64_MAX,g_last_physical_generation=0;
void* g_last_physical_device=nullptr;
Binding g_physical_identity{},g_observed_binding{};
InputStateDraftV2 g_observed_physical{};
std::uint64_t g_observed_loop=UINT64_MAX;
std::uint32_t g_observed_mask=0,g_observed_edges=0,g_published_edges=0,g_observed_blocks=UINT32_MAX,
    g_observed_foreground_pid=0;
bool g_observed_valid=false;
active::detail::PauseGapTracker g_pause{};
active::detail::PostLoadingAuthorityGate g_loading{};
active::detail::LifecycleSubmissionGate g_lifecycle{};
struct ConsumerScope {
    void* player=nullptr;
    std::uint32_t ordinal=UINT32_MAX;
    Consumer consumer=Consumer::None;
    std::uint32_t update_caller=UINT32_MAX;
};
thread_local ConsumerScope g_tls{};
struct PlayerCallSignature {
    void* caller=nullptr;
    std::uint32_t ordinal=UINT32_MAX,phase=0,flags=0;
};
std::array<PlayerCallSignature,32> g_player_call_sources{};
std::size_t g_player_call_source_count=0;
bool g_repeat_queries=false;
std::uint32_t g_repeat_budget=0,g_repeat_group=0;
std::array<std::uint64_t,2> g_repeat_generation{};
std::array<std::array<bool,72>,2> g_repeat_used{};
// Participant contains its full queue and 4096-entry history. Keep this bounded
// read-only diagnostic copy off the game thread's stack; main-thread use only.
std::array<std::uint8_t,sizeof(Participant)> g_repeat_snapshot{};

class Callback final {
public:
    Callback() noexcept {InterlockedIncrement(&g_callbacks);if(g_shared) InterlockedIncrement(&g_shared->active_callbacks);}
    ~Callback() {if(g_shared) InterlockedDecrement(&g_shared->active_callbacks);InterlockedDecrement(&g_callbacks);}
};
bool Enabled() noexcept {return InterlockedCompareExchange(&g_enabled,0,0)!=0;}
bool MainThread() noexcept
{
    const auto context=loop::CurrentObservationContext();
    if (!context.main_thread_id || context.thread_id!=context.main_thread_id) return false;
    const LONG previous=InterlockedCompareExchange(&g_shared->main_thread,
        static_cast<LONG>(context.thread_id),0);
    return !previous || previous==static_cast<LONG>(context.thread_id);
}
Event MakeEvent(EventKind kind,std::uint32_t ordinal=UINT32_MAX) noexcept
{
    Event event{};event.kind=kind;event.tick=g_authority_clock.Current()?g_authority_clock.Current():g_authority_clock.Completed();event.ordinal=ordinal;
    if (ordinal<2) {const auto& p=g_participants[ordinal];event.generation=p.Generation();
        event.command=p.CurrentSequence();event.state=p.Current();}
    return event;
}
// Keep the aligned aggregate out of by-value x86 LTCG calls: tx-01-46's
// generated stack callers disagreed with its register-pointer callee.
bool Publish(const Event& event) noexcept
{
    if (!g_shared) return false;
    const LONG64 next=InterlockedIncrement64(&g_shared->produced);
    if (next-InterlockedCompareExchange64(&g_shared->consumed,0,0)>static_cast<LONG64>(kEvents)) {
        // Terminal admission does not depend on successfully publishing an event.
        g_authority_session.Fail(SessionCause::PublicationFailure);
        InterlockedIncrement64(&g_shared->overwritten);
        if(!InterlockedExchange(&g_failed,1)) {
            InterlockedExchange(&g_shared->error,static_cast<LONG>(Error::Overflow));
            InterlockedExchange(&g_shared->state,5);
        }
        InterlockedExchange(&g_enabled,0);g_device.Neutralize();return false;
    }
    auto& slot=g_shared->events[static_cast<std::size_t>(next-1)%kEvents];
    InterlockedExchange64(&slot.published,0);
    std::memcpy(reinterpret_cast<std::uint8_t*>(&slot)+8,
        reinterpret_cast<const std::uint8_t*>(&event)+8,sizeof(Event)-8);
    MemoryBarrier();InterlockedExchange64(&slot.published,next);return true;
}
bool PublishInputRuntimeConfig(const InputRuntimeConfig& config,std::uint32_t mapping_version) noexcept
{
    Event event{};
    FillInputRuntimeConfigEvent(event,config,mapping_version);
    return Publish(event);
}
std::array<std::uint32_t,4> ProgressWords(std::uint64_t first,std::uint64_t second) noexcept
{return {static_cast<std::uint32_t>(first),static_cast<std::uint32_t>(first>>32),
    static_cast<std::uint32_t>(second),static_cast<std::uint32_t>(second>>32)};}
// Main-thread retirement only: at most 30 extra rows per client/session. Queries
// and submissions never emit these snapshots. Publication failure still permits
// the caller to complete device/Player cleanup through the existing path.
void PublishClientProgress(std::uint32_t ordinal) noexcept
{
    const auto p=g_participants[ordinal].Progress(g_authority_clock.Completed());
    Event event{};event.kind=EventKind::ClientInputProgress;event.ordinal=ordinal;
    event.tick=g_authority_clock.Completed();event.generation=p.generation;
    event.detail=(p.has_received?1u:0u)|(p.current_valid?2u:0u)|(p.last_state_valid?4u:0u)|
        (p.future_window_valid?8u:0u)|(p.scope_pending?16u:0u)|(p.retired?32u:0u)|(p.counters_saturated?64u:0u);
    event.query=0;event.command=p.last_accepted_sequence;event.target=p.last_processed_sequence;
    event.state=p.last_state;event.before=ProgressWords(p.last_received_tick,p.last_processed_tick);
    event.after=ProgressWords(p.last_resolved_tick,p.last_accepted_target_tick);if(!Publish(event))return;
    event.state={};event.query=1;event.command=p.next_sequence;event.target=p.last_received_sequence;
    event.decision=p.last_received_decision;event.boolean=p.future_window_valid?1u:0u;
    event.before=ProgressWords(p.last_received_target_tick,p.last_received_generation);
    event.after=ProgressWords(p.future_first_tick,p.future_last_tick);if(!Publish(event))return;
    event.query=2;event.command=p.submit_count;event.target=p.received_count;event.decision=0;event.boolean=0;
    event.before=ProgressWords(p.accepted_count,p.duplicate_count);
    event.after=ProgressWords(p.rejected_count,p.expired_before_resolve);if(!Publish(event))return;
    event.query=3;event.command=p.discarded_on_invalidate;event.target=p.last_processed_source_sequence;
    event.before=ProgressWords(p.last_state_tick,p.last_state_generation);
    event.after={static_cast<std::uint32_t>(p.last_resolved_sequence),static_cast<std::uint32_t>(p.last_resolved_sequence>>32),
        p.resolved_contributors,p.processed_contributors};if(!Publish(event))return;
    for(std::uint32_t reason=3;reason<p.rejections.size();++reason) {
        event.query=4;event.action=reason;event.command=p.rejections[reason];event.target=0;event.before={};event.after={};
        if(!Publish(event))return;
    }
    for(std::uint32_t action=0;action<p.edges.size();++action) {
        const auto& edge=p.edges[action];event.action=action;
        event.query=5;event.command=edge.accepted_sequence;event.target=edge.accepted_tick;
        event.before=ProgressWords(edge.accepted_generation,0);if(!Publish(event))return;
        event.query=6;event.command=edge.resolved_sequence;event.target=edge.resolved_tick;
        event.before=ProgressWords(edge.resolved_generation,0);if(!Publish(event))return;
        event.query=7;event.command=edge.processed_sequence;event.target=edge.processed_tick;
        event.before=ProgressWords(edge.processed_generation,edge.processed_count);if(!Publish(event))return;
    }
    event.query=8;event.action=(p.availability.effective_available?1u:0u)|
        (p.availability.neutral_applied?2u:0u)|(p.availability.has_valid_arrival?4u:0u);
    event.command=p.availability.last_valid_arrival_sequence;
    event.target=p.availability.effective_source_sequence;
    event.decision=static_cast<std::uint32_t>(p.availability.missing_reason);
    event.detail=static_cast<std::uint32_t>(p.availability.source);
    event.boolean=p.availability.source==SourceActivity::TimedOut?1u:0u;
    event.before=ProgressWords(p.availability.last_valid_arrival_tick,p.availability.effective_source_tick);
    event.after=ProgressWords(p.availability.effective_age_ticks,p.availability.silent_ticks);
    if(!Publish(event))return;
    event.query=9;event.command=p.availability.valid_arrival_count;event.target=p.availability.silent_entries;
    event.action=static_cast<std::uint32_t>(p.availability.timeout_recoveries);
    event.decision=static_cast<std::uint32_t>(p.availability.silence_recoveries);
    event.detail=static_cast<std::uint32_t>(p.availability.timeout_entries);
    event.boolean=p.availability.has_valid_arrival?1u:0u;
    event.before=ProgressWords(kMaximumInputAgeTicks,kClientSilenceTimeoutTicks);
    event.after=ProgressWords(p.availability.last_valid_arrival_generation,0);
    if(!Publish(event))return;
}
void PublishSessionTransition(SessionStatus before) noexcept
{
    const auto after=g_authority_session.Status();
    if(before.state==after.state && before.cause==after.cause) return;
    // No game/Participant reads: also safe during initializer and drained shutdown.
    Event event{};event.kind=EventKind::AuthoritySessionTransition;
    event.action=static_cast<std::uint32_t>(before.state);
    event.decision=static_cast<std::uint32_t>(after.state);event.detail=static_cast<std::uint32_t>(after.cause);
    const auto& identity=g_authority_session.Identity();
    std::memcpy(event.before.data(),identity.session.data(),16);
    std::memcpy(event.after.data(),identity.clients.data(),16);Publish(event);
}
std::uint32_t Address(const void* pointer) noexcept
{return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer));}
struct DiagnosticState {
    bool seen=false;
    std::uint32_t reason=0;
    std::array<std::uint32_t,4> before{},after{};
};
std::array<std::array<DiagnosticState,3>,6> g_diagnostics{};
bool Diagnose(DiagnosticStage stage,std::uint32_t ordinal,DiagnosticReason reason,
    const std::array<std::uint32_t,4>& before={},const std::array<std::uint32_t,4>& after={}) noexcept
{
    const auto code=static_cast<std::uint32_t>(reason);
    auto& previous=g_diagnostics[static_cast<std::size_t>(stage)-1][ordinal<2?ordinal:2];
    if(!previous.seen || previous.reason!=code || previous.before!=before || previous.after!=after) {
        previous={true,code,before,after};
        auto event=MakeEvent(EventKind::Diagnostic,ordinal);
        event.action=static_cast<std::uint32_t>(stage);event.detail=code;event.boolean=code==0;
        event.before=before;event.after=after;Publish(event);
    }
    return code==0;
}
void FailFrom(Error error,SessionCause cause,SessionStatus before,
    std::uint32_t native_hook_result=0) noexcept
{
    if(error==Error::Hook) cause=SessionCause::HookFailure;
    else if(error==Error::Restore) cause=SessionCause::RestoreFailure;
    else if(error==Error::Arguments) cause=SessionCause::Configuration;
    g_authority_session.Fail(cause);
    InterlockedExchange(&g_enabled,0);g_device.Neutralize();
    if (InterlockedExchange(&g_failed,1)) return;
    InterlockedExchange(&g_shared->error,static_cast<LONG>(error));
    InterlockedExchange(&g_shared->state,5);
    if(g_t09_enabled)g_t09_link.FailInitialization(static_cast<std::uint32_t>(error));
    PublishSessionTransition(before);
    auto event=MakeEvent(EventKind::Failure);event.detail=static_cast<std::uint32_t>(error);
    event.query=native_hook_result;Publish(event);
}
void Fail(Error error,SessionCause cause=SessionCause::RuntimeFailure,
    std::uint32_t native_hook_result=0) noexcept
{FailFrom(error,cause,g_authority_session.Status(),native_hook_result);}
TickBoundary CurrentTickBoundary(void* owner,void* return_address) noexcept
{
    const auto context=loop::CurrentObservationContext();
    TickBoundary boundary{};boundary.kind=g_player_only_scope?TickKind::OddPlayer:TickKind::StandardRoom;
    boundary.loop=context.loop_iteration;boundary.thread=context.thread_id;boundary.main_thread=context.main_thread_id;
    boundary.owner=reinterpret_cast<std::uintptr_t>(owner);
    boundary.return_rva=Address(return_address)-Address(g_image);
    boundary.player_return_rva=g_player_only_scope && g_player_only_started?0x2FD60Cu:0;
    return boundary;
}
void PublishTickBoundary(TickDecision decision,const TickBoundary& boundary) noexcept
{
    auto event=MakeEvent(EventKind::AuthorityTickBoundary);
    event.target=boundary.loop;event.query=static_cast<std::uint32_t>(boundary.kind);
    event.action=boundary.return_rva;event.detail=boundary.player_return_rva;
    event.decision=static_cast<std::uint32_t>(decision);
    const auto completed=g_authority_clock.Completed(),current=g_authority_clock.Current();
    event.before={static_cast<std::uint32_t>(completed),static_cast<std::uint32_t>(completed>>32),
        static_cast<std::uint32_t>(current),static_cast<std::uint32_t>(current>>32)};
    event.after={boundary.thread,boundary.main_thread,static_cast<std::uint32_t>(boundary.owner),
        g_authority_clock.Closed()?1u:0u};Publish(event);
}
void RejectTick(TickDecision decision,const TickBoundary& boundary) noexcept
{
    g_tick_pipeline.Abort(decision);
    world::PublishPostState(false);
    PublishTickBoundary(decision,boundary);
    if(decision==TickDecision::Cancelled || decision==TickDecision::Closed) return;
    Fail(decision==TickDecision::Exhausted?Error::Overflow:
        decision==TickDecision::Reentry || decision==TickDecision::DuplicateLoop ||
        decision==TickDecision::MissingReturn?Error::Reentry:Error::Protocol);
}
void Invalidate(std::uint32_t ordinal,Lifecycle reason) noexcept
{
    g_participants[ordinal].Invalidate();
    world::PlayerAnchor(ordinal,nullptr,false);
    // Physical rising-edge history follows the actual device, not a gameplay
    // generation. Held keys across a room/pause must not become new edges.
    if (!ordinal) g_last_physical_tick=UINT64_MAX;
    auto event=MakeEvent(EventKind::Lifecycle,ordinal);event.detail=static_cast<std::uint32_t>(reason);Publish(event);
}
void InvalidateBoth(Lifecycle reason) noexcept {Invalidate(0,reason);Invalidate(1,reason);}
struct Topology {void* root=nullptr;std::array<void*,2> players{};std::uint32_t count=0;};
bool ReadTopology(Topology& topology) noexcept
{
    __try {
        topology={};topology.root=*reinterpret_cast<void**>(g_image+0x871678);
        constexpr auto stage=DiagnosticStage::Topology;
        if (!topology.root) return Diagnose(stage,UINT32_MAX,DiagnosticReason::RootAbsent);
        auto* root=static_cast<std::uint8_t*>(topology.root);
        if (!root[0x2658A]) return Diagnose(stage,UINT32_MAX,DiagnosticReason::RunInactive,{Address(root)});
        auto* online=*reinterpret_cast<std::uint8_t**>(g_image+0x87169C);
        std::uint32_t mode=UINT32_MAX,callback_mask=0;
        const auto* lua=*reinterpret_cast<const std::uint8_t**>(g_image+0x871690);
        if (online) {
            // Observe isolation only; never use Online Manager to provide C/S input/state.
            const auto begin=*reinterpret_cast<std::uintptr_t*>(online+0x4B3D8);
            const auto end=*reinterpret_cast<std::uintptr_t*>(online+0x4B3DC);
            if (begin!=end) return Diagnose(stage,UINT32_MAX,DiagnosticReason::OnlineRoster,
                {static_cast<std::uint32_t>(begin),static_cast<std::uint32_t>(end)});
            const auto frame=*reinterpret_cast<std::uint32_t*>(online+0x4B3E4);
            if(frame) return Diagnose(stage,UINT32_MAX,DiagnosticReason::OnlineFrame,{frame});
            mode=*reinterpret_cast<std::uint32_t*>(online+8);
            if(mode==2 && lua) {
                // 864C30/864D80 fall through unless callback 13 is enabled.
                // Engine presence alone does not mean it overrides a reader.
                const auto* mask=*reinterpret_cast<const std::uint32_t* const*>(lua);
                if(!mask) return Diagnose(stage,UINT32_MAX,DiagnosticReason::CallbackMaskUnavailable,
                    {mode,Address(lua)});
                callback_mask=*mask;
                if(callback_mask&0x2000u) return Diagnose(stage,UINT32_MAX,DiagnosticReason::InputCallbackEnabled,
                    {mode,1,callback_mask,root[0x2658A]});
            }
        }
        auto** begin=*reinterpret_cast<void***>(root+0x1BAA8);
        auto** end=*reinterpret_cast<void***>(root+0x1BAAC);
        const auto first=reinterpret_cast<std::uintptr_t>(begin),last=reinterpret_cast<std::uintptr_t>(end);
        const auto bytes=last-first;
        if (!begin || last<first || bytes<4 || bytes>8 || bytes%4)
            return Diagnose(stage,UINT32_MAX,DiagnosticReason::PlayerVector,{Address(begin),Address(end)});
        topology.count=static_cast<std::uint32_t>(bytes/4);
        for (std::uint32_t i=0;i<topology.count;++i) {
            auto* p=static_cast<std::uint8_t*>(begin[i]);
            if (!p) return Diagnose(stage,UINT32_MAX,DiagnosticReason::PlayerEntry,{i});
            const auto* vtable=*reinterpret_cast<void**>(p);
            if(vtable!=g_image+0x76BDD0) return Diagnose(stage,UINT32_MAX,DiagnosticReason::PlayerVtable,
                {i,Address(p),Address(vtable),Address(g_image+0x76BDD0)});
            const auto ordinal=*reinterpret_cast<std::uint32_t*>(p+0x161C);
            if(ordinal!=i) return Diagnose(stage,UINT32_MAX,DiagnosticReason::PlayerOrdinal,{i,ordinal,Address(p)});
            topology.players[i]=p;
        }
        MemoryBarrier();
        if(topology.root!=*reinterpret_cast<void**>(g_image+0x871678) ||
            begin!=*reinterpret_cast<void***>(root+0x1BAA8) || end!=*reinterpret_cast<void***>(root+0x1BAAC))
            return Diagnose(stage,UINT32_MAX,DiagnosticReason::TopologyChanged);
        return Diagnose(stage,UINT32_MAX,DiagnosticReason::Ready,
            {mode,lua?1u:0u,callback_mask,root[0x2658A]},
            {Address(root),Address(begin),Address(end),topology.count});
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return Diagnose(DiagnosticStage::Topology,UINT32_MAX,DiagnosticReason::ReadFault);
    }
}
bool FindBinding(void* player,Binding& binding) noexcept
{
    __try {
        binding={};binding.player=player;
        const auto ordinal=*reinterpret_cast<std::uint32_t*>(static_cast<std::uint8_t*>(player)+0x161C);
        binding.key=*reinterpret_cast<std::int32_t*>(static_cast<std::uint8_t*>(player)+0x1618);
        if (binding.key<0) return Diagnose(DiagnosticStage::PairLookup,ordinal,DiagnosticReason::KeyPending,
            {Address(player),static_cast<std::uint32_t>(binding.key)});
        auto* manager=g_image+0x857B18;
        auto* begin=*reinterpret_cast<DevicePair**>(manager+8);
        auto* end=*reinterpret_cast<DevicePair**>(manager+12);
        const auto first=reinterpret_cast<std::uintptr_t>(begin),last=reinterpret_cast<std::uintptr_t>(end);
        if (last<first || last-first>32*8 || (last-first)%8 || (!begin && end))
            return Diagnose(DiagnosticStage::PairLookup,ordinal,DiagnosticReason::PairVector,{Address(begin),Address(end)});
        std::uint32_t matches=0;
        for (auto* pair=begin;pair!=end;++pair) {
            if (!pair->object || !pair->control) return Diagnose(DiagnosticStage::PairLookup,ordinal,
                DiagnosticReason::PairEntry,{Address(pair->object),Address(pair->control)});
            if (*reinterpret_cast<std::int32_t*>(static_cast<std::uint8_t*>(pair->object)+12)==binding.key) {
                ++matches;binding.device=pair->object;binding.control=pair->control;
            }
        }
        return Diagnose(DiagnosticStage::PairLookup,ordinal,matches==1?DiagnosticReason::Ready:DiagnosticReason::KeyNotUnique,
            {Address(player),static_cast<std::uint32_t>(binding.key),matches,static_cast<std::uint32_t>((last-first)/8)},
            {Address(binding.device),Address(binding.control)});
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return Diagnose(DiagnosticStage::PairLookup,UINT32_MAX,DiagnosticReason::ReadFault);
    }
}
bool Dead(void* player) noexcept
{
    // IDA-DIRECT 417470: mov al,[ecx+173h]; 42A320: mov al,[ecx+20A9h].
    const auto* p=static_cast<const std::uint8_t*>(player);return p[0x173]!=0 || p[0x20A9]!=0;
}
bool FullPlayer(void* player) noexcept
{
    __try {
        const auto* p=static_cast<const std::uint8_t*>(player);
        return p && *reinterpret_cast<const std::uint32_t*>(p+0x28)==1 &&
            *reinterpret_cast<const std::uint32_t*>(p+0x2C)==0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool ReadJoinHealth(void* player,std::array<std::uint32_t,4>& health) noexcept
{
    __try {
        const auto* p=static_cast<const std::uint8_t*>(player);
        health={*reinterpret_cast<const std::uint32_t*>(p+0x1340),
            *reinterpret_cast<const std::uint32_t*>(p+0x1344),
            *reinterpret_cast<const std::uint32_t*>(p+0x134C),
            *reinterpret_cast<const std::uint32_t*>(p+0x1D88)};
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool JoinContextValid() noexcept
{
    if(!Enabled() || !MainThread() || !g_join_scope || g_attempt || g_session_retired ||
       g_pause.BlocksInput() || g_lifecycle.BlocksInput() || !g_device.Registered() ||
       !g_device.Healthy() || !g_device.ExactMember()) return false;
    Topology topology{};Binding host{};
    return ReadTopology(topology) && topology.root==g_join_root && topology.count==1 &&
        topology.players[0]==g_join_host && g_join_manager==static_cast<std::uint8_t*>(topology.root)+0x1BAA8 &&
        g_participants[0].BoundValid() && FindBinding(g_join_host,host) && host==g_participants[0].Bound();
}
bool ReadJoinRowKey(void* manager,void* selector,std::int32_t& key) noexcept
{
    // Four native selector rows, selected-index at manager+14h+20*i.
    const auto row=Address(selector)-Address(manager);
    if(row<0x14 || row>0x50 || (row-0x14)%20) return false;
    __try {
        key=*reinterpret_cast<const std::int32_t*>(static_cast<const std::uint8_t*>(selector)-8);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool ReadJoinFlags(void* root,std::uint32_t& flags) noexcept
{
    __try {
        if(!root || root!=*reinterpret_cast<void**>(g_image+0x871678)) return false;
        flags=*reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(root)+0x26548);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool SelectableCharacter(void* manager,std::int32_t id,std::uint32_t& count) noexcept
{
    // IDA 5BB313/327/32A: manager+70h/74h holds IDs, not selector indexes.
    __try {
        count=0;if(id<0) return false; // Native -1 is possessor/random, never an explicit role.
        const auto* base=static_cast<const std::uint8_t*>(manager);
        const auto* begin=*reinterpret_cast<const std::int32_t* const*>(base+0x70);
        const auto* end=*reinterpret_cast<const std::int32_t* const*>(base+0x74);
        const auto first=reinterpret_cast<std::uintptr_t>(begin),last=reinterpret_cast<std::uintptr_t>(end);
        if(!begin || last<=first || first%4 || (last-first)%4 || (last-first)/4>4096) return false;
        count=static_cast<std::uint32_t>((last-first)/4);
        bool found=false;
        for(std::uint32_t i=0;i<count;++i) if(begin[i]==id) found=true;
        MemoryBarrier();
        return found && begin==*reinterpret_cast<const std::int32_t* const*>(base+0x70) &&
            end==*reinterpret_cast<const std::int32_t* const*>(base+0x74);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
void RefreshBindings(const Topology& topology) noexcept
{
    for (std::uint32_t ordinal=0;ordinal<2;++ordinal) {
        auto& participant=g_participants[ordinal];if (participant.Retired()) continue;
        void* player=topology.players[ordinal];
        if (!player) {
            Diagnose(DiagnosticStage::Binding,ordinal,DiagnosticReason::PlayerAbsent);
            if (participant.BoundValid()) Invalidate(ordinal,Lifecycle::Rebind);continue;
        }
        if(ordinal && !FullPlayer(player)) {
            Diagnose(DiagnosticStage::Binding,ordinal,DiagnosticReason::OwnedPlayerKind,{Address(player)});
            if(participant.BoundValid()) Invalidate(ordinal,Lifecycle::Rebind);continue;
        }
        const bool dead=Dead(player);
        if (dead!=g_dead[ordinal]) {g_dead[ordinal]=dead;Invalidate(ordinal,dead?Lifecycle::Death:Lifecycle::Respawn);}
        if (dead) {Diagnose(DiagnosticStage::Binding,ordinal,DiagnosticReason::Dead,{Address(player)});continue;}
        Binding binding{};
        if (!FindBinding(player,binding)) {if (participant.BoundValid()) Invalidate(ordinal,Lifecycle::Rebind);continue;}
        bool identity=false;
        if (!ordinal) {
            void* vptr=*static_cast<void**>(binding.device);
            identity=vptr==g_image+0x7A1678 || vptr==g_image+0x782C80;
        } else identity=binding.device==g_device.Pair().object && binding.control==g_device.Pair().control &&
            binding.key==g_device.Key() && g_attempt==1;
        if (ordinal && g_attempt==2) identity=binding.device==g_device.Pair().object &&
            binding.control==g_device.Pair().control && binding.key==g_device.Key();
        if (!identity) {
            Diagnose(DiagnosticStage::Binding,ordinal,ordinal?DiagnosticReason::OwnedIdentity:DiagnosticReason::PhysicalIdentity,
                {Address(binding.device),Address(binding.control),static_cast<std::uint32_t>(binding.key),g_attempt},
                {Address(*static_cast<void**>(binding.device)),Address(g_device.Pair().object),Address(g_device.Pair().control)});
            if (participant.BoundValid()) Invalidate(ordinal,Lifecycle::Rebind);continue;
        }
        Diagnose(DiagnosticStage::Binding,ordinal,DiagnosticReason::Ready,
            {Address(player),Address(binding.device),Address(binding.control),static_cast<std::uint32_t>(binding.key)});
        const bool changed=!participant.BoundValid() || !(participant.Bound()==binding);
        if(changed && participant.BoundValid()) Invalidate(ordinal,Lifecycle::Rebind);
        if (changed && participant.Bind(binding)) {
            auto event=MakeEvent(EventKind::Binding,ordinal);event.action=static_cast<std::uint32_t>(binding.key);
            event.before={Address(player),Address(binding.device),Address(binding.control),ordinal};
            event.after={static_cast<std::uint32_t>(participant.Client()),static_cast<std::uint32_t>(participant.Client()>>32),ordinal,0};
            Publish(event);
        }
        world::PlayerAnchor(ordinal,player,participant.BoundValid());
    }
}
Decision Submit(std::uint32_t ordinal,const InputCmdDraftV2& command,std::uint64_t generation) noexcept
{
    const auto authority_before=g_authority_session.Status();
    EncodedInputCmdV2 bytes{};
    // Both local A and decoded B enter the same byte admission/validator.
    // Preserve malformed legacy fault payloads for the validator's own rejection.
    if(!Encode(command,bytes))std::memcpy(bytes.data(),&command,bytes.size());
    const auto decision=g_tick_pipeline.AcceptInput(ordinal,std::span<const std::uint8_t>{bytes},generation);
    auto event=MakeEvent(EventKind::Submit,ordinal);event.command=command.sequence;
    event.target=command.target_tick;event.state=command.state;event.decision=static_cast<std::uint32_t>(decision);
    if(g_protocol_enabled || g_t09_enabled) {
        std::memcpy(event.before.data(),command.session_id.data(),16);
        event.after=ProgressWords(command.client_id,generation);
    }
    Publish(event);
    if (decision==Decision::SequenceConflict && !ordinal)
        FailFrom(Error::Protocol,SessionCause::RuntimeFailure,authority_before);
    if(g_tick_pipeline.Status().phase==TickPipelinePhase::Failed)
        FailFrom(Error::Protocol,SessionCause::RuntimeFailure,authority_before);
    return decision;
}
Decision SubmitProduced(std::uint32_t ordinal,const InputCmdDraftV2& command,
    std::uint64_t generation) noexcept
{
    if(!ordinal) {
        auto sample=MakeEvent(EventKind::PhysicalSample,0);sample.command=command.sequence;
        sample.query=2;sample.action=g_observed_foreground_pid;
        sample.boolean=g_observed_foreground_pid==GetCurrentProcessId()?1u:0u;
        sample.target=command.target_tick;sample.state=command.state;sample.detail=1;Publish(sample);
    }
    return Submit(ordinal,command,generation);
}
bool SubmitFresh(std::uint32_t ordinal,const InputStateDraftV2& state,
    std::uint64_t nominal_target,bool duplicate=false) noexcept
{
    if(!g_authority_session.AcceptsInput()) return false;
    auto& participant=g_participants[ordinal];const auto generation=participant.Generation();
    if(g_protocol_enabled && ordinal==1) {
        // The first fresh source sample uses the same +1 startup intent as A.
        // Transport delay may expire it; never regenerate it from receiver state.
        const auto completed=g_authority_clock.Completed();
        if(duplicate) {Fail(Error::Protocol);return false;}
        if(g_protocol_startup_generation!=generation) {
            std::uint64_t first_target=0;
            if(!NextStartupTarget(completed,first_target)) {Fail(Error::Protocol);return false;}
            const auto first=participant.MakeCommand(state,first_target);
            auto levels=state;levels.edge_flags=0;
            const auto nominal=participant.MakeCommand(levels,nominal_target);
            if(!g_protocol_link.SendInput(first,generation,completed) ||
               !g_protocol_link.SendInput(nominal,generation,completed)) {Fail(Error::Protocol);return false;}
            g_protocol_startup_generation=generation;
            auto event=MakeEvent(EventKind::StartupInput,ordinal);event.command=first.sequence;
            event.target=first.target_tick;event.state=state;event.detail=ordinal+1;
            event.query=static_cast<std::uint32_t>(loop::CurrentObservationContext().loop_iteration);
            event.before=ProgressWords(nominal.sequence,nominal.target_tick);
            event.after={Address(participant.Bound().player),Address(participant.Bound().device),
                Address(participant.Bound().control),static_cast<std::uint32_t>(participant.Bound().key)};
            return Publish(event);
        }
        const auto command=participant.MakeCommand(state,nominal_target);
        if(!g_protocol_link.SendInput(command,generation,completed)) {
            Fail(Error::Protocol);return false;
        }
        return true;
    }
    std::uint64_t configured_target=0;
    if(!NextInputTarget(g_authority_clock.Completed(),g_input_config,configured_target)) {
        Fail(Error::Protocol);return false;
    }
    const bool startup=participant.StartupPending() && !g_authority_clock.Current() &&
        nominal_target==configured_target;
    if(startup) {
        std::uint64_t first_target=0;
        if(!NextStartupTarget(g_authority_clock.Completed(),first_target)) {
            Fail(Error::Protocol);return false;
        }
        const auto first=participant.MakeCommand(state,first_target);
        if(SubmitProduced(ordinal,first,generation)!=Decision::Accepted) {
            if(g_authority_session.AcceptsInput()) Fail(Error::Protocol);return false;
        }
        if(duplicate) Submit(ordinal,first,generation);
        auto levels=state;levels.edge_flags=0;
        const auto nominal=participant.MakeCommand(levels,nominal_target);
        if(SubmitProduced(ordinal,nominal,generation)!=Decision::Accepted) {
            if(g_authority_session.AcceptsInput()) Fail(Error::Protocol);return false;
        }
        if(duplicate) Submit(ordinal,nominal,generation);
        auto event=MakeEvent(EventKind::StartupInput,ordinal);event.command=first.sequence;
        event.target=first.target_tick;event.state=state;event.detail=ordinal+1;
        event.query=static_cast<std::uint32_t>(loop::CurrentObservationContext().loop_iteration);
        event.before={static_cast<std::uint32_t>(nominal.sequence),static_cast<std::uint32_t>(nominal.sequence>>32),
            static_cast<std::uint32_t>(nominal.target_tick),static_cast<std::uint32_t>(nominal.target_tick>>32)};
        event.after={Address(participant.Bound().player),Address(participant.Bound().device),
            Address(participant.Bound().control),static_cast<std::uint32_t>(participant.Bound().key)};
        Publish(event);return true;
    }
    const auto command=participant.MakeCommand(state,nominal_target);
    const auto accepted=SubmitProduced(ordinal,command,generation)==Decision::Accepted;
    if(duplicate) Submit(ordinal,command,generation);
    return accepted;
}
void PublishShootingSource(std::uint32_t ordinal,const std::array<float,4>& directions,
    std::uint32_t previous_held,std::uint32_t previous_selected,const InputStateDraftV2& state,
    std::uint32_t source,std::uint64_t target,bool reset,std::uint32_t script_index=UINT32_MAX) noexcept
{
    auto event=MakeEvent(EventKind::ShootingSource,ordinal);
    event.command=++g_shoot_samples[ordinal];event.target=target;event.detail=source;
    event.action=g_shoot_directions[ordinal].HeldMask();event.query=g_shoot_directions[ordinal].SelectedMask();
    event.state=state;std::memcpy(event.before.data(),directions.data(),sizeof(event.before));
    event.after={previous_held,previous_selected,reset?1u:0u,script_index};Publish(event);
}
void ScriptInput() noexcept
{
    // T0-09 Authority receives ordinal1 canonical inputs only from the remote
    // endpoint wire; local script replay belongs to the remote observer (T0-09-C).
    if(g_t09_enabled)return;
    if(!g_authority_session.AcceptsInput()) return;
    auto& participant=g_participants[1];
    if (!participant.BoundValid() || g_pause.BlocksInput() || g_lifecycle.BlocksInput()) return;
    std::uint64_t target=0;
    if(!NextInputTarget(g_authority_clock.Completed(),g_input_config,target)) {
        Fail(Error::Protocol);return;
    }
    if (!g_script_origin) g_script_origin=target;
    const auto relative=target-g_script_origin;
    std::uint32_t drained=0;
    while (g_script_index<g_shared->script_count && ScriptDrainAllowed(drained,g_input_config)) {
        ++drained;
        const auto& step=g_shared->script[g_script_index];if (step.tick>relative) break;
        ++g_script_index;
        // Late scripted intents expire rather than sliding an edge into a future target.
        if(step.fault<=1 && g_script_origin+step.tick==target) {
            auto state=step.state;
            if(step.shoot_directions) {
                const bool reset=!g_script_shoot_active || g_script_shoot_generation!=participant.Generation();
                if(reset) g_shoot_directions[1]={};
                auto& history=g_shoot_directions[1];
                const auto previous_held=history.HeldMask(),previous_selected=history.SelectedMask();
                std::array<float,4> directions{};
                for(std::uint32_t i=0;i<4;++i) if(step.shoot_directions&(1u<<i)) directions[i]=1;
                input::RawInputSample raw{};raw.held_flags=state.held_flags;raw.edge_flags=state.edge_flags;
                if(!CanonicalizeShooting(raw,directions,history,state)) {Fail(Error::Protocol);return;}
                // Only shooting is raw in this script form. The movement pair
                // was already validated as canonical V2; do not deadzone it twice.
                state.move_x=step.state.move_x;state.move_y=step.state.move_y;
                g_script_shoot_active=true;g_script_shoot_generation=participant.Generation();
                PublishShootingSource(1,directions,previous_held,previous_selected,state,3,target,reset,g_script_index-1);
            } else g_script_shoot_active=false;
            SubmitFresh(1,state,target,step.fault==1);if(!Enabled()) return;continue;
        }
        auto command=participant.MakeCommand(step.state,g_script_origin+step.tick);
        const auto generation=participant.Generation();
        if(g_protocol_enabled) {
            // An overdue intent still crosses the whole wire path with its
            // original target; the common validator expires it. Never bypass
            // the protocol or slide an old edge into a fresh target.
            if(!g_protocol_link.SendInput(command,generation,g_authority_clock.Completed()))Fail(Error::Protocol);
            if(!Enabled())return;continue;
        }
        if (step.fault==2) command.target_tick=g_authority_clock.Completed();
        if (step.fault==3) command.state.reserved=1;
        if (step.fault==6) command.session_id[0]^=0x80;
        Submit(1,command,step.fault==4?generation-1:generation);
        if (step.fault==1) Submit(1,command,generation);
        if (step.fault==5) {command.state.move_x=command.state.move_x?0:32767;Submit(1,command,generation);}
    }
    if (g_script_index==g_shared->script_count && g_shared->script_count) {
        auto event=MakeEvent(EventKind::ScriptComplete,1);event.detail=g_script_index;Publish(event);
        // Once-only completion marker, no implicit B retirement.
        ++g_script_index;
    }
}
bool PhysicalSample(void* device,const Binding& binding,input::RawInputSample& raw,
    std::uint32_t& held_mask,InputStateDraftV2& canonical) noexcept
{
    if (device!=binding.device) return false;
    const bool keyboard=*static_cast<void**>(device)==g_image+0x7A1678;
    std::array<float,input::kGameplayActionCount> values{};std::uint16_t valid=0;
    const bool ok=keyboard?sampler::ReadKeyboardProjection(g_image,device,values,valid):
        sampler::ReadGamepadProjection(g_image,device,values,valid);
    if (!ok || valid!=0xFFF) return false;
    for(std::uint32_t action=0;action<12;++action) if(values[action]!=0) held_mask|=1u<<action;
    auto& history=g_physical_history[keyboard?0:1];
    const auto edges=raw_apply::detail::UpdateProjectionHistory(history,
        static_cast<std::uint32_t>(binding.key),values,valid);
    raw.move_x=values[1]-values[0];raw.move_y=values[3]-values[2];
    for (std::uint32_t a=9;a<=11;++a) if(values[a]!=0) raw.held_flags|=1u<<(a-8);
    for (std::uint32_t a=8;a<=11;++a) if(edges.rising&(1u<<a)) raw.edge_flags|=1u<<(a-8);
    auto& shooting=g_shoot_directions[0];
    const auto previous_held=shooting.HeldMask(),previous_selected=shooting.SelectedMask();
    const std::array<float,4> directions{values[4],values[5],values[6],values[7]};
    if(!CanonicalizeShooting(raw,directions,shooting,canonical)) return false;
    PublishShootingSource(0,directions,previous_held,previous_selected,canonical,keyboard?1u:2u,
        loop::CurrentObservationContext().loop_iteration,g_physical_shoot_reset);
    g_physical_shoot_reset=false;
    return true;
}
void PublishPhysical() noexcept
{
    if(!g_authority_session.AcceptsInput()) return;
    auto& participant=g_participants[0];
    const auto context=loop::CurrentObservationContext();
    if(g_authority_clock.Current() || g_pause.BlocksInput() || g_loading.BlocksInput() || g_lifecycle.BlocksInput() ||
       !participant.BoundValid() || !g_observed_valid || context.loop_iteration!=g_observed_loop ||
       !(participant.Bound()==g_observed_binding)) return;
    auto canonical=g_observed_physical;canonical.edge_flags=g_observed_edges&~g_published_edges;
    if(!canonical.edge_flags && g_last_physical_tick==g_authority_clock.Completed() &&
       g_last_physical_generation==participant.Generation() && g_last_physical_device==g_observed_binding.device &&
       std::memcmp(&g_last_physical,&canonical,sizeof(canonical))==0) return;
    std::uint64_t target=0;
    if(!NextInputTarget(g_authority_clock.Completed(),g_input_config,target)) {
        Fail(Error::Protocol);return;
    }
    if(!SubmitFresh(0,canonical,target)) {
        if(g_authority_session.AcceptsInput()) Fail(Error::Protocol);return;
    }
    g_published_edges|=canonical.edge_flags;
    g_last_physical=canonical;g_last_physical.edge_flags=0;g_last_physical_tick=g_authority_clock.Completed();
    g_last_physical_generation=participant.Generation();g_last_physical_device=g_observed_binding.device;
}
void PhysicalExit(void* device,bool result,void* return_address,void*) noexcept
{
    Callback callback;
    if (!Enabled() || !result || return_address!=g_image+0x61F30C) return;
    if (!MainThread()) {Fail(Error::Thread);return;}
    if(g_session_retired) return;
    auto& participant=g_participants[0];
    Topology topology{};Binding exact{};
    if(!ReadTopology(topology) || !FindBinding(topology.players[0],exact)) {
        g_observed_valid=false;if(participant.BoundValid()) Invalidate(0,Lifecycle::Rebind);return;
    }
    if(exact.device!=device) return;
    const auto vptr=*static_cast<void**>(device);
    if(vptr!=g_image+0x7A1678 && vptr!=g_image+0x782C80) {Fail(Error::Topology);return;}
    const bool changed=g_physical_identity.device!=exact.device || g_physical_identity.control!=exact.control ||
        g_physical_identity.key!=exact.key;
    if(changed) {
        g_physical_history={};g_shoot_directions[0]={};g_physical_shoot_reset=true;
        g_observed_valid=false;g_physical_identity=exact;
    }
    input::RawInputSample raw{};InputStateDraftV2 canonical{};
    std::uint32_t held_mask=0;
    if(!PhysicalSample(device,exact,raw,held_mask,canonical)) {Fail(Error::ReadFault);return;}
    const auto context=loop::CurrentObservationContext();
    const auto blocks=(g_pause.BlocksInput()?4u:0u)|(g_lifecycle.BlocksInput()?8u:0u)|
        (g_loading.BlocksInput()?16u:0u)|(Dead(topology.players[0])?32u:0u);
    DWORD foreground_pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground_pid);
    if(g_observed_loop!=context.loop_iteration || changed) {g_observed_edges=0;g_published_edges=0;}
    if(!g_observed_valid || !participant.BoundValid() || participant.StartupPending() || blocks ||
       g_observed_mask!=held_mask || g_observed_blocks!=blocks ||
       g_observed_foreground_pid!=foreground_pid ||
       !(g_observed_binding==exact) || std::memcmp(&g_observed_physical,&canonical,sizeof(canonical))) {
        auto event=MakeEvent(EventKind::PhysicalObserved,0);event.action=held_mask;
        event.query=2;event.boolean=foreground_pid==GetCurrentProcessId()?1u:0u;
        event.target=context.loop_iteration;event.state=canonical;event.detail=blocks;
        event.before={Address(exact.player),Address(device),Address(exact.control),static_cast<std::uint32_t>(exact.key)};
        event.after={g_published_edges,g_observed_edges|canonical.edge_flags,foreground_pid,0};
        Publish(event);
    }
    g_observed_edges|=canonical.edge_flags;g_observed_physical=canonical;g_observed_binding=exact;
    g_observed_loop=context.loop_iteration;g_observed_mask=held_mask;g_observed_blocks=blocks;
    g_observed_foreground_pid=foreground_pid;g_observed_valid=true;
    if(participant.BoundValid() && !(participant.Bound()==exact)) Invalidate(0,Lifecycle::Rebind);
    // Source observation only. Both A and B enter admission in Authority's drain.
}
bool Witness(void* player,Consumer consumer,std::array<std::uint32_t,4>& data) noexcept
{
    __try {
        const auto* p=static_cast<const std::uint8_t*>(player);data={};
        switch(consumer) {
        case Consumer::Movement: std::memcpy(data.data(),p+0x360,8);std::memcpy(&data[2],p+0x1624,4);return true;
        case Consumer::Shooting: std::memcpy(data.data(),p+0x162C,12);data[3]=p[0x1638];return true;
        case Consumer::Bomb: data[0]=*reinterpret_cast<const std::uint32_t*>(p+0x1378)&1;return true;
        case Consumer::Item:
            // Primary active slot (the frozen caller pushes slot=0). These are
            // native consumer fields, not a universal item-effect/spawn digest.
            data[0]=*reinterpret_cast<const std::uint32_t*>(p+0x1378)&0x10;
            std::memcpy(&data[1],p+0x1580,4);std::memcpy(&data[2],p+0x1588,8);return true;
        case Consumer::Pill:
            data[0]=*reinterpret_cast<const std::uint32_t*>(p+0x1378)&8;
            std::memcpy(&data[1],p+0x17A0,12);return true;
        case Consumer::Drop:
            std::memcpy(data.data(),p+0x17C0,4);
            data[1]=*reinterpret_cast<const std::uint32_t*>(p+0x1378)&0x20;return true;
        default: return false;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
template<class Function>
void Consume(void* player,Consumer consumer,Function original) noexcept
{
    Callback callback;const auto previous=g_tls;
    const bool controlled=g_hook_owner && g_tls.player==player;
    if (controlled) {
        if (g_tls.consumer!=Consumer::None) {Fail(Error::Reentry);original();return;}
        g_tls.consumer=consumer;
    }
    auto event=MakeEvent(EventKind::WorldWitness,g_tls.ordinal);event.detail=static_cast<std::uint32_t>(consumer);
    event.action=g_tick_pipeline.SimulationTick()?(g_player_only_scope?2u:1u):0u;
    event.query=g_tls.update_caller;
    if(g_tls.ordinal<2 && g_participants[g_tls.ordinal].CurrentEligible(g_participants[g_tls.ordinal].Bound(),g_tick_pipeline.SimulationTick())) event.decision=1;
    const bool before=controlled && Witness(player,consumer,event.before);
    original();
    if (controlled) {event.boolean=before && Witness(player,consumer,event.after)?1u:0u;Publish(event);g_tls=previous;}
}
float RepeatQueryReads(const Participant& participant,const Binding& exact,
    std::uint32_t action,Query query,float original,std::uint32_t eligibility,std::uint32_t source=1) noexcept
{
    std::memcpy(g_repeat_snapshot.data(),&participant,sizeof(participant));
    const auto group=++g_repeat_group;
    const auto depth=static_cast<std::uint32_t>(participant.Depth());
    std::array<float,3> values{};bool unchanged=true;
    for(std::uint32_t i=0;i<values.size();++i) {
        values[i]=participant.Read(action,query,g_tls.consumer,exact,g_tick_pipeline.SimulationTick());
        unchanged=unchanged && std::memcmp(g_repeat_snapshot.data(),&participant,sizeof(participant))==0;
        auto read=MakeEvent(EventKind::QueryResult,g_tls.ordinal);
        read.action=action;read.query=static_cast<std::uint32_t>(query);
        read.detail=static_cast<std::uint32_t>(g_tls.consumer);read.decision=eligibility;
        read.scalar=values[i];read.boolean=values[i]!=0;
        std::memcpy(&read.before[0],&original,4);
        read.before[1]=i;read.before[2]=source;read.before[3]=group;
        Publish(read);
    }
    const bool same=values[0]==values[1] && values[0]==values[2];
    auto event=MakeEvent(EventKind::QueryRepeat,g_tls.ordinal);
    event.action=action;event.query=static_cast<std::uint32_t>(query);
    event.detail=static_cast<std::uint32_t>(g_tls.consumer);event.command=group;
    event.target=loop::CurrentObservationContext().loop_iteration;
    event.decision=(same?1u:0u)|(unchanged?2u:0u);event.boolean=same && unchanged;
    event.scalar=values[0];event.before[2]=depth;
    std::memcpy(&event.before[0],&values[0],4);std::memcpy(&event.before[1],&values[1],4);
    std::memcpy(&event.after[0],&values[2],4);
    event.after[1]=static_cast<std::uint32_t>(participant.Depth());event.after[2]=source;event.after[3]=3;
    Publish(event);
    if(!same || !unchanged) {Fail(Error::Protocol);return 0;}
    return values[0];
}
float Evaluate(void* device,std::uint32_t action,Query query,float original,bool owned) noexcept
{
    if (!g_hook_owner || action>=12 || g_tls.consumer==Consumer::None) return owned?0:original;
    if (!Enabled() || !g_tick_pipeline.SimulationTick()) return 0;
    float result=0;std::uint32_t eligibility=0;
    const auto ordinal=g_tls.ordinal;
    if (MainThread() && ordinal<2) {
        const auto& participant=g_participants[ordinal];Binding exact{};Topology topology{};
        const bool exact_member=ReadTopology(topology) && ordinal<topology.count &&
            topology.players[ordinal]==g_tls.player && FindBinding(g_tls.player,exact) && exact.device==device;
        if (exact_member && ((owned && ordinal==1) || (!owned && ordinal==0))) {
            if(participant.CurrentEligible(exact,g_tick_pipeline.SimulationTick()) && ConsumerAllows(g_tls.consumer,action)) eligibility|=1;
            if(participant.PreviousComparable()) eligibility|=2;
            const auto query_index=static_cast<std::uint32_t>(query)-1;
            if(g_repeat_queries && g_repeat_budget && (eligibility&1) && query_index<3) {
                if(g_repeat_generation[ordinal]!=participant.Generation()) {
                    g_repeat_generation[ordinal]=participant.Generation();g_repeat_used[ordinal]={};
                }
                const bool level=query==Query::Triggered && action>=8?
                    (participant.Current().edge_flags&(1u<<(action-8)))!=0:
                    Magnitude(participant.Current(),action)!=0;
                const auto index=query_index*24+action*2+(level?1u:0u);
                if(!g_repeat_used[ordinal][index]) {
                    g_repeat_used[ordinal][index]=true;--g_repeat_budget;
                    result=RepeatQueryReads(participant,exact,action,query,original,eligibility);
                    // Exercise the other pure reader interfaces in this same
                    // real consumer/action/binding context, even when the engine
                    // only asks one query type. Source=2 explicitly identifies
                    // these controlled companion reads; no extra native calls.
                    for(std::uint32_t q=1;q<=3 && Enabled() && g_repeat_budget;++q) {
                        if(q==static_cast<std::uint32_t>(query)) continue;
                        const auto companion=static_cast<Query>(q);
                        const bool companion_level=companion==Query::Triggered && action>=8?
                            (participant.Current().edge_flags&(1u<<(action-8)))!=0:
                            Magnitude(participant.Current(),action)!=0;
                        const auto companion_index=(q-1)*24+action*2+(companion_level?1u:0u);
                        if(g_repeat_used[ordinal][companion_index]) continue;
                        g_repeat_used[ordinal][companion_index]=true;--g_repeat_budget;
                        RepeatQueryReads(participant,exact,action,companion,0,eligibility,2);
                    }
                    return result;
                }
            }
            result=participant.Read(action,query,g_tls.consumer,exact,g_tick_pipeline.SimulationTick());
        }
    }
    auto event=MakeEvent(EventKind::QueryResult,ordinal);event.action=action;
    event.query=static_cast<std::uint32_t>(query);event.detail=static_cast<std::uint32_t>(g_tls.consumer);
    event.scalar=result;event.boolean=result!=0;event.decision=eligibility;
    std::memcpy(&event.before[0],&original,4);Publish(event);
    return result;
}
float OwnedReader(void*,std::uint32_t action,Query query) noexcept
{
    Callback callback;
    if (Enabled() && g_authority_session.AllowsBinding() && MainThread() && g_join_scope) {
        if(g_attempt) return 0;
        if(!JoinContextValid()) {Fail(Error::Topology);return 0;}
        const bool join_action=query==Query::Triggered &&
            (action==19 || action==14 || action==15 || action==20 || action==21);
        const bool value=(g_join_phase==1 && action==19) || (g_join_phase==2 && action==14);
        if (join_action) {const std::uint32_t actions[5]{19,14,15,20,21};
            for(std::size_t i=0;i<5;++i) if(actions[i]==action) ++g_join_queries[i];
            auto event=MakeEvent(EventKind::JoinQuery,1);event.action=action;
            event.query=static_cast<std::uint32_t>(query);event.decision=value?2u:1u;
            event.command=g_pass;event.boolean=value;Publish(event);return value?1.0F:0.0F;}
        if(query==Query::Pressed && (action==8 || action==10)) {
            auto event=MakeEvent(EventKind::JoinQuery,1);event.action=action;
            event.query=static_cast<std::uint32_t>(query);event.decision=1;event.command=g_pass;Publish(event);
        }
        return 0; // Native possessor uses Pressed, not Triggered. All gameplay input is neutral here.
    }
    return Evaluate(g_device.Pair().object,action,query,0,true);
}
extern "C" bool __fastcall KeyboardPressed(void* self,void*,std::uint32_t action)
{Callback callback;const bool value=g_bool_queries[0](self,action);return Evaluate(self,action,Query::Pressed,value?1.0F:0.0F,false)!=0;}
extern "C" bool __fastcall KeyboardTriggered(void* self,void*,std::uint32_t action)
{Callback callback;const bool value=g_bool_queries[1](self,action);return Evaluate(self,action,Query::Triggered,value?1.0F:0.0F,false)!=0;}
extern "C" float __fastcall KeyboardScalar(void* self,void*,std::uint32_t action)
{Callback callback;const float value=g_scalar_queries[0](self,action);return Evaluate(self,action,Query::Scalar,value,false);}
extern "C" bool __fastcall GamepadPressed(void* self,void*,std::uint32_t action)
{Callback callback;const bool value=g_bool_queries[2](self,action);return Evaluate(self,action,Query::Pressed,value?1.0F:0.0F,false)!=0;}
extern "C" bool __fastcall GamepadTriggered(void* self,void*,std::uint32_t action)
{Callback callback;const bool value=g_bool_queries[3](self,action);return Evaluate(self,action,Query::Triggered,value?1.0F:0.0F,false)!=0;}
extern "C" float __fastcall GamepadScalar(void* self,void*,std::uint32_t action)
{Callback callback;const float value=g_scalar_queries[1](self,action);return Evaluate(self,action,Query::Scalar,value,false);}
__declspec(noinline) void ObservePlayerCallSource(void* player,void* caller,std::uint32_t ordinal) noexcept
{
    if(!Enabled() || !MainThread() || ordinal>=2 || g_player_call_source_count==g_player_call_sources.size()) return;
    const auto context=loop::CurrentObservationContext();
    const auto phase=static_cast<std::uint32_t>(context.phase);
    const auto& participant=g_participants[ordinal];
    const std::uint32_t flags=(g_tick_pipeline.SimulationTick()?1u:0u) |
        (participant.CurrentEligible(participant.Bound(),g_tick_pipeline.SimulationTick())?2u:0u) |
        (g_pause.BlocksInput()?4u:0u) | (g_lifecycle.BlocksInput()?8u:0u) |
        (g_loading.BlocksInput()?16u:0u);
    for(std::size_t i=0;i<g_player_call_source_count;++i) {
        const auto& previous=g_player_call_sources[i];
        if(previous.caller==caller && previous.ordinal==ordinal && previous.phase==phase && previous.flags==flags) return;
    }
    const auto index=static_cast<std::uint32_t>(g_player_call_source_count++);
    g_player_call_sources[index]={caller,ordinal,phase,flags};
    auto event=MakeEvent(EventKind::NativePlayerCall,ordinal);
    event.action=Address(caller)>=Address(g_image) && Address(caller)-Address(g_image)<0x8FE000u?
        Address(caller)-Address(g_image):UINT32_MAX;
    event.query=phase;event.detail=flags;event.target=context.loop_iteration;
    event.before={Address(player),Address(caller),Address(g_image),context.thread_id};
    event.after={context.phase_depth,index,static_cast<std::uint32_t>(g_tick_pipeline.SimulationTick()),static_cast<std::uint32_t>(g_tick_pipeline.SimulationTick()>>32)};
    Publish(event);
    void* frames[16]{};
    // No backtrace hash requested. Capture only bounded addresses for source attribution.
    const auto count=CaptureStackBackTrace(0,16,frames,nullptr);
    for(std::uint32_t offset=0;offset<count;offset+=4) {
        auto stack=MakeEvent(EventKind::NativePlayerStack,ordinal);
        stack.action=index;stack.query=offset;stack.detail=count;stack.target=context.loop_iteration;
        for(std::uint32_t lane=0;lane<4 && offset+lane<count;++lane) stack.before[lane]=Address(frames[offset+lane]);
        Publish(stack);
    }
}
void Authority(bool entering,void*,void*,void*) noexcept;
extern "C" void __fastcall PlayerUpdate(void* self,void*)
{
    Callback callback;const auto previous=g_tls;
    if (g_hook_owner) {
        // 6FD609 is the ordinary odd-frame simulation call, after native pause
        // and gameplay guards. Special FSM callers 6FD4A9/6FD54A stay neutral.
        if(g_player_only_scope && !g_player_only_started && Enabled() && MainThread() &&
           _ReturnAddress()==g_image+0x2FD60C) {
            Topology topology{};
            if(ReadTopology(topology) &&
               (topology.players[0]==self || (topology.count==2 && topology.players[1]==self))) {
                g_player_only_started=true;
                g_player_only_owner=self;
                Authority(true,self,g_player_only_return_address,nullptr);
            }
        }
        g_tls={self,UINT32_MAX,Consumer::None,Address(_ReturnAddress())-Address(g_image)};
        for(std::uint32_t i=0;i<2;++i) if(g_participants[i].BoundValid() && g_participants[i].Bound().player==self) g_tls.ordinal=i;
        if(g_tls.ordinal>=2) {
            // Event attribution during invalidation uses the live Main list,
            // never a retired binding. Input eligibility still needs exact Bind.
            Topology topology{};
            if(ReadTopology(topology)) for(std::uint32_t i=0;i<topology.count;++i)
                if(topology.players[i]==self) g_tls.ordinal=i;
        }
        ObservePlayerCallSource(self,_ReturnAddress(),g_tls.ordinal);
    }
    Binding world_binding{};std::uint64_t world_generation=0;
    const auto world_ordinal=g_tls.ordinal;
    bool world_witness=false;
    if(world::Observing() && Enabled() && MainThread() && world_ordinal<2 && g_tick_pipeline.SimulationTick() &&
       !g_pause.BlocksInput() && !g_loading.BlocksInput() && !g_lifecycle.BlocksInput() && FullPlayer(self)) {
        const auto& participant=g_participants[world_ordinal];
        world_witness=participant.BoundValid() && participant.Bound().player==self;
        if(world_witness){world_binding=participant.Bound();world_generation=participant.Generation();
            world_witness=world::PlayerPairMatches(self,world_binding.device,world_binding.control,world_binding.key);}
        if(!world_witness)world::PlayerAnchor(world_ordinal,nullptr,false);
    }
    g_player_update(self);
    if(world_witness && Enabled() && MainThread() && g_tick_pipeline.SimulationTick() &&
       !g_pause.BlocksInput() && !g_loading.BlocksInput() && !g_lifecycle.BlocksInput()) {
        const auto& participant=g_participants[world_ordinal];
        if(participant.BoundValid() && participant.Generation()==world_generation && participant.Bound()==world_binding &&
           world::PlayerPairMatches(self,world_binding.device,world_binding.control,world_binding.key))
            world::PlayerReadReady(world_ordinal,self);
        else world::PlayerAnchor(world_ordinal,nullptr,false);
    }
    g_tls=previous;
}
extern "C" void __cdecl PlayerOnlyUpdate()
{
    Callback callback;bool scoped=false;
    void* const native_return=_ReturnAddress();
    if(Enabled()) {
        if(!MainThread()) Fail(Error::Thread);
        else if(g_player_only_scope || g_authority_clock.Current()) Fail(Error::Reentry);
        else if(native_return!=g_image+0x555246) Fail(Error::Protocol);
        else scoped=true;
    }
    if(scoped) {g_player_only_scope=true;g_player_only_started=false;g_player_only_return_address=native_return;g_player_only_owner=nullptr;}
    g_player_only_original(); // Native pause state machine and update exactly once.
    if(scoped) {
        if(g_player_only_started)
            Authority(false,g_player_only_owner,native_return,nullptr);
        g_player_only_started=false;g_player_only_scope=false;g_player_only_return_address=nullptr;g_player_only_owner=nullptr;
    }
}
extern "C" void __fastcall Movement(void* self,void*) {Consume(self,Consumer::Movement,[=](){g_consumers[0](self);});}
extern "C" void __fastcall Bomb(void* self,void*) {Consume(self,Consumer::Bomb,[=](){g_consumers[1](self);});}
extern "C" void __fastcall Item(void* self,void*,std::int32_t slot)
{Consume(self,Consumer::Item,[=](){reinterpret_cast<ItemFn>(g_consumers[2])(self,slot);});}
extern "C" void __fastcall Pill(void* self,void*) {Consume(self,Consumer::Pill,[=](){g_consumers[3](self);});}
extern "C" void __fastcall Shooting(void* self,void*)
{
    // Active C/S dispatch, not an observation wrapper: a neutral query still
    // advances the native weapon state machine and can release a charged shot.
    // Transition/FSM Player updates outside Authority must not run that step.
    if(!g_hook_owner || g_tls.player!=self) {
        Consume(self,Consumer::Shooting,[=](){g_consumers[4](self);});return;
    }
    Callback callback;
    if(!MainThread()) {Fail(Error::Thread);return;}
    const std::uint32_t blocks=(!g_tick_pipeline.SimulationTick()?1u:0u)|(g_pause.BlocksInput()?2u:0u)|
        (g_lifecycle.BlocksInput()?4u:0u)|(g_loading.BlocksInput()?8u:0u)|(!Enabled()?16u:0u);
    const auto ordinal=g_tls.ordinal;
    auto event=MakeEvent(EventKind::ShootingDispatch,ordinal);
    event.action=blocks?0u:1u;event.query=g_tls.update_caller;event.detail=blocks;
    event.target=loop::CurrentObservationContext().loop_iteration;
    if(ordinal<2 && g_participants[ordinal].CurrentEligible(g_participants[ordinal].Bound(),g_tick_pipeline.SimulationTick())) event.decision=1;
    const bool before=Witness(self,Consumer::Shooting,event.before);
    // Do not predicate execution on CurrentEligible, command or held: an actual
    // Authority tick with missing input must consume neutral, not retain charge.
    if(!blocks) Consume(self,Consumer::Shooting,[=](){g_consumers[4](self);});
    event.boolean=before && Witness(self,Consumer::Shooting,event.after)?1u:0u;
    Publish(event);
}
extern "C" void __fastcall Drop(void* self,void*) {Consume(self,Consumer::Drop,[=](){g_consumers[5](self);});}
extern "C" std::uint32_t __cdecl SelectJoinMode(void* manager,std::int32_t key,
    void* selector,std::uint32_t callsite) noexcept
{
    Callback callback;
    const bool observed=g_join_scope && MainThread();
    std::uint32_t before_flags=0,after_flags=0;
    const bool before_valid=!observed || ReadJoinFlags(g_join_root,before_flags);
    const bool native=g_join_mode_original(); // Exactly once, including passthrough devices.
    bool selected=native;
    if(observed) {
        const bool owned=key==g_device.Key();
        bool valid=before_valid && ReadJoinFlags(g_join_root,after_flags) && before_flags==after_flags;
        if(owned && !g_attempt) {
            std::int32_t row_key=-1;
            valid=valid && manager==g_join_manager && JoinContextValid() &&
                (!selector || (ReadJoinRowKey(manager,selector,row_key) && row_key==key));
            // The C/S participant is a full Player. Never enter the Baby route,
            // including after a recognized identity fault; subsequent input is neutral.
            selected=false;
            if(!valid) Fail(Error::Topology);
        }
        auto event=MakeEvent(EventKind::JoinRoute,owned?1u:UINT32_MAX);
        event.command=g_pass;event.target=GetCurrentThreadId();event.action=callsite;
        event.query=native?1u:0u;event.decision=selected?1u:0u;
        event.detail=(owned?1u:0u)|((owned && !g_attempt)?2u:0u);event.boolean=valid?1u:0u;
        event.before={static_cast<std::uint32_t>(key),Address(manager),before_flags,Address(g_join_host)};
        event.after={after_flags,Address(g_device.Pair().object),Address(g_device.Pair().control),1};
        Publish(event);
    }
    return selected?1u:0u;
}
// The original no-argument predicate only clobbers EAX/flags. Preserve all other
// registers: EDX is live across the first call; ESI/ECX carry K at these sites.
extern "C" __declspec(naked) void JoinHealthMode()
{
    __asm {
        pushfd
        pushad
        sub esp,128
        movdqu xmmword ptr [esp],xmm0
        movdqu xmmword ptr [esp+16],xmm1
        movdqu xmmword ptr [esp+32],xmm2
        movdqu xmmword ptr [esp+48],xmm3
        movdqu xmmword ptr [esp+64],xmm4
        movdqu xmmword ptr [esp+80],xmm5
        movdqu xmmword ptr [esp+96],xmm6
        movdqu xmmword ptr [esp+112],xmm7
        push 05BAE7Eh
        push 0
        push esi
        push edi
        call SelectJoinMode
        add esp,16
        movdqu xmm0,xmmword ptr [esp]
        movdqu xmm1,xmmword ptr [esp+16]
        movdqu xmm2,xmmword ptr [esp+32]
        movdqu xmm3,xmmword ptr [esp+48]
        movdqu xmm4,xmmword ptr [esp+64]
        movdqu xmm5,xmmword ptr [esp+80]
        movdqu xmm6,xmmword ptr [esp+96]
        movdqu xmm7,xmmword ptr [esp+112]
        add esp,128
        mov dword ptr [esp+28],eax
        popad
        popfd
        ret
    }
}
extern "C" __declspec(naked) void JoinDeviceMode()
{
    __asm {
        pushfd
        pushad
        sub esp,128
        movdqu xmmword ptr [esp],xmm0
        movdqu xmmword ptr [esp+16],xmm1
        movdqu xmmword ptr [esp+32],xmm2
        movdqu xmmword ptr [esp+48],xmm3
        movdqu xmmword ptr [esp+64],xmm4
        movdqu xmmword ptr [esp+80],xmm5
        movdqu xmmword ptr [esp+96],xmm6
        movdqu xmmword ptr [esp+112],xmm7
        push 05BB081h
        push 0
        push esi
        push edi
        call SelectJoinMode
        add esp,16
        movdqu xmm0,xmmword ptr [esp]
        movdqu xmm1,xmmword ptr [esp+16]
        movdqu xmm2,xmmword ptr [esp+32]
        movdqu xmm3,xmmword ptr [esp+48]
        movdqu xmm4,xmmword ptr [esp+64]
        movdqu xmm5,xmmword ptr [esp+80]
        movdqu xmm6,xmmword ptr [esp+96]
        movdqu xmm7,xmmword ptr [esp+112]
        add esp,128
        mov dword ptr [esp+28],eax
        popad
        popfd
        ret
    }
}
extern "C" __declspec(naked) void JoinSelectorMode()
{
    __asm {
        pushfd
        pushad
        sub esp,128
        movdqu xmmword ptr [esp],xmm0
        movdqu xmmword ptr [esp+16],xmm1
        movdqu xmmword ptr [esp+32],xmm2
        movdqu xmmword ptr [esp+48],xmm3
        movdqu xmmword ptr [esp+64],xmm4
        movdqu xmmword ptr [esp+80],xmm5
        movdqu xmmword ptr [esp+96],xmm6
        movdqu xmmword ptr [esp+112],xmm7
        push 05BB288h
        push ebx
        push ecx
        push edi
        call SelectJoinMode
        add esp,16
        movdqu xmm0,xmmword ptr [esp]
        movdqu xmm1,xmmword ptr [esp+16]
        movdqu xmm2,xmmword ptr [esp+32]
        movdqu xmm3,xmmword ptr [esp+48]
        movdqu xmm4,xmmword ptr [esp+64]
        movdqu xmm5,xmmword ptr [esp+80]
        movdqu xmm6,xmmword ptr [esp+96]
        movdqu xmm7,xmmword ptr [esp+112]
        add esp,128
        mov dword ptr [esp+28],eax
        popad
        popfd
        ret
    }
}
extern "C" bool __cdecl AllowCreate(void* manager,void* selector,std::int32_t* selected_type) noexcept
{
    Callback callback;
    std::int32_t key=-1;
    if(!ReadJoinRowKey(manager,selector,key)) {Fail(Error::Topology);return false;}
    if (key!=g_device.Key()) return true;
    if (!JoinContextValid()) return false;
    if(manager!=g_join_manager) {Fail(Error::Topology);return false;}
    const auto original_type=*selected_type;
    const auto chosen=g_b_character<0?original_type:g_b_character;
    std::uint32_t available=0;
    const bool valid=chosen>=0 && (g_b_character<0 || SelectableCharacter(manager,chosen,available));
    auto selection=MakeEvent(EventKind::CharacterSelection,1);selection.command=g_pass;
    selection.action=2;selection.query=static_cast<std::uint32_t>(original_type);
    selection.decision=static_cast<std::uint32_t>(chosen);selection.detail=available;selection.boolean=valid;
    selection.before={static_cast<std::uint32_t>(key),Address(manager),Address(selector),g_attempt};
    if(!Publish(selection)) return false;
    if(!valid) {Fail(Error::Character);return false;}
    // Only our validated stack argument changes. No selector/save/unlock writes,
    // additional factory calls, or changes to a physical device's native choice.
    *selected_type=chosen;g_b_selected_character=chosen;
    g_attempt=1;g_b_join_attempted=true;
    auto event=MakeEvent(EventKind::CreateAttempt,1);event.command=g_pass;event.detail=0x5BB415;
    event.before={static_cast<std::uint32_t>(key),Address(manager),Address(selector),g_attempt};
    event.after={Address(g_device.Pair().object),Address(g_device.Pair().control),
        *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(g_join_root)+0x26548),g_join_phase};
    return Publish(event);
}
extern "C" __declspec(naked) void CreateHook()
{
    __asm {
        pushfd
        pushad
        // Entry [esp+4] is selectedType. pushfd+pushad add 36 bytes.
        lea eax,[esp+40]
        push eax
        push ebx
        push ecx
        call AllowCreate
        add esp,12
        test al,al
        jz denied
        popad
        popfd
        jmp dword ptr [g_coop_create_original]
    denied:
        popad
        popfd
        add esp,8
        jmp dword ptr [g_coop_no_create]
    }
}
extern "C" std::uint32_t __fastcall JoinPass(void* self,void*)
{
    Callback callback;bool scoped=false;std::array<std::uint32_t,4> host_health{};
    if (Enabled() && g_authority_session.AllowsBinding() && !g_session_retired && g_device.Registered() && !g_attempt &&
        !g_pause.BlocksInput() && !g_lifecycle.BlocksInput() && MainThread()) {
        if (g_join_scope) Fail(Error::Reentry);
        else if (g_join_phase<2) {
            Topology topology{};Binding host{};
            if(ReadTopology(topology) && topology.count==1 && FullPlayer(topology.players[0]) &&
               self==static_cast<std::uint8_t*>(topology.root)+0x1BAA8 && g_participants[0].BoundValid() &&
               FindBinding(topology.players[0],host) && host==g_participants[0].Bound() &&
               g_device.Healthy() && g_device.ExactMember() && ReadJoinHealth(topology.players[0],host_health)) {
            g_join_root=topology.root;g_join_manager=self;g_join_host=topology.players[0];
            ++g_join_phase;++g_pass;g_join_scope=true;scoped=true;g_join_queries.fill(0);
            Diagnose(DiagnosticStage::Join,1,DiagnosticReason::Ready,
                {g_join_phase,static_cast<std::uint32_t>(g_device.Key()),Address(self)});
            auto event=MakeEvent(EventKind::JoinPass,1);event.command=g_pass;event.detail=g_join_phase;Publish(event);
            } else Diagnose(DiagnosticStage::Join,1,DiagnosticReason::JoinNotEligible);
        }
        else Diagnose(DiagnosticStage::Join,1,DiagnosticReason::JoinScopeExhausted,{g_join_phase,g_attempt});
    } else if(Enabled() && !g_session_retired) {
        Diagnose(DiagnosticStage::Join,1,DiagnosticReason::JoinNotEligible,
            {static_cast<std::uint32_t>(g_device.State()),g_attempt,g_pause.BlocksInput()?1u:0u,g_lifecycle.BlocksInput()?1u:0u});
    }
    const auto result=g_join_original(self);
    if (scoped) {
        // Emit NOT_CALLED for each command action without inventing a native result.
        const std::uint32_t actions[5]{19,14,15,20,21};
        for(std::size_t i=0;i<5;++i) if(!g_join_queries[i]) {
            auto missing=MakeEvent(EventKind::JoinQuery,1);missing.action=actions[i];
            missing.command=g_pass;missing.query=static_cast<std::uint32_t>(Query::Triggered);missing.decision=0;Publish(missing);
        }
        auto close=MakeEvent(EventKind::JoinPass,1);close.command=g_pass;close.detail=0;Publish(close);
        g_join_scope=false;
        Topology topology{};
        if (ReadTopology(topology)) {
            auto outcome=MakeEvent(EventKind::JoinOutcome,1);outcome.command=g_pass;outcome.action=g_join_phase;
            outcome.query=1;outcome.decision=topology.count;outcome.before=host_health;
            outcome.detail=topology.count==2?*reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(topology.players[1])+0x2C):UINT32_MAX;
            outcome.boolean=topology.root==g_join_root && topology.players[0]==g_join_host &&
                ReadJoinHealth(topology.players[0],outcome.after) && outcome.before==outcome.after;
            Publish(outcome);
            if(!outcome.boolean || (!g_attempt && topology.count!=1)) {Fail(Error::JoinUnconfirmed);return result;}
            if(g_attempt==1 && topology.count==2) {
                const auto* player=static_cast<const std::uint8_t*>(topology.players[1]);
                const auto actual=*reinterpret_cast<const std::int32_t*>(player+0x30);
                auto character=MakeEvent(EventKind::CharacterSelection,1);character.command=g_pass;
                character.action=3;character.query=static_cast<std::uint32_t>(actual);
                character.decision=static_cast<std::uint32_t>(g_b_selected_character);
                character.before={Address(g_device.Pair().object),Address(g_device.Pair().control),
                    static_cast<std::uint32_t>(g_device.Key()),static_cast<std::uint32_t>(g_b_character)};
                character.after={Address(topology.players[0]),Address(topology.players[1]),
                    *reinterpret_cast<const std::uint32_t*>(player+0x28),*reinterpret_cast<const std::uint32_t*>(player+0x2C)};
                character.boolean=actual==g_b_selected_character && FullPlayer(topology.players[1]);
                if(!Publish(character)) return result;
                if(!character.boolean) {Fail(Error::Character);return result;}
            }
            RefreshBindings(topology);
            if(g_attempt==1 && topology.count==2 && g_participants[1].BoundValid()) {
                g_attempt=2;auto joined=MakeEvent(EventKind::Joined,1);joined.command=g_pass;
                joined.action=*reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(topology.players[1])+0x30);
                joined.detail=0; // FullPlayer requires native Type=1, Variant=0 before binding.
                joined.before={Address(topology.players[0]),Address(topology.players[1]),topology.count,static_cast<std::uint32_t>(g_device.Key())};
                joined.after={Address(g_device.Pair().object),Address(g_device.Pair().control),
                    *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(topology.players[1])+0x28),
                    *reinterpret_cast<const std::uint32_t*>(static_cast<const std::uint8_t*>(topology.players[1])+0x2C)};
                Publish(joined);
            } else if(g_attempt==1) Fail(Error::Publication);
        } else if(g_attempt) Fail(Error::Publication);
    }
    return result;
}
bool RetireB() noexcept
{
    if (g_attempt!=3) {
        g_attempt=3;g_participants[1].Retire();world::PlayerAnchor(1,nullptr,false);g_join_scope=false;g_device.Neutralize();
        PublishClientProgress(1);
    }
    if (g_device.Pair().control && !g_device.Remove()) return false;
    Topology topology{};
    if (ReadTopology(topology) && topology.count==2) {
        if(!g_b_join_attempted) return false;
        Binding binding{};
        if (!FindBinding(topology.players[1],binding)) {
            // The device is now absent: compare the still-live Player's key, not its old pointer binding.
            binding.key=*reinterpret_cast<std::int32_t*>(static_cast<std::uint8_t*>(topology.players[1])+0x1618);
        }
        if(binding.key!=g_device.Key()) return false;
        reinterpret_cast<ExitFn>(g_image+0x5BA000)(static_cast<std::uint8_t*>(topology.root)+0x1BAA8,
            topology.players[1],0);
        if(!ReadTopology(topology) || topology.count!=1) return false;
    } else if (!ReadTopology(topology) && !g_session_retired) return false;
    g_b_retired_done=true;
    auto event=MakeEvent(EventKind::Retired,1);event.detail=g_device.Strong();
    event.boolean=g_device.Detached() || !g_device.Pair().control;Publish(event);return true;
}
void Authority(bool entering,void* owner,void* return_address,void*) noexcept
{
    Callback callback;
    if(!MainThread()) {if(Enabled()) Fail(Error::Thread);return;}
    const auto boundary=CurrentTickBoundary(owner,return_address);
    if(entering) {
        if(g_tick_call.observed) {RejectTick(TickDecision::Reentry,boundary);return;}
        g_tick_call={boundary,true};
    } else {
        if(!g_tick_call.observed) {
            if(Enabled() && g_authority_session.AcceptsInput()) RejectTick(TickDecision::WrongReturn,boundary);
            return;
        }
        const auto expected=g_tick_call.boundary;g_tick_call.observed=false;
        if(!(expected==boundary)) {RejectTick(TickDecision::WrongReturn,boundary);return;}
    }
    if(!Enabled()) {
        if(!entering && g_authority_clock.Current())
            RejectTick(g_authority_clock.Complete(boundary,true,false),boundary);
        return;
    }
    // A stop request must not start another authority tick. A simulation that
    // already entered still takes its normal return/post/publication path.
    if(entering && InterlockedCompareExchange(&g_shared->stop,0,0)) return;
    if(!loop::CompanionHealthy()) {
        if(g_authority_clock.Current()) RejectTick(TickDecision::AbnormalReturn,boundary);
        else Fail(Error::Thread);
        return;
    }
    if(entering) {
        if(g_authority_clock.Current()) {
            RejectTick(g_authority_clock.Begin(boundary,g_authority_session.AcceptsInput(),false),boundary);return;
        }
        g_authority_seen=true;
        const bool loading_recovery=g_loading.BlocksInput();
        const auto resume=active::detail::AuthorityObservedWithLoadingRecovery(g_pause,g_loading);
        if(loading_recovery) {
            auto recovery=MakeEvent(EventKind::LifecycleWindow);
            recovery.action=static_cast<std::uint32_t>(loop::EmbeddedLifecycleSignal::AlternateEnvelope);
            recovery.query=2;recovery.target=loop::CurrentObservationContext().loop_iteration;
            recovery.decision=g_player_only_scope?2u:1u;
            recovery.after={g_lifecycle.loading_depth,g_lifecycle.room_depth,
                (g_participants[0].BoundValid()?1u:0u)|(g_participants[1].BoundValid()?2u:0u),
                g_loading.BlocksInput()?1u:0u};
            recovery.detail=(g_pause.BlocksInput()?1u:0u)|(g_loading.BlocksInput()?2u:0u)|(g_lifecycle.BlocksInput()?4u:0u);
            recovery.boolean=g_lifecycle.Healthy();Publish(recovery);
        }
        if(resume==active::detail::PauseGapTransition::Exit) {InvalidateBoth(Lifecycle::Resume);}
        if(g_lifecycle.BlocksInput() || g_session_retired) return;
        if(!g_authority_session.AllowsBinding()) return;
        Topology topology{};
        if(!ReadTopology(topology)) InvalidateBoth(Lifecycle::Rebind);
        else RefreshBindings(topology);
        if(g_ever_registered && !g_device.Detached() && (!g_device.ExactMember() || !g_device.Healthy())) {Fail(Error::Device);return;}
        // Warmup must leave topology/Join operational, but must not consume a
        // script, allocate input sequences, resolve, or advance the scheduler.
        if(g_authority_session.Status().state==SessionState::Warmup) {
            const auto before=g_authority_session.Status();
            if(g_attempt==2 && g_device.Registered() && g_device.ExactMember())
                g_authority_session.Activate(g_participants[0].BoundValid(),g_participants[1].BoundValid());
            PublishSessionTransition(before);
        }
        if(!g_authority_session.AcceptsInput()) return;
        const bool blocked=g_pause.BlocksInput() || g_loading.BlocksInput() || g_lifecycle.BlocksInput();
        const auto admissible=g_authority_clock.CheckStart(boundary,true,blocked);
        if(admissible!=TickDecision::Started) {
            if(admissible!=TickDecision::Blocked && admissible!=TickDecision::Inactive)
                RejectTick(g_authority_clock.Begin(boundary,true,blocked),boundary);
            return;
        }
        const auto draining=g_tick_pipeline.BeginDrain(boundary,blocked);
        if(draining!=TickDecision::Started) {RejectTick(draining,boundary);return;}
        PublishPhysical();
        ScriptInput();
        if((g_protocol_enabled || g_t09_enabled) && Enabled()) {
            const auto admit=[](const InputCmdDraftV2& command,std::uint64_t generation,void*) noexcept ->std::uint32_t {
                return static_cast<std::uint32_t>(Submit(1,command,generation));
            };
            const bool pumped=g_t09_enabled?
                g_t09_link.PumpInputs(g_authority_clock.Completed(),g_tick_pipeline.RemainingAdmissionAttempts(),admit,nullptr,g_participants[1].Generation()):
                g_protocol_link.PumpInputs(g_authority_clock.Completed(),g_tick_pipeline.RemainingAdmissionAttempts(),admit,nullptr,g_participants[1].Generation());
            if(!pumped) {
                Fail(Error::Protocol);return;
            }
        }
        if(g_t09_enabled && g_ever_registered && g_participants[0].BoundValid() &&
           g_participants[1].BoundValid() && g_authority_session.Status().state==SessionState::Active)
            g_t09_link.MarkGameReady();
        if(g_conflict_pending && g_conflict_generation!=g_participants[1].Generation())g_conflict_pending=false;
        if(g_conflict_pending && Enabled() && g_authority_session.AcceptsInput() && g_participants[1].BoundValid()) {
            g_conflict_pending=false;
            // conflict-b is optional diagnosis. With a small configured admission
            // budget, never let that diagnostic starve or abort a valid A/tick.
            if(g_tick_pipeline.RemainingAdmissionAttempts()>=2) {
                std::uint64_t target=0;
                if(!NextInputTarget(g_authority_clock.Completed(),g_input_config,target)) {
                    RejectTick(TickDecision::Exhausted,boundary);return;
                }
                auto command=g_participants[1].MakeCommand(InputStateDraftV2{},target);
                Submit(1,command,g_participants[1].Generation());
                command.state.move_x=32767;Submit(1,command,g_participants[1].Generation());
            }
        }
        if(!Enabled() || !g_authority_session.AcceptsInput()) {RejectTick(TickDecision::Cancelled,boundary);return;}
        const auto started=g_tick_pipeline.BeginSimulation();
        if(started!=TickDecision::Started) {RejectTick(started,boundary);return;}
        PublishTickBoundary(started,boundary);if(!Enabled()) return;
        for(std::uint32_t i=0;i<2;++i) if(!g_participants[i].Retired()) {
            auto event=MakeEvent(EventKind::Resolve,i);event.boolean=g_participants[i].Visible();
            event.detail=g_player_only_scope?2u:1u;Publish(event);
        }
    } else if(g_authority_clock.Current()) {
        const auto decision=g_tick_pipeline.NativeReturned(boundary,true);
        if(decision!=TickDecision::Committed) {RejectTick(decision,boundary);return;}
        // World Commit is the normal native-return witness, not completed_tick.
        // Outer/tail collection follows; publication waits for TopExit below.
        world::Commit(g_tick_pipeline.Status().candidate,static_cast<std::uint32_t>(boundary.kind));
    }
}
DWORD WINAPI ControllerLostRecovery(void*) noexcept
{
    return IsaacCsT0CoopActiveShutdown(nullptr);
}
void TopExit(void*,void*) noexcept
{
    Callback callback;if(!g_shared || !MainThread()) return;
    const bool controller_lost=g_controller && WaitForSingleObject(g_controller,0)==WAIT_OBJECT_0;
    if(controller_lost) {InterlockedExchange(&g_shared->stop,1);Fail(Error::Publication,SessionCause::ControllerLost);}
    const auto context=loop::CurrentObservationContext();
    const bool missed_native_post=g_tick_call.observed;g_tick_call.observed=false;
    if(g_tick_pipeline.Status().phase==TickPipelinePhase::AwaitPostState) {
        const auto post=world::PendingPostState();
        const auto decision=g_tick_pipeline.FinishPostState(context.loop_iteration,post);
        if(decision!=TickDecision::Committed) {
            TickBoundary invalid{};invalid.loop=context.loop_iteration;RejectTick(decision,invalid);
        } else {
            InterlockedExchange64(&g_shared->completed_tick,static_cast<LONG64>(g_authority_clock.Completed()));
            const auto& receipt=g_tick_pipeline.Receipt();
            // Release copied world values after completed is public. A mismatched
            // sample is explicitly unavailable and must never publish its buffer.
            const bool matching=post.tick==receipt.tick && post.status==receipt.post.status && post.capture==receipt.post.capture;
            // T0-09 P1 does not create a world-record mapping; its minimal
            // observation is the typed Gap packet. Do not turn the intentionally
            // absent world buffer into a Publication failure.
            bool published=g_t09_enabled ||
                world::PublishPostState(matching && receipt.post.status!=PostStateStatus::Unavailable);
            PublishTickBoundary(decision,receipt.boundary);
            auto commit=MakeEvent(EventKind::AuthorityCommit);commit.detail=static_cast<std::uint32_t>(receipt.kind);
            const auto progress=g_tick_pipeline.Status();
            commit.before={progress.drained,progress.budget,static_cast<std::uint32_t>(receipt.post.attempt),static_cast<std::uint32_t>(receipt.post.attempt>>32)};
            commit.after={static_cast<std::uint32_t>(receipt.post.status),receipt.post.reason,
                static_cast<std::uint32_t>(receipt.post.capture),static_cast<std::uint32_t>(receipt.post.capture>>32)};
            published=Publish(commit) && published;
            for(std::uint32_t i=0;i<2;++i) {
                const auto& client=receipt.clients[i];auto event=MakeEvent(EventKind::AuthorityTickReceipt,i);
                event.command=client.client;event.target=client.processed;
                event.generation=client.processed_generation;event.query=static_cast<std::uint32_t>(receipt.kind);
                event.detail=static_cast<std::uint32_t>(receipt.post.status);event.action=receipt.post.reason;
                event.boolean=client.current_binding?1u:0u;
                event.before={static_cast<std::uint32_t>(client.accepted),static_cast<std::uint32_t>(client.accepted>>32),
                    static_cast<std::uint32_t>(client.processed_tick),static_cast<std::uint32_t>(client.processed_tick>>32)};
                event.after={static_cast<std::uint32_t>(client.source),static_cast<std::uint32_t>(client.source>>32),
                    static_cast<std::uint32_t>(receipt.post.capture),static_cast<std::uint32_t>(receipt.post.capture>>32)};
                published=Publish(event) && published;
                const auto& availability=client.availability;
                auto input_event=MakeEvent(EventKind::ClientInputAvailability,i);
                input_event.tick=receipt.tick;input_event.generation=client.generation;
                input_event.command=availability.last_valid_arrival_sequence;
                input_event.target=client.source;
                input_event.query=static_cast<std::uint32_t>(availability.source);
                input_event.decision=static_cast<std::uint32_t>(availability.missing_reason);
                input_event.detail=(availability.effective_available?1u:0u)|
                    (availability.neutral_applied?2u:0u)|(availability.has_valid_arrival?4u:0u);
                input_event.action=static_cast<std::uint32_t>(availability.valid_arrival_count);
                input_event.boolean=client.current_binding?1u:0u;
                input_event.before=ProgressWords(availability.last_valid_arrival_tick,
                    availability.effective_source_tick);
                input_event.after=ProgressWords(availability.effective_age_ticks,availability.silent_ticks);
                published=Publish(input_event) && published;
            }
            if((g_protocol_enabled || g_t09_enabled) && published && Enabled()) {
                const auto progress_b=g_participants[1].Progress(receipt.tick);
                const auto& b=receipt.clients[1];protocol::Ack ack{};
                ack.server_tick=receipt.tick;ack.client_id=b.client;ack.accepted=b.accepted;ack.processed=b.processed;
                ack.processed_tick=b.processed_tick;ack.processed_source=b.source;ack.generation=b.processed_generation;
                ack.rejected_count=progress_b.rejected_count;ack.session_state=g_authority_session.Status().state;
                for(std::uint32_t i=3;i<progress_b.rejections.size();++i)if(progress_b.rejections[i])ack.rejection_mask|=1u<<i;
                protocol::StateObservation state{};state.tick=receipt.tick;state.status=protocol::StateStatus::Gap;
                state.attempt=receipt.post.attempt;state.capture=receipt.post.capture;
                state.reason=static_cast<std::uint16_t>(receipt.post.reason?receipt.post.reason:protocol::kValuesNotExported);
                if(receipt.kind==TickKind::OddPlayer) {state.phase=protocol::CapturePhase::OddOnly;state.reason=2;}
                if(g_t09_enabled) {
                    // P1 transports a truthfully marked minimal observation gap.
                    // State equality/sampling is T0-09-C scope.
                    state.run=1;state.room=1;state.attempt=receipt.post.attempt?receipt.post.attempt:1;
                    state.capture=receipt.post.capture;state.source_token=receipt.tick;
                    if(!g_t09_link.Publish(ack,state,receipt.tick)) published=false;
                } else if(!world::ObservationIdentity(receipt.tick,state.run,state.room,state.source_token) ||
                          !g_protocol_link.Publish(ack,state,receipt.tick)) published=false;
            }
            if(!g_tick_pipeline.PublishCompleted(published && Enabled()))Fail(Error::Publication,SessionCause::PublicationFailure);
        }
    }
    auto escaped=g_tick_pipeline.EndOuter(context.loop_iteration);
    if(escaped==TickDecision::Idle && missed_native_post)
        escaped=g_authority_clock.Cancel(Enabled() && g_authority_session.AllowsBinding()?
            TickDecision::MissingReturn:TickDecision::Cancelled);
    if(escaped!=TickDecision::Idle) {
        TickBoundary boundary{};boundary.loop=context.loop_iteration;
        boundary.thread=context.thread_id;boundary.main_thread=context.main_thread_id;
        RejectTick(escaped,boundary);
    }
    const bool stop=InterlockedCompareExchange(&g_shared->stop,0,0)!=0;
    if(g_protocol_enabled && (stop || g_session_retired || g_failed))g_protocol_link.Retire(g_authority_clock.Completed());
    if(g_t09_enabled && (stop || g_session_retired || g_failed))g_t09_link.Retire(g_authority_clock.Completed());
    if(stop) {
        const auto before=g_authority_session.Status();
        g_authority_session.BeginRetirement(SessionCause::Stop);PublishSessionTransition(before);
    }
    if(Enabled() && g_authority_session.AllowsBinding() && !g_session_retired && !g_ever_registered && !g_lifecycle.BlocksInput()) {
        Topology topology{};
        if(ReadTopology(topology) && topology.count==1) {
            RefreshBindings(topology);
            if(g_participants[0].BoundValid()) {
                Diagnose(DiagnosticStage::DeviceCreate,1,DiagnosticReason::Ready,
                    {static_cast<std::uint32_t>(g_device.State())});
                const bool created=g_device.Create(g_image,g_module,GetCurrentThreadId(),&OwnedReader,nullptr);
                InterlockedExchange(&g_shared->pinned,(world::EntryCount() || g_device.Pinned())?1:0);
                Diagnose(DiagnosticStage::DeviceCreate,1,static_cast<DiagnosticReason>(g_device.Error()),
                    {static_cast<std::uint32_t>(g_device.State()),static_cast<std::uint32_t>(g_device.Key()),g_device.Pinned()?1u:0u},
                    {Address(g_device.Pair().object),Address(g_device.Pair().control),g_device.Strong()});
                if(!created) {Fail(Error::Device);return;}
                const bool registered=g_device.Register();
                Diagnose(DiagnosticStage::DeviceRegister,1,static_cast<DiagnosticReason>(g_device.Error()),
                    {static_cast<std::uint32_t>(g_device.State()),static_cast<std::uint32_t>(g_device.Key()),registered?1u:0u},
                    {Address(g_device.Pair().object),Address(g_device.Pair().control),g_device.Strong()});
                if(!registered) {Fail(Error::Device);return;}
                g_ever_registered=true;auto event=MakeEvent(EventKind::DeviceState,1);
                event.detail=static_cast<std::uint32_t>(g_device.State());event.boolean=1;
                event.before={static_cast<std::uint32_t>(g_device.Key()),Address(g_device.Pair().object),Address(g_device.Pair().control),g_device.Strong()};
                Publish(event);
            }
        }
    }
    const LONG control=InterlockedExchange(&g_shared->retire_b,0);
    if(control==4 && Enabled() && !g_session_retired) {
        g_repeat_queries=true;g_repeat_budget=256;g_repeat_generation={};g_repeat_used={};
    }
    if(control==2 && g_authority_session.AcceptsInput() && !g_participants[1].Retired() && g_script_index>g_shared->script_count) {
        g_script_origin=0;g_script_index=0;g_script_shoot_active=false;
    }
    if(control==3 && !g_protocol_enabled && !g_t09_enabled && g_authority_session.AcceptsInput() && g_participants[1].BoundValid()) {
        g_conflict_pending=true;
        g_conflict_generation=g_participants[1].Generation();
    }
    const bool retire=!g_b_retired_done &&
        (control==1 || g_participants[1].Retired());
    if(stop || g_session_retired || g_failed) world::Close();
    if((stop || retire || g_session_retired || g_failed) && !g_stop_handled) {
        if(!g_b_retired_done && !RetireB()) {Fail(Error::Restore);return;}
        if(stop || g_session_retired || g_failed) {
            g_participants[0].Retire();PublishClientProgress(0);InterlockedExchange(&g_enabled,0);
            InterlockedExchange(&g_stop_handled,1);
        } else InterlockedExchange(&g_shared->retire_b,0);
    }
    if(Enabled() && !g_session_retired && !g_lifecycle.BlocksInput() &&
       active::detail::TopUpdateCompletedWithLoadingRecovery(g_pause,g_loading,g_authority_seen)==
           active::detail::PauseGapTransition::Enter) InvalidateBoth(Lifecycle::Pause);
    g_authority_seen=false;
    if(controller_lost && g_stop_handled && !InterlockedExchange(&g_recovery_started,1)) {
        HANDLE recovery=CreateThread(nullptr,0,&ControllerLostRecovery,nullptr,0,nullptr);
        if(recovery) CloseHandle(recovery); // Resident shutdown only; never unload/reinject.
    }
}
void LifecycleCallback(loop::EmbeddedLifecycleSignal signal,bool entering,void*,void* caller,void*) noexcept
{
    world::Lifecycle(static_cast<std::uint32_t>(signal),entering);
    Callback callback;if(!Enabled() || !MainThread()) return;
    auto event=MakeEvent(EventKind::LifecycleWindow);
    event.action=static_cast<std::uint32_t>(signal);event.query=entering?1u:0u;
    event.target=loop::CurrentObservationContext().loop_iteration;
    event.decision=Address(caller)-Address(g_image);
    event.before={g_lifecycle.loading_depth,g_lifecycle.room_depth,
        (g_participants[0].BoundValid()?1u:0u)|(g_participants[1].BoundValid()?2u:0u),
        g_pause.consecutive_missing};
    if(signal==loop::EmbeddedLifecycleSignal::RunTeardown) {
        if(entering) {
            const auto before=g_authority_session.Status();
            g_authority_session.BeginRetirement(SessionCause::RunTeardown);PublishSessionTransition(before);
            g_session_retired=true;g_join_scope=false;g_lifecycle.Retire();InvalidateBoth(Lifecycle::Teardown);
        }
    } else {
        const bool loading=signal==loop::EmbeddedLifecycleSignal::AlternateEnvelope;
        const auto domain=loading?active::detail::LifecycleInputDomain::Loading:active::detail::LifecycleInputDomain::Room;
        if(entering) {
            g_lifecycle.Enter(domain);InvalidateBoth(loading?Lifecycle::Loading:Lifecycle::Room);
            if(loading) g_loading.LoadingEntered();
        } else {
            const auto transition=g_lifecycle.Exit(domain);
            if(loading && transition==active::detail::LifecycleGateTransition::Exited) {
                g_loading.LoadingExited();InvalidateBoth(Lifecycle::LoadingExit);
            }
        }
    }
    event.after={g_lifecycle.loading_depth,g_lifecycle.room_depth,
        (g_participants[0].BoundValid()?1u:0u)|(g_participants[1].BoundValid()?2u:0u),
        g_loading.BlocksInput()?1u:0u};
    event.detail=(g_pause.BlocksInput()?1u:0u)|(g_loading.BlocksInput()?2u:0u)|(g_lifecycle.BlocksInput()?4u:0u);
    event.boolean=g_lifecycle.Healthy();Publish(event);
    if(!g_lifecycle.Healthy()) Fail(Error::Topology);
}
bool Prepare() noexcept
{
    active::ActiveReplacements replacements{};
    replacements.relative={reinterpret_cast<void*>(&Movement),reinterpret_cast<void*>(&Bomb),
        reinterpret_cast<void*>(&Item),reinterpret_cast<void*>(&Pill),reinterpret_cast<void*>(&Shooting),reinterpret_cast<void*>(&Drop)};
    replacements.pointers={reinterpret_cast<void*>(&KeyboardPressed),reinterpret_cast<void*>(&KeyboardTriggered),reinterpret_cast<void*>(&KeyboardScalar),
        reinterpret_cast<void*>(&GamepadPressed),reinterpret_cast<void*>(&GamepadTriggered),reinterpret_cast<void*>(&GamepadScalar),reinterpret_cast<void*>(&PlayerUpdate)};
    std::array<mode::RelativeCallPatch,6> consumers{};active::TargetFailure failure{};
    if(!active::PrepareActiveTargets(g_image,replacements,consumers,g_pointers,failure)) return false;
    for(std::size_t i=0;i<6;++i) {g_relative[i]=consumers[i];g_consumers[i]=reinterpret_cast<ThisVoid>(consumers[i].original);}
    constexpr std::uint32_t rvas[2]{0x2FADA7,0x5BB415},targets[2]{0x5BAC40,0x5B9CD0};
    constexpr std::uint8_t bytes[2][5]{{0xE8,0x94,0xFE,0x2B,0},{0xE8,0xB6,0xE8,0xFF,0xFF}};
    for(std::size_t i=0;i<2;++i) {
        auto& patch=g_relative[6+i];patch.call=g_image+rvas[i];
        if(std::memcmp(patch.call,bytes[i],5)) return false;
        std::memcpy(patch.expected,bytes[i],5);patch.original=g_image+targets[i];
    }
    g_relative[6].replacement=reinterpret_cast<void*>(&JoinPass);g_relative[7].replacement=reinterpret_cast<void*>(&CreateHook);
    auto& player_only=g_relative[8];player_only.call=g_image+0x555241;
    constexpr std::uint8_t player_only_bytes[5]{0xE8,0xAA,0x81,0xDA,0xFF};
    if(std::memcmp(player_only.call,player_only_bytes,5)) return false;
    std::memcpy(player_only.expected,player_only_bytes,5);
    player_only.original=g_image+0x2FD3F0;
    player_only.replacement=reinterpret_cast<void*>(&PlayerOnlyUpdate);
    g_player_only_original=reinterpret_cast<VoidFn>(player_only.original);
    constexpr std::uint32_t mode_rvas[3]{0x5BAE7E,0x5BB081,0x5BB288};
    constexpr std::uint8_t mode_bytes[3][5]{{0xE8,0xFD,0xFA,0xFF,0xFF},
        {0xE8,0xFA,0xF8,0xFF,0xFF},{0xE8,0xF3,0xF6,0xFF,0xFF}};
    const std::array<void*,3> modes{reinterpret_cast<void*>(&JoinHealthMode),
        reinterpret_cast<void*>(&JoinDeviceMode),reinterpret_cast<void*>(&JoinSelectorMode)};
    for(std::size_t i=0;i<modes.size();++i) {
        auto& patch=g_relative[9+i];patch.call=g_image+mode_rvas[i];
        if(std::memcmp(patch.call,mode_bytes[i],5)) return false;
        std::memcpy(patch.expected,mode_bytes[i],5);patch.original=g_image+0x5BA980;
        patch.replacement=modes[i];
    }
    g_join_mode_original=reinterpret_cast<JoinModeFn>(g_relative[9].original);
    g_join_original=reinterpret_cast<JoinFn>(g_relative[6].original);
    g_coop_create_original=g_relative[7].original;g_coop_no_create=g_image+0x5BB561;
    g_bool_queries={reinterpret_cast<ThisBool>(g_pointers[0].original),reinterpret_cast<ThisBool>(g_pointers[1].original),
        reinterpret_cast<ThisBool>(g_pointers[3].original),reinterpret_cast<ThisBool>(g_pointers[4].original)};
    g_scalar_queries={reinterpret_cast<ThisFloat>(g_pointers[2].original),reinterpret_cast<ThisFloat>(g_pointers[5].original)};
    g_player_update=reinterpret_cast<ThisVoid>(g_pointers[6].original);return true;
}
void RestorationTarget(std::uint32_t rva,const std::uint8_t* expected,
    const std::uint8_t* actual,std::size_t size,void*) noexcept
{
    auto event=MakeEvent(EventKind::Restored);event.action=rva;
    event.query=static_cast<std::uint32_t>(size);event.boolean=std::memcmp(expected,actual,size)==0;
    std::memcpy(event.before.data(),expected,size);std::memcpy(event.after.data(),actual,size);Publish(event);
}
}
void SetModule(HMODULE module) noexcept {g_module=module;}
#if defined(ISAAC_CS_WORLD_E2E)
void CopyWorldE2EWindowFacts(std::uint32_t* out) noexcept
{
    out[0]=(g_pause.BlocksInput()?1u:0u)|(g_loading.BlocksInput()?2u:0u)|
        (g_lifecycle.BlocksInput()?4u:0u)|(g_session_retired?8u:0u);
    out[1]=g_pause.consecutive_missing;
    out[2]=g_lifecycle.loading_depth;out[3]=g_lifecycle.room_depth;
    out[4]=(g_participants[0].BoundValid()?1u:0u)|(g_participants[1].BoundValid()?2u:0u);
    out[5]=g_authority_seen?1u:0u;
}
#endif
void ProcessDetach() noexcept {InterlockedExchange(&g_enabled,0);}
extern "C" DWORD WINAPI IsaacCsT0CoopActiveInitialize(const wchar_t* name)
{
    if(!name || !*name || g_shared) return static_cast<DWORD>(Error::Arguments);
    g_image=reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    g_mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);if(!g_mapping) return static_cast<DWORD>(Error::Arguments);
    g_shared=static_cast<Shared*>(MapViewOfFile(g_mapping,FILE_MAP_ALL_ACCESS,0,0,0));
    if(!g_shared) return static_cast<DWORD>(Error::Arguments);
    MEMORY_BASIC_INFORMATION region{};
    if(!VirtualQuery(g_shared,&region,sizeof(region)) || region.RegionSize<sizeof(Shared))
        return static_cast<DWORD>(Error::Arguments);
    if(g_shared->magic!=kMagic || !((g_shared->version==3 && g_shared->size==sizeof(Shared)) ||
         (g_shared->version==4 && g_shared->size==sizeof(SharedV4)) ||
         (g_shared->version==5 && g_shared->size==sizeof(SharedV5)) ||
         (g_shared->version==6 && g_shared->size==sizeof(SharedV6)) ||
         (g_shared->version==7 && g_shared->size==sizeof(SharedV7))) ||
       g_shared->pid!=GetCurrentProcessId() || g_shared->script_count>kScriptSteps ||
       !g_shared->clients[0] || !g_shared->clients[1] || g_shared->clients[0]==g_shared->clients[1] ||
       !g_shared->loop_name[0] || g_shared->loop_name[95]) return static_cast<DWORD>(Error::Arguments);
    if(g_shared->size>region.RegionSize) return static_cast<DWORD>(Error::Arguments);
    if(g_shared->version==6) {
        g_protocol_shared=reinterpret_cast<SharedV6*>(g_shared);
        if(!protocol::ValidFaultConfig(g_protocol_shared->protocol_config))return static_cast<DWORD>(Error::Arguments);
        g_protocol_enabled=true;
    } else if(g_shared->version==7) {
        g_t09_shared=reinterpret_cast<SharedV7*>(g_shared);
        const auto& t09=g_t09_shared->t09;
        const bool run_nonce_nonzero=std::any_of(t09.run_nonce.begin(),t09.run_nonce.end(),
            [](std::uint8_t value){return value!=0;});
        bool build_tail=false,build_nonempty=false,build_valid=true;
        for(auto value:t09.build) {
            if(!value){build_tail=true;continue;}
            if(build_tail||value<33||value>126){build_valid=false;break;}
            build_nonempty=true;
        }
        if(t09.version!=1||t09.size!=sizeof(T09AuthorityConfig)||t09.role!=1||t09.flags!=0||
           !run_nonce_nonzero||!t09.run_ticks||!build_valid||!build_nonempty||
           std::any_of(t09.reserved.begin(),t09.reserved.end(),[](std::uint8_t value){return value!=0;}))
            return static_cast<DWORD>(Error::Arguments);
        g_t09_enabled=true;
    }
    const bool approved=g_t09_enabled?target_image::IsApprovedT09(g_image):
        target_image::IsApproved(g_image);
    if(!approved)return static_cast<DWORD>(Error::Image);
    if(std::all_of(g_shared->session.begin(),g_shared->session.end(),[](auto value){return value==0;}) ||
       std::all_of(g_shared->nonce.begin(),g_shared->nonce.end(),[](auto value){return value==0;}))
        return static_cast<DWORD>(Error::Arguments);
    InputRuntimeConfig input_config{};
    if(ReadInputRuntimeConfig(g_shared,g_shared->version,g_shared->size,input_config)!=
       InputConfigDecision::Accepted) return static_cast<DWORD>(Error::Arguments);
    if(!ApplyInputConfig(input_config)) return static_cast<DWORD>(Error::Arguments);
    const SessionIdentity frozen_identity{g_shared->session,g_shared->clients};
    const auto dormant=g_authority_session.Status();
    if(!g_authority_session.Start(frozen_identity)) return static_cast<DWORD>(Error::Arguments);
    PublishSessionTransition(dormant);
    if(g_failed) return 8;
    const auto initialization_failure=[](Error error) noexcept {
        Fail(error);return static_cast<DWORD>(error);
    };
    FILETIME created{},exited{},kernel{},user{};
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user) ||
       ((static_cast<std::uint64_t>(created.dwHighDateTime)<<32)|created.dwLowDateTime)!=g_shared->creation_time)
        return initialization_failure(Error::Arguments);
    if(!g_shared->controller_pid || g_shared->b_character < -1) return initialization_failure(Error::Arguments);
    g_b_character=g_shared->b_character;
    g_controller=OpenProcess(SYNCHRONIZE,FALSE,g_shared->controller_pid);
    if(!g_controller || WaitForSingleObject(g_controller,0)!=WAIT_TIMEOUT) return initialization_failure(Error::Arguments);
    for(std::uint32_t i=0;i<g_shared->script_count;++i) {
        const auto& step=g_shared->script[i];
        const auto source=step.shoot_directions;
        if(!Valid(step.state) || step.fault>6 || ((g_protocol_enabled||g_t09_enabled) && step.fault!=0) || (source&~31u) ||
           (source && (!(source&16u) || step.fault>1 || (step.state.held_flags&input::kHeldShoot) ||
                       step.state.aim_x || step.state.aim_y)) ||
           (i && g_shared->script[i-1].tick>=step.tick)) return initialization_failure(Error::Arguments);
    }
    if(g_shared->version==4 || g_shared->version==5 || g_shared->version==6 || g_shared->version==7) {
        const auto* config=reinterpret_cast<const SharedV4*>(g_shared);
        const bool world_required=g_shared->version==4 || g_shared->version==6;
        const bool has_world=config->world_name[0]!=0;
        if(config->world_name[95] || (world_required && !has_world) ||
           (has_world && !world::Prepare(config->world_name,g_shared->pid,g_shared->controller_pid,
                                         g_shared->nonce)))
            return initialization_failure(Error::Arguments);
        if(has_world) InterlockedExchange(&g_shared->pinned,1);
    }
    if(!Prepare()) return initialization_failure(Error::Hook);
    const auto& identity_config=g_authority_session.Identity();
    for(std::uint32_t i=0;i<2;++i)
        g_participants[i].Initialize(identity_config.session,identity_config.clients[i],i,&g_authority_session);
    if(g_protocol_enabled) {
        protocol::Config config{};config.session=identity_config.session;config.clients=identity_config.clients;
        config.build=protocol::TextId("isaac-t0-tx-02-00");
        std::copy_n(g_shared->nonce.begin(),16,config.local_nonce.begin());
        std::copy_n(g_shared->nonce.begin()+16,16,config.peer_nonce.begin());
        const auto trace=[](const protocol::TraceRecord& record,void*) noexcept ->bool {
            auto* shared=g_protocol_shared;if(!shared || record.packet.size>protocol::kMaximumPacket)return false;
            const auto next=InterlockedCompareExchange64(&shared->protocol_produced,0,0)+1;
            if(next-InterlockedCompareExchange64(&shared->protocol_consumed,0,0)>static_cast<LONG64>(kProtocolTraceCapacity)) {
                InterlockedIncrement64(&shared->protocol_overwritten);return false;
            }
            auto& slot=shared->protocol_trace[(next-1)%kProtocolTraceCapacity];
            slot.tick=record.tick;slot.token=record.token;slot.generation=record.generation;
            slot.action=static_cast<std::uint32_t>(record.action);slot.direction=static_cast<std::uint32_t>(record.direction);
            slot.decision=record.decision;slot.length=static_cast<std::uint32_t>(record.packet.size);
            std::copy_n(record.packet.bytes.begin(),record.packet.size,slot.bytes.begin());
            MemoryBarrier();InterlockedExchange64(&slot.published,next);InterlockedExchange64(&shared->protocol_produced,next);return true;
        };
        if(!g_protocol_link.Initialize(config,g_protocol_shared->protocol_config,trace,nullptr))return initialization_failure(Error::Protocol);
    }
    if(g_t09_enabled) {
        protocol::Config config{};config.session=identity_config.session;config.clients=identity_config.clients;
        config.build=g_t09_shared->t09.build;
        std::copy_n(g_shared->nonce.begin(),16,config.local_nonce.begin());
        std::copy_n(g_shared->nonce.begin()+16,16,config.peer_nonce.begin());
        constexpr char digits[]="0123456789abcdef";
        std::wstring wire_name=L"Local\\IsaacCsT0.Wire.";
        for(auto byte:g_t09_shared->t09.run_nonce) {
            wire_name.push_back(static_cast<wchar_t>(digits[byte>>4]));
            wire_name.push_back(static_cast<wchar_t>(digits[byte&15]));
        }
        wire_name+=L".A";
        if(!g_t09_link.Initialize(config,wire_name.c_str(),g_t09_shared->t09.run_nonce,
                                  g_t09_shared->t09.run_ticks))
            return initialization_failure(Error::Protocol);
    }
    if(!PublishInputRuntimeConfig(g_input_config,g_shared->version))
        return initialization_failure(Error::Publication);
    auto character=MakeEvent(EventKind::CharacterSelection,1);character.action=1;
    character.decision=static_cast<std::uint32_t>(g_b_character);character.boolean=1;
    if(!Publish(character)) return static_cast<DWORD>(Error::Publication);
    loop::EmbeddedObserverHooks hooks{};
    hooks.relative=g_relative.data();hooks.relative_count=g_relative.size();hooks.relative_ordinals=g_relative_ordinals.data();
    hooks.pointers=g_pointers.data();hooks.pointer_count=g_pointers.size();hooks.pointer_ordinals=g_pointer_ordinals.data();
    hooks.entries=world::Entries();hooks.entry_count=world::EntryCount();
    hooks.entry_ordinals=world::Ordinals();
    hooks.strict_ordered_transaction=true; // Null digest pointer: no hashing on this path.
    hooks.physical_exit=&PhysicalExit;hooks.authority=&Authority;hooks.top_update_exit=&TopExit;hooks.lifecycle=&LifecycleCallback;
    loop::EmbeddedObserverIdentity identity{g_shared->pid,1,g_shared->nonce};
    const auto negotiating=g_authority_session.Status();
    if(!g_authority_session.Prepared()) return initialization_failure(Error::Protocol);
    PublishSessionTransition(negotiating);
    if(g_failed) return 8;
    InterlockedExchange(&g_enabled,1);InterlockedExchange(&g_hook_owner,1);
    const DWORD result=loop::StartEmbeddedObserver(g_shared->loop_name,hooks,identity);
    if(result) {Fail(Error::Hook,SessionCause::HookFailure,result);return 8;}
    if(g_failed) return 8;
    world::Arm();
    InterlockedExchange(&g_shared->state,2);Publish(MakeEvent(EventKind::Installed));return 0;
}
extern "C" DWORD WINAPI IsaacCsT0CoopActiveShutdown(void*)
{
    if(!g_shared) return static_cast<DWORD>(Error::Arguments);
    if(InterlockedCompareExchange(&g_shared->restored,0,0)) return 0;
    if(InterlockedCompareExchange(&g_shutdown_owner,1,0)) return 8;
    // Restoration failures close session admission without touching game objects
    // from this helper thread. Main-thread retirement remains in TopExit.
    const auto restoration_failure=[]() noexcept {
        const auto before=g_authority_session.Status();
        g_authority_session.Fail(SessionCause::RestoreFailure);PublishSessionTransition(before);
        return DWORD{8};
    };
    // Only request here. TopExit completes the in-flight native tick, publishes
    // its receipt and protocol lanes, then closes world collection on main thread.
    InterlockedExchange(&g_shared->stop,1);
    const auto deadline=GetTickCount64()+5000;
    while(!InterlockedCompareExchange(&g_stop_handled,0,0) && GetTickCount64()<deadline) Sleep(1);
    if(!g_stop_handled) return restoration_failure();
    if(loop::RestoreEmbeddedInputHooks()) return restoration_failure();
    while((InterlockedCompareExchange(&g_callbacks,0,0) || !world::Drained()) && GetTickCount64()<deadline) Sleep(1);
    if(g_callbacks || !world::Drained() || loop::StopEmbeddedObserver()) return restoration_failure();
    __try {
        for(const auto& patch:g_relative)
            if(patch.installed || std::memcmp(patch.call,patch.expected,5)) return restoration_failure();
        for(const auto& patch:g_pointers)
            if(patch.installed || *patch.slot!=patch.expected_original) return restoration_failure();
    } __except(EXCEPTION_EXECUTE_HANDLER) {return restoration_failure();}
    std::uint32_t count=0;if(!loop::VerifyEmbeddedObserverRestored(count)) return restoration_failure();
    if(!loop::ObserveEmbeddedRestoredTargets(&RestorationTarget,nullptr)) return restoration_failure();
    InterlockedExchange(&g_hook_owner,0);
    auto event=MakeEvent(EventKind::Restored);event.detail=count;event.boolean=1;Publish(event);
    const auto retiring=g_authority_session.Status();
    if(!g_authority_session.CompleteRetirement(true)) return restoration_failure();
    PublishSessionTransition(retiring);
    InterlockedExchange(&g_shared->restored,1);InterlockedExchange(&g_shared->state,4);
    // Deliberately no FreeLibrary/FreeLibraryAndExitThread; owned ABI code stays valid.
    return 0;
}
} // namespace isaac_cs_t0::coop_active
