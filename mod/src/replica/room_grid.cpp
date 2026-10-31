#include "replica/room_grid.h"
#include "replica/model.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <set>

namespace cs::replica {
namespace {
[[noreturn]] void Bad(const char *s){throw ContractError(s);}
const Value &At(const Value &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Bad("room_grid_missing_member");return v[k];}
std::string Str(const Value &v){if(!v.IsString())Bad("room_grid_string");return {v.GetString(),v.GetStringLength()};}
unsigned Index(const Value &v,unsigned end){if(!v.IsUint()||v.GetUint()>=end)Bad("room_grid_index");return v.GetUint();}
int Integer(const Value &v){if(!v.IsInt())Bad("room_grid_integer");return v.GetInt();}
int Scalar(const Value &v){
  if(v.IsInt())return v.GetInt();
  const auto &s=At(v,"value");const auto text=Str(s);int out=0;const auto r=std::from_chars(text.data(),text.data()+text.size(),out);
  if(r.ec!=std::errc{}||r.ptr!=text.data()+text.size())Bad("room_grid_scalar");return out;
}
bool Flag(const Value &v){if(!v.IsBool())Bad("room_grid_boolean");return v.GetBool();}
float Number(const Value &v){if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>1000000)Bad("room_grid_number");return float(v.GetDouble());}
void Vector(const Value &v){if(!v.IsArray()||v.Size()!=2)Bad("room_grid_vector");Number(v[0]);Number(v[1]);}
const Value &Array(const Value &v,unsigned limit){if(!v.IsArray()||v.Size()>limit)Bad("room_grid_collection_budget");return v;}
// 与J273 Factory已核定的1..27分派一致；NULL和未知类不能退回通用Rock。
const char *Class(unsigned type){
  static const char *classes[]={"", "Decoration","Rock","Rock","Rock","Rock","Rock","Pit","Spikes","Spikes","Web","Lock","TNT","Fire","Poop","Wall","Door","TrapDoor","Stairs","Gravity","PressurePlate","Statue","Rock","Teleporter","Rock","Rock","Rock","Rock"};
  if(!type||type>=28)Bad("room_grid_unknown_type");return classes[type];
}
std::string Key(const Value &v,std::uint64_t run,unsigned space){
  if(!v.IsObject()||v.MemberCount()!=3||Unsigned(At(v,"run"))!=run||Unsigned(At(v,"space"))!=space||!Unsigned(At(v,"id")))Bad("room_grid_identity");
  return std::to_string(Unsigned(v["id"]));
}
// 保留公开只读字段完整目录，不能以少量常见字段代替派生合同。目录只检查结构，
// 当前值/NotApplicable/AuthorityOnly等语义仍由源状态提供，不制造Init或出生见证。
void Fields(const Value &state,const std::string &owner){
  if(Str(At(state,"type"))!=owner)Bad("room_grid_state_class");
  const auto &fields=At(state,"fields");if(!fields.IsObject())Bad("room_grid_fields");
  for(const auto &row:Catalog()["members"].GetArray()){
    if(Str(row["source_catalog"])!="grid-catalog.json")continue;
    const auto id=Str(row["id"]),category=Str(row["category"]);
    if(id.rfind(owner+".",0)!=0)continue;
    if(category=="field"||category=="query"||category=="parameter_query")
      if(!fields.HasMember(id.c_str()))Bad("room_grid_incomplete_fields");
  }
}
void Complete(const Value &v){
  if(v.IsArray())for(const auto &x:v.GetArray())Complete(x);
  if(!v.IsObject())return;
  if(v.HasMember("status")&&v["status"].IsString()){
    const auto s=Str(v["status"]);
    if(s=="ReadFailed"||s=="Truncated"||s=="Unsupported"||s=="Unavailable"||s=="Nonfinite"||s=="NotCaptured")Bad("room_grid_incomplete_state");
  }
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){
    const auto k=Str(i->name);if(k=="pointer"||k=="ptr"||k=="vptr"||k=="address"||k=="native_address"||k=="local_id")Bad("room_grid_noncanonical_state");
    Complete(i->value);
  }
}
} // namespace

