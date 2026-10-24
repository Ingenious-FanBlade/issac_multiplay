#include "isaac_cs_t0/world_record.hpp"
#include "isaac_cs_t0/world_e2e.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"
#include "isaac_cs_t0/coop_active_runtime.hpp"
#include <algorithm>
#include <cstring>
#include <intrin.h>
#include <limits>
#include <atomic>

namespace isaac_cs_t0::world {
namespace {
Shared* g_shared=nullptr;
HANDLE g_mapping=nullptr;
HANDLE g_ready=nullptr;
std::uint8_t* g_image=nullptr;
std::array<mode::EntryPatch,19> g_entries{};
const std::array<std::uint32_t,19> g_ordinals{51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69};
extern "C" {void* g_world_trampolines[19]{};volatile LONG g_world_active=0;}
std::uint64_t g_run=1,g_room=1,g_mutation=0,g_next_id=0,g_birth=0,g_outer=0,g_attempt=0,g_capture=0;
std::uint32_t g_lifecycle_depth=0;
bool g_retired=false;
std::atomic<Batch*> g_pending_batch{nullptr};
coop_active::TickPostState g_post_state{}; // Main-thread value receipt, outside IPC.
std::uint64_t g_post_run=0,g_post_room=0,g_post_token=0;
struct Identity {
    std::uint32_t address=0,domain=0,origin=0,lifecycle=0,init=0,ordinal=UINT32_MAX;
    std::uint64_t id=0,use=0,birth=0,revision=0;
    std::uint32_t birth_kind=0,type=0,variant=0,subtype=0,seed=0;
    std::uint32_t owner_scope=0,memberships=0;std::uint64_t membership_attempt=0;
    bool born=false,destroyed=false,reused=false,published=false,candidate=false;
};
std::array<Identity,kIdentityBudget> g_identities{};
std::array<std::uint32_t,kIdentityBudget> g_batch_indices{};
std::array<std::uint32_t,kIdentityBudget> g_current_memberships{};
std::uint32_t g_source_addresses[kMembershipBudget];
#if defined(ISAAC_CS_WORLD_E2E)
struct RngAudit {std::uint32_t address=0,domain=0;std::array<std::uint32_t,4> before{};bool valid=false,eligible=false;};
std::array<RngAudit,kObjectBudget> g_rng_audit{};
#endif
std::array<std::uint32_t,2> g_players{};
struct Token {
    std::uint64_t value=0,mutation=0,tick=0,scope=0;
    std::uint32_t owner_enter=0,owner_exit=0,room_enter=0,room_exit=0,tail_enter=0,tail_exit=0,
        commit_kind=0,commits=0;
    bool dirty=false,owner_active=false,room_active=false,tail_active=false;
    std::uint32_t room_address=0;
};
thread_local Token g_token{};
struct Frame {
    std::uint32_t hook=0,self=0,stack=0,return_address=0;
    std::array<std::uint32_t,8> args{};
};
thread_local std::array<Frame,64> g_frames{};
thread_local std::uint32_t g_depth=0;
constexpr std::uint32_t kArgumentWords[19]{2,4,1,0,0,0,0,1,1,2,1,4,6,0,1,5,1,3,8};
bool Enabled() noexcept {return g_shared && InterlockedCompareExchange(&g_shared->enabled,0,0)!=0;}
std::uint32_t Address(const void* p) noexcept {return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p));}
bool Read(std::uint64_t address,void* out,std::size_t size) noexcept
{
    const std::uint32_t words[]{static_cast<std::uint32_t>(address),static_cast<std::uint32_t>(size)};
    if(e2e::Copying())e2e::Record(e2e::Kind::Read,0,words,2);
    if(!address || address>UINT32_MAX || size>UINT32_MAX-address)return false;
    __try {std::memcpy(out,reinterpret_cast<const void*>(static_cast<std::uintptr_t>(address)),size);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool U32(std::uint64_t address,std::uint32_t& value) noexcept {return Read(address,&value,4);}
bool Notify() noexcept
{
    if(g_ready && !SetEvent(g_ready) && g_shared) {
        InterlockedCompareExchange(&g_shared->error,static_cast<LONG>(Reason::Stopped),0);
        InterlockedExchange(&g_shared->enabled,0);
        return false;
    }
    return true;
}
void Error(Reason reason) noexcept
{
    if(!g_shared)return;
    e2e::Record(e2e::Kind::Error,static_cast<std::uint32_t>(reason));
    InterlockedCompareExchange(&g_shared->error,static_cast<LONG>(reason),0);
    InterlockedExchange(&g_shared->enabled,0);
    g_token.dirty=true;
    Notify();
}
bool Next(std::uint64_t& value) noexcept
{if(value>=UINT64_MAX-1){Error(Reason::SequenceExhausted);return false;}++value;return true;}
bool MainThread() noexcept
{
    const auto context=loop::CurrentObservationContext();
    if(!context.main_thread_id)return false; // Installation can precede the first clock anchor.
    if(context.thread_id!=context.main_thread_id){Error(Reason::WrongThread);return false;}
    if(!loop::CompanionHealthy()){Error(Reason::Continuity);return false;}
    return true;
}
#include "world_e2e_bridges.inc"
// Bounded radix tree over the literal (domain,address) bits. Addresses are
// diagnostic associations only; no digest, checksum or hash is computed.
struct IdentityNode {std::uint32_t child[2];std::uint32_t index;};
IdentityNode g_identity_nodes[kIdentityBudget*34];
std::uint32_t g_identity_nodes_used=1,g_identities_used=0;
Identity* Find(std::uint32_t domain,std::uint32_t address,bool create=false) noexcept
{
    if(!address)return nullptr;
    const auto key=(static_cast<std::uint64_t>(domain)<<32)|address;
    std::uint32_t node=0;
    for(int bit=32;bit>=0;--bit) {
        const auto branch=static_cast<std::uint32_t>((key>>bit)&1u);
        auto& child=g_identity_nodes[node].child[branch];
        if(!child) {
            if(!create)return nullptr;
            if(g_identity_nodes_used==std::size(g_identity_nodes)){Error(Reason::Budget);return nullptr;}
            child=g_identity_nodes_used++;
        }
        node=child;
    }
    auto& index=g_identity_nodes[node].index;
    if(index)return &g_identities[index-1];
    if(!create)return nullptr;
    if(g_identities_used==kIdentityBudget || !Next(g_next_id)){Error(Reason::Budget);return nullptr;}
    index=++g_identities_used;auto& identity=g_identities[index-1];
    identity.address=address;identity.domain=domain;identity.id=g_next_id;identity.use=1;
    identity.ordinal=UINT32_MAX;return &identity;
}

void Emit(std::uint32_t hook,bool entering,Identity* identity,std::uint32_t address,
          std::uint32_t result,Reason reason=Reason::None,std::uint32_t kind=0) noexcept
{
    if(!g_shared)return;
    const auto produced=InterlockedCompareExchange64(&g_shared->produced,0,0);
    const auto consumed=InterlockedCompareExchange64(&g_shared->consumed,0,0);
    if(produced==INT64_MAX){Error(Reason::SequenceExhausted);return;}
    if(produced-consumed>=kEventBudget || e2e::Take(e2e::Fault::EventOverflow)){Error(Reason::EventOverflow);return;}
    auto& event=g_shared->events[static_cast<std::size_t>(produced)%kEventBudget];
    event.run=g_run;event.room=g_room;event.outer=g_token.value;
    event.id=identity?identity->id:0;event.use=identity?identity->use:0;
    event.birth=identity?identity->birth:0;event.revision=identity?identity->revision:0;
    event.hook=hook;event.entering=entering?1u:0u;event.address=address;
    event.result=result;event.reason=static_cast<std::uint32_t>(reason);event.kind=kind;
    MemoryBarrier();InterlockedExchange64(&event.published,produced+1);
    InterlockedExchange64(&g_shared->produced,produced+1);
    // Re-read after publication: the reader may have consumed the previous tail
    // since the capacity check. Using the old value can miss an empty->nonempty
    // transition exactly as the reader starts waiting.
    if(produced==InterlockedCompareExchange64(&g_shared->consumed,0,0))Notify();
}
bool HasContext(Hook hook) noexcept
{for(std::uint32_t i=0;i<g_depth;++i)if(g_frames[i].hook==static_cast<std::uint32_t>(hook))return true;return false;}
bool InImage(std::uint32_t value,std::uint32_t bytes=4) noexcept
{return value>=Address(g_image) && value-Address(g_image)<=0x8FE000u-bytes;}
bool HasBase(std::uint32_t vptr,std::uint32_t descriptor) noexcept
{
    std::uint32_t col=0,hierarchy=0,count=0,table=0;
    if(!InImage(vptr) || !U32(vptr-4,col) || !InImage(col,20) ||
        !U32(static_cast<std::uint64_t>(col)+16,hierarchy) || !InImage(hierarchy,16) ||
        !U32(static_cast<std::uint64_t>(hierarchy)+8,count) || count>32 || !U32(static_cast<std::uint64_t>(hierarchy)+12,table) || !InImage(table,count*4))return false;
    for(std::uint32_t i=0;i<count;++i) {
        std::uint32_t base=0,type=0;
        if(!U32(static_cast<std::uint64_t>(table)+i*4,base) || !InImage(base) || !U32(base,type))return false;
        if(type==Address(g_image)+descriptor)return true;
    }
    return false;
}
Class Classify(std::uint32_t domain,std::uint32_t vptr) noexcept
{
    if(domain==1)return HasBase(vptr,0x85D7A4)?Class::Grid:Class::Unknown;
    constexpr std::uint32_t descriptors[7]{0x85D6DC,0x85D6AC,0x85D900,0x85D70C,0x85D7D4,0x85D678,0x85D5AC};
    for(std::uint32_t i=0;i<7;++i)if(HasBase(vptr,descriptors[i]))return static_cast<Class>(i);
    return Class::Unknown;
}
void Acquire(Identity& id,Hook kind,bool physical=false) noexcept
{
    if(id.origin || id.destroyed) {
        if(!Next(id.use))return;
        id.reused=true;
        if(physical || id.destroyed) {if(!Next(g_next_id))return;id.id=g_next_id;}
    }
    id.origin=2;id.lifecycle=1;id.init=0;id.birth=0;id.born=false;id.revision=0;id.ordinal=UINT32_MAX;
    id.destroyed=false;id.published=false;id.candidate=true;id.memberships=0;id.membership_attempt=0;
    id.birth_kind=static_cast<std::uint32_t>(kind);id.owner_scope=0;
    for(std::uint32_t i=g_depth;i;--i) {
        const auto hook=static_cast<Hook>(g_frames[i-1].hook);
        if(hook==Hook::Spawn || hook==Hook::PlayerCreate || hook==Hook::ManagerRestore ||
            hook==Hook::GridSpawn || hook==Hook::Door){id.owner_scope=g_frames[i-1].stack;break;}
    }
}
void Confirm(Identity& id,Hook kind) noexcept
{
    if(id.born || !id.candidate || !id.init)return;
    std::uint32_t values[3]{};
    const bool header=id.domain==1?U32(static_cast<std::uint64_t>(id.address)+4,values[0]) && U32(static_cast<std::uint64_t>(id.address)+8,values[1]):
        Read(static_cast<std::uint64_t>(id.address)+0x28,values,12);
    std::uint32_t seed=0;
    if(!header || !U32(static_cast<std::uint64_t>(id.address)+(id.domain==1?0x14:0x3EC),seed)) {
        Emit(static_cast<std::uint32_t>(kind),false,&id,id.address,0,Reason::ReadFailed,3);return;
    }
    if(!Next(g_birth))return;
    id.birth=g_birth;id.born=true;id.candidate=false;
    if(kind==Hook::Spawn || kind==Hook::PlayerCreate || kind==Hook::ManagerRestore ||
        kind==Hook::GridSpawn || kind==Hook::Door)id.init=2;
    id.type=values[0];id.variant=values[1];id.subtype=values[2];id.seed=seed;
    if(id.lifecycle<4)id.lifecycle=id.published?2u:3u;
    Emit(static_cast<std::uint32_t>(kind),false,&id,id.address,0,Reason::None,1);
}
void ObserveEnter(const Frame& frame) noexcept
{
    const auto hook=static_cast<Hook>(frame.hook);
    // Replace is a PlayerManager method; its two stack arguments are Players.
    // Never introduce the manager into the Entity identity table.
    const auto object=hook==Hook::Replace?frame.args[0]:frame.self;
    auto* id=Find((hook==Hook::GridInit)?1u:0u,object,false);
    if(hook==Hook::PlayerCtor) {id=Find(0,frame.self,true);if(id)Acquire(*id,hook,true);}
    if(hook==Hook::NpcMorph || hook==Hook::PickupMorph || hook==Hook::PlayerRestore ||
       hook==Hook::ManagerRestore || hook==Hook::Replace) {
        if(id) {Next(id->revision);if(hook==Hook::PlayerRestore)id->born=false;}
        if(hook==Hook::Replace) {
            auto* target=Find(0,frame.args[1],false);
            if(target && target!=id)Next(target->revision);
        }
        if(hook==Hook::PlayerRestore || hook==Hook::ManagerRestore || hook==Hook::Replace) {
            Next(g_mutation);g_token.dirty=true;
        }
    }
    Emit(frame.hook,true,id,object,hook==Hook::Replace?frame.args[1]:0);
}
void ObserveReturn(const Frame& frame,std::uint32_t result) noexcept
{
    const auto hook=static_cast<Hook>(frame.hook);
    Identity* id=nullptr;
    if(hook==Hook::Factory || hook==Hook::GridFactory) {
        id=Find(hook==Hook::GridFactory?1u:0u,result,result!=0);
        if(id)Acquire(*id,hook,hook==Hook::GridFactory);
    } else if(hook==Hook::Spawn || hook==Hook::PlayerCreate) {
        id=Find(0,result,result!=0);if(id)Confirm(*id,hook);
    } else if(hook==Hook::GridSpawn || hook==Hook::Door) {
        std::uint32_t object=0;
        const auto limit=hook==Hook::Door?8u:448u;
        if(frame.args[0]<limit && U32(static_cast<std::uint64_t>(frame.self)+(hook==Hook::Door?0x724:0x24)+frame.args[0]*4,object)) {
            id=Find(1,object,object!=0);
            if(id && (hook==Hook::Door || (result&0xFF))) {id->published=true;Confirm(*id,hook);}
        }
    } else if(hook==Hook::ManagerRestore) {
        for(auto& retained:g_identities)if(retained.address && retained.candidate &&
            retained.owner_scope==frame.stack)Confirm(retained,hook);
    } else if(hook==Hook::Replace) {
        // A replacement is not a construction or destruction. Both identities
        // keep their existing birth/use; membership comes from actual sources.
        id=Find(0,frame.args[0],false);
    } else if(hook==Hook::GridFree) {
        // Slot replacement can retain the same pointer. No physical retirement
        // is inferred from a free request without a confirmed slot change.
    } else {
        const auto object=hook==Hook::Publish?frame.args[0]:frame.self;
        id=Find(hook==Hook::GridInit?1u:0u,object,object!=0);
        if(id) {
            if(hook==Hook::Init || hook==Hook::GridInit) {
                if(!id->candidate && !HasContext(Hook::NpcMorph) && !HasContext(Hook::PickupMorph) &&
                    !HasContext(Hook::PlayerCreate) && !HasContext(Hook::ManagerRestore)) {
                    id->born=false;id->origin=1;Next(id->revision);
                    Emit(frame.hook,false,id,object,0,Reason::Continuity,3);
                }
                id->init=1;
            }
            if(hook==Hook::Publish) {
                id->published=true;if(id->lifecycle<4)id->lifecycle=2;
                if(!HasContext(Hook::Spawn) && !HasContext(Hook::PlayerCreate))Confirm(*id,hook);
            }
            if((hook==Hook::NpcMorph || hook==Hook::PickupMorph) && frame.args[6]>=2)id->init=2;
            if(hook==Hook::Remove)id->lifecycle=4;
            if(hook==Hook::ClearRefs) {Next(id->revision);}
            if(hook==Hook::Destroy) {id->destroyed=true;id->published=false;id->lifecycle=6;}
            if(hook==Hook::PlayerCtor && !HasContext(Hook::PlayerCreate) && !HasContext(Hook::ManagerRestore))
                id->lifecycle=3;
        }
    }
    if((hook==Hook::GridSpawn || hook==Hook::Door || hook==Hook::GridFree) && frame.args[7]) {
        std::uint32_t current=0;
        const auto limit=hook==Hook::Door?8u:448u;
        if(frame.args[0]<limit && U32(static_cast<std::uint64_t>(frame.self)+(hook==Hook::Door?0x724:0x24)+frame.args[0]*4,current) && current!=frame.args[7]) {
            auto* old=Find(1,frame.args[7]);
            if(old){old->destroyed=true;old->published=false;old->lifecycle=6;
                Emit(frame.hook,false,old,old->address,current,Reason::None,4);}
        }
    }
    Emit(frame.hook,false,id,id?id->address:(hook==Hook::Replace?frame.args[0]:frame.self),result,
        ((hook==Hook::Factory || hook==Hook::GridFactory || hook==Hook::Spawn || hook==Hook::PlayerCreate) && !result)?
        Reason::NotReady:Reason::None);
}
// PUSHAD layout followed by saved EFLAGS, then the native return address.
// Fixed-signature forwarding bridges keep the native caller's return address
// and argument slots intact. The original runs once on a shadow argument stack
// with identical modulo-16 entry alignment. TLS is observation metadata only;
// losing it cannot redirect control flow or alter a native return value.
extern "C" std::uint32_t __cdecl WorldEnter(std::uint32_t hook,std::uint32_t* registers) noexcept;
extern "C" void __cdecl WorldReturn(std::uint32_t hook,std::uint32_t* registers,std::uint32_t cookie) noexcept;

extern "C" __declspec(naked) void WorldEntry0()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 0
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        mov eax,[esp+708]
        mov [esp+4],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+0*4]
        lea esp,[esp-8]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 0
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 8
    }
}

