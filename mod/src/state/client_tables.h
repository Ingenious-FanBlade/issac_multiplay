#pragma once
#include "state/client_memory.h"
#include <thread>
#include <vector>

namespace cs::state {
template<class T> using ClientStateVector=std::vector<T,ClientStateAllocator<T>>;
struct ClientStateTableStorage;
struct ClientResourceContent;

// 注册来自本地产品owner，不来自网络；未注册的空间／根布局组合明确拒绝。
struct ClientMemberLayoutBinding {
  std::uint32_t space=0;
  std::uint16_t layout=0;
  auto operator<=>(const ClientMemberLayoutBinding &) const = default;
};
// 只读快照固定完整目录与原update；输入报文／scratch在安装返回后可立即复用。
class ClientMemberSnapshot final {
public:
  explicit ClientMemberSnapshot(const std::shared_ptr<ClientStateMemory> &);
  StateMemberDirectory Directory() const noexcept;
  const StateRoute &Route() const noexcept {return route_;}
private:
  friend struct ClientStateTableStorage;
  StateRoute route_{};
  std::uint32_t epoch_=0,previous_=0;
  ClientStateVector<StateMemberEntry> entries_,update_;
};
// 资源内容按项共享，不通过旧snapshot链延寿；图计划与目录数组随此快照固定。
class ClientResourceSnapshot final {
public:
  explicit ClientResourceSnapshot(const std::shared_ptr<ClientStateMemory> &);
  ResourceDirectoryView Directory() const noexcept;
  ResourceGraphPlan Graph() const noexcept;
  // 主线程从首个资源工作沿用同一 Peer 账户，不另建一份准备额度。
  std::shared_ptr<ClientStateMemory> Memory()const noexcept{return entries_.get_allocator().Memory();}
private:
  friend struct ClientStateTableStorage;
  std::uint64_t session_=0,run_=0;
  std::uint32_t epoch_=0,previous_=0;
  ClientStateVector<ResourceEntryView> entries_,update_;
  ClientStateVector<std::shared_ptr<const ClientResourceContent>> content_;
  ClientStateVector<ResourceEdge> edges_;
  ClientStateVector<std::uint32_t> offsets_;
};
using ClientMemberSnapshotRef=std::shared_ptr<const ClientMemberSnapshot>;
using ClientResourceSnapshotRef=std::shared_ptr<const ClientResourceSnapshot>;
enum class ClientTableInstall : std::uint8_t { Installed, Duplicate, Stale };
// 本地准备与元数据分开；Ready不代表候选完整、原生安装或可以ACK。
enum class ClientResourceState : std::uint8_t { Unprepared, Pending, Ready, MissingLocalResource, Failed };
struct ClientResourcePreparationTicket {
  StateRoute route;
  ResourceKey key;
  std::uint32_t epoch=0;
  std::uint64_t generation=0,serial=0;
  auto operator<=>(const ClientResourcePreparationTicket &) const = default;
};

// 此类没有线程／游戏／网络调用，只由显式指定的后台线程修改。
// 主线程只能借用返回的const快照；registry保留真实owner到Collect／Drained。
// 生命周期owner必须在所有外部快照与准备票据归还后，于后台销毁本对象。
class ClientStateTables final : public ResourceReadyContext {
public:
  ClientStateTables(std::shared_ptr<ClientStateMemory>,std::thread::id owner,
      std::span<const ClientMemberLayoutBinding>);
  ~ClientStateTables();
  ClientStateTables(const ClientStateTables &)=delete;
  ClientStateTables &operator=(const ClientStateTables &)=delete;
  // 普通换route保留同session/run内容历史；新run必须先Close且外部借用已排空。
  void Open(const StateRoute &,StateAgreement,std::uint64_t resource_generation);
  void Close();
  bool Drained() const;
  void Collect();
  ClientTableInstall InstallMembers(const StateRoute &,std::span<const std::byte>);
  ClientTableInstall InstallResources(const StateRoute &,std::span<const std::byte>);
  // 生命周期owner证明旧route及原生借用已排空后显式调用；普通Open不重编号epoch。
  ClientTableInstall RebaseResources(const StateRoute &retired,std::span<const std::byte>);
  ClientMemberSnapshotRef Members(std::uint32_t epoch) const;
  ClientResourceSnapshotRef Resources(std::uint32_t epoch) const;
  std::optional<StateContextReady> MetadataReady() const;
  // 仅固定本目标的非空资源根。空集合直接返回合法空闭包，避免审阅整个无关目录。
  ResourceGraphView Prepare(const ClientResourceSnapshotRef &,std::span<const ResourceKey>,ResourceGraphScratch &) const;
  // 完整计划的图校验复用调用者租约；不将同线程的另一项独立工作默认视为可重入。
  ResourceGraphView Prepare(const ClientResourceSnapshotRef &,std::span<const ResourceKey>,ResourceGraphScratch &,const ClientStateWorkspace &) const;
  ClientResourceState ResourceState(std::uint32_t epoch,ResourceKey) const;
  ClientResourcePreparationTicket BeginResourcePreparation(std::uint32_t epoch,ResourceKey);
  // Pending保留票据和快照；只有最终完成／取消释放准备借用。旧票据最终反馈仅排空。
  bool CompleteResourcePreparation(const ClientResourcePreparationTicket &,ClientResourceState);
  void InvalidateResourceGeneration(std::uint64_t fresh_generation);
  bool Materialized(const ResourceDirectoryView &,const ResourceEntryView &) const override;
  const std::shared_ptr<ClientStateMemory> &Memory() const noexcept {return memory_;}
private:
  std::shared_ptr<ClientStateMemory> memory_;
  std::shared_ptr<ClientStateTableStorage> storage_;
};
} // namespace cs::state
