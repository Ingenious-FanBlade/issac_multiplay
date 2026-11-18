#pragma once
#include "world/common/native_source.h"
#include <span>
namespace cs::world {
// 主体登记、去重和计数仍归协调器；域只取得本批值视图，不取得 Collector 可写状态。
struct SubjectRegistry final {
  void *owner=nullptr;
  RawObject *(*add)(void *,ObjectDomain,std::uint32_t,std::uint32_t,std::uint32_t) noexcept=nullptr;
  std::uint32_t (*count)(void *) noexcept=nullptr;
  const RawObject *(*object)(void *,std::uint32_t) noexcept=nullptr;
  const Identity *(*identity)(void *,ObjectDomain,std::uint32_t) noexcept=nullptr;
};
using MembershipSink=void (*)(void *,std::uint32_t,std::uint32_t) noexcept;
// 公共层仅复制有界容器及两遍一致性；成员属于 Entity 还是 Grid 由域回调解释。
void EnumerateSource(const NativeSource &,std::uint32_t,RawSource &,
                     std::span<std::uint32_t> scratch,std::span<std::uint32_t> fixed,
                     void *owner,MembershipSink,bool final_review_elsewhere=false) noexcept;
}
