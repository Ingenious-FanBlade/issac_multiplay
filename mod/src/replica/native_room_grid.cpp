#include "replica/native_room_grid.h"
#include <charconv>
#include <cmath>

namespace cs::replica {
namespace {
[[noreturn]] void Fail(const char *reason){throw ContractError(reason);}
const Value &At(const Value &v,const char *name){
  if(!v.IsObject()||!v.HasMember(name))throw ContractError(std::string("native_room_grid_missing_field:")+name);return v[name];
}
std::string String(const Value &v){
  if(!v.IsString())Fail("native_room_grid_string");return {v.GetString(),v.GetStringLength()};
}
// 观测整数以有符号十进制int64编码；先检查范围再用于SDK的32位字段。
int Integer(const Value &v){
  if(v.IsInt())return v.GetInt();
  if(String(At(v,"type"))!="int64")Fail("native_room_grid_integer");
  const auto s=String(At(v,"value"));int n=0;
  const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
  if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())Fail("native_room_grid_integer_range");return n;
}
std::uint32_t Unsigned32(const Value &v){
  const Value *value=&v;
  if(v.IsObject()){
    if(At(v,"type")!="int64")Fail("native_room_grid_unsigned");value=&At(v,"value");
  }
  const auto n=Unsigned(*value);
  if(n>UINT32_MAX)Fail("native_room_grid_unsigned_range");return std::uint32_t(n);
}
float Number(const Value &v){
  if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>1000000)
    Fail("native_room_grid_coordinate");return float(v.GetDouble());
}
unsigned Index(const Value &v,unsigned end){
  if(!v.IsUint()||v.GetUint()>=end)Fail("native_room_grid_index");return v.GetUint();
}
const Value &Array(const Value &v,unsigned count,bool exact=false){
  if(!v.IsArray()||v.Size()>count||(exact&&v.Size()!=count))Fail("native_room_grid_collection");return v;
}
const char *GridClass(unsigned type){
  static const char *names[]={"", "Decoration","Rock","Rock","Rock","Rock","Rock","Pit",
    "Spikes","Spikes","Web","Lock","TNT","Fire","Poop","Wall","Door","TrapDoor","Stairs",
    "Gravity","PressurePlate","Statue","Rock","Teleporter","Rock","Rock","Rock","Rock"};
  if(!type||type>=28)Fail("native_room_grid_unknown_type");return names[type];
}
std::uint64_t Key(const Value &key,std::uint64_t run,unsigned space){
  if(!key.IsObject()||key.MemberCount()!=3||Unsigned(At(key,"run"))!=run||
     Unsigned(At(key,"space"))!=space)Fail("native_room_grid_key");
  const auto id=Unsigned(At(key,"id"));if(!id)Fail("native_room_grid_key");return id;
}
void Fields(const Value &state,const std::string &owner,const char *catalog){
  // RoomConfig按15的资源合同包装为resource.state；Grid/Room本体则直接带fields。
  if(At(state,"type")=="resource"){Fields(At(state,"state"),owner,catalog);return;}
  // 15会把原始Sprite规范化为resource_pose；字段仍使用Sprite目录，不能要求观测前的标签。
  const auto type=String(At(state,"type"));
  if((type!=owner&&!(owner=="Sprite"&&type=="resource_pose"))||!At(state,"fields").IsObject())Fail("native_room_grid_fields");
  for(const auto &row:Catalog()["members"].GetArray()){
    if(row["source_catalog"]!=catalog||String(row["id"]).rfind(owner+".",0)!=0)continue;
    const auto category=String(row["category"]);
    if(category=="field"||category=="query"||category=="parameter_query")
      if(!state["fields"].HasMember(row["id"].GetString()))Fail("native_room_grid_incomplete_fields");
  }
}
// 不完整状态不能成为删除依据。AuthorityOnly等归属标记原样保留，由backend按应用含义处理。
void Complete(const Value &v){
  if(v.IsArray())for(const auto &e:v.GetArray())Complete(e);
  if(!v.IsObject())return;
  if(v.HasMember("status")&&v["status"].IsString()){
    const auto status=String(v["status"]);
    if(status=="ReadFailed"||status=="Truncated"||status=="Unsupported"||status=="Unavailable"||
       status=="Nonfinite"||status=="NotCaptured")Fail("native_room_grid_incomplete_state");
  }
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){
    // 这两项保留观测原始归属：概率来自Run域，旧FX集合公开接口不可读。
    // 18的FX实际状态由必填native_context.environment校验及应用，不把占位伪造成值。
    if(i->value.IsObject()&&i->value.HasMember("status")&&
       ((i->name=="Room.GetDevilRoomChance"&&i->value["status"]=="NotCaptured")||
        (i->name=="Room.GetFXLayers"&&i->value["status"]=="Unavailable")))continue;
    Complete(i->value);
  }
}
template<class T> void Queries(const Value &fields,const char *name,T &result,bool nullable=false){
  std::set<unsigned> seen;
  for(const auto &entry:Array(At(At(fields,name),"values"),448,true).GetArray()){
    const auto &parameters=At(entry,"parameters");int index=-1;
    if(parameters.IsArray()&&parameters.Size()==1)index=Integer(parameters[0]);
    else{
      const auto &entries=Array(At(parameters,"entries"),1,true);index=Integer(At(entries[0],"value"));
    }
    if(index<0||index>=448||!seen.insert(unsigned(index)).second)Fail("native_room_grid_query_index");
    const auto &v=At(entry,"value");
    // 无轨道的公开getter返回nil；255保留其真实原生无轨道语义，不把nil误作数值0。
    result[unsigned(index)]=nullable&&v.IsObject()&&v.HasMember("status")&&v["status"]=="Null"?255:Integer(v);
  }
}
const Value &Room(const NativeRoomGridTarget &target){return target.model["components"]["room"]["state"];}
const Value &ResourceState(const Value &v){return At(v,"type")=="resource"?At(v,"state"):v;}
void Boolean(const Value &v){if(!v.IsBool())Fail("native_room_grid_boolean");}
void ResourcePath(const Value &v,bool empty=false){const auto s=String(v);if((s.empty()&&!empty)||s.size()>512||(!s.empty()&&s.front()=='/')||s.find(':')!=s.npos||s.find("..")!=s.npos||s.find('\\')!=s.npos)Fail("native_room_grid_resource_path");}
void VectorValue(const Value &v){if(At(v,"type")!="vector")Fail("native_room_grid_vector");Number(At(v,"x"));Number(At(v,"y"));}
void ColorValue(const Value &v,bool offsets){for(const char *k:{"R","G","B","A"})Number(At(v,k));if(offsets)for(const char *k:{"RO","GO","BO"})Number(At(v,k));}
void SpriteValue(const Value &v){Fields(v,"Sprite","entity-catalog.json");const auto &f=At(v,"fields");Boolean(At(f,"Sprite.IsLoaded"));
 ResourcePath(At(f,"Sprite.GetFilename"),!f["Sprite.IsLoaded"].GetBool());
 if(v.HasMember("resource")&&At(v["resource"],"logical_path")!=f["Sprite.GetFilename"])Fail("native_room_grid_sprite_resource_mismatch");
 if(!f["Sprite.IsLoaded"].GetBool())return;
 for(const char *k:{"Sprite.GetAnimation","Sprite.GetOverlayAnimation"})String(At(f,k));
 for(const char *k:{"Sprite.GetFrame","Sprite.GetOverlayFrame","Sprite.GetRenderFlags"})Integer(At(f,k));
 for(const char *k:{"Sprite.FlipX","Sprite.FlipY"})Boolean(At(f,k));
 for(const char *k:{"Sprite.Offset","Sprite.Scale"})VectorValue(At(f,k));
 for(const char *k:{"Sprite.Rotation","Sprite.PlaybackSpeed"})Number(At(f,k));ColorValue(At(f,"Sprite.Color"),true);
 for(const auto &entry:Array(At(At(f,"Sprite.GetAllLayers"),"entries"),64).GetArray()){
  const auto &layer=At(entry,"value");Fields(layer,"LayerState","entity-catalog.json");const auto &l=layer["fields"];
  for(const char *k:{"LayerState.GetLayerID","LayerState.GetRenderFlags","LayerState.GetWrapSMode","LayerState.GetWrapTMode"})Integer(At(l,k));
  for(const char *k:{"LayerState.GetFlipX","LayerState.GetFlipY","LayerState.IsVisible"})Boolean(At(l,k));
  for(const char *k:{"LayerState.GetSize","LayerState.GetPos","LayerState.GetCropOffset"})VectorValue(At(l,k));
  Number(At(l,"LayerState.GetRotation"));ColorValue(At(l,"LayerState.GetColor"),true);ResourcePath(At(l,"LayerState.GetSpritesheetPath"));
  const auto &blend=At(At(l,"LayerState.GetBlendMode"),"fields");for(const char *k:{"BlendMode.Equation","BlendMode.RGBSourceFactor","BlendMode.RGBDestinationFactor","BlendMode.AlphaSourceFactor","BlendMode.AlphaDestinationFactor"})Integer(At(blend,k));
 }
}
// Room内嵌Backdrop／门额外层等也要先通过相同资源schema，不能只检查Grid主Sprite。
void ResourceValues(const Value &v){if(v.IsArray()){for(const auto &e:v.GetArray())ResourceValues(e);return;}if(!v.IsObject())return;
 if(v.HasMember("type")&&(v["type"]=="Sprite"||v["type"]=="resource_pose"))SpriteValue(v);
 for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)ResourceValues(i->value);
}
// FX只恢复具名状态和原版资源依赖，递归树／成员数在接触SDK之前限制。
void EnvironmentValue(const Value &v){Boolean(At(v,"loaded"));Integer(At(v,"selected_ray"));Integer(At(v,"poop_countdown"));
 auto entry=[&](const Value &e,bool poop){
  if(!poop)Integer(At(e,"id"));for(const char *k:{"count","flag","position_mode","mode","blend"})Integer(At(e,k));Boolean(At(e,"projected"));ResourcePath(At(e,"image"),true);
  for(const char *k:{"velocity_x_min","velocity_x_max","velocity_y_min","velocity_y_max"})Number(At(e,k));
  for(const char *k:{"configured_stage","configuration_kind"})Integer(At(e,k));if(!poop){Integer(At(e,"render_pass"));Number(At(e,"depth_scale"));}
  const auto &instances=Array(At(e,"instances"),4096);if(Integer(e["count"])!=int(instances.Size()))Fail("native_room_grid_fx_count");
  for(const auto &p:instances.GetArray()){for(const char *k:{"x","y","vx","vy"})Number(At(p,k));Boolean(At(p,"flip_x"));Boolean(At(p,"flip_y"));}
 };for(const auto &e:Array(At(v,"layers"),256).GetArray())entry(e,false);if(!At(v,"poop").IsNull())entry(v["poop"],true);else if(Integer(v["poop_countdown"])>0)Fail("native_room_grid_poop_fx");
 for(const auto &n:Array(At(v,"poop_color"),11,true).GetArray())Number(n);
 for(const char *k:{"player_anchor","vein_anchor","bounds_start","bounds_end"})VectorValue(At(v,k));
 const auto &groups=Array(At(v,"rays"),256);const int selected=Integer(v["selected_ray"]);if(selected<-1||selected>=int(groups.Size()))Fail("native_room_grid_fx_selected_ray");
 for(const auto &e:groups.GetArray()){
  for(const char *k:{"x_min","x_max","y_min","y_max","ray_spacing","group_spacing","phase"})Number(At(e,k));Boolean(At(e,"enabled"));Boolean(At(e,"alternate"));
  for(const auto &p:Array(At(e,"rays"),4096).GetArray()){for(const char *k:{"x","y","opacity"})Number(At(p,k));ResourcePath(At(p,"image"),true);}
 }
 unsigned nodes=0;auto vein=[&](auto &&self,const Value &e,unsigned depth)->void{if(depth>6||++nodes>4096)Fail("native_room_grid_vein_budget");
  for(const char *k:{"rotation","scale","opacity","x","y"})Number(At(e,k));for(const char *k:{"sprite","frame","mode","depth"})Integer(At(e,k));Boolean(At(e,"active"));
  if(Integer(e["sprite"])<0||Integer(e["sprite"])>1)Fail("native_room_grid_vein_sprite");for(const auto &c:Array(At(e,"children"),3,true).GetArray())if(!c.IsNull())self(self,c,depth+1);
 };
 for(const auto &e:Array(At(v,"veins"),10,true).GetArray())vein(vein,e,0);
 for(const auto &e:Array(At(v,"sprites"),2,true).GetArray())SpriteValue(e);
}
void ConfigValue(const Value &v){const auto &f=At(ResourceState(v),"fields");
 for(const char *k:{"StageID","Mode","Type","Subtype","Shape","Width","Height","Difficulty"})Integer(At(f,(std::string("RoomConfigRoom.")+k).c_str()));
 for(const char *k:{"Variant","Doors","OriginalVariant"})Unsigned32(At(f,(std::string("RoomConfigRoom.")+k).c_str()));String(At(f,"RoomConfigRoom.Name"));Number(At(f,"RoomConfigRoom.InitialWeight"));Number(At(f,"RoomConfigRoom.Weight"));
 const auto &spawns=Array(At(At(f,"RoomConfigRoom.Spawns"),"values"),65535);if(Integer(At(f,"RoomConfigRoom.SpawnCount"))!=int(spawns.Size()))Fail("native_room_grid_config_spawns");
 for(const auto &spawn:spawns.GetArray()){const auto &s=At(spawn,"fields");for(const char *k:{"RoomConfigSpawn.X","RoomConfigSpawn.Y","RoomConfigSpawn.EntryCount"})Integer(At(s,k));Number(At(s,"RoomConfigSpawn.SumWeights"));
  const auto &entries=Array(At(At(s,"RoomConfigSpawn.Entries"),"values"),255);if(Integer(s["RoomConfigSpawn.EntryCount"])!=int(entries.Size()))Fail("native_room_grid_config_entries");
  for(const auto &entry:entries.GetArray()){const auto &e=At(entry,"fields");for(const char *k:{"RoomConfigEntry.Type","RoomConfigEntry.Variant","RoomConfigEntry.Subtype"})Unsigned32(At(e,k));Number(At(e,"RoomConfigEntry.Weight"));}
 }
}
void MotionValue(const Value &v){if(!v.IsObject()||v.MemberCount()>914)Fail("native_room_grid_motion_budget");
 for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){const auto &pose=i->value;Boolean(At(pose,"overlay_first"));for(const char *k:{"main","overlay"}){
  const auto &s=At(pose,k);if(s.IsNull())continue;String(At(s,"animation"));Number(At(s,"frame"));std::set<int> ids;
  for(const auto &l:Array(At(s,"layers"),64).GetArray()){const int id=Integer(At(l,"id")),key=Integer(At(l,"key"));if(id<0||id>=64||key<-1||key>65535||!ids.insert(id).second)Fail("native_room_grid_motion_key");}
  for(const auto &n:Array(At(s,"nulls"),64).GetArray())if(Integer(n)<-1||Integer(n)>65535)Fail("native_room_grid_motion_null");
 }}
}
}

