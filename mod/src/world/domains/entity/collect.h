#pragma once
#include "world/common/native_source.h"
#include "world/common/contracts.h"
#include "world/common/membership.h"
namespace cs::world::entity {
const observation::DomainDescriptor &DescribeDomain() noexcept;
void CollectNative(const NativeSource &,RawObject &,Identity &) noexcept;
bool CreationOwner(Observation) noexcept;
bool Morph(Observation) noexcept;
bool OwnsSource(std::uint32_t) noexcept;
std::uint32_t FixedSourceOffset(std::uint32_t) noexcept;
void Enumerate(const NativeSource &,const SubjectRegistry &,std::uint32_t,RawSource &,
               std::span<std::uint32_t>,std::span<std::uint32_t>) noexcept;
void CollectRelations(const NativeSource &,const SubjectRegistry &,RawSource &,std::span<std::uint32_t>) noexcept;
}
