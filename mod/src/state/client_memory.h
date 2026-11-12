#pragma once
#include "core/replication_memory.h"
#include "state/budget.h"
#include <new>
#include <limits>

namespace cs::state {
// 类型化本地失败只用于后台调度；不新增线上枚举，也不通过Gap.what()猜等待原因。
enum class ClientTableFailure : std::uint8_t {
  Budget, WorkspaceBusy, WrongThread, WrongRoute, InvalidEncoding, InvalidReference,
  EpochChain, EpochConflict, SnapshotLimit, RetiredIdentity, MemberReuse,
  MissingTable, MissingLocalResource, ResourceFailure, Busy, Generation, Internal
};
class ClientTableError final : public std::runtime_error {
public:
  ClientTableError(ClientTableFailure failure,const char *reason,std::uint64_t requested=0,std::uint64_t available=0,
      memory::Pool pool=memory::Pool::Values):std::runtime_error(reason),failure_(failure),pool_(pool),requested_(requested),available_(available) {}
  ClientTableFailure Failure() const noexcept {return failure_;}
  memory::Pool Pool() const noexcept {return pool_;}
  std::uint64_t Requested() const noexcept {return requested_;}
  std::uint64_t Available() const noexcept {return available_;}
private:
  ClientTableFailure failure_;
  memory::Pool pool_;
  std::uint64_t requested_=0,available_=0;
};

// 同一个Peer的表、后续影子、缓存与正文必须共享本账户，实际额度仍取自进程三池。
// 分配器按实际allocate数量收费，包含vector capacity及allocate_shared控制块；释放归还。
class ClientStateMemory final {
public:
  static std::shared_ptr<ClientStateMemory> Create();
  void *Allocate(memory::Pool,std::size_t,std::size_t alignment);
  void Deallocate(memory::Pool,void *,std::size_t,std::size_t alignment) noexcept;
  std::array<std::uint64_t,3> Usage() const noexcept;
private:
  ClientStateMemory();
  mutable std::mutex mutex_;
  std::array<memory::Lease,3> charges_;
  std::array<std::uint64_t,3> limits_{};
};
// Client表与影子共用大型后台工作位置；竞争立即报告，不阻塞主线程。
class ClientStateWorkspace final {
public:
  ClientStateWorkspace();
  ~ClientStateWorkspace();
  ClientStateWorkspace(const ClientStateWorkspace &)=delete;
  ClientStateWorkspace &operator=(const ClientStateWorkspace &)=delete;
};
template<class T> class ClientStateAllocator {
public:
  using value_type=T;
  using propagate_on_container_move_assignment=std::true_type;
  using propagate_on_container_swap=std::true_type;
  ClientStateAllocator(std::shared_ptr<ClientStateMemory> memory,memory::Pool pool=memory::Pool::Values) noexcept
      :memory_(std::move(memory)),pool_(pool) {}
  template<class U> ClientStateAllocator(const ClientStateAllocator<U> &other) noexcept:memory_(other.Memory()),pool_(other.Pool()) {}
  T *allocate(std::size_t count) {
    if(!memory_ || count>std::numeric_limits<std::size_t>::max()/sizeof(T))
      throw ClientTableError(ClientTableFailure::Budget,"client_allocation_overflow");
    return static_cast<T *>(memory_->Allocate(pool_,count*sizeof(T),alignof(T)));
  }
  void deallocate(T *value,std::size_t count) noexcept {memory_->Deallocate(pool_,value,count*sizeof(T),alignof(T));}
  const std::shared_ptr<ClientStateMemory> &Memory() const noexcept {return memory_;}
  memory::Pool Pool() const noexcept {return pool_;}
  template<class U> bool operator==(const ClientStateAllocator<U> &other) const noexcept {
    return memory_==other.Memory() && pool_==other.Pool();
  }
private:
  std::shared_ptr<ClientStateMemory> memory_;
  memory::Pool pool_;
};
} // namespace cs::state
