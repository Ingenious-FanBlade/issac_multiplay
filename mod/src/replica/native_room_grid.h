#pragma once
#include "replica/model.h"
#include <array>
#include <map>
#include <set>

namespace cs::replica {
// 原生应用只接收15已重建的完整目标。此层不包含SDK、原生地址或绘制资源。
struct NativeRoomGridTarget final {
  Scope scope{};
  std::uint64_t revision=0;
  unsigned shape=0;
  Json model;
  // 对应J273的GridEntityDesc真实值；create后必须覆盖回这些最终值，
  // 不能保留SpawnGridEntityDesc内部增加的SpawnCount/Initialized。
  struct Descriptor final {
    int type=0,variant=0,state=0,spawn_count=0,var_data=0;
    std::uint32_t spawn_seed=0,variable_seed=0;
    bool initialized=false;
  };
  struct Grid final {
    std::uint64_t id=0;
    unsigned index=0,type=0;
    int variant=0;
    int collision=0;
    Descriptor descriptor{};
    std::set<unsigned> door_slots;
    // 保存整个规范对象，不能用State/seed摘要替代派生状态、描述及姿态。
    Json state;
  };
  std::map<std::uint64_t,Grid> grids;
  std::array<std::uint64_t,8> doors{};
  std::array<int,448> collision{},path{},rail{};
  struct Position final {float x=0,y=0;};
  std::array<Position,448> positions{};
  std::set<std::string> external_dependencies;
};

enum class NativeRoomGridDisposition { Apply, Duplicate, Stale };
// 每个change保留逻辑身份；类型/格位改变时由backend重建本地载体。
struct NativeRoomGridPlan final {
  NativeRoomGridDisposition disposition=NativeRoomGridDisposition::Apply;
  bool create_room=false,change_room=false;
  // 原生半途失败时清理整个不可信载体；高水位与同版本内容判定仍使用current。
  bool reset_room=false;
  std::vector<std::uint64_t> remove,create,change,rebuild;
  std::set<unsigned> affected_indices;
};

NativeRoomGridTarget PrepareNativeRoomGrid(const std::string &complete_model);
// current保存最后实际原生应用成功的C及版本凭据。失败后标记native_state_valid=false，
// 不再把C当作当前原生状态，但仍保留它防止恢复期间旧包覆盖或同版本内容冲突。
// B→T的重建仍由15的Reconstruct完成，不能把Delta直接交给本函数。
NativeRoomGridPlan PlanNativeRoomGrid(const NativeRoomGridTarget *current,
                                     const NativeRoomGridTarget &target,
                                     bool native_state_valid=true);
} // namespace cs::replica
