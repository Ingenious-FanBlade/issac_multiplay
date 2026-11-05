#include "replica/room_grid_target.h"
#include "replica/room_geometry.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <set>
#include <map>
#include <string_view>
#include <tuple>

namespace cs::replica {
namespace {
using V=Value;
[[noreturn]] void Error(const std::string &path,const char *why){throw ContractError("room_grid:"+path+":"+why);}
const V &At(const V &v,const char *name,const std::string &path){
  if(!v.IsObject()||!v.HasMember(name))Error(path+"/"+name,"required_missing");return v[name];
}
bool Is(const V &v,const char *s){return v.IsString()&&std::string_view(v.GetString(),v.GetStringLength())==s;}
const V &Scalar(const V &v){return v.IsObject()&&v.HasMember("type")&&Is(v["type"],"int64")&&v.HasMember("value")?v["value"]:v;}
std::int64_t Integer(const V &v,const std::string &path){
  const auto &x=Scalar(v);std::int64_t n=0;
  if(x.IsInt64())return x.GetInt64();
  if(x.IsUint64()&&x.GetUint64()<=INT64_MAX)return std::int64_t(x.GetUint64());
  if(x.IsString()){const auto r=std::from_chars(x.GetString(),x.GetString()+x.GetStringLength(),n);
    if(r.ec==std::errc{}&&r.ptr==x.GetString()+x.GetStringLength())return n;}
  Error(path,"integer_required");
}
int Number(const V &v,const std::string &path,int low=INT32_MIN,int high=INT32_MAX){
  const auto n=Integer(v,path);if(n<low||n>high)Error(path,"integer_range");return int(n);
}
int Field(const V &v,const char *name,const std::string &path,int low=INT32_MIN,int high=INT32_MAX){return Number(At(v,name,path),path+"/"+name,low,high);}
std::uint64_t U64(const V &v,const std::string &path){try{return Unsigned(Scalar(v));}catch(const ContractError &){Error(path,"unsigned_required");}}
std::uint32_t U32(const V &v,const std::string &path){auto n=U64(v,path);if(n>UINT32_MAX)Error(path,"unsigned32_range");return std::uint32_t(n);}
// descriptor三个seed在J273 SDK声明为int32，公开绑定可能给出负数；保留其32位原值语义。
std::uint32_t SeedBits(const V &v,const std::string &path){const auto n=Integer(v,path);if(n<INT32_MIN||n>UINT32_MAX)Error(path,"seed_bits_range");return std::uint32_t(n);}
bool Boolean(const V &v,const std::string &path){if(!v.IsBool())Error(path,"boolean_required");return v.GetBool();}
const V &Fields(const V &v,const std::string &path){const auto &f=At(v,"fields",path);if(!f.IsObject())Error(path,"fields_required");return f;}
// 正式Lua编码保留空table为{}；只有精确空对象可作为有来源的空集合，不接受带status的对象。
const V &Sequence(const V &v,const std::string &path){
  if(v.IsArray())return v;if(v.IsObject()&&v.MemberCount()==0){static const V empty(rapidjson::kArrayType);return empty;}Error(path,"array_required");
}
bool Null(const V &v){return v.IsNull()||(v.IsObject()&&((v.HasMember("status")&&Is(v["status"],"Null"))||(v.HasMember("resolution")&&Is(v["resolution"],"Null"))));}
SubjectKey Key(const V &v,const std::string &path,std::uint64_t run,unsigned space){
  SubjectKey k{U64(At(v,"run",path),path+"/run"),U64(At(v,"id",path),path+"/id"),unsigned(Field(v,"space",path,1,14))};
  if(k.run!=run||!k.id||k.space!=space)Error(path,"key_scope_or_space");return k;
}
DomainReference Ref(const V &v,const std::string &path,std::uint64_t run,unsigned space,bool nullable=false){
  if(Null(v)){if(!nullable)Error(path,"required_reference_null");if(v.IsObject()&&v.HasMember("target"))Error(path,"null_has_target");return {true,{}};}
  if(!Is(At(v,"type",path),"reference")||!Is(At(v,"status",path),"KnownCurrent"))Error(path,"reference_unresolved");
  return {false,Key(At(v,"target",path),path+"/target",run,space)};
}
// 配置私有Flags/QuestDoors/MergedData没有完整公开接口；正式只读来源补齐，而非在Client猜值。
RoomConfiguration ConfigValue(const V &v,const std::string &path,const std::string &version,unsigned depth,unsigned &entries){
  RoomConfiguration c;c.content_version=version;if(Null(v))return c;
  if(depth>8||Field(v,"version",path)!=1)Error(path,"config_content_version_or_depth");
  c.is_null=false;c.stage=Field(v,"stage",path,0,36);c.mode=Field(v,"mode",path,0,1);c.type=Field(v,"type",path,1,29);
  c.variant=U32(At(v,"variant",path),path);c.subtype=Field(v,"subtype",path,0);c.shape=Field(v,"shape",path,1,12);
  c.width=Field(v,"width",path,1,26);c.height=Field(v,"height",path,1,14);c.difficulty=Field(v,"difficulty",path);
  c.original_variant=U32(At(v,"original_variant",path),path);c.doors=U32(At(v,"doors",path),path);
  c.quest_doors=U32(At(v,"quest_doors",path),path);c.flags=U32(At(v,"flags",path),path);
  const auto real=[&](const char *name){const auto &n=At(v,name,path);if(!n.IsNumber()||!std::isfinite(n.GetDouble())||std::abs(n.GetDouble())>1e30)Error(path+"/"+name,"finite_config_number");return float(n.GetDouble());};
  c.initial_weight=real("initial_weight");c.weight=real("weight");const auto &name=At(v,"name",path);
  if(!name.IsString()||name.GetStringLength()>4096)Error(path,"config_name");c.name.assign(name.GetString(),name.GetStringLength());
  const auto &spawns=Sequence(At(v,"spawns",path),path);if(spawns.Size()>65535)Error(path,"config_spawn_budget");
  for(const auto &s:spawns.GetArray()){
    RoomSpawnValue spawn;spawn.x=std::uint16_t(Field(s,"x",path,0,65535));spawn.y=std::uint16_t(Field(s,"y",path,0,65535));
    const auto &sum=At(s,"sum_weights",path);if(!sum.IsNumber()||!std::isfinite(sum.GetDouble())||std::abs(sum.GetDouble())>1e30)Error(path,"config_weight");spawn.sum_weights=float(sum.GetDouble());
    const auto &items=Sequence(At(s,"entries",path),path);if(items.Size()>255)Error(path,"config_entry_budget");
    if(entries+items.Size()+1>262144)Error(path,"config_total_budget");entries+=items.Size()+1;
    for(const auto &e:items.GetArray()){
      RoomEntryValue entry;entry.type=U32(At(e,"type",path),path);entry.variant=std::uint16_t(Field(e,"variant",path,0,65535));entry.subtype=std::uint16_t(Field(e,"subtype",path,0,65535));
      const auto &w=At(e,"weight",path);if(!w.IsNumber()||!std::isfinite(w.GetDouble())||std::abs(w.GetDouble())>1e30)Error(path,"config_entry_weight");entry.weight=float(w.GetDouble());spawn.entries.push_back(entry);
    }
    c.spawns.push_back(std::move(spawn));
  }
  const auto &merged=At(v,"merged",path);if(!merged.IsArray()||merged.Size()!=2)Error(path,"config_merged_pair");
  for(const auto &m:merged.GetArray())c.merged.push_back(ConfigValue(m,path+"/merged",version,depth+1,entries));
  return c;
}
RoomConfiguration Config(const V &v,const std::string &path,const std::string &version){
  RoomConfiguration c;c.content_version=version;if(Null(v))return c;
  unsigned entries=0;c=ConfigValue(At(v,"application_config",path),path+"/application_config",version,0,entries);
  const auto &f=Fields(At(v,"state",path),path+"/state");
  if(c.is_null||c.stage!=Field(f,"RoomConfigRoom.StageID",path)||c.mode!=Field(f,"RoomConfigRoom.Mode",path)||
     c.type!=Field(f,"RoomConfigRoom.Type",path)||c.variant!=U32(At(f,"RoomConfigRoom.Variant",path),path)||
     c.subtype!=Field(f,"RoomConfigRoom.Subtype",path)||c.shape!=Field(f,"RoomConfigRoom.Shape",path)||
     c.width!=Field(f,"RoomConfigRoom.Width",path)||c.height!=Field(f,"RoomConfigRoom.Height",path))Error(path,"configuration_source_conflict");
  return c;
}
DescriptorLocation Placement(const V &p,const V &fields,const std::string &path){
  if(!Is(At(p,"status",path),"Value")||Field(p,"version",path)!=1)Error(path,"placement_source_version");
  DescriptorLocation d;d.grid_index=Field(p,"grid_index",path);d.safe_grid_index=Field(p,"safe_grid_index",path);
  d.list_index=Field(p,"list_index",path);d.dimension=Field(p,"dimension_index",path,0,2);
  d.storage_index=Field(p,"storage_index",path,-1,506);
  if(p.HasMember("source_slot"))d.source_slot=Field(p,"source_slot",path,-20,-1);
  if(d.source_slot==-19)Error(path,"original_online_excluded");
  if(d.source_slot){if(d.storage_index!=-1)Error(path,"off_grid_storage_conflict");}
  else if(d.storage_index<0||d.list_index!=d.storage_index||(d.dimension!=1&&(d.grid_index<0||d.grid_index>168))||d.safe_grid_index<0||d.safe_grid_index>168)Error(path,"map_location_range");
  const auto &a=Sequence(At(p,"occupied_indices",path),path+"/occupied_indices");if(a.Size()>4)Error(path,"occupied_budget");
  std::set<int> indices;for(const auto &x:a.GetArray())if(!indices.insert(Number(x,path+"/occupied_indices",0,168)).second)Error(path,"duplicate_occupied_index");
  d.occupied.assign(indices.begin(),indices.end());
  if(d.source_slot&&!d.occupied.empty())Error(path,"off_grid_has_map_offsets");
  // Mineshaft生成可保留Data非空但已不占图的descriptor；它仍有存储身份，不补回旧占用。
  if(!d.source_slot&&!d.occupied.empty()&&!indices.count(d.safe_grid_index))Error(path,"safe_index_not_occupied");
  if(d.grid_index!=Field(fields,"RoomDescriptor.GridIndex",path)||d.safe_grid_index!=Field(fields,"RoomDescriptor.SafeGridIndex",path)||
     d.list_index!=Field(fields,"RoomDescriptor.ListIndex",path)||d.dimension!=Field(fields,"RoomDescriptor.GetDimension",path,0,2))Error(path,"placement_fields_disagree");
  return d;
}
RoomDescriptorTarget Descriptor(const V &subject,const V &state,const V &placement,const std::string &path,std::uint64_t run,const std::string &version){
  RoomDescriptorTarget d;d.key=Ref(subject,path+"/subject",run,6).key;
  d.construction_revision=U64(At(subject,"construction_revision",path),path+"/construction_revision");
  if(!d.construction_revision)Error(path,"construction_revision_zero");
  const auto &f=Fields(state,path);d.location=Placement(placement,f,path+"/placement");
  d.data=Config(At(f,"RoomDescriptor.Data",path),path+"/Data",version);
  d.override_data=Config(At(f,"RoomDescriptor.OverrideData",path),path+"/OverrideData",version);
  d.location.native_grid_origin=d.location.grid_index;
  const auto &shape_config=d.data.is_null?d.override_data:d.data;
  if(!d.location.source_slot&&d.location.dimension==1&&!shape_config.is_null&&shape_config.shape==1&&
     d.location.occupied.size()==1&&d.location.occupied[0]==d.location.safe_grid_index){
    // J273镜像样本的公开GridIndex出现0x00600060，含义未确认，不能送入native数组/坐标转换。
    // 1x1实际offset已唯一确定原点；只用这个真实几何事实构造本方native缓存，Source raw不改、不冒认等价。
    d.location.native_grid_origin=d.location.occupied[0];
    d.location.raw_grid_index_adopted=d.location.grid_index==d.location.native_grid_origin;
  }
  if(!d.location.source_slot&&!d.location.occupied.empty()){const auto &c=d.data.is_null?d.override_data:d.data;std::array<int,4> expected{};unsigned count=0;
    if(c.is_null||!RoomShapeCells(d.location.native_grid_origin,c.shape,expected,count)||count!=d.location.occupied.size()||
       !std::equal(expected.begin(),expected.begin()+count,d.location.occupied.begin()))Error(path,"shape_occupancy_disagrees");}
  if(!d.location.source_slot&&(d.location.native_grid_origin<0||d.location.native_grid_origin>168))Error(path,"native_grid_origin_range");
  d.allowed_doors=Field(f,"RoomDescriptor.AllowedDoors",path,0,255);d.flags=Field(f,"RoomDescriptor.Flags",path);
  d.display_flags=Field(f,"RoomDescriptor.DisplayFlags",path);
  d.visited=Field(f,"RoomDescriptor.VisitedCount",path,0);d.clear_count=Field(f,"RoomDescriptor.ClearCount",path,0,32767);
  d.spawn_seed=SeedBits(At(f,"RoomDescriptor.SpawnSeed",path),path+"/SpawnSeed");d.decoration_seed=SeedBits(At(f,"RoomDescriptor.DecorationSeed",path),path+"/DecorationSeed");d.award_seed=SeedBits(At(f,"RoomDescriptor.AwardSeed",path),path+"/AwardSeed");
  const auto &doors=At(f,"RoomDescriptor.Doors",path);if(!doors.IsArray()||doors.Size()!=8)Error(path,"door_slots_required");
  unsigned seen=0;for(const auto &x:doors.GetArray()){const int slot=Field(x,"slot",path,0,7);if(seen&(1u<<slot))Error(path,"duplicate_door_slot");seen|=1u<<slot;d.door_targets[slot]=Field(x,"target_grid_index",path);}
  return d;
}
const V &Component(const V &model,const char *name,DomainCapture &capture){
  const std::string path=std::string("components/")+name;const auto &c=At(At(model,"components","model"),name,"components");
  if(!Is(At(c,"coverage",path),"Complete")||!Boolean(At(c,"membership_complete",path),path))Error(path,"complete_required");
  capture.version=U64(At(c,"version",path),path);const auto &p=At(c,"capture",path);
  capture.scope=U64(At(p,"scope",path),path);capture.sequence=U64(At(p,"sequence",path),path);capture.tick=U64(At(p,"authority_tick",path),path);
  return At(c,"state",path);
}
// 检查资源路径而不加载资源；未知必需姿态/资源由具体adapter拒绝，不在解析中伪造就绪。
void Resources(const V &v,const std::string &path){
  if(v.IsArray()){unsigned n=0;for(const auto &x:v.GetArray())Resources(x,path+"/"+std::to_string(n++));}
  if(!v.IsObject())return;
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){
    const std::string k(i->name.GetString(),i->name.GetStringLength());
    if((k=="logical_path"||k=="image"||k=="Sprite.GetFilename"||k=="LayerState.GetSpritesheetPath")&&i->value.IsString()){
      std::string s(i->value.GetString(),i->value.GetStringLength());std::replace(s.begin(),s.end(),'\\','/');
      if(s.size()>1024||(!s.empty()&&s.front()=='/')||s.find(':')!=s.npos||std::any_of(s.begin(),s.end(),[](unsigned char c){return c<32;}))Error(path+"/"+k,"resource_path");
      std::size_t start=0;do{const auto end=s.find('/',start);if(s.substr(start,end==s.npos?s.npos:end-start)=="..")Error(path+"/"+k,"resource_path");if(end==s.npos)break;start=end+1;}while(true);
    }
    Resources(i->value,path+"/"+k);
  }
}
}
const Value &RoomGridTarget::GridState(const GridTarget &g) const {return source["components"]["grid"]["state"]["objects"][g.source_index];}
const Value &RoomGridTarget::DescriptorState(const RoomDescriptorTarget &d) const {
  const auto &level=source["components"]["level"]["state"];
  if(d.source_kind==1)return level["descriptors"][d.source_index]["state"];
  if(d.source_kind==2)return level["off_grid"]["entries"][d.source_index]["state"];
  return level["current_descriptor"];
}

