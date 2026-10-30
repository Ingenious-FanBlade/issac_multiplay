#include "world/domains/entity/collect.h"
#include "world/compatibility/min_read_v1.h"
#include "binding/world_layout.h"
namespace cs::world::entity {
const observation::DomainDescriptor &DescribeDomain() noexcept {
  static const observation::DomainDescriptor value{observation::DomainId::Entity,"isaac.cs.entity",2,
    "native:min-read-v1;lua:public-entity-v2",observation::SupportedPhases(observation::DomainId::Entity),{}};
  return value;
}
bool CreationOwner(Observation kind) noexcept {
  return kind==Observation::Spawn||kind==Observation::PlayerCreate||kind==Observation::ManagerRestore;
}
bool Morph(Observation kind) noexcept { return kind==Observation::NpcMorph||kind==Observation::PickupMorph; }
// 身份只由公共身份服务分配；新 Lua 字段使用同一主体，不能给 Player 再分配一次身份。
void CollectNative(const NativeSource &source,RawObject &out,Identity &id) noexcept {
  bool base=false;
  const auto kind=ClassifyNative(source,ObjectDomain::Entity,out.address,out.vptr,base);
  out.kind=unsigned(kind);
  if(kind==Class::Unknown) { source.Fail(Reason::UnknownClass); if(!base) return; }
  compatibility::CopyNativeFields(source,out,id);
}
bool OwnsSource(std::uint32_t source) noexcept { return source<=10; }
std::uint32_t FixedSourceOffset(std::uint32_t) noexcept { return 0; }
void Enumerate(const NativeSource &reader,const SubjectRegistry &registry,std::uint32_t source,RawSource &out,std::span<std::uint32_t> scratch,std::span<std::uint32_t> fixed) noexcept {
  struct State { const SubjectRegistry &registry; std::uint32_t source; } state{registry,source};
  EnumerateSource(reader,source,out,scratch,fixed,&state,[](void *owner,std::uint32_t address,std::uint32_t index) noexcept {
    auto &s=*static_cast<State *>(owner); s.registry.add(s.registry.owner,ObjectDomain::Entity,address,s.source,index);
  });
}
void CollectRelations(const NativeSource &reader,const SubjectRegistry &registry,RawSource &relations,std::span<std::uint32_t> nodes) noexcept {
  // 只从分类且有当前读取依据的 Player 扩展四种关系，64 节点含初始和保留节点。
  std::uint32_t count = 0;
  for (std::uint32_t i = 0; i < registry.count(registry.owner);
       ++i) {
    const auto address = registry.object(registry.owner,i)->address;
    auto *id = registry.identity(registry.owner,ObjectDomain::Entity,address);
    if (!id || registry.object(registry.owner,i)->domain != 0)
      continue;
    std::uint32_t vptr = 0;
    bool base = false;
    if (ClassifyNative(reader,ObjectDomain::Entity,address,vptr,base) != Class::Player)
      continue;
    if (count >= (reader.faults ? reader.faults->relation_limit
                                    : kRelationNodes)) {
      if (auto *f = reader.faults; f && f->mode.load() == 4) {
        f->before.store(f->relation_limit);
        f->after.store(count + 1);
        f->applied.store(1);
      }
      relations.status = static_cast<std::uint32_t>(Reason::RelationBudget);
      reader.Fail(Reason::RelationBudget);
      break;
    }
    nodes[count++] = address;
    if (id->readiness < 2 || id->Retired()) {
      relations.status = static_cast<std::uint32_t>(Reason::NotReady);
      reader.Fail(Reason::NotReady);
      continue;
    }
    for (std::uint16_t f = 0x010d; f <= 0x0110; ++f) {
      const auto *spec = layout::FindField(f);
      std::uint32_t target = 0, after = 0;
      if (!spec || !reader.Value(std::uint64_t(address) + spec->offset, target)) {
        relations.status = static_cast<std::uint32_t>(Reason::ReadFailed);
        reader.Fail(Reason::ReadFailed);
        continue;
      }
      if (target)
        registry.add(registry.owner,ObjectDomain::Entity,target,10,(count-1)*4+f-0x010d);
      if (!reader.Value(std::uint64_t(address) + spec->offset, after) ||
          after != target) {
        relations.status = static_cast<std::uint32_t>(Reason::SourceChanged);
        reader.Fail(Reason::SourceChanged);
      }
    }
  }
  relations.count = count;
}

}
