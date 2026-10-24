#include "isaac_cs_t0/world_e2e.hpp"
#if defined(ISAAC_CS_WORLD_E2E)
#include <algorithm>
#include <cstring>

namespace isaac_cs_t0::world::e2e {
namespace {
Shared* g_probe=nullptr;
HANDLE g_mapping=nullptr;
volatile LONG g_writer=0;
void* g_read_fault_page=nullptr;
thread_local std::uint64_t g_outer=0,g_tick=0,g_attempt=0;
thread_local bool g_copying=false;
thread_local std::uint64_t g_reads=0,g_bytes=0;
thread_local std::uint32_t g_read_samples=0;
}
bool Prepare(const wchar_t* name,std::uint32_t pid,std::uint32_t controller,
             const std::array<std::uint8_t,32>& nonce) noexcept
{
    wchar_t probe_name[512]{};
    const auto length=wcsnlen_s(name,480);
    if(!length || length>=480)return false;
    std::memcpy(probe_name,name,length*sizeof(wchar_t));
    constexpr wchar_t suffix[]=L".E2E";
    std::memcpy(probe_name+length,suffix,sizeof(suffix));
    g_mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,probe_name);
    if(!g_mapping)return false; // E2E binary cannot silently run without evidence.
    g_probe=static_cast<Shared*>(MapViewOfFile(g_mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared)));
    if(!g_probe || g_probe->magic!=kMagic || g_probe->version!=1 || g_probe->size!=sizeof(Shared) ||
       g_probe->pid!=pid || g_probe->controller!=controller || g_probe->nonce!=nonce ||
       g_probe->produced || g_probe->consumed || g_probe->command || g_probe->closed)return false;
    g_read_fault_page=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    if(!g_read_fault_page)return false;
    const auto page=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(g_read_fault_page));
    std::memcpy(g_probe->reserved.data(),&page,4);
    LARGE_INTEGER frequency{};
    if(!QueryPerformanceFrequency(&frequency))return false;
    g_probe->frequency=static_cast<std::uint64_t>(frequency.QuadPart);
    return true;
}
void Context(std::uint64_t outer,std::uint64_t tick,std::uint64_t attempt) noexcept
{g_outer=outer;g_tick=tick;g_attempt=attempt;}
void Record(Kind kind,std::uint32_t tag,const std::uint32_t* words,
            std::uint32_t count,std::uint64_t subject,std::uint64_t use) noexcept
{
    if(!g_probe || g_probe->closed)return;
    const auto thread=GetCurrentThreadId();
    const auto writer=InterlockedCompareExchange(&g_writer,static_cast<LONG>(thread),0);
    if(writer && writer!=static_cast<LONG>(thread))return; // Fault worker writes only the observer's atomic error.
    // The Read record is a bounded address/width witness; totals include every
    // literal read in a copy. Never compute a digest of game memory.
    if(kind==Kind::Read && g_copying) {
        ++g_reads;g_bytes+=count?words[1]:0;
        if(g_read_samples++>=16)return;
    }
    const auto produced=InterlockedCompareExchange64(&g_probe->produced,0,0);
    const auto consumed=InterlockedCompareExchange64(&g_probe->consumed,0,0);
    if(produced<consumed || produced==INT64_MAX || produced-consumed>=kCapacity) {
        InterlockedIncrement64(&g_probe->overflow);
        InterlockedCompareExchange(&g_probe->error,1,0);return;
    }
    auto& event=g_probe->events[static_cast<std::size_t>(produced)%kCapacity];
    event.outer=g_outer;event.tick=g_tick;event.attempt=g_attempt;
    event.subject=subject;event.use=use;
    LARGE_INTEGER now{};QueryPerformanceCounter(&now);event.qpc=static_cast<std::uint64_t>(now.QuadPart);
    event.kind=static_cast<std::uint32_t>(kind);event.tag=tag;
    event.thread=GetCurrentThreadId();event.count=(std::min)(count,14u);event.words.fill(0);
    if(event.count)std::memcpy(event.words.data(),words,event.count*4);
    MemoryBarrier();InterlockedExchange64(&event.published,produced+1);
    InterlockedExchange64(&g_probe->produced,produced+1);
}
void Fx(std::uint32_t hook,std::uint32_t phase,const std::uint8_t* fx) noexcept
{
    for(std::uint32_t offset=0;offset<512;offset+=48) {
        std::uint32_t words[14]{phase,offset};
        std::memcpy(words+2,fx+offset,(std::min)(48u,512u-offset));
        Record(Kind::Fx,hook,words,14);
    }
}
void BeginCopy() noexcept
{g_copying=true;g_reads=0;g_bytes=0;g_read_samples=0;Record(Kind::CopyBegin,0);}
void EndCopy() noexcept
{
    std::uint32_t words[]{static_cast<std::uint32_t>(g_reads),static_cast<std::uint32_t>(g_reads>>32),
        static_cast<std::uint32_t>(g_bytes),static_cast<std::uint32_t>(g_bytes>>32)};
    Record(Kind::CopyEnd,0,words,4);g_copying=false;
}
bool Copying() noexcept {return g_copying;}
bool Pending(Fault fault) noexcept
{return g_probe && !g_probe->closed && InterlockedCompareExchange(&g_probe->command,0,0)==static_cast<LONG>(fault);}
bool Take(Fault fault) noexcept
{
    if(!g_probe || InterlockedCompareExchange(&g_probe->command,0,0)!=static_cast<LONG>(fault))return false;
    InterlockedExchange(&g_probe->command,0);InterlockedExchange(&g_probe->ack,static_cast<LONG>(fault));
    const std::uint32_t words[]{static_cast<std::uint32_t>(fault)};
    Record(Kind::Fault,static_cast<std::uint32_t>(fault),words,1);return true;
}
void Close() noexcept {if(g_probe)InterlockedExchange(&g_probe->closed,1);}
void Worker(std::uint32_t main,std::uint32_t thread,bool accepted) noexcept {
    if(!g_probe)return;
    InterlockedExchange(&g_probe->worker_main,static_cast<LONG>(main));
    InterlockedExchange(&g_probe->worker_thread,static_cast<LONG>(thread));
    InterlockedExchange(&g_probe->worker_result,accepted?1:0);
}
std::uint32_t ReadFaultAddress() noexcept {return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(g_read_fault_page));}
}
#endif
