#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "runtime.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <map>
#include <set>
#include <functional>

namespace cs::game {
bool ReadMemory(const void *,void *,std::size_t) noexcept;
namespace {
// E2E 原生读回供 Lua 对照，布局与实际绘制分离。
struct ClientPresentationVein {
  int kind=0,depth=0; float rotation=0,scale=0,factor=0,opacity=0,x=0,y=0;
  std::array<unsigned,3> children{};
};
template <class T> struct NativeFXVector final {T *begin=nullptr,*end=nullptr,*capacity=nullptr;};
struct NativeFXPose final {float x=0,y=0,velocity_x=0,velocity_y=0;bool flip_x=false,flip_y=false;unsigned char padding[2]{};};
struct NativeFX final {
  unsigned id=0;
  KAGE_SmartPointer_ImageBase image{};
  NativeFXVector<NativeFXPose> poses{};
  int count=0;
  unsigned parameters[12]{};
};
struct NativeRay final {float opacity=0,speed=0,phase=0;KAGE_SmartPointer_ImageBase image{};};
struct NativeRayGroup final {float geometry[7]{};NativeFXVector<NativeRay> rays{};unsigned reserved=0;};
struct NativeVein final {
  int kind=0;NativeVein *children[3]{};unsigned unused=0;
  float rotation=0,scale=1,factor=0,opacity=1;int depth=0;float x=0,y=0;unsigned padding=0;
};
static_assert(sizeof(NativeVein)==0x34&&offsetof(NativeVein,rotation)==0x14&&offsetof(NativeVein,x)==0x28);
NativeVein *RoomVeins(Room &room){return reinterpret_cast<NativeVein *>(reinterpret_cast<unsigned char *>(&room._fxLayers)+0xc0);}
NativeFXVector<NativeFXPose> &PoopVector(Room &room){return *reinterpret_cast<NativeFXVector<NativeFXPose> *>(reinterpret_cast<unsigned char *>(&room._fxLayers)+0x44);}
KAGE_SmartPointer_ImageBase &PoopImage(Room &room){return *reinterpret_cast<KAGE_SmartPointer_ImageBase *>(reinterpret_cast<unsigned char *>(&room._fxLayers)+0x3c);}
static_assert(sizeof(NativeFXVector<NativeFX>)==12&&sizeof(NativeFXPose)==0x14&&sizeof(NativeFX)==0x4c);
static_assert(offsetof(NativeFX,image)==4&&offsetof(NativeFX,poses)==0xc&&offsetof(NativeFX,count)==0x18);
static_assert(sizeof(NativeRay)==0x14&&offsetof(NativeRay,image)==0xc&&sizeof(NativeRayGroup)==0x2c&&offsetof(NativeRayGroup,rays)==0x1c);
NativeFXVector<NativeFX> &FXLayersVector(Room &room) noexcept {
  return *reinterpret_cast<NativeFXVector<NativeFX> *>(reinterpret_cast<unsigned char *>(&room._fxLayers)+0x18);
}
NativeFXVector<NativeRayGroup> &RayGroupsVector(Room &room) noexcept {
  return *reinterpret_cast<NativeFXVector<NativeRayGroup> *>(reinterpret_cast<unsigned char *>(&room._fxLayers)+0x24);
}
unsigned FXRayGroups(Room &room) noexcept {
  const auto &v=RayGroupsVector(room);
  const auto begin=reinterpret_cast<std::uintptr_t>(v.begin),end=reinterpret_cast<std::uintptr_t>(v.end),cap=reinterpret_cast<std::uintptr_t>(v.capacity);
  if(end<begin||cap<end||(end-begin)%sizeof(NativeRayGroup)||(cap-begin)%sizeof(NativeRayGroup)||(!begin&&(end||cap)))return UINT_MAX;
  return unsigned((end-begin)/0x2c);
}
int &SelectedRayGroup(Room &room) noexcept {
  return *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(&room._fxLayers)+0x30);
}
bool RayLighting() noexcept {
  return g_Manager&&*reinterpret_cast<const bool *>(reinterpret_cast<const unsigned char *>(g_Manager)+0x2a1fd);
}
unsigned EffectiveCurses(Game &game) {
  // 96D242—264的原生最终darkness条件；两个helper仅查询真实run-effect map。
  const auto add=reinterpret_cast<unsigned (__cdecl *)()>(Base+0x2f55c0)();
  const auto remove=reinterpret_cast<unsigned (__cdecl *)()>(Base+0x2f5750)();
  unsigned temporary=0;std::memcpy(&temporary,reinterpret_cast<const unsigned char *>(&game)+0x25ac8,4);
  return (game._curses|temporary|add)&~remove;
}
bool RayDarkness(Game &game) {return (EffectiveCurses(game)&1u)!=0;}
int &CoopSelection(Game &game,unsigned i) noexcept {
  // HUD::Render 97DC46：每个选择面板只在两个具名索引均非负时绘制，stride20。
  return *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(&game._playerManager)+0xc+(i/2)*20+(i%2)*8);
}
KAGE_SmartPointer_ImageBase &QuestionImage(Minimap &map) noexcept {
  return *reinterpret_cast<KAGE_SmartPointer_ImageBase *>(reinterpret_cast<unsigned char *>(&map)+0x614);
}
bool FXVectorValid(const NativeFXVector<NativeFX> &v,unsigned &count,unsigned maximum=256) noexcept {
  const auto first=reinterpret_cast<std::uintptr_t>(v.begin),last=reinterpret_cast<std::uintptr_t>(v.end),cap=reinterpret_cast<std::uintptr_t>(v.capacity);
  if(last<first||cap<last||(last-first)%sizeof(NativeFX)||(cap-first)%sizeof(NativeFX)||(!first&&(last||cap)))return false;
  count=unsigned((last-first)/sizeof(NativeFX));return count<=maximum&&((!count&&!first)||first);
}
struct NativeStatValue final {float value,delta,previous,threshold;int countdown;};
static_assert(sizeof(NativeStatValue)==20);
NativeStatValue *Stats(StatHUD &hud,unsigned player,unsigned stat) noexcept {
  return reinterpret_cast<NativeStatValue *>(reinterpret_cast<unsigned char *>(&hud)+0x118+player*0xcc+stat*0x14);
}
Entity_Player *&StatPlayer(StatHUD &hud,unsigned index) noexcept {
  return *reinterpret_cast<Entity_Player **>(reinterpret_cast<unsigned char *>(&hud)+0x114+index*0xcc);
}
bool &CombinedProbability(StatHUD &hud) noexcept {
  return *reinterpret_cast<bool *>(reinterpret_cast<unsigned char *>(&hud)+0x2ac);
}
float &DropScale(PlayerHUD &hud) noexcept {
  // PlayerHUD::Update尾部834734之后只供HUD显示的缩放，不是输入查询或Drop动作。
  return *reinterpret_cast<float *>(reinterpret_cast<unsigned char *>(&hud)+0x6bc);
}
void CopyColor(const ColorMod &c,float (&v)[11]) noexcept {
  std::copy(c._tint,c._tint+4,v);std::copy(c._offset,c._offset+3,v+4);std::copy(c._colorize,c._colorize+4,v+7);
}
void CopyColor(const KColor &c,float (&v)[5]) noexcept {
  v[0]=c._red;v[1]=c._green;v[2]=c._blue;v[3]=c._alpha;v[4]=float(c._unk);
}
KAGE_SmartPointer_ImageBase &Surface(std::uintptr_t rva) noexcept {
  return *reinterpret_cast<KAGE_SmartPointer_ImageBase *>(Base+rva);
}
bool FXPresent(Room &room) noexcept {
  const auto &layers=FXLayersVector(room);
  // 96CE20等消费者先检查loaded；Reset未初始化的停用计数不构成可见FX。
  return (room._fxLayers._loaded&&(layers.begin!=layers.end||FXRayGroups(room)||room._fxLayers._poopFxCountdown>0))||RoomVeins(room)[0].children[0];
}
}
namespace {
void LuaNumber(lua_State *L,const char *key,float value){lua_pushnumber(L,value);lua_setfield(L,-2,key);}
void LuaBoolean(lua_State *L,const char *key,bool value){lua_pushboolean(L,value);lua_setfield(L,-2,key);}
template <std::size_t N> void LuaArray(lua_State *L,const char *key,const float (&v)[N]) {
  lua_createtable(L,int(N),0);for(unsigned i=0;i<N;++i){lua_pushnumber(L,v[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,key);
}
// Image格式由正式来源逐属性携带；不以同为shader3就假定相同stride。
bool LuaImageFormat(lua_State *L,const KAGE_Graphics_ImageBase &image){
  if(!image._vertexFormat||image._numVertexAttributes>8)return false;
  LuaNumber(L,"vertex_stride",float(image._vertexFormatStride));lua_createtable(L,image._numVertexAttributes,0);
  for(unsigned i=0;i<image._numVertexAttributes;++i){lua_pushinteger(L,image._vertexFormat[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"attributes");return true;
}
// 复制实际挂载Image身份与采样/格式参数，冷加载不得根据seed或默认图片重选。
bool LuaImage(lua_State *L,const char *key,const KAGE_SmartPointer_ImageBase &pair,bool optional=false) {
  lua_createtable(L,0,8);LuaBoolean(L,"present",pair.image!=nullptr);
  if(!pair.image){lua_setfield(L,-2,key);return optional;}
  auto *image=pair.image;if(!pair.counter||!image->_name)return false;
  char name[1025]{};unsigned size=0;
  for(;size<1024;++size){if(!ReadMemory(image->_name+size,name+size,1))return false;if(!name[size])break;}
  if(!size||size==1024)return false;
  lua_pushlstring(L,name,size);lua_setfield(L,-2,"image");LuaNumber(L,"shader",float(image->_predefinedShader));
  LuaNumber(L,"min_filter",float(image->_minFilterMode));LuaNumber(L,"mag_filter",float(image->_magFilterMode));
  LuaNumber(L,"wrap_s",float(image->_wrapSMode));LuaNumber(L,"wrap_t",float(image->_wrapTMode));
  if(!LuaImageFormat(L,*image))return false;lua_setfield(L,-2,key);return true;
}
bool LuaCacheString(lua_State *L,const char *key,const char *source) {
  char buffer[97]{};unsigned count=0;
  if(source){for(;count<96;++count){if(!ReadMemory(source+count,buffer+count,1))return false;if(!buffer[count])break;}if(count==96)return false;}
  lua_pushlstring(L,buffer,count);lua_setfield(L,-2,key);return true;
}
bool LuaFXImageName(lua_State *L,const char *source) {
  char buffer[1025]{};unsigned count=0;
  if(!source)return false;
  for(;count<1024;++count){if(!ReadMemory(source+count,buffer+count,1))return false;if(!buffer[count])break;}
  if(!count||count==1024)return false;
  lua_pushlstring(L,buffer,count);lua_setfield(L,-2,"image");return true;
}
// 复制原生已选图片和随机后pose；不调用FXLayers Load、Update、GetItemPool或任何RNG。
// 只记录当前复制失败的具名阶段；错误文本随Partial返回，不增加逐帧日志或原生副作用。
thread_local const char *presentation_capture_failure="presentation_capture";
thread_local char presentation_capture_detail[128]{};
bool LuaCaptureFX(lua_State *L,Room &room) {
  // J273 7E7D6C/96BD30清空容器并关闭loaded，却不写选择索引及poop计数。
  // 未加载时不读取这些停用标量；显式适用性与有效的零值/空容器分开。
  if(!room._fxLayers._loaded){
    lua_createtable(L,0,2);LuaBoolean(L,"loaded",false);
    lua_pushliteral(L,"NotApplicable");lua_setfield(L,-2,"applicability");
    lua_setfield(L,-2,"room_fx");return true;
  }
  unsigned count=0;const auto &vector=FXLayersVector(room);const auto rays=FXRayGroups(room);
  // 正式来源预算独立于代表applier的4组/16条限制；不把未采用的真实组隐藏为缺字段。
  presentation_capture_failure="fx_vector_shape";
  if(!FXVectorValid(vector,count,256)||rays>256)return false;
  presentation_capture_failure="fx_ray_selection";
  if(SelectedRayGroup(room)<-1||SelectedRayGroup(room)>=int(rays)){
    std::snprintf(presentation_capture_detail,sizeof(presentation_capture_detail),"fx_ray_selection:loaded=%u:groups=%u:selected=%d",unsigned(room._fxLayers._loaded),rays,SelectedRayGroup(room));
    presentation_capture_failure=presentation_capture_detail;return false;}
  lua_createtable(L,0,10);LuaBoolean(L,"loaded",room._fxLayers._loaded);
  lua_pushliteral(L,"Loaded");lua_setfield(L,-2,"applicability");
  LuaNumber(L,"ray_groups",float(rays));LuaNumber(L,"poop_countdown",float(room._fxLayers._poopFxCountdown));
  LuaNumber(L,"ray_group",float(SelectedRayGroup(room)));LuaBoolean(L,"ray_lighting",RayLighting());LuaBoolean(L,"ray_darkness",RayDarkness(*g_Game));
  lua_createtable(L,int(rays),0);
  for(unsigned i=0;i<rays;++i){const auto &group=RayGroupsVector(room).begin[i];
    presentation_capture_failure="ray_vector_or_budget";
    const auto first=reinterpret_cast<std::uintptr_t>(group.rays.begin),last=reinterpret_cast<std::uintptr_t>(group.rays.end),cap=reinterpret_cast<std::uintptr_t>(group.rays.capacity);
    if(last<first||cap<last||(last-first)%sizeof(NativeRay)||(cap-first)%sizeof(NativeRay)||(!first&&(last||cap)))return false;
    const auto count_rays=unsigned((last-first)/sizeof(NativeRay));if(count_rays>64)return false;
    lua_createtable(L,0,2);LuaArray(L,"geometry",group.geometry);lua_createtable(L,int(count_rays),0);
    for(unsigned j=0;j<count_rays;++j){const auto &ray=group.rays.begin[j];const auto *image=ray.image.image;
      presentation_capture_failure="ray_image_pair_path_or_format";
      if(!image||!ray.image.counter)return false;
      lua_createtable(L,0,9);LuaNumber(L,"opacity",ray.opacity);LuaNumber(L,"speed",ray.speed);LuaNumber(L,"phase",ray.phase);
      if(!LuaFXImageName(L,image->_name))return false;
      LuaNumber(L,"min_filter",float(image->_minFilterMode));LuaNumber(L,"mag_filter",float(image->_magFilterMode));
      LuaNumber(L,"wrap_s",float(image->_wrapSMode));LuaNumber(L,"wrap_t",float(image->_wrapTMode));LuaNumber(L,"shader",float(image->_predefinedShader));
      if(!LuaImageFormat(L,*image))return false;
      lua_rawseti(L,-2,j+1);
    }
    lua_setfield(L,-2,"rays");lua_rawseti(L,-2,i+1);
  }
  lua_setfield(L,-2,"ray_group_members");
  const auto *bytes=reinterpret_cast<const unsigned char *>(&room._fxLayers);
  float average[2]{},bounds[4]{};std::memcpy(average,bytes,sizeof(average));std::memcpy(bounds,bytes+0xb0,sizeof(bounds));
  LuaArray(L,"average",average);LuaArray(L,"bounds",bounds);lua_createtable(L,int(count),0);
  for(unsigned i=0;i<count;++i){
    presentation_capture_failure="fx_pose_vector_or_image_pair";
    const auto &fx=vector.begin[i];auto *image=fx.image.image;
    const auto first=reinterpret_cast<std::uintptr_t>(fx.poses.begin),last=reinterpret_cast<std::uintptr_t>(fx.poses.end),cap=reinterpret_cast<std::uintptr_t>(fx.poses.capacity);
    if(!image||!fx.image.counter||fx.count<0||fx.count>256||last<first||cap<last||(!first&&(last||cap))||
       last-first!=std::size_t(fx.count)*sizeof(NativeFXPose)||(cap-first)%sizeof(NativeFXPose))return false;
    presentation_capture_failure="fx_pass";const auto pass=fx.parameters[4];if(pass<1||pass>6)return false;
    const bool generic=pass==1||pass==2||pass==3||pass==6;unsigned char mask_byte=0;
    if(generic)std::memcpy(&mask_byte,reinterpret_cast<const unsigned char *>(&fx)+0x48,1);
    const bool mask=mask_byte!=0;std::uint32_t color_condition_bits=0,mask_backdrop=0;
    // 96C620先以count>0进入pose循环，才读取+3c/+40；空层没有这些条件值的来源合同。
    // 与未采用的mask/pass分支一样输出规范0，不读取未消费字；采用的IEEE条件位完整复制。
    if(generic&&!mask&&fx.count>0)std::memcpy(&color_condition_bits,reinterpret_cast<const unsigned char *>(&fx)+0x3c,4);
    if(generic&&mask&&fx.count>0)std::memcpy(&mask_backdrop,reinterpret_cast<const unsigned char *>(&fx)+0x40,4);
    // pass1/2/3/6绘制前按实际+44调用BlendMode构造；不从图片、XML或id推测混合模式。
    const unsigned blend_mode=generic?fx.parameters[10]:0;
    presentation_capture_failure="fx_blend_mode";if(blend_mode>2)return false;
    lua_createtable(L,0,12);LuaNumber(L,"id",float(fx.id));LuaNumber(L,"pass",float(pass));
    // 只在这个非算术条件上表达NaN/Inf分类，不把非有限JSON number送进通用数值/pose路径。
    lua_createtable(L,0,3);lua_pushliteral(L,"float32_condition");lua_setfield(L,-2,"type");
    const auto exponent=color_condition_bits&0x7f800000u,mantissa=color_condition_bits&0x007fffffu;
    const char *classification=exponent!=0x7f800000u?"Finite":mantissa?"NaN":color_condition_bits&0x80000000u?"NegativeInfinity":"PositiveInfinity";
    lua_pushstring(L,classification);lua_setfield(L,-2,"classification");lua_pushinteger(L,color_condition_bits);lua_setfield(L,-2,"bits");lua_setfield(L,-2,"color_branch");
    LuaBoolean(L,"mask",mask);lua_pushinteger(L,mask_backdrop);lua_setfield(L,-2,"mask_backdrop");
    LuaNumber(L,"blend_mode",float(blend_mode));
    presentation_capture_failure="fx_image_path_or_format";if(!LuaFXImageName(L,image->_name))return false;
    LuaNumber(L,"min_filter",float(image->_minFilterMode));LuaNumber(L,"mag_filter",float(image->_magFilterMode));
    LuaNumber(L,"wrap_s",float(image->_wrapSMode));LuaNumber(L,"wrap_t",float(image->_wrapTMode));LuaNumber(L,"shader",float(image->_predefinedShader));
    if(!LuaImageFormat(L,*image))return false;
    lua_createtable(L,fx.count,0);
    for(int j=0;j<fx.count;++j){const auto &pose=fx.poses.begin[j];lua_createtable(L,0,6);
      LuaNumber(L,"x",pose.x);LuaNumber(L,"y",pose.y);LuaNumber(L,"velocity_x",pose.velocity_x);LuaNumber(L,"velocity_y",pose.velocity_y);
      LuaBoolean(L,"flip_x",pose.flip_x);LuaBoolean(L,"flip_y",pose.flip_y);lua_rawseti(L,-2,j+1);
    }
    lua_setfield(L,-2,"poses");lua_rawseti(L,-2,i+1);
  }
  lua_setfield(L,-2,"layers");lua_setfield(L,-2,"room_fx");return true;
}
// 只读复制poop随机后态及vein图；图地址仅用于本次主线程去重，出栈仅有有界整数引用。
// node+10/+30既无draw消费合同，也不参与复制；不把未知padding或child裸地址上网。
bool LuaCaptureAux(lua_State *L,Room &room){
  const bool poop_applicable=room._fxLayers._loaded&&room._fxLayers._poopFxCountdown>0;
  const bool veins_present=RoomVeins(room)[0].children[0]!=nullptr;
  presentation_capture_failure="aux_poop_vector";
  const auto &v=PoopVector(room);const auto first=reinterpret_cast<std::uintptr_t>(v.begin),last=reinterpret_cast<std::uintptr_t>(v.end),cap=reinterpret_cast<std::uintptr_t>(v.capacity);
  int count=0;if(poop_applicable){std::memcpy(&count,reinterpret_cast<unsigned char *>(&room._fxLayers)+0x50,4);
    if(count<0||count>256||last<first||cap<last||(last-first)%sizeof(NativeFXPose)||(cap-first)%sizeof(NativeFXPose)||(!first&&(last||cap))||std::size_t(count)>(last-first)/sizeof(NativeFXPose))return false;}
  std::map<const NativeVein *,unsigned> ids;std::set<const NativeVein *> active;std::vector<ClientPresentationVein> nodes;std::array<unsigned,10> roots{};
  std::function<bool(const NativeVein *,unsigned,unsigned &)> visit=[&](const NativeVein *p,unsigned depth,unsigned &id){
    if(!p||depth>32||active.count(p))return false;if(auto found=ids.find(p);found!=ids.end()){id=found->second;return true;}
    if(nodes.size()>=4096)return false;NativeVein n;
    if(!ReadMemory(p,&n.kind,4)||!ReadMemory(reinterpret_cast<const unsigned char *>(p)+4,n.children,12)||
       !ReadMemory(reinterpret_cast<const unsigned char *>(p)+0x14,&n.rotation,28)||n.kind<0||n.kind>1)return false;
    id=unsigned(nodes.size());ids.emplace(p,id);nodes.emplace_back();active.insert(p);
    ClientPresentationVein value;value.kind=n.kind;value.depth=n.depth;value.rotation=n.rotation;value.scale=n.scale;value.factor=n.factor;value.opacity=n.opacity;value.x=n.x;value.y=n.y;
    for(unsigned i=0;i<3;++i)if(n.children[i]){unsigned child=0;if(!visit(n.children[i],depth+1,child))return false;value.children[i]=int(child);}
    nodes[id]=value;active.erase(p);return true;
  };
  // 96EA50只在首根child非空时消费图；Reset后的根标量没有Source值合同。
  presentation_capture_failure="aux_vein_graph";if(veins_present)for(unsigned i=0;i<10;++i)if(!visit(RoomVeins(room)+i,0,roots[i]))return false;
  lua_createtable(L,0,7);LuaBoolean(L,"veins_present",veins_present);LuaBoolean(L,"poop_applicable",poop_applicable);
  if(poop_applicable){
  presentation_capture_failure="aux_poop_image";if(!LuaImage(L,"poop_image",PoopImage(room),true))return false;
  float color[11]{},settings[4]{};CopyColor(room._fxLayers._poopFxColor,color);std::memcpy(settings,reinterpret_cast<unsigned char *>(&room._fxLayers)+0x54,sizeof(settings));
  LuaArray(L,"poop_color",color);LuaArray(L,"poop_settings",settings);lua_createtable(L,count,0);
  for(int i=0;i<count;++i){const auto &s=v.begin[i];lua_createtable(L,0,6);LuaNumber(L,"x",s.x);LuaNumber(L,"y",s.y);
    LuaNumber(L,"velocity_x",s.velocity_x);LuaNumber(L,"velocity_y",s.velocity_y);LuaBoolean(L,"flip_x",s.flip_x);LuaBoolean(L,"flip_y",s.flip_y);lua_rawseti(L,-2,i+1);}
  lua_setfield(L,-2,"poop_poses");}
  if(veins_present){lua_createtable(L,10,0);for(unsigned i=0;i<10;++i){lua_pushinteger(L,roots[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"roots");
  lua_createtable(L,int(nodes.size()),0);for(unsigned i=0;i<nodes.size();++i){const auto &n=nodes[i];lua_createtable(L,0,9);
    LuaNumber(L,"kind",float(n.kind));LuaNumber(L,"depth",float(n.depth));LuaNumber(L,"rotation",n.rotation);LuaNumber(L,"scale",n.scale);
    LuaNumber(L,"factor",n.factor);LuaNumber(L,"opacity",n.opacity);LuaNumber(L,"x",n.x);LuaNumber(L,"y",n.y);lua_createtable(L,3,0);
    for(unsigned j=0;j<3;++j){lua_pushinteger(L,n.children[j]);lua_rawseti(L,-2,j+1);}lua_setfield(L,-2,"children");lua_rawseti(L,-2,i+1);}
  lua_setfield(L,-2,"veins");}lua_setfield(L,-2,"auxiliary");return true;
}
}
int ClientPresentationCaptureLua(void *state) {
  auto *L=static_cast<lua_State *>(state);
  const int top=lua_gettop(L);
  const auto failed=[&](const char *reason){lua_settop(L,top);lua_pushnil(L);lua_pushstring(L,reason);return 2;};
  if(!IsMain()||!g_Game||!g_Game->_room||!g_Game->_room->_descriptor||g_Game->_playerManager._playerList.empty()||g_Game->_room->_backdrop.backdropId>=63){return failed("presentation_roots");}
  auto &room=*g_Game->_room;auto &hud=g_Game->_hud;auto &ph=hud._playerHUD[0];
  if(ph._player!=g_Game->_playerManager._playerList[0])return failed("presentation_hud_owner");
  float floor[11]{},wall[11]{},shadow[5]{},light[5]{};
  CopyColor(room._floorColor,floor);CopyColor(room._wallColor,wall);
  CopyColor(room._fxLayers._fxParams.shadowColor,shadow);CopyColor(room._fxLayers._fxParams.lightColor,light);
  lua_createtable(L,0,40);LuaNumber(L,"version",5);
  const unsigned curses=EffectiveCurses(*g_Game);
  const bool mirror=g_Game->_currentDimensionIdx==1&&(g_Game->_stageType==4||g_Game->_stageType==5)&&(g_Game->_stage==2||(g_Game->_stage==1&&(curses&2)));
  LuaBoolean(L,"mirror",mirror);LuaBoolean(L,"mirror_shader",g_Manager&&*reinterpret_cast<const bool *>(reinterpret_cast<const unsigned char *>(g_Manager)+0x2a204));
  const unsigned parity=g_Game->IsPaused()?0:(g_Manager->_framecount+1)%2;
  LuaNumber(L,"mirror_phase",mirror?float(parity+2*(unsigned(g_Game->_frameCount)%600))/120.0f:0.0f);
  const float backdrop_inputs[4]={room._backdrop.unkVec1.x,room._backdrop.unkVec1.y,room._backdrop.unkVec2.x,room._backdrop.unkVec2.y};
  LuaArray(L,"backdrop_surface_inputs",backdrop_inputs);
  // 当前Backdrop配置为原生构建采用的实际资源集合；不以本地Stage/RoomType重新选图。
  const auto &bc=room._backdrop.configurations[room._backdrop.backdropId];lua_createtable(L,0,2);lua_createtable(L,12,0);
  const std::string *paths[]={&bc.gfx,&bc.waterGfx,&bc.lFloorGfx,&bc.nFloorGfx,&bc.props,&bc.rocks,&bc.pit,&bc.waterPit,&bc.bridge,&bc.door,&bc.holeInWall,&bc.spikes};
  for(unsigned i=0;i<12;++i){lua_pushlstring(L,paths[i]->data(),paths[i]->size());lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"paths");
  const unsigned values[]={bc.backdropId,bc.walls,bc.wallVariants,bc.floors,bc.floorVariants,bc.unk1,bc.waterPitsMode};lua_createtable(L,7,0);
  for(unsigned i=0;i<7;++i){lua_pushinteger(L,values[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"values");lua_setfield(L,-2,"backdrop_config");
  // Backdrop独立装饰流：保留真实构造seed、当前后态及shift；不重抽Authority玩法随机。
  lua_createtable(L,0,5);
  lua_pushinteger(L,lua_Integer(room._backdrop.seed));lua_setfield(L,-2,"construction_seed");
  lua_pushinteger(L,lua_Integer(room._backdrop.rng._seed));lua_setfield(L,-2,"rng_seed");
  lua_pushinteger(L,lua_Integer(room._backdrop.rng._shift1));lua_setfield(L,-2,"shift1");
  lua_pushinteger(L,lua_Integer(room._backdrop.rng._shift2));lua_setfield(L,-2,"shift2");
  lua_pushinteger(L,lua_Integer(room._backdrop.rng._shift3));lua_setfield(L,-2,"shift3");lua_setfield(L,-2,"backdrop_runtime");
  LuaArray(L,"floor_color",floor);LuaArray(L,"wall_color",wall);LuaArray(L,"shadow_color",shadow);LuaArray(L,"light_color",light);
  LuaNumber(L,"shadow_alpha",room._fxLayers._fxParams.shadowAlpha);LuaNumber(L,"water_amount",room._waterAmount);
  auto &fx=room._fxLayers._fxParams;
  const auto kcolor=[L](const char *name,const KColor &c){float values[5]{};CopyColor(c,values);LuaArray(L,name,values);};
  kcolor("water_color",room._waterColor);kcolor("water_multiplier",room._waterColorMultiplier);
  kcolor("water_target",room._waterLerpTargetColor);kcolor("water_target_multiplier",room._waterLerpTargetColorMult);
  kcolor("fx_water_color",fx.waterColor);kcolor("fx_water_multiplier",fx.waterColorMultiplier);kcolor("fx_room_color",fx.roomColor);
  float water_effect[11]{};CopyColor(fx.waterEffectColor,water_effect);LuaArray(L,"water_effect",water_effect);
  const auto modifier=[L](const char *name,const ColorModState &c){const float values[6]={c.r,c.g,c.b,c.a,c.brightness,c.contrast};LuaArray(L,name,values);};
  modifier("current_color",g_Game->_currentColorModifier);modifier("target_color",g_Game->_targetColorModifier);modifier("lerp_color",g_Game->_lerpColorModifier);
  LuaBoolean(L,"color_lerp",g_Game->_shouldLerpColorModState);LuaBoolean(L,"use_water_v2",fx.useWaterV2);
  LuaBoolean(L,"water_enabled",g_Manager&&*reinterpret_cast<const bool *>(reinterpret_cast<const unsigned char *>(g_Manager)+0x2a203));
  LuaBoolean(L,"color_enabled",g_Manager&&*reinterpret_cast<const bool *>(reinterpret_cast<const unsigned char *>(g_Manager)+0x2a205));
  const float current[2]={room._waterCurrent.x,room._waterCurrent.y};LuaArray(L,"water_current",current);
  LuaNumber(L,"fx_brightness",fx.brightness);LuaNumber(L,"fx_contrast",fx.contrast);LuaNumber(L,"lava",room._lavaIntensity);LuaNumber(L,"rain",room._rainIntensity);
  LuaNumber(L,"rain_spawners",float(room._numRainSpawners));LuaNumber(L,"lightning",g_Game->_lightningIntensity);
  LuaNumber(L,"water_lerp_duration",float(room._waterLerpColorDuration));LuaNumber(L,"water_lerp_countdown",float(room._waterLerpColorCountdown));
  LuaNumber(L,"lighting_alpha",room.GetLightingAlpha());LuaBoolean(L,"hud_visible",hud._isVisible);LuaBoolean(L,"room_fx_present",FXPresent(room));
  float darkness=0,darkness_scale=0;std::memcpy(&darkness,reinterpret_cast<unsigned char *>(g_Game)+0x25a90,4);
  std::memcpy(&darkness_scale,reinterpret_cast<unsigned char *>(g_Game)+0x25a94,4);
  LuaNumber(L,"darkness",darkness);LuaNumber(L,"darkness_scale",darkness_scale);
  // 803F24—8041B3的最终混色。两个窄helper仅查询现有楼层/curse/收集品，不触发生成。
  // 只在Authority计算一次；Client不依赖自己的curse、路线或共享随机状态再计算。
  KColor fill=fx.shadowColor,tint=fx.lightColor;const bool pitch=(room._descriptor->Flags&0x200)!=0,special=room._backdrop.backdropId==57;
  if(pitch)fill=KColor(0,0,0,1);
  if(!pitch&&!special){float factor=RayDarkness(*g_Game)?1.0f:darkness*darkness_scale;
    const bool boost=room._roomType==8||room._roomType==29||
      (reinterpret_cast<bool (__thiscall *)(Game *)>(Base+0x348f30)(g_Game)&&g_Game->_currentDimensionIdx==1)||
      reinterpret_cast<bool (__thiscall *)(Room *)>(Base+0x41cb60)(&room);
    if(boost)factor=(std::min)(factor+0.6f,1.0f);
    if(g_Game->_stage==13&&g_Game->_stageType==1&&!(room._descriptor->Flags&4))factor=1;
    factor=(std::max)(factor-g_Game->_lightningIntensity,0.0f);
    tint=KColor(tint._red+(fill._red-tint._red)*factor,tint._green+(fill._green-tint._green)*factor,
      tint._blue+(fill._blue-tint._blue)*factor,tint._alpha);
  }
  float fill_value[5]{},tint_value[5]{};CopyColor(fill,fill_value);CopyColor(tint,tint_value);
  LuaArray(L,"shadow_fill",fill_value);LuaArray(L,"light_tint",tint_value);LuaBoolean(L,"special_light_pass",special);
  LuaBoolean(L,"layer4_enabled",!pitch&&!special);
  // 保留17已有的普通分支事实；最终绘制使用上面的Source混色，不用此事实限制其它Room。
  LuaBoolean(L,"ordinary_light_tint",room._roomType==1&&room._backdrop.backdropId==1&&g_Game->_stage==1&&
      g_Game->_currentDimensionIdx==0&&!(room._descriptor->Flags&0x200)&&!RayDarkness(*g_Game)&&darkness*darkness_scale==0&&g_Game->_lightningIntensity==0);
  if(!LuaImage(L,"shading",Surface(0x7e7370),true))return failed("presentation_shading_image");
  lua_createtable(L,0,9);LuaBoolean(L,"achievement_blocked",g_Manager->AchievementUnlocksDisallowed(false));
  LuaBoolean(L,"seed_blocked",g_Game->_seedEffects.AchievementUnlocksDisallowed());LuaBoolean(L,"custom_run",g_Game->_seedEffects._isCustomRun);
  LuaBoolean(L,"rerun",g_Game->_isRerun);LuaBoolean(L,"debug",g_Game->_isDebug);
  LuaNumber(L,"challenge",float(g_Game->_challenge));LuaNumber(L,"difficulty",float(g_Game->_difficulty));lua_createtable(L,8,0);
  for(unsigned i=0;i<8;++i){lua_pushinteger(L,CoopSelection(*g_Game,i));lua_rawseti(L,-2,i+1);}
  lua_setfield(L,-2,"coop_selection");lua_setfield(L,-2,"hud_context");
  auto &map=g_Game->_minimap;lua_createtable(L,0,7);LuaNumber(L,"effective_curses",float(EffectiveCurses(*g_Game)));
  LuaNumber(L,"state",float(map._state));LuaNumber(L,"small_alpha",map._config[0]._alpha);LuaNumber(L,"big_alpha",map._config[1]._alpha);
  LuaNumber(L,"shake_duration",float(map._shakeDuration));const float map_shake[2]={map._shakeOffset.x,map._shakeOffset.y};LuaArray(L,"shake",map_shake);
  if(!LuaImage(L,"question",QuestionImage(map)))return failed("presentation_minimap_question");lua_setfield(L,-2,"minimap_runtime");
  const float shake[2]={g_Game->_screenShakeOffset.x,g_Game->_screenShakeOffset.y},scroll[2]={room._renderScrollOffset.x,room._renderScrollOffset.y};
  LuaArray(L,"screen_shake",shake);LuaArray(L,"render_scroll",scroll);
  if(!LuaCaptureFX(L,room))return failed(presentation_capture_failure);
  if(!LuaCaptureAux(L,room))return failed(presentation_capture_failure);
  lua_createtable(L,0,5);LuaNumber(L,"red_flash",float(ph._redHeartFlashCountdown));LuaNumber(L,"soul_flash",float(ph._soulHeartFlashCountdown));
  LuaNumber(L,"gold_flash",float(ph._goldHeartFlashCountdown));LuaNumber(L,"drop_scale",DropScale(ph));
  lua_createtable(L,24,0);
  for(unsigned i=0;i<24;++i){
    const auto &h=ph._heart[i];lua_createtable(L,0,7);LuaBoolean(L,"visible",h._visibility);LuaBoolean(L,"eternal",h._eternalHeartVisibility);
    LuaBoolean(L,"golden",h._goldenHeartVisibility);LuaBoolean(L,"fading",h._fadeHeart);LuaNumber(L,"flash",float(h._flashType));
    if(!LuaCacheString(L,"animation",h._spriteAnim)||!LuaCacheString(L,"overlay",h._spriteOverlayAnim))return failed("presentation_heart_cache");
    lua_rawseti(L,-2,i+1);
  }
  lua_setfield(L,-2,"hearts");lua_setfield(L,-2,"player_hud");
  lua_createtable(L,0,2);LuaBoolean(L,"combined_devil_angel",CombinedProbability(hud._statHUD));lua_createtable(L,10,0);
  for(unsigned i=0;i<10;++i){const auto &s=*Stats(hud._statHUD,0,i);lua_createtable(L,0,5);
    LuaNumber(L,"value",s.value);LuaNumber(L,"delta",s.delta);LuaNumber(L,"previous",s.previous);LuaNumber(L,"threshold",s.threshold);LuaNumber(L,"countdown",float(s.countdown));lua_rawseti(L,-2,i+1);}
  lua_setfield(L,-2,"values");lua_setfield(L,-2,"stat_hud");return 1;
}
} // namespace cs::game
