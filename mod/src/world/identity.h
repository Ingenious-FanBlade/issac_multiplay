#pragma once
// tx-04-08 本地身份：地址 radix 关联与对象/use/出生/revision/readiness 状态。
//
// 本模块不包含 SDK 对象方法、不读取游戏内存、不调用 getter：所有状态转移都由
// 主线程桥以“已观察到的值”调用。地址只是当前录制中的临时 radix 关联，
// 不是网络 ID，也不跨进程比较。
//
// 关键规则（迁移设计第 6.2–6.4 节、ST5 第 4.3 节）：
//   - 已观察到的正常 Factory 非零取得才增加 use；物理构造/复用另增 local_id；
//   - 外层创建 owner 正常完成后才冻结出生描述，Publish 不抢先确认；
//   - 已启用前存在的对象保持 FIRST_OBSERVED，不补造构造/出生历史；
//   - Init 返回覆盖更弱的当前配置读取依据，Morph 内层 Init 因此重新要求 owner；
//   - 未知 Init 撤销不成立的连续性，不由相同地址/seed/Index 修复；
//   - 引用只在目标唯一、同代且连续性证据成立时解析。

#include "world/model.h"

#include <array>
#include <cstdint>
#include <memory>

namespace cs::world {

// 出生描述：外层创建 owner 正常返回后读取并冻结的类型信息。
struct BirthValues final {
  std::uint32_t kind = 0;        // Observation
  std::uint32_t entity_type = 0;
  std::uint32_t variant = 0;
  std::uint32_t subtype = 0;
  std::uint32_t seed = 0;
};

// 单个本地身份。字段顺序按预算登记整理，sizeof 必须不超过
// model.h 的 kIdentityRecordBytes（静态断言在 identity.cpp）。
struct Identity final {
  // —— 地址关联（仅当前录制内）——
  std::uint32_t address = 0;
  std::uint32_t domain = 0; // ObjectDomain

  // —— 本地身份 ——
  std::uint64_t local_id = 0;
  std::uint64_t use = 1;
  std::uint64_t birth = 0;
  std::uint64_t revision = 0;
  std::uint32_t origin = 0;    // Origin
  std::uint32_t lifecycle = 0; // Lifecycle
  std::uint32_t readiness = 0; // Readiness
  std::uint32_t ordinal = kNoOrdinal;

  // —— 出生描述（冻结后不随当前分类变化）——
  std::uint32_t birth_kind = 0;
  std::uint32_t birth_type = 0;
  std::uint32_t birth_variant = 0;
  std::uint32_t birth_subtype = 0;
  std::uint32_t birth_seed = 0;

  // 生命周期标记占同一字节，为形态元组留出空间，保持身份表内存上限。
  bool born : 1 = false;
  bool destroyed : 1 = false;
  bool reused : 1 = false;   // 同址被再次取得，跨进程引用因此有歧义风险
  bool published : 1 = false;
  bool candidate : 1 = false; // 已取得、等待外层 owner 确认出生
  bool current_public_ready : 1 = false; // +177 当前值资格，独立于初始化历史和出生。
  bool native_shape_known : 1 = false;
  bool direct_effect_init_returned : 1 = false; // 同一直接取得的 Effect 已走完派生 Init，尚待发布。
  std::uint64_t owner_scope = 0; // 外层创建 owner 的观察作用域（0=未知）

  // —— 来源成员关系：committed 是上次已提交快照，pending 是本批累积 ——
  std::uint32_t memberships = 0;
  std::uint32_t pending_memberships = 0;
  std::uint64_t membership_attempt = 0;
  std::uint64_t pending_attempt = 0;

  // —— 本批对象去重（batch_attempt 与当前批次不同则 batch_index 无效）——
  std::uint64_t batch_attempt = 0;
  std::int32_t batch_index = -1;
  // 同一 use 在普通采集见到的 Type、Variant、SubType，独立于出生描述。
  std::array<std::uint32_t,3> native_shape{};

