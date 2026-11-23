#pragma once
#include <cstdint>
#include <string>
namespace cs::game {
// 固定纯值计数贯穿真实连接至安全退休；不持有对象、VM或渲染context。
enum class ClientIsolationKind : unsigned {
  Begin, NativeCall, NativeReturn, ExpectedSkip, RejectedEntry, LuaSkip, OwnLua,
  NativePersistence, LuaPersistence, RandomRejected, Fault, Recover, Stop, Retired,
  ObservationLost, ModInstall, Count
};
// 当前主线程场景的纯值身份快照；零值保留尚未分配/外线程未知的事实。
struct ClientIsolationIdentity final {
 std::uint64_t local_connection=0,connection=0,route=0,scene=0,instance=0,resource=0,lua=0,context=0;
 unsigned phase=0;
};
ClientIsolationIdentity ClientIsolationCurrent() noexcept;
// Host/Off保持透传；诊断失效不改变正式输入、提交或ACK。
void ClientIsolationObserve(ClientIsolationKind,const char *entry) noexcept;
std::string ClientIsolationFields();
} // namespace cs::game
