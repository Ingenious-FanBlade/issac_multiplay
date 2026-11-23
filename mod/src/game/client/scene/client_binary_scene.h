#pragma once
#include "game/client/player/client_player_control.h"
#include "net/mod_intent.h"
#include "state/wire_protocol.h"
#include <memory>
#include <string>

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
  void Close(bool preserve_room=false,bool preserve_mod=false) noexcept;
  void Pump() noexcept;
  void InstallMods() noexcept;
  bool ModIntentWorld(net::ModIntent &) const noexcept;
  std::uint16_t ModIntentMember(const void *) const noexcept;
  void Recover(state::StateResyncReason) noexcept;
  bool Drained() const noexcept;
  ClientPlayerControlView Control(unsigned ordinal) const noexcept;
  std::string InstallTimingFields() const;
  std::string Diagnostics(unsigned page)const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
