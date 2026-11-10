#pragma once
#include "game/client/grid/client_grid_native.h"
#include "replica/room_grid_plan.h"

namespace cs::game {
class ClientObjects;
// Grid 域独立执行器复用 I05 的计划/Key；I25 的场景提交是唯一整目标提交点。
class ClientGrids final {
public:
  ClientGrids() noexcept=default;
  ~ClientGrids();
  bool PrepareTarget(const replica::RoomGridPlan &,const char *&reason) noexcept;
  bool CancelPlanned() noexcept;
  ClientGridStep Step(const ClientNativeRoots &,unsigned budget=1) noexcept;
  ClientGridStep Retire(const ClientNativeRoots &) noexcept;
  bool Visit(void *context,ClientGridVisitor visitor) const noexcept;
#ifdef ISAACCS_E2E
  std::string TestInspect(const ClientNativeRoots &) const noexcept;
#endif
  // 全部实例登记后解析依赖，允许合法环；不跨本次调用保存 Entity 裸指针。
  ClientGridStep ResolveReferences(const ClientObjects &,std::uint64_t attempt) noexcept;
  using LiftedVisitor=bool (*)(void *,const replica::LiftedGridTarget &) noexcept;
  bool VisitLifted(void *,LiftedVisitor) const noexcept;
  ClientGridStep Check(const ClientNativeRoots &) noexcept;
  // 门目标或原生容器布局变化必须撤销旧借用，交结构重建统一安装。
  bool StreamCompatible(const replica::RoomGridTarget &) const noexcept;
  std::vector<LegacyClientGridDescription> CompileStream(const replica::RoomGridTarget &) const;
  ClientGridStep InstallStream(const ClientNativeRoots &,const std::vector<LegacyClientGridDescription> &) noexcept;
  std::uint64_t StreamValueBytes() const;
private:
  ClientNativeGrids *native_=nullptr;
  std::vector<LegacyClientGridCommand> commands_;
  // 不可变T只解析一次；最终检查分步消费缓存，下一目标/退休立即使旧缓存失效。
  std::vector<LegacyClientGridCommand> checks_,planned_checks_;
  std::array<ClientGridDoorBinding,8> doors_{};
  std::shared_ptr<const replica::RoomGridTarget> target_,committed_;
  std::string reason_;
  std::size_t cursor_=0;
  std::size_t check_cursor_=0;
  bool written_=false,failed_=false,doors_bound_=false,retired_=true,native_started_=false;
};
} // namespace cs::game
