#include "game/client/player/client_player_control.h"
namespace cs::game {
bool ClientPlayerControlSame(const ClientPlayerControlView &a,const ClientPlayerControlView &b) noexcept {
  return a.present==b.present&&(!a.present||(a.participant==b.participant&&a.group==b.group&&a.ordinal==b.ordinal&&a.control_session==b.control_session&&a.binding_revision==b.binding_revision));
}
}
