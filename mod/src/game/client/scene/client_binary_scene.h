#pragma once
#include "game/client/player/client_player_control.h"
#include "state/wire_protocol.h"
#include <memory>

namespace cs::net::live {class Client;}
namespace cs::state {class ClientStatePreparation;class ClientStateMemory;}
namespace cs::game {
class ClientScene;
// Manager 独占资源邮箱、应用协调器与已发布批次的原生租约。
class ClientBinaryScene final {
public:
  ClientBinaryScene(ClientScene &,net::live::Client &,std::shared_ptr<state::ClientStateMemory>);
  ~ClientBinaryScene();
  static std::shared_ptr<const state::ClientStatePreparation> Preparation();
  bool Open(const state::StateRoute &) noexcept;
  void Close(bool preserve_room=false) noexcept;
  void Pump() noexcept;
  void Recover(state::StateResyncReason) noexcept;
  bool Drained() const noexcept;
  ClientPlayerControlView Control(unsigned ordinal) const noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
