#include "game/client_scene.h"
#include "game/client_objects.h"
#include "game/client_grids.h"
#include "game/client_presentation.h"
#include "game/client_resource_model.h"
#include "replica/room_geometry.h"
#include "game/context.h"
#include "game/bridge.h"
#include "runtime.h"
#include <cstdio>
#include <cstring>
#include <charconv>
#include <cmath>
#include <limits>
#include <algorithm>
#include <type_traits>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
constexpr unsigned kMaximumInFlight=16;
// 每个连接都重新构造 ClientScene，故进程级 epoch 防止旧票据命中新连接同序号。
std::atomic<std::uint64_t> next_scene_epoch{0};
std::uint64_t NewSceneEpoch() noexcept {
  auto value=next_scene_epoch.load(std::memory_order_relaxed);
  while(value!=UINT64_MAX){
    if(next_scene_epoch.compare_exchange_weak(value,value+1,std::memory_order_acq_rel))return value+1;
  }
  return 0;
}
const replica::Value *Member(const replica::Value *v,const char *name) noexcept {
  return v&&v->IsObject()&&v->HasMember(name)?&(*v)[name]:nullptr;
}
const replica::Value *Fields(const replica::Value *v) noexcept {return Member(v,"fields");}
const replica::Value *Scalar(const replica::Value *v) noexcept {
  if(v&&v->IsObject()&&v->HasMember("type")&&(*v)["type"].IsString()&&
     std::strcmp((*v)["type"].GetString(),"int64")==0)return Member(v,"value");
  return v;
}
bool Integer(const replica::Value *v,std::int64_t &out) noexcept {
  v=Scalar(v);if(!v)return false;
  if(v->IsInt64()){out=v->GetInt64();return true;}
  if(v->IsUint64()&&v->GetUint64()<=std::uint64_t(INT64_MAX)){out=std::int64_t(v->GetUint64());return true;}
  if(v->IsString()){
    const auto result=std::from_chars(v->GetString(),v->GetString()+v->GetStringLength(),out);
    return result.ec==std::errc{}&&result.ptr==v->GetString()+v->GetStringLength();
  }
  return false;
}
bool Int(const replica::Value *v,int &out) noexcept {
  std::int64_t number=0;if(!Integer(v,number)||number<INT32_MIN||number>INT32_MAX)return false;
  out=int(number);return true;
}
bool UInt(const replica::Value *v,std::uint32_t &out) noexcept {
  std::int64_t number=0;if(!Integer(v,number)||number<0||number>UINT32_MAX)return false;
  out=std::uint32_t(number);return true;
}
bool Float(const replica::Value *v,float &out) noexcept {
  v=Scalar(v);if(!v)return false;
  double number=0;
  if(v->IsNumber())number=v->GetDouble();
  else {std::int64_t integer=0;if(!Integer(v,integer))return false;number=double(integer);}
  if(!std::isfinite(number)||number<-std::numeric_limits<float>::max()||number>std::numeric_limits<float>::max())return false;
  out=float(number);return true;
}
// 正式来源的Room缓存与公开Getter交叉约束只做写前解释，不调用游戏验证或补造默认值。
bool RoomState(const replica::Value *room,const replica::RoomGridTarget &target,ClientRoomState &out,const char *&reason){
  reason="native_room_runtime_fields";const auto *v=Member(room,"application_runtime");int version=0;
  if(!Int(Member(v,"version"),version)||(version!=2&&version!=3))return false;
  const auto integer=[&](const char *name,int &value){return Int(Member(v,name),value);};
  // 旧v2保存SDK原始word；只解释已核定的低字节。v3来源直接提供真实byte，绝不补零。
  int render_flag=0;
  if(!integer(version==2?"slowdown":"render_flag_7220",render_flag)||(version==3&&(render_flag<0||render_flag>255)))return false;
  out.render_flag_7220=std::uint8_t(render_flag);
  const auto boolean=[&](const char *name,bool &value){const auto *x=Member(v,name);if(!x||!x->IsBool())return false;value=x->GetBool();return true;};
  if(!boolean("initialized",out.initialized)||!boolean("red_heart_damage",out.red_heart_damage)||!boolean("pacifist",out.pacifist)||
     !boolean("ambush_active",out.ambush_active)||!integer("frame",out.frame)||!integer("game_frame",out.game_frame)||!integer("clear_delay",out.clear_delay)||
     !integer("greed_timer",out.greed_timer)||!integer("greed_wave",out.greed_wave)||!integer("ambush_wave",out.ambush_wave)||
     !integer("ambush_max",out.ambush_max)||!integer("ambush_bosses",out.ambush_bosses)||!integer("total_money",out.total_money)||
     !integer("shop_item",out.shop_item)||!integer("shop_discount",out.shop_discount)||!integer("greed_subtype",out.greed_subtype)||
     !integer("poop_count",out.poop_count)||!integer("keeper_coins",out.keeper_coins)||!integer("pits",out.pits)||!UInt(Member(v,"boss"),out.boss))return false;
  const auto small=[](int n){return n>=INT16_MIN&&n<=INT16_MAX;};
  if(!out.initialized||out.frame<0||out.greed_wave<0||!small(out.total_money)||!small(out.shop_item)||!small(out.shop_discount)||!small(out.poop_count)||!small(out.keeper_coins))return false;
  const auto numbers=[&](const char *name,auto &array){const auto *a=Member(v,name);if(!a||!a->IsArray()||a->Size()!=array.size())return false;
    for(unsigned i=0;i<array.size();++i){if constexpr(std::is_same_v<typename std::decay_t<decltype(array)>::value_type,float>){if(!Float(&(*a)[i],array[i]))return false;}
      else if(!Int(&(*a)[i],array[i]))return false;}return true;};
  if(!numbers("bounds",out.bounds)||!numbers("l_area",out.l_area)||!numbers("l_tiles",out.l_tiles)||!numbers("door_positions",out.door_positions))return false;
  if(out.bounds[0]>=out.bounds[2]||out.bounds[1]>=out.bounds[3])return false;
  if(out.l_tiles!=replica::RoomShapeTiles(target.room.layout.shape))return false;
  const auto *paths=Member(v,"grid_paths"),*path_scope=Member(v,"path_scope");
  if(!paths||!paths->IsArray()||paths->Size()!=unsigned(target.room.layout.capacity)||!path_scope||!path_scope->IsString()||
     std::strcmp(path_scope->GetString(),"active_extent;outside_not_applicable")){reason="native_grid_path_source_missing";return false;}
  out.grid_paths.resize(paths->Size());for(unsigned i=0;i<paths->Size();++i)if(!Int(&(*paths)[i],out.grid_paths[i]))return false;
  if(!numbers("grid_light_queries",out.grid_light_queries)||out.grid_light_queries[0]<0||out.grid_light_queries[0]>1||
     out.grid_light_queries[1]<0||out.grid_light_queries[1]>255||out.grid_light_queries[2]<0||out.grid_light_queries[2]>1||
     out.grid_light_queries[3]<0||out.grid_light_queries[3]>1||out.grid_light_queries[4]<0||out.grid_light_queries[4]>4)return false;
  // 原生缓存包含全部八槽位置，1x1的停用右/下半槽仍保留大房模板坐标。
  // 仅当前房形采用的槽要求位于活动extent；其余保留Source值但不作为门/格子解引用。
  for(unsigned i=0;i<out.door_positions.size();++i){const int position=out.door_positions[i];
    if(position<-1||position>=448||(target.room.doors[i].shape_allowed&&position>=0&&
       !replica::RoomShapeGridCell(target.room.layout.shape,position,true))){reason="native_room_door_position";return false;}}
  for(const auto &pair:{std::pair{"valid",&out.valid},std::pair{"interior",&out.interior}}){const auto *a=Member(v,pair.first);
    if(!a||!a->IsArray()||a->Size()!=448)return false;for(unsigned i=0;i<448;++i){if(!(*a)[i].IsBool())return false;(*pair.second)[i]=(*a)[i].GetBool();}}
  const auto *rails=Member(v,"rails");if(!rails||!rails->IsArray()||rails->Size()!=448)return false;
  const auto *rail_scope=Member(v,"rail_scope");if(!rail_scope||!rail_scope->IsString()||std::strcmp(rail_scope->GetString(),"active_extent;outside_not_applicable")!=0)return false;
  for(unsigned i=0;i<448;++i){int rail=0;if(!Int(&(*rails)[i],rail)||!replica::RoomRailTypeValid(rail))return false;out.rails[i]=std::uint8_t(rail);out.rails_present|=rail!=255;
    if(i>=unsigned(target.room.layout.capacity)&&(out.valid[i]||out.interior[i]||rail!=255))return false;
    if(out.interior[i]&&!out.valid[i])return false;
    if(out.valid[i]!=replica::RoomShapeGridCell(target.room.layout.shape,int(i),true)||out.interior[i]!=replica::RoomShapeGridCell(target.room.layout.shape,int(i),false))return false;
  }
  const auto *fields=Fields(Member(room,"state"));const auto *descriptor=Fields(&target.DescriptorState(target.room.descriptors.front()));
  const auto agrees=[&](const char *name,bool value){const auto *x=Member(descriptor,name);return x&&x->IsBool()&&x->GetBool()==value;};
  const auto flags=unsigned(target.room.descriptors.front().flags);
  for(const auto &flag:{std::pair{"RoomDescriptor.Clear",1u},std::pair{"RoomDescriptor.PressurePlatesTriggered",2u},
      std::pair{"RoomDescriptor.SacrificeDone",4u},std::pair{"RoomDescriptor.ChallengeDone",8u},std::pair{"RoomDescriptor.NoReward",128u}})
    if(!agrees(flag.first,(flags&flag.second)!=0)){reason="native_room_descriptor_flag_conflict";return false;}
  const auto *clear=Member(fields,"Room.IsClear");int frame=0;
  if(!clear||!clear->IsBool()||clear->GetBool()!=bool(flags&1)||!Int(Member(fields,"Room.GetFrameCount"),frame)||frame!=out.frame){reason="native_room_progress_source_conflict";return false;}
  if(out.rails_present){const auto *sprite=Member(room,"rail_sprite");std::string error;
    if(!sprite||!ClientParseResourcePose(*sprite,out.rail_sprite,error)||(out.rail_sprite.animation!="rail"&&out.rail_sprite.animation!="pitrail")){reason="native_room_rail_resource";return false;}}
  return true;
}
// 地图投影与完整探索/离房恢复分开；每项都来自正式采集，不能使用本地菜单保存记录。
bool MapState(const replica::Value *room,const replica::RoomGridTarget &target,ClientMapTarget &out,const char *&reason){
  reason="native_map_camera_source";const auto *state=Member(room,"map_application");int version=0;
  if(!Int(Member(state,"version"),version)||version!=1)return false;
  const auto vector=[](const replica::Value *v,ClientPoseVector &value){return Float(Member(v,"x"),value.x)&&Float(Member(v,"y"),value.y);};
  const auto boolean=[](const replica::Value *v,bool &value){if(!v||!v->IsBool())return false;value=v->GetBool();return true;};
  const auto *camera=Member(state,"camera"),*vectors=Member(camera,"vectors");auto &c=out.state.camera;
  if(!vectors||!vectors->IsArray()||vectors->Size()!=5)return false;
  for(unsigned i=0;i<5;++i)if(!vector(&(*vectors)[i],c.vectors[i]))return false;
  if(!boolean(Member(camera,"overridden"),c.overridden)||!boolean(Member(camera,"focus_present"),c.focus_present)||!vector(Member(camera,"focus_tail"),c.focus_tail)||!Float(Member(camera,"smoothing"),c.smoothing))return false;
  const auto *configs=Member(state,"configs"),*flags=Member(state,"game_flags");
  if(!configs||!configs->IsArray()||configs->Size()!=2||!flags||!flags->IsArray()||flags->Size()!=2)return false;
  for(unsigned i=0;i<2;++i){auto &d=out.state.configs[i];const auto *v=&(*configs)[i];
    if(!vector(Member(v,"anchor"),d.anchor)||!vector(Member(v,"padding"),d.padding)||!vector(Member(v,"size1"),d.size1)||!vector(Member(v,"size2"),d.size2)||
       !Float(Member(v,"offset"),d.offset)||!Float(Member(v,"border_scale"),d.border_scale)||!Float(Member(v,"alpha"),d.alpha)||
       !Int(Member(v,"width"),d.width)||!Int(Member(v,"height"),d.height)||!Int(Member(v,"icons"),d.icons)||d.width<1||d.width>13||d.height<1||d.height>13||d.icons<0||d.icons>64||d.alpha<0||d.alpha>1||!UInt(&(*flags)[i],out.state.game_flags[i]))return false;}
  unsigned total=0;
  for(const auto &descriptor:target.room.descriptors){const auto *v=Member(&target.DescriptorState(descriptor),"map_icons");const auto *items=Member(v,"icons");
    if(!Int(Member(v,"version"),version)||version!=1||!items)return false;
    const bool empty=items->IsObject()&&!items->MemberCount();if(!empty&&!items->IsArray())return false;
    ClientMapDescriptor d;d.storage=descriptor.location.storage_index;d.source_slot=descriptor.location.source_slot;
    if(!empty){if(items->Size()>4096||(total+=items->Size())>65536)return false;for(const auto &item:items->GetArray()){
      ClientMapSavedIcon icon;std::uint32_t bits=0;
      if(!Int(Member(&item,"type"),icon.type)||icon.type<0||!Int(Member(&item,"variant"),icon.variant)||!Int(Member(&item,"subtype"),icon.subtype)||
         !UInt(Member(&item,"storage3"),bits)||!vector(Member(&item,"position"),icon.position))return false;
      std::memcpy(&icon.storage3,&bits,4);d.icons.push_back(icon);}}
    out.descriptors.push_back(std::move(d));
  }
  return true;
}
// 只读取15已经表达的具名值；缺字段不以菜单、seed或本地模拟补齐。
bool SceneDescription(const replica::Value &model,const replica::RoomGridTarget &target,ClientNativeSceneDescription &out,const char *&reason) {
  const auto *components=Member(&model,"components");
  const auto *room=Member(Member(components,"room"),"state");
  const auto *level=Member(Member(components,"level"),"state");
  const auto *run=Member(Member(components,"run"),"state");
  const auto *room_fields=Fields(Member(room,"state"));
  const auto *level_fields=Fields(Member(level,"state"));
  const auto *run_fields=Fields(Member(run,"state"));
  const auto *config_fields=Fields(Member(Member(room,"config"),"state"));
  const auto *descriptor_fields=Fields(Member(level,"current_descriptor"));
  const auto *backdrop_runtime=Member(Member(room,"presentation_runtime"),"backdrop_runtime");
  const auto *presentation_runtime=Member(room,"presentation_runtime");
  out={};reason="native_scene_fields_missing";
  if(!Int(Member(room,"room_index"),out.room_index)||!Int(Member(room,"dimension_index"),out.dimension)||
     !Int(Member(level_fields,"Level.GetStage"),out.stage)||!Int(Member(level_fields,"Level.GetStageType"),out.stage_type)||
     !Int(Member(level_fields,"Level.EnterDoor"),out.enter_door)||
     !Int(Member(level_fields,"Level.GetStartingRoomIndex"),out.starting_index)||
     !Int(Member(run_fields,"Game.Difficulty"),out.difficulty)||!Int(Member(run_fields,"Game.Challenge"),out.challenge)||
     !Int(Member(config_fields,"RoomConfigRoom.StageID"),out.config_stage)||
     !Int(Member(config_fields,"RoomConfigRoom.Mode"),out.config_mode)||
     !Int(Member(config_fields,"RoomConfigRoom.Type"),out.room_type)||
     !UInt(Member(config_fields,"RoomConfigRoom.Variant"),out.variant)||
     !Int(Member(config_fields,"RoomConfigRoom.Subtype"),out.subtype)||
     !Int(Member(config_fields,"RoomConfigRoom.Shape"),out.shape)||
     !Int(Member(config_fields,"RoomConfigRoom.Width"),out.width)||
     !Int(Member(config_fields,"RoomConfigRoom.Height"),out.height)||
     !Int(Member(descriptor_fields,"RoomDescriptor.ListIndex"),out.list_index)||
     !Int(Member(descriptor_fields,"RoomDescriptor.SafeGridIndex"),out.safe_grid_index)||
     !Int(Member(descriptor_fields,"RoomDescriptor.AllowedDoors"),out.allowed_doors)||
     !Int(Member(descriptor_fields,"RoomDescriptor.Flags"),out.descriptor_flags)||
     !Int(Member(descriptor_fields,"RoomDescriptor.VisitedCount"),out.visited_count)||
     !Int(Member(descriptor_fields,"RoomDescriptor.ClearCount"),out.clear_count)||
     !UInt(Member(room_fields,"Room.GetBackdropType"),out.backdrop)||
     !UInt(Member(backdrop_runtime,"construction_seed"),out.backdrop_seed)||
     !UInt(Member(backdrop_runtime,"shift1"),out.backdrop_shift1)||
     !UInt(Member(backdrop_runtime,"shift2"),out.backdrop_shift2)||
     !UInt(Member(backdrop_runtime,"shift3"),out.backdrop_shift3)||
     !Float(Member(presentation_runtime,"water_amount"),out.water_amount))return false;
  const auto *center=Member(room_fields,"Room.GetCenterPos");
  if(!Float(Member(center,"x"),out.camera_x)||!Float(Member(center,"y"),out.camera_y))return false;
  const auto *doors=Member(descriptor_fields,"RoomDescriptor.Doors");
  if(!doors||!doors->IsArray()||doors->Size()!=8){reason="native_descriptor_doors_missing";return false;}
  unsigned seen=0;
  for(const auto &door:doors->GetArray()){
    int slot=0,door_target=0;
    if(!Int(Member(&door,"slot"),slot)||slot<0||slot>7||(seen&(1u<<slot))||
       !Int(Member(&door,"target_grid_index"),door_target)){reason="native_descriptor_door_invalid";return false;}
    seen|=1u<<slot;out.door_targets[slot]=door_target;
  }
  out.spawn_seed=target.room.descriptors.front().spawn_seed;
  out.decoration_seed=target.room.descriptors.front().decoration_seed;
  out.award_seed=target.room.descriptors.front().award_seed;
  out.grid_index=target.room.descriptors.front().location.native_grid_origin;
  for(const auto &d:target.room.descriptors){
    // 两份完整资源描述独立交给SDK owner；不能用查找键相同抹掉Override差异。
    // 地图占用由Data描述；Override是替换配置，不能用它的房形推翻实际已发布占用。
    const auto &c=d.data.is_null?d.override_data:d.data;
    ClientNativeDescriptorDescription v;const auto &p=d.location;
    v.key_run=d.key.run;v.key_id=d.key.id;v.construction_revision=d.construction_revision;
    v.grid=p.native_grid_origin;v.safe=p.safe_grid_index;v.list=p.list_index;v.dimension=p.dimension;v.storage=p.storage_index;v.source_slot=p.source_slot;
    v.occupied_count=unsigned(p.occupied.size());std::copy(p.occupied.begin(),p.occupied.end(),v.occupied.begin());
    v.config_stage=c.stage;v.config_mode=c.mode;v.type=c.type;v.variant=c.variant;v.subtype=c.subtype;v.shape=c.shape;v.width=c.width;v.height=c.height;
    v.data_null=d.data.is_null;v.override_null=d.override_data.is_null;v.allowed=d.allowed_doors;v.flags=d.flags;v.visited=d.visited;v.clear_count=d.clear_count;
    v.display_flags=d.display_flags;
    v.data=d.data;v.override_data=d.override_data;
    v.doors=d.door_targets;v.spawn=d.spawn_seed;v.decoration=d.decoration_seed;v.award=d.award_seed;
    out.descriptors.push_back(v);
  }
  const auto &active=target.room.descriptors.front();const auto &effective=active.override_data.is_null?active.data:active.override_data;
  out.config_stage=effective.stage;out.config_mode=effective.mode;out.variant=effective.variant;out.subtype=effective.subtype;
  out.width=effective.width;out.height=effective.height;out.shape=target.room.layout.shape;
  out.grid_width=target.room.layout.grid_width;out.grid_height=target.room.layout.grid_height;
  if(!Int(Member(room_fields,"Room.GetType"),out.room_type))return false;
  if(!RoomState(room,target,out.room_state,reason))return false;
  if(!MapState(room,target,out.map,reason))return false;
  const auto *background=Member(presentation_runtime,"backdrop_config"),*paths=Member(background,"paths"),*values=Member(background,"values");
  if(!paths||!paths->IsArray()||paths->Size()!=12||!values||!values->IsArray()||values->Size()!=7){reason="native_backdrop_config_missing";return false;}
  for(unsigned i=0;i<12;++i){const auto &p=(*paths)[i];if(!p.IsString()||p.GetStringLength()>1024)return false;out.backdrop_config.paths[i].assign(p.GetString(),p.GetStringLength());
    const auto &s=out.backdrop_config.paths[i];if(s.find("..")!=s.npos||s.find(':')!=s.npos||(!s.empty()&&(s.front()=='/'||s.front()=='\\'))||s.find_first_of("\r\n\0",0,3)!=s.npos)return false;}
  for(unsigned i=0;i<7;++i)if(!UInt(&(*values)[i],out.backdrop_config.values[i]))return false;
  if(out.backdrop_config.values[0]!=out.backdrop){reason="native_backdrop_config_id";return false;}
  // 语义构建轨迹必须完整；不从Client本地重抽，也不将旧来源缺失解释为空轨迹。
  const auto *wall_trace=Member(room,"backdrop_wall_trace");unsigned trace_version=0;
  const auto *applicable=Member(wall_trace,"applicable");
  if(!UInt(Member(wall_trace,"version"),trace_version)||trace_version!=1||!applicable||!applicable->IsBool()){
    reason="native_backdrop_wall_trace_required";return false;
  }
  auto &trace=out.backdrop_wall_trace;trace.present=true;trace.applicable=applicable->GetBool();
  if(!UInt(Member(wall_trace,"initial"),trace.initial)||(trace.applicable&&!trace.initial)){
    reason="native_backdrop_initial_rng_required";return false;
  }
  if(trace.applicable){
    const auto *steps=Member(wall_trace,"steps");
    if(!UInt(Member(wall_trace,"before"),trace.before)||!trace.before||!UInt(Member(wall_trace,"after"),trace.after)||!trace.after||
       !steps||!steps->IsArray()||steps->Size()>4096){reason="native_backdrop_wall_trace_fields";return false;}
    for(const auto &step:steps->GetArray()){
      ClientBackdropEffectStep value;const auto *args=Member(&step,"arguments"),*quad=Member(&step,"quad"),*x=Member(&step,"flip_x"),*y=Member(&step,"flip_y");
      if(!args||!args->IsArray()||args->Size()!=4||!quad||!quad->IsArray()||quad->Size()!=8||!x||!x->IsBool()||!y||!y->IsBool()||
         !UInt(Member(&step,"before"),value.before)||!value.before||!UInt(Member(&step,"after"),value.after)||!value.after){reason="native_backdrop_wall_trace_step";return false;}
      for(unsigned i=0;i<4;++i)if(!UInt(&(*args)[i],value.arguments[i]))return false;
      for(unsigned i=0;i<8;++i)if(!Float(&(*quad)[i],value.quad[i])||std::fabs(value.quad[i])>1000000)return false;
      value.flip_x=x->GetBool();value.flip_y=y->GetBool();trace.steps.push_back(value);
    }
  }
  const auto *surface=Member(presentation_runtime,"backdrop_surface_inputs");if(!surface||!surface->IsArray()||surface->Size()!=4)return false;
  for(unsigned i=0;i<4;++i)if(!Float(&(*surface)[i],out.backdrop_surface_inputs[i]))return false;
  if(out.backdrop_surface_inputs[0]>=out.backdrop_surface_inputs[2]||out.backdrop_surface_inputs[1]>=out.backdrop_surface_inputs[3])return false;
  const auto *sprites=Member(room,"backdrop_sprites");if(!sprites||!sprites->IsArray()||sprites->Size()!=4){reason="native_backdrop_sprites_missing";return false;}
  for(unsigned i=0;i<4;++i){const auto &s=(*sprites)[i];if(s.IsObject()&&s.HasMember("status")&&s["status"].IsString()&&std::strcmp(s["status"].GetString(),"Null")==0)continue;
    std::string error;if(!ClientParseResourcePose(s,out.backdrop_sprites[i],error)){reason="native_backdrop_sprite_resource";return false;}out.backdrop_sprite_present[i]=true;}
  reason="native_scene_description_ready";return true;
}
}
ClientScene::ClientScene() noexcept=default;
ClientScene::~ClientScene() {ClientNativeWorldDestroy(native_world_);}
// 新连接只建立等待身份；从不把等待 Scope 的 run/room 伪装成原生场景。
void ClientScene::Begin(const replica::Scope &scope) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(!CanReturnToMenu()){FailureResult(ClientResultKind::PartialNativeFailure,"begin","old_owner_inflight");state_=ClientSceneState::Failed;return;}
  waiting_scope_=scope;target_scope_={};applied_scope_={};target_.reset();applied_.reset();
  room_grid_target_.reset();room_grid_applied_.reset();room_grid_plan_={};
  ClientNativeWorldDestroy(native_world_);native_world_=nullptr;objects_.reset();presentation_.reset();grids_.reset();
  grid_events_.Reset(scope);grids_planned_=grids_complete_=grids_retired_=references_complete_=false;
  applied_valid_=rebuild_pending_=objects_planned_=objects_retired_=pumping_=false;
  attempt_invalidated_=false;
  objects_complete_=presentation_retired_=false;
  retirement_complete_=false;
  retired_scopes_={};retired_scope_count_=0;
  native_dispatch_prefix_=false;
  target_revision_=applied_revision_=0;
  const auto epoch=NewSceneEpoch();
  if(!epoch){FailureResult(ClientResultKind::BudgetExhausted,"begin","scene_generation_exhausted");state_=ClientSceneState::Failed;return;}
  generations_.scene=epoch;
  state_=ClientSceneState::Waiting;result_={};result_.scope=scope;
  FailureResult(ClientResultKind::WaitingDependency,"waiting","world_payload_unavailable");
  current_dc_=current_context_=observed_dc_=observed_context_=nullptr;
}
void ClientScene::FailureResult(ClientResultKind kind,const char *phase,const char *reason) noexcept {
  if(kind==ClientResultKind::PartialNativeFailure||kind==ClientResultKind::ContextOrThreadFault||kind==ClientResultKind::UnsupportedRequirement){
    if(!result_.failure_phase[0]){
      std::snprintf(result_.failure_phase.data(),result_.failure_phase.size(),"%s",phase?phase:"unknown");
      std::snprintf(result_.failure_reason.data(),result_.failure_reason.size(),"%s",reason?reason:"unknown");
      if(!result_.failed_domain[0])std::snprintf(result_.failed_domain.data(),result_.failed_domain.size(),"%s",
        phase&&std::strstr(phase,"grid")?"grid":phase&&std::strstr(phase,"objects")?"entity":phase&&std::strstr(phase,"presentation")?"presentation":"room-grid");
    }
  }
  if(kind==ClientResultKind::PartialNativeFailure||kind==ClientResultKind::ContextOrThreadFault||kind==ClientResultKind::UnsupportedRequirement)
    Trace("client_apply_failure","revision=%llu attempt=%llu kind=%u phase=%s reason=%s prefix=%u written=%u",
      result_.revision,result_.attempt,unsigned(kind),phase?phase:"unknown",reason?reason:"unknown",result_.executed_prefix,unsigned(result_.native_written));
  result_.kind=kind;
  std::snprintf(result_.phase.data(),result_.phase.size(),"%s",phase?phase:"unknown");
  std::snprintf(result_.reason.data(),result_.reason.size(),"%s",reason?reason:"unknown");
  result_.generations=generations_;
  result_.native_committed=false;
  result_.committed_domains=0;
  std::snprintf(result_.recovery.data(),result_.recovery.size(),"%s",kind==ClientResultKind::NativeCommitted?"none":result_.native_written?"retire_actual_owners_then_rebuild":"preserve_applied_or_replace_unwritten_target");
}
bool ClientScene::NoteNativeDispatchEntry() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(result_.executed_prefix==UINT32_MAX){
    FailureResult(ClientResultKind::BudgetExhausted,"dispatch","native_prefix_budget");
    state_=ClientSceneState::Failed;return false;
  }
  native_dispatch_prefix_=true;
  ++result_.executed_prefix;
  return true;
}
// 接纳只建立自有T和计划；较新T取消已写前缀时先退休原生世界，再从完整T重建。
bool ClientScene::AdmitTarget(const replica::Scope &scope,std::uint64_t revision,
                              const replica::Value &value) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if((state_==ClientSceneState::Retiring&&!rebuild_pending_)||pumping_||
     active_native_||active_lua_||active_graphics_||active_observation_||active_dispatch_){
    FailureResult(ClientResultKind::Retiring,"admit","owner_unavailable");return false;
  }
  if(!scope.session||!scope.connection||!scope.run||!scope.scene||!revision||
     scope.session!=waiting_scope_.session||scope.connection!=waiting_scope_.connection) {
    FailureResult(ClientResultKind::WaitingDependency,"admit","formal_scope_required");return false;
  }
  try {
    replica::CheckValue(value);
    replica::ValidateModel(value);
    if(value["publishable"]!=true||replica::ReadScope(value["scope"])!=scope||
       replica::Unsigned(value["revision"])!=revision) {
      FailureResult(ClientResultKind::UnsupportedRequirement,"admit","target_identity_or_coverage");return false;
    }
    for(const auto &borrow:borrows_)if(borrow.id){
      FailureResult(ClientResultKind::Retiring,"admit","borrow_inflight");return false;
    }
    for(unsigned i=0;i<retired_scope_count_;++i)if(scope==retired_scopes_[i]){
      FailureResult(ClientResultKind::WaitingDependency,"admit","retired_scope");return false;
    }
    if(applied_valid_&&applied_&&scope==applied_scope_&&revision==applied_revision_&&*applied_==value){
      FailureResult(ClientResultKind::DuplicateTarget,"committed","duplicate_native_committed");
      result_.scope=scope;result_.revision=revision;result_.native_committed=true;result_.old_display_valid=true;
      std::snprintf(result_.recovery.data(),result_.recovery.size(),"none");
      result_.committed_domains=(1u<<unsigned(world::observation::DomainId::Grid))|(1u<<unsigned(world::observation::DomainId::Room));return true;
    }
    if(target_&&scope==target_scope_&&revision==target_revision_) {
      FailureResult(*target_==value?ClientResultKind::DuplicateTarget:ClientResultKind::UnsupportedRequirement,
                    "admit",*target_==value?"duplicate_target":"same_version_conflict");return *target_==value;
    }
    if(target_&&scope==target_scope_&&revision<target_revision_) {
      FailureResult(ClientResultKind::WaitingDependency,"admit","stale_revision");return false;
    }
    if(applied_&&scope==applied_scope_&&revision<=applied_revision_) {
      FailureResult(ClientResultKind::WaitingDependency,"admit","already_applied_revision");return false;
    }
    auto domain=replica::ParseRoomGridTarget(value);
    auto domain_plan=replica::PlanRoomGridTarget(domain,applied_valid_?room_grid_applied_:nullptr);
    ClientNativeSceneDescription description{};const char *reason=nullptr;
    if(!SceneDescription(value,*domain,description,reason)){
      FailureResult(ClientResultKind::UnsupportedRequirement,"admit",reason);return false;
    }
    if(next_attempt_==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"admit","attempt_exhausted");return false;}
    auto copy=std::make_unique<replica::Json>(replica::Copy(value));
    if(!objects_)objects_=std::make_unique<ClientObjects>();
    if(!presentation_)presentation_=std::make_unique<ClientPresentation>();
    if(!grids_)grids_=std::make_unique<ClientGrids>();
    const bool reset_events=!target_||scope!=target_scope_;
    const bool changing_scope=target_&&scope!=target_scope_;
    if(changing_scope&&retired_scope_count_==retired_scopes_.size()){
      FailureResult(ClientResultKind::BudgetExhausted,"admit","retired_scope_budget");return false;
    }
    const bool need_rebuild=state_==ClientSceneState::Failed||rebuild_pending_||
        (ClientNativeWorldHasPrefix(native_world_)&&
         (domain_plan.room==replica::RoomPlanKind::Rebuild||(state_!=ClientSceneState::Displaying&&
           !(state_==ClientSceneState::Preparing&&replica::CancelRoomGridPlan(result_.native_written,result_.executed_prefix)==replica::PlanCancellation::PreserveApplied))||
          changing_scope||!ClientNativeWorldCanReuse(native_world_,description)));
    // 重建也先完成全部纯值预检；不能先退休 A，才发现 T 依赖尚未支持的 Entity19。
    {ClientObjects objects;ClientGrids grids;ClientPresentation presentation;
      if(!objects.PrepareTarget(value,nullptr,reason,domain.get())||!grids.PrepareTarget(replica::PlanRoomGridTarget(domain),reason)||
         !presentation.PrepareTarget(value,reason,description.room_state,description.map)){
        FailureResult(ClientResultKind::UnsupportedRequirement,"preflight",reason);return false;}}
    // 写世界前先完成所有纯值计划；失败保留上一已提交展示资格。
    if(!need_rebuild&&!objects_->PrepareTarget(value,applied_valid_?applied_.get():nullptr,reason,domain.get())){
      FailureResult(ClientResultKind::UnsupportedRequirement,"plan",reason);return false;
    }
    if(!need_rebuild&&!grids_->PrepareTarget(domain_plan,reason)){
      objects_->CancelPlanned();objects_planned_=false;
      grids_planned_=false;
      FailureResult(ClientResultKind::UnsupportedRequirement,"grid_plan",reason);return false;
    }
    if(!need_rebuild&&!presentation_->PrepareTarget(value,reason,description.room_state,description.map)){
      // 纯值展示预检失败时撤销尚未执行的对象计划，保留当前原生A及其身份表。
      if(!objects_->CancelPlanned())Fail("unexecuted_plan_cancel_failed");
      if(!grids_->CancelPlanned())Fail("unexecuted_grid_plan_cancel_failed");
      grids_planned_=false;
      // 新T预检失败，下一轮重新规划仍保留的旧T；不留下target_与已撤计划不一致的状态。
      objects_planned_=false;
      FailureResult(ClientResultKind::UnsupportedRequirement,"presentation_plan",reason);return false;
    }
    if(changing_scope)retired_scopes_[retired_scope_count_++]=target_scope_;
    // 空菜单快照虽无原生写入，也绑定了旧T的descriptor地址；替换时撤掉该未执行owner。
    if(!need_rebuild&&native_world_&&!ClientNativeWorldHasPrefix(native_world_)){
      ClientNativeWorldDestroy(native_world_);native_world_=nullptr;
    }
    target_=std::move(copy);target_scope_=scope;target_revision_=revision;
    room_grid_target_=std::move(domain);room_grid_plan_=std::move(domain_plan);
    native_description_=std::move(description);objects_planned_=!need_rebuild;objects_retired_=false;attempt_invalidated_=false;
    objects_complete_=presentation_retired_=false;
    grids_planned_=!need_rebuild;grids_complete_=grids_retired_=references_complete_=false;
    if(reset_events)grid_events_.Reset(scope);
    retirement_complete_=false;
    const auto pending=result_.pending_owners;
    result_={};result_.scope=scope;result_.revision=revision;result_.attempt=++next_attempt_;
    result_.pending_owners=pending;result_.old_display_valid=applied_valid_;
    if(need_rebuild){
      rebuild_pending_=true;applied_valid_=false;state_=ClientSceneState::Retiring;
      for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
      FailureResult(ClientResultKind::Retiring,"rebuild","target_requires_native_rebuild");
    }else{
      state_=ClientSceneState::Preparing;
      FailureResult(ClientResultKind::WaitingDependency,"prepare","target_planned");
    }
    return true;
  } catch(const replica::ContractError &e) {FailureResult(ClientResultKind::UnsupportedRequirement,"admit",e.what());return false;}
    catch(...) {FailureResult(ClientResultKind::UnsupportedRequirement,"admit","invalid_or_unavailable_target");return false;}
}
void ClientScene::NoteNativeStep(const ClientNativeStep &step,const char *phase) noexcept {
  result_.executed_prefix=(std::max)(result_.executed_prefix,step.prefix);
  if(step.written&&!step.safe_menu){result_.native_written=true;result_.old_display_valid=false;applied_valid_=false;}
  if(step.state==ClientNativeStepState::Failed){
    FailureResult(ClientResultKind::PartialNativeFailure,phase,step.reason);
    state_=ClientSceneState::Failed;rebuild_pending_=false;
  }else if(step.state==ClientNativeStepState::Waiting)
    FailureResult(ClientResultKind::WaitingDependency,phase,step.reason);
  else FailureResult(ClientResultKind::WaitingDependency,phase,step.reason);
}
// 同一主线程泵只做一个完整子步骤；借用/原生队列未退尽时返回下一轮，不等待网络或磁盘。
void ClientScene::Pump() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(pumping_||TopOwner()!=Owner::Manager||!ContextHealthy()||InBlockedContext())return;
  pumping_=true;
  ContextWindow();DrainRetained();
  const bool retiring=state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed;
  if(!DestructiveReady()){pumping_=false;return;}
  if(retiring){
    // 已停止展示与新借用后先撤本方映射，再让原生Room/Player/HUD单一owner真实退休。
    ClientBorrowToken borrow{};
    if(!Borrow(ClientPurpose::NativeRetire,borrow)){pumping_=false;return;}
    if(presentation_&&!presentation_retired_){
      const auto step=presentation_->Retire(ClientNativeWorldRoots(native_world_));
      if(step.failed){FailureResult(ClientResultKind::PartialNativeFailure,"presentation_retire",step.reason);state_=ClientSceneState::Failed;}
      else presentation_retired_=step.done;
      Release(borrow);pumping_=false;return;
    }
    if(objects_&&!objects_retired_){
      const auto step=objects_->Retire(ClientNativeWorldRoots(native_world_));
      result_.executed_prefix=(std::max)(result_.executed_prefix,step.executed_prefix);
      if(step.failed){FailureResult(ClientResultKind::PartialNativeFailure,"objects_retire",step.reason);state_=ClientSceneState::Failed;}
      else objects_retired_=step.done;
      Release(borrow);pumping_=false;return;
    }
    // 所有新draw已关闭、CPU队列退尽；先由Grid owner撤别名/副缓冲并Free，再让Room析构。
    if(grids_&&!grids_retired_){
      const auto step=grids_->Retire(ClientNativeWorldRoots(native_world_));
      result_.grid_prefix=step.prefix;result_.grid_owners=step.owners;
      if(step.failed){FailureResult(ClientResultKind::PartialNativeFailure,"grid_retire",step.reason);state_=ClientSceneState::Failed;}
      else grids_retired_=step.done;
      Release(borrow);pumping_=false;return;
    }
    if(native_world_){
      const auto step=ClientNativeWorldRetire(native_world_);NoteNativeStep(step,"native_retire");
      if(step.safe_menu){result_.native_written=false;result_.native_owners=0;}
    }
    Release(borrow);FinishRetirementIfDrained();pumping_=false;return;
  }
  if((state_!=ClientSceneState::Preparing&&state_!=ClientSceneState::Applying)||!target_){pumping_=false;return;}
  if(!objects_planned_){
    const char *reason=nullptr;
    if(!objects_){try{objects_=std::make_unique<ClientObjects>();}catch(...){Fail("objects_allocation");pumping_=false;return;}}
    if(!objects_->PrepareTarget(*target_,applied_valid_?applied_.get():nullptr,reason,room_grid_target_.get())){
      FailureResult(ClientResultKind::UnsupportedRequirement,"plan",reason);Fail(reason);pumping_=false;return;
    }
    if(!presentation_){try{presentation_=std::make_unique<ClientPresentation>();}catch(...){Fail("presentation_allocation");pumping_=false;return;}}
    if(!presentation_->PrepareTarget(*target_,reason,native_description_.room_state,native_description_.map)){Fail(reason);pumping_=false;return;}
    objects_planned_=true;
  }
  if(!grids_planned_){const char *reason=nullptr;
    if(!grids_){try{grids_=std::make_unique<ClientGrids>();}catch(...){Fail("grid_owner_allocation");pumping_=false;return;}}
    if(!grids_->PrepareTarget(room_grid_plan_,reason)){Fail(reason);pumping_=false;return;}grids_planned_=true;
  }
  if(!GraphicsWindowValid()){
    FailureResult(ClientResultKind::WaitingDependency,"prepare","native_graphics_context");pumping_=false;return;
  }
  if(!native_world_){
    native_world_=ClientNativeWorldCreate();
    if(!native_world_){Fail("native_world_allocation");pumping_=false;return;}
  }
  if(!attempt_invalidated_){InvalidateInstance();attempt_invalidated_=true;}
  if(state_==ClientSceneState::Failed){pumping_=false;return;}
  ClientBorrowToken native{},graphics{};
  if(!Borrow(ClientPurpose::NativeApply,native)){pumping_=false;return;}
  if(!Borrow(ClientPurpose::Graphics,graphics)){Release(native);pumping_=false;return;}
  if(state_==ClientSceneState::Preparing){
    const auto step=ClientNativeWorldPrepare(native_world_,native_description_);
    NoteNativeStep(step,"native_prepare");
    if(step.state==ClientNativeStepState::Ready)state_=ClientSceneState::Applying;
  }else{
    const auto roots=ClientNativeWorldRoots(native_world_);
    // 每轮最多一个对象命令；候选资源在释放用途借用后由下一轮DrainRetained收口。
    const auto step=objects_complete_?ClientObjectStep{true,false,false,0,result_.native_owners,"objects_complete"}:objects_->Step(roots,1);
    result_.executed_prefix=(std::max)(result_.executed_prefix,step.executed_prefix);
    result_.native_owners=step.pending_owners;
    if(Settings.diagnostics&&step.key.id)Trace("client_object_step","key=%llu:%u:%llu instance=%llu prefix=%u done=%u failed=%u reason=%s",
      step.key.run,step.key.space,step.key.id,step.instance,step.object_prefix,unsigned(step.done),unsigned(step.failed),step.reason);
    if(step.written){result_.native_written=true;result_.old_display_valid=false;applied_valid_=false;}
    if(step.failed){
      FailureResult(ClientResultKind::PartialNativeFailure,"objects_apply",step.reason);state_=ClientSceneState::Failed;
    }else if(step.done){
      objects_complete_=true;
      // 几何/真实Entity先就绪，再分步安装Grid；所有依赖/资源/缓存均在唯一Scene提交前闭合。
      if(!grids_complete_){const auto grid=grids_->Step(roots,1);
        result_.grid_prefix=grid.prefix;result_.grid_owners=grid.owners;
        if(grid.written){result_.native_written=true;result_.old_display_valid=false;applied_valid_=false;}
        if(grid.failed){std::snprintf(result_.failed_domain.data(),result_.failed_domain.size(),"grid");
          result_.failed_key={grid.key.run,grid.key.id,grid.key.space};
          FailureResult(ClientResultKind::PartialNativeFailure,"grid_apply",grid.reason);state_=ClientSceneState::Failed;}
        else grids_complete_=grid.done;
        Release(graphics);Release(native);pumping_=false;return;
      }
      if(!references_complete_){const auto linked=grids_->ResolveReferences(*objects_);
        if(linked.failed){FailureResult(ClientResultKind::PartialNativeFailure,"grid_references",linked.reason);state_=ClientSceneState::Failed;}
        else references_complete_=linked.done;
        Release(graphics);Release(native);pumping_=false;return;
      }
      // Backdrop.Init的教程surface读取首Player，必须等正式对象计划已完成roster/EL，
      // 并在I4缓存准备/绘制前具名收尾；空PM不得靠旧vector存储继续解引用。
      const auto finalized=ClientNativeWorldFinalize(native_world_);
      NoteNativeStep(finalized,"native_finalize");
      if(finalized.state==ClientNativeStepState::Ready){
        const auto present=presentation_->Step(roots);
        if(present.written){result_.native_written=true;result_.old_display_valid=false;applied_valid_=false;}
        if(present.failed){
          FailureResult(ClientResultKind::PartialNativeFailure,"presentation_prepare",present.reason);state_=ClientSceneState::Failed;
        }else if(!present.done)FailureResult(ClientResultKind::WaitingDependency,"presentation_prepare",present.reason);
        else {
          const auto grid_check=grids_->Check(roots);
          if(grid_check.failed){result_.failed_key={grid_check.key.run,grid_check.key.id,grid_check.key.space};
            FailureResult(ClientResultKind::PartialNativeFailure,"grid_commit",grid_check.reason);state_=ClientSceneState::Failed;
            Release(graphics);Release(native);pumping_=false;return;}
          if(!grid_check.done){FailureResult(ClientResultKind::WaitingDependency,"grid_commit",grid_check.reason);
            Release(graphics);Release(native);pumping_=false;return;}
          const auto check=ClientNativeWorldCheckCommit(native_world_);NoteNativeStep(check,"native_commit");
          if(check.state==ClientNativeStepState::Ready){
            try{
              auto committed=std::make_unique<replica::Json>(replica::Copy(*target_));
              applied_=std::move(committed);applied_scope_=target_scope_;applied_revision_=target_revision_;
              room_grid_applied_=room_grid_target_;
              room_grid_plan_.applied.reset();
              applied_valid_=true;state_=ClientSceneState::Displaying;
              FailureResult(ClientResultKind::NativeCommitted,"committed","native_target_committed");
              result_.native_committed=true;result_.old_display_valid=true;
              result_.committed_domains=(1u<<unsigned(world::observation::DomainId::Grid))|(1u<<unsigned(world::observation::DomainId::Room));
            }catch(...){Fail("native_commit_model_allocation");}
          }
        }
      }
    }else FailureResult(ClientResultKind::WaitingDependency,"objects_apply",step.reason);
  }
  Release(graphics);Release(native);pumping_=false;
}
bool ClientScene::DisplayReady() const noexcept {
  return IsMain()&&state_==ClientSceneState::Displaying&&applied_valid_&&applied_&&
         !foreign_fault_.load(std::memory_order_acquire)&&!ClientNativeWorldSafeMenu(native_world_);
}
ClientNativeRoots ClientScene::NativeRoots() const noexcept {
  return DisplayReady()?ClientNativeWorldRoots(native_world_):ClientNativeRoots{};
}
const replica::Value *ClientScene::Published() const noexcept {return DisplayReady()?applied_.get():nullptr;}
// 独立记录与普通目标走不同入口；没有已知主线程/场景时不接纳，也不写原生世界。
bool ClientScene::AdmitGridIndependent(const replica::Message &message,const char *&reason) noexcept {
  if(!IsMain()||TopOwner()!=Owner::Manager||!target_||state_==ClientSceneState::Failed||state_==ClientSceneState::Retiring){reason="grid_independent_scene_unavailable";return false;}
  return grid_events_.Admit(message,reason);
}
bool ClientScene::VisitGridIndependent(void *context,ClientGridEvents::Visitor visitor) noexcept {
  if(!DisplayReady()||TopOwner()!=Owner::Render)return false;
  ClientBorrowToken borrow{};if(!Borrow(ClientPurpose::RenderObservation,borrow))return false;
  const bool result=grid_events_.Visit(applied_revision_,context,visitor);Release(borrow);return result;
}
bool ClientScene::CompleteGridIndependent(std::uint64_t sequence,std::uint64_t scope,const char *phase) noexcept {
  return DisplayReady()&&grid_events_.Complete(sequence,scope,phase);
}
// 展示只读取真实提交后的同一原生世界；Render失败立即撤资格，并把退休留给Manager有效owner。
void ClientScene::RenderPublished() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(!DisplayReady()||!presentation_||TopOwner()!=Owner::Render)return;
  ClientBorrowToken graphics{};
  if(!Borrow(ClientPurpose::Graphics,graphics))return;
  const auto step=presentation_->Render(ClientNativeWorldRoots(native_world_));
  if(step.failed)Fail(step.reason);
  Release(graphics);
}
// 每个原生、Lua、图形用途都要求已知主线程和 owner；图形另要求当前 OS context。
bool ClientScene::Enter(ClientPurpose purpose) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(draining_)return false;
  if(foreign_fault_.exchange(false,std::memory_order_acq_rel)){
    FailureResult(ClientResultKind::ContextOrThreadFault,"enter","foreign_use_attempt");state_=ClientSceneState::Failed;return false;
  }
  if(!ContextHealthy()||InBlockedContext()) {
    FailureResult(ClientResultKind::ContextOrThreadFault,"enter","owner_or_thread");return false;
  }
  if(purpose==ClientPurpose::RenderObservation){
    if(TopOwner()!=Owner::Render||
       active_observation_==kMaximumInFlight)return false;
    ++active_observation_;return true;
  }
  // 分派票据只维持用途帧寿命，不授权任何原生写入或图形操作。
  if(purpose==ClientPurpose::Dispatch){
    if((TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)||
       active_dispatch_==kMaximumInFlight)return false;
    ++active_dispatch_;return true;
  }
  if(purpose==ClientPurpose::LocalSettings)return TopOwner()!=Owner::None;
  if(purpose==ClientPurpose::NativeRetire) {
    if(state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed)return false;
    if(TopOwner()!=Owner::Manager||active_native_)return false;
    ++active_native_;return true;
  }
  if(state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  if(purpose==ClientPurpose::NativeApply){
    if(state_==ClientSceneState::Waiting||!target_){FailureResult(ClientResultKind::UnsupportedRequirement,"apply","target_unavailable");return false;}
    if(TopOwner()!=Owner::Manager||active_native_||
       (state_!=ClientSceneState::Preparing&&state_!=ClientSceneState::Applying)){
      FailureResult(ClientResultKind::ContextOrThreadFault,"apply","native_phase");return false;
    }
    ++active_native_;return true;
  }
  if(purpose==ClientPurpose::Lua) {
    if(lua_closing_||!lua_state_||(TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)||
       active_lua_==kMaximumInFlight){FailureResult(ClientResultKind::ContextOrThreadFault,"lua","state_phase_or_budget");return false;}
    ++active_lua_;return true;
  }
  if(purpose==ClientPurpose::Graphics) {
    if(!GraphicsWindowValid()||
       active_graphics_==kMaximumInFlight) {
      FailureResult(ClientResultKind::ContextOrThreadFault,"graphics","os_context_or_phase");return false;
    }
    ++active_graphics_;return true;
  }
  FailureResult(ClientResultKind::UnsupportedRequirement,"enter","purpose_unavailable");return false;
}
void ClientScene::Leave(ClientPurpose purpose) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  unsigned *count=purpose==ClientPurpose::Lua?&active_lua_:
                  purpose==ClientPurpose::Graphics?&active_graphics_:
                  purpose==ClientPurpose::RenderObservation?&active_observation_:
                  purpose==ClientPurpose::Dispatch?&active_dispatch_:&active_native_;
  if(purpose==ClientPurpose::LocalSettings)return;
  if(!*count){FailureResult(ClientResultKind::ContextOrThreadFault,"leave","unbalanced_use");state_=ClientSceneState::Failed;return;}
  --*count;
  DrainRetained();
}
ClientScene::RetainedEntry *ClientScene::FindRetained(const ClientRetainedTicket &ticket) noexcept {
  if(!ticket.id)return nullptr;
  for(auto &entry:retained_)if(entry.ticket.id==ticket.id&&
      entry.ticket.scene==ticket.scene&&entry.ticket.instance==ticket.instance)return &entry;
  return nullptr;
}
// 逐次比较 OS-current 与真正 renderer window；缓存的句柄只用于识别代次，不充当许可。
bool ClientScene::GraphicsWindowValid() const noexcept {
  if(!IsMain()||context_closing_||
     (TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)||
     !current_dc_||!current_context_)return false;
  void *dc=nullptr,*context=nullptr;
  return ClientRendererHandles(dc,context)&&dc==current_dc_&&context==current_context_&&
      wglGetCurrentDC()==dc&&wglGetCurrentContext()==context;
}
// 资源 admission 与 release 分离：旧修订不再可用，但其真正 owner 仍保留到有效阶段退休。
bool ClientScene::ResourceUseValid(const RetainedEntry &entry) const noexcept {
  if(entry.generations.scene!=generations_.scene||
     entry.generations.instance!=generations_.instance||
     entry.generations.resource!=generations_.resource)return false;
  if(entry.kind==ClientRetainedKind::LuaRegistry)
    return !lua_closing_&&lua_state_&&entry.generations.lua==generations_.lua;
  if(entry.kind==ClientRetainedKind::Graphics||entry.phase==ClientReleasePhase::ManagerContext||
     entry.phase==ClientReleasePhase::RenderContext)
    return entry.generations.context==generations_.context&&GraphicsWindowValid();
  return true;
}
bool ClientScene::DestructiveReady() const noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(foreign_fault_.load(std::memory_order_acquire)||draining_||lua_closing_||context_closing_||
     !ContextHealthy()||InBlockedContext()||TopOwner()!=Owner::Manager||
     active_native_||active_lua_||active_graphics_||active_observation_||active_dispatch_)return false;
  for(const auto &borrow:borrows_)if(borrow.id)return false;
  for(const auto &owner:retained_)if(owner.ticket.id&&
      (owner.cpu_queued||owner.gpu_queued||owner.releasing||owner.failed))return false;
  // 未登记于本方表的原生 raw 图像队列仍必须为空；这不是 Render 返回或 GPU idle 推断。
  return ClientNativeCpuQueuesEmpty();
}
// 调用方在已核主线程阶段登记其合法持有的资源；同一指针只接受一个释放 owner。
bool ClientScene::Retain(ClientRetainedKind kind,ClientReleasePhase phase,void *owned,ClientRetainedRelease release,
                         ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(ticket.id)return false;
  ticket={};
  if(!owned||!release||!generations_.scene||draining_||!ContextHealthy()||InBlockedContext()||
     state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  // 这里只登记调用方已合法拥有的对象寿命，不执行构造或应用，也不授予 NativeApply。
  if(kind!=ClientRetainedKind::Native&&kind!=ClientRetainedKind::LuaRegistry&&
     kind!=ClientRetainedKind::Graphics)return false;
  if(kind==ClientRetainedKind::Native&&(TopOwner()!=Owner::Manager||
     (phase!=ClientReleasePhase::Manager&&phase!=ClientReleasePhase::ManagerContext)))return false;
  if((kind==ClientRetainedKind::LuaRegistry&&phase!=ClientReleasePhase::LuaState)||
     (kind==ClientRetainedKind::Graphics&&phase!=ClientReleasePhase::RenderContext&&
      phase!=ClientReleasePhase::ManagerContext))return false;
  if(kind==ClientRetainedKind::LuaRegistry&&(lua_closing_||!lua_state_||
      (TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)))return false;
  if(kind==ClientRetainedKind::Graphics&&!GraphicsWindowValid())return false;
  if(phase==ClientReleasePhase::LuaState&&!lua_state_)return false;
  if((phase==ClientReleasePhase::RenderContext||phase==ClientReleasePhase::ManagerContext)&&
     (!GraphicsWindowValid()||
      TopOwner()!=(phase==ClientReleasePhase::RenderContext?Owner::Render:Owner::Manager)))return false;
  for(const auto &entry:retained_)if(entry.ticket.id&&entry.owned==owned){
    FailureResult(ClientResultKind::UnsupportedRequirement,"retain","duplicate_native_owner");return false;
  }
  RetainedEntry *free=nullptr;
  for(auto &entry:retained_)if(!entry.ticket.id){free=&entry;break;}
  if(!free||next_retained_==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"retain","native_owner_budget");return false;}
  ticket={++next_retained_,generations_.scene,generations_.instance};
  *free={ticket,kind,phase,owned,release,generations_,0,0,false,false,false};
  ++result_.pending_owners;return true;
}
bool ClientScene::QueueCpuUse(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(draining_||state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  auto *owner=FindRetained(ticket);
  if(!owner||owner->retiring||owner->releasing||owner->failed||!ResourceUseValid(*owner)||
     !ContextHealthy()||InBlockedContext()||owner->cpu_queued==kMaximumInFlight)return false;
  if(owner->kind==ClientRetainedKind::Native&&TopOwner()!=Owner::Manager)return false;
  if(owner->kind==ClientRetainedKind::LuaRegistry&&
     (!lua_state_||(TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)))return false;
  if(owner->kind==ClientRetainedKind::Graphics&&!GraphicsWindowValid())return false;
  ++owner->cpu_queued;return true;
}
bool ClientScene::QueueGpuUse(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(draining_||state_==ClientSceneState::Retiring||state_==ClientSceneState::Failed)return false;
  auto *owner=FindRetained(ticket);
  if(!owner||owner->kind!=ClientRetainedKind::Graphics||owner->retiring||owner->releasing||owner->failed||
     !ResourceUseValid(*owner)||!ContextHealthy()||InBlockedContext()||!GraphicsWindowValid()||
     owner->gpu_queued==kMaximumInFlight)return false;
  ++owner->gpu_queued;return true;
}
void ClientScene::CpuUseComplete(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(ticket.scene!=generations_.scene)return;
  auto *owner=FindRetained(ticket);
  if(!owner||!owner->cpu_queued){Fail("cpu_use_unbalanced");return;}
  if(!ContextHealthy()||InBlockedContext()||
     (TopOwner()!=Owner::Manager&&TopOwner()!=Owner::Render)){
    Fail("cpu_complete_without_owner");return;
  }
  --owner->cpu_queued;DrainRetained();
}
void ClientScene::GpuUseComplete(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(ticket.scene!=generations_.scene)return;
  auto *owner=FindRetained(ticket);
  if(!owner||!owner->gpu_queued){Fail("gpu_use_unbalanced");return;}
  // 只有持原 context 的真实 fence 消费方能报告；禁止错误线程或旧 context 替它归零。
  if(owner->generations.context!=generations_.context||!GraphicsWindowValid()){
    Fail("gpu_complete_without_context");return;
  }
  --owner->gpu_queued;DrainRetained();
}
void ClientScene::RetireRetained(const ClientRetainedTicket &ticket) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(ticket.scene!=generations_.scene)return;
  auto *owner=FindRetained(ticket);
  if(!owner)return; // 已完成或重复取消，不能污染当前连接。
  owner->retiring=true;DrainRetained();
}
bool ClientScene::UseLuaRegistry(const ClientRetainedTicket &ticket,void *&owned) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  owned=nullptr;
  auto *owner=FindRetained(ticket);
  if(!owner||owner->kind!=ClientRetainedKind::LuaRegistry||owner->retiring||owner->releasing||owner->failed||
     !ResourceUseValid(*owner)||!active_lua_||draining_||
     !ContextHealthy()||InBlockedContext())return false;
  owned=owner->owned;return true;
}
// CPU 队列和 GPU 完成各由真实消费方显式回报；Render 返回、Lua unref 均不冒充完成。
void ClientScene::DrainRetained() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(state_==ClientSceneState::Failed)
    for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  if(draining_||!ContextHealthy()||InBlockedContext()||
     active_native_||active_lua_||active_graphics_||active_observation_||active_dispatch_)return;
  for(const auto &borrow:borrows_)if(borrow.id)return;
  draining_=true;
  for(auto &owner:retained_){
    if(!owner.ticket.id||!owner.retiring||owner.releasing||owner.failed||
       owner.cpu_queued||owner.gpu_queued)continue;
    if(!ContextHealthy()||InBlockedContext()||active_native_||active_lua_||
       active_graphics_||active_observation_||active_dispatch_)break;
    // 旧资源代次禁止新借用，但原 owner 的独占引用仍须在其 VM/context 有效时释放。
    bool phase_valid=false;
    switch(owner.phase){
      case ClientReleasePhase::Manager:phase_valid=TopOwner()==Owner::Manager;break;
      case ClientReleasePhase::LuaState:
        phase_valid=lua_state_&&owner.generations.lua==generations_.lua&&
              (TopOwner()==Owner::Manager||TopOwner()==Owner::Render||
               (lua_closing_&&TopOwner()==Owner::None));break;
      case ClientReleasePhase::RenderContext:
        phase_valid=current_dc_&&current_context_&&
              owner.generations.context==generations_.context&&
              (TopOwner()==Owner::Render||(context_closing_&&TopOwner()==Owner::None))&&
              wglGetCurrentDC()==current_dc_&&wglGetCurrentContext()==current_context_;break;
      case ClientReleasePhase::ManagerContext:
        phase_valid=current_dc_&&current_context_&&
              owner.generations.context==generations_.context&&TopOwner()==Owner::Manager&&
              wglGetCurrentDC()==current_dc_&&wglGetCurrentContext()==current_context_;break;
    }
    if(!phase_valid)continue;
    if(owner.kind==ClientRetainedKind::Graphics||owner.phase==ClientReleasePhase::RenderContext||
       owner.phase==ClientReleasePhase::ManagerContext){
      void *dc=nullptr,*context=nullptr;
      if(!ClientRendererHandles(dc,context)||dc!=current_dc_||context!=current_context_||
         !ClientNativeCpuQueuesEmpty())continue;
    }
    owner.releasing=true;
    if(!owner.release(owner.owned)){
      // false 表示释放可能已有原生执行前缀；禁止自动重试同一回调。
      owner.releasing=false;owner.failed=true;
      FailureResult(ClientResultKind::PartialNativeFailure,"drain","native_release_failed");
      state_=ClientSceneState::Failed;continue;
    }
    owner={};
    if(result_.pending_owners)--result_.pending_owners;
  }
  draining_=false;
  FinishRetirementIfDrained();
}
void ClientScene::FinishRetirementIfDrained() noexcept {
  if(retirement_complete_)return;
  if(state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed)return;
  if(active_native_||active_lua_||active_graphics_||active_observation_||active_dispatch_)return;
  for(const auto &entry:borrows_)if(entry.id)return;
  for(const auto &owner:retained_)if(owner.ticket.id)return;
  if((presentation_&&!presentation_retired_)||(objects_&&!objects_retired_)||(grids_&&!grids_retired_)||
     !ClientNativeWorldSafeMenu(native_world_))return;
  // 此处只有实际原生退休已经完成；清本方记录不能替代Room/Player/HUD析构。
  ClientNativeWorldDestroy(native_world_);native_world_=nullptr;
  native_dispatch_prefix_=false;result_.native_written=false;result_.native_owners=0;
  result_.executed_prefix=0;result_.native_committed=false;result_.old_display_valid=false;
  applied_valid_=false;applied_.reset();applied_revision_=0;applied_scope_={};
  room_grid_applied_.reset();room_grid_plan_.applied.reset();
  result_.grid_owners=result_.grid_prefix=result_.committed_domains=0;
  InvalidateInstance();result_.generations=generations_;
  if(rebuild_pending_&&target_){
    rebuild_pending_=false;objects_planned_=false;objects_retired_=false;
    objects_complete_=presentation_retired_=attempt_invalidated_=false;
    grids_planned_=grids_complete_=grids_retired_=references_complete_=false;
    state_=ClientSceneState::Preparing;
    try{room_grid_plan_=replica::PlanRoomGridTarget(room_grid_target_);}
    catch(...){Fail("room_grid_rebuild_plan_unavailable");return;}
    FailureResult(ClientResultKind::WaitingDependency,"rebuild","native_retired_rebuild_target");
  }else{
    target_.reset();target_revision_=0;target_scope_={};
    room_grid_target_.reset();room_grid_plan_={};
    retirement_complete_=true;
    FailureResult(ClientResultKind::Retiring,"retired","native_safe_menu");
  }
}
// 借用只能在同一主线程 owner 栈内使用；不同代次即使地址相同也失效。
bool ClientScene::Borrow(ClientPurpose purpose,ClientBorrowToken &out) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  if(out.active)return false;
  out={};
  BorrowEntry *free=nullptr;
  for(auto &entry:borrows_)if(!entry.id){free=&entry;break;}
  if(!free||next_borrow_==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"borrow","borrow_budget_or_id");return false;}
  if(!Enter(purpose))return false;
  const auto id=++next_borrow_;
  const auto owner_token=OwnerToken(TopOwner());
  if(!owner_token){Leave(purpose);return false;}
  *free={id,generations_.scene,owner_token,purpose};
  out={id,purpose,generations_,owner_token,true};
  return true;
}
bool ClientScene::Valid(const ClientBorrowToken &borrow) const noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return false;}
  bool registered=false;
  for(const auto &entry:borrows_)if(entry.id==borrow.id&&entry.purpose==borrow.purpose&&
      entry.scene==borrow.generations.scene&&entry.owner_token==borrow.owner_token){registered=true;break;}
  if(!borrow.active||!ContextHealthy()||InBlockedContext()||
     !registered||
     borrow.owner_token==0||borrow.owner_token!=OwnerToken(TopOwner())||
     borrow.generations.scene!=generations_.scene)return false;
  if(borrow.purpose==ClientPurpose::RenderObservation)return TopOwner()==Owner::Render;
  if(borrow.purpose==ClientPurpose::LocalSettings)return true;
  if((state_==ClientSceneState::Failed||state_==ClientSceneState::Retiring)&&
     borrow.purpose!=ClientPurpose::NativeRetire&&borrow.purpose!=ClientPurpose::Dispatch)return false;
  if(borrow.generations.instance!=generations_.instance||
     borrow.generations.resource!=generations_.resource)return false;
  if(borrow.purpose==ClientPurpose::Lua)
    return !lua_closing_&&lua_state_&&borrow.generations.lua==generations_.lua;
  if(borrow.purpose==ClientPurpose::Graphics)
    return borrow.generations.context==generations_.context&&GraphicsWindowValid();
  return true;
}
void ClientScene::Release(ClientBorrowToken &borrow) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(!borrow.active)return;
  BorrowEntry *registered=nullptr;
  for(auto &entry:borrows_)if(entry.id==borrow.id&&entry.purpose==borrow.purpose&&
      entry.scene==borrow.generations.scene&&entry.owner_token==borrow.owner_token){registered=&entry;break;}
  if(!registered){borrow.active=false;return;}
  // 取消/失败只是关闭 admission；退出原有栈不要求重新取得已经撤销的使用许可。
  // 正常运行下的错误栈或意外代次变化仍报告，但总会归还本票据一次，绝不等待自己。
  if(!Valid(borrow)&&state_!=ClientSceneState::Retiring&&state_!=ClientSceneState::Failed){
    FailureResult(ClientResultKind::ContextOrThreadFault,"borrow","borrow_invalidated");state_=ClientSceneState::Failed;
  }
  registered->id=0;
  borrow.active=false;Leave(borrow.purpose);
}
bool ClientScene::ConsumeForeignFault() noexcept {
  if(!IsMain())return false;
  return foreign_fault_.exchange(false,std::memory_order_acq_rel);
}
// renderer 身份与 OS-current 分开观察：临时切到 ImGui context 不制造原 context 的新代次。
void ClientScene::ContextWindow() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(TopOwner()!=Owner::Render&&TopOwner()!=Owner::Manager)return;
  if(context_closing_)return;
  void *dc=nullptr,*context=nullptr;
  const bool known=ClientRendererHandles(dc,context);
  if(!known)dc=context=nullptr;
  if(dc!=observed_dc_||context!=observed_context_) {
    if(active_graphics_){
      FailureResult(ClientResultKind::ContextOrThreadFault,"render","context_replaced_inflight");
      state_=ClientSceneState::Failed;
    }
    if(observed_context_)for(const auto &owner:retained_)if(owner.ticket.id&&
        (owner.phase==ClientReleasePhase::RenderContext||owner.phase==ClientReleasePhase::ManagerContext)){
      FailureResult(ClientResultKind::ContextOrThreadFault,"render","context_replaced_with_owner");
      state_=ClientSceneState::Failed;break;
    }
    observed_dc_=dc;observed_context_=context;
    if(generations_.context==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"render","context_generation_exhausted");state_=ClientSceneState::Failed;return;}
    ++generations_.context;
  }
  current_dc_=dc;current_context_=context;
  if(dc!=wglGetCurrentDC()||context!=wglGetCurrentContext())current_dc_=current_context_=nullptr;
  DrainRetained();
}
void ClientScene::RenderWindow() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(TopOwner()==Owner::Render)ContextWindow();
}
// Lua state 身份在主线程观察；替换/消失即换代，旧 registry 借用不能复活。
void ClientScene::ObserveLuaState(void *state) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(state==lua_state_)return;
  if(lua_state_)for(const auto &owner:retained_)if(owner.ticket.id&&owner.phase==ClientReleasePhase::LuaState){
    FailureResult(ClientResultKind::ContextOrThreadFault,"lua","state_replaced_with_owner");
    state_=ClientSceneState::Failed;break;
  }
  lua_state_=state;
  if(generations_.lua==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"lua","generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.lua;
  if(active_lua_){FailureResult(ClientResultKind::ContextOrThreadFault,"lua","state_replaced_inflight");state_=ClientSceneState::Failed;}
}
void ClientScene::LuaClosing() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(lua_closing_){Fail("lua_close_reentrant");return;}
  for(auto &owner:retained_)if(owner.ticket.id&&owner.phase==ClientReleasePhase::LuaState)owner.retiring=true;
  lua_closing_=true;
  DrainRetained();
  lua_closing_=false;
  if(generations_.lua==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"lua_close","lua_generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.lua;
  lua_state_=nullptr;
  for(const auto &owner:retained_)if(owner.ticket.id&&owner.phase==ClientReleasePhase::LuaState){FailureResult(ClientResultKind::PartialNativeFailure,"lua_close","lua_owner_inflight");state_=ClientSceneState::Failed;break;}
  if(active_lua_){FailureResult(ClientResultKind::ContextOrThreadFault,"lua_close","borrow_inflight");state_=ClientSceneState::Failed;}
}
void ClientScene::ContextClosing() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(context_closing_){Fail("context_close_reentrant");return;}
  for(auto &owner:retained_)if(owner.ticket.id&&
      (owner.phase==ClientReleasePhase::RenderContext||owner.phase==ClientReleasePhase::ManagerContext))owner.retiring=true;
  context_closing_=true;
  DrainRetained();
  context_closing_=false;
  current_dc_=current_context_=observed_dc_=observed_context_=nullptr;
  for(const auto &owner:retained_)if(owner.ticket.id&&
      (owner.phase==ClientReleasePhase::RenderContext||owner.phase==ClientReleasePhase::ManagerContext)){
    FailureResult(ClientResultKind::PartialNativeFailure,"context_close","context_owner_inflight");state_=ClientSceneState::Failed;break;}
  if(generations_.context==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"context_close","context_generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.context;
  if(active_graphics_){FailureResult(ClientResultKind::ContextOrThreadFault,"context_close","draw_inflight");state_=ClientSceneState::Failed;}
}
void ClientScene::InvalidateInstance() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(generations_.instance==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"instance","generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.instance;InvalidateResource();
}
void ClientScene::InvalidateResource() noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  if(generations_.resource==UINT64_MAX){FailureResult(ClientResultKind::BudgetExhausted,"resource","generation_exhausted");state_=ClientSceneState::Failed;return;}
  ++generations_.resource;
  // 旧语义关联即刻关闭 admission；实际 owner 仍由 Drain 在 CPU/VM/context 条件满足后释放。
  for(auto &owner:retained_)if(owner.ticket.id&&owner.generations.resource!=generations_.resource)
    owner.retiring=true;
}
void ClientScene::Retire(const char *reason) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  rebuild_pending_=false;applied_valid_=false;result_.old_display_valid=false;
  state_=ClientSceneState::Retiring;FailureResult(ClientResultKind::Retiring,"retire",reason);
  for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  DrainRetained();
}
void ClientScene::Fail(const char *reason) noexcept {
  if(!IsMain()){foreign_fault_.store(true,std::memory_order_release);return;}
  rebuild_pending_=false;applied_valid_=false;result_.old_display_valid=false;
  state_=ClientSceneState::Failed;FailureResult(ClientResultKind::PartialNativeFailure,"failed",reason);
  for(auto &owner:retained_)if(owner.ticket.id)owner.retiring=true;
  DrainRetained();
}
bool ClientScene::CanReturnToMenu() const noexcept {
  if(!IsMain()||foreign_fault_.load(std::memory_order_acquire))return false;
  for(const auto &entry:borrows_)if(entry.id)return false;
  for(const auto &owner:retained_)if(owner.ticket.id)return false;
  return !HasNativePrefix()&&
         ClientNativeWorldSafeMenu(native_world_)&&(!objects_||objects_retired_)&&
         (!presentation_||presentation_retired_)&&(!grids_||grids_retired_)&&!target_&&
         !active_native_&&!active_lua_&&!active_graphics_&&!active_observation_&&!active_dispatch_&&!applied_;
}
} // namespace cs::game
