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
  std::uint64_t instance=0;
  // 完整基线可以全量重写已安装的同身份成员，实际借用的对象不执行 Init。
  bool borrowed=false;
  bool unbound=false;
  ClientMemberFailure failure=ClientMemberFailure::None;
};
// 暂存槽在Describe结束后不再扩容，并由同一准备／已安装租约保管至writer排空。
// 此处只借用其只读输入，避免为每个普通标量复制五槽Weapon等容器描述。
struct ClientAtomicField {
  const ClientStagedFieldInput *input=nullptr;
  ClientStagedField *native=nullptr;
  bool staged=false;
  const ClientStagedFieldInput &Input()const noexcept {
    static const ClientStagedFieldInput empty;
    return input?*input:empty;
  }
};
struct ClientAtomicResource {state::ResourceKey key;void *address=nullptr;};
// 同一 Manager 写段内的退役身份，地址连同实例号核对，随后即可用于新成员。
struct ClientLiveRetirement {
  state::PresentationMember identity;
  void *address=nullptr;
  std::uint64_t instance=0;
  std::uint32_t morph_target=state::kClientPlanNone;
  bool morphed=false;
};
// 仅本次 World 发布可把仍在显示的 Body 引用跨旧、新形态接回同一池实例。
struct ClientMorphedMember {
  state::PresentationMember before,after;
  void *address=nullptr;
  std::uint64_t instance=0;
};
// Describe 已判定的纯数字归属。Seal 只物化这些具名处理器及显示字段的当前绑定。
struct ClientAtomicFieldUse {
  std::uint8_t kind=0;
  bool covered=false,install=false,display=false;
};
struct ClientAtomicBindingField {
  std::uint32_t field=0;
  std::uint8_t kind=0;
  bool display=false;
};
class ClientNativeReadWindow;
struct ClientAtomicInputs {
  const ClientSceneInstallPlan &scene;
  std::span<const ClientAtomicObject> objects;
  std::span<const ClientAtomicField> fields;
  std::span<const ClientAtomicResource> resources;
  // 只在当前调用栈使用。路径定位可在原生 Init 和资源装载之后完成。
  void *locator=nullptr;
  bool (*locate)(void *,std::uint32_t record,std::uint32_t path,void *&,std::size_t &)=nullptr;
  // 仅显式纯读窗口内存在；窗口结束即清空，不能跨写入或回调沿用页见证。
  ClientNativeReadWindow *reads=nullptr;
  std::span<const ClientAtomicBindingField> binding_fields;
  // 具名写入器通过同一回调拒绝单根，保留全局根的整体错误语义。
  bool (*reject)(void *,std::uint32_t,ClientMemberFailure,const char *) noexcept=nullptr;
};
enum class ClientAtomicReadback { Untracked, Match, Drift, Unsafe };
class ClientSceneFieldIndex;
using ClientSceneFieldIndexRef=std::shared_ptr<const ClientSceneFieldIndex>;
// 纯索引随场景发布，字段调用借同一发布的原生保管。场景在撤显或更换流根时撤回该借用。
struct ClientSceneFieldBinding {
  ClientSceneFieldIndexRef index;
  void *owner=nullptr;
  state::ClientStateNativeStep (*scan)(void *,const ClientSceneFieldIndex &,const state::ClientShadowStateRef &,
      state::BodyStream,std::size_t &,bool) noexcept=nullptr;
};
// 数字计划到真实 ABI 的窄补充 owner：纯协议、资源、伴随对象和结构关系不能通用 memcpy。
// 每个委托必须明确接管完整字段；未接管就是 Unsupported，接口存在不授予覆盖资格。
class ClientAtomicInstallBindings {
public:
  virtual ~ClientAtomicInstallBindings()=default;
  // 绑定器只核自身状态；安装器已核过主线程、route、Game／Room 和 VM。
  virtual bool Current(const state::ClientStateApplyBatch &)const noexcept=0;
  // 在任何 Init／世界写入前核全体对象、退休、成员集合及最终发布容量；不得改变已核的原生承载。
  virtual state::ClientStateNativeStep Preflight(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  virtual bool WorldMembersRetained() const noexcept{return false;}
  // 只读定位真实嵌套承载，供Companion／Synthetic路径使用；extent必须是实际对象大小。
  virtual bool Resolve(const ClientAtomicInputs &,std::uint32_t record,std::uint32_t path,void *parent,void *&address,std::size_t &extent) noexcept=0;
  virtual bool LatePath(const state::Field &)const noexcept=0;
  virtual unsigned Order(const state::Field &)const noexcept{return 4;}
  // 只用于未被具名处理器接管的通用字段，缺省沿用原顺序。
  virtual unsigned ValueOrder(const state::Field &f)const noexcept{return Order(f);}
  // 已覆盖的纯描述元数据由Preflight/发布消费者核定，不产生空的原生写入或回读指令。
  virtual ClientAtomicFieldUse DescribeField(const ClientAtomicInputs &,std::uint32_t field) const noexcept=0;
  // 静止记录（同实例、上次已写、本批无任何块变化）可省掉的具名字段。只有原生对象自行持有、
  // 提交后撤销不回写、且不承载跨对象引用的类别才可省；缺省一律保留。
  virtual bool QuietSkippable(const ClientAtomicFieldUse &) const noexcept {return false;}
  // 快速路径（设计 10.6 节）：依赖帧号的具名字段（Time）。写程序按“本批帧号减锚点”换算，不调用具名写入器。
  virtual bool FrameField(const ClientAtomicFieldUse &) const noexcept {return false;}
  // 快速路径：已提交的当前 World 绑定改看同形的新计划，并安装新帧号。
  // 只在安装器整批换计划时调用；失败表示该绑定已不是当前发布者，调用者须整批失败。
  virtual bool AdvancePublished(const ClientSceneInstallPlan &,std::uint32_t) noexcept {return false;}
  // 所有对象已取得承载后才进入；Init和字段安装必须在同一Manager窗口正常返回，不允许Waiting。
  virtual state::ClientStateNativeStep Initialize(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  virtual state::ClientStateNativeStep PrepareFieldOwners(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept {
    return {true,false,false,"binary_field_owners_ready"};
  }
  virtual state::ClientStateNativeStep InstallField(const state::ClientStateApplyBatch &,const ClientAtomicInputs &,
      std::uint32_t field) noexcept=0;
  // 完整 flags／资源绑定落地后完成指定流的派生图形状态；安装传本批流掩码，
  // 漂移修复只传当前发布根对应的流，不能检查旧 joint 中已被另一批替换的地址。
  virtual state::ClientStateNativeStep FinishFields(const ClientAtomicInputs &,std::uint8_t) noexcept {
    return {true,false,false,"binary_field_graphics_ready"};
  }
  virtual ClientAtomicReadback ReadField(const ClientAtomicInputs &,std::uint32_t) noexcept {return ClientAtomicReadback::Untracked;}
  // 先为同一窗口内的原生显示建立成员关系，外层成功后才公布可复用身份。
  virtual state::ClientStateNativeStep InstallMembers(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  // 对最终原生成员集合执行只读检查。Render／显示和事件由外层协调器先完成。
  virtual state::ClientStateNativeStep Publish(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  // 保管区接收根与字段后才切换主线程可见的已提交成员。
  virtual void Committed(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept {}
  virtual void Invalidate(const state::ClientStateApplyBatch &,const char *) noexcept=0;
  // 先撤本批集合／回调／图形借用；objects[].retire_root为false的根保留给存活批次。
  // 不得按原计划的Join／Reuse推导退休责任，也不销毁仍有安装租约的根或字段。
  virtual bool Quiesce(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
  virtual bool ForgetMembers(std::span<const ClientLiveRetirement>) noexcept {return true;}
  virtual bool UnbindMemberOverlays(std::span<const ClientLiveRetirement>) noexcept {return true;}
  virtual bool ClearReferenceTo(const ClientAtomicInputs &,std::uint32_t,
      std::span<const ClientLiveRetirement>) noexcept {return true;}
  virtual bool UndoField(const state::ClientStateApplyBatch &,const ClientAtomicInputs &,std::uint32_t) noexcept{return true;}
  // 退休不依赖已撤销的显示资格；每次拆边仍须核对同一个原生use，防止旧事务写进复用地址。
  virtual bool RetirementCurrent(const state::ClientStateApplyBatch &,const ClientAtomicInputs &)const noexcept=0;
  // 通用字段拆除后撤本批特殊前缀；仅对retire_root为true的根撤Init。
  // 包括从旧批接管的根；根和字段的最终析构仍交准备owner，异常前缀不得重试。
  virtual bool Retire(const state::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept=0;
};
// 成员写程序表（设计 10.6 节）：按最近发布的 World 登记每个成员记录的纯值写法，场景跨安装器保管。
// 只保存地址与承载句柄的见证，不拥有根或承载；每次使用前核对象实例和承载链。
class ClientSceneMemberStore;
std::shared_ptr<ClientSceneMemberStore> ClientMakeMemberStore(const std::shared_ptr<state::ClientStateMemory> &);
// 撤显、恢复或换路由时清空；下一份完整描述重新登记。
void ClientClearMemberStore(ClientSceneMemberStore &) noexcept;
// 当前 Manager 窗口内在活对象上解析和写入。全局写入失败后撤显，已接入的原生承载按用途退休。
class ClientSceneAtomicInstaller final {
public:
  class RootScalars;
  // store 为空时不登记写程序、不吸收；absorb 为 false 时只登记（强制刷新事务）。
  ClientSceneAtomicInstaller(std::shared_ptr<state::ClientStateMemory>,ClientSceneMainPreparation &,std::shared_ptr<RootScalars>,
      const std::array<ClientSceneFieldIndexRef,2> &published={},std::shared_ptr<ClientSceneMemberStore> store={},bool absorb=false);
  ~ClientSceneAtomicInstaller();
  ClientSceneAtomicInstaller(const ClientSceneAtomicInstaller &)=delete;
  ClientSceneAtomicInstaller &operator=(const ClientSceneAtomicInstaller &)=delete;
  state::ClientStateNativeStep Stage(state::ClientStateApplyBatchRef) noexcept;
  using StagedVisitor=bool (*)(void *,const ClientAtomicInputs &) noexcept;
  bool VisitStaged(const state::ClientStateApplyBatch &,void *,StagedVisitor) noexcept;
  state::ClientStateNativeStep Install(const state::ClientStateApplyBatch &) noexcept;
  // 构图之后安装被原生 Reset 覆盖的 Sprite 和姿态。
  state::ClientStateNativeStep InstallDisplayFields(const state::ClientStateApplyBatch &,bool rebuilt=false) noexcept;
  state::ClientStateNativeStep Publish(const state::ClientStateApplyBatch &) noexcept;
  // 场景按流接收纯数值索引；原生保管区仍负责动态承载、字段 owner 和退场日志。
  ClientSceneFieldBinding PublishedFields()const noexcept;
  void Invalidate(const state::ClientStateApplyBatch &,const char *) noexcept;
  bool Drain(const state::ClientStateApplyBatch &,bool final_scene=false) noexcept;
  bool SelectRetiring(const state::ClientStateApplyBatch &,state::ClientStateVector<ClientLiveRetirement> &) const;
  bool MorphMembers(const state::ClientStateApplyBatch &,std::span<ClientLiveRetirement>) noexcept;
  bool UnbindRetiring(std::span<const ClientLiveRetirement>) noexcept;
  bool ReclaimRetiring(std::span<const ClientLiveRetirement>) noexcept;
  void ResolveDeferredBodyReferences(std::span<const ClientMorphedMember> morphed={}) noexcept;
  void FailedMembers(state::ClientStateVector<ClientLiveRetirement> &) const;
  bool ReclaimFailed(std::span<const ClientLiveRetirement>) noexcept;
  bool ForgetRetired(std::span<const ClientLiveRetirement>) noexcept;
  using FailureVisitor=void (*)(void *,const state::PresentationMember &,ClientMemberFailure);
  void VisitFailures(void *,FailureVisitor) const;
  // 整批快速帧（设计 10.6 节一期）。只在本安装器已发布、仍是当前 World 发布者时使用；
  // ticket 是同形的新 World 票据，安装器的根、承载和退休责任仍属于原票据。
  // 资格判定只读并在写前核原生现场；返回首个不满足的原因，满足时返回 nullptr。
  const char *FastCheck(const state::ClientStateApplyBatch &ticket) noexcept;
  // 写全部非显示纯值叶、Time 与帧号，然后把当前计划换成票据计划。写前失败返回 written=false。
  state::ClientStateNativeStep FastInstall(const state::ClientStateApplyBatchRef &ticket) noexcept;
  // 显示安装之后写显示路径上的纯值叶。
  state::ClientStateNativeStep FastDisplay(const state::ClientStateApplyBatch &ticket) noexcept;
  // 发布成功后把写程序表改记为当前计划。
  void FastPublished(const state::ClientStateApplyBatch &ticket) noexcept;
  // 已发布安装器按当前计划提供输入（事件／声音绑定用）。
  bool VisitPublished(void *,StagedVisitor) noexcept;
  // 本事务是否允许吸收（false 表示强制刷新事务）。
  bool Absorbing()const noexcept;
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