  // 已退休：物理销毁或生命周期进入 Retiring 之后。成员退出不算退休。
  bool Retired() const noexcept {
    return destroyed ||
           lifecycle >= static_cast<std::uint32_t>(Lifecycle::Retiring);
  }
  // 基础 Init 正常返回：readiness 降到 1，2/3 的更弱依据被覆盖。
  // 这是保守选择：Morph 内层 Init 之后必须重新等到外层 owner 完成。
  void MarkInitReturned() noexcept {
    current_public_ready = false;
    native_shape_known = false;
    direct_effect_init_returned = false;
    readiness = static_cast<std::uint32_t>(Readiness::BaseInitReturned);
  }
  // 加入实体列表只是成员关系观察，不确认出生；首次发布推进 lifecycle。
  void MarkPublished() noexcept {
    published = true;
    if (lifecycle < static_cast<std::uint32_t>(Lifecycle::RemoveRequested))
      lifecycle = static_cast<std::uint32_t>(Lifecycle::Published);
  }
  void MarkRemoveRequested() noexcept {
    lifecycle = static_cast<std::uint32_t>(Lifecycle::RemoveRequested);
  }
  // 物理析构：与 allocator 释放分开记录。
  void MarkDestroyed() noexcept {
    current_public_ready = false;
    destroyed = true;
    published = false;
    lifecycle = static_cast<std::uint32_t>(Lifecycle::Destroyed);
  }
  // 撤销“当前 Player 读取依据”；创建 owner 依据（2）不受影响。
  void RevokeCurrentWitness() noexcept {
    // 具名原生owner的初始化/换槽证言属于当前use，独立于控制绑定；
    // Init/Restore/Acquire/Destroy仍明确撤销，不能用此分支恢复旧输入或旧对象。
    if(readiness==std::uint32_t(Readiness::PlayerChildInitCompleted)||readiness==std::uint32_t(Readiness::PlayerStateOwnerCompleted)||
       readiness==std::uint32_t(Readiness::PlayerLifeOwnerCompleted))return;
    current_public_ready = false;
    if (readiness == static_cast<std::uint32_t>(Readiness::CurrentPlayerUpdateWitness) ||
        readiness == static_cast<std::uint32_t>(Readiness::NativeJoinEffectCompleted))
      readiness = static_cast<std::uint32_t>(Readiness::NoObservedInit);
  }
  void SetOrdinal(std::uint32_t value) noexcept { ordinal = value; }
  void ClearOrdinal() noexcept { ordinal = kNoOrdinal; }
  // 启用前已存在的对象：只记录 FIRST_OBSERVED，不补出生历史。
  void MarkFirstObserved() noexcept {
    if (!origin)
      origin = static_cast<std::uint32_t>(Origin::FirstObserved);
  }
};
static_assert(sizeof(Identity) <= kIdentityRecordBytes,
              "Identity 超出 model.h 预算登记的 128 字节");

// 地址 (domain,address) → 身份的二叉 radix 关联，加上序号分配与批次关联。
// 单录制单实例，只在主线程使用；分配在准备阶段完成，运行期不再分配。
class IdentityTable final {
public:
  enum class Error : std::uint32_t {
    None,
    NotInitialized,
    Budget,
    SequenceExhausted
  };
  // 未知 Init 的连续性处理结果。
  enum class UnknownInit : std::uint32_t {
    Kept,      // 仍在已观察的创建/Morph 上下文中，只更新 Init 依据
    Reset,     // 撤销不成立的出生/连续性，调用方应发布 Continuity 事件
    Exhausted  // revision 序号耗尽
  };
  // A/B 锚点变化结果；identity 为受影响身份（可能为空）。
  struct AnchorResult final {
    Identity *identity = nullptr;
    std::uint32_t previous = 0;
    bool changed = false;
    bool valid = false;
  };
  // 引用解析结果。
  struct Reference final {
    Relation relation = Relation::Unresolved;
    std::uint64_t local_id = 0;
    std::uint64_t use = 0;
  };

  IdentityTable() noexcept;
  ~IdentityTable();
  IdentityTable(const IdentityTable &) = delete;
  IdentityTable &operator=(const IdentityTable &) = delete;

  // 分配身份表与 radix 节点；预算超限返回 false，不保留半分配状态。
  bool Initialize(BudgetLedger &ledger) noexcept;
  // 归还未完成初始化的分配；已发布的记录不受影响。
  void Release() noexcept;
  bool Ready() const noexcept { return identities_ != nullptr; }
  Error LastError() const noexcept { return error_; }
  // run 域退休后断开地址关联；历史身份保留，序号不重置，旧引用不能在新 run 复活。
  void ResetAssociations() noexcept;
  std::uint32_t Count() const noexcept { return used_; }
  static constexpr std::uint32_t Capacity() noexcept { return kIdentityBudget; }

