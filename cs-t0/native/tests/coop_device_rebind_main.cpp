#include <Windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <vector>
#include "isaac_cs_t0/coop_passive_ipc.hpp"
#include "isaac_cs_t0/coop_passive_runtime.hpp"
#include "isaac_cs_t0/hooks.hpp"

namespace {
using namespace isaac_cs_t0::coop_passive;
using namespace isaac_cs_t0::coop_passive::detail;
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x); ++failures; return false; } } while(false)

struct Call { void* self; std::int32_t id; std::uint32_t propagate; };
std::vector<Call> original_calls;
std::uint32_t manager_calls = 0;
void* manager_self = nullptr;
bool post_read_fault = false;
HANDLE original_entered = nullptr;
HANDLE original_release = nullptr;
bool manager_route = false;
void* route_player = nullptr;
void* route_hud = nullptr;
std::uint32_t spawn_calls = 0, hud_calls = 0;
void* __fastcall SpawnOriginal(void*,void*,std::int32_t) { ++spawn_calls; return route_player; }
void __fastcall HudOriginal(void*,void*) { ++hud_calls; }

void __fastcall AssignmentOriginal(void* self, void*, std::int32_t id, std::uint32_t propagate)
{
    original_calls.push_back({self,id,propagate});
    *reinterpret_cast<std::int32_t*>(static_cast<std::uint8_t*>(self)+0x1618) = id;
    if (post_read_fault) SetDeviceRebindHarnessFault(DeviceRebindHarnessFault::ReadException);
    if (original_entered) {
        SetEvent(original_entered);
        WaitForSingleObject(original_release,15000);
    }
}
std::uint32_t __fastcall ManagerOriginal(void* self, void*)
{
    ++manager_calls;
    manager_self=self;
    if(manager_route) {
        manager_route=false;
        using Spawn=void*(__thiscall*)(void*,std::int32_t);
        using Assignment=void(__thiscall*)(void*,std::int32_t,std::uint32_t);
        using Hud=void(__thiscall*)(void*);
        reinterpret_cast<Spawn>(DeviceRebindOrdinaryWrapperAddress(46))(self,8);
        reinterpret_cast<Assignment>(DeviceRebindOrdinaryWrapperAddress(49))(route_player,8,1);
        reinterpret_cast<Hud>(DeviceRebindOrdinaryWrapperAddress(51))(route_hud);
    }
    return 0xC001CAFE;
}

struct Fixture {
    std::vector<void*> allocations;
    std::uint8_t* main = Allocate(0x900000);
    std::uint8_t* root = Allocate(0x30000);
    std::uint8_t* online = Allocate(0x50000);
    std::uint8_t* storage = Allocate(0x1000);
    std::array<std::uint8_t*,2> players{Allocate(0x2000),Allocate(0x2000)};
    std::array<std::uint8_t*,2> devices{Allocate(0x100),Allocate(0x100)};
    SharedCoopPassiveBlock* shared = reinterpret_cast<SharedCoopPassiveBlock*>(Allocate(sizeof(SharedCoopPassiveBlock)));
    bool started = false;

    std::uint8_t* Allocate(std::size_t size) {
        void* p=VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        if(p) allocations.push_back(p);
        return static_cast<std::uint8_t*>(p);
    }
    static void Vector(std::uint8_t* header,std::uint8_t* first,std::size_t bytes) {
        auto** h=reinterpret_cast<std::uint8_t**>(header);
        h[0]=first; h[1]=first?first+bytes:nullptr; h[2]=h[1];
    }
    bool Begin() {
        CHECK(main && root && online && storage && players[0] && players[1] && devices[0] && devices[1] && shared);
        *reinterpret_cast<void**>(main+0x871678)=root;
        *reinterpret_cast<void**>(main+0x87169C)=online;
        *reinterpret_cast<std::uint16_t*>(root+0x2658A)=1;
        Vector(main+0x857B18+8,storage,16);
        Vector(root+0x1BAA8,storage+0x100,8);
        Vector(online+0x4B3D8,nullptr,0);
        reinterpret_cast<void**>(main+0x76BDD0)[-1]=main+0x7AE31C;
        for(std::size_t i=0;i<2;++i) {
            reinterpret_cast<void**>(storage)[i*2]=devices[i];
            reinterpret_cast<void**>(storage)[i*2+1]=root+0x10+i*0x10;
            reinterpret_cast<void**>(storage+0x100)[i]=players[i];
            *reinterpret_cast<void**>(players[i])=main+0x76BDD0;
            *reinterpret_cast<std::int32_t*>(players[i]+0x1618)=static_cast<std::int32_t>(7+i);
            *reinterpret_cast<std::uint32_t*>(players[i]+0x161C)=static_cast<std::uint32_t>(i);
            *reinterpret_cast<void**>(devices[i])=main+(i?0x782C80:0x7A1678);
            *reinterpret_cast<std::int32_t*>(devices[i]+0xC)=static_cast<std::int32_t>(7+i);
            *reinterpret_cast<void**>(root+0x1DA04+i*0x6DC)=players[i];
        }
        original_calls.clear(); manager_calls=0; manager_self=nullptr; post_read_fault=false; manager_route=false;
        started=BeginDeviceRebindHarness(main,shared,reinterpret_cast<void*>(&AssignmentOriginal),reinterpret_cast<void*>(&ManagerOriginal));
        CHECK(started);
        return true;
    }
    ~Fixture() {
        if(started) EndDeviceRebindHarness();
        for(auto i=allocations.rbegin();i!=allocations.rend();++i) VirtualFree(*i,0,MEM_RELEASE);
    }
    void* Manager() const { return root+0x1BAA8; }
    bool Pass() {
        const auto before=manager_calls;
        CHECK(InvokeDeviceRebindManager(Manager())==0xC001CAFE);
        CHECK(manager_calls==before+1 && manager_self==Manager());
        return true;
    }
    std::vector<NativePassiveEvent> Events() const {
        std::vector<NativePassiveEvent> result;
        const auto count=static_cast<std::size_t>(shared->next_sequence);
        for(std::size_t i=0;i<count && i<kEventCapacity;++i)
            if(shared->events[i].published_sequence==static_cast<LONG64>(i+1)) result.push_back(shared->events[i]);
        return result;
    }
    std::size_t Count(EventKind kind,std::uint64_t generation=0) const {
        std::size_t count=0;
        for(const auto& e:Events()) if(e.kind==static_cast<std::uint32_t>(kind) && (!generation||e.owner_generation==generation)) ++count;
        return count;
    }
    bool Bound() {
        CHECK(Pass()); CHECK(Count(EventKind::BindingCandidate,1)==2); CHECK(Count(EventKind::BindingBound)==0);
        CHECK(Pass()); CHECK(Count(EventKind::BindingBound,1)==2); return true;
    }
};

