// 预测插值阶段 1 的原生访问：只读遍历 renderEL，并在覆盖作用域内交换 pos。
// 本单元按 C++17 编译（SDK 头限制），逻辑与时间基准在 client_motion_overlay.cpp。
#include "IsaacRepentance.h"
#include "game/client/presentation/client_motion_overlay.h"
#include "runtime.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace cs::game {
namespace {
// 与安装器使用的上限一致：形状异常的列表一律不遍历。
constexpr unsigned kListLimit=16384;
constexpr unsigned kLocalLimit=8;

// 只接受 g_Game 当前房间；列表形状必须是原生 vector 的正常状态。
EntityList_EL *RenderList(void *room_pointer) noexcept {
  if(!IsMain()||!g_Game||!room_pointer||g_Game->_room!=room_pointer)return nullptr;
  auto &list=static_cast<Room *>(room_pointer)->_entityList._renderEL;
  if(list._sublist||list._size>list._capacity||list._capacity>kListLimit||(list._capacity&&!list._data))return nullptr;
  return &list;
}
ClientMotionIdentity IdentityOf(const Entity &e) noexcept {
  return {e._type,e._variant,e._subtype,e._initSeed};
}
bool Contains(void *const *values,unsigned count,const void *p) noexcept {
  if(!p)return false;
  for(unsigned i=0;i<count;++i)if(values[i]==p)return true;
  return false;
}
// 先看提示下标，再线性查找；只返回仍在列表中的成员，避免解引用已释放地址。
Entity *Locate(EntityList_EL &list,void *address,unsigned slot) noexcept {
  if(slot<list._size&&list._data[slot]==address)return list._data[slot];
  for(unsigned i=0;i<list._size;++i)if(list._data[i]==address)return list._data[i];
  return nullptr;
}
// 原生对象的按偏移纯读（偏移来自 J273 逆向；SDK 未命名或为避开 SDK 数组界限的字段用它）。
template<class T>T Raw(const void *base,std::size_t offset) noexcept {
  T value{};std::memcpy(&value,static_cast<const char *>(base)+offset,sizeof(T));return value;
}
// 引用字段（EntityPtr 首字为对象指针）指向的对象只在仍位于 renderEL 时才解引用。
Entity *Referenced(EntityList_EL &list,const void *owner,std::size_t offset) noexcept {
  void *p=Raw<void *>(owner,offset);
  return p?Locate(list,p,UINT32_MAX):nullptr;
}
} // namespace

unsigned ClientMotionCollect(void *room,void *const *locals,unsigned local_count,
                             ClientMotionEntity *out,unsigned capacity,bool &overflow) noexcept {
  overflow=false;
  auto *list=RenderList(room);
  if(!list||!out)return 0;
  local_count=(std::min)(local_count,kLocalLimit);
  // 第一遍：确认本机身体确实在列表中，再读它举起的实体（Player+0x140C）。
  std::array<void *,kLocalLimit> live_locals{},held{},held_owner{};
  unsigned live_count=0,held_count=0;
  for(unsigned i=0;i<list->_size;++i){
    auto *e=list->_data[i];
    if(!e||!Contains(locals,local_count,e)||live_count>=kLocalLimit)continue;
    live_locals[live_count++]=e;
    if(e->_type==1){
      auto *h=static_cast<Entity_Player *>(e)->_heldEntity;
      if(h&&held_count<kLocalLimit){held_owner[held_count]=e;held[held_count++]=h;}
    }
  }
  // 第二遍：快照位置与速度，并标出不参与远端插值的对象。
  unsigned n=0;
  for(unsigned i=0;i<list->_size;++i){
    auto *e=list->_data[i];
    if(!e)continue;
    if(n>=capacity){overflow=true;break;}
    auto &v=out[n++];
    v.address=e;v.identity=IdentityOf(*e);v.slot=i;
    v.x=e->_pos.x;v.y=e->_pos.y;v.vx=e->_velocity.x;v.vy=e->_velocity.y;
    v.local=Contains(live_locals.data(),live_count,e);
    v.laser=e->_type==7;
    // 附着判定（RE-P5）：Parent 指向本机身体、跟班所属玩家是本机身体、被本机身体举起。
    // 只比较地址，不经由这些引用读取其他对象。阶段 2 另记所附着的身体与是否平移 Δ。
    void *parent=e->_parent._ref;
    v.owner=nullptr;v.follow=false;
    if(Contains(live_locals.data(),live_count,parent)){v.owner=parent;v.follow=!v.laser;}
    if(!v.owner)for(unsigned h=0;h<held_count;++h)if(held[h]==e){v.owner=held_owner[h];v.follow=true;break;}
    if(!v.owner&&e->_type==3){
      void *player=static_cast<Entity_Familiar *>(e)->_player;
      if(Contains(live_locals.data(),live_count,player)){
        // 环绕跟班 OrbitLayer（+0xD70）≥0 才随身体平移；跟随队列跟班沿路径追随，首版不平移（RE-P5 第 1 节）。
        v.owner=player;v.follow=Raw<std::int32_t>(e,0xD70)>=0;
      }
    }
    if(v.local){v.owner=nullptr;v.follow=false;}
    v.attached=v.owner!=nullptr;
  }
  return n;
}