RoomGrid PrepareRoomGrid(const std::string &text){
  Limits limits;limits.bytes=8u<<20;limits.nodes=524288;auto root=Parse(text,limits);
  if(Str(At(root,"format"))=="isaac.cs.room-grid-source")return PrepareRoomGrid(ComposeRoomGrid(Text(At(root,"model")),Text(At(root,"display"))));
  if(Str(At(root,"format"))!="isaac.cs.room-grid"||Integer(At(root,"version"))!=1)Bad("room_grid_schema");
  const auto scope=ReadScope(At(root,"scope"));if(!scope.run||!scope.floor||!scope.room)Bad("room_grid_scope");
  RoomGrid out;out.session=scope.session;out.connection=scope.connection;out.run=scope.run;out.floor=scope.floor;out.dimension=scope.dimension;
  out.room=scope.room;out.scene=scope.scene;out.revision=Unsigned(At(root,"revision"));
  if(!out.revision||!Unsigned(At(root,"connection")))Bad("room_grid_revision");
  if(!Flag(At(root,"membership_complete")))Bad("room_grid_partial_membership");
  const auto &room=At(root,"room");out.shape=Index(At(room,"shape"),13);if(!out.shape)Bad("room_grid_shape");
  Vector(At(room,"size"));if(Number(room["size"][0])<=0||Number(room["size"][1])<=0)Bad("room_grid_geometry");
  // 实际448格逐项复制。inside由Authority最终布局决定，不按seed或形状重建缺失格。
  const auto &cells=Array(At(room,"cells"),448);if(cells.Size()!=448)Bad("room_grid_cells_incomplete");
  out.cells.resize(448);std::set<unsigned> indices;unsigned inside=0;
  for(const auto &cell:cells.GetArray()){
    const auto index=Index(At(cell,"index"),448);if(!indices.insert(index).second)Bad("room_grid_cell_duplicate");
    auto &c=out.cells[index];c.inside=Flag(At(cell,"inside"));inside+=c.inside;
    Vector(At(cell,"position"));c.x=Number(cell["position"][0]);c.y=Number(cell["position"][1]);
    c.collision=Integer(At(cell,"collision"));c.path=Integer(At(cell,"path"));c.rail=Integer(At(cell,"rail"));c.occlusion=Integer(At(cell,"occlusion"));
    if(c.collision<0||c.collision>5)Bad("room_grid_collision");
  }
  if(!inside)Bad("room_grid_empty_geometry");
  for(const char *k:{"state","config","override_config","environment"})At(room,k);
  if(!room["state"].IsObject()||!room["environment"].IsObject())Bad("room_grid_room_state");
  // 环境的实际颜色/水/FX与资源部件一起拥有；不把Backdrop枚举当作足够的房间资源。
  for(const char *k:{"backdrop","floor_color","wall_color","water","fx"})At(room["environment"],k);
  const auto &nodes=Array(At(root,"nodes"),4096);std::map<std::string,const Value *> resources;
  for(const auto &node:nodes.GetArray()){
    const auto id=std::to_string(Unsigned(At(node,"id")));if(id=="0"||!resources.emplace(id,&node).second)Bad("room_grid_node_duplicate");
    if(Flag(At(node,"screen")))Bad("room_grid_screen_node");
  }
  std::set<std::string> used;
  auto parts=[&](const Value &list,const std::set<std::string> &roles){
    std::set<std::string> present;
    for(const auto &part:Array(list,4096).GetArray()){
      const auto role=Str(At(part,"role"));if(!roles.count(role))Bad("room_grid_visual_role");
      const auto node=std::to_string(Unsigned(At(part,"node")));const auto it=resources.find(node);
      if(it==resources.end()||!used.insert(node).second)Bad("room_grid_visual_owner");
      const auto pass=Index(At(*it->second,"pass"),8);
      if((role=="floor"&&pass!=0)||(role=="shadow"&&pass!=1)||(role=="wall"&&pass!=2)||(role=="water"&&pass!=3)||
         ((role=="main"||role=="extra"||role=="timer"||role=="timer_text")&&pass!=4)||(role=="top"&&pass!=5)||(role=="light"&&pass!=6)||((role=="foreground"||role=="outline")&&pass!=7))Bad("room_grid_visual_pass");
      present.insert(role);++out.parts;
    }return present;
  };
  const auto room_parts=parts(At(room,"parts"),{"floor","shadow","wall","water","environment","light","foreground","outline"});
  if(!room_parts.count("floor")||!room_parts.count("wall"))Bad("room_grid_room_visual_missing");
  const auto &water=At(room["environment"],"water");if(Number(At(water,"amount"))>0&&!room_parts.count("water"))Bad("room_grid_water_visual_missing");
  // 地面部件必须逐格声明覆盖，L房缺角/窄房不能只有元数据而仍画完整矩形。
  std::set<unsigned> covered;
  for(const auto &part:room["parts"].GetArray())if(part["role"]=="floor"){
    for(const auto &cell:Array(At(part,"cells"),448).GetArray()){
      const auto index=Index(cell,448);if(!out.cells[index].inside||!covered.insert(index).second)Bad("room_grid_floor_coverage");
    }
  }
  if(covered.size()!=inside)Bad("room_grid_floor_missing_cells");
  const auto &grids=Array(At(root,"grids"),1024);std::map<std::string,const Value *> members;
  std::set<std::string> occupied;std::set<unsigned> types;std::set<std::string> classes;
  for(const auto &g:grids.GetArray()){
    const auto id=Key(At(g,"key"),scope.run,2);if(!members.emplace(id,&g).second)Bad("room_grid_member_duplicate");
    const auto type=Index(At(g,"type"),28);const auto name=std::string("GridEntity")+Class(type);
    if(Str(At(g,"class"))!=name)Bad("room_grid_class_mismatch");
    types.insert(type);classes.insert(name);Index(At(g,"index"),448);
    const auto &membership=Array(At(g,"memberships"),16);if(membership.Empty())Bad("room_grid_membership_missing");
    for(const auto &m:membership.GetArray()){
      const auto source=Str(At(m,"source"));if(source!="grid"&&source!="door"&&source!="decoration")Bad("room_grid_membership_source");
      const auto index=Index(At(m,"index"),source=="door"?8:448);
      if(source=="grid"&&index!=g["index"].GetUint())Bad("room_grid_slot_mismatch");
      if(source=="door"&&type!=16)Bad("room_grid_door_class");
      if(!occupied.insert(source+":"+std::to_string(index)).second)Bad("room_grid_slot_duplicate");
    }
    const auto &state=At(g,"state");Fields(At(state,"common"),"GridEntity");Fields(At(state,"derived"),name);
    Fields(At(g,"save"),"GridEntityDesc");
    const auto visual=parts(At(g,"parts"),{"main","extra","timer","timer_text","top","shadow","light","outline"});
    if(Flag(At(g,"visible"))&&!visual.count("main"))Bad("room_grid_main_missing");
    if(!g["visible"].GetBool()&&!g["parts"].Empty())Bad("room_grid_invisible_parts");
    if(type!=16&&visual.count("extra"))Bad("room_grid_extra_owner");
    if(type!=20&&visual.count("timer"))Bad("room_grid_timer_owner");
    if(type!=20&&visual.count("timer_text"))Bad("room_grid_timer_owner");
    if(name!="GridEntityRock"&&visual.count("top"))Bad("room_grid_top_owner");
    if(type==16){
      const auto &f=state["derived"]["fields"];const auto &extra=At(f,"GridEntityDoor.ExtraVisible");
      if(!extra.IsBool()||extra.GetBool()!=bool(visual.count("extra")))Bad("room_grid_extra_missing");
    }
    if(type==20){const auto &timer=At(state["derived"]["fields"],"GridEntityPressurePlate.TimerPlate");
      const bool absent=timer.IsNull()||(timer.IsObject()&&timer.HasMember("status")&&timer["status"]=="NotApplicable");
      if(!absent&&!visual.count("timer"))Bad("room_grid_timer_missing");
    }
    ++out.grids;
  }
  // 门槽与Grid主体双向核对，门不是另一套可变对象；目标路由保留有符号原值。
  const auto &doors=Array(At(room,"doors"),8);if(doors.Size()!=8)Bad("room_grid_doors_incomplete");indices.clear();
  for(const auto &door:doors.GetArray()){
    const auto slot=Index(At(door,"slot"),8);if(!indices.insert(slot).second)Bad("room_grid_door_duplicate");
    Flag(At(door,"allowed"));Flag(At(door,"descriptor_enabled"));At(door,"target");const auto &grid=At(door,"grid");
    if(grid.IsNull()){if(occupied.count("door:"+std::to_string(slot)))Bad("room_grid_door_alias_missing");continue;}
    const auto id=Key(grid,scope.run,2);const auto found=members.find(id);
    if(found==members.end()||(*found->second)["type"]!=16||!door["allowed"].GetBool())Bad("room_grid_door_reference");
    bool alias=false;for(const auto &m:(*found->second)["memberships"].GetArray())if(m["source"]=="door"&&m["index"].GetUint()==slot)alias=true;
    if(!alias)Bad("room_grid_door_alias_missing");++out.doors;
  }
  if(used.size()!=resources.size())Bad("room_grid_unowned_visual");
  auto references=[&](auto &&self,const Value &v)->void{
    if(v.IsArray())for(const auto &x:v.GetArray())self(self,x);
    if(!v.IsObject())return;
    if(v.HasMember("type")&&v["type"]=="reference"){
      const auto status=Str(At(v,"status"));
      if(status=="KnownCurrent"){
        const auto &target=At(v,"target");if(Unsigned(At(target,"run"))!=scope.run)Bad("room_grid_cross_run_reference");
        if(Unsigned(At(target,"space"))==2&&!members.count(Key(target,scope.run,2)))Bad("room_grid_reference_missing");
      }else if(status!="Null"&&status!="NotApplicable"&&status!="KnownHistorical"&&status!="Retired")Bad("room_grid_reference_unresolved");
      ++out.references;
    }
    for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)self(self,i->value);
  };
  Complete(root);references(references,room);references(references,grids);
  out.types=unsigned(types.size());out.classes=unsigned(classes.size());
  Json descriptor(rapidjson::kObjectType);auto &a=descriptor.GetAllocator();
  for(const char *k:{"connection","revision","camera","hud","nodes"})Put(descriptor,k,Value(At(root,k),a),a);
  Put(descriptor,"epoch",Decimal(scope.scene,a),a);Value r(rapidjson::kObjectType);Put(r,"size",Value(room["size"],a),a);Put(descriptor,"room",std::move(r),a);
  out.model=Text(root);out.descriptor=Text(descriptor);return out;
}
std::string ComposeRoomGrid(const std::string &model,const std::string &display){
  auto source=Parse(model);ValidateModel(source);auto out=Parse(display);auto &a=out.GetAllocator();
  if(Str(At(out,"format"))!="isaac.cs.room-grid")Bad("room_grid_display_schema");
  if(ReadScope(At(source,"scope"))!=ReadScope(At(out,"scope"))||At(source,"revision")!=At(out,"revision"))Bad("room_grid_source_version");
  const auto &components=At(source,"components");const auto &r=At(components,"room"),&g=At(components,"grid");
  for(const auto *c:{&r,&g})if(At(*c,"coverage")!="Complete"||!Flag(At(*c,"membership_complete")))Bad("room_grid_source_partial");
  const auto &room=At(r,"state"),&grid=At(g,"state");
  At(out,"room");auto &target=out["room"];Array(At(target,"cells"),448);
  for(const char *k:{"state","config","override_config"})Put(target,k,Value(At(room,k),a),a);
  const auto &room_fields=At(At(room,"state"),"fields");
  if(Scalar(At(room_fields,"Room.GetRoomShape"))!=Integer(At(target,"shape")))Bad("room_grid_source_shape");
  // 原生坐标/碰撞/路径/轨道来自12同窗口的有限参数查询，视觉补充不能另造一套几何。
  for(const auto &binding:std::vector<std::pair<const char *,const char *>>{{"Room.GetGridPosition","position"},{"Room.GetGridCollision","collision"},{"Room.GetGridPath","path"},{"Room.GetRail","rail"}}){
    const auto &values=Array(At(At(room_fields,binding.first),"values"),448);if(values.Size()!=448)Bad("room_grid_source_cells");
    std::set<unsigned> seen;
    for(const auto &query:values.GetArray()){
      const auto &params=At(query,"parameters");int index=-1;
      if(params.IsArray()&&params.Size()==1)index=Scalar(params[0]);
      else{const auto &entries=Array(At(params,"entries"),1);if(entries.Size()!=1)Bad("room_grid_source_parameter");index=Scalar(At(entries[0],"value"));}
      if(index<0||index>=448||!seen.insert(unsigned(index)).second)Bad("room_grid_source_parameter");
      Value value;const auto &raw=At(query,"value");
      if(binding.second==std::string("position")){value.SetArray();value.PushBack(Number(At(raw,"x")),a);value.PushBack(Number(At(raw,"y")),a);}
      else value.SetInt(Scalar(raw));
      bool found=false;for(auto &cell:target["cells"].GetArray())if(At(cell,"index").IsUint()&&cell["index"].GetUint()==unsigned(index)){
        Put(cell,binding.second,Value(value,a),a);found=true;break;}
      if(!found)Bad("room_grid_source_cells");
    }
  }
  // 视觉补充按规范主体逐一对应，成员缺席不能变成静默删除或新增伪对象。
  const auto &objects=Array(At(grid,"objects"),1024);auto &grids=At(out,"grids");
  if(!grids.IsArray()||objects.Size()!=grids.Size())Bad("room_grid_source_membership");
  std::set<std::string> copied;
  for(auto &dst:out["grids"].GetArray()){
    const auto id=Key(At(dst,"key"),ReadScope(source["scope"]).run,2);const Value *found=nullptr;
    for(const auto &obj:objects.GetArray())if(At(At(obj,"subject"),"target")==dst["key"]){if(found)Bad("room_grid_source_duplicate");found=&obj;}
    if(!found||!copied.insert(id).second)Bad("room_grid_source_identity");
    for(const char *k:{"type","variant","index","class"})Put(dst,k,Value(At(*found,k),a),a);
    Value state(rapidjson::kObjectType);Put(state,"common",Value(At(*found,"common"),a),a);
    Put(state,"derived",Value(At(At(*found,"bodies"),Str(dst["class"]).c_str()),a),a);Put(dst,"state",std::move(state),a);
    Put(dst,"save",Value(At(At(At(*found,"common"),"fields"),"GridEntity.GetSaveState"),a),a);
    Value membership(rapidjson::kArrayType);
    for(const auto &m:Array(At(*found,"memberships"),16).GetArray()){
      const auto source_id=Index(At(m,"source"),14);const char *name=source_id==11?"grid":source_id==12?"door":source_id==13?"decoration":nullptr;
      if(!name)Bad("room_grid_source_membership_kind");Value entry(rapidjson::kObjectType);
      Put(entry,"source",name,a);Put(entry,"index",Value(At(m,"index"),a),a);membership.PushBack(std::move(entry),a);
    }Put(dst,"memberships",std::move(membership),a);
  }
  const auto &source_doors=Array(At(room,"doors"),8);Value doors(rapidjson::kArrayType);
  for(const auto &d:source_doors.GetArray()){
    Value door(rapidjson::kObjectType);Put(door,"slot",Value(At(d,"slot"),a),a);
    Put(door,"allowed",Value(At(d,"shape_allowed"),a),a);Put(door,"descriptor_enabled",Value(At(d,"descriptor_enabled"),a),a);
    Put(door,"target",Value(At(d,"actual_connection"),a),a);const auto &actual=At(d,"actual");
    Put(door,"grid",At(actual,"status")=="Null"?Value(rapidjson::kNullType):Value(At(actual,"target"),a),a);
    doors.PushBack(std::move(door),a);
  }Put(target,"doors",std::move(doors),a);
  return Text(out);
}
std::string RoomGridObject(const RoomGrid &world,const std::string &id){
  auto root=Parse(world.model,Limits{8u<<20,524288,64,2u<<20});
  if(id=="room")return Text(root["room"]);
  if(id=="metadata"){root["room"].RemoveMember("cells");return Text(root["room"]);}
  if(id.rfind("cell-",0)==0){
    const auto number=Unsigned(Value(id.substr(5).c_str(),root.GetAllocator()));
    for(const auto &c:root["room"]["cells"].GetArray())if(c["index"].GetUint()==number)return Text(c);
    return "null";
  }
  if(id=="all")return world.model;
  for(const auto &g:root["grids"].GetArray())if(Str(g["key"]["id"])==id)return Text(g);
  return "null";
}
} // namespace cs::replica