// Deliberately bypass InvokeDeviceRebindAssignment: the real wrapper must consume
// its two stack arguments and preserve all x86 nonvolatile registers itself.
bool CallAbi(void* address,void* self,std::int32_t id,std::uint32_t propagate)
{
    std::uint32_t before=0,after=0,saved_bx=0,saved_si=0,saved_di=0,bp_before=0,bp_after=0;
    __asm {
        push ebx
        push esi
        push edi
        mov before,esp
        mov bp_before,ebp
        mov ebx,0xA1B2C3D4
        mov esi,0x10293847
        mov edi,0x56473829
        mov ecx,self
        push propagate
        push id
        call address
        mov after,esp
        mov saved_bx,ebx
        mov saved_si,esi
        mov saved_di,edi
        mov bp_after,ebp
        pop edi
        pop esi
        pop ebx
    }
    CHECK(before==after);
    CHECK(saved_bx==0xA1B2C3D4 && saved_si==0x10293847 && saved_di==0x56473829 && bp_before==bp_after);
    return true;
}

bool OriginalOnceMatrix()
{
    const std::array<DeviceRebindHarnessFault,7> faults{DeviceRebindHarnessFault::None,
        DeviceRebindHarnessFault::ObservationDisabled,DeviceRebindHarnessFault::WrongThread,
        DeviceRebindHarnessFault::ReadException,DeviceRebindHarnessFault::RingFull,
        DeviceRebindHarnessFault::PossessorStop,DeviceRebindHarnessFault::Unsafe};
    for(std::uint32_t propagate:{1u,0x76543210u}) for(std::uint32_t slot=53;slot<=55;++slot) for(const auto fault:faults) {
        Fixture f; CHECK(f.Begin()); CHECK(f.Bound());
        if(slot==55) { InvokeDeviceRebindAssignment(54,f.players[1],-1,1); original_calls.clear(); }
        SetDeviceRebindHarnessFault(fault);
        const auto id=slot==55?8:-1;
        CHECK(CallAbi(DeviceRebindAssignmentWrapperAddress(slot),f.players[1],id,propagate));
        CHECK(original_calls.size()==1);
        CHECK(original_calls[0].self==f.players[1] && original_calls[0].id==id && original_calls[0].propagate==propagate);
        CHECK(*reinterpret_cast<std::int32_t*>(f.players[1]+0x1618)==id);
        CHECK(f.shared->active_hooks==0);
        CHECK(f.shared->override_count==0 && f.shared->apply_count==0 && f.shared->world_write_count==0);
        CHECK(f.shared->scheduler_count==0 && f.shared->transport_count==0 && f.shared->mode_latch_write_count==0);
        if(fault==DeviceRebindHarnessFault::Unsafe) CHECK(f.shared->state==static_cast<LONG>(NativeState::Unsafe));
    }
    for(std::uint32_t slot=53;slot<=55;++slot) {
        Fixture f; CHECK(f.Begin()); CHECK(f.Bound());
        if(slot==55) { InvokeDeviceRebindAssignment(54,f.players[1],-1,1); original_calls.clear(); }
        post_read_fault=true;
        CHECK(CallAbi(DeviceRebindAssignmentWrapperAddress(slot),f.players[1],slot==55?8:-1,1));
        CHECK(original_calls.size()==1);
    }
    return true;
}

bool CausalRoutes()
{
    for(bool menu:{false,true}) for(bool changed:{false,true}) {
        Fixture f; CHECK(f.Begin()); CHECK(f.Bound());
        const auto initial=f.Events();
        InvokeDeviceRebindAssignment(menu?53:54,f.players[1],-1,1);
        CHECK(f.shared->owner_generation==2);
        CHECK(f.Count(EventKind::BindingUnbound,2)==2);
        CHECK(f.Count(EventKind::BindingBound,2)==0);
        if(menu) InvokeDeviceRebindAssignment(54,f.players[1],-1,1);
        const auto assigned=changed?9:8;
        *reinterpret_cast<std::int32_t*>(f.devices[1]+0xC)=assigned;
        InvokeDeviceRebindAssignment(55,f.players[1],assigned,1);
        CHECK(original_calls.size()==(menu?3u:2u));
        CHECK(f.Count(EventKind::RelationChange)==0);
        CHECK(f.Pass());
        if(f.Count(EventKind::RelationChange,2)!=1) {
            std::fprintf(stderr,"route menu=%d changed=%d state=%ld error=%ld generation=%lld\n",menu,changed,f.shared->state,f.shared->error_code,f.shared->owner_generation);
            for(const auto& e:f.Events()) std::fprintf(stderr,"event kind=%u phase=%u gen=%llu slot=%u ordinal=%u flags=%u original=%u\n",e.kind,e.phase,e.owner_generation,e.slot_ordinal,e.local_ordinal,e.flags,e.original_calls);
        }
        CHECK(f.Count(EventKind::RelationChange,2)==1);
        CHECK(f.Count(EventKind::BindingCandidate,2)==2);
        CHECK(f.Count(EventKind::BindingBound,2)==0);
        CHECK(f.Pass()); CHECK(f.Count(EventKind::BindingBound,2)==2);
        const auto events=f.Events();
        std::vector<NativePassiveEvent> cause;
        const NativePassiveEvent* relation=nullptr;
        for(const auto& e:events) {
            if(e.kind>=28 && e.kind<=31) cause.push_back(e);
            if(e.kind==24) relation=&e;
        }
        CHECK(cause.size()==(menu?6u:4u)); CHECK(relation);
        const std::vector<std::uint32_t> expected=menu?std::vector<std::uint32_t>{53,53,54,54,55,55}:std::vector<std::uint32_t>{54,54,55,55};
        for(std::size_t i=0;i<cause.size();++i) {
            const auto& e=cause[i];
            CHECK(e.slot_ordinal==expected[i] && e.phase==10 && e.depth==0 && e.owner_generation==2);
            CHECK(e.local_ordinal==1 && e.endpoint==2);
            CHECK(e.player_digest==cause[0].player_digest);
            CHECK(e.original_calls==i%2 && ((e.flags&kFlagOriginalCalled)!=0)==(i%2==1));
            if(i>0) { CHECK((e.topology_digest==std::array<std::uint8_t,32>{})); CHECK((e.device_digest==std::array<std::uint8_t,32>{})); }
        }
        CHECK(cause[0].assignment_digest!=cause[1].assignment_digest);
        CHECK(cause.back().assignment_digest!=cause[1].assignment_digest);
        CHECK(relation->assignment_digest==cause.back().assignment_digest);
        CHECK(relation->slot_ordinal==55 && relation->phase==10 && (relation->flags&kFlagRelationChanged));
        std::size_t final_count=0;
        for(const auto& e:events) if(e.kind==19 && e.owner_generation==2) {
            CHECK(e.local_ordinal<2 && e.endpoint==e.local_ordinal+1);
            CHECK(e.topology_digest==relation->topology_digest);
            if(e.local_ordinal==1) CHECK(e.assignment_digest==relation->assignment_digest);
            else {
                const NativePassiveEvent* baseline=nullptr;
                for(const auto& before:initial) if(before.kind==19 && before.local_ordinal==0) baseline=&before;
                CHECK(baseline);
                CHECK(e.player_digest==baseline->player_digest && e.device_digest==baseline->device_digest && e.assignment_digest==baseline->assignment_digest);
            }
            ++final_count;
        }
        CHECK(final_count==2); CHECK(DrainDeviceRebindHarness()); CHECK(f.shared->active_hooks==0);
        const auto size=f.Events().size();
        CHECK(f.Pass()); CHECK(f.Events().size()==size);
    }
    return true;
}

