#include "world/domains/grid/collect.h"
#include "world/compatibility/min_read_v1.h"
#include "binding/world_layout.h"
namespace cs::world::grid {
const observation::DomainDescriptor &DescribeDomain() noexcept {
  static const observation::DomainDescriptor value{observation::DomainId::Grid,"isaac.cs.grid.legacy",2,
    "min-read-v1:public-grid-and-door-only;full-grid:tx-04-11",observation::SupportedPhases(observation::DomainId::Grid),{}};
  return value;
}
bool ObservationFor(Observation kind) noexcept {
  return kind==Observation::GridFactory||kind==Observation::GridInit||kind==Observation::GridSpawn||kind==Observation::Door||kind==Observation::GridFree;
}
bool CreationOwner(Observation kind) noexcept { return kind==Observation::GridSpawn||kind==Observation::Door; }
bool OwnsSource(std::uint32_t source) noexcept { return source==11||source==12; }
std::uint32_t FixedSourceOffset(std::uint32_t source) noexcept { return source==11?4:452; }
// Grid 与 Door 使用各自实际槽和索引表，公共容器读取器不解释这些成员语义。
void Enumerate(const NativeSource &reader,const SubjectRegistry &registry,std::uint32_t source,RawSource &out,
               std::span<std::uint32_t> scratch,std::span<std::uint32_t> fixed) noexcept {
  const auto offset=FixedSourceOffset(source);
  struct State { const NativeSource &reader; const SubjectRegistry &registry; RawSource &out; std::uint32_t source; } state{reader,registry,out,source};
  EnumerateSource(reader,source,out,scratch,fixed.subspan(offset),&state,[](void *owner,std::uint32_t address,std::uint32_t index) noexcept {
    auto &s=*static_cast<State *>(owner);
    auto *object=s.registry.add(s.registry.owner,ObjectDomain::Grid,address,s.source,index);
    if(!object) return;
    const auto *spec=layout::FindSource(s.source);
    if(s.source==12&&(!spec||!s.reader.Value(std::uint64_t(s.reader.room)+spec->index_map_offset+index*4,index)||index>=layout::LayoutConstants().room_grid_slot_count)) {
      s.out.status=unsigned(Reason::NotReady); s.reader.Fail(Reason::NotReady); return;
    }
    if(object->grid_index!=kNoGridIndex&&object->grid_index!=index) {
      s.out.status=unsigned(Reason::SourceChanged); s.reader.Fail(Reason::SourceChanged);
    }
    object->grid_index=index;
  });
}
// 保留实际槽位、初始化字节和原生 index 的三方一致性；不从请求变体推导实际结果。
void CollectNative(const NativeSource &source,RawObject &out,Identity &id) noexcept {
  bool base=false;
  const auto kind=ClassifyNative(source,ObjectDomain::Grid,out.address,out.vptr,base);
  out.kind=unsigned(kind);
  if(kind==Class::Unknown) { source.Fail(Reason::UnknownClass); return; }
  const auto *initialized=layout::FindField(0x0807), *index_spec=layout::FindField(0x0809);
  std::uint8_t ready=0;
  std::uint32_t index=kNoGridIndex;
  if(!initialized||!index_spec||!source.Value(std::uint64_t(out.address)+initialized->offset,ready)||
     !source.Value(std::uint64_t(out.address)+index_spec->offset,index)) {
    source.Fail(Reason::ReadFailed); compatibility::CopyNativeFields(source,out,id,false); return;
  }
  const bool usable=ready&&index<layout::LayoutConstants().room_grid_slot_count&&index==out.grid_index;
  if(!usable) source.Fail(Reason::NotReady);
  compatibility::CopyNativeFields(source,out,id,usable);
}
}