extern "C" __declspec(naked) void WorldEntry1()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 1
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        mov eax,[esp+708]
        mov [esp+4],eax
        mov eax,[esp+712]
        mov [esp+8],eax
        mov eax,[esp+716]
        mov [esp+12],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+1*4]
        lea esp,[esp-16]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 1
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 16
    }
}

extern "C" __declspec(naked) void WorldEntry2()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 2
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+2*4]
        lea esp,[esp-4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 2
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 4
    }
}

extern "C" __declspec(naked) void WorldEntry3()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 3
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+3*4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 3
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 0
    }
}

extern "C" __declspec(naked) void WorldEntry4()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 4
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+4*4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 4
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 0
    }
}

extern "C" __declspec(naked) void WorldEntry5()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 5
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+5*4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 5
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 0
    }
}

extern "C" __declspec(naked) void WorldEntry6()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 6
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+6*4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 6
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 0
    }
}

extern "C" __declspec(naked) void WorldEntry7()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 7
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+7*4]
        lea esp,[esp-4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 7
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 4
    }
}

extern "C" __declspec(naked) void WorldEntry8()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 8
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+8*4]
        lea esp,[esp-4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 8
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 4
    }
}

extern "C" __declspec(naked) void WorldEntry9()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 9
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        mov eax,[esp+708]
        mov [esp+4],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+9*4]
        lea esp,[esp-8]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 9
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 8
    }
}

