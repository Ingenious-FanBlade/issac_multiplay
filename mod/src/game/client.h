#pragma once
#include "game/bridge.h"
#include "replica/model.h"
#include "game/client_entity_events.h"
#include <string>
#include <memory>

namespace cs::game {
struct ClientPlayerDisplayView;
// Client角色独立于Authority SessionRuntime，socket失败也不自动交还原生模拟。
bool ClientBusy() noexcept;
const char *ClientStateName() noexcept;
const char *StartClient(const std::string &ipv4, unsigned port, int device_key) noexcept;
// E2E 只提供有明确来源的纯值目标，复用正式 Client 场景，不启动网络或本地模拟。
const char *ClientTestBegin(const std::string &token, const replica::Scope &scope) noexcept;
bool ClientTestCancelAfter(const std::string &token,unsigned stage) noexcept;
std::string ClientTestExerciseOwnership(const std::string &token) noexcept;
std::string ClientTestExerciseVm(const std::string &token) noexcept;
bool ClientAdmitTarget(const replica::Scope &scope, std::uint64_t revision,
                       const replica::Value &value) noexcept;
// 正式HUD/角色展示的纯值入口；只在主线程取当前Scene A，旧快照不得继续冒作当前展示。
std::shared_ptr<const ClientPlayerDisplayView> ClientPlayerDisplay() noexcept;
bool ClientPlayerDisplayCurrent(const std::shared_ptr<const ClientPlayerDisplayView> &) noexcept;
// 交给27的独立表现记录；这里只接纳，不重放Grid玩法或宣称已呈现。
bool ClientAdmitGridIndependent(const replica::Message &,const char *&reason) noexcept;
using ClientGridIndependentVisitor=bool (*)(void *,const replica::Value &,std::uint64_t) noexcept;
bool ClientVisitGridIndependent(void *,ClientGridIndependentVisitor) noexcept;
bool ClientCompleteGridIndependent(std::uint64_t sequence,std::uint64_t source_scope,const char *phase) noexcept;
// Entity历史/瞬时结果单独交给27；接纳与真实呈现各自显式完成，不发送事件ACK。
bool ClientAdmitEntityIndependent(const replica::Message &,const char *&reason) noexcept;
bool ClientVisitEntityIndependent(void *,ClientEntityEvents::Visitor) noexcept;
bool ClientCompleteEntityIndependent(replica::Kind,std::uint64_t,std::uint64_t,const char *) noexcept;
void StopClient() noexcept;
std::string ClientFields();
void ClientUiHeartbeat() noexcept;
// 仅在最外层Manager/受控Input owner执行；采样失败保持中性，绝不建立本地Player绑定。
bool ClientDeviceReturned(void *device, bool success, unsigned polls, bool poll_ok);
void ClientInputFinished() noexcept;
} // namespace cs::game