bool RemovedDeviceAndBackend()
{
    for(int variant=0;variant<8;++variant) {
        Fixture f; CHECK(f.Begin());
        if(variant>=3 && variant<=5) {
            *reinterpret_cast<void**>(f.main+0x879A58)=f.storage+0x400;
            *reinterpret_cast<void**>(f.storage+0x400)=f.main+0x7A2440;
            CHECK(SetDeviceRebindHarnessOrdinaryOriginals(reinterpret_cast<void*>(&SpawnOriginal),reinterpret_cast<void*>(&HudOriginal)));
            route_player=f.players[1]; route_hud=f.root+0x1DA04+0x6DC; spawn_calls=0; hud_calls=0; manager_route=true;
        }
        if(variant>=3 && variant<=5) {
            CHECK(f.Pass()); CHECK(f.Count(EventKind::BindingCandidate)==2); CHECK(f.Count(EventKind::BindingBound)==0);
            CHECK(f.Pass()); CHECK(f.Count(EventKind::BindingBound)==2);
        } else CHECK(f.Bound());
        if(variant>=3 && variant<=5) CHECK(spawn_calls==1 && hud_calls==1);
        const auto old_gen=static_cast<std::uint64_t>(f.shared->owner_generation);
        if(variant==0) Fixture::Vector(f.main+0x857B18+8,f.storage,8); // removed B, A remains coherent
        if(variant==1) { reinterpret_cast<void**>(f.storage)[0]=f.devices[1]; Fixture::Vector(f.main+0x857B18+8,f.storage,8); }
        if(variant==2) *reinterpret_cast<std::int32_t*>(f.players[0]+0x1618)=88;
        if(variant==4) *reinterpret_cast<std::uint32_t*>(f.online+0x4B3E4)=1;
        if(variant==5) Fixture::Vector(f.online+0x4B3D8,f.storage+0x100,8);
        if(variant==6) reinterpret_cast<void**>(f.storage)[1]=f.root+0x30;
        InvokeDeviceRebindAssignment(54,f.players[1],-1,1);
        if(variant==7) reinterpret_cast<void**>(f.storage)[1]=f.root+0x30;
        Fixture::Vector(f.main+0x857B18+8,f.storage,16);
        InvokeDeviceRebindAssignment(55,f.players[1],8,1);
        CHECK(f.Pass()); CHECK(f.Pass());
        if(variant==0 || variant==3) {
            CHECK(f.Count(EventKind::RelationChange)==1);
            CHECK(f.Count(EventKind::BindingBound,old_gen+1)==2);
        } else CHECK(f.Count(EventKind::RelationChange)==0);
    }
    return true;
}

struct ThreadCall { void* self; bool drained=false; HANDLE finished=nullptr; };
DWORD WINAPI HeldAssignment(void* raw) {
    auto* call=static_cast<ThreadCall*>(raw);
    InvokeDeviceRebindAssignment(54,call->self,-1,1);
    return 0;
}
DWORD WINAPI DrainThread(void* raw) {
    auto* call=static_cast<ThreadCall*>(raw);
    call->drained=DrainDeviceRebindHarness();
    SetEvent(call->finished); return 0;
}
bool CallbackDrain()
{
    for(bool timeout:{false,true}) {
        Fixture f; CHECK(f.Begin()); CHECK(f.Bound());
        original_entered=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        original_release=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        ThreadCall call{f.players[1],false,CreateEventW(nullptr,FALSE,FALSE,nullptr)};
        HANDLE writer=CreateThread(nullptr,0,HeldAssignment,&call,0,nullptr);
        CHECK(writer && WaitForSingleObject(original_entered,2000)==WAIT_OBJECT_0);
        CHECK(f.shared->active_hooks>0);
        HANDLE drain=CreateThread(nullptr,0,DrainThread,&call,0,nullptr);
        CHECK(drain);
        CHECK(WaitForSingleObject(call.finished,100)==WAIT_TIMEOUT);
        if(timeout) {
            CHECK(WaitForSingleObject(call.finished,6000)==WAIT_OBJECT_0);
            CHECK(!call.drained && f.shared->state==static_cast<LONG>(NativeState::Unsafe));
        }
        SetEvent(original_release);
        CHECK(WaitForSingleObject(writer,2000)==WAIT_OBJECT_0);
        CHECK(WaitForSingleObject(drain,2000)==WAIT_OBJECT_0);
        CHECK(f.shared->active_hooks==0 && original_calls.size()==1);
        if(!timeout) CHECK(call.drained);
        else CHECK(f.shared->state==static_cast<LONG>(NativeState::Unsafe));
        CloseHandle(writer); CloseHandle(drain); CloseHandle(call.finished);
        CloseHandle(original_entered); CloseHandle(original_release); original_entered=nullptr; original_release=nullptr;
    }
    return true;
}

bool RejectedRoutes()
{
    for(int variant=0;variant<10;++variant) {
        Fixture f; CHECK(f.Begin()); CHECK(f.Bound());
        if(variant==0) {
            *reinterpret_cast<std::int32_t*>(f.players[1]+0x1618)=9;
            *reinterpret_cast<std::int32_t*>(f.devices[1]+0xC)=9;
        } else {
            InvokeDeviceRebindAssignment(54,f.players[1],-1,1);
            if(variant==1) SetDeviceRebindHarnessFault(DeviceRebindHarnessFault::LifecycleEnter);
            if(variant==2) SetDeviceRebindHarnessFault(DeviceRebindHarnessFault::MenuTeardown);
            if(variant==3) SetDeviceRebindHarnessFault(DeviceRebindHarnessFault::AdvanceGeneration);
            if(variant==4) InvokeDeviceRebindAssignment(54,f.players[1],-1,1);
            if(variant==5) InvokeDeviceRebindAssignment(53,f.players[1],-1,1);
            if(variant==6) { *reinterpret_cast<std::uint32_t*>(f.players[1]+0x161C)=0; }
            if(variant==7) { *reinterpret_cast<void**>(f.players[1])=f.main+0x1000; }
            InvokeDeviceRebindAssignment(55,variant==8?f.players[0]:f.players[1],variant==9?-1:8,1);
        }
        CHECK(f.Pass()); CHECK(f.Pass());
        CHECK(f.Count(EventKind::RelationChange)==0);
        if(variant==0) CHECK(f.Count(EventKind::UnclassifiedRelationChange)==1);
    }
    return true;
}