extern "C" __declspec(naked) void WorldEntry10()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 10
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+10*4]
        lea esp,[esp-4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 10
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 4
    }
}

extern "C" __declspec(naked) void WorldEntry11()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 11
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        mov eax,[esp+708]
        mov [esp+4],eax
        mov eax,[esp+712]
        mov [esp+8],eax
        mov eax,[esp+716]
        mov [esp+12],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+11*4]
        lea esp,[esp-16]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 11
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 16
    }
}

extern "C" __declspec(naked) void WorldEntry12()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 12
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        mov eax,[esp+708]
        mov [esp+4],eax
        mov eax,[esp+712]
        mov [esp+8],eax
        mov eax,[esp+716]
        mov [esp+12],eax
        mov eax,[esp+720]
        mov [esp+16],eax
        mov eax,[esp+724]
        mov [esp+20],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+12*4]
        lea esp,[esp-24]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 12
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 24
    }
}

extern "C" __declspec(naked) void WorldEntry13()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 13
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+13*4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 13
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 0
    }
}

extern "C" __declspec(naked) void WorldEntry14()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 14
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+14*4]
        lea esp,[esp-4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 14
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 4
    }
}

extern "C" __declspec(naked) void WorldEntry15()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 15
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        mov eax,[esp+708]
        mov [esp+4],eax
        mov eax,[esp+712]
        mov [esp+8],eax
        mov eax,[esp+716]
        mov [esp+12],eax
        mov eax,[esp+720]
        mov [esp+16],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+15*4]
        lea esp,[esp-20]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 15
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 20
    }
}

extern "C" __declspec(naked) void WorldEntry16()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 16
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+16*4]
        lea esp,[esp-4]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 16
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 4
    }
}

extern "C" __declspec(naked) void WorldEntry17()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 17
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        mov eax,[esp+708]
        mov [esp+4],eax
        mov eax,[esp+712]
        mov [esp+8],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+17*4]
        lea esp,[esp-12]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 17
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 12
    }
}

extern "C" __declspec(naked) void WorldEntry18()
{
    __asm {
        pushfd
        lock inc dword ptr [g_world_active]
        pushad
        sub esp,664
        lea eax,[esp+63]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+576],eax
        lea eax,[esp+664]
        push eax
        push 18
        call WorldEnter
        add esp,8
        mov [esp+580],eax
        mov eax,[esp+576]
        fxrstor [eax]
        mov eax,[esp+704]
        mov [esp+0],eax
        mov eax,[esp+708]
        mov [esp+4],eax
        mov eax,[esp+712]
        mov [esp+8],eax
        mov eax,[esp+716]
        mov [esp+12],eax
        mov eax,[esp+720]
        mov [esp+16],eax
        mov eax,[esp+724]
        mov [esp+20],eax
        mov eax,[esp+728]
        mov [esp+24],eax
        mov eax,[esp+732]
        mov [esp+28],eax
        push dword ptr [esp+696]
        popfd
        mov edi,[esp+664]
        mov esi,[esp+668]
        mov ebp,[esp+672]
        mov ebx,[esp+680]
        mov edx,[esp+684]
        mov ecx,[esp+688]
        mov eax,[esp+692]
        call dword ptr [g_world_calls+18*4]
        lea esp,[esp-32]
        pushfd
        pushad
        lea eax,[esp+99]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+612],eax
        mov edx,esp
        push dword ptr [esp+616]
        push edx
        push 18
        call WorldReturn
        add esp,12
        mov eax,[esp+612]
        fxrstor [eax]
        lock dec dword ptr [g_world_active]
        popad
        popfd
        lea esp,[esp+700]
        ret 32
    }
}

const std::array<void*,19> g_wrappers{reinterpret_cast<void*>(WorldEntry0),reinterpret_cast<void*>(WorldEntry1),reinterpret_cast<void*>(WorldEntry2),reinterpret_cast<void*>(WorldEntry3),reinterpret_cast<void*>(WorldEntry4),reinterpret_cast<void*>(WorldEntry5),reinterpret_cast<void*>(WorldEntry6),reinterpret_cast<void*>(WorldEntry7),reinterpret_cast<void*>(WorldEntry8),reinterpret_cast<void*>(WorldEntry9),reinterpret_cast<void*>(WorldEntry10),reinterpret_cast<void*>(WorldEntry11),reinterpret_cast<void*>(WorldEntry12),reinterpret_cast<void*>(WorldEntry13),reinterpret_cast<void*>(WorldEntry14),reinterpret_cast<void*>(WorldEntry15),reinterpret_cast<void*>(WorldEntry16),reinterpret_cast<void*>(WorldEntry17),reinterpret_cast<void*>(WorldEntry18)};

extern "C" std::uint32_t __cdecl WorldEnter(std::uint32_t hook,std::uint32_t* registers) noexcept
{
    if(!Enabled() || !MainThread())return 0;
    e2e::Context(g_token.value,g_token.tick,g_attempt);
    if(g_depth==g_frames.size()){Error(Reason::Continuity);return 0;}
    Frame frame{};frame.hook=hook;frame.self=registers[6];frame.stack=Address(registers+9);
    frame.return_address=registers[9];
    if(kArgumentWords[hook] && !Read(static_cast<std::uint64_t>(frame.stack)+4,frame.args.data(),kArgumentWords[hook]*4)) {
        Error(Reason::ReadFailed);return 0;
    }
    if(hook==13)frame.args[0]=registers[5];
    if(hook==11 || hook==12) {
        const auto* id=Find(0,frame.self);frame.args[6]=id?id->init:0;
    }
    if(hook==15 || hook==16 || hook==17) {
        const auto limit=hook==16?8u:448u;
        if(frame.args[0]<limit)U32(static_cast<std::uint64_t>(frame.self)+(hook==16?0x724:0x24)+frame.args[0]*4,frame.args[7]);
    }
    if(g_shared->enters[hook]==UINT64_MAX){Error(Reason::SequenceExhausted);return 0;}
    ObserveEnter(frame);g_frames[g_depth++]=frame;
#if defined(ISAAC_CS_WORLD_E2E)
    auto& audit=g_audit[g_depth-1];audit={};
    std::memcpy(audit.entry.data(),registers,9*4);
    const auto* fx=*reinterpret_cast<const std::uint8_t* const*>(reinterpret_cast<const std::uint8_t*>(registers)-88);
    std::memcpy(audit.fx.data(),fx,512);
    audit.first=!g_audit_sampled[hook];g_audit_sampled[hook]=true;
    e2e::Record(e2e::Kind::Entry,hook,registers,9);
    e2e::Record(e2e::Kind::Arguments,hook,frame.args.data(),kArgumentWords[hook],0);
    if(audit.first)e2e::Fx(hook,0,fx);
#endif
    InterlockedIncrement(&g_shared->callbacks);++g_shared->enters[hook];
    return g_depth;
}
extern "C" void __cdecl WorldReturn(std::uint32_t hook,std::uint32_t* registers,std::uint32_t cookie) noexcept
{
    if(!cookie)return;
    if(cookie>g_depth || g_frames[cookie-1].hook!=hook ||
        g_frames[cookie-1].stack!=Address(registers)+736) {
        Error(Reason::UnpairedScope);return; // Native return is independent of this record.
    }
    if(cookie!=g_depth)Error(Reason::Continuity);
    const auto frame=g_frames[cookie-1];
#if defined(ISAAC_CS_WORLD_E2E)
    const auto audit=g_audit[cookie-1];
    std::array<std::uint32_t,9> saved{};std::memcpy(saved.data(),registers,9*4);
    auto* fx=*reinterpret_cast<std::uint8_t**>(reinterpret_cast<std::uint8_t*>(registers)+612);
    std::array<std::uint8_t,512> saved_fx{};std::memcpy(saved_fx.data(),fx,512);
#endif
    if(Enabled())ObserveReturn(frame,registers[7]);
#if defined(ISAAC_CS_WORLD_E2E)
    std::uint32_t errors=audit.originals==1?0u:1u;
    if(std::memcmp(saved.data(),registers,9*4) || !SameFx(saved_fx.data(),fx))errors|=2;
    std::uint32_t callee_changes=0;
    for(const auto i:{0u,1u,2u,4u})if(registers[i]!=audit.entry[i])callee_changes|=1u<<i;
    std::uint32_t words[14]{};std::memcpy(words,registers,9*4);
    words[9]=errors;words[10]=audit.originals;words[11]=frame.stack;words[12]=Address(registers)+736;
    // Custom userpurge entries may have register outputs. Preserve and expose
    // actual original outputs; never rewrite them to an assumed input value.
    words[13]=(e2e::Copying()?1u:0u)|(callee_changes<<8);
    e2e::Record(e2e::Kind::Returned,hook,words,14);
    if(audit.first)e2e::Fx(hook,2,fx);
#endif
    if(g_shared->returns[hook]==UINT64_MAX)Error(Reason::SequenceExhausted);
    else ++g_shared->returns[hook];
    InterlockedExchangeAdd(&g_shared->callbacks,-static_cast<LONG>(g_depth-cookie+1));
    g_depth=cookie-1;
}