void ClientMotionSwap(void *room,ClientMotionWrite *writes,unsigned count,bool apply,
                      unsigned &missing,unsigned &changed) noexcept {
  missing=changed=0;
  auto *list=RenderList(room);
  if(!writes)return;
  for(unsigned i=0;i<count;++i){
    auto &w=writes[i];
    if(!apply&&!w.applied)continue;
    Entity *e=list?Locate(*list,w.address,w.slot):nullptr;
    if(!e||IdentityOf(*e)!=w.identity){
      // 进入时找不到：不写；退出时找不到：对象已离开渲染列表，不能再触碰该地址。
      ++missing;w.applied=false;continue;
    }
    if(apply){
      w.saved_x=e->_pos.x;w.saved_y=e->_pos.y;
      e->_pos.x=w.display_x;e->_pos.y=w.display_y;
      if(w.velocity){w.saved_vx=e->_velocity.x;w.saved_vy=e->_velocity.y;e->_velocity.x=w.display_vx;e->_velocity.y=w.display_vy;}
      w.applied=true;
    }else{
      // 作用域内有别的代码改了 pos：计数，但权威值仍须写回。
      if(std::memcmp(&e->_pos.x,&w.display_x,sizeof(float))||std::memcmp(&e->_pos.y,&w.display_y,sizeof(float)))++changed;
      e->_pos.x=w.saved_x;e->_pos.y=w.saved_y;
      if(w.velocity){e->_velocity.x=w.saved_vx;e->_velocity.y=w.saved_vy;}
      w.applied=false;
    }
  }
}
// 阶段 2：读本机身体的移动参数与资格事实（偏移见 client_motion_predict.h 各字段注释）。
bool ClientMotionReadBody(void *room,void *address,ClientMotionBody &out) noexcept {
  out={};
  auto *list=RenderList(room);
  if(!list||!address)return false;
  Entity *e=Locate(*list,address,UINT32_MAX);
  if(!e||e->_type!=1)return false;
  auto *p=static_cast<Entity_Player *>(e);
  out.address=address;
  out.x=e->_pos.x;out.y=e->_pos.y;out.vx=e->_velocity.x;out.vy=e->_velocity.y;
  out.move_speed=Raw<float>(e,0x155C);out.friction=Raw<float>(e,0x36C);out.time_scale=Raw<float>(e,0x39C);
  out.size=Raw<float>(e,0x370);out.size_mx=Raw<float>(e,0x374);out.size_my=Raw<float>(e,0x378);
  out.flags=Raw<std::uint64_t>(e,0x168);out.grid_class=Raw<std::int32_t>(e,0x184);
  out.variant=Raw<std::int32_t>(e,0x2C);out.player_type=Raw<std::int32_t>(e,0x13BC);
  // 碰撞采样点：std::vector<Vector>，begin/end 在 +0x178/+0x17C。
  const auto *begin=Raw<const float *>(e,0x178);
  const auto *end=Raw<const float *>(e,0x17C);
  if(begin&&end>=begin){
    const auto count=std::size_t(end-begin)/2;
    if(count>kClientMotionGridPoints)out.too_many_points=true;
    else{
      out.point_count=unsigned(count);
      for(std::size_t i=0;i<count;++i){out.point_x[i]=begin[i*2];out.point_y[i]=begin[i*2+1];}
    }
  }
  out.controls=Raw<std::uint8_t>(e,0x410)!=0;out.cooldown=Raw<std::int32_t>(e,0x414);
  out.can_fly=Raw<std::uint8_t>(e,0x1564)!=0;out.coop_ghost=Raw<std::uint8_t>(e,0x207D)!=0;
  out.input_lock=Raw<std::uint8_t>(e,0x1FE0)!=0;
  out.airborne=Raw<float>(e,0x1E54)<0.f||Raw<float>(e,0x350)<0.f;
  out.has_target=Raw<void *>(e,0x3C4)!=nullptr;out.in_minecart=Raw<void *>(e,0x3D4)!=nullptr;
  out.has_parent=Raw<void *>(e,0x3BC)!=nullptr;
  // 父体不在渲染列表时无法核对类型，按不可预测处理。
  if(Entity *parent=Referenced(*list,e,0x3BC))out.parent_player=parent->_type==1;
  else out.parent_player=out.has_parent;
  out.suplex=Raw<std::int32_t>(e,0x1F08);out.pony=Raw<std::int32_t>(e,0x1874);
  auto &effects=p->_temporaryeffects;
  // 控制块整体跳过（RE-P3）：冻结倒计时 +0x218 或 PAUSE（478）临时效果。
  out.frozen=Raw<std::int32_t>(e,0x218)>0||effects.HasCollectibleEffect(478);
  out.chariot=effects.HasNullEffect(63);out.spin=effects.HasNullEffect(50);
  out.mars=effects.HasCollectibleEffect(593);out.mega_mush=effects.HasCollectibleEffect(625);
  // GetMovementInput 的全局变换（0x771D70）：Game+0x25B8C≥2 时混乱取反；种子效果 12／76；镜像世界。
  // 客机的种子效果不由同步安装（显示委托只解码摘要），读到的是本地值；不一致时由对账纠正。
  out.confused=Raw<std::int32_t>(g_Game,0x25B8C)>=2&&(out.flags&(1ull<<9));
  out.reversed=g_Game->_seedEffects.HasSeedEffect(12);
  out.axis_aligned=g_Game->_seedEffects.HasSeedEffect(76);
  const auto stage=Raw<std::uint32_t>(g_Game,0x0),stage_type=Raw<std::uint32_t>(g_Game,0x4);
  // 原生用 0x742450（当前诅咒）判 bit1；这里读 Game+0xC 的诅咒位，未核是否包含临时诅咒。
  out.mirrored=Raw<std::int32_t>(g_Game,0x1830C)==1&&(stage_type==4||stage_type==5)&&
               (stage==2||(stage==1&&(Raw<std::uint32_t>(g_Game,0xC)&2u)));
  return true;
}

