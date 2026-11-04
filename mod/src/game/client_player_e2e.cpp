#include "game/client_player_e2e.h"
#include "game/context.h"
#include "game/bridge.h"
#include "runtime.h"
#include <cstring>
#ifndef ISAACCS_E2E
#error Player fault tools require the explicit E2E target
#endif
namespace cs::game {
namespace {
constexpr const char *points[]={"player-allocate-before","player-allocate-after","player-ctor-before","player-ctor-after",
  "player-init-before","player-init-after","player-fields-before","player-fields-after","player-resources-before","player-resources-after",
  "player-hud-before","player-hud-after","player-roster-before","player-roster-after","player-publish-before","player-publish-after",
  "player-weapon-before","player-weapon-after","player-relations-before","player-relations-after","player-check-before","player-check-after",
  "player-draw-before","player-draw-after"};
// 整个记录只有纯值；一次命中即撤arm，完整T安全退休后的恢复不会再次触发同一故障。
struct Fault final {const char *armed=nullptr,*hit=nullptr;ClientObjectKey key{};std::uint64_t instance=0,child_generation=0;unsigned prefix=0,slot=0,hits=0;} fault;
bool Token(const std::string &token){return IsMain()&&TopOwner()==Owner::Manager&&!Settings.world_e2e_token.empty()&&token==Settings.world_e2e_token;}
}
std::string ClientPlayerTestArm(const std::string &token,const std::string &name) noexcept {
  if(!Token(token))return "ok=0\nerror=player_fault_context\n";
  if(name=="clear"){fault={};return "ok=1\ncleared=1\n";}
  if(fault.armed)return "ok=0\nerror=player_fault_already_armed\n";
  try{for(const auto *p:points)if(name==p){auto response="ok=1\narmed="+name+"\n";fault={};fault.armed=p;return response;}}
  catch(...){return "ok=0\nerror=player_fault_diagnostic_allocation\n";}
  return "ok=0\nerror=player_fault_unknown_point\n";
}
bool ClientPlayerTestFault(const char *point,ClientObjectKey key,std::uint64_t instance,unsigned prefix,unsigned slot,std::uint64_t generation) noexcept {
  if(!IsMain()||!ClientBusy()||Settings.world_e2e_token.empty()||!fault.armed||std::strcmp(point,fault.armed))return false;
  fault.hit=fault.armed;fault.armed=nullptr;fault.key=key;fault.instance=instance;fault.prefix=prefix;fault.slot=slot;fault.child_generation=generation;++fault.hits;return true;
}
std::string ClientPlayerTestStatus(const std::string &token) noexcept {
  if(!Token(token))return "ok=0\nerror=player_fault_context\n";
  try{return "ok=1\narmed="+std::string(fault.armed?fault.armed:"")+"\nhit="+std::string(fault.hit?fault.hit:"")+"\nhits="+std::to_string(fault.hits)+
    "\nkey_run="+std::to_string(fault.key.run)+"\nkey_space="+std::to_string(fault.key.space)+"\nkey_id="+std::to_string(fault.key.id)+
    "\ninstance="+std::to_string(fault.instance)+"\nprefix="+std::to_string(fault.prefix)+"\nslot="+std::to_string(fault.slot)+
    "\nchild_generation="+std::to_string(fault.child_generation)+"\nprefix_kind="+(fault.hit&&std::strstr(fault.hit,"draw")?"draw-pass":fault.hit&&std::strstr(fault.hit,"weapon")?"resource-stage-entered":"body-mask")+"\n";}
  catch(...){return "ok=0\nerror=player_fault_diagnostic_allocation\n";}
}
}
