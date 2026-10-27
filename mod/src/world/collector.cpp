#include "world/collector.h"
#include "binding/world_layout.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <windows.h>

namespace cs::world {
namespace {
bool Creation(Observation kind) noexcept {
  return kind == Observation::Spawn || kind == Observation::PlayerCreate ||
         kind == Observation::ManagerRestore || kind == Observation::GridSpawn ||
         kind == Observation::Door;
}
bool Morph(Observation kind) noexcept {
  return kind == Observation::NpcMorph || kind == Observation::PickupMorph;
}
bool GridObservation(Observation kind) noexcept {
  return kind == Observation::GridFactory || kind == Observation::GridInit ||
         kind == Observation::GridSpawn || kind == Observation::Door ||
         kind == Observation::GridFree;
}
// 字段只保存原始位；非有限值保留位型并使本批不完整，不调用浮点 getter。
bool Finite(const RawField &field) noexcept {
  std::uint32_t bits = 0;
  std::memcpy(&bits, field.raw.data(), sizeof(bits));
  return (bits & 0x7f800000u) != 0x7f800000u;
}
} // namespace

Collector::Collector(WorldStorage &storage, IdentityTable &identities,
                     MemoryReader reader, std::uint32_t generation) noexcept
    : storage_(storage), identities_(identities), memory_(reader),
      generation_(generation) {
  // 构造发生在后台准备阶段；Collector 本体包含固定工作表，也实际计入账本。
  charged_ = storage_.Ready() && storage_.Ledger().Charge(sizeof(Collector));
}
Collector::~Collector() {
  pending_ = {};
  if (charged_) storage_.Ledger().Release(sizeof(Collector));
}
static_assert(sizeof(Collector) < kManagementBytes, "Collector 工作表必须落在管理预算之内");
bool Collector::Activate() noexcept {
  const char *reason = nullptr;
  if (active_ || stopping_ || !charged_ || !generation_ || !memory_.copy ||
      !memory_.image_base || !memory_.image_size || !memory_.main_thread ||
      !storage_.Ready() || !identities_.Ready() || !layout::Validate(&reason))
    return false;
  active_ = true;
  return true;
}
bool Collector::Recording() const noexcept {
  return active_ && !stopping_ && storage_.Ready() &&
         storage_.Diag().FaultReason() == Reason::None;
}
bool Collector::Drained() const noexcept {
  return !depth_ && !lifecycle_depth_ && !outer_active_ && !pending_;
}
void Collector::CloseIfDrained() noexcept {
  if ((stopping_ || storage_.Diag().FaultReason() != Reason::None) && Drained()) {
    active_ = false;
    storage_.RequestStop(storage_.Diag().FaultReason() == Reason::None
                             ? Reason::Stopped : storage_.Diag().FaultReason());
  }
}
void Collector::Stop(Reason reason) noexcept {
  stopping_ = true;
  if (reason != Reason::Stopped && reason != Reason::None)
    storage_.Diag().Fail(reason, FailSource::Producer, 0, "collector_stop");
  CloseIfDrained();
}
void Collector::Fault(Reason reason, const char *stage) noexcept {
  storage_.Diag().Fail(reason, FailSource::Producer, 0, stage);
  stopping_ = true;
  polluted_ = true;
  CloseIfDrained();
}
bool Collector::Read(std::uint64_t address, void *value, std::size_t size) noexcept {
  return GetCurrentThreadId() == memory_.main_thread && memory_.copy && address &&
         address <= UINT32_MAX && size <= UINT32_MAX - address &&
         memory_.copy(static_cast<std::uint32_t>(address), value, size);
}
bool Collector::Checkpoint(std::uint64_t request, const char *label) noexcept {
  if (!Recording() || GetCurrentThreadId() != memory_.main_thread || !label)
    return false;
  CheckpointRecord marker{};
  marker.request = request; marker.run = run_; marker.room = room_;
  marker.outer = outer_active_ ? outer_ : 0; marker.mutation = mutation_;
  marker.recording_generation = generation_;
  marker.attempt = storage_.Diag().last_attempt.load();
  marker.capture = storage_.Diag().last_capture.load();
  marker.event_watermark = storage_.EventWatermark();
  const auto length = std::strlen(label);
  if (!ValidWorldLabel(label)) return false;
  std::memcpy(marker.label, label, length);
  if (storage_.PublishCheckpoint(marker)) return true;
  Fault(Reason::EventOverflow, "checkpoint_publish");
  return false;
}
bool Collector::Image(std::uint32_t address, std::size_t bytes) const noexcept {
  return address >= memory_.image_base && bytes <= memory_.image_size &&
         address - memory_.image_base <= memory_.image_size - bytes;
}
bool Collector::ReadRoots(Roots &out) noexcept {
  const auto &c = layout::LayoutConstants();
  return Value(std::uint64_t(memory_.image_base) + c.game_root_rva, out.game) &&
         out.game && Value(std::uint64_t(out.game) + c.game_room_offset, out.room) &&
         Value(std::uint64_t(out.game) + c.game_room_index_offset, out.index) &&
         Value(std::uint64_t(out.game) + c.game_dimension_offset, out.dimension);
}
Identity *Collector::Find(ObjectDomain domain, std::uint32_t address) noexcept {
  if (!address)
    return nullptr;
  auto *id = identities_.FindOrCreate(domain, address);
  if (!id)
    Fault(identities_.LastError() == IdentityTable::Error::SequenceExhausted
              ? Reason::SequenceExhausted : Reason::Budget, "identity");
  return id;
}
bool Collector::Event(EventKind kind, Reason reason, Identity *id,
                       const Call *call, bool entering, bool exceptional,
                       std::uint32_t result, std::uint64_t candidate_tick) noexcept {
  LifecycleEvent event{};
  event.run = run_; event.room = room_; event.outer = outer_active_ ? outer_ : 0;
  event.kind = kind; event.reason = reason; event.entering = entering;
  event.exceptional = exceptional; event.result = result;
  event.mutation = mutation_; event.tick = candidate_tick ? candidate_tick : authority_.tick;
  event.thread = GetCurrentThreadId();
  if (id) {
    event.local_id = id->local_id; event.use = id->use; event.birth = id->birth;
    event.revision = id->revision; event.address = id->address;
    event.memberships = kind == EventKind::SourceMembershipSnapshot ? result : id->memberships;
    event.domain = id->domain; event.lifecycle = id->lifecycle; event.readiness = id->readiness;
    event.ordinal = id->ordinal;
    if (id->ordinal < anchors_.size()) {
      const auto &anchor = anchors_[id->ordinal];
      event.session = anchor.session; event.participant_generation = anchor.generation;
      event.assignment_key = anchor.key; event.device = anchor.device;
    }
  }
  if (call) {
    event.call = call->id; event.parent = call->parent;
    event.observation = static_cast<std::uint32_t>(call->observation);
    if (!id) event.address = call->object;
    event.args = call->args; event.arg_count = call->count;
    event.slot_before = call->before; event.slot_after = call->body_after;
  } else if (id && kind == EventKind::SourceMembershipSnapshot) {
    event.slot_before = id->memberships; event.slot_after = result;
  }
  if (!storage_.PublishEvent(event)) {
    Fault(storage_.Diag().FaultReason() == Reason::None ? Reason::EventOverflow
                  : storage_.Diag().FaultReason(), "lifecycle_event");
    return false;
  }
  return true;
}
void Collector::Incomplete(Reason reason) noexcept {
  if (!pending_) return;
  if (auto *f = storage_.Faults(); f && f->applied.load()) {
    const auto mode = f->mode.load();
    if (((mode == 2 || mode == 3) && reason == Reason::Budget) ||
        (mode == 4 && reason == Reason::RelationBudget) ||
        (mode == 5 && reason == Reason::ReadFailed) ||
        (mode == 6 && reason == Reason::SourceChanged) ||
        (mode == 7 && reason == Reason::RootChanged) ||
        (mode == 8 && reason == Reason::NotReady))
      f->rejection.store(static_cast<std::uint32_t>(reason));
  }
  auto &header = pending_.Header();
  header.complete = 0;
  if (!header.reason) header.reason = static_cast<std::uint32_t>(reason);
}

// 所有窗口值在 Manager 最外层重新建立，不能继承上一轮的阶段或 Authority。
void Collector::BeginOuter(std::uint64_t loop, std::uint64_t outer, bool run) noexcept {
  if (!Recording()) { CloseIfDrained(); return; }
  if (GetCurrentThreadId() != memory_.main_thread) {
    Fault(Reason::WrongThread, "outer_thread"); return;
  }
  if (outer_active_ || pending_ || depth_ || lifecycle_depth_ || !outer || !loop) {
    Fault(Reason::UnpairedScope, "outer_begin"); return;
  }
  if (attempt_ == UINT64_MAX || (run && !run_active_ && run_ == UINT64_MAX) ||
      (lifecycle_episode_ && room_ == UINT64_MAX)) {
    Fault(Reason::SequenceExhausted, "outer_sequence"); return;
  }
  outer_active_ = true; prepared_ = false; polluted_ = false;
  loop_ = loop; outer_ = outer; authority_ = {}; authority_count_ = 0;
  phases_ = {}; outer_reason_ = Reason::None; post_ = {};
  const auto previous_roots = roots_;
  const bool previous_run = run_active_;
  if (run && !run_active_) {
    ++run_; identities_.ResetAssociations();
    anchors_ = {}; run_ended_ = false; lifecycle_episode_ = true;
  } else if (!run && run_active_ && !run_ended_ && !run_start_pending_) {
    LifecycleEnter(true); LifecycleLeave(true);
  }
  if (run) run_start_pending_ = false;
  run_active_ = run || run_start_pending_;
  if (!ReadRoots(roots_) || (run && !roots_.room))
    outer_reason_ = Reason::ReadFailed;
  if (previous_run && run && previous_roots != roots_ && !lifecycle_episode_) {
    LifecycleEnter(); LifecycleLeave(true);
  }
  if (lifecycle_episode_) {
    if (room_ == UINT64_MAX) { Fault(Reason::SequenceExhausted, "room_epoch"); return; }
    ++room_; lifecycle_episode_ = false;
  }
  if (!run) outer_reason_ = Reason::NotReady;
}
void Collector::EnterStage(Stage stage, std::uint64_t scope,
                            std::uint32_t object) noexcept {
  if (!Recording() || !outer_active_) return;
  const auto index = static_cast<std::size_t>(stage);
  if (index == 0 || index >= phases_.size()) {
    Fault(Reason::UnpairedScope, "stage_index"); return;
  }
  auto &phase = phases_[index];
  const bool order = stage == Stage::Owner ? !phases_[2].entered && !phases_[3].entered
      : stage == Stage::Room ? phases_[1].active && !phases_[3].entered
      : phases_[1].active && phases_[2].returned == 1 && !phases_[2].active;
  if (!scope || phase.active || phase.entered || !order) {
    polluted_ = true; outer_reason_ = Reason::PhaseIncomplete; return;
  }
  phase.scope = scope; phase.object = object; phase.active = true; ++phase.entered;
  if (stage == Stage::Room && object != roots_.room) {
    polluted_ = true; outer_reason_ = Reason::RootChanged;
  }
  if (stage == Stage::Owner && object != roots_.game) {
    polluted_ = true; outer_reason_ = Reason::RootChanged;
  }
}
void Collector::LeaveStage(Stage stage, std::uint64_t scope, bool normal) noexcept {
  if (!active_ || !outer_active_) return;
  const auto index = static_cast<std::size_t>(stage);
  if (!index || index >= phases_.size()) return;
  auto &phase = phases_[index];
  if (!phase.active || phase.scope != scope || !normal) {
    polluted_ = true; outer_reason_ = Reason::UnpairedScope;
  } else ++phase.returned;
  phase.active = false;
}
void Collector::NativeReturned(const Authority &authority) noexcept {
  if (!active_ || !outer_active_) return;
  ++authority_count_;
  if (authority_count_ != 1 || authority.loop != loop_ || authority.outer != outer_ ||
      !authority.tick || !authority.scope || authority.kind == AuthorityKind::None) {
    Fault(Reason::UnpairedScope, "authority_candidate"); return;
  }
  authority_ = authority;
}
void Collector::RunBeginning() noexcept {
  if (!Recording()) return;
  if (run_ == UINT64_MAX) { Fault(Reason::SequenceExhausted, "run_epoch"); return; }
  ++run_; identities_.ResetAssociations(); anchors_ = {};
  run_active_ = true; run_start_pending_ = true; run_ended_ = false;
  run_start_owner_ = true;
  lifecycle_episode_ = true;
}
void Collector::RunOwnerReturned(bool normal) noexcept {
  run_start_owner_ = false;
  if (normal) run_ended_ = false;
  else Fault(Reason::UnpairedScope, "start_owner_exception");
}
void Collector::LifecycleEnter(bool run_end) noexcept {
  if (!Recording()) return;
  if (lifecycle_depth_ == UINT32_MAX || mutation_ == UINT64_MAX) {
    Fault(Reason::SequenceExhausted, "lifecycle_sequence"); return;
  }
  ++lifecycle_depth_; ++mutation_; polluted_ = true;
  outer_reason_ = Reason::Lifecycle; lifecycle_episode_ = true;
  RevokePlayer(0); RevokePlayer(1);
  if (run_end && !run_ended_) {
    run_ended_ = true;
    // Start 内清理旧 Game 的 Exit 不能取消尚在创建的新 run 观察域。
    run_start_pending_ = run_start_owner_; run_active_ = run_start_owner_;
    identities_.ForEach([](Identity &id) noexcept {
      id.lifecycle = static_cast<std::uint32_t>(Lifecycle::Retiring);
      id.readiness = 0; id.memberships = 0; id.ClearOrdinal();
    });
  }
  Event(EventKind::LifecycleBoundary, Reason::Lifecycle, nullptr, nullptr, true);
}
void Collector::LifecycleLeave(bool normal) noexcept {
  if (!active_) return;
  if (!lifecycle_depth_) { Fault(Reason::UnpairedScope, "lifecycle_return"); return; }
  --lifecycle_depth_;
  if (!lifecycle_depth_ && normal && Recording()) {
    // ManagerRestore 先于 GameRestore 的 PostLevelInit 返回；新取得对象的出生
    // 延迟到最外层恢复/加载 owner 正常结束，既存 Player 不因此补出生。
    identities_.ForEach([&](Identity &id) noexcept {
      if (id.candidate && id.owner_scope == UINT64_MAX) {
        id.owner_scope = 0;
        Birth(id, Observation::ManagerRestore);
      }
    });
  }
  Event(EventKind::LifecycleBoundary, normal ? Reason::Lifecycle : Reason::UnpairedScope,
        nullptr, nullptr, false, !normal);
  if (!normal) Fault(Reason::UnpairedScope, "lifecycle_exception");
  CloseIfDrained();
}
void Collector::RevokePlayer(std::uint32_t ordinal) noexcept {
  if (!active_ || ordinal >= anchors_.size()) return;
  auto *id = identities_.RevokeCurrentWitness(ordinal);
  if (id) Event(EventKind::CurrentPlayerReadWitness, Reason::NotReady, id);
  identities_.AnchorPlayer(ordinal, 0); anchors_[ordinal] = {};
}
void Collector::AnchorPlayer(std::uint32_t ordinal, const PlayerAnchor &value) noexcept {
  if (!Recording() || ordinal >= anchors_.size()) return;
  if (!value.valid || !value.player || !value.device || value.key < 0 ||
      !value.generation || !value.session ||
      (anchors_[1 - ordinal].valid && anchors_[1 - ordinal].player == value.player)) {
    RevokePlayer(ordinal); return;
  }
  if (anchors_[ordinal] == value) return;
  RevokePlayer(ordinal);
  if (!identities_.AnchorPlayer(ordinal, value.player).valid) {
    Fault(Reason::Budget, "anchor_identity"); return;
  }
  anchors_[ordinal] = value;
}
void Collector::PlayerReturned(std::uint32_t ordinal, const PlayerAnchor &before,
                               const PlayerAnchor &after, std::uint64_t tick,
                               bool normal, bool reached) noexcept {
  if (!Recording() || ordinal >= anchors_.size()) return;
  AnchorPlayer(ordinal, after);
  if (!normal || !reached || !tick || !outer_active_ || polluted_ ||
      !before.valid || before != after || anchors_[ordinal] != before) {
    RevokePlayer(ordinal); return;
  }
  // 依据 2 的 Player 也须在重绑后重建 +177 当前值资格，不把原出生当作本代次 witness。
  auto *id = identities_.Find(ObjectDomain::Entity, before.player);
  const bool rebuilt = id && !id->current_public_ready;
  auto *witness = identities_.MarkCurrentWitness(ordinal);
  if (id && !id->Retired()) {
    id->current_public_ready = true;
    if (witness || rebuilt) Event(EventKind::CurrentPlayerReadWitness, Reason::None, id,
        nullptr, false, false, 0, tick);
  }
}

// RTTI 只读分类：验证 COL/CHD/每个 BCD 的地址、签名与非虚单继承形状。
// 类型标识只匹配当前清单 TD，不以 Type 数值范围或 vtable Hook 字节猜分类。
Class Collector::Classify(ObjectDomain domain, std::uint32_t address,
                           std::uint32_t &vptr, bool &base_known) noexcept {
  base_known = false;
  if ((address & 3u) || !Value(address, vptr) || vptr < 4 || (vptr & 3u) ||
      !Image(vptr - 4, 8)) return Class::Unknown;
  std::uint32_t locator = 0, col[5]{}, chd[4]{};
  const auto &c = layout::LayoutConstants();
  if (!Value(vptr - 4, locator) || !Image(locator, sizeof(col)) ||
      !Read(locator, col, sizeof(col)) || col[0] != c.rtti_expected_signature ||
      col[1] || col[2] || !Image(col[3], 8) || !Image(col[4], sizeof(chd)) ||
      !Read(col[4], chd, sizeof(chd)) || chd[0] != c.rtti_expected_signature ||
      chd[1] != c.rtti_expected_hierarchy_attributes || !chd[2] || chd[2] > 16 ||
      !Image(chd[3], chd[2] * 4)) return Class::Unknown;
  std::size_t count = 0;
  const auto *classes = layout::Classes(count);
  const auto root = domain == ObjectDomain::Grid ? layout::RttiClass::Grid
                                                : layout::RttiClass::Entity;
  const layout::ClassSpec *exact = nullptr, *root_spec = nullptr;
  for (std::size_t i = 0; i < count; ++i) {
    if (classes[i].rtti == root) root_spec = &classes[i];
    if (col[3] - memory_.image_base == classes[i].td_rva) exact = &classes[i];
  }
  if (!root_spec) return Class::Unknown;
  bool found_root = false;
  for (std::uint32_t i = 0; i < chd[2]; ++i) {
    std::uint32_t pointer = 0, bcd[6]{};
    if (!Value(std::uint64_t(chd[3]) + i * 4, pointer) ||
        !Image(pointer, sizeof(bcd)) || !Read(pointer, bcd, sizeof(bcd)) ||
        !Image(bcd[0], 8) || bcd[1] >= chd[2] || bcd[2] || bcd[3] != UINT32_MAX ||
        bcd[4] || bcd[5] != c.rtti_expected_base_attributes ||
        (!i && bcd[0] != col[3])) return Class::Unknown;
    if (bcd[0] - memory_.image_base == root_spec->td_rva) found_root = true;
    if (exact && (chd[2] != exact->base_count ||
                  bcd[0] - memory_.image_base != exact->bases[i].td_rva))
      return Class::Unknown;
  }
  base_known = found_root;
  if (!found_root || !exact) return Class::Unknown;
  if (domain == ObjectDomain::Grid) return Class::Grid;
  return exact->schema ? exact->schema_class : Class::Unknown;
}

// 主线程先枚举完整来源、合并别名，再复制对象和解析引用。来源缺失不提交成员退出。
std::int32_t Collector::Add(ObjectDomain domain, std::uint32_t address,
                            std::uint32_t source, std::uint32_t index) noexcept {
  if (!address || !pending_) return -1;
  auto &h = pending_.Header();
  ++h.required_memberships;
  auto *id = Find(domain, address);
  if (!id) { Incomplete(Reason::Budget); return -1; }
  id->MarkFirstObserved(); identities_.AddMembership(*id, source);
  auto object_index = identities_.BatchObjectIndex(*id);
  if (object_index == -2) return -1;
  if (object_index < 0) {
    ++h.required_objects;
    const auto *faults = storage_.Faults();
    if (h.object_count >= (faults ? faults->object_limit : kObjectBudget)) {
      if (faults && faults->mode.load() == 2) {
        auto *f = storage_.Faults(); f->before.store(f->object_limit); f->after.store(h.required_objects); f->applied.store(1);
      }
      h.sources[source].status = static_cast<std::uint32_t>(Reason::Budget);
      identities_.SetBatchObjectIndex(*id, -2); Incomplete(Reason::Budget); return -1;
    }
    object_index = static_cast<std::int32_t>(h.object_count++);
    identities_.SetBatchObjectIndex(*id, object_index);
    auto &object = pending_.Objects()[object_index];
    object.Reset(); object.address = address; object.domain = static_cast<std::uint32_t>(domain);
  }
  if (h.membership_count >= (storage_.Faults() ? storage_.Faults()->membership_limit : kMembershipBudget)) {
    if (auto *f = storage_.Faults(); f && f->mode.load() == 3) {
      f->before.store(f->membership_limit); f->after.store(h.required_memberships); f->applied.store(1);
    }
    h.sources[source].status = static_cast<std::uint32_t>(Reason::Budget);
    Incomplete(Reason::Budget); return object_index;
  }
  pending_.Memberships()[h.membership_count++] = {
      static_cast<std::uint32_t>(object_index), source, index, 0};
  return object_index;
}
void Collector::CopySource(std::uint32_t source) noexcept {
  auto &out = pending_.Header().sources[source];
  const auto *spec = layout::FindSource(source);
  if (!spec) { out.status = static_cast<std::uint32_t>(Reason::NotReady); Incomplete(Reason::NotReady); return; }
  if (spec->kind == layout::SourceKind::Reserved) { out.status = kSourceNotApplicable; return; }
  if (spec->kind == layout::SourceKind::Relation) return;
  const auto base = std::strcmp(spec->base, "game") == 0 ? roots_.game : roots_.room;
  const auto descriptor = std::uint64_t(base) + spec->offset;
  std::uint32_t data = 0, count = 0, bytes = 0;
  const auto fail = [&](Reason reason) noexcept {
    out.status = static_cast<std::uint32_t>(reason); Incomplete(reason);
  };
  if (spec->kind == layout::SourceKind::EntityList || spec->kind == layout::SourceKind::Roster) {
    bytes = spec->kind == layout::SourceKind::Roster ? 12u : 16u;
    if (!Read(descriptor, out.descriptor.data(), bytes)) { fail(Reason::ReadFailed); return; }
    if (spec->kind == layout::SourceKind::Roster) {
      const auto begin = out.descriptor[0], end = out.descriptor[1], capacity = out.descriptor[2];
      if (begin || end || capacity) {
        if (!begin || begin > end || end > capacity || ((begin | end | capacity) & 3u)) {
          fail(Reason::ReadFailed); return;
        }
        data = begin; count = (end - begin) / 4;
      }
    } else {
      data = out.descriptor[1]; count = out.descriptor[3];
      if (count > out.descriptor[2] || (count && !data) || (data & 3u)) {
        fail(Reason::ReadFailed); return;
      }
    }
  } else {
    if (descriptor > UINT32_MAX) { fail(Reason::ReadFailed); return; }
    data = static_cast<std::uint32_t>(descriptor); count = spec->count;
  }
  out.count = count;
  if (count > kMembershipBudget || std::uint64_t(data) + std::uint64_t(count) * 4 > UINT32_MAX) {
    fail(Reason::Budget); return;
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    std::uint32_t address = 0;
    if (!Value(std::uint64_t(data) + i * 4, address)) { fail(Reason::ReadFailed); return; }
    source_addresses_[i] = address;
    if (spec->kind == layout::SourceKind::FixedArray)
      fixed_addresses_[(source == 9 ? 0u : source == 11 ? 4u : 452u) + i] = address;
    if (!address) {
      if (spec->kind != layout::SourceKind::FixedArray) fail(Reason::NotReady);
      continue;
    }
    const auto domain = source == 11 || source == 12 ? ObjectDomain::Grid : ObjectDomain::Entity;
    const auto object = Add(domain, address, source, i);
    if (object >= 0 && domain == ObjectDomain::Grid) {
      std::uint32_t index = i;
      if (source == 12 && (!Value(std::uint64_t(roots_.room) + spec->index_map_offset + i * 4, index) ||
                          index >= layout::LayoutConstants().room_grid_slot_count)) {
        fail(Reason::NotReady); continue;
      }
      auto &value = pending_.Objects()[object];
      if (value.grid_index != kNoGridIndex && value.grid_index != index) fail(Reason::SourceChanged);
      value.grid_index = index;
    }
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    std::uint32_t value = 0;
    if (!Value(std::uint64_t(data) + i * 4, value) || value != source_addresses_[i]) {
      fail(Reason::SourceChanged); return;
    }
  }
  if (bytes) {
    std::uint32_t after[4]{};
    if (!Read(descriptor, after, bytes) || std::memcmp(after, out.descriptor.data(), bytes))
      fail(Reason::SourceChanged);
  }
}
void Collector::CopySources() noexcept {
  for (std::uint32_t source = 0; source < kSourceCount && Recording(); ++source) CopySource(source);
  // 只从分类且有当前读取依据的 Player 扩展四种关系，64 节点含初始和保留节点。
  auto &relations = pending_.Header().sources[10];
  std::uint32_t count = 0;
  for (std::uint32_t i = 0; i < pending_.Header().object_count && Recording(); ++i) {
    const auto address = pending_.Objects()[i].address;
    auto *id = identities_.Find(ObjectDomain::Entity, address);
    if (!id || pending_.Objects()[i].domain != 0) continue;
    std::uint32_t vptr = 0; bool base = false;
    if (Classify(ObjectDomain::Entity, address, vptr, base) != Class::Player) continue;
    if (count >= (storage_.Faults() ? storage_.Faults()->relation_limit : kRelationNodes)) {
      if (auto *f = storage_.Faults(); f && f->mode.load() == 4) {
        f->before.store(f->relation_limit); f->after.store(count + 1); f->applied.store(1);
      }
      relations.status = static_cast<std::uint32_t>(Reason::RelationBudget); Incomplete(Reason::RelationBudget); break;
    }
    relation_nodes_[count++] = address;
    if (id->readiness < 2 || id->Retired()) {
      relations.status = static_cast<std::uint32_t>(Reason::NotReady); Incomplete(Reason::NotReady); continue;
    }
    for (std::uint16_t f = 0x010d; f <= 0x0110; ++f) {
      const auto *spec = layout::FindField(f);
      std::uint32_t target = 0, after = 0;
      if (!spec || !Value(std::uint64_t(address) + spec->offset, target)) {
        relations.status = static_cast<std::uint32_t>(Reason::ReadFailed); Incomplete(Reason::ReadFailed); continue;
      }
      if (target) Add(ObjectDomain::Entity, target, 10, (count - 1) * 4 + f - 0x010d);
      if (!Value(std::uint64_t(address) + spec->offset, after) || after != target) {
        relations.status = static_cast<std::uint32_t>(Reason::SourceChanged); Incomplete(Reason::SourceChanged);
      }
    }
  }
  relations.count = count;
}
void Collector::CopyFields(RawObject &object, const Identity &identity, bool ready) noexcept {
  const auto kind = static_cast<Class>(object.kind);
  for (const auto id : ProfileFieldsFor(kind)) {
    if (object.field_count == kFieldBudget) { Incomplete(Reason::Budget); break; }
    auto &field = object.fields[object.field_count++]; field.Reset();
    const auto *spec = layout::FindField(id);
    const auto *declaration = ProfileField(id);
    if (!spec || !declaration) { Incomplete(Reason::InvalidRecord); continue; }
    field.id = id; field.width = spec->width; field.encoding = spec->encoding;
    field.status = FieldStatus::Value;
    // 派生字段不能凭来源存在取得资格。Grid 由独立初始化标志和槽关联判定。
    if (!ready || identity.Retired() || (id >= 0x0101 && id < 0x0800 && identity.readiness < 2)) {
      field.status = FieldStatus::NotReady; Incomplete(Reason::NotReady); continue;
    }
    if (spec->status == layout::FieldStatus::InitUnknown) {
      field.status = FieldStatus::Unconfirmed; Incomplete(Reason::NotReady); continue;
    }
    // 用户于 2026-10-27 采纳：初始化写入与本代当前字节的读取资格分别表达。
    // FIRST_OBSERVED 本身无资格；最终来源/root 复核失败仍撤销整批完整性。
    if (id == 0x000a && (!identity.current_public_ready || identity.readiness < 2 || polluted_)) {
      field.status = FieldStatus::NotReady; Incomplete(Reason::NotReady); continue;
    }
    bool ok = true;
    if (spec->base == layout::FieldBase::Derived) {
      if (id == 0x001e) {
        std::uint32_t stamp = 0;
        ok = Value(std::uint64_t(object.address) + layout::LayoutConstants().entity_frame_stamp_offset, stamp);
        const auto value = game_frame_ - stamp;
        if (ok) std::memcpy(field.raw.data(), &value, 4);
      } else if (id == 0x0810 || id == 0x0811) {
        if (!grid_width_ || object.grid_index == kNoGridIndex) {
          field.status = FieldStatus::NotReady; Incomplete(Reason::NotReady); continue;
        }
        const auto &c = layout::LayoutConstants();
        const float value = id == 0x0810
            ? c.grid_position_step * float(object.grid_index % grid_width_) + c.grid_position_origin_x
            : c.grid_position_step * float(object.grid_index / grid_width_) + c.grid_position_origin_y;
        std::memcpy(field.raw.data(), &value, 4);
      } else ok = false;
    } else {
      const auto base = spec->base == layout::FieldBase::Game ? roots_.game : object.address;
      auto address = std::uint64_t(base) + spec->offset;
      if (auto *f = storage_.Faults(); f && f->pending == 5) {
        // 只替换本次读取的源为自有不可读页，SEH 路径与正常 Read 完全相同。
        f->before.store(static_cast<std::uint32_t>(address)); f->after.store(f->unreadable);
        address = f->unreadable; f->pending = 0;
        ok = Read(address, field.raw.data(), field.width); f->applied.store(!ok);
      } else ok = Read(address, field.raw.data(), field.width);
    }
    if (!ok) { field.raw = {}; field.status = FieldStatus::ReadFailed; Incomplete(Reason::ReadFailed); }
    else if (field.encoding == Encoding::F32 && !Finite(field)) Incomplete(Reason::Nonfinite);
  }
}
void Collector::CopyObject(RawObject &out, Identity &id) noexcept {
  out.local_id = id.local_id; out.use = id.use; out.birth = id.birth; out.revision = id.revision;
  out.origin = id.origin; out.lifecycle = id.lifecycle; out.readiness = id.readiness;
  out.birth_kind = id.birth_kind; out.birth_type = id.birth_type; out.birth_variant = id.birth_variant;
  out.birth_subtype = id.birth_subtype; out.birth_seed = id.birth_seed; out.birth_known = id.born;
  out.ordinal = id.ordinal;
  bool base = false;
  const auto kind = Classify(static_cast<ObjectDomain>(out.domain), out.address, out.vptr, base);
  out.kind = static_cast<std::uint32_t>(kind);
  if (kind == Class::Unknown) {
    Incomplete(Reason::UnknownClass);
    if (!base || out.domain != 0) return;
  }
  bool values_ready = true;
  if (kind == Class::Grid) {
    const auto *initialized = layout::FindField(0x0807);
    std::uint8_t ready = 0; std::uint32_t index = kNoGridIndex;
    const auto *index_spec = layout::FindField(0x0809);
    if (!initialized || !index_spec || !Value(std::uint64_t(out.address) + initialized->offset, ready) ||
        !Value(std::uint64_t(out.address) + index_spec->offset, index)) {
      Incomplete(Reason::ReadFailed); CopyFields(out, id, false); return;
    }
    if (!ready || index >= layout::LayoutConstants().room_grid_slot_count || index != out.grid_index)
      { Incomplete(Reason::NotReady); values_ready = false; }
  }
  CopyFields(out, id, values_ready);
}
void Collector::ResolveReferences() noexcept {
  for (std::uint32_t i = 0; i < pending_.Header().object_count; ++i) {
    auto &object = pending_.Objects()[i];
    const auto *requester = identities_.Find(static_cast<ObjectDomain>(object.domain), object.address);
    if (!requester) continue;
    for (std::uint32_t j = 0; j < object.field_count; ++j) {
      auto &field = object.fields[j];
      if (field.encoding != Encoding::Reference || field.status != FieldStatus::Value) continue;
      std::uint32_t address = 0; std::memcpy(&address, field.raw.data(), 4);
      const auto result = identities_.ResolveReference(address, *requester);
      field.relation = result.relation; field.target_id = result.local_id; field.target_use = result.use;
      if (auto *f = storage_.Faults(); address && f && f->pending == 8) {
        f->before.store(static_cast<std::uint32_t>(result.relation));
        field.relation = Relation::AmbiguousReuse; field.target_id = 0; field.target_use = 0;
        f->after.store(static_cast<std::uint32_t>(field.relation)); f->pending = 0; f->applied.store(1);
        Incomplete(Reason::NotReady);
      }
    }
  }
}
void Collector::Memberships(bool commit) noexcept {
  if (!pending_) return;
  for (const auto &source : pending_.Header().sources)
    if (source.status && source.status != kSourceNotApplicable) return;
  identities_.ForEach([&](Identity &id) noexcept {
    const auto mask = identities_.MembershipMask(id);
    if (identities_.MembershipChanged(id, mask)) {
      if (commit) identities_.CommitMembership(id, mask);
      else Event(EventKind::SourceMembershipSnapshot, Reason::None, &id, nullptr, false, false, mask);
    }
  });
}

void Collector::Gap(Reason reason, bool authority) noexcept {
  GapRecord gap{};
  gap.attempt = attempt_; gap.run = run_; gap.room = room_; gap.mutation = mutation_;
  gap.outer = outer_; gap.tick = authority ? authority_.tick : 0;
  gap.scope = authority ? authority_.scope : 0; gap.event_watermark = storage_.EventWatermark();
  gap.recording_generation = generation_; gap.reason = reason;
  gap.thread = GetCurrentThreadId();
  gap.authority_kind = authority ? static_cast<std::uint32_t>(authority_.kind) : 0;
  if (!storage_.PublishGap(gap)) Fault(Reason::EventOverflow, "gap_publish");
}
core::TickPostState Collector::PreparePostState(bool normal) noexcept {
  if (!active_ || !outer_active_) return {};
  if (prepared_) { Fault(Reason::UnpairedScope, "duplicate_prepare"); return post_; }
  prepared_ = true;
  if (attempt_ == UINT64_MAX) { Fault(Reason::SequenceExhausted, "attempt"); return {}; }
  ++attempt_;
  post_ = {}; post_.tick = authority_.tick; post_.outer_token = outer_;
  post_.recording_generation = generation_; post_.attempt_seq = attempt_;
  const bool authority = authority_count_ == 1 && authority_.tick;
  Reason reason = outer_reason_;
  if (!normal || depth_ || lifecycle_depth_) reason = Reason::UnpairedScope;
  else if (stopping_ || !Recording()) reason = storage_.Diag().FaultReason() == Reason::None ? Reason::Stopped : storage_.Diag().FaultReason();
  else if (authority && authority_.kind == AuthorityKind::OddPlayer && !polluted_ && reason == Reason::None) reason = Reason::OddOnly;
  else if (!authority) reason = reason == Reason::None ? Reason::PhaseIncomplete : reason;
  else if (reason == Reason::None) {
    for (std::size_t i = 1; i < phases_.size(); ++i)
      if (phases_[i].entered != 1 || phases_[i].returned != 1 || phases_[i].active)
        reason = Reason::PhaseIncomplete;
    if (authority_.scope != phases_[1].scope) reason = Reason::UnpairedScope;
  }
  post_.status = storage_.Diag().FaultReason() != Reason::None ? core::PostStateStatus::Faulted
      : stopping_ ? core::PostStateStatus::Unavailable : core::PostStateStatus::Gap;
  if (reason != Reason::None) {
    storage_.Diag().attempts.fetch_add(1, std::memory_order_relaxed);
    storage_.Diag().last_attempt.store(attempt_, std::memory_order_relaxed);
    post_.reason = static_cast<std::uint32_t>(reason);
    Gap(reason, authority); return post_;
  }
  BatchBegin begin{};
  begin.attempt = attempt_; begin.run = run_; begin.room = room_; begin.mutation = mutation_;
  begin.outer = outer_; begin.tick = authority_.tick; begin.scope = authority_.scope;
  begin.recording_generation = generation_; begin.authority = true;
  begin.authority_kind = authority_.kind; begin.event_watermark = storage_.EventWatermark();
  pending_ = storage_.Begin(begin);
  if (!pending_) {
    post_.status = Recording() ? core::PostStateStatus::Backpressure : core::PostStateStatus::Faulted;
    post_.reason = static_cast<std::uint32_t>(Recording() ? Reason::Backpressure : storage_.Diag().FaultReason());
    return post_;
  }
  auto &h = pending_.Header(); h.complete = 1;
  h.loop = loop_;
  for (std::size_t i = 1; i < phases_.size(); ++i) {
    h.phase_scopes[i - 1] = phases_[i].scope;
    h.phase_entered[i - 1] = phases_[i].entered;
    h.phase_returned[i - 1] = phases_[i].returned;
  }
  if (capture_ == UINT64_MAX) { Fault(Reason::SequenceExhausted, "capture"); Incomplete(Reason::SequenceExhausted); }
  else h.capture = ++capture_;
  identities_.BeginBatch(attempt_);
  const auto &c = layout::LayoutConstants();
  if (!Value(std::uint64_t(roots_.game) + c.game_frame_offset, game_frame_) ||
      !Value(std::uint64_t(roots_.room) + c.room_grid_width_offset, grid_width_)) Incomplete(Reason::ReadFailed);
  CopySources();
  bool stable_sources = true;
  for (const auto &source : h.sources)
    if (source.status && source.status != kSourceNotApplicable) stable_sources = false;
  for (std::uint32_t i = 0; i < h.object_count && Recording(); ++i) {
    auto &object = pending_.Objects()[i];
    auto *id = identities_.Find(static_cast<ObjectDomain>(object.domain), object.address);
    if (id) CopyObject(object, *id);
  }
  ResolveReferences();
  // 在整个复制尾部再复读所有动态 descriptor 和固定槽/door 映射。
  // 源内的两遍指针检查之外，再覆盖复制其他来源/字段期间的描述变化。
  for (std::uint32_t source = 0; source < kSourceCount && Recording(); ++source) {
    const auto *spec = layout::FindSource(source);
    if (!spec || spec->kind == layout::SourceKind::Relation || spec->kind == layout::SourceKind::Reserved) continue;
    const auto base = std::strcmp(spec->base, "game") == 0 ? roots_.game : roots_.room;
    const auto &saved = h.sources[source];
    if (spec->kind == layout::SourceKind::EntityList || spec->kind == layout::SourceKind::Roster) {
      std::uint32_t descriptor[4]{};
      const std::size_t bytes = spec->kind == layout::SourceKind::Roster ? 12 : 16;
      auto expected = saved.descriptor;
      if (auto *f = storage_.Faults(); f && f->pending == 6) {
        f->before.store(expected[0]); expected[0] ^= 1u; f->after.store(expected[0]);
        f->pending = 0; f->applied.store(1);
      }
      if (!Read(std::uint64_t(base) + spec->offset, descriptor, bytes) ||
          std::memcmp(descriptor, expected.data(), bytes)) {
        stable_sources = false; Incomplete(Reason::SourceChanged);
      }
    } else {
      for (std::uint32_t i = 0; i < spec->count; ++i) {
        std::uint32_t now = 0;
        if (!Value(std::uint64_t(base) + spec->offset + i * 4, now)) { stable_sources = false; Incomplete(Reason::ReadFailed); continue; }
        const auto original = fixed_addresses_[(source == 9 ? 0u : source == 11 ? 4u : 452u) + i];
        if (original != now) { stable_sources = false; Incomplete(Reason::SourceChanged); }
        if (source == 12 && now) {
          std::uint32_t index = 0;
          auto *id = identities_.Find(ObjectDomain::Grid, now);
          const auto slot = id ? identities_.BatchObjectIndex(*id) : -1;
          if (!Value(std::uint64_t(base) + spec->index_map_offset + i * 4, index) || slot < 0 ||
              pending_.Objects()[slot].grid_index != index) { stable_sources = false; Incomplete(Reason::SourceChanged); }
        }
      }
    }
  }
  Roots after{};
  auto expected = roots_;
  if (auto *f = storage_.Faults(); f && f->pending == 7) {
    f->before.store(expected.index); expected.index ^= 1u; f->after.store(expected.index);
    f->pending = 0; f->applied.store(1);
  }
  if (!ReadRoots(after) || after != expected || polluted_) {
    stable_sources = false; Incomplete(Reason::RootChanged);
  }
  if (!stable_sources) {
    // 前后复核拒绝时撤销本批 +177 的当前值资格，原始读取不冒充可用字段。
    RevokePlayer(0); RevokePlayer(1);
    for (std::uint32_t i = 0; i < h.object_count; ++i) {
      auto &object = pending_.Objects()[i];
      if (auto *id = identities_.Find(static_cast<ObjectDomain>(object.domain), object.address))
        id->current_public_ready = false;
      for (std::uint32_t f = 0; f < object.field_count; ++f)
        if (object.fields[f].id == 0x000a && object.fields[f].status == FieldStatus::Value)
          object.fields[f].status = FieldStatus::NotReady;
    }
  }
  Memberships(false);
  h.event_watermark = storage_.EventWatermark();
  if (!Recording()) Incomplete(storage_.Diag().FaultReason());
  post_.reason = h.reason; post_.capture_seq = h.capture;
  post_.status = storage_.Diag().FaultReason() != Reason::None ? core::PostStateStatus::Faulted
      : h.complete ? core::PostStateStatus::Complete : core::PostStateStatus::Gap;
  if (!pending_.Prepare()) { Fault(Reason::UnpairedScope, "batch_prepare"); post_.status = core::PostStateStatus::Faulted; }
  else {
    storage_.Diag().captures.fetch_add(1, std::memory_order_relaxed);
    storage_.Diag().last_capture.store(h.capture, std::memory_order_relaxed);
  }
  return post_;
}
void Collector::PublishPostState(bool committed, const core::TickPostState &post) noexcept {
  if (!pending_) return;
  const bool match = post.tick == post_.tick && post.outer_token == outer_ &&
      post.recording_generation == generation_ && post.attempt_seq == attempt_ &&
      post.capture_seq == post_.capture_seq && post.status == post_.status && post.reason == post_.reason;
  if (!committed || !match) {
    // 析构租约会产生 UNPAIRED_SCOPE gap，不静默退回空槽。
    pending_ = {}; if (!match) Fault(Reason::UnpairedScope, "post_pair"); return;
  }
  Memberships(true);
  pending_.Header().event_watermark = storage_.EventWatermark();
  if (storage_.Diag().FaultReason() != Reason::None) Incomplete(storage_.Diag().FaultReason());
  const bool complete = pending_.Header().complete != 0;
  if (!pending_.Release(true)) Fault(Reason::NotificationFailed, "batch_publish");
  else if (complete) storage_.Diag().complete_captures.fetch_add(1, std::memory_order_relaxed);
  else storage_.Diag().gaps.fetch_add(1, std::memory_order_relaxed);
}
void Collector::EndOuter() noexcept {
  if (!outer_active_) { CloseIfDrained(); return; }
  if (pending_) pending_ = {};
  outer_active_ = false;
  CloseIfDrained();
}

std::uint32_t Collector::CreationOwner(ObjectDomain domain) const noexcept {
  // 取真实最外层 owner：嵌套 Spawn/Publish/Init 不能抢先冻结出生。
  for (std::uint32_t i = 0; i < depth_; ++i) {
    const auto kind = calls_[i].observation;
    if (Creation(kind) && (GridObservation(kind) == (domain == ObjectDomain::Grid)))
      return calls_[i].id;
  }
  return 0;
}
bool Collector::Birth(Identity &id, Observation owner) noexcept {
  if (!id.candidate || id.born || id.Retired()) return false;
  std::uint32_t vptr = 0; bool base = false;
  Classify(static_cast<ObjectDomain>(id.domain), id.address, vptr, base);
  if (!base) {
    id.candidate = false; id.readiness = 0;
    Event(EventKind::GapOrUnknown, Reason::UnknownClass, &id); return false;
  }
  BirthValues values{}; values.kind = static_cast<std::uint32_t>(owner);
  const std::uint16_t first = id.domain == 1 ? 0x0801 : 0x0002;
  const std::uint16_t ids[] = {first, static_cast<std::uint16_t>(first + 1),
      static_cast<std::uint16_t>(id.domain == 1 ? 0 : 0x0004),
      static_cast<std::uint16_t>(id.domain == 1 ? 0x0805 : 0x0011)};
  std::uint32_t *destinations[] = {&values.entity_type, &values.variant, &values.subtype, &values.seed};
  for (std::size_t i = 0; i < 4; ++i) {
    if (!ids[i]) continue;
    const auto *field = layout::FindField(ids[i]);
    if (!field || !Value(std::uint64_t(id.address) + field->offset, *destinations[i])) {
      Fault(Reason::ReadFailed, "birth_fields"); return false;
    }
  }
  // 本次取得和完整创建 owner 均被观察；它不授予 FIRST_OBSERVED 对象出生历史。
  id.readiness = static_cast<std::uint32_t>(Readiness::CreationOwnerCompleted);
  const Call *owner_call = nullptr;
  for (std::uint32_t i = 0; i < depth_; ++i)
    if (calls_[i].id == id.owner_scope) owner_call = &calls_[i];
  if (!identities_.ConfirmBirth(id, values, owner)) {
    if (identities_.LastError() != IdentityTable::Error::None)
      Fault(Reason::SequenceExhausted, "birth_sequence");
    return false;
  }
  return Event(EventKind::BirthConfirmed, Reason::None, &id, owner_call);
}
void Collector::CompleteOwner(std::uint32_t call, Observation kind,
                              std::uint32_t result) noexcept {
  identities_.ForEach([&](Identity &id) noexcept {
    if (!Recording() || !id.candidate || id.owner_scope != call) return;
    // Spawn/PlayerCreate 的返回对象必须确实等于该次取得的对象。
    if ((kind == Observation::Spawn || kind == Observation::PlayerCreate) && id.address != result) {
      id.candidate = false; id.owner_scope = 0; id.readiness = 0;
      Event(EventKind::GapOrUnknown, Reason::Continuity, &id); return;
    }
    if (kind == Observation::ManagerRestore && lifecycle_depth_ > 1) {
      id.owner_scope = UINT64_MAX; id.readiness = 0;
    } else Birth(id, kind);
  });
}
std::uint32_t Collector::ObserveEnter(Observation kind, std::uint32_t object,
                                     const std::uint32_t *args, std::uint32_t count) noexcept {
  if (!Recording()) return 0;
  if (depth_ == calls_.size() || next_call_ == UINT32_MAX || count > 8) {
    Fault(Reason::Budget, "observation_stack"); return 0;
  }
  auto &call = calls_[depth_]; call = {};
  call.id = ++next_call_; call.parent = depth_ ? calls_[depth_ - 1].id : 0;
  call.observation = kind; call.object = object; call.count = count;
  for (std::uint32_t i = 0; i < count; ++i) call.args[i] = args[i];
  ++depth_; storage_.Diag().callbacks.fetch_add(1, std::memory_order_relaxed);
  const auto domain = GridObservation(kind) ? ObjectDomain::Grid : ObjectDomain::Entity;
  Identity *id = nullptr;
  const bool direct = kind == Observation::Init || kind == Observation::Remove ||
      kind == Observation::ClearReferences || kind == Observation::Destroy ||
      kind == Observation::PlayerCtor || kind == Observation::PlayerRestore ||
      kind == Observation::GridInit || Morph(kind);
  if (direct) id = Find(domain, object);
  if (kind == Observation::Publish && count) id = Find(ObjectDomain::Entity, args[0]);
  if (kind == Observation::GridSpawn || kind == Observation::GridFree || kind == Observation::Door) {
    const auto &c = layout::LayoutConstants();
    const auto limit = kind == Observation::Door ? c.room_door_slot_count : c.room_grid_slot_count;
    const auto offset = kind == Observation::Door ? c.room_door_slot_offset : c.room_grid_slot_offset;
    if (count && args[0] < limit && !Value(std::uint64_t(object) + offset + args[0] * 4, call.before))
      Fault(Reason::ReadFailed, "grid_before");
  }
  if (kind == Observation::ManagerRestore || kind == Observation::Replace || kind == Observation::PlayerRestore) {
    call.lifecycle = true; LifecycleEnter();
    // Restore/Replace 可以原址改变状态，立即撤销当前出生连续性；不猜新构造。
    identities_.ForEach([&](Identity &value) noexcept {
      bool affected = kind == Observation::ManagerRestore;
      if (kind == Observation::PlayerRestore) affected = value.domain == 0 && value.address == object;
      if (kind == Observation::Replace) affected = value.domain == 0 && count == 2 &&
          (value.address == args[0] || value.address == args[1]);
      if (!affected || value.domain != 0) return;
      std::uint32_t vptr = 0; bool base = false;
      if (kind == Observation::ManagerRestore &&
          Classify(ObjectDomain::Entity, value.address, vptr, base) != Class::Player) return;
      if (value.revision == UINT64_MAX) { Fault(Reason::SequenceExhausted, "restore_revision"); return; }
      ++value.revision; value.readiness = 0; value.birth = 0; value.born = false;
      value.current_public_ready = false;
      value.candidate = false; value.owner_scope = 0;
      value.origin = static_cast<std::uint32_t>(Origin::FirstObserved);
      Event(EventKind::GapOrUnknown, Reason::Continuity, &value, &call, true);
    });
  }
  if (id && Morph(kind)) {
    // 内层 Init 只给基类依据；外层 Morph 正常返回才恢复派生配置读取。
    id->readiness = 0;
    id->current_public_ready = false;
  }
  if (id && (kind == Observation::Init || kind == Observation::GridInit)) {
    bool known_owner = CreationOwner(domain) != 0;
    for (std::uint32_t i = 0; i + 1 < depth_; ++i)
      known_owner = known_owner || (Morph(calls_[i].observation) && calls_[i].object == object);
    // 连续性在进入即撤销，正常返回前不能调用 MarkInitReturned。
    if (!known_owner && !id->candidate) {
      if (id->revision == UINT64_MAX) Fault(Reason::SequenceExhausted, "init_revision");
      else ++id->revision;
      id->born = false; id->birth = 0;
      id->origin = static_cast<std::uint32_t>(Origin::FirstObserved);
      Event(EventKind::GapOrUnknown, Reason::Continuity, id, &call, true);
    }
    id->readiness = 0;
    id->current_public_ready = false;
  }
  Event(EventKind::NativeHook, Reason::None, id, &call, true);
  return call.id;
}
std::uint32_t Collector::NativeEnter(Observation kind, std::uint32_t object,
                                    const std::uint32_t *args, std::uint32_t count) noexcept {
  if (!Recording()) return 0;
  if (!depth_ || calls_[depth_ - 1].observation != kind ||
      calls_[depth_ - 1].object != object || calls_[depth_ - 1].body_reached || count > 8) {
    Fault(Reason::UnpairedScope, "native_body_pair"); return 0;
  }
  auto &call = calls_[depth_ - 1]; call.body_reached = true;
  call.count = count;
  for (std::uint32_t i = 0; i < count; ++i) call.args[i] = args[i];
  if (kind == Observation::GridSpawn || kind == Observation::GridFree || kind == Observation::Door) {
    const auto &c = layout::LayoutConstants();
    const auto limit = kind == Observation::Door ? c.room_door_slot_count : c.room_grid_slot_count;
    const auto offset = kind == Observation::Door ? c.room_door_slot_offset : c.room_grid_slot_offset;
    call.before = 0;
    if (count && args[0] < limit && !Value(std::uint64_t(object) + offset + args[0] * 4, call.before))
      Fault(Reason::ReadFailed, "native_grid_before");
    if (kind == Observation::Door && count && args[0] < limit && !call.before) {
      if (!Value(std::uint64_t(object) + c.room_door_grid_map_offset + args[0] * 4, call.grid_index) ||
          call.grid_index >= c.room_grid_slot_count ||
          !Value(std::uint64_t(object) + c.room_grid_slot_offset + call.grid_index * 4, call.before_grid))
        Fault(Reason::ReadFailed, "door_grid_before");
      if (call.before_grid) {
        std::uint32_t vptr = 0; bool base = false;
        if (Classify(ObjectDomain::Grid, call.before_grid, vptr, base) != Class::Grid)
          Fault(Reason::UnknownClass, "door_grid_type");
        std::size_t size = 0; const auto *classes = layout::Classes(size);
        for (std::size_t i = 0; i < size; ++i)
          if (classes[i].rtti == layout::RttiClass::Door)
            call.before_grid_door = vptr == memory_.image_base + classes[i].vtable_rva;
      }
    }
  }
  if (auto *id = identities_.Find(ObjectDomain::Entity, object)) {
    if (Morph(kind) || kind == Observation::ClearReferences) {
      if (id->revision == UINT64_MAX) Fault(Reason::SequenceExhausted, "native_revision");
      else ++id->revision;
      id->RevokeCurrentWitness();
    }
    if (kind == Observation::Remove) id->MarkRemoveRequested();
    if (kind == Observation::Destroy) {
      id->lifecycle = static_cast<std::uint32_t>(Lifecycle::Retiring); id->readiness = 0;
    }
  }
  Event(EventKind::NativeBody, Reason::None, nullptr, &call, true);
  return call.id;
}
void Collector::NativeLeave(std::uint32_t call_id, std::uint32_t result, bool normal) noexcept {
  if (!call_id) return;
  if (!depth_ || calls_[depth_ - 1].id != call_id || !calls_[depth_ - 1].body_reached) {
    Fault(Reason::UnpairedScope, "native_body_return"); return;
  }
  calls_[depth_ - 1].body_returned = normal;
  calls_[depth_ - 1].body_result = result;
  auto &call = calls_[depth_ - 1];
  if (normal && (call.observation == Observation::GridSpawn ||
                 call.observation == Observation::GridFree || call.observation == Observation::Door)) {
    const auto &c = layout::LayoutConstants();
    const bool door = call.observation == Observation::Door;
    const auto count = door ? c.room_door_slot_count : c.room_grid_slot_count;
    const auto offset = door ? c.room_door_slot_offset : c.room_grid_slot_offset;
    if (call.count && call.args[0] < count) {
      call.body_slot_known = Value(std::uint64_t(call.object) + offset + call.args[0] * 4, call.body_after);
      if (!call.body_slot_known) Fault(Reason::ReadFailed, "native_grid_after");
    }
  }
  Event(EventKind::NativeBody, normal ? Reason::None : Reason::UnpairedScope,
        nullptr, &calls_[depth_ - 1], false, !normal, result);
}
void Collector::ObserveLeave(std::uint32_t call_id, std::uint32_t result, bool normal) noexcept {
  if (!call_id) return;
  if (!depth_ || calls_[depth_ - 1].id != call_id) {
    Fault(Reason::UnpairedScope, "observation_return"); return;
  }
  const auto call = calls_[depth_ - 1];
  const auto kind = call.observation;
  const auto domain = GridObservation(kind) ? ObjectDomain::Grid : ObjectDomain::Entity;
  Identity *id = nullptr;
  const bool completed = normal && call.body_reached && call.body_returned;
  if (completed && !stopping_ && storage_.Ready()) {
    if (kind == Observation::Factory || kind == Observation::GridFactory || kind == Observation::PlayerCtor) {
      const auto address = kind == Observation::PlayerCtor ? call.object : call.body_result;
      id = Find(domain, address);
      if (id) {
        const auto owner = CreationOwner(domain);
        // Factory 包含 ctor 时，同一次真实取得只计一次 use。
        if (!(kind == Observation::Factory && id->candidate && id->owner_scope == owner &&
              id->lifecycle == static_cast<std::uint32_t>(Lifecycle::Constructed))) {
          if (!identities_.Acquire(*id, kind, kind == Observation::PlayerCtor, owner))
            Fault(Reason::SequenceExhausted, "acquire");
        }
        if (kind == Observation::PlayerCtor)
          id->lifecycle = static_cast<std::uint32_t>(Lifecycle::Constructed);
      }
    } else if (kind == Observation::Publish) {
      id = Find(ObjectDomain::Entity, call.args[0]); if (id) id->MarkPublished();
    } else if (kind == Observation::Init || kind == Observation::GridInit || Morph(kind) ||
               kind == Observation::Remove || kind == Observation::ClearReferences ||
               kind == Observation::Destroy || kind == Observation::PlayerRestore) {
      id = identities_.Find(domain, call.object);
      if (id) {
        if (kind == Observation::Init || kind == Observation::GridInit) id->MarkInitReturned();
        if (Morph(kind)) {
          id->readiness = static_cast<std::uint32_t>(Readiness::CreationOwnerCompleted);
          id->current_public_ready = true;
        }
        if (kind == Observation::Destroy) id->MarkDestroyed();
      }
    }
    if (kind == Observation::GridSpawn || kind == Observation::Door || kind == Observation::GridFree) {
      const auto &c = layout::LayoutConstants();
      const auto limit = kind == Observation::Door ? c.room_door_slot_count : c.room_grid_slot_count;
      const auto offset = kind == Observation::Door ? c.room_door_slot_offset : c.room_grid_slot_offset;
      std::uint32_t after = 0;
      if (call.count && call.args[0] < limit) {
        if (!Value(std::uint64_t(call.object) + offset + call.args[0] * 4, after)) Fault(Reason::ReadFailed, "grid_after");
        else if (call.body_slot_known && after != call.body_after) {
          Event(EventKind::GapOrUnknown, Reason::Continuity, nullptr, &call);
        } else if (kind != Observation::GridFree && (kind != Observation::GridSpawn || (result && call.body_result)) && after) {
          id = Find(ObjectDomain::Grid, after);
          if (id && kind == Observation::Door && !call.before && !call.before_grid_door &&
              call.grid_index != kNoGridIndex) {
            std::uint32_t grid = 0;
            if (!Value(std::uint64_t(call.object) + c.room_grid_slot_offset + call.grid_index * 4, grid) || grid != after)
              Fault(Reason::SourceChanged, "door_alias_after");
            else if (!id->candidate && !identities_.Acquire(*id, kind, true, call_id))
              Fault(Reason::SequenceExhausted, "door_acquire");
          }
          if (id && id->candidate && id->owner_scope == call_id) Birth(*id, kind);
        }
        // 槽真实改变后还需检查全部 Grid/door 别名，保留对象不冒称释放。
        const auto old_address = kind == Observation::Door ? call.before_grid : call.before;
        const bool native_replaced = call.body_slot_known && after == call.body_after &&
            after != old_address && (kind != Observation::GridSpawn || call.body_result);
        if (old_address && native_replaced) {
          bool retained = false, readable = true;
          for (std::uint32_t i = 0; i < c.room_grid_slot_count + c.room_door_slot_count; ++i) {
            std::uint32_t value = 0;
            if (!Value(std::uint64_t(call.object) + c.room_grid_slot_offset + i * 4, value)) { readable = false; break; }
            retained = retained || value == old_address;
          }
          if (!readable) Fault(Reason::ReadFailed, "grid_aliases");
          else if (!retained) if (auto *old = identities_.Find(ObjectDomain::Grid, old_address)) {
            old->MarkDestroyed(); Event(EventKind::GridPhysicalRetired, Reason::None, old, &call);
          }
        }
      }
    }
    if (Creation(kind) && kind != Observation::GridSpawn && kind != Observation::Door)
      CompleteOwner(call_id, kind, result == call.body_result ? result : 0);
    if ((kind == Observation::GridSpawn || kind == Observation::Door) &&
        (kind == Observation::GridSpawn && !result)) {
      identities_.ForEach([&](Identity &value) noexcept {
        if (value.owner_scope == call_id) { value.candidate = false; value.readiness = 0; }
      });
    }
  }
  if (normal && !completed) {
    Event(EventKind::GapOrUnknown, Reason::NotReady, nullptr, &call);
    identities_.ForEach([&](Identity &value) noexcept {
      if (value.owner_scope == call_id) {
        value.owner_scope = 0; value.candidate = false; value.readiness = 0;
      }
    });
  }
  Event(EventKind::NativeHook, normal ? Reason::None : Reason::UnpairedScope,
        id, &call, false, !normal, result);
  --depth_; storage_.Diag().callbacks.fetch_sub(1, std::memory_order_relaxed);
  if (call.lifecycle) LifecycleLeave(normal);
  if (!normal) { storage_.Diag().callback_faults.fetch_add(1, std::memory_order_relaxed); Fault(Reason::UnpairedScope, "native_exception"); }
  CloseIfDrained();
}
} // namespace cs::world
