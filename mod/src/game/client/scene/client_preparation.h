#pragma once
#include "game/client/scene/client_install_plan.h"
#include "game/client/scene/client_preparation_native.h"
#include "game/client/scene/client_field_staging.h"
#include "state/client_apply.h"
#include <optional>
#include <thread>

namespace cs::net::live {class Client;}
namespace cs::game {
// 已安装对象由唯一原生安装 owner 解析；每次按完整身份重借，不按地址／成员序号猜复用。
// 只供主线程调用；没有实际 owner 时 Reuse／Remap 明确失败，不能重新创建冒充复用。
class ClientSceneObjectSource {
public:
  virtual ~ClientSceneObjectSource()=default;
  virtual bool Resolve(const state::PresentationMember &,void *&object,std::uint64_t &instance)const noexcept=0;
  virtual bool Current(const state::PresentationMember &,void *object,std::uint64_t instance)const noexcept=0;
};
// 第12步主线程准备 owner。资源工作与应用批次独立；准备成功不授予安装／显示／ACK资格。
// 所有原生句柄和纯值账户留在本实例，必须在主线程显式 Close／Drain 后销毁。
class ClientSceneMainPreparation final {
public:
  explicit ClientSceneMainPreparation(std::shared_ptr<state::ClientStateMemory>,std::shared_ptr<const ClientSceneObjectSource> source={});
  ~ClientSceneMainPreparation();
  ClientSceneMainPreparation(const ClientSceneMainPreparation &)=delete;
  ClientSceneMainPreparation &operator=(const ClientSceneMainPreparation &)=delete;
  bool Open(const state::StateRoute &,std::uint64_t resource_generation) noexcept;
  void Close() noexcept;
  // 可先从首个工作取得 resources->Memory() 创建本 owner，再将原工作归还给本入口。
  bool OfferResource(state::ClientStateResourceJob) noexcept;
  // 直接消费已有专用邮箱；反馈失败时保留原工作和最终结果，下次只重发，不重复加载。
  void PumpResources(net::live::Client &) noexcept;
  state::ClientStateNativeStep Prepare(state::ClientStateApplyBatchRef) noexcept;
  state::ClientStateNativeStep StageObjects(const state::ClientStateApplyBatch &) noexcept;
  state::ClientStateNativeStep StageFields(const state::ClientStateApplyBatch &) noexcept;
  bool Current(const state::ClientStateApplyBatch &)const noexcept;
  bool ObjectsReady(const state::ClientStateApplyBatch &)const noexcept;
  bool FieldsReady(const state::ClientStateApplyBatch &)const noexcept;
  // 借用仅在 visitor 栈内有效，不发布 Render／原生成员关系；退出前必定归还用途计数。
  using ObjectVisitor=bool (*)(void *,std::uint32_t,void *,const state::PresentationMember &) noexcept;
  bool VisitObjects(const state::ClientStateApplyBatch &,void *,ObjectVisitor) noexcept;
  using ResourceVisitor=bool (*)(void *,state::ResourceKey,void *) noexcept;
  bool VisitResources(const state::ClientStateApplyBatch &,void *,ResourceVisitor) noexcept;
  // 原始 field 索引可共享同一投影 owner；所有字段仍为候选私有，不授予安装／发布资格。
  bool VisitFields(const state::ClientStateApplyBatch &,void *,ClientSceneFieldStaging::Visitor) noexcept;
  // 独占安装租约阻止DrainBatch提前销毁已接入世界的暂存；owner必须是实际主线程安装器。
  bool HoldInstallation(const state::ClientStateApplyBatch &,const void *owner) noexcept;
  bool ReleaseInstallation(const state::ClientStateApplyBatch &,const void *owner) noexcept;
  // 成功发布后把整批对象／字段／资源需求移入有界安装保管区，允许继续准备下一批。
  // 原安装租约不变；本调用不发送ACK，也不退休仍被展示／配对流借用的前一批。
  // 成功时逐对象转交复用根的构造责任；旧批次不再因该根固定全部历史字段和计划。
  bool CommitInstallation(const state::ClientStateApplyBatch &,const void *owner) noexcept;
  // 退休前把仍有已提交使用者的根交给存活批次；未提交候选的借用仍须先结束。
  // 本调用不执行原生析构，进入退休后也不再接纳新的根借用／构造责任。
  bool BeginInstallationRetirement(const state::ClientStateApplyBatch &) noexcept;
  bool InstalledObjectCurrent(const state::ClientStateApplyBatch &,std::uint32_t,const void *)const noexcept;
  using RetirementObjectVisitor=bool (*)(void *,std::uint32_t,bool retire_root) noexcept;
  bool VisitRetirementObjects(const state::ClientStateApplyBatch &,void *,RetirementObjectVisitor) noexcept;
  using InstallationAction=state::ClientStateNativeStep (*)(void *) noexcept;
  state::ClientStateNativeStep ApplyInstallation(const state::ClientStateApplyBatch &,const void *owner,bool retirement,
      void *context,InstallationAction) noexcept;
  bool DrainBatch(const state::ClientStateApplyBatch &) noexcept;
  bool Drain() noexcept;
  bool Drained()const noexcept;
  const char *Reason()const noexcept;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace cs::game
