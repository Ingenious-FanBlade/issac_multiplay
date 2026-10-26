// tx-04-08 本地身份实现：radix 关联、序号分配、批次成员关系与引用解析。
// 全部状态转移都是纯内存操作，不读取游戏对象、不调用原生函数。

#include "world/identity.h"

#include <new>

namespace cs::world {
namespace {

constexpr bool IsCreationOwner(Observation kind) noexcept {
  switch (kind) {
  case Observation::Spawn:
  case Observation::PlayerCreate:
  case Observation::ManagerRestore:
  case Observation::GridSpawn:
  case Observation::Door:
    return true;
  default:
    return false;
  }
}

} // namespace

IdentityTable::IdentityTable() noexcept = default;
IdentityTable::~IdentityTable() { Release(); }

bool IdentityTable::Initialize(BudgetLedger &ledger) noexcept {
  if (identities_)
    return false;
  constexpr std::uint64_t identity_bytes =
      std::uint64_t(sizeof(Identity)) * kIdentityBudget;
  constexpr std::uint64_t node_bytes =
      std::uint64_t(sizeof(Node)) * kNodeCapacity;
  // 两块存储都先记账再分配：预算不足时不留下半分配状态。
  if (!ledger.Charge(identity_bytes)) {
    SetError(Error::Budget);
    return false;
  }
  if (!ledger.Charge(node_bytes)) {
    ledger.Release(identity_bytes);
    SetError(Error::Budget);
    return false;
  }
  auto identities =
      std::unique_ptr<Identity[]>(new (std::nothrow) Identity[kIdentityBudget]());
  auto nodes = std::unique_ptr<Node[]>(new (std::nothrow) Node[kNodeCapacity]());
  if (!identities || !nodes) {
    ledger.Release(identity_bytes + node_bytes);
    SetError(Error::Budget);
    return false;
  }
  identities_ = std::move(identities);
  nodes_ = std::move(nodes);
  ledger_ = &ledger;
  charged_ = identity_bytes + node_bytes;
  used_ = 0;
  nodes_used_ = 1;
  next_local_id_ = 0;
  next_birth_ = 0;
  batch_attempt_ = 0;
  player_address_.fill(0);
  error_ = Error::None;
  return true;
}

void IdentityTable::Release() noexcept {
  if (ledger_ && charged_) {
    ledger_->Release(charged_);
    ledger_ = nullptr;
    charged_ = 0;
  }
  identities_.reset();
  nodes_.reset();
  used_ = 0;
  nodes_used_ = 1;
}

Identity *IdentityTable::Find(ObjectDomain domain,
                              std::uint32_t address) noexcept {
  if (!identities_ || !address)
    return nullptr;
  // 键是 (domain,address) 的字面位串，不计算摘要。
  const auto key = (static_cast<std::uint64_t>(domain) << 32) | address;
  std::uint32_t node = 0;
  for (int bit = 32; bit >= 0; --bit) {
    const auto branch = static_cast<std::uint32_t>((key >> bit) & 1u);
    const auto child = nodes_[node].child[branch];
    if (!child)
      return nullptr;
    node = child;
  }
  const auto index = nodes_[node].index;
  return index ? &identities_[index - 1] : nullptr;
}

Identity *IdentityTable::FindOrCreate(ObjectDomain domain,
                                     std::uint32_t address) noexcept {
  if (!identities_) {
    SetError(Error::NotInitialized);
    return nullptr;
  }
  if (!address)
    return nullptr;
  const auto key = (static_cast<std::uint64_t>(domain) << 32) | address;
  std::uint32_t node = 0;
  for (int bit = 32; bit >= 0; --bit) {
    const auto branch = static_cast<std::uint32_t>((key >> bit) & 1u);
    auto &slot = nodes_[node].child[branch];
    if (!slot) {
      // 最坏情形 32768 个身份 × 34 节点，容量在准备阶段一次性分配。
      if (nodes_used_ >= kNodeCapacity) {
        SetError(Error::Budget);
        return nullptr;
      }
      slot = nodes_used_++;
      auto &fresh = nodes_[slot];
      fresh.child[0] = 0;
      fresh.child[1] = 0;
      fresh.index = 0;
    }
    node = slot;
  }
  auto &index = nodes_[node].index;
  if (index)
    return &identities_[index - 1];
  if (used_ >= kIdentityBudget) {
    SetError(Error::Budget);
    return nullptr;
  }
  std::uint64_t local_id = 0;
  if (!NextLocalId(local_id))
    return nullptr;
  index = ++used_;
  auto &identity = identities_[index - 1];
  identity.address = address;
  identity.domain = static_cast<std::uint32_t>(domain);
  identity.local_id = local_id;
  identity.use = 1;
  identity.ordinal = kNoOrdinal;
  identity.batch_index = -1;
  return &identity;
}

bool IdentityTable::NextLocalId(std::uint64_t &out) noexcept {
  // 序号到顶即报 SequenceExhausted，不回绕、不复用旧 ID。
  if (next_local_id_ >= UINT64_MAX - 1) {
    SetError(Error::SequenceExhausted);
    return false;
  }
  out = ++next_local_id_;
  return true;
}

bool IdentityTable::NextBirth(std::uint64_t &out) noexcept {
  if (next_birth_ >= UINT64_MAX - 1) {
    SetError(Error::SequenceExhausted);
    return false;
  }
  out = ++next_birth_;
  return true;
}

std::int32_t IdentityTable::BatchObjectIndex(const Identity &id) const noexcept {
  return id.batch_attempt == batch_attempt_ ? id.batch_index : -1;
}

void IdentityTable::SetBatchObjectIndex(Identity &id,
                                        std::int32_t index) noexcept {
  id.batch_attempt = batch_attempt_;
  id.batch_index = index;
}

void IdentityTable::AddMembership(Identity &id, std::uint32_t source) noexcept {
  if (source >= kSourceCount)
    return;
  // 每次 attempt 的第一次累积清空旧位集；BeginBatch 必须传入非零 attempt。
  if (id.pending_attempt != batch_attempt_) {
    id.pending_attempt = batch_attempt_;
    id.pending_memberships = 0;
  }
  id.pending_memberships |= (1u << source);
}

std::uint32_t IdentityTable::MembershipMask(const Identity &id) const noexcept {
  return id.pending_attempt == batch_attempt_ ? id.pending_memberships : 0u;
}

bool IdentityTable::MembershipChanged(const Identity &id,
                                      std::uint32_t mask) const noexcept {
  // 只有“曾经出现来源”或“本批仍有来源”的身份才产生快照事件。
  if (!id.membership_attempt && !mask)
    return false;
  return !id.membership_attempt || id.memberships != mask;
}

void IdentityTable::CommitMembership(Identity &id, std::uint32_t mask) noexcept {
  if (!id.membership_attempt && !mask)
    return;
  id.memberships = mask;
  id.membership_attempt = batch_attempt_;
}

IdentityTable::AnchorResult
IdentityTable::AnchorPlayer(std::uint32_t ordinal, std::uint32_t address) noexcept {
  AnchorResult result{};
  if (ordinal >= player_address_.size())
    return result;
  result.previous = player_address_[ordinal];
  result.valid = address != 0;
  result.changed = result.previous != address;
  player_address_[ordinal] = address;
  if (address) {
    auto *identity = FindOrCreate(ObjectDomain::Entity, address);
    if (!identity) {
      // 分配失败不保留半绑定：旧地址保持原样，由调用方按错误停用录制。
      player_address_[ordinal] = result.previous;
      result.valid = false;
      result.changed = false;
      return result;
    }
    identity->ordinal = ordinal;
    result.identity = identity;
    return result;
  }
  // 解绑：清 ordinal 并撤销当前 Player 读取依据；不去重同址复用证据。
  for (std::uint32_t i = 0; i < used_; ++i) {
    auto &identity = identities_[i];
    if (identity.ordinal != ordinal)
      continue;
    identity.ordinal = kNoOrdinal;
    identity.RevokeCurrentWitness();
    if (!result.identity)
      result.identity = &identity;
  }
  return result;
}

Identity *IdentityTable::MarkCurrentWitness(std::uint32_t ordinal) noexcept {
  if (ordinal >= player_address_.size() || !player_address_[ordinal])
    return nullptr;
  auto *identity = Find(ObjectDomain::Entity, player_address_[ordinal]);
  if (!identity || identity->ordinal != ordinal)
    return nullptr;
  // 创建 owner 已完成时保留更完整的依据；witness 只覆盖缺失的 0/1。
  if (identity->readiness >=
      static_cast<std::uint32_t>(Readiness::CreationOwnerCompleted))
    return nullptr;
  identity->readiness =
      static_cast<std::uint32_t>(Readiness::CurrentPlayerUpdateWitness);
  return identity;
}

Identity *IdentityTable::RevokeCurrentWitness(std::uint32_t ordinal) noexcept {
  for (std::uint32_t i = 0; i < used_; ++i) {
    auto &identity = identities_[i];
    if (identity.ordinal != ordinal)
      continue;
    if (identity.readiness !=
        static_cast<std::uint32_t>(Readiness::CurrentPlayerUpdateWitness))
      continue;
    identity.RevokeCurrentWitness();
    return &identity;
  }
  return nullptr;
}

bool IdentityTable::Acquire(Identity &id, Observation kind, bool physical,
                            std::uint64_t owner_scope) noexcept {
  if (id.origin || id.destroyed) {
    // 同址再次取得：use 递增；物理重构或已销毁时换新物理 local_id。
    if (id.use >= UINT64_MAX - 1) {
      SetError(Error::SequenceExhausted);
      return false;
    }
    ++id.use;
    id.reused = true;
    if (physical || id.destroyed) {
      std::uint64_t local_id = 0;
      if (!NextLocalId(local_id))
        return false;
      id.local_id = local_id;
    }
  }
  id.origin = static_cast<std::uint32_t>(Origin::ObservedCreation);
  id.lifecycle = static_cast<std::uint32_t>(Lifecycle::Acquired);
  id.readiness = static_cast<std::uint32_t>(Readiness::NoObservedInit);
  id.birth = 0;
  id.born = false;
  id.revision = 0;
  id.ordinal = kNoOrdinal;
  id.destroyed = false;
  id.published = false;
  id.candidate = true;
  id.memberships = 0;
  id.membership_attempt = 0;
  id.pending_memberships = 0;
  id.pending_attempt = 0;
  id.birth_kind = static_cast<std::uint32_t>(kind);
  id.birth_type = 0;
  id.birth_variant = 0;
  id.birth_subtype = 0;
  id.birth_seed = 0;
  id.owner_scope = owner_scope;
  id.batch_attempt = 0;
  id.batch_index = -1;
  return true;
}

bool IdentityTable::ConfirmBirth(Identity &id, const BirthValues &values,
                                 Observation kind) noexcept {
  // 只在外层创建 owner 正常返回后确认：必须已取得、尚无出生且已有 Init 依据。
  if (id.born || !id.candidate ||
      id.readiness == static_cast<std::uint32_t>(Readiness::NoObservedInit))
    return false;
  std::uint64_t birth = 0;
  if (!NextBirth(birth))
    return false;
  id.birth = birth;
  id.born = true;
  id.candidate = false;
  if (IsCreationOwner(kind))
    id.readiness =
        static_cast<std::uint32_t>(Readiness::CreationOwnerCompleted);
  id.birth_kind = values.kind;
  id.birth_type = values.entity_type;
  id.birth_variant = values.variant;
  id.birth_subtype = values.subtype;
  id.birth_seed = values.seed;
  if (id.lifecycle < static_cast<std::uint32_t>(Lifecycle::RemoveRequested))
    id.lifecycle = id.published ? static_cast<std::uint32_t>(Lifecycle::Published)
                                : static_cast<std::uint32_t>(Lifecycle::Constructed);
  return true;
}

IdentityTable::UnknownInit
IdentityTable::OnUnknownInit(Identity &id, bool inside_creation_owner) noexcept {
  // 创建/Morph 上下文中的 Init 属于已观察路径，不撤销出生连续性。
  if (!inside_creation_owner && !id.candidate) {
    if (id.revision >= UINT64_MAX) {
      SetError(Error::SequenceExhausted);
      return UnknownInit::Exhausted;
    }
    // 未知 Init 不补造连续性：出生依据作废，退回 FIRST_OBSERVED。
    id.born = false;
    id.birth = 0;
    id.origin = static_cast<std::uint32_t>(Origin::FirstObserved);
    ++id.revision;
    id.MarkInitReturned();
    return UnknownInit::Reset;
  }
  id.MarkInitReturned();
  return UnknownInit::Kept;
}

IdentityTable::Reference
IdentityTable::ResolveReference(std::uint32_t address,
                                const Identity &requester) noexcept {
  Reference out{};
  if (!address) {
    out.relation = Relation::Null;
    return out;
  }
  auto *target = Find(ObjectDomain::Entity, address);
  if (!target) {
    // 未知地址不能变成 NULL，也不能凭空给出 local target。
    out.relation = Relation::Unresolved;
    return out;
  }
  if (target->Retired()) {
    out.relation = Relation::Retired;
    return out;
  }
  if (target->reused) {
    out.relation = Relation::AmbiguousReuse;
    return out;
  }
  const bool requester_ready =
      requester.born && requester.readiness != 0 && requester.revision == 0;
  const bool target_in_batch = BatchObjectIndex(*target) >= 0;
  // 只有同代、当前候选唯一且出生顺序成立时才给出已知目标。
  if (requester_ready && target->born && target->birth &&
      target->birth < requester.birth && target_in_batch) {
    out.relation = Relation::KnownCurrent;
    out.local_id = target->local_id;
    out.use = target->use;
    return out;
  }
  out.relation = Relation::Unresolved;
  return out;
}

} // namespace cs::world
