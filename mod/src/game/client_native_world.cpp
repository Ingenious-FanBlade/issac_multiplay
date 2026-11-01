#include "IsaacRepentance.h"
#include "game/client_native_world.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <new>
#include <iterator>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
// 复用现有主线程run位读取；不为SDK单元引入C++20 Authority模型头。
bool RunActive() noexcept;
namespace {
// 仅保存本适配实际修改的具名标量；descriptor 的原生容器由已有构造 owner 管理。
struct DescriptorScalars final {
  int grid=0,safe=0,list=0,dimension=0,allowed=0,flags=0,visited=0;
  std::int16_t clear=0;
  RoomConfig_Room *data=nullptr,*override_data=nullptr;
  int doors[8]{};
  std::uint32_t spawn=0,decoration=0,award=0;
  void Read(RoomDescriptor &d) noexcept {
    grid=d.GridIndex;safe=d.SafeGridIndex;list=d.ListIndex;dimension=d.Dimension;
    data=d.Data;override_data=d.OverrideData;allowed=d.AllowedDoors;flags=d.Flags;
    visited=d.VisitedCount;clear=d.ClearCount;spawn=d.SpawnSeed;decoration=d.DecorationSeed;award=d.AwardSeed;
    std::copy(std::begin(d.Doors),std::end(d.Doors),doors);
  }
  void Restore(RoomDescriptor &d) const noexcept {
    d.GridIndex=grid;d.SafeGridIndex=safe;d.ListIndex=list;d.Dimension=dimension;
    d.Data=data;d.OverrideData=override_data;d.AllowedDoors=allowed;d.Flags=flags;
    d.VisitedCount=visited;d.ClearCount=clear;d.SpawnSeed=spawn;d.DecorationSeed=decoration;d.AwardSeed=award;
    std::copy(std::begin(doors),std::end(doors),d.Doors);
  }
};
// J273 7E8410：ECX=slot、EDX=width、栈=height/shape，caller 清栈8字节。
// 普通 fastcall 会错误地重复清栈，所以用具名适配保存此已核调用合同。
int DoorGridPosition(int slot,int width,int height,int shape) noexcept {
  const auto function=Base+0x3e8410;
  int result=0;
  __asm {
    push shape
    push height
    mov ecx,slot
    mov edx,width
    call function
    add esp,8
    mov result,eax
  }
  return result;
}
// J273 RoomInit 7E92F2：ECX=Room、push1，7E86C0 callee清栈4字节。
// true分支只依据有效descriptor/几何写四个房间边界；不扫描Grid、不分配、不调用RNG。
// Room ctor把这些边界置零，Camera ctor/Snap会立即读取，不能依赖先前Host局留下的缓存。
void InitializeRoomBounds(Room *room) noexcept {
  reinterpret_cast<void (__thiscall *)(Room *,bool)>(Base+0x3e86c0)(room,true);
}
bool ManagerPhase() noexcept {
  return IsMain()&&TopOwner()==Owner::Manager&&ContextHealthy()&&!InBlockedContext()&&
         ClientBusy()&&ClientPersistenceBlocked();
}
bool GraphicsContext() noexcept {
  void *dc=nullptr,*context=nullptr;
  return ClientRendererHandles(dc,context)&&dc&&context&&
         dc==wglGetCurrentDC()&&context==wglGetCurrentContext();
}
bool DescriptionValid(const ClientNativeSceneDescription &d) noexcept {
  // 尺寸由官方 shape 表核对；不让网络值扩大448格的原生数组。
  static constexpr int widths[]={0,13,13,5,13,5,26,26,26,26,26,26,26};
  static constexpr int heights[]={0,7,3,7,14,14,7,3,14,14,14,14,14};
  return d.stage>=1&&d.stage<=13&&d.stage_type>=0&&d.stage_type<=5&&
         d.difficulty>=0&&d.difficulty<=3&&d.challenge>=0&&d.challenge<1024&&
         d.config_stage>=0&&d.config_stage<37&&d.config_mode>=0&&d.config_mode<=1&&
         d.room_type>=1&&d.room_type<=29&&d.variant>=0&&d.subtype>=0&&
         d.shape>=1&&d.shape<=12&&d.width==widths[d.shape]&&d.height==heights[d.shape]&&
         (d.width+2)*(d.height+2)<=448&&d.dimension>=0&&d.dimension<=2&&
         d.room_index>=0&&d.room_index<169&&d.safe_grid_index>=0&&d.safe_grid_index<169&&
         d.list_index>=0&&d.list_index<507&&d.enter_door>=-1&&d.enter_door<=7&&
         d.allowed_doors>=0&&d.allowed_doors<=255&&d.clear_count>=0&&d.clear_count<=32767&&
         d.visited_count>=0&&d.backdrop>0&&d.backdrop<63&&
         d.backdrop_seed&&d.backdrop_shift1>0&&d.backdrop_shift1<32&&
         d.backdrop_shift2>0&&d.backdrop_shift2<32&&d.backdrop_shift3>0&&d.backdrop_shift3<32&&
         std::isfinite(d.water_amount)&&d.water_amount>=0&&d.water_amount<=1&&
         std::isfinite(d.camera_x)&&std::isfinite(d.camera_y);
}
}
// 一个 owner 只管理既有原生世界的受控改造前缀；异常后不重放已开始的原生helper。
struct ClientNativeWorldOwner final {
  Game *game=nullptr;
  Room *room=nullptr;
  RoomDescriptor *descriptor=nullptr;
  RoomConfig_Room *config=nullptr;
  Camera *camera=nullptr;
  ClientNativeSceneDescription description{};
  DescriptorScalars old_descriptor{};
  std::uint32_t old_stage=0,old_stage_type=0,old_difficulty=0,old_challenge=0,
                old_current=0,old_last=0,old_dimension=0,old_last_dimension=0,
                old_enter=0,old_leave=0,old_count=0,old_start=0,old_offset=0,old_curses=0;
  unsigned prepare_phase=0,retire_phase=0,prefix=0;
  bool written=false,ready=false,backdrop_ready=false,retired=false,uncertain=false,snapshot=false;
};
namespace {
ClientNativeStep Status(ClientNativeWorldOwner *o,ClientNativeStepState state,const char *reason) noexcept {
  return {state,o?o->prefix:0,o&&o->written,o&&o->retired,reason};
}
bool RootsCurrent(ClientNativeWorldOwner &o) noexcept {
  return g_Game==o.game&&o.game&&o.game->_room==o.room&&o.room&&
         (!o.descriptor||o.room->_descriptor==o.descriptor);
}
// Backdrop教程绘制会直接取首Player；空vector的旧capacity或旧地址不构成有效玩家。
bool PlayersCurrent(const ClientNativeWorldOwner &o) noexcept {
  if(o.game->_playerManager._playerList.empty())return false;
  for(auto *player:o.game->_playerManager._playerList){
    if(!player||!player->_valid)return false;
    unsigned owners=0;auto &members=o.room->_entityList;
    for(auto *list:{&members._mainEL,&members._persistentEL})
      for(unsigned i=0;i<list->_size;++i)if(list->_data[i]==player)++owners;
    unsigned rendered=0;
    for(unsigned i=0;i<members._renderEL._size;++i)if(members._renderEL._data[i]==player)++rendered;
    if(owners!=1||rendered!=1)return false;
  }
  return true;
}
}
ClientNativeWorldOwner *ClientNativeWorldCreate() noexcept {
  return ManagerPhase()?new(std::nothrow) ClientNativeWorldOwner:nullptr;
}
void ClientNativeWorldDestroy(ClientNativeWorldOwner *o) noexcept {
  if(!o||!IsMain()||(!o->retired&&o->written))return;
  delete o;
}
ClientNativeRoots ClientNativeWorldRoots(ClientNativeWorldOwner *o) noexcept {
  if(!IsMain()||!o||!RootsCurrent(*o)||o->retired||o->uncertain)return {};
  return {o->game,o->room,o->descriptor,o->camera};
}
bool ClientNativeWorldHasPrefix(const ClientNativeWorldOwner *o) noexcept {return o&&o->written&&!o->retired;}
bool ClientNativeWorldSafeMenu(const ClientNativeWorldOwner *o) noexcept {return !o||o->retired||!o->written;}
bool ClientNativeWorldCanReuse(const ClientNativeWorldOwner *o,const ClientNativeSceneDescription &d) noexcept {
  if(!IsMain()||!o||!o->ready||o->uncertain||o->retired)return false;
  const auto &a=o->description;
  return a.config_stage==d.config_stage&&a.config_mode==d.config_mode&&a.room_type==d.room_type&&
         a.variant==d.variant&&a.subtype==d.subtype&&a.shape==d.shape&&a.width==d.width&&a.height==d.height&&
         a.room_index==d.room_index&&a.list_index==d.list_index&&a.dimension==d.dimension&&a.backdrop==d.backdrop;
}

