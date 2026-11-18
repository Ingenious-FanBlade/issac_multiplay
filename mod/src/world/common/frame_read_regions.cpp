#include "world/common/frame_read_regions.h"
#include "core/replication_memory.h"
#include <algorithm>
#include <atomic>
#include <memory>
#include <windows.h>

namespace cs::world {
namespace {
// 页权限可以跨原生调用复用。身份、分配范围和源证言仍由各读取点单独核定。
struct ReadRegionStorage {
  memory::Charge charge{memory::Pool::Values,sizeof(ReadRegionStorage),"frame_read_regions"};
  std::array<NativeReadRegion,1024> regions{};
};
struct FrameReadRegions {
  std::unique_ptr<ReadRegionStorage> storage;
  NativeReadRegion recent{};
  std::size_t used=0;
  ULONG_PTR stack_low=0,stack_high=0;
  std::uint64_t epoch_started=GetTickCount64(),max_epoch_ms=0,high_water=0;
  std::uint64_t inserts=0,retries=0,retry_success=0,faults_fresh=0;
  std::array<std::uint64_t,unsigned(FrameRegionReset::Count)> resets{};
  std::array<NativeReadCounts,unsigned(FrameRegionReader::Count)> readers{};
  void (*first_guard)() noexcept;
  explicit FrameReadRegions(void (*guard)() noexcept):first_guard(guard) {
    // 无法确定栈范围时仍提供受检读取，只关闭跨调用缓存。
    GetCurrentThreadStackLimits(&stack_low,&stack_high);
  }
  void Reset(FrameRegionReset cause) noexcept {
    const auto now=GetTickCount64();
    max_epoch_ms=(std::max)(max_epoch_ms,now-epoch_started);
    epoch_started=now;
    ++resets[unsigned(cause)];
    used=0;recent={};
  }
};
std::atomic<DWORD> main_thread{0};
FrameReadRegions *main_regions=nullptr;

FrameReadRegions *MainRegions() noexcept {
  return GetCurrentThreadId()==main_thread.load(std::memory_order_acquire)?main_regions:nullptr;
}
void Count(FrameReadRegions *table,FrameRegionReader reader,NativeReadCounts *local,
           std::uint64_t NativeReadCounts::*member) noexcept {
  if(table)++(table->readers[unsigned(reader)].*member);
  if(local)++(local->*member);
}
// 新查询与旧条目重叠说明布局曾变化。始终只保留互不重叠的正向可读结论。
bool Find(FrameReadRegions *table,std::uint64_t address,NativeReadRegion &region,FrameRegionReader reader,
          NativeReadCounts *counts,bool &cached) noexcept {
  Count(table,reader,counts,&NativeReadCounts::lookups);
  cached=false;
  if(table&&table->storage){
    if(table->recent.Contains(address)){
      region=table->recent;
      cached=true;
    }else{
      const auto end=table->storage->regions.begin()+table->used;
      const auto at=std::upper_bound(table->storage->regions.begin(),end,address,
        [](auto value,const auto &entry){return value<entry.first;});
      if(at!=table->storage->regions.begin()&&(at-1)->Contains(address)){
        region=table->recent=*(at-1);
        cached=true;
      }
    }
    if(cached){
      Count(table,reader,counts,&NativeReadCounts::hits);
      return true;
    }
  }
  Count(table,reader,counts,&NativeReadCounts::queries);
  if(!QueryNativeReadRegion(address,region))return false;
  if(!table||!table->storage||
      (region.first<table->stack_high&&table->stack_low<region.end))return true;

  auto begin=table->storage->regions.begin();
  auto end=begin+table->used;
  auto at=std::lower_bound(begin,end,region.first,[](const auto &entry,auto value){return entry.first<value;});
  if((at!=begin&&(at-1)->end>region.first)||(at!=end&&at->first<region.end)){
    table->Reset(FrameRegionReset::Overlap);
    at=end=begin;
  }
  if(table->used==table->storage->regions.size()){
    Count(table,reader,counts,&NativeReadCounts::fallbacks);
    return true;
  }
  std::move_backward(at,end,end+1);
  *at=table->recent=region;
  ++table->inserts;
  table->high_water=(std::max)(table->high_water,std::uint64_t(++table->used));
  return true;
}

bool CheckedCopy(std::uint64_t address,void *output,std::size_t size,FrameRegionReader reader,
                 NativeReadCounts *counts,bool *copy_fault,std::size_t *string_size) noexcept {
  if(copy_fault)*copy_fault=false;
  if(!output||!address||address>=kNativeAddressLimit)return false;
  if(string_size)size=(std::min)(size,std::size_t{64});
  else if(size>kNativeAddressLimit-address)return false;
  if(!size)return true;
  auto *table=MainRegions();
  for(unsigned attempt=0;attempt<2;++attempt){
    auto length=size;
    bool used_cached=false;
    // 字符串按当前首区截断。重查时重新核区末尾，避免沿用已过期的块长度。
    for(auto next=address;next<address+length;){
      NativeReadRegion region;
      bool cached=false;
      if(!Find(table,next,region,reader,counts,cached))return false;
      used_cached|=cached;
      if(string_size)length=std::size_t((std::min)(std::uint64_t(length),region.end-address));
      next=region.end;
    }
    const auto fault=CopyNativeReadBytesFault(output,reinterpret_cast<const void *>(std::uintptr_t(address)),length);
    if(!fault){
      if(table&&attempt)++table->retry_success;
      if(string_size)*string_size=length;
      if(copy_fault)*copy_fault=false;
      return true;
    }
    if(copy_fault)*copy_fault=true;
    if(!table)return false;
    if(fault==STATUS_GUARD_PAGE_VIOLATION){
      const bool first=!table->resets[unsigned(FrameRegionReset::Guard)];
      table->storage.reset();
      table->Reset(FrameRegionReset::Guard);
      if(first&&table->first_guard)table->first_guard();
      return false;
    }
    table->Reset(FrameRegionReset::Fault);
    if(!used_cached)++table->faults_fresh;
    if(attempt||!used_cached)return false;
    ++table->retries;
  }
  return false;
}
}

void InitializeFrameReadRegions(void (*first_guard)() noexcept) noexcept {
  if(main_thread.load(std::memory_order_relaxed))return;
  static FrameReadRegions regions(first_guard);
  main_regions=&regions;
  main_thread.store(GetCurrentThreadId(),std::memory_order_release);
  SetFrameReadRegionCache(true);
}
void SetFrameReadRegionCache(bool enabled) noexcept {
  auto *table=MainRegions();
  if(!table)return;
  if(!enabled){
    if(table->storage)table->Reset(FrameRegionReset::Revoke);
    table->storage.reset();
    return;
  }
  if(table->storage||!table->stack_low||table->stack_low>=table->stack_high||
     table->resets[unsigned(FrameRegionReset::Guard)])return;
  try {
    table->storage=std::make_unique<ReadRegionStorage>();
  }catch(...){
    // 公共额度耗尽仍可安全逐次查询，由额度账本记录拒绝。
  }
}
void InvalidateFrameReadRegions(FrameRegionReset cause) noexcept {
  if(auto *table=MainRegions())table->Reset(cause);
}
bool CheckedNativeCopy(std::uint64_t address,void *output,std::size_t size,FrameRegionReader reader,
                       NativeReadCounts *counts,bool *copy_fault) noexcept {
  return CheckedCopy(address,output,size,reader,counts,copy_fault,nullptr);
}
bool CheckedNativeStringBlock(std::uint64_t address,std::span<std::byte> output,std::size_t &size,
                              FrameRegionReader reader,NativeReadCounts *counts) noexcept {
  size=0;
  return !output.empty()&&CheckedCopy(address,output.data(),output.size(),reader,counts,nullptr,&size);
}
std::string FrameReadRegionFields() {
  const auto *table=MainRegions();
  if(!table)return "read_regions.enabled=0\n";
  std::string out;
  const auto add=[&](const std::string &name,std::uint64_t value){
    out+="read_regions."+name+"="+std::to_string(value)+"\n";
  };
  add("enabled",bool(table->storage));
  NativeReadCounts total;
  constexpr const char *readers[]={"read_memory","consumer","topology","roots","classify","capture"};
  static_assert(std::size(readers)==unsigned(FrameRegionReader::Count));
  for(unsigned i=0;i<table->readers.size();++i){
    const auto &reader=table->readers[i];
    total.lookups+=reader.lookups;total.hits+=reader.hits;
    total.queries+=reader.queries;total.fallbacks+=reader.fallbacks;
    const auto prefix=std::string("by_reader.")+readers[i];
    add(prefix+".lookups",reader.lookups);add(prefix+".queries",reader.queries);
  }
  add("lookups",total.lookups);add("hits",total.hits);add("queries",total.queries);
  add("inserts",table->inserts);add("full_fallbacks",total.fallbacks);
  constexpr const char *resets[]={"frame","fault","guard","overlap","revoke"};
  static_assert(std::size(resets)==unsigned(FrameRegionReset::Count));
  for(unsigned i=0;i<table->resets.size();++i)add(std::string("reset.")+resets[i],table->resets[i]);
  add("retries",table->retries);add("retry_success",table->retry_success);add("faults_fresh",table->faults_fresh);
  add("high_water",table->high_water);
  add("max_epoch_ms",(std::max)(table->max_epoch_ms,GetTickCount64()-table->epoch_started));
  return out;
}
}
