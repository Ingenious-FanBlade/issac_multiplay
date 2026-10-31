#include "replica/room_grid_original.h"
#include "replica/model.h"
#include <set>

namespace cs::replica {
namespace {
[[noreturn]] void Bad(const char *s){throw ContractError(s);}
const Value &At(const Value &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Bad("original_missing_field");return v[k];}
std::string Str(const Value &v){if(!v.IsString())Bad("original_string");return {v.GetString(),v.GetStringLength()};}
int Scalar(const Value &v){if(v.IsInt())return v.GetInt();return std::stoi(Str(At(v,"value")));}
const Value &Array(const Value &v,unsigned n){if(!v.IsArray()||v.Size()>n)Bad("original_collection_budget");return v;}
Value CopyV(const Value &v,Allocator &a){return Value(v,a);}
unsigned Pass(const std::string &role){if(role=="floor")return 0;if(role=="wall")return 2;if(role=="water")return 3;if(role=="top")return 5;if(role=="light")return 6;if(role=="environment"||role=="outline")return 7;if(role=="main"||role=="extra"||role=="timer")return 4;Bad("original_visual_role");}
}
RoomGrid PrepareOriginalRoomGrid(const std::string &text){
 return PrepareOriginalRoomGridValue(Parse(text,Limits{32u<<20,4194304,64,16u<<20}));
}
RoomGrid PrepareOriginalRoomGridValue(Json packet){
 if(At(packet,"format")!="isaac.cs.room-grid-original"||Scalar(At(packet,"version"))!=1)Bad("original_schema");
 const auto &model=At(packet,"model");
 const auto &components=At(model,"components"),&room=At(At(components,"room"),"state"),&grid=At(At(components,"grid"),"state");
 for(const char *name:{"grid","room"})if(At(components[name],"coverage")!="Complete"||At(components[name],"membership_complete")!=true)Bad("original_source_partial");
 const auto &fields=At(At(room,"state"),"fields");const auto &geometry=At(packet,"geometry");
 Json display(rapidjson::kObjectType);auto &a=display.GetAllocator();Put(display,"format","isaac.cs.room-grid",a);Put(display,"version",Value(1),a);
 Put(display,"connection",CopyV(At(packet,"connection"),a),a);Put(display,"scope",CopyV(model["scope"],a),a);Put(display,"revision",CopyV(At(model,"revision"),a),a);Put(display,"membership_complete",Value(true),a);
 Value room_out(rapidjson::kObjectType);Put(room_out,"shape",Value(Scalar(At(fields,"Room.GetRoomShape"))),a);Put(room_out,"size",CopyV(At(geometry,"size"),a),a);
 Value cells=CopyV(Array(At(geometry,"cells"),448),a);if(cells.Size()!=448)Bad("original_cells_incomplete");
 for(auto &c:cells.GetArray()){Value p(rapidjson::kArrayType);p.PushBack(0,a);p.PushBack(0,a);Put(c,"position",std::move(p),a);for(const char *k:{"collision","path","rail"})Put(c,k,Value(0),a);}
 Put(room_out,"cells",std::move(cells),a);Value env(rapidjson::kObjectType);
 for(const auto &binding:std::vector<std::pair<const char *,const char *>>{{"backdrop","Room.GetBackdropType"},{"floor_color","Room.GetFloorColor"},{"wall_color","Room.GetWallColor"}})Put(env,binding.first,CopyV(At(fields,binding.second),a),a);
 Value water(rapidjson::kObjectType);Put(water,"amount",CopyV(At(fields,"Room.GetWaterAmount"),a),a);Put(water,"color",CopyV(At(fields,"Room.GetWaterColor"),a),a);Put(water,"current",CopyV(At(fields,"Room.GetWaterCurrent"),a),a);Put(env,"water",std::move(water),a);
 Value fx(rapidjson::kObjectType);for(const char *k:{"Room.GetFXParams","Room.GetLightingAlpha","Room.GetRainIntensity","Room.GetLavaIntensity"})if(fields.HasMember(k))Put(fx,k,CopyV(fields[k],a),a);Put(env,"fx",std::move(fx),a);
 Put(room_out,"environment",std::move(env),a);Put(room_out,"parts",Value(rapidjson::kArrayType),a);Put(display,"room",std::move(room_out),a);
 Value grids(rapidjson::kArrayType);for(const auto &g:Array(At(grid,"objects"),1024).GetArray()){
  Value object(rapidjson::kObjectType);Put(object,"key",CopyV(At(At(g,"subject"),"target"),a),a);Put(object,"visible",Value(false),a);Put(object,"parts",Value(rapidjson::kArrayType),a);grids.PushBack(std::move(object),a);
 }Put(display,"grids",std::move(grids),a);
 const auto &images=Array(At(packet,"images"),256),&draws=Array(At(packet,"draws"),16384);Value nodes(rapidjson::kArrayType);bool floor_covered=false;
 for(const auto &draw:draws.GetArray()){
  const auto id=Unsigned(At(draw,"id"));if(!id||Unsigned(At(draw,"image"))>=images.Size())Bad("original_image_reference");
  const auto role=Str(At(draw,"role"));Value node(rapidjson::kObjectType);Put(node,"id",Decimal(id,a),a);Put(node,"pass",Value(Pass(role)),a);Put(node,"screen",Value(false),a);Put(node,"native_draw",Value(true),a);
  Value at(rapidjson::kArrayType);at.PushBack(0,a);at.PushBack(0,a);Put(node,"position",std::move(at),a);nodes.PushBack(std::move(node),a);
  Value part(rapidjson::kObjectType);Put(part,"role",role,a);Put(part,"node",Decimal(id,a),a);
  const auto &owner=At(draw,"grid");
  if(owner.IsNull()){
   if(role=="floor"){Value coverage(rapidjson::kArrayType);if(!floor_covered){for(const auto &c:display["room"]["cells"].GetArray())if(c["inside"]==true)coverage.PushBack(CopyV(c["index"],a),a);floor_covered=true;}Put(part,"cells",std::move(coverage),a);}
   display["room"]["parts"].PushBack(std::move(part),a);
  }else{
   bool found=false;for(auto &g:display["grids"].GetArray())if(g["key"]==owner){g["parts"].PushBack(std::move(part),a);g["visible"].SetBool(true);found=true;break;}if(!found)Bad("original_grid_reference");
  }
 }
 Put(display,"nodes",std::move(nodes),a);Value camera(rapidjson::kObjectType),origin(rapidjson::kArrayType);origin.PushBack(0,a);origin.PushBack(0,a);Put(camera,"origin",std::move(origin),a);Put(camera,"zoom",Value(1),a);Put(display,"camera",std::move(camera),a);Put(display,"hud","",a);
 // 最终native顶点已经包含Extra/Timer/Top和水裁边；不由Client依据当前字段再生成视觉。
 Put(display,"native_images",CopyV(images,a),a);Put(display,"native_draws",CopyV(draws,a),a);Put(display,"original_render",Value(true),a);
 Put(display,"orthographic",CopyV(At(packet,"orthographic"),a),a);
 Put(display,"color_modifier",CopyV(At(packet,"color_modifier"),a),a);
 Put(display,"color_modifier_enabled",packet.HasMember("color_modifier_enabled")?CopyV(packet["color_modifier_enabled"],a):Value(true),a);
 return PrepareRoomGridValue(ComposeRoomGridValue(model,std::move(display)));
}
} // namespace cs::replica
