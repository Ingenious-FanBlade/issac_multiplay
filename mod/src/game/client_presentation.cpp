#include "game/client_presentation.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace cs::game {
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
bool ClientPresentation::PrepareTarget(const replica::Value &model,const char *&reason) noexcept {
  try {
    const auto &v=Member(Member(Member(Member(model,"components"),"room"),"state"),"presentation_runtime");
    const auto &transition=Member(Member(Member(Member(Member(Member(model,"components"),"room"),"state"),"transition"),"fields"),"RoomTransition.GetTransitionMode");
    // 既有正式域采用int64封装；本代表只展示稳定房间，不把菜单残留过渡fade当来源。
    if(!transition.IsObject()||!Member(transition,"type").IsString()||std::string(Member(transition,"type").GetString())!="int64"||
       !Member(transition,"value").IsString()||std::string(Member(transition,"value").GetString())!="0")
      throw std::runtime_error("presentation_stable_room_transition_required");
    if(Integer(Member(v,"version"),1,1)!=1)throw std::runtime_error("presentation_version");
    ClientPresentationDescription d;
    Colors(Member(v,"floor_color"),d.floor);Colors(Member(v,"wall_color"),d.wall);
    Colors(Member(v,"shadow_color"),d.shadow);Colors(Member(v,"light_color"),d.light);
    d.shadow_alpha=Number(Member(v,"shadow_alpha"));d.water_amount=Number(Member(v,"water_amount"));
    d.lighting_alpha=Number(Member(v,"lighting_alpha"));
    d.ordinary_light_tint=Boolean(Member(v,"ordinary_light_tint"));
    if(!d.ordinary_light_tint)throw std::runtime_error("presentation_light_tint_adapter_required");
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
    if(d.hud_seed_blocked||d.hud_challenge||d.hud_difficulty>1)throw std::runtime_error("presentation_hud_run_adapter_required");
    d.effective_curses=unsigned(Integer(Member(map,"effective_curses"),0,511));d.minimap_state=Integer(Member(map,"state"),0,4);
    d.minimap_small_alpha=Number(Member(map,"small_alpha"));d.minimap_big_alpha=Number(Member(map,"big_alpha"));
    d.minimap_shake_duration=Integer(Member(map,"shake_duration"),0,100000);
    const auto &map_shake=Member(map,"shake");if(!map_shake.IsArray()||map_shake.Size()!=2)throw std::runtime_error("presentation_minimap_shake");
    for(unsigned i=0;i<2;++i)d.minimap_shake[i]=Number(map_shake[i]);
    // 普通地图依赖18的楼层拓扑；本代表明确采用Lost小图的真实原生问号分支。
    if(!(d.effective_curses&4)||d.minimap_state!=0||d.minimap_big_alpha!=0||d.minimap_small_alpha<0||d.minimap_small_alpha>1)
      throw std::runtime_error("presentation_minimap_lost_branch_required");
    const auto image_description=[&](const replica::Value &source,ClientPresentationFX &out){
      out.image=ImagePath(Member(source,"image"));out.shader=unsigned(Integer(Member(source,"shader"),0,64));
      out.min_filter=unsigned(Integer(Member(source,"min_filter"),0,1));out.mag_filter=unsigned(Integer(Member(source,"mag_filter"),0,1));
      out.wrap_s=unsigned(Integer(Member(source,"wrap_s"),0,2));out.wrap_t=unsigned(Integer(Member(source,"wrap_t"),0,2));
    };
    image_description(Member(map,"question"),d.minimap_question);
    const auto &shading=Member(v,"shading");d.shading_present=Boolean(Member(shading,"present"));
    if(d.shading_present)image_description(shading,d.shading);
    if(d.shadow_alpha<0||d.shadow_alpha>1)throw std::runtime_error("presentation_shadow_alpha");
    if(d.lighting_alpha<0||d.lighting_alpha>1)throw std::runtime_error("presentation_lighting_alpha");
    if(d.water_amount!=0)throw std::runtime_error("presentation_water_requires_native_reflection_adapter");
    const auto &fx=Member(v,"room_fx");
    d.fx_loaded=Boolean(Member(fx,"loaded"));d.fx_ray_groups=unsigned(Integer(Member(fx,"ray_groups"),0,256));
    d.fx_poop_countdown=Integer(Member(fx,"poop_countdown"),-1,100000);
    d.fx_ray_group=Integer(Member(fx,"ray_group"),-1,255);
    d.fx_ray_lighting=Boolean(Member(fx,"ray_lighting"));d.fx_ray_darkness=Boolean(Member(fx,"ray_darkness"));
    const auto &average=Member(fx,"average"),&bounds=Member(fx,"bounds"),&layers=Sequence(Member(fx,"layers"));
    if(!average.IsArray()||average.Size()!=2||!bounds.IsArray()||bounds.Size()!=4||!layers.IsArray()||layers.Size()>8)
      throw std::runtime_error("presentation_fx_budget");
    for(unsigned i=0;i<2;++i)d.fx_average[i]=Number(average[i]);
    for(unsigned i=0;i<4;++i)d.fx_bounds[i]=Number(bounds[i]);
    if(d.fx_poop_countdown>0)throw std::runtime_error("presentation_fx_poop_adapter_required");
    const auto &groups=Sequence(Member(fx,"ray_group_members"));
    if(!groups.IsArray()||groups.Size()!=d.fx_ray_groups||groups.Size()>4)throw std::runtime_error("presentation_fx_ray_group_budget");
    if(d.fx_ray_group>=int(groups.Size()))throw std::runtime_error("presentation_fx_ray_group_index");
    for(const auto &source:groups.GetArray()){
      ClientPresentationRayGroup group;const auto &geometry=Member(source,"geometry"),&rays=Sequence(Member(source,"rays"));
      if(!geometry.IsArray()||geometry.Size()!=7||!rays.IsArray()||rays.Size()>16)throw std::runtime_error("presentation_fx_ray_budget");
      for(unsigned i=0;i<7;++i)group.geometry[i]=Number(geometry[i]);
      if(!rays.Empty()&&(group.geometry[1]<=group.geometry[0]||group.geometry[4]<=0||group.geometry[6]<=0))throw std::runtime_error("presentation_fx_ray_geometry");
      for(const auto &source_ray:rays.GetArray()){
        ClientPresentationRay ray;ray.opacity=Number(Member(source_ray,"opacity"));ray.speed=Number(Member(source_ray,"speed"));ray.phase=Number(Member(source_ray,"phase"));
        if(ray.opacity<0||ray.opacity>1||ray.speed<0||ray.speed>1)throw std::runtime_error("presentation_fx_ray_phase");
        ray.image=ImagePath(Member(source_ray,"image"));
        ray.min_filter=unsigned(Integer(Member(source_ray,"min_filter"),0,1));ray.mag_filter=unsigned(Integer(Member(source_ray,"mag_filter"),0,1));
        ray.wrap_s=unsigned(Integer(Member(source_ray,"wrap_s"),0,2));ray.wrap_t=unsigned(Integer(Member(source_ray,"wrap_t"),0,2));
        ray.shader=unsigned(Integer(Member(source_ray,"shader"),0,64));group.rays.push_back(std::move(ray));
      }
      d.ray_groups.push_back(std::move(group));
    }
    for(const auto &source:layers.GetArray()){
      ClientPresentationFX layer;
      layer.id=unsigned(Integer(Member(source,"id"),0,100000));layer.pass=unsigned(Integer(Member(source,"pass"),0,6));
      if(layer.id!=7||layer.pass!=4)throw std::runtime_error("presentation_fx_layer_adapter_required");
      layer.image=ImagePath(Member(source,"image"));
      layer.min_filter=unsigned(Integer(Member(source,"min_filter"),0,1));layer.mag_filter=unsigned(Integer(Member(source,"mag_filter"),0,1));
      layer.wrap_s=unsigned(Integer(Member(source,"wrap_s"),0,2));layer.wrap_t=unsigned(Integer(Member(source,"wrap_t"),0,2));
      layer.shader=unsigned(Integer(Member(source,"shader"),0,64));
      const auto &poses=Member(source,"poses");
      if(!poses.IsArray()||poses.Empty()||poses.Size()>16)throw std::runtime_error("presentation_fx_pose_budget");
      for(const auto &pose:poses.GetArray()){
        ClientPresentationFXPose p;p.x=Number(Member(pose,"x"));p.y=Number(Member(pose,"y"));
        p.velocity_x=Number(Member(pose,"velocity_x"));p.velocity_y=Number(Member(pose,"velocity_y"));
        p.flip_x=Boolean(Member(pose,"flip_x"));p.flip_y=Boolean(Member(pose,"flip_y"));layer.poses.push_back(p);
      }
      d.fx_layers.push_back(std::move(layer));
    }
    if(d.room_fx_present!=(!d.fx_layers.empty()||!d.ray_groups.empty())||
       ((!d.fx_layers.empty()||!d.ray_groups.empty())&&!d.fx_loaded))
      throw std::runtime_error("presentation_fx_membership_conflict");
    if(d.shadow[4]<0||d.shadow[4]>255||std::floor(d.shadow[4])!=d.shadow[4]||
       d.light[4]<0||d.light[4]>255||std::floor(d.light[4])!=d.light[4])throw std::runtime_error("presentation_kcolor_reserved");
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