struct Spec {std::uint16_t id;std::uint16_t offset;std::uint8_t width;Encoding encoding;};
const Spec kCommon[]{
    {0x1,0x20,4,Encoding::Raw},{0x2,0x28,4,Encoding::Raw},{0x3,0x2C,4,Encoding::Raw},{0x4,0x30,4,Encoding::Raw},
    {0x5,0x168,8,Encoding::Raw},{0x6,0x170,1,Encoding::Raw},{0x7,0x171,1,Encoding::Raw},
    {0x8,0x172,1,Encoding::Raw},{0x9,0x173,1,Encoding::Raw},{0xA,0x177,1,Encoding::Raw},
    {0xB,0x33C,4,Encoding::F32},{0xC,0x340,4,Encoding::F32},{0xD,0x360,4,Encoding::F32},
    {0xE,0x364,4,Encoding::F32},{0xF,0x380,4,Encoding::F32},{0x10,0x384,4,Encoding::F32},
    {0x11,0x3EC,4,Encoding::Raw},{0x12,0x3DC,4,Encoding::Raw},{0x13,0x3E0,4,Encoding::Raw},
    {0x14,0x3E4,4,Encoding::Raw},{0x15,0x3E8,4,Encoding::Raw},
    {0x16,0x3BC,4,Encoding::Reference},{0x17,0x3C0,4,Encoding::Reference},
    {0x18,0x3C4,4,Encoding::Reference},{0x19,0x3C8,4,Encoding::Reference},
    {0x1A,0x34,4,Encoding::Raw},{0x1B,0x38,4,Encoding::Raw},{0x1C,0x328,4,Encoding::Raw}};
const Spec kPlayer[]{
    {0x101,0x13C0,4,Encoding::Raw},{0x102,0x161C,4,Encoding::Raw},{0x103,0x13BC,4,Encoding::Raw},
    {0x104,0x1340,4,Encoding::Raw},{0x105,0x1344,4,Encoding::Raw},{0x106,0x1348,4,Encoding::Raw},
    {0x107,0x134C,4,Encoding::Raw},{0x108,0x1350,4,Encoding::Raw},{0x109,0x1D88,4,Encoding::Raw},
    {0x10A,0x194C,4,Encoding::Raw},{0x10B,0x1DA0,4,Encoding::Raw},{0x10C,0x1DA4,4,Encoding::Raw},
    {0x10D,0x1D98,4,Encoding::Reference},{0x10E,0x1E68,4,Encoding::Reference},
    {0x10F,0x1E6C,4,Encoding::Reference},{0x110,0x26C8,4,Encoding::Reference}};
const Spec kNpc[]{{0x201,0x410,4,Encoding::Raw},{0x202,0xB64,4,Encoding::Raw},
    {0x203,0xBA0,4,Encoding::Raw},{0x204,0xBA4,4,Encoding::Raw}};
const Spec kFamiliar[]{{0x301,0x410,4,Encoding::Reference},{0x302,0xD4C,4,Encoding::Raw},{0x303,0xD88,4,Encoding::Raw}};
const Spec kEffect[]{{0x401,0x450,4,Encoding::Raw},{0x402,0x45C,4,Encoding::Raw},{0x403,0x460,4,Encoding::Raw}};
const Spec kPickup[]{{0x501,0x540,4,Encoding::Raw},{0x502,0x544,4,Encoding::Raw},{0x503,0x584,4,Encoding::Raw}};
const Spec kTear[]{{0x601,0x428,16,Encoding::Raw}};
const Spec kProjectile[]{{0x701,0x438,8,Encoding::Raw}};
const Spec kGrid[]{
    {0x801,0x4,4,Encoding::Raw},{0x802,0x8,4,Encoding::Raw},{0x803,0xC,4,Encoding::Raw},
    {0x804,0x10,4,Encoding::Raw},{0x805,0x14,4,Encoding::Raw},{0x806,0x18,4,Encoding::Raw},
    {0x807,0x1C,1,Encoding::Raw},{0x808,0x20,4,Encoding::Raw},{0x809,0x24,4,Encoding::Raw},
    {0x80A,0x28,4,Encoding::Raw},{0x80B,0x2C,4,Encoding::Raw},{0x80C,0x30,4,Encoding::Raw},
    {0x80D,0x34,4,Encoding::Raw},{0x80E,0x38,4,Encoding::Raw},{0x80F,0x3C,4,Encoding::Raw}};