struct Patches {
    std::uint8_t* memory=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,0x4000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    std::array<isaac_cs_t0::mode::RelativeCallPatch,44> calls{};
    std::array<isaac_cs_t0::mode::PointerPatch,12> pointers{};
    std::array<isaac_cs_t0::mode::OrderedPatchDescriptor,56> descriptors{};
    ~Patches() { if(memory) VirtualFree(memory,0,MEM_RELEASE); }
    void Reset() {
        std::memset(memory,0,0x4000); calls={}; pointers={}; descriptors={};
        std::size_t r=0,p=0;
        for(std::uint32_t i=0;i<56;++i) {
            descriptors[i].ordinal=i;
            if(i>=27 && i<=38) {
                auto& x=pointers[p++]; x.slot=reinterpret_cast<void**>(memory+0x2000+i*8);
                x.expected_original=memory+0x2400; x.replacement=memory+0x2800; *x.slot=x.expected_original;
                descriptors[i].pointer=&x;
            } else {
                auto& x=calls[r++]; x.call=memory+i*8; x.replacement=memory+0x1400;
                x.call[0]=0xE8; const auto displacement=static_cast<std::int32_t>((memory+0x1000)-(x.call+5));
                std::memcpy(x.call+1,&displacement,4); std::memcpy(x.expected,x.call,5); descriptors[i].relative=&x;
            }
        }
    }
    bool Restored() const {
        for(const auto& x:calls) if(x.installed || std::memcmp(x.call,x.expected,5)) return false;
        for(const auto& x:pointers) if(x.installed || *x.slot!=x.expected_original) return false;
        return true;
    }
};

struct PatchTrace { isaac_cs_t0::mode::OrderedPatchTestOperation operation; std::uint32_t ordinal; bool succeeded; bool installed; bool original_bytes; };
std::array<PatchTrace,512> patch_trace{};
std::size_t patch_trace_count=0;
Patches* traced_patches=nullptr;
void TracePatch(isaac_cs_t0::mode::OrderedPatchTestOperation operation,std::uint32_t ordinal,bool succeeded) noexcept {
    if(!traced_patches || ordinal>=56 || patch_trace_count>=patch_trace.size()) return;
    const auto& d=traced_patches->descriptors[ordinal];
    const bool installed=d.relative?d.relative->installed:d.pointer->installed;
    const bool original=d.relative?std::memcmp(d.relative->call,d.relative->expected,5)==0:*d.pointer->slot==d.pointer->expected_original;
    patch_trace[patch_trace_count++]={operation,ordinal,succeeded,installed,original};
}

bool Ordered56Transaction()
{
    using namespace isaac_cs_t0::mode;
    Patches f; CHECK(f.memory); traced_patches=&f; SetOrderedPatchTestObserver(TracePatch);
    for(std::uint32_t slot=0;slot<56;++slot) for(int stage=0;stage<5;++stage) {
        f.Reset(); const bool ptr=slot>=27 && slot<=38;
        SetPatchTestFault(PatchTestFaultStage::None,UINT32_MAX);
        if(stage==3) CHECK(InstallOrderedPatchTransaction(f.descriptors.data(),56));
        patch_trace_count=0;
        const auto fault=stage==0?(ptr?PatchTestFaultStage::PreparePointer:PatchTestFaultStage::PrepareRelative):
            stage==1?(ptr?PatchTestFaultStage::ApplyPointer:PatchTestFaultStage::ApplyRelative):
            stage==2?PatchTestFaultStage::Protect:stage==4?PatchTestFaultStage::ApplyAfterWrite:(ptr?PatchTestFaultStage::RestorePointer:PatchTestFaultStage::RestoreRelative);
        SetPatchTestFault(fault,slot);
        if(stage==3) {
            CHECK(!RestoreOrderedPatchTransaction(f.descriptors.data(),56));
            CHECK(patch_trace_count==56);
            for(std::size_t i=0;i<56;++i) {
                CHECK(patch_trace[i].operation==OrderedPatchTestOperation::Restore && patch_trace[i].ordinal==55-i);
                CHECK(patch_trace[i].succeeded==(patch_trace[i].ordinal!=slot));
            }
            SetPatchTestFault(PatchTestFaultStage::None,UINT32_MAX);
            CHECK(RestoreOrderedPatchTransaction(f.descriptors.data(),56));
        } else {
            PatchTransactionFailure failure{};
            CHECK(!InstallOrderedPatchTransaction(f.descriptors.data(),56,&failure)); CHECK(failure.index==slot);
            if(stage==0) CHECK(patch_trace_count==0);
            else {
                const std::size_t rollback_count=slot+(stage==4?1u:0u);
                CHECK(patch_trace_count==slot+1+rollback_count);
                for(std::size_t i=0;i<=slot;++i) {
                    CHECK(patch_trace[i].operation==OrderedPatchTestOperation::Apply && patch_trace[i].ordinal==i);
                    CHECK(patch_trace[i].succeeded==(i!=slot));
                    CHECK(patch_trace[i].installed==(i!=slot || stage==4));
                }
                for(std::size_t i=0;i<rollback_count;++i) {
                    const auto& entry=patch_trace[slot+1+i];
                    CHECK(entry.operation==OrderedPatchTestOperation::Rollback && entry.ordinal==rollback_count-1-i);
                    CHECK(entry.succeeded && !entry.installed && entry.original_bytes);
                }
            }
        }
        CHECK(f.Restored());
    }
    f.Reset(); patch_trace_count=0; SetPatchTestFault(PatchTestFaultStage::None,UINT32_MAX);
    CHECK(InstallOrderedPatchTransaction(f.descriptors.data(),56));
    CHECK(patch_trace_count==56);
    for(std::size_t i=0;i<56;++i) CHECK(patch_trace[i].ordinal==i && patch_trace[i].installed && !patch_trace[i].original_bytes);
    patch_trace_count=0;
    CHECK(RestoreOrderedPatchTransaction(f.descriptors.data(),56)); CHECK(f.Restored());
    CHECK(patch_trace_count==56);
    for(std::size_t i=0;i<56;++i) CHECK(patch_trace[i].ordinal==55-i && !patch_trace[i].installed && patch_trace[i].original_bytes);
    f.Reset(); SetPatchTestFaultPair(PatchTestFaultStage::ApplyRelative,55,PatchTestFaultStage::RollbackRelative,54);
    PatchTransactionFailure failure{};
    CHECK(!InstallOrderedPatchTransaction(f.descriptors.data(),56,&failure)); CHECK(failure.index==54); CHECK(!f.Restored());
    SetPatchTestFault(PatchTestFaultStage::None,UINT32_MAX);
    CHECK(RestoreOrderedPatchTransaction(f.descriptors.data(),56)); CHECK(f.Restored());
    SetOrderedPatchTestObserver(nullptr); traced_patches=nullptr;
    return true;
}

