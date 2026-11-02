#include "game/client_resource_model.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace cs::game {
namespace {
using V=replica::Value;
// 缺项和显式未知都拒绝；零、false、空overlay是实际目标值，不能与缺失混同。
const V &At(const V &v,const char *key) {
  if(!v.IsObject()||!v.HasMember(key))throw std::runtime_error(std::string("resource_missing:")+key);
  return v[key];
}
const V &Fields(const V &v){return At(v,"fields");}
std::string String(const V &v){if(!v.IsString())throw std::runtime_error("resource_string");return {v.GetString(),v.GetStringLength()};}
double Number(const V &v) {
  double n=0;
  if(v.IsNumber())n=v.GetDouble();
  else if(v.IsObject()&&v.HasMember("type")&&String(v["type"])=="int64"){
    const auto text=String(At(v,"value"));std::int64_t i=0;
    const auto r=std::from_chars(text.data(),text.data()+text.size(),i);
    if(r.ec!=std::errc{}||r.ptr!=text.data()+text.size())throw std::runtime_error("resource_integer");n=double(i);
  }else throw std::runtime_error("resource_number");
  if(!std::isfinite(n)||std::abs(n)>double((std::numeric_limits<float>::max)()))throw std::runtime_error("resource_nonfinite");
  return n;
}
int Integer(const V &v,int low=0,int high=1000000){const auto n=Number(v);if(n<low||n>high||std::floor(n)!=n)throw std::runtime_error("resource_integer_range");return int(n);}
bool Boolean(const V &v){if(!v.IsBool())throw std::runtime_error("resource_boolean");return v.GetBool();}
ClientPoseVector Vector(const V &v){return {float(Number(At(v,"x"))),float(Number(At(v,"y")))};}
std::vector<const V *> Sequence(const V &v) {
  std::vector<const V *> out;
  if(v.IsArray()){for(const auto &item:v.GetArray())out.push_back(&item);}
  else if(v.IsObject()&&v.HasMember("entries")){
    const auto &entries=At(v,"entries");if(!entries.IsArray()||entries.Size()>256)throw std::runtime_error("resource_table_budget");
    std::vector<std::pair<int,const V *>> ordered;
    for(const auto &entry:entries.GetArray())ordered.push_back({Integer(At(entry,"key"),1,256),&At(entry,"value")});
    std::sort(ordered.begin(),ordered.end(),[](auto a,auto b){return a.first<b.first;});
    for(const auto &item:ordered){if(item.first!=int(out.size()+1))throw std::runtime_error("resource_table_hole");out.push_back(item.second);}
  }else if(v.IsObject()&&v.HasMember("values"))return Sequence(v["values"]);
  else if(v.IsObject()&&v.MemberCount()==0)return out;
  else throw std::runtime_error("resource_sequence");
  if(out.size()>256)throw std::runtime_error("resource_sequence_budget");return out;
}
ClientPoseColor Color(const V &v) {
  ClientPoseColor out;
  const char *tint[]={"R","G","B","A"},*offset[]={"RO","GO","BO"};
  for(unsigned i=0;i<4;++i)out.tint[i]=float(Number(At(v,tint[i])));
  for(unsigned i=0;i<3;++i)out.offset[i]=float(Number(At(v,offset[i])));
  // 官方GetColorize返回R/G/B/A命名表，不能按1起始数组解释。
  const auto &entries=At(At(v,"GetColorize"),"entries");
  if(!entries.IsArray()||entries.Size()!=4)throw std::runtime_error("resource_colorize");
  unsigned mask=0;
  for(const auto &entry:entries.GetArray()){
    const auto key=String(At(entry,"key"));unsigned i=0;while(i<4&&key!=tint[i])++i;
    if(i==4||(mask&(1u<<i)))throw std::runtime_error("resource_colorize_key");
    mask|=1u<<i;out.colorize[i]=float(Number(At(entry,"value")));
  }
  return out;
}
bool Path(const std::string &path) {
  return !path.empty()&&path.size()<=1024&&path.front()!='/'&&path.front()!='\\'&&
      path.find(':')==std::string::npos&&path.find("..") == std::string::npos&&
      path.find_first_of("\r\n\0",0,3)==std::string::npos;
}
void ShaderAbsent(const V &fields,const char *key) {
  const auto &v=At(fields,key);
  if(v.IsBool()){if(v.GetBool())throw std::runtime_error("resource_shader_path_unavailable");return;}
  if(v.IsObject()&&v.HasMember("values")){
    for(const auto *row:Sequence(v["values"]))if(Boolean(At(*row,"value")))throw std::runtime_error("resource_shader_path_unavailable");
    return;
  }
  throw std::runtime_error("resource_shader_state_unavailable");
}
ClientLayerPose Layer(const V &v) {
  const auto &f=Fields(v);ClientLayerPose l;
  l.id=unsigned(Integer(At(f,"LayerState.GetLayerID"),0,63));l.name=String(At(f,"LayerState.GetName"));
  l.sheet=String(At(f,"LayerState.GetSpritesheetPath"));if(!Path(l.sheet))throw std::runtime_error("resource_sheet_path");
  l.size=Vector(At(f,"LayerState.GetSize"));l.position=Vector(At(f,"LayerState.GetPos"));l.crop=Vector(At(f,"LayerState.GetCropOffset"));
  l.rotation=float(Number(At(f,"LayerState.GetRotation")));l.color=Color(At(f,"LayerState.GetColor"));
  l.visible=Boolean(At(f,"LayerState.IsVisible"));l.flip_x=Boolean(At(f,"LayerState.GetFlipX"));l.flip_y=Boolean(At(f,"LayerState.GetFlipY"));
  l.flags=unsigned(Integer(At(f,"LayerState.GetRenderFlags"),0,0x7fffffff));
  l.wrap_s=unsigned(Integer(At(f,"LayerState.GetWrapSMode"),0,2));l.wrap_t=unsigned(Integer(At(f,"LayerState.GetWrapTMode"),0,2));
  const auto &blend=Fields(At(f,"LayerState.GetBlendMode"));
  const char *keys[]={"BlendMode.Equation","BlendMode.RGBSourceFactor","BlendMode.RGBDestinationFactor","BlendMode.AlphaSourceFactor","BlendMode.AlphaDestinationFactor"};
  for(unsigned i=0;i<5;++i)l.blend[i]=unsigned(Integer(At(blend,keys[i]),0,i?14:4));
  ShaderAbsent(f,"LayerState.HasCustomShader");ShaderAbsent(f,"LayerState.HasCustomChampionShader");return l;
}
void Selectors(const V &v,std::vector<int> &out){for(const auto *item:Sequence(v))out.push_back(Integer(*item,-1,1000000));if(out.size()>64)throw std::runtime_error("resource_selector_budget");}
}
bool ClientParseResourcePose(const replica::Value &v,ClientResourcePose &out,std::string &reason) noexcept {
  try {
    if(String(At(v,"type"))!="resource_pose")throw std::runtime_error("resource_pose_type");
    ClientResourcePose p;const auto &f=Fields(v);
    if(v.HasMember("pose_runtime")&&v["pose_runtime"].IsObject()&&v["pose_runtime"].HasMember("loaded"))p.loaded=Boolean(v["pose_runtime"]["loaded"]);
    p.path=String(At(f,"Sprite.GetFilename"));if(p.loaded?!Path(p.path):!p.path.empty())throw std::runtime_error("resource_path_or_unloaded_nonempty");
    p.animation=String(At(f,"Sprite.GetAnimation"));p.frame=Integer(At(f,"Sprite.GetFrame"),-1);
    p.overlay_animation=String(At(f,"Sprite.GetOverlayAnimation"));p.overlay_present=!p.overlay_animation.empty();
    if(p.overlay_present)p.overlay_frame=Integer(At(f,"Sprite.GetOverlayFrame"));
    p.offset=Vector(At(f,"Sprite.Offset"));p.scale=Vector(At(f,"Sprite.Scale"));p.color=Color(At(f,"Sprite.Color"));
    p.rotation=float(Number(At(f,"Sprite.Rotation")));p.playback_speed=float(Number(At(f,"Sprite.PlaybackSpeed")));
    p.flip_x=Boolean(At(f,"Sprite.FlipX"));p.flip_y=Boolean(At(f,"Sprite.FlipY"));
    p.flags=unsigned(Integer(At(f,"Sprite.GetRenderFlags"),0,0x7fffffff));
    ShaderAbsent(f,"Sprite.HasCustomShader");ShaderAbsent(f,"Sprite.HasCustomChampionShader");
    const auto layer_values=Sequence(At(f,"Sprite.GetAllLayers"));
    if((p.loaded&&layer_values.empty())||layer_values.size()>64||(!p.loaded&&!layer_values.empty()))throw std::runtime_error("resource_layer_budget");
    for(const auto *layer:layer_values)p.layers.push_back(Layer(*layer));
    std::sort(p.layers.begin(),p.layers.end(),[](const auto &a,const auto &b){return a.id<b.id;});
    for(unsigned i=0;i<p.layers.size();++i)if(p.layers[i].id!=i)throw std::runtime_error("resource_layer_identity");
    if(v.HasMember("pose_runtime")){
      const auto &runtime=v["pose_runtime"];if(Integer(At(runtime,"version"),1,1)!=1)throw std::runtime_error("resource_phase_version");
      p.overlay_first=Boolean(At(runtime,"overlay_first"));
      const auto &main=At(runtime,"main");p.main_phase=float(Number(At(main,"phase")));
      Selectors(At(main,"layers"),p.main_selectors);Selectors(At(main,"nulls"),p.main_null_selectors);
      if(p.animation.empty()&&(p.main_phase!=0||!p.main_selectors.empty()||!p.main_null_selectors.empty()))throw std::runtime_error("resource_absent_main_phase_conflict");
      if(p.overlay_present){const auto &overlay=At(runtime,"overlay");p.overlay_phase=float(Number(At(overlay,"phase")));Selectors(At(overlay,"layers"),p.overlay_selectors);Selectors(At(overlay,"nulls"),p.overlay_null_selectors);}
      p.exact_phase=true;
    }else throw std::runtime_error("resource_exact_phase_unavailable");
    if(!p.loaded&&(!p.animation.empty()||p.overlay_present||p.main_phase!=0||!p.main_selectors.empty()||!p.main_null_selectors.empty()))throw std::runtime_error("resource_unloaded_animation_conflict");
    out=std::move(p);reason.clear();return true;
  }catch(const std::exception &e){reason=e.what();return false;}catch(...){reason="resource_parse_allocation";return false;}
}
} // namespace cs::game
