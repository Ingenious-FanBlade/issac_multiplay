#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace cs::memory {
enum class Pool : unsigned { Values, Wire, Workspace };
// 2026-11-07按用户要求翻倍三池额度；所有owner仍共享总账本，不按Peer或阶段重复领取。
inline constexpr std::array<std::uint64_t,3> kReplicationPoolCeilings={512ull<<20,128ull<<20,128ull<<20};
struct Usage final {std::uint64_t current=0,high_water=0,limit=0;};
// 固定大小保留首次/最近拒绝的实际请求与占用，池耗尽后仍可查到失败时刻而非事后零占用。
struct Rejection final {std::uint64_t sequence=0,requested=0,current=0,limit=0;Pool pool=Pool::Values;unsigned owner=0;};
// 名称只能传进程寿命的常量；固定槽记录当前及首次拒绝时的真实租约归属，不分配诊断容器。
struct OwnerUsage final {const char *name=nullptr;std::array<std::uint64_t,3> current{};};
// 进程内所有复制owner共用三池；引用转移不重新收费，最后一个纯值引用释放才归还。
class Budget final {
public:
  Budget(){for(unsigned i=0;i<3;++i)usage_[i].limit=kReplicationPoolCeilings[i];}
  // 启动/空闲重载只能在本地硬上限内调整进程池；已有租约时保持原合同。
  bool Configure(std::array<std::uint64_t,3> limits) noexcept {std::lock_guard lock(mutex_);for(unsigned i=0;i<3;++i)if(usage_[i].current||!limits[i]||limits[i]>kReplicationPoolCeilings[i])return false;for(unsigned i=0;i<3;++i)usage_[i].limit=limits[i];return true;}
  unsigned Owner(const char *name){if(!name)return 0;std::lock_guard lock(mutex_);for(unsigned i=1;i<owners_.size();++i){if(!owners_[i].name){owners_[i].name=name;return i;}if(!std::strcmp(owners_[i].name,name))return i;}++owner_overflow_;return 0;}
  void Acquire(Pool p,std::uint64_t n,unsigned owner=0){std::lock_guard lock(mutex_);auto &u=usage_[unsigned(p)];if(n>u.limit-u.current){last_={last_.sequence+1,n,u.current,u.limit,p,owner};if(!first_.sequence){first_=last_;first_owners_=owners_;}throw std::runtime_error("replication_pool_exhausted");}u.current+=n;owners_[owner].current[unsigned(p)]+=n;if(u.current>u.high_water)u.high_water=u.current;}
  void Release(Pool p,std::uint64_t n,unsigned owner=0) noexcept {std::lock_guard lock(mutex_);usage_[unsigned(p)].current-=n;owners_[owner].current[unsigned(p)]-=n;}
  // 相同池内将已预付的实际保留分配从临时owner转交持久owner；总占用不变，不产生新额度。
  void Transfer(Pool p,std::uint64_t n,unsigned from,unsigned to) noexcept {std::lock_guard lock(mutex_);owners_[from].current[unsigned(p)]-=n;owners_[to].current[unsigned(p)]+=n;}
  std::array<Usage,3> Snapshot() const noexcept {std::lock_guard lock(mutex_);return usage_;}
  // 仅显式诊断时格式化；Acquire只记录纯值，不调用日志、Lua或外部owner。
  std::string FailureFields() const {std::array<Rejection,2> records;std::array<OwnerUsage,64> current,first;std::uint64_t overflow;{std::lock_guard lock(mutex_);records={first_,last_};current=owners_;first=first_owners_;overflow=owner_overflow_;}std::string out;for(unsigned i=0;i<2;++i){const auto &r=records[i];const auto p=std::string("stream.pool_failure.")+(i?"last.":"first.");out+=p+"sequence="+std::to_string(r.sequence)+"\n"+p+"pool="+std::to_string(unsigned(r.pool))+"\n"+p+"requested="+std::to_string(r.requested)+"\n"+p+"current="+std::to_string(r.current)+"\n"+p+"limit="+std::to_string(r.limit)+"\n";if(r.sequence)out+=p+"owner="+(current[r.owner].name?current[r.owner].name:"unattributed")+"\n";}
    const char *pools[]={"values","wire","workspace"};for(unsigned snapshot=0;snapshot<2;++snapshot){if(snapshot&&!records[0].sequence)continue;const auto &entries=snapshot?first:current;for(unsigned i=0;i<entries.size();++i){const auto &e=entries[i];if(!e.current[0]&&!e.current[1]&&!e.current[2])continue;const auto p=std::string("stream.pool_owner.")+(snapshot?"first.":"current.")+std::to_string(i)+".";out+=p+"name="+(e.name?e.name:"unattributed")+"\n";for(unsigned j=0;j<3;++j)out+=p+pools[j]+"="+std::to_string(e.current[j])+"\n";}}out+="stream.pool_owner.overflow="+std::to_string(overflow)+"\n";return out;}
private:
  mutable std::mutex mutex_;
  std::array<Usage,3> usage_;
  Rejection first_,last_;
  std::array<OwnerUsage,64> owners_{},first_owners_{};
  std::uint64_t owner_overflow_=0;
};
inline Budget *ReplicationBudget() noexcept {static Budget budget;return &budget;}
// 租约本身不可复制；shared_ptr共享租约只代表同一份内存，禁止复制内容时沿用旧租约。
class Charge final {
public:
  Charge(Pool p,std::uint64_t n,const char *name=nullptr):owner_(ReplicationBudget()),pool_(p),slot_(owner_->Owner(name)){Resize(n);}
  ~Charge(){owner_->Release(pool_,bytes_,slot_);}
  Charge(const Charge &)=delete;
  Charge &operator=(const Charge &)=delete;
  void Resize(std::uint64_t n){if(n>bytes_)owner_->Acquire(pool_,n-bytes_,slot_);else owner_->Release(pool_,bytes_-n,slot_);bytes_=n;}
  std::uint64_t Bytes() const noexcept {return bytes_;}
  void TransferFrom(Charge &from,std::uint64_t n){if(this==&from||owner_!=from.owner_||pool_!=from.pool_||n>from.bytes_)throw std::runtime_error("replication_charge_transfer");owner_->Transfer(pool_,n,from.slot_,slot_);from.bytes_-=n;bytes_+=n;}
private:
  Budget *owner_;
  Pool pool_;
  unsigned slot_=0;
  std::uint64_t bytes_=0;
};
using Lease=std::shared_ptr<Charge>;
inline Lease Reserve(Pool p,std::uint64_t n,const char *name=nullptr){return std::make_shared<Charge>(p,n,name);}
} // namespace cs::memory