void Incomplete(Batch& batch,Reason reason) noexcept
{batch.complete=0;if(!batch.reason)batch.reason=static_cast<std::uint32_t>(reason);}
Field* Append(Object& object,std::uint16_t id,Encoding encoding,std::uint32_t length) noexcept
{
    if(object.field_count>=kFieldBudget)return nullptr;
    auto& field=object.fields[object.field_count++];field={};
    field.id=id;field.encoding=encoding;field.length=length;return &field;
}
void Fields(Batch& batch,Object& object,const Spec* specs,std::size_t count,bool ready=true) noexcept
{
    for(std::size_t i=0;i<count;++i) {
        const auto& spec=specs[i];auto* field=Append(object,spec.id,spec.encoding,spec.width);
        if(!field){Incomplete(batch,Reason::Budget);return;}
        if(!ready){field->status=FieldStatus::NotReady;Incomplete(batch,Reason::NotReady);continue;}
        const bool own_read_fault=e2e::Take(e2e::Fault::FieldRead);
        const auto read_address=own_read_fault?e2e::ReadFaultAddress():static_cast<std::uint64_t>(object.address)+spec.offset;
        // E2E PAGE_NOACCESS is collector-owned. Exercise the real guarded Read
        // exception path without corrupting or manufacturing a game pointer.
        if(!Read(read_address,field->raw.data(),spec.width)) {
            field->status=FieldStatus::ReadFailed;Incomplete(batch,Reason::ReadFailed);Error(Reason::ReadFailed);continue;
        }
        if(spec.encoding==Encoding::F32) {
            std::uint32_t bits=0;std::memcpy(&bits,field->raw.data(),4);
            if((bits&0x7F800000u)==0x7F800000u)Incomplete(batch,Reason::Nonfinite);
        }
    }
}
void RawDerived(Object& object,std::uint16_t id,std::uint32_t bits,Encoding encoding) noexcept
{auto* f=Append(object,id,encoding,4);if(f)std::memcpy(f->raw.data(),&bits,4);}
void CopyObject(Batch& batch,Object& object,Identity& identity,std::uint32_t game_frame) noexcept
{
    object.id=identity.id;object.use=identity.use;object.birth=identity.birth;object.revision=identity.revision;
    object.domain=identity.domain;object.address=identity.address;object.origin=identity.origin;
    object.lifecycle=identity.lifecycle;object.init=identity.init;object.ordinal=identity.ordinal;
    object.birth_kind=identity.birth_kind;object.birth_type=identity.type;object.birth_variant=identity.variant;
    object.birth_subtype=identity.subtype;object.birth_seed=identity.seed;object.birth_known=identity.born?1u:0u;
    object.field_count=0;object.kind=static_cast<std::uint32_t>(Class::Unknown);object.vptr=0;object.reserved=UINT32_MAX;
    if(!U32(object.address,object.vptr)){Incomplete(batch,Reason::ReadFailed);Error(Reason::ReadFailed);return;}
    const auto kind=Classify(identity.domain,object.vptr);object.kind=static_cast<std::uint32_t>(kind);
    if(kind==Class::Unknown) {
        Incomplete(batch,Reason::UnknownClass);
        if(identity.domain==0 && HasBase(object.vptr,0x85D5DC))Fields(batch,object,kCommon,4);
        return;
    }
    if(kind==Class::Grid) {Fields(batch,object,kGrid,std::size(kGrid));return;}
    Fields(batch,object,kCommon,std::size(kCommon));
    RawDerived(object,0x1D,game_frame,Encoding::Raw);
    std::uint32_t birth_counter=0;
    if(U32(static_cast<std::uint64_t>(object.address)+0x328,birth_counter))RawDerived(object,0x1E,game_frame-birth_counter,Encoding::Derived);
    else {auto* f=Append(object,0x1E,Encoding::Derived,4);if(f)f->status=FieldStatus::ReadFailed;Incomplete(batch,Reason::ReadFailed);}
    const bool ready=identity.init>=2;
    switch(kind) {
    case Class::Player:Fields(batch,object,kPlayer,std::size(kPlayer),ready);break;
    case Class::NPC:Fields(batch,object,kNpc,std::size(kNpc),ready);break;
    case Class::Tear:Fields(batch,object,kTear,std::size(kTear),ready);break;
    case Class::Projectile:Fields(batch,object,kProjectile,std::size(kProjectile),ready);break;
    case Class::Pickup:Fields(batch,object,kPickup,std::size(kPickup),ready);break;
    case Class::Familiar:Fields(batch,object,kFamiliar,std::size(kFamiliar),ready);break;
    case Class::Effect:Fields(batch,object,kEffect,std::size(kEffect),ready);break;
    default:break;
    }
}
std::uint32_t Add(Batch& batch,std::uint32_t domain,std::uint32_t address,std::uint32_t source,
                  std::uint32_t index,std::uint32_t game_frame) noexcept
{
    if(!address)return UINT32_MAX;
    ++batch.required_memberships;
    auto* identity=Find(domain,address,true);
    if(!identity){Incomplete(batch,Reason::Budget);return UINT32_MAX;}
    if(!identity->origin)identity->origin=1;
    const auto slot=static_cast<std::size_t>(identity-g_identities.data());
    g_current_memberships[slot]|=1u<<source;
    auto object_index=g_batch_indices[slot];
    if(object_index==UINT32_MAX) {
        ++batch.required_objects;
        if(batch.object_count==kObjectBudget || e2e::Take(e2e::Fault::ObjectBudget)){g_batch_indices[slot]=UINT32_MAX-1;Incomplete(batch,Reason::Budget);return UINT32_MAX;}
        object_index=batch.object_count++;g_batch_indices[slot]=object_index;
#if defined(ISAAC_CS_WORLD_E2E)
        auto& rng=g_rng_audit[object_index];rng.address=address;rng.domain=domain;
        std::uint32_t audit_vptr=0;
        rng.eligible=U32(address,audit_vptr) && (domain==1?HasBase(audit_vptr,0x85D7A4):HasBase(audit_vptr,0x85D5DC));
        rng.valid=rng.eligible && Read(static_cast<std::uint64_t>(address)+(domain==1?0x2Cu:0x3DCu),rng.before.data(),16);
#endif
        CopyObject(batch,batch.objects[object_index],*identity,game_frame);
    }
    if(object_index==UINT32_MAX-1)return UINT32_MAX;
    if(batch.membership_count==kMembershipBudget || e2e::Take(e2e::Fault::MembershipBudget)){Incomplete(batch,Reason::Budget);return object_index;}
    batch.memberships[batch.membership_count++]={object_index,source,index,0};return object_index;
}
void CopySource(Batch& batch,std::uint32_t source,std::uint32_t descriptor,
                bool vector,std::uint32_t game_frame) noexcept
{
    auto& out=batch.sources[source];out={};
    const std::uint32_t bytes=vector?12u:16u;
    if(!Read(descriptor,out.descriptor,bytes)){out.status=static_cast<std::uint32_t>(Reason::ReadFailed);Incomplete(batch,Reason::ReadFailed);return;}
    std::uint32_t data=0,count=0;
    if(vector) {
        const auto first=out.descriptor[0],end=out.descriptor[1],cap=out.descriptor[2];
        if(first || end || cap) {
            if(!first || first>end || end>cap || (first|end|cap)&3u) {out.status=static_cast<std::uint32_t>(Reason::ReadFailed);Incomplete(batch,Reason::ReadFailed);return;}
            data=first;count=(end-first)/4;
        }
    } else {
        data=out.descriptor[1];count=out.descriptor[3];
        if(count>out.descriptor[2] || (count && !data) || (data&3u)) {
            out.status=static_cast<std::uint32_t>(Reason::ReadFailed);Incomplete(batch,Reason::ReadFailed);return;
        }
    }
    out.count=count;
    if(count>kMembershipBudget || count>UINT32_MAX/4 || (count && data>UINT32_MAX-count*4)) {
        out.status=static_cast<std::uint32_t>(Reason::Budget);Incomplete(batch,Reason::Budget);return;
    }
    for(std::uint32_t i=0;i<count;++i) {
        std::uint32_t object=0;
        if(!U32(static_cast<std::uint64_t>(data)+i*4,object)){out.status=static_cast<std::uint32_t>(Reason::ReadFailed);Incomplete(batch,Reason::ReadFailed);break;}
        if(!object){out.status=static_cast<std::uint32_t>(Reason::NotReady);Incomplete(batch,Reason::NotReady);continue;}
        g_source_addresses[i]=object;
        Add(batch,0,object,source,i,game_frame);
    }
    if(!out.status)for(std::uint32_t i=0;i<count;++i) {
        std::uint32_t after_pointer=0;
        if(!U32(static_cast<std::uint64_t>(data)+i*4,after_pointer) || after_pointer!=g_source_addresses[i]) {
            out.status=static_cast<std::uint32_t>(Reason::SourceChanged);
            Incomplete(batch,Reason::SourceChanged);break;
        }
    }
    std::uint32_t after[4]{};
    if(e2e::Take(e2e::Fault::SourceChanged) || !Read(descriptor,after,bytes) || std::memcmp(out.descriptor,after,bytes)) {
        out.status=static_cast<std::uint32_t>(Reason::SourceChanged);Incomplete(batch,Reason::SourceChanged);
    }
    if(out.status==static_cast<std::uint32_t>(Reason::SourceChanged))Error(Reason::SourceChanged);
    else if(out.status==static_cast<std::uint32_t>(Reason::ReadFailed))Error(Reason::ReadFailed);
}
void ResolveReferences(Batch& batch) noexcept
{
    for(std::uint32_t i=0;i<batch.object_count;++i) {
        auto& object=batch.objects[i];
        for(std::uint32_t j=0;j<object.field_count;++j) {
            auto& field=object.fields[j];
            if(field.encoding!=Encoding::Reference || field.status!=FieldStatus::Value)continue;
            std::uint32_t address=0;std::memcpy(&address,field.raw.data(),4);
            if(!address){field.relation=static_cast<std::uint32_t>(Relation::Null);continue;}
            auto* target=Find(0,address);
            auto state=Relation::Unresolved;
            if(target) {
                if(target->destroyed || target->lifecycle>=5)state=Relation::Retired;
                else if(target->reused)state=Relation::AmbiguousReuse;
                else if(object.birth_known && object.init && !object.revision && target->born && target->birth<object.birth &&
                    g_batch_indices[static_cast<std::size_t>(target-g_identities.data())]!=UINT32_MAX) {
                    state=Relation::KnownCurrent;field.target_id=target->id;field.target_use=target->use;
                }
            }
            const bool forced_ambiguity=state==Relation::KnownCurrent && e2e::Take(e2e::Fault::ReferenceAmbiguity);
            if(forced_ambiguity){
                const std::uint32_t words[]{field.id,address,static_cast<std::uint32_t>(state),
                    static_cast<std::uint32_t>(Relation::AmbiguousReuse),
                    static_cast<std::uint32_t>(field.target_id),static_cast<std::uint32_t>(field.target_id>>32),
                    static_cast<std::uint32_t>(field.target_use),static_cast<std::uint32_t>(field.target_use>>32),
                    object.birth_known,object.init,static_cast<std::uint32_t>(object.revision),static_cast<std::uint32_t>(object.revision>>32)};
                e2e::Record(e2e::Kind::ReferenceFault,0,words,12,object.id,object.use);
                state=Relation::AmbiguousReuse;field.target_id=0;field.target_use=0;
            }
            field.relation=static_cast<std::uint32_t>(state);
            if(state!=Relation::KnownCurrent)Incomplete(batch,Reason::Continuity);
        }
    }
}
#if defined(ISAAC_CS_WORLD_E2E)
struct CopyAudit {
    std::array<std::uint8_t,2504> before{};
    bool valid=false;
    static bool Snapshot(std::array<std::uint8_t,2504>& bytes) noexcept {
        // tx-01-71 saved instructions, not a producer/getter invocation:
        // MT array C7AC70/9C0; index C34F04 (6EF356: 8B 1D 04 4F C3 00);
        // rand bypass flag C7AC68. Preserve literal bytes; no digest.
        return Read(Address(g_image)+0x87AC70,bytes.data(),2496) &&
               Read(Address(g_image)+0x834F04,bytes.data()+2496,4) &&
               Read(Address(g_image)+0x87AC68,bytes.data()+2500,1);
    }
    static void Raw(std::uint32_t phase,const std::array<std::uint8_t,2504>& bytes) noexcept {
        for(std::uint32_t offset=0;offset<2504;offset+=48) {
            std::uint32_t words[14]{offset,(std::min)(48u,2504u-offset)};
            std::memcpy(words+2,bytes.data()+offset,words[1]);
            e2e::Record(e2e::Kind::GlobalRng,phase,words,14);
        }
    }
    CopyAudit() noexcept {valid=Snapshot(before);Raw(0,before);}
    ~CopyAudit() noexcept {
        std::array<std::uint8_t,2504> after{};const bool readable=Snapshot(after);
        Raw(1,after);
        const std::uint32_t words[]{valid?1u:0u,readable?1u:0u,
            !std::memcmp(before.data(),after.data(),2504)?1u:0u,before[2500],after[2500]};
        e2e::Record(e2e::Kind::GlobalRng,2,words,5);e2e::EndCopy();
    }
};
#else
struct CopyAudit {~CopyAudit(){e2e::EndCopy();}};
#endif
void FinishBatch(Batch& batch) noexcept
{
    if(batch.authority==1) {
        g_post_state={batch.complete?coop_active::PostStateStatus::Complete:coop_active::PostStateStatus::Gap,
            batch.reason,batch.tick,batch.attempt,batch.capture};
        g_pending_batch.store(&batch,std::memory_order_release);
        return; // Keep WRITING until the Authority completed tick is committed.
    }
    MemoryBarrier();InterlockedExchange(&batch.state,2);Notify();
}
void Capture(Reason initial) noexcept
{
    g_post_run=g_run;g_post_room=g_room;g_post_token=g_token.value;
    g_post_state={coop_active::PostStateStatus::Unavailable,static_cast<std::uint32_t>(Reason::Stopped),g_token.tick,0,0};
    if(!Next(g_attempt))return;
    g_post_state.attempt=g_attempt;
    InterlockedExchange64(&g_shared->attempts,static_cast<LONG64>(g_attempt));
#if defined(ISAAC_CS_WORLD_E2E)
    e2e::Context(g_token.value,g_token.tick,g_attempt);
    const auto window=loop::CurrentObservationContext();
    std::uint32_t facts[14]{static_cast<std::uint32_t>(window.loop_iteration),static_cast<std::uint32_t>(window.loop_iteration>>32)};
    coop_active::CopyWorldE2EWindowFacts(facts+2);
    facts[8]=g_lifecycle_depth;facts[9]=g_retired?1u:0u;facts[10]=g_token.dirty?1u:0u;
    facts[11]=g_token.owner_enter;facts[12]=g_token.room_enter;facts[13]=g_token.tail_enter;
    e2e::Record(e2e::Kind::Window,static_cast<std::uint32_t>(initial),facts,14);
    if(initial==Reason::None && e2e::Pending(e2e::Fault::EventOverflow)) {
        // Explicit collector-owned diagnostic delivery, not a fabricated game
        // lifecycle event. Emit's actual capacity-failure branch consumes it.
        const std::uint32_t delivery[]{1,static_cast<std::uint32_t>(e2e::Fault::EventOverflow)};
        e2e::Record(e2e::Kind::DiagnosticDelivery,0,delivery,2);
        Emit(UINT32_MAX,false,nullptr,0,0,Reason::None,3);
        if(!Enabled())return;
    }
#endif
    Batch* selected=nullptr;
    for(auto& batch:g_shared->batches)if(InterlockedCompareExchange(&batch.state,1,0)==0){selected=&batch;break;}
    if(!selected){g_post_state.status=coop_active::PostStateStatus::Backpressure;
        g_post_state.reason=static_cast<std::uint32_t>(Reason::Backpressure);
        InterlockedIncrement64(&g_shared->backpressure);Emit(UINT32_MAX,false,nullptr,0,0,Reason::Backpressure,3);return;}
    auto& batch=*selected;batch.reason=static_cast<std::uint32_t>(initial);
    batch.run=g_run;batch.room=g_room;batch.mutation=g_mutation;batch.attempt=g_attempt;
    batch.capture=0;batch.outer=g_token.value;batch.tick=g_token.tick;batch.scope=g_token.scope;
    batch.authority=g_token.commits==1?1u:0u;batch.commit_kind=g_token.commit_kind;
    batch.thread=GetCurrentThreadId();batch.complete=initial==Reason::None?1u:0u;
    batch.object_count=0;batch.membership_count=0;batch.required_objects=0;batch.required_memberships=0;
    batch.sources.fill({});
    if(initial!=Reason::None) {FinishBatch(batch);return;}
    e2e::Context(g_token.value,g_token.tick,g_attempt);e2e::BeginCopy();
    CopyAudit copy_audit;
#if defined(ISAAC_CS_WORLD_E2E)
    if(e2e::Take(e2e::Fault::WrongThread)) {
        InterlockedIncrement(&g_fault_workers);
        const auto worker=CreateThread(nullptr,0,&WrongThreadFault,nullptr,0,nullptr);
        if(worker)CloseHandle(worker);else {InterlockedDecrement(&g_fault_workers);Error(Reason::WrongThread);}
    }
#endif
    g_batch_indices.fill(UINT32_MAX);g_current_memberships.fill(0);
    std::uint32_t root=0,room=0,game_frame=0,width=0;
    const auto root_slot=Address(g_image)+0x871678;
    if(!U32(root_slot,root) || !root || root>UINT32_MAX-0x26500 || !U32(static_cast<std::uint64_t>(root)+0x18300,room) || !room || room>UINT32_MAX-0x1400 ||
        !U32(static_cast<std::uint64_t>(root)+0x264F8,game_frame) || room!=g_token.room_address || !U32(static_cast<std::uint64_t>(room)+0xC,width)) {
        Incomplete(batch,Reason::ReadFailed);
    } else {
        for(std::uint32_t i=0;i<8;++i)CopySource(batch,i,room+0x1218+0x20+i*0x10,false,game_frame);
        CopySource(batch,8,root+0x1BAA8,true,game_frame);
        auto& esau=batch.sources[9];esau.count=4;
        for(std::uint32_t i=0;i<4;++i) {
            std::uint32_t value=0;
            if(!U32(static_cast<std::uint64_t>(root)+0x1BB28+i*4,value)){esau.status=static_cast<std::uint32_t>(Reason::ReadFailed);Incomplete(batch,Reason::ReadFailed);}
            else Add(batch,0,value,9,i,game_frame);
        }
        // Worklist walks copied, classified Player objects only; cyclic relations
        // are deduplicated by the batch identity map.
        std::uint32_t relations=0;
        for(std::uint32_t i=0;i<batch.object_count;++i) {
            const auto& player=batch.objects[i];if(player.kind!=static_cast<std::uint32_t>(Class::Player))continue;
            if(++relations>64 || e2e::Take(e2e::Fault::RelationBudget)){Incomplete(batch,Reason::RelationBudget);batch.sources[10].status=static_cast<std::uint32_t>(Reason::RelationBudget);break;}
            constexpr std::uint32_t offsets[]{0x1D98,0x1E68,0x1E6C,0x26C8};
            for(std::uint32_t j=0;j<4;++j) {
                std::uint32_t value=0;
                if(!U32(static_cast<std::uint64_t>(player.address)+offsets[j],value)){Incomplete(batch,Reason::ReadFailed);batch.sources[10].status=static_cast<std::uint32_t>(Reason::ReadFailed);}
                else if(value) {Add(batch,0,value,10,relations*4+j,game_frame);++batch.sources[10].count;}
            }
        }
        for(std::uint32_t source=11;source<13;++source) {
            const auto count=source==11?448u:8u,offset=source==11?0x24u:0x724u;
            batch.sources[source].count=count;
            for(std::uint32_t i=0;i<count;++i) {
                std::uint32_t value=0;
                if(!U32(static_cast<std::uint64_t>(room)+offset+i*4,value)){Incomplete(batch,Reason::ReadFailed);batch.sources[source].status=static_cast<std::uint32_t>(Reason::ReadFailed);continue;}
                const auto index=Add(batch,1,value,source,i,game_frame);
                if(source==11 && index!=UINT32_MAX && batch.objects[index].kind==static_cast<std::uint32_t>(Class::Grid)) {
                    auto& object=batch.objects[index];
                    if(object.reserved!=UINT32_MAX && object.reserved!=i) {
                        for(std::uint32_t f=0;f<object.field_count;++f)
                            if(object.fields[f].id==0x810 || object.fields[f].id==0x811)object.fields[f].status=FieldStatus::NotReady;
                        Incomplete(batch,Reason::Continuity);
                    } else if(object.reserved==UINT32_MAX && width && width<=448) {
                        object.reserved=i;
                        const float x=static_cast<float>(40*(i%width)+40),y=static_cast<float>(40*(i/width)+120);
                        std::uint32_t xb=0,yb=0;std::memcpy(&xb,&x,4);std::memcpy(&yb,&y,4);
                        RawDerived(object,0x810,xb,Encoding::F32);RawDerived(object,0x811,yb,Encoding::F32);
                    } else if(object.reserved==UINT32_MAX) {Incomplete(batch,Reason::NotReady);}
                }
            }
        }
        for(std::uint32_t i=0;i<batch.object_count;++i) {
            auto& object=batch.objects[i];
            if(object.kind==static_cast<std::uint32_t>(Class::Grid) && object.field_count==std::size(kGrid)) {
                auto* x=Append(object,0x810,Encoding::F32,4);if(x)x->status=FieldStatus::NotReady;
                auto* y=Append(object,0x811,Encoding::F32,4);if(y)y->status=FieldStatus::NotReady;
                Incomplete(batch,Reason::NotReady);
            }
        }
        // Reserved source ids are explicitly not applicable, never failed-as-empty.
        batch.sources[13].status=UINT32_MAX;batch.sources[14].status=UINT32_MAX;
        batch.sources[13].descriptor[0]=root;batch.sources[13].descriptor[1]=room;
        batch.sources[13].descriptor[2]=game_frame;batch.sources[13].descriptor[3]=width;
        ResolveReferences(batch);
        std::uint32_t after_root=0,after_room=0;
        const bool roots_stable=!e2e::Take(e2e::Fault::RootChanged) && U32(root_slot,after_root) && after_root==root &&
            U32(static_cast<std::uint64_t>(root)+0x18300,after_room) && after_room==room &&
            g_mutation==g_token.mutation && !g_lifecycle_depth;
        if(!roots_stable){Incomplete(batch,Reason::RootChanged);Error(Reason::RootChanged);}
        const bool source_complete=std::all_of(batch.sources.begin(),batch.sources.begin()+13,
            [](const auto& source){return source.status==0;});
        if(source_complete && roots_stable) {
            for(std::uint32_t i=0;i<g_identities_used;++i) {
                auto& identity=g_identities[i];const auto memberships=g_current_memberships[i];
                if((identity.membership_attempt || memberships) &&
                    (!identity.membership_attempt || identity.memberships!=memberships)) {
                    // This describes source views, not physical destruction.
                    Emit(UINT32_MAX,false,&identity,identity.address,memberships,Reason::None,7);
                }
                if(identity.membership_attempt || memberships) {
                    identity.memberships=memberships;identity.membership_attempt=g_attempt;
                }
            }
        }
        if(!Next(g_capture)){Incomplete(batch,Reason::SequenceExhausted);}
        else {batch.capture=g_capture;InterlockedExchange64(&g_shared->captures,static_cast<LONG64>(g_capture));}
    }
#if defined(ISAAC_CS_WORLD_E2E)
    for(std::uint32_t i=0;i<batch.object_count;++i) {
        const auto& rng=g_rng_audit[i];const auto& object=batch.objects[i];
        if(!rng.eligible)continue;
        std::uint32_t words[11]{rng.domain,rng.address};std::memcpy(words+2,rng.before.data(),16);
        const bool after=Read(static_cast<std::uint64_t>(rng.address)+(rng.domain==1?0x2Cu:0x3DCu),words+6,16);
        words[10]=(rng.valid?1u:0u)|(after?2u:0u);
        e2e::Record(e2e::Kind::Rng,(rng.valid && after && !std::memcmp(words+2,words+6,16))?0u:1u,
                    words,11,object.id,object.use);
    }
#endif
    FinishBatch(batch);
}
} // namespace