// 阶段 2：当前房间的地形快照。只读格子实体表（Room+0x24，按原生下标读 448 项）与边界，不读寻路表 Room+0x76C。
bool ClientMotionReadTerrain(void *room,ClientMotionTerrain &out) noexcept {
  out.valid=false;
  if(!RenderList(room))return false;
  out.room_type=Raw<std::int32_t>(room,0x8);out.width=Raw<std::int32_t>(room,0xC);out.height=Raw<std::int32_t>(room,0x10);
  if(out.width<=0||out.height<=0||out.width*out.height>int(kClientMotionGridCells))return false;
  out.tl_x=Raw<float>(room,0x14);out.tl_y=Raw<float>(room,0x18);out.br_x=Raw<float>(room,0x1C);out.br_y=Raw<float>(room,0x20);
  out.shape=0;out.special_zero=false;
  // RoomDescriptor（Room+0x4）→ RoomConfig_Room（+0x10）：StageId +0、Type +8、Shape +72。
  if(void *descriptor=Raw<void *>(room,0x4))if(void *config=Raw<void *>(descriptor,0x10)){
    out.shape=Raw<std::int32_t>(config,72);
    out.special_zero=Raw<std::int32_t>(config,0)==35&&Raw<std::int32_t>(config,8)==16;
  }
  for(unsigned i=0;i<kClientMotionGridCells;++i){
    void *g=Raw<void *>(room,0x24+4*std::size_t(i));
    if(!g){out.entity_class[i]=-1;out.entity_kind[i]=0;out.entity_index[i]=-1;continue;}
    // GridEntity：类型 +0x4、变体 +0x8、自身格号 +0x24、碰撞类别 +0x3C。
    const auto type=Raw<std::int32_t>(g,0x4),variant=Raw<std::int32_t>(g,0x8),cls=Raw<std::int32_t>(g,0x3C);
    out.entity_class[i]=std::int8_t(cls<0||cls>127?0:cls);
    out.entity_kind[i]=std::uint8_t(type==14?(unsigned(variant-7)>3u?1:2):type==16?3:0);
    out.entity_index[i]=std::int16_t(Raw<std::int32_t>(g,0x24));
  }
  out.valid=true;
  return true;
}
} // namespace cs::game

