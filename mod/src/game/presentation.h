#pragma once
#include <cstdint>
#include <string>

namespace cs::game {
// Client独立场景服务。所有参数为自有值；绝不把网络DTO直接写入原生Game/Room。
// Scene包含Room几何、Camera、HUD及分pass复合节点；各域adapter由18—20提供。
bool PresentationBegin(std::uint64_t connection) noexcept;
const char *PresentationSubmit(const std::string &descriptor) noexcept;
void PresentationStop() noexcept;
bool PresentationDrained() noexcept;
bool PresentationProgressUnchanged() noexcept;
// 只由核定的图形owner调用；未通过OS context检查时不触碰资源。
void PresentationDraw() noexcept;
void PresentationRenderReturned() noexcept;
void PresentationQueueReturned(bool reset) noexcept;
std::string PresentationFields();
// 失败状态留在Client角色内；主线程在Manager边界读取，不从图形回调切换角色。
const char *PresentationFailure() noexcept;
// 仅显式E2E配置开放的真实进程夹具；读取本次测试目录内有界纯值描述。
std::string PresentationTest(const std::string &token,const std::string &name);
} // namespace cs::game