// These dependencies replace only the embedded observer boundary. Shutdown's
// admission, cleanup, generation, counters, Event/Publish and state machine are
// always the real production implementation. Restore writes 56 real test slots.
struct ShutdownDependencies {
    Fixture& fixture;
    Patches patches;
    std::array<std::uint32_t,16> operations{};
    std::size_t operation_count=0;
    unsigned drains=0;
    int fault=0;
    bool callback_state_valid=true;
    explicit ShutdownDependencies(Fixture& owner):fixture(owner) {}
    ~ShutdownDependencies() { isaac_cs_t0::mode::SetOrderedPatchTestObserver(nullptr); traced_patches=nullptr; }
    bool Prepare() {
        using namespace isaac_cs_t0::mode;
        CHECK(patches.memory); patches.Reset();
        SetPatchTestFault(PatchTestFaultStage::None,UINT32_MAX);
        CHECK(InstallOrderedPatchTransaction(patches.descriptors.data(),56));
        fixture.shared->installed_hooks_mask=0x00FFFFFFFFFFFFFFll;
        fixture.shared->slot_snapshot_before_sha256.fill(0x5A);
        traced_patches=&patches; patch_trace_count=0; SetOrderedPatchTestObserver(TracePatch);
        return true;
    }
    void Record(std::uint32_t operation) noexcept {
        if(operation_count<operations.size()) operations[operation_count++]=operation;
        else callback_state_valid=false;
        if(fixture.shared->state!=static_cast<LONG>(NativeState::ShuttingDown) || fixture.shared->producer_closed!=0)
            callback_state_valid=false;
    }
    static ShutdownDependencies& Self(void* p) noexcept { return *static_cast<ShutdownDependencies*>(p); }
    static bool Wait(void* p) noexcept {
        auto& x=Self(p); ++x.drains; x.Record(x.drains==1?1u:3u);
        return x.fixture.shared->active_hooks==0 && !(x.fault==1 && x.drains==1) && !(x.fault==3 && x.drains==2);
    }
    static DWORD Restore(void* p) noexcept {
        auto& x=Self(p); x.Record(2);
        if(x.fault==2) return 1;
        return isaac_cs_t0::mode::RestoreOrderedPatchTransaction(x.patches.descriptors.data(),56)?0u:1u;
    }
    static bool Additional(void* p) noexcept { auto& x=Self(p); x.Record(4); return x.fault!=4 && x.patches.Restored(); }
    static DWORD Stop(void* p) noexcept { auto& x=Self(p); x.Record(5); return x.fault==5?1u:0u; }
    static bool Verify(void* p,std::uint32_t& restored) noexcept {
        auto& x=Self(p); x.Record(6); restored=x.fault==7?55u:56u; return x.fault!=6 && x.patches.Restored();
    }
    static bool Snapshot(void* p,std::array<std::uint8_t,32>& result) noexcept {
        auto& x=Self(p); x.Record(7); result.fill(x.fault==9?0xAA:0x5A);
        if(x.fault==10 || x.fault==11) {
            // Real Publish capacity failures: no fake return value, and no
            // fabricated terminal records. All memory belongs to this fixture.
            x.fixture.shared->controller_sequence=0;
            x.fixture.shared->next_sequence=static_cast<LONG64>(kEventCapacity)-(x.fault==11?1:0);
        }
        return x.fault!=8;
    }
    DeviceRebindShutdownDependencies Callbacks() noexcept {
        DeviceRebindShutdownDependencies d{}; d.context=this; d.wait_for_callbacks_drained=Wait;
        d.restore_embedded_input_hooks=Restore; d.verify_additional_targets_restored=Additional;
        d.stop_embedded_observer=Stop; d.verify_embedded_observer_restored=Verify;
        d.snapshot_embedded_observer_slots=Snapshot; return d;
    }
};

template<typename T,std::size_t N> bool All(const std::array<T,N>& values,T expected) {
    for(const auto value:values) if(value!=expected) return false;
    return true;
}
bool ClearedTopology(const DeviceRebindTopologySnapshot& x) {
    CHECK(All(x.roots,std::uintptr_t{0}) && x.online_frame==0 && x.device_count==0 && x.player_count==0);
    CHECK(All(x.device_objects,std::uintptr_t{0}) && All(x.device_reference_owners,std::uintptr_t{0}));
    CHECK(All(x.device_assignments,std::int32_t{-1}) && All(x.device_kinds,std::uint32_t{1}));
    CHECK(All(x.player_objects,std::uintptr_t{0}) && All(x.player_devices,std::uintptr_t{0}));
    CHECK(All(x.player_assignments,std::int32_t{-1}) && All(x.player_ordinals,UINT32_MAX));
    CHECK(All(x.player_device_kinds,std::uint32_t{1}) && All(x.hud,std::uintptr_t{0}));
    return true;
}
bool ClearedPrivateState(std::uint64_t generation,bool retired) {
    const auto x=SnapshotDeviceRebindPrivateState();
    CHECK(x.generation==generation && x.retired==retired);
    CHECK(!x.candidate_valid && !x.bound_valid && x.candidate_generation==0 && x.candidate_pass==0);
    CHECK(All(x.binding_states,retired?std::uint32_t{3}:std::uint32_t{0}));
    CHECK(ClearedTopology(x.candidate) && ClearedTopology(x.bound) && ClearedTopology(x.device_baseline));
    CHECK(x.device_stage==0 && x.device_ordinal==UINT32_MAX && x.device_primary_slot==UINT32_MAX);
    CHECK(x.device_generation==0 && x.device_last_manager_pass==0 && x.device_candidate_pass==0);
    CHECK(x.device_new_assignment==-1 && !x.device_ordinary_backend_authorized);
    CHECK(!x.ordinary_transaction_active && x.ordinary_manager==0 && x.ordinary_manager_pass==0);
    CHECK(x.ordinary_stage==0 && x.ordinary_route_generation==0 && x.ordinary_authorized_generation==0);
    CHECK(All(x.token_secret,std::uint8_t{0})); return true;
}
bool PostShutdownOriginalOnce(Fixture& f) {
    const auto published=f.shared->next_sequence;
    for(std::uint32_t slot=53;slot<=55;++slot) {
        const auto count=original_calls.size();
        CHECK(CallAbi(DeviceRebindAssignmentWrapperAddress(slot),f.players[1],slot==55?8:-1,1));
        CHECK(original_calls.size()==count+1);
        CHECK(original_calls.back().self==f.players[1] && original_calls.back().id==(slot==55?8:-1) && original_calls.back().propagate==1);
    }
    CHECK(f.Pass()); CHECK(f.shared->next_sequence==published && f.shared->active_hooks==0);
    CHECK(f.shared->override_count==0 && f.shared->apply_count==0 && f.shared->world_write_count==0);
    CHECK(f.shared->scheduler_count==0 && f.shared->transport_count==0 && f.shared->mode_latch_write_count==0);
    return true;
}

