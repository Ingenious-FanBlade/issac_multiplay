#pragma once
#include "state/client_plan.h"
#include "game/client/scene/client_field_staging_native.h"

namespace cs::game {
// 主线程数字索引把同一 root 的投影归并；只消费后台已定界字段／叶，不重新解码正文。
class ClientSceneFieldStaging final {
public:
  ClientSceneFieldStaging(std::shared_ptr<state::ClientStateMemory>,ClientPreparationAllocator);
  ~ClientSceneFieldStaging();
  ClientSceneFieldStaging(const ClientSceneFieldStaging &)=delete;
  ClientSceneFieldStaging &operator=(const ClientSceneFieldStaging &)=delete;
  // 只在同一主线程转交整个owner；原生节点、分配账目及真实析构责任一起移动。
  ClientSceneFieldStaging(ClientSceneFieldStaging &&) noexcept;
  ClientSceneFieldStaging &operator=(ClientSceneFieldStaging &&) noexcept;
  void Describe(const state::ClientStateInstallPlan &);
  // 借用已提交的同一根实例上的容器；校验根实例由场景保管 owner 提供。
  using SameObject=bool (*)(const void *,std::uint32_t,std::uint32_t) noexcept;
  void ReuseFrom(const ClientSceneFieldStaging &,const void *,SameObject) noexcept;
  void SkipObjects(const void *,bool (*skip)(const void *,std::uint32_t) noexcept) noexcept;
  // 调用者提供当前 BinaryPreparation 分派；一次登记本帧的全部承载。
  ClientPreparationResult Stage() noexcept;
  bool HasNative()const noexcept;
  bool Drain() noexcept;
  using Visitor=bool (*)(void *,std::uint32_t,const ClientStagedFieldInput &,ClientStagedField *) noexcept;
  using Continue=bool (*)(const void *) noexcept;
  bool Visit(void *,Visitor,const void *window,Continue,std::uint64_t ticket,const char *phase) noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace cs::game
