#include "game/client_grids.h"
#include "game/client_objects.h"
#include "game/client_resource_model.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>

namespace cs::game {
namespace {
using V=replica::Value;
constexpr const char *casts[]={"Decoration","Door","Fire","Gravity","Lock","Pit","Poop","PressurePlate","Rock","Spikes","Stairs","Statue","Teleporter","TNT","TrapDoor","Wall","Web"};
const V &At(const V &v,const char *name){if(!v.IsObject()||!v.HasMember(name))throw std::runtime_error(std::string("grid_required:")+name);return v[name];}
bool Is(const V &v,const char *name){return v.IsString()&&std::string_view(v.GetString(),v.GetStringLength())==name;}
int Integer(const V &input){const auto &v=input.IsObject()&&input.HasMember("type")&&Is(input["type"],"int64")?At(input,"value"):input;
  std::int64_t n=0;if(v.IsInt64())n=v.GetInt64();else if(v.IsUint64()&&v.GetUint64()<=INT32_MAX)n=std::int64_t(v.GetUint64());
  else if(v.IsString()){const auto r=std::from_chars(v.GetString(),v.GetString()+v.GetStringLength(),n);if(r.ec!=std::errc{}||r.ptr!=v.GetString()+v.GetStringLength())throw std::runtime_error("grid_integer");}
  else throw std::runtime_error("grid_integer");if(n<INT32_MIN||n>INT32_MAX)throw std::runtime_error("grid_integer_range");return int(n);}
bool Boolean(const V &v){if(!v.IsBool())throw std::runtime_error("grid_boolean");return v.GetBool();}
std::string Text(const V &v){if(!v.IsString()||v.GetStringLength()>256)throw std::runtime_error("grid_text");std::string s(v.GetString(),v.GetStringLength());
  if(s.find('\0')!=s.npos)throw std::runtime_error("grid_text_nul");return s;}
float Real(const V &v){if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>1000000)throw std::runtime_error("grid_vector");return float(v.GetDouble());}
ClientPoseVector Vector(const V &v){if(!Is(At(v,"type"),"vector"))throw std::runtime_error("grid_vector_type");return {Real(At(v,"x")),Real(At(v,"y"))};}
ClientObjectKey Key(const replica::SubjectKey &k){return {k.run,k.id,k.space};}
bool Null(const V &v){return v.IsNull()||(v.IsObject()&&v.HasMember("status")&&Is(v["status"],"Null"));}
// 旧来源两次读取同值Sprite时，局部table节点编号不同；比较图的对应关系而非裸编号。
// 只对typed table/graph_ref的node做双向映射；定义编号可因组件外部引用而仅一侧保留，
// graph_ref本身仍需两侧存在并保持双向对应，普通数值和图连接关系仍必须相等。
bool SameSpriteValue(const V &a,const V &b){
  std::map<std::uint64_t,std::uint64_t> forward,backward;
  const auto compare=[&](auto &&self,const V &x,const V &y)->bool {
    if(x.IsArray()&&y.IsArray()){if(x.Size()!=y.Size())return false;for(unsigned i=0;i<x.Size();++i)if(!self(self,x[i],y[i]))return false;return true;}
    if(!x.IsObject()||!y.IsObject())return x==y;
    const bool table=x.HasMember("type")&&Is(x["type"],"table")&&y.HasMember("type")&&Is(y["type"],"table");
    if(x.MemberCount()-(table&&x.HasMember("node"))!=y.MemberCount()-(table&&y.HasMember("node")))return false;
    const bool graph=x.HasMember("type")&&(Is(x["type"],"table")||Is(x["type"],"graph_ref"));
    for(auto i=x.MemberBegin();i!=x.MemberEnd();++i){if(table&&Is(i->name,"node")&&!y.HasMember("node"))continue;
      if(!y.HasMember(i->name))return false;const auto &v=y[i->name];
      if(graph&&Is(i->name,"node")){const auto left=replica::Unsigned(i->value),right=replica::Unsigned(v);
        auto f=forward.emplace(left,right),r=backward.emplace(right,left);if(f.first->second!=right||r.first->second!=left)return false;
      }else if(!self(self,i->value,v))return false;
    }return true;
  };return compare(compare,a,b);
}
// 每个类别只接收真实公开派生字段；无新增字段的类别仍核对全部 cast 和原生资源。
void Field(ClientGridDescription &d,const V &v,unsigned cast,const char *name,ClientGridValueKind kind){
  const std::string id=std::string("GridEntity")+casts[cast]+"."+name;const auto &value=At(v,id.c_str());
  ClientGridField field;field.cast=cast;field.name=name;field.kind=kind;
  if(kind==ClientGridValueKind::Integer)field.integer=Integer(value);
  else if(kind==ClientGridValueKind::Boolean)field.boolean=Boolean(value);
  else if(Null(value)&&cast==7&&field.name=="NextGreedAnimation")field.null_text=true;else field.text=Text(value);
  d.fields.push_back(std::move(field));
}
void Sprite(ClientGridDescription &d,const V &v,unsigned role,bool nullable=false){
  ClientGridSprite sprite;sprite.role=role;sprite.present=!Null(v);
  if(!sprite.present&&!nullable)throw std::runtime_error("grid_required_sprite_null");
  if(sprite.present){std::string reason;if(!ClientParseResourcePose(v,sprite.pose,reason))throw std::runtime_error("grid_sprite:"+reason);}
  d.sprites.push_back(std::move(sprite));
}
ClientGridDescription Parse(const replica::RoomGridTarget &target,const replica::GridTarget &g){
  ClientGridDescription d;d.key=Key(g.key);d.room=Key(target.room.key);d.revision=g.instance_revision;
  d.index=g.index;d.type=g.type;d.variant=g.variant;d.state=g.state;d.var_data=g.var_data;d.collision=g.collision;
  const auto &desc=g.descriptor;d.desc_state=desc.state;d.desc_var_data=desc.var_data;d.spawn_count=desc.spawn_count;
  d.spawn_seed=desc.spawn_seed;d.variable_seed=desc.variable_seed;d.source_initialized=desc.initialized;
  // J273 common State/VarData 与 Desc 是同一存储；矛盾来源不能先后覆盖而假装两份都完成。
  if(d.state!=d.desc_state||d.var_data!=d.desc_var_data)throw std::runtime_error("grid_desc_alias_conflict");
  for(const auto &m:g.memberships)if(m.source==11)d.grid_member=true;else d.door_aliases|=1u<<m.index;
  const auto &source=target.GridState(g),&common=At(At(source,"common"),"fields"),&mask=At(source,"casts");
  const auto &runtime=At(source,"application_runtime");
  if(Integer(At(runtime,"version"))!=2)throw std::runtime_error("grid_runtime_version");
  const auto &spawn=At(runtime,"spawn_frame");if(!spawn.IsUint())throw std::runtime_error("grid_spawn_frame");d.spawn_frame=spawn.GetUint();
  if(g.type==16){d.door_animation_mode=Boolean(At(runtime,"door_animation_mode"));
    const auto &flags=At(runtime,"door_light_flags");if(!flags.IsUint())throw std::runtime_error("grid_door_light_flags");d.door_light_flags=flags.GetUint();}
  const auto sequence=[](const V &v)->const V &{if(v.IsArray())return v;if(v.IsObject()&&!v.MemberCount()){static const V empty(rapidjson::kArrayType);return empty;}throw std::runtime_error("grid_runtime_array");};
  if(g.type==13){d.fire_timer=Integer(At(runtime,"fire_timer"));d.fire_style=Integer(At(runtime,"fire_style"));if(d.fire_style<0||d.fire_style>2)throw std::runtime_error("grid_fire_style");
    const auto &embers=sequence(At(runtime,"embers"));if(embers.Size()>4096)throw std::runtime_error("grid_ember_budget");
    for(const auto &e:embers.GetArray()){ClientGridEmber v;v.x=Real(At(e,"x"));v.y=Real(At(e,"y"));v.vx=Real(At(e,"vx"));v.vy=Real(At(e,"vy"));v.age=Integer(At(e,"age"));v.max_life=Integer(At(e,"max_life"));
      if(v.age<0||v.max_life<=0||v.age>v.max_life)throw std::runtime_error("grid_ember_lifetime");d.embers.push_back(v);}
    const auto &sprites=At(source,"application_sprites");if(!sprites.IsArray()||sprites.Size()!=2)throw std::runtime_error("grid_fire_sprites");
    for(unsigned i=0;i<2;++i)Sprite(d,sprites[i],i+3);
  }
  d.position=Vector(At(common,"GridEntity.Position"));d.render_position=Vector(At(common,"GridEntity.GetRenderPosition"));
  const float x=float(g.index%target.room.layout.grid_width)*40+40,y=float(g.index/target.room.layout.grid_width)*40+120;
  if(d.position.x!=x||d.position.y!=y)throw std::runtime_error("grid_position_geometry_conflict");
  const auto &render=At(runtime,"render_inputs");const float width=Real(At(render,"width")),height=Real(At(render,"height")),scale=Real(At(render,"pixel_scale"));
  if(width<=0||height<=0||scale<=0)throw std::runtime_error("grid_render_input_range");
  const auto project=[scale](float value){return std::floor(value*scale+0.5f)/scale;};
  if(d.render_position.x!=project((x-60)*0.65f+(width-338)*0.5f)||d.render_position.y!=project((y-140)*0.65f+(height-182)*0.5f))throw std::runtime_error("grid_render_source_conflict");
  const int water=Integer(At(common,"GridEntity.GetWaterClipFlags"));if(water<0||water>255)throw std::runtime_error("grid_water_flags");d.water_clip=unsigned(water);
  Sprite(d,At(common,"GridEntity.GetSprite"),0);
  for(unsigned i=0;i<17;++i){const auto &value=At(mask,casts[i]);if(Is(At(value,"status"),"Value"))d.cast_mask|=1u<<i;
    else if(!Is(value["status"],"NotApplicable"))throw std::runtime_error("grid_cast_unknown");}
  if(!d.cast_mask)throw std::runtime_error("grid_cast_empty");
  // 706100 的真实 vtable 映射；Pillar=24 经工厂证实为 Rock，不能凭名称降级。
  static constexpr unsigned type_cast[]={0,0,8,8,8,8,8,5,9,9,16,4,13,2,6,15,1,14,10,3,7,11,8,12,8,8,8,8};
  if(d.cast_mask!=(1u<<type_cast[d.type]))throw std::runtime_error("grid_factory_cast_contract_conflict");
  if(d.cast_mask&(1u<<8)){const auto &rubble=sequence(At(runtime,"rubble"));if(rubble.Size()>4096)throw std::runtime_error("grid_rubble_budget");
    for(const auto &v:rubble.GetArray())d.rubble.push_back({Real(At(v,"x")),Real(At(v,"y"))});}
  const auto &bodies=At(source,"bodies");
  for(unsigned i=0;i<17;++i)if(d.cast_mask&(1u<<i)){
    const std::string class_name=std::string("GridEntity")+casts[i];const auto &body=At(At(bodies,class_name.c_str()),"fields");
    const auto integer=[&](const char *name){Field(d,body,i,name,ClientGridValueKind::Integer);};
    const auto boolean=[&](const char *name){Field(d,body,i,name,ClientGridValueKind::Boolean);};
    const auto text=[&](const char *name){Field(d,body,i,name,ClientGridValueKind::Text);};
    switch(i){
      case 1:
        boolean("Busted");integer("Direction");integer("Slot");integer("CurrentRoomType");integer("TargetRoomType");integer("TargetRoomIndex");
        integer("PreviousState");integer("PreviousVariant");boolean("ExtraVisible");text("CloseAnimation");text("LockedAnimation");text("OpenAnimation");text("OpenLockedAnimation");
        Sprite(d,At(body,"GridEntityDoor.ExtraSprite"),1);break;
      case 5:boolean("HasLadder");break;
      case 6:
        integer("ReviveTimer");boolean("UnderPlayer");text("StateAnimation");
        // Wiki明确Repentance已移除此字段；旧来源实际Getter为nil，保留Null/Unavailable版本事实。
        if(body.HasMember("GridEntityPoop.ReducedSpawnRate")){const auto &old=body["GridEntityPoop.ReducedSpawnRate"];
          if(!old.IsObject()||!old.HasMember("status")||(!Is(old["status"],"Unavailable")&&!Is(old["status"],"Null")))throw std::runtime_error("grid_removed_field_status");}break;
      case 7:text("NextGreedAnimation");Sprite(d,At(body,"GridEntityPressurePlate.TimerPlate"),2,true);break;
      case 8:text("Anim");text("RubbleAnim");integer("FrameCnt");d.big_rock_frame=Integer(At(body,"GridEntityRock.GetBigRockFrame"));
        {const auto &alias=At(body,"GridEntityRock.GetSprite");
          const bool canonical=alias.IsObject()&&alias.MemberCount()==2&&alias.HasMember("type")&&Is(alias["type"],"resource_alias")&&alias.HasMember("member")&&Is(alias["member"],"GridEntity.GetSprite");
          if(!canonical&&!SameSpriteValue(alias,At(common,"GridEntity.GetSprite")))throw std::runtime_error("grid_rock_sprite_alias_conflict");}
        if(d.big_rock_frame<-1||d.big_rock_frame>7||(d.big_rock_frame>=0&&d.variant!=1000+d.big_rock_frame))throw std::runtime_error("grid_big_rock_variant_conflict");break;
      case 9:integer("Timeout");break;
      case 13:integer("FrameCnt");break;
      default:break;
    }
  }
  // 普通非Greed板的真实后态为非空指针的空字符串；它与nil均不选择下一动画。
  // 有名字时才按官方资源目录核验，不能把合法空字符串误拒绝或改写成nil。
  for(const auto &field:d.fields)if(field.cast==7&&field.name=="NextGreedAnimation"&&!field.null_text&&!field.text.empty()){
    static const std::set<std::string> animations={"Off","On","Switched","SwitchedRed","SwitchedSkull","OffRed","OffSkull","OnSkull","SwitchedPentagram","OffPentagram","OnPentagram","OffRedStart","OffRedEnd"};
    if(!animations.count(field.text))throw std::runtime_error("grid_pressureplate_animation_unknown");}
  return d;
}
}
ClientGrids::~ClientGrids(){ClientNativeGridsDestroy(native_);}
// 完整预检成功后才替换候选；失败时已提交 A 和原生映射均不变。
bool ClientGrids::PrepareTarget(const replica::RoomGridPlan &plan,const char *&reason) noexcept {
  try{
    if(failed_||(target_&&(cursor_||written_))||!plan.target)throw std::runtime_error("grid_plan_inflight_or_failed");
    if(plan.applied!=committed_&&!(retired_&&!plan.applied))throw std::runtime_error("grid_applied_owner_mismatch");
    if(plan.room==replica::RoomPlanKind::Rebuild&&committed_)throw std::runtime_error("grid_room_rebuild_requires_retirement");
    // v3/v4仍可纯值解析，但其Door/TimerPlate值副本丢失owner偏移/flags，不能冒认原生成员姿态。
    if(Integer(At(At(plan.target->source["components"]["grid"]["state"],"application_source"),"version"))<5)
      throw std::runtime_error("grid_actual_member_pose_source_required");
    std::map<replica::SubjectKey,ClientGridDescription> values;
    for(const auto &g:plan.target->grids)values.emplace(g.key,Parse(*plan.target,g));
    // 缺失依赖写前具名失败；先登记全体Key再解析，Grid环不会触发构造顺序等待。
    std::set<replica::SubjectKey> entities,descriptors;
    const auto &rows=plan.target->source["components"]["entity"]["state"]["objects"];
    if(!rows.IsArray())throw std::runtime_error("grid_entity_membership_required");
    for(const auto &row:rows.GetArray()){const auto &key=At(At(row,"subject"),"target");
      entities.insert({replica::Unsigned(At(key,"run")),replica::Unsigned(At(key,"id")),unsigned(Integer(At(key,"space")))});}
    for(const auto &d:plan.target->room.descriptors)descriptors.insert(d.key);
    for(const auto &g:plan.target->grids)for(const auto &ref:g.references){
      if(!(ref.space==2?values.count(ref):ref.space==1?entities.count(ref):ref.space==6?descriptors.count(ref):false))
        throw std::runtime_error("grid_required_reference_missing:key="+std::to_string(g.key.id)+":target="+std::to_string(ref.id));}
    for(const auto &v:plan.target->lifted)if(!entities.count(v.owner))throw std::runtime_error("grid_lifted_entity_owner_missing:key="+std::to_string(v.owner.id));
    std::vector<ClientGridCommand> commands;
    for(const auto &step:plan.steps){ClientGridOperation op;
      switch(step.operation){
        case replica::RoomGridOperation::GridRemove:case replica::RoomGridOperation::GridRetireInstance:op=ClientGridOperation::Remove;break;
        case replica::RoomGridOperation::GridCreate:case replica::RoomGridOperation::GridReplace:op=ClientGridOperation::Create;break;
        case replica::RoomGridOperation::GridChange:op=ClientGridOperation::Change;break;
        default:continue;
      }
      ClientGridDescription value;
      if(op==ClientGridOperation::Remove){value.key=Key(step.key);value.room=Key(plan.target->room.key);}
      else value=values.at(step.key);commands.push_back({op,std::move(value)});
    }
    // 最终检查由Scene提交前按一格/步执行，不在安装和提交两次重复解析/全图检查。
    std::vector<ClientGridCommand> checks;
    checks.reserve(values.size());
    for(auto &[key,value]:values)checks.push_back({ClientGridOperation::Check,std::move(value)});
    std::array<ClientGridDoorBinding,8> doors{};
    for(unsigned i=0;i<8;++i){const auto &s=plan.target->room.doors[i];auto &d=doors[i];
      d.slot=int(i);d.candidate=s.shape_target;d.target=s.actual_target;d.shape_allowed=s.shape_allowed;d.enabled=s.descriptor_enabled;
      d.is_null=s.actual.is_null;d.actual=Key(s.actual.key);d.route=int(s.route);
      d.descriptor_null=s.target_descriptor.is_null;d.descriptor=Key(s.target_descriptor.key);
      if(!d.is_null&&(s.route==replica::DoorRoute::Map||s.route==replica::DoorRoute::OffGrid)){
        if(s.target_uninitialized)continue;
        bool resolved=false;
        for(const auto &descriptor:plan.target->room.descriptors){const auto &p=descriptor.location;
          const bool matches=p.source_slot?p.source_slot==d.target:(p.dimension==plan.target->room.dimension&&std::find(p.occupied.begin(),p.occupied.end(),d.target)!=p.occupied.end());
          if(matches){if(resolved||!(d.descriptor==Key(descriptor.key)))throw std::runtime_error("grid_door_target_ambiguous");resolved=true;}}
        if(!resolved||d.descriptor_null)throw std::runtime_error("grid_door_target_descriptor_missing");
      }
    }
    commands_=std::move(commands);planned_checks_=std::move(checks);doors_=doors;target_=plan.target;cursor_=0;written_=doors_bound_=native_started_=false;retired_=false;
    reason_="grid_target_ready";reason=reason_.c_str();return true;
  }catch(const std::exception &e){reason_=e.what();}catch(...){reason_="grid_plan_allocation";}
  reason=reason_.c_str();return false;
}
bool ClientGrids::CancelPlanned() noexcept {if(cursor_||written_||failed_)return false;target_.reset();commands_.clear();planned_checks_.clear();return true;}
bool ClientGrids::StreamCompatible(const replica::RoomGridTarget &target) const noexcept {
  if(failed_||target_||!committed_||target.grids.size()!=committed_->grids.size())return false;
  for(const auto &g:target.grids){auto old=std::find_if(committed_->grids.begin(),committed_->grids.end(),[&](const auto &v){return v.key==g.key;});if(old==committed_->grids.end()||old->instance_revision!=g.instance_revision||old->type!=g.type||old->variant!=g.variant||old->memberships!=g.memberships)return false;}
  for(unsigned i=0;i<8;++i){const auto &a=target.room.doors[i],&b=committed_->room.doors[i];
    if(a.slot!=b.slot||a.shape_allowed!=b.shape_allowed||a.descriptor_enabled!=b.descriptor_enabled||a.shape_target!=b.shape_target||a.actual_target!=b.actual_target||a.actual.is_null!=b.actual.is_null||a.actual.key!=b.actual.key||a.target_descriptor.is_null!=b.target_descriptor.is_null||a.target_descriptor.key!=b.target_descriptor.key||a.target_uninitialized!=b.target_uninitialized||a.route!=b.route)return false;}
  return true;
}
std::vector<ClientGridDescription> ClientGrids::CompileStream(const replica::RoomGridTarget &target) const {
  if(failed_||target_||!committed_||target.grids.size()!=committed_->grids.size())throw std::runtime_error("grid_stream_structure");std::vector<ClientGridDescription> out;
  for(const auto &g:target.grids){auto old=std::find_if(committed_->grids.begin(),committed_->grids.end(),[&](const auto &v){return v.key==g.key;});if(old==committed_->grids.end()||old->instance_revision!=g.instance_revision||old->type!=g.type||old->variant!=g.variant||old->memberships!=g.memberships)throw std::runtime_error("grid_stream_instance");out.push_back(Parse(target,g));}return out;
}
ClientGridStep ClientGrids::InstallStream(const ClientNativeRoots &roots,const std::vector<ClientGridDescription> &values) noexcept {
  if(failed_||target_||!committed_||values.size()!=checks_.size())return {false,false,true,0,0,"grid_stream_owner"};ClientGridStep step;bool written=false;
  for(const auto operation:{ClientGridOperation::Change,ClientGridOperation::Check})for(const auto &d:values){ClientGridCommand c;c.operation=operation;c.value=d;step=ClientNativeGridsExecute(native_,roots,c);written|=step.written;if(step.failed||!step.done){failed_|=step.failed;step.written=written;return step;}}
  try{checks_.clear();for(const auto &d:values)checks_.push_back({ClientGridOperation::Check,d});check_cursor_=0;}catch(...){failed_=true;return {false,true,true,step.prefix,step.owners,"grid_stream_commit_allocation"};}step.done=true;step.written=written;return step;
}
ClientGridStep ClientGrids::Step(const ClientNativeRoots &roots,unsigned budget) noexcept {
  if(failed_||!target_)return {false,written_,true,0,0,"grid_target_not_ready"};
  if(!budget||budget>16)return {false,written_,true,0,0,"grid_step_budget"};
  if(!native_)native_=ClientNativeGridsCreate();if(!native_)return {false,false,true,0,0,"grid_owner_allocation"};
  if(!native_started_){if(!ClientNativeGridsBeginAttempt(native_))return {false,false,true,0,0,"grid_attempt_context"};native_started_=true;}
  ClientGridStep result;
  for(unsigned n=0;n<budget&&cursor_<commands_.size();++n){result=ClientNativeGridsExecute(native_,roots,commands_[cursor_]);written_|=result.written;result.written=written_;
    if(result.failed){failed_=true;return result;}if(!result.done)return result;++cursor_;}
  if(cursor_<commands_.size()){result.done=false;return result;}
  if(!doors_bound_){result=ClientNativeGridsBindDoors(native_,roots,doors_);written_|=result.written;result.written=written_;
    if(result.failed){failed_=true;return result;}doors_bound_=result.done;if(!doors_bound_)return result;}
  committed_=target_;target_.reset();commands_.clear();checks_=std::move(planned_checks_);check_cursor_=0;cursor_=0;written_=false;
  result.done=true;result.reason="grid_domain_installed_scene_commit_pending";return result;
}
ClientGridStep ClientGrids::Retire(const ClientNativeRoots &roots) noexcept {
  auto result=native_?ClientNativeGridsRetire(native_,roots):ClientGridStep{true,false,false,0,0,"grid_owner_empty"};
  if(result.done&&!result.failed){target_.reset();committed_.reset();commands_.clear();checks_.clear();planned_checks_.clear();check_cursor_=0;cursor_=0;written_=failed_=doors_bound_=native_started_=false;retired_=true;}
  return result;
}
bool ClientGrids::Visit(void *context,ClientGridVisitor visitor) const noexcept {return !failed_&&!target_&&committed_&&ClientNativeGridsVisit(native_,context,visitor);}
// Pit.MakeBridge 没有持久父指针，Door 目标为 descriptor 纯值关系；不发明可落写的指针字段。
// 必需 Entity 只接受同场景已安装实例；19 尚未提供的真实类别会由对象预检写前拒绝。
ClientGridStep ClientGrids::ResolveReferences(const ClientObjects &objects,std::uint64_t attempt) noexcept {
  if(failed_||target_||!committed_)return {false,written_,true,0,0,"grid_reference_target_not_installed"};
  try{
    std::set<ClientObjectKey> entities;
    const auto collect=[](void *c,void *,const ClientObjectKey &key,std::uint64_t,unsigned) noexcept {
      try{static_cast<std::set<ClientObjectKey> *>(c)->insert(key);return true;}catch(...){return false;}
    };
    if(!objects.VisitPrepared(attempt,&entities,collect))return {false,false,true,0,0,"grid_required_entity_attempt_unavailable"};
    std::set<replica::SubjectKey> grids,descriptors;
    for(const auto &g:committed_->grids)grids.insert(g.key);
    for(const auto &d:committed_->room.descriptors)descriptors.insert(d.key);
    for(const auto &g:committed_->grids)for(const auto &ref:g.references){
      const bool resolved=ref.space==2?grids.count(ref):ref.space==1?entities.count(Key(ref)):
        ref.space==6?descriptors.count(ref):false;
      if(!resolved){failed_=true;return {false,false,true,0,0,"grid_required_reference_unresolved"};}
    }
    const char *reason="grid_lifted_requires_entity19_owner";
    for(const auto &v:committed_->lifted)if(!entities.count(Key(v.owner))||!objects.CheckLifted(v,reason)){failed_=true;return {false,false,true,0,0,reason};}
    return {true,false,false,0,unsigned(grids.size()),"grid_references_resolved"};
  }catch(...){failed_=true;return {false,false,true,0,0,"grid_reference_allocation"};}
}
bool ClientGrids::VisitLifted(void *context,LiftedVisitor visitor) const noexcept {
  if(!visitor||failed_||target_||!committed_)return false;
  for(const auto &value:committed_->lifted)if(!visitor(context,value))return false;return true;
}
// 每轮只检查一个主体；无全图解析/资源重载，返回done前Scene不得提交。
ClientGridStep ClientGrids::Check(const ClientNativeRoots &roots) noexcept {
  if(failed_||target_||!committed_)return {false,false,true,0,0,"grid_commit_owner_not_ready"};
  try{
    if(check_cursor_<checks_.size()){
      auto result=ClientNativeGridsExecute(native_,roots,checks_[check_cursor_]);
      if(result.failed){failed_=true;return result;}
      if(result.done)++check_cursor_;
      result.done=false;return result;
    }
    return ClientNativeGridsBindDoors(native_,roots,doors_);
  }catch(...){failed_=true;return {false,false,true,0,0,"grid_commit_check_allocation"};}
}
} // namespace cs::game
