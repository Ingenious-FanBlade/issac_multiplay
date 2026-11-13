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
  // 调用者提供当前 BinaryPreparation 分派；每次只进入有限个独立原生承载。
  ClientPreparationResult Stage(unsigned maximum) noexcept;
  bool Ready()const noexcept;
  bool HasNative()const noexcept;
  bool Drain() noexcept;
  using Visitor=bool (*)(void *,std::uint32_t,const ClientStagedFieldInput &,ClientStagedField *) noexcept;
  bool Visit(void *,Visitor) noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace cs::game