bool ShutdownSuccessfulStates() {
    for(int variant=0;variant<11;++variant) {
        Fixture f; CHECK(f.Begin()); ShutdownDependencies dependencies(f); CHECK(dependencies.Prepare());
        if(variant==5) CHECK(SetDeviceRebindHarnessGeneration(INT64_MAX));
        if(variant==6) CHECK(SetDeviceRebindHarnessGeneration(UINT64_MAX));
        if(variant==7) {
            CHECK(!SetDeviceRebindHarnessGeneration(0));
            CHECK(SnapshotDeviceRebindPrivateState().generation==1 && f.shared->owner_generation==1);
        }
        if(variant>=9) {
            *reinterpret_cast<void**>(f.main+0x879A58)=f.storage+0x400;
            *reinterpret_cast<void**>(f.storage+0x400)=f.main+0x7A2440;
            CHECK(SetDeviceRebindHarnessOrdinaryOriginals(reinterpret_cast<void*>(&SpawnOriginal),reinterpret_cast<void*>(&HudOriginal)));
            route_player=f.players[1]; route_hud=f.root+0x1DA04+0x6DC; manager_route=true;
        }
        CHECK(PublishDeviceRebindHarnessInitialization());
        if(variant==1) CHECK(f.Pass()); // candidate, no confirmed binding
        else if(variant!=2 && variant!=7) { CHECK(f.Pass()); CHECK(f.Pass()); }
        if(variant>=9) CHECK(SnapshotDeviceRebindPrivateState().ordinary_authorized_generation!=0);
        if(variant==0 || variant==3 || variant==8 || variant==9) {
            InvokeDeviceRebindAssignment(54,f.players[1],-1,1);
            if(variant!=3) {
                InvokeDeviceRebindAssignment(55,f.players[1],8,1);
                CHECK(f.Pass()); if(variant==0 || variant==9) CHECK(f.Pass());
            }
        }
        if(variant==4) SetDeviceRebindHarnessFault(DeviceRebindHarnessFault::MenuTeardown);
        const auto before=SnapshotDeviceRebindPrivateState();
        if(variant==9) CHECK(before.ordinary_authorized_generation==0 && before.device_ordinary_backend_authorized);
        if(variant==10) CHECK(before.ordinary_authorized_generation!=0 && !before.device_ordinary_backend_authorized);
        const auto observed=before.generation;
        const auto prior_events=f.shared->next_sequence;
        CHECK(InvokeDeviceRebindShutdownHarness(dependencies.Callbacks())==0);
        const auto events=f.Events(); CHECK(events.size()>=2);
        const auto& restored=events[events.size()-2]; const auto& closed=events.back();
        if(restored.owner_generation!=observed || closed.owner_generation!=observed)
            std::fprintf(stderr,"REAL_SHUTDOWN_GENERATION_MISMATCH variant=%d observed=%llu shared=%llu private=%llu restored=%llu closed=%llu\n",variant,observed,static_cast<std::uint64_t>(f.shared->owner_generation),SnapshotDeviceRebindPrivateState().generation,restored.owner_generation,closed.owner_generation);
        CHECK(restored.kind==25 && closed.kind==26);
        CHECK(restored.owner_generation==observed && closed.owner_generation==observed);
        CHECK(static_cast<std::uint64_t>(f.shared->owner_generation)==observed);
        CHECK(f.shared->next_sequence==prior_events+2 && f.shared->state==static_cast<LONG>(NativeState::Stopped));
        CHECK(f.shared->producer_closed==1 && f.shared->publication_enabled==0 && f.shared->active_hooks==0);
        CHECK(f.shared->installed_hooks_mask==0x00FFFFFFFFFFFFFFll && f.shared->restored_hooks_mask==0x00FFFFFFFFFFFFFFll && f.shared->restored_slot_count==56);
        CHECK(dependencies.drains==2 && dependencies.callback_state_valid && dependencies.operation_count==7);
        for(std::size_t i=0;i<7;++i) CHECK(dependencies.operations[i]==i+1);
        CHECK(dependencies.patches.Restored() && patch_trace_count==56);
        for(std::size_t i=0;i<56;++i) CHECK(patch_trace[i].ordinal==55-i && patch_trace[i].original_bytes && !patch_trace[i].installed);
        CHECK(ClearedPrivateState(observed,before.retired));
        CHECK(PostShutdownOriginalOnce(f)); CHECK(ClearedPrivateState(observed,before.retired));
        const auto operation_count=dependencies.operation_count;
        CHECK(InvokeDeviceRebindShutdownHarness(dependencies.Callbacks())!=0);
        CHECK(dependencies.operation_count==operation_count && f.shared->state==static_cast<LONG>(NativeState::Stopped));
    }
    return true;
}

bool ShutdownDependencyFailures() {
    for(int fault=1;fault<=9;++fault) {
        Fixture f; CHECK(f.Begin()); CHECK(f.Bound());
        ShutdownDependencies dependencies(f); CHECK(dependencies.Prepare()); dependencies.fault=fault;
        const auto before=SnapshotDeviceRebindPrivateState(); const auto count=f.shared->next_sequence;
        CHECK(InvokeDeviceRebindShutdownHarness(dependencies.Callbacks())!=0);
        CHECK(f.shared->state==static_cast<LONG>(NativeState::Unsafe) && f.shared->producer_closed==0);
        CHECK(f.shared->restored_hooks_mask==0 && f.shared->restored_slot_count==0 && f.shared->next_sequence==count);
        const std::size_t expected=fault<=6?static_cast<std::size_t>(fault):fault==7?6u:7u;
        CHECK(dependencies.operation_count==expected && dependencies.callback_state_valid);
        for(std::size_t i=0;i<expected;++i) CHECK(dependencies.operations[i]==i+1);
        if(fault==1) {
            const auto after=SnapshotDeviceRebindPrivateState();
            CHECK(after.bound_valid==before.bound_valid && after.generation==before.generation && after.token_secret==before.token_secret);
        } else CHECK(ClearedPrivateState(before.generation,before.retired));
        CHECK(PostShutdownOriginalOnce(f));
        CHECK(f.shared->state==static_cast<LONG>(NativeState::Unsafe));
        const auto ops=dependencies.operation_count;
        CHECK(InvokeDeviceRebindShutdownHarness(dependencies.Callbacks())!=0);
        CHECK(dependencies.operation_count==ops && f.shared->state==static_cast<LONG>(NativeState::Unsafe));
    }
    Fixture f; CHECK(f.Begin()); CHECK(f.Bound()); ShutdownDependencies dependencies(f); CHECK(dependencies.Prepare());
    SetDeviceRebindHarnessFault(DeviceRebindHarnessFault::Unsafe);
    CHECK(InvokeDeviceRebindShutdownHarness(dependencies.Callbacks())!=0 && dependencies.operation_count==0);
    CHECK(f.shared->state==static_cast<LONG>(NativeState::Unsafe)); CHECK(PostShutdownOriginalOnce(f)); return true;
}

