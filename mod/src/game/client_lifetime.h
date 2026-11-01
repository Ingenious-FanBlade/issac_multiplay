#pragma once
#include <cstdint>
namespace cs::game {
// SDK 桥与核心共用的纯值资源身份；原生指针只在主线程 C++ owner 内。
enum class ClientRetainedKind : unsigned { Native, LuaRegistry, Graphics };
enum class ClientReleasePhase : unsigned { Manager, LuaState, RenderContext, ManagerContext };
struct ClientRetainedTicket final {std::uint64_t id=0, scene=0, instance=0;};
using ClientRetainedRelease=bool (*)(void *) noexcept;
// 只报告已采用 ImageManager 普通/透明 CPU raw 队列，绝不表示 GPU 完成。
bool ClientNativeCpuQueuesEmpty() noexcept;
// 原生资源候选先登记，再开始构造；退休在主线程 context 且所有借用退出后执行。
bool ClientRetainGraphics(void *owner,ClientRetainedRelease release,ClientRetainedTicket &) noexcept;
void ClientRetireGraphics(const ClientRetainedTicket &) noexcept;
// 真实 Lua registry 借用由 SDK-facing 桥生成，成功后必须以票据退休。
bool ClientLuaRegistryCapture(int stack_index,ClientRetainedTicket &ticket) noexcept;
// visitor 在受保护 Lua 调用内执行；registry 值位于当前 Lua 栈顶部，栈底另有桥的 lightuserdata 参数。
// 不得跨本次调用保存 Lua／原生裸指针；visitor 内有析构职责的 C++ 局部对象不得被 Lua longjmp 跨越。
bool ClientLuaRegistryVisit(const ClientRetainedTicket &ticket,
                            bool (*visitor)(void *lua_state,void *context) noexcept,
                            void *context) noexcept;
void ClientLuaRegistryRetire(const ClientRetainedTicket &ticket) noexcept;
// CaptureRaw／VisitRaw 只供核心已持主线程 Lua 借用时调用；ReleaseRaw 仅由核心在
// 所有借用退尽、VM 与释放阶段仍有效时调用，不暴露 SDK 类或 Lua 对象给核心。
void *ClientLuaRegistryMakeOwner() noexcept;
void ClientLuaRegistryDiscardOwner(void *owner) noexcept;
bool ClientLuaRegistryCaptureRaw(void *owner,int stack_index) noexcept;
bool ClientLuaRegistryVisitRaw(void *owner,
                                bool (*visitor)(void *,void *) noexcept,
                                void *context) noexcept;
bool ClientLuaRegistryReleaseRaw(void *owner) noexcept;
} // namespace cs::game
