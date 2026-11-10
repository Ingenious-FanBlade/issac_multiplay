#include "game/client/presentation/client_presentation.h"
#include "game/client/scene/client_dto_memory.h"
#include "game/client/resources/client_resource_model.h"
#include "replica/player_target.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <functional>
#include <limits>

namespace cs::game {
std::uint64_t ClientPresentation::StreamValueBytes() const {return sizeof(*this)+dto_memory::Extra(target_)+ClientPresentationValueBytes(native_);}
namespace {
const replica::Value &Member(const replica::Value &v,const char *name) {
  if(!v.IsObject()||!v.HasMember(name))throw std::runtime_error(std::string("presentation_missing:")+name);
  return v[name];
}
float Number(const replica::Value &v) {
  if(!v.IsNumber())throw std::runtime_error("presentation_number");
  const double n=v.GetDouble();
  if(!std::isfinite(n)||std::abs(n)>1000000)throw std::runtime_error("presentation_nonfinite");
  return float(n);
}
bool Boolean(const replica::Value &v) {
  if(!v.IsBool())throw std::runtime_error("presentation_boolean");
  return v.GetBool();
}
int Integer(const replica::Value &v,int low,int high) {
  const float n=Number(v);
  if(std::floor(n)!=n||n<low||n>high)throw std::runtime_error("presentation_integer");
  return int(n);
}
std::uint32_t Word(const replica::Value &v){if(!v.IsUint64()||v.GetUint64()>UINT32_MAX)throw std::runtime_error("presentation_word");return std::uint32_t(v.GetUint64());}
// v5只为96C620的零比较条件提供精确IEEE表示，并核对分类；坐标/颜色/速度仍走有限数检查。
// 原生ctor未完整初始化此字，不能把实机ffffffff擅自命名为“初始化sentinel”或改成默认0。
std::uint32_t FXCondition(const replica::Value &v,int version){
  if(version>=5){
    if(!v.IsObject()||v.MemberCount()!=3||!Member(v,"type").IsString()||std::string(Member(v,"type").GetString())!="float32_condition"||!Member(v,"classification").IsString())throw std::runtime_error("presentation_fx_condition_encoding");
    const auto bits=Word(Member(v,"bits"));const auto exponent=bits&0x7f800000u,mantissa=bits&0x007fffffu;
    const char *classification=exponent!=0x7f800000u?"Finite":mantissa?"NaN":bits&0x80000000u?"NegativeInfinity":"PositiveInfinity";
    if(std::string(Member(v,"classification").GetString())!=classification)throw std::runtime_error("presentation_fx_condition_classification");
    return bits;
  }
  if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>std::numeric_limits<float>::max())throw std::runtime_error("presentation_fx_nonfinite_condition");
  const float value=float(v.GetDouble());std::uint32_t bits=0;std::memcpy(&bits,&value,4);return bits;
}
std::string Text(const replica::Value &v) {
  if(!v.IsString()||v.GetStringLength()>96)throw std::runtime_error("presentation_animation");
  return {v.GetString(),v.GetStringLength()};
}
std::string ImagePath(const replica::Value &v) {
  if(!v.IsString()||!v.GetStringLength()||v.GetStringLength()>1024)throw std::runtime_error("presentation_fx_image");
  std::string path(v.GetString(),v.GetStringLength());
  if((path.rfind("gfx/",0)!=0&&path.rfind("resources/gfx/",0)!=0)||path.find("..")!=std::string::npos||path.find(':')!=std::string::npos||
     path.find('\\')!=std::string::npos||path.find_first_of("\r\n\0",0,3)!=std::string::npos)
    throw std::runtime_error("presentation_fx_image_path");
  return path;
}
template <std::size_t N> void Colors(const replica::Value &v,float (&out)[N]) {
  if(!v.IsArray()||v.Size()!=N)throw std::runtime_error("presentation_color_size");
  for(unsigned i=0;i<N;++i)out[i]=Number(v[i]);
}
// 正式Lua编码器把空table输出为{}；仅接受这个精确空值，保留空成员的真实证明。
const replica::Value &Sequence(const replica::Value &v) {
  if(v.IsArray())return v;
  if(v.IsObject()&&v.MemberCount()==0){static const replica::Value empty(rapidjson::kArrayType);return empty;}
  throw std::runtime_error("presentation_sequence");
}
}
ClientPresentation::ClientPresentation() noexcept=default;
ClientPresentation::~ClientPresentation(){ClientPresentationDestroy(native_);}
// 先完整解释采用范围再触碰原生owner；缺失字段不能被默认的白色/零水面掩盖。
bool ClientPresentation::PrepareTarget(const replica::Value &model,const char *&reason,const ClientRoomState &room_state,const ClientMapTarget &map_target) noexcept {
  try {
    const auto &v=Member(Member(Member(Member(model,"components"),"room"),"state"),"presentation_runtime");
    const auto &transition=Member(Member(Member(Member(Member(Member(model,"components"),"room"),"state"),"transition"),"fields"),"RoomTransition.GetTransitionMode");
    // 既有正式域采用int64封装；本代表只展示稳定房间，不把菜单残留过渡fade当来源。
    if(!transition.IsObject()||!Member(transition,"type").IsString()||std::string(Member(transition,"type").GetString())!="int64"||
       !Member(transition,"value").IsString()||std::string(Member(transition,"value").GetString())!="0")
      throw std::runtime_error("presentation_stable_room_transition_required");
    const int presentation_version=Integer(Member(v,"version"),2,5);
    ClientPresentationDescription d;d.room_state=room_state;d.map=map_target;d.full_players=replica::HasPlayerTarget(model);
    const auto &room=Member(Member(Member(model,"components"),"room"),"state");
    if(room.HasMember("map_sprites")){
      const auto &sprites=room["map_sprites"];if(!sprites.IsArray()||sprites.Size()!=2)throw std::runtime_error("presentation_minimap_poses");
      for(unsigned i=0;i<2;++i){std::string error;if(!ClientParseResourcePose(sprites[i],d.minimap_poses[i],error))throw std::runtime_error("presentation_minimap_pose:"+error);}
      d.minimap_poses_present=true;
    }else if(Integer(Member(Member(room,"application_source"),"version"),4,7)>=7)throw std::runtime_error("presentation_minimap_pose_required");
    Colors(Member(v,"floor_color"),d.floor);Colors(Member(v,"wall_color"),d.wall);
    Colors(Member(v,"shadow_color"),d.shadow);Colors(Member(v,"light_color"),d.light);
    Colors(Member(v,"water_color"),d.water_color);Colors(Member(v,"water_multiplier"),d.water_multiplier);
    Colors(Member(v,"water_target"),d.water_target);Colors(Member(v,"water_target_multiplier"),d.water_target_multiplier);
    Colors(Member(v,"fx_water_color"),d.fx_water_color);Colors(Member(v,"fx_water_multiplier"),d.fx_water_multiplier);
    Colors(Member(v,"water_effect"),d.water_effect);Colors(Member(v,"fx_room_color"),d.fx_room_color);
    Colors(Member(v,"current_color"),d.current_color);Colors(Member(v,"target_color"),d.target_color);Colors(Member(v,"lerp_color"),d.lerp_color);
    Colors(Member(v,"water_current"),d.water_current);
    Colors(Member(v,"light_tint"),d.light_tint);Colors(Member(v,"shadow_fill"),d.shadow_fill);
    d.special_light_pass=Boolean(Member(v,"special_light_pass"));
    d.layer4_enabled=Boolean(Member(v,"layer4_enabled"));
    d.mirror=Boolean(Member(v,"mirror"));d.mirror_shader=Boolean(Member(v,"mirror_shader"));d.mirror_phase=Number(Member(v,"mirror_phase"));
    if(d.mirror_phase<0||d.mirror_phase>=10)throw std::runtime_error("presentation_mirror_phase");
    d.fx_brightness=Number(Member(v,"fx_brightness"));d.fx_contrast=Number(Member(v,"fx_contrast"));
    d.lava=Number(Member(v,"lava"));d.rain=Number(Member(v,"rain"));d.lightning=Number(Member(v,"lightning"));
    d.darkness=Number(Member(v,"darkness"));d.darkness_scale=Number(Member(v,"darkness_scale"));
    d.rain_spawners=Integer(Member(v,"rain_spawners"),0,100000);d.water_lerp_duration=Integer(Member(v,"water_lerp_duration"),INT32_MIN,INT32_MAX);
    d.water_lerp_countdown=Integer(Member(v,"water_lerp_countdown"),INT32_MIN,INT32_MAX);
    d.use_water_v2=Boolean(Member(v,"use_water_v2"));d.color_lerp=Boolean(Member(v,"color_lerp"));
    d.water_enabled=Boolean(Member(v,"water_enabled"));
    d.color_enabled=Boolean(Member(v,"color_enabled"));
    d.shadow_alpha=Number(Member(v,"shadow_alpha"));d.water_amount=Number(Member(v,"water_amount"));
    d.lighting_alpha=Number(Member(v,"lighting_alpha"));
    d.ordinary_light_tint=Boolean(Member(v,"ordinary_light_tint"));
    const auto &shake=Member(v,"screen_shake"),&scroll=Member(v,"render_scroll");
    if(!shake.IsArray()||shake.Size()!=2||!scroll.IsArray()||scroll.Size()!=2)throw std::runtime_error("presentation_scroll");
    for(unsigned i=0;i<2;++i){d.screen_shake[i]=Number(shake[i]);d.render_scroll[i]=Number(scroll[i]);}
    d.hud_visible=Boolean(Member(v,"hud_visible"));d.room_fx_present=Boolean(Member(v,"room_fx_present"));
    const auto &context=Member(v,"hud_context"),&map=Member(v,"minimap_runtime");
    d.hud_achievement_blocked=Boolean(Member(context,"achievement_blocked"));d.hud_seed_blocked=Boolean(Member(context,"seed_blocked"));
    d.hud_custom_run=Boolean(Member(context,"custom_run"));d.hud_rerun=Boolean(Member(context,"rerun"));d.hud_debug=Boolean(Member(context,"debug"));
    d.hud_challenge=unsigned(Integer(Member(context,"challenge"),0,100000));d.hud_difficulty=unsigned(Integer(Member(context,"difficulty"),0,3));
    const auto &selection=Member(context,"coop_selection");
    if(!selection.IsArray()||selection.Size()!=8)throw std::runtime_error("presentation_coop_selection");
    for(unsigned i=0;i<8;++i)d.coop_selection[i]=Integer(selection[i],-100000,100000);
    // 97DC46仅在同槽两个索引均非负时绘制；已加入首Player的(0,-1)仍是关闭面板。
    for(unsigned i=0;i<4;++i)if(d.coop_selection[2*i]>=0&&d.coop_selection[2*i+1]>=0)
      throw std::runtime_error("presentation_coop_menu_adapter_required");
    // 97D0BB/97D370直接以difficulty 2/3和已安装的greed_wave绘制计数；不另选模式或推进波次。
    if(d.hud_seed_blocked||d.hud_challenge)throw std::runtime_error("presentation_hud_run_adapter_required");
    d.effective_curses=unsigned(Integer(Member(map,"effective_curses"),0,511));d.minimap_state=Integer(Member(map,"state"),0,4);
    d.minimap_small_alpha=Number(Member(map,"small_alpha"));d.minimap_big_alpha=Number(Member(map,"big_alpha"));
    d.minimap_shake_duration=Integer(Member(map,"shake_duration"),0,100000);
    const auto &map_shake=Member(map,"shake");if(!map_shake.IsArray()||map_shake.Size()!=2)throw std::runtime_error("presentation_minimap_shake");
    for(unsigned i=0;i<2;++i)d.minimap_shake[i]=Number(map_shake[i]);
    // 普通/Lost 均消费真实 descriptor DisplayFlags 和来源已裁决 alpha，不创建额外诅咒。
    if(d.minimap_small_alpha<0||d.minimap_small_alpha>1||d.minimap_big_alpha<0||d.minimap_big_alpha>1)
      throw std::runtime_error("presentation_minimap_alpha");
    const auto image_description=[&](const replica::Value &source,auto &out){
      out.image=ImagePath(Member(source,"image"));out.shader=unsigned(Integer(Member(source,"shader"),0,64));
      out.min_filter=unsigned(Integer(Member(source,"min_filter"),0,1));out.mag_filter=unsigned(Integer(Member(source,"mag_filter"),0,1));
      out.wrap_s=unsigned(Integer(Member(source,"wrap_s"),0,2));out.wrap_t=unsigned(Integer(Member(source,"wrap_t"),0,2));
      out.vertex_stride=unsigned(Integer(Member(source,"vertex_stride"),0,512));const auto &attributes=Member(source,"attributes");
      if(!attributes.IsArray()||attributes.Size()>8)throw std::runtime_error("presentation_image_attributes");
      for(const auto &attribute:attributes.GetArray())out.attributes.push_back(static_cast<unsigned char>(Integer(attribute,1,8)));
      const std::vector<unsigned char> position{5,6,7},mask{5,6,7,2},full{5,6,7,4,3,2,1,3};
      if(!((out.shader==1&&out.vertex_stride==36&&out.attributes==position)||(out.shader==3&&out.vertex_stride==44&&out.attributes==mask)||
           (out.shader==3&&out.vertex_stride==88&&out.attributes==full)))throw std::runtime_error("presentation_image_format");
    };
    const auto &aux=Member(v,"auxiliary");auto &a=d.auxiliary;
    const bool poop_applicable=presentation_version==2||Boolean(Member(aux,"poop_applicable"));
    a.veins_present=Boolean(Member(aux,"veins_present"));
    if(poop_applicable){const auto &poop_image=Member(aux,"poop_image");
    a.poop_image_present=Boolean(Member(poop_image,"present"));if(a.poop_image_present)image_description(poop_image,a.poop_image);
    Colors(Member(aux,"poop_color"),a.poop_color);Colors(Member(aux,"poop_settings"),a.poop_settings);
    const auto &poop=Sequence(Member(aux,"poop_poses"));if(poop.Size()>256)throw std::runtime_error("presentation_poop_budget");
    for(const auto &s:poop.GetArray()){ClientPresentationFXPose pose;pose.x=Number(Member(s,"x"));pose.y=Number(Member(s,"y"));
      pose.velocity_x=Number(Member(s,"velocity_x"));pose.velocity_y=Number(Member(s,"velocity_y"));pose.flip_x=Boolean(Member(s,"flip_x"));pose.flip_y=Boolean(Member(s,"flip_y"));a.poop_poses.push_back(pose);}
    }
    if(presentation_version==2||a.veins_present){
    const auto &nodes=Member(aux,"veins"),&roots=Member(aux,"roots");if(!nodes.IsArray()||nodes.Empty()||nodes.Size()>4096||!roots.IsArray()||roots.Size()!=10)throw std::runtime_error("presentation_vein_budget");
    for(const auto &n:nodes.GetArray()){ClientPresentationVein node;node.kind=Integer(Member(n,"kind"),0,1);node.depth=Integer(Member(n,"depth"),-1000000,1000000);
      node.rotation=Number(Member(n,"rotation"));node.scale=Number(Member(n,"scale"));node.factor=Number(Member(n,"factor"));node.opacity=Number(Member(n,"opacity"));node.x=Number(Member(n,"x"));node.y=Number(Member(n,"y"));
      const auto &children=Member(n,"children");if(!children.IsArray()||children.Size()!=3)throw std::runtime_error("presentation_vein_children");
      for(unsigned i=0;i<3;++i)node.children[i]=Integer(children[i],-1,int(nodes.Size()-1));a.veins.push_back(node);}
    for(unsigned i=0;i<10;++i)a.roots[i]=unsigned(Integer(roots[i],0,int(a.veins.size()-1)));
    std::vector<bool> active(a.veins.size()),seen(a.veins.size());unsigned visits=0;
    std::function<void(unsigned,unsigned)> visit=[&](unsigned i,unsigned depth){if(depth>32||active[i]||++visits>65536)throw std::runtime_error("presentation_vein_cycle_depth_or_draw_budget");
      active[i]=true;seen[i]=true;for(int child:a.veins[i].children)if(child>=0)visit(unsigned(child),depth+1);active[i]=false;};
    for(unsigned root:a.roots)visit(root,0);for(bool value:seen)if(!value)throw std::runtime_error("presentation_vein_unowned_node");
    if(a.veins_present!=(a.veins[a.roots[0]].children[0]>=0))throw std::runtime_error("presentation_vein_membership");
    }
    if(a.veins_present){const auto &sprites=Member(Member(Member(Member(model,"components"),"room"),"state"),"vein_sprites");
    if(!sprites.IsArray()||sprites.Size()!=2)throw std::runtime_error("presentation_vein_sprites");
    for(unsigned i=0;i<2;++i){const auto &s=sprites[i];if(s.IsObject()&&s.HasMember("status")&&s["status"].IsString()&&std::string(s["status"].GetString())=="Null")continue;
      std::string error;if(!ClientParseResourcePose(s,a.vein_sprites[i],error))throw std::runtime_error("presentation_vein_sprite_pose:"+error);a.vein_sprite_present[i]=true;}}
    image_description(Member(map,"question"),d.minimap_question);
    const auto &shading=Member(v,"shading");d.shading_present=Boolean(Member(shading,"present"));
    if(d.shading_present)image_description(shading,d.shading);
    if(d.shading_present&&d.shading.vertex_stride==44)throw std::runtime_error("presentation_shading_format_conflict");
    if(d.shadow_alpha<0||d.shadow_alpha>1)throw std::runtime_error("presentation_shadow_alpha");
    if(d.lighting_alpha<0||d.lighting_alpha>1)throw std::runtime_error("presentation_lighting_alpha");
    if(d.water_amount<0||d.water_amount>1)throw std::runtime_error("presentation_water_range");
    const auto &fx=Member(v,"room_fx");
    d.fx_loaded=Boolean(Member(fx,"loaded"));
    // v3显式区分停用字段；本方描述的默认值只用于构造关闭的Client状态。
    if(presentation_version>=3&&Text(Member(fx,"applicability"))!=(d.fx_loaded?"Loaded":"NotApplicable"))
      throw std::runtime_error("presentation_fx_applicability");
    if(presentation_version==2||d.fx_loaded){
    d.fx_ray_groups=unsigned(Integer(Member(fx,"ray_groups"),0,256));
    d.fx_poop_countdown=Integer(Member(fx,"poop_countdown"),-1,100000);
    d.fx_ray_group=Integer(Member(fx,"ray_group"),-1,255);
    d.fx_ray_lighting=Boolean(Member(fx,"ray_lighting"));d.fx_ray_darkness=Boolean(Member(fx,"ray_darkness"));
    const auto &average=Member(fx,"average"),&bounds=Member(fx,"bounds"),&layers=Sequence(Member(fx,"layers"));
    if(!average.IsArray()||average.Size()!=2||!bounds.IsArray()||bounds.Size()!=4||!layers.IsArray()||layers.Size()>256)
      throw std::runtime_error("presentation_fx_budget");
    for(unsigned i=0;i<2;++i)d.fx_average[i]=Number(average[i]);
    for(unsigned i=0;i<4;++i)d.fx_bounds[i]=Number(bounds[i]);
    if(d.fx_poop_countdown>0&&(!a.poop_image_present||a.poop_poses.empty()))throw std::runtime_error("presentation_poop_resource_missing");
    const auto &groups=Sequence(Member(fx,"ray_group_members"));
    if(!groups.IsArray()||groups.Size()!=d.fx_ray_groups||groups.Size()>256)throw std::runtime_error("presentation_fx_ray_group_budget");
    if(d.fx_ray_group>=int(groups.Size()))throw std::runtime_error("presentation_fx_ray_group_index");
    for(const auto &source:groups.GetArray()){
      ClientPresentationRayGroup group;const auto &geometry=Member(source,"geometry"),&rays=Sequence(Member(source,"rays"));
      if(!geometry.IsArray()||geometry.Size()!=7||!rays.IsArray()||rays.Size()>64)throw std::runtime_error("presentation_fx_ray_budget");
      for(unsigned i=0;i<7;++i)group.geometry[i]=Number(geometry[i]);
      if(!rays.Empty()&&(group.geometry[1]<=group.geometry[0]||group.geometry[4]<=0||group.geometry[6]<=0))throw std::runtime_error("presentation_fx_ray_geometry");
      for(const auto &source_ray:rays.GetArray()){
        ClientPresentationRay ray;ray.opacity=Number(Member(source_ray,"opacity"));ray.speed=Number(Member(source_ray,"speed"));ray.phase=Number(Member(source_ray,"phase"));
        if(ray.opacity<0||ray.opacity>1||ray.speed<0||ray.speed>1)throw std::runtime_error("presentation_fx_ray_phase");
        image_description(source_ray,ray);if(ray.vertex_stride!=88)throw std::runtime_error("presentation_ray_format");group.rays.push_back(std::move(ray));
      }
      d.ray_groups.push_back(std::move(group));
    }
    for(const auto &source:layers.GetArray()){
      ClientPresentationFX layer;
      layer.id=unsigned(Integer(Member(source,"id"),0,100000));layer.pass=unsigned(Integer(Member(source,"pass"),1,6));
      layer.color_condition_bits=FXCondition(Member(source,"color_branch"),presentation_version);layer.mask_backdrop=Word(Member(source,"mask_backdrop"));layer.mask=Boolean(Member(source,"mask"));
      image_description(source,layer);
      const bool generic=layer.pass==1||layer.pass==2||layer.pass==3||layer.pass==6;
      // v2/v3未采混合模式；仅未消费该字段的pass4/5可继续使用旧来源，generic缺值必须拒绝。
      if(generic||presentation_version>=4)layer.blend_mode=unsigned(Integer(Member(source,"blend_mode"),0,2));
      if(!generic&&layer.blend_mode)throw std::runtime_error("presentation_fx_unused_blend_mode");
      if((layer.mask&&(!generic||layer.vertex_stride!=44||(layer.color_condition_bits&0x7fffffffu)!=0))||(!layer.mask&&layer.mask_backdrop)||
         (!generic&&(layer.color_condition_bits&0x7fffffffu)!=0))throw std::runtime_error("presentation_fx_mask_format_conflict");
      const auto &poses=Member(source,"poses");
      if(!poses.IsArray()||poses.Size()>256)throw std::runtime_error("presentation_fx_pose_budget");
      for(const auto &pose:poses.GetArray()){
        ClientPresentationFXPose p;p.x=Number(Member(pose,"x"));p.y=Number(Member(pose,"y"));
        p.velocity_x=Number(Member(pose,"velocity_x"));p.velocity_y=Number(Member(pose,"velocity_y"));
        p.flip_x=Boolean(Member(pose,"flip_x"));p.flip_y=Boolean(Member(pose,"flip_y"));layer.poses.push_back(p);
      }
      d.fx_layers.push_back(std::move(layer));
    }
    }
    if(presentation_version>=3&&poop_applicable!=(d.fx_loaded&&d.fx_poop_countdown>0))throw std::runtime_error("presentation_poop_applicability");
    if(d.room_fx_present!=(!d.fx_layers.empty()||!d.ray_groups.empty()||d.fx_poop_countdown>0||a.veins_present)||
       ((!d.fx_layers.empty()||!d.ray_groups.empty())&&!d.fx_loaded))
      throw std::runtime_error("presentation_fx_membership_conflict");
    for(const float *color:{d.shadow,d.light,d.water_color,d.water_multiplier,d.water_target,d.water_target_multiplier,
        d.fx_water_color,d.fx_water_multiplier,d.fx_room_color,d.light_tint,d.shadow_fill})
      if(color[4]<0||color[4]>255||std::floor(color[4])!=color[4])throw std::runtime_error("presentation_kcolor_reserved");
    const auto &hud=Member(v,"player_hud");
    d.red_flash=Integer(Member(hud,"red_flash"),-128,127);d.soul_flash=Integer(Member(hud,"soul_flash"),-128,127);
    d.gold_flash=Integer(Member(hud,"gold_flash"),-128,127);d.drop_scale=Number(Member(hud,"drop_scale"));
    if(d.drop_scale<0||d.drop_scale>4)throw std::runtime_error("presentation_hud_scale");
    const auto &hearts=Member(hud,"hearts");
    if(!hearts.IsArray()||hearts.Size()!=d.hearts.size())throw std::runtime_error("presentation_hearts");
    for(unsigned i=0;i<d.hearts.size();++i){
      auto &h=d.hearts[i];const auto &source=hearts[i];
      h.visible=Boolean(Member(source,"visible"));h.eternal=Boolean(Member(source,"eternal"));
      h.golden=Boolean(Member(source,"golden"));h.fading=Boolean(Member(source,"fading"));
      h.flash=Integer(Member(source,"flash"),0,5);h.animation=Text(Member(source,"animation"));h.overlay=Text(Member(source,"overlay"));
      if(h.visible&&h.animation.empty())throw std::runtime_error("presentation_heart_animation_missing");
    }
    const auto &stats=Member(v,"stat_hud");
    d.combined_devil_angel=Boolean(Member(stats,"combined_devil_angel"));
    const auto &values=Member(stats,"values");
    if(!values.IsArray()||values.Size()!=d.stats.size())throw std::runtime_error("presentation_stats");
    for(unsigned i=0;i<d.stats.size();++i){
      auto &s=d.stats[i];const auto &source=values[i];
      s.value=Number(Member(source,"value"));s.delta=Number(Member(source,"delta"));
      s.previous=Number(Member(source,"previous"));s.threshold=Number(Member(source,"threshold"));
      s.countdown=Integer(Member(source,"countdown"),-1,100000);
    }
    // 完整纯值候选成功才替换；缺字段/OOM等写前失败仍保留旧native展示资格。
    std::swap(target_,d);planned_=true;ready_=false;reason_="presentation_target_ready";reason=reason_.c_str();return true;
  }catch(const std::exception &e){reason_=e.what();}catch(...){reason_="presentation_target_allocation";}
  reason=reason_.c_str();return false;
}
ClientPresentationStep ClientPresentation::Step(const ClientNativeRoots &roots) noexcept {
  if(!planned_)return {false,false,true,"presentation_target_not_prepared"};
  // 同一T已准备完毕，Grid分步终检期间不重复分配FX/加载Image。
  if(ready_)return {true,false,false,"presentation_target_ready"};
  if(!native_)native_=ClientPresentationCreate();
  if(!native_)return {false,false,true,"presentation_owner_allocation"};
  auto step=ClientPresentationPrepare(native_,roots,target_);ready_=step.done&&!step.failed;return step;
}
ClientPresentationStep ClientPresentation::Render(const ClientNativeRoots &roots) noexcept {
  if(!ready_)return {false,false,true,"presentation_target_not_committed"};
  return ClientPresentationDraw(native_,roots);
}
ClientPresentationStep ClientPresentation::Retire(const ClientNativeRoots &roots) noexcept {
  ready_=planned_=false;
  if(!native_)return {true,false,false,"presentation_empty_retired"};
  return ClientPresentationRetire(native_,roots);
}
} // namespace cs::game
