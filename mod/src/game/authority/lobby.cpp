#include "game/authority/lobby.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "game/client/session/client.h"
#include "ops/world_commands.h"
#include "runtime.h"
#include <bit>

namespace cs::game {
namespace {
using core::LobbyPhase;

const char *PhaseName(LobbyPhase phase) noexcept {
  switch (phase) {
  case LobbyPhase::Waiting: return "Waiting";
  case LobbyPhase::Starting: return "Starting";
  case LobbyPhase::Playing: return "Playing";
  default: return "Off";
  }
}
}

void RefreshHostLobby(SessionRuntime &r) noexcept {
  auto &lobby = r.lobby;
  auto &roster = lobby.roster;
  if (roster.phase == LobbyPhase::Off || r.retiring) return;
  roster.present = 1;
  roster.ready = 0;
  for (unsigned i = 1; i < core::kMaximumParticipants; ++i) {
    const auto &remote = r.Remote(i);
    if (!remote.peer.established || remote.retiring || remote.retired) continue;
    roster.present |= 1u << i;
    if (roster.phase == LobbyPhase::Waiting) {
      const auto selection = remote.peer.observed.lobby_selection;
      roster.characters[i] = selection & 63;
      if (selection & core::kLobbyReady) roster.ready |= 1u << i;
    } else roster.ready |= 1u << i;
  }
  if (roster.phase == LobbyPhase::Waiting && RunActive()) {
    FailRuntime("lobby_requires_start_button");
  } else if (roster.phase == LobbyPhase::Starting) {
    if (!lobby.native_start_requested && RunActive()) FailRuntime("lobby_requires_start_button");
    else if (lobby.native_start_requested && r.run_observed && RunActive()) roster.phase = LobbyPhase::Playing;
    else if (GetTickCount64() - lobby.started_at >= 30000)
      FailRuntime(lobby.native_start_requested ? "lobby_start_timeout" : "lobby_capture_prepare_timeout");
  }
}

const char *SelectLobbyCharacter(unsigned character) noexcept {
  if (!core::LobbyCharacter(character)) return "lobby_character";
  if (ClientBusy()) return UpdateClientLobby(int(character), false);
  auto *r = Runtime();
  if (!r || r->retiring || r->lobby.roster.phase != LobbyPhase::Waiting) return "lobby_not_open";
  r->lobby.roster.characters[0] = character;
  return "";
}

const char *SetLobbyReady(bool ready) noexcept {
  return UpdateClientLobby(-1, ready);
}

const char *StartLobbyGame() noexcept {
  auto *r = Runtime();
  if (!r || r->retiring || r->lobby.roster.phase != LobbyPhase::Waiting) return "lobby_not_open";
  auto &lobby = r->lobby;
  if (!lobby.roster.CanStart()) return "lobby_wait_for_ready";
  if (!LobbyMenuReady()) return "lobby_requires_save_menu";

  // 先冻结名单与角色，再请求原生加载。加载期间的迟到握手不加入本局。
  lobby.locked_members = lobby.roster.present;
  for (unsigned i = 1; i < core::kMaximumParticipants; ++i)
    if (lobby.locked_members & (1u << i)) r->Remote(i).join.character = int(lobby.roster.characters[i]);
  r->network.CloseLobby();
  lobby.roster.phase = LobbyPhase::Starting;
  lobby.started_at = GetTickCount64();
  return "";
}

void PumpLobbyStart() noexcept {
  auto *r = Runtime();
  if (!r || r->retiring || r->finished || r->lobby.roster.phase != LobbyPhase::Starting ||
      r->lobby.native_start_requested) return;

  // 当前 Manager 的采集入口必须先参与。否则原生开局的出生和音频事件可能先于采集。
  const char *error = nullptr;
  if (!WorldLiveStartReady(LiveCaptureSession(), error)) {
    if (error) FailRuntime(error);
    return;
  }
  auto &lobby = r->lobby;
  lobby.native_start_requested = true;
  lobby.started_at = GetTickCount64();
  if (!RequestLobbyNativeStart(lobby.roster.characters[0])) {
    FailRuntime("lobby_native_start_failed");
    return;
  }
  Trace("session", "event=lobby_native_start generation=%llu members=%u character=%u loop=%llu",
        SessionGeneration(*r), lobby.locked_members, lobby.roster.characters[0], r->loop);
}

std::string LobbyFields() {
  core::LobbyRoster roster;
  unsigned ordinal = 0, desired = 0;
  std::string role = "Off", details;
  const auto *r = Runtime();
  if (ClientBusy()) {
    role = "Client";
    roster = ClientLobbyRoster(ordinal, desired, details);
  } else if (r && !r->finished && r->lobby.roster.phase != LobbyPhase::Off) {
    role = "Host";
    roster = r->lobby.roster;
    desired = roster.characters[0];
    const auto *close_error = !r->cleanup_failed ? "" :
        r->peer.stop_failed ? "host_network_stop_failed" : "host_native_cleanup_failed";
    details = "addresses=" + r->lobby.addresses + "\nport=" + std::to_string(r->lobby.port) +
              "\nretiring=" + std::to_string(r->retiring) + "\nstart_menu_ready=" +
              std::to_string(roster.phase == LobbyPhase::Waiting && LobbyMenuReady()) +
              "\nclose_error=" + close_error + "\nsession_error=" + r->last_error + "\n";
  } else if (r && r->lobby.roster.phase != LobbyPhase::Off && r->failure) {
    details = std::string("session_error=") + r->last_error + "\n";
  } else {
    details = "session_error=" + ClientLobbyFailure() + "\n";
  }
  std::string fields = "ok=1\nlobby_version=1\nrole=" + role + "\nphase=" + PhaseName(roster.phase) +
      "\nordinal=" + std::to_string(ordinal) + "\ncount=" + std::to_string(std::popcount(roster.present)) +
      "\ncan_start=" + std::to_string(roster.CanStart()) + "\ndesired_character=" +
      std::to_string(desired & 63) + "\ndesired_ready=" + std::to_string(bool(desired & core::kLobbyReady)) + "\n";
  for (unsigned i = 0; i < roster.characters.size(); ++i) {
    if (!(roster.present & (1u << i))) continue;
    const auto prefix = "member." + std::to_string(i) + ".";
    fields += prefix + "character=" + std::to_string(roster.characters[i]) + "\n" +
              prefix + "ready=" + std::to_string(bool(roster.ready & (1u << i))) + "\n";
  }
  return fields + details;
}
}
