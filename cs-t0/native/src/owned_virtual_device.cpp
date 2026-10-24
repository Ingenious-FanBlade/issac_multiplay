#include "isaac_cs_t0/owned_virtual_device.hpp"
#include <array>
#include <cstring>
#include <new>
#include <climits>

namespace isaac_cs_t0::coop_active {
namespace {
struct alignas(4) Object {
    std::uint8_t base[0x370];
    OwnedRead reader = nullptr;
    void* context = nullptr;
    volatile LONG neutral = 0;
};
struct alignas(4) Control {
    std::uint8_t base[0x18];
    volatile LONG strong = 1, weak = 1, busy = 0, fault = 0;
    Object* object = nullptr;
};
static_assert(offsetof(Object,reader) == 0x370);
static_assert(offsetof(Control,strong) == 0x18);
struct TypeDescriptor { void* vtable; void* spare; char name[48]; };
struct Hierarchy;
struct BaseDescriptor { void* type; std::uint32_t count; std::int32_t mdisp,pdisp,vdisp;
    std::uint32_t attributes; Hierarchy* hierarchy; };
struct Hierarchy { std::uint32_t signature,attributes,count; BaseDescriptor** bases; };
struct Locator { std::uint32_t signature,offset,ctor; void* type; Hierarchy* hierarchy; };
struct DeviceTable { Locator* locator = nullptr; std::array<void*,38> slots{}; };
TypeDescriptor g_type{};
BaseDescriptor g_self{};
std::array<BaseDescriptor*,2> g_bases{};
Hierarchy g_hierarchy{};
Locator g_locator{};
DeviceTable g_table{};
std::array<void*,6> g_control_table{};

// The normal-path contract is sequential. Unexpected concurrent entry is terminal:
// preserve memory/root and neutralize; do not claim an exact concurrent refcount.
bool Enter(Control* control) noexcept
{
    if (InterlockedCompareExchange(&control->busy,1,0) == 0) return true;
    InterlockedOr(&control->fault,1);
    InterlockedExchange(&control->object->neutral,1);
    return false;
}
void Leave(Control* control) noexcept
{
    const auto strong = static_cast<std::uint32_t>(control->strong);
    const auto weak = static_cast<std::uint32_t>(control->weak);
    const LONG mirror = static_cast<LONG>(((weak > 0xFFFF ? 0xFFFF : weak)<<16) |
                                         (strong > 0xFFFF ? 0xFFFF : strong));
    InterlockedExchange(reinterpret_cast<volatile LONG*>(control->base+4),mirror);
    InterlockedExchange(&control->busy,0);
}
void Fault(Control* control) noexcept
{
    InterlockedOr(&control->fault,2);
    InterlockedExchange(&control->object->neutral,1);
}
void* __fastcall ObjectDestroy(Object* object,void*,std::uint32_t) noexcept
{
    InterlockedExchange(&object->neutral,1); return object;
}
const char* __fastcall Name(Object*,void*) noexcept { return "IsaacCsT0 VirtualInputDevice"; }
bool __fastcall Poll(Object*,void*) noexcept { return true; }
float Read(Object* object,std::uint32_t action,Query query) noexcept
{
    if (InterlockedCompareExchange(&object->neutral,0,0) || !object->reader || action >= 32) return 0;
    return object->reader(object->context,action,query);
}
bool __fastcall Pressed(Object* object,void*,std::uint32_t action) noexcept { return Read(object,action,Query::Pressed)!=0; }
bool __fastcall Triggered(Object* object,void*,std::uint32_t action) noexcept { return Read(object,action,Query::Triggered)!=0; }
float __fastcall Scalar(Object* object,void*,std::uint32_t action) noexcept { return Read(object,action,Query::Scalar); }
void* __fastcall ControlDestroy(Control* control,void*,std::uint32_t) noexcept { Fault(control); return control; }
bool __fastcall TryStrong(Control* control,void*) noexcept
{
    if (!Enter(control)) return false;
    const auto strong=static_cast<std::uint32_t>(control->strong);
    const bool result = !control->fault && strong != 0 && strong != UINT32_MAX;
    if (result) {
        InterlockedIncrement(&control->strong);
        if (static_cast<std::uint32_t>(control->strong) > 0xFFFF) Fault(control);
    }
    Leave(control); return result;
}
std::uint32_t __fastcall AddStrong(Control* control,void*) noexcept
{
    if (!Enter(control)) return 1;
    const auto strong=static_cast<std::uint32_t>(control->strong);
    if (!strong || strong==UINT32_MAX) Fault(control);
    else { InterlockedIncrement(&control->strong); if (static_cast<std::uint32_t>(control->strong) > 0xFFFF) Fault(control); }
    Leave(control); return 1;
}
bool __fastcall ReleaseStrong(Control* control,void*) noexcept
{
    if (!Enter(control)) return true;
    // Permanent root is never handed to native code. Reaching its floor is an
    // unexpected release; retain it and all valid base/Mutex/object fields.
    if (static_cast<std::uint32_t>(control->strong) <= 1) Fault(control); else InterlockedDecrement(&control->strong);
    Leave(control); return true;
}
std::uint32_t __fastcall AddWeak(Control* control,void*) noexcept
{
    if (!Enter(control)) return 1;
    const auto weak=static_cast<std::uint32_t>(control->weak);
    if (!weak || weak==UINT32_MAX) Fault(control);
    else { InterlockedIncrement(&control->weak); if (static_cast<std::uint32_t>(control->weak) > 0xFFFF) Fault(control); }
    Leave(control); return 1;
}
bool __fastcall ReleaseWeak(Control* control,void*) noexcept
{
    if (!Enter(control)) return true;
    if (static_cast<std::uint32_t>(control->weak) <= 1) Fault(control); else InterlockedDecrement(&control->weak);
    Leave(control); return true;
}
void BuildTables(std::uint8_t* image) noexcept
{
    const auto* base_col = *reinterpret_cast<Locator**>(image+0x782754);
    g_type.vtable = *static_cast<void**>(base_col->type);
    constexpr char type_name[]=".?AVVirtualInputDevice@IsaacCsT0@@";
    std::memcpy(g_type.name,type_name,sizeof(type_name));
    g_self = {&g_type,1,0,-1,0,0x40,&g_hierarchy};
    g_bases = {&g_self,base_col->hierarchy->bases[0]};
    g_hierarchy = {0,0,2,g_bases.data()};
    g_locator = {0,0,0,&g_type,&g_hierarchy};
    g_table.locator = &g_locator;
    std::memcpy(g_table.slots.data(),image+0x782758,sizeof(g_table.slots));
    g_table.slots[0] = reinterpret_cast<void*>(&ObjectDestroy);
    g_table.slots[3] = reinterpret_cast<void*>(&Name);
    g_table.slots[12] = reinterpret_cast<void*>(&Pressed);
    g_table.slots[13] = reinterpret_cast<void*>(&Triggered);
    g_table.slots[14] = reinterpret_cast<void*>(&Scalar);
    g_table.slots[30] = reinterpret_cast<void*>(&Poll);
    g_control_table = {reinterpret_cast<void*>(&ControlDestroy),reinterpret_cast<void*>(&TryStrong),
        reinterpret_cast<void*>(&AddStrong),reinterpret_cast<void*>(&ReleaseStrong),
        reinterpret_cast<void*>(&AddWeak),reinterpret_cast<void*>(&ReleaseWeak)};
}
using Allocate = void*(__cdecl*)(std::size_t);
using Ctor = void*(__thiscall*)(void*);
using Init = bool(__thiscall*)(void*,void*);
using PairOperation = void(__thiscall*)(void*,void*,void*);
}
bool OwnedVirtualDevice::Create(std::uint8_t* image,HMODULE module,DWORD main_thread,
                                OwnedRead reader,void* context) noexcept
{
    if (attempted_) return false;
    attempted_ = true; image_ = image; main_thread_ = main_thread;
    if (GetCurrentThreadId()!=main_thread || !image || !module || !reader) {error_=1;return false;}
    HMODULE pinned = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Name),&pinned) || pinned!=module) {error_=2;return false;}
    pinned_=true;
    constexpr std::uint8_t ctor_start[]{0x55,0x8B,0xEC,0x6A,0xFF};
    constexpr std::uint8_t init_start[]{0x55,0x8B,0xEC,0x83,0xE4,0xF8,0x56,0x8B,0xF1,0x57,0x8B,0x06,0xFF,0x50,0x14};
    if(std::memcmp(image+0x61E050,ctor_start,sizeof(ctor_start)) ||
       std::memcmp(image+0xCC10,ctor_start,sizeof(ctor_start)) ||
       std::memcmp(image+0x61E210,init_start,sizeof(init_start))) {error_=3;return false;}
    // Successful Init is within this implementation's declared normal-allocation scope.
    // Validate the template before Init (which internally calls slot5 and allocates).
    const auto begin = *reinterpret_cast<std::uintptr_t*>(image+0x8799E8);
    const auto end = *reinterpret_cast<std::uintptr_t*>(image+0x8799EC);
    if (!begin || end<begin || (end-begin)!=32*16) {error_=3;return false;}
    BuildTables(image);
    auto allocate = reinterpret_cast<Allocate>(image+0x60F4C0);
    void* raw_object=allocate(sizeof(Object));
    if (!raw_object) {error_=4;return false;}
    if (reinterpret_cast<std::uintptr_t>(raw_object)&3u) {error_=4;return false;}
    auto* object=::new(raw_object) Object;
    state_=DeviceState::RawAllocated;
    // No this escape occurs before this point. After ctor starts, NEVER applies.
    state_=DeviceState::BaseConstructed;
    reinterpret_cast<Ctor>(image+0x61E050)(object);
    pair_.object = object;
    object->reader=reader;object->context=context;object->neutral=0;
    *reinterpret_cast<void***>(object)=g_table.slots.data();
    if (!reinterpret_cast<Init>(image+0x61E210)(object,nullptr)) {error_=5;Neutralize();return false;}
    const auto* bytes = object->base;
    const auto n = (*reinterpret_cast<const std::uintptr_t*>(bytes+0x14)-
                    *reinterpret_cast<const std::uintptr_t*>(bytes+0x10))/16;
    if (n!=32 || !*reinterpret_cast<void* const*>(bytes+0xC0) ||
        !*reinterpret_cast<void* const*>(bytes+0xC4)) {error_=6;Neutralize();return false;}
    key_=*reinterpret_cast<const std::int32_t*>(bytes+0x0C);
    state_=DeviceState::Initialized;
    if (key_<=0) {error_=7;Neutralize();return false;}
    void* raw_control=allocate(sizeof(Control));
    if (!raw_control) {error_=8;Neutralize();return false;}
    if (reinterpret_cast<std::uintptr_t>(raw_control)&3u) {error_=8;Neutralize();return false;}
    auto* control=::new(raw_control) Control;
    reinterpret_cast<Ctor>(image+0xCC10)(control);
    control->strong=1;control->weak=1;control->busy=0;control->fault=0;control->object=object;
    *reinterpret_cast<void***>(control)=g_control_table.data();
    *reinterpret_cast<void**>(control->base+0x14)=object;
    pair_.control=control;
    if (!*reinterpret_cast<void**>(control->base+0x10)) {error_=9;Neutralize();return false;}
    state_=DeviceState::ControlReady;
    return true;
}
bool OwnedVirtualDevice::MemberCount(std::uint32_t& exact,std::uint32_t& same_key) const noexcept
{
    exact=0;same_key=0;
    __try {
        auto* manager=image_+0x857B18;
        auto* begin=*reinterpret_cast<DevicePair**>(manager+8);
        auto* end=*reinterpret_cast<DevicePair**>(manager+12);
        const auto first=reinterpret_cast<std::uintptr_t>(begin),last=reinterpret_cast<std::uintptr_t>(end);
        if (last<first || (last-first)%8 || (last-first)>8*32 || (!begin && end)) return false;
        for (auto* p=begin;p!=end;++p) {
            if (!p->object || !p->control) return false;
            if (p->object==pair_.object && p->control==pair_.control) ++exact;
            if (*reinterpret_cast<std::int32_t*>(static_cast<std::uint8_t*>(p->object)+12)==key_) ++same_key;
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool OwnedVirtualDevice::ExactMember() const noexcept
{
    std::uint32_t exact=0,key=0;return MemberCount(exact,key)&&exact==1&&key==1;
}
bool OwnedVirtualDevice::Healthy() const noexcept
{
    auto* control=static_cast<Control*>(pair_.control);
    auto* object=static_cast<Object*>(pair_.object);
    return object && control && !InterlockedCompareExchange(&object->neutral,0,0) &&
        !InterlockedCompareExchange(&control->fault,0,0);
}
std::uint32_t OwnedVirtualDevice::Strong() const noexcept
{
    auto* control=static_cast<Control*>(pair_.control);
    return control?static_cast<std::uint32_t>(InterlockedCompareExchange(&control->strong,0,0)):0;
}
bool OwnedVirtualDevice::Register() noexcept
{
    if (GetCurrentThreadId()!=main_thread_ || state_!=DeviceState::ControlReady) return false;
    std::uint32_t exact=0,key=0;
    if (!MemberCount(exact,key) || exact || key) {error_=10;Neutralize();return false;}
    // Native consumes the by-value argument pair. Give it one strong, distinct from root.
    AddStrong(static_cast<Control*>(pair_.control),nullptr);
    reinterpret_cast<PairOperation>(image_+0x620DF0)(image_+0x857B18,pair_.object,pair_.control);
    if (!ExactMember() || !Healthy()) {error_=11;Neutralize();return false;}
    state_=DeviceState::Registered;return true;
}
void OwnedVirtualDevice::Neutralize() noexcept
{
    if (pair_.object) InterlockedExchange(&static_cast<Object*>(pair_.object)->neutral,1);
    if (state_>=DeviceState::ControlReady && state_<DeviceState::Unregistered) state_=DeviceState::Retiring;
}
bool OwnedVirtualDevice::Remove() noexcept
{
    Neutralize();
    if (GetCurrentThreadId()!=main_thread_ || !pair_.control) return false;
    std::uint32_t exact=0,key=0;
    if (!MemberCount(exact,key)) return false;
    if (exact==1 && key==1 && !removal_attempted_) {
        removal_attempted_=true;
        AddStrong(static_cast<Control*>(pair_.control),nullptr);
        reinterpret_cast<PairOperation>(image_+0x620EB0)(image_+0x857B18,pair_.object,pair_.control);
    } else if (exact || key) return false;
    if (!MemberCount(exact,key) || exact || key) return false;
    // Native consumed vector ownership. Root remains permanently held.
    state_=DeviceState::Unregistered;return true;
}
} // namespace isaac_cs_t0::coop_active
