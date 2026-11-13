#pragma once
#include "game/client/scene/client_atomic_install.h"

namespace cs::game {
std::unique_ptr<ClientAtomicInstallBindings> ClientMakeAtomicInstallBindings(
    const std::shared_ptr<state::ClientStateMemory> &,ClientSceneMainPreparation &);
// 仅当前主线程窗口借用已提交完整状态。显示专用结果仍由其具名消费者安装。
const ClientSceneInstallPlan *ClientBinaryPublishedWorld() noexcept;
} // namespace cs::game
