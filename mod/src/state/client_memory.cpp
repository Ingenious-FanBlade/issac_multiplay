#include "state/client_memory.h"
#include <atomic>

namespace cs::state {
namespace {std::atomic_flag client_workspace=ATOMIC_FLAG_INIT;}
ClientStateWorkspace::ClientStateWorkspace() {
  if(client_workspace.test_and_set(std::memory_order_acquire))throw ClientTableError(ClientTableFailure::WorkspaceBusy,"client_state_workspace_busy");
}
ClientStateWorkspace::~ClientStateWorkspace() {client_workspace.clear(std::memory_order_release);}
ClientStateMemory::ClientStateMemory() {
  const auto &policy=StateBudgets();limits_={policy.peer_value_bytes,policy.peer_wire_bytes,policy.workspace_job_bytes};
  // 账户及三份既有Charge的管理对象冷分配保守预留；容器及其控制块另外按实际分配收费。
  charges_={memory::Reserve(memory::Pool::Values,sizeof(*this)+1024,"client_state_values"),
      memory::Reserve(memory::Pool::Wire,0,"client_state_wire"),memory::Reserve(memory::Pool::Workspace,0,"client_state_workspace")};
}
std::shared_ptr<ClientStateMemory> ClientStateMemory::Create() {
  try {return std::shared_ptr<ClientStateMemory>(new ClientStateMemory);}
  catch(const std::bad_alloc &) {throw ClientTableError(ClientTableFailure::Budget,"client_memory_allocation");}
  catch(const std::runtime_error &) {throw ClientTableError(ClientTableFailure::Budget,"client_memory_pool");}
}
void *ClientStateMemory::Allocate(memory::Pool pool,std::size_t bytes,std::size_t alignment) {
  std::lock_guard lock(mutex_);const auto index=unsigned(pool);
  if(index>=charges_.size() || !alignment || (alignment&(alignment-1)))
    throw ClientTableError(ClientTableFailure::Internal,"client_allocation_arguments");
  auto &charge=*charges_[index];const auto before=charge.Bytes();
  const auto available=before<=limits_[index]?limits_[index]-before:0;
  if(before>limits_[index] || bytes>available)throw ClientTableError(ClientTableFailure::Budget,"client_peer_quota",bytes,available,pool);
  try {charge.Resize(before+bytes);}
  catch(const std::runtime_error &) {const auto shared=memory::ReplicationBudget()->Snapshot()[index];
    throw ClientTableError(ClientTableFailure::Budget,"client_shared_pool",bytes,shared.limit-shared.current,pool);}
  try {
    if(alignment>__STDCPP_DEFAULT_NEW_ALIGNMENT__)return ::operator new(bytes,std::align_val_t(alignment));
    return ::operator new(bytes);
  } catch(const std::bad_alloc &) {
    charge.Resize(before);throw ClientTableError(ClientTableFailure::Budget,"client_native_allocation",bytes,available,pool);
  }
}
void ClientStateMemory::Deallocate(memory::Pool pool,void *value,std::size_t bytes,std::size_t alignment) noexcept {
  if(alignment>__STDCPP_DEFAULT_NEW_ALIGNMENT__)::operator delete(value,std::align_val_t(alignment));else ::operator delete(value);
  std::lock_guard lock(mutex_);auto &charge=*charges_[unsigned(pool)];charge.Resize(charge.Bytes()-bytes);
}
std::array<std::uint64_t,3> ClientStateMemory::Usage() const noexcept {
  std::lock_guard lock(mutex_);return {charges_[0]->Bytes(),charges_[1]->Bytes(),charges_[2]->Bytes()};
}
} // namespace cs::state