bool Prepare(const wchar_t* name,std::uint32_t pid,std::uint32_t controller,
             const std::array<std::uint8_t,32>& nonce) noexcept
{
    if(!name || !*name || g_shared)return false;
    if(!e2e::Prepare(name,pid,controller,nonce))return false;
    g_mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);if(!g_mapping)return false;
    g_shared=static_cast<Shared*>(MapViewOfFile(g_mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared)));
    if(!g_shared || g_shared->magic!=kMagic || g_shared->version!=2 || g_shared->size!=sizeof(Shared) ||
        g_shared->pid!=pid || pid!=GetCurrentProcessId() || g_shared->controller_pid!=controller ||
        g_shared->nonce!=nonce || g_shared->enabled || g_shared->closed || g_shared->produced || g_shared->consumed)return false;
    wchar_t ready_name[512]{};
    constexpr wchar_t suffix[]=L".Ready";
    std::size_t name_length=0;
    while(name[name_length] && name_length<std::size(ready_name)-std::size(suffix))++name_length;
    if(name[name_length])return false;
    std::memcpy(ready_name,name,name_length*sizeof(wchar_t));
    std::memcpy(ready_name+name_length,suffix,sizeof(suffix));
    g_ready=OpenEventW(EVENT_MODIFY_STATE,FALSE,ready_name);if(!g_ready)return false;
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&Prepare),&pinned))return false;
    g_image=reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    constexpr std::uint32_t rvas[19]{0x2F0090,0x2A93A0,0x18500,0x2ACB00,0x2ACB60,0x2A8850,
        0x353DA0,0x5B9CD0,0x5BCAA0,0x5BEE80,0x3AE4B0,0x2C0F30,0x2E30A0,
        0x30B2E0,0x309DB0,0x3EBCA0,0x3EEA90,0x41E980,0x2FE410};
    constexpr std::uint32_t lengths[19]{7,6,7,5,5,5,6,5,5,6,6,6,5,5,6,5,5,6,6};
    constexpr std::uint8_t bytes[19][7]{
        {0x55,0x8B,0xEC,0x51,0x8B,0x45,0x08},{0x55,0x8B,0xEC,0x83,0xEC,0x30},
        {0x55,0x8B,0xEC,0x53,0x8B,0x5D,0x08},{0x51,0x56,0x8B,0xF1,0x57},
        {0x55,0x8B,0xEC,0x51,0x53},{0x55,0x8B,0xEC,0x6A,0xFF},
        {0x53,0x8B,0xDC,0x83,0xEC,0x08},{0x55,0x8B,0xEC,0x6A,0xFF},
        {0x55,0x8B,0xEC,0x6A,0xFF},{0x53,0x8B,0xDC,0x83,0xEC,0x08},
        {0x53,0x8B,0xDC,0x83,0xEC,0x08},{0x55,0x8B,0xEC,0x83,0xE4,0xF8},
        {0x55,0x8B,0xEC,0x6A,0xFF},{0x55,0x8B,0xEC,0x6A,0xFF},
        {0x55,0x8B,0xEC,0x8B,0x45,0x08},{0x55,0x8B,0xEC,0x53,0x56},
        {0x55,0x8B,0xEC,0x6A,0xFF},{0x55,0x8B,0xEC,0x8B,0x55,0x08},
        {0x55,0x8B,0xEC,0x83,0xE4,0xF8}};
    for(std::size_t i=0;i<19;++i) {
        auto& entry=g_entries[i];entry.entry=g_image+rvas[i];entry.length=lengths[i];
        std::memcpy(entry.expected.data(),bytes[i],lengths[i]);entry.replacement=g_wrappers[i];
        entry.original_slot=&g_world_trampolines[i];entry.replacement_guard_length=8;
    }
    return true;
}
mode::EntryPatch* Entries() noexcept {return g_entries.data();}
const std::uint32_t* Ordinals() noexcept {return g_ordinals.data();}
std::size_t EntryCount() noexcept {return g_shared?19u:0u;}
void Arm() noexcept {if(g_shared && !g_shared->error)InterlockedExchange(&g_shared->enabled,1);}
void Close() noexcept {if(g_shared)InterlockedExchange(&g_shared->enabled,0);Notify();}
bool Drained() noexcept
{if(!g_shared)return true;if(InterlockedCompareExchange(&g_shared->callbacks,0,0) || InterlockedCompareExchange(&g_world_active,0,0))return false;
    if(g_pending_batch.load(std::memory_order_acquire))return false;
    #if defined(ISAAC_CS_WORLD_E2E)
    if(InterlockedCompareExchange(&g_fault_workers,0,0))return false;
    #endif
    MemoryBarrier();InterlockedExchange(&g_shared->closed,1);e2e::Close();Notify();return true;}
