#include "game/client_entity_e2e.h"
#include "game/context.h"
#include "game/bridge.h"
#include "runtime.h"
#include <array>
#include <cstring>
#include <map>
namespace cs::game {
namespace {
// 此状态仅主线程访问；不保存原生地址、不改变类别owner或回收策略。
constexpr std::array<const char *,16> points={"acquire-before","acquire-after","init-before","init-after",
  "fields-before","fields-after","resources-before","resources-after","publish-before","publish-after",
  "references-before","references-after","check-before","check-after","draw-before","draw-after"};
const char *armed=nullptr,*hit=nullptr;unsigned hits=0;
// 测量只保存累计值和单次峰值，不写逐tick日志，也不持有任何原生借用。
struct Time {std::uint64_t count=0,total=0,maximum=0;};
std::map<std::string,Time> times;
bool Token(const std::string &token){return IsMain()&&TopOwner()==Owner::Manager&&!Settings.world_e2e_token.empty()&&token==Settings.world_e2e_token;}
}
std::string ClientEntityTestArm(const std::string &token,const std::string &name) noexcept {
  if(!Token(token))return "ok=0\nerror=entity_fault_context\n";
  if(name=="clear"){armed=hit=nullptr;hits=0;return "ok=1\ncleared=1\n";}
  if(armed)return "ok=0\nerror=entity_fault_already_armed\n";
  for(const auto *p:points)if(name==p){armed=p;hit=nullptr;hits=0;return "ok=1\narmed="+name+"\n";}
  return "ok=0\nerror=entity_fault_unknown_point\n";
}
bool ClientEntityTestFault(const char *point) noexcept {
  if(!IsMain()||!ClientBusy()||!armed||std::strcmp(point,armed))return false;
  hit=armed;armed=nullptr;++hits;return true;
}
std::string ClientEntityTestStatus(const std::string &token) noexcept {
  if(!Token(token))return "ok=0\nerror=entity_fault_token\n";
  auto out="ok=1\narmed="+std::string(armed?armed:"")+"\nhit="+std::string(hit?hit:"")+"\nhits="+std::to_string(hits)+"\n";
  for(const auto &p:times){const auto prefix="timing."+p.first;out+=prefix+".count="+std::to_string(p.second.count)+"\n"+prefix+".total_ns="+std::to_string(p.second.total)+"\n"+prefix+".max_ns="+std::to_string(p.second.maximum)+"\n";}
  return out;
}
void ClientEntityTestTime(const char *name,std::uint64_t ns) noexcept {
  if(!IsMain()||Settings.world_e2e_token.empty())return;
  try{auto &t=times[name];++t.count;t.total+=ns;if(ns>t.maximum)t.maximum=ns;}catch(...){}
}
}