// 解析先完成全部有界纯值校验，最后发布不可变T；无SDK调用，也不依赖录制文件。
std::shared_ptr<const RoomGridTarget> ParseRoomGridTarget(const V &model,RoomGridLimits limits){
  if(!limits.descriptors||limits.descriptors>526||!limits.grids||limits.grids>456||!limits.references||limits.references>4096)Error("limits","only_tightening_allowed");
  const auto usage=MeasureRoomGridBudget(model);
  ValidateModel(model);if(model["publishable"]!=true)Error("model","not_publishable");
  auto out=std::shared_ptr<RoomGridTarget>(new RoomGridTarget);out->budget=usage;out->scope=ReadScope(model["scope"]);out->revision=Unsigned(model["revision"]);
  const auto run=out->scope.run;if(!run||!out->scope.floor||!out->scope.dimension||!out->scope.room)Error("scope","world_scope_required");
  const auto &room=Component(model,"room",out->room_capture),&level=Component(model,"level",out->level_capture),&grid=Component(model,"grid",out->grid_capture);
  const auto &contract=At(room,"application_source","room"),&gc=At(grid,"application_source","grid");
  const auto grid_version=Field(gc,"version","grid/application_source"),room_version=Field(contract,"version","room/application_source");
  if((room_version!=4&&room_version!=5&&room_version!=6&&room_version!=7&&room_version!=8)||(grid_version!=3&&grid_version!=4&&grid_version!=5)||
     !Is(At(contract,"target","room"),"REPENTOGON-1.1.3/J273")||!Is(At(gc,"target","grid"),"REPENTOGON-1.1.3/J273"))Error("application_source","unsupported_contract");
  for(const auto *c:{&contract,&gc})if(c->HasMember("required_contracts")){
    const auto &required=(*c)["required_contracts"];if(!required.IsArray()||required.Size()>32)Error("application_source/required_contracts","budget");
    for(const auto &name:required.GetArray())if(!Is(name,"descriptor-placement-v1")&&!Is(name,"grid-membership-v2")&&!Is(name,"resource-pose-v1")&&!Is(name,"room-configuration-v1")&&!Is(name,"room-runtime-v2")&&!Is(name,"room-runtime-v3")&&!Is(name,"grid-runtime-v2")&&!Is(name,"grid-path-v1")&&!Is(name,"map-camera-v1")&&!Is(name,"map-frame-pose-v1")&&!Is(name,"backdrop-wall-trace-v1")&&!Is(name,"grid-sprite-alias-v1")&&!Is(name,"grid-member-sprite-pose-v1")&&!Is(name,"room-fx-applicability-v1"))Error("application_source/required_contracts/"+(name.IsString()?std::string(name.GetString(),name.GetStringLength()):"invalid"),"unknown_required");
  }
  const auto &cv=At(contract,"config_version","room");if(!cv.IsString()||cv.GetStringLength()==0||cv.GetStringLength()>256)Error("config_version","resource_version_required");
  const std::string config_version(cv.GetString(),cv.GetStringLength());
  if(config_version!="REPENTOGON-1.1.3/J273/room-configuration-v1")Error("config_version","unsupported_configuration_version");
  auto &r=out->room;r.current_index=Field(room,"room_index","room",-20,168);r.dimension=Field(room,"dimension_index","room",0,2);
  if(r.current_index==-19)Error("room/room_index","original_online_excluded");
  const auto &location=At(room,"location","room");r.key=Key(At(location,"room_instance","room/location"),"room/location/room_instance",run,7);r.floor=Key(At(location,"floor","room/location"),"room/location/floor",run,4);
  const auto &subject=At(room,"descriptor","room");r.descriptor=Ref(subject,"room/descriptor",run,6).key;
  const auto current=Descriptor(subject,At(level,"current_descriptor","level"),At(contract,"descriptor_placement","room"),"current_descriptor",run,config_version);
  if(current.location.source_slot?(r.current_index!=current.location.source_slot):(std::find(current.location.occupied.begin(),current.location.occupied.end(),r.current_index)==current.location.occupied.end()))Error("room/current_index","not_in_descriptor");
  if(r.dimension!=current.location.dimension)Error("room/dimension_index","current_descriptor_disagrees");
  const auto data=Config(At(room,"config","room"),"room/config",config_version),over=Config(At(room,"override_config","room"),"room/override_config",config_version);
  if(data!=current.data||over!=current.override_data)Error("room/config","current_descriptor_disagrees");
  const auto &effective=over.is_null?data:over;if(effective.is_null)Error("room/config","active_configuration_required");
  r.layout={effective.shape,effective.width,effective.height,448};r.descriptors.push_back(current);
  std::map<SubjectKey,unsigned> descriptors{{current.key,0}};std::map<std::pair<int,int>,SubjectKey> storage,occupied;
  auto add=[&](RoomDescriptorTarget d){auto found=descriptors.find(d.key);if(found!=descriptors.end()){
      const auto &old=r.descriptors[found->second];if(old.location!=d.location||old.construction_revision!=d.construction_revision||old.data!=d.data||old.override_data!=d.override_data||
        old.allowed_doors!=d.allowed_doors||old.flags!=d.flags||old.display_flags!=d.display_flags||old.visited!=d.visited||old.clear_count!=d.clear_count||old.spawn_seed!=d.spawn_seed||old.decoration_seed!=d.decoration_seed||old.award_seed!=d.award_seed||old.door_targets!=d.door_targets)Error("level/descriptors","conflicting_current_descriptor");return;}
    if(r.descriptors.size()>=limits.descriptors)Error("level/descriptors","descriptor_budget");descriptors.emplace(d.key,unsigned(r.descriptors.size()));r.descriptors.push_back(std::move(d));};
  const auto &list=Sequence(At(level,"descriptors","level"),"level/descriptors");if(list.Size()>507||list.Size()>limits.descriptors)Error("level/descriptors","descriptor_budget");
  const auto &lm=At(level,"membership","level");
  if(!Boolean(At(lm,"complete","level/membership"),"level/membership")||Field(lm,"size","level/membership",0,507)!=int(list.Size()))Error("level/membership","incomplete_or_size_conflict");
  unsigned position=0;for(const auto &d:list.GetArray()){
    auto parsed=Descriptor(At(d,"subject","level/descriptors"),At(d,"state","level/descriptors"),At(d,"placement","level/descriptors"),"level/descriptors/"+std::to_string(position),run,config_version);
    if(parsed.location.source_slot||parsed.location.storage_index!=int(position)||Field(d,"list_position","level/descriptors",0,506)!=int(position))Error("level/descriptors/"+std::to_string(position),"list_storage_conflict");
    parsed.source_kind=1;parsed.source_index=position++;add(std::move(parsed));
  }
  const auto &off=At(level,"off_grid","level");if(!Boolean(At(off,"complete","level/off_grid"),"level/off_grid"))Error("level/off_grid","incomplete");
  const auto &entries=At(off,"entries","level/off_grid");if(!entries.IsArray()||entries.Size()!=19)Error("level/off_grid","complete_slot_set_required");
  position=0;std::set<int> slots;for(const auto &entry:entries.GetArray()){
    const int slot=Field(entry,"slot","level/off_grid",-20,-1);if(slot==-19||!slots.insert(slot).second)Error("level/off_grid","slot_conflict_or_online");
    if(Is(At(entry,"status","level/off_grid"),"Initialized")){auto d=Descriptor(At(entry,"subject","level/off_grid"),At(entry,"state","level/off_grid"),At(entry,"placement","level/off_grid"),"level/off_grid",run,config_version);if(d.location.source_slot!=slot)Error("level/off_grid","slot_source_conflict");d.source_kind=2;d.source_index=position;add(std::move(d));}
    else if(!Is(entry["status"],"Uninitialized"))Error("level/off_grid","unknown_slot_status");
    ++position;
  }
  for(const auto &d:r.descriptors){const auto &l=d.location;
    if(!l.raw_grid_index_adopted)++out->budget.unadopted_grid_indices;
    // 普通存储和负槽用符号区分；同一ListIndex不能被另一维度重用为第二个对象。
    const auto unique=std::make_pair(l.source_slot?-1:0,l.source_slot?l.source_slot:l.storage_index);
    if(!storage.emplace(unique,d.key).second)Error("level/descriptors","duplicate_storage");
    for(int index:l.occupied)if(!occupied.emplace(std::make_pair(l.dimension,index),d.key).second)Error("level/descriptors","overlapping_occupancy");
  }
  const auto &rf=Fields(At(room,"state","room"),"room/state");r.environment.backdrop=unsigned(Field(rf,"Room.GetBackdropType","room",1,62));
  r.layout.shape=Field(rf,"Room.GetRoomShape","room",1,12);
  // J273几何Getter直接读取当前Data.Shape；不能接受与实际原生派生结构矛盾的展示形状。
  if(data.is_null||r.layout.shape!=data.shape)Error("room/layout","active_data_shape_required");
  r.layout.grid_width=Field(rf,"Room.GetGridWidth","room",1,28);r.layout.grid_height=Field(rf,"Room.GetGridHeight","room",1,16);
  r.layout.capacity=Field(rf,"Room.GetGridSize","room",1,448);
  if(r.layout.grid_width*r.layout.grid_height!=r.layout.capacity)Error("room/layout","geometry_capacity_disagrees");
  if(r.layout.grid_width!=RoomShapeGridWidth(r.layout.shape)||r.layout.grid_height!=RoomShapeGridHeight(r.layout.shape))Error("room/layout","shape_array_stride_disagrees");
  const auto &water=At(At(room,"presentation_runtime","room"),"water_amount","room/presentation_runtime");if(!water.IsNumber()||!std::isfinite(water.GetDouble())||water.GetDouble()<0||water.GetDouble()>1)Error("room/water_amount","finite_unit_range");r.environment.water=float(water.GetDouble());
  const auto &objects=Sequence(At(grid,"objects","grid"),"grid/objects"),&membership=At(grid,"membership","grid");
  const auto &lifted=Sequence(At(grid,"lifted_descriptors","grid"),"grid/lifted_descriptors");
  if(lifted.Size()>limits.grids)Error("grid/lifted_descriptors","budget");
  std::set<SubjectKey> lifted_owners;
  for(const auto &row:lifted.GetArray()){
    const auto path="grid/lifted_descriptors/"+std::to_string(out->lifted.size());LiftedGridTarget item;
    item.owner=Ref(At(row,"owner",path),path+"/owner",run,1).key;
    if(!lifted_owners.insert(item.owner).second)Error(path,"duplicate_entity_owner");
    const auto &source=At(row,"source_grid",path);
    if(!Is(At(source,"resolution",path),"Unresolved")||source.HasMember("target"))Error(path,"source_identity_must_remain_unresolved");
    const auto &value=At(row,"descriptor",path);item.descriptor_null=Null(value);
    if(!item.descriptor_null){const auto &f=Fields(value,path+"/descriptor");auto &d=item.descriptor;
      d.type=Field(f,"GridEntityDesc.Type",path,0,27);d.variant=Field(f,"GridEntityDesc.Variant",path,0);
      d.state=Field(f,"GridEntityDesc.State",path);d.var_data=Field(f,"GridEntityDesc.VarData",path);
      d.spawn_seed=U32(At(f,"GridEntityDesc.SpawnSeed",path),path);d.variable_seed=U32(At(f,"GridEntityDesc.VariableSeed",path),path);
      d.spawn_count=Field(f,"GridEntityDesc.SpawnCount",path);d.initialized=Boolean(At(f,"GridEntityDesc.Initialized",path),path);
    }
    out->lifted.push_back(item);
  }
  if(objects.Size()>limits.grids||!Boolean(At(membership,"complete","grid/membership"),"grid/membership")||Field(membership,"unavailable","grid/membership",0)!=0)Error("grid/membership","incomplete_or_budget");
  std::map<SubjectKey,unsigned> keys;std::set<std::pair<unsigned,unsigned>> aliases;std::map<int,SubjectKey> grid_slots;
  unsigned index=0,reference_count=0;
  for(const auto &v:objects.GetArray()){
    const auto path="grid/objects/"+std::to_string(index);GridTarget g;g.source_index=index++;const auto &s=At(v,"subject",path);g.key=Ref(s,path+"/subject",run,2).key;
    // 正式身份服务的首次shape revision为0；字段缺失仍失败，0不是未知/空实例。
    g.instance_revision=U64(At(s,"shape_revision",path),path+"/shape_revision");
    g.index=Field(v,"index",path,0,r.layout.capacity-1);g.type=Field(v,"type",path,1,27);g.variant=Field(v,"variant",path,0);
    const auto &f=Fields(At(v,"common",path),path+"/common");g.state=Field(f,"GridEntity.State",path);g.var_data=Field(f,"GridEntity.VarData",path);g.collision=Field(f,"GridEntity.CollisionClass",path,0,6);
    if(g.type!=Field(f,"GridEntity.GetType",path)||g.variant!=Field(f,"GridEntity.GetVariant",path)||g.index!=Field(f,"GridEntity.GetGridIndex",path))Error(path,"common_type_or_slot_conflict");
    const auto &df=Fields(At(f,"GridEntity.GetSaveState",path),path+"/descriptor");auto &d=g.descriptor;
    d.type=Field(df,"GridEntityDesc.Type",path,1,27);d.variant=Field(df,"GridEntityDesc.Variant",path,0);d.state=Field(df,"GridEntityDesc.State",path);d.var_data=Field(df,"GridEntityDesc.VarData",path);
    d.spawn_seed=U32(At(df,"GridEntityDesc.SpawnSeed",path),path);d.variable_seed=U32(At(df,"GridEntityDesc.VariableSeed",path),path);d.spawn_count=Field(df,"GridEntityDesc.SpawnCount",path);d.initialized=Boolean(At(df,"GridEntityDesc.Initialized",path),path);
    if(d.type!=g.type||d.variant!=g.variant)Error(path,"descriptor_type_conflict");
    const auto &members=At(v,"memberships",path);if(!members.IsArray()||members.Empty()||members.Size()>456)Error(path,"memberships_required");
    for(const auto &m:members.GetArray()){GridMembership alias{unsigned(Field(m,"source",path,11,12)),unsigned(Field(m,"index",path,0,447))};
      if(alias.source==12&&alias.index>7)Error(path,"door_slot_range");if(!aliases.emplace(alias.source,alias.index).second)Error(path,"duplicate_membership");
      if(++out->budget.aliases>kRoomGridAliases)Error(path,"alias_total_budget");g.memberships.push_back(alias);
      if(alias.source==11&&int(alias.index)!=g.index)Error(path,"grid_slot_disagrees");}
    std::sort(g.memberships.begin(),g.memberships.end(),[](const auto &a,const auto &b){return std::tie(a.source,a.index)<std::tie(b.source,b.index);});
    if(!keys.emplace(g.key,unsigned(out->grids.size())).second)Error(path,"duplicate_key");
    // Door-only 别名可与同 index 的主格成员共存；双占用只针对实际 +24 格数组。
    if(std::find(g.memberships.begin(),g.memberships.end(),GridMembership{11,unsigned(g.index)})!=g.memberships.end()&&!grid_slots.emplace(g.index,g.key).second)Error(path,"duplicate_grid_index");
    const auto refs=[&](auto &&self,const V &x)->void{
      if(x.IsArray())for(const auto &y:x.GetArray())self(self,y);
      if(!x.IsObject())return;
      if(x.HasMember("type")&&Is(x["type"],"reference")){
        if(++reference_count>limits.references)Error(path,"reference_budget");if(Null(x))return;
        const auto &key=At(x,"target",path);const auto space=unsigned(Field(key,"space",path,1,14));g.references.push_back(Ref(x,path+"/reference",run,space).key);return;
      }
      for(auto i=x.MemberBegin();i!=x.MemberEnd();++i)self(self,i->value);
    };refs(refs,v);out->grids.push_back(std::move(g));
  }
  const auto &doors=At(room,"doors","room");if(!doors.IsArray()||doors.Size()!=8)Error("room/doors","eight_slots_required");unsigned seen=0;
  for(const auto &v:doors.GetArray()){const unsigned slot=unsigned(Field(v,"slot","room/doors",0,7));if(seen&(1u<<slot))Error("room/doors","duplicate_slot");seen|=1u<<slot;auto &d=r.doors[slot];d.slot=slot;
    d.shape_allowed=Boolean(At(v,"shape_allowed","room/doors"),"room/doors");d.descriptor_enabled=Boolean(At(v,"descriptor_enabled","room/doors"),"room/doors");d.shape_target=Field(v,"shape_target_grid_index","room/doors");d.actual=Ref(At(v,"actual","room/doors"),"room/doors/actual",run,2,true);
    if(d.shape_target!=current.door_targets[slot]||d.descriptor_enabled!=bool(current.allowed_doors&(1u<<slot)))Error("room/doors","descriptor_binding_conflict");
    if(!d.actual.is_null){auto g=keys.find(d.actual.key);if(g==keys.end()||out->grids[g->second].type!=16)Error("room/doors","door_subject_missing");
      const auto &ms=out->grids[g->second].memberships;if(std::find(ms.begin(),ms.end(),GridMembership{12,slot})==ms.end())Error("room/doors","door_membership_missing");
      d.actual_target=Field(v,"actual_target","room/doors");const auto target=d.actual_target;
      const auto &body=Fields(At(At(objects[out->grids[g->second].source_index],"bodies","grid/door"),"GridEntityDoor","grid/door"),"grid/door/body");
      if(Field(body,"GridEntityDoor.Slot","grid/door",0,7)!=int(slot)||Field(body,"GridEntityDoor.TargetRoomIndex","grid/door")!=target)Error("room/doors","body_binding_conflict");
      if(target==-19)Error("room/doors","original_online_excluded");
      if(target==-100)d.route=DoorRoute::Mirror;else if(target==-101)d.route=DoorRoute::Mineshaft;
      else if(target>=0&&target<169)d.route=DoorRoute::Map;else if(target>=-20&&target<0)d.route=DoorRoute::OffGrid;else Error("room/doors","unknown_actual_route");
      // 镜门/矿车门的实际路由不依赖descriptor普通邻接启用位，不能据此抹掉真实门。
      if(!d.shape_allowed&&d.route!=DoorRoute::Mirror&&d.route!=DoorRoute::Mineshaft)Error("room/doors","actual_door_in_disallowed_slot");
      const auto &connection=At(v,"actual_connection","room/doors");if(Field(connection,"index","room/doors")!=target)Error("room/doors","connection_index_conflict");
      if(d.route==DoorRoute::Map||d.route==DoorRoute::OffGrid){
        if(!Is(At(connection,"kind","room/doors"),d.route==DoorRoute::Map?"MapRoom":"OffGridSlot"))Error("room/doors","connection_route_conflict");
        const auto &ref=At(connection,"target","room/doors");
        if(ref.IsObject()&&ref.HasMember("resolution")&&Is(ref["resolution"],"NotApplicable")&&ref.HasMember("reason")&&Is(ref["reason"],"uninitialized_descriptor"))d.target_uninitialized=true;
        else{d.target_descriptor=Ref(ref,"room/doors/actual_connection/target",run,6);const auto found=descriptors.find(d.target_descriptor.key);
          if(found==descriptors.end())Error("room/doors","target_descriptor_missing");const auto &p=r.descriptors[found->second].location;
          if(d.route==DoorRoute::OffGrid?p.source_slot!=target:(p.source_slot||p.dimension!=r.dimension||std::find(p.occupied.begin(),p.occupied.end(),target)==p.occupied.end()))Error("room/doors","target_descriptor_location_conflict");}
      }else if(!Is(At(connection,"kind","room/doors"),"DimensionRoute")||!Is(At(connection,"route","room/doors"),d.route==DoorRoute::Mirror?"Mirror":"Mineshaft"))Error("room/doors","dimension_route_conflict");
    }
  }
  for(const auto &g:out->grids){for(const auto &m:g.memberships)if(m.source==12&&r.doors[m.index].actual.key!=g.key)Error("grid/memberships","orphan_door_alias");
    for(const auto &ref:g.references)if(ref.space==2&&!keys.count(ref))Error("grid/reference","required_grid_missing");}
  // 三处成员视图必须指向同一份Grid主体；空集合与成员缺失分别处理，删除只来自完整集合。
  const auto membership_rows=[&](const V &rows,const std::string &path,bool with_index){
    const auto &items=Sequence(rows,path);if(items.Size()!=out->grids.size())Error(path,"membership_size_conflict");
    std::set<SubjectKey> seen;
    for(const auto &v:items.GetArray()){const auto key=Ref(At(v,"subject",path),path+"/subject",run,2).key;const auto found=keys.find(key);
      if(found==keys.end()||!seen.insert(key).second)Error(path,"unknown_or_duplicate_subject");
      const auto &g=out->grids[found->second];const auto &ms=Sequence(At(v,"memberships",path),path+"/memberships");
      if(ms.Size()!=g.memberships.size()||(with_index&&Field(v,"index",path)!=g.index))Error(path,"membership_conflict");
      std::set<std::pair<unsigned,unsigned>> actual;for(const auto &m:ms.GetArray())actual.emplace(unsigned(Field(m,"source",path,11,12)),unsigned(Field(m,"index",path,0,447)));
      for(const auto &m:g.memberships)if(!actual.count({m.source,m.index}))Error(path,"membership_alias_conflict");
    }
  };
  membership_rows(At(membership,"entries","grid/membership"),"grid/membership/entries",true);
  membership_rows(At(room,"grids","room"),"room/grids",false);
  Resources(room,"room");Resources(grid,"grid");
  out->source=Copy(model);return out;
}
Json RoomGridCapabilities(){return Parse(R"({"id":"room-grid-target-v10","version":10,"source_extension":8,"room_source_accepted":[4,5,6,7,8],"native_backdrop_source_minimum":8,"legacy_minimap_pose":"not-captured-before-source-7","descriptor_raw_grid_index":"dimension1 1x1 mismatch retained as diagnostic;native origin from real occupied cell;not raw equivalence","room_runtime_versions":[2,3],"presentation_versions":[2,3,4],"legacy_generic_fx":"requires-explicit-blend-mode;missing-source-rejected","grid_source_extension":5,"grid_source_accepted":[3,4,5],"native_grid_source_minimum":5,"resource_aliases":["grid-sprite-alias-v1"],"target":"REPENTOGON-1.1.3/J273","grid_types":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27],"descriptor_capacity":527,"excluded_off_grid":[-19],"room_shapes":[1,2,3,4,5,6,7,8,9,10,11,12],"room_adapters":["distinct-nullable-configs","shape-geometry","progress","backdrop-config-pose","water-color-fx","rails-lroom","camera-native-minimap"],"grid_adapters":["stable-key-instance-owner","factory-final-state-no-init","desc-common-aliases","17-derived-interfaces","fire-three-anm-embers","rock-rubble","door-alias-routes","instance-effective-water-clip","stable-references-and-lifted-descriptor-interface","source-collision-path-neighbor-pose","native-layered-grid-draw","source-grid-light-queries","scoped-independent-history","prefix-retirement","scene-atomic-commit"],"room_dependencies":["tx-04-19-special-entity-pass","tx-04-20-player-roster","tx-04-27-hud"],"environment_limits":{"layers":256,"poses_per_layer":256,"ray_groups":256,"rays_per_group":64,"vein_nodes":4096,"vein_depth":32,"vein_draw_visits":65536,"poop_poses":256},"limits":{"descriptors":526,"grids":456,"steps":8192,"references":4096,"dependency_edges":1048576,"target_bytes":33554432,"target_nodes":67108864,"target_depth":64,"resources":4096,"resource_layers":65536,"fx_members":32768,"grid_aliases":456,"native_grid_owners":456,"pending_targets":1,"retained_owners":1056,"borrow_tickets":16,"retired_scopes":128,"grid_commands_per_step":1,"grid_checks_per_step":1,"grid_retirements_per_step":1},"native_apply":true,"full_world_commit":false,"runtime_acceptance":"tx-04-20-I39:49-fresh-Source/49-cold-Client/12-checks+5-full-Player-environments;named-visual-reviewed;not-all-content","independent_effects":"handoff-to-tx-04-27;no-presentation-watermark-ack","native_responsibility":"I07-I25 Room/Grid scene;Entity19/Player20/HUD27 retain their own capabilities"})");}
const Json &RoomGridApplicationCatalog(){static const auto catalog=Parse(
#include "room_grid_application.inc"
);return catalog;}
} // namespace cs::replica
