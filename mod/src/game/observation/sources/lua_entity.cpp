#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/observation/sources/lua_owned_source.h"
#include "LuaEntitySaveState.h"
#include "binding/world_layout_constants.h"
#include "game/observation/sources/lua_entity.h"
#include "game/common/bridge.h"
#include "game/observation/sources/global_random.h"
#include "game/observation/sources/random_sources.h"
#include "game/client/room/client_room_native.h"
#include "game/client/room/client_map_native.h"
#include "game/client/room/client_backdrop_native.h"
#include "replica/stream_resource_e2e.h"
#include "runtime.h"
#include "game/tests/client_entity_e2e.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <utility>
#include <stdexcept>
#include <string_view>
namespace cs::game {
#ifdef ISAACCS_E2E
// 已观测ANM入口在主线程仍有效；构造前不读未初始化字段，随后立即复制纯值。
void E2EResourceChangeFromNative(void *object,std::uint32_t address,std::uint64_t generation,bool reading,bool retired,bool constructing,bool copying,const char *operation) noexcept {
  if(!IsMain())return;
  const auto *anm=static_cast<const ANM2 *>(object);
  const bool available=!constructing&&anm->_filename.size()<=1024;
  replica::stream::E2EResourceChange(address,generation,reading,retired,constructing,copying,
    available?anm->_filename.c_str():nullptr,available,available&&anm->_loaded,operation);
}
int ClientTestEntitySourceLua(lua_State *);
#endif
int ClientPresentationCaptureLua(void *state);
int ClientGridCaptureLua(void *);
int ClientGridSpritesLua(void *);
int ClientEntityGeometryLua(lua_State *);
int ClientEntityOwnedPresentationLua(lua_State *);
int ClientEntityNpcPresentationLua(lua_State *);
int ClientPlayerStateCaptureLua(lua_State *);
int ClientPlayerEquipmentCaptureLua(lua_State *);
int ClientPlayerRenderCaptureLua(lua_State *);
int ClientPlayerRoleCaptureLua(lua_State *);
int ClientPlayerSpecialCaptureLua(lua_State *);
namespace {
#ifdef ISAACCS_E2E
// 纯计时诊断只接收固定域／阶段编号，不读游戏对象，也不把耗时混入同步值。
int SourceClockLua(lua_State *L){lua_pushinteger(L,lua_Integer(ClientEntityTestClock()));return 1;}
int SourceTimeLua(lua_State *L){
  const auto domain=luaL_checkinteger(L,1),stage=luaL_checkinteger(L,2),start=luaL_checkinteger(L,3);
  constexpr const char *domains[]={"entity","grid","room","level","exploration","room_save","player","run","random","presentation","mod"};
  constexpr const char *stages[]={"collect","partition","encode","sprite","player_fields","item_config","animation"};
  const auto now=ClientEntityTestClock();
  if(domain>=0&&domain<11&&stage>=0&&stage<7&&start>=0&&now>=std::uint64_t(start)){
    char name[80];std::snprintf(name,sizeof(name),"source_%s_%s",domains[domain],stages[stage]);ClientEntityTestTime(name,now-std::uint64_t(start));
  }
  return 0;
}
#endif
lua_State *State() noexcept { return IsMain()&&g_LuaEngine ? g_LuaEngine->_state : nullptr; }
void Decimal(lua_State *L,const char *key,std::uint64_t value) {
  const auto s=std::to_string(value); lua_pushlstring(L,s.data(),s.size()); lua_setfield(L,-2,key);
}
int Subject(lua_State *L,bool registering,bool grid=false) {
  if(!IsMain()) { lua_pushnil(L); return 1; }
  if(lua_isnil(L,1)) { lua_newtable(L); lua_pushstring(L,"Null"); lua_setfield(L,-2,"resolution"); return 1; }
  void *entity=grid?static_cast<void *>(lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity")):static_cast<void *>(lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity"));
  const auto subject=ObservationSubject(entity,registering,grid);
  lua_newtable(L);
  Decimal(L,"session",subject.session); Decimal(L,"epoch",subject.epoch);
  Decimal(L,"local_id",subject.local); Decimal(L,"use",subject.use); Decimal(L,"revision",subject.revision);
  lua_pushinteger(L,grid?2:1); lua_setfield(L,-2,"space");
  constexpr const char *names[]={"Null","KnownCurrent","KnownHistorical","Unresolved","NotCaptured","UnsupportedTarget","Retired","AmbiguousReuse","CrossEpoch"};
  lua_pushstring(L,subject.status<9?names[subject.status]:"Unresolved"); lua_setfield(L,-2,"resolution");
  lua_pushinteger(L,subject.origin); lua_setfield(L,-2,"origin");
  lua_pushinteger(L,subject.readiness); lua_setfield(L,-2,"native_readiness");
  if(subject.ordinal<4&&subject.participant&&subject.group) {
    lua_newtable(L); lua_pushinteger(L,subject.ordinal); lua_setfield(L,-2,"ordinal");
    Decimal(L,"participant_local_id",subject.participant); Decimal(L,"control_group_local_id",subject.group);
    Decimal(L,"control_session",subject.control_session); Decimal(L,"binding_revision",subject.binding_revision);
    Decimal(L,"device_relation_revision",subject.device_revision);
    if(subject.control_active) { lua_pushinteger(L,subject.body_slot); lua_setfield(L,-2,"body_slot"); }
    if(subject.group_known) {
      lua_pushinteger(L,subject.body_role); lua_setfield(L,-2,"body_role");
      lua_pushboolean(L,subject.body_active); lua_setfield(L,-2,"active");
      lua_pushboolean(L,subject.can_consume); lua_setfield(L,-2,"can_consume");
      lua_pushvalue(L,-1); lua_setfield(L,-3,"control_group_membership");
    }
    if(subject.control_active) lua_setfield(L,-2,"control_relation"); else lua_pop(L,1);
  }
  return 1;
}
int RegisterSubject(lua_State *L) { return Subject(L,true); }
int Reference(lua_State *L) { return Subject(L,false); }
int GridSubject(lua_State *L) { return Subject(L,true,true); }
int GridReference(lua_State *L) { return Subject(L,false,true); }
int GridApplication(lua_State *L) {return ClientGridCaptureLua(L);}
int GridApplicationSprites(lua_State *L) {return ClientGridSpritesLua(L);}
int MinimapDescriptor(lua_State *L) {return ClientMapDescriptorCaptureLua(L);}
int MapApplication(lua_State *L) {return ClientMapStateCaptureLua(L);}
// 官方SaveStateVector的公开Get逐槽返回同一vector元素；一次复制八个已公开字段，
// 不保存vector地址、不以长度/seed推断未改变，也不调用保存/还原操作。
int GridSaveValues(lua_State *L){
  if(!IsMain())return luaL_error(L,"save_grid_wrong_thread");
  auto *view=lua::GetRawUserdata<Lua_GridEntitiesSaveStateVector *>(L,1,lua::metatables::GridEntitiesSaveStateVectorMT);
  if(!IsMain()||!view||!view->data)return luaL_error(L,"save_grid_source_unavailable");
  const auto limit=luaL_checkinteger(L,2);const auto size=view->data->size();
  if(limit<0||limit>16384||size>std::size_t(limit))return luaL_error(L,"save_grid_source_budget");
  const auto *data=view->data->data();lua_createtable(L,int(size),0);
  for(std::size_t i=0;i<size;++i){const auto &v=data[i];lua_createtable(L,8,0);lua_pushboolean(L,v._initialized);lua_rawseti(L,-2,1);
    const std::int64_t fields[]={v._spawnCount,v._spawnSeed,v._state,v._type,v._varData,v._variableSeed,v._variant};
    for(unsigned j=0;j<7;++j){char text[32];const auto count=std::snprintf(text,sizeof(text),"%lld",static_cast<long long>(fields[j]));lua_pushlstring(L,text,std::size_t(count));lua_rawseti(L,-2,j+2);}lua_rawseti(L,-2,lua_Integer(i+1));}
  if(view->data->data()!=data||view->data->size()!=size)return luaL_error(L,"save_grid_source_changed");return 1;
}
// 批量调用原公开Lua绑定，保留GetCollectibleNum的IgnoreSpoof处理及所有已接入回调。
// 不用原生库存计数冒充四种参数组合；结果在本窗逐项取得，错误逐项返回。
int PlayerScalarQueries(lua_State *L){
  if(!IsMain())return luaL_error(L,"player_batch_wrong_thread");
  lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");
  const auto *name=luaL_checkstring(L,2);
  if(std::strcmp(name,"GetCollectibleNum")&&std::strcmp(name,"GetActiveItemSlot"))return luaL_error(L,"player_batch_member");
  luaL_checktype(L,3,LUA_TTABLE);const auto count=lua_rawlen(L,3);if(count>131072)return luaL_error(L,"player_batch_budget");
  lua_getfield(L,1,name);const auto fn=lua_absindex(L,-1);if(!lua_isfunction(L,fn))return luaL_error(L,"player_batch_missing");
  lua_createtable(L,int(count),0);const auto out=lua_absindex(L,-1);
  for(std::size_t i=1;i<=count;++i){lua_rawgeti(L,3,lua_Integer(i));const auto args=lua_absindex(L,-1);const auto length=lua_rawlen(L,args);
    if(length>3)return luaL_error(L,"player_batch_arguments");lua_pushvalue(L,fn);lua_pushvalue(L,1);for(std::size_t j=1;j<=length;++j)lua_rawgeti(L,args,lua_Integer(j));
    const auto ok=lua_pcall(L,int(length)+1,1,0)==LUA_OK;lua_createtable(L,0,2);lua_pushboolean(L,ok);lua_setfield(L,-2,"ok");lua_pushvalue(L,-2);lua_setfield(L,-2,ok?"value":"error");lua_rawseti(L,out,lua_Integer(i));lua_pop(L,2);
  }return 1;
}
// 正式来源直接封闭查询矩阵，不再先构造数万个Lua参数/整数包装表再逐个复制。
// 每项仍调用同一个公开绑定一次，四种bool组合、真实错误及参数图别名完全保留。
int PlayerScalarQueriesOwned(lua_State *L){
  if(!IsMain())return luaL_error(L,"player_batch_wrong_thread");
  lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");
  const auto *name=luaL_checkstring(L,2);
  const bool collectible=std::strcmp(name,"GetCollectibleNum")==0;
  if(!collectible&&std::strcmp(name,"GetActiveItemSlot"))return luaL_error(L,"player_batch_member");
  luaL_checktype(L,3,LUA_TTABLE);luaL_checktype(L,4,LUA_TTABLE);
  auto graph=luaL_checkinteger(L,5);const auto node_limit=luaL_checkinteger(L,6);
  const auto count=lua_rawlen(L,3);const unsigned arity=collectible?3u:1u;
  if(count>131072||graph<0||node_limit<0||node_limit>(64u<<20))return luaL_error(L,"player_batch_budget");
  // 先检查整张声明参数表；拒绝时不部分执行后又退回旧路径重读。
  for(std::size_t i=1;i<=count;++i){lua_rawgeti(L,3,lua_Integer(i));
    if(!lua_istable(L,-1)||lua_rawlen(L,-1)!=arity)return luaL_error(L,"player_batch_arguments");
    for(unsigned j=1;j<=arity;++j){lua_rawgeti(L,-1,j);const bool valid=j==1?lua_isinteger(L,-1):lua_isboolean(L,-1);lua_pop(L,1);if(!valid)return luaL_error(L,"player_batch_argument_type");}lua_pop(L,1);}
  // seen表扩容可能触发Lua长跳转，须在建立C++来源owner之前完成；负编号标记首次定义。
  lua_createtable(L,int(count),0);const auto ids=lua_absindex(L,-1);std::uint64_t consumed=0;
  for(std::size_t i=1;i<=count;++i){lua_rawgeti(L,3,lua_Integer(i));const auto args=lua_absindex(L,-1);
    lua_pushvalue(L,args);lua_rawget(L,4);const bool known=!lua_isnil(L,-1);const auto id=known?lua_tointeger(L,-1):++graph;lua_pop(L,1);
    consumed+=2+(known?0:2*arity);if(consumed>std::uint64_t(node_limit))return luaL_error(L,"player_batch_value_budget");
    if(!known){lua_pushvalue(L,args);lua_pushinteger(L,id);lua_rawset(L,4);}
    lua_pushinteger(L,known?id:-id);lua_rawseti(L,ids,lua_Integer(i));lua_pop(L,1);}
  lua_getfield(L,1,name);const auto fn=lua_absindex(L,-1);if(!lua_isfunction(L,fn))return luaL_error(L,"player_batch_missing");
  auto *block=NewOwnedBlock(L);const auto top=lua_gettop(L);
  bool complete=true;
  try{
    ClientEntityTestMeasure timing("player_scalar_owned");
    block->source=capture::OwnedSource::CreateGraph([&](replica::Json &out,std::uint64_t &bytes,std::uint64_t &nodes,auto &){
      auto generate=[&](auto &h){
        const auto add=[&](std::uint64_t n){if(n>(32u<<20)-bytes)throw std::runtime_error("player_batch_bytes");bytes+=n;};
        const auto node=[&](){if(++nodes>(64u<<20))throw std::runtime_error("player_batch_nodes");};
        const auto quoted=[&](const char *s,std::size_t size){add(2);for(std::size_t i=0;i<size;++i){const auto c=static_cast<unsigned char>(s[i]);add(c<32||c=='\\'||c=='"'?6:1);}};
        const auto key=[&](const char *s){const auto size=std::strlen(s);quoted(s,size);add(1);h.Key(s,rapidjson::SizeType(size),true);};
        const auto str=[&](const char *s,std::size_t size){node();quoted(s,size);h.String(s,rapidjson::SizeType(size),true);};
        const auto text=[&](const char *s){str(s,std::strlen(s));};
        const auto object=[&](){node();add(2);h.StartObject();};
        const auto end_object=[&](unsigned n){if(n)add(n-1);h.EndObject(n);};
        const auto array=[&](){node();add(2);h.StartArray();};
        const auto end_array=[&](unsigned n){if(n)add(n-1);h.EndArray(n);};
        const auto integer=[&](lua_Integer n,bool typed){char buffer[32];const auto size=std::snprintf(buffer,sizeof(buffer),"%lld",static_cast<long long>(n));
          if(typed){object();key("type");text("int64");key("value");str(buffer,unsigned(size));end_object(2);}
          else {node();add(unsigned(size));h.Int64(n);}};
        const auto scalar=[&](int at){if(lua_isinteger(L,at))integer(lua_tointeger(L,at),true);
          else if(lua_isboolean(L,at)){const bool b=lua_toboolean(L,at)!=0;node();add(b?4:5);h.Bool(b);}
          else if(lua_type(L,at)==LUA_TNUMBER&&std::isfinite(lua_tonumber(L,at))){node();add(32);h.Double(lua_tonumber(L,at));}
          else throw std::runtime_error("player_batch_scalar");};
        array();
        for(std::size_t i=1;i<=count;++i){
          lua_rawgeti(L,3,lua_Integer(i));const auto args=lua_absindex(L,-1);
          lua_rawgeti(L,ids,lua_Integer(i));const auto marked=lua_tointeger(L,-1);lua_pop(L,1);const bool known=marked>0;const auto id=known?marked:-marked;
          lua_pushvalue(L,fn);lua_pushvalue(L,1);for(unsigned j=1;j<=arity;++j)lua_rawgeti(L,args,j);
          const bool ok=lua_pcall(L,int(arity)+1,1,0)==LUA_OK;const auto answer=lua_absindex(L,-1);
          object();key("parameters");object();key("type");text(known?"graph_ref":"table");key("node");integer(id,false);
          if(!known){key("entries");array();for(unsigned j=1;j<=arity;++j){object();key("key");integer(j,true);key("value");lua_rawgeti(L,args,j);scalar(-1);lua_pop(L,1);end_object(2);}end_array(arity);}end_object(known?2:3);
          key("value");
          if(ok)scalar(answer);
          else {
            complete=false;object();key("status");text("ReadFailed");key("reason");
            // 与Lua textError相同，只保留有界语义文本，移除本地十六进制地址。
            char clean[257]{};std::size_t size=0,used=0;const char *error=lua_type(L,answer)==LUA_TSTRING?lua_tolstring(L,answer,&size):"lua_error";
            if(lua_type(L,answer)!=LUA_TSTRING)size=9;
            for(std::size_t at=0;at<size&&used<256;){const auto hex=[](char c){return c>='0'&&c<='9'||c>='a'&&c<='f'||c>='A'&&c<='F';};
              if(at+2<size&&error[at]=='0'&&error[at+1]=='x'&&hex(error[at+2])){at+=2;while(at<size&&hex(error[at]))++at;for(const char c:std::string_view("<local>"))if(used<256)clean[used++]=c;}
              else clean[used++]=error[at++];}
            str(clean,used);end_object(2);
          }
          end_object(2);lua_settop(L,top);
        }
        end_array(unsigned(count));return true;
      };out.Populate(generate);
    });
  }catch(...){lua_settop(L,top);return luaL_error(L,"player_owned_batch_failed");}
  lua_settop(L,top);lua_pushboolean(L,complete);lua_pushinteger(L,graph);lua_pushinteger(L,lua_Integer(consumed));return 4;
}
// 当前专用关系只发布稳定引用；不把本地地址、getter派生语义或历史未初始化值发出去。
void PlayerLink(lua_State *L,const char *name,Entity *entity){
  auto s=ObservationSubject(entity,false,false);
  // 原生槽的真实空指针是Null；身份注册表缺失只用于非空未知对象，不能混淆两者。
  if(!entity)s.status=0;
  lua_newtable(L);
  constexpr const char *names[]={"Null","KnownCurrent","KnownHistorical","Unresolved","NotCaptured","UnsupportedTarget","Retired","AmbiguousReuse","CrossEpoch"};
  lua_pushstring(L,names[s.status<9?s.status:3]);lua_setfield(L,-2,"resolution");
  lua_pushinteger(L,1);lua_setfield(L,-2,"space");Decimal(L,"local_id",s.local);Decimal(L,"use",s.use);Decimal(L,"revision",s.revision);
  lua_setfield(L,-2,name);
}
// 正式Player集合来源只数当前原生列表，不将roster/可输入资格推断为Room成员。
int PlayerApplicationMembership(lua_State *L) {
  auto *p=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");
  lua_newtable(L);lua_pushinteger(L,1);lua_setfield(L,-2,"version");
  const auto subject=ObservationSubject(p,false,false);
  if(!IsMain()||!g_Game||!g_Game->_room||subject.status!=1){lua_pushstring(L,"Unavailable");lua_setfield(L,-2,"status");return 1;}
  auto &list=g_Game->_room->_entityList;
  unsigned owner=0,render=0,update=0;
  for(auto entry:{std::make_pair(&list._mainEL,&owner),std::make_pair(&list._persistentEL,&owner),std::make_pair(&list._renderEL,&render),std::make_pair(&list._updateEL,&update)}){
    const auto &values=*entry.first;
    if(values._sublist||values._size>values._capacity||values._capacity>16384||(values._size&&!values._data)){
      lua_pushstring(L,"Unavailable");lua_setfield(L,-2,"status");return 1;}
    for(unsigned i=0;i<values._size;++i)*entry.second+=values._data[i]==p;
  }
  lua_pushstring(L,"Value");lua_setfield(L,-2,"status");
  lua_pushinteger(L,owner);lua_setfield(L,-2,"room_owners");
  lua_pushinteger(L,render);lua_setfield(L,-2,"render_members");
  lua_pushinteger(L,update);lua_setfield(L,-2,"update_members");
  lua_pushboolean(L,p->_valid);lua_setfield(L,-2,"valid");
  lua_newtable(L);
  if(subject.readiness>=2){
    lua_pushstring(L,"Value");lua_setfield(L,-2,"status");
    Entity *sub=nullptr;std::memcpy(&sub,reinterpret_cast<const unsigned char *>(p)+world::layout::LayoutConstants().player_sub_storage_offset,4);
    PlayerLink(L,"sub_storage",sub);PlayerLink(L,"twin_storage",p->_twinPlayer.GetReference());
    PlayerLink(L,"backup_storage",p->_backupPlayer);PlayerLink(L,"replaced_storage",p->_replacedPlayer);
  }else {lua_pushstring(L,"Unavailable");lua_setfield(L,-2,"status");}
  lua_setfield(L,-2,"links");
  return 1;
}
// 官方 Lua 包装每次 new Vector 未回收；此处使用已声明 SDK Getter 的栈缓冲。
int GridRenderPosition(lua_State *L) {
  auto *grid=lua::GetLuabridgeUserdata<GridEntity *>(L,1,lua::Metatables::GRID_ENTITY,"GridEntity");
  Vector buffer(0,0); grid->GetRenderPosition(&buffer);
  lua_newtable(L); lua_pushstring(L,"vector"); lua_setfield(L,-2,"type");
  lua_pushnumber(L,buffer.x); lua_setfield(L,-2,"x"); lua_pushnumber(L,buffer.y); lua_setfield(L,-2,"y"); return 1;
}
// I02 正式只读来源：复制实际descriptor存储和已发布offset，不能从房形/seed猜占用。
// 只比较已构造数组内地址；离图-19不读取，未知userdata不参与寻址，不调用GetRoomByIdx初始化分支。
int DescriptorPlacement(lua_State *L) {
  auto *d=lua::GetLuabridgeUserdata<RoomDescriptor *>(L,1,lua::Metatables::ROOM_DESCRIPTOR,lua::Metatables::CONST_ROOM_DESCRIPTOR,"RoomDescriptor");
  lua_newtable(L);
  int storage=-1,slot=0;
  if(IsMain()&&g_Game&&d){
    for(int i=0;i<507;++i)if(d==&g_Game->_gridRooms[i]){storage=i;break;}
    if(storage<0)for(int i=0;i<20;++i)if(i!=18&&d==&g_Game->_negativeGridRooms[i]){slot=-i-1;break;}
  }
  if(storage<0&&!slot){lua_pushstring(L,"Unavailable");lua_setfield(L,-2,"status");return 1;}
  lua_pushstring(L,"Value");lua_setfield(L,-2,"status");
  lua_pushinteger(L,1);lua_setfield(L,-2,"version");
  lua_pushinteger(L,storage);lua_setfield(L,-2,"storage_index");
  if(slot){lua_pushinteger(L,slot);lua_setfield(L,-2,"source_slot");}
  lua_pushinteger(L,d->GridIndex);lua_setfield(L,-2,"grid_index");
  lua_pushinteger(L,d->SafeGridIndex);lua_setfield(L,-2,"safe_grid_index");
  lua_pushinteger(L,d->ListIndex);lua_setfield(L,-2,"list_index");
  lua_pushinteger(L,d->Dimension);lua_setfield(L,-2,"dimension_index");
  lua_newtable(L);int count=0;
  if(storage>=0&&d->Dimension>=0&&d->Dimension<3)
    for(int index=0;index<169;++index)if(g_Game->_roomOffset[d->Dimension*169+index]==unsigned(storage)){
      lua_pushinteger(L,index);lua_rawseti(L,-2,++count);
    }
  lua_setfield(L,-2,"occupied_indices");return 1;
}
int GridSources(lua_State *L) {
  lua_newtable(L);
  struct Sink { lua_State *L; int index=0; } sink{L};
  const auto unavailable=ObservationGrids(&sink,[](void *owner,void *grid,std::uint32_t memberships) noexcept {
    auto &s=*static_cast<Sink *>(owner); lua_newtable(s.L);
    lua::luabridge::UserdataPtr::push(s.L,grid,lua::Metatables::GRID_ENTITY); lua_setfield(s.L,-2,"grid");
    lua_pushinteger(s.L,memberships); lua_setfield(s.L,-2,"membership_mask");
    lua_newtable(s.L);
    struct Members { lua_State *L; int index=0; } ms{s.L};
    ObservationGridMemberships(grid,&ms,[](void *owner,std::uint32_t source,std::uint32_t index) noexcept {
      auto &m=*static_cast<Members *>(owner); lua_newtable(m.L);
      lua_pushinteger(m.L,source); lua_setfield(m.L,-2,"source"); lua_pushinteger(m.L,index); lua_setfield(m.L,-2,"index");
      lua_rawseti(m.L,-2,++m.index);
    });
    lua_setfield(s.L,-2,"memberships"); lua_rawseti(s.L,-2,++s.index);
  });
  lua_pushinteger(L,unavailable); return 2;
}
int SourceSnapshotToken(lua_State *L){std::string reason;const auto token=ObservationSourceSnapshotToken(&reason);lua_pushlstring(L,token.data(),token.size());lua_pushlstring(L,reason.data(),reason.size());return 2;}
// 不更改回调传入userdata的元表；构造经保护读取验证过的借用基类视图。
int GridCallbackView(lua_State *L) {
  if(!IsMain()||lua_type(L,1)!=LUA_TUSERDATA||lua_rawlen(L,1)!=sizeof(lua::luabridge::UserdataPtr)) { lua_pushnil(L); return 1; }
  auto *grid=lua::UserdataToData<GridEntity *>(lua_touserdata(L,1));
  if(!ObservationValidGrid(grid)) { lua_pushnil(L); return 1; }
  lua::luabridge::UserdataPtr::push(L,grid,lua::Metatables::GRID_ENTITY); return 1;
}
int GridNeeded(lua_State *L) { lua_pushboolean(L,IsMain()&&ObservationLuaNeeded(luaL_checkstring(L,1),true)); return 1; }
int Supplement(lua_State *L) {
  auto *entity=lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity");
  const auto v=EntitySupplement(entity);
  lua_newtable(L); lua_pushstring(L,v.class_name); lua_setfield(L,-2,"class");
  lua_pushstring(L,v.status); lua_setfield(L,-2,"status");
  if(!std::strcmp(v.class_name,"NativeText")) {
    lua_newtable(L);lua_pushstring(L,v.color_status);lua_setfield(L,-2,"status");
    if(!std::strcmp(v.color_status,"Value")){
      constexpr const char *keys[]={"R","G","B","A"};
      for(unsigned i=0;i<4;++i){lua_pushnumber(L,v.draw_color[i]);lua_setfield(L,-2,keys[i]);}
      lua_pushinteger(L,v.color_extra);lua_setfield(L,-2,"opaque32");
    }
    lua_setfield(L,-2,"draw_color");
  }
  if(!std::strcmp(v.class_name,"NativeText")&&!std::strcmp(v.state_status,"Value")) {
    lua_pushinteger(L,v.counter); lua_setfield(L,-2,"counter");
    lua_pushinteger(L,v.duration); lua_setfield(L,-2,"duration");
  }
  if(!std::strcmp(v.class_name,"NativeText")&&(!std::strcmp(v.status,"Value")||!std::strcmp(v.status,"Truncated"))) {
    lua_pushlstring(L,v.text,v.text_size); lua_setfield(L,-2,"text");
    static const char digits[]="0123456789abcdef";std::string hex;hex.reserve(v.text_size*2);
    for(unsigned i=0;i<v.text_size;++i){const auto b=static_cast<unsigned char>(v.text[i]);hex+=digits[b>>4];hex+=digits[b&15];}
    lua_pushlstring(L,hex.data(),hex.size());lua_setfield(L,-2,"text_hex");
  }
  return 1;
}
int LaserPresentation(lua_State *L) {
  auto *entity=lua::GetLuabridgeUserdata<Entity_Laser *>(L,1,lua::Metatables::ENTITY_LASER,"EntityLaser");
  const auto v=EntityLaserPresentation(entity);lua_newtable(L);lua_pushstring(L,v.status);lua_setfield(L,-2,"status");
  if(!std::strcmp(v.status,"Value")){
    lua_pushinteger(L,v.sample_mode_raw);lua_setfield(L,-2,"sample_mode_raw");
    lua_pushinteger(L,v.start_offset_mode_opaque32);lua_setfield(L,-2,"start_offset_mode_opaque32");
  }
  return 1;
}
// NPC两份辅助ANM由物理ctor分别构造；只借用同use当前成员，不拷贝ANM或运行AI。
bool OrdinaryEntityTypeContext() noexcept {
  if(!IsMain()||!g_Game)return false;
  std::int32_t state=0;
  std::memcpy(&state,reinterpret_cast<const unsigned char *>(g_Game)+0x25b8c,4);
  return state<2;
}
int EntityApplicationSprites(lua_State *L){
  auto *e=lua::GetLuabridgeUserdata<Entity *>(L,1,lua::Metatables::ENTITY,"Entity");const auto subject=ObservationSubject(e,false);
  lua_newtable(L);if(!e||subject.status!=1||subject.readiness<2){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");lua_pushinteger(L,1);lua_setfield(L,-2,"version");
  // 类型来自Lua已经成功的派生转换，再用当前对象作交叉检查，不能按Type范围猜NPC。
  const char *source_class=luaL_checkstring(L,2);const unsigned category=!std::strcmp(source_class,"EntityPlayer")&&e->ToPlayer()?0:e->ToNPC()?1:e->ToProjectile()?3:
      !std::strcmp(source_class,"EntityTear")&&e->_type==2?2:!std::strcmp(source_class,"EntityBomb")&&e->_type==4?4:
      !std::strcmp(source_class,"EntityLaser")&&e->_type==7?5:!std::strcmp(source_class,"EntityKnife")&&e->_type==8?6:
      !std::strcmp(source_class,"EntityPickup")&&e->_type==5?7:!std::strcmp(source_class,"EntityFamiliar")&&e->_type==3?8:
      !std::strcmp(source_class,"EntityEffect")&&e->_type==1000?9:
      !std::strcmp(source_class,"EntitySlot")&&e->_type==6?10:
      !std::strcmp(source_class,"NativeText")&&!std::strcmp(EntitySupplement(e).class_name,"NativeText")?11:
      !std::strcmp(source_class,"NativeDummy")&&!std::strcmp(EntitySupplement(e).class_name,"NativeDummy")?12:13;
  if(category==13)return luaL_error(L,"entity_application_class_witness");
  lua_newtable(L);
  const auto link=[&](const char *name,unsigned offset){auto *target=*reinterpret_cast<Entity **>(reinterpret_cast<unsigned char *>(e)+offset);
    lua::luabridge::UserdataPtr::push(L,target,lua::GetMetatableKey(lua::Metatables::ENTITY));lua_setfield(L,-2,name);};
#define CS_CLASS_LINK(cat,id,offset) if(cat==0||cat==category)link(id,offset);
#include "game/client/entity/client_entity_links.inc"
#undef CS_CLASS_LINK
  lua_setfield(L,-2,"links");
  // 固定声明列显式另列名称，空关系也能由Lua序列化为有名字的Null。
  lua_newtable(L);unsigned link_index=0;
#define CS_CLASS_LINK(cat,id,offset) if(cat==0||cat==category){lua_pushliteral(L,id);lua_rawseti(L,-2,++link_index);}
#include "game/client/entity/client_entity_links.inc"
#undef CS_CLASS_LINK
  lua_setfield(L,-2,"link_names");
  // 只复制本次普通采集窗口的表现输入，不借用独立Render的旧scope或本地玩法随机源。
  lua_newtable(L);lua_pushinteger(L,1);lua_setfield(L,-2,"version");
  lua_pushliteral(L,"same-ordinary-window");lua_setfield(L,-2,"scope");
  if(g_Manager&&g_Game&&OrdinaryEntityTypeContext()){
    lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");
    lua_pushinteger(L,g_Manager->_framecount);lua_setfield(L,-2,"manager_frame");
    std::uint32_t frame=0;std::memcpy(&frame,reinterpret_cast<unsigned char *>(g_Game)+0x25a70,4);
    lua_pushinteger(L,frame);lua_setfield(L,-2,"game_frame");
    lua_pushinteger(L,g_Game->IsPaused()?0:(g_Manager->_framecount+1u)%2u);lua_setfield(L,-2,"subframe");
    lua_pushboolean(L,g_Game->IsPaused());lua_setfield(L,-2,"paused");
    // 两个inline xorshift各为seed+三shift；没有推进它们，也不序列化全局地址。
    for(const auto &p:{std::pair<const char *,unsigned>{"pickup_random",0x7d0ca4u},{"familiar_random",0x7d0674u}}){
      lua_createtable(L,4,0);for(unsigned i=0;i<4;++i){std::uint32_t word=0;std::memcpy(&word,reinterpret_cast<void *>(Base+p.second+i*4),4);lua_pushinteger(L,word);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,p.first);
    }
  }else {lua_pushstring(L,g_Manager&&g_Game?"NotCaptured":"Unavailable");lua_setfield(L,-2,"status");
    if(g_Manager&&g_Game){lua_pushliteral(L,"native_deathmatch_type_context");lua_setfield(L,-2,"reason");}}
  lua_setfield(L,-2,"draw");
  if(category==0){
    // Player ctor 74DC70在74DD16/74DD25分别构造两份真实ANM；当前合法来源只借用，
    // 不把结构存在推断成已Load，更不把未正常更新的备份提升为私有字段可读。
    auto *player=e->ToPlayer();lua_newtable(L);
    const auto &layout=world::layout::LayoutConstants();
    // slot0就是公开BodySprite；其余12项分别复制，未知用途仅用物理slot命名。
    for(unsigned i=1;i<layout.player_inline_sprite_count;++i){
      const auto name=i==10?std::string("blood_gush"):i==11?std::string("held"):"inline_"+std::to_string(i);
      auto *sprite=reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(player)+layout.player_inline_sprite_first_offset+i*layout.player_inline_sprite_stride);
      lua::luabridge::UserdataPtr::push(L,sprite,lua::Metatables::SPRITE);lua_setfield(L,-2,name.c_str());
    }
    lua_setfield(L,-2,"player_sprites");
    lua_newtable(L);lua_pushinteger(L,1);lua_setfield(L,-2,"version");
    auto *minecart=e->GetMinecart();const bool used=minecart&&minecart->_type==965&&minecart->_variant==10;
    lua_pushboolean(L,used);lua_setfield(L,-2,"minecart_draw");
    // 真正Player的+C28是+758起ANM2数组第5项的layerCount，绝不能作为NPC路径长度读取。
    if(used){Vector offset;std::memcpy(&offset,reinterpret_cast<unsigned char *>(e)+0x205c,8);
      if(!std::isfinite(offset.x)||!std::isfinite(offset.y))return luaL_error(L,"entity_player_minecart_draw_nonfinite");
      lua_newtable(L);lua_pushnumber(L,offset.x);lua_setfield(L,-2,"x");lua_pushnumber(L,offset.y);lua_setfield(L,-2,"y");lua_setfield(L,-2,"offset");
      std::uint32_t sort_bits=0;std::memcpy(&sort_bits,&offset.y,4);lua_pushinteger(L,sort_bits);lua_setfield(L,-2,"sort_bits");}
    lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");lua_setfield(L,-2,"player_draw");
  }
  if(category==1){lua_pushinteger(L,*reinterpret_cast<unsigned *>(reinterpret_cast<unsigned char *>(e)+0xc00));lua_setfield(L,-2,"npc_private_reference_count");
    lua_pushboolean(L,*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(e)+0xf40)!=nullptr);lua_setfield(L,-2,"npc_owned_actor_present");}
  if(category==4){lua_pushboolean(L,*reinterpret_cast<bool *>(reinterpret_cast<unsigned char *>(e)+0x462));lua_setfield(L,-2,"bomb_costume_draw_enabled");}
  if(e->ToNPC())for(const auto &p:{std::pair<const char *,unsigned>{"npc_overlay",0xcd8u},{"npc_effect",0xdecu}}){
    auto *sprite=reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(e)+p.second);
    lua::luabridge::UserdataPtr::push(L,sprite,lua::GetMetatableKey(lua::Metatables::SPRITE));lua_setfield(L,-2,p.first);
  }
  if(category==7){auto *sprite=&static_cast<Entity_Pickup *>(e)->_altPedestalANM2;
    lua::luabridge::UserdataPtr::push(L,sprite,lua::GetMetatableKey(lua::Metatables::SPRITE));lua_setfield(L,-2,"pickup_alternate");}
  if(category==6){auto *sprite=*reinterpret_cast<ANM2 **>(Base+0x7e5594);
    lua::luabridge::UserdataPtr::push(L,sprite,lua::GetMetatableKey(lua::Metatables::SPRITE));lua_setfield(L,-2,"knife_dead_eye");}
  if(category>=5){
    lua_newtable(L);lua_pushinteger(L,2);lua_setfield(L,-2,"version");lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");
    if(category==8){lua_pushboolean(L,*reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(e)+0xecc)!=nullptr);lua_setfield(L,-2,"private_owner_present");
      lua_pushboolean(L,static_cast<Entity_Familiar *>(e)->_weapon!=nullptr);lua_setfield(L,-2,"weapon_present");}
    if(category==9){auto *bytes=reinterpret_cast<unsigned char *>(e);lua_pushboolean(L,*reinterpret_cast<void **>(bytes+0x4b4)!=nullptr);lua_setfield(L,-2,"private_owner_present");
      lua_pushboolean(L,*reinterpret_cast<void **>(bytes+0x4a8)!=*reinterpret_cast<void **>(bytes+0x4ac));lua_setfield(L,-2,"private_container_nonempty");}
    lua_setfield(L,-2,"runtime");
  }
  return 1;
}
int Sources(lua_State *L) {
  lua_newtable(L);
  struct Sink { lua_State *L; int index=0; } sink{L};
  const auto unavailable=ObservationEntities(&sink,[](void *owner,void *entity,std::uint32_t readiness) noexcept {
    auto &s=*static_cast<Sink *>(owner);
    lua_newtable(s.L);
    lua::luabridge::UserdataPtr::push(s.L,entity,lua::Metatables::ENTITY); lua_setfield(s.L,-2,"entity");
    lua_pushinteger(s.L,readiness); lua_setfield(s.L,-2,"native_readiness");
    lua_rawseti(s.L,-2,++s.index);
  });
  lua_pushinteger(L,unavailable); return 2;
}
int SpriteNames(lua_State *L) {
  auto *sprite=lua::GetLuabridgeUserdata<ANM2 *>(L,1,lua::Metatables::SPRITE,"Sprite");
  lua_newtable(L); lua_newtable(L); lua_setfield(L,-2,"nulls"); lua_newtable(L); lua_setfield(L,-2,"events");
  struct Sink { lua_State *L; int nulls=0,events=0; } sink{L};
  const auto *status=ObservationSpriteNames(sprite,&sink,[](void *owner,bool event,const char *name,std::size_t length) noexcept {
    auto &s=*static_cast<Sink *>(owner); lua_getfield(s.L,-1,event?"events":"nulls");
    lua_pushlstring(s.L,name,length); lua_rawseti(s.L,-2,event?++s.events:++s.nulls); lua_pop(s.L,1);
  });
  lua_pushstring(L,status); lua_setfield(L,-2,"status"); return 1;
}
// 公开 GetFrame 会截断分数帧；这里只复制当前动画的纯值相位和选择器，不推进动画。
int SpriteRuntimePose(lua_State *L) {
  if(!IsMain())return luaL_error(L,"sprite_pose_main_thread");
  auto *sprite=lua::GetLuabridgeUserdata<ANM2 *>(L,1,lua::Metatables::SPRITE,"Sprite");
  if(!sprite)return luaL_error(L,"sprite_pose_owner");
  const auto valid=[sprite](const AnimationState &s) {
    const auto *d=s._animData;
    return !d||(s._animation==sprite&&std::isfinite(s._animFrame)&&d->_layerCount>=0&&d->_layerCount<=64&&
      d->_nullLayerCount>=0&&d->_nullLayerCount<=64&&(!d->_layerCount||s._layerFrames)&&(!d->_nullLayerCount||s._nullLayerFrames));
  };
  if(!valid(sprite->_animState)||!valid(sprite->_overlayAnimState))return luaL_error(L,"sprite_pose_layout");
  lua_newtable(L);lua_pushinteger(L,1);lua_setfield(L,-2,"version");
  lua_pushboolean(L,sprite->_loaded);lua_setfield(L,-2,"loaded");
  lua_pushboolean(L,sprite->_overlayRenderFirst);lua_setfield(L,-2,"overlay_first");
  const auto copy=[L](const AnimationState &s,const char *key) {
    lua_newtable(L);lua_pushnumber(L,s._animData?s._animFrame:0);lua_setfield(L,-2,"phase");
    lua_pushboolean(L,s._animData&&s._isPlaying);lua_setfield(L,-2,"playing");
    lua_newtable(L);if(s._animData)for(int i=0;i<s._animData->_layerCount;++i){lua_pushinteger(L,s._layerFrames[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"layers");
    lua_newtable(L);if(s._animData)for(int i=0;i<s._animData->_nullLayerCount;++i){lua_pushinteger(L,s._nullLayerFrames[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"nulls");
    lua_setfield(L,-2,key);
  };
  copy(sprite->_animState,"main");copy(sprite->_overlayAnimState,"overlay");
  // 第二份冠军ColorMod直接复制11个纯float，不调用着色状态机。
  lua_newtable(L);const auto &champion=sprite->_championColor;
  for(unsigned i=0;i<4;++i){lua_pushnumber(L,champion._tint[i]);lua_rawseti(L,-2,i+1);lua_pushnumber(L,champion._colorize[i]);lua_rawseti(L,-2,i+5);}
  for(unsigned i=0;i<3;++i){lua_pushnumber(L,champion._offset[i]);lua_rawseti(L,-2,i+9);}lua_setfield(L,-2,"champion_color");
  if(!ClientResourceCaptureShaderPaths(L,sprite))return luaL_error(L,"sprite_shader_paths");lua_setfield(L,-2,"shaders");return 1;
}
int SpriteResourceGeneration(lua_State *L){
  auto *sprite=lua::GetLuabridgeUserdata<ANM2 *>(L,1,lua::Metatables::SPRITE,"Sprite");
  // Lua公开的userdata存储范围可直接证明按值所有权，不猜vtable或私有包装偏移。
  const auto storage=reinterpret_cast<std::uintptr_t>(lua_touserdata(L,1));
  const auto address=reinterpret_cast<std::uintptr_t>(sprite);const auto size=lua_rawlen(L,1);
  const bool copied=storage&&address>=storage&&address-storage<=size&&sizeof(ANM2)<=size-(address-storage);
  const auto generation=StreamResourceGeneration(sprite,copied);if(!generation)return luaL_error(L,"stream_resource_instance_unavailable");
  const auto value=std::to_string(generation);lua_pushlstring(L,value.data(),value.size());return 1;
}
int RoomPresentation(lua_State *L) {return ClientPresentationCaptureLua(L);}
int RoomConfigContents(lua_State *L) {return ClientRoomConfigCaptureLua(L);}
int RoomApplication(lua_State *L) {return ClientRoomStateCaptureLua(L);}
int RoomBackdropSprites(lua_State *L) {return ClientRoomBackdropSpritesLua(L);}
int RoomVeinSprites(lua_State *L) {return ClientRoomVeinSpritesLua(L);}
int RoomMapSprites(lua_State *L) {return ClientRoomMapSpritesLua(L);}
int RoomBackdropTrace(lua_State *L) {return ClientBackdropWallTraceLua(L);}
// Lua只取得十进制纯值身份，不持有后台可访问的Room/Level userdata。
int Location(lua_State *L) {
  const auto v=ObservationLocation(int(luaL_checkinteger(L,1)),int(luaL_checkinteger(L,2)),std::uint32_t(luaL_checkinteger(L,3)),int(luaL_checkinteger(L,4)),int(luaL_checkinteger(L,5)),int(luaL_checkinteger(L,6)),luaL_checkstring(L,7));
  lua_newtable(L); Decimal(L,"session",v.session); Decimal(L,"epoch",v.epoch); Decimal(L,"run",v.run); Decimal(L,"floor",v.floor); Decimal(L,"dimension",v.dimension); Decimal(L,"descriptor",v.descriptor); Decimal(L,"room_instance",v.room); return 1;
}
int Descriptor(lua_State *L) { const auto v=ObservationDescriptor(int(luaL_checkinteger(L,1)),int(luaL_checkinteger(L,2)),int(luaL_checkinteger(L,3)),luaL_checkstring(L,4)); const auto s=std::to_string(v); lua_pushlstring(L,s.data(),s.size()); const auto rev=std::to_string(ObservationDescriptorRevision(v)); lua_pushlstring(L,rev.data(),rev.size()); return 2; }
int Dimension(lua_State *L) { const auto s=std::to_string(ObservationDimension(int(luaL_checkinteger(L,1)))); lua_pushlstring(L,s.data(),s.size()); return 1; }
int LocationBoundary(lua_State *L) { ObservationLocationBoundary(luaL_checkstring(L,1),lua_toboolean(L,2)!=0); return 0; }
int RelatedSubject(lua_State *L) { const auto s=std::to_string(ObservationRelatedSubject(unsigned(luaL_checkinteger(L,1)))); lua_pushlstring(L,s.data(),s.size()); return 1; }
int LocationNeeded(lua_State *L) { lua_pushboolean(L,ObservationLocationNeeded()); return 1; }
int PublishLocation(lua_State *L) {
  std::size_t bytes=0; const auto *p=luaL_checklstring(L,1,&bytes);
  lua_pushboolean(L,ObservationPublishLocation(p,bytes,lua_toboolean(L,2)!=0,std::uint32_t(luaL_checkinteger(L,4)),luaL_optstring(L,5,""),unsigned(luaL_checkinteger(L,6)),unsigned(luaL_checkinteger(L,7)))); return 1;
}
int CaptureContext(lua_State *L) { const auto s=ObservationCaptureContext(); lua_pushlstring(L,s.data(),s.size()); return 1; }
int LuaGlobalRandomState(lua_State *L) { const auto s=ObservationGlobalRandomState();lua_pushlstring(L,s.data(),s.size());return 1; }
int LuaNativeRandomSources(lua_State *L){const auto s=RandomSourcesSummary();lua_pushlstring(L,s.data(),s.size());return 1;}
#ifdef ISAACCS_E2E
int LuaTestNativeRandomSources(lua_State *L){
  try{const auto s=TestNativeRandomSources(luaL_checkstring(L,1),luaL_checkstring(L,2),unsigned(luaL_optinteger(L,3,0)),std::uint32_t(luaL_optinteger(L,4,1)));lua_pushlstring(L,s.data(),s.size());return 1;}
  catch(const std::exception &e){return luaL_error(L,"%s",e.what());}
}
#endif
int LuaRegisterRandomSource(lua_State *L){
  auto *rng=lua::GetLuabridgeUserdata<RNG *>(L,1,lua::Metatables::RNG,"RNG");
  std::size_t n=0;const auto *owner=luaL_checklstring(L,2,&n);
  const auto *purpose=luaL_checkstring(L,3);const auto *life=luaL_checkstring(L,4);
  if(n>4096||std::strlen(purpose)>256||std::strlen(life)>256)return luaL_error(L,"random_owner_metadata_budget");
  BindRandomSource({rng,RandomAlgorithm::XorShift32,std::string(owner,n),purpose,life});
  const auto s=RandomSourceReference(rng);lua_pushlstring(L,s.data(),s.size());return 1;
}
// 测试入口只在专用token配置中开放；动作由E2E夹具请求，正式采集器不推进随机源。
#ifdef ISAACCS_E2E
int TestGlobalRandom(lua_State *L) {
  const auto *token=luaL_checkstring(L,1);const auto count=luaL_checkinteger(L,2);
  if(!IsMain()||Settings.world_e2e_token.empty()||Settings.world_e2e_token!=token||count<0||count>1400)return luaL_error(L,"global_random_test_disabled_or_bounds");
  lua_newtable(L);const auto before=ObservationGlobalRandomState(true);lua_pushlstring(L,before.data(),before.size());lua_setfield(L,-2,"before");
  if(!lua_isnoneornil(L,3)){
    const auto value=std::uint32_t(luaL_checkinteger(L,3));
    // 已核定ECX传种子、无栈参数；该函数不把ECX当对象。
    const auto seed=reinterpret_cast<int (__thiscall *)(void *)>(Base+0x2eb470);
    const auto result=seed(reinterpret_cast<void *>(uintptr_t(value)));
    lua_pushinteger(L,result);lua_setfield(L,-2,"seed_result");
  }
  lua_newtable(L);for(lua_Integer i=1;i<=count;++i){lua_pushinteger(L,::Random());lua_rawseti(L,-2,i);}lua_setfield(L,-2,"results");
  const auto after=ObservationGlobalRandomState(true);lua_pushlstring(L,after.data(),after.size());lua_setfield(L,-2,"after");return 1;
}
#endif
// 官方GetGenericPrompt对未构造userdata赋值复制ANM2；只借用活根，复制公开纯值，不复制C++对象。
int PromptState(lua_State *L) {
  if(!IsMain()||!g_Game) { lua_pushnil(L); return 1; }
  auto *p=g_Game->GetGenericPrompt(); lua_newtable(L);
  lua_pushinteger(L,p->_currentSelection); lua_setfield(L,-2,"selection");
  lua_pushinteger(L,p->_submittedSelection); lua_setfield(L,-2,"submitted");
  lua_pushboolean(L,p->IsActive()); lua_setfield(L,-2,"active");
  lua::luabridge::UserdataPtr::push(L,&p->_anm2,lua::Metatables::SPRITE); lua_setfield(L,-2,"sprite");
  return 1;
}
int SaveEntry(lua_State *L) { const auto s=std::to_string(ObservationSaveEntry()); lua_pushlstring(L,s.data(),s.size()); return 1; }
int Needed(lua_State *L) {
  lua_pushboolean(L,IsMain()&&ObservationLuaNeeded(luaL_checkstring(L,1))); return 1;
}
int Publish(lua_State *L,unsigned phase,bool grid=false) {
  std::size_t bytes=0; const auto *json=luaL_checklstring(L,1,&bytes);
  const auto complete=lua_toboolean(L,2)!=0;
  const auto objects=luaL_checkinteger(L,3),nodes=luaL_checkinteger(L,4);
  const auto *reason=luaL_optstring(L,5,"");
  const bool valid=bytes<=(16u<<20)&&objects>=0&&objects<=16384&&nodes>=0&&nodes<=kLuaObservationNodeBudget;
  lua_pushboolean(L,valid&&ObservationPublishLua(json,bytes,complete,unsigned(objects),unsigned(nodes),reason,phase,grid));
  return 1;
}
int PublishEvent(lua_State *L) { return Publish(L,false); }
int PublishQuery(lua_State *L) { return Publish(L,true); }
int PublishRender(lua_State *L) { return Publish(L,2); }
int PublishGridEvent(lua_State *L) { return Publish(L,lua_toboolean(L,6)?2:lua_toboolean(L,7)?4:0,true); }
int PublishGridQuery(lua_State *L) { return Publish(L,1,true); }
}
void EnsureObservationLuaBridge() noexcept {
  auto *L=State(); if(!L) return;
  const auto top=lua_gettop(L);
  lua_getglobal(L,"IsaacCSObservationNative");
  const bool exists=lua_istable(L,-1); lua_settop(L,top);
  if(exists) return;
  RegisterRuntimeRandomFunctions();
  lua_newtable(L);
  const luaL_Reg funcs[]={
#ifdef ISAACCS_E2E
  {"TestNativeRandomSources",LuaTestNativeRandomSources},
  {"TestEntitySource",ClientTestEntitySourceLua},
  {"SourceClock",SourceClockLua},{"SourceTime",SourceTimeLua},
#endif
  {"NativeRandomSources",LuaNativeRandomSources},{"RegisterRandomSource",LuaRegisterRandomSource},{"GlobalRandomState",LuaGlobalRandomState},
#ifdef ISAACCS_E2E
  {"TestGlobalRandom",TestGlobalRandom},
#endif
  {"PromptState",PromptState},{"RelatedSubject",RelatedSubject},{"CaptureContext",CaptureContext},{"SaveEntry",SaveEntry},{"Dimension",Dimension},{"Location",Location},{"Descriptor",Descriptor},{"LocationBoundary",LocationBoundary},{"LocationNeeded",LocationNeeded},{"PublishLocation",PublishLocation},{"GridCallbackView",GridCallbackView},{"GridSubject",GridSubject},{"GridReference",GridReference},{"SourceSnapshotToken",SourceSnapshotToken},{"GridSources",GridSources},{"GridRenderPosition",GridRenderPosition},{"GridNeeded",GridNeeded},{"PublishGridEvent",PublishGridEvent},{"PublishGridQuery",PublishGridQuery},{"Subject",RegisterSubject},{"Reference",Reference},{"Supplement",Supplement},{"Sources",Sources},{"SpriteNames",SpriteNames},{"Needed",Needed},{"PublishEvent",PublishEvent},{"PublishQuery",PublishQuery},{"PublishRender",PublishRender},{nullptr,nullptr}};
  luaL_setfuncs(L,funcs,0);
  lua_pushcfunction(L,SpriteRuntimePose);lua_setfield(L,-2,"SpriteRuntimePose");
  lua_pushcfunction(L,SpriteResourceGeneration);lua_setfield(L,-2,"SpriteResourceGeneration");
  lua_pushcfunction(L,GridSaveValues);lua_setfield(L,-2,"GridSaveValues");
  lua_pushcfunction(L,PlayerScalarQueries);lua_setfield(L,-2,"PlayerScalarQueries");
  lua_pushcfunction(L,PlayerScalarQueriesOwned);lua_setfield(L,-2,"PlayerScalarQueriesOwned");
  lua_pushcfunction(L,SealOwnedBlock);lua_setfield(L,-2,"SealOwnedBlock");
  lua_pushcfunction(L,JoinOwnedBlocks);lua_setfield(L,-2,"JoinOwnedBlocks");
  lua_pushcfunction(L,OwnedBlockText);lua_setfield(L,-2,"OwnedBlockText");
  lua_pushcfunction(L,OwnedBlockEqual);lua_setfield(L,-2,"OwnedBlockEqual");
  // GC只释放自有handle，最后的大树释放由有界回收owner执行。
  if(luaL_newmetatable(L,kOwnedBlockMetatable)){lua_pushcfunction(L,OwnedBlockGc);lua_setfield(L,-2,"__gc");}lua_pop(L,1);
  lua_pushcfunction(L,LaserPresentation);lua_setfield(L,-2,"LaserPresentation");
  lua_pushcfunction(L,EntityApplicationSprites);lua_setfield(L,-2,"EntityApplicationSprites");
  lua_pushcfunction(L,PlayerApplicationMembership);lua_setfield(L,-2,"PlayerApplicationMembership");
  lua_pushcfunction(L,ClientPlayerStateCaptureLua);lua_setfield(L,-2,"PlayerApplicationState");
  lua_pushcfunction(L,ClientPlayerEquipmentCaptureLua);lua_setfield(L,-2,"PlayerApplicationEquipment");
  lua_pushcfunction(L,ClientPlayerRenderCaptureLua);lua_setfield(L,-2,"PlayerApplicationRender");
  lua_pushcfunction(L,ClientPlayerRoleCaptureLua);lua_setfield(L,-2,"PlayerApplicationRole");
  lua_pushcfunction(L,ClientPlayerSpecialCaptureLua);lua_setfield(L,-2,"PlayerApplicationSpecial");
  lua_pushcfunction(L,ClientEntityGeometryLua);lua_setfield(L,-2,"EntityApplicationGeometry");
  lua_pushcfunction(L,ClientEntityOwnedPresentationLua);lua_setfield(L,-2,"EntityApplicationOwned");
  lua_pushcfunction(L,ClientEntityNpcPresentationLua);lua_setfield(L,-2,"EntityApplicationNpc");
  lua_pushcfunction(L,RoomPresentation);lua_setfield(L,-2,"RoomPresentation");
  lua_pushcfunction(L,RoomConfigContents);lua_setfield(L,-2,"RoomConfigContents");
  lua_pushcfunction(L,RoomApplication);lua_setfield(L,-2,"RoomApplication");
  lua_pushcfunction(L,GridApplication);lua_setfield(L,-2,"GridApplication");
  lua_pushcfunction(L,GridApplicationSprites);lua_setfield(L,-2,"GridApplicationSprites");
  lua_pushcfunction(L,MinimapDescriptor);lua_setfield(L,-2,"MinimapDescriptor");
  lua_pushcfunction(L,MapApplication);lua_setfield(L,-2,"MapApplication");
  lua_pushcfunction(L,RoomBackdropSprites);lua_setfield(L,-2,"RoomBackdropSprites");
  lua_pushcfunction(L,RoomVeinSprites);lua_setfield(L,-2,"RoomVeinSprites");
  lua_pushcfunction(L,RoomMapSprites);lua_setfield(L,-2,"RoomMapSprites");
  lua_pushcfunction(L,RoomBackdropTrace);lua_setfield(L,-2,"RoomBackdropTrace");
  lua_pushcfunction(L,DescriptorPlacement);lua_setfield(L,-2,"DescriptorPlacement");
  lua_setglobal(L,"IsaacCSObservationNative"); lua_settop(L,top);
}
// Coordinator 在复制窗口内部调用一次；错误返回自有原因，不让 Lua 异常越过 Authority。
LuaEntityResult CaptureLuaEntity(std::uint32_t objects,std::uint32_t nodes,std::uint32_t depth,std::uint32_t limit,unsigned domain) noexcept {
  LuaEntityResult result; auto *L=State(); if(!L) { result.reason="lua_state_unavailable"; return result; }
  const auto top=lua_gettop(L);
  try {
    const char *modules[]={"IsaacCSObservation","IsaacCSGridObservation","IsaacCSRoomObservation","IsaacCSLevelObservation","IsaacCSExplorationObservation","IsaacCSRoomSaveObservation","IsaacCSPlayerObservation","IsaacCSRunObservation","IsaacCSRandomObservation","IsaacCSPresentationObservation","IsaacCSModObservation"};
    if(domain>=11) { result.reason="unsupported_domain"; return result; }
    lua_getglobal(L,modules[domain]);
    if(!lua_istable(L,-1)) { lua_settop(L,top); result.reason="formal_observation_module_unavailable"; return result; }
    lua_getfield(L,-1,"Capture");
    lua_newtable(L);
    lua_pushinteger(L,objects); lua_setfield(L,-2,"objects");
    lua_pushinteger(L,nodes); lua_setfield(L,-2,"nodes");
    lua_pushinteger(L,depth); lua_setfield(L,-2,"depth");
    lua_pushinteger(L,limit); lua_setfield(L,-2,"bytes");
    lua_pushboolean(L,false);lua_setfield(L,-2,"stream_capture");
    lua_pushboolean(L,false);lua_setfield(L,-2,"stream_owned");
    lua_pushboolean(L,true);lua_setfield(L,-2,"stream_cold");
    // Lua分层与SourceTime共用modules的零基下标，不传一基Observation DomainId。
    lua_pushinteger(L,domain);lua_setfield(L,-2,"stream_domain");
    // Entity含全部身体/备份及内嵌资源；与Grid同用有界524288次有限查询，字节/节点另有上限。
    lua_pushinteger(L,domain<=1?524288:domain==5?262144:131072); lua_setfield(L,-2,"queries");
    if(!lua_isfunction(L,-2)||lua_pcall(L,1,7,0)!=LUA_OK) {
      const auto *error=lua_tostring(L,-1); result.reason=error?std::string(error).substr(0,256):"lua_capture_failed";
      lua_settop(L,top); return result;
    }
    std::size_t bytes=0; const auto *payload=lua_tolstring(L,-7,&bytes);
    if(payload&&bytes<=limit) {
      result.payload.assign(payload,bytes); result.available=true;
      result.complete=lua_toboolean(L,-6)!=0;
      const auto captured_objects=lua_tointeger(L,-5),captured_nodes=lua_tointeger(L,-4);
      if(captured_objects<0||captured_objects>objects||captured_nodes<0||captured_nodes>nodes) { result.complete=false; result.reason="lua_count_budget"; }
      else { result.objects=unsigned(captured_objects); result.nodes=unsigned(captured_nodes); }
      const auto *reason=lua_tostring(L,-3); if(reason&&*reason&&result.reason.empty()) result.reason=reason;
    } else result.reason="lua_payload_budget";
  } catch(...) { result.complete=false; result.reason="lua_owned_value_budget"; }
  lua_settop(L,top); return result;
}
}

namespace cs::game {
void FinishLuaObservation(unsigned domain,std::uint64_t scope,bool committed) noexcept {
  auto *L=State(); if(!L||domain<4||domain>10) return;
  const auto top=lua_gettop(L); const char *names[]={"IsaacCSExplorationObservation","IsaacCSRoomSaveObservation","IsaacCSPlayerObservation","IsaacCSRunObservation","IsaacCSRandomObservation","IsaacCSPresentationObservation","IsaacCSModObservation"};
  lua_getglobal(L,names[domain-4]);
  if(lua_istable(L,-1)) { lua_getfield(L,-1,"Outcome"); if(lua_isfunction(L,-1)) {
    const auto s=std::to_string(scope); lua_pushlstring(L,s.data(),s.size()); lua_pushboolean(L,committed); lua_pcall(L,2,0,0);
  } }
  lua_settop(L,top);
}
}
