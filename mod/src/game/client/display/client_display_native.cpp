#include "IsaacRepentance.h"
#include "game/client/display/client_display_native.h"
#include "game/client/scene/client_lifetime.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "core/replication_memory.h"
#include "runtime.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <list>
#include <new>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
namespace {
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
}
// 主线程显示owner持有本方种子树、原值快照和surface资格；原生Room资源仍由Scene退休。
struct ClientDisplayNative final {
  Game *game=nullptr;Room *room=nullptr;Manager *manager=nullptr;
  RoomDescriptor *descriptor=nullptr;
  Camera *owned_camera=nullptr;
  bool hud_loaded=false,root_uncertain=false,audio_uncertain=false;
  std::array<ANM2 *,2> owned_lights{};
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
  KAGE_Graphics_ImageBase *captured_old=nullptr;
  int captured_width=0,captured_height=0;
  memory::Lease charge;
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
thread_local bool boss_replay=false;
struct DisplayScope final {DisplayScope(){++display_depth;}~DisplayScope(){--display_depth;}};
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
// 离屏捕获只运行原生PreRender/Render和图形提交；不调用转场FSM或完整ChangeRoom。
bool Capture(ClientDisplayNative &o,bool old) {
  if(!Graphics()||!o.room||!o.room->_descriptor||!o.room->_descriptor->SpawnSeed)return false;
  auto &graphics=*__ptr_g_KAGE_Graphics_Manager;auto &transition=o.game->_roomTransition;
  auto *target=Read<KAGE_Graphics_ImageBase *>(&transition,old?0x28:0x30);
  if(!target){reinterpret_cast<void (__thiscall *)(RoomTransition *)>(Base+0x423820)(&transition);
    target=Read<KAGE_Graphics_ImageBase *>(&transition,old?0x28:0x30);if(!target)return false;}
  DisplayScope scope;
  // 新surface在publication前绘制，Hook仍须读取同批已安装paused和原生对象，不借旧展示输入。
  struct Source final {ClientDisplayNative *old;explicit Source(ClientDisplayNative &o):old(capture_owner){capture_owner=&o;}
    ~Source(){capture_owner=old;}} source(o);
  o.room->PreRender();graphics.PresentWithoutSwap();Rendering::PushCurrentRenderTarget();
  struct Restore final {KAGE_Graphics_Manager &graphics;BlendMode blend;KAGE_Graphics_Shader *shader;
    ~Restore(){Rendering::RestorePreviousRenderTarget();graphics._blendMode=blend;graphics._shader=shader;}} restore{graphics,graphics._blendMode,graphics._shader};
  graphics.SetCurrentRenderTarget(target,true);graphics.Clear();
  // 与原生mode1相同的默认7号混合模式；由外层原生target owner恢复图形目标。
  graphics._blendMode=BlendMode();
  reinterpret_cast<void (__thiscall *)(Room *)>(Base+0x404500)(o.room);
  graphics.PresentWithoutSwap();
  if(!ClientNativeCpuQueuesEmpty()||Read<std::uint32_t>(o.game,0x25a70)!=ClientDisplayMeta(o.installed).frame)return false;
  Reapply(o);
  return true;
}
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
ClientDisplayNative *ClientDisplayCreate() noexcept {
  if(!MainWindow()||TopOwner()!=Owner::Manager)return nullptr;
  try{auto charge=memory::Reserve(memory::Pool::Values,sizeof(ClientDisplayNative)+2*81*20+128,"client_display_native");
    auto *o=new ClientDisplayNative;o->charge=std::move(charge);return o;}catch(...){return nullptr;}
}
void ClientDisplayDestroy(ClientDisplayNative *o) noexcept {
  if(o&&IsMain()&&!o->written&&!ClientDisplayAudioHeld(o)&&!o->owned_camera&&!o->root_uncertain){
    if(active==o)active=nullptr;delete o;
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
  if(!o||!o->installed||!Roots(*o,r)||!o->room->_descriptor->Data||o->rendering)return false;
  try{
    if(o->old_surface&&o->captured_old==Read<KAGE_Graphics_ImageBase *>(&o->game->_roomTransition,0x28)&&
       int(g_WIDTH)==o->captured_width&&int(g_HEIGHT)==o->captured_height)return true;
    if(!Capture(*o,true))return false;
    const auto *data=o->room->_descriptor->Data;
    o->old_scroll=o->room->_renderScrollOffset;o->old_surface=true;o->new_surface=false;
    o->old_grid=o->room->_descriptor->GridIndex;o->old_entry=int(o->game->_currentRoomIdx);o->old_dimension=int(o->game->_currentDimensionIdx);
    o->old_width=data->Width;o->old_height=data->Height;o->old_mirror=MirrorDimension(*o->game);
    o->captured_old=Read<KAGE_Graphics_ImageBase *>(&o->game->_roomTransition,0x28);
    o->captured_width=int(g_WIDTH);o->captured_height=int(g_HEIGHT);
    return true;
  }catch(...){return false;}
}
ClientDisplayStep ClientDisplayBuildRoots(ClientDisplayNative *o,const ClientNativeRoots &r) noexcept {
  if(!o||!MainWindow()||TopOwner()!=Owner::Manager||!Graphics()||r.game!=g_Game||!g_Game||r.room!=g_Game->_room||
     !r.room||!r.descriptor||o->root_uncertain)return {false,false,"display_root_window"};
  if(o->owned_camera)return {o->room==r.room&&r.camera==o->owned_camera&&o->hud_loaded,false,"display_root_current"};
  if(r.camera||g_Manager->_state!=1)return {false,false,"display_cold_root_occupied"};
  o->game=g_Game;o->room=g_Game->_room;o->manager=g_Manager;
  // Camera 构造读取已安装的房间边界。HUD 与本机历史 surface 使用原生图形资源 owner。
  try{
    auto *raw=reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(sizeof(Camera));
    if(!raw)return {false,false,"display_camera_allocation"};
    o->root_uncertain=true;
    o->owned_camera=static_cast<Camera *>(raw);
    o->owned_camera->constructor(o->room);o->room->_Camera=o->owned_camera;
    DisplayScope scope;
    o->game->_hud.LoadGraphics();o->hud_loaded=true;
    // 复用 Room 正常构造好的两份灯光 ANM2，只在冷场景加载所需图层。
    constexpr unsigned light_offsets[]={0x6e40,0x6f54};
    constexpr const char *light_paths[]={"gfx/LightGradient.anm2","gfx/SpotLight.anm2"};
    for(unsigned i=0;i<2;++i){
      auto *sprite=reinterpret_cast<ANM2 *>(reinterpret_cast<std::byte *>(o->room)+light_offsets[i]);
      if(!sprite->_loaded){
        std::string path=light_paths[i];sprite->Load(path,true);
        o->owned_lights[i]=sprite;
      }
      auto *layer=sprite->_loaded?sprite->GetLayer(0):nullptr;
      if(!layer){o->root_uncertain=false;return {false,true,"display_light_resource_missing"};}
      Write(layer,0x20,1u);Write(layer,0x24,1u);
    }
    if(!Read<void *>(reinterpret_cast<void *>(Base+0x7e72b4),0))
      reinterpret_cast<void (__cdecl *)()>(Base+0x2efb70)();
    o->root_uncertain=false;
    if(!FlushDisplay())return {false,true,"display_roots_cpu_borrow"};
    return {true,true,"display_roots_ready"};
  }catch(...){return {false,true,"display_root_construction_exception"};}
}
bool ClientDisplayReleaseRoots(ClientDisplayNative *o) noexcept {
  if(!o||(!o->owned_camera&&!o->hud_loaded))return !o||!o->root_uncertain;
  if(o->root_uncertain||o->written||!MainWindow()||!Graphics()||g_Game!=o->game||g_Manager!=o->manager||
     o->game->_room!=o->room||o->room->_Camera!=o->owned_camera)return false;
  try{
    o->root_uncertain=true;
    if(o->hud_loaded){
      reinterpret_cast<void (__thiscall *)(HUD *)>(Base+0x57ab10)(&o->game->_hud);
      reinterpret_cast<void (__thiscall *)(HUD *)>(Base+0x579d80)(&o->game->_hud);
      o->hud_loaded=false;
    }
    for(auto *&light:o->owned_lights)if(light){light->destructor();light->constructor();light=nullptr;}
    // 走 SDK 析构和同一原生分配器，保留 Game 当前 Room 给后续路由。
    o->owned_camera->destructor();
    reinterpret_cast<void (__cdecl *)(void *)>(Base+0x68717b)(o->owned_camera);
    o->room->_Camera=nullptr;o->owned_camera=nullptr;o->root_uncertain=false;
    return true;
  }catch(...){return false;}
}
ClientDisplayStep ClientDisplayBeforeInstall(ClientDisplayNative *o,const ClientDisplayWorldRef &target,const ClientNativeRoots &r) noexcept {
  if(!o||!target||!MainWindow()||TopOwner()!=Owner::Manager||o->rendering||o->failed)return {false,false,"display_prepare_owner"};
  bool capture_started=false;
  try{
    const auto meta=ClientDisplayMeta(target);
    const bool transition_started=o->installed&&meta.transition_mode!=0&&ClientDisplayMeta(o->installed).transition_mode==0;
    const bool room_changed=o->installed&&ChangedRoom(o->installed,target);
    if(o->installed&&(room_changed||transition_started)){
      if(!Roots(*o,r)||!o->room->_descriptor->Data)return {false,false,"display_old_world_binding"};
      capture_started=true;
      if(!ClientDisplayPreserveRoom(o,r)){o->failed=true;if(active==o)active=nullptr;return {false,true,"display_old_surface_capture"};}
      if(room_changed)o->new_surface=false;
    }
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
    const auto d=ClientDisplayMeta(target);auto *descriptor=static_cast<RoomDescriptor *>(r.descriptor);
    if(!d.spawn_seed||descriptor->GridIndex!=d.grid||descriptor->Dimension!=d.dimension)return {false,false,"display_descriptor_binding"};
    if(o->game&&o->game!=r.game)return {false,false,"display_game_replaced"};
    // 6F7D28直接读取本地历史surface；没有正常资源owner时不能把空目标交给Render。
    if(!Read<void *>(reinterpret_cast<void *>(Base+0x7e72b4),0))return {false,false,"display_history_surface_pending"};
    if(o->old_surface&&(o->captured_old!=Read<KAGE_Graphics_ImageBase *>(&g_Game->_roomTransition,0x28)||
       int(g_WIDTH)!=o->captured_width||int(g_HEIGHT)!=o->captured_height))o->old_surface=o->new_surface=false;
    for(unsigned i=0;i<ClientDisplayMapCount(target);++i){const auto v=ClientDisplayMapAt(target,i);auto *native=MapDescriptor(*g_Game,v.slot);
      if(!native||native->GridIndex!=v.grid||native->Dimension!=v.dimension)return {false,false,"display_map_descriptor_binding"};}
    const bool map_changed=!ClientDisplaySameMap(o->installed,target);
    o->game=static_cast<Game *>(r.game);o->room=static_cast<Room *>(r.room);o->manager=g_Manager;o->descriptor=descriptor;
    const auto previous_shake=o->previous_shake;
    // 所有可分配工作和旧值快照先完成；从written起失败必须退休，不能宣称自动回滚。
    for(const unsigned offset:{0x25a84u,0x25a88u,0x25ab8u})o->Save(o->game,offset,offset==0x25ab8?1:4);
    o->Save(o->manager,0x8,4);
    {o->Save(o->game,0x1aee4,60);o->Save(o->game,0x1af20,15);o->Save(o->room,0x7460,8);o->Save(o->game,0x1adb4,4);}
    o->written=true;if(active==o)active=nullptr;
    // 本地已构造C/S游戏承载进入原生游戏显示态；Manager更新仍整体拦截。
    // 音乐的dizzy／红房分支和Manager::Render因此读真实本地游戏态，退休先还原菜单态。
    o->manager->_state=2;
    o->installed=target;
    {
      const auto input=ClientDisplayInputs(target);
      Write(o->game,0x1aee4,input.pill_effects);Write(o->game,0x1af20,input.pill_identified);Write(o->room,0x7460,input.water);
      Write(o->game,0x1adb4,input.planetarium);
    }
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
    Write(reinterpret_cast<void *>(Base+0x7d01e8),0,std::uint32_t(o->manager->_framecount-1));
    // 中途加入只拥有当前房间。用它建立两张有效本地图，保持 Host 的阶段和滚动方向。
    if((d.transition_mode==2||d.transition_mode==3)&&!o->old_surface&&!ClientDisplayPreserveRoom(o,r)){
      o->failed=true;return {false,true,"display_transition_baseline_surface"};
    }
    if(o->old_surface&&!o->new_surface){
      TransitionScroll(*o,d);
      if(!Capture(*o,false)){o->failed=true;return {false,true,"display_new_surface_capture"};}o->new_surface=true;
    }
    if(!d.transition_mode){o->old_surface=false;o->new_surface=false;}
    if(!FlushDisplay()){o->failed=true;return {false,true,"display_install_cpu_borrow"};}
    o->surface_width=int(g_WIDTH);o->surface_height=int(g_HEIGHT);o->pending.reset();active=o;
    return {true,true,"display_installed"};
  }catch(...){o->failed=true;if(active==o)active=nullptr;return {false,o->written,"display_install_exception"};}
}
bool ClientDisplayRetire(ClientDisplayNative *o,bool preserve_target,bool preserve_audio) noexcept {
  if(!o)return true;if(!MainWindow()||TopOwner()!=Owner::Manager||o->rendering)return false;
  if(active==o)active=nullptr;
  try{
    if(ClientDisplayAudioHeld(o)&&!preserve_audio&&(g_Manager!=o->manager||!StopAudio(*o)))return false;
    if(o->written){
      if(g_Game!=o->game||g_Manager!=o->manager||o->game->_room!=o->room)return false;
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
      for(unsigned i=o->saved_count;i;--i){const auto &s=o->saved[i-1];std::memcpy(static_cast<std::byte *>(s.root)+s.offset,s.bytes.data(),s.size);}
      if(!preserve_target&&std::max(modifier,0.0f)!=std::max(Read<float>(o->manager,0x2a170),0.0f)){
        DisplayScope scope;Named(ClientEntry::DisplaySfxRefresh,[&]{o->manager->_sfxManager.ClearVolumeModifier();});
      }
    }
    o->written=false;o->saved_count=0;o->installed.reset();o->descriptor=nullptr;o->boss_generation=0;
    if(!o->owned_camera)o->room=nullptr;
    if(!preserve_target)o->pending.reset();
    return true;
  }catch(...){o->failed=true;return false;}
}
bool ClientDisplayHasWorld(const ClientDisplayNative *o) noexcept {return o&&bool(o->installed);}
bool ClientDisplayCurrent(const ClientDisplayNative *o,const ClientNativeRoots &r) noexcept {
  return MainWindow()&&o&&o==active&&!o->failed&&o->installed&&Roots(*o,r)&&
    Read<std::uint32_t>(o->game,0x25a70)==ClientDisplayMeta(o->installed).frame;
}
bool ClientDisplayPaused(const ClientDisplayNative *o,bool &v) noexcept {
  if(!o||o!=DisplaySource()||o->failed||!o->installed||!IsMain())return false;v=ClientDisplayMeta(o->installed).paused;return true;
}
bool ClientNativeDisplayActive() noexcept{const auto *source=DisplaySource();return IsMain()&&source&&source->installed&&!source->failed&&ClientBusy();}
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
bool ClientDisplayDealQualified(const void *site,const void *game,bool &value) noexcept {
  const auto caller=reinterpret_cast<std::uintptr_t>(site);
  const auto *o=DisplaySource();
  if(!ClientNativeDisplayCallbacks()||!o||!o->installed||o->game!=game||
     (caller!=Base+0x43d14a&&caller!=Base+0x43d755))return false;
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
bool ClientDisplayAdvance(ClientDisplayNative *o,const ClientNativeRoots &r,bool local_blocked) noexcept {
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
      if(branch!=Branch::ConsoleHold){Named(ClientEntry::DisplayHudUpdate,[&]{g._hud.Update();});
        if(Read<float>(&g,0x25b28)>0)Named(ClientEntry::DisplayHudPostUpdate,[&]{g._hud.PostUpdate();});
        if(branch==Branch::Normal||branch==Branch::RoomTransition){
          if(branch==Branch::RoomTransition)Named(ClientEntry::DisplayTransition,[&]{auto &t=g._roomTransition;if(t._unkStartRoomTransitionCond){t._versusScreenANM2.Update();
              for(auto &entry:t._extraLayerANM2s)entry.second.Update();}
            if(t._mode==2||t._mode==3){Write(&g,0x22e94,2);Write(&g,0x22e9c,2);}});
          Named(ClientEntry::DisplayMinimapUpdate,[&]{g._minimap.Update();});
          if(o->map_dirty){Named(ClientEntry::DisplayMinimapRefresh,[&]{g._minimap.Refresh();});o->map_dirty=false;}
          if(branch==Branch::Normal){Named(ClientEntry::DisplaySort,[&]{Sort(room);});
            Named(ClientEntry::DisplayFxUpdate,[&]{reinterpret_cast<void (__thiscall *)(FXLayers *,bool)>(Base+0x56bf40)(&room._fxLayers,false);});
            Named(ClientEntry::DisplayBackground,[&]{auto &floor=room._backdrop.floorANM2;floor.Update();
              // 7F93D8..7F9449：26号背景保持Idle；不调用Room::Update相邻的玩法段。
              const auto *animation=floor._animState._animData;
              if(Read<int>(&room,0x1d18)==26&&(!animation||animation->_name!="Idle"||
                 (Read<std::uint8_t>(animation,0x34)&&!floor._animState._isPlaying)))floor.Play("Idle",true);
              auto *hell=reinterpret_cast<std::byte *>(&room)+0x776c;
              if(Read<std::uint8_t>(hell,0x118))reinterpret_cast<ANM2 *>(hell+4)->Update();});}
          Named(ClientEntry::DisplayHudPostUpdate,[&]{g._hud.PostUpdate();});
        }else if(branch==Branch::PauseCountdown||branch==Branch::StageTransition||branch==Branch::ActiveOverlay||branch==Branch::SecondaryFade)
          Named(ClientEntry::DisplayHudPostUpdate,[&]{g._hud.PostUpdate();});
      }
    }
    const bool minecart=g._roomTransition._mode!=0&&Read<int>(&g,0x1b858)==19;
    if(!local_blocked&&(!ClientDisplayMeta(o->installed).paused||minecart)&&room._Camera)Named(ClientEntry::DisplayCamera,[&]{
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
  if(o->game->_roomTransition._mode&&(int(g_WIDTH)!=o->surface_width||int(g_HEIGHT)!=o->surface_height||Read<int>(o->game,0x25aac))){
    o->failed=true;active=nullptr;return false;}
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
  if(!Roots(*o,r)){o->failed=true;if(active==o)active=nullptr;return;}
  // RE-5：只纠正已核Q8写集，不回放玩法；F在Render中只检查，从不借用或恢复。
  if(Read<std::uint32_t>(o->game,0x25a70)!=ClientDisplayMeta(o->installed).frame){o->failed=true;if(active==o)active=nullptr;return;}
  Reapply(*o);
}
namespace {
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
    if(old.enabled&&old.id&&old.id<m._entries.size())m.Play(old.id,1.f);
    m._queuedId=old.queued;m._pitch=old.pitch;m._targetPitch=old.target_pitch;
    o.music={};o.previous_music={};
  }
  return true;
}
bool PlaybackWindow(ClientDisplayNative *o) noexcept {
  return o&&o==active&&o->installed&&!o->failed&&MainWindow()&&Graphics()&&TopOwner()==Owner::Manager&&g_Manager==o->manager;
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
        m._enabled=v.enabled;
        if(old.generation!=v.generation||old.id!=v.id||old.mode!=v.mode){
          switch(Mode(v.mode)){
            case Mode::Play:m.Play(v.id,v.parameter);break;
            case Mode::Crossfade:m.Crossfade(v.id,v.fade);break;
            case Mode::Fadein:m.Fadein(v.id,v.parameter,v.fade);break;
            case Mode::Fadeout:m.Fadeout(v.fade);break;
            case Mode::QueuedContinuation:if(v.id&&m._currentId!=unsigned(v.id))m.Play(v.id,v.parameter>0?v.parameter:1.f);break;
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
  if(!o||!o->installed)return false;
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
  // 转场事件由当前值和旧／新 surface 兑现。完成事件仍必须匹配已安装的实际目的地。
  if(event.kind==5){
    const bool valid=!event.completed||(int(o->game->_currentRoomIdx)==event.index&&int(o->game->_currentDimensionIdx)==event.dimension);
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
