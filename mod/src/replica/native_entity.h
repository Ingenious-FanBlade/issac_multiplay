#pragma once
#include "replica/model.h"
#include <map>
#include <set>
#include <array>

namespace cs::replica {
// 19与18共用完整模型、scope和Registry；Player主体仍留在Entity component中。
// 本结构只拥有纯值，原生指针和资源owner严格留在主线程SDK适配器。
struct NativeEntityTarget final {
  struct Object final {
    std::uint64_t id=0,shape=0;
    std::uint32_t type=0,variant=0,subtype=0,seed=0;
    std::string category;
    bool player=false;
    Json state;
    // 普通关系可后绑；初始化前必须成立的Owner单独排序，不把未知关系当null。
    std::set<std::uint64_t> entities,grids,initialization;
    std::map<std::uint64_t,std::uint64_t> entity_shapes,grid_shapes;
  };
  Scope scope{};
  std::uint64_t revision=0,watermark=0;
  Json component;
  std::map<std::uint64_t,Object> objects;
  std::vector<std::uint64_t> creation_order;
  std::vector<std::uint64_t> render_order;
  std::set<std::uint64_t> player_dependencies;
  std::vector<std::uint64_t> player_roster;
  std::array<std::uint64_t,4> player_esau{};
  std::size_t bytes=0,dependencies=0;
};

enum class NativeEntityDisposition { Apply, Duplicate, Stale };
// C→T调和计划保留逻辑key；Morph或不兼容载体变化通过重建原生对象完成。
// failed后仍保留最后成功C作版本高水位，但必须全量重建，不能重复判定掩盖恢复。
struct NativeEntityPlan final {
  NativeEntityDisposition disposition=NativeEntityDisposition::Apply;
  bool reset=false;
  std::vector<std::uint64_t> remove,create,change,rebuild;
};

NativeEntityTarget PrepareNativeEntity(const Value &complete_model);
NativeEntityPlan PlanNativeEntity(const NativeEntityTarget *current,
                                 const NativeEntityTarget &target,bool healthy=true);
} // namespace cs::replica
