#pragma once
#include <rapidjson/document.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdio>
#include "core/replication_memory.h"

namespace cs::replica_detail {
inline constexpr std::uint64_t kJsonArenaChunk=4096;
// 分配器失败时保留实际chunk请求；固定缓冲不在OOM现场再分配诊断字符串。
class ArenaAllocationFailure final : public std::bad_alloc {
public:
  ArenaAllocationFailure(std::size_t requested,std::uint64_t held) noexcept {
    std::snprintf(reason_,sizeof(reason_),"json_arena_allocation:requested=%llu:held=%llu",
      static_cast<unsigned long long>(requested),static_cast<unsigned long long>(held));
  }
  const char *what() const noexcept override {return reason_;}
private:
  char reason_[128]{};
};
// 仅在arena真正新增chunk时预付实际分配（包含RapidJSON的chunk头），不逐标量扫描整条chunk链。
// 租约与chunk分配器同寿命；64KiB预付余量始终占用原Values额度。
class ArenaChunks final {
public:
  explicit ArenaChunks(bool tracked=false):charge_(tracked?memory::Reserve(memory::Pool::Values,0,"capture_owned_source"):nullptr){}
  void *Malloc(std::size_t n){
    if(charge_){const auto need=allocated_+n;if(need>charge_->Bytes())charge_->Resize(((need+65535)/65536)*65536);}
    auto *out=std::malloc(n);if(!out&&n)throw ArenaAllocationFailure(n,allocated_);if(out)allocated_+=n;return out;
  }
  static void Free(void *p){std::free(p);}
  void Reset(){allocated_=0;if(charge_)charge_->Resize(0);}
private:
  memory::Lease charge_;
  std::uint64_t allocated_=0;
};
// SDK与纯值worker共用C++17可见的DOM类型。普通DOM仍由既有外层租约承担。
// pool先析构、chunks后析构，所有chunk释放之前不会退还来源租约。
class Allocator final {
  using Base=rapidjson::MemoryPoolAllocator<ArenaChunks>;
public:
  static constexpr bool kNeedFree=false;
  explicit Allocator(bool tracked=false):chunks_(tracked),pool_(std::size_t(kJsonArenaChunk),&chunks_){}
  Allocator(const Allocator &)=delete;Allocator &operator=(const Allocator &)=delete;
  void *Malloc(std::size_t n){return pool_.Malloc(n);}
  void *Realloc(void *p,std::size_t old,std::size_t n){return pool_.Realloc(p,old,n);}
  static void Free(void *){}
  void Clear(){pool_.Clear();chunks_.Reset();}
  std::size_t Capacity() const{return pool_.Capacity();}
  std::size_t Size() const{return pool_.Size();}
private:
  ArenaChunks chunks_;
  Base pool_;
};
}
namespace cs::replica {
inline constexpr auto kJsonArenaChunk=replica_detail::kJsonArenaChunk;
using Allocator=replica_detail::Allocator;
using Value=rapidjson::GenericValue<rapidjson::UTF8<>,Allocator>;
using Json=rapidjson::GenericDocument<rapidjson::UTF8<>,Allocator>;
// SDK版本没有MemberReserve；通过SAX只建立空成员槽，再移交同一arena的容量。
// 不复制子树、不借用外部字符串，避免每个两三成员对象默认占用16个槽。
inline Value SizedObject(rapidjson::SizeType capacity,Allocator &allocator){
  if(!capacity)return Value(rapidjson::kObjectType);
  // 小对象复用不可变的空槽原型；Value复制构造只分配MemberCount个成员。
  // 避免每个两三字段对象都另建Document及其临时SAX堆栈。
  if(capacity<=16){
    static const auto shapes=[](){
      std::array<Json,16> values;
      for(unsigned i=0;i<values.size();++i){auto &v=values[i];v.SetObject();
        for(unsigned j=0;j<=i;++j){Value key(rapidjson::StringRef("")),empty;v.AddMember(key,empty,v.GetAllocator());}}
      return values;
    }();
    Value out(shapes[capacity-1],allocator);out.RemoveAllMembers();return out;
  }
  Json shell(&allocator);
  auto generate=[&](auto &handler){
    handler.StartObject();
    for(rapidjson::SizeType i=0;i<capacity;++i){handler.Key("",0,false);handler.Null();}
    return handler.EndObject(capacity);
  };
  shell.Populate(generate);shell.RemoveAllMembers();return Value(std::move(static_cast<Value &>(shell)));
}
}
