#pragma once
#include <cstddef>
#include <cstdint>
namespace cs::game {
// 仅主线程当前原生调用栈持有的用途帧；不保存原生对象或授予额外随机能力。
enum class ClientPlayerOperation : unsigned { Construct,BaseInit,Install,Relations,Draw,Retire };
class ClientPlayerIsolationScope final {
public:
  explicit ClientPlayerIsolationScope(ClientPlayerOperation) noexcept;
  ~ClientPlayerIsolationScope();
  ClientPlayerIsolationScope(const ClientPlayerIsolationScope &)=delete;
  ClientPlayerIsolationScope &operator=(const ClientPlayerIsolationScope &)=delete;
  bool Ready() const noexcept {return active_;}
private:
  ClientPlayerIsolationScope *previous_=nullptr;
  ClientPlayerOperation operation_=ClientPlayerOperation::Install;
  bool active_=false;
};
// true为主动拒绝：不进入随机original/观测完成计数，也不先消费随机再恢复其状态。
bool ClientPlayerRandomDenied() noexcept;
bool ClientPlayerIsolationInstalled() noexcept;
std::uint64_t ClientPlayerRandomDenials() noexcept;
// 当前Scene只在应用/已提交展示阶段允许新借用；撤展示即刻关闭，不等到逐对象清理。
bool ClientPlayerBorrowAllowed() noexcept;
} // namespace cs::game