NativeRoomGridTarget PrepareNativeRoomGrid(const std::string &text){
  const Limits limits{32u<<20,4194304,64,2u<<20};
  NativeRoomGridTarget out;out.model=Parse(text,limits);ValidateModel(out.model,limits);
  out.scope=ReadScope(out.model["scope"]);out.revision=Unsigned(out.model["revision"]);
  if(!out.scope.run||!out.scope.floor||!out.scope.room)Fail("native_room_grid_scope");
  const auto &components=out.model["components"];
  for(const char *name:{"grid","room"}){
    const auto &component=At(components,name);
    if(At(component,"coverage")!="Complete"||At(component,"membership_complete")!=true)
      Fail("native_room_grid_partial_membership");
    Complete(At(component,"state"));
  }
  const auto &room=Room(out),&room_state=At(room,"state"),&fields=At(room_state,"fields");
  ResourceValues(room);ResourceValues(At(At(components,"grid"),"state"));
  // 最小原生上下文和资源在任何SDK写入之前完整解码；未知依赖不得进入半套原生世界。
  const auto &context=At(room,"native_context");
  EnvironmentValue(At(context,"environment"));
  MotionValue(At(context,"motion"));
  for(const auto &v:Array(At(context,"color_modifier"),6,true).GetArray())Number(v);
  Number(At(context,"lighting_blend"));Number(At(context,"lighting_intensity"));
  const auto &water=At(context,"water_lerp");ColorValue(At(water,"target_color"),false);ColorValue(At(water,"target_multiplier"),false);
  Integer(At(water,"duration"));Integer(At(water,"countdown"));
  const auto &decor=At(context,"wall_decor"),&decor_rng=At(decor,"entry_rng");
  if(!Unsigned32(At(decor_rng,"seed")))Fail("native_room_grid_decor_seed");
  for(const char *k:{"shift1","shift2","shift3"}){const auto n=Integer(At(decor_rng,k));if(n<1||n>31)Fail("native_room_grid_decor_shift");}
  for(const auto &v:Array(At(decor,"choices"),4096).GetArray()){
   for(const char *k:{"x","y","layer","kind"})Integer(At(v,k));Number(At(v,"px"));Number(At(v,"py"));Boolean(At(v,"flip_x"));Boolean(At(v,"flip_y"));
   if(!Unsigned32(At(v,"after_seed")))Fail("native_room_grid_decor_seed");
  }
  for(const auto &v:Array(At(context,"grid_inside"),448,true).GetArray())Boolean(v);
  for(const auto &v:Array(At(context,"grid_birth"),448,true).GetArray())if(!v.IsNull())Unsigned32(v);
  for(const char *name:{"stage","stage_type","curses","difficulty","challenge","level_flags","render_random"})Unsigned32(At(context,name));
  for(const char *name:{"current_room","previous_room","starting_room","dimension","enter_door","leave_door","game_frame"})Integer(At(context,name));
  if(Integer(At(context,"controls_character"))<0||Integer(context["controls_character"])>40)Fail("native_room_grid_controls_character");
  Unsigned(At(context,"game_flags"));
  const int current_index=Integer(context["current_room"]),dimension_index=Integer(context["dimension"]);
  if(current_index==-19||current_index<-20||current_index>168||dimension_index<0||dimension_index>2)Fail("native_room_grid_context_location");
  std::set<std::pair<int,int>> descriptor_keys;
  for(const auto &d:Array(At(context,"descriptors"),25).GetArray()){
    const auto index=Integer(At(d,"lookup_index")),dim=Integer(At(d,"lookup_dimension"));
    if(index==-19||dim<0||dim>2||!descriptor_keys.emplace(index,dim).second)Fail("native_room_grid_descriptor_key");
    for(const char *name:{"grid_index","safe_grid_index","list_index","dimension","allowed_doors","display_flags","visited_count","flags",
        "clear_count","pits_count","group","greed_subtype","delirium_distance"})Integer(At(d,name));
    for(const char *name:{"decoration_seed","spawn_seed","award_seed","boss_death_seed"})Unsigned32(At(d,name));
    for(const auto &door:Array(At(d,"doors"),8,true).GetArray())Integer(door);
    const auto &config=At(d,"data");
    for(const char *name:{"stage","mode","type","subtype","shape","width","height"})Integer(At(config,name));
    for(const char *name:{"variant","flags","quest_doors"})Unsigned32(At(config,name));
  }
  if(!descriptor_keys.count({current_index,dimension_index}))Fail("native_room_grid_current_descriptor");
  Fields(room_state,"Room","room-level-catalog.json");
  const auto shape=Integer(At(fields,"Room.GetRoomShape"));
  if(shape<1||shape>12)Fail("native_room_grid_shape");out.shape=unsigned(shape);
  Fields(At(room,"config"),"RoomConfigRoom","room-level-catalog.json");
  ConfigValue(room["config"]);
  if(Integer(At(At(ResourceState(room["config"]),"fields"),"RoomConfigRoom.Shape"))!=shape)
    Fail("native_room_grid_config_shape");
  const auto &override_config=At(room,"override_config");
  if(!(override_config.IsObject()&&override_config.HasMember("status")&&override_config["status"]=="Null"))
    {Fields(override_config,"RoomConfigRoom","room-level-catalog.json");ConfigValue(override_config);}
  const auto &seeds=At(room,"seeds");
  for(const char *name:{"spawn","decoration","award"})Unsigned32(At(seeds,name));
  Queries(fields,"Room.GetGridCollision",out.collision);Queries(fields,"Room.GetGridPath",out.path);
  Queries(fields,"Room.GetRail",out.rail,true);
  for(auto n:out.collision)if(n<0||n>5)Fail("native_room_grid_collision");
  for(auto n:out.rail)if(n<0||n>255)Fail("native_room_grid_rail");
  // 实际坐标来自12的完整有限查询；这里不从shape/seed重算另一份布局。
  std::set<unsigned> position_indices;
  for(const auto &entry:Array(At(At(fields,"Room.GetGridPosition"),"values"),448,true).GetArray()){
    const auto &parameters=At(entry,"parameters");int index=-1;
    if(parameters.IsArray()&&parameters.Size()==1)index=Integer(parameters[0]);
    else index=Integer(At(Array(At(parameters,"entries"),1,true)[0],"value"));
    if(index<0||index>=448||!position_indices.insert(unsigned(index)).second)Fail("native_room_grid_query_index");
    const auto &position=At(entry,"value");
    if(At(position,"type")!="vector")Fail("native_room_grid_coordinate");
    out.positions[unsigned(index)]={Number(At(position,"x")),Number(At(position,"y"))};
  }
  const auto &grid=At(At(components,"grid"),"state");
  const auto &membership=At(grid,"membership");
  if(At(membership,"complete")!=true||Unsigned(At(membership,"unavailable")))Fail("native_room_grid_membership");
  std::set<std::pair<unsigned,unsigned>> slots;
  std::set<unsigned> grid_indices;
  for(const auto &object:Array(At(grid,"objects"),448).GetArray()){
    NativeRoomGridTarget::Grid item;
    const auto &subject=At(object,"subject");if(At(subject,"status")!="KnownCurrent")Fail("native_room_grid_subject");
    item.id=Key(At(subject,"target"),out.scope.run,2);item.index=Index(At(object,"index"),448);
    if(context["grid_birth"][item.index].IsNull())Fail("native_room_grid_missing_birth_clock");
    if(!grid_indices.insert(item.index).second)Fail("native_room_grid_duplicate_native_index");
    item.type=Index(At(object,"type"),28);item.variant=Integer(At(object,"variant"));
    const auto name=std::string("GridEntity")+GridClass(item.type);
    if(At(object,"class")!=name.c_str())Fail("native_room_grid_class");
    Fields(At(object,"common"),"GridEntity","grid-catalog.json");
    SpriteValue(At(At(object,"common"),"fields")["GridEntity.GetSprite"]);
    Fields(At(At(object,"bodies"),name.c_str()),name,"grid-catalog.json");
    Fields(At(At(object["common"],"fields"),"GridEntity.GetSaveState"),"GridEntityDesc","grid-catalog.json");
    const auto &common=object["common"]["fields"];
    const auto &desc=common["GridEntity.GetSaveState"]["fields"];
    auto &d=item.descriptor;
    d.type=Integer(At(desc,"GridEntityDesc.Type"));d.variant=Integer(At(desc,"GridEntityDesc.Variant"));
    d.state=Integer(At(desc,"GridEntityDesc.State"));d.spawn_count=Integer(At(desc,"GridEntityDesc.SpawnCount"));
    d.var_data=Integer(At(desc,"GridEntityDesc.VarData"));d.spawn_seed=Unsigned32(At(desc,"GridEntityDesc.SpawnSeed"));
    d.variable_seed=Unsigned32(At(desc,"GridEntityDesc.VariableSeed"));
    if(!At(desc,"GridEntityDesc.Initialized").IsBool())Fail("native_room_grid_descriptor_initialized");
    d.initialized=desc["GridEntityDesc.Initialized"].GetBool();
    item.collision=Integer(At(common,"GridEntity.CollisionClass"));
    if(d.type!=int(item.type)||d.variant!=item.variant||Integer(At(common,"GridEntity.GetType"))!=d.type||
       Integer(At(common,"GridEntity.GetVariant"))!=d.variant||Integer(At(common,"GridEntity.State"))!=d.state||
       Integer(At(common,"GridEntity.GetGridIndex"))!=int(item.index))Fail("native_room_grid_descriptor_alias");
    if(item.collision<0||item.collision>5)Fail("native_room_grid_collision");
    bool owner=false;
    for(const auto &m:Array(At(object,"memberships"),16).GetArray()){
      const auto source=Index(At(m,"source"),14),index=Index(At(m,"index"),source==12?8:448);
      if(source!=11&&source!=12&&source!=13)Fail("native_room_grid_membership_source");
      if(!slots.emplace(source,index).second)Fail("native_room_grid_duplicate_slot");
      if(source==12){if(item.type!=16)Fail("native_room_grid_door_type");item.door_slots.insert(index);}
      else {if(index!=item.index)Fail("native_room_grid_slot_index");owner=true;}
    }
    if(!owner&&item.door_slots.empty())Fail("native_room_grid_missing_owner");
    item.state=Copy(object);
    if(!out.grids.emplace(item.id,std::move(item)).second)Fail("native_room_grid_duplicate_subject");
  }
  // 双向核对主成员表，不能仅凭objects完整就忽略显式membership中的遗漏或额外主体。
  std::set<std::uint64_t> membership_ids;
  for(const auto &entry:Array(At(membership,"entries"),448).GetArray()){
    const auto &subject=At(entry,"subject");if(At(subject,"status")!="KnownCurrent")Fail("native_room_grid_membership_subject");
    const auto id=Key(At(subject,"target"),out.scope.run,2);const auto found=out.grids.find(id);
    if(found==out.grids.end()||!membership_ids.insert(id).second||
       At(entry,"memberships")!=found->second.state["memberships"]||
       At(entry,"index")!=found->second.state["index"])Fail("native_room_grid_membership_mismatch");
  }
  if(membership_ids.size()!=out.grids.size())Fail("native_room_grid_membership_mismatch");
  std::set<std::uint64_t> room_grid_ids;
  for(const auto &entry:Array(At(room,"grids"),448).GetArray()){
    const auto &subject=At(entry,"subject");
    if(At(subject,"status")!="KnownCurrent")Fail("native_room_grid_room_member");
    const auto id=Key(At(subject,"target"),out.scope.run,2);const auto found=out.grids.find(id);
    if(found==out.grids.end()||!room_grid_ids.insert(id).second||
       At(entry,"memberships")!=found->second.state["memberships"])Fail("native_room_grid_room_membership");
  }
  if(room_grid_ids.size()!=out.grids.size())Fail("native_room_grid_room_membership");
  std::set<unsigned> seen_doors;
  for(const auto &door:Array(At(room,"doors"),8,true).GetArray()){
    const auto slot=Index(At(door,"slot"),8);if(!seen_doors.insert(slot).second)Fail("native_room_grid_duplicate_door");
    const auto &actual=At(door,"actual");
    // 15保留无门观测为resolution=Null；已解析引用才使用status/target。
    if((actual.HasMember("resolution")&&actual["resolution"]=="Null")||
       (actual.HasMember("status")&&actual["status"]=="Null")){
      if(slots.count({12,slot}))Fail("native_room_grid_door_alias");continue;
    }
    if(actual["status"]!="KnownCurrent")Fail("native_room_grid_door_reference");
    const auto id=Key(At(actual,"target"),out.scope.run,2);const auto found=out.grids.find(id);
    if(found==out.grids.end()||!found->second.door_slots.count(slot)||At(door,"shape_allowed")!=true)
      Fail("native_room_grid_door_alias");out.doors[slot]=id;
    const auto &door_fields=found->second.state["bodies"]["GridEntityDoor"]["fields"];
    if(Integer(At(door_fields,"GridEntityDoor.Slot"))!=int(slot)||
       Integer(At(door_fields,"GridEntityDoor.TargetRoomIndex"))!=Integer(At(door,"actual_target")))
      Fail("native_room_grid_door_route");
  }
  // 本域引用必须存在；其他域仅携带规范依赖，绝不把尚未应用的Entity引用伪造为nullptr。
  auto references=[&](auto &&self,const Value &v)->void{
    if(v.IsArray())for(const auto &e:v.GetArray())self(self,e);
    if(!v.IsObject())return;
    if(v.HasMember("type")&&v["type"]=="reference"){
      const auto status=String(At(v,"status"));
      if(status=="KnownCurrent"){
        const auto &key=At(v,"target");const auto space=Unsigned(At(key,"space"));
        if(space==2){if(!out.grids.count(Key(key,out.scope.run,2)))Fail("native_room_grid_missing_reference");}
        else out.external_dependencies.insert(Text(key));
      }else if(status!="Null"&&status!="NotApplicable"&&status!="KnownHistorical"&&status!="Retired")
        Fail("native_room_grid_unresolved_reference");
    }
    for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)self(self,i->value);
  };
  references(references,room);references(references,grid);return out;
}

