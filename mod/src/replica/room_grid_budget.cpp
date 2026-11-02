#include "replica/room_grid_budget.h"
#include <string_view>

namespace cs::replica {
// 只遍历有界纯值，不序列化副本、不加载资源；深度在递归入口前检查。
RoomGridBudgetUsage MeasureRoomGridBudget(const Value &model) {
  RoomGridBudgetUsage u;u.bytes=24;const Limits limits;
  const auto walk=[&](auto &&self,const Value &v,unsigned depth)->void {
    if(depth>limits.depth||++u.nodes>limits.nodes)throw ContractError("room_grid:target_node_or_depth_budget");
    u.bytes+=v.IsString()?std::uint64_t(v.GetStringLength())+5:9;
    if(u.bytes>limits.bytes)throw ContractError("room_grid:target_byte_budget");
    if(v.IsString()&&v.GetStringLength()>limits.string_bytes)throw ContractError("room_grid:target_string_budget");
    if(v.IsArray()){for(const auto &x:v.GetArray())self(self,x,depth+1);return;}
    if(!v.IsObject())return;
    if(v.HasMember("type")&&v["type"].IsString()&&std::string_view(v["type"].GetString(),v["type"].GetStringLength())=="resource_pose") {
      if(++u.resources>kRoomGridResources)throw ContractError("room_grid:resource_total_budget");
    }
    for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){
      const std::string_view name(i->name.GetString(),i->name.GetStringLength());
      const Value *array=&i->value;
      if(name=="Sprite.GetAllLayers"&&array->IsObject()&&array->HasMember("entries"))array=&(*array)["entries"];
      if(array->IsArray()) {
        if(name=="Sprite.GetAllLayers") {u.layers+=array->Size();if(u.layers>kRoomGridLayers)throw ContractError("room_grid:layer_total_budget");}
        if(name=="embers"||name=="rubble"||name=="poses"||name=="rays") {u.fragments+=array->Size();if(u.fragments>kRoomGridFragments)throw ContractError("room_grid:fx_total_budget");}
      }
      self(self,i->name,depth+1);self(self,i->value,depth+1);
    }
  };
  walk(walk,model,0);return u;
}
} // namespace cs::replica