bool ShutdownRingFailure(bool second) {
    Fixture f; CHECK(f.Begin()); CHECK(f.Bound()); ShutdownDependencies dependencies(f); CHECK(dependencies.Prepare());
    dependencies.fault=second?11:10;
    const auto generation=SnapshotDeviceRebindPrivateState().generation;
    const auto result=InvokeDeviceRebindShutdownHarness(dependencies.Callbacks());
    if(result==0 || f.shared->state!=static_cast<LONG>(NativeState::Unsafe))
        std::fprintf(stderr,"REAL_SHUTDOWN_RING_FAILURE point=%u result=%lu state=%ld closed=%ld overwrite=%lld restoredEvents=%zu closedEvents=%zu\n",second?2u:1u,result,f.shared->state,f.shared->producer_closed,f.shared->overwritten_events,f.Count(EventKind::HooksRestored),f.Count(EventKind::ProducerClosed));
    CHECK(result!=0 && f.shared->state==static_cast<LONG>(NativeState::Unsafe));
    CHECK(f.shared->producer_closed==0 && f.shared->publication_enabled==0 && f.shared->overwritten_events==1);
    CHECK(f.Count(EventKind::HooksRestored)==(second?1u:0u) && f.Count(EventKind::ProducerClosed)==0);
    CHECK(ClearedPrivateState(generation,false)); CHECK(PostShutdownOriginalOnce(f));
    CHECK(f.shared->state==static_cast<LONG>(NativeState::Unsafe)); return true;
}

struct ProductionShutdownThreadCall {
    DeviceRebindShutdownDependencies dependencies{};
    DWORD result=UINT32_MAX;
    HANDLE finished=nullptr;
};
DWORD WINAPI ProductionShutdownThread(void* raw) {
    auto& call=*static_cast<ProductionShutdownThreadCall*>(raw);
    call.result=InvokeDeviceRebindShutdownHarness(call.dependencies);
    SetEvent(call.finished); return 0;
}
bool ShutdownWaitsForActualCallback() {
    Fixture f; CHECK(f.Begin()); CHECK(f.Bound()); ShutdownDependencies dependencies(f); CHECK(dependencies.Prepare());
    original_entered=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    original_release=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    ThreadCall original{f.players[1],false,nullptr};
    HANDLE writer=CreateThread(nullptr,0,HeldAssignment,&original,0,nullptr);
    CHECK(writer);
    const bool entered=WaitForSingleObject(original_entered,2000)==WAIT_OBJECT_0;
    ProductionShutdownThreadCall call{dependencies.Callbacks(),UINT32_MAX,CreateEventW(nullptr,FALSE,FALSE,nullptr)};
    call.dependencies.wait_for_callbacks_drained=nullptr; // both drains use the real production wait
    HANDLE shutdown=CreateThread(nullptr,0,ProductionShutdownThread,&call,0,nullptr);
    const bool blocked=shutdown && WaitForSingleObject(call.finished,100)==WAIT_TIMEOUT;
    const auto waiting=SnapshotDeviceRebindPrivateState();
    const bool still_admitted_callback=f.shared->active_hooks>0;
    const bool identity_retained_while_draining=waiting.device_baseline.player_count==2 && !All(waiting.token_secret,std::uint8_t{0});
    SetEvent(original_release);
    const bool writer_done=WaitForSingleObject(writer,2000)==WAIT_OBJECT_0;
    const bool shutdown_done=shutdown && WaitForSingleObject(shutdown,2000)==WAIT_OBJECT_0;
    CloseHandle(writer); if(shutdown) CloseHandle(shutdown); CloseHandle(call.finished);
    CloseHandle(original_entered); CloseHandle(original_release); original_entered=nullptr; original_release=nullptr;
    CHECK(entered && blocked && still_admitted_callback && identity_retained_while_draining);
    CHECK(writer_done && shutdown_done && call.result==0);
    CHECK(original_calls.size()==1 && f.shared->active_hooks==0 && f.shared->state==static_cast<LONG>(NativeState::Stopped));
    CHECK(ClearedPrivateState(waiting.generation,false));
    CHECK(dependencies.operation_count==5 && dependencies.callback_state_valid);
    const std::array<std::uint32_t,5> expected{2,4,5,6,7};
    for(std::size_t i=0;i<expected.size();++i) CHECK(dependencies.operations[i]==expected[i]);
    CHECK(f.Count(EventKind::HooksRestored)==1 && f.Count(EventKind::ProducerClosed)==1);
    CHECK(PostShutdownOriginalOnce(f)); return true;
}

bool RealShutdownSuite() {
    const bool success=ShutdownSuccessfulStates();
    const bool dependencies=ShutdownDependencyFailures();
    const bool first=ShutdownRingFailure(false);
    const bool second=ShutdownRingFailure(true);
    const bool actual_drain=ShutdownWaitsForActualCallback();
    if(success && dependencies && first && second && actual_drain) { std::puts("PASS coop-device-real-shutdown-v1"); return true; }
    return false;
}

