#include "IsaacRepentance.h"
#include "game/client/room/client_backdrop_native.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include "LuaCore.h"
#include <cstring>
#include <cmath>

namespace cs::game {
namespace {
constexpr unsigned MaxSteps=4096;
// 只关联当前真实Room；每次原生墙面重建替换前次轨迹，内存与原生构建次数无关。
struct SourceTrace final {
  void *room=nullptr,*backdrop=nullptr;
  unsigned id=0,seed=0;
  bool active=false,complete=false,valid=false;
  ClientBackdropWallTrace value{};
} source;
// Client构图的调用前缀只挂在当前主线程栈，既不采Host轨迹也不复现Spawn。
struct LocalBuild final {
  void *backdrop=nullptr;
  bool started=false,finished=false,wall=false,valid=false;
} local;
bool Current(void *value) noexcept {return IsMain()&&g_Game&&g_Game->_room&&value==&g_Game->_room->_backdrop;}
unsigned &Seed(void *value) noexcept {return static_cast<Backdrop *>(value)->rng._seed;}
void Fault(const char *reason) noexcept {if(IsMain())local.valid=false;ClientCallbackFault(reason);}
void Number(lua_State *L,const char *key,unsigned value){lua_pushinteger(L,lua_Integer(value));lua_setfield(L,-2,key);}
}
ClientBackdropBuildScope::ClientBackdropBuildScope(void *value) noexcept:backdrop_(value){
  if(ClientBusy()&&Current(value)&&local.backdrop==value&&local.valid&&!local.started){
    local.started=true;return;
  }
  if(ClientBusy()){
    allowed_=false;Fault("backdrop_init_build_context");return;
  }
  source_=Current(value)&&!ClientBusy();
  if(source_){initial_=Seed(value);source={};source.room=g_Game->_room;source.backdrop=value;}
}
void ClientBackdropBuildScope::Returned() noexcept {
  returned_=true;
  if(Current(backdrop_)&&local.backdrop==backdrop_&&local.valid){local.finished=!local.wall;return;}
  if(!source_)return;
  const auto &b=*static_cast<Backdrop *>(backdrop_);source.id=b.backdropId;source.seed=b.seed;
  source.value.initial=initial_;
  // 只有真实Init正常返回且未调用墙面构建，才记录不适用；缓存完成后gfx可卸载，不能拿它推断。
  if(!source.value.present){source.value.present=true;source.value.applicable=false;source.complete=source.valid=true;}
}
ClientBackdropBuildScope::~ClientBackdropBuildScope(){if(source_&&!returned_)source.complete=source.valid=false;}
ClientBackdropWallScope::ClientBackdropWallScope(void *value) noexcept:backdrop_(value) {
  if(!Current(value)){if(ClientBusy()){allowed_=false;Fault("backdrop_wall_owner");}return;}
  if(ClientBusy()){
    client_=true;
    if(!local.valid||local.backdrop!=value||!local.started||local.finished||local.wall){
      allowed_=false;Fault("backdrop_wall_build_context");return;
    }
    local.wall=true;
    // 917420先完成h/v层随机选择和绘制，最后仅用该计数枚举*null标记生成121。
    // null标记不绘制；同步构建期间借用空枚举，离栈归还真实ANM2计数与资源所有权。
    auto &sprite=static_cast<Backdrop *>(value)->_wallDetailANM2;
    null_count_=sprite._numNullLayers;sprite._numNullLayers=0;nulls_borrowed_=true;
  }else{
    source_=true;
    source={};source.room=g_Game->_room;source.backdrop=value;
    const auto &b=*static_cast<Backdrop *>(value);source.id=b.backdropId;source.seed=b.seed;
    source.active=source.valid=true;source.value.present=source.value.applicable=true;source.value.before=b.rng._seed;
  }
}
void ClientBackdropWallScope::Returned() noexcept {
  returned_=true;
  if(source_){source.active=false;source.complete=source.valid;source.value.after=Seed(backdrop_);}
  if(client_)local.wall=false;
}
ClientBackdropWallScope::~ClientBackdropWallScope(){
  if(nulls_borrowed_)static_cast<Backdrop *>(backdrop_)->_wallDetailANM2._numNullLayers=null_count_;
  if(!returned_&&source_){source.active=source.complete=source.valid=false;}
  if(!returned_&&client_)Fault("backdrop_wall_build_exception");
}
ClientBackdropEffectScope::ClientBackdropEffectScope(void *value,int a,int b,unsigned kind,int part,const void *quad,unsigned char flip_x,bool flip_y) noexcept:backdrop_(value){
  client_=ClientBusy();
  if(!Current(value)||!quad){if(client_)Fault("backdrop_effect_context");return;}
  // Client的装饰实体已从Host成员集合安装。此helper只记录被隔离的调用。
  if(client_){
    if(!local.valid||local.backdrop!=value||!local.wall||
       ClientDispatchBefore(ClientEntry::BackdropEffects)!=ClientEntryDecision::ExpectedSkip)Fault("backdrop_local_effect_context");
    return;
  }
  step_.arguments={unsigned(a),unsigned(b),kind,unsigned(part)};std::memcpy(step_.quad.data(),quad,32);
  step_.before=Seed(value);step_.flip_x=flip_x!=0;step_.flip_y=flip_y;
  for(float f:step_.quad)if(!std::isfinite(f)||std::fabs(f)>1000000){source.valid=false;return;}
  source_=source.active&&source.valid&&source.backdrop==value;
}
void ClientBackdropEffectScope::Returned() noexcept {
  returned_=true;if(!source_)return;
  if(source.value.steps.size()>=MaxSteps){source.valid=false;return;}
  step_.after=Seed(backdrop_);
  try{source.value.steps.push_back(step_);}catch(...){source.valid=false;}
}
ClientBackdropEffectScope::~ClientBackdropEffectScope(){if(source_&&!returned_)source.valid=false;}
ClientBackdropLocalBuild::ClientBackdropLocalBuild(void *value) noexcept {
  if(!Current(value)||!ClientBusy()||local.backdrop)return;
  local={value,false,false,false,true};
  active_=true;
}
ClientBackdropLocalBuild::~ClientBackdropLocalBuild(){if(active_)local={};}
bool ClientBackdropLocalBuild::Complete() const noexcept {return active_&&local.valid&&local.finished;}
// 两个原生variant7分支已经完成位置/距离判断；命中后唯一背景随机消费用于新实体scale。
// 保持这一次本地独立流推进，略去全局seed、Spawn、配置和Entity.Update。
bool ClientBackdropSkipDirectEffect(void *value) noexcept {
  if(!ClientBusy())return false;
  if(!Current(value)||!local.valid||local.backdrop!=value||!local.wall||
     ClientDispatchBefore(ClientEntry::BackdropEffects)!=ClientEntryDecision::ExpectedSkip){Fault("backdrop_direct_effect_context");return true;}
  auto &r=static_cast<Backdrop *>(value)->rng;
  if(!r._seed||!r._shift1||r._shift1>31||!r._shift2||r._shift2>31||!r._shift3||r._shift3>31){Fault("backdrop_direct_effect_rng");return true;}
  r._seed^=r._seed>>r._shift1;r._seed^=r._seed<<r._shift2;r._seed^=r._seed>>r._shift3;return true;
}
int ClientBackdropWallTraceLua(void *state){
  auto *L=static_cast<lua_State *>(state);
  if(!IsMain()||ClientBusy()||!g_Game||!g_Game->_room){lua_pushnil(L);return 1;}
  const auto &b=g_Game->_room->_backdrop;
  const bool captured=source.complete&&source.valid&&!source.active&&source.room==g_Game->_room&&source.backdrop==&b&&source.id==b.backdropId&&source.seed==b.seed;
  // Crawlspace原生Room绘制不消费普通Backdrop；其它分支必须有正常构建见证。
  if(!captured&&g_Game->_room->_roomType!=16){
    lua_pushnil(L);return 1;
  }
  const bool applicable=captured&&source.value.applicable;
  if(applicable&&!source.value.initial){lua_pushnil(L);return 1;}
  lua_createtable(L,0,5);Number(L,"version",1);lua_pushboolean(L,applicable);lua_setfield(L,-2,"applicable");
  Number(L,"initial",captured?source.value.initial:0);
  if(!applicable)return 1;
  Number(L,"before",source.value.before);Number(L,"after",source.value.after);
  lua_createtable(L,int(source.value.steps.size()),0);
  unsigned index=0;for(const auto &s:source.value.steps){lua_createtable(L,0,6);Number(L,"before",s.before);Number(L,"after",s.after);
    lua_createtable(L,4,0);for(unsigned i=0;i<4;++i){lua_pushinteger(L,lua_Integer(s.arguments[i]));lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"arguments");
    lua_createtable(L,8,0);for(unsigned i=0;i<8;++i){lua_pushnumber(L,s.quad[i]);lua_rawseti(L,-2,i+1);}lua_setfield(L,-2,"quad");
    lua_pushboolean(L,s.flip_x);lua_setfield(L,-2,"flip_x");lua_pushboolean(L,s.flip_y);lua_setfield(L,-2,"flip_y");lua_rawseti(L,-2,++index);
  }lua_setfield(L,-2,"steps");return 1;
}
}
