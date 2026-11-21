#include "IsaacRepentance.h"
#include "binding/offsets.h"
#include "game/client/display/client_display_native.h"
#include "game/client/room/client_backdrop_native.h"
#include "game/client/scene/client_lifetime.h"
#include "game/common/bridge.h"
#include "game/common/binary_effect_e2e.h"
#include "game/common/context.h"
#include "core/replication_memory.h"
#include "runtime.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <list>
#include <new>
#include <tuple>
#include <utility>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
using RenderClock=std::chrono::steady_clock;
constexpr auto kDoorFadeOut=std::chrono::milliseconds(50);
constexpr auto kDoorFadeIn=std::chrono::milliseconds(70);
constexpr int kBossRoom=5;

// 普通门的本地显示时钟跨旧房退休保留；新房发布后才开始淡入。
struct DoorFade final {
  std::uint64_t source_scene=0,destination_scene=0;
  int target_grid=0;
  RenderClock::time_point fade_out{},fade_in{};
  bool rendered_end=false;
};
struct DoorFadeFrame final {
  const char *phase=nullptr;
  float opacity=0;
  bool draw_old_room=false;
};
template<class T> T Read(const void *p,unsigned offset){T v;std::memcpy(&v,static_cast<const std::byte *>(p)+offset,sizeof(v));return v;}
template<class T> void Write(void *p,unsigned offset,const T &v){std::memcpy(static_cast<std::byte *>(p)+offset,&v,sizeof(v));}
bool Graphics() noexcept {
  void *dc=nullptr,*rc=nullptr;return ClientRendererHandles(dc,rc)&&dc==wglGetCurrentDC()&&rc==wglGetCurrentContext();
}
bool MainWindow() noexcept{return IsMain()&&ClientBusy()&&ClientPersistenceBlocked()&&ContextHealthy()&&!InBlockedContext();}
// MSVC Release set的8B头与20B节点对应J273 Seeds内联查找；只借用本方树，不移交析构权。
using SeedNode=std::_Tree_node<int,void *>;
static_assert(_ITERATOR_DEBUG_LEVEL==0&&sizeof(std::set<int>)==8&&sizeof(SeedNode)==20&&
  offsetof(SeedNode,_Myval)==16&&offsetof(SeedNode,_Isnil)==13&&offsetof(SeedNode,_Color)==12);
