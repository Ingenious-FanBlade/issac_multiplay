#include "IsaacRepentance.h"
#include "game/client/scene/client_native_world.h"
#include "game/client/scene/client_dto_memory.h"
#include "replica/room_geometry.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <new>
#include <iterator>
#include <cstring>
#include <cstddef>
#include <memory>
#include <windows.h>
#include <GL/gl.h>

namespace cs::game {
// 复用现有主线程run位读取；不为SDK单元引入C++20 Authority模型头。
bool RunActive() noexcept;
namespace {
// 仅保存本适配实际修改的具名标量；descriptor 的原生容器由已有构造 owner 管理。
struct DescriptorScalars final {
  int grid=0,safe=0,list=0,dimension=0,allowed=0,flags=0,display_flags=0,visited=0;
  std::int16_t clear=0;
  std::int16_t money=0,shop=0,discount=0,poop=0,keeper=0;
  int greed_subtype=0,pits=0;
  RoomConfig_Room *data=nullptr,*override_data=nullptr;
  int doors[8]{};
  std::uint32_t spawn=0,decoration=0,award=0;
  void Read(RoomDescriptor &d) noexcept {
    grid=d.GridIndex;safe=d.SafeGridIndex;list=d.ListIndex;dimension=d.Dimension;
    data=d.Data;override_data=d.OverrideData;allowed=d.AllowedDoors;flags=d.Flags;
    display_flags=d.DisplayFlags;visited=d.VisitedCount;clear=d.ClearCount;spawn=d.SpawnSeed;decoration=d.DecorationSeed;award=d.AwardSeed;
    money=d.TotalMoneySpent;shop=d.ShopItemIdx;discount=d.ShopItemDiscountIdx;poop=d.PoopCount;keeper=d.TKeeperCoinSpawns;greed_subtype=d.GreedSubType;pits=d.PitsCount;
    std::copy(std::begin(d.Doors),std::end(d.Doors),doors);
  }
  void Restore(RoomDescriptor &d) const noexcept {
    d.GridIndex=grid;d.SafeGridIndex=safe;d.ListIndex=list;d.Dimension=dimension;
    d.Data=data;d.OverrideData=override_data;d.AllowedDoors=allowed;d.Flags=flags;
    d.DisplayFlags=display_flags;d.VisitedCount=visited;d.ClearCount=clear;d.SpawnSeed=spawn;d.DecorationSeed=decoration;d.AwardSeed=award;
    d.TotalMoneySpent=money;d.ShopItemIdx=shop;d.ShopItemDiscountIdx=discount;d.PoopCount=poop;d.TKeeperCoinSpawns=keeper;d.GreedSubType=greed_subtype;d.PitsCount=pits;
    std::copy(std::begin(doors),std::end(doors),d.Doors);
  }
};
// I06只用已核有界数组，不调用会首次构造静态fallback、或解释-100/-101路由的Getter。
RoomDescriptor *DescriptorStorage(Game &g,const ClientNativeDescriptorDescription &d) noexcept {
  if(d.source_slot>=-20&&d.source_slot<=-1&&d.source_slot!=-19&&d.storage==-1)
    return &g._negativeGridRooms[-d.source_slot-1];
  return !d.source_slot&&d.storage>=0&&d.storage<507?&g._gridRooms[d.storage]:nullptr;
}
// SDK将J273当前维度offset指针保留为pad2；206D10的负维度分支实际读取此字段。
// memcpy避免把字符padding强转为带别名风险的指针lvalue；退休恢复原始指针位。
static_assert(offsetof(Game,pad2)==0x182c8&&sizeof(void *)==4,"J273 current dimension offsets");
std::uint32_t CurrentOffsets(const Game &g) noexcept {std::uint32_t bits=0;std::memcpy(&bits,g.pad2,4);return bits;}
void CurrentOffsets(Game &g,std::uint32_t bits) noexcept {std::memcpy(g.pad2,&bits,4);}
struct ConfigurationPair final {RoomConfig_Room *data=nullptr,*override_data=nullptr;};
// SDK以bitset<8>声明这两处4字节存储；按J273实际word保留全部原始位，不经构造器截掉高位。
template <class T> std::uint32_t ConfigurationBits(const T &value){static_assert(sizeof(T)==4);std::uint32_t bits=0;std::memcpy(&bits,&value,4);return bits;}
void ApplyDescriptor(RoomDescriptor &d,const ClientNativeDescriptorDescription &s,const ConfigurationPair &configs) noexcept {
  d.GridIndex=s.grid;d.SafeGridIndex=s.safe;d.ListIndex=s.list;d.Dimension=s.dimension;
  d.Data=configs.data;d.OverrideData=configs.override_data;
  d.AllowedDoors=s.allowed;d.Flags=s.flags;d.VisitedCount=s.visited;d.ClearCount=std::int16_t(s.clear_count);
  d.DisplayFlags=s.display_flags;
  d.SpawnSeed=s.spawn;d.DecorationSeed=s.decoration;d.AwardSeed=s.award;
  std::copy(s.doors.begin(),s.doors.end(),d.Doors);
}
// 借用仅限内容完全相同的已加载资源；不LoadStageBinary，避免退休快照持有悬空Data。
// 内容不同或尚未加载时使用SDK具名构造的自有深副本，释放在Room退休且descriptor恢复之后。
bool ConfigurationMatches(const RoomConfig_Room *c,const replica::RoomConfiguration &s,unsigned depth=0) {
  if(s.is_null)return !c;
  if(!c||depth>8||c->StageId!=s.stage||c->Mode!=s.mode||c->Type!=s.type||c->Variant!=s.variant||c->Subtype!=s.subtype||
     c->Shape!=s.shape||c->Width!=s.width||c->Height!=s.height||c->Name!=s.name||c->Difficulty!=s.difficulty||
     c->InitialWeight!=s.initial_weight||c->Weight!=s.weight||c->Doors!=s.doors||ConfigurationBits(c->QuestDoors)!=s.quest_doors||
     ConfigurationBits(c->Flags)!=s.flags||c->originalVariant!=s.original_variant||c->SpawnCount!=s.spawns.size()||s.merged.size()!=2)return false;
  if(c->SpawnCount&&!c->Spawns)return false;
  for(unsigned i=0;i<c->SpawnCount;++i){const auto &a=c->Spawns[i];const auto &b=s.spawns[i];
    if(a.X!=b.x||a.Y!=b.y||a.SumWeights!=b.sum_weights||a.CountEntries!=b.entries.size()||(a.CountEntries&&!a.Entries))return false;
    for(unsigned j=0;j<a.CountEntries;++j){const auto &x=a.Entries[j];const auto &y=b.entries[j];if(x.type!=y.type||x.variant!=y.variant||x.subtype!=y.subtype||x.weight!=y.weight)return false;}}
  return ConfigurationMatches(c->MergedData[0],s.merged[0],depth+1)&&ConfigurationMatches(c->MergedData[1],s.merged[1],depth+1);
}
RoomConfig_Room *OwnConfiguration(const replica::RoomConfiguration &s,std::vector<std::unique_ptr<RoomConfig_Room>> &owner) {
  if(s.is_null)return nullptr;
  auto value=std::make_unique<RoomConfig_Room>();auto *c=value.get();owner.push_back(std::move(value));
  c->StageId=s.stage;c->Mode=s.mode;c->Type=s.type;c->Variant=s.variant;c->Subtype=s.subtype;c->Name=s.name;c->Difficulty=s.difficulty;
  c->InitialWeight=s.initial_weight;c->Weight=s.weight;c->Doors=s.doors;std::memcpy(&c->QuestDoors,&s.quest_doors,4);c->Width=std::int8_t(s.width);c->Height=std::int8_t(s.height);
  c->Shape=s.shape;std::memcpy(&c->Flags,&s.flags,4);c->originalVariant=s.original_variant;
  // 先完整分配再发布SpawnCount；SDK析构逐项拥有Entries，不接管游戏共享数组。
  if(!s.spawns.empty()){c->Spawns=new RoomSpawn[s.spawns.size()];c->SpawnCount=std::uint16_t(s.spawns.size());}
  for(unsigned i=0;i<s.spawns.size();++i){auto &a=c->Spawns[i];const auto &b=s.spawns[i];a.X=b.x;a.Y=b.y;a.SumWeights=b.sum_weights;
    if(!b.entries.empty()){a.Entries=new RoomEntry[b.entries.size()];a.CountEntries=std::uint8_t(b.entries.size());}
    for(unsigned j=0;j<b.entries.size();++j){a.Entries[j].type=b.entries[j].type;a.Entries[j].variant=b.entries[j].variant;a.Entries[j].subtype=b.entries[j].subtype;a.Entries[j].weight=b.entries[j].weight;}}
  for(unsigned i=0;i<2;++i)c->MergedData[i]=OwnConfiguration(s.merged[i],owner);return c;
}
RoomConfig_Room *ResolveConfiguration(Game &g,const replica::RoomConfiguration &s,std::vector<std::unique_ptr<RoomConfig_Room>> &owner) {
  if(s.is_null)return nullptr;
  auto &set=g._roomConfig._stages[s.stage]._rooms[s.mode];
  if(set._loaded){auto *candidate=g._roomConfig.GetRoomByStageTypeAndVariant(s.stage,s.type,s.variant,s.mode);
    if(ConfigurationMatches(candidate,s))return candidate;}
  // 冷Client中相同内容也复用同一自有配置；Data/Override相同不会制造两份替代资源。
  for(const auto &candidate:owner)if(ConfigurationMatches(candidate.get(),s))return candidate.get();
  return OwnConfiguration(s,owner);
}
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
  // 窄房有效区域不等于数组尺寸；采用来源实际grid_width/height，最多448格。
  if(d.descriptors.empty()||d.descriptors.size()>526)return false;
  bool storage[527]{},occupied[507]{};
  for(const auto &s:d.descriptors){
    if(!s.key_run||!s.key_id||!s.construction_revision||s.dimension<0||s.dimension>2||s.occupied_count>4||
       s.allowed<0||s.allowed>255||s.clear_count<0||s.clear_count>32767||s.visited<0)return false;
    const bool off=s.source_slot<0;
    if(off?(s.source_slot<-20||s.source_slot==-19||s.storage!=-1||s.occupied_count!=0):
        (s.source_slot!=0||s.storage<0||s.storage>=507||s.list!=s.storage||s.grid<0||s.grid>168||s.safe<0||s.safe>168))return false;
    const int slot=off?506-s.source_slot:s.storage;if(storage[slot])return false;storage[slot]=true;
    bool safe=off||!s.occupied_count;
    for(unsigned i=0;i<s.occupied_count;++i){const int index=s.occupied[i];if(index<0||index>168||occupied[s.dimension*169+index])return false;occupied[s.dimension*169+index]=true;safe|=index==s.safe;}
    if(!safe)return false;
    if(!(s.data_null&&s.override_null)&&(s.config_stage<0||s.config_stage>=37||s.config_mode<0||s.config_mode>1||s.type<1||s.type>29||
       s.subtype<0||s.shape<1||s.shape>12||s.width<1||s.width>26||s.height<1||s.height>14))return false;
    if(!off&&s.occupied_count){std::array<int,4> cells{};unsigned count=0;
      if(!replica::RoomShapeCells(s.grid,s.shape,cells,count)||count!=s.occupied_count||
         !std::equal(cells.begin(),cells.begin()+count,s.occupied.begin()))return false;}
  }
  const auto &current=d.descriptors.front();
  if(d.grid_index!=current.grid||d.safe_grid_index!=current.safe||d.list_index!=current.list||d.dimension!=current.dimension||
     (current.source_slot?d.room_index!=current.source_slot:
       std::find(current.occupied.begin(),current.occupied.begin()+current.occupied_count,d.room_index)==current.occupied.begin()+current.occupied_count))return false;
  return d.stage>=1&&d.stage<=13&&d.stage_type>=0&&d.stage_type<=5&&
         d.difficulty>=0&&d.difficulty<=3&&d.challenge>=0&&d.challenge<1024&&
         d.config_stage>=0&&d.config_stage<37&&d.config_mode>=0&&d.config_mode<=1&&
         d.room_type>=1&&d.room_type<=29&&d.subtype>=0&&
         d.shape>=1&&d.shape<=12&&d.width>=1&&d.width<=26&&d.height>=1&&d.height<=14&&
         d.grid_width>=3&&d.grid_width<=28&&d.grid_height>=3&&d.grid_height<=16&&d.grid_width*d.grid_height<=448&&d.dimension>=0&&d.dimension<=2&&
         d.room_index>=-20&&d.room_index<169&&d.room_index!=-19&&d.enter_door>=-1&&d.enter_door<=7&&
         d.starting_index>=0&&d.starting_index<169&&
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
  // 保存所有将被改写的已构造标量；不复制容器、不析构descriptor，不读取Online槽。
  std::array<DescriptorScalars,527> old_descriptors{};
  std::array<std::uint32_t,507> old_offsets{};
  std::vector<ConfigurationPair> descriptor_configs;
  std::vector<std::unique_ptr<RoomConfig_Room>> owned_configs;
  std::uint32_t old_stage=0,old_stage_type=0,old_difficulty=0,old_challenge=0,
                old_current=0,old_last=0,old_dimension=0,old_last_dimension=0,
                old_enter=0,old_leave=0,old_count=0,old_start=0,old_curses=0,old_current_offsets=0;
  unsigned prepare_phase=0,retire_phase=0,prefix=0;
  unsigned old_greed_wave=0;
  int old_ambush_wave=0,old_ambush_max=0,old_ambush_bosses=0;
  bool old_ambush_active=false;
  bool written=false,ready=false,backdrop_ready=false,backdrop_pose_ready=false,retired=false,uncertain=false,snapshot=false;
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
std::uint64_t ClientNativeWorldValueBytes(const ClientNativeWorldOwner *o){return o?sizeof(*o)+dto_memory::Extra(o->description):0;}
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
  if(a.descriptors.size()!=d.descriptors.size())return false;
  for(unsigned i=0;i<a.descriptors.size();++i){const auto &x=a.descriptors[i],&y=d.descriptors[i];
    if(x.key_run!=y.key_run||x.key_id!=y.key_id||x.grid!=y.grid||x.safe!=y.safe||x.list!=y.list||x.dimension!=y.dimension||
       x.storage!=y.storage||x.source_slot!=y.source_slot||x.occupied_count!=y.occupied_count||x.occupied!=y.occupied||
       x.config_stage!=y.config_stage||x.config_mode!=y.config_mode||x.type!=y.type||x.variant!=y.variant||x.subtype!=y.subtype||
       x.shape!=y.shape||x.width!=y.width||x.height!=y.height||x.data!=y.data||x.override_data!=y.override_data)return false;
  }
  return a.config_stage==d.config_stage&&a.config_mode==d.config_mode&&a.room_type==d.room_type&&
         a.variant==d.variant&&a.subtype==d.subtype&&a.shape==d.shape&&a.width==d.width&&a.height==d.height&&
         a.grid_width==d.grid_width&&a.grid_height==d.grid_height&&a.room_index==d.room_index&&a.list_index==d.list_index&&a.dimension==d.dimension&&a.backdrop==d.backdrop;
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
      auto &g=*o->game;
      if(o->description.backdrop_seed!=d.backdrop_seed||o->description.backdrop_shift1!=d.backdrop_shift1||
         o->description.backdrop_shift2!=d.backdrop_shift2||o->description.backdrop_shift3!=d.backdrop_shift3||!(o->description.backdrop_config==d.backdrop_config)||
         !(o->description.backdrop_wall_trace==d.backdrop_wall_trace))
        o->backdrop_ready=false;
      o->description=d;o->backdrop_pose_ready=false;o->prefix|=256u;
      g._stage=d.stage;g._stageType=d.stage_type;g._difficulty=d.difficulty;g._challenge=d.challenge;
      g._enterDoor=d.enter_door;g._startingRoomIdx=d.starting_index;
      for(unsigned i=0;i<d.descriptors.size();++i)ApplyDescriptor(*DescriptorStorage(g,d.descriptors[i]),d.descriptors[i],o->descriptor_configs[i]);
      o->room->_waterAmount=d.water_amount;
      if(!ClientRoomStateApply(&g,o->room,d.room_state))return Status(o,ClientNativeStepState::Failed,"native_room_state_apply");
      if(!ClientCameraApply(o->camera,d.map.state.camera))return Status(o,ClientNativeStepState::Failed,"native_camera_values");
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
        o->descriptor=DescriptorStorage(*g_Game,d.descriptors.front());
        if(!o->descriptor)return Status(o,ClientNativeStepState::Failed,"native_descriptor_storage");
        o->descriptor_configs.resize(d.descriptors.size());
        for(unsigned i=0;i<507;++i)o->old_descriptors[i].Read(g_Game->_gridRooms[i]);
        for(unsigned i=0;i<20;++i)if(i!=18)o->old_descriptors[507+i].Read(g_Game->_negativeGridRooms[i]);
        std::copy(std::begin(g_Game->_roomOffset),std::end(g_Game->_roomOffset),o->old_offsets.begin());
        o->old_current_offsets=CurrentOffsets(*g_Game);
        o->old_stage=g_Game->_stage;o->old_stage_type=g_Game->_stageType;o->old_curses=g_Game->_curses;
        o->old_difficulty=g_Game->_difficulty;o->old_challenge=g_Game->_challenge;
        o->old_current=g_Game->_currentRoomIdx;o->old_last=g_Game->_lastRoomIdx;
        o->old_dimension=g_Game->_currentDimensionIdx;o->old_last_dimension=g_Game->_lastRoomDimensionIdx;
        o->old_enter=g_Game->_enterDoor;o->old_leave=g_Game->_leaveDoor;
        o->old_count=g_Game->_nbRooms;o->old_start=g_Game->_startingRoomIdx;
        o->old_greed_wave=g_Game->_greedModeWave;o->old_ambush_wave=g_Game->_ambush.currentWave;
        o->old_ambush_active=g_Game->_ambush.active;o->old_ambush_max=g_Game->_ambush.maxBossWaves;o->old_ambush_bosses=g_Game->_ambush.bossesPerWave;
        o->snapshot=true;++o->prepare_phase;
        return Status(o,ClientNativeStepState::Progress,"native_menu_roots_captured");
      }
      case 1: {
        if(g_Game!=o->game||o->game->_room!=o->room)return Status(o,ClientNativeStepState::Failed,"native_roots_replaced");
        // LoadStageBinary会删除并重分配整个RoomSet；重连不得重载已加载配置，
        // 否则现有descriptor及退休快照中的Data/Override都会成为悬空裸指针。
        for(unsigned i=0;i<d.descriptors.size();++i){const auto &s=d.descriptors[i];
          auto &pair=o->descriptor_configs[i];pair.data=ResolveConfiguration(*o->game,s.data,o->owned_configs);
          pair.override_data=ResolveConfiguration(*o->game,s.override_data,o->owned_configs);
          if((!s.data_null&&!pair.data)||(!s.override_null&&!pair.override_data))return Status(o,ClientNativeStepState::Failed,"native_room_config_mismatch");
        }
        o->config=o->descriptor_configs.front().override_data?o->descriptor_configs.front().override_data:o->descriptor_configs.front().data;
        if(!o->config)return Status(o,ClientNativeStepState::Failed,"native_active_config_missing");
        ++o->prepare_phase;return Status(o,ClientNativeStepState::Progress,"native_config_resolved");
      }
      case 2: {
        o->written=true;o->prefix|=2u;
        auto &g=*o->game;
        g._stage=d.stage;g._stageType=d.stage_type;g._difficulty=d.difficulty;g._challenge=d.challenge;
        g._currentRoomIdx=d.room_index;g._lastRoomIdx=d.room_index;
        g._currentDimensionIdx=d.dimension;g._lastRoomDimensionIdx=d.dimension;
        g._enterDoor=d.enter_door;g._leaveDoor=std::uint32_t(-1);
        // 全部旧offset先撤销，避免同索引不同维度/旧楼层描述穿透；未观测条目不制造配置。
        std::fill(std::begin(g._roomOffset),std::end(g._roomOffset),UINT32_MAX);g._nbRooms=0;
        for(auto &s:g._gridRooms){s.Data=nullptr;s.OverrideData=nullptr;}
        for(unsigned i=0;i<20;++i)if(i!=18){g._negativeGridRooms[i].Data=nullptr;g._negativeGridRooms[i].OverrideData=nullptr;}
        for(unsigned i=0;i<d.descriptors.size();++i){const auto &s=d.descriptors[i];
          ApplyDescriptor(*DescriptorStorage(g,s),s,o->descriptor_configs[i]);
          if(!s.source_slot){g._nbRooms=(std::max)(g._nbRooms,std::uint32_t(s.storage+1));
            for(unsigned j=0;j<s.occupied_count;++j)g._roomOffset[s.dimension*169+s.occupied[j]]=std::uint32_t(s.storage);}
        }
        CurrentOffsets(g,std::uint32_t(reinterpret_cast<std::uintptr_t>(&g._roomOffset[d.dimension*169])));
        g._startingRoomIdx=d.starting_index;
        o->room->_descriptor=o->descriptor;o->room->_roomType=d.room_type;
        o->room->_gridWidth=d.grid_width;o->room->_gridHeight=d.grid_height;
        // Room ctor不写+7240，原RoomInit7E97B8才设置；必须安装Authority最终值，
        // 不能把新鲜菜单heap残值当作额外水面，也不能在Client重新裁决淹水条件。
        o->room->_waterAmount=d.water_amount;
        for(int slot=0;slot<8;++slot)o->room->_doorGridPositions[slot]=DoorGridPosition(slot,d.grid_width,d.grid_height,d.shape);
        InitializeRoomBounds(o->room);
        // Camera构造/Snap立即读取边界；正式来源覆盖必须先于Camera，不能先让它缓存临时几何。
        std::memcpy(reinterpret_cast<unsigned char *>(o->room)+0x14,d.room_state.bounds.data(),sizeof(float)*4);
        std::copy(d.room_state.door_positions.begin(),d.room_state.door_positions.end(),o->room->_doorGridPositions);
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
        if(!ClientCameraApply(camera,d.map.state.camera))return Status(o,ClientNativeStepState::Failed,"native_camera_values");
        ++o->prepare_phase;return Status(o,ClientNativeStepState::Progress,"native_camera_constructed");
      }
      case 4: {
        if(!GraphicsContext())return Status(o,ClientNativeStepState::Waiting,"native_graphics_context");
        o->prefix|=8u;o->game->_hud.LoadGraphics();
        ++o->prepare_phase;return Status(o,ClientNativeStepState::Progress,"native_hud_resources");
      }
      case 5: {
        if(!GraphicsContext())return Status(o,ClientNativeStepState::Waiting,"native_room_state_context");
        o->written=true;o->prefix|=512u;
        if(!ClientRoomStateApply(o->game,o->room,d.room_state))return Status(o,ClientNativeStepState::Failed,"native_room_state_apply");
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
  if(o->backdrop_ready&&o->backdrop_pose_ready)return Status(o,ClientNativeStepState::Ready,"native_resources_finalized");
  if(!GraphicsContext())return Status(o,ClientNativeStepState::Waiting,"native_finalize_context");
  if(!ClientNativeCpuQueuesEmpty())return Status(o,ClientNativeStepState::Waiting,"native_finalize_inflight");
  try{
    o->written=true;o->prefix|=16u;
    // J273 7E7ADF—7E7B0F同时初始化Backdrop.seed与其独立RNG；Init自身不会重设RNG。
    // 源Backdrop本体保留真正构建seed/shift，descriptor同值或ctor默认0x11111111都不能代替。
    // 这里只重建既定背景surface，不使用/推进共享玩法RNG，也不把本地后态上报为Authority状态。
    auto &backdrop=o->room->_backdrop;
    auto &config=backdrop.configurations[o->description.backdrop];const auto &source=o->description.backdrop_config;
    std::string *paths[]={&config.gfx,&config.waterGfx,&config.lFloorGfx,&config.nFloorGfx,&config.props,&config.rocks,&config.pit,&config.waterPit,&config.bridge,&config.door,&config.holeInWall,&config.spikes};
    for(unsigned i=0;i<12;++i)*paths[i]=source.paths[i];
    config.backdropId=source.values[0];config.walls=source.values[1];config.wallVariants=source.values[2];config.floors=source.values[3];
    config.floorVariants=source.values[4];config.unk1=source.values[5];config.waterPitsMode=source.values[6];
    if(!o->backdrop_ready){
      backdrop.seed=o->description.backdrop_seed;backdrop.rng._seed=o->description.backdrop_seed;
      backdrop.rng._shift1=o->description.backdrop_shift1;backdrop.rng._shift2=o->description.backdrop_shift2;backdrop.rng._shift3=o->description.backdrop_shift3;
      ClientBackdropReplay replay(&backdrop,o->description.backdrop_wall_trace);
      o->room->_backdrop.Init(o->description.backdrop,true);
      if(!replay.Complete())return Status(o,ClientNativeStepState::Failed,"native_backdrop_wall_trace");
    }
    // 816BB0消费的实际surface输入，SDK仅命名unkVec1/2；保持来源值，不从形状猜尺寸。
    const auto &inputs=o->description.backdrop_surface_inputs;
    backdrop.unkVec1=Vector(inputs[0],inputs[1]);backdrop.unkVec2=Vector(inputs[2],inputs[3]);
    ANM2 *sprites[]={&backdrop.floorANM2,&backdrop._wallDetailANM2,&backdrop._controlsANM2,&backdrop._controlsButtonsANM2};
    for(unsigned i=0;i<4;++i){if(o->description.backdrop_sprite_present[i]){
        if(!ClientApplyResourcePose(sprites[i],o->description.backdrop_sprites[i]).complete)return Status(o,ClientNativeStepState::Failed,"native_backdrop_sprite_apply");
      }else if(sprites[i]->_loaded){sprites[i]->destructor();sprites[i]->constructor();}}
    o->backdrop_ready=o->backdrop_pose_ready=true;
    return Status(o,ClientNativeStepState::Ready,"native_resources_finalized");
  }catch(...){o->uncertain=true;return Status(o,ClientNativeStepState::Failed,"native_finalize_exception");}
}
// 只检查实际结构资格；A/T提交仍由公共applier在全部字段/资源完成后决定。
ClientNativeStep ClientNativeWorldCheckCommit(ClientNativeWorldOwner *o) noexcept {
  if(!o||!ManagerPhase()||!o->ready||!o->backdrop_ready||!o->backdrop_pose_ready||o->uncertain||!RootsCurrent(*o)||
     !o->camera||o->room->_Camera!=o->camera)return Status(o,ClientNativeStepState::Failed,"native_commit_roots");
  if(!PlayersCurrent(*o))return Status(o,ClientNativeStepState::Failed,"native_player_membership");
  const auto &paths=o->description.room_state.grid_paths;
  if(paths.size()!=o->room->_gridWidth*o->room->_gridHeight||!std::equal(paths.begin(),paths.end(),o->room->_gridPaths))
    return Status(o,ClientNativeStepState::Failed,"native_grid_path_cache_changed");
  const auto &d=o->description;const auto &g=*o->game;
  if(std::int32_t(g._currentRoomIdx)!=d.room_index||int(g._currentDimensionIdx)!=d.dimension||
     CurrentOffsets(g)!=std::uint32_t(reinterpret_cast<std::uintptr_t>(&g._roomOffset[d.dimension*169])))
    return Status(o,ClientNativeStepState::Failed,"native_current_descriptor_context");
  for(const auto &s:d.descriptors){const auto *actual=DescriptorStorage(*o->game,s);
    if(!actual||actual->GridIndex!=s.grid||actual->SafeGridIndex!=s.safe||actual->ListIndex!=s.list||actual->Dimension!=s.dimension)
      return Status(o,ClientNativeStepState::Failed,"native_descriptor_location_changed");
    const auto index=std::size_t(&s-d.descriptors.data());const auto &configs=o->descriptor_configs[index];
    if(actual->Data!=configs.data||actual->OverrideData!=configs.override_data)
      return Status(o,ClientNativeStepState::Failed,"native_descriptor_configuration_changed");
    for(unsigned i=0;i<s.occupied_count;++i)if(g._roomOffset[s.dimension*169+s.occupied[i]]!=unsigned(s.storage))
      return Status(o,ClientNativeStepState::Failed,"native_descriptor_offset_changed");
  }
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
        auto &g=*o->game;
        for(unsigned i=0;i<507;++i)o->old_descriptors[i].Restore(g._gridRooms[i]);
        for(unsigned i=0;i<20;++i)if(i!=18)o->old_descriptors[507+i].Restore(g._negativeGridRooms[i]);
        std::copy(o->old_offsets.begin(),o->old_offsets.end(),std::begin(g._roomOffset));
        CurrentOffsets(g,o->old_current_offsets);
        g._stage=o->old_stage;g._stageType=o->old_stage_type;g._curses=o->old_curses;
        g._difficulty=o->old_difficulty;g._challenge=o->old_challenge;
        g._currentRoomIdx=o->old_current;g._lastRoomIdx=o->old_last;
        g._currentDimensionIdx=o->old_dimension;g._lastRoomDimensionIdx=o->old_last_dimension;
        g._enterDoor=o->old_enter;g._leaveDoor=o->old_leave;g._nbRooms=o->old_count;g._startingRoomIdx=o->old_start;
        g._greedModeWave=o->old_greed_wave;g._ambush.currentWave=o->old_ambush_wave;g._ambush.active=o->old_ambush_active;
        g._ambush.maxBossWaves=o->old_ambush_max;g._ambush.bossesPerWave=o->old_ambush_bosses;
        o->descriptor_configs.clear();o->owned_configs.clear();
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
  // Init(true)仍完成原生资源/surface，只显式跳过915890生成本地Authority背景Entity的分支。
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply,ClientEntryMask(ClientEntry::BackdropEffects));
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)
    return Status(o,ClientNativeStepState::Failed,"native_finalize_dispatch");
  const auto step=FinalizeImpl(o);
  ClientDispatchReturned(ClientEntry::NativeApply);
  if(!dispatch.Succeeded())return Status(o,ClientNativeStepState::Failed,"native_finalize_dispatch_return");
  return step;
}
// 热值只进入既有ready owner的具名最终量分支；不会退回菜单构造路径。
ClientNativeStep ClientNativeWorldRefresh(ClientNativeWorldOwner *o,const ClientNativeSceneDescription &d) noexcept {
  if(!ClientNativeWorldCanReuse(o,d))return Status(o,ClientNativeStepState::Failed,"native_stream_structure_changed");
  ClientDispatchScope dispatch(ClientOperation::Apply,ClientEntry::NativeApply);
  if(!dispatch.Ready()||ClientDispatchBefore(ClientEntry::NativeApply)!=ClientEntryDecision::CallOriginal)return Status(o,ClientNativeStepState::Failed,"native_stream_dispatch");
  const auto result=PrepareImpl(o,d);ClientDispatchReturned(ClientEntry::NativeApply);if(!dispatch.Succeeded())return Status(o,ClientNativeStepState::Failed,"native_stream_dispatch_return");return result;
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
