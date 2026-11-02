#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_room_native.h"
#include "runtime.h"
#include "replica/room_geometry.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <stdexcept>

namespace cs::game {
namespace {
void Integer(lua_State *L,const char *key,std::int64_t value){lua_pushinteger(L,lua_Integer(value));lua_setfield(L,-2,key);}
bool Real(lua_State *L,const char *key,float value){if(!std::isfinite(value))return false;lua_pushnumber(L,value);lua_setfield(L,-2,key);return true;}
// SDK明确的配置布局包含私有Flags/MergedData；递归只复制已存在内容，不加载或挑选候选。
// 限制环、深度和总成员；任何失败丢弃整份输出，不发布截断配置。
bool Configuration(lua_State *L,const RoomConfig_Room *c,std::array<const RoomConfig_Room *,9> &stack,unsigned depth,unsigned &nodes){
  if(!c){lua_pushnil(L);return true;}
  if(depth>=stack.size()||std::find(stack.begin(),stack.begin()+depth,c)!=stack.begin()+depth||++nodes>262144||c->Name.size()>4096)return false;
  stack[depth]=c;lua_createtable(L,0,20);
  Integer(L,"version",1);Integer(L,"stage",c->StageId);Integer(L,"mode",c->Mode);Integer(L,"type",c->Type);Integer(L,"variant",c->Variant);
  Integer(L,"subtype",c->Subtype);Integer(L,"shape",c->Shape);Integer(L,"width",c->Width);Integer(L,"height",c->Height);
  Integer(L,"difficulty",c->Difficulty);Integer(L,"original_variant",c->originalVariant);Integer(L,"doors",c->Doors);
  static_assert(sizeof(c->QuestDoors)==4&&sizeof(c->Flags)==4);std::uint32_t quest=0,flags=0;
  std::memcpy(&quest,&c->QuestDoors,4);std::memcpy(&flags,&c->Flags,4);Integer(L,"quest_doors",quest);Integer(L,"flags",flags);
  lua_pushlstring(L,c->Name.data(),c->Name.size());lua_setfield(L,-2,"name");
  if(!Real(L,"initial_weight",c->InitialWeight)||!Real(L,"weight",c->Weight)||(c->SpawnCount&&!c->Spawns))return false;
  lua_createtable(L,c->SpawnCount,0);
  for(unsigned i=0;i<c->SpawnCount;++i){const auto &s=c->Spawns[i];if(++nodes>262144||(s.CountEntries&&!s.Entries))return false;
    lua_createtable(L,0,4);Integer(L,"x",s.X);Integer(L,"y",s.Y);if(!Real(L,"sum_weights",s.SumWeights))return false;
    lua_createtable(L,s.CountEntries,0);
    for(unsigned j=0;j<s.CountEntries;++j){const auto &e=s.Entries[j];if(++nodes>262144)return false;lua_createtable(L,0,4);
      Integer(L,"type",e.type);Integer(L,"variant",e.variant);Integer(L,"subtype",e.subtype);if(!Real(L,"weight",e.weight))return false;lua_rawseti(L,-2,j+1);}
    lua_setfield(L,-2,"entries");lua_rawseti(L,-2,i+1);
  }
  lua_setfield(L,-2,"spawns");lua_createtable(L,2,0);
  // nil不能保留Lua数组槽，显式Null标签在Lua层归一，编码后仍是两个确定成员。
  for(unsigned i=0;i<2;++i){if(!c->MergedData[i]){lua_createtable(L,0,1);lua_pushstring(L,"Null");lua_setfield(L,-2,"status");}
    else if(!Configuration(L,c->MergedData[i],stack,depth+1,nodes))return false;lua_rawseti(L,-2,i+1);}
  lua_setfield(L,-2,"merged");return true;
}
}
int ClientRoomConfigCaptureLua(void *state){
  auto *L=static_cast<lua_State *>(state);const int top=lua_gettop(L);
  if(!IsMain()){lua_pushnil(L);return 1;}
  auto *config=lua::GetLuabridgeUserdata<RoomConfig_Room *>(L,1,lua::Metatables::ROOM_CONFIG_ROOM,lua::Metatables::CONST_ROOM_CONFIG_ROOM,"RoomConfigRoom");
  std::array<const RoomConfig_Room *,9> stack{};unsigned nodes=0;
  if(!Configuration(L,config,stack,0,nodes)){lua_settop(L,top);lua_pushnil(L);}return 1;
}
namespace {
template <class T> T Read(const void *object,unsigned offset){T value{};std::memcpy(&value,static_cast<const unsigned char *>(object)+offset,sizeof(T));return value;}
template <class T> void Write(void *object,unsigned offset,const T &value){std::memcpy(static_cast<unsigned char *>(object)+offset,&value,sizeof(T));}
// 405F50(reset=true)->403DE0会free并重建main索引数组，不能在draw后恢复旧的已释放指针。
// 原数组暂由此栈持有且不变；native draw只拥有临时数组，离栈按同一j_j_free释放后归还原state。
class RailDrawBorrow final {
public:
  RailDrawBorrow(ANM2 &sprite,bool water):state_(sprite._animState),previous_(state_) {
    if((sprite._loaded||state_._animData)&&(!sprite.GetAnimationByName("rail")||(water&&!sprite.GetAnimationByName("pitrail"))))
      throw std::runtime_error("native_rail_draw_animation_missing");
    state_._layerFrames=nullptr;state_._nullLayerFrames=nullptr;
  }
  ~RailDrawBorrow(){
    const auto release=reinterpret_cast<void (__cdecl *)(void *)>(Base+0x686bc9);
    if(state_._layerFrames)release(state_._layerFrames);if(state_._nullLayerFrames)release(state_._nullLayerFrames);
    state_=previous_;
  }
  RailDrawBorrow(const RailDrawBorrow &)=delete;
  RailDrawBorrow &operator=(const RailDrawBorrow &)=delete;
private:AnimationState &state_;AnimationState previous_;
};
template <class T,std::size_t N> void Array(lua_State *L,const char *name,const T (&values)[N]){
  lua_createtable(L,int(N),0);for(unsigned i=0;i<N;++i){lua_pushnumber(L,values[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,name);
}
}
// J273 RailManager为room+7468，轨道448个字节覆盖+746c..762b，Sprite从+762c开始。
// 官方Lua Getter把index截成uint8；这里按真实数组复制，保留256..447而不读Sprite。
int ClientRoomStateCaptureLua(void *state){
  auto *L=static_cast<lua_State *>(state);const int top=lua_gettop(L);
  if(!IsMain()||!g_Game||!g_Game->_room||!g_Game->_room->_descriptor||!g_Game->_room->_descriptor->Data){lua_pushnil(L);return 1;}
  auto &g=*g_Game;auto &r=*g._room;auto &d=*r._descriptor;auto *config=d.Data;
  if(config->Shape<1||config->Shape>12||config->Type<1||config->Type>29){lua_pushnil(L);return 1;}
  const auto extent=r._gridWidth*r._gridHeight;if(!r._gridWidth||!r._gridHeight||r._gridWidth>28||r._gridHeight>16||extent>448){lua_pushnil(L);return 1;}
  lua_createtable(L,0,32);Integer(L,"version",3);
  lua_createtable(L,int(extent),0);for(unsigned i=0;i<extent;++i){lua_pushinteger(L,r._gridPaths[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"grid_paths");
  lua_pushstring(L,"active_extent;outside_not_applicable");lua_setfield(L,-2,"path_scope");
  const int light_queries[]={int(g._seedEffects.HasSeedEffect(79)),int(g.GetStageID(false)),
    int(reinterpret_cast<bool (__thiscall *)(Game *)>(Base+0x348900)(&g)),
    int(reinterpret_cast<bool (__cdecl *)()>(Base+0x415b80)()),
    reinterpret_cast<int (__thiscall *)(Game *)>(Base+0x345b10)(&g)};
  Array(L,"grid_light_queries",light_queries);
  lua_pushboolean(L,Read<bool>(&r,0));lua_setfield(L,-2,"initialized");
  lua_pushboolean(L,r._redHeartDamage);lua_setfield(L,-2,"red_heart_damage");lua_pushboolean(L,r._pacifist);lua_setfield(L,-2,"pacifist");
  Integer(L,"frame",r.GetFrameCount());Integer(L,"game_frame",g._frameCount);Integer(L,"clear_delay",r._roomClearDelay);Integer(L,"greed_timer",r._greedWaveTimer);Integer(L,"greed_wave",g._greedModeWave);
  Integer(L,"render_flag_7220",Read<std::uint8_t>(&r,0x7220));
  lua_pushboolean(L,g._ambush.active);lua_setfield(L,-2,"ambush_active");Integer(L,"ambush_wave",g._ambush.currentWave);
  Integer(L,"ambush_max",g._ambush.maxBossWaves);Integer(L,"ambush_bosses",g._ambush.bossesPerWave);
  Integer(L,"total_money",d.TotalMoneySpent);Integer(L,"shop_item",d.ShopItemIdx);Integer(L,"shop_discount",d.ShopItemDiscountIdx);
  Integer(L,"greed_subtype",d.GreedSubType);Integer(L,"poop_count",d.PoopCount);Integer(L,"keeper_coins",d.TKeeperCoinSpawns);Integer(L,"pits",d.PitsCount);Integer(L,"boss",r._bossId);
  float bounds[4]{};std::memcpy(bounds,reinterpret_cast<unsigned char *>(&r)+0x14,sizeof(bounds));Array(L,"bounds",bounds);
  LRoomAreaDesc area;LRoomTileDesc tiles;r.GetLRoomAreaDesc(&area);r.GetLRoomTileDesc(&tiles);
  float areas[8]{};int coordinates[8]{};std::memcpy(areas,&area,sizeof(area));std::memcpy(coordinates,&tiles,sizeof(tiles));
  Array(L,"l_area",areas);Array(L,"l_tiles",coordinates);
  lua_createtable(L,8,0);for(unsigned i=0;i<8;++i){lua_pushinteger(L,std::int32_t(r._doorGridPositions[i]));lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"door_positions");
  for(const char *name:{"valid","interior"}){lua_createtable(L,448,0);for(int i=0;i<448;++i){lua_pushboolean(L,replica::RoomShapeGridCell(config->Shape,i,name[0]=='v'));lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,name);}
  lua_pushstring(L,"active_extent;outside_not_applicable");lua_setfield(L,-2,"rail_scope");
  lua_createtable(L,448,0);for(unsigned i=0;i<448;++i){const auto rail=i<extent?Read<std::uint8_t>(&r,0x746c+i):std::uint8_t(255);
    if(!replica::RoomRailTypeValid(rail)){lua_settop(L,top);lua_pushnil(L);return 1;}lua_pushinteger(L,rail);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"rails");return 1;
}
// 只把Room实际拥有且已加载的四个ANM2借给同一Lua栈；正式read.Value随后复制值并撤销借用。
int ClientRoomBackdropSpritesLua(void *state){
  auto *L=static_cast<lua_State *>(state);lua_createtable(L,4,0);
  if(!IsMain()||!g_Game||!g_Game->_room)return 1;auto &b=g_Game->_room->_backdrop;
  ANM2 *sprites[]={&b.floorANM2,&b._wallDetailANM2,&b._controlsANM2,&b._controlsButtonsANM2};
  for(unsigned i=0;i<4;++i){if(sprites[i]->_loaded)lua::luabridge::UserdataPtr::push(L,sprites[i],lua::Metatables::SPRITE);
    else lua_pushboolean(L,false);lua_rawseti(L,-2,i+1);}return 1;
}
int ClientRoomVeinSpritesLua(void *state){
  auto *L=static_cast<lua_State *>(state);lua_createtable(L,2,0);if(!IsMain()||!g_Game||!g_Game->_room)return 1;
  auto *fx=reinterpret_cast<unsigned char *>(&g_Game->_room->_fxLayers);const bool visible=Read<void *>(fx,0xc4)!=nullptr;
  for(unsigned i=0;i<2;++i){auto *sprite=reinterpret_cast<ANM2 *>(fx+0x2c8+i*sizeof(ANM2));
    if(visible&&sprite->_loaded)lua::luabridge::UserdataPtr::push(L,sprite,lua::Metatables::SPRITE);else lua_pushboolean(L,false);lua_rawseti(L,-2,i+1);}return 1;
}
// GetConfig未公开注册；只借用真实Minimap owner内的两份ANM2，Lua在当前窗口立即复制语义pose。
int ClientRoomMapSpritesLua(void *state){
  auto *L=static_cast<lua_State *>(state);lua_createtable(L,2,0);
  if(!IsMain()||!g_Game)return 1;
  for(unsigned i=0;i<2;++i){auto *sprite=&g_Game->_minimap._config[i]._mapSprite;
    lua::luabridge::UserdataPtr::push(L,sprite,lua::Metatables::SPRITE);lua_rawseti(L,-2,i+1);}return 1;
}
// GetFrameCount=Game._frameCount-Room[11f0]；每次展示前重新锚定，服务器帧数可合法回到旧值。
void ClientRoomFrameMaintain(void *game,void *room,int frame,bool initialized) noexcept {
  if(!IsMain()||!game||!room)return;auto &g=*static_cast<Game *>(game);
  const auto baseline=std::uint32_t(g._frameCount)-std::uint32_t(frame);Write(room,0x11f0,baseline);Write(room,0,initialized);
}
bool ClientRoomStateApply(void *game,void *room,const ClientRoomState &s) noexcept {
  if(!IsMain()||!game||!room)return false;auto &g=*static_cast<Game *>(game);auto &r=*static_cast<Room *>(room);if(!r._descriptor)return false;
  auto &d=*r._descriptor;ClientRoomFrameMaintain(game,room,s.frame,s.initialized);
  r._roomClearDelay=s.clear_delay;
  // SDK将7220标成int slowdown，但J273 807146仅读取低字节谓词；高三字节没有采用合同。
  // 兼容旧来源的原始word时只供应已核定的byte条件，不把冷堆高位覆盖进Client。
  Write<std::uint8_t>(&r,0x7220,s.render_flag_7220);
  r._greedWaveTimer=s.greed_timer;r._redHeartDamage=s.red_heart_damage;r._pacifist=s.pacifist;r._bossId=s.boss;
  g._greedModeWave=unsigned(s.greed_wave);g._ambush.active=s.ambush_active;g._ambush.currentWave=s.ambush_wave;g._ambush.maxBossWaves=s.ambush_max;g._ambush.bossesPerWave=s.ambush_bosses;
  d.TotalMoneySpent=std::int16_t(s.total_money);d.ShopItemIdx=std::int16_t(s.shop_item);d.ShopItemDiscountIdx=std::int16_t(s.shop_discount);d.GreedSubType=s.greed_subtype;
  d.PoopCount=std::int16_t(s.poop_count);d.TKeeperCoinSpawns=std::int16_t(s.keeper_coins);d.PitsCount=s.pits;
  std::memcpy(reinterpret_cast<unsigned char *>(&r)+0x14,s.bounds.data(),sizeof(float)*4);
  std::copy(s.door_positions.begin(),s.door_positions.end(),r._doorGridPositions);
  std::memcpy(reinterpret_cast<unsigned char *>(&r)+0x746c,s.rails.data(),448);
  if(s.grid_paths.size()!=r._gridWidth*r._gridHeight||s.grid_paths.size()>448)return false;
  std::copy(s.grid_paths.begin(),s.grid_paths.end(),r._gridPaths);
  r._railManager._room=&r;
  if(s.rails_present&&!ClientApplyResourcePose(&r._railManager._sprite,s.rail_sprite).complete)return false;
  return true;
}
// 9B7C70只选择每格rail帧并绘制，不生成Minecart；坑上的轨道由后续GridPit pass接替。
// SetAnimation会重建内嵌ANM索引，暂摘原数组后离栈归还；重复draw不推进服务器姿态。
void ClientRoomRailsDraw(void *room,const ClientRoomState &s,const ClientPoseVector &offset){
  if(!s.rails_present)return;auto &r=*static_cast<Room *>(room);RailDrawBorrow borrow(r._railManager._sprite,false);Vector value(offset.x,offset.y);
  reinterpret_cast<void (__thiscall *)(RailManager *,const Vector *)>(Base+0x5b7c70)(&r._railManager,&value);
}
void ClientRoomWaterPreDraw(void *room){
  auto &r=*static_cast<Room *>(room);RailDrawBorrow borrow(r._railManager._sprite,true);
  reinterpret_cast<void (__thiscall *)(Room *)>(Base+0x416250)(&r);
}
// 普通轨道与坑轨道位于不同pass；已核9B7F00同样只选帧/绘制，不创建矿车。
void ClientRoomPitRailsDraw(void *room,const ClientRoomState &s,const ClientPoseVector &offset){
  if(!s.rails_present)return;auto &r=*static_cast<Room *>(room);RailDrawBorrow borrow(r._railManager._sprite,true);Vector value(offset.x,offset.y);
  reinterpret_cast<void (__thiscall *)(RailManager *,const Vector *)>(Base+0x5b7f00)(&r._railManager,&value);
}
} // namespace cs::game