// 每轮只推进一个完整原生子步骤；调用前先记录责任，失败不冒称旧场景仍完整。
static ClientNativeStep PrepareImpl(ClientNativeWorldOwner *o,const ClientNativeSceneDescription &d) noexcept {
  if(!o||!ManagerPhase())return Status(o,ClientNativeStepState::Failed,"native_prepare_owner");
  if(!DescriptionValid(d))return Status(o,ClientNativeStepState::Failed,"native_scene_description");
  if(o->uncertain||o->retire_phase||o->retired)return Status(o,ClientNativeStepState::Failed,"native_prepare_cancelled");
  try {
    if(o->ready){
      if(!ClientNativeWorldCanReuse(o,d)||!RootsCurrent(*o))return Status(o,ClientNativeStepState::Failed,"native_scene_rebuild_required");
      if(!GraphicsContext())return Status(o,ClientNativeStepState::Waiting,"native_graphics_context");
      // 同一承载的新目标只写具名目标值，缺省/回到基线的值同样实际覆盖。
      auto &g=*o->game;auto &descriptor=*o->descriptor;
      if(o->description.backdrop_seed!=d.backdrop_seed||o->description.backdrop_shift1!=d.backdrop_shift1||
         o->description.backdrop_shift2!=d.backdrop_shift2||o->description.backdrop_shift3!=d.backdrop_shift3)
        o->backdrop_ready=false;
      o->description=d;o->prefix|=256u;
      g._stage=d.stage;g._stageType=d.stage_type;g._difficulty=d.difficulty;g._challenge=d.challenge;
      g._enterDoor=d.enter_door;descriptor.SafeGridIndex=d.safe_grid_index;
      descriptor.AllowedDoors=d.allowed_doors;descriptor.Flags=d.descriptor_flags;
      descriptor.VisitedCount=d.visited_count;descriptor.ClearCount=std::int16_t(d.clear_count);
      descriptor.SpawnSeed=d.spawn_seed;descriptor.DecorationSeed=d.decoration_seed;descriptor.AwardSeed=d.award_seed;
      o->room->_waterAmount=d.water_amount;
      std::copy(std::begin(d.door_targets),std::end(d.door_targets),descriptor.Doors);
      Vector focus(d.camera_x,d.camera_y);o->camera->SnapToPosition(focus);
      return Status(o,ClientNativeStepState::Ready,"native_description_applied");
    }
    switch(o->prepare_phase){
      case 0: {
        if(!DescriptionValid(d))return Status(o,ClientNativeStepState::Failed,"native_scene_description");
        if(!g_Game||!g_Manager||g_Manager->_state!=1||g_Manager->_starting||
           RunActive()||!g_Game->_room||g_Game->_room->_Camera||
           !g_Game->_playerManager._playerList.empty()||g_Game->_room->_entityList._mainEL._size)
          return Status(o,ClientNativeStepState::Failed,"native_requires_empty_menu_roots");
        o->game=g_Game;o->room=g_Game->_room;o->description=d;
        o->descriptor=&g_Game->_gridRooms[d.list_index];
        o->old_descriptor.Read(*o->descriptor);
        o->old_stage=g_Game->_stage;o->old_stage_type=g_Game->_stageType;o->old_curses=g_Game->_curses;
        o->old_difficulty=g_Game->_difficulty;o->old_challenge=g_Game->_challenge;
        o->old_current=g_Game->_currentRoomIdx;o->old_last=g_Game->_lastRoomIdx;
        o->old_dimension=g_Game->_currentDimensionIdx;o->old_last_dimension=g_Game->_lastRoomDimensionIdx;
        o->old_enter=g_Game->_enterDoor;o->old_leave=g_Game->_leaveDoor;
        o->old_count=g_Game->_nbRooms;o->old_start=g_Game->_startingRoomIdx;
        o->old_offset=g_Game->_roomOffset[d.dimension*169+d.room_index];
        o->snapshot=true;++o->prepare_phase;
        return Status(o,ClientNativeStepState::Progress,"native_menu_roots_captured");
      }
      case 1: {
        if(g_Game!=o->game||o->game->_room!=o->room)return Status(o,ClientNativeStepState::Failed,"native_roots_replaced");
        // LoadStageBinary会删除并重分配整个RoomSet；重连不得重载已加载配置，
        // 否则现有descriptor及退休快照中的Data/Override都会成为悬空裸指针。
        auto &set=o->game->_roomConfig._stages[d.config_stage]._rooms[d.config_mode];
        if(!set._loaded){
          // 只加载真正尚无配置实例的集合；已有storage但loaded=false时不猜其引用寿命。
          if(set._configs||set._count)return Status(o,ClientNativeStepState::Failed,"native_unloaded_config_has_instances");
          o->written=true;o->prefix|=1u;
          o->game->_roomConfig.LoadStageBinary(d.config_stage,d.config_mode);
          if(!set._loaded)return Status(o,ClientNativeStepState::Failed,"native_room_set_not_loaded");
        }
        o->config=o->game->_roomConfig.GetRoomByStageTypeAndVariant(d.config_stage,d.room_type,d.variant,d.config_mode);
        if(!o->config||o->config->Type!=d.room_type||o->config->Subtype!=d.subtype||
           o->config->Shape!=d.shape||o->config->Width!=d.width||o->config->Height!=d.height)
          return Status(o,ClientNativeStepState::Failed,"native_room_config_mismatch");
        ++o->prepare_phase;return Status(o,ClientNativeStepState::Progress,"native_config_resolved");
      }
      case 2: {
        o->written=true;o->prefix|=2u;
        auto &g=*o->game;auto &descriptor=*o->descriptor;
        g._stage=d.stage;g._stageType=d.stage_type;g._difficulty=d.difficulty;g._challenge=d.challenge;
        g._currentRoomIdx=d.room_index;g._lastRoomIdx=d.room_index;
        g._currentDimensionIdx=d.dimension;g._lastRoomDimensionIdx=d.dimension;
        g._enterDoor=d.enter_door;g._leaveDoor=std::uint32_t(-1);
        g._roomOffset[d.dimension*169+d.room_index]=d.list_index;
        g._nbRooms=(std::max)(g._nbRooms,std::uint32_t(d.list_index+1));g._startingRoomIdx=d.room_index;
        descriptor.GridIndex=d.room_index;descriptor.SafeGridIndex=d.safe_grid_index;
        descriptor.ListIndex=d.list_index;descriptor.Dimension=d.dimension;
        descriptor.Data=o->config;descriptor.OverrideData=o->config;
        descriptor.AllowedDoors=d.allowed_doors;descriptor.Flags=d.descriptor_flags;
        descriptor.VisitedCount=d.visited_count;descriptor.ClearCount=std::int16_t(d.clear_count);
        descriptor.SpawnSeed=d.spawn_seed;descriptor.DecorationSeed=d.decoration_seed;descriptor.AwardSeed=d.award_seed;
        std::copy(std::begin(d.door_targets),std::end(d.door_targets),descriptor.Doors);
        o->room->_descriptor=o->descriptor;o->room->_roomType=d.room_type;
        o->room->_gridWidth=d.width+2;o->room->_gridHeight=d.height+2;
        // Room ctor不写+7240，原RoomInit7E97B8才设置；必须安装Authority最终值，
        // 不能把新鲜菜单heap残值当作额外水面，也不能在Client重新裁决淹水条件。
        o->room->_waterAmount=d.water_amount;
        for(int slot=0;slot<8;++slot)o->room->_doorGridPositions[slot]=DoorGridPosition(slot,d.width+2,d.height+2,d.shape);
        InitializeRoomBounds(o->room);
        ++o->prepare_phase;return Status(o,ClientNativeStepState::Progress,"native_descriptor_geometry");
      }
      case 3: {
        if(!GraphicsContext())return Status(o,ClientNativeStepState::Waiting,"native_graphics_context");
        o->prefix|=4u;
        // 使用游戏自身operator new，Room析构最后仍由同一原生分配器释放Camera。
        const auto allocate=reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5);
        auto *camera=static_cast<Camera *>(allocate(sizeof(Camera)));
        if(!camera)return Status(o,ClientNativeStepState::Failed,"native_camera_allocation");
        camera->constructor(o->room); // 原生EH负责其已构造缓冲；异常时owner进入不确定态。
        o->camera=camera;o->room->_Camera=camera;
        Vector focus(d.camera_x,d.camera_y);camera->SnapToPosition(focus);
        ++o->prepare_phase;return Status(o,ClientNativeStepState::Progress,"native_camera_constructed");
      }
      case 4: {
        if(!GraphicsContext())return Status(o,ClientNativeStepState::Waiting,"native_graphics_context");
        o->prefix|=8u;o->game->_hud.LoadGraphics();
        ++o->prepare_phase;return Status(o,ClientNativeStepState::Progress,"native_hud_resources");
      }
      case 5: {
        // 先把结构承载交给I2；Backdrop.Init内部教程surface绘制会解引用首Player，
        // 所以不能在尚无玩家的Preparing阶段执行，也不能靠空vector旧存储侥幸存活。
        ++o->prepare_phase;o->ready=true;
        return Status(o,ClientNativeStepState::Ready,"native_roots_ready_player_pending");
      }
      default:return Status(o,ClientNativeStepState::Failed,"native_prepare_phase");
    }
  }catch(...){o->uncertain=true;return Status(o,ClientNativeStepState::Failed,"native_prepare_exception");}
}
// 只有实际Player roster与EL完成后才进入依赖Player的资源收尾；不创建占位玩家、不开玩法Update。
static ClientNativeStep FinalizeImpl(ClientNativeWorldOwner *o) noexcept {
  if(!o||!ManagerPhase()||!o->ready||o->uncertain||o->retired||!RootsCurrent(*o)||
     !o->camera||o->room->_Camera!=o->camera)return Status(o,ClientNativeStepState::Failed,"native_finalize_roots");
  if(!PlayersCurrent(*o))return Status(o,ClientNativeStepState::Failed,"native_finalize_player_membership");
  if(o->backdrop_ready)return Status(o,ClientNativeStepState::Ready,"native_resources_finalized");
  if(!GraphicsContext())return Status(o,ClientNativeStepState::Waiting,"native_finalize_context");
  try{
    o->written=true;o->prefix|=16u;
    // J273 7E7ADF—7E7B0F同时初始化Backdrop.seed与其独立RNG；Init自身不会重设RNG。
    // 源Backdrop本体保留真正构建seed/shift，descriptor同值或ctor默认0x11111111都不能代替。
    // 这里只重建既定背景surface，不使用/推进共享玩法RNG，也不把本地后态上报为Authority状态。
    auto &backdrop=o->room->_backdrop;
    backdrop.seed=o->description.backdrop_seed;
    backdrop.rng._seed=o->description.backdrop_seed;
    backdrop.rng._shift1=o->description.backdrop_shift1;
    backdrop.rng._shift2=o->description.backdrop_shift2;
    backdrop.rng._shift3=o->description.backdrop_shift3;
    o->room->_backdrop.Init(o->description.backdrop,true);
    o->backdrop_ready=true;
    return Status(o,ClientNativeStepState::Ready,"native_resources_finalized");
  }catch(...){o->uncertain=true;return Status(o,ClientNativeStepState::Failed,"native_finalize_exception");}
}
// 只检查实际结构资格；A/T提交仍由公共applier在全部字段/资源完成后决定。
ClientNativeStep ClientNativeWorldCheckCommit(ClientNativeWorldOwner *o) noexcept {
  if(!o||!ManagerPhase()||!o->ready||!o->backdrop_ready||o->uncertain||!RootsCurrent(*o)||
     !o->camera||o->room->_Camera!=o->camera)return Status(o,ClientNativeStepState::Failed,"native_commit_roots");
  if(!PlayersCurrent(*o))return Status(o,ClientNativeStepState::Failed,"native_player_membership");
  return Status(o,ClientNativeStepState::Ready,"native_commit_roots_valid");
}
// 采用Exit已核的具名清理顺序，不调用含保存/Online切换的Exit或菜单请求大函数。
static ClientNativeStep RetireImpl(ClientNativeWorldOwner *o) noexcept {
  if(!o||!ManagerPhase())return Status(o,ClientNativeStepState::Failed,"native_retire_owner");
  if(o->retired)return Status(o,ClientNativeStepState::Ready,"native_retired");
  if(!o->written){o->retired=true;return Status(o,ClientNativeStepState::Ready,"native_unwritten_cancelled");}
  if(o->uncertain||g_Game!=o->game)return Status(o,ClientNativeStepState::Failed,"native_owner_uncertain");
  if(!GraphicsContext())return Status(o,ClientNativeStepState::Waiting,"native_retire_context");
  try {
    switch(o->retire_phase){
      case 0:
        if(o->game->_room!=o->room)return Status(o,ClientNativeStepState::Failed,"native_retire_room_replaced");
        // 732D20 __thiscall(Game*)：先析构/释放旧Room，再构造发布菜单空Room；不能重入或重试异常前缀。
        o->prefix|=32u;
        reinterpret_cast<void (__thiscall *)(Game *)>(Base+0x332d20)(o->game);
        o->room=o->game->_room;o->camera=nullptr;
        if(!o->room||o->room->_Camera){o->uncertain=true;return Status(o,ClientNativeStepState::Failed,"native_retire_room_postcondition");}
        ++o->retire_phase;return Status(o,ClientNativeStepState::Progress,"native_room_retired");
      case 1:
        // 98F7C0 __thiscall(PlayerManager*)：必须在新的有效Room已经发布后清Player roster。
        o->prefix|=64u;
        reinterpret_cast<void (__thiscall *)(PlayerManager *)>(Base+0x58f7c0)(&o->game->_playerManager);
        if(!o->game->_playerManager._playerList.empty()){o->uncertain=true;return Status(o,ClientNativeStepState::Failed,"native_retire_player_postcondition");}
        ++o->retire_phase;return Status(o,ClientNativeStepState::Progress,"native_players_retired");
      case 2:
        o->prefix|=128u;
        reinterpret_cast<void (__thiscall *)(HUD *)>(Base+0x57ab10)(&o->game->_hud);
        reinterpret_cast<void (__thiscall *)(HUD *)>(Base+0x579d80)(&o->game->_hud);
        ++o->retire_phase;return Status(o,ClientNativeStepState::Progress,"native_hud_retired");
      case 3: {
        auto &g=*o->game;const auto &d=o->description;
        o->old_descriptor.Restore(*o->descriptor);
        g._roomOffset[d.dimension*169+d.room_index]=o->old_offset;
        g._stage=o->old_stage;g._stageType=o->old_stage_type;g._curses=o->old_curses;
        g._difficulty=o->old_difficulty;g._challenge=o->old_challenge;
        g._currentRoomIdx=o->old_current;g._lastRoomIdx=o->old_last;
        g._currentDimensionIdx=o->old_dimension;g._lastRoomDimensionIdx=o->old_last_dimension;
        g._enterDoor=o->old_enter;g._leaveDoor=o->old_leave;g._nbRooms=o->old_count;g._startingRoomIdx=o->old_start;
        if(!g_Manager||g_Manager->_state!=1||g_Manager->_starting||RunActive()||
           g._room->_entityList._mainEL._size||!g._playerManager._playerList.empty())
          return Status(o,ClientNativeStepState::Failed,"native_retire_menu_postcondition");
        o->descriptor=nullptr;o->retired=true;o->ready=false;o->prefix=0;
        return Status(o,ClientNativeStepState::Ready,"native_safe_menu");
      }
      default:return Status(o,ClientNativeStepState::Failed,"native_retire_phase");
    }
  }catch(...){o->uncertain=true;return Status(o,ClientNativeStepState::Failed,"native_retire_exception");}
}
// 用途根覆盖本次完整子步骤；任何意外Update/完整Render入口都由外层常驻dispatcher拒绝。
ClientNativeStep ClientNativeWorldPrepare(ClientNativeWorldOwner *o,const ClientNativeSceneDescription &d) noexcept {
  ClientDispatchScope dispatch(ClientOperation::Build,ClientEntry::NativeBuild);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeBuild)!=ClientEntryDecision::CallOriginal)
    return Status(o,ClientNativeStepState::Failed,"native_build_dispatch");
  const auto step=PrepareImpl(o,d);
  ClientDispatchReturned(ClientEntry::NativeBuild);
  if(!dispatch.Succeeded())return Status(o,ClientNativeStepState::Failed,"native_build_dispatch_return");
  return step;
}
ClientNativeStep ClientNativeWorldFinalize(ClientNativeWorldOwner *o) noexcept {
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)
    return Status(o,ClientNativeStepState::Failed,"native_finalize_dispatch");
  const auto step=FinalizeImpl(o);
  ClientDispatchReturned(ClientEntry::NativeApply);
  if(!dispatch.Succeeded())return Status(o,ClientNativeStepState::Failed,"native_finalize_dispatch_return");
  return step;
}
ClientNativeStep ClientNativeWorldRetire(ClientNativeWorldOwner *o) noexcept {
  ClientDispatchScope dispatch(ClientOperation::Retire,ClientEntry::NativeTeardown);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeTeardown)!=ClientEntryDecision::CallOriginal)
    return Status(o,ClientNativeStepState::Failed,"native_teardown_dispatch");
  const auto step=RetireImpl(o);
  ClientDispatchReturned(ClientEntry::NativeTeardown);
  if(!dispatch.Succeeded())return Status(o,ClientNativeStepState::Failed,"native_teardown_dispatch_return");
  return step;
}
} // namespace cs::game
