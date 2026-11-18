#pragma once
#include <cstddef>
#include <memory>

namespace cs::state {class ClientStateMemory;}
namespace cs::game {
// C++17 SDK 边界只传账户所有权；具体计费仍由同一 Peer 的 ClientStateMemory 执行。
using ClientDisplayMemoryRef=std::shared_ptr<state::ClientStateMemory>;
void ClientDisplayCharge(const ClientDisplayMemoryRef &,std::size_t);
void ClientDisplayUncharge(const ClientDisplayMemoryRef &,std::size_t) noexcept;
bool InstallClientDisplayMemoryHooks() noexcept;
bool ClientDisplayMemoryHooksReady() noexcept;

// 仅归属本线程受控显示调用的新分配。分配记录独立存活到原生 free，不能随作用域退账。
class ClientDisplayMemoryScope final {
public:
  explicit ClientDisplayMemoryScope(const ClientDisplayMemoryRef &);
  ~ClientDisplayMemoryScope();
  ClientDisplayMemoryScope(const ClientDisplayMemoryScope &)=delete;
  ClientDisplayMemoryScope &operator=(const ClientDisplayMemoryScope &)=delete;
  const ClientDisplayMemoryRef &Memory() const noexcept{return memory_;}
  void Reject() noexcept{rejected_=true;}
  void Complete() const;
private:
  ClientDisplayMemoryRef memory_;
  ClientDisplayMemoryScope *previous_=nullptr;
  bool rejected_=false;
};
} // namespace cs::game
