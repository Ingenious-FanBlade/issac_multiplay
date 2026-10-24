#include "runtime.h"
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <filesystem>
#include <cstdarg>
#include <cstdio>
namespace cs {
namespace {
struct Event { uint64_t t,seq; char ev[40],detail[3500]; };
Ring<Event,512> events;
std::atomic<uint64_t> sequence{},dropped{};
std::atomic<bool> stopping{};
HANDLE worker=nullptr;
FILE* output=nullptr;
std::string path;
LARGE_INTEGER frequency{};
DWORD WINAPI WriteLoop(void*) noexcept {
    // Serialization and disk writes are confined to this worker.
    Event e{};
    for (;;) {
        if (!events.Pop(e)) { if(stopping) break; Sleep(10); continue; }
        rapidjson::StringBuffer b; rapidjson::Writer<rapidjson::StringBuffer> w(b);
        w.StartObject(); w.Key("t");w.Uint64(e.t);w.Key("seq");w.Uint64(e.seq);
        w.Key("ev");w.String(e.ev);w.Key("detail");w.String(e.detail);w.EndObject();
        if(output) { fwrite(b.GetString(),1,b.GetSize(),output); fputc('\n',output); fflush(output); }
    }
    return 0;
}
}
void StartTrace(const std::string& directory) {
    std::filesystem::create_directories(directory);
    QueryPerformanceFrequency(&frequency);
    path=directory+"/boot-"+std::to_string(GetCurrentProcessId())+".jsonl";
    output=fopen(path.c_str(),"wx");
    if(!output) throw std::runtime_error("trace_open_failed");
    worker=CreateThread(nullptr,0,WriteLoop,nullptr,0,nullptr);
    if(!worker) throw std::runtime_error("trace_thread_failed");
}
void Trace(const char* ev,const char* format,...) noexcept {
    if(!worker) return;
    LARGE_INTEGER now{}; QueryPerformanceCounter(&now);
    Event e{}; e.t=uint64_t(now.QuadPart/frequency.QuadPart)*1000000+uint64_t(now.QuadPart%frequency.QuadPart)*1000000/frequency.QuadPart;
    e.seq=sequence.fetch_add(1)+1; strncpy_s(e.ev,ev,_TRUNCATE);
    va_list args;va_start(args,format);_vsnprintf_s(e.detail,sizeof(e.detail),_TRUNCATE,format,args);va_end(args);
    if(!events.Push(e)) ++dropped;
}
uint64_t TraceDropped() noexcept { return dropped.load(); }
const std::string& TracePath() noexcept { return path; }
void FinishTrace() noexcept {
    stopping=true;
    if(worker) { WaitForSingleObject(worker,2000); CloseHandle(worker); worker=nullptr; }
    if(output) { fclose(output);output=nullptr; }
}
}
