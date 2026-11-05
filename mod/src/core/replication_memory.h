#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace cs::memory {
enum class Pool : unsigned { Values, Wire, Workspace };
struct Usage final {std::uint64_t current=0,high_water=0,limit=0;};
// 进程内所有复制owner共用三池；引用转移不重新收费，最后一个纯值引用释放才归还。
class Budget final {
public:
  Budget():usage_{{{0,0,256ull<<20},{0,0,64ull<<20},{0,0,64ull<<20}}}{}
  // 启动/空闲重载只能在本地硬上限内调整进程池；已有租约时保持原合同。
  bool Configure(std::array<std::uint64_t,3> limits) noexcept {std::lock_guard lock(mutex_);for(unsigned i=0;i<3;++i)if(usage_[i].current||!limits[i]||limits[i]>std::array<std::uint64_t,3>{256ull<<20,64ull<<20,64ull<<20}[i])return false;for(unsigned i=0;i<3;++i)usage_[i].limit=limits[i];return true;}
  void Acquire(Pool p,std::uint64_t n){std::lock_guard lock(mutex_);auto &u=usage_[unsigned(p)];if(n>u.limit-u.current)throw std::runtime_error("replication_pool_exhausted");u.current+=n;if(u.current>u.high_water)u.high_water=u.current;}
  void Release(Pool p,std::uint64_t n) noexcept {std::lock_guard lock(mutex_);usage_[unsigned(p)].current-=n;}
  std::array<Usage,3> Snapshot() const noexcept {std::lock_guard lock(mutex_);return usage_;}
private:
  mutable std::mutex mutex_;
  std::array<Usage,3> usage_;
};
inline Budget *ReplicationBudget() noexcept {static Budget budget;return &budget;}
// 租约本身不可复制；shared_ptr共享租约只代表同一份内存，禁止复制内容时沿用旧租约。
class Charge final {
public:
  Charge(Pool p,std::uint64_t n):owner_(ReplicationBudget()),pool_(p){Resize(n);}
  ~Charge(){owner_->Release(pool_,bytes_);}
  Charge(const Charge &)=delete;
  Charge &operator=(const Charge &)=delete;
  void Resize(std::uint64_t n){if(n>bytes_)owner_->Acquire(pool_,n-bytes_);else owner_->Release(pool_,bytes_-n);bytes_=n;}
  std::uint64_t Bytes() const noexcept {return bytes_;}
private:
  Budget *owner_;
  Pool pool_;
  std::uint64_t bytes_=0;
};
using Lease=std::shared_ptr<Charge>;
inline Lease Reserve(Pool p,std::uint64_t n){return std::make_shared<Charge>(p,n);}
} // namespace cs::memory
