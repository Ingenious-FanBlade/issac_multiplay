#pragma once
#include <Windows.h>
#include <array>
#include <cstdint>

// Private acceptance instrumentation. Not part of ISCS, Lua, coop or world v2.
// The production build compiles all call sites out.
namespace isaac_cs_t0::world::e2e {
inline constexpr std::uint64_t kMagic=0x3145324557534349ull;
inline constexpr std::uint32_t kCapacity=65536;
enum class Kind : std::uint32_t { Stage=1,Commit,Entry,Original,Returned,Fx,
    CopyBegin,CopyEnd,Read,Ready,Anchor,Lifecycle,Fault,Error,TopExit,Rng,GlobalRng,Arguments,
    Window,ReferenceFault,DiagnosticDelivery };
enum class Fault : LONG { None,ObjectBudget,MembershipBudget,RelationBudget,
    FieldRead,SourceChanged,EventOverflow,WrongThread,ReferenceAmbiguity,RootChanged };
struct alignas(8) Event {
    volatile LONG64 published;
    std::uint64_t outer,tick,attempt,subject,use,qpc;
    std::uint32_t kind,tag,thread,count;
    std::array<std::uint32_t,14> words;
};
struct alignas(8) Shared {
    std::uint64_t magic;
    std::uint32_t version,size,pid,controller;
    std::array<std::uint8_t,32> nonce;
    volatile LONG command,ack,error,closed;
    volatile LONG64 produced,consumed,overflow;
    std::uint64_t frequency;
    volatile LONG worker_main,worker_thread,worker_result;
    std::array<std::uint8_t,12> reserved;
    std::array<Event,kCapacity> events;
};
static_assert(sizeof(Event)==128 && offsetof(Shared,events)==128);
#if defined(ISAAC_CS_WORLD_E2E)
bool Prepare(const wchar_t* world_name,std::uint32_t pid,std::uint32_t controller,
             const std::array<std::uint8_t,32>& nonce) noexcept;
void Context(std::uint64_t outer,std::uint64_t tick,std::uint64_t attempt) noexcept;
void Record(Kind kind,std::uint32_t tag,const std::uint32_t* words=nullptr,
            std::uint32_t count=0,std::uint64_t subject=0,std::uint64_t use=0) noexcept;
void Fx(std::uint32_t hook,std::uint32_t phase,const std::uint8_t* fx) noexcept;
void BeginCopy() noexcept;
void EndCopy() noexcept;
bool Copying() noexcept;
bool Take(Fault fault) noexcept;
bool Pending(Fault fault) noexcept;
void Worker(std::uint32_t main,std::uint32_t thread,bool accepted) noexcept;
std::uint32_t ReadFaultAddress() noexcept;
void Close() noexcept;
#else
inline bool Prepare(const wchar_t*,std::uint32_t,std::uint32_t,const std::array<std::uint8_t,32>&) noexcept {return true;}
inline void Context(std::uint64_t,std::uint64_t,std::uint64_t) noexcept {}
inline void Record(Kind,std::uint32_t,const std::uint32_t* =nullptr,std::uint32_t =0,std::uint64_t =0,std::uint64_t =0) noexcept {}
inline void Fx(std::uint32_t,std::uint32_t,const std::uint8_t*) noexcept {}
inline void BeginCopy() noexcept {}
inline void EndCopy() noexcept {}
inline bool Copying() noexcept {return false;}
inline bool Take(Fault) noexcept {return false;}
inline bool Pending(Fault) noexcept {return false;}
inline void Worker(std::uint32_t,std::uint32_t,bool) noexcept {}
inline std::uint32_t ReadFaultAddress() noexcept {return 0;}
inline void Close() noexcept {}
#endif
}
