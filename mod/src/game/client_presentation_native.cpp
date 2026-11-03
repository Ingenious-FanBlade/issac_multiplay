#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_presentation_native.h"
#include "game/client_grid_native.h"
#include "game/client_objects_native.h"
#include "game/client_entity_draw_native.h"
#include "game/client_entity_budget.h"
#include "game/client_lifetime.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <climits>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <new>
#include <memory>
#include <map>
#include <set>
#include <functional>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
bool ReadMemory(const void *,void *,std::size_t) noexcept;
namespace {
constexpr unsigned kMaximumRenderMembers=kClientEntityMembers;
// J273 96BD30/96DB20：真实vector三指针；FX stride=0x4C，pose stride=0x14。
// 只构造独立pass1..6实际读取的成员，不把配置交给Update/随机消费者。
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
// 96CE20先除以成员数才判断循环，空组必须在外层跳过，不能用总group数量代替成员。
bool HasSelectedRays(const ClientPresentationDescription &d) noexcept {
  return d.fx_ray_group>=0&&unsigned(d.fx_ray_group)<d.ray_groups.size()&&!d.ray_groups[unsigned(d.fx_ray_group)].rays.empty();
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
bool RayContextMatches(Room &room,Game &game,const ClientPresentationDescription &d) {
  // 96CE20的57背景先绘制特殊Effect；该跨域主体缺失由Prepare具名报告。
  // 也不让菜单遗留curse/run-effect/lighting决定Authority已选光线的最终可见条件。
  return !HasSelectedRays(d)||(room._backdrop.backdropId!=57&&RayLighting()==d.fx_ray_lighting&&RayDarkness(game)==d.fx_ray_darkness);
}
bool FXVectorValid(const NativeFXVector<NativeFX> &v,unsigned &count,unsigned maximum=256) noexcept {
  const auto first=reinterpret_cast<std::uintptr_t>(v.begin),last=reinterpret_cast<std::uintptr_t>(v.end),cap=reinterpret_cast<std::uintptr_t>(v.capacity);
  if(last<first||cap<last||(last-first)%sizeof(NativeFX)||(cap-first)%sizeof(NativeFX)||(!first&&(last||cap)))return false;
  count=unsigned((last-first)/sizeof(NativeFX));return count<=maximum&&((!count&&!first)||first);
}
void *NativeFXAllocate(std::size_t bytes) {
  return reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(bytes);
}
// 与原生vector析构使用同一allocator；本方STL的operator delete不接管这些块。
void NativeFXFree(void *pointer) noexcept {
  if(pointer)reinterpret_cast<void (__cdecl *)(void *)>(Base+0x68717b)(pointer);
}
// 所有分配都在候选owner内登记；原生Room仅借用图根/数组，不拥有本方节点的释放责任。
struct NativeAux final {
  NativeVein *nodes=nullptr;unsigned node_count=0;
  std::array<unsigned,10> roots{};
  NativeFXPose *poop=nullptr;unsigned poop_count=0;
  KAGE_SmartPointer_ImageBase image{};
  ~NativeAux(){NativeFXFree(nodes);NativeFXFree(poop);}
};
void DestroyFXRecords(NativeFX *records,unsigned count) {
  if(!records)return;
  // 原生96FFD0：ECX=begin、EDX=end、ret无栈参数；内部析构pose vector再Decr Image。
  reinterpret_cast<void (__fastcall *)(NativeFX *,NativeFX *)>(Base+0x56ffd0)(records,records+count);
  NativeFXFree(records);
}
void DestroyRayGroups(NativeRayGroup *groups,unsigned count) {
  if(!groups)return;
  // 96BD30对每个group+1C调用此真实vector dtor，内部970060析构ray的Image strong。
  for(unsigned i=0;i<count;++i)
    reinterpret_cast<void (__thiscall *)(NativeFXVector<NativeRay> *)>(Base+0x56f760)(&groups[i].rays);
  NativeFXFree(groups);
}
// J273 StatHUD::RecomputeStats 83CFD0/83DC60：每项为当前值、差值、前值、阈值、倒计时。
// 这些是服务器已产生的显示缓存；Client绝不调用恶魔/天使/星象馆概率生产者。
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
bool Current(const ClientNativeRoots &r) noexcept {
  auto *game=static_cast<Game *>(r.game);auto *room=static_cast<Room *>(r.room);
  return IsMain()&&game&&room&&game==g_Game&&game->_room==room&&
      r.descriptor==room->_descriptor&&room->_descriptor&&room->_descriptor->Data&&
      r.camera&&r.camera==room->_Camera;
}
// 渲染异常/早退也保持活跃资格到最后一个原生pass和renderer恢复离栈之后。
class ActiveDraw final {
public:
  explicit ActiveDraw(unsigned &active) noexcept:active_(active){++active_;}
  ~ActiveDraw(){--active_;}
private:unsigned &active_;
};
bool GraphicsContext() noexcept {
  void *dc=nullptr,*context=nullptr;
  return ClientRendererHandles(dc,context)&&dc&&context&&dc==wglGetCurrentDC()&&context==wglGetCurrentContext();
}
ColorMod Color(const float (&v)[11]) noexcept {
  return ColorMod(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10]);
}
KColor Color(const float (&v)[5]) noexcept {return KColor(v[0],v[1],v[2],v[3],int(v[4]));}
void CopyColor(const ColorMod &c,float (&v)[11]) noexcept {
  std::copy(c._tint,c._tint+4,v);std::copy(c._offset,c._offset+3,v+4);std::copy(c._colorize,c._colorize+4,v+7);
}
void CopyColor(const KColor &c,float (&v)[5]) noexcept {
  v[0]=c._red;v[1]=c._green;v[2]=c._blue;v[3]=c._alpha;v[4]=float(c._unk);
}
KAGE_SmartPointer_ImageBase &Surface(std::uintptr_t rva) noexcept {
  return *reinterpret_cast<KAGE_SmartPointer_ImageBase *>(Base+rva);
}
KAGE_Graphics_Shader *ImageShader() noexcept {
  // J273 RenderShadows 415541和Backdrop原生默认pass使用的已构造Image shader对象。
  return reinterpret_cast<KAGE_Graphics_Shader *>(Base+0x7ab44c);
}
bool PassResourcesReady(const ClientPresentationDescription &description) noexcept {
  const auto surface_format=[](const KAGE_SmartPointer_ImageBase &p){return p.image&&p.counter&&p.image->_predefinedShader==3&&
      p.image->_numVertexAttributes==8&&p.image->_vertexFormatStride==88;};
  if(!surface_format(Surface(0x7e7360))||!ImageShader()->_initialized)return false;
  if(description.mirror){const auto &pair=Surface(0x7e7340);auto *image=pair.image;
    if(!image||!pair.counter||image->_vertexFormatStride!=(description.mirror_shader?48:88)||image->GetWidth()<=0||image->GetHeight()<=0)return false;}
  if(description.water_amount>0&&description.water_enabled){const auto &water=Surface(0x7e7388);
    if(!water.image||!water.counter||!water.image->_vertexFormat||water.image->_vertexFormatStride<72)return false;}
  if(description.color_enabled){const auto &color=Surface(0x7e72e4);
    if(!color.image||!color.counter||color.image->_vertexFormatStride!=48||!reinterpret_cast<KAGE_Graphics_Shader *>(Base+0x7ab2ec)->_initialized)return false;}
  return (description.lighting_alpha<=0&&description.fx_layers.empty())||(surface_format(Surface(0x7e7398))&&
      *reinterpret_cast<KAGE_Graphics_ImageBase **>(Base+0x7e743c));
}
ANM2 &RoomLight(Room &room,bool spot) noexcept {
  // Room构造/析构真实拥有的连续两ANM；Init 7E8E34/7E9009分别窄Load，不另建Sprite。
  return *reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&room)+(spot?0x6f54:0x6e40));
}
Vector &RoomSurfaceOrigin(Room &room) noexcept {
  return *reinterpret_cast<Vector *>(reinterpret_cast<unsigned char *>(&room)+0x11fc);
}
void MaintainRoomSurfaceOrigin(Room &room) noexcept {
  // 原生703F05—46的纯视口派生标量；不用完整703E40，不重写目标scroll/Camera历史。
  RoomSurfaceOrigin(room)=Vector((g_WIDTH-338.0f)*0.5f-52.0f,(g_HEIGHT-182.0f)*0.5f-52.0f);
}
bool LoadRoomLight(Room &room,bool spot) {
  auto &sprite=RoomLight(room,spot);std::string path=spot?"gfx/SpotLight.anm2":"gfx/LightGradient.anm2";
  sprite.Load(path,true);
  if(!sprite._loaded||sprite._filename!=path||sprite._layerCount<1||sprite._layerCount>16||!sprite._layerState)return false;
  auto *layer=sprite.GetLayer(0);
  if(!layer||!layer->_spriteSheetLoaded||!layer->_spriteSheet.image||!layer->_spriteSheet.counter)return false;
  layer->_minFilterMode=layer->_magFilterMode=1;layer->_blendMode=BlendMode();
  if(sprite._animDefaultName.empty()||!sprite.SetAnimation(sprite._animDefaultName.c_str(),true))return false;
  auto animation=sprite._animDefaultName;sprite.SetFrame(&animation,0);
  return sprite._animState._animData&&sprite._animState._layerFrames;
}
unsigned RenderZ(Entity *entity) noexcept {
  using GetZ=unsigned (__thiscall *)(Entity *);
  auto **table=*reinterpret_cast<void ***>(entity);
  return reinterpret_cast<GetZ>(table[13])(entity);
}
// 列表中的每个非Player必须属于正式实例表；完整源成员不会被过滤成代表投影。
bool Members(Room &room,Game &game,std::array<Entity *,kMaximumRenderMembers> &out,unsigned &count,const char *&reason) {
  const auto &list=room._entityList._renderEL;
  count=list._size;
  if(count>kMaximumRenderMembers||count>list._capacity||(count&&!list._data)){
    reason="presentation_render_list_budget";return false;
  }
  if(game._playerManager._playerList.size()!=1){reason="presentation_requires_one_ordinary_player";return false;}
  if(!room._gridWidth||!room._gridHeight||room._gridWidth>28||room._gridHeight>16||
     room._gridWidth*room._gridHeight>448){reason="presentation_room_geometry";return false;}
  auto *player=game._playerManager._playerList[0];
  if(!player||player->_playerType!=0){reason="presentation_player_shape";return false;}
  unsigned players=0;
  std::set<Entity *> seen;
  for(unsigned i=0;i<count;++i){
    auto *entity=list._data[i];
    if(!entity||!entity->_valid){reason="presentation_invalid_member";return false;}
    if(entity->_type==1){if(entity!=player){reason="presentation_foreign_player";return false;}++players;}
    else if(const auto *d=ClientNativeObjectDrawDescription(entity);!(d&&d->full_entity&&d->category>0&&d->category<13)&&
        (entity->_type!=1000||entity->_variant!=57||entity->_subtype>1)){
      // 记录真实多出的native成员；不把未知Entity过滤后冒认完整展示。
      static thread_local char detail[96];std::snprintf(detail,sizeof(detail),"presentation_entity_renderer_unsupported:%u:%u:%u",entity->_type,entity->_variant,entity->_subtype);
      reason=detail;return false;}
    if(!seen.insert(entity).second){reason="presentation_duplicate_member";return false;}
    out[i]=entity;
  }
  if(players!=1){reason="presentation_player_render_membership";return false;}
  if(!ClientGridMembersMatch(&room)){reason="presentation_grid_owner_or_membership";return false;}
  return true;
}
// 以当前原生GetRenderZ全排，保留原索引作为tie次序；不使用4150C0的共享scratch、
// 空表data[0]读取或渐进预算，也不伪造Game帧奇偶。容量固定并由上方完整预检保证。
void FullSort(Room &room,std::array<Entity *,kMaximumRenderMembers> &entities,unsigned count) noexcept {
  struct Key final {Entity *entity;unsigned z,index;};
  std::array<Key,kMaximumRenderMembers> keys{};
  for(unsigned i=0;i<count;++i){const auto *d=ClientNativeObjectDrawDescription(entities[i]);keys[i]={entities[i],RenderZ(entities[i]),d&&d->full_entity?d->authority_index:entities[i]->_index};}
  std::sort(keys.begin(),keys.begin()+count,[](const Key &a,const Key &b){return a.z!=b.z?a.z<b.z:a.index<b.index;});
  for(unsigned i=0;i<count;++i)room._entityList._renderEL._data[i]=entities[i]=keys[i].entity;
}
// 9E4140消费CPU队列后重置BA4714，却不清当前FBO的depth。相同target内的flush
// 必须保留连续递减的z，否则世界合成quad与HUD第一张Frame同深度而遮掉它。
void FlushSameTarget(KAGE_Graphics_Manager &manager) {
  auto &depth=*reinterpret_cast<float *>(Base+0x7a4714);const float next=depth;
  manager.PresentWithoutSwap();depth=next;
}
// 每个pass保存renderer显式状态；所有CPU raw提交在改变目标/释放借用前消费。
class RenderState final {
public:
  RenderState():manager_(*__ptr_g_KAGE_Graphics_Manager),blend_(manager_._blendMode),shader_(manager_._shader) {
    FlushSameTarget(manager_);Rendering::PushCurrentRenderTarget();active_=true;
  }
  ~RenderState(){if(active_){
    // 原版Present只有在本方刚设置的非空offscreen目标上才消费并清共享depth；绝不对外层main调用。
    if(offscreen_&&manager_.currentRenderTarget==offscreen_)manager_.Present();else FlushSameTarget(manager_);
    Rendering::RestorePreviousRenderTarget();manager_._blendMode=blend_;manager_._shader=shader_;
  }}
  void Target(KAGE_Graphics_ImageBase *target,bool screen) {
    FlushSameTarget(manager_);offscreen_=target;manager_.SetCurrentRenderTarget(target,screen);manager_.Clear();
  }
  bool FinishOffscreen(KAGE_Graphics_ImageBase *expected){
    if(!expected||offscreen_!=expected||manager_.currentRenderTarget!=expected)return false;
    // 803E6C的真实Present：flush后清同一FBO共享depth，再还原目标；WithoutSwap不会清depth。
    manager_.Present();offscreen_=nullptr;Rendering::RestorePreviousRenderTarget();Rendering::PushCurrentRenderTarget();return true;
  }
private:
  KAGE_Graphics_Manager &manager_;BlendMode blend_;KAGE_Graphics_Shader *shader_;KAGE_Graphics_ImageBase *offscreen_=nullptr;bool active_=false;
};
void Composite(KAGE_Graphics_ImageBase *image,const KColor &color,const BlendMode &blend) {
  // 803FB4/FC6使用实际solid-image四角色入口与basic-color shader，不走纹理UV或ColorOffset属性。
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;graphics._shader=reinterpret_cast<KAGE_Graphics_Shader *>(Base+0x7ae1d8);graphics._blendMode=blend;
  DestinationQuad destination{};destination._topLeft=Vector(0,0);destination._topRight=Vector(g_OrthographicWidth,0);
  destination._bottomLeft=Vector(0,g_OrthographicHeight);destination._bottomRight=Vector(g_OrthographicWidth,g_OrthographicHeight);
  auto **table=*reinterpret_cast<void ***>(image);
  reinterpret_cast<float *(__thiscall *)(KAGE_Graphics_ImageBase *,const DestinationQuad *,const KColor *,const KColor *,const KColor *,const KColor *)>(table[6])(
      image,&destination,&color,&color,&color,&color);
}
// 原版8059AB/80672B的surface入口自行按实际像素尺寸建立source UV，不能采整张padded纹理。
// 随后的67E7B0是ColorMod顶点补全而非提交队列：shader3还读取stride88后半部属性。
void FillSurfaceVertices(KAGE_SmartPointer_ImageBase &pair,float *vertices) {
  auto *image=pair.image;
  if(vertices&&image->_predefinedShader==3&&image->_vertexFormatStride==88){
    struct PairArgument final {KAGE_Graphics_ImageBase *image;KAGE_ReferenceCount_ImageBase *counter;};
    static_assert(sizeof(PairArgument)==8);
    pair.counter->IncrementReference();ColorMod white;
    reinterpret_cast<void (__thiscall *)(ColorMod *,float *,PairArgument)>(Base+0x27e7b0)(&white,vertices,{pair.image,pair.counter});
  }
}
void ImageAt(KAGE_SmartPointer_ImageBase &pair,const Vector &position,const KColor &color) {
  using RenderAt=float *(__thiscall *)(KAGE_Graphics_ImageBase *,const Vector *,const KColor *);
  auto *image=pair.image;auto **table=*reinterpret_cast<void ***>(image);
  float *vertices=reinterpret_cast<RenderAt>(table[9])(image,&position,&color);
  FillSurfaceVertices(pair,vertices);
}
void SurfaceComposite(KAGE_SmartPointer_ImageBase &pair,const KColor &color,const BlendMode &blend,bool screen=false) {
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;graphics._shader=ImageShader();graphics._blendMode=blend;
  if(!screen){ImageAt(pair,Vector(0,0),color);return;}
  // 806720/72B采用dest-only槽+1C，源区仍由Image按实际像素尺寸产生。
  DestinationQuad quad{};quad._topRight=Vector(g_WIDTH,0);quad._bottomLeft=Vector(0,g_HEIGHT);quad._bottomRight=Vector(g_WIDTH,g_HEIGHT);
  auto **table=*reinterpret_cast<void ***>(pair.image);
  auto *vertices=reinterpret_cast<float *(__thiscall *)(KAGE_Graphics_ImageBase *,const DestinationQuad *,const KColor *)>(table[7])(pair.image,&quad,&color);
  FillSurfaceVertices(pair,vertices);
}
#ifdef ISAACCS_E2E
// 只在本次显式E2E的首个负色彩场景、既有offscreen排空后读取四份GPU缓存，不补绘制或Present。
// 官方getter内部临时绑定图像；只在有真实旧target/mode可恢复时调用，并核对返回的FBO/viewport。
bool SampleSceneSurfaces(Room &room){
  static bool sampled=false;if(sampled||Settings.world_e2e_token.empty())return true;
  if(!__ptr_g_KAGE_Graphics_Manager||!__ptr_g_KAGE_Graphics_Manager->currentRenderTarget)return false;sampled=true;
  GLint framebuffer=0,viewport[4]{},after_framebuffer=0,after_viewport[4]{};
  glGetIntegerv(0x8ca6,&framebuffer);glGetIntegerv(GL_VIEWPORT,viewport);
  const std::pair<const char *,KAGE_Graphics_ImageBase *> images[]={
    {"floor",room._backdrop.floorBuffer.image},{"walls",room._backdrop.wallsBuffer.image},
    {"color",Surface(0x7e72e4).image},{"light",Surface(0x7e7398).image}};
  for(const auto &entry:images){auto *image=entry.second;if(!image)return false;
    const int w=image->GetWidth(),h=image->GetHeight();if(w<=0||h<=0)return false;unsigned char rgba[4]{};
    image->GetTexelRegion(w/2,h/2,1,1,rgba);
    Trace("client_e2e_surface","name=%s size=%d,%d stride=%u shader=%u rgba=%u,%u,%u,%u",entry.first,w,h,
        image->_vertexFormatStride,image->_predefinedShader,rgba[0],rgba[1],rgba[2],rgba[3]);
  }
  glGetIntegerv(0x8ca6,&after_framebuffer);glGetIntegerv(GL_VIEWPORT,after_viewport);
  return framebuffer==after_framebuffer&&std::memcmp(viewport,after_viewport,sizeof(viewport))==0;
}
#endif
// GameRender的窄ColorModifier合成：直接供应Source最终6个值，不运行其更新/随机/完整GameRender。
bool ColorComposite(const ClientPresentationDescription &d){
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;auto &pair=Surface(0x7e72e4);auto *image=pair.image;
  if(!image||image->_vertexFormatStride!=48)return false;graphics._shader=reinterpret_cast<KAGE_Graphics_Shader *>(Base+0x7ab2ec);graphics._blendMode=BlendMode(0,1,0,1,0);
  SourceQuad source{};source._topRight=Vector(1,0);source._bottomLeft=Vector(0,1);source._bottomRight=Vector(1,1);
  source._coordinateSpace=SourceQuad::NORMALIZED_UV;
  DestinationQuad destination{};destination._topRight=Vector(g_WIDTH,0);destination._bottomLeft=Vector(0,g_HEIGHT);destination._bottomRight=Vector(g_WIDTH,g_HEIGHT);
  const KColor tint(d.current_color[0],d.current_color[1],d.current_color[2],1);auto **table=*reinterpret_cast<void ***>(image);
  auto *vertices=reinterpret_cast<float *(__thiscall *)(KAGE_Graphics_ImageBase *,const SourceQuad *,const DestinationQuad *,const KColor *)>(table[3])(image,&source,&destination,&tint);
  if(!vertices)return false;for(unsigned i=0;i<4;++i){vertices[i*12+9]=d.current_color[3];vertices[i*12+10]=d.current_color[4];vertices[i*12+11]=d.current_color[5];}return true;
}
// 807961 的 SourceQuad 水平反转；镜像 shader 的 48 字节顶点在 807A..807D 逐点写七个参数。
// 参数仅依赖本地 surface/视口、已安装边界/scroll/震动及 Authority phase，不调用 RoomRender 大链。
bool MirrorComposite(Room &room,const ClientPresentationDescription &d){
  auto &pair=Surface(0x7e7340);auto *image=pair.image;if(!image||!pair.counter)return false;
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;graphics._blendMode=BlendMode(0,1,0,1,0);
  graphics._shader=d.mirror_shader?reinterpret_cast<KAGE_Graphics_Shader *>(Base+0x7ab294):ImageShader();
  SourceQuad source{};source._coordinateSpace=SourceQuad::NORMALIZED_UV;
  source._topLeft=Vector(1,0);source._topRight=Vector(0,0);source._bottomLeft=Vector(1,1);source._bottomRight=Vector(0,1);
  DestinationQuad dest{};dest._topRight=Vector(g_WIDTH,0);dest._bottomLeft=Vector(0,g_HEIGHT);dest._bottomRight=Vector(g_WIDTH,g_HEIGHT);
  if(!d.mirror_shader){auto *vertices=image->Render(source,dest,KColor(1,1,1,1));if(!vertices)return false;FillSurfaceVertices(pair,vertices);return true;}
  if(image->_vertexFormatStride!=48||image->_numVertexAttributes!=5)return false;
  const float padded_w=float(image->GetPaddedWidth()),padded_h=float(image->GetPaddedHeight());
  const float pixel=*reinterpret_cast<const float *>(Base+0x7883e8),scale=pixel* *reinterpret_cast<const float *>(Base+0x788730);
  if(padded_w<=0||padded_h<=0||pixel<=0||scale<=0)return false;
  float bounds[4]{};std::memcpy(bounds,reinterpret_cast<unsigned char *>(&room)+0x14,16);
  const float px=std::floor(((bounds[0]-60)*0.65f+(g_WIDTH-338)*0.5f)*scale+0.5f)/scale;
  const float py=std::floor(((bounds[1]-140)*0.65f+(g_HEIGHT-182)*0.5f)*scale+0.5f)/scale;
  const float values[]={float(image->GetWidth())/padded_w,float(image->GetHeight())/padded_h,padded_w/(pixel*1024),padded_h/(pixel*1024),
    (px+room._renderScrollOffset.x+d.screen_shake[0])*pixel/padded_w,(py+room._renderScrollOffset.y+d.screen_shake[1])*pixel/padded_h,d.mirror_phase};
  for(float value:values)if(!std::isfinite(value))return false;
  auto *vertices=image->Render(source,dest,KColor(1,1,1,1));if(!vertices)return false;
  for(unsigned vertex=0;vertex<4;++vertex)std::copy(std::begin(values),std::end(values),vertices+vertex*12+5);return true;
}
// 首draw只读诊断实际GL窗口与待消费顶点，区分FBO投影、blit及后续本体pass；不修正坐标。
void ShadowDiagnostic(const char *phase,bool batches=false) noexcept {
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;GLint viewport[4]{},framebuffer=0,program=0;
  glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(0x8ca6,&framebuffer);glGetIntegerv(0x8b8d,&program);
  Trace("client_shadow_gl","phase=%s viewport=%d,%d,%d,%d fbo=%d program=%d target=%p screen=%u light=%u reflection=%u glitch=%u ortho=%.3f,%.3f",
      phase,viewport[0],viewport[1],viewport[2],viewport[3],framebuffer,program,graphics.currentRenderTarget,graphics.renderTargetUsesScreenSize,
      g_ANM2_LightRendering,g_ANM2_ReflectionRendering,g_ANM2_GlitchRendering,g_OrthographicWidth,g_OrthographicHeight);
  if(!batches||!__ptr_g_KAGE_Graphics_ImageManager)return;
  unsigned emitted=0;
  const auto batch_values=[&](KAGE_Graphics_ImageBase *image,KAGE_Graphics_RenderBatch *batch){
    if(!image||!batch||emitted>=8)return;
    auto &v=batch->_vertexBuffer;if(!v.container||v.activeBufferIdx||v.elementSize<12||v.elementSize>512)return;
    const auto &buffer=v.container[0];
    if(!buffer.data||buffer.processed>=buffer.size||buffer.size>buffer.capacity||buffer.size>4096)return;
    float first[3]{},last[3]{};
    const auto *data=static_cast<const unsigned char *>(buffer.data);
    if(!ReadMemory(data+std::size_t(buffer.processed)*v.elementSize,first,12)||
       !ReadMemory(data+std::size_t(buffer.size-1)*v.elementSize,last,12))return;
    ++emitted;
    Trace("client_shadow_batch","phase=%s image=%s batch=%p shader=%p stride=%u vertices=%u processed=%u first=%.3f,%.3f,%.3f last=%.3f,%.3f,%.3f",
        phase,image->_name?image->_name:"",batch,batch->shader,v.elementSize,buffer.size,buffer.processed,
        first[0],first[1],first[2],last[0],last[1],last[2]);
  };
  auto &images=*__ptr_g_KAGE_Graphics_ImageManager;
  unsigned scanned_images=0,scanned_batches=0;
  for(auto *image:images._frameImages){
    if(scanned_images++>=16||emitted>=8||scanned_batches>=16)break;
    if(image)for(auto *batch:image->_renderBatches){
      if(scanned_batches++>=16||emitted>=8)break;batch_values(image,batch);
    }
  }
  for(const auto &pending:images._transparentBatches){
    if(scanned_batches++>=16||emitted>=8)break;batch_values(pending.second,pending.first);
  }
}
bool FXPresent(Room &room) noexcept {
  const auto &layers=FXLayersVector(room);
  // 96CE20等消费者先检查loaded；Reset未初始化的停用计数不构成可见FX。
  return (room._fxLayers._loaded&&(layers.begin!=layers.end||FXRayGroups(room)||room._fxLayers._poopFxCountdown>0))||RoomVeins(room)[0].children[0];
}
void DrawEntity(Entity *entity,Vector &offset) {
  // 各类真实虚表Render；统一Hook保护也覆盖水面415350的间接调用，不再错误cast为Effect。
  auto **table=*reinterpret_cast<void ***>(entity);
  reinterpret_cast<void (__thiscall *)(Entity *,Vector *)>(table[5])(entity,&offset);
}
// 415350是原生按Z绘制的窄consumer；mode3/4/5依据已提供water clip flags分层，不能直接全员再画一次。
void DrawBand(Room &room,unsigned limit,Vector &offset,int mode,bool reset=false){
  auto *bytes=reinterpret_cast<unsigned char *>(&room._entityList);std::memcpy(bytes+0xa4,&mode,4);
  if(reset){const unsigned zero=0;std::memcpy(bytes+0xa0,&zero,4);}
  reinterpret_cast<void (__thiscall *)(EntityList *,unsigned,Vector *)>(Base+0x15350)(&room._entityList,limit,&offset);
}
}
// 本owner拥有服务器心形字符串，不拥有Game/Room/HUD/Entity或原生全局渲染surface。
// surface由真实Manager owner创建并在原生图形清理阶段释放，不在本方析构中DecrRef。
struct ClientNativePresentation final {
  ClientPresentationDescription description{};
  Game *game=nullptr;Room *room=nullptr;
  unsigned active=0;
  // 拓扑和排序每个T只核定一次；稳定draw只做线性寿命见证，换T立即重建。
  std::array<Entity *,kMaximumRenderMembers> entities{};
  Entity **render_storage=nullptr;
  unsigned entity_count=0;
  NativeFX *fx_installed=nullptr,*fx_candidate=nullptr;
  unsigned fx_installed_count=0,fx_candidate_count=0;
  NativeRayGroup *rays_installed=nullptr,*rays_candidate=nullptr;
  unsigned rays_installed_count=0,rays_candidate_count=0;
  Vector previous_screen_shake{};
  ColorModState previous_current_color{},previous_target_color{},previous_lerp_color{};
  float previous_lightning=0,previous_darkness=0,previous_darkness_scale=0;
  bool environment_snapshot=false,previous_color_lerp=false;
  KAGE_SmartPointer_ImageBase shading{};
  std::unique_ptr<NativeAux> auxiliary,auxiliary_candidate;
  // MinimapConfig+114为Image strong，明确排除；+11C..15F只有原生几何/alpha数值缓存。
  std::array<unsigned char,0x18> previous_map_header{};
  std::array<std::array<unsigned char,0x44>,2> previous_map_config{};
  std::array<unsigned char,12> previous_map_shake{};
  bool map_snapshot=false;
  bool mirror_format_snapshot=false,previous_mirror_extended=false;
  bool screen_shake_captured=false,minimap_loaded=false;
  bool written=false,ready=false,retired=false,uncertain=false,diagnostic_drawn=false,fx_owner_reported=false;
  unsigned minimap_diagnostic=0;
};
ClientNativePresentation *ClientPresentationCreate() noexcept {return IsMain()?new(std::nothrow) ClientNativePresentation:nullptr;}
void ClientPresentationDestroy(ClientNativePresentation *p) noexcept {
  if(p&&IsMain()&&!p->active&&(!p->written||p->retired))delete p;
}
namespace {
ClientNativePresentation *hud_draw=nullptr;
// 只在同步HUD draw期间供应Authority显示条件；离栈恢复本地菜单上下文与原生查询。
class HudDraw final {
public:
  explicit HudDraw(ClientNativePresentation &p):game_(*p.game),previous_(hud_draw),
      curses_(game_._curses),challenge_(game_._challenge),difficulty_(game_._difficulty),
      custom_(game_._seedEffects._isCustomRun),rerun_(game_._isRerun),debug_(game_._isDebug),transition_(game_._roomTransition._mode) {
    std::memcpy(&temporary_,reinterpret_cast<unsigned char *>(&game_)+0x25ac8,4);
    for(unsigned i=0;i<8;++i){selection_[i]=CoopSelection(game_,i);CoopSelection(game_,i)=p.description.coop_selection[i];}
    game_._curses=p.description.effective_curses;unsigned zero=0;std::memcpy(reinterpret_cast<unsigned char *>(&game_)+0x25ac8,&zero,4);
    game_._challenge=p.description.hud_challenge;game_._difficulty=p.description.hud_difficulty;
    game_._seedEffects._isCustomRun=p.description.hud_custom_run;game_._isRerun=p.description.hud_rerun;game_._isDebug=p.description.hud_debug;
    // 966802以827350的RoomTransition fade乘minimap alpha；正式目标已限定稳定mode0。
    game_._roomTransition._mode=0;
    hud_draw=&p;
  }
  ~HudDraw(){
    hud_draw=previous_;game_._curses=curses_;game_._challenge=challenge_;game_._difficulty=difficulty_;
    game_._seedEffects._isCustomRun=custom_;game_._isRerun=rerun_;game_._isDebug=debug_;
    game_._roomTransition._mode=transition_;
    std::memcpy(reinterpret_cast<unsigned char *>(&game_)+0x25ac8,&temporary_,4);
    for(unsigned i=0;i<8;++i)CoopSelection(game_,i)=selection_[i];
  }
private:
  Game &game_;ClientNativePresentation *previous_;
  unsigned curses_,challenge_,difficulty_,temporary_=0;bool custom_,rerun_,debug_;std::array<int,8> selection_{};
  int transition_=0;
};
bool FXOwnerMatches(ClientNativePresentation &p,Room &room) noexcept {
  unsigned count=0;const auto &v=FXLayersVector(room);const auto &r=RayGroupsVector(room);
  const auto *fx_end=p.fx_installed?p.fx_installed+p.fx_installed_count:nullptr;
  const auto *ray_end=p.rays_installed?p.rays_installed+p.rays_installed_count:nullptr;
  // vector身份属于真实owner；倒计时/loaded/selected等显示值不能参与所有权判断。
  const bool valid=FXVectorValid(v,count)&&v.begin==p.fx_installed&&count==p.fx_installed_count&&v.end==fx_end&&v.capacity==fx_end&&
      r.begin==p.rays_installed&&r.end==ray_end&&r.capacity==ray_end&&FXRayGroups(room)==p.rays_installed_count;
  if(!valid&&!p.fx_owner_reported){
    p.fx_owner_reported=true;
    Trace("client_presentation_fx_owner","fx=%p,%p,%p installed=%p count=%u actual_count=%u rays=%p,%p,%p installed=%p count=%u actual_count=%u poop=%d loaded=%u selected=%d",
        v.begin,v.end,v.capacity,p.fx_installed,p.fx_installed_count,count,r.begin,r.end,r.capacity,p.rays_installed,p.rays_installed_count,
        FXRayGroups(room),room._fxLayers._poopFxCountdown,room._fxLayers._loaded,SelectedRayGroup(room));
  }
  return valid;
}
template <class Source> bool LoadFXImage(KAGE_SmartPointer_ImageBase &pair,const Source &source,bool ray=false) {
  // Manager::LoadImage经8D2AB0解析逻辑资源名；Image._name却是已挂载resources/gfx/...。
  // 只还原这个已核根，保留Authority选中的完整图片身份，并核对实际返回name，避免重选/双前缀。
  const std::string path=source.image.rfind("resources/gfx/",0)==0?source.image.substr(10):source.image;
  // 返回值out-slot只能指向空候选；换代时用smart赋值释放旧strong，不直接覆盖已有owner。
  KAGE_SmartPointer_ImageBase candidate;
  Manager::LoadImage(&candidate,path.c_str(),__ptr_g_VertexAttributeDescriptor_Position,false);
  auto *image=candidate.image;
  if(!image||!candidate.counter||image->GetWidth()<=0||image->GetHeight()<=0||
     !image->_name||source.image!=image->_name)return false;
  // 原生96B3D2先按Position加载ray，96B488/48E再以[AACB00]的描述符调用vtable+3C。
  // 9E2210据描述符身份设置shader并复制完整属性布局；每次显式执行，不能依赖Host缓存。
  // J273 FX loader三种真实格式；shader3同时可能是44字节mask或88字节扩展格式，不能混同。
  const bool position=source.shader==1&&source.vertex_stride==36,mask=source.shader==3&&source.vertex_stride==44,full=source.shader==3&&source.vertex_stride==88;
  if((!position&&!mask&&!full)||(ray&&!full))return false;
  const auto *format=position?__ptr_g_VertexAttributeDescriptor_Position:
      reinterpret_cast<const KAGE_Graphics_ImageBase_VertexAttributeDescriptor *>(Base+(mask?0x788734:0x788760));
  if(!format)return false;
  image->SetVertexFormat(*format);
  using ImageParameters=void (__thiscall *)(KAGE_Graphics_ImageBase *,unsigned,unsigned);
  auto **table=*reinterpret_cast<void ***>(image);
  reinterpret_cast<ImageParameters>(table[16])(image,source.min_filter,source.mag_filter);
  reinterpret_cast<ImageParameters>(table[17])(image,source.wrap_s,source.wrap_t);
  // J273 B88760为ray八属性（stride88）；Position为三属性（stride36）。
  const bool valid=image->_predefinedShader==source.shader&&
      image->_minFilterMode==source.min_filter&&image->_magFilterMode==source.mag_filter&&
      image->_wrapSMode==source.wrap_s&&image->_wrapTMode==source.wrap_t&&
      image->_vertexFormat&&image->_numVertexAttributes==source.attributes.size()&&image->_vertexFormatStride==source.vertex_stride&&
      std::memcmp(image->_vertexFormat,source.attributes.data(),source.attributes.size())==0;
  if(!valid)return false;
  pair=std::move(candidate);return true;
}
bool AuxOwnerMatches(const ClientNativePresentation &p,Room &room){
  const auto &v=PoopVector(room);const auto *aux=p.auxiliary.get();
  if(!aux){if(v.begin||v.end||v.capacity||PoopImage(room).image)return false;
    for(unsigned i=0;i<10;++i)for(auto *child:RoomVeins(room)[i].children)if(child)return false;return true;}
  const auto *end=aux->poop?aux->poop+aux->poop_count:nullptr;
  if(v.begin!=aux->poop||v.end!=end||v.capacity!=end||PoopImage(room).image!=aux->image.image||PoopImage(room).counter!=aux->image.counter)return false;
  for(unsigned i=0;i<10;++i){const auto &root=RoomVeins(room)[i],&expected=aux->nodes[aux->roots[i]];
    for(unsigned j=0;j<3;++j)if(root.children[j]!=expected.children[j])return false;}return true;
}
bool BuildAuxCandidate(ClientNativePresentation &p,const ClientPresentationDescription &d,Room &room){
  if(p.auxiliary_candidate||!AuxOwnerMatches(p,room))return false;
  p.auxiliary_candidate=std::make_unique<NativeAux>();auto &a=*p.auxiliary_candidate;const auto &source=d.auxiliary;
  a.roots=source.roots;
  // 不适用的Source图没有标量；本方只构造十个空根，保证原生首child判定关闭绘制。
  a.node_count=source.veins.empty()?10u:unsigned(source.veins.size());a.poop_count=unsigned(source.poop_poses.size());
  a.nodes=static_cast<NativeVein *>(NativeFXAllocate(a.node_count*sizeof(NativeVein)));if(!a.nodes)return false;
  if(source.veins.empty())for(unsigned i=0;i<10;++i){new(a.nodes+i) NativeVein{};a.roots[i]=i;}
  for(unsigned i=0;i<source.veins.size();++i){const auto &s=source.veins[i];auto &n=a.nodes[i];new(&n) NativeVein;
    n.kind=s.kind;n.rotation=s.rotation;n.scale=s.scale;n.factor=s.factor;n.opacity=s.opacity;n.depth=s.depth;n.x=s.x;n.y=s.y;
    for(unsigned j=0;j<3;++j)n.children[j]=s.children[j]<0?nullptr:a.nodes+unsigned(s.children[j]);}
  if(a.poop_count){a.poop=static_cast<NativeFXPose *>(NativeFXAllocate(a.poop_count*sizeof(NativeFXPose)));if(!a.poop)return false;
    for(unsigned i=0;i<a.poop_count;++i){const auto &s=source.poop_poses[i];new(a.poop+i) NativeFXPose{s.x,s.y,s.velocity_x,s.velocity_y,s.flip_x,s.flip_y,{0,0}};}}
  if(source.poop_image_present&&!LoadFXImage(a.image,source.poop_image,true))return false;
  if(source.veins_present)for(unsigned i=0;i<2;++i){auto *sprite=reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&room._fxLayers)+0x2c8+i*sizeof(ANM2));
    if(source.vein_sprite_present[i]){if(!ClientApplyResourcePose(sprite,source.vein_sprites[i]).complete)return false;}
    else if(sprite->_loaded){sprite->destructor();sprite->constructor();}}
  return true;
}
void InstallAuxCandidate(ClientNativePresentation &p,Room &room,const ClientPresentationDescription &d){
  auto &a=*p.auxiliary_candidate;for(unsigned i=0;i<10;++i)RoomVeins(room)[i]=a.nodes[a.roots[i]];
  PoopImage(room)=a.image;PoopVector(room)={a.poop,a.poop?a.poop+a.poop_count:nullptr,a.poop?a.poop+a.poop_count:nullptr};
  auto *bytes=reinterpret_cast<unsigned char *>(&room._fxLayers);const int count=int(a.poop_count);std::memcpy(bytes+0x50,&count,4);
  std::memcpy(bytes+0x54,d.auxiliary.poop_settings,sizeof(float)*4);room._fxLayers._poopFxColor=Color(d.auxiliary.poop_color);
  p.auxiliary=std::move(p.auxiliary_candidate);
}
void RetireAux(ClientNativePresentation &p,Room &room){
  for(unsigned i=0;i<10;++i)RoomVeins(room)[i]=NativeVein{};
  PoopVector(room)={};PoopImage(room)=KAGE_SmartPointer_ImageBase{};const unsigned zero=0;
  std::memcpy(reinterpret_cast<unsigned char *>(&room._fxLayers)+0x50,&zero,4);
  p.auxiliary.reset();p.auxiliary_candidate.reset();
}
bool BuildFXCandidate(ClientNativePresentation &p,const ClientPresentationDescription &description) {
  if(p.fx_candidate)return false;
  if(description.fx_layers.empty())return true;
  p.fx_candidate=static_cast<NativeFX *>(NativeFXAllocate(description.fx_layers.size()*sizeof(NativeFX)));
  if(!p.fx_candidate)return false;
  for(const auto &source:description.fx_layers){
    auto &target=p.fx_candidate[p.fx_candidate_count];new(&target) NativeFX;
    ++p.fx_candidate_count;target.id=source.id;
    // 仅填已核消费者读取的字段；未采用的原生word保持本方零初始化，不复制未知内存。
    target.parameters[4]=source.pass;std::memcpy(&target.parameters[8],&source.color_branch,4);
    target.parameters[9]=source.mask_backdrop;target.parameters[10]=source.blend_mode;target.parameters[11]=source.mask?1u:0u;
    // 真实selected Image路径来自Authority当前资源；不按seed或XML重新挑选overlay。
    if(!LoadFXImage(target.image,source))return false;
    target.count=int(source.poses.size());
    target.poses.begin=static_cast<NativeFXPose *>(NativeFXAllocate(source.poses.size()*sizeof(NativeFXPose)));
    if(!source.poses.empty()&&!target.poses.begin)return false;
    target.poses.end=target.poses.capacity=source.poses.empty()?target.poses.begin:target.poses.begin+source.poses.size();
    for(unsigned i=0;i<source.poses.size();++i){const auto &s=source.poses[i];
      new(target.poses.begin+i) NativeFXPose{s.x,s.y,s.velocity_x,s.velocity_y,s.flip_x,s.flip_y,{0,0}};
    }
  }
  return true;
}
bool BuildRayCandidate(ClientNativePresentation &p,const ClientPresentationDescription &description) {
  if(p.rays_candidate)return false;
  if(description.ray_groups.empty())return true;
  p.rays_candidate=static_cast<NativeRayGroup *>(NativeFXAllocate(description.ray_groups.size()*sizeof(NativeRayGroup)));
  if(!p.rays_candidate)return false;
  for(const auto &source:description.ray_groups){
    auto &group=p.rays_candidate[p.rays_candidate_count];new(&group) NativeRayGroup;++p.rays_candidate_count;
    std::copy(source.geometry.begin(),source.geometry.end(),group.geometry);
    if(source.rays.empty())continue;
    group.rays.begin=static_cast<NativeRay *>(NativeFXAllocate(source.rays.size()*sizeof(NativeRay)));
    if(!group.rays.begin)return false;
    group.rays.end=group.rays.begin;group.rays.capacity=group.rays.begin+source.rays.size();
    for(const auto &ray:source.rays){auto &target=*group.rays.end;new(&target) NativeRay;++group.rays.end;
      target.opacity=ray.opacity;target.speed=ray.speed;target.phase=ray.phase;
      if(!LoadFXImage(target.image,ray,true))return false;
    }
  }
  return true;
}
void InstallFXCandidate(ClientNativePresentation &p,Room &room,const ClientPresentationDescription &description) {
  auto &v=FXLayersVector(room);
  // 全部新Image/pose已准备，且当前CPU提交退尽后才释放上一代；原生Room始终是实际owner。
  if(p.fx_installed){DestroyFXRecords(p.fx_installed,p.fx_installed_count);p.fx_installed=nullptr;p.fx_installed_count=0;v={};}
  if(p.fx_candidate){
    v={p.fx_candidate,p.fx_candidate+p.fx_candidate_count,p.fx_candidate+p.fx_candidate_count};
    p.fx_installed=p.fx_candidate;p.fx_installed_count=p.fx_candidate_count;p.fx_candidate=nullptr;p.fx_candidate_count=0;
  }
  auto &ray_vector=RayGroupsVector(room);
  if(p.rays_installed){DestroyRayGroups(p.rays_installed,p.rays_installed_count);p.rays_installed=nullptr;p.rays_installed_count=0;ray_vector={};}
  if(p.rays_candidate){
    ray_vector={p.rays_candidate,p.rays_candidate+p.rays_candidate_count,p.rays_candidate+p.rays_candidate_count};
    p.rays_installed=p.rays_candidate;p.rays_installed_count=p.rays_candidate_count;p.rays_candidate=nullptr;p.rays_candidate_count=0;
  }
  auto *bytes=reinterpret_cast<unsigned char *>(&room._fxLayers);
  std::memcpy(bytes,description.fx_average.data(),sizeof(float)*2);
  std::memcpy(bytes+0xb0,description.fx_bounds.data(),sizeof(float)*4);
  SelectedRayGroup(room)=description.fx_ray_group;
  // loaded时安装Authority实际倒计时；不适用时仅构造本方关闭状态，不复制Source冷堆值。
  room._fxLayers._poopFxCountdown=description.fx_poop_countdown;
  room._fxLayers._loaded=description.fx_loaded;
}
bool FXDrawReady(ClientNativePresentation &p,Room &room) noexcept {
  if(!FXOwnerMatches(p,room)||p.fx_installed_count!=p.description.fx_layers.size())return false;
  if(p.rays_installed_count!=p.description.ray_groups.size()||SelectedRayGroup(room)!=p.description.fx_ray_group||
      room._fxLayers._poopFxCountdown!=p.description.fx_poop_countdown||room._fxLayers._loaded!=p.description.fx_loaded)return false;
  const auto *bytes=reinterpret_cast<const unsigned char *>(&room._fxLayers);
  if(std::memcmp(bytes,p.description.fx_average.data(),sizeof(float)*2)||std::memcmp(bytes+0xb0,p.description.fx_bounds.data(),sizeof(float)*4))return false;
  for(unsigned i=0;i<p.rays_installed_count;++i){const auto &group=p.rays_installed[i];const auto count=p.description.ray_groups[i].rays.size();
    if(count?(!group.rays.begin||group.rays.end!=group.rays.begin+count||group.rays.capacity!=group.rays.end):
       (group.rays.begin||group.rays.end||group.rays.capacity))return false;
    for(unsigned j=0;j<count;++j)if(!group.rays.begin[j].image.image||!group.rays.begin[j].image.counter)return false;
  }
  if(!p.fx_installed_count&&!p.rays_installed_count)return true;
  if(!room._fxLayers._loaded)return false;
  for(unsigned i=0;i<p.fx_installed_count;++i){const auto &v=p.fx_installed[i];
    if(v.id!=p.description.fx_layers[i].id||v.parameters[4]!=p.description.fx_layers[i].pass||v.parameters[10]!=p.description.fx_layers[i].blend_mode||!v.image.image||!v.image.counter||(v.count&&!v.poses.begin)||
       v.count!=int(p.description.fx_layers[i].poses.size())||v.poses.end!=(v.count?v.poses.begin+v.count:v.poses.begin))return false;
  }
  return true;
}
// 原生96DAAA会phase+=speed；保存每条原位相位并在正常/异常离栈恢复，不推进展示目标。
class RayPhases final {
public:
  explicit RayPhases(NativeRayGroup &group) noexcept:group_(group),count_(group.rays.begin?unsigned(group.rays.end-group.rays.begin):0) {
    for(unsigned i=0;i<count_;++i)phase_[i]=group.rays.begin[i].phase;
  }
  ~RayPhases(){for(unsigned i=0;i<count_;++i)group_.rays.begin[i].phase=phase_[i];}
private:
  NativeRayGroup &group_;unsigned count_=0;std::array<float,64> phase_{};
};
// 96EAB0不推进动画，但为各node覆盖两只vein Sprite的scale/rotation/ColorMod；绘制后归还来源姿态。
class VeinDrawBorrow final {
public:
  VeinDrawBorrow(Room &room,bool enabled):room_(room),enabled_(enabled){if(enabled_)for(unsigned i=0;i<2;++i){auto &s=Sprite(i);previous_[i]={s._scale,s._rotation,s._color};}}
  ~VeinDrawBorrow(){if(enabled_)for(unsigned i=0;i<2;++i){auto &s=Sprite(i);s._scale=previous_[i].scale;s._rotation=previous_[i].rotation;s._color=previous_[i].color;}}
private:
  ANM2 &Sprite(unsigned i){return *reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&room_._fxLayers)+0x2c8+i*sizeof(ANM2));}
  struct Pose {Vector scale;float rotation=0;ColorMod color;};
  Room &room_;bool enabled_;std::array<Pose,2> previous_{};
};
ClientPresentationStep PrepareImpl(ClientNativePresentation *p,const ClientNativeRoots &roots,const ClientPresentationDescription &description) {
  if(!p||p->active||p->uncertain||!Current(roots)||TopOwner()!=Owner::Manager||!ClientDispatchIsolationInstalled())return {false,false,true,"presentation_prepare_owner"};
  if(!GraphicsContext()||!ClientNativeCpuQueuesEmpty())return {false,false,false,"presentation_prepare_context_or_inflight"};
  auto &game=*static_cast<Game *>(roots.game);auto &room=*static_cast<Room *>(roots.room);
  const bool mirror=game._currentDimensionIdx==1&&(game._stageType==4||game._stageType==5)&&(game._stage==2||(game._stage==1&&(description.effective_curses&2)));
  if(mirror!=description.mirror)return {false,false,true,"presentation_mirror_source_conflict"};
  auto &entities=p->entities;unsigned count=0;const char *reason=nullptr;
  if(!Members(room,game,entities,count,reason))return {false,false,true,reason};
  if(!FXOwnerMatches(*p,room))return {false,false,true,"presentation_foreign_fx_owner"};
  // Home专用pass直接消费Effect166/sub951，由19的真实Entity承载提供；不能用其它背景代画。
  if(description.special_light_pass)return {false,false,true,"presentation_dependency_I19_home_effect166_951"};
  if(game._seedEffects.AchievementUnlocksDisallowed()!=description.hud_seed_blocked)
    return {false,false,true,"presentation_hud_seed_context_requires_adapter"};
  ClientPresentationDescription candidate=description;
  p->ready=false;std::swap(p->description,candidate);p->retired=false;p->game=&game;p->room=&room;
  if(!p->screen_shake_captured){p->previous_screen_shake=game._screenShakeOffset;p->screen_shake_captured=true;}
  if(!p->environment_snapshot){p->previous_current_color=game._currentColorModifier;p->previous_target_color=game._targetColorModifier;p->previous_lerp_color=game._lerpColorModifier;
    p->previous_color_lerp=game._shouldLerpColorModState;p->previous_lightning=game._lightningIntensity;
    std::memcpy(&p->previous_darkness,reinterpret_cast<unsigned char *>(&game)+0x25a90,4);std::memcpy(&p->previous_darkness_scale,reinterpret_cast<unsigned char *>(&game)+0x25a94,4);p->environment_snapshot=true;}
  p->written=true;
  if(!BuildAuxCandidate(*p,description,room))return {false,true,true,"presentation_auxiliary_resource_candidate"};
  if(!BuildFXCandidate(*p,description))return {false,true,true,"presentation_fx_resource_candidate"};
  if(!BuildRayCandidate(*p,description))return {false,true,true,"presentation_ray_resource_candidate"};
  // 7E08E0只保证具名native render surfaces/资源存在，不调用703E40的其余全局链。
  // 已读其全部分支：CreateProceduralImage/LoadImage/SetVertexFormat/filter/wrap，无模拟tick。
  reinterpret_cast<void (__cdecl *)()>(Base+0x3e08e0)();
  if(description.mirror){auto &pair=Surface(0x7e7340);auto *image=pair.image;
    if(!image||!pair.counter||(image->_vertexFormatStride!=48&&image->_vertexFormatStride!=88))return {false,true,true,"presentation_mirror_surface"};
    if(!p->mirror_format_snapshot){p->previous_mirror_extended=image->_vertexFormatStride==88;p->mirror_format_snapshot=true;}
    image->SetVertexFormat(*reinterpret_cast<const KAGE_Graphics_ImageBase_VertexAttributeDescriptor *>(Base+(description.mirror_shader?0x788670:0x788760)));
    if(image->_vertexFormatStride!=(description.mirror_shader?48:88)||(description.mirror_shader&&!reinterpret_cast<KAGE_Graphics_Shader *>(Base+0x7ab294)->_initialized))return {false,true,true,"presentation_mirror_format_or_shader"};}
  // 6EFB70仅分配/配置全局图形surface；不使用703E40的Room/全局更新链。
  if(description.color_enabled)reinterpret_cast<void (__cdecl *)()>(Base+0x2efb70)();
  if(!PassResourcesReady(description))return {false,true,true,"presentation_native_pass_resources"};
  if(!LoadRoomLight(room,false)||!LoadRoomLight(room,true))return {false,true,true,"presentation_native_room_light_resources"};
  if(description.shading_present&&!LoadFXImage(p->shading,description.shading))return {false,true,true,"presentation_shading_resource"};
  if(!description.shading_present)p->shading=KAGE_SmartPointer_ImageBase{};
  auto &minimap=game._minimap;
  if(!p->map_snapshot){
    std::memcpy(p->previous_map_header.data(),&minimap,0x18);
    for(unsigned i=0;i<2;++i)std::memcpy(p->previous_map_config[i].data(),reinterpret_cast<unsigned char *>(&minimap._config[i])+0x11c,0x44);
    std::memcpy(p->previous_map_shake.data(),reinterpret_cast<unsigned char *>(&minimap)+0x61c,12);p->map_snapshot=true;
  }
  if(!p->minimap_loaded){
    // 965740逐段已核仅Load五个内嵌ANM/Question Image、创建地图surface和设置显示标量；无Refresh/Update/拓扑/RNG。
    reinterpret_cast<void (__thiscall *)(Minimap *)>(Base+0x565740)(&minimap);p->minimap_loaded=true;
  }
  if(!minimap._config[0]._mapSprite._loaded||!minimap._itemIconsSprite._loaded||
     !LoadFXImage(QuestionImage(minimap),description.minimap_question,true))return {false,true,true,"presentation_minimap_resources"};
  minimap._state=description.minimap_state;minimap._holdTime=0;minimap._shakeDuration=description.minimap_shake_duration;
  minimap._shakeOffset=Vector(description.minimap_shake[0],description.minimap_shake[1]);
  minimap._config[0]._alpha=description.minimap_small_alpha;minimap._config[1]._alpha=description.minimap_big_alpha;
  // Refresh 仅构建真实原生 Minimap surface；保存图标/flags 在该调用内借用正式来源。
  // 外层完整保存 renderer/target，内部 push/pop 不会破坏17已修复的深度提交顺序。
  {RenderState restore;
    if(!ClientMinimapBuild(&game,description.map.descriptors,description.map.state,description.effective_curses))return {false,true,true,"presentation_minimap_source_cache"};}
  if(description.minimap_poses_present)for(unsigned i=0;i<2;++i)
    if(!ClientApplyResourcePose(&minimap._config[i]._mapSprite,description.minimap_poses[i]).complete)
      return {false,true,true,"presentation_minimap_source_pose"};
  InstallAuxCandidate(*p,room,description);InstallFXCandidate(*p,room,description);
  game._screenShakeOffset=Vector(description.screen_shake[0],description.screen_shake[1]);
  room._renderScrollOffset=Vector(description.render_scroll[0],description.render_scroll[1]);
  // 环境字段全量安装，包括清零/清空与真实lerp后态；没有普通Update或重新处理颜色。
  room._waterAmount=description.water_amount;
  room._waterColor=Color(description.water_color);room._waterColorMultiplier=Color(description.water_multiplier);
  room._waterLerpTargetColor=Color(description.water_target);room._waterLerpTargetColorMult=Color(description.water_target_multiplier);
  room._waterLerpColorDuration=description.water_lerp_duration;room._waterLerpColorCountdown=description.water_lerp_countdown;
  room._waterCurrent=Vector(description.water_current[0],description.water_current[1]);room._lavaIntensity=description.lava;room._rainIntensity=description.rain;room._numRainSpawners=description.rain_spawners;
  auto &fx_params=room._fxLayers._fxParams;fx_params.useWaterV2=description.use_water_v2;
  fx_params.waterColor=Color(description.fx_water_color);fx_params.waterColorMultiplier=Color(description.fx_water_multiplier);
  fx_params.waterEffectColor=Color(description.water_effect);fx_params.roomColor=Color(description.fx_room_color);fx_params.brightness=description.fx_brightness;fx_params.contrast=description.fx_contrast;
  const auto modifier=[](const float (&c)[6]){return ColorModState(c[0],c[1],c[2],c[3],c[4],c[5]);};
  game._currentColorModifier=modifier(description.current_color);game._targetColorModifier=modifier(description.target_color);game._lerpColorModifier=modifier(description.lerp_color);
  game._shouldLerpColorModState=description.color_lerp;game._lightningIntensity=description.lightning;
  std::memcpy(reinterpret_cast<unsigned char *>(&game)+0x25a90,&description.darkness,4);std::memcpy(reinterpret_cast<unsigned char *>(&game)+0x25a94,&description.darkness_scale,4);
  MaintainRoomSurfaceOrigin(room);
  room._floorColor=Color(description.floor);room._wallColor=Color(description.wall);
  room._fxLayers._fxParams.shadowColor=Color(description.shadow);room._fxLayers._fxParams.lightColor=Color(description.light);
  room._fxLayers._fxParams.shadowAlpha=description.shadow_alpha;
  FullSort(room,entities,count);
  p->entity_count=count;p->render_storage=room._entityList._renderEL._data;
  auto &hud=game._hud;auto &player_hud=hud._playerHUD[0];
  player_hud._player=game._playerManager._playerList[0];player_hud._HUD=&hud;player_hud._playerHudIndex=0;
  for(unsigned i=1;i<8;++i)hud._playerHUD[i]._player=nullptr;
  player_hud._redHeartFlashCountdown=std::int8_t(description.red_flash);
  player_hud._soulHeartFlashCountdown=std::int8_t(description.soul_flash);
  player_hud._goldHeartFlashCountdown=std::int8_t(description.gold_flash);DropScale(player_hud)=description.drop_scale;
  for(unsigned i=0;i<24;++i){
    const auto &source=p->description.hearts[i];auto &heart=player_hud._heart[i];
    heart._visibility=source.visible;heart._eternalHeartVisibility=source.eternal;heart._goldenHeartVisibility=source.golden;
    heart._fadeHeart=source.fading;heart._flashType=std::int8_t(source.flash);
    heart._spriteAnim=source.animation.empty()?nullptr:source.animation.c_str();
    heart._spriteOverlayAnim=source.overlay.empty()?nullptr:source.overlay.c_str();
  }
  StatPlayer(hud._statHUD,0)=player_hud._player;StatPlayer(hud._statHUD,1)=nullptr;
  for(unsigned i=0;i<10;++i){const auto &s=description.stats[i];*Stats(hud._statHUD,0,i)={s.value,s.delta,s.previous,s.threshold,s.countdown};}
  CombinedProbability(hud._statHUD)=description.combined_devil_angel;hud._isVisible=description.hud_visible;
  p->diagnostic_drawn=false;p->ready=true;return {true,true,false,"presentation_native_caches_ready"};
}
ClientPresentationStep DrawImpl(ClientNativePresentation *p,const ClientNativeRoots &roots) {
  if(!p||!p->ready||p->active||p->uncertain||p->retired||!Current(roots)||p->game!=roots.game||p->room!=roots.room||
     !HasOwner(Owner::Render)||!GraphicsContext()||!ClientDispatchIsolationInstalled())return {false,false,true,"presentation_draw_owner"};
  auto &room=*p->room;auto &game=*p->game;
  // 水纹等原生draw读取Game帧；仅本draw栈借用服务器帧，离栈恢复本地计数，不推进计时。
  struct FrameBorrow {Game &game;Room &room;int previous;std::uint32_t baseline=0;
    FrameBorrow(Game &g,Room &r):game(g),room(r),previous(g._frameCount){std::memcpy(&baseline,reinterpret_cast<unsigned char *>(&room)+0x11f0,4);}
    ~FrameBorrow(){game._frameCount=previous;std::memcpy(reinterpret_cast<unsigned char *>(&room)+0x11f0,&baseline,4);}
  } frame_borrow(game,room);
  game._frameCount=p->description.room_state.game_frame;
  ClientRoomFrameMaintain(&game,&room,p->description.room_state.frame,p->description.room_state.initialized);
  // 光线可见条件是已采绘制上下文；仅draw栈借用，用户本地图形设置在离栈恢复。
  struct FXDrawContext {Game &game;bool &lighting,&water;unsigned curses,temporary=0;bool old_lighting,old_water;
    FXDrawContext(Game &g,const ClientPresentationDescription &d):game(g),lighting(*reinterpret_cast<bool *>(reinterpret_cast<unsigned char *>(g_Manager)+0x2a1fd)),water(*reinterpret_cast<bool *>(reinterpret_cast<unsigned char *>(g_Manager)+0x2a203)),curses(g._curses),old_lighting(lighting),old_water(water){
      std::memcpy(&temporary,reinterpret_cast<unsigned char *>(&game)+0x25ac8,4);const unsigned zero=0;
      std::memcpy(reinterpret_cast<unsigned char *>(&game)+0x25ac8,&zero,4);game._curses=d.effective_curses;lighting=d.fx_ray_lighting;water=d.water_enabled;}
    ~FXDrawContext(){game._curses=curses;lighting=old_lighting;water=old_water;std::memcpy(reinterpret_cast<unsigned char *>(&game)+0x25ac8,&temporary,4);}
  } fx_context(game,p->description);
  const auto &entities=p->entities;const unsigned count=p->entity_count;const auto &render=room._entityList._renderEL;
  if(render._data!=p->render_storage||render._size!=count||render._size>render._capacity||!count||
     game._playerManager._playerList.size()!=1||!game._playerManager._playerList[0]||game._playerManager._playerList[0]->_playerType!=0)return {false,false,true,"presentation_cached_topology_changed"};
  for(unsigned i=0;i<count;++i)if(render._data[i]!=entities[i]||!entities[i]||!entities[i]->_valid||
      (entities[i]->_type==1&&entities[i]!=game._playerManager._playerList[0])||!ClientNativeObjectDrawDescription(entities[i]))
    return {false,false,true,"presentation_cached_instance_changed"};
  if(!ClientGridMembersMatch(&room))return {false,false,true,"presentation_cached_grid_owner_changed"};
  if(!FXDrawReady(*p,room)||!AuxOwnerMatches(*p,room))return {false,false,true,"presentation_fx_owner_changed"};
  if(!RayContextMatches(room,game,p->description))return {false,false,true,"presentation_ray_context_changed"};
  if(!PassResourcesReady(p->description)||*reinterpret_cast<unsigned *>(Base+0x7e72b0)>=13)
    return {false,false,true,"presentation_pass_resource_or_target_stack"};
  const bool first_diagnostic=Settings.diagnostics&&!p->diagnostic_drawn;
  // 仅显式诊断模式下记录每个已提交目标的首draw，不推进原生更新或改变排序/道具状态。
  if(first_diagnostic){
    p->diagnostic_drawn=true;
    for(unsigned i=0;i<count;++i){
      const auto *entity=entities[i];int layer=0,bias=0;
      std::memcpy(&layer,reinterpret_cast<const unsigned char *>(entity)+0x35c,4);
      std::memcpy(&bias,reinterpret_cast<const unsigned char *>(entity)+0x354,4);
      Trace("client_presentation_entity","index=%u type=%u variant=%u subtype=%u z=%u layer=%d bias=%d pos=%.6f,%.6f offset=%.6f,%.6f parent=%p",
          entity->_index,entity->_type,entity->_variant,entity->_subtype,RenderZ(entities[i]),layer,bias,
          entity->_pos.x,entity->_pos.y,entity->_posOffset.x,entity->_posOffset.y,entities[i]->GetParent());
    }
    auto *player=game._playerManager._playerList[0];auto &hud=game._hud._playerHUD[0];
    for(unsigned i=0;i<4;++i)
      Trace("client_presentation_active","slot=%u native_id=%d hud_id=%d item_image=%p book_image=%p",
          i,player->GetActiveItem(int(i)),hud._activeItem[i].id,hud._activeItem[i].itemImage,hud._activeItem[i].bookImage);
    // 诊断完整组合draw的实际门值，不把LayerState可见误当AnimationLayer/Frame均可见。
    unsigned short body_gate[2]{};std::memcpy(body_gate,reinterpret_cast<unsigned char *>(player)+0x1398,4);
    Trace("client_presentation_player_body","costumes=%u gate1398=%u gate139a=%u",unsigned(player->_costumeSpriteDescs.size()),body_gate[0],body_gate[1]);
    for(unsigned i=0;i<15;++i){const auto &m=player->_playerCostumeMap[i];
      Trace("client_presentation_costume_map","id=%u index=%d layer=%d priority=%d body=%u",i,m._index,m._layerID,m._priority,m._isBodyLayer);
    }
    auto &sprite=player->_sprite;auto *body=sprite._layerCount>1?sprite.GetLayer(1):nullptr;
    for(unsigned i=0;i<2;++i){auto &state=i?sprite._overlayAnimState:sprite._animState;
      auto *layer=state._animData?state._animData->GetLayerById(1):nullptr;
      const int selector=state._layerFrames&&layer?state._layerFrames[1]:-1;auto *frame=layer?layer->GetFrame(selector):nullptr;
      Trace("client_presentation_body_frame","overlay=%u animation_layer=%u frames=%d selector=%d frame_visible=%u state_visible=%u image=%p counter=%p",
          i,layer&&layer->IsVisible(),layer?layer->GetFrameCount():0,selector,frame&&frame->visible,body&&body->_visible,
          body?body->_spriteSheet.image:nullptr,body?body->_spriteSheet.counter:nullptr);
    }
  }
  ActiveDraw active(p->active);
  ClientEntityFrameScope entity_frame(roots);
  if(!entity_frame.Ready())return {false,false,true,"presentation_entity_draw_inputs_or_hooks"};
  ClientGridDrawScope grid_pose(roots,p->description.room_state.grid_light_queries);
  if(!grid_pose.Ready())return {false,false,true,"presentation_grid_pose_borrow"};
  VeinDrawBorrow vein_pose(room,p->description.auxiliary.veins_present);
  {
    // 原生 Mirror Surface 是 Manager 的常驻图形资源；只借用其 target，不持有第二个世界。
    // 外层先选镜像 surface，内层各 prepass/颜色 surface 结束后恢复到它；HUD 在合成后绘制。
    RenderState mirror_restore;
    auto &graphics=*__ptr_g_KAGE_Graphics_Manager;
    if(p->description.mirror)mirror_restore.Target(Surface(0x7e7340).image,true);
    {
    RenderState restore;
    MaintainRoomSurfaceOrigin(room);
    {RenderState pit_restore;
      if(!ClientGridDraw(roots,ClientGridPass::TexturedPitPre,{}))return {false,false,true,"presentation_textured_pit_surface"};}
    // RoomRender 804E81—EBE分别合成floor/walls的offset；实体自带世界→屏幕变换，不加此原点。
    const Vector backdrop_offset=RoomSurfaceOrigin(room)+room._renderScrollOffset+game._screenShakeOffset;
    Vector offset=room._renderScrollOffset;
    if(first_diagnostic)ShadowDiagnostic("before_target");
    auto &shadow=Surface(0x7e7360);restore.Target(shadow.image,false);
    if(first_diagnostic)ShadowDiagnostic("shadow_target");
    graphics._blendMode=BlendMode();graphics._shader=ImageShader();
    if(first_diagnostic)Trace("client_presentation_surface","shadow=%d,%d padded=%d,%d ortho=%.3f,%.3f world=%.3f,%.3f",
        shadow.image->GetWidth(),shadow.image->GetHeight(),shadow.image->GetPaddedWidth(),shadow.image->GetPaddedHeight(),g_OrthographicWidth,g_OrthographicHeight,g_WIDTH,g_HEIGHT);
    // 只观测shadow pass是否改了已有姿态；不覆写shadow层或body可见性掩盖来源缺口。
    const auto trace_shadow=[&](const char *phase){
      if(!first_diagnostic)return;
      for(unsigned i=0;i<count;++i){auto &sprite=entities[i]->_sprite;int shadow_id=0;
        std::memcpy(&shadow_id,reinterpret_cast<const unsigned char *>(&sprite)+0x10c,4);
        auto *body=sprite._layerCount>1?sprite.GetLayer(1):nullptr;
        Trace("client_presentation_shadow","phase=%s type=%u variant=%u shadow_id=%d body_visible=%u loaded=%u animation_owner=%u",
            phase,entities[i]->_type,entities[i]->_variant,shadow_id,body&&body->_visible,sprite._loaded,sprite._animState._animation==&sprite);
        if(phase[0]=='b'){
          float shift[2]{};std::memcpy(shift,reinterpret_cast<unsigned char *>(entities[i])+0x160,8);
          Trace("client_presentation_shadow_center","type=%u variant=%u pos=%.4f,%.4f shift160=%.4f,%.4f size=%.4f scale=%.4f",
              entities[i]->_type,entities[i]->_variant,entities[i]->_pos.x,entities[i]->_pos.y,shift[0],shift[1],entities[i]->_shadowSize,sprite._scale.x);
        }
      }
    };
    trace_shadow("before");
    room._entityList.RenderShadows(&room._renderScrollOffset);
    trace_shadow("after");
    if(first_diagnostic)ShadowDiagnostic("shadow_submitted",true);
    if(!restore.FinishOffscreen(shadow.image))return {false,false,true,"presentation_shadow_target_changed"};
    if(first_diagnostic)ShadowDiagnostic("restored");
    // 阴影、光照与水面预pass先完成；主场景在选定的最终颜色surface内保持连续深度序列。
    const bool has_light=p->description.lighting_alpha>0||!p->description.fx_layers.empty();
    if(has_light){
      auto &light=Surface(0x7e7398);
      if(!light.image)return {false,false,true,"presentation_light_surface"};
      restore.Target(light.image,true);
      auto *solid=*reinterpret_cast<KAGE_Graphics_ImageBase **>(Base+0x7e743c);
      if(!solid)return {false,false,true,"presentation_light_solid_image"};
      Composite(solid,Color(p->description.shadow_fill),BlendMode());
      if(!p->description.fx_layers.empty()&&p->description.layer4_enabled){
        graphics._shader=ImageShader();
        graphics._blendMode=BlendMode(0,*reinterpret_cast<const bool *>(Base+0x7ae1b3)?1:6,1,1,1);
        KColor color=Color(p->description.light_tint);
        reinterpret_cast<void (__thiscall *)(FXLayers *,KColor *)>(Base+0x56db20)(&room._fxLayers,&color);
      }
      for(unsigned i=0;i<count;++i){ClientEntityDrawScope light(entities[i],&offset,3);if(light.CallOriginal())room.RenderEntityLight(entities[i],offset);}
      if(!ClientGridDraw(roots,ClientGridPass::Light,{offset.x,offset.y}))return {false,false,true,"presentation_grid_light"};
      if(!restore.FinishOffscreen(light.image))return {false,false,true,"presentation_light_target_changed"};
    }
    trace_shadow("after_light");
    if(first_diagnostic)ShadowDiagnostic("after_light");
    const bool water=p->description.water_amount>0&&p->description.water_enabled;
    struct BandRestore {Room &room;std::array<unsigned,2> previous{};
      explicit BandRestore(Room &r):room(r){std::memcpy(previous.data(),reinterpret_cast<unsigned char *>(&room._entityList)+0xa0,8);}
      ~BandRestore(){std::memcpy(reinterpret_cast<unsigned char *>(&room._entityList)+0xa0,previous.data(),8);}
    } band_restore(room);
    if(water){
      // 816250是RoomPreRender末端的独立水面surface绘制；无Room/Entity/Grid Update。
      // 使用已有正式成员及资源，原生Present在非空水面FBO上消费raw draw，再恢复外层target。
      const auto mode_offset=0x12bcu,cursor_offset=0x12b8u;int previous_mode=0,previous_cursor=0;
      std::memcpy(&previous_mode,reinterpret_cast<unsigned char *>(&room)+mode_offset,4);std::memcpy(&previous_cursor,reinterpret_cast<unsigned char *>(&room)+cursor_offset,4);
      struct WaterState {Room &room;int mode,cursor;bool reflection=g_ANM2_ReflectionRendering;
        ~WaterState(){std::memcpy(reinterpret_cast<unsigned char *>(&room)+0x12bc,&mode,4);std::memcpy(reinterpret_cast<unsigned char *>(&room)+0x12b8,&cursor,4);g_ANM2_ReflectionRendering=reflection;}
      } water_state{room,previous_mode,previous_cursor};
      ClientRoomWaterPreDraw(&room);
      if(!ClientNativeCpuQueuesEmpty())return {false,false,true,"presentation_water_prepass_inflight"};
    }
    // 原生Backdrop含floor/wall细节资源，原生Player含身体/头/服装组合，均非菜单Sprite。
    if(p->description.color_enabled)restore.Target(Surface(0x7e72e4).image,true);
    // FXLayers独立绘制入口仅消费已安装参数/pose，不调用Load/Update/随机。
    reinterpret_cast<void (__thiscall *)(FXLayers *)>(Base+0x56cc00)(&room._fxLayers); // pass3：背景之前
    if(water)reinterpret_cast<void (__thiscall *)(Room *,const Vector *)>(Base+0x416bb0)(&room,&backdrop_offset);
    else if(room._roomType!=16)room._backdrop.RenderFloor(backdrop_offset,room._floorColor);
    if(room._roomType!=16)reinterpret_cast<void (__thiscall *)(FXLayers *)>(Base+0x56ea50)(&room._fxLayers); // 已采vein树，无生成/随机
    if(!water&&!ClientGridDraw(roots,room._roomType==16?ClientGridPass::Dungeon:ClientGridPass::Decoration,{offset.x,offset.y}))
      return {false,false,true,"presentation_grid_floor_pass"};
    unsigned cursor=0;
    // 普通Room 804F1A设mode=1；80513E绘制Z<5000，8051A0绘墙；415350的mode1无额外筛选。
    if(water)DrawBand(room,5000,offset,3,true);
    else while(cursor<count&&RenderZ(entities[cursor])<5000)DrawEntity(entities[cursor++],offset);
    if(room._roomType!=16)room._backdrop.RenderWalls(backdrop_offset,room._wallColor);
    if(!water&&room._roomType!=16)ClientRoomRailsDraw(&room,p->description.room_state,{offset.x,offset.y});
    if(!water&&room._roomType!=16&&!ClientGridDraw(roots,ClientGridPass::Under,{offset.x,offset.y}))return {false,false,true,"presentation_grid_under_pass"};
    if(!water&&room._roomType!=16&&!ClientGridDraw(roots,ClientGridPass::TexturedPit,{offset.x,offset.y}))return {false,false,true,"presentation_textured_pit_pass"};
    if(!water&&room._roomType!=16)ClientRoomPitRailsDraw(&room,p->description.room_state,{offset.x,offset.y});
    // 80548B继续处理Z<10000；shading/阴影合成位于其后，主层实体位于其前景。
    if(water)DrawBand(room,10000,offset,3);
    else while(cursor<count&&RenderZ(entities[cursor])<10000)DrawEntity(entities[cursor++],offset);
    if(room._roomType!=16&&!ClientGridDraw(roots,ClientGridPass::Door,{offset.x,offset.y}))return {false,false,true,"presentation_grid_door_pass"};
    // 7E814F选出的实际墙边shading图，805954在walls后以同一Backdrop offset绘制。
    if(p->shading.image){graphics._blendMode=BlendMode();graphics._shader=ImageShader();ImageAt(p->shading,backdrop_offset,KColor(1,1,1,1));}
    // J273 805980的原生阴影合成色为(1,1,1,0.24)，不按猜测改写实体视觉flag。
    SurfaceComposite(shadow,KColor(1,1,1,0.24f),BlendMode());
    reinterpret_cast<void (__thiscall *)(FXLayers *)>(Base+0x56cd10)(&room._fxLayers); // pass2：阴影之后
    if(first_diagnostic)ShadowDiagnostic("shadow_composite",true);
    if(room._roomType!=16){
      if(!ClientGridDraw(roots,ClientGridPass::Main,{offset.x,offset.y}))return {false,false,true,"presentation_grid_main_pass"};
      // J273柱顶与实体按行深度交错；全部Grid先画完再画玩家会破坏前后遮挡。
      unsigned threshold=10000;
      for(unsigned row=0;row<room._gridHeight;++row){threshold=24030+row*4000;
        if(water)DrawBand(room,threshold,offset,1);else while(cursor<count&&RenderZ(entities[cursor])<threshold)DrawEntity(entities[cursor++],offset);
        if(!ClientGridDraw(roots,ClientGridPass::PillarRow,{offset.x,offset.y},row))return {false,false,true,"presentation_grid_pillar_row"};
      }
      if(water)DrawBand(room,threshold+5000,offset,1);else while(cursor<count&&RenderZ(entities[cursor])<threshold+5000)DrawEntity(entities[cursor++],offset);
      if(!ClientGridDraw(roots,ClientGridPass::PillarEnd,{offset.x,offset.y}))return {false,false,true,"presentation_grid_pillar_end"};
    }
    if(water)DrawBand(room,UINT_MAX,offset,1);else for(;cursor<count;++cursor)DrawEntity(entities[cursor],offset);
    // J273 8061A8—8061D2：全部主层结束后，按真实已消费范围调用slot7，无offset参数。
    // 水面和普通路径在此共用一次Post，不在每张Sprite/每个反射pass重复执行。
    unsigned post_count=count;if(water)std::memcpy(&post_count,reinterpret_cast<unsigned char *>(&room._entityList)+0xa0,4);
    if(post_count>count)return {false,false,true,"presentation_entity_post_cursor"};
    for(unsigned i=0;i<post_count;++i){auto **table=*reinterpret_cast<void ***>(entities[i]);reinterpret_cast<void (__thiscall *)(Entity *)>(table[7])(entities[i]);}
    // light集合只由所选实体的真实native helper维护，包含普通Isaac必需的原生LightGradient。
    // 来源已选layer4 FX写同一light surface；不调用含共享随机的FXLayers Load/Update。
    if(has_light)SurfaceComposite(Surface(0x7e7398),KColor(1,1,1,p->description.lighting_alpha),BlendMode(0,4,7,8,7),true);
    if(!p->description.fx_layers.empty()||p->description.fx_poop_countdown>0||HasSelectedRays(p->description)){
      // RoomRender原生80685F：ECX=实际FXLayers，无栈参数，尾96DB1A ret。
      // foreground/poop/ray均消费已安装后态；ray相位自增由下面的栈借用恢复。
      NativeRayGroup empty;auto &selected=HasSelectedRays(p->description)?p->rays_installed[unsigned(p->description.fx_ray_group)]:empty;
      RayPhases phases(selected);
      struct SelectedRayBorrow {Room &room;int previous;~SelectedRayBorrow(){SelectedRayGroup(room)=previous;}} selection{room,SelectedRayGroup(room)};
      if(!HasSelectedRays(p->description))SelectedRayGroup(room)=-1;
      graphics._blendMode=BlendMode();graphics._shader=ImageShader();
      reinterpret_cast<void (__thiscall *)(FXLayers *)>(Base+0x56ce20)(&room._fxLayers);
    }
    graphics._blendMode=BlendMode();graphics._shader=ImageShader();
    reinterpret_cast<void (__thiscall *)(FXLayers *)>(Base+0x56e2b0)(&room._fxLayers); // pass6：最终持续覆盖层
    if(p->description.color_enabled){
      if(!restore.FinishOffscreen(Surface(0x7e72e4).image))return {false,false,true,"presentation_color_surface_or_packet"};
#ifdef ISAACCS_E2E
      if(first_diagnostic&&p->description.current_color[0]<0&&!SampleSceneSurfaces(room))return {false,false,true,"e2e_surface_read_context"};
#endif
      if(!ColorComposite(p->description))return {false,false,true,"presentation_color_surface_or_packet"};
    }
    }
    if(p->description.mirror){
      if(!mirror_restore.FinishOffscreen(Surface(0x7e7340).image)||!MirrorComposite(room,p->description))return {false,false,true,"presentation_mirror_surface_or_packet"};
      // 镜像 quad 消费后再接 HUD，保持 native 递减 z 与 FBO depth 的真实提交边界。
      FlushSameTarget(graphics);
    }
    graphics._blendMode=BlendMode();graphics._shader=ImageShader();
    if(game._hud._isVisible){
      HudDraw hud(*p);
      if(first_diagnostic){
        auto &map=game._minimap;int outer_mode=0;std::memcpy(&outer_mode,reinterpret_cast<unsigned char *>(&game)+0x25b8c,4);
        Trace("client_presentation_minimap","outer_mode=%d transition=%d curses=%u small=%.4f big=%.4f frame_loaded=%u question=%p",
            outer_mode,game._roomTransition._mode,EffectiveCurses(game),map._config[0]._alpha,map._config[1]._alpha,map._config[0]._mapSprite._loaded,QuestionImage(map).image);
      }
      if(EffectiveCurses(game)!=p->description.effective_curses)return {false,false,true,"presentation_hud_curse_context_changed"};
      game._hud.Render();
    }
    // floor到HUD保留连续递减的native z；统一由下方RenderState离栈消费主队列一次。
    if(first_diagnostic)ShadowDiagnostic("main_submitted",true);
  }
  if(!ClientNativeCpuQueuesEmpty())return {false,false,true,"presentation_cpu_raw_use_not_consumed"};
  return {true,false,false,"presentation_native_draw_complete"};
}
}
bool ClientPresentationHudAchievementQuery(bool &result) noexcept {
  // hud_draw只能由已通过PresentDraw分派的DrawImpl同步设置；其它线程/owner/退休状态拒绝覆盖。
  if(!IsMain()||!HasOwner(Owner::Render))return false;
  auto *p=hud_draw;
  if(!p||!p->ready||p->retired||p->uncertain||!p->active||p->game!=g_Game||
      !GraphicsContext()||!ClientDispatchIsolationInstalled())return false;
  result=p->description.hud_achievement_blocked;return true;
}
void ClientPresentationMinimapDiagnostic(void *value,bool after) noexcept {
  // 只记录真实Minimap调用的首个前/后状态，不补调Render，也不改原生分支。
  if(!Settings.diagnostics||!IsMain()||!g_Game||value!=&g_Game->_minimap)return;
  auto *p=hud_draw;const unsigned bit=after?2u:1u;
  const bool source=!p&&!ClientBusy()&&!Settings.world_e2e_token.empty()&&g_Game->_room&&*reinterpret_cast<const bool *>(g_Game->_room);
  if(source){
    if(g_Game->_roomTransition._mode!=0||g_Game->_minimap._config[0]._alpha<0.99f)return;
    // 只读E2E的Source/Client差异诊断；每个连续房间至多一次前后，最多32房，不增加Hook。
    static int previous=INT32_MIN;static unsigned mask=0,rooms=0;
    if(previous!=g_Game->_currentRoomIdx){previous=g_Game->_currentRoomIdx;mask=0;++rooms;}
    if(rooms>32||(mask&bit))return;mask|=bit;
  }else{
    if(!HasOwner(Owner::Render)||!p||!p->active||!p->ready||p->game!=g_Game||(p->minimap_diagnostic&bit))return;
    p->minimap_diagnostic|=bit;
  }
  auto &map=g_Game->_minimap;auto &sprite=map._config[0]._mapSprite;
  const auto &geometry=map._config[0];
  Trace("minimap_geometry_diagnostic","source=%u after=%u anchor=%.4f,%.4f padding=%.4f,%.4f offset=%.4f scale=%.4f cells=%d,%d icons=%d size1=%.4f,%.4f size2=%.4f,%.4f alpha=%.4f",
      source,after,geometry._vec1.x,geometry._vec1.y,geometry._borderPadding.x,geometry._borderPadding.y,geometry._posOffsetX,geometry._borderScale,
      geometry._i1,geometry._i2,geometry._minimapIconNum,geometry._size1.x,geometry._size1.y,geometry._size2.x,geometry._size2.y,geometry._alpha);
  float transition=0;
  auto *raw=reinterpret_cast<const unsigned char *>(&g_Game->_roomTransition);
  std::memcpy(&transition,raw+0xc,4);
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;
  Trace("minimap_pose_diagnostic","source=%u after=%u room=%d loaded=%u main=%s frame=%.4f overlay=%s overlay_frame=%.4f overlay_first=%u layers=%u shader_rva=%llx",
      source,after,g_Game->_currentRoomIdx,sprite._loaded,sprite._animState._animData?sprite._animState._animData->_name.c_str():"",sprite._animState._animFrame,
      sprite._overlayAnimState._animData?sprite._overlayAnimState._animData->_name.c_str():"",sprite._overlayAnimState._animFrame,sprite._overlayRenderFirst,sprite._layerCount,
      static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(graphics._shader)-Base));
  for(unsigned i=0;i<(std::min)(sprite._layerCount,4u);++i){const auto *layer=sprite.GetLayer(int(i));
    if(layer)Trace("minimap_layer_diagnostic","source=%u after=%u layer=%u visible=%u loaded=%u color=%.4f image=%s main_selector=%d overlay_selector=%d",
        source,after,i,layer->_visible,layer->_spriteSheetLoaded,layer->_color._tint[3],layer->_spriteSheetPath.c_str(),
        sprite._animState._layerFrames?sprite._animState._layerFrames[i]:-1,sprite._overlayAnimState._layerFrames?sprite._overlayAnimState._layerFrames[i]:-1);
  }
  // 对照真实Frame消费者的只读条件；不补调绘制、不修改全局颜色或深度。
  auto *animation_layer=sprite._animState._animData?sprite._animState._animData->GetLayerById(0):nullptr;
  auto *frame=animation_layer&&sprite._animState._layerFrames?animation_layer->GetFrame(sprite._animState._layerFrames[0]):nullptr;
  Trace("minimap_frame_diagnostic","source=%u after=%u layer_visible=%u frames=%d frame_visible=%u size=%.4f,%.4f color=%.4f override_count=%u override_alpha=%.4f light=%u reflection=%u glitch=%u native_z=%.6f",
      source,after,animation_layer&&animation_layer->IsVisible(),animation_layer?animation_layer->GetFrameCount():0,
      frame&&frame->visible,frame?frame->width:0,frame?frame->height:0,frame?frame->color._tint[3]:0,
      unsigned(g_ColorOverrideStack.size()),g_ColorOverrideStack.empty()?1.0f:g_ColorOverrideStack.back()._tint[3],
      g_ANM2_LightRendering,g_ANM2_ReflectionRendering,g_ANM2_GlitchRendering,*reinterpret_cast<const float *>(Base+0x7a4714));
  Trace("client_presentation_minimap_call","after=%u visible=%u seed10=%u alpha=%.4f,%.4f transition=%d,%.4f shake=%.4f,%.4f screen=%.4f,%.4f loaded=%u anim=%s flags=%u color=%.4f scale=%.4f,%.4f target=%p shader=%p",
      after,g_Game->_hud._isVisible,g_Game->_seedEffects.HasSeedEffect(10),map._config[0]._alpha,map._config[1]._alpha,
      g_Game->_roomTransition._mode,transition,map._shakeOffset.x,map._shakeOffset.y,g_WIDTH,g_HEIGHT,sprite._loaded,
      sprite._animState._animData?sprite._animState._animData->_name.c_str():"",sprite._bitflags,sprite._color._tint[3],sprite._scale.x,sprite._scale.y,
      graphics.currentRenderTarget,graphics._shader);
}
ClientPresentationStep ClientPresentationPrepare(ClientNativePresentation *p,const ClientNativeRoots &roots,const ClientPresentationDescription &d) noexcept {
  ClientDispatchScope dispatch(ClientOperation::PresentMaintenance,ClientEntry::PresentationMaintenance);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::PresentationMaintenance)!=ClientEntryDecision::CallOriginal)return {false,false,true,"presentation_maintenance_dispatch"};
  ClientPresentationStep result;
  try{result=PrepareImpl(p,roots,d);}catch(...){if(p){p->ready=false;p->uncertain=true;}result={false,p&&p->written,true,"presentation_maintenance_exception"};}
  ClientDispatchReturned(ClientEntry::PresentationMaintenance);
  if(!dispatch.Succeeded())return {false,result.written,true,"presentation_maintenance_dispatch_return"};return result;
}
ClientPresentationStep ClientPresentationDraw(ClientNativePresentation *p,const ClientNativeRoots &roots) noexcept {
  ClientDispatchScope dispatch(ClientOperation::PresentDraw,ClientEntry::PresentationDraw);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::PresentationDraw)!=ClientEntryDecision::CallOriginal)return {false,false,true,"presentation_draw_dispatch"};
  ClientPresentationStep result;
  try{result=DrawImpl(p,roots);}catch(...){if(p){p->ready=false;p->uncertain=true;p->active=0;}result={false,false,true,"presentation_draw_exception"};}
  ClientDispatchReturned(ClientEntry::PresentationDraw);
  if(!dispatch.Succeeded())return {false,false,true,"presentation_draw_dispatch_return"};return result;
}
ClientPresentationStep ClientPresentationRetire(ClientNativePresentation *p,const ClientNativeRoots &roots) noexcept {
  if(!p)return {true,false,false,"presentation_empty_retired"};
  p->ready=false;
  if(p->retired)return {true,p->written,false,"presentation_retired"};
  if(!p->written){p->retired=true;return {true,false,false,"presentation_unwritten_cancelled"};}
  if(p->active||!IsMain()||TopOwner()!=Owner::Manager||!ContextHealthy()||!ClientBusy()||
     !GraphicsContext()||!ClientNativeCpuQueuesEmpty())return {false,p->written,false,"presentation_retire_inflight_or_context"};
  if(p->written){
    if(p->uncertain||!Current(roots)||p->game!=roots.game||p->room!=roots.room)return {false,true,true,"presentation_retire_owner_uncertain"};
    if(!FXOwnerMatches(*p,*p->room)||!AuxOwnerMatches(*p,*p->room))return {false,true,true,"presentation_retire_fx_owner"};
    try {
      RetireAux(*p,*p->room);
      if(p->fx_candidate){DestroyFXRecords(p->fx_candidate,p->fx_candidate_count);p->fx_candidate=nullptr;p->fx_candidate_count=0;}
      if(p->fx_installed){DestroyFXRecords(p->fx_installed,p->fx_installed_count);p->fx_installed=nullptr;p->fx_installed_count=0;}
      if(p->rays_candidate){DestroyRayGroups(p->rays_candidate,p->rays_candidate_count);p->rays_candidate=nullptr;p->rays_candidate_count=0;}
      if(p->rays_installed){DestroyRayGroups(p->rays_installed,p->rays_installed_count);p->rays_installed=nullptr;p->rays_installed_count=0;}
      FXLayersVector(*p->room)={};RayGroupsVector(*p->room)={};SelectedRayGroup(*p->room)=-1;p->room->_fxLayers._loaded=false;
    }catch(...){p->uncertain=true;return {false,true,true,"presentation_retire_fx_exception"};}
    auto &hud=p->game->_hud;
    for(auto &ph:hud._playerHUD){ph._player=nullptr;for(auto &h:ph._heart){h._spriteAnim=h._spriteOverlayAnim=nullptr;h._visibility=false;}}
    StatPlayer(hud._statHUD,0)=StatPlayer(hud._statHUD,1)=nullptr;
    p->shading=KAGE_SmartPointer_ImageBase{};
    if(p->mirror_format_snapshot){auto *image=Surface(0x7e7340).image;
      if(!image)return {false,true,true,"presentation_mirror_retire_surface_missing"};
      try{image->SetVertexFormat(*reinterpret_cast<const KAGE_Graphics_ImageBase_VertexAttributeDescriptor *>(Base+(p->previous_mirror_extended?0x788760:0x788670)));p->mirror_format_snapshot=false;}
      catch(...){p->uncertain=true;return {false,true,true,"presentation_mirror_retire_exception"};}}
    if(p->map_snapshot){
      auto &map=p->game->_minimap;std::memcpy(&map,p->previous_map_header.data(),0x18);
      for(unsigned i=0;i<2;++i)std::memcpy(reinterpret_cast<unsigned char *>(&map._config[i])+0x11c,p->previous_map_config[i].data(),0x44);
      std::memcpy(reinterpret_cast<unsigned char *>(&map)+0x61c,p->previous_map_shake.data(),12);p->map_snapshot=false;p->minimap_loaded=false;
    }
    if(p->screen_shake_captured){p->game->_screenShakeOffset=p->previous_screen_shake;p->screen_shake_captured=false;}
    if(p->environment_snapshot){auto &game=*p->game;game._currentColorModifier=p->previous_current_color;game._targetColorModifier=p->previous_target_color;
      game._lerpColorModifier=p->previous_lerp_color;game._shouldLerpColorModState=p->previous_color_lerp;game._lightningIntensity=p->previous_lightning;
      std::memcpy(reinterpret_cast<unsigned char *>(&game)+0x25a90,&p->previous_darkness,4);std::memcpy(reinterpret_cast<unsigned char *>(&game)+0x25a94,&p->previous_darkness_scale,4);p->environment_snapshot=false;}
  }
  p->retired=true;p->game=nullptr;p->room=nullptr;return {true,p->written,false,"presentation_retired"};
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
    const bool mask=mask_byte!=0;float color_branch=0;std::uint32_t mask_backdrop=0;
    if(generic&&!mask)std::memcpy(&color_branch,reinterpret_cast<const unsigned char *>(&fx)+0x3c,4);
    if(generic&&mask)std::memcpy(&mask_backdrop,reinterpret_cast<const unsigned char *>(&fx)+0x40,4);
    // pass1/2/3/6绘制前按实际+44调用BlendMode构造；不从图片、XML或id推测混合模式。
    const unsigned blend_mode=generic?fx.parameters[10]:0;
    presentation_capture_failure="fx_color_condition";if(!std::isfinite(color_branch))return false;
    presentation_capture_failure="fx_blend_mode";if(blend_mode>2)return false;
    lua_createtable(L,0,12);LuaNumber(L,"id",float(fx.id));LuaNumber(L,"pass",float(pass));
    LuaNumber(L,"color_branch",color_branch);LuaBoolean(L,"mask",mask);lua_pushinteger(L,mask_backdrop);lua_setfield(L,-2,"mask_backdrop");
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
  lua_createtable(L,0,40);LuaNumber(L,"version",4);
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