struct SavedScalar final {void *root=nullptr;unsigned offset=0,size=0;std::array<std::byte,88> bytes{};};
// 房间原生资源的依赖值和本机surface身份。相同输入的热帧复用已建资源。
struct RoomGraphicsKey final {
  const RoomConfig_Room *config=nullptr;
  unsigned stage=0,stage_type=0,difficulty=0,backdrop=0,flags=0,curse=0,room_type=0;
  int width=0,height=0,wall_style=0;
  bool special_ray=false;
  bool tutorial=false,ascent=false,water=false;
  std::uint8_t graphics_flags=0;
  std::array<float,4> bounds{};
  std::array<const void *,5> targets{};
  auto Values() const noexcept {
    return std::tie(config,stage,stage_type,difficulty,backdrop,flags,curse,room_type,width,height,wall_style,
      tutorial,graphics_flags,ascent,water,special_ray,bounds,targets);
  }
};
}
// 主线程显示owner持有本方种子树、原值快照和surface资格；原生Room资源仍由Scene退休。
struct ClientDisplayNative final {
  Game *game=nullptr;Room *room=nullptr;Manager *manager=nullptr;
  RoomDescriptor *descriptor=nullptr;
#ifdef ISAACCS_E2E
  std::uint64_t e39_revision=0;
  bool e39_active=false;
#endif
  Camera *owned_camera=nullptr;
  bool hud_loaded=false,root_uncertain=false,audio_uncertain=false;
  std::array<ANM2 *,2> owned_lights{};
  RoomGraphicsKey room_graphics{};
  bool graphics_ready=false,fx_owned=false,backdrop_entered=false,radial_owned=false,hell_owned=false;
  bool global_images_owned=false,shading_target_owned=false,shading_applicable=false,display_complete=false,capture_initial=false;
  KAGE_SmartPointer_ImageBase previous_shading,previous_shading_target,previous_ray;
  std::array<std::uint32_t,4> previous_fx_rng{};
  int graphics_width=0,graphics_height=0;
  KAGE_Graphics_ImageBase *history_surface=nullptr;
  // 持续声由 native slot 持有强引用，跨路由不借旧批次的资源目录或文字。
  struct Voice final {
    std::uint64_t instance=0,generation=0;
    int id=0;
    unsigned sample=0;
    float volume=0,pitch=1,pan=0;
    void *source=nullptr;
  };
  std::array<Voice,512> voices{};
  std::size_t voice_count=0;
  ClientDisplayMusicRequest music;
  struct MusicPrevious final {
    unsigned id=0,queued=0;
    float pitch=1,target_pitch=1;
    bool enabled=true,captured=false;
  } previous_music;
  ClientDisplayWorldRef installed,pending;
  std::array<SavedScalar,96> saved{};
  unsigned saved_count=0;
  bool written=false,failed=false,rendering=false,mixing_dirty=true,map_dirty=true;
  bool old_surface=false,new_surface=false,have_period=false,hud_dirty=false;
  std::uint64_t boss_generation=0;
  bool boss_owned=false,boss_retire_entered=false;
  bool routed=false;
  Vector old_scroll{};
  int old_grid=0,old_entry=0,old_dimension=0,old_width=0,old_height=0;
  bool old_mirror=false;
  std::uint32_t period=0,old_frame=0;
  float last_sfx_option=-1,last_sfx_modifier=1;
  int previous_shake=0;
  int surface_width=0,surface_height=0;
  // 原生换房重建可以换掉 Game 里的强引用。本地过渡持有同图形上下文中的旧画面。
  KAGE_SmartPointer_ImageBase captured_old;
  HGLRC captured_context=nullptr;
  ClientDisplayScopeValue captured_scope{};
  int captured_width=0,captured_height=0;
  DoorFade door_fade;
  std::uint64_t door_arrival_event_scene=0;
  DoorFadeFrame render_door_fade;
  int borrowed_transition_mode=0;
  ClientDisplayMemoryRef memory;
  void Save(void *root,unsigned offset,unsigned size){
    for(unsigned i=0;i<saved_count;++i)if(saved[i].root==root&&saved[i].offset==offset)return;
    if(saved_count==saved.size()||size>88)throw std::runtime_error("display_snapshot_budget");
    auto &s=saved[saved_count++];s.root=root;s.offset=offset;s.size=size;std::memcpy(s.bytes.data(),static_cast<std::byte *>(root)+offset,size);
  }
};
namespace {
// 只在已提交world期间公开；未提交／失效时绝不从本机菜单重算Host paused。
ClientDisplayNative *active=nullptr;
thread_local ClientDisplayNative *capture_owner=nullptr;
thread_local unsigned display_depth=0;
thread_local unsigned hud_post_depth=0;
thread_local unsigned fx_build_seed=0;
thread_local bool boss_replay=false;
struct DisplayScope final {DisplayScope(){++display_depth;}~DisplayScope(){--display_depth;}};
// 只在已安装 World 的 HUD::PostUpdate 内提供 StatHUD 所需的 Host 资格值。
// ZHL 的中转层会改变 Hook 可见的返回地址，不能用该地址判断原生调用点。
struct HudPostScope final {HudPostScope(){++hud_post_depth;}~HudPostScope(){--hud_post_depth;}};
ClientDisplayNative *DisplaySource() noexcept{return capture_owner?capture_owner:active;}
template<class F> void Named(ClientEntry entry,F &&call) {
  if(ClientDispatchBefore(entry)!=ClientEntryDecision::CallOriginal)throw std::runtime_error("display_named_dispatch");
  call();ClientDispatchReturned(entry);
}
bool Roots(const ClientDisplayNative &o,const ClientNativeRoots &r) noexcept {
  return o.game&&o.manager&&g_Game==o.game&&g_Manager==o.manager&&r.game==o.game&&r.room==o.room&&
    o.game->_room==o.room&&o.room&&o.descriptor&&r.descriptor==o.descriptor&&o.room->_descriptor==o.descriptor&&
    o.room->_Camera==r.camera&&(!o.owned_camera||o.owned_camera==r.camera);
}
void Reapply(ClientDisplayNative &);
bool RoomResourcesCurrent(const ClientDisplayNative &,ClientDisplayIssue *issue=nullptr) noexcept;
bool DisplayMismatch(ClientDisplayIssue *issue,const char *reason,std::uintptr_t expected=0,std::uintptr_t actual=0,
    std::uint32_t resource=UINT32_MAX,std::uint32_t field=UINT32_MAX) noexcept {
  if(issue)*issue={reason,expected,actual,resource,field};
  return false;
}
std::uintptr_t Address(const void *value) noexcept{return reinterpret_cast<std::uintptr_t>(value);}
KAGE_SmartPointer_ImageBase &ImageSlot(unsigned) noexcept;
bool DirectionalWalk(int animation,int direction) noexcept {
  return animation==0&&direction>=0&&direction<4;
}
void ForgetRoomHistory(ClientDisplayNative &o) noexcept {
  o.old_surface=o.new_surface=false;
  o.captured_old={};o.captured_context=nullptr;
  o.captured_scope={};o.door_fade={};
}
bool HistoryFitsViewport(const ClientDisplayNative &o) noexcept {
  return o.old_surface&&o.captured_old.image&&o.captured_old.counter&&
         o.captured_context==wglGetCurrentContext()&&int(g_WIDTH)==o.captured_width&&int(g_HEIGHT)==o.captured_height;
}
bool DoorArrivalHistory(const ClientDisplayNative &o,const ClientDisplayMetadata &d) noexcept {
  const auto &old=o.captured_scope;
  const auto &now=d.scope;
  return HistoryFitsViewport(o)&&old.scene&&old.scene!=now.scene&&old.room!=now.room&&
         old.session==now.session&&old.run==now.run&&old.floor==now.floor&&old.dimension==now.dimension&&
         o.old_dimension==d.dimension&&o.descriptor&&o.descriptor->Data&&o.descriptor->Data->Type!=kBossRoom;
}
// 正常 World 仍照常安装 Host 字段；这里只用已提交的换房身份选择客户机视觉效果。
void UpdateDoorFade(ClientDisplayNative &o,const ClientDisplayMetadata &d) noexcept {
  auto &fade=o.door_fade;
  const auto scene=d.scope.scene;
  const bool walk=DirectionalWalk(d.transition_animation,d.transition_direction)&&!d.transition_boss;
  if(d.transition_mode==1&&walk&&HistoryFitsViewport(o)&&o.captured_scope.scene==scene){
    const auto &t=o.game->_roomTransition;
    if(t._roomDimension==d.dimension&&t._roomIndex>=0&&t._roomIndex<169){
      const auto *target=o.game->GetRoomByIdx(t._roomIndex,t._roomDimension);
      if(target&&target->Data&&target->Data->Type!=kBossRoom&&target->GridIndex!=d.grid){
        if(fade.source_scene!=scene||fade.target_grid!=target->GridIndex)fade={scene,0,target->GridIndex};
        return;
      }
    }
  }
  if(fade.destination_scene==scene){
    if(d.transition_mode&&(!walk||d.transition_mode!=3))fade={};
    return;
  }
  if(DoorArrivalHistory(o,d)){
    const bool expected=fade.source_scene==o.captured_scope.scene&&fade.target_grid==d.grid;
    const bool arrived=d.transition_mode==3&&walk;
    if(arrived||(!d.transition_mode&&(expected||o.door_arrival_event_scene==scene))){
      if(!expected)fade={o.captured_scope.scene,0,d.grid};
      fade.destination_scene=scene;
      return;
    }
  }
  if(fade.source_scene!=scene||!d.transition_mode||!walk)fade={};
}
DoorFadeFrame ReadDoorFade(ClientDisplayNative &o,bool ready,bool draw_history) noexcept {
  auto &fade=o.door_fade;
  if(!fade.source_scene)return {};
  const auto now=RenderClock::now();
  if(fade.fade_out==RenderClock::time_point{})fade.fade_out=now;
  const auto elapsed=now-fade.fade_out;
  if(elapsed<kDoorFadeOut)
    return {"out",std::chrono::duration<float>(elapsed)/kDoorFadeOut,draw_history||ready};
  if(!ready)return {"black",1,false};
  if(fade.fade_in==RenderClock::time_point{})fade.fade_in=now;
  const auto since_ready=now-fade.fade_in;
  if(since_ready<kDoorFadeIn)return {"in",1-std::chrono::duration<float>(since_ready)/kDoorFadeIn,false};
  return {"done",0,false};
}
// 离屏捕获只运行原生PreRender/Render和图形提交；不调用转场FSM或完整ChangeRoom。
bool Capture(ClientDisplayNative &o,bool old) {
  if(!Graphics()||!RoomResourcesCurrent(o)||!o.room->_descriptor||!o.room->_descriptor->SpawnSeed)return false;
  ClientDisplayMemoryScope memory(o.memory);
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;auto &transition=o.game->_roomTransition;
  auto *target=Read<KAGE_Graphics_ImageBase *>(&transition,old?0x28:0x30);
  if(!target){reinterpret_cast<void (__thiscall *)(RoomTransition *)>(Base+0x423820)(&transition);
    target=Read<KAGE_Graphics_ImageBase *>(&transition,old?0x28:0x30);if(!target)return false;}
  DisplayScope scope;
  // 新surface在publication前绘制，Hook仍须读取同批已安装paused和原生对象，不借旧展示输入。
  struct Source final {ClientDisplayNative *old;explicit Source(ClientDisplayNative &o):old(capture_owner){capture_owner=&o;}
    ~Source(){capture_owner=old;}} source(o);
  o.room->PreRender();
  graphics.PresentWithoutSwap();Rendering::PushCurrentRenderTarget();
  struct Restore final {KAGE_Graphics_Manager &graphics;BlendMode blend;KAGE_Graphics_Shader *shader;
    ~Restore(){Rendering::RestorePreviousRenderTarget();graphics._blendMode=blend;graphics._shader=shader;}} restore{graphics,graphics._blendMode,graphics._shader};
  graphics.SetCurrentRenderTarget(target,true);graphics.Clear();
  // 与原生mode1相同的默认7号混合模式；由外层原生target owner恢复图形目标。
  // 原生离屏路径经图形 owner 的 slot 14 设置模式，同时更新后端状态。
  // J273 的两份常量只有前 16 字节，原生另行写入 alpha 目标因子 7。
  const BlendMode blend(0,Read<bool>(&graphics,0x23)?1:6,7,1,7);
  using SetBlend=void (__thiscall *)(KAGE_Graphics_Manager *,BlendMode);
  const auto blend_table=Read<void *const *>(&graphics,0);
  reinterpret_cast<SetBlend>(blend_table[14])(&graphics,blend);
  reinterpret_cast<void (__thiscall *)(Room *)>(Base+0x404500)(o.room);
  graphics.PresentWithoutSwap();
  if(!ClientNativeCpuQueuesEmpty()||Read<std::uint32_t>(o.game,0x25a70)!=ClientDisplayMeta(o.installed).frame)return false;
  Reapply(o);
  memory.Complete();
  return true;
}
// 首次公开前用同批已安装引用验证原生房间绘制，转场复用自己刚捕获的目标。
bool FlushDisplay() {
  if(!Graphics()||!__ptr_g_KAGE_Graphics_Manager)return false;
  __ptr_g_KAGE_Graphics_Manager->PresentWithoutSwap();
  return ClientNativeCpuQueuesEmpty();
}
bool ChangedRoom(const ClientDisplayWorldRef &a,const ClientDisplayWorldRef &b) noexcept {
  const auto ma=ClientDisplayMeta(a),mb=ClientDisplayMeta(b);const auto &x=ma.scope,&y=mb.scope;
  return x.run!=y.run||x.floor!=y.floor||x.dimension!=y.dimension||x.room!=y.room||x.scene!=y.scene||
    ma.room_use!=mb.room_use||ma.room_shape!=mb.room_shape;
}
// 只借用原生函数确切读取的标量，离栈立即恢复。
template<class T> struct BorrowValue final {
  void *object;
  unsigned offset;
  T before;
  BorrowValue(void *p,unsigned at,const T &value):object(p),offset(at),before(Read<T>(p,at)){Write(object,offset,value);}
  ~BorrowValue(){Write(object,offset,before);}
};
KAGE_SmartPointer_ImageBase &ImageSlot(unsigned offset) noexcept {
  return *reinterpret_cast<KAGE_SmartPointer_ImageBase *>(Base+offset);
}
KAGE_SmartPointer_ImageBase LoadDisplayImage(const char *path) {
  KAGE_SmartPointer_ImageBase image;
  Manager::LoadImage(&image,path,__ptr_g_VertexAttributeDescriptor_Position,false);
  return image;
}
ANM2 &RoomSprite(Room &r,unsigned offset) noexcept {
  return *reinterpret_cast<ANM2 *>(reinterpret_cast<std::byte *>(&r)+offset);
}
void ReleaseCamera(ClientDisplayNative &o) {
  if(!o.owned_camera)return;
  // 官方 ASMCamera 的常驻 Hook 将析构入口包装成无栈参数的 SDK 方法；必须经
  // SDK 调用并保留官方 Clamp 状态恢复。照搬裸 J273 的额外栈 word 会使返回地址错位。
  o.owned_camera->destructor();
  o.room->_Camera=nullptr;
  o.owned_camera=nullptr;
}
void ConstructCamera(ClientDisplayNative &o,const ClientDisplayInjections &input) {
  auto &scroll=o.room->_renderScrollOffset;
  o.Save(&scroll,0,sizeof(scroll));
  BorrowValue<int> previous(o.game,0x18308,input.camera_previous_entry);
  BorrowValue<int> door(o.game,0x1831c,input.camera_entry_door);
  BorrowValue<int> return_room(o.game,0x18328,input.camera_return_focus?int(o.game->_currentRoomIdx):-1);
  BorrowValue<std::array<float,2>> return_position(o.game,0x18320,input.camera_return_position);
  o.root_uncertain=true;
  ReleaseCamera(o);
  o.root_uncertain=false;
  auto *camera=static_cast<Camera *>(reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(sizeof(Camera)));
  if(!camera)throw std::bad_alloc();
  // 构造异常仍保留实际分配前缀；完整构造返回前Room不能借用该指针。
  o.owned_camera=camera;
  o.root_uncertain=true;
  camera->constructor(o.room);
  o.room->_Camera=camera;
  // 中途加入和换房的首帧抓图也按本端身体定位；原生返房焦点及本端尚未绑定时沿用 Host 首帧值。
  if(input.camera_return_focus||!ClientSnapLocalCamera(camera))
    scroll=Vector(input.camera_initial_scroll[0],input.camera_initial_scroll[1]);
  o.root_uncertain=false;
}
// J273 Room主题12可借Data->StageId提供另一楼层的FX配置。
std::pair<int,int> FxStage(int stage,int type,int id,bool greed) noexcept {
  if(stage!=12||id<=0)return {stage,type};
  if(greed){
    if(id<13)return {(id-1)/3+1,(id-1)%3};
    if(id==24||id==25)return {id-18,0};
    if(id>=27)return {(id-27)/2+1,(id-27)%2+4};
    return {stage,type};
  }
  if(id<13)return {2*((id-1)/3)+1,(id-1)%3};
  if(id==13)return {9,0};
  if(id<=17)return {(id-14)/2+10,(id-14)%2};
  if(id==26)return {12,0};
  if(id==35)return {13,0};
  if(id>=27)return {2*((id-27)/2)+1,(id-27)%2+4};
  return {stage,type};
}
// J273 0x821D00 从 ECX/EDX 取 stage/type，第三参在栈上，由 caller 清理。
// 这里不能用三参 __fastcall：MSVC 会把栈参数交给 callee 清理。
int FxPhysicalStage(int stage,int stage_type) noexcept {
  const auto entry=Base+0x421d00;
  int result;
  __asm {
    mov ecx,stage
    mov edx,stage_type
    push -1
    call dword ptr [entry]
    add esp,4
    mov result,eax
  }
  return result;
}
void ClearFx(FXLayers &fx) {
  reinterpret_cast<void (__thiscall *)(FXLayers *)>(Base+0x56bd30)(&fx);
  fx._poopFxCountdown=0;
}
// 原生vector仍属Room。校验实际就绪的图像、pose和选中ray引用，无层是合法配置。
bool FxVector(const void *owner,unsigned at,unsigned stride,unsigned &count) noexcept {
  const auto begin=Read<std::uintptr_t>(owner,at),end=Read<std::uintptr_t>(owner,at+4),cap=Read<std::uintptr_t>(owner,at+8);
  if(end<begin||cap<end||(!begin&&cap)||(end-begin)%stride||(cap-begin)%stride)return false;
  count=unsigned((end-begin)/stride);
  return true;
}
bool FxReady(const FXLayers &fx,bool applies) noexcept {
  if(fx._loaded!=applies)return false;
  unsigned layers=0,groups=0;
  if(!FxVector(&fx,0x18,0x4c,layers)||!FxVector(&fx,0x24,0x2c,groups))return false;
  if(!applies)return !layers&&!groups&&!Read<void *>(&fx,0xc4);
  const auto selected=Read<int>(&fx,0x30);
  if(selected< -1||(selected>=0&&unsigned(selected)>=groups))return false;
  const auto *layer=Read<const std::byte *>(&fx,0x18);
  for(unsigned n=0;n<layers;++n,layer+=0x4c){
    unsigned poses=0;
    if(!FxVector(layer,0xc,0x14,poses)||!Read<void *>(layer,4)||!Read<void *>(layer,8)||
       Read<unsigned>(layer,0x18)>poses)return false;
  }
  if(selected>=0){
    const auto *group=Read<const std::byte *>(&fx,0x24)+0x2c*selected;
    unsigned rays=0;
    if(!FxVector(group,0x1c,0x14,rays)||!rays)return false;
    const auto *ray=Read<const std::byte *>(group,0x1c);
    for(unsigned n=0;n<rays;++n,ray+=0x14)if(!Read<void *>(ray,0xc)||!Read<void *>(ray,0x10))return false;
  }
  if(Read<void *>(&fx,0xc4))
    return Read<bool>(&fx,0x2c8+0x109)&&Read<bool>(&fx,0x3dc+0x109);
  return true;
}
// 图形配置表和文件路径都由本机原生资源系统提供。
bool BuildFx(ClientDisplayNative &o,const RoomGraphicsKey &key,unsigned seed) {
  auto &fx=o.room->_fxLayers;
  const bool applies=key.room_type!=16&&key.backdrop!=18;
  if(!o.fx_owned&&fx._loaded)return false;
  if(!o.fx_owned){
    o.previous_fx_rng=Read<std::array<std::uint32_t,4>>(reinterpret_cast<const void *>(Base+0x7ac6d8),0);
    o.fx_owned=true;
  }
  if(!applies){ClearFx(fx);return FxReady(fx,false);}
  const auto [stage,type]=FxStage(key.stage,key.stage_type,key.config->StageId,key.difficulty==2||key.difficulty==3);
  const auto physical=FxPhysicalStage(stage,type);
  if(physical<0||physical>=37||o.game->_roomConfig._stages[physical]._backdrop<0||
     o.game->_roomConfig._stages[physical]._backdrop>=63)return false;
  // 解析器用游戏CRT构造返回串，交游戏自己的析构，不把跨模块分配转交本DLL。
  struct ResolvedPath final {
    alignas(4) std::byte storage[24]{};
    bool assigned=false;
    ~ResolvedPath(){if(assigned)reinterpret_cast<void (__thiscall *)(void *)>(Base+0x85f0)(storage);}
    std_string *Address() noexcept {return reinterpret_cast<std_string *>(storage);}
  } path;
  static_assert(sizeof(std_string)==24);
  const std_string file="fxlayers.xml";
  const auto resolve=reinterpret_cast<std_string *(__thiscall *)(void *,std_string *,const std_string *)>(Base+0x4d2ab0);
  resolve(reinterpret_cast<std::byte *>(o.manager)+0x2a3bc,path.Address(),&file);
  path.assigned=true;
  if(path.Address()->empty())return false;
  // 该入口自行设置loaded。DecorSeed只覆盖本次读取，玩法全局Random由call site替换。
  BorrowValue<unsigned> decor(o.room->_descriptor,0x58,seed);
  struct LocalSeed final {explicit LocalSeed(unsigned value){fx_build_seed=value;}~LocalSeed(){fx_build_seed=0;}} local(seed);
  fx._poopFxCountdown=0;
  reinterpret_cast<void (__thiscall *)(FXLayers *,const char *,int,int)>(Base+0x568e10)(&fx,path.Address()->c_str(),stage,type);
  if(!FxReady(fx,true))return false;
  DisplayScope display;
  Named(ClientEntry::DisplayFxUpdate,[&]{reinterpret_cast<void (__thiscall *)(FXLayers *,bool)>(Base+0x56bf40)(&fx,true);});
  return FxReady(fx,true);
}
bool PreparePoop(Room &room,const ClientDisplayInjections &input,unsigned seed) {
  auto &fx=room._fxLayers;
  if(!input.poop_fx_countdown){fx._poopFxCountdown=0;return true;}
  if(!fx._loaded)return false;
  ColorMod color;
  static_assert(sizeof(color)==sizeof(input.poop_fx_color));
  std::memcpy(&color,input.poop_fx_color.data(),sizeof(color));
  if(fx._poopFxCountdown<=0){
    BorrowValue<unsigned> decor(room._descriptor,0x58,seed);
    fx.AddPoopFx(&color);
  }
  unsigned poses=0;
  if(!Read<void *>(&fx,0x3c)||!Read<void *>(&fx,0x40)||!FxVector(&fx,0x44,0x14,poses)||
     !poses||Read<unsigned>(&fx,0x50)>poses)return false;
  fx._poopFxColor=color;
  fx._poopFxCountdown=input.poop_fx_countdown;
  return true;
}
// 原生floor/wall构图执行前设置Q4独立流；启动房投影只在本次教程检查生效。
bool BuildBackdrop(ClientDisplayNative &o,const RoomGraphicsKey &key,unsigned seed) {
  auto &b=o.room->_backdrop;
  for(unsigned i=0;i<4;++i){
    const auto &surface=*reinterpret_cast<const KAGE_SmartPointer_ImageBase *>(reinterpret_cast<const std::byte *>(&b)+i*8);
    if(!surface.image||!surface.counter||surface.image->GetWidth()!=780||surface.image->GetHeight()!=468)return false;
  }
  const auto &config=b.configurations[key.backdrop];
  if(key.backdrop&&(config.backdropId!=key.backdrop||config.gfx.empty()))return false;
  const bool sprite_floor=key.backdrop==18||key.backdrop==26||key.backdrop==35||
    key.backdrop==52||key.backdrop==53||key.backdrop==54;
  KAGE_SmartPointer_ImageBase atlas;
  KAGE_SmartPointer_ImageBase water_atlas;
  if(key.backdrop&&!sprite_floor){
    atlas=LoadDisplayImage(config.gfx.c_str());
    if(!atlas.image||!atlas.counter)return false;
  }
  if(!config.waterGfx.empty()&&!sprite_floor){
    water_atlas=LoadDisplayImage(config.waterGfx.c_str());
    if(!water_atlas.image||!water_atlas.counter)return false;
  }
  if(key.tutorial&&(o.game->_playerManager._playerList.empty()||!o.game->_playerManager._playerList.front()))return false;
  b.seed=b.rng._seed=seed;
  b.rng._shift1=5;b.rng._shift2=9;b.rng._shift3=7;
  BorrowValue<int> style(o.room,0x7068,key.wall_style);
  const auto entry=int(o.game->_currentRoomIdx);
  // Ascent首层使用固定入口84，保持原生Ascent位一致供其它构图分支读取。
  if(key.ascent&&key.stage==1&&key.stage_type!=4&&key.stage_type!=5&&((entry==84)!=key.tutorial))return false;
  BorrowValue<int> start(o.game,0x182d0,key.tutorial?entry:INT32_MIN);
  const Vector min=b.unkVec1,max=b.unkVec2;
  ClientBackdropLocalBuild build(&b);
  o.backdrop_entered=true;
  // 通过官方Init包装进入原生构图，保留回调与配置处理的原有顺序。
  b.Init(key.backdrop,true);
  b.unkVec1=min;b.unkVec2=max;
  if(!build.Complete()||b.backdropId!=key.backdrop)return false;
  return !sprite_floor||b.floorANM2._loaded;
}
const char *ShadingPath(const RoomGraphicsKey &key) noexcept {
  constexpr const char *paths[]={"gfx/backdrop/shading.png","gfx/backdrop/shading_ih.png","gfx/backdrop/shading_iv.png",
    "gfx/backdrop/shading_1x2.png","gfx/backdrop/shading_iiv.png","gfx/backdrop/shading_2x1.png","gfx/backdrop/shading_iih.png",
    "gfx/backdrop/shading_2x2.png","gfx/backdrop/shading_ltl.png","gfx/backdrop/shading_ltr.png",
    "gfx/backdrop/shading_lbl.png","gfx/backdrop/shading_lbr.png"};
  if(key.backdrop==52||key.backdrop==53)
    return key.config->Subtype==10?"gfx/backdrop/shading_closet_L.png":"gfx/backdrop/shading_closet_R.png";
  const auto shape=unsigned(key.config->Shape);
  if(key.backdrop==11)switch(shape){
    case 4:return "gfx/backdrop/shading_1x2_utero.png";
    case 6:return "gfx/backdrop/shading_2x1_utero.png";
    case 8:return "gfx/backdrop/shading_2x2_utero.png";
    case 1:return "gfx/backdrop/shading_utero.png";
  }
  return paths[shape>=1&&shape<=12?shape-1:0];
}
bool BuildShading(const RoomGraphicsKey &key,const BackdropConfig &config,KAGE_SmartPointer_ImageBase &image,
    KAGE_SmartPointer_ImageBase &surface) {
  if((config.unk1&0xffu)||!config.walls)return true;
  // 本批重绘自己的目标。普通游戏的全局shade图与surface在退出时按对归还。
  if(!surface.image){
    KColor clear(0,0,0,0);
    surface=KAGE_Graphics_ImageManager::CreateProceduralImage(780,468,"Client Room Shading",clear);
  }
  auto *target=surface.image;
  image=LoadDisplayImage(ShadingPath(key));
  if(!target||!image.image||!image.counter||target->GetWidth()!=780||target->GetHeight()!=468)return false;
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;
  Rendering::PushCurrentRenderTarget();
  struct Restore final {KAGE_Graphics_Manager &graphics;BlendMode blend;KAGE_Graphics_Shader *shader;
    ~Restore(){Rendering::RestorePreviousRenderTarget();graphics._blendMode=blend;graphics._shader=shader;}
  } restore{graphics,graphics._blendMode,graphics._shader};
  graphics.SetCurrentRenderTarget(target,false);
  graphics.Clear();
  // 按 J273 0x7E8216–0x7E8281 使用覆盖混合和原生图像 slot 9。
  // 该入口只接收位置和颜色，由图像根据当前纹理尺寸生成原生顶点。
  graphics._blendMode=BlendMode(0,1,0,1,0);
  const Vector position(0,0);
  const KColor white(1,1,1,1);
  using DrawAt=void (__thiscall *)(KAGE_Graphics_ImageBase *,const Vector *,const KColor *);
  const auto table=Read<void *const *>(image.image,0);
  reinterpret_cast<DrawAt>(table[9])(image.image,&position,&white);
  graphics.PresentWithoutSwap();
  return ClientNativeCpuQueuesEmpty();
}
bool BuildSpecialBackgrounds(ClientDisplayNative &o,const RoomGraphicsKey &key,unsigned seed) {
  KAGE_SmartPointer_ImageBase shading,ray,shading_target;
  if(o.shading_target_owned)shading_target=ImageSlot(0x7e7350);
  if(!BuildShading(key,o.room->_backdrop.configurations[key.backdrop],shading,shading_target))return false;
  if(key.special_ray){
    ray=LoadDisplayImage(seed&1?"gfx/backdrop/fx_ray_3.png":"gfx/backdrop/fx_ray_4.png");
    if(!ray.image||!ray.counter)return false;
    auto **table=*reinterpret_cast<void ***>(ray.image);
    reinterpret_cast<void (__thiscall *)(KAGE_Graphics_ImageBase *,int,int)>(table[16])(ray.image,1,1);
    const auto *format=*reinterpret_cast<const KAGE_Graphics_ImageBase_VertexAttributeDescriptor *const *>(Base+0x6acb00);
    if(!format)return false;
    ray.image->SetVertexFormat(*format);
  }
  if(!o.global_images_owned){
    o.previous_shading=ImageSlot(0x7e7370);o.previous_shading_target=ImageSlot(0x7e7350);
    o.previous_ray=ImageSlot(0x7e7358);o.global_images_owned=true;
  }
  if(shading_target.image){ImageSlot(0x7e7350)=std::move(shading_target);o.shading_target_owned=true;}
  o.shading_applicable=shading.image!=nullptr;
  ImageSlot(0x7e7370)=std::move(shading);ImageSlot(0x7e7358)=std::move(ray);
  auto &radial=RoomSprite(*o.room,0x7100);
  if(o.radial_owned){radial.Reset();o.radial_owned=false;}
  if(key.backdrop==18||key.backdrop==28){
    o.radial_owned=true;
    std_string path="gfx/Effect_SatanRadial.anm2";
    radial.Load(path,false);
    if(key.backdrop==28){
      std_string sheet="gfx/satan_radial_ultragreed.png";
      if(!radial.ReplaceSpritesheet(0,sheet))return false;
    }
    radial.LoadGraphics(false);
    auto *layer=radial.GetLayer(0);
    if(!radial._loaded||!layer||!layer->_spriteSheetLoaded)return false;
  }
  auto *hell=reinterpret_cast<std::byte *>(o.room)+0x776c;
  if(Read<Room *>(hell,0)!=o.room)return false;
  const bool uses_hell=key.config->Type==16&&key.config->StageId==35;
  if(!uses_hell&&o.hell_owned){RoomSprite(*o.room,0x7770).Reset();o.hell_owned=false;}
  if(uses_hell){
    o.hell_owned=true;
    reinterpret_cast<void (__thiscall *)(void *)>(Base+0x42e960)(hell);
    auto &sprite=RoomSprite(*o.room,0x7770);
    if(!sprite._loaded||sprite._layerCount<15)return false;
    for(int n=0;n<15;++n){const auto *layer=sprite.GetLayer(n);if(!layer||!layer->_spriteSheetLoaded)return false;}
  }
  return true;
}
// 目标地址由本机图形生命周期产生。resize损失内容时，下一批重新构图。
std::array<const void *,5> RoomTargets(const Room &room) noexcept {
  const auto &b=room._backdrop;
  return {b.floorBuffer.image,b.wallsBuffer.image,b.floorSurface2.image,b.tempBackdropSurface.image,ImageSlot(0x7e7350).image};
}
bool RoomResourcesCurrent(const ClientDisplayNative &o,ClientDisplayIssue *issue) noexcept {
  if(!o.graphics_ready||!o.game||!o.room)return DisplayMismatch(issue,"graphics_owner_not_ready");
  if(!o.owned_camera||o.room->_Camera!=o.owned_camera)
    return DisplayMismatch(issue,"graphics_camera_changed",Address(o.owned_camera),Address(o.room->_Camera));
  if(!o.hud_loaded)return DisplayMismatch(issue,"graphics_hud_not_loaded",1);
  const auto targets=RoomTargets(*o.room);
  constexpr const char *target_names[]={"floor_buffer","walls_buffer","floor_surface_two","temporary_backdrop","global_backdrop"};
  for(unsigned i=0;i<targets.size();++i)if(targets[i]!=o.room_graphics.targets[i])
    return DisplayMismatch(issue,target_names[i],Address(o.room_graphics.targets[i]),Address(targets[i]),i);
  if(int(g_WIDTH)!=o.graphics_width)return DisplayMismatch(issue,"graphics_width_changed",o.graphics_width,int(g_WIDTH));
  if(int(g_HEIGHT)!=o.graphics_height)return DisplayMismatch(issue,"graphics_height_changed",o.graphics_height,int(g_HEIGHT));
  const auto &shading=ImageSlot(0x7e7370);
  if((shading.image!=nullptr)!=o.shading_applicable)
    return DisplayMismatch(issue,"graphics_shading_presence",o.shading_applicable,Address(shading.image),0x7e7370);
  const auto &ray=ImageSlot(0x7e7358);
  if((ray.image!=nullptr)!=o.room_graphics.special_ray)
    return DisplayMismatch(issue,"graphics_ray_presence",o.room_graphics.special_ray,Address(ray.image),0x7e7358);
  const auto *history=ImageSlot(0x7e72b4).image;
  if(!o.history_surface||history!=o.history_surface)
    return DisplayMismatch(issue,"graphics_history_changed",Address(o.history_surface),Address(history),0x7e72b4);
  const bool fx=o.room_graphics.room_type!=16&&o.room_graphics.backdrop!=18;
  if(o.room->_fxLayers._loaded!=fx)return DisplayMismatch(issue,"graphics_fx_loaded_changed",fx,o.room->_fxLayers._loaded);
  return true;
}
RoomGraphicsKey RoomGraphics(const ClientDisplayNative &o,const ClientDisplayInjections &input) noexcept {
  const auto &room=*o.room;const auto &game=*o.game;
  RoomGraphicsKey key{};
  key.config=room._descriptor->Data;
  // Host 资源目录只追加身份；无关 Sprite/图像入目录不改变本房图形依赖，不能触发重建。
  key.stage=Read<unsigned>(&game,0);key.stage_type=Read<unsigned>(&game,4);
  key.difficulty=Read<unsigned>(&game,0x25f40);key.backdrop=room._backdrop.backdropId;
  key.flags=Read<unsigned>(room._descriptor,0x44)&0x1200u;key.room_type=Read<unsigned>(&room,8);
  const auto raw=Read<unsigned>(&game,12),temporary=Read<unsigned>(&game,0x25ac8);
  const auto forced=reinterpret_cast<unsigned (__cdecl *)()>(Base+0x2f55c0)();
  const auto prevented=reinterpret_cast<unsigned (__cdecl *)()>(Base+0x2f5750)();
  key.curse=(raw&2u)|(((raw|temporary|forced)&~prevented&2u)<<1);
  key.width=Read<int>(&room,12);key.height=Read<int>(&room,16);key.bounds=Read<std::array<float,4>>(&room,20);
  key.wall_style=key.backdrop==20?input.backdrop_wall_style:0;
  key.tutorial=input.backdrop_tutorial;key.graphics_flags=input.room_graphics_flags;
  key.ascent=(Read<unsigned>(&game,0x25ac4)&0x10000u)!=0;
  key.special_ray=reinterpret_cast<bool (__cdecl *)()>(Base+0x4194d0)();
  key.water=key.backdrop==45&&(input.water[0]!=0||input.water[1]!=0);
  key.targets=RoomTargets(room);
  return key;
}
// 使用同一个构图条件预判与执行，安装改变墙面、房间标志或资源时会在调用前复核。
bool NeedsRoomGraphics(const ClientDisplayNative &o,bool new_room,const RoomGraphicsKey &key) noexcept {
  return new_room||!RoomResourcesCurrent(o)||key.Values()!=o.room_graphics.Values();
}
// 排空后的唯一退出点。四个Backdrop surface仍由Room持有，回收构图临时引用与内嵌Sprite。
void ReleaseRoomGraphics(ClientDisplayNative &o) {
  o.graphics_ready=o.display_complete=false;
  if(o.fx_owned){
    auto &fx=o.room->_fxLayers;
    ClearFx(fx);
    RoomSprite(*o.room,0x18e0).Reset();
    RoomSprite(*o.room,0x19f4).Reset();
    *reinterpret_cast<KAGE_SmartPointer_ImageBase *>(reinterpret_cast<std::byte *>(&fx)+0x3c)=KAGE_SmartPointer_ImageBase{};
    Write(reinterpret_cast<void *>(Base+0x7ac6d8),0,o.previous_fx_rng);
    o.fx_owned=false;
  }
  if(o.backdrop_entered){
    auto &b=o.room->_backdrop;
    b.floorANM2.Reset();b._wallDetailANM2.Reset();b._controlsANM2.Reset();b._controlsButtonsANM2.Reset();
    b.gfx=KAGE_SmartPointer_ImageBase{};
    b.nlfloorGfx=KAGE_SmartPointer_ImageBase{};
    b.waterGfx=KAGE_SmartPointer_ImageBase{};
    o.backdrop_entered=false;
  }
  if(o.radial_owned){RoomSprite(*o.room,0x7100).Reset();o.radial_owned=false;}
  if(o.hell_owned){RoomSprite(*o.room,0x7770).Reset();Write<std::uint8_t>(o.room,0x7884,0);o.hell_owned=false;}
  if(o.global_images_owned){
    ImageSlot(0x7e7370)=std::move(o.previous_shading);ImageSlot(0x7e7350)=std::move(o.previous_shading_target);
    ImageSlot(0x7e7358)=std::move(o.previous_ray);
    o.global_images_owned=o.shading_target_owned=false;
  }
}
RoomDescriptor *MapDescriptor(Game &g,unsigned slot) noexcept {
  return slot<507?&g._gridRooms[slot]:slot<527&&slot!=525?&g._negativeGridRooms[slot-507]:nullptr;
}
bool MirrorDimension(Game &g) {
  if(g._currentDimensionIdx!=1||(g._stageType!=4&&g._stageType!=5))return false;
  if(g._stage==2)return true;if(g._stage!=1)return false;
  return (reinterpret_cast<unsigned (__thiscall *)(Game *)>(Base+0x342450)(&g)&2u)!=0;
}
// 8254B0的纯几何/scroll部分；所有切房、玩家动作、RNG、选曲及保存分支均不执行。
void TransitionScroll(ClientDisplayNative &o,const ClientDisplayMetadata &next) {
  auto &t=o.game->_roomTransition;Write(&t,0x22c,0.0f);Write(&t,0x230,0.0f);
  if(next.transition_animation!=0)return;float dx=0,dy=0;
  if(o.old_entry>=0&&next.entry>=0&&o.old_dimension==next.dimension){
    const int old_row=o.old_grid/13,new_row=next.grid/13;
    dx=float(13*(next.grid+13*(old_row-new_row)-o.old_grid))*26.0f;
    dy=float(7*(new_row-old_row))*26.0f;
  }else switch(next.transition_direction){case 0:dx=-float(next.width)*26;break;case 1:dy=-float(next.height)*26;break;
    case 2:dx=float(o.old_width)*26;break;case 3:dy=float(o.old_height)*26;break;default:break;}
  if(o.old_mirror)dx=-dx;
  const auto scroll=o.room->_renderScrollOffset;const bool mirror=MirrorDimension(*o.game);
  switch(next.transition_direction){
    case 0:Write(&t,0x22c,(mirror?o.old_scroll.x-scroll.x:scroll.x-o.old_scroll.x)-g_WIDTH-dx);break;
    case 1:Write(&t,0x230,scroll.y-o.old_scroll.y-g_HEIGHT-dy);break;
    case 2:Write(&t,0x22c,(mirror?scroll.x-o.old_scroll.x:o.old_scroll.x-scroll.x)-g_WIDTH+dx);break;
    case 3:Write(&t,0x230,o.old_scroll.y-scroll.y-g_HEIGHT+dy);break;
    default:break;
  }
}
// 当前世界已安装后，Q8可重写标量；不再调用生产颜色、fade、水色或冲击波的Update。
void Reapply(ClientDisplayNative &o) {
  for(const auto &v:ClientDisplayCopies(o.installed)){
    void *root=v.root==DisplayRoot::Game?static_cast<void *>(o.game):v.root==DisplayRoot::Room?
      static_cast<void *>(o.room):static_cast<void *>(o.room->_descriptor);
    std::memcpy(static_cast<std::byte *>(root)+v.offset,v.data,v.size);}
}
// J273 0x980130 的纯显示绑定：保留原生主身体／双子选择，只用 C/S 组槽替代设备查找。
// 不调用整段 AssignPlayerHUDs：该函数还会重写合作选择表，并查询原版 NetInputDevice。
bool DescribePlayerHud(const Game &game,std::array<Entity_Player *,8> &players) {
  const auto primary=[](const Entity_Player &p){
    const auto *t=static_cast<const Entity_Player *>(p._twinPlayer._ref);
    return !t||t==&p||t->_playerIndex==-1||t->_playerIndex>=p._playerIndex;
  };
  for(auto *p:game._playerManager._playerList){
    if(!p||!p->_exists||p->_parent._ref||!primary(*p))continue;
    unsigned slot=0;
    if(!ClientBinaryPlayerOrdinal(p,slot))return false;
    players[slot]=p;
    auto *t=static_cast<Entity_Player *>(p->_twinPlayer._ref);
    if(t&&t->_exists&&t->_playerType!=40&&
       ((t->_playerType!=29&&t->_playerType!=38)||primary(*t))){
      players[slot+4]=t;
    }
  }
  return true;
}
bool BindPlayerHud(ClientDisplayNative &o) {
  std::array<Entity_Player *,8> players{};
  if(!DescribePlayerHud(*o.game,players))return false;
  for(unsigned i=0;i<players.size();++i){
    auto &hud=o.game->_hud._playerHUD[i];
    o.Save(&hud,0,sizeof(hud._player));
    if(hud._player==players[i])continue;
    // 仅身份变化时清理旧图标／心形缓存；热帧保留原生倒计时与资源引用。
    reinterpret_cast<void (__thiscall *)(PlayerHUD *)>(Base+0x4326c0)(&hud);
    hud._player=players[i];
    Write<std::uint16_t>(&hud,0x6b0,257);
  }
  return true;
}
bool StopAudio(ClientDisplayNative &o);
void RefreshLoopMix(ClientDisplayNative &o);
void Sort(Room &room) {
  auto &list=room._entityList._renderEL;
  if(list._size>4096||list._size>list._capacity||(list._size&&!list._data))throw std::runtime_error("display_render_list_bounds");
  struct Key final {Entity *entity;unsigned z,index;};std::array<Key,4096> keys{};
  for(unsigned i=0;i<list._size;++i){auto *e=list._data[i];if(!e)throw std::runtime_error("display_render_null_member");
    auto **vtable=*reinterpret_cast<void ***>(e);const auto z=reinterpret_cast<unsigned (__thiscall *)(Entity *)>(vtable[13])(e);
    // 已提交原生绑定提供 Host 顺序，原生索引供本地成员使用。
    std::uint32_t order=e->_index;
    ClientBinaryDrawOrder(e,order);keys[i]={e,z,order};}
  std::sort(keys.begin(),keys.begin()+list._size,[](const auto &a,const auto &b){return a.z!=b.z?a.z<b.z:a.index<b.index;});
  for(unsigned i=0;i<list._size;++i)list._data[i]=keys[i].entity;
}
void Cursor(Manager &m) {
  if(!m._optionsConfig._enableMouseControl||!m._cursorSprite._loaded)return;
  // 使用已泵过事件的本机窗口鼠标状态；不再调用平台输入更新或消费canonical动作。
  if(!__ptr_g_KAGE_Graphics_Manager||!__ptr_g_KAGE_Graphics_Manager->_window)return;
  void *window=__ptr_g_KAGE_Graphics_Manager->_window;
  if(!Read<std::uint8_t>(reinterpret_cast<void *>(Base+0x7e65a5),0))return;
  // 9F26E0使用ECX窗口、EDX键号，会消费过渡态3；本阶段只在唯一输入泵之后调用一次。
  const auto clicked=reinterpret_cast<int (__fastcall *)(void *,unsigned)>(Base+0x5f26e0)(window,0);
  m._cursorSprite.Play(clicked?"Clicked":"Idle",false);
  const float x=float(Read<double>(window,0x48)),y=float(Read<double>(window,0x50));
  if(!std::isfinite(x)||!std::isfinite(y))return;
  auto idle=Read<int>(&m,0x4afa4);
  if(x!=Read<float>(&m,0x4af9c)||y!=Read<float>(&m,0x4afa0))idle=300;else if(idle>0)--idle;
  Write(&m,0x4af9c,x);Write(&m,0x4afa0,y);Write(&m,0x4afa4,idle);
}
}
ClientDisplayNative *ClientDisplayCreate(const ClientDisplayWorldRef &target) noexcept {
  if(!MainWindow()||TopOwner()!=Owner::Manager||!ClientDisplayMemoryHooksReady())return nullptr;
  auto memory=ClientDisplayMemory(target);
  constexpr auto bytes=sizeof(ClientDisplayNative)+2*81*20+128;
  try{ClientDisplayCharge(memory,bytes);}catch(...){return nullptr;}
  try{auto *o=new ClientDisplayNative;o->memory=std::move(memory);return o;}
  catch(...){ClientDisplayUncharge(memory,bytes);return nullptr;}
}
void ClientDisplayDestroy(ClientDisplayNative *o) noexcept {
  if(o&&IsMain()&&!o->written&&!ClientDisplayAudioHeld(o)&&!o->owned_camera&&!o->root_uncertain){
    if(active==o)active=nullptr;auto memory=o->memory;delete o;
    ClientDisplayUncharge(memory,sizeof(ClientDisplayNative)+2*81*20+128);
  }
}
ClientNativeRoots ClientDisplayRoots(const ClientDisplayNative *o) noexcept {
  if(!o||!IsMain()||g_Game!=o->game||!o->game||o->game->_room!=o->room||!o->room)return {};
  return {o->game,o->room,o->room->_descriptor,o->room->_Camera};
}
void ClientDisplayAbortPending(ClientDisplayNative *o) noexcept {
  if(!o)return;
  o->pending.reset();
  if(!o->written)o->saved_count=0;
}
bool ClientDisplayPreserveRoom(ClientDisplayNative *o,const ClientNativeRoots &r) noexcept {
  if(!o||!o->installed||o->rendering)return false;
  try{
    // 同一图形代次且确属待退出房间的完整画面，可安全跨越已取消的原生写票。
    const auto installed=ClientDisplayMeta(o->installed);
    if(HistoryFitsViewport(*o)&&g_Game==o->game&&o->captured_scope.scene==installed.scope.scene&&
       !Read<std::uint8_t>(o->game,0x25aac))return true;
    if(!Roots(*o,r)||!o->room->_descriptor->Data)return false;
    if(!Capture(*o,true))return false;
    const auto *data=o->room->_descriptor->Data;
    o->old_scroll=o->room->_renderScrollOffset;o->old_surface=true;o->new_surface=false;
    o->old_grid=o->room->_descriptor->GridIndex;o->old_entry=int(o->game->_currentRoomIdx);o->old_dimension=int(o->game->_currentDimensionIdx);
    o->old_width=data->Width;o->old_height=data->Height;o->old_mirror=MirrorDimension(*o->game);
    o->captured_old=*reinterpret_cast<const KAGE_SmartPointer_ImageBase *>(
      reinterpret_cast<const std::byte *>(&o->game->_roomTransition)+0x28);
    o->captured_context=wglGetCurrentContext();
    o->captured_scope=installed.scope;
    o->captured_width=int(g_WIDTH);o->captured_height=int(g_HEIGHT);
#ifdef ISAACCS_E2E
    if(replica::stream::E2ETraceEnabled())
      Trace("e39-surface","side=old grid=%d dimension=%d width=%d height=%d",o->old_grid,o->old_dimension,o->captured_width,o->captured_height);
#endif
    return true;
  }catch(...){return false;}
}
namespace {
// 借用原生转场的混合和着色器，提交后恢复，外层 Manager 仍负责交换缓冲。
struct TransitionBlend final {
  using Setter=void (__thiscall *)(KAGE_Graphics_Manager *,BlendMode);
  KAGE_Graphics_Manager &graphics=*__ptr_g_KAGE_Graphics_Manager;
  Setter set=reinterpret_cast<Setter>(Read<void *const *>(&graphics,0)[14]);
  BlendMode previous=graphics._blendMode;
  KAGE_Graphics_Shader *shader=graphics._shader;
  ~TransitionBlend(){graphics._shader=shader;set(&graphics,previous);}
};
DestinationQuad ScreenQuad() noexcept {
  const float width=g_WIDTH,height=g_HEIGHT;
  return {Vector(0,0),Vector(width,0),Vector(0,height),Vector(width,height)};
}
bool DrawOldRoom(const ClientDisplayNative &o) {
  if(!HistoryFitsViewport(o)||!FlushDisplay())return false;
  TransitionBlend blend;
  blend.set(&blend.graphics,BlendMode(0,1,0,1,0));
  blend.graphics._shader=nullptr;
  const auto quad=ScreenQuad();
  const KColor white(1,1,1,1);
  auto *image=o.captured_old.image;
  using DrawQuad=void (__thiscall *)(KAGE_Graphics_ImageBase *,const DestinationQuad *,const KColor *);
  reinterpret_cast<DrawQuad>(Read<void *const *>(image,0)[7])(image,&quad,&white);
  return FlushDisplay();
}
bool DrawDoorFade(const ClientDisplayNative &o,const DoorFadeFrame &frame,int native_mode=0) {
  if(frame.draw_old_room&&!DrawOldRoom(o))return false;
  if(frame.opacity>0){
    if(!FlushDisplay())return false;
    TransitionBlend blend;
    // J273 RoomTransition::Render 的纯色四边形入口（0x827BA4），覆盖当前原生绘制目标。
    auto *shape=ImageSlot(0x7e743c).image;
    if(!shape)return false;
    blend.set(&blend.graphics,BlendMode(0,Read<bool>(&blend.graphics,0x23)?1:6,7,1,7));
    blend.graphics._shader=reinterpret_cast<KAGE_Graphics_Shader *>(Base+0x7ae1d8);
    const auto quad=ScreenQuad();
    const KColor black(0,0,0,frame.opacity);
    using DrawSolid=void (__thiscall *)(KAGE_Graphics_ImageBase *,const DestinationQuad *,
      const KColor *,const KColor *,const KColor *,const KColor *);
    reinterpret_cast<DrawSolid>(Read<void *const *>(shape,0)[6])(shape,&quad,&black,&black,&black,&black);
    if(!FlushDisplay())return false;
  }
#ifdef ISAACCS_E2E
  if(frame.phase&&replica::stream::E2ETraceEnabled())
    Trace("e39-door-fade","phase=%s opacity=%u source=%llu destination=%llu grid=%d old_image=%u mode=%d",
      frame.phase,unsigned(frame.opacity*1000+0.5f),o.door_fade.source_scene,o.door_fade.destination_scene,
      o.installed?ClientDisplayMeta(o.installed).grid:o.old_grid,unsigned(frame.draw_old_room),native_mode);
#else
  (void)native_mode;
#endif
  return true;
}
}
// 等待同局新房时沿用旧图，普通门淡出后保持黑屏，仍由原生 Manager 提交。
bool ClientDisplayRenderHistory(ClientDisplayNative *o) noexcept {
  if(!o||!MainWindow()||TopOwner()!=Owner::Render||!Graphics()||g_Manager!=o->manager||!HistoryFitsViewport(*o))return false;
  try{
    const auto frame=ReadDoorFade(*o,false,true);
    const bool drawn=frame.phase?DrawDoorFade(*o,frame):DrawOldRoom(*o);
    if(!drawn)return false;
#ifdef ISAACCS_E2E
    if(!frame.phase&&replica::stream::E2ETraceEnabled())
      Trace("e39-held-draw","scene=%llu grid=%d dimension=%d width=%d height=%d manager_state=%d",
        o->captured_scope.scene,o->old_grid,o->old_dimension,o->captured_width,o->captured_height,g_Manager->_state);
#endif
    return true;
  }catch(...){return false;}
}
static ClientDisplayStep BuildDisplayRoots(ClientDisplayNative *o,const ClientNativeRoots &r) noexcept {
  if(!o||!MainWindow()||TopOwner()!=Owner::Manager||!Graphics()||r.game!=g_Game||!g_Game||r.room!=g_Game->_room||
     !r.room||!r.descriptor||o->root_uncertain)return {false,false,"display_root_window"};
  if(o->hud_loaded)return {o->room==r.room&&r.camera==o->owned_camera,false,"display_root_current"};
  if(r.camera||g_Manager->_state!=1)return {false,false,"display_cold_root_occupied"};
  o->game=g_Game;o->room=g_Game->_room;o->manager=g_Manager;
  // HUD与全局surface沿用本机游戏图形owner。Camera在本房的实装输入就绪后创建。
  try{
    o->root_uncertain=true;
    DisplayScope scope;
    o->game->_hud.LoadGraphics();o->hud_loaded=true;
    o->root_uncertain=false;
    // 复用 Room 正常构造好的两份灯光 ANM2，只在冷场景加载所需图层。
    constexpr unsigned light_offsets[]={0x6e40,0x6f54};
    constexpr const char *light_paths[]={"gfx/LightGradient.anm2","gfx/SpotLight.anm2"};
    for(unsigned i=0;i<2;++i){
      auto *sprite=reinterpret_cast<ANM2 *>(reinterpret_cast<std::byte *>(o->room)+light_offsets[i]);
      if(!sprite->_loaded){
        // 先登记接管，再进原生加载；异常前缀保持不可退休，分配记录继续持有 Peer 账户。
        o->owned_lights[i]=sprite;
        o->root_uncertain=true;
        std::string path=light_paths[i];sprite->Load(path,true);
        o->root_uncertain=false;
      }
      auto *layer=sprite->_loaded?sprite->GetLayer(0):nullptr;
      if(!layer){o->root_uncertain=false;return {false,true,"display_light_resource_missing"};}
      Write(layer,0x20,1u);Write(layer,0x24,1u);
      if(i==0)Write(layer,0x78,std::array<unsigned,5>{0,1,7,1,7});
      const char *animation=i==0?"Default":"Idle";
      if(!sprite->_animState._animData)sprite->Play(animation,true);
      if(!sprite->_animState._animData||!layer->_spriteSheetLoaded)return {false,true,"display_light_binding"};
    }
    o->root_uncertain=false;
    if(!FlushDisplay())return {false,true,"display_roots_cpu_borrow"};
    return {true,true,"display_roots_ready"};
  }catch(...){return {false,true,"display_root_construction_exception"};}
}
bool ClientDisplayReleaseRoots(ClientDisplayNative *o) noexcept {
  if(!o||(!o->owned_camera&&!o->hud_loaded&&!o->fx_owned&&!o->backdrop_entered&&!o->global_images_owned))return !o||!o->root_uncertain;
  if(o->root_uncertain||o->written||!MainWindow()||!Graphics()||g_Game!=o->game||g_Manager!=o->manager||
     o->game->_room!=o->room||o->room->_Camera!=o->owned_camera)return false;
  try{
    o->root_uncertain=true;
    ReleaseRoomGraphics(*o);
    if(o->hud_loaded){
      reinterpret_cast<void (__thiscall *)(HUD *)>(Base+0x57ab10)(&o->game->_hud);
      reinterpret_cast<void (__thiscall *)(HUD *)>(Base+0x579d80)(&o->game->_hud);
      o->hud_loaded=false;
    }
    for(auto *&light:o->owned_lights)if(light){light->destructor();light->constructor();light=nullptr;}
    ReleaseCamera(*o);
    o->root_uncertain=false;
    o->failed=false;
    return true;
  }catch(...){return false;}
}
ClientDisplayStep ClientDisplayBeforeInstall(ClientDisplayNative *o,const ClientDisplayWorldRef &target,const ClientNativeRoots &r) noexcept {
  if(!o||!target||ClientDisplayMemory(target)!=o->memory||!MainWindow()||TopOwner()!=Owner::Manager||o->rendering||o->failed)
    return {false,false,"display_prepare_owner"};
  bool capture_started=false;
  try{
    const auto meta=ClientDisplayMeta(target);
    const bool transition_started=o->installed&&meta.transition_mode!=0&&ClientDisplayMeta(o->installed).transition_mode==0;
    const bool room_changed=o->installed&&ChangedRoom(o->installed,target);
    if(o->installed&&(room_changed||transition_started)&&RoomResourcesCurrent(*o)){
      if(!Roots(*o,r)||!o->room->_descriptor->Data)return {false,false,"display_old_world_binding"};
      capture_started=true;
      if(!ClientDisplayPreserveRoom(o,r)){o->failed=true;if(active==o)active=nullptr;return {false,true,"display_old_surface_capture"};}
      if(room_changed)o->new_surface=false;
    }
    if(o->installed&&!RoomResourcesCurrent(*o))ForgetRoomHistory(*o);
    o->previous_shake=g_Game?Read<int>(g_Game,0x25a80):0;
    o->pending=target;return {true,capture_started,"display_prepared"};
  }catch(...){if(capture_started){o->failed=true;if(active==o)active=nullptr;}else if(!o->written)o->saved_count=0;
    return {false,capture_started,"display_prepare_exception"};}
}
ClientDisplayStep ClientDisplayInstall(ClientDisplayNative *o,const ClientDisplayWorldRef &target,const ClientNativeRoots &r) noexcept {
  if(!o||!target||o->pending!=target||!MainWindow()||TopOwner()!=Owner::Manager||o->failed||o->rendering||
     !r.game||!r.room||!r.descriptor||r.game!=g_Game||r.room!=g_Game->_room||r.descriptor!=g_Game->_room->_descriptor||!Graphics())
    return {false,false,"display_install_owner"};
  try{
    ClientDisplayMemoryScope memory(o->memory);
    const auto d=ClientDisplayMeta(target);auto *descriptor=static_cast<RoomDescriptor *>(r.descriptor);
    if(!d.spawn_seed||!descriptor->Data||descriptor->GridIndex!=d.grid||descriptor->Dimension!=d.dimension)return {false,false,"display_descriptor_binding"};
    if(o->game&&o->game!=r.game)return {false,false,"display_game_replaced"};
    if(o->old_surface&&(wglGetCurrentContext()!=o->captured_context||Read<std::uint8_t>(g_Game,0x25aac)||
       int(g_WIDTH)!=o->captured_width||int(g_HEIGHT)!=o->captured_height)){
      ForgetRoomHistory(*o);
    }
    for(unsigned i=0;i<ClientDisplayMapCount(target);++i){const auto v=ClientDisplayMapAt(target,i);auto *native=MapDescriptor(*g_Game,v.slot);
      if(!native||native->GridIndex!=v.grid||native->Dimension!=v.dimension)return {false,false,"display_map_descriptor_binding"};}
    const auto built=BuildDisplayRoots(o,r);
    if(!built.done)return built;
    const bool map_changed=!ClientDisplaySameMap(o->installed,target);
    const bool new_room=!o->installed||ChangedRoom(o->installed,target);
    o->descriptor=descriptor;
    const auto previous_shake=o->previous_shake;
    // 先保存旧值；后续原生分配逐次预收费。从written起失败必须退休，不能宣称自动回滚。
    for(const unsigned offset:{0x25a84u,0x25a88u,0x25ab8u})o->Save(o->game,offset,offset==0x25ab8?1:4);
    o->Save(o->manager,0x8,4);
    // 保留Ascent及其他状态位。FX、墙面和灯光都可能读取这两个已投影的位。
    o->Save(o->game,0x25ac5,1);
    {o->Save(o->game,0x1aee4,60);o->Save(o->game,0x1af20,15);o->Save(o->room,0x7460,8);o->Save(o->game,0x1adb4,4);}
    // J273 HUD::Render 0x97DC30/0x97DC3E：四个原版合作选择面板仅由两个非负
    // 索引开启。客机从菜单直接安装世界，候选角色表可为空，必须关闭此原版入口。
    // Save 按地址去重，热帧不覆盖接管前快照；失败退休／退出 C/S 时由 Retire 原样恢复。
    for(unsigned slot=0;slot<4;++slot)for(const unsigned member:{0xcu,0x14u})
      o->Save(&o->game->_playerManager,slot*20+member,sizeof(int));
    o->written=true;o->display_complete=false;if(active==o)active=nullptr;
    // 此处只在 MainWindow 验证过的 C/S 客机安装作用域执行，Host 与原版流程不经过。
    for(unsigned slot=0;slot<4;++slot)for(const unsigned member:{0xcu,0x14u})
      Write(&o->game->_playerManager,slot*20+member,-1);
    // 本地已构造C/S游戏承载进入原生游戏显示态；Manager更新仍整体拦截。
    // 音乐的dizzy／红房分支和Manager::Render因此读真实本地游戏态，退休先还原菜单态。
    o->manager->_state=2;
    o->installed=target;
    o->root_uncertain=true;
    const bool hud_bound=BindPlayerHud(*o);
    o->root_uncertain=false;
    if(!hud_bound)return {false,true,"display_player_hud_binding"};
    const auto input=ClientDisplayInputs(target);
    const auto phases=Read<unsigned>(o->game,0x25ac4);
    Write(o->game,0x25ac4,(phases&~0xc000u)|(unsigned(input.room_graphics_flags)<<14));
    Write(o->game,0x1aee4,input.pill_effects);Write(o->game,0x1af20,input.pill_identified);Write(o->room,0x7460,input.water);
    Write(o->game,0x1adb4,input.planetarium);
    if(previous_shake>0&&Read<int>(o->game,0x25a80)==0){Write(o->game,0x25a84,0.0f);Write(o->game,0x25a88,0.0f);}
    const auto old_modifier=o->last_sfx_modifier,modifier=d.sfx;
    o->mixing_dirty=o->mixing_dirty||std::max(old_modifier,0.0f)!=std::max(modifier,0.0f);
    o->last_sfx_modifier=modifier;
    if(!o->have_period||d.frame<o->old_frame){o->have_period=false;Write<std::uint8_t>(o->game,0x25ab8,255);}
    if(!o->have_period||d.frame/30!=o->period){
      o->hud_dirty=true;
      o->period=d.frame/30;o->have_period=true;}
    o->old_frame=d.frame;o->map_dirty=o->map_dirty||map_changed;o->hud_dirty=o->hud_dirty||map_changed;
    // 同M换world使平均位置缓存失效；不伪造实体年龄，也不补调用玩家模拟。
    Write(reinterpret_cast<void *>(Base+offset::PlayerCenterFrame),0,std::uint32_t(o->manager->_framecount-1));
    // 只填补本机生命周期下尚未初始化的图形缓存。
    o->root_uncertain=true;
    // 菜单遗留的 resize 请求会让首次 Game::Render 重建所有屏幕 surface。
    // 在安装期先执行同一原生显示维护，再构图和记录资源身份，避免发布后身份失效。
    if(Read<std::uint8_t>(o->game,0x25aac)){
      reinterpret_cast<void (__thiscall *)(Game *)>(Base+0x303e40)(o->game);
      Write<std::uint8_t>(o->game,0x25aac,0);
    }
    if(!ImageSlot(0x7e7350).image||!ImageSlot(0x7e7360).image)
      reinterpret_cast<void (__cdecl *)()>(Base+0x3e08e0)();
    if(!ImageSlot(0x7e72b4).image)
      reinterpret_cast<void (__cdecl *)()>(Base+0x2efb70)();
    o->root_uncertain=false;
    if(!ImageSlot(0x7e7350).image||!ImageSlot(0x7e7360).image||!ImageSlot(0x7e72b4).image||
       (Read<bool>(o->manager,0x2a1fd)&&!ImageSlot(0x7e7398).image))
      return {false,true,"display_shared_surfaces"};
    auto key=RoomGraphics(*o,input);
    if(key.backdrop>=63)return {false,true,"display_backdrop_id"};
    // 使用本机外观流，保证同房重建稳定。SpawnSeed原位保留给Q9消费者。
    const unsigned seed=std::max(1u,d.spawn_seed^0xa63f218du);
    if(NeedsRoomGraphics(*o,new_room,key)){
      o->graphics_ready=false;
      if(!o->owned_camera||new_room||key.bounds!=o->room_graphics.bounds||
         key.width!=o->room_graphics.width||key.height!=o->room_graphics.height)
        ConstructCamera(*o,input);
      // 原生异常可能留下尚未完成的 owner；只有正常返回才恢复可退休资格。
      o->root_uncertain=true;const bool fx=BuildFx(*o,key,seed);o->root_uncertain=false;
      if(!fx)return {false,true,"display_fx_resources"};
      o->root_uncertain=true;const bool backdrop=BuildBackdrop(*o,key,seed);o->root_uncertain=false;
      if(!backdrop)return {false,true,"display_backdrop_resources"};
      o->root_uncertain=true;const bool background=BuildSpecialBackgrounds(*o,key,seed);o->root_uncertain=false;
      if(!background)return {false,true,"display_background_resources"};
      key.targets=RoomTargets(*o->room);
      Write<std::uint8_t>(o->room,0x11f6,1);
      o->room_graphics=key;
      o->graphics_width=int(g_WIDTH);o->graphics_height=int(g_HEIGHT);
      o->history_surface=ImageSlot(0x7e72b4).image;
      o->graphics_ready=true;
      o->capture_initial=true;
      o->new_surface=false;
    }
    o->root_uncertain=true;const bool poop=PreparePoop(*o->room,input,seed);o->root_uncertain=false;
    if(!poop)return {false,true,"display_poop_resources"};
    Reapply(*o);
    if(!FlushDisplay())return {false,true,"display_install_cpu_borrow"};
    memory.Complete();
    return {true,true,"display_resources_installed",o->capture_initial};
  }catch(...){o->failed=true;ClientDisplaySuspend(o);return {false,o->written||o->hud_loaded,"display_install_exception"};}
}
ClientDisplayStep ClientDisplayFinishInstall(ClientDisplayNative *o,const ClientDisplayWorldRef &target,const ClientNativeRoots &r) noexcept {
  if(!o||o->pending!=target||o->installed!=target||!MainWindow()||TopOwner()!=Owner::Manager||o->failed||
     !Roots(*o,r)||!RoomResourcesCurrent(*o))return {false,false,"display_capture_resource_binding"};
  try{
    const auto d=ClientDisplayMeta(target);
    if(o->old_surface&&d.transition_mode){
      auto &old_target=*reinterpret_cast<KAGE_SmartPointer_ImageBase *>(
        reinterpret_cast<std::byte *>(&o->game->_roomTransition)+0x28);
      if(old_target.image!=o->captured_old.image)old_target=o->captured_old;
    }
    // 中途加入只拥有当前房间。用它建立两张有效本地图，保持 Host 的阶段和滚动方向。
    if((d.transition_mode==2||d.transition_mode==3)&&!o->old_surface&&!ClientDisplayPreserveRoom(o,r)){
      o->failed=true;return {false,true,"display_transition_baseline_surface"};
    }
    if(o->old_surface&&!o->new_surface){
      TransitionScroll(*o,d);
      if(!Capture(*o,false)){o->failed=true;return {false,true,"display_new_surface_capture"};}o->new_surface=true;
#ifdef ISAACCS_E2E
      if(replica::stream::E2ETraceEnabled()){
        const auto &transition=o->game->_roomTransition;
        Trace("e39-surface","side=new grid=%d dimension=%d frame=%u scroll_x=%.2f scroll_y=%.2f",d.grid,d.dimension,d.frame,
          Read<float>(&transition,0x22c),Read<float>(&transition,0x230));
      }
#endif
    }
    // 首帧与任何重新构图都要走一次原生绘制，复用同批已生成的转场新图。
    if(o->capture_initial&&!o->new_surface&&!Capture(*o,false))return {false,true,"display_initial_surface_capture"};
    if(!FlushDisplay()){o->failed=true;return {false,true,"display_install_cpu_borrow"};}
    o->surface_width=int(g_WIDTH);o->surface_height=int(g_HEIGHT);o->pending.reset();o->display_complete=true;o->capture_initial=false;
    return {true,true,"display_installed"};
  }catch(...){o->failed=true;if(active==o)active=nullptr;return {false,o->written,"display_install_exception"};}
}
void ClientDisplayPublish(ClientDisplayNative *o) noexcept {
  if(!o||!o->display_complete||!o->installed)return;
  const auto d=ClientDisplayMeta(o->installed);
  UpdateDoorFade(*o,d);
  if(!d.transition_mode&&(!o->door_fade.source_scene||o->door_fade.rendered_end))ForgetRoomHistory(*o);
  o->door_arrival_event_scene=0;
  active=o;
}
void ClientDisplaySuspend(ClientDisplayNative *o) noexcept {if(active==o)active=nullptr;}
bool ClientDisplayFxRandom(const void *site,unsigned &value) noexcept {
  if(!IsMain()||!ClientBusy()||!fx_build_seed||reinterpret_cast<std::uintptr_t>(site)!=Base+0x568e96)return false;
  value=fx_build_seed;
  return true;
}
bool ClientDisplayRetire(ClientDisplayNative *o,bool preserve_target,bool preserve_audio) noexcept {
  if(!o)return true;if(!MainWindow()||TopOwner()!=Owner::Manager||o->rendering)return false;
  if(active==o)active=nullptr;
  try{
    if(ClientDisplayAudioHeld(o)&&!preserve_audio&&(g_Manager!=o->manager||!StopAudio(*o)))return false;
    if(o->written){
      if(g_Game!=o->game||g_Manager!=o->manager||o->game->_room!=o->room)return false;
      ClearClientFoundHudSlots(&o->game->_hud._statHUD);
      if(o->boss_owned){
        if(o->boss_retire_entered)return false;
        o->boss_retire_entered=true;
        auto &transition=o->game->_roomTransition;
        // 827CA0 是正常演出重建时使用的 map 节点析构，包含各 ANM2 的原生清理。
        struct MapHead {MapHead *left,*parent,*right;};
        auto **head=reinterpret_cast<MapHead **>(&transition._extraLayerANM2s);
        if(!*head)return false;
        reinterpret_cast<void (__thiscall *)(void *,void *,MapHead *)>(Base+0x427ca0)(
          &transition._extraLayerANM2s,&transition._extraLayerANM2s,(*head)->parent);
        (*head)->left=(*head)->parent=(*head)->right=*head;
        Write(&transition._extraLayerANM2s,4,0u);
        transition._versusScreenANM2.destructor();transition._versusScreenANM2.constructor();
        o->boss_owned=o->boss_retire_entered=false;
      }
      const auto modifier=Read<float>(o->manager,0x2a170);
      // 先撤树借用，再允许 Game／Seeds 或 Room 资源 owner 析构。
      for(unsigned i=o->saved_count;i;--i){
        const auto &s=o->saved[i-1];
        if(s.root==o->game&&s.offset==0x25ac5){
          const auto current=Read<std::uint8_t>(s.root,s.offset);
          Write<std::uint8_t>(s.root,s.offset,(current&0x3fu)|(std::to_integer<unsigned>(s.bytes[0])&0xc0u));
        }else std::memcpy(static_cast<std::byte *>(s.root)+s.offset,s.bytes.data(),s.size);
      }
      if(!preserve_target&&std::max(modifier,0.0f)!=std::max(Read<float>(o->manager,0x2a170),0.0f)){
        DisplayScope scope;Named(ClientEntry::DisplaySfxRefresh,[&]{o->manager->_sfxManager.ClearVolumeModifier();});
      }
    }
    o->written=false;o->saved_count=0;o->installed.reset();o->descriptor=nullptr;o->boss_generation=0;o->display_complete=false;
    o->new_surface=false;
    if(!preserve_audio)ForgetRoomHistory(*o);
    else if(o->door_fade.destination_scene)o->door_fade={};
    o->door_arrival_event_scene=0;
    if(!o->owned_camera&&!o->hud_loaded)o->room=nullptr;
    if(!preserve_target)o->pending.reset();
    return true;
  }catch(...){o->failed=true;return false;}
}
bool ClientDisplayHasWorld(const ClientDisplayNative *o) noexcept {return o&&bool(o->installed);}
bool ClientDisplayGraphicsChanged(const ClientDisplayNative *o) noexcept {
  return o&&o==active&&o->installed&&g_Game==o->game&&o->game->_room==o->room&&o->graphics_ready&&
    (Read<std::uint8_t>(o->game,0x25aac)||int(g_WIDTH)!=o->graphics_width||int(g_HEIGHT)!=o->graphics_height||RoomTargets(*o->room)!=o->room_graphics.targets||
     ImageSlot(0x7e72b4).image!=o->history_surface);
}
bool ClientDisplayCurrent(const ClientDisplayNative *o,const ClientNativeRoots &r,ClientDisplayIssue *issue) noexcept {
  if(!MainWindow())return DisplayMismatch(issue,"display_window_changed");
  if(!o||o!=active)return DisplayMismatch(issue,"display_owner_changed",Address(o),Address(active));
  if(o->failed||!o->installed)return DisplayMismatch(issue,"display_owner_not_installed");
  if(!Roots(*o,r)){
    if(!o->game||g_Game!=o->game||r.game!=o->game)
      return DisplayMismatch(issue,"display_game_changed",Address(o->game),Address(g_Game!=o->game?g_Game:r.game));
    if(!o->manager||g_Manager!=o->manager)return DisplayMismatch(issue,"display_manager_changed",Address(o->manager),Address(g_Manager));
    if(!o->room||r.room!=o->room||o->game->_room!=o->room)
      return DisplayMismatch(issue,"display_room_changed",Address(o->room),Address(o->game->_room!=o->room?o->game->_room:r.room));
    if(!o->descriptor||r.descriptor!=o->descriptor||o->room->_descriptor!=o->descriptor)
      return DisplayMismatch(issue,"display_descriptor_changed",Address(o->descriptor),
        Address(o->room->_descriptor!=o->descriptor?o->room->_descriptor:r.descriptor));
    return DisplayMismatch(issue,"display_camera_changed",Address(o->owned_camera?o->owned_camera:o->room->_Camera),Address(r.camera));
  }
  if(!RoomResourcesCurrent(*o,issue))return false;
  const auto expected=ClientDisplayMeta(o->installed).frame,actual=Read<std::uint32_t>(o->game,0x25a70);
  return expected==actual||DisplayMismatch(issue,"display_game_frame_changed",expected,actual,UINT32_MAX,0x25a70);
}
void ClientDisplayReportIssue(const ClientDisplayNative *o,const ClientDisplayIssue &issue,const char *phase,
    std::uint64_t route,std::uint64_t scene,std::uint32_t revision) noexcept {
  if(!issue.reason)return;
  if(o)ClientDisplayRecordMemoryIssue(o->memory,issue,phase,route,scene,revision);
  Trace("client_error","event=binary_display_witness route=%llu scene=%llu revision=%u phase=%s reason=%s expected=%llu actual=%llu resource=%u field=%u",
    route,scene,revision,phase,issue.reason,std::uint64_t(issue.expected),std::uint64_t(issue.actual),issue.resource,unsigned(issue.field));
}
bool ClientDisplayPaused(const ClientDisplayNative *o,bool &v) noexcept {
  if(!o||o!=DisplaySource()||o->failed||!o->installed||!IsMain())return false;v=ClientDisplayMeta(o->installed).paused;return true;
}
bool ClientNativeDisplayActive() noexcept{const auto *source=DisplaySource();return IsMain()&&source&&source->installed&&!source->failed&&ClientBusy();}
#ifdef ISAACCS_E2E
// 对照提交列表与实际 Game::Render 的原生输入。每个血效种类只记录首个实例变化。
void TraceBinaryBloodRender(void *pointer,const char *role) noexcept {
  if(!replica::stream::E2ETraceEnabled())return;
  auto *game=static_cast<Game *>(pointer);
  if(!game||game!=g_Game||!game->_room)return;
  const auto &list=game->_room->_entityList._renderEL;
  if(list._size>4096||list._size>list._capacity||(list._size&&!list._data))return;
  constexpr std::array<unsigned,3> variants{2,3,5};
  static std::array<unsigned,variants.size()> prior_seeds{};
  std::array<unsigned,variants.size()> seeds{};
  for(unsigned i=0;i<list._size;++i){
    const auto *entity=list._data[i];
    if(!entity||entity->_type!=1000)continue;
    const auto match=std::find(variants.begin(),variants.end(),entity->_variant);
    if(match==variants.end())continue;
    const auto n=std::size_t(match-variants.begin());
    if(seeds[n])continue;
    seeds[n]=entity->_initSeed;
    if(seeds[n]==prior_seeds[n])continue;
    const auto &sprite=entity->_sprite;
    Trace("binary_blood_render_e2e_fault_probe","side=%s frame=%d variant=%u seed=%u visible=%u loaded=%u animation=%u pose=%.1f alpha=%.3f scale=%.3f/%.3f layers=%u",
      role,game->_frameCount,entity->_variant,seeds[n],unsigned(entity->_visible),unsigned(sprite._loaded),
      unsigned(sprite._animState._animData!=nullptr),double(sprite._animState._animFrame),double(sprite._color._tint[3]),
      double(sprite._scale.x),double(sprite._scale.y),sprite._layerCount);
  }
  prior_seeds=seeds;
}
#endif
ClientProjectileRenderScope::ClientProjectileRenderScope(void *pointer) noexcept {
  if(!ClientNativeDisplayActive())return;
  auto *entity=static_cast<Entity *>(pointer);
  unsigned order=0;
  if(!entity||!g_Game||!g_Game->_room||entity->_type!=9||!ClientBinaryDrawOrder(entity,order)){
    ready_=false;ClientCallbackFault("projectile_render_unbound");return;
  }
  projectile_=entity;room_=g_Game->_room;
  vptr_=Read<unsigned>(entity,0);
  index_=entity->_index;birth_=Read<unsigned>(entity,0x328);
  seed_=entity->_initSeed;bits_=Read<unsigned>(entity,0x350);
}
ClientProjectileRenderScope::~ClientProjectileRenderScope() noexcept {
  if(!projectile_||!returned_)return;
  auto *entity=static_cast<Entity *>(projectile_);
  unsigned order=0;
  if(!ContextHealthy()||!g_Game||g_Game->_room!=room_||!ClientBinaryDrawOrder(entity,order)||
     Read<unsigned>(entity,0)!=vptr_||entity->_type!=9||unsigned(entity->_index)!=index_||
     Read<unsigned>(entity,0x328)!=birth_||entity->_initSeed!=seed_){
    ClientCallbackFault("projectile_render_identity_changed");return;
  }
  std::memcpy(static_cast<std::byte *>(projectile_)+0x350,&bits_,sizeof(bits_));
}
bool ClientNativeDisplayPaused(bool &v) noexcept{return ClientDisplayPaused(DisplaySource(),v);}
bool ClientNativeDisplayCallbacks() noexcept{return IsMain()&&ClientBusy()&&display_depth!=0;}
bool ClientDisplayBossAudioSuppressed(const void *site) noexcept {
  const auto caller=reinterpret_cast<std::uintptr_t>(site);
  return boss_replay&&(caller==Base+0x423d91||caller==Base+0x423da2);
}
bool ClientDisplayDealChance(const void *room,float &value) noexcept {
  const auto *o=DisplaySource();
  if(!ClientNativeDisplayCallbacks()||!o||!o->installed||o->room!=room)return false;
  value=ClientDisplayInputs(o->installed).devil;return true;
}
bool ClientDisplayAngelShare(float &value) noexcept {
  const auto *o=DisplaySource();
  if(!ClientNativeDisplayCallbacks()||!o||!o->installed||o->game!=g_Game)return false;
  value=ClientDisplayInputs(o->installed).angel;return true;
}
bool ClientDisplayDealQualified(const void *game,bool &value) noexcept {
  const auto *o=DisplaySource();
  if(!hud_post_depth||!ClientNativeDisplayCallbacks()||!o||!o->installed||o->game!=game)return false;
  value=ClientDisplayInputs(o->installed).deal_qualified;return true;
}
bool ClientDisplayProgressBlocked(const void *site,bool &value) noexcept {
  const auto caller=reinterpret_cast<std::uintptr_t>(site);
  const auto *o=DisplaySource();
  if(!ClientNativeDisplayCallbacks()||!o||!o->installed||caller<Base+0x57c300||caller>=Base+0x57d000)return false;
  value=ClientDisplayInputs(o->installed).progress_blocked;return true;
}
void ClientDisplaySfxRefreshed(void *owner) noexcept {
  if(!ClientNativeDisplayActive()||!g_Manager||owner!=reinterpret_cast<std::byte *>(g_Manager)+0x2a160)return;
  auto *source=DisplaySource();source->last_sfx_option=g_Manager->_optionsConfig._sfxVolume;source->mixing_dirty=false;
}
bool ClientDisplayAdvance(ClientDisplayNative *o,const ClientNativeRoots &r,bool local_blocked,
                          const ClientDisplayCameraHook &camera_hook) noexcept {
  if(!ClientDisplayCurrent(o,r)||TopOwner()!=Owner::Manager||!Graphics())return false;
  try{
    auto &m=*o->manager;const auto frame=m._framecount;
    const bool hold=Read<std::uint8_t>(&m,0x4a8c0)&&!Read<std::uint8_t>(&m,0x4a8c1);
    if(!m._optionsConfig._enableInterpolation&&(frame&1)){if(!hold)++m._framecount;return true;}
    if(hold)return true;
    DisplayScope scope;auto &g=*o->game;auto &room=*o->room;
    if(o->hud_dirty&&Read<int>(&g,0x22e94)!=2){Write(&g,0x22e94,1);Write(&g,0x22e9c,2);o->hud_dirty=false;}
    if(!(frame&1)&&!local_blocked){
      const auto branch=ClientDisplayMeta(o->installed).branch;
      using Branch=ClientDisplayBranch;
      const auto post_hud=[&]{
        Named(ClientEntry::DisplayHudPostUpdate,[&]{
          HudPostScope input_scope;
          g._hud.PostUpdate();
        });
      };
      if(branch!=Branch::ConsoleHold){Named(ClientEntry::DisplayHudUpdate,[&]{g._hud.Update();});
        if(Read<float>(&g,0x25b28)>0)post_hud();
        if(branch==Branch::Normal||branch==Branch::RoomTransition){
          if(branch==Branch::RoomTransition)Named(ClientEntry::DisplayTransition,[&]{auto &t=g._roomTransition;if(t._unkStartRoomTransitionCond){t._versusScreenANM2.Update();
              for(auto &entry:t._extraLayerANM2s)entry.second.Update();}
            if(t._mode==2||t._mode==3){Write(&g,0x22e94,2);Write(&g,0x22e9c,2);}});
          Named(ClientEntry::DisplayMinimapUpdate,[&]{
            ClientMinimapInputScope input_scope;
            g._minimap.Update();
          });
          if(o->map_dirty){Named(ClientEntry::DisplayMinimapRefresh,[&]{g._minimap.Refresh();});o->map_dirty=false;}
          if(branch==Branch::Normal){Named(ClientEntry::DisplaySort,[&]{Sort(room);});
            Named(ClientEntry::DisplayFxUpdate,[&]{
              ClientDisplayMemoryScope memory(o->memory);o->root_uncertain=true;
              reinterpret_cast<void (__thiscall *)(FXLayers *,bool)>(Base+0x56bf40)(&room._fxLayers,false);
              memory.Complete();o->root_uncertain=false;
            });
            Named(ClientEntry::DisplayBackground,[&]{auto &floor=room._backdrop.floorANM2;floor.Update();
              // 7F93D8..7F9449：26号背景保持Idle；不调用Room::Update相邻的玩法段。
              const auto *animation=floor._animState._animData;
              if(Read<int>(&room,0x1d18)==26&&(!animation||animation->_name!="Idle"||
                 (Read<std::uint8_t>(animation,0x34)&&!floor._animState._isPlaying)))floor.Play("Idle",true);
              auto *hell=reinterpret_cast<std::byte *>(&room)+0x776c;
              if(Read<std::uint8_t>(hell,0x118))reinterpret_cast<ANM2 *>(hell+4)->Update();});}
          post_hud();
        }else if(branch==Branch::PauseCountdown||branch==Branch::StageTransition||branch==Branch::ActiveOverlay||branch==Branch::SecondaryFade)
          post_hud();
      }
    }
    const bool minecart=g._roomTransition._mode!=0&&Read<int>(&g,0x1b858)==19;
    if(!local_blocked&&(!ClientDisplayMeta(o->installed).paused||minecart)&&room._Camera)Named(ClientEntry::DisplayCamera,[&]{
      // 相机读到的是覆盖层的显示位置；作用域对象保证相机返回或抛出后写回权威值。
      struct CameraOverlay final {
        const ClientDisplayCameraHook &hook;bool on;
        explicit CameraOverlay(const ClientDisplayCameraHook &h) noexcept:hook(h),on(h.enter&&h.exit&&h.enter(h.context)){}
        ~CameraOverlay(){if(on)hook.exit(hook.context);}
      } overlay(camera_hook);
      if(m._optionsConfig._cameraStyle==2){room._Camera->UpdateDrag2();if(frame&1)Write<std::uint8_t>(room._Camera,0x8c,0);}
      else{room._Camera->UpdateUltrasmooth(bool(frame&1));Write<std::uint8_t>(room._Camera,0x8c,0);}});
    // 暂停仅阻断相机；音乐仍按每个有效尾部周期一次，不随到包量补跑。
    if(o->last_sfx_option!=m._optionsConfig._sfxVolume){o->mixing_dirty=true;o->last_sfx_option=m._optionsConfig._sfxVolume;}
    if(o->mixing_dirty){Named(ClientEntry::DisplaySfxRefresh,[&]{
      m._sfxManager.ClearVolumeModifier();
      RefreshLoopMix(*o); // 原生 Clear 按 sound 混音，随后归还各样本自己的逻辑音量。
    });o->mixing_dirty=false;}
    Named(ClientEntry::DisplayMusic,[&]{reinterpret_cast<void (__thiscall *)(Music *)>(Base+0x3d7da0)(&m._musicmanager);});
    Named(ClientEntry::DisplayCursor,[&]{Cursor(m);});++m._framecount;return true;
  }catch(...){o->failed=true;if(active==o)active=nullptr;return false;}
}
bool ClientDisplayRenderBegin(ClientDisplayNative *o,const ClientNativeRoots &r) noexcept {
  if(!ClientDisplayCurrent(o,r)||TopOwner()!=Owner::Render||!Graphics()||o->rendering||!o->room->_descriptor->SpawnSeed)return false;
  // 活跃转场resize丢失旧画面时明确停止该显示事务，等待完整恢复；不画空surface。
  if(o->game->_roomTransition._mode&&(int(g_WIDTH)!=o->surface_width||int(g_HEIGHT)!=o->surface_height||Read<std::uint8_t>(o->game,0x25aac))){
    o->failed=true;active=nullptr;return false;}
  const auto scene=ClientDisplayMeta(o->installed).scope.scene;
  const auto &fade=o->door_fade;
  o->render_door_fade={};
  if(fade.source_scene==scene||fade.destination_scene==scene){
    if(!fade.rendered_end)o->render_door_fade=ReadDoorFade(*o,fade.destination_scene==scene,false);
    // 仅在原生绘制调用内选择正常房间分支，返回后归还 Host 的转场模式。
    auto &mode=o->game->_roomTransition._mode;
    if(mode==2||mode==3){o->borrowed_transition_mode=mode;mode=0;}
  }
  o->rendering=true;++display_depth;return true;
}
bool ClientDisplayRouteRender(ClientDisplayNative *o,bool begin) noexcept {
  if(!o||!IsMain()||!ClientBusy()||TopOwner()!=Owner::Render||g_Manager!=o->manager)return false;
  if(begin){if(o!=active||o->failed||o->routed)return false;
    // 状态由world owner安装，不在每次Render借换；此处只固定原生分派窗口。
    if(o->manager->_state!=2)return false;o->routed=true;return true;}
  if(!o->routed)return false;o->routed=false;return true;
}
void ClientDisplayRenderEnd(ClientDisplayNative *o,const ClientNativeRoots &r) noexcept {
  if(!o||!o->rendering)return;o->rendering=false;if(display_depth)--display_depth;
  if(o->borrowed_transition_mode&&g_Game==o->game)o->game->_roomTransition._mode=o->borrowed_transition_mode;
  const auto native_mode=std::exchange(o->borrowed_transition_mode,0);
  const auto frame=std::exchange(o->render_door_fade,DoorFadeFrame{});
  if(!Roots(*o,r)){Trace("client_error","event=display_render_roots_changed");o->failed=true;if(active==o)active=nullptr;return;}
  // RE-5：只纠正已核Q8写集，不回放玩法；F在Render中只检查，从不借用或恢复。
  if(Read<std::uint32_t>(o->game,0x25a70)!=ClientDisplayMeta(o->installed).frame){
    Trace("client_error","event=display_render_frame_changed actual=%u expected=%u",Read<std::uint32_t>(o->game,0x25a70),ClientDisplayMeta(o->installed).frame);
    o->failed=true;if(active==o)active=nullptr;return;}
#ifdef ISAACCS_E2E
  // 记录原生 Render 真正消费的字段和本地 surface，发生在纠正写集之前。
  if(replica::stream::E2ETraceEnabled()){
    const auto d=ClientDisplayMeta(o->installed);
    const auto &t=o->game->_roomTransition;
    if(!native_mode&&(t._mode||o->e39_active)&&d.source_revision!=o->e39_revision){
      Trace("e39-client","revision=%llu frame=%u scope_room=%llu grid=%d mode=%d expected_mode=%d counter=%d duration=%d progress=%08x direction=%d target=%d dimension=%d animation=%d old=%u new=%u old_grid=%d boss=%d hud=%u",
        d.source_revision,d.frame,d.scope.room,d.grid,t._mode,d.transition_mode,Read<int>(&t,4),Read<int>(&t,8),Read<unsigned>(&t,12),
        Read<int>(&t,16),Read<int>(&t,20),Read<int>(&t,24),Read<int>(&t,28),unsigned(o->old_surface),unsigned(o->new_surface),
        o->old_grid,t._unkStartRoomTransitionCond,unsigned(Read<std::uint8_t>(o->game,0x22e90)));
      o->e39_revision=d.source_revision;o->e39_active=t._mode!=0;
    }
  }
#endif
  Reapply(*o);
  if(frame.phase){
    // 先绘制当前房间，再叠黑；极快装入仍用旧图把淡出画完。
    try{
      if(!DrawDoorFade(*o,frame,native_mode))o->door_fade={};
      else if(frame.opacity==0&&o->door_fade.fade_in!=RenderClock::time_point{})o->door_fade.rendered_end=true;
    }catch(...){o->door_fade={};}
  }
  // 首次 Render 的缓存变化必须指明失效项，不能让外层 return mismatch 掩盖根因。
  if(!ClientDisplayCurrent(o,r))Trace("client_error",
    "event=display_render_current window=%u active=%u failed=%u resources=%u targets=%u viewport=%u shading=%u ray=%u history=%u fx=%u expected_fx=%u",
    unsigned(MainWindow()),unsigned(active==o),unsigned(o->failed),unsigned(RoomResourcesCurrent(*o)),
    unsigned(RoomTargets(*o->room)==o->room_graphics.targets),unsigned(int(g_WIDTH)==o->graphics_width&&int(g_HEIGHT)==o->graphics_height),
    unsigned((ImageSlot(0x7e7370).image!=nullptr)==o->shading_applicable),unsigned((ImageSlot(0x7e7358).image!=nullptr)==o->room_graphics.special_ray),
    unsigned(ImageSlot(0x7e72b4).image==o->history_surface),unsigned(o->room->_fxLayers._loaded),unsigned(o->room_graphics.room_type!=16&&o->room_graphics.backdrop!=18));
}
namespace {
// Client 本机音乐音量：与原版 Crossfade／队列消费 7D7DA0 相同的取法——Manager+0x2A120 开关为真时取
// OptionsConfig._musicVolume（Manager+0x2A190），否则 0。Client 的音乐音量完全由本机设定决定：
// 原版 Play／Fadein 的音量参数本就是调用方本机的这个选项，Host 传来的值是 Host 的设定，一律不用。
float LocalMusicVolume(const Manager &m) noexcept {
  return Read<std::uint8_t>(&m,0x2a120)?m._optionsConfig._musicVolume:0.0f;
}
std::byte *SoundSlot(const ClientDisplaySound &v) noexcept {
  if(!g_Manager||!v.asset||v.id<0||v.path.empty())return nullptr;
  auto &sfx=g_Manager->_sfxManager;
  auto *begin=Read<std::byte *>(&sfx,0),*end=Read<std::byte *>(&sfx,4);
  if(!begin||end<begin||(end-begin)%0x1b8||std::size_t(v.id)>=std::size_t(end-begin)/0x1b8)return nullptr;
  auto *sound=begin+std::size_t(v.id)*0x1b8;
  const auto count=Read<unsigned>(sound,0x190);
  if(!count||count>7)return nullptr;
  const auto equal=[](std::string_view a,std::string_view b){
    const auto strip=[](std::string_view p){for(std::string_view root:{"resources/","resources-dlc3/","resources-repentogon/"})
      if(p.substr(0,root.size())==root)return p.substr(root.size());return p;};
    a=strip(a);b=strip(b);
    return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin(),[](char x,char y){return x==y||(x=='/'&&y=='\\')||(x=='\\'&&y=='/');});
  };
  for(unsigned i=0;i<count;++i){auto *sample=sound+8+0x38*i;
    if(equal(*reinterpret_cast<const std::string *>(sample),v.path))return sample+0x18;
  }
  return nullptr;
}
std::byte *SoundSlot(int id,unsigned sample) noexcept {
  if(!g_Manager||id<0)return nullptr;
  auto &sounds=g_Manager->_sfxManager._sounds;
  if(std::size_t(id)>=sounds.size()||sample>=7)return nullptr;
  auto *effect=reinterpret_cast<std::byte *>(&sounds[id]);
  return sample<Read<unsigned>(effect,0x190)?effect+0x20+sample*0x38:nullptr;
}
template<class R,class... Args> R AudioCall(void *source,unsigned offset,Args... args){
  return reinterpret_cast<R (__thiscall *)(void *,Args...)>((*static_cast<void ***>(source))[offset/4])(source,args...);
}
void SetVoice(std::byte *slot,void *source,const ClientDisplaySound &v,bool loop,bool start){
  auto &m=*g_Manager;
  const auto gain=(std::min)((std::max)(m._sfxManager._volume,0.0f)*v.volume*m._optionsConfig._sfxVolume,1.0f);
  Write(slot,0x08,std::uint8_t(loop));AudioCall<void>(source,0x40,int(loop));
  Write(slot,0x14,v.pitch);AudioCall<void>(source,0x58,v.pitch);
  Write(slot,0x0c,gain);AudioCall<void>(source,0x48,gain);
  if(start){AudioCall<void>(source,0x2c,1);Write(slot,0x09,std::uint16_t(1));}
  Write(slot,0x10,v.pan);
  if(v.pan==0||AudioCall<int>(source,0x60)<=1)AudioCall<void>(source,0x50,v.pan);
}
bool PlaySelected(const ClientDisplaySound &v,bool loop,std::byte *slot) {
  if(!slot)return false;
  // 9F7B20 接收已准备的 strong pair。后端参数复用 907E21–907EAE，
  // 省去 907D2F 的截止检查和 BD0F40 的样本抽取。
  reinterpret_cast<void (__thiscall *)(void *,void *)>(Base+0x5f7b20)(slot,v.asset);
  auto *source=Read<void *>(slot,0);
  if(!source||source!=Read<void *>(v.asset,0))return false;
  SetVoice(slot,source,v,loop,true);return true;
}
void RefreshLoopMix(ClientDisplayNative &o){
  for(std::size_t i=0;i<o.voice_count;++i){
    const auto &voice=o.voices[i];auto *slot=SoundSlot(voice.id,voice.sample);
    if(!slot||Read<void *>(slot,0)!=voice.source||!Read<std::uint8_t>(slot,8))continue;
    const auto gain=(std::max)(o.manager->_sfxManager._volume,0.f)*voice.volume*o.manager->_optionsConfig._sfxVolume;
    Write(slot,0x0c,gain);AudioCall<void>(voice.source,0x48,gain);
  }
}
void StopVoice(const ClientDisplayNative::Voice &voice){
  auto *slot=SoundSlot(voice.id,voice.sample);
  if(slot&&Read<void *>(slot,0)==voice.source&&Read<std::uint8_t>(slot,8)){
    AudioCall<void>(voice.source,0x3c);Write(slot,9,std::uint8_t(0));Write(slot,8,std::uint8_t(0));
  }
}
bool StopAudio(ClientDisplayNative &o){
  if(o.audio_uncertain)return false;
  DisplayScope scope;
  for(std::size_t i=0;i<o.voice_count;++i)StopVoice(o.voices[i]);
  o.voice_count=0;
  if(o.previous_music.captured){auto &m=o.manager->_musicmanager;
    m._queuedId=0;m.StopJingle();m.Fadeout(1.f);
    const auto &old=o.previous_music;m._enabled=old.enabled;
    if(old.enabled&&old.id&&old.id<m._entries.size())m.Play(old.id,LocalMusicVolume(*o.manager));
    m._queuedId=old.queued;m._pitch=old.pitch;m._targetPitch=old.target_pitch;
    o.music={};o.previous_music={};
  }
  return true;
}
bool PlaybackWindow(ClientDisplayNative *o) noexcept {
  return o&&o->display_complete&&o->installed&&!o->failed&&MainWindow()&&Graphics()&&TopOwner()==Owner::Manager&&g_Manager==o->manager;
}
void FortuneLines(const ClientDisplayEvent &event,std::list<std::string> &lines){
  if(event.index>=0&&event.index<16384&&!event.category.empty()){
    // 原版文本表按已选择的类别、当前语言和索引读值。三个错误字符串仍用 Host 提供的行。
    auto &table=g_Manager->_stringTable;
    const std::string category(event.category);
    const auto query=reinterpret_cast<const char *(__thiscall *)(StringTable *,const char *,int,int)>(Base+0x5f2d90);
    const auto *text=query(&table,category.c_str(),table.language,event.index);
    if(text&&text!=reinterpret_cast<const char *>(Base+0x7335b4)&&
       text!=reinterpret_cast<const char *>(Base+0x73357c)&&text!=reinterpret_cast<const char *>(Base+0x7335d4)){
      std::string_view remaining(text);
      while(lines.size()<32){
        const auto delimiter=remaining.find('|');
        lines.emplace_back(remaining.substr(0,delimiter));
        if(delimiter==std::string_view::npos)return;
        remaining.remove_prefix(delimiter+1);
      }
      lines.clear();
    }
  }
  for(unsigned i=0;i<event.line_count;++i)lines.emplace_back(event.lines[i]);
}
}
bool ClientDisplaySoundReady(const ClientDisplaySound &v) noexcept {
  return MainWindow()&&TopOwner()==Owner::Manager&&std::isfinite(v.volume)&&std::isfinite(v.pitch)&&
    std::isfinite(v.pan)&&SoundSlot(v)&&Read<void *>(v.asset,0)&&Read<void *>(v.asset,4);
}
bool ClientDisplayAudioHeld(const ClientDisplayNative *o) noexcept {return o&&(o->voice_count||o->previous_music.captured||o->audio_uncertain);}
ClientDisplayStep ClientDisplayAudio(ClientDisplayNative *o,const ClientDisplayMusicRequest &v,ClientDisplayArray<ClientDisplaySound> loops) noexcept {
  if(!PlaybackWindow(o)||!v.revision||loops.size()>o->voices.size())return {false,false,"binary_audio_window"};
  using Mode=state::MusicRequestMode;
  const auto same=[](const auto &a,const auto &b,unsigned sample){return a.instance==b.instance&&a.generation==b.generation&&a.id==b.id&&a.sample==sample;};
  std::array<std::byte *,512> slots{};
  std::array<unsigned,512> samples{};
  for(std::size_t i=0;i<loops.size();++i){
    slots[i]=SoundSlot(loops.data[i]);
    if(!slots[i]||std::find(slots.begin(),slots.begin()+i,slots[i])!=slots.begin()+i)
      return {false,false,"binary_audio_voice_slot_conflict"};
    samples[i]=unsigned(slots[i]-SoundSlot(loops.data[i].id,0))/0x38;
  }
  ClientDispatchScope dispatch(ClientOperation::PresentMaintenance,ClientEntry::PresentationMaintenance);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::PresentationMaintenance)!=ClientEntryDecision::CallOriginal)
    return {false,false,"binary_audio_dispatch"};
  bool ok=false;
  try{
    DisplayScope scope;
    Named(ClientEntry::DisplayMusic,[&]{
      auto &m=o->manager->_musicmanager;const auto old=o->music;
      if(!o->previous_music.captured)o->previous_music={m._currentId,m._queuedId,m._pitch,m._targetPitch,m._enabled,true};
      if(old.revision!=v.revision){
        // 不写 Host 的 enabled：Music+0x328 即 Manager+0x2A120，由选项载入 8FE75C 置为“本机 MusicVolume>0”，
        // 是各机自己的音量设定。音乐 tick 7D7DA0 在 STATE_GAME 下见其为 0 即把总音乐增益置 0；
        // 照抄 Host（Host 静音时为 0）会让 Client 全程无背景音乐，且 LocalMusicVolume 也随之为 0。
        if(old.generation!=v.generation||old.id!=v.id||old.mode!=v.mode){
          switch(Mode(v.mode)){
            case Mode::Play:m.Play(v.id,LocalMusicVolume(*o->manager));break;
            case Mode::Crossfade:m.Crossfade(v.id,v.fade);break;
            case Mode::Fadein:m.Fadein(v.id,LocalMusicVolume(*o->manager),v.fade);break;
            case Mode::Fadeout:m.Fadeout(v.fade);break;
            case Mode::QueuedContinuation:if(v.id&&m._currentId!=unsigned(v.id))m.Play(v.id,LocalMusicVolume(*o->manager));break;
            case Mode::Idle:if(old.id)m.Fadeout(1.f);break;
          }
        }
        m._queuedId=v.queued;
        for(unsigned i=0;i<7;++i)if(!old.revision||((old.layers^v.layers)&(1u<<i))){
          if(v.layers&(1u<<i))m.EnableLayer(i,true);else if(m.IsLayerEnabled(i))m.DisableLayer(i);
        }
        if(v.jingle_active&&(!old.jingle_active||old.jingle_generation!=v.jingle_generation))m.PlayJingle(v.jingle,0,false);
        if(old.jingle_active&&!v.jingle_active)m.StopJingle();
        if(old.paused!=v.paused){if(v.paused)m.Pause();else m.Resume();}
      }
      m._pitch=v.pitch;m._targetPitch=v.target_pitch;o->music=v;
    });
    Named(ClientEntry::DisplaySfxReplay,[&]{
      const auto previous=o->voices;const auto end=previous.begin()+o->voice_count;
      for(auto it=previous.begin();it!=end;++it)
        if(std::none_of(loops.begin(),loops.end(),[&](const auto &n){return same(*it,n,samples[&n-loops.begin()]);}))StopVoice(*it);
      o->voice_count=0;
      for(std::size_t i=0;i<loops.size();++i){
        const auto &value=loops.data[i];auto *slot=slots[i];
        const auto found=std::find_if(previous.begin(),end,[&](const auto &v){return same(v,value,samples[i]);});
        auto *source=Read<void *>(slot,0);
        if(found==end||source!=found->source||!Read<std::uint8_t>(slot,8)){
          if(!PlaySelected(value,true,slot)){
            // 仍按旧快照逐槽核 source，停止尚未转入新账的循环声。
            for(auto it=previous.begin();it!=end;++it)StopVoice(*it);
            return;
          }
          source=Read<void *>(slot,0);
        }else if(o->mixing_dirty||found->volume!=value.volume||found->pitch!=value.pitch||found->pan!=value.pan)
          SetVoice(slot,source,value,true,false);
        o->voices[o->voice_count++]={value.instance,value.generation,value.id,samples[i],value.volume,value.pitch,value.pan,source};
      }
    });
    ok=o->voice_count==loops.size();
  }catch(...){o->audio_uncertain=true;}
  ClientDispatchReturned(ClientEntry::PresentationMaintenance);
  return {ok&&dispatch.Succeeded(),true,ok?"binary_audio_installed":"binary_audio_partial"};
}
bool ClientDisplayCheckPublication(const ClientDisplayNative *o) noexcept {
  if(!o||!o->installed||!o->display_complete||!RoomResourcesCurrent(*o))return false;
  const auto d=ClientDisplayMeta(o->installed);
  if(d.transition_mode==2&&d.transition_boss)return o->boss_generation&&o->game->_roomTransition._versusScreenANM2._loaded;
  if(d.transition_mode==2||d.transition_mode==3)return o->old_surface&&o->new_surface;
  return true;
}
ClientDisplayStep ClientDisplayRestoreBoss(ClientDisplayNative *o,const ClientDisplayBoss &boss) noexcept {
  if(!PlaybackWindow(o))return {false,false,"binary_boss_window"};
  if(!boss.generation){o->boss_generation=0;return {true,false,"binary_boss_inactive"};}
  ClientDispatchScope dispatch(ClientOperation::PresentMaintenance,ClientEntry::PresentationMaintenance);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::PresentationMaintenance)!=ClientEntryDecision::CallOriginal)
    return {false,false,"binary_boss_dispatch"};
  bool written=false,ok=false;
  try{
    DisplayScope scope;
    auto &game=*o->game;auto &t=game._roomTransition;
    const auto install=[&]{
      if(boss.dimension<0||boss.dimension>2||boss.index<0||boss.index>=169||!boss.count||boss.poses[0].key!=-1||
         t._mode!=2||unsigned(t._unkStartRoomTransitionCond)!=boss.ids[0])return false;
      if(o->boss_generation!=boss.generation){
        const auto slot=game._roomOffset[boss.dimension*169+boss.index];
        if(slot>=game._nbRooms||slot>=507||!game._gridRooms[slot].Data)return false;
        // 823D10建立原生versus及肖像；暂停其选曲和环境声支路，归还同步转场标量。
        struct Restore final {
          RoomTransition &transition;std::array<std::byte,0x20> phase;int marker;bool replay;
          explicit Restore(RoomTransition &t):transition(t),marker(t._unkStartRoomTransitionCond),replay(boss_replay){
            std::memcpy(phase.data(),&t,phase.size());boss_replay=true;
          }
          ~Restore(){boss_replay=replay;std::memcpy(&transition,phase.data(),phase.size());transition._unkStartRoomTransitionCond=marker;}
        } restore(t);
        t._roomIndex=boss.index;t._roomDimension=boss.dimension;
        written=true;o->boss_owned=true;t.StartBossIntro(boss.ids[0],boss.ids[1]);Reapply(*o);
      }
      if(!t._versusScreenANM2._loaded||t._extraLayerANM2s.size()+1!=boss.count)return false;
      for(unsigned n=0;n<boss.count;++n){
        const auto &pose=boss.poses[n];ANM2 *sprite=&t._versusScreenANM2;
        if(n){const auto it=t._extraLayerANM2s.find(pose.key);if(it==t._extraLayerANM2s.end())return false;sprite=&it->second;}
        auto &state=sprite->_animState;
        if(!state._animData){if(pose.playing||state._animFrame!=pose.frame)return false;continue;}
        if(!std::isfinite(pose.frame)||pose.frame< -1||pose.frame>float(state._animData->_length))return false;
        if(state._animFrame!=pose.frame||state._isPlaying!=pose.playing){
          auto animation=state._animData->_name;
          written=true;sprite->SetFrame(&animation,int(pose.frame));
          state._animFrame=pose.frame;state._isPlaying=pose.playing;
        }
      }
      return FlushDisplay();
    };
    ok=install();
  }catch(...){}
  ClientDispatchReturned(ClientEntry::PresentationMaintenance);
  ok=ok&&dispatch.Succeeded();
  if(ok)o->boss_generation=boss.generation;
  return {ok,written,ok?"binary_boss_baseline_restored":"binary_boss_baseline_failed"};
}
ClientDisplayStep ClientDisplayEventReplay(ClientDisplayNative *o,const ClientDisplayEvent &event) noexcept {
  if(!PlaybackWindow(o))return {false,false,"binary_event_window"};
  // 快速切房会把多个完成事件带到同一世界目标。已过的房间代次由当前世界覆盖；
  // 同代次用 RoomDescriptor.GridIndex 核实际目的地，入口象限可能是同房的另一个格。
  if(event.kind==5){
    if(!event.completed)return {true,false,"binary_transition_state_current"};
    const auto meta=ClientDisplayMeta(o->installed);
    if(event.final_room<meta.scope.room)return {true,false,"binary_transition_covered_by_world"};
    const bool valid=event.final_room==meta.scope.room&&o->room&&o->room->_descriptor&&
      o->room->_descriptor->GridIndex==event.index&&int(o->game->_currentDimensionIdx)==event.dimension;
    // 若第一份新房基线已越过原生滑动阶段，仍可用同目标的完成事件识别普通门。
    if(valid&&DirectionalWalk(event.animation,event.direction))o->door_arrival_event_scene=meta.scope.scene;
    return {valid,false,valid?"binary_transition_state_current":"binary_transition_destination_mismatch"};
  }
  // Boss事件仍经可靠顺序消费；当前world基线决定实际显示代次，历史事件不会覆盖它。
  if(event.kind==4)return {true,false,"binary_boss_event_covered_by_world"};
  ClientDispatchScope dispatch(ClientOperation::PresentMaintenance,ClientEntry::PresentationMaintenance);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::PresentationMaintenance)!=ClientEntryDecision::CallOriginal)
    return {false,false,"binary_event_dispatch"};
  bool written=false,ok=false;
  try{
    DisplayScope scope;
    switch(event.kind){
      case 1:{
        auto *slot=SoundSlot(event.sound);
        if(slot){written=true;Named(ClientEntry::DisplaySfxReplay,[&]{ok=PlaySelected(event.sound,false,slot);});}
        break;
      }
      case 2:{
        auto *player=static_cast<Entity_Player *>(event.player);
        if(event.configuration){
          written=true;o->game->_hud.ShowItemText(player,static_cast<ItemConfig_Item *>(event.configuration));
        }else{
          std::string title(event.title),subtitle(event.subtitle);
          written=true;o->game->_hud.ShowItemTextCustomUTF8(player,title.data(),subtitle.data());
        }
        ok=true;break;
      }
      case 3:{
        static_assert(sizeof(std::list<std::string>)==8&&sizeof(std::_List_node<std::string,void *>)==0x20);
        std::list<std::string> lines;
        FortuneLines(event,lines);
        written=true;o->game->_hud.ShowFortuneText(reinterpret_cast<int **>(&lines));ok=true;break;
      }
    }
  }catch(...){}
  if(ok&&written)try{ok=FlushDisplay();}catch(...){ok=false;}
  ClientDispatchReturned(ClientEntry::PresentationMaintenance);
  return {ok&&dispatch.Succeeded(),written,ok?"binary_event_replayed":"binary_event_native_failure"};
}

