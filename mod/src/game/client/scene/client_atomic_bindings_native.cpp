#include "IsaacRepentance.h"
#include "game/client/scene/client_atomic_bindings_native.h"
#include "game/client/scene/client_lifetime.h"
#include "game/client/entity/client_entity_budget.h"
#include "game/client/entity/client_objects_native.h"
#include "game/client/isolation/client_player_isolation.h"
#include "game/client/player/client_player_spoof_layout.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include <algorithm>
#include <bitset>
#include <cstring>
#include <new>
#include <typeinfo>
#include <vector>
#include <memory>

namespace cs::game {
bool ClientBinaryEntityInitialize(void *,unsigned,unsigned,unsigned,unsigned,unsigned,const char *&);
bool ClientBinaryEntityCleanup(void *,unsigned,bool init_auxiliary) noexcept;
bool ReadMemory(const void *,void *,std::size_t) noexcept;
bool ClientNativeReadWindow::Copy(const void *source,void *output,std::size_t size) noexcept {
  if(!IsMain()||!source||!output)return false;
  const auto address=std::uint64_t(reinterpret_cast<std::uintptr_t>(source));
  if(address>=world::kNativeAddressLimit||size>world::kNativeAddressLimit-address)return false;
  for(auto cursor=address;cursor<address+size;){
    if(!recent_.Contains(cursor)){
      const auto end=regions_.begin()+used_;
      const auto at=std::upper_bound(regions_.begin(),end,cursor,
        [](auto address,const auto &region){return address<region.first;});
      if(at!=regions_.begin()&&(at-1)->Contains(cursor))recent_=*(at-1);
      if(!recent_.Contains(cursor)){
        ++client_native_page_counters.read_queries;
        if(used_==regions_.size())++client_native_page_counters.read_full;
        if(!world::QueryNativeReadRegion(cursor,recent_))return false;
        // 表满时仍查询并保护复制；只缓存当前页，不扩容或跨窗口延长见证。
        if(used_<regions_.size()){
          const auto insert=std::lower_bound(regions_.begin(),end,recent_.first,
            [](const auto &region,auto address){return region.first<address;});
          std::move_backward(insert,end,end+1);*insert=recent_;++used_;
        }
      }
    }
    cursor=recent_.end;
  }
  return world::CopyNativeReadBytes(output,source,size);
}
namespace {
bool Roots(const ClientNativeRoots &roots) noexcept {
  return IsMain()&&g_Game&&roots.game==g_Game&&roots.room&&roots.room==g_Game->_room;
}
bool Window(const ClientNativeRoots &roots,bool retire=false) noexcept {
  return Roots(roots)&&ClientBinaryInstallationScopeActive(retire)&&ClientNativeCpuQueuesEmpty();
}
std::byte *Bytes(void *p) noexcept {return static_cast<std::byte *>(p);}
using Member=ClientBinaryEntityMember;
bool Persistent(const Member &m) noexcept {return m.type==1||m.type==3||(m.type==8&&m.subtype==0);}
bool Wisp(const Member &m) noexcept {return m.type==3&&m.variant==206;}
auto Lists(Room &room) noexcept {
  auto &el=room._entityList;
  return std::array{&el._mainEL,&el._persistentEL,&el._renderEL,&el._updateEL,&el._wispEL,&el._effectEL,&el._tempEL};
}
auto Slots(Room &room) noexcept {return reinterpret_cast<GridEntity **>(Bytes(&room)+0x24);}
unsigned &NextIndex(Room &room) noexcept {return *reinterpret_cast<unsigned *>(Bytes(&room)+0x1214);}
constexpr unsigned kNativeMemberListLimit=16384;
constexpr std::size_t kRenderList=2;
bool ListCurrent(const EntityList_EL &list) noexcept {
  return !list._sublist&&list._size<=list._capacity&&list._capacity<=kNativeMemberListLimit&&(!list._capacity||list._data);
}
// 原生登记及客机显示推进均可按 Z 重排 renderEL。有序地址和位图只核成员身份与重数。
// 调用者已检查列表形状。陌生指针仅用于查找，不解引用，也不改动原生列表。
bool RenderMembersCurrent(const EntityList_EL &list,void *const *expected,std::size_t count) noexcept {
  if(list._size!=count)return false;
  const auto end=expected+count;
  std::bitset<kNativeMemberListLimit> seen;
  for(unsigned n=0;n<list._size;++n){
    const auto *entity=list._data[n];
    const auto at=std::lower_bound(expected,end,entity,std::less<const void *>{});
    if(at==end||*at!=entity)return false;
    const auto slot=std::size_t(at-expected);
    if(seen[slot])return false;
    seen[slot]=true;
  }
  return true;
}
RoomDescriptor &Descriptor(Game &g,unsigned i) noexcept {return i<507?g._gridRooms[i]:g._negativeGridRooms[i-507];}
// 与Host的八位投影配对；保留原生Flags中的其他位。
unsigned DescriptorDisplayFlags(unsigned original,unsigned bits) noexcept {
  return (original&~0x41e25u)|(bits&1u)|((bits&6u)<<9)|((bits&8u)<<15)|((bits&16u)<<1)|
    ((bits&32u)<<4)|((bits&64u)<<6)|((bits&128u)>>5);
}
unsigned Count(const EntityList_EL &list,const Entity *entity) noexcept {
  return list._size?unsigned(std::count(list._data,list._data+list._size,entity)):0;
}
void Erase(EntityList_EL &list,const Entity *entity) noexcept {
  for(auto i=list._size;i>0;--i)if(list._data[i-1]==entity)list.RemoveAt(i-1);
}
void RetireCount(Room &room,unsigned type,unsigned variant,unsigned subtype) {
  reinterpret_cast<void (__thiscall *)(EntityList *,int,int,int)>(Base+0x16d10)(
      &room._entityList,int(type),int(variant),int(subtype));
}
bool Match(const Member &m) noexcept {
  const auto *e=static_cast<const Entity *>(m.object);
  return e&&e->_type==m.type&&e->_variant==m.variant&&e->_subtype==m.subtype;
}
bool Owned(void *const *members,std::size_t count,const void *p) noexcept {
  return count&&std::binary_search(members,members+count,p,std::less<const void *>{});
}
std::array<bool,7> Membership(const Member &m) noexcept {
  return {m.room&&!Persistent(m),m.room&&Persistent(m),m.room&&m.render,m.room&&m.update,m.room&&Wisp(m),false,false};
}
bool Registered(Room &room,const Member &m) noexcept {
  if(!Match(m)||static_cast<const Entity *>(m.object)->_valid!=m.valid)return false;
  if(m.room&&static_cast<const Entity *>(m.object)->_index!=m.index)return false;
  const auto lists=Lists(room);const auto wanted=Membership(m);
  for(std::size_t i=0;i<lists.size();++i)if(!ListCurrent(*lists[i])||Count(*lists[i],static_cast<const Entity *>(m.object))!=unsigned(wanted[i]))return false;
  return true;
}
bool CheckSets(Game &game,const Member *members,std::size_t count,const ClientBinaryRoomMembers &m) noexcept {
  auto &room=*game._room;const auto &roster=game._playerManager._playerList;
  if(!std::equal(m.grids.begin(),m.grids.end(),Slots(room))||!std::equal(m.doors.begin(),m.doors.end(),room._doors)||
     roster.size()!=m.roster_count||!std::equal(roster.begin(),roster.end(),m.roster.begin()))return false;
  const auto lists=Lists(room);std::array<std::size_t,7> sizes{};
  for(std::size_t i=0;i<count;++i){
    if(!Registered(room,members[i]))return false;
    const auto wants=Membership(members[i]);for(std::size_t j=0;j<wants.size();++j)sizes[j]+=wants[j];
  }
  for(std::size_t j=0;j<lists.size();++j)if(!ListCurrent(*lists[j])||lists[j]->_size!=sizes[j])return false;
  return true;
}
void RemoveMembers(Room &room,const Member *members,std::size_t count) {
  for(std::size_t i=0;i<count;++i){
    auto &e=*static_cast<Entity *>(members[i].object);const auto lists=Lists(room);
    const bool registered=Count(*lists[0],&e)||Count(*lists[1],&e);
    for(auto *list:lists)Erase(*list,&e);
    e._valid=false;
    // 原地 Morph 已改变三元组；计数仍属于旧列表登记使用的形态。
    if(registered)RetireCount(room,members[i].type,members[i].variant,members[i].subtype);
  }
}
void SetList(EntityList_EL &list,Entity &e,bool member) noexcept {
  const auto count=Count(list,&e);
  if(!member)Erase(list,&e);
  else if(!count)list._data[list._size++]=&e;
}
bool SetRoster(Game &g,const ClientBinaryRoomMembers &members) {
  auto &roster=g._playerManager._playerList;
  std::array<Entity_Player *,256> players{};
  for(std::size_t i=0;i<members.roster_count;++i){players[i]=static_cast<Entity_Player *>(members.roster[i]);if(!players[i]||!players[i]->ToPlayer())return false;}
  roster.assign(players.begin(),players.begin()+members.roster_count);
  return roster.size()==members.roster_count&&std::equal(roster.begin(),roster.end(),players.begin());
}
template<class T>T Scalar(const ClientBinarySpecialWrite &w) {
  if(w.length!=sizeof(T))throw std::bad_alloc();
  T value;std::memcpy(&value,w.bytes,sizeof(value));return value;
}
bool Save(ClientBinarySpecialUndo &u,void *destination,std::size_t size) noexcept {
  if(!destination||!size||size>u.original.size()||!ReadMemory(destination,u.original.data(),size))return false;
  u.destination=destination;u.original_size=size;u.applied=true;return true;
}
void *Allocate(std::size_t size){return reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(size);}
void Free(void *p){reinterpret_cast<void (__cdecl *)(void *)>(Base+0x68717b)(p);}
void SwapSprite(ANM2 &a,ANM2 &b) {
  Isaac::SwapANM2(&a,&b);
  a._animState._animation=&a;a._overlayAnimState._animation=&a;
  b._animState._animation=&b;b._overlayAnimState._animation=&b;
}
// 把原Sprite的整个所有权移到回退承载。SwapANM2同步修复AnimationState及LayerState的反向指针。
bool ReplaceSprite(ANM2 &sprite,ANM2 *source,ClientBinarySpecialUndo &u) {
  auto *next=static_cast<ANM2 *>(Allocate(sizeof(ANM2)));if(!next)return false;
  u.storage=next;u.destination=&sprite;u.uncertain=true;
  if(source)next->construct_from_copy(source);else next->constructor();
  SwapSprite(sprite,*next);
  u.uncertain=false;u.applied=true;return true;
}
// 交换已核的动画字段与两组 selector 所有权。反向指针、原生未知字节和 LayerState 都留在原承载。
void SwapAnimationPose(AnimationState &live,AnimationState &held) noexcept {
  std::swap(live._layerFrames,held._layerFrames);
  std::swap(live._nullLayerFrames,held._nullLayerFrames);
  std::swap(live._animFrame,held._animFrame);
  std::swap(live._isPlaying,held._isPlaying);
  std::swap(live._currentlyTriggeredEvents,held._currentlyTriggeredEvents);
  std::swap(live._previouslyTriggeredEvents,held._previouslyTriggeredEvents);
}
bool SetSharedPlayerAnimation(AnimationState &live,ANM2 &sprite,bool main,AnimationData *selected,ClientBinarySpecialUndo &u) {
  auto *copy=static_cast<ANM2 *>(Allocate(sizeof(ANM2)));if(!copy)return false;
  u.storage=copy;u.destination=&live;u.uncertain=true;
  copy->construct_from_copy(&sprite);
  if(copy->_layerCount!=sprite._layerCount||(copy->_layerState&&copy->_layerState==sprite._layerState))return false;
  copy->_animState._animation=copy;
  copy->_overlayAnimState._animation=copy;
  auto &held=main?copy->_animState:copy->_overlayAnimState;
  const auto independent=[](const AnimationState &a,const AnimationState &b){
    const std::array own{a._layerFrames,a._nullLayerFrames};
    return std::none_of(own.begin(),own.end(),[&](const auto *p){
      return p&&(p==b._layerFrames||p==b._nullLayerFrames);
    });
  };
  const auto &live_other=main?sprite._overlayAnimState:sprite._animState;
  const auto &held_other=main?copy->_overlayAnimState:copy->_animState;
  if(!independent(live,live_other)||!independent(held,held_other)||
     !independent(held,live)||!independent(held,live_other)||
     !independent(held_other,live)||!independent(held_other,live_other))return false;
  auto *before=held._animData;
  auto *expected=selected?copy->GetAnimationData(selected->_name):nullptr;
  if(expected&&(expected->_length!=selected->_length||expected->_layerCount!=selected->_layerCount||
      expected->_nullLayerCount!=selected->_nullLayerCount))expected=nullptr;
  if(selected&&expected){
    if(main)copy->SetAnimation(selected->_name.c_str(),true);else copy->SetOverlayAnimation(selected->_name.c_str(),true);
  }else if(!selected){
    held._animData=nullptr;held._isPlaying=false;
  }
  if((selected&&!expected)||held._animData!=expected){
    copy->destructor();Free(copy);u.storage=nullptr;u.uncertain=false;return false;
  }
  if(!independent(held,held_other)||!independent(held,live)||!independent(held,live_other))return false;
  held._currentlyTriggeredEvents=held._previouslyTriggeredEvents=0;
  // 临时 Sprite 承接交换出的旧 selector。两边的 AnimationData 仍指向各自资源表。
  const std::array<AnimationData *,2> undo{live._animData,expected};
  static_assert(sizeof(undo)<=sizeof(u.original));
  std::memcpy(u.original.data(),undo.data(),sizeof(undo));
  SwapAnimationPose(live,held);
  live._animData=selected;
  held._animData=before;
  u.applied=true;u.uncertain=false;
  return true;
}
bool SetAnimation(AnimationState &state,AnimationData *asset,ClientBinarySpecialUndo &u,bool shared=false) {
  auto *sprite=state._animation;
  if(!sprite||(&state!=&sprite->_animState&&&state!=&sprite->_overlayAnimState))return false;
  auto *selected=asset?sprite->GetAnimationData(asset->_name):nullptr;
  if(asset&&(!selected||selected->_length!=asset->_length||selected->_layerCount!=asset->_layerCount||
     selected->_nullLayerCount!=asset->_nullLayerCount))return false;
  if(state._animData==selected)return true;
  const bool main=&state==&sprite->_animState;
  if(shared)return SetSharedPlayerAnimation(state,*sprite,main,selected,u);
  if(!ReplaceSprite(*sprite,sprite,u))return false;
  auto &live=main?sprite->_animState:sprite->_overlayAnimState;
  if(!asset){live._animData=nullptr;live._isPlaying=false;live._currentlyTriggeredEvents=live._previouslyTriggeredEvents=0;return true;}
  u.uncertain=true;
  if(main)sprite->SetAnimation(asset->_name.c_str(),true);else sprite->SetOverlayAnimation(asset->_name.c_str(),true);
  live._currentlyTriggeredEvents=live._previouslyTriggeredEvents=0;
  u.uncertain=false;
  return live._animData==sprite->GetAnimationData(asset->_name);
}
}
// 区分对象生命周期与身份字段不一致，只把通过检查的 vptr 留给写段复核。
const char *ClientBinaryInitFailure(ClientBinaryInit &in,ClientNativeReadWindow *reads) noexcept {
  auto *e=static_cast<Entity *>(in.object);
  // vptr和新建非玩家的隔离检查均为纯读；只有下方真正进入ToPlayer时撤销页见证。
  const auto copy=[&](const void *source,void *output,std::size_t size){
    return reads?reads->Copy(source,output,size):ReadMemory(source,output,size);
  };
  void *entry=nullptr;
  if(!e||!copy(e,&in.vptr,sizeof(in.vptr))||!in.vptr||!copy(in.vptr,&entry,sizeof(entry))||!entry)
    return "binary_native_member_entity_vptr";
  if(!in.seed)return "binary_native_member_entity_seed_absent";
  if(*reinterpret_cast<void **>(Bytes(e)+0x310))return "binary_native_member_entity_collision_owner";
  if(!in.fresh){
    if(e->_type!=in.type||e->_variant!=in.variant||e->_subtype!=in.subtype){
#ifdef ISAACCS_E2E
      // 记录当前承载与待装目标的实际身份值，判断失配来自 Host 形态还是本地漂移。
      Trace("client_entity_identity_fault","local=%u/%u/%u target=%u/%u/%u seed=%u/%u",
        e->_type,e->_variant,e->_subtype,in.type,in.variant,in.subtype,e->_initSeed,in.seed);
#endif
      return "binary_native_member_entity_type_changed";
    }
    return e->_initSeed==in.seed?nullptr:"binary_native_member_entity_seed_changed";
  }
  if(e->_valid||e->_exists)return "binary_native_member_entity_new_is_live";
  if(in.category){
    if(!ClientEntityInitIsolationInstalled())return "binary_native_member_entity_init_isolation";
  }else{
    // 原生虚调用前后不沿用任何页信息；不得把新建玩家的回调资格并入纯读成员批次。
    if(reads)reads->Clear();
    const bool player=e->ToPlayer()==e;
    if(reads)reads->Clear();
    if(!player)return "binary_native_member_entity_init_isolation";
  }
  if(in.category==1&&(*reinterpret_cast<unsigned *>(Bytes(e)+0xc00)||*reinterpret_cast<void **>(Bytes(e)+0xf40)))return "binary_native_member_entity_npc_owner";
  return nullptr;
}
static bool TryPlayerBaseInit(Entity &entity,unsigned type,unsigned variant,unsigned subtype,unsigned seed) noexcept {
  __try {
    reinterpret_cast<void (__thiscall *)(Entity *,unsigned,unsigned,unsigned,unsigned)>(Base+0x2a4a20)(
      &entity,type,variant,subtype,seed);
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool ApplyEntityInit(ClientBinaryInit &in,const char *&why) noexcept {
  if(!ClientBinaryInstallationScopeActive(false)||!in.object||*static_cast<void **>(in.object)!=in.vptr)return false;
  if(!in.fresh)return true;
  if(in.entered)return false;
  try {
    auto &e=*static_cast<Entity *>(in.object);
    in.entered=true;
    e.SetParent(static_cast<Entity *>(in.parent));e.SetSpawnerEntity(static_cast<Entity *>(in.spawner));
    e.SetChild(nullptr);e.SetTarget(nullptr);e._minecart.SetReference(nullptr);
    std::memcpy(Bytes(&e)+0x34,&in.spawner_type,4);
    std::memcpy(Bytes(&e)+0x38,&in.spawner_variant,4);
    e._pos=Vector(in.position[0],in.position[1]);e._velocity=Vector(in.velocity[0],in.velocity[1]);
    if(!in.category){
      ClientPlayerIsolationScope player(ClientPlayerOperation::BaseInit);if(!player.Ready())return false;
      auto &body=static_cast<Entity_Player &>(e);
      body._controllerIndex=-1;body._playerIndex=-1;body._playerType=in.player_type;
      if(!TryPlayerBaseInit(e,in.type,in.variant,in.subtype,in.seed)){why="binary_native_player_init_seh";return false;}
    }else if(!ClientBinaryEntityInitialize(&e,in.category,in.type,in.variant,in.subtype,in.seed,why))return false;
    in.returned=true;
    why="binary_native_init_postcondition";
    // Entity+330是Host实际选中的配置，可与按当前三元组执行base Init得到的配置不同。
    // 它由后续priority=4的Config资源操作安装，并在发布前读回；不能在Init刚返回时
    // 提前要求已经等于尚未安装的权威配置（角色切换掉落卡牌即会触发此差异）。
    const bool valid=*static_cast<void **>(in.object)==in.vptr&&!e._valid&&e._type==in.type&&e._variant==in.variant&&
        e._subtype==in.subtype&&e._initSeed==in.seed;
    // 只在失败时记录原生Init的实际结果；不把随机种子、类型或配置差异静默修正为期望值。
    if(!valid)Trace("client_entity_init_fault","category=%u local=%u/%u/%u target=%u/%u/%u seed=%u/%u valid=%u vptr_match=%u config_match=%u",
      in.category,e._type,e._variant,e._subtype,in.type,in.variant,in.subtype,e._initSeed,in.seed,unsigned(e._valid),
      unsigned(*static_cast<void **>(in.object)==in.vptr),unsigned(!in.config||*reinterpret_cast<void **>(Bytes(&e)+0x330)==in.config));
    return valid;
  }catch(...){why="binary_native_init_prefix_retained";return false;}
}
bool ClientBinaryInitApply(ClientBinaryInit &in,const char *&why) noexcept {
  why="binary_native_init_owner";
  __try {return ApplyEntityInit(in,why);}
  __except(EXCEPTION_EXECUTE_HANDLER){why="binary_native_init_seh";return false;}
}
bool ClientBinaryGridInit(void *object,unsigned index) noexcept {
  if(!object||index>=448||!ClientBinaryInstallationScopeActive(false))return false;
  __try {
    auto &grid=*static_cast<GridEntity *>(object);grid._gridIndex=index;grid._collisionClass=0;return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void *ClientBinaryGlobalOverlay(const ClientNativeRoots &roots) noexcept {
  return Roots(roots)?&g_Game->_itemOverlay:nullptr;
}
bool ClientBinaryCreateOverlay(void *&owner) noexcept {
  if(owner||!ClientBinaryInstallationScopeActive(false))return false;
  static_assert(sizeof(ItemOverlay)==0x1260);
  try {
    owner=Allocate(sizeof(ItemOverlay));if(!owner)return false;
    std::memset(owner,0,sizeof(ItemOverlay));
    auto &overlay=*static_cast<ItemOverlay *>(owner);
    overlay._sprite.constructor();overlay._megaMushPlayerSprite.constructor();
    return true;
  }catch(...){return false;}
}
bool ClientBinaryDestroyOverlay(void *&owner) noexcept {
  if(!owner)return true;
  if(!ClientBinaryInstallationScopeActive(true)||!ClientNativeCpuQueuesEmpty())return false;
  try {
    auto &overlay=*static_cast<ItemOverlay *>(owner);if(overlay._player)return false;
    overlay._megaMushPlayerSprite.destructor();overlay._sprite.destructor();
    Free(owner);owner=nullptr;return true;
  }catch(...){return false;}
}
void ClientBinaryRenderOverlay(void *owner) noexcept {
  if(owner)reinterpret_cast<void (__thiscall *)(ItemOverlay *)>(Base+0x584660)(static_cast<ItemOverlay *>(owner));
}
void *ClientBinaryOverlayCurrentPlayer(void *owner) noexcept {
  return owner?static_cast<ItemOverlay *>(owner)->_player:nullptr;
}
bool ClientBinaryOverlayPlayer(void *owner,void *player) noexcept {
  if(!owner||(!ClientBinaryInstallationScopeActive(false)&&!ClientBinaryInstallationScopeActive(true)))return false;
  try{
    auto &overlay=*static_cast<ItemOverlay *>(owner);auto **slot=&overlay._player;
    auto *next=static_cast<Entity_Player *>(player);
    if(*slot==next)return true;
    if(auto *old=*slot)
      reinterpret_cast<void (__thiscall *)(void *,Entity_Player ***)>(Base+0xc8ee0)(Bytes(old)+0x3cc,&slot);
    *slot=next;
    if(next)reinterpret_cast<void (__thiscall *)(Entity_Player *,Entity_Player **)>(Base+0xc79d0)(next,slot);
    return true;
  }catch(...){return false;}
}
bool ClientBinaryShowOverlay(void *owner,int id,int delay,void *player) noexcept {
  if(!owner||!ClientBinaryInstallationScopeActive(false)||id<0||id>46)return false;
  try{
    auto &overlay=*static_cast<ItemOverlay *>(owner);overlay.Show(id,delay,static_cast<Entity_Player *>(player));
    return overlay._overlayID==id&&overlay._player==player;
  }catch(...){return false;}
}
void ClientBinaryHideOverlay(void *owner) noexcept {
  if(owner)static_cast<ItemOverlay *>(owner)->_state=ItemOverlay::INACTIVE;
}
struct ClientBinaryProcedural {
  struct Row {
    ProceduralItem item{};
    std::string gfx;
  };
  using Registry=std::vector<ProceduralItem *>;
  std::vector<Row> rows;
  Registry previous_entries;
  std::array<KAGE_SmartPointer_ImageBase,2> atlas,previous_atlas,original_atlas;
  ProceduralItem **data=nullptr;
  bool attached=false;
};
namespace {
auto &ProceduralEntries() noexcept {return *g_Game->_proceduralItemManager.GetProceduralItems();}
KAGE_SmartPointer_ImageBase &ProceduralAtlas(unsigned n) noexcept {
  return *reinterpret_cast<KAGE_SmartPointer_ImageBase *>(Bytes(&g_Game->_proceduralItemManager)+(n?0x14:0xc));
}
bool SameAtlas(const KAGE_SmartPointer_ImageBase &a,const KAGE_SmartPointer_ImageBase &b) noexcept {
  return a.image==b.image&&a.counter==b.counter;
}
void ProceduralQuad(KAGE_Graphics_ImageBase &image,unsigned index,unsigned stride) {
  const float x=float((index%32)*stride),y=float((index/32)*stride),s=float(stride);
  SourceQuad source{};source._coordinateSpace=SourceQuad::NORMALIZED_UV;
  source._topLeft={0,0};source._topRight={1,0};source._bottomLeft={0,1};source._bottomRight={1,1};
  DestinationQuad dest{};dest._topLeft={x,y};dest._topRight={x+s,y};dest._bottomLeft={x,y+s};dest._bottomRight={x+s,y+s};
  image.Render(source,dest,KColor(1,1,1,1));
}
}
namespace {
bool SameDefinition(const ClientBinaryProcedural::Row &row,const ClientBinaryProceduralDefinition &def) {
  if(row.item.item!=def.configuration||!def.configuration)return false;
  const auto &config=*row.item.item;
  return row.item._id==def.id&&config.id==def.id&&config.name==def.name&&
    config.desc==def.description&&config.gfxFilename==def.gfx;
}
}
bool ClientBinaryCheckProcedural(const ClientNativeRoots &roots,const ClientBinaryProcedural *owner) noexcept {
  if(!Roots(roots))return false;
  if(!owner)return ProceduralEntries().empty();
  const auto &entries=ProceduralEntries();
  return owner->attached&&entries.data()==owner->data&&entries.size()==owner->rows.size()&&
    SameAtlas(ProceduralAtlas(0),owner->atlas[0])&&SameAtlas(ProceduralAtlas(1),owner->atlas[1]);
}
bool ClientBinaryCheckProceduralEntry(const ClientBinaryProcedural *owner,std::size_t index,const ClientBinaryProceduralDefinition &def) noexcept {
  if(!owner||!owner->attached||index>=owner->rows.size())return false;
  const auto &entries=ProceduralEntries();
  return entries.data()==owner->data&&entries.size()==owner->rows.size()&&entries[index]==&owner->rows[index].item&&SameDefinition(owner->rows[index],def);
}
bool ClientBinarySetProcedural(const ClientNativeRoots &roots,ClientBinaryProcedural *&owner,const ClientBinaryProcedural *previous,
    const ClientBinaryProceduralDefinition *defs,std::size_t count,void *atlas,void *small_atlas) noexcept {
  if(!Window(roots)||owner||!ClientBinaryCheckProcedural(roots,previous)||count>1024)return false;
  if(!previous&&!count)return true;
  if(previous&&count<previous->rows.size())return false;
  try{
    auto next=std::make_unique<ClientBinaryProcedural>();
    ClientBinaryProcedural::Registry entries;
    next->rows.resize(count);entries.reserve(count);
    for(unsigned i=0;i<2;++i){
      next->previous_atlas[i]=ProceduralAtlas(i);
      next->original_atlas[i]=previous?previous->original_atlas[i]:next->previous_atlas[i];
      const auto *asset=static_cast<const KAGE_SmartPointer_ImageBase *>(i?small_atlas:atlas);
      if(asset)next->atlas[i]=*asset;
      else if(count)return false;
    }
    if(count){
      if(!__ptr_g_KAGE_Graphics_Manager)return false;
      for(unsigned i=0;i<2;++i){
        const auto &asset=next->atlas[i];const int size=i?512:1024;
        if(!asset.image||!asset.counter||*reinterpret_cast<const std::uintptr_t *>(asset.image)!=Base+0x73bcac||
           asset.image->GetWidth()!=size||asset.image->GetHeight()!=size)return false;
      }
    }
    std::vector<KAGE_SmartPointer_ImageBase> icons(count);
    std::vector<bool> changed(count);
    for(unsigned n=0;n<count;++n){
      const auto &def=defs[n];auto &row=next->rows[n];auto &config=*static_cast<ItemConfig_Item *>(def.configuration);
      if(def.id!=~int(n)||config.id!=def.id)return false;
      row.item._id=def.id;row.item.item=&config;row.gfx=def.gfx;
      entries.push_back(&row.item);
      // 同一资源表的配置可以被存活成员引用；首次建文本，以后核对同一不可变定义。
      if(previous&&n<previous->rows.size()&&previous->rows[n].item.item==&config){
        if(!SameDefinition(previous->rows[n],def))return false;
      }else {config.name.assign(def.name);config.desc.assign(def.description);config.gfxFilename.assign(def.gfx);}
      changed[n]=!previous||n>=previous->rows.size()||previous->rows[n].gfx!=row.gfx||
        !SameAtlas(next->atlas[0],previous->atlas[0])||!SameAtlas(next->atlas[1],previous->atlas[1]);
      if(changed[n]){
        // 部分程序定义原生gfx为空，仍须在本地为原生负ID查询保留可见图集槽。
        const char *path=row.gfx.empty()?"gfx/items/collectibles/collectibles_721_tmtrainer.png":row.gfx.c_str();
        Manager::LoadImage(&icons[n],path,__ptr_g_VertexAttributeDescriptor_Position,false);
        if(!icons[n].image||!icons[n].counter)return false;
        // 同一图集已发布的槽不可原地改写。正常程序ID只追加，路径变更交完整恢复。
        if(previous&&n<previous->rows.size()&&SameAtlas(next->atlas[0],previous->atlas[0])&&
           SameAtlas(next->atlas[1],previous->atlas[1]))return false;
      }
    }
    if(count&&std::any_of(changed.begin(),changed.end(),[](bool v){return v;})){
      auto &g=*__ptr_g_KAGE_Graphics_Manager;
      Rendering::PushCurrentRenderTarget();
      struct Restore {KAGE_Graphics_Manager &g;BlendMode blend;KAGE_Graphics_Shader *shader;
        ~Restore(){Rendering::RestorePreviousRenderTarget();g._blendMode=blend;g._shader=shader;}
      } restore{g,g._blendMode,g._shader};
      g._blendMode=BlendMode();
      // 图集槽位置由负ID决定；使用已选gfx本地绘制。生成器的随机图案与effects均不运行。
      for(unsigned i=0;i<2;++i){
        if(!next->atlas[i].image||!next->atlas[i].counter)return false;
        g.SetCurrentRenderTarget(next->atlas[i].image,false);
        for(unsigned n=0;n<count;++n)if(icons[n].image)ProceduralQuad(*icons[n].image,n,i?16:32);
        g.PresentWithoutSwap();
        if(!ClientNativeCpuQueuesEmpty())return false;
      }
    }
    // 先完成分配，接入真实vector与图集只交换本地所有权。
    owner=next.release();owner->attached=true;
    ProceduralEntries().swap(entries);
    owner->previous_entries.swap(entries);
    owner->data=ProceduralEntries().data();
    for(unsigned i=0;i<2;++i)ProceduralAtlas(i)=owner->atlas[i];
    return ClientBinaryCheckProcedural(roots,owner);
  }catch(...){return false;}
}
bool ClientBinaryRetireProcedural(const ClientNativeRoots &roots,ClientBinaryProcedural *&owner,bool restore,bool committed) noexcept {
  if(!owner)return true;
  if(!Window(roots,true))return false;
  try{
    if(restore&&owner->attached){
      // 完整恢复允许已知vector内的条目漂移；换过整个承载则没有资格接管新的原生owner。
      if(ProceduralEntries().data()!=owner->data)return false;
      if(committed){ClientBinaryProcedural::Registry empty;ProceduralEntries().swap(empty);}
      else ProceduralEntries().swap(owner->previous_entries);
      for(unsigned i=0;i<2;++i)ProceduralAtlas(i)=committed?owner->original_atlas[i]:owner->previous_atlas[i];
    }
    delete owner;owner=nullptr;return true;
  }catch(...){return false;}
}
void ClientBinaryCommitProcedural(ClientBinaryProcedural *owner) noexcept {
  if(!owner)return;
  ClientBinaryProcedural::Registry previous;previous.swap(owner->previous_entries);
  owner->previous_atlas={};
}
unsigned ClientBinaryLocalIndex(void *object) noexcept {
  return object?static_cast<const Entity *>(object)->_index:UINT32_MAX;
}
unsigned ClientBinaryRetireCategory(void *object) noexcept {
  if(!object)return 0;
  switch(static_cast<Entity *>(object)->_type){
    case 1:return 0;case 2:return 2;case 3:return 8;case 4:return 4;case 5:return 7;
    case 6:return 10;case 7:return 5;case 8:return 6;case 9:return 3;case 1000:return 9;case 9001:return 11;
    default:return static_cast<Entity *>(object)->ToNPC()?1u:12u;
  }
}
bool ClientBinaryInstallFrame(const ClientNativeRoots &roots,std::uint32_t value,std::uint32_t &previous) noexcept {
  if(!Window(roots))return false;
  previous=g_Game->_frameCount;std::memcpy(&g_Game->_frameCount,&value,sizeof(value));return true;
}
bool ClientBinaryRestoreFrame(const ClientNativeRoots &roots,std::uint32_t value) noexcept {
  if(!Window(roots,true))return false;
  std::memcpy(&g_Game->_frameCount,&value,sizeof(value));return true;
}
void *ClientBinaryRunClock(const ClientNativeRoots &roots) noexcept {
  return Roots(roots)?Bytes(g_Game)+0x25a74:nullptr;
}
namespace {
// J273 Factory 只检查 valid。归还池槽前还须确认本次移除不再留下 EL 指针。
bool PooledMemberUnlinked(const ClientNativeRoots &roots,const Entity &entity,bool retirement) noexcept {
  if(!Window(roots,retirement)||entity._exists||entity._valid)return false;
  auto &room=*g_Game->_room;
  for(const auto *list:Lists(room))
    if(!ListCurrent(*list)||Count(*list,&entity))return false;
  return true;
}
}
bool ClientBinaryEntityRetire(const ClientNativeRoots &roots,void *object,unsigned category) noexcept {
  if(!object||!Window(roots,true))return false;
  try {
    auto &e=*static_cast<Entity *>(object);if(e._valid)return false;
    if(*reinterpret_cast<void **>(Bytes(&e)+0x310))return false;
    if(!category){
      ClientPlayerIsolationScope scope(ClientPlayerOperation::Retire);if(!scope.Ready())return false;
      // HUD 在 World 间保存原生 Player 指针；实际回收前撤销仍指向此实例的显示槽。
      for(auto &hud:g_Game->_hud._playerHUD)if(hud._player==object)hud._player=nullptr;
      auto &p=static_cast<Entity_Player &>(e);p._twinPlayer.SetReference(nullptr);p._backupPlayer=nullptr;p._heldEntity=nullptr;
      // Player只进入基类清理，避免玩法装备及玩家管理器副作用。
      reinterpret_cast<void (__thiscall *)(Entity *)>(Base+0x2a8000)(&e);
      reinterpret_cast<void (__thiscall *)(Entity *)>(Base+0x2a8060)(&e);
    }else {
      if(!ClientBinaryEntityCleanup(&e,category,false))return false;
      return PooledMemberUnlinked(roots,e,true);
    }
    return !e._exists&&!e._valid;
  }catch(...){return false;}
}
bool ClientBinaryDetachMember(const ClientNativeRoots &,void *,bool,const std::array<std::uint32_t,3> *) noexcept;
bool ClientBinaryRetireLiveMember(const ClientNativeRoots &roots,void *object,bool grid,bool player,
    const std::array<std::uint32_t,3> *registered_shape) noexcept {
  if(!Window(roots,true)||!object)return false;
  __try {
    if(grid)return ClientBinaryDetachMember(roots,object,true);
    if(player)return ClientBinaryDetachMember(roots,object,false)&&ClientBinaryEntityRetire(roots,object,0);
    auto &entity=*static_cast<Entity *>(object);
    const auto category=ClientBinaryRetireCategory(object);
    // Remove/ClearReferences 仍在该实例占用时运行。清 valid 和摘表完成后方可发送 Delete 并归还槽。
    return category&&!*reinterpret_cast<void **>(Bytes(&entity)+0x310)&&
      ClientBinaryEntityCleanup(&entity,category,false)&&ClientBinaryDetachMember(roots,object,false,registered_shape)&&
      PooledMemberUnlinked(roots,entity,true);
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientBinaryRememberInitPools(const ClientNativeRoots &roots,ClientBinaryInitPoolSnapshot &snapshot) noexcept {
  static_assert([]() constexpr {unsigned total=0;for(const auto count:kClientEntityPools)total+=count;return total;}()<=4096);
  if(!Window(roots)||!g_Game->_entityFactory)return false;
  __try {
    snapshot.factory=g_Game->_entityFactory;snapshot.used.reset();
    const auto *base=static_cast<const std::byte *>(snapshot.factory);
    std::size_t ordinal=0;
    for(unsigned category=1;category<kClientEntityPools.size();++category)
      for(unsigned slot=0;slot<kClientEntityPools[category];++slot,++ordinal){
        const auto *entity=reinterpret_cast<const Entity *>(base+kClientEntityPoolOffsets[category]+slot*kClientEntityPoolStrides[category]);
        if(entity->_exists||entity->_valid)snapshot.used.set(ordinal);
      }
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientBinaryCleanupInitPools(const ClientNativeRoots &roots,const ClientBinaryInitPoolSnapshot &before,
    const void *context,bool (*target)(const void *,const void *) noexcept,bool &changed) noexcept {
  if(!Window(roots)||!target||!before.factory||before.factory!=g_Game->_entityFactory)return false;
  __try {
    auto *base=reinterpret_cast<std::byte *>(g_Game->_entityFactory);
    auto &room=*g_Game->_room;
    std::size_t ordinal=0;
    for(unsigned category=1;category<kClientEntityPools.size();++category)
      for(unsigned slot=0;slot<kClientEntityPools[category];++slot,++ordinal){
        if(before.used.test(ordinal))continue;
        auto *entity=reinterpret_cast<Entity *>(base+kClientEntityPoolOffsets[category]+slot*kClientEntityPoolStrides[category]);
        if((!entity->_exists&&!entity->_valid)||target(context,entity))continue;
        const char *reason="binary_init_auxiliary_essm";
        if(!Window(roots)||before.factory!=g_Game->_entityFactory||ClientBinaryRetireCategory(entity)!=category||
            *reinterpret_cast<void **>(Bytes(entity)+0x310)||
            !ClientObjectStagedPoolNotification(entity,false,false,reason))return false;
        for(const auto *list:Lists(room))if(!ListCurrent(*list))return false;
        changed=true;
        Member member;member.object=entity;member.type=entity->_type;member.variant=entity->_variant;member.subtype=entity->_subtype;
        if(!ClientBinaryEntityCleanup(entity,category,true))return false;
        RemoveMembers(room,&member,1);
        if(!PooledMemberUnlinked(roots,*entity,false)||
            !ClientObjectStagedPoolNotification(entity,true,false,reason))return false;
      }
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// 原生成员校验在预检和安装前共用；失败只返回条件名，不修改成员或分配诊断对象。
const char *ClientBinaryMemberFailure(const ClientNativeRoots &roots,void *const *prior,std::size_t prior_count,
    const Member *next,std::size_t count,const ClientBinaryRoomMembers *old_room,const ClientBinaryRoomMembers *new_room) noexcept {
  if(!Roots(roots))return "binary_native_members_roots";
  auto &room=*g_Game->_room;const auto lists=Lists(room);
  std::array<std::size_t,7> needed{};
  for(std::size_t i=0;i<count;++i){auto wants=Membership(next[i]);for(std::size_t j=0;j<wants.size();++j)needed[j]+=wants[j];}
  // Add原生入口可能先加入render/update再按目标撤回，预留瞬时登记及本地index。
  std::size_t new_indices=0;
  for(std::size_t i=0;i<count;++i)if(next[i].room&&next[i].index==UINT32_MAX)++new_indices;
  if(new_indices>UINT32_MAX-NextIndex(room))return "binary_native_members_index_capacity";
  unsigned render=0,update=0;
  for(std::size_t i=0;i<count;++i)if(next[i].room){
    if(render>=lists[2]->_capacity||(next[i].add_update&&update>=lists[3]->_capacity))return "binary_native_members_registration_capacity";
    render+=next[i].render;update+=next[i].update;
  }
  for(std::size_t i=0;i<lists.size();++i){
    const auto &list=*lists[i];if(!ListCurrent(list)||needed[i]>list._capacity)return "binary_native_members_list_capacity_or_shape";
    for(unsigned n=0;n<list._size;++n)if(!Owned(prior,prior_count,list._data[n]))return "binary_native_members_unowned_list_entry";
  }
  if(!new_room)return nullptr;
  if(new_room->roster_count>new_room->roster.size()||new_room->descriptor_count>507||new_room->dimension>2||
     !new_room->descriptor||!new_room->width||!new_room->height||std::uint64_t(new_room->width)*new_room->height>448)return "binary_native_members_room_geometry";
  const auto &roster=g_Game->_playerManager._playerList;
  if(roster.size()!=(old_room?old_room->roster_count:0))return "binary_native_members_roster_size";
  for(std::size_t i=0;i<roster.size();++i)if(roster[i]!=old_room->roster[i])return "binary_native_members_roster_entry";
  for(unsigned i=0;i<448;++i)if(Slots(room)[i]!=(old_room?old_room->grids[i]:nullptr))return "binary_native_members_grid_slot";
  for(unsigned i=0;i<8;++i)if(room._doors[i]!=(old_room?old_room->doors[i]:nullptr))return "binary_native_members_door_slot";
  return nullptr;
}
bool ClientBinaryCaptureRoom(const ClientNativeRoots &roots,ClientBinaryRoomPrevious &out) noexcept {
  if(!Roots(roots))return false;
  const auto &game=*g_Game;const auto &room=*game._room;auto &m=out.members;
  m.descriptor=room._descriptor;m.width=room._gridWidth;m.height=room._gridHeight;m.dimension=game._currentDimensionIdx;
  m.entry=int(game._currentRoomIdx);m.descriptor_count=game._nbRooms;m.last_boss=int(game._lastBossRoomListIdx);m.greed_treasure=int(game._greedModeTreasureRoomIdx);
  std::copy_n(game._roomOffset,507,m.occupancy.begin());out.last_entry=game._lastRoomIdx;out.last_dimension=game._lastRoomDimensionIdx;
  m.type=room._roomType;
  out.sfx_volume=g_Manager->_sfxManager._volume;
  for(unsigned i=0;i<527;++i)if(i!=525){const auto &d=Descriptor(*g_Game,i);out.descriptors[i]={d.Data,d.OverrideData,d.ListIndex};}
  out.offsets=*reinterpret_cast<void *const *>(game.pad2);out.index=NextIndex(*game._room);return true;
}
bool ClientBinarySetRoom(const ClientNativeRoots &roots,const ClientBinaryRoomMembers &m) noexcept {
  if(!Window(roots)||!m.descriptor||m.dimension>2)return false;
  auto &game=*g_Game;auto &room=*game._room;
  auto *desc=static_cast<RoomDescriptor *>(m.descriptor);auto *config=desc->OverrideData?desc->OverrideData:desc->Data;
  if(!config||!desc->SpawnSeed)return false;
  if(game._currentRoomIdx!=unsigned(m.entry)||game._currentDimensionIdx!=m.dimension){
    game._lastRoomIdx=game._currentRoomIdx;game._lastRoomDimensionIdx=game._currentDimensionIdx;
  }
  game._currentRoomIdx=m.entry;game._currentDimensionIdx=m.dimension;game._nbRooms=m.descriptor_count;
  game._lastBossRoomListIdx=m.last_boss;game._greedModeTreasureRoomIdx=m.greed_treasure;
  std::copy(m.occupancy.begin(),m.occupancy.end(),game._roomOffset);
  *reinterpret_cast<unsigned **>(game.pad2)=game._roomOffset+m.dimension*169;
  for(unsigned i=0;i<527;++i)if(i!=525){
    auto &d=Descriptor(game,i);
    if(m.descriptors[i])d.ListIndex=i<507?int(i):-1;
    else {d.Data=nullptr;d.OverrideData=nullptr;}
  }
  room._descriptor=desc;room._roomType=m.type;room._gridWidth=m.width;room._gridHeight=m.height;
  return true;
}
bool ClientBinaryRoomGeometryCurrent(const ClientNativeRoots &roots,const ClientBinaryRoomMembers &m) noexcept {
  if(!Roots(roots)||!m.descriptor||m.dimension>2)return false;
  auto &game=*g_Game;auto &room=*game._room;
  const auto &desc=*static_cast<const RoomDescriptor *>(m.descriptor);
  if((!desc.OverrideData&&!desc.Data)||!desc.SpawnSeed||room._descriptor!=m.descriptor||room._roomType!=m.type||
      room._gridWidth!=m.width||room._gridHeight!=m.height||game._currentRoomIdx!=unsigned(m.entry)||
      game._currentDimensionIdx!=m.dimension||game._nbRooms!=m.descriptor_count||
      int(game._lastBossRoomListIdx)!=m.last_boss||int(game._greedModeTreasureRoomIdx)!=m.greed_treasure||
      *reinterpret_cast<unsigned **>(game.pad2)!=game._roomOffset+m.dimension*169||
      !std::equal(m.occupancy.begin(),m.occupancy.end(),game._roomOffset))return false;
  for(unsigned i=0;i<527;++i)if(i!=525){
    const auto &d=Descriptor(game,i);
    if(m.descriptors[i]?(!d.Data&&!d.OverrideData)||d.ListIndex!=(i<507?int(i):-1):d.Data||d.OverrideData)return false;
  }
  return true;
}
bool ClientBinaryMemberSetsCurrent(const ClientNativeRoots &roots,const Member *members,std::size_t count,
    const ClientBinaryRoomMembers &m,void *const *render_members,std::size_t render_count) noexcept {
  if(!Roots(roots))return false;
  const auto &roster=g_Game->_playerManager._playerList;
  auto &room=*g_Game->_room;
  if(roster.size()!=m.roster_count||!std::equal(roster.begin(),roster.end(),m.roster.begin())||
      !std::equal(m.grids.begin(),m.grids.end(),Slots(room))||!std::equal(m.doors.begin(),m.doors.end(),room._doors))return false;
  const auto lists=Lists(room);
  for(const auto *list:lists)if(!ListCurrent(*list))return false;
  const bool check_render_set=render_count!=0;
  if(check_render_set&&!RenderMembersCurrent(*lists[kRenderList],render_members,render_count))return false;
  std::array<std::size_t,7> cursor{};
  for(std::size_t n=0;n<count;++n){
    const auto &member=members[n];if(!Match(member))return false;
    const auto *entity=static_cast<const Entity *>(member.object);
    if(entity->_valid!=member.valid||(member.room&&entity->_index!=member.index))return false;
    const auto wanted=Membership(member);
    for(std::size_t i=0;i<lists.size();++i)if(wanted[i]&&(!check_render_set||i!=kRenderList)){
      if(cursor[i]>=lists[i]->_size||lists[i]->_data[cursor[i]++]!=entity)return false;
    }
  }
  for(std::size_t i=0;i<lists.size();++i)if((!check_render_set||i!=kRenderList)&&cursor[i]!=lists[i]->_size)return false;
  return true;
}
bool ClientBinarySetMembers(const ClientNativeRoots &roots,const Member *prior,std::size_t prior_count,
    Member *next,std::size_t count,const ClientBinaryRoomMembers &m) noexcept {
  if(!Window(roots)&&!Window(roots,true))return false;
  try {
    auto &game=*g_Game;auto &room=*game._room;
    for(std::size_t i=0;i<count;++i)if(!Match(next[i]))return false;
    if(!SetRoster(game,m))return false;
    RemoveMembers(room,prior,prior_count);
    for(std::size_t i=0;i<count;++i){
      auto &wanted=next[i];auto &e=*static_cast<Entity *>(wanted.object);
      if(wanted.room){
        const auto lists=Lists(room);
        const auto has_room=[&](unsigned n){return ListCurrent(*lists[n])&&lists[n]->_size<lists[n]->_capacity;};
        if(!has_room(Persistent(wanted)?1u:0u)||!has_room(2)||(!(e._flags&8)&&!has_room(3))||(Wisp(wanted)&&!has_room(4)))return false;
        if(wanted.index==UINT32_MAX){if(NextIndex(room)==UINT32_MAX)return false;wanted.index=NextIndex(room)++;}
        e._index=wanted.index;
        reinterpret_cast<void (__thiscall *)(EntityList *,Entity *)>(Base+0x13d00)(&room._entityList,&e);
      }
      auto &render=room._entityList._renderEL;auto &update=room._entityList._updateEL;
      if((wanted.render&&wanted.room&&!Count(render,&e)&&render._size==render._capacity)||
         (wanted.update&&wanted.room&&!Count(update,&e)&&update._size==update._capacity))return false;
      SetList(render,e,wanted.room&&wanted.render);
      SetList(update,e,wanted.room&&wanted.update);
      e._valid=wanted.valid;
    }
    for(unsigned i=0;i<448;++i)Slots(room)[i]=static_cast<GridEntity *>(m.grids[i]);
    for(unsigned i=0;i<8;++i)room._doors[i]=static_cast<GridEntity *>(m.doors[i]);
    return CheckSets(game,next,count,m);
  }catch(...){return false;}
}
bool ClientBinaryRestoreRoom(const ClientNativeRoots &roots,const ClientBinaryRoomPrevious &old) noexcept {
  if(!Window(roots,true))return false;
  auto &game=*g_Game;auto &room=*game._room;const auto &m=old.members;
  game._currentRoomIdx=m.entry;game._lastRoomIdx=old.last_entry;game._currentDimensionIdx=m.dimension;game._lastRoomDimensionIdx=old.last_dimension;
  game._nbRooms=m.descriptor_count;game._lastBossRoomListIdx=m.last_boss;game._greedModeTreasureRoomIdx=m.greed_treasure;
  g_Manager->_sfxManager._volume=old.sfx_volume;
  std::copy(m.occupancy.begin(),m.occupancy.end(),game._roomOffset);*reinterpret_cast<void **>(game.pad2)=old.offsets;
  room._descriptor=static_cast<RoomDescriptor *>(m.descriptor);room._roomType=m.type;room._gridWidth=m.width;room._gridHeight=m.height;
  for(unsigned i=0;i<527;++i)if(i!=525){
    auto &d=Descriptor(game,i);const auto &saved=old.descriptors[i];
    d.Data=static_cast<RoomConfig_Room *>(saved.data);d.OverrideData=static_cast<RoomConfig_Room *>(saved.override_data);d.ListIndex=saved.list;
  }
  NextIndex(room)=old.index;return true;
}
bool ClientBinaryDetachMember(const ClientNativeRoots &roots,void *object,bool grid,
    const std::array<std::uint32_t,3> *registered_shape) noexcept {
  if(!Window(roots,true)||!object)return false;
  try {
    auto &room=*g_Game->_room;
    if(grid){
      for(unsigned i=0;i<448;++i)if(Slots(room)[i]==object)Slots(room)[i]=nullptr;
      for(auto &door:room._doors)if(door==object)door=nullptr;
    }else{
      for(auto *list:Lists(room))if(!ListCurrent(*list))return false;
      auto &roster=g_Game->_playerManager._playerList;
      roster.erase(std::remove_if(roster.begin(),roster.end(),[object](const auto *p){return p==object;}),roster.end());
      const auto &e=*static_cast<const Entity *>(object);
      const auto shape=registered_shape?*registered_shape:std::array{e._type,e._variant,e._subtype};
      Member entry;entry.object=object;entry.type=shape[0];entry.variant=shape[1];entry.subtype=shape[2];
      RemoveMembers(room,&entry,1);
    }
    return true;
  }catch(...){return false;}
}
bool ClientBinaryDetachMember(const ClientNativeRoots &roots,void *object,bool grid) noexcept {
  return ClientBinaryDetachMember(roots,object,grid,nullptr);
}
bool ClientBinaryUnpublishWorld(const ClientNativeRoots &roots,const Member *members,std::size_t count,const ClientBinaryRoomMembers &m) noexcept {
  if(!Window(roots,true))return false;
  try {
    auto &room=*g_Game->_room;
    for(auto *list:Lists(room))if(!ListCurrent(*list))return false;
    if(g_Game->_playerManager._playerList.size()!=m.roster_count||
       !std::equal(g_Game->_playerManager._playerList.begin(),g_Game->_playerManager._playerList.end(),m.roster.begin()))return false;
    g_Game->_playerManager._playerList.clear();
    RemoveMembers(room,members,count);
    for(unsigned i=0;i<448;++i)if(Slots(room)[i]==m.grids[i])Slots(room)[i]=nullptr;
    for(unsigned i=0;i<8;++i)if(room._doors[i]==m.doors[i])room._doors[i]=nullptr;
    return true;
  }catch(...){return false;}
}
void *ClientBinaryLayer(void *object,unsigned element) noexcept {
  auto *sprite=static_cast<ANM2 *>(object);
  if(!sprite||!sprite->_layerState||element>=sprite->_layerCount)return nullptr;
  auto *layer=sprite->_layerState+element;
  return layer->_animation==sprite&&layer->_layerData?layer:nullptr;
}
// 对齐 J273 ANM2::LoadGraphics（RVA 0x7da0）的派生格式。普通 Layer 的 ColorMod
// 写入器每顶点写 88 字节，资源预载默认的 36 字节格式不能直接交给它绘制。
static bool ApplyLayerGraphics(void *object,bool repair,bool &written) noexcept {
  written=false;
  if(!IsMain()||!ClientBinaryInstallationScopeActive(false)||!ClientNativeCpuQueuesEmpty()||!object)return false;
  try {
    auto &layer=*static_cast<LayerState *>(object);
    if(!layer._animation)return false;
    auto *image=layer._spriteSheet.image;
    if(!image)return !layer._spriteSheetLoaded;
    if(!layer._spriteSheetLoaded)return false;
    const auto flags=layer._animation->_bitflags|layer._bitflags;
    const bool extended=(flags&0x10)!=0,plain=!extended&&(flags&0x200)!=0;
    const unsigned char attributes[]={5,6,7,4,3,2,1,3,4};
    const unsigned count=plain?3:extended?9:8,stride=plain?36:extended?104:88;
    const auto *descriptor=reinterpret_cast<const KAGE_Graphics_ImageBase_VertexAttributeDescriptor *>(
      Base+(plain?0x7a47b0:extended?0x788620:0x788760));
    const bool format=image->_predefinedShader==(plain?1u:3u)&&image->_vertexFormat&&
      image->_numVertexAttributes==count&&image->_vertexFormatStride==stride&&
      !std::memcmp(image->_vertexFormat,attributes,count);
    const auto compatible=[&](const auto &batches){
      for(const auto *batch:batches)if(!batch||batch->_vertexBuffer.elementSize!=stride)return false;
      return true;
    };
    const bool batches=compatible(image->_renderBatches)&&compatible(image->_reusableTransparentRenderBatches);
    if(format&&batches)return true;
    if(!repair)return false;
    // SetVertexFormat 不会淘汰旧批次。只在 CPU 提交队列排空的安装边界调用原生
    // slot 29（RVA 0x5e1cd0），同时释放普通／透明批次并清空 last 指针；保留图像登记不动。
    written=true;
    const auto *vtable=*reinterpret_cast<void *const *const *>(image);
    reinterpret_cast<void (__thiscall *)(KAGE_Graphics_ImageBase *)>(vtable[29])(image);
    image->SetVertexFormat(*descriptor);
    return image->_predefinedShader==(plain?1u:3u)&&image->_vertexFormat&&
      image->_numVertexAttributes==count&&image->_vertexFormatStride==stride&&
      !std::memcmp(image->_vertexFormat,attributes,count)&&image->_renderBatches.empty()&&
      image->_reusableTransparentRenderBatches.empty()&&!image->_lastRenderBatch&&!image->_lastTransparentRenderBatch;
  }catch(...){return false;}
}
void *ClientBinaryPlus(void *object,unsigned layout,ClientNativeReadWindow *reads) noexcept {
  if(!object)return nullptr;
  try {
    auto &e=*static_cast<Entity *>(object);
    const auto address=(std::uintptr_t(e._entityExtraDataPtrUpperBytes)<<16)|e._entityExtraDataPtrLowerBytes;
    player_sdk::PlusHeader *plus=nullptr;void *vptr=nullptr;
    const auto copy=[&](const void *source,void *output,std::size_t size){
      return reads?reads->Copy(source,output,size):ReadMemory(source,output,size);
    };
    // Entity padding编码的是官方unique_ptr holder的地址，真实Plus承载在holder内。
    if(!address||!copy(reinterpret_cast<void *>(address),&plus,sizeof(plus))||!plus||
       !copy(plus,&vptr,sizeof(vptr))||!vptr)return nullptr;
    if(layout==15)return plus;
    const char *name=nullptr;
    // 当前目标固定为MSVC官方SDK；直接比较RTTI原始类名，避免name()每次进入CRT反修饰锁。
    // 仍从当前holder逐次读取真实Plus和动态类型，不缓存对象、地址或跨生命周期资格。
    switch(layout){case 16:name=".?AVEntityFamiliarPlus@@";break;case 17:name=".?AVEntityKnifePlus@@";break;
      case 20:name=".?AVEntityTearPlus@@";break;case 21:name=".?AVEntityPickupPlus@@";break;case 22:name=".?AVEntityLaserPlus@@";break;
      case 173:case 174:name=".?AVEntityPlayerPlus@@";break;default:return nullptr;}
    if(std::strcmp(typeid(*plus).raw_name(),name))return nullptr;
    return Bytes(plus)+(layout==173?sizeof(player_sdk::PlusHeader):0);
  }catch(...){return nullptr;}
}
bool ClientBinaryGridClip(const ClientNativeRoots &roots,const ClientBinaryRoomMembers &members,const void *object,std::uint32_t &flags) noexcept {
  if(!Roots(roots)||!object)return false;
  const auto index=static_cast<const GridEntity *>(object)->_gridIndex;
  if(index>=members.grids.size()||members.grids[index]!=object||Slots(*g_Game->_room)[index]!=object)return false;
  flags=members.water_clip[index];return true;
}
static bool ApplySpecial(const ClientNativeRoots &roots,const ClientBinarySpecialWrite &w,ClientBinarySpecialUndo &u) noexcept {
  if(!Window(roots)||!w.parent||u.entered)return false;
  u.entered=true;
  auto *slot=w.size?Bytes(w.parent)+w.offset:Bytes(w.parent);
  try {
    using Kind=ClientBinarySpecial;
    switch(w.kind){
      case Kind::File:{
        auto &sprite=*static_cast<ANM2 *>(w.parent);auto *file=static_cast<ANM2 *>(w.resource);
        if(file&&sprite._loaded&&sprite._filename==file->_filename)return true;
        if(!file&&!sprite._loaded)return true;
        return ReplaceSprite(sprite,file,u)&&sprite._loaded==bool(file);
      }
      case Kind::Animation:return SetAnimation(*static_cast<AnimationState *>(w.parent),static_cast<AnimationData *>(w.resource),u,w.shared_player_sprite);
      case Kind::WaveAnimation:return SetAnimation(static_cast<ANM2 *>(w.parent)->_animState,static_cast<AnimationData *>(w.resource),u);
      case Kind::LayerCount:{
        auto &sprite=*static_cast<ANM2 *>(w.parent);
        return sprite._layerCount==w.count&&(!w.count||(ClientBinaryLayer(&sprite,0)&&ClientBinaryLayer(&sprite,w.count-1)));
      }
      case Kind::LayerFrames:case Kind::NullFrames:{
        auto &state=*static_cast<AnimationState *>(w.parent);const bool null=w.kind==Kind::NullFrames;
        auto *frames=null?state._nullLayerFrames:state._layerFrames;
        const auto count=state._animData?(null?state._animData->_nullLayerCount:state._animData->_layerCount):0;
        if(count<0||unsigned(count)!=w.count||w.length!=std::size_t(w.count)*sizeof(int))return false;
        if(!count)return true;
        if(!frames)return false;
        const std::byte *null_layers=nullptr;
        if(null)std::memcpy(&null_layers,state._animData->_nullLayersPtr,sizeof(null_layers));
        if(null?!null_layers:!state._animData->_animLayers)return false;
        for(int i=0;i<count;++i){
          int selector=0,limit=0;std::memcpy(&selector,w.bytes+i*4,4);
          if(null)std::memcpy(&limit,null_layers+i*16+12,4);else limit=state._animData->_animLayers[i]._numFrames;
          if(limit<0||selector< -1||(limit?selector>=limit:selector>0))return false;
        }
        // 选择器属于当前动画的原生数组，完成边界检查后直接覆盖变化值。
        if(!std::memcmp(frames,w.bytes,w.length))return true;
        u.applied=true;std::memcpy(frames,w.bytes,w.length);return true;
      }
      case Kind::LayerPath:case Kind::DefaultName:case Kind::Rubble:{
        auto &destination=*reinterpret_cast<std_string *>(slot);
        const char *data="";std::size_t length=0;
        if(w.kind==Kind::LayerPath&&w.resource){const auto *image=static_cast<KAGE_SmartPointer_ImageBase *>(w.resource)->image;if(!image||!image->_name)return false;
          data=image->_name;length=std::strlen(data);}
        if(w.kind==Kind::Rubble&&w.resource){const auto &name=static_cast<AnimationData *>(w.resource)->_name;data=name.data();length=name.size();
          auto &sprite=*reinterpret_cast<ANM2 *>(Bytes(w.parent)+0x40);
          if(!sprite.GetAnimationData(name))return false;}
        if(w.kind==Kind::DefaultName&&w.length){data=reinterpret_cast<const char *>(w.bytes);length=w.length;}
        if(destination.size()==length&&(!length||!std::memcmp(destination.data(),data,length)))return true;
        auto *previous=new std_string;u.storage=previous;u.destination=&destination;
        // 先构造新字符串，再交换所有权。旧串由同一原生STL类型析构。
        try {previous->assign(data,length);}catch(...){delete previous;u.storage=nullptr;return false;}
        destination.swap(*previous);u.applied=true;return true;
      }
      case Kind::Image:case Kind::LayerImage:{
        auto &image=*reinterpret_cast<KAGE_SmartPointer_ImageBase *>(slot);auto *source=static_cast<KAGE_SmartPointer_ImageBase *>(w.resource);
        if(source&&(!source->image||!source->counter))return false;
        auto *layer=w.kind==Kind::LayerImage?static_cast<LayerState *>(w.parent):nullptr;
        if(source?image.image==source->image&&image.counter==source->counter&&(!layer||layer->_spriteSheetLoaded):
           !image.image&&!image.counter&&(!layer||!layer->_spriteSheetLoaded))return true;
        auto *previous=new KAGE_SmartPointer_ImageBase(image);u.storage=previous;u.destination=&image;u.applied=true;
        if(layer)u.original[0]=std::byte(layer->_spriteSheetLoaded);
        u.uncertain=true;if(source)image=*source;else image=KAGE_SmartPointer_ImageBase{};u.uncertain=false;
        if(layer)layer->_spriteSheetLoaded=bool(source);
        return source?image.image==source->image&&image.counter==source->counter:!image.image&&!image.counter;
      }
      case Kind::Config:case Kind::DescriptorConfig:case Kind::BorrowedEntity:case Kind::BorrowedPlayer:{
        if(w.kind==Kind::BorrowedPlayer&&w.resource&&!static_cast<Entity *>(w.resource)->ToPlayer())return false;
        // 已定位且已定界的原生字段，相等时沿用既有承载。
        if(!std::memcmp(slot,&w.resource,sizeof(w.resource)))return true;
        if(!Save(u,slot,sizeof(void *)))return false;
        *reinterpret_cast<void **>(slot)=w.resource;return true;
      }
      case Kind::Text:{
        if(w.size!=256||w.length>=w.size)return false;
        u.applied=true;
        if(w.length)std::memcpy(slot,w.bytes,w.length);std::memset(slot+w.length,0,w.size-w.length);return true;
      }
      case Kind::Time:{
        const auto next=w.frame-Scalar<std::uint32_t>(w);
        if(!std::memcmp(slot,&next,sizeof(next)))return true;
        u.applied=true;
        *reinterpret_cast<std::uint32_t *>(slot)=next;return true;
      }
      case Kind::FXCount:{
        auto *count=Bytes(w.parent)+24;
        if(!std::memcmp(count,&w.count,sizeof(w.count)))return true;
        u.applied=true;
        *reinterpret_cast<unsigned *>(count)=w.count;return true;
      }
      case Kind::Widen:case Kind::DescriptorFlags:case Kind::DoorLight:case Kind::DoorAlpha:
      case Kind::FXCondition:case Kind::FXBackdrop:case Kind::Daily:case Kind::Ascent:{
        const auto bits=Scalar<std::uint8_t>(w);
        if(w.size!=4)return false;
        unsigned original=0;std::memcpy(&original,slot,sizeof(original));auto word=original;
        if(w.kind==Kind::DescriptorFlags)word=DescriptorDisplayFlags(word,bits);
        else if(w.kind==Kind::DoorLight)word=(word&~0x20u)|(bits?0x20u:0);
        else if(w.kind==Kind::DoorAlpha){const float value=bits?static_cast<Room *>(roots.room)->_wallColor._tint[3]:0.f;std::memcpy(&word,&value,sizeof(word));}
        else if(w.kind==Kind::FXCondition){const float value=bits?1.f:0.f;std::memcpy(&word,&value,sizeof(word));}
        else if(w.kind==Kind::FXBackdrop)word=bits?35u:0u;
        else if(w.kind==Kind::Daily)word=bits?(word?word:1u):0u;
        else if(w.kind==Kind::Ascent)word=(word&~0x10000u)|(bits?0x10000u:0);
        else word=bits;
        // 保留同一个原生字中的无关位，结果在活字段上一次更新。
        if(word==original)return true;
        u.applied=true;
        std::memcpy(slot,&word,sizeof(word));
        return true;
      }
      case Kind::VeinChildren:{
        if(w.length!=3||w.size!=12||!Save(u,slot,12))return false;
        if(!w.resource)return false;
        // Native Vein存同一Room内嵌节点的指针，线上只携带该数组的局部索引。
        for(unsigned i=0;i<3;++i){const auto value=std::to_integer<unsigned>(w.bytes[i]);if(value!=255&&value>=10)return false;}
        for(unsigned i=0;i<3;++i){
          const auto value=std::to_integer<unsigned>(w.bytes[i]);reinterpret_cast<void **>(slot)[i]=value==255?nullptr:Bytes(w.resource)+value*52;
        }
        return true;
      }
      default:return false;
    }
  }catch(...){return false;}
}
bool ClientBinaryApplySpecial(const ClientNativeRoots &roots,const ClientBinarySpecialWrite &w,ClientBinarySpecialUndo &u) noexcept {
  // 只捕捉当前具名写入器的异常。资源 swap 的不确定前缀继续由原生撤销记录保管。
  __try {return ApplySpecial(roots,w,u);}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void *ClientBinaryFlatLayer(void *root,unsigned index) noexcept {
  __try {return ClientBinaryLayer(root,index);}
  __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
void *ClientBinaryFlatPlus(void *root,unsigned layout) noexcept {
  __try {return ClientBinaryPlus(root,layout);}
  __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
// 这些槽是借用指针，EntityPtr 则由独立的原生反向树维护。
bool ClientBinaryFlatBorrow(void *slot,void *target,bool player,bool check) noexcept {
  __try {
    const bool active=ClientBinaryInstallationScopeActive(false)||
      (!target&&!check&&ClientBinaryInstallationScopeActive(true));
    if(!active||!slot||
       (target&&player&&!static_cast<Entity *>(target)->ToPlayer()))return false;
    auto &value=*static_cast<void **>(slot);
    if(check)return value==target;
    value=target;
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientBinaryClearFlatReference(void *slot) noexcept {
  if(!ClientBinaryInstallationScopeActive(true)||!slot)return false;
  __try {
    static_cast<EntityPtr *>(slot)->SetReference(nullptr);
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static bool RetireSpecial(const ClientBinarySpecialWrite &,ClientBinarySpecialUndo &) noexcept;
bool ClientBinaryCommitFlatSpecial(const ClientBinarySpecialWrite &write,ClientBinarySpecialUndo &undo) noexcept {
  if(!ClientBinaryInstallationScopeActive(false)&&!ClientBinaryInstallationScopeActive(true))return false;
  // Apply 已把完成构造的旧承载换出时，后续活对象 setter 的失败不妨碍释放该旧承载。
  if(undo.committed&&undo.uncertain)return false;
  if(undo.applied)undo.uncertain=false;
  undo.committed=true;
  __try {return RetireSpecial(write,undo);}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientBinaryLayerGraphics(void *object,bool repair,bool &written) noexcept {
  __try {return ApplyLayerGraphics(object,repair,written);}
  __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static unsigned ReadSpecial(const ClientNativeRoots &roots,const ClientBinarySpecialWrite &w) noexcept {
  if(!Window(roots)||!w.parent)return 3;
  const auto compare=[&](void *slot,const void *expected,std::size_t size){
    if(!size)return 1u;
    if(!slot||!expected)return 3u;
    if(!std::memcmp(slot,expected,size))return 1u;
    return 2u;
  };
  const auto check=[](bool same){return same?1u:3u;};
  auto *slot=Bytes(w.parent)+w.offset;
  using Kind=ClientBinarySpecial;
  try{
    switch(w.kind){
      case Kind::File:{
        const auto &sprite=*static_cast<const ANM2 *>(w.parent);const auto *file=static_cast<const ANM2 *>(w.resource);
        return check(file?sprite._loaded&&sprite._filename==file->_filename:!sprite._loaded);
      }
      case Kind::Animation:case Kind::WaveAnimation:{
        const auto &state=w.kind==Kind::Animation?*static_cast<const AnimationState *>(w.parent):static_cast<const ANM2 *>(w.parent)->_animState;
        const auto *asset=static_cast<const AnimationData *>(w.resource);
        return check(asset?state._animation&&state._animData&&state._animData->_name==asset->_name:!state._animData);
      }
      case Kind::LayerCount:return check(static_cast<const ANM2 *>(w.parent)->_layerCount==w.count);
      case Kind::LayerFrames:case Kind::NullFrames:{
        auto &state=*static_cast<AnimationState *>(w.parent);const bool null=w.kind==Kind::NullFrames;
        const auto *a=state._animData;
        const auto count=a?(null?a->_nullLayerCount:a->_layerCount):0;
        if(count<0||unsigned(count)!=w.count||w.length!=std::size_t(count)*4)return 3;
        return compare(null?state._nullLayerFrames:state._layerFrames,w.bytes,w.length);
      }
      case Kind::Image:case Kind::LayerImage:{
        const auto &image=*reinterpret_cast<const KAGE_SmartPointer_ImageBase *>(slot);
        const auto *asset=static_cast<const KAGE_SmartPointer_ImageBase *>(w.resource);
        const bool loaded=w.kind==Kind::LayerImage&&static_cast<const LayerState *>(w.parent)->_spriteSheetLoaded;
        if(!(asset?SameAtlas(image,*asset):!image.image&&!image.counter)||
          (w.kind==Kind::LayerImage&&loaded!=bool(asset)))return 3;
        bool changed=false;
        return w.kind!=Kind::LayerImage||ClientBinaryLayerGraphics(w.parent,false,changed)?1u:2u;
      }
      case Kind::LayerPath:case Kind::DefaultName:case Kind::Rubble:{
        const auto &text=*reinterpret_cast<const std::string *>(slot);std::string_view expected;
        if(w.kind==Kind::DefaultName&&w.length)expected={reinterpret_cast<const char *>(w.bytes),w.length};
        if(w.kind==Kind::Rubble&&w.resource)expected=static_cast<const AnimationData *>(w.resource)->_name;
        if(w.kind==Kind::LayerPath&&w.resource){
          const auto *image=static_cast<const KAGE_SmartPointer_ImageBase *>(w.resource)->image;
          if(!image||!image->_name)return 3;expected=image->_name;
        }
        return check(text==expected);
      }
      case Kind::Config:case Kind::DescriptorConfig:case Kind::BorrowedEntity:case Kind::BorrowedPlayer:
        return check(*reinterpret_cast<void *const *>(slot)==w.resource);
      case Kind::Text:{
        std::array<std::byte,256> text{};
        if(w.size!=text.size()||w.length>=text.size())return 3;
        if(w.length)std::memcpy(text.data(),w.bytes,w.length);
        return compare(slot,text.data(),text.size());
      }
      case Kind::Time:{
        const auto value=w.frame-Scalar<std::uint32_t>(w);return compare(slot,&value,4);
      }
      case Kind::FXCount:return compare(Bytes(w.parent)+24,&w.count,4);
      case Kind::Widen:case Kind::DescriptorFlags:case Kind::DoorLight:case Kind::DoorAlpha:
      case Kind::FXCondition:case Kind::FXBackdrop:case Kind::Daily:case Kind::Ascent:{
        const auto bits=Scalar<std::uint8_t>(w);unsigned old=0;std::memcpy(&old,slot,4);
        unsigned value=bits;
        if(w.kind==Kind::DescriptorFlags)value=DescriptorDisplayFlags(old,bits);
        else if(w.kind==Kind::DoorLight)value=(old&~0x20u)|(bits?0x20u:0u);
        else if(w.kind==Kind::DoorAlpha){const float alpha=bits?static_cast<Room *>(roots.room)->_wallColor._tint[3]:0.f;std::memcpy(&value,&alpha,4);}
        else if(w.kind==Kind::FXCondition)value=bits?0x3f800000u:0u;
        else if(w.kind==Kind::FXBackdrop)value=bits?35u:0u;
        else if(w.kind==Kind::Daily)value=bits?(old?old:1u):0u;
        else if(w.kind==Kind::Ascent)value=(old&~0x10000u)|(bits?0x10000u:0u);
        return compare(slot,&value,4);
      }
      case Kind::VeinChildren:{
        if(!w.resource||w.length!=3||w.size!=12)return 3;
        std::array<void *,3> expected{};
        for(unsigned n=0;n<expected.size();++n){
          const auto v=std::to_integer<unsigned>(w.bytes[n]);
          if(v==255)continue;
          if(v>=10)return 3;
          expected[n]=Bytes(w.resource)+v*52;
        }
        return compare(slot,expected.data(),sizeof(expected));
      }
      default:return 0;
    }
  }catch(...){return 3;}
}
unsigned ClientBinaryReadSpecial(const ClientNativeRoots &roots,const ClientBinarySpecialWrite &w) noexcept {
  __try {return ReadSpecial(roots,w);}
  __except(EXCEPTION_EXECUTE_HANDLER){return 3;}
}
static bool RetireSpecial(const ClientBinarySpecialWrite &w,ClientBinarySpecialUndo &u) noexcept {
  if(u.uncertain)return false;
  if(!u.applied)return true;
  u.uncertain=true;
  try {
    using Kind=ClientBinarySpecial;
    switch(w.kind){
      case Kind::File:case Kind::Animation:case Kind::WaveAnimation:{
        auto *old=static_cast<ANM2 *>(u.storage);
        if(!u.committed){
          if(w.shared_player_sprite&&w.kind==Kind::Animation){
            auto &live=*static_cast<AnimationState *>(u.destination);
            const auto *sprite=live._animation;
            if(!sprite||(&live!=&sprite->_animState&&&live!=&sprite->_overlayAnimState))return false;
            auto &held=(&live==&sprite->_animState)?old->_animState:old->_overlayAnimState;
            std::array<AnimationData *,2> original{};
            std::memcpy(original.data(),u.original.data(),sizeof(original));
            SwapAnimationPose(live,held);live._animData=original[0];held._animData=original[1];
          }else SwapSprite(*static_cast<ANM2 *>(u.destination),*old);
        }
        old->destructor();Free(old);break;
      }
      case Kind::LayerPath:case Kind::DefaultName:case Kind::Rubble:{
        auto *old=static_cast<std_string *>(u.storage);
        if(!u.committed)static_cast<std_string *>(u.destination)->swap(*old);
        delete old;break;
      }
      case Kind::Image:case Kind::LayerImage:{
        auto *old=static_cast<KAGE_SmartPointer_ImageBase *>(u.storage);
        if(!u.committed){
          *static_cast<KAGE_SmartPointer_ImageBase *>(u.destination)=*old;
          if(w.kind==Kind::LayerImage)static_cast<LayerState *>(w.parent)->_spriteSheetLoaded=u.original[0]!=std::byte{};
        }
        delete old;break;
      }
      default:if(!u.committed&&u.original_size)std::memcpy(u.destination,u.original.data(),u.original_size);break;
    }
    u.storage=nullptr;u.applied=false;u.uncertain=false;return true;
  }catch(...){return false;}
}
bool ClientBinaryRetireSpecial(const ClientBinarySpecialWrite &w,ClientBinarySpecialUndo &u) noexcept {
  return ClientBinaryInstallationScopeActive(true)&&RetireSpecial(w,u);
}
std::size_t ClientBinarySpecialCharge(const ClientBinarySpecialWrite &w) noexcept {
  using Kind=ClientBinarySpecial;
  switch(w.kind){
    case Kind::File:case Kind::Animation:case Kind::WaveAnimation:return sizeof(ANM2);
    case Kind::Image:case Kind::LayerImage:return sizeof(KAGE_SmartPointer_ImageBase);
    case Kind::DefaultName:case Kind::Rubble:case Kind::LayerPath:return sizeof(std_string)+w.length;
    default:return 0;
  }
}
} // namespace cs::game
