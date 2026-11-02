#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace cs::replica {
// 配置是资源描述，候选表从不作为最终成员执行；该纯值合同也供 C++17 SDK 边界使用。
struct RoomEntryValue final {
  std::uint32_t type=0;
  std::uint16_t variant=0,subtype=0;
  float weight=0;
  bool operator==(const RoomEntryValue &b) const {return type==b.type&&variant==b.variant&&subtype==b.subtype&&weight==b.weight;}
};
struct RoomSpawnValue final {
  std::uint16_t x=0,y=0;
  float sum_weights=0;
  std::vector<RoomEntryValue> entries;
  bool operator==(const RoomSpawnValue &b) const {return x==b.x&&y==b.y&&sum_weights==b.sum_weights&&entries==b.entries;}
};
// Data/Override 独立持值；merged 只表达配置关系，不携带原生指针；深度/条目数由解析器限制。
struct RoomConfiguration final {
  bool is_null=true;
  int stage=0,mode=0,type=0,subtype=0,shape=0,width=0,height=0,difficulty=0;
  std::uint32_t variant=0,original_variant=0,doors=0,quest_doors=0,flags=0;
  float initial_weight=0,weight=0;
  std::string content_version,name;
  std::vector<RoomSpawnValue> spawns;
  std::vector<RoomConfiguration> merged;
  bool operator==(const RoomConfiguration &b) const {
    return is_null==b.is_null&&stage==b.stage&&mode==b.mode&&type==b.type&&subtype==b.subtype&&shape==b.shape&&
      width==b.width&&height==b.height&&difficulty==b.difficulty&&variant==b.variant&&original_variant==b.original_variant&&
      doors==b.doors&&quest_doors==b.quest_doors&&flags==b.flags&&initial_weight==b.initial_weight&&weight==b.weight&&
      content_version==b.content_version&&name==b.name&&spawns==b.spawns&&merged==b.merged;
  }
  bool operator!=(const RoomConfiguration &b) const {return !(*this==b);}
};
} // namespace cs::replica
