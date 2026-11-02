#pragma once
#include "game/client_grid_native.h"
#include "replica/room_grid_plan.h"

namespace cs::game {
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
private:
  ClientNativeGrids *native_=nullptr;
  std::vector<ClientGridCommand> commands_;
  std::array<ClientGridDoorBinding,8> doors_{};
  std::shared_ptr<const replica::RoomGridTarget> target_,committed_;
  std::string reason_;
  std::size_t cursor_=0;
  bool written_=false,failed_=false,doors_bound_=false,retired_=true,native_started_=false;
};
} // namespace cs::game