NativeRoomGridPlan PlanNativeRoomGrid(const NativeRoomGridTarget *current,const NativeRoomGridTarget &target,bool native_state_valid){
  NativeRoomGridPlan plan;
  if(current){
    if(current->scope!=target.scope)Fail("native_room_grid_scope_changed");
    if(target.revision<current->revision){plan.disposition=NativeRoomGridDisposition::Stale;return plan;}
    if(target.revision==current->revision){
      // 只比较本域内容；其他域的模型字段不能让相同Room/Grid版本重复执行原生操作。
      if(Room(*current)!=Room(target)||current->model["components"]["grid"]["state"]!=target.model["components"]["grid"]["state"])
        Fail("native_room_grid_revision_conflict");
      if(native_state_valid){plan.disposition=NativeRoomGridDisposition::Duplicate;return plan;}
    }
  }
  bool structure_changed=false;
  if(current){const auto &before=Room(*current)["state"]["fields"],&after=Room(target)["state"]["fields"];
    for(const char *key:{"Room.GetRoomShape","Room.GetGridWidth","Room.GetGridHeight"})if(before[key]!=after[key])structure_changed=true;
  }
  plan.reset_room=!native_state_valid||structure_changed;
  const auto *native_current=native_state_valid&&!structure_changed?current:nullptr;
  plan.create_room=!native_current;plan.change_room=native_current&&Room(*native_current)!=Room(target);
  if(native_current)for(const auto &[id,old]:native_current->grids)if(!target.grids.count(id)){
    plan.remove.push_back(id);plan.affected_indices.insert(old.index);
  }
  for(const auto &[id,next]:target.grids){
    const auto old=native_current?native_current->grids.find(id):target.grids.end();
    if(!native_current||old==native_current->grids.end())plan.create.push_back(id);
    else if(old->second.state!=next.state){
      plan.change.push_back(id);
      if(old->second.type!=next.type||old->second.index!=next.index)plan.rebuild.push_back(id);
      plan.affected_indices.insert(old->second.index);
    }else continue;
    plan.affected_indices.insert(next.index);
  }
  return plan;
}
} // namespace cs::replica