void PlayerAnchor(std::uint32_t ordinal,void* player,bool valid) noexcept
{
    if(!Enabled() || ordinal>=2 || !MainThread())return;
    const auto previous=g_players[ordinal];
    if(previous!=(valid?Address(player):0)) {
        const std::uint32_t words[]{previous,Address(player),valid?1u:0u};
        e2e::Record(e2e::Kind::Anchor,ordinal,words,3);
    }
    g_players[ordinal]=valid?Address(player):0;
    if(valid){auto* id=Find(0,Address(player),true);if(id)id->ordinal=ordinal;}
    else for(auto& id:g_identities)if(id.ordinal==ordinal){
        const std::uint32_t words[]{id.init,0,id.address,id.origin};
        e2e::Record(e2e::Kind::Ready,ordinal,words,4,id.id,id.use);
        id.ordinal=UINT32_MAX;if(id.init==3)id.init=0;}
}
bool Observing() noexcept {return Enabled();}
bool PlayerPairMatches(void* player,void* device,void* control,std::int32_t key) noexcept
{
    if(!Enabled() || !MainThread() || key<0)return false;
    std::uint32_t player_key=0,descriptor[2]{};
    const auto manager=Address(g_image)+0x857B18;
    if(!U32(static_cast<std::uint64_t>(Address(player))+0x1618,player_key) ||
       player_key!=static_cast<std::uint32_t>(key) || !Read(static_cast<std::uint64_t>(manager)+8,descriptor,8))return false;
    const auto begin=descriptor[0],end=descriptor[1];
    if(!begin || begin>end || end-begin>32*8 || ((end-begin)%8) || ((begin|end)&3u))return false;
    std::uint32_t matches=0;bool exact=false;
    for(std::uint64_t cursor=begin;cursor<end;cursor+=8) {
        std::uint32_t pair[2]{},device_key=0;
        if(!Read(cursor,pair,8) || !pair[0] || !pair[1] ||
            !U32(static_cast<std::uint64_t>(pair[0])+12,device_key))return false;
        if(device_key==player_key){++matches;exact=pair[0]==Address(device) && pair[1]==Address(control);}
    }
    std::uint32_t after[2]{},after_key=0;
    return matches==1 && exact && Read(static_cast<std::uint64_t>(manager)+8,after,8) &&
        !std::memcmp(descriptor,after,8) && U32(static_cast<std::uint64_t>(Address(player))+0x1618,after_key) && after_key==player_key;
}
void PlayerReadReady(std::uint32_t ordinal,void* player) noexcept
{
    if(!Enabled() || ordinal>=2 || !MainThread() || g_players[ordinal]!=Address(player))return;
    auto* id=Find(0,Address(player));
    if(!id || id->ordinal!=ordinal || id->init>=2)return;
    // tx-01-73 user-accepted current-value readiness rule. This is not an
    // observed constructor/Init or a reconstructed birth/use history.
    id->init=3;Emit(UINT32_MAX,false,id,id->address,ordinal,Reason::None,6);
    const std::uint32_t words[]{0,3,id->address,id->origin};
    e2e::Record(e2e::Kind::Ready,ordinal,words,4,id->id,id->use);
}
void Commit(std::uint64_t tick,std::uint32_t kind) noexcept
{
    if(!Enabled() || !MainThread() || !g_token.value)return;
    ++g_token.commits;g_token.tick=tick;g_token.commit_kind=kind;g_token.scope=g_token.value;
    e2e::Context(g_token.value,tick,g_attempt);
    const std::uint32_t words[]{g_token.commits,g_token.room_active?1u:0u};
    e2e::Record(e2e::Kind::Commit,kind,words,2);
    if(g_token.commits!=1 || (kind==1 && !g_token.room_active))g_token.dirty=true;
}
bool ObservationIdentity(std::uint64_t expected_tick,std::uint64_t& run,std::uint64_t& room,std::uint64_t& token) noexcept
{
    if(!g_shared || g_post_state.tick!=expected_tick)return false;
    run=g_post_run;room=g_post_room;token=g_post_token;return run && room && token;
}
coop_active::TickPostState PendingPostState() noexcept
{
    if(!g_shared)return {};
    return g_post_state;
}
bool PublishPostState(bool committed) noexcept
{
    auto* batch=g_pending_batch.exchange(nullptr,std::memory_order_acq_rel);
    if(!batch)return true;
    MemoryBarrier();InterlockedExchange(&batch->state,committed?2:0);
    return Notify();
}
void Lifecycle(std::uint32_t signal,bool entering) noexcept
{
    if(!Enabled() || !MainThread())return;
    const std::uint32_t audit[]{entering?1u:0u,g_lifecycle_depth};
    e2e::Record(e2e::Kind::Lifecycle,signal,audit,2);
    if(entering) {
        if(!Next(g_mutation))return;g_token.dirty=true;
        if(!g_lifecycle_depth && signal!=4)Next(g_room);
        if(g_lifecycle_depth==UINT32_MAX){Error(Reason::SequenceExhausted);return;}
        ++g_lifecycle_depth;
        if(signal==4){g_retired=true;Next(g_run);for(auto& id:g_identities)if(id.address){id.destroyed=true;id.born=false;id.ordinal=UINT32_MAX;}}
    } else if(!g_lifecycle_depth) {Error(Reason::UnpairedScope);return;}
    else --g_lifecycle_depth;
    Emit(UINT32_MAX,entering,nullptr,0,signal,Reason::Lifecycle,2);
}
void StageEvent(Stage stage,bool entering,void* owner) noexcept
{
    if(!Enabled() || !MainThread())return;
    if(stage==Stage::Outer) {
        if(entering) {
            if(g_pending_batch.load(std::memory_order_acquire)) {PublishPostState(false);Error(Reason::UnpairedScope);return;}
            g_post_state={coop_active::PostStateStatus::Unavailable,static_cast<std::uint32_t>(Reason::Stopped),0,0,0};
            g_post_run=0;g_post_room=0;g_post_token=0;
            if(g_token.value){Error(Reason::UnpairedScope);return;}
            if(!Next(g_outer))return;g_token={};g_token.value=g_outer;g_token.mutation=g_mutation;
            e2e::Context(g_token.value,0,g_attempt);
            const std::uint32_t words[]{1};e2e::Record(e2e::Kind::Stage,0,words,1);
            if(g_lifecycle_depth || g_retired)g_token.dirty=true;
        } else {
            if(!g_token.value){Error(Reason::UnpairedScope);return;}
            Reason reason=Reason::None;
            if(g_depth){Error(Reason::Continuity);reason=Reason::Continuity;}
            else if(g_token.dirty || g_mutation!=g_token.mutation || g_lifecycle_depth)reason=Reason::Lifecycle;
            else if(g_token.commit_kind==2)reason=Reason::OddOnly;
            else if(g_token.owner_enter!=1 || g_token.owner_exit!=1 || g_token.room_enter!=1 ||
                g_token.room_exit!=1 || g_token.tail_enter!=1 || g_token.tail_exit!=1 ||
                g_token.owner_active || g_token.room_active || g_token.tail_active)reason=Reason::PhaseIncomplete;
            __declspec(align(16)) std::uint8_t fp[512];
            __asm {fxsave fp}
            Capture(reason);
            __asm {fxrstor fp}
            const std::uint32_t words[]{0,static_cast<std::uint32_t>(reason),g_token.commits};
            e2e::Record(e2e::Kind::Stage,0,words,3);
            g_token={};
        }
        return;
    }
    if(!g_token.value)return;
    e2e::Context(g_token.value,g_token.tick,g_attempt);
    const std::uint32_t words[]{entering?1u:0u,Address(owner)};
    e2e::Record(e2e::Kind::Stage,static_cast<std::uint32_t>(stage),words,2);
    auto& token=g_token;
    if(stage==Stage::Owner) {
        if(entering){if(token.owner_active)token.dirty=true;++token.owner_enter;token.owner_active=true;}
        else {if(!token.owner_active || token.room_active || token.tail_active)token.dirty=true;++token.owner_exit;token.owner_active=false;}
    } else if(stage==Stage::Room) {
        if(entering){if(!token.owner_active || token.room_active || token.tail_enter)token.dirty=true;
            ++token.room_enter;token.room_active=true;token.room_address=Address(owner);}
        else {if(!token.room_active || token.room_address!=Address(owner))token.dirty=true;++token.room_exit;token.room_active=false;}
    } else {
        if(entering){if(!token.owner_active || token.room_active || token.room_exit!=1 || token.tail_active)token.dirty=true;++token.tail_enter;token.tail_active=true;}
        else {if(!token.tail_active)token.dirty=true;++token.tail_exit;token.tail_active=false;}
    }
}
} // namespace isaac_cs_t0::world
