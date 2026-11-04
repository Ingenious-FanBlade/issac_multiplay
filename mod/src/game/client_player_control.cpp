#include "game/client_player_control.h"
namespace cs::game {
ClientPlayerControlView ClientPlayerControl(const replica::PlayerTarget &target,unsigned ordinal) noexcept {
  ClientPlayerControlView out;
  for(const auto &g:target.control_groups)if(g.ordinal==ordinal){
    out.participant=g.participant;out.group=g.key;out.ordinal=ordinal;out.revision=target.revision;
    out.control_session=g.control_session;out.binding_revision=g.binding_revision;out.bodies=unsigned(g.members.size());out.present=true;
    for(const auto &b:target.bodies)if(b.membership.known&&b.membership.group.key==g.key){out.active+=b.membership.active;out.consumers+=b.membership.can_consume;}
    break;
  }return out;
}
bool ClientPlayerControlSame(const ClientPlayerControlView &a,const ClientPlayerControlView &b) noexcept {
  return a.present==b.present&&(!a.present||(a.participant==b.participant&&a.group==b.group&&a.ordinal==b.ordinal&&a.control_session==b.control_session&&a.binding_revision==b.binding_revision));
}
}
