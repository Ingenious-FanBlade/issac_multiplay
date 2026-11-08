#pragma once
#include "core/value_types.h"
#include "core/deferred_values.h"
#include <vector>

namespace cs::capture {
// 封闭构造后只暴露const纯值；不包含Lua状态、userdata、借用字符串或游戏对象。
class OwnedSource final {
public:
  OwnedSource():allocator_(true),value_(&allocator_),metadata_(memory::Reserve(memory::Pool::Values,sizeof(OwnedSource)+128,"capture_source_owner")){}
  OwnedSource(const OwnedSource &)=delete;
  OwnedSource &operator=(const OwnedSource &)=delete;
  template<class Build> static std::shared_ptr<const OwnedSource> Create(Build build){
    auto source=memory::DeferredValue<OwnedSource>();build(source->value_,source->bytes_);return source;
  }
  template<class Build> static std::shared_ptr<const OwnedSource> CreateGraph(Build build,bool concatenate=false){
    auto source=memory::DeferredValue<OwnedSource>();source->concatenate_=concatenate;
    source->metadata_->Resize(sizeof(OwnedSource)+128+4096*sizeof(std::shared_ptr<const OwnedSource>));
    build(source->value_,source->bytes_,source->nodes_,source->blocks_);
    source->metadata_->Resize(sizeof(OwnedSource)+128+source->blocks_.capacity()*sizeof(std::shared_ptr<const OwnedSource>));return source;
  }
  const replica::Json &Read() const noexcept {return value_;}
  std::uint64_t Bytes() const noexcept {return bytes_;}
  std::uint64_t Nodes() const noexcept {return nodes_;}
  const auto &Blocks() const noexcept {return blocks_;}
  bool Concatenate() const noexcept {return concatenate_;}
private:
  replica::Allocator allocator_;
  replica::Json value_;
  memory::Lease metadata_;
  std::uint64_t bytes_=0;
  std::uint64_t nodes_=0;
  std::vector<std::shared_ptr<const OwnedSource>> blocks_;
  bool concatenate_=false;
};
using Source=std::shared_ptr<const OwnedSource>;
// 仅诊断编码使用；正文来自已经取得的纯值块，不能补调原生读取。
std::string SourceText(const Source &);
bool EqualSources(const Source &,const Source &);
}
