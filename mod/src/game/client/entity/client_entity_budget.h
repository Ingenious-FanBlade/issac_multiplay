#pragma once
#include "game/client/entity/client_entity_adapters.h"
#include <array>
#include <stdexcept>

namespace cs::game {
// 目标、计划、原生列表和draw共用成员上限；池容量仍独立限制真实可取得的use。
inline constexpr unsigned kClientEntityMembers=16384,kClientEntityCommands=131073;
// J273 Factory ctor 6EEE60的真实槽数；Player另由具名owner提供，不挤占Factory。
inline constexpr std::array<unsigned,13> kClientEntityPools={0,512,512,512,64,128,64,512,64,1024,128,128,128};
struct ClientEntityBudget final {
  unsigned members=0,players=0,retired=0,resources=0,references=0,curve_points=0,commands=0,completed=0;
  std::array<unsigned,13> categories{};
};
// 只统计已经解析的不可变描述，不重复遍历JSON；任何超限都在原生写入前报告。
inline ClientEntityBudget ClientMeasureEntities(const std::vector<ClientObjectDescription> &values,unsigned commands) {
  ClientEntityBudget out;out.commands=commands;
  if(values.size()>kClientEntityMembers||commands>kClientEntityCommands)throw std::runtime_error("entity_budget:members_or_commands");
  for(const auto &d:values){if(!d.key.id){++out.retired;continue;}++out.members;
    if(d.category>=out.categories.size())throw std::runtime_error("entity_budget:classification");
    if(++out.categories[d.category]>kClientEntityPools[d.category]&&d.category)
      throw std::runtime_error("entity_budget:native_pool_category="+std::to_string(d.category)+":capacity="+std::to_string(kClientEntityPools[d.category]));
    out.players+=d.category==0;out.references+=unsigned(d.relations.size());++out.resources;
    if(d.derived){const auto &s=*d.derived;out.resources+=unsigned(s.resources.size());
      out.curve_points+=unsigned(s.npc_path.size()+s.npc_secondary_path.size()+s.owned.trail.size());
      if(s.laser)out.curve_points+=unsigned(s.laser->optimized.size()+s.laser->unoptimized.size()+s.laser->indices.size()+s.laser->bezier.size());}
    // 包含合法Null的成员槽另计，不能把4096个实际resource_pose的既有上限误作空槽上限。
    if(out.resources>16384||out.references>262144||out.curve_points>262144)throw std::runtime_error("entity_budget:resource_slots_references_or_geometry");
  }return out;
}
} // namespace cs::game