  // 查找；不存在返回 nullptr。
  Identity *Find(ObjectDomain domain, std::uint32_t address) noexcept;
  // 查找或创建；地址为 0、预算耗尽或序号耗尽返回 nullptr。
  Identity *FindOrCreate(ObjectDomain domain, std::uint32_t address) noexcept;

  // —— 批次关联 ——
  // 每次收束尝试开始时调用：属于上一批的 pending 状态随之失效。
  void BeginBatch(std::uint64_t attempt) noexcept { batch_attempt_ = attempt; }
  std::uint64_t BatchAttempt() const noexcept { return batch_attempt_; }
  // 本批对象去重：已加入返回其下标，未加入返回 -1。
  std::int32_t BatchObjectIndex(const Identity &id) const noexcept;
  void SetBatchObjectIndex(Identity &id, std::int32_t index) noexcept;
  // 来源成员关系累积；source 必须小于 kSourceCount。
  void AddMembership(Identity &id, std::uint32_t source) noexcept;
  // 该身份在本批的来源位集（本批未出现为 0）。
  std::uint32_t MembershipMask(const Identity &id) const noexcept;
  // 是否需要在提交前发布成员关系快照事件。
  bool MembershipChanged(const Identity &id, std::uint32_t mask) const noexcept;
  // 提交位集；只有曾经出现来源的身份才更新 attempt。
  void CommitMembership(Identity &id, std::uint32_t mask) noexcept;

  // —— A/B 锚点与当前 Player 读取依据 ——
  // address=0 表示解绑：清 ordinal 并撤销 witness，返回被清理的身份。
  AnchorResult AnchorPlayer(std::uint32_t ordinal, std::uint32_t address) noexcept;
  // 只在该 ordinal 已唯一锚定同一身份且 readiness<2 时置 3；否则返回 nullptr。
  Identity *MarkCurrentWitness(std::uint32_t ordinal) noexcept;
  // 独立撤销（ST7）：不依赖 binding 层是否触发了共同的 generation 变化。
  Identity *RevokeCurrentWitness(std::uint32_t ordinal) noexcept;

  // —— 取得 / 出生 / 连续性与引用 ——
  // 观察到的取得：同址再次取得增加 use，物理重构换新 local_id。不补出生。
  bool Acquire(Identity &id, Observation kind, bool physical,
               std::uint64_t owner_scope) noexcept;
  // 外层创建 owner 正常返回后冻结出生；条件不成立返回 false。
  bool ConfirmBirth(Identity &id, const BirthValues &values,
                    Observation kind) noexcept;
  // 未知 Init：在创建/Morph 上下文外撤销不成立的出生连续性。
  UnknownInit OnUnknownInit(Identity &id, bool inside_creation_owner) noexcept;
  // 引用解析：只处理 Entity 域；未知地址保持 Unresolved，不填 NULL。
  Reference ResolveReference(std::uint32_t address,
                             const Identity &requester) noexcept;

  // 遍历全部已创建身份（升序，等于创建顺序）。
  template <class F> void ForEach(F &&visitor) noexcept {
    for (std::uint32_t i = 0; i < used_; ++i)
      visitor(identities_[i]);
  }
  template <class F> void ForEach(F &&visitor) const noexcept {
    for (std::uint32_t i = 0; i < used_; ++i)
      visitor(identities_[i]);
  }

private:
  // radix 节点：两个子节点下标 + 1 基身份下标（0 = 无）。
  struct Node final {
    std::uint32_t child[2];
    std::uint32_t index;
  };
  static constexpr std::uint32_t kNodeCapacity =
      static_cast<std::uint32_t>(kRadixNodes);
  bool AllocateIdentity(std::uint64_t &local_id) noexcept;
  bool NextLocalId(std::uint64_t &out) noexcept;
  bool NextBirth(std::uint64_t &out) noexcept;
  void SetError(Error error) noexcept {
    if (error_ == Error::None)
      error_ = error;
  }

  std::unique_ptr<Identity[]> identities_;
  std::unique_ptr<Node[]> nodes_;
  BudgetLedger *ledger_ = nullptr;
  std::uint64_t charged_ = 0;
  std::uint32_t used_ = 0, nodes_used_ = 1;
  std::uint64_t next_local_id_ = 0, next_birth_ = 0;
  std::uint64_t batch_attempt_ = 0;
  std::array<std::uint32_t, kControlAnchorCapacity> player_address_{};
  Error error_ = Error::None;
};

} // namespace cs::world
