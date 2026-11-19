#pragma once
#ifdef ISAACCS_E2E
#include "state/frame_events.h"
namespace cs::state {struct HostStateSnapshot;struct CapturePacket;struct ClientStateBlockTable;}
namespace cs::game {
class ClientSceneMainPreparation;
void E09Enable(bool enabled) noexcept;
void E09Host(const state::HostStateSnapshot &,const state::CapturePacket &) noexcept;
void E09Client(const state::ClientStateBlockTable &,const ClientSceneMainPreparation &,unsigned streams) noexcept;
void E09ClientRender(const state::ClientStateBlockTable &) noexcept;
}
#endif