int EmitCausal(int argc,wchar_t** argv) {
    if(argc!=4) return 2;
    const bool menu=std::wcscmp(argv[3],L"menu")==0;
    const bool removed=std::wcscmp(argv[3],L"removed-device")==0;
    const bool changed=std::wcscmp(argv[3],L"same-device")!=0;
    const bool incomplete=std::wcscmp(argv[3],L"incomplete-unassign")==0;
    const bool candidate=std::wcscmp(argv[3],L"incomplete-candidate")==0;
    Fixture f; if(!f.Begin()) return 3;
    ShutdownDependencies dependencies(f);
    if(!dependencies.Prepare() || !PublishDeviceRebindHarnessInitialization() || !f.Bound()) return 3;
    if(removed) Fixture::Vector(f.main+0x857B18+8,f.storage,8);
    InvokeDeviceRebindAssignment(menu?53:54,f.players[1],-1,1);
    if(menu) InvokeDeviceRebindAssignment(54,f.players[1],-1,1);
    Fixture::Vector(f.main+0x857B18+8,f.storage,16);
    if(!incomplete) {
        const int assignment=changed?9:8;
        *reinterpret_cast<std::int32_t*>(f.devices[1]+0xC)=assignment;
        InvokeDeviceRebindAssignment(55,f.players[1],assignment,1);
        if(!f.Pass() || (!candidate && !f.Pass()) || f.Count(EventKind::RelationChange)!=1) return 4;
    }
    const auto observed_generation=SnapshotDeviceRebindPrivateState().generation;
    const auto shutdown=InvokeDeviceRebindShutdownHarness(dependencies.Callbacks());
    if(shutdown!=0) return 4;
    const auto rows=f.Events();
    // Test-only file header, not a new production IPC. Preserve the real shared
    // terminal state separately from raw events so generation disagreement can
    // never be hidden by deriving the manifest generation from the last event.
    std::array<std::uint64_t,21> metadata{};
    metadata[0]=0x3154554853564544ull; // ASCII DEVSHUT1 (little-endian)
    metadata[1]=rows.size(); metadata[2]=observed_generation;
    metadata[3]=static_cast<std::uint64_t>(f.shared->owner_generation); metadata[4]=shutdown;
    metadata[5]=static_cast<std::uint64_t>(f.shared->state);
    metadata[6]=static_cast<std::uint64_t>(f.shared->installed_hooks_mask);
    metadata[7]=static_cast<std::uint64_t>(f.shared->restored_hooks_mask);
    metadata[8]=static_cast<std::uint64_t>(f.shared->restored_slot_count);
    metadata[9]=static_cast<std::uint64_t>(f.shared->producer_closed);
    metadata[10]=static_cast<std::uint64_t>(f.shared->publication_enabled);
    metadata[11]=static_cast<std::uint64_t>(f.shared->override_count|f.shared->apply_count|f.shared->world_write_count|f.shared->scheduler_count|f.shared->transport_count|f.shared->mode_latch_write_count);
    metadata[12]=dependencies.drains;
    std::memcpy(&metadata[13],f.shared->slot_snapshot_before_sha256.data(),32);
    std::memcpy(&metadata[17],f.shared->slot_snapshot_after_sha256.data(),32);
    HANDLE file=CreateFileW(argv[2],GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) return 5;
    const auto bytes=static_cast<DWORD>(rows.size()*sizeof(NativePassiveEvent)); DWORD written=0;
    const bool header=WriteFile(file,metadata.data(),sizeof(metadata),&written,nullptr) && written==sizeof(metadata);
    const bool complete=header && WriteFile(file,rows.data(),bytes,&written,nullptr) && written==bytes;
    CloseHandle(file); return complete?0:6;
}

int Publish(int argc,wchar_t** argv)
{
    if(argc!=5) return 2;
    HANDLE mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,argv[2]);
    HANDLE ready=OpenEventW(EVENT_MODIFY_STATE,FALSE,argv[3]);
    HANDLE ack=OpenEventW(SYNCHRONIZE,FALSE,argv[4]);
    if(!mapping||!ready||!ack) return 3;
    auto* b=static_cast<SharedCoopPassiveBlock*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(SharedCoopPassiveBlock)));
    if(!b || b->version!=2 || b->scenario!=5 || b->struct_size!=3277472) return 4;
    int result=0;
    for(int stage=0;stage<16;++stage) {
        b->version=2; b->scenario=5; b->state=2; b->next_sequence=1; b->overwritten_events=0; b->producer_closed=0;
        std::memset(b->events.data(),0,sizeof(NativePassiveEvent)*6);
        for(std::size_t i=0;i<5;++i) {
            auto& e=b->events[i]; e.published_sequence=static_cast<LONG64>(i+1);
            e.qpc=100+static_cast<std::int64_t>(i); e.loop_iteration=42; e.owner_generation=2;
            e.kind=28+static_cast<std::uint32_t>(i); e.slot_ordinal=i<2?54:55; e.phase=10;
            e.local_ordinal=1; e.endpoint=2; e.thread_id=17; e.main_thread_id=17;
            e.original_calls=static_cast<std::uint32_t>(i%2); e.flags=kFlagMainThread|kFlagEndpointB;
            if(i%2) e.flags|=kFlagOriginalCalled;
            e.player_digest.fill(0x12); e.assignment_digest.fill(0x34);
        }
        switch(stage) {
        case 0: b->next_sequence=5; break;
        case 1: b->events[0].published_sequence=0; break;
        case 2: break;
        case 3: b->events[0].flags|=0x80000000u; break;
        case 4: b->events[0].kind=33; break;
        case 5: b->events[0].slot_ordinal=56; break;
        case 6: b->events[0].phase=9; break;
        case 7: b->version=1; break;
        case 8: b->scenario=4; break;
        case 9: b->state=99; break;
        case 10: b->events[0].published_sequence=2; break;
        case 11: b->overwritten_events=1; break;
        case 12: b->next_sequence=kEventCapacity+1; break;
        case 13: b->producer_closed=1; b->events[0].published_sequence=0; break;
        case 14: b->producer_closed=1; b->state=5; b->events[0].kind=26; b->events[0].slot_ordinal=UINT32_MAX;
            b->installed_hooks_mask=0x00FFFFFFFFFFFFFFll; b->restored_hooks_mask=0x00FFFFFFFFFFFFFFll; b->restored_slot_count=56; break;
        case 15: b->version=3; break;
        }
        MemoryBarrier(); SetEvent(ready);
        if(WaitForSingleObject(ack,30000)!=WAIT_OBJECT_0) { result=5; break; }
    }
    UnmapViewOfFile(b); CloseHandle(mapping); CloseHandle(ready); CloseHandle(ack);
    return result;
}
}

int wmain(int argc,wchar_t** argv)
{
    if(argc>1 && std::wcscmp(argv[1],L"--test-device-shutdown")==0) return RealShutdownSuite()?0:1;
    if(argc>1 && std::wcscmp(argv[1],L"--publish-coop-device-cross-language")==0) return Publish(argc,argv);
    if(argc>1 && std::wcscmp(argv[1],L"--emit-device-causal-events")==0) return EmitCausal(argc,argv);
    const bool abi=OriginalOnceMatrix(); std::printf("%s device original-once x86 ABI\n",abi?"PASS":"FAIL");
    const bool causal=CausalRoutes(); std::printf("%s device causal routes\n",causal?"PASS":"FAIL");
    const bool reject=RejectedRoutes(); std::printf("%s device rejected routes\n",reject?"PASS":"FAIL");
    const bool boundaries=RemovedDeviceAndBackend(); std::printf("%s device removed-device and backend boundaries\n",boundaries?"PASS":"FAIL");
    const bool drain=CallbackDrain(); std::printf("%s device callback drain and Unsafe\n",drain?"PASS":"FAIL");
    const bool transaction=Ordered56Transaction(); std::printf("%s device 56-slot fault matrix\n",transaction?"PASS":"FAIL");
    const bool shutdown=RealShutdownSuite();
    if(abi && causal && reject && boundaries && drain && transaction && shutdown && failures==0) { std::puts("PASS coop-device-rebind-native-v2"); return 0; }
    return 1;
}