namespace {
// 9635B0只消费这些POD字节；其余保存对象内容不上线、不构造、不交给游戏释放。
struct MapEntity final {std::int32_t type=0,variant=0,subtype=0;std::uint32_t unused[7]{};float x=1,y=1;std::uint32_t tail[18]{};};
struct MapGrid final {std::uint32_t type=18,unused[7]{};};
struct MapHeader final {void *begin=nullptr,*end=nullptr,*capacity=nullptr;};
static_assert(sizeof(MapEntity)==0x78&&offsetof(MapEntity,x)==0x28&&sizeof(MapGrid)==0x20&&sizeof(MapHeader)==12);
struct MirrorEntry final {
  RoomDescriptor *descriptor=nullptr;MapHeader old_entities,old_grids;
  std::array<MapEntity,16> entities{};MapGrid grid{};unsigned count=0;bool has_grid=false;
};
struct MirrorFrame final {
  std::vector<MirrorEntry> entries;memory::Lease charge;
  int focus_index=-1;
};
thread_local MirrorFrame *map_frame=nullptr;
int Card(bool rune){
  auto *manager=reinterpret_cast<std::byte *>(g_Manager);
  auto **begin=Read<void **>(manager,0x2a258),**end=Read<void **>(manager,0x2a25c);
  if(!begin||end<begin||end-begin>65536)throw std::runtime_error("map_card_config_bounds");
  for(int i=0;i<end-begin;++i)if(begin[i]&&(Read<int>(begin[i],0x58)==2)==rune)return i;
  throw std::runtime_error("map_card_kind_resource_missing");
}
// 逐项反推9635B0及scene_map.cpp的分类；同类只保留产生该最终帧的一项。
MapEntity Representative(unsigned bit,unsigned frame){
  MapEntity v;v.type=5;
  constexpr int heart[]={2,1,12,8,10,3,6,11,7,4},coin[]={1,2,3,7},key[]={1,2,4};
  constexpr int chest[]={52,51,69,69,50,360,56,53,60,57};
  constexpr int beggar[]={12,8,1,2,3,16,10,17,19},machine[]={4,18,5,7,9,13};
  switch(bit){
    case 0:if(frame>=std::size(heart))break;v.variant=10;v.subtype=heart[frame];return v;
    case 1:if(frame>=std::size(coin))break;v.variant=20;v.subtype=coin[frame];return v;
    case 2:v.variant=42;return v;
    case 3:if(frame>=std::size(key))break;v.variant=30;v.subtype=key[frame];return v;
    case 4:if(frame>1)break;v.variant=40;v.subtype=frame?4:1;return v;
    case 5:case 12:v.variant=300;v.subtype=Card(bit==12);return v;
    case 6:v.variant=100;return v;
    case 7:if(frame>1)break;v.variant=70;v.subtype=frame?14:1;return v;
    case 8:if(frame>1)break;v.variant=90;v.subtype=frame?4:1;return v;
    case 9:if(frame>=std::size(chest))break;v.variant=chest[frame];v.subtype=frame==3?2:1;return v;
    case 10:v.variant=350;return v;
    case 11:if(frame>=std::size(beggar))break;v.type=6;v.variant=beggar[frame];return v;
    case 13:if(frame>=std::size(machine))break;v.type=6;v.variant=machine[frame];return v;
    case 15:v.type=1000;v.variant=161;v.subtype=980;return v;
  }
  throw std::runtime_error("map_icon_frame_unrepresentable");
}
}
bool ClientDisplayMapRandom(const void *site,unsigned maximum,unsigned &result) noexcept {
  if(!map_frame||reinterpret_cast<std::uintptr_t>(site)!=Base+0x5636cc)return false;
  if(map_frame->focus_index<0||unsigned(map_frame->focus_index)>=maximum){
    ClientCallbackFault("binary_map_focus_binding_changed");result=0;return true;
  }
  result=unsigned(map_frame->focus_index);return true;
}
ClientMapMirrorScope::ClientMapMirrorScope(void *configuration) noexcept {
  if(!ClientNativeDisplayActive()){ready_=true;return;}
  auto *source=DisplaySource();
  if(!ClientNativeDisplayCallbacks()||!source||map_frame||!Graphics())return;
  try{
    const auto count=ClientDisplayMapCount(source->installed);
    auto charge=memory::Reserve(memory::Pool::Values,sizeof(MirrorFrame)+count*sizeof(MirrorEntry)+64,"client_map_mirror");
    auto owner=std::make_unique<MirrorFrame>();owner->charge=std::move(charge);owner->entries.resize(count);
    const auto meta=ClientDisplayMeta(source->installed);
    if(meta.challenge==32&&g_Game->_currentRoomIdx<0x80000000u){
      const auto *config=g_Game->_minimap._config;
      const int focus=configuration==&config[0]?meta.map_focus[0]:configuration==&config[1]?meta.map_focus[1]:-1;
      const auto index=std::find_if(g_Game->_gridRooms,g_Game->_gridRooms+g_Game->_nbRooms,[focus](const auto &d){
        return d.SafeGridIndex==focus&&(d.Data||d.OverrideData);
      });
      if(index==g_Game->_gridRooms+g_Game->_nbRooms)throw std::runtime_error("binary_map_focus_missing");
      owner->focus_index=int(index-g_Game->_gridRooms);
    }
    for(unsigned i=0;i<owner->entries.size();++i){auto &entry=owner->entries[i];const auto icons=ClientDisplayMapAt(source->installed,i);
      const auto slot=icons.slot;entry.descriptor=slot<507?&g_Game->_gridRooms[slot]:slot<527&&slot!=525?&g_Game->_negativeGridRooms[slot-507]:nullptr;
      if(!entry.descriptor)throw std::runtime_error("map_descriptor_slot");
      // +3C是原生扫描门控；不临时放大可见性，也不把隐藏保存体转换成显示摘要。
      if(icons.present!=(entry.descriptor->DisplayFlags!=0))throw std::runtime_error("map_display_flags_presence");
      if(icons.present)for(unsigned bit=0;bit<16;++bit)if(icons.mask&(1u<<bit)){
        if(bit==14)entry.has_grid=true;else entry.entities[entry.count++]=Representative(bit,icons.frames[bit]);}
      std::memcpy(&entry.old_entities,reinterpret_cast<std::byte *>(entry.descriptor)+0x74,12);
      std::memcpy(&entry.old_grids,reinterpret_cast<std::byte *>(entry.descriptor)+0x68,12);
    }
    for(unsigned slot=0;slot<507;++slot)if(g_Game->_gridRooms[slot].DisplayFlags&&
       std::none_of(owner->entries.begin(),owner->entries.end(),[&](const auto &v){return v.descriptor==&g_Game->_gridRooms[slot];}))
      throw std::runtime_error("map_visible_descriptor_not_in_target");
    // 全部代表项准备成功后才发布header；任何后续异常由作用域精确归还原指针与capacity。
    for(auto &entry:owner->entries){MapHeader entities{entry.entities.data(),entry.entities.data()+entry.count,entry.entities.data()+entry.count};
      MapHeader grids{&entry.grid,&entry.grid+unsigned(entry.has_grid),&entry.grid+unsigned(entry.has_grid)};
      std::memcpy(reinterpret_cast<std::byte *>(entry.descriptor)+0x74,&entities,12);
      std::memcpy(reinterpret_cast<std::byte *>(entry.descriptor)+0x68,&grids,12);}
    state_=owner.release();map_frame=static_cast<MirrorFrame *>(state_);ready_=true;
  }catch(...){ClientCallbackFault("client_map_mirror_prepare");}
}
ClientMapMirrorScope::~ClientMapMirrorScope() noexcept {
  auto *owner=static_cast<MirrorFrame *>(state_);if(!owner)return;
  for(auto &entry:owner->entries){std::memcpy(reinterpret_cast<std::byte *>(entry.descriptor)+0x74,&entry.old_entities,12);
    std::memcpy(reinterpret_cast<std::byte *>(entry.descriptor)+0x68,&entry.old_grids,12);}
  map_frame=nullptr;delete owner;
}
} // namespace cs::game
