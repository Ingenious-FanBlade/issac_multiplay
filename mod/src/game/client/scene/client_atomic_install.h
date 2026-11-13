#pragma once
#include "game/client/scene/client_preparation.h"
#include "game/client/scene/client_atomic_install_native.h"

namespace cs::game {
// 这些地址只存在于唯一主线程安装器，不进入接收器／影子／完成邮箱。
struct ClientAtomicObject {
  void *address=nullptr;state::PresentationMember identity;
  // 仅在退休输入有效：构造责任可能已从早期批次转入，也可能已交给后继。
  // false时不得撤Init／移除该根；仍须解除本批自己的字段和用途借用。
  bool retire_root=false;
};
struct ClientAtomicField {ClientStagedFieldInput input;ClientStagedField *native=nullptr;};
struct ClientAtomicResource {state::ResourceKey key;void *address=nullptr;};
struct ClientAtomicInputs {
  const ClientSceneInstallPlan &scene;
  std::span<const ClientAtomicObject> objects;
  std::span<const ClientAtomicField> fields;
  std::span<const ClientAtomicResource> resources;
  // 只在当前调用栈使用。路径定位可在原生 Init 和资源装载之后完成。
  void *locator=nullptr;
  bool (*locate)(void *,std::uint32_t record,std::uint32_t path,void *&,std::size_t &)=nullptr;
};
enum class ClientAtomicReadback { Untracked, Match, Drift, Unsafe };
// 数字计划到真实 ABI 的窄补充 owner：纯协议、资源、伴随对象和结构关系不能通用 memcpy。
// 每个委托必须明确接管完整字段；未接管就是 Unsupported，接口存在不授予覆盖资格。
class ClientAtomicInstallBindings {
public:
  virtual ~ClientAtomicInstallBindings()=default;
  virtual bool Current(const state::ClientStateApplyBatch &)const noexcept=0;
  // 在任何 Init／世界写入前核全体对象、退休、成员集合及最终发布容量；不得写世界。
  virtual state::ClientStateNativeStep Preflight(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  // 返回真实嵌套承载，供Companion／Synthetic路径使用；extent必须是实际对象大小。
  virtual bool Resolve(const ClientAtomicInputs &,std::uint32_t record,std::uint32_t path,void *parent,void *&address,std::size_t &extent) noexcept=0;
  virtual bool LatePath(const state::Field &)const noexcept=0;
  virtual unsigned Order(const state::Field &)const noexcept{return 4;}
  virtual bool Covers(const ClientAtomicInputs &,std::uint32_t field)const noexcept=0;
  // 所有对象已取得承载后才进入；Init和字段安装必须在同一Manager窗口正常返回，不允许Waiting。
  virtual state::ClientStateNativeStep Initialize(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  virtual state::ClientStateNativeStep PrepareFieldOwners(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept {
    return {true,false,false,"binary_field_owners_ready"};
  }
  virtual state::ClientStateNativeStep InstallField(const state::ClientStateApplyBatch &,const ClientAtomicInputs &,std::uint32_t field) noexcept=0;
  virtual ClientAtomicReadback ReadField(const ClientAtomicInputs &,std::uint32_t,bool) noexcept {return ClientAtomicReadback::Untracked;}
  // 先为同一窗口内的原生显示建立成员关系，外层成功后才公布可复用身份。
  virtual state::ClientStateNativeStep InstallMembers(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  // 只发布原生成员集合。Render／显示和事件由外层协调器先完成，AppliedAck仍由接收器决定。
  virtual state::ClientStateNativeStep Publish(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  // 保管区接收根与字段后才切换主线程可见的已提交成员。
  virtual void Committed(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept {}
  virtual void Invalidate(const state::ClientStateApplyBatch &,const char *) noexcept=0;
  // 先撤本批集合／回调／图形借用；objects[].retire_root为false的根保留给存活批次。
  // 不得按原计划的Join／Reuse推导退休责任，也不销毁仍有安装租约的根或字段。
  virtual bool Quiesce(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  virtual bool UndoField(const state::ClientStateApplyBatch &,const ClientAtomicInputs &,std::uint32_t) noexcept{return true;}
  // 退休不依赖已撤销的显示资格；每次拆边仍须核对同一个原生use，防止旧事务写进复用地址。
  virtual bool RetirementCurrent(const state::ClientStateApplyBatch &,const ClientAtomicInputs &)const noexcept=0;
  // 通用字段拆除后撤本批特殊前缀；仅对retire_root为true的根撤Init。
  // 包括从旧批接管的根；根和字段的最终析构仍交准备owner，异常前缀不得重试。
  virtual bool Retire(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
};
// 第12步实际原子writer。Stage允许等待，Install／Publish禁止跨Manager窗口；
// 写入失败先撤展示，持有全部租约直到真实排空。成功也不能提前释放已安装容器。
class ClientSceneAtomicInstaller final {
public:
  class RootScalars;
  ClientSceneAtomicInstaller(std::shared_ptr<state::ClientStateMemory>,ClientSceneMainPreparation &);
  ClientSceneAtomicInstaller(std::shared_ptr<state::ClientStateMemory>,ClientSceneMainPreparation &,std::shared_ptr<RootScalars>);
  ClientSceneAtomicInstaller(std::shared_ptr<state::ClientStateMemory>,ClientSceneMainPreparation &,ClientAtomicInstallBindings &);
  ~ClientSceneAtomicInstaller();
  ClientSceneAtomicInstaller(const ClientSceneAtomicInstaller &)=delete;
  ClientSceneAtomicInstaller &operator=(const ClientSceneAtomicInstaller &)=delete;
  state::ClientStateNativeStep Stage(state::ClientStateApplyBatchRef) noexcept;
  state::ClientStateNativeStep Install(const state::ClientStateApplyBatch &) noexcept;
  state::ClientStateNativeStep Publish(const state::ClientStateApplyBatch &) noexcept;
  // 普通字段归各流安装器；共享costume的两流投影只由最新提交的安装器回读。
  state::ClientStateNativeStep Repair(const state::ClientStateApplyBatch &,state::BodyStream,std::size_t &cursor,bool costumes=false) noexcept;
  void Invalidate(const state::ClientStateApplyBatch &,const char *) noexcept;
  bool Drain(const state::ClientStateApplyBatch &,bool final_scene=false) noexcept;
  bool Current(const state::ClientStateApplyBatch &)const noexcept;
  bool Installed(const state::ClientStateApplyBatch &)const noexcept;
  bool Published(const state::ClientStateApplyBatch &)const noexcept;
  bool Drained()const noexcept;
  const char *Reason()const noexcept;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
std::shared_ptr<ClientSceneAtomicInstaller::RootScalars> ClientBinaryRootScalars(const std::shared_ptr<state::ClientStateMemory> &);
} // namespace cs::game
