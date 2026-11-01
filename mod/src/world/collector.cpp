#include "world/collector.h"
#include "binding/world_layout.h"
#include "world/domains/entity/collect.h"
#include "world/domains/grid/collect.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <windows.h>

namespace cs::world {
Collector::Collector(WorldStorage &storage, IdentityTable &identities,
                     MemoryReader reader, std::uint32_t generation) noexcept
    : storage_(storage), identities_(identities), memory_(reader),
      generation_(generation) {
  // 构造发生在后台准备阶段；Collector 本体包含固定工作表，也实际计入账本。
  charged_ = storage_.Ready() && storage_.Ledger().Charge(sizeof(Collector));
}
Collector::~Collector() {
  pending_ = {};
  if (charged_)
    storage_.Ledger().Release(sizeof(Collector));
}
static_assert(sizeof(Collector) < kManagementBytes,
              "Collector 工作表必须落在管理预算之内");
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
  if ((stopping_ || storage_.Diag().FaultReason() != Reason::None) &&
      Drained()) {
    active_ = false;
    storage_.RequestStop(storage_.Diag().FaultReason() == Reason::None
                             ? Reason::Stopped
                             : storage_.Diag().FaultReason());
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
bool Collector::Read(std::uint64_t address, void *value,
                     std::size_t size) noexcept {
  return GetCurrentThreadId() == memory_.main_thread && memory_.copy &&
         address && address <= UINT32_MAX && size <= UINT32_MAX - address &&
         memory_.copy(static_cast<std::uint32_t>(address), value, size);
}
bool Collector::Checkpoint(std::uint64_t request, const char *label) noexcept {
  if (!Recording() || GetCurrentThreadId() != memory_.main_thread || !label)
    return false;
  CheckpointRecord marker{};
  marker.request = request;
  marker.run = run_;
  marker.room = room_;
  marker.outer = outer_active_ ? outer_ : 0;
  marker.mutation = mutation_;
  marker.recording_generation = generation_;
  marker.attempt = storage_.Diag().last_attempt.load();
  marker.capture = storage_.Diag().last_capture.load();
  marker.event_watermark = storage_.EventWatermark();
  const auto length = std::strlen(label);
  if (!ValidWorldLabel(label))
    return false;
  std::memcpy(marker.label, label, length);
  if (storage_.PublishCheckpoint(marker))
    return true;
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
         out.game &&
         Value(std::uint64_t(out.game) + c.game_room_offset, out.room) &&
         Value(std::uint64_t(out.game) + c.game_room_index_offset, out.index) &&
         Value(std::uint64_t(out.game) + c.game_dimension_offset,
               out.dimension);
}
Identity *Collector::Find(ObjectDomain domain, std::uint32_t address) noexcept {
  if (!address)
    return nullptr;
  auto *id = identities_.FindOrCreate(domain, address);
  if (!id)
    Fault(identities_.LastError() == IdentityTable::Error::SequenceExhausted
              ? Reason::SequenceExhausted
              : Reason::Budget,
          "identity");
  return id;
}
bool Collector::Event(EventKind kind, Reason reason, Identity *id,
                      const Call *call, bool entering, bool exceptional,
                      std::uint32_t result,
                      std::uint64_t candidate_tick) noexcept {
  LifecycleEvent event{};
  event.run = run_;
  event.room = room_;
  event.outer = outer_active_ ? outer_ : 0;
  event.kind = kind;
  event.reason = reason;
  event.entering = entering;
  event.exceptional = exceptional;
  event.result = result;
  event.mutation = mutation_;
  event.tick = candidate_tick ? candidate_tick : authority_.tick;
  event.thread = GetCurrentThreadId();
  if (id) {
    event.local_id = id->local_id;
    event.use = id->use;
    event.birth = id->birth;
    event.revision = id->revision;
    event.address = id->address;
    event.memberships =
        kind == EventKind::SourceMembershipSnapshot ? result : id->memberships;
    event.domain = id->domain;
    event.lifecycle = id->lifecycle;
    event.readiness = id->readiness;
    event.ordinal = id->ordinal;
    for (const auto &anchor : anchors_) {
      if (!anchor.valid || anchor.player != id->address) continue;
      event.session = anchor.session;
      event.participant_generation = anchor.generation;
      event.assignment_key = anchor.key;
      event.device = anchor.device;
      break;
    }
  }
  if(call&&grid::ObservationFor(call->observation)) event.domain=unsigned(ObjectDomain::Grid);
  if (call) {
    event.call = call->id;
    event.parent = call->parent;
    event.observation = static_cast<std::uint32_t>(call->observation);
    if (!id)
      event.address = call->object;
    event.args = call->args;
    event.arg_count = call->count;
    event.slot_before = call->before;
    event.slot_after = call->body_after;
  } else if (id && kind == EventKind::SourceMembershipSnapshot) {
    event.slot_before = id->memberships;
    event.slot_after = result;
  }
  if (!storage_.PublishEvent(event)) {
    Fault(storage_.Diag().FaultReason() == Reason::None
              ? Reason::EventOverflow
              : storage_.Diag().FaultReason(),
          "lifecycle_event");
    return false;
  }
  event.sequence=storage_.EventWatermark();
  if(sink_.event) sink_.event(sink_.owner,event);
  return true;
}
void Collector::Incomplete(Reason reason) noexcept {
  if (!pending_)
    return;
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
  if (!header.reason)
    header.reason = static_cast<std::uint32_t>(reason);
}

// 所有窗口值在 Manager 最外层重新建立，不能继承上一轮的阶段或 Authority。
void Collector::BeginOuter(std::uint64_t loop, std::uint64_t outer,
                           bool run) noexcept {
  if (!Recording()) {
    CloseIfDrained();
    return;
  }
  if (GetCurrentThreadId() != memory_.main_thread) {
    Fault(Reason::WrongThread, "outer_thread");
    return;
  }
  if (outer_active_ || pending_ || depth_ || lifecycle_depth_ || !outer ||
      !loop) {
    Fault(Reason::UnpairedScope, "outer_begin");
    return;
  }
  if (attempt_ == UINT64_MAX || (run && !run_active_ && run_ == UINT64_MAX) ||
      (lifecycle_episode_ && room_ == UINT64_MAX)) {
    Fault(Reason::SequenceExhausted, "outer_sequence");
    return;
  }
  outer_active_ = true;
  prepared_ = false;
  polluted_ = false;
  loop_ = loop;
  outer_ = outer;
  authority_ = {};
  authority_count_ = 0;
  phases_ = {};
  outer_reason_ = Reason::None;
  post_ = {};
  const auto previous_roots = roots_;
  const bool previous_run = run_active_;
  if (run && !run_active_) {
    ++run_;
    identities_.ResetAssociations();
    anchors_ = {};
    update_witnesses_ = {};
    run_ended_ = false;
    lifecycle_episode_ = true;
  } else if (!run && run_active_ && !run_ended_ && !run_start_pending_) {
    LifecycleEnter(true);
    LifecycleLeave(true);
  }
  if (run)
    run_start_pending_ = false;
  run_active_ = run || run_start_pending_;
  if (!ReadRoots(roots_) || (run && !roots_.room))
    outer_reason_ = Reason::ReadFailed;
  if (previous_run && run && previous_roots != roots_ && !lifecycle_episode_) {
    LifecycleEnter();
    LifecycleLeave(true);
  }
  if (lifecycle_episode_) {
    if (room_ == UINT64_MAX) {
      Fault(Reason::SequenceExhausted, "room_epoch");
      return;
    }
    ++room_;
    lifecycle_episode_ = false;
  }
  if (!run)
    outer_reason_ = Reason::NotReady;
}
void Collector::EnterStage(Stage stage, std::uint64_t scope,
                           std::uint32_t object) noexcept {
  if (!Recording() || !outer_active_)
    return;
  const auto index = static_cast<std::size_t>(stage);
  if (index == 0 || index >= phases_.size()) {
    Fault(Reason::UnpairedScope, "stage_index");
    return;
  }
  auto &phase = phases_[index];
  const bool order =
      stage == Stage::Owner ? !phases_[2].entered && !phases_[3].entered
      : stage == Stage::Room
          ? phases_[1].active && !phases_[3].entered
          : phases_[1].active && phases_[2].returned == 1 && !phases_[2].active;
  if (!scope || phase.active || phase.entered || !order) {
    polluted_ = true;
    outer_reason_ = Reason::PhaseIncomplete;
    return;
  }
  phase.scope = scope;
  phase.object = object;
  phase.active = true;
  ++phase.entered;
  if (stage == Stage::Room && object != roots_.room) {
    polluted_ = true;
    outer_reason_ = Reason::RootChanged;
  }
  if (stage == Stage::Owner && object != roots_.game) {
    polluted_ = true;
    outer_reason_ = Reason::RootChanged;
  }
}
void Collector::LeaveStage(Stage stage, std::uint64_t scope,
                           bool normal) noexcept {
  if (!active_ || !outer_active_)
    return;
  const auto index = static_cast<std::size_t>(stage);
  if (!index || index >= phases_.size())
    return;
  auto &phase = phases_[index];
  if (!phase.active || phase.scope != scope || !normal) {
    polluted_ = true;
    outer_reason_ = Reason::UnpairedScope;
  } else
    ++phase.returned;
  phase.active = false;
}
void Collector::NativeReturned(const Authority &authority) noexcept {
  if (!active_ || !outer_active_)
    return;
  ++authority_count_;
  if (authority_count_ != 1 || authority.loop != loop_ ||
      authority.outer != outer_ || !authority.tick || !authority.scope ||
      authority.kind == AuthorityKind::None) {
    Fault(Reason::UnpairedScope, "authority_candidate");
    return;
  }
  authority_ = authority;
}
void Collector::RunBeginning() noexcept {
  if (!Recording())
    return;
  if (run_ == UINT64_MAX) {
    Fault(Reason::SequenceExhausted, "run_epoch");
    return;
  }
  ++run_;
  identities_.ResetAssociations();
  anchors_ = {};
  update_witnesses_ = {};
  run_active_ = true;
  run_start_pending_ = true;
  run_ended_ = false;
  run_start_owner_ = true;
  lifecycle_episode_ = true;
}
void Collector::RunOwnerReturned(bool normal) noexcept {
  run_start_owner_ = false;
  if (normal)
    run_ended_ = false;
  else
    Fault(Reason::UnpairedScope, "start_owner_exception");
}
void Collector::LifecycleEnter(bool run_end) noexcept {
  if (!Recording())
    return;
  if (lifecycle_depth_ == UINT32_MAX || mutation_ == UINT64_MAX) {
    Fault(Reason::SequenceExhausted, "lifecycle_sequence");
    return;
  }
  ++lifecycle_depth_;
  ++mutation_;
  polluted_ = true;
  outer_reason_ = Reason::Lifecycle;
  lifecycle_episode_ = true;
  // 全局生命周期边界撤销所有参与者的全部身体，不能遗留旧双人锚点限制。
  for(std::uint32_t slot=0;slot<anchors_.size();++slot) RevokePlayer(slot);
  if (run_end && !run_ended_) {
    run_ended_ = true;
    // Start 内清理旧 Game 的 Exit 不能取消尚在创建的新 run 观察域。
    run_start_pending_ = run_start_owner_;
    run_active_ = run_start_owner_;
    identities_.ForEach([](Identity &id) noexcept {
      id.lifecycle = static_cast<std::uint32_t>(Lifecycle::Retiring);
      id.readiness = 0;
      id.memberships = 0;
      id.ClearOrdinal();
    });
  }
  Event(EventKind::LifecycleBoundary, Reason::Lifecycle, nullptr, nullptr,
        true);
}
void Collector::LifecycleLeave(bool normal) noexcept {
  if (!active_)
    return;
  if (!lifecycle_depth_) {
    Fault(Reason::UnpairedScope, "lifecycle_return");
    return;
  }
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
  Event(EventKind::LifecycleBoundary,
        normal ? Reason::Lifecycle : Reason::UnpairedScope, nullptr, nullptr,
        false, !normal);
  if (!normal)
    Fault(Reason::UnpairedScope, "lifecycle_exception");
  CloseIfDrained();
}
void Collector::RevokePlayer(std::uint32_t ordinal) noexcept {
  if (!active_ || ordinal >= anchors_.size())
    return;
  update_witnesses_[ordinal] = {};
  auto *id = identities_.RevokeCurrentWitness(ordinal);
  if (id)
    Event(EventKind::CurrentPlayerReadWitness, Reason::NotReady, id);
  identities_.AnchorPlayer(ordinal, 0);
  anchors_[ordinal] = {};
}
void Collector::AnchorPlayer(std::uint32_t ordinal,
                             const PlayerAnchor &value) noexcept {
  if (!Recording() || ordinal >= anchors_.size())
    return;
  if (!value.valid || !value.player || !value.device || value.key < 0 ||
      !value.generation || !value.session) {
    RevokePlayer(ordinal);
    return;
  }
  for (std::size_t i = 0; i < anchors_.size(); ++i)
    if (i != ordinal && anchors_[i].valid && anchors_[i].player == value.player) {
      RevokePlayer(static_cast<std::uint32_t>(i));
      RevokePlayer(ordinal);
      return;
    }
  if (anchors_[ordinal] == value)
    return;
  RevokePlayer(ordinal);
  if (!identities_.AnchorPlayer(ordinal, value.player).valid) {
    Fault(Reason::Budget, "anchor_identity");
    return;
  }
  anchors_[ordinal] = value;
}
void Collector::PlayerReturned(std::uint32_t ordinal,
                               const PlayerAnchor &before,
                               const PlayerAnchor &after, std::uint64_t tick,
                               bool normal, bool reached) noexcept {
  if (!Recording() || ordinal >= anchors_.size())
    return;
  AnchorPlayer(ordinal, after);
  if (!normal || !reached || !tick || !outer_active_ || polluted_ ||
      !before.valid || before != after || anchors_[ordinal] != before) {
    RevokePlayer(ordinal);
    return;
  }
  // 依据 2 的 Player 也须在重绑后重建 +177 当前值资格，不把原出生当作本代次
  // witness。
  auto *id = identities_.Find(ObjectDomain::Entity, before.player);
  const bool rebuilt = id && !id->current_public_ready;
  const bool first_update = update_witnesses_[ordinal] != before;
  auto *witness = identities_.MarkCurrentWitness(ordinal);
  if (id && !id->Retired()) {
    id->current_public_ready = true;
    // 创建依据2不覆盖“本绑定实际更新过”的事实；每个绑定首次正常更新同样登记。
    if (witness || rebuilt || first_update) {
      if (Event(EventKind::CurrentPlayerReadWitness, Reason::None, id, nullptr,
                false, false, 0, tick))
        update_witnesses_[ordinal] = before;
    }
  }
}

// RTTI 只读分类：验证 COL/CHD/每个 BCD 的地址、签名与非虚单继承形状。
// 类型标识只匹配当前清单 TD，不以 Type 数值范围或 vtable Hook 字节猜分类。
// Grid 白名单包含根与 17 个已核定派生类，共用公共 Grid profile；未知 TD
// 仍拒绝。
NativeSource Collector::Source() noexcept {
  NativeSource result;
  result.owner=this;
  result.copy=[](void *owner,std::uint64_t address,void *out,std::size_t size) noexcept { return static_cast<Collector *>(owner)->Read(address,out,size); };
  result.incomplete=[](void *owner,Reason reason) noexcept { static_cast<Collector *>(owner)->Incomplete(reason); };
  result.image_base=memory_.image_base; result.image_size=memory_.image_size;
  result.game=roots_.game; result.room=roots_.room; result.game_frame=game_frame_; result.grid_width=grid_width_;
  result.polluted=polluted_; result.faults=storage_.Faults(); return result;
}
Class Collector::Classify(ObjectDomain domain,std::uint32_t address,std::uint32_t &vptr,bool &base_known) noexcept {
  return ClassifyNative(Source(),domain,address,vptr,base_known);
}

// 主线程先枚举完整来源、合并别名，再复制对象和解析引用。来源缺失不提交成员退出。
std::int32_t Collector::Add(ObjectDomain domain, std::uint32_t address,
                            std::uint32_t source,
                            std::uint32_t index) noexcept {
  if (!address || !pending_)
    return -1;
  auto &h = pending_.Header();
  ++h.required_memberships;
  auto *id = Find(domain, address);
  if (!id) {
    Incomplete(Reason::Budget);
    return -1;
  }
  id->MarkFirstObserved();
  identities_.AddMembership(*id, source);
  auto object_index = identities_.BatchObjectIndex(*id);
  if (object_index == -2)
    return -1;
  if (object_index < 0) {
    ++h.required_objects;
    const auto *faults = storage_.Faults();
    if (h.object_count >= (faults ? faults->object_limit : kObjectBudget)) {
      if (faults && faults->mode.load() == 2) {
        auto *f = storage_.Faults();
        f->before.store(f->object_limit);
        f->after.store(h.required_objects);
        f->applied.store(1);
      }
      h.sources[source].status = static_cast<std::uint32_t>(Reason::Budget);
      identities_.SetBatchObjectIndex(*id, -2);
      Incomplete(Reason::Budget);
      return -1;
    }
    object_index = static_cast<std::int32_t>(h.object_count++);
    identities_.SetBatchObjectIndex(*id, object_index);
    auto &object = pending_.Objects()[object_index];
    object.Reset();
    object.address = address;
    object.domain = static_cast<std::uint32_t>(domain);
  }
  if (h.membership_count >= (storage_.Faults()
                                 ? storage_.Faults()->membership_limit
                                 : kMembershipBudget)) {
    if (auto *f = storage_.Faults(); f && f->mode.load() == 3) {
      f->before.store(f->membership_limit);
      f->after.store(h.required_memberships);
      f->applied.store(1);
    }
    h.sources[source].status = static_cast<std::uint32_t>(Reason::Budget);
    Incomplete(Reason::Budget);
    return object_index;
  }
  pending_.Memberships()[h.membership_count++] = {
      static_cast<std::uint32_t>(object_index), source, index, 0};
  return object_index;
}
SubjectRegistry Collector::Registry() noexcept {
  SubjectRegistry r; r.owner=this;
  r.add=[](void *owner,ObjectDomain domain,std::uint32_t address,std::uint32_t source,std::uint32_t index) noexcept -> RawObject * {
    auto &c=*static_cast<Collector *>(owner); const auto slot=c.Add(domain,address,source,index);
    return slot<0?nullptr:&c.pending_.Objects()[slot];
  };
  r.count=[](void *owner) noexcept { return static_cast<Collector *>(owner)->pending_.Header().object_count; };
  r.object=[](void *owner,std::uint32_t index) noexcept -> const RawObject * { auto &c=*static_cast<Collector *>(owner); return index<c.pending_.Header().object_count?&c.pending_.Objects()[index]:nullptr; };
  r.identity=[](void *owner,ObjectDomain domain,std::uint32_t address) noexcept -> const Identity * { return static_cast<Collector *>(owner)->identities_.Find(domain,address); };
  return r;
}
void Collector::CopySource(std::uint32_t source) noexcept {
  auto &out=pending_.Header().sources[source];
  if(entity::OwnsSource(source)) entity::Enumerate(Source(),Registry(),source,out,source_addresses_,fixed_addresses_);
  else if(grid::OwnsSource(source)) grid::Enumerate(Source(),Registry(),source,out,source_addresses_,fixed_addresses_);
  else out.status=kSourceNotApplicable;
}
void Collector::CopySources() noexcept {
  for (std::uint32_t source = 0; source < kSourceCount && Recording(); ++source)
    CopySource(source);
  entity::CollectRelations(Source(),Registry(),pending_.Header().sources[10],relation_nodes_);
}
// 预枚举只用来固定观察集合，不属于被证明无 RNG 推进的复制窗口。
// 随后的 CopySources 会从头重新读取所有来源；两次集合差异一律不可确认。
void Collector::BeginRngWindow() noexcept {
  auto &h = pending_.Header();
  const auto baseline = h;
  CopySources();
  rng_count_ = h.object_count;
  const auto inventory_complete = h.complete;
  const auto inventory_reason = h.reason;
  LARGE_INTEGER now{}, frequency{};
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&now);
  for (std::uint32_t i = 0; i < rng_count_; ++i) {
    const auto &o = pending_.Objects()[i];
    auto &w = rng_inventory_[i];
    w = {};
    w.domain = o.domain;
    w.address = o.address;
    auto *id = identities_.Find(static_cast<ObjectDomain>(o.domain), o.address);
    bool base = false;
    w.kind = static_cast<std::uint32_t>(
        Classify(static_cast<ObjectDomain>(o.domain), o.address, w.vptr, base));
    if (id) {
      w.local = id->local_id;
      w.use = id->use;
      w.revision = id->revision;
    }
    // Grid 与字段复制共用已确认的初始化 byte/槽规则，不把 FIRST_OBSERVED
    // 补成观察到 Init。
    bool ready = id && id->readiness >= 2;
    if (w.kind == static_cast<std::uint32_t>(Class::Grid)) {
      const auto *flag = layout::FindField(0x0807);
      const auto *index = layout::FindField(0x0809);
      std::uint8_t initialized = 0;
      std::uint32_t actual = kNoGridIndex;
      ready = flag && index &&
              Value(std::uint64_t(o.address) + flag->offset, initialized) &&
              initialized &&
              Value(std::uint64_t(o.address) + index->offset, actual) &&
              actual == o.grid_index && actual < 448;
    }
    if (inventory_complete && id && !id->Retired() && ready &&
        w.kind != static_cast<std::uint32_t>(Class::Unknown)) {
      const auto *field = layout::FindField(o.domain ? 0x080b : 0x0012);
      w.value.before_status =
          field && Read(std::uint64_t(o.address) + field->offset,
                        w.value.before.data(), sizeof(w.value.before))
              ? 1u
              : 2u;
    }
  }
  std::sort(rng_inventory_.begin(), rng_inventory_.begin() + rng_count_,
            [](const auto &a, const auto &b) {
              return a.domain < b.domain ||
                     (a.domain == b.domain && a.address < b.address);
            });
  // 丢弃预枚举的临时去重/成员位；历史 committed 位与身份/出生不变。
  identities_.ForEach([&](Identity &id) {
    if (id.batch_attempt == attempt_) {
      id.batch_attempt = 0;
      id.batch_index = -1;
    }
    if (id.pending_attempt == attempt_) {
      id.pending_attempt = 0;
      id.pending_memberships = 0;
    }
  });
  h = baseline;
  h.complete = inventory_complete;
  h.reason = inventory_reason;
  h.rng_inventory = rng_count_;
  h.rng_thread_before = GetCurrentThreadId();
  h.copy_started_qpc = static_cast<std::uint64_t>(now.QuadPart);
  h.qpc_frequency = static_cast<std::uint64_t>(frequency.QuadPart);
}
void Collector::EndRngWindow(bool stable_sources) noexcept {
  auto &h = pending_.Header();
  bool set_stable =
      stable_sources && h.object_count == rng_count_ && !polluted_;
  for (std::uint32_t i = 0; i < h.object_count; ++i) {
    auto &o = pending_.Objects()[i];
    const auto it = std::lower_bound(
        rng_inventory_.begin(), rng_inventory_.begin() + rng_count_, o,
        [](const auto &a, const auto &b) {
          return a.domain < b.domain ||
                 (a.domain == b.domain && a.address < b.address);
        });
    if (it == rng_inventory_.begin() + rng_count_ || it->domain != o.domain ||
        it->address != o.address) {
      set_stable = false;
      continue;
    }
    o.rng =
        it->value; // 即使尾部拒绝，也保留真实前值；after_status=0，不读旧地址。
    auto *id = identities_.Find(static_cast<ObjectDomain>(o.domain), o.address);
    if (!stable_sources || !id || id->Retired() || id->local_id != it->local ||
        id->use != it->use || id->revision != it->revision ||
        o.vptr != it->vptr || o.kind != it->kind || polluted_) {
      set_stable = false;
      continue;
    }
    ++h.rng_matched;
    if (o.rng.before_status == 1) {
      const auto *field = layout::FindField(o.domain ? 0x080b : 0x0012);
      o.rng.after_status =
          field && Read(std::uint64_t(o.address) + field->offset,
                        o.rng.after.data(), sizeof(o.rng.after))
              ? 1u
              : 2u;
    }
  }
  LARGE_INTEGER now{};
  QueryPerformanceCounter(&now);
  h.copy_finished_qpc = static_cast<std::uint64_t>(now.QuadPart);
  h.rng_thread_after = GetCurrentThreadId();
  h.rng_set_stable = set_stable && h.rng_matched == rng_count_;
}
void Collector::CopyObject(RawObject &out, Identity &id) noexcept {
  out.local_id = id.local_id;
  out.use = id.use;
  out.birth = id.birth;
  out.revision = id.revision;
  out.origin = id.origin;
  out.lifecycle = id.lifecycle;
  out.readiness = id.readiness;
  out.birth_kind = id.birth_kind;
  out.birth_type = id.birth_type;
  out.birth_variant = id.birth_variant;
  out.birth_subtype = id.birth_subtype;
  out.birth_seed = id.birth_seed;
  out.birth_known = id.born;
  out.ordinal = id.ordinal;
  if(out.domain==unsigned(ObjectDomain::Grid)) grid::CollectNative(Source(),out,id);
  else entity::CollectNative(Source(),out,id);
}
void Collector::ResolveReferences() noexcept {
  for (std::uint32_t i = 0; i < pending_.Header().object_count; ++i) {
    auto &object = pending_.Objects()[i];
    const auto *requester = identities_.Find(
        static_cast<ObjectDomain>(object.domain), object.address);
    if (!requester)
      continue;
    for (std::uint32_t j = 0; j < object.field_count; ++j) {
      auto &field = object.fields[j];
      if (field.encoding != Encoding::Reference ||
          field.status != FieldStatus::Value)
        continue;
      std::uint32_t address = 0;
      std::memcpy(&address, field.raw.data(), 4);
      const auto result = identities_.ResolveReference(address, *requester);
      field.relation = result.relation;
      field.target_id = result.local_id;
      field.target_use = result.use;
      if (auto *f = storage_.Faults(); address && f && f->pending == 8) {
        f->before.store(static_cast<std::uint32_t>(result.relation));
        field.relation = Relation::AmbiguousReuse;
        field.target_id = 0;
        field.target_use = 0;
        f->after.store(static_cast<std::uint32_t>(field.relation));
        f->pending = 0;
        f->applied.store(1);
        Incomplete(Reason::NotReady);
      }
    }
  }
}
void Collector::Memberships(bool commit) noexcept {
  if (!pending_)
    return;
  for (const auto &source : pending_.Header().sources)
    if (source.status && source.status != kSourceNotApplicable)
      return;
  identities_.ForEach([&](Identity &id) noexcept {
    const auto mask = identities_.MembershipMask(id);
    if (identities_.MembershipChanged(id, mask)) {
      if (commit)
        identities_.CommitMembership(id, mask);
      else
        Event(EventKind::SourceMembershipSnapshot, Reason::None, &id, nullptr,
              false, false, mask);
    }
  });
}

void Collector::Gap(Reason reason, bool authority) noexcept {
  GapRecord gap{};
  gap.attempt = attempt_;
  gap.run = run_;
  gap.room = room_;
  gap.mutation = mutation_;
  gap.outer = outer_;
  gap.tick = authority ? authority_.tick : 0;
  gap.scope = authority ? authority_.scope : 0;
  gap.event_watermark = storage_.EventWatermark();
  gap.recording_generation = generation_;
  gap.reason = reason;
  gap.thread = GetCurrentThreadId();
  gap.authority_kind =
      authority ? static_cast<std::uint32_t>(authority_.kind) : 0;
  if(sink_.gap) sink_.gap(sink_.owner,gap);
  if (!storage_.PublishGap(gap))
    Fault(Reason::EventOverflow, "gap_publish");
}
core::TickPostState Collector::PreparePostState(bool normal) noexcept {
  if (!active_ || !outer_active_)
    return {};
  if (prepared_) {
    Fault(Reason::UnpairedScope, "duplicate_prepare");
    return post_;
  }
  prepared_ = true;
  if (attempt_ == UINT64_MAX) {
    Fault(Reason::SequenceExhausted, "attempt");
    return {};
  }
  ++attempt_;
  post_stable_=false;
  post_ = {};
  post_.tick = authority_.tick;
  post_.outer_token = outer_;
  post_.recording_generation = generation_;
  post_.attempt_seq = attempt_;
  const bool authority = authority_count_ == 1 && authority_.tick;
  Reason reason = outer_reason_;
  if (!normal || depth_ || lifecycle_depth_)
    reason = Reason::UnpairedScope;
  else if (stopping_ || !Recording())
    reason = storage_.Diag().FaultReason() == Reason::None
                 ? Reason::Stopped
                 : storage_.Diag().FaultReason();
  else if (authority && authority_.kind == AuthorityKind::OddPlayer &&
           !polluted_ && reason == Reason::None)
    reason = Reason::OddOnly;
  else if (!authority)
    reason = reason == Reason::None ? Reason::PhaseIncomplete : reason;
  else if (reason == Reason::None) {
    for (std::size_t i = 1; i < phases_.size(); ++i)
      if (phases_[i].entered != 1 || phases_[i].returned != 1 ||
          phases_[i].active)
        reason = Reason::PhaseIncomplete;
    if (authority_.scope != phases_[1].scope)
      reason = Reason::UnpairedScope;
  }
  post_.status = storage_.Diag().FaultReason() != Reason::None
                     ? core::PostStateStatus::Faulted
                 : stopping_ ? core::PostStateStatus::Unavailable
                             : core::PostStateStatus::Gap;
  if (reason != Reason::None) {
    storage_.Diag().attempts.fetch_add(1, std::memory_order_relaxed);
    storage_.Diag().last_attempt.store(attempt_, std::memory_order_relaxed);
    post_.reason = static_cast<std::uint32_t>(reason);
    Gap(reason, authority);
    return post_;
  }
  BatchBegin begin{};
  begin.attempt = attempt_;
  begin.run = run_;
  begin.room = room_;
  begin.mutation = mutation_;
  begin.outer = outer_;
  begin.tick = authority_.tick;
  begin.scope = authority_.scope;
  begin.recording_generation = generation_;
  begin.authority = true;
  begin.authority_kind = authority_.kind;
  begin.event_watermark = storage_.EventWatermark();
  pending_ = storage_.Begin(begin);
  if (!pending_) {
    post_.status = Recording() ? core::PostStateStatus::Backpressure
                               : core::PostStateStatus::Faulted;
    post_.reason = static_cast<std::uint32_t>(
        Recording() ? Reason::Backpressure : storage_.Diag().FaultReason());
    return post_;
  }
  auto &h = pending_.Header();
  h.complete = 1;
  h.loop = loop_;
  for (std::size_t i = 1; i < phases_.size(); ++i) {
    h.phase_scopes[i - 1] = phases_[i].scope;
    h.phase_entered[i - 1] = phases_[i].entered;
    h.phase_returned[i - 1] = phases_[i].returned;
  }
  if (capture_ == UINT64_MAX) {
    Fault(Reason::SequenceExhausted, "capture");
    Incomplete(Reason::SequenceExhausted);
  } else
    h.capture = ++capture_;
  identities_.BeginBatch(attempt_);
  const auto &c = layout::LayoutConstants();
  if (!Value(std::uint64_t(roots_.game) + c.game_frame_offset, game_frame_) ||
      !Value(std::uint64_t(roots_.room) + c.room_grid_width_offset,
             grid_width_))
    Incomplete(Reason::ReadFailed);
  BeginRngWindow();
  CopySources();
  bool stable_sources = true;
  for (std::size_t i=0;i<h.sources.size();++i) {
    const auto &source=h.sources[i];
    // 旧v1关系私有字段未获读取资格仍保留NOT_READY/不完整；它不是源指针变化。
    // 正式Lua以公开PlayerManager/twin/sub/备份Getter闭合并复核自己的成员来源。
    const bool unavailable_private_relation=i==10&&source.status==static_cast<std::uint32_t>(Reason::NotReady);
    if (source.status && source.status != kSourceNotApplicable&&!unavailable_private_relation)
      stable_sources = false;
  }
  for (std::uint32_t i = 0; i < h.object_count && Recording(); ++i) {
    auto &object = pending_.Objects()[i];
    auto *id = identities_.Find(static_cast<ObjectDomain>(object.domain),
                                object.address);
    if (id)
      CopyObject(object, *id);
  }
  ResolveReferences();
  if(sink_.lua) sink_.lua(sink_.owner,h,pending_.Objects(),pending_.Memberships());
  // 在整个复制尾部再复读所有动态 descriptor 和固定槽/door 映射。
  // 源内的两遍指针检查之外，再覆盖复制其他来源/字段期间的描述变化。
  for (std::uint32_t source = 0; source < kSourceCount && Recording();
       ++source) {
    const auto *spec = layout::FindSource(source);
    if (!spec || spec->kind == layout::SourceKind::Relation ||
        spec->kind == layout::SourceKind::Reserved)
      continue;
    const auto base =
        std::strcmp(spec->base, "game") == 0 ? roots_.game : roots_.room;
    const auto &saved = h.sources[source];
    if (spec->kind == layout::SourceKind::EntityList ||
        spec->kind == layout::SourceKind::Roster) {
      std::uint32_t descriptor[4]{};
      const std::size_t bytes =
          spec->kind == layout::SourceKind::Roster ? 12 : 16;
      auto expected = saved.descriptor;
      if (auto *f = storage_.Faults(); f && f->pending == 6) {
        f->before.store(expected[0]);
        expected[0] ^= 1u;
        f->after.store(expected[0]);
        f->pending = 0;
        f->applied.store(1);
      }
      if (!Read(std::uint64_t(base) + spec->offset, descriptor, bytes) ||
          std::memcmp(descriptor, expected.data(), bytes)) {
        stable_sources = false;
        Incomplete(Reason::SourceChanged);
      }
      // descriptor
      // 不变不保证元素不变：复读本来源全部实际指针，覆盖后续字段复制期间的变化。
      const auto data = spec->kind == layout::SourceKind::Roster
                            ? saved.descriptor[0]
                            : saved.descriptor[1];
      for (std::uint32_t m = 0; m < h.membership_count; ++m) {
        const auto &membership = pending_.Memberships()[m];
        if (membership.source != source || membership.object >= h.object_count)
          continue;
        std::uint32_t now = 0;
        if (!Value(std::uint64_t(data) + std::uint64_t(membership.index) * 4,
                   now) ||
            now != pending_.Objects()[membership.object].address) {
          stable_sources = false;
          Incomplete(Reason::SourceChanged);
        }
      }
    } else {
      for (std::uint32_t i = 0; i < spec->count; ++i) {
        std::uint32_t now = 0;
        if (!Value(std::uint64_t(base) + spec->offset + i * 4, now)) {
          stable_sources = false;
          Incomplete(Reason::ReadFailed);
          continue;
        }
        const auto original = fixed_addresses_[(grid::OwnsSource(source)?grid::FixedSourceOffset(source):entity::FixedSourceOffset(source))+i];
        if (original != now) {
          stable_sources = false;
          Incomplete(Reason::SourceChanged);
        }
        if (source == 12 && now) {
          std::uint32_t index = 0;
          auto *id = identities_.Find(ObjectDomain::Grid, now);
          const auto slot = id ? identities_.BatchObjectIndex(*id) : -1;
          if (!Value(std::uint64_t(base) + spec->index_map_offset + i * 4,
                     index) ||
              slot < 0 || pending_.Objects()[slot].grid_index != index) {
            stable_sources = false;
            Incomplete(Reason::SourceChanged);
          }
        }
      }
    }
  }
  Roots after{};
  auto expected = roots_;
  if (auto *f = storage_.Faults(); f && f->pending == 7) {
    f->before.store(expected.index);
    expected.index ^= 1u;
    f->after.store(expected.index);
    f->pending = 0;
    f->applied.store(1);
  }
  if (!ReadRoots(after) || after != expected || polluted_) {
    stable_sources = false;
    Incomplete(Reason::RootChanged);
  }
  if (!stable_sources) {
    // 前后复核拒绝时撤销本批 +177 的当前值资格，原始读取不冒充可用字段。
    for(std::uint32_t slot=0;slot<anchors_.size();++slot) RevokePlayer(slot);
    for (std::uint32_t i = 0; i < h.object_count; ++i) {
      auto &object = pending_.Objects()[i];
      if (auto *id = identities_.Find(static_cast<ObjectDomain>(object.domain),
                                      object.address))
        id->current_public_ready = false;
      for (std::uint32_t f = 0; f < object.field_count; ++f)
        if (object.fields[f].id == 0x000a &&
            object.fields[f].status == FieldStatus::Value)
          object.fields[f].status = FieldStatus::NotReady;
    }
  }
  post_stable_=stable_sources;
  EndRngWindow(stable_sources);
  Memberships(false);
  h.event_watermark = storage_.EventWatermark();
  if (!Recording())
    Incomplete(storage_.Diag().FaultReason());
  post_.reason = h.reason;
  post_.capture_seq = h.capture;
  post_.status = storage_.Diag().FaultReason() != Reason::None
                     ? core::PostStateStatus::Faulted
                 : h.complete ? core::PostStateStatus::Complete
                              : core::PostStateStatus::Gap;
  if(sink_.native) sink_.native(sink_.owner,h,pending_.Objects(),pending_.Memberships());
  if (!pending_.Prepare()) {
    Fault(Reason::UnpairedScope, "batch_prepare");
    post_.status = core::PostStateStatus::Faulted;
  } else {
    storage_.Diag().captures.fetch_add(1, std::memory_order_relaxed);
    storage_.Diag().last_capture.store(h.capture, std::memory_order_relaxed);
  }
  return post_;
}
void Collector::PublishPostState(bool committed,
                                 const core::TickPostState &post) noexcept {
  if (!pending_)
    return;
  const bool match = post.tick == post_.tick && post.outer_token == outer_ &&
                     post.recording_generation == generation_ &&
                     post.attempt_seq == attempt_ &&
                     post.capture_seq == post_.capture_seq &&
                     post.status == post_.status && post.reason == post_.reason;
  if(sink_.commit) sink_.commit(sink_.owner,committed&&match,post_stable_);
  if (!committed || !match) {
    // 析构租约会产生 UNPAIRED_SCOPE gap，不静默退回空槽。
    pending_ = {};
    if (!match)
      Fault(Reason::UnpairedScope, "post_pair");
    return;
  }
  Memberships(true);
  pending_.Header().event_watermark = storage_.EventWatermark();
  if (storage_.Diag().FaultReason() != Reason::None)
    Incomplete(storage_.Diag().FaultReason());
  const bool complete = pending_.Header().complete != 0;
  if (!pending_.Release(true))
    Fault(Reason::NotificationFailed, "batch_publish");
  else if (complete)
    storage_.Diag().complete_captures.fetch_add(1, std::memory_order_relaxed);
  else
    storage_.Diag().gaps.fetch_add(1, std::memory_order_relaxed);
}
void Collector::EndOuter() noexcept {
  if (!outer_active_) {
    CloseIfDrained();
    return;
  }
  if (pending_) {
    if(sink_.commit) sink_.commit(sink_.owner,false,false);
    pending_ = {};
  }
  outer_active_ = false;
  CloseIfDrained();
}

std::uint32_t Collector::CreationOwner(ObjectDomain domain) const noexcept {
  // 取真实最外层 owner：嵌套 Spawn/Publish/Init 不能抢先冻结出生。
  for (std::uint32_t i = 0; i < depth_; ++i) {
    const auto kind = calls_[i].observation;
    if ((entity::CreationOwner(kind)||grid::CreationOwner(kind)) &&
        (grid::ObservationFor(kind) == (domain == ObjectDomain::Grid)))
      return calls_[i].id;
  }
  return 0;
}
bool Collector::Birth(Identity &id, Observation owner) noexcept {
  if (!id.candidate || id.born || id.Retired())
    return false;
  std::uint32_t vptr = 0;
  bool base = false;
  Classify(static_cast<ObjectDomain>(id.domain), id.address, vptr, base);
  if (!base) {
    id.candidate = false;
    id.readiness = 0;
    Event(EventKind::GapOrUnknown, Reason::UnknownClass, &id);
    return false;
  }
  BirthValues values{};
  values.kind = static_cast<std::uint32_t>(owner);
  const std::uint16_t first = id.domain == 1 ? 0x0801 : 0x0002;
  const std::uint16_t ids[] = {
      first, static_cast<std::uint16_t>(first + 1),
      static_cast<std::uint16_t>(id.domain == 1 ? 0 : 0x0004),
      static_cast<std::uint16_t>(id.domain == 1 ? 0x0805 : 0x0011)};
  std::uint32_t *destinations[] = {&values.entity_type, &values.variant,
                                   &values.subtype, &values.seed};
  for (std::size_t i = 0; i < 4; ++i) {
    if (!ids[i])
      continue;
    const auto *field = layout::FindField(ids[i]);
    if (!field ||
        !Value(std::uint64_t(id.address) + field->offset, *destinations[i])) {
      Fault(Reason::ReadFailed, "birth_fields");
      return false;
    }
  }
  // 本次取得和完整创建 owner 均被观察；它不授予 FIRST_OBSERVED 对象出生历史。
  id.readiness = static_cast<std::uint32_t>(Readiness::CreationOwnerCompleted);
  const Call *owner_call = nullptr;
  for (std::uint32_t i = 0; i < depth_; ++i)
    if (calls_[i].id == id.owner_scope)
      owner_call = &calls_[i];
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
    if (!Recording() || !id.candidate || id.owner_scope != call)
      return;
    // Spawn/PlayerCreate 的返回对象必须确实等于该次取得的对象。
    if ((kind == Observation::Spawn || kind == Observation::PlayerCreate) &&
        id.address != result) {
      id.candidate = false;
      id.owner_scope = 0;
      id.readiness = 0;
      Event(EventKind::GapOrUnknown, Reason::Continuity, &id);
      return;
    }
    if (kind == Observation::ManagerRestore && lifecycle_depth_ > 1) {
      id.owner_scope = UINT64_MAX;
      id.readiness = 0;
    } else
      Birth(id, kind);
  });
}
std::uint32_t Collector::ObserveEnter(Observation kind, std::uint32_t object,
                                      const std::uint32_t *args,
                                      std::uint32_t count) noexcept {
  if (!Recording())
    return 0;
  if (depth_ == calls_.size() || next_call_ == UINT32_MAX || count > 8) {
    Fault(Reason::Budget, "observation_stack");
    return 0;
  }
  auto &call = calls_[depth_];
  call = {};
  call.id = ++next_call_;
  call.parent = depth_ ? calls_[depth_ - 1].id : 0;
  call.observation = kind;
  call.object = object;
  call.count = count;
  for (std::uint32_t i = 0; i < count; ++i)
    call.args[i] = args[i];
  ++depth_;
  storage_.Diag().callbacks.fetch_add(1, std::memory_order_relaxed);
  const auto domain =
      grid::ObservationFor(kind) ? ObjectDomain::Grid : ObjectDomain::Entity;
  Identity *id = nullptr;
  const bool direct =
      kind == Observation::Init || kind == Observation::Remove ||
      kind == Observation::ClearReferences || kind == Observation::Destroy ||
      kind == Observation::PlayerCtor || kind == Observation::PlayerRestore ||
      kind == Observation::GridInit || entity::Morph(kind);
  if (direct)
    id = Find(domain, object);
  if (kind == Observation::Publish && count)
    id = Find(ObjectDomain::Entity, args[0]);
  if (kind == Observation::GridSpawn || kind == Observation::GridFree ||
      kind == Observation::Door) {
    const auto &c = layout::LayoutConstants();
    const auto limit = kind == Observation::Door ? c.room_door_slot_count
                                                 : c.room_grid_slot_count;
    const auto offset = kind == Observation::Door ? c.room_door_slot_offset
                                                  : c.room_grid_slot_offset;
    if (count && args[0] < limit &&
        !Value(std::uint64_t(object) + offset + args[0] * 4, call.before))
      Fault(Reason::ReadFailed, "grid_before");
  }
  if (kind == Observation::ManagerRestore || kind == Observation::Replace ||
      kind == Observation::PlayerRestore) {
    call.lifecycle = true;
    LifecycleEnter();
    // Restore/Replace 可以原址改变状态，立即撤销当前出生连续性；不猜新构造。
    identities_.ForEach([&](Identity &value) noexcept {
      bool affected = kind == Observation::ManagerRestore;
      if (kind == Observation::PlayerRestore)
        affected = value.domain == 0 && value.address == object;
      if (kind == Observation::Replace)
        affected = value.domain == 0 && count == 2 &&
                   (value.address == args[0] || value.address == args[1]);
      if (!affected || value.domain != 0)
        return;
      std::uint32_t vptr = 0;
      bool base = false;
      if (kind == Observation::ManagerRestore &&
          Classify(ObjectDomain::Entity, value.address, vptr, base) !=
              Class::Player)
        return;
      if (value.revision == UINT64_MAX) {
        Fault(Reason::SequenceExhausted, "restore_revision");
        return;
      }
      ++value.revision;
      value.readiness = 0;
      value.birth = 0;
      value.born = false;
      value.current_public_ready = false;
      value.candidate = false;
      value.owner_scope = 0;
      value.origin = static_cast<std::uint32_t>(Origin::FirstObserved);
      Event(EventKind::GapOrUnknown, Reason::Continuity, &value, &call, true);
    });
  }
  if (id && entity::Morph(kind)) {
    // 内层 Init 只给基类依据；外层 Morph 正常返回才恢复派生配置读取。
    id->readiness = 0;
    id->current_public_ready = false;
  }
  if (id && (kind == Observation::Init || kind == Observation::GridInit)) {
    bool known_owner = CreationOwner(domain) != 0;
    for (std::uint32_t i = 0; i + 1 < depth_; ++i)
      known_owner = known_owner || (entity::Morph(calls_[i].observation) &&
                                    calls_[i].object == object);
    // 连续性在进入即撤销，正常返回前不能调用 MarkInitReturned。
    if (!known_owner && !id->candidate) {
      if (id->revision == UINT64_MAX)
        Fault(Reason::SequenceExhausted, "init_revision");
      else
        ++id->revision;
      id->born = false;
      id->birth = 0;
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
                                     const std::uint32_t *args,
                                     std::uint32_t count) noexcept {
  if (!Recording())
    return 0;
  if (!depth_ || calls_[depth_ - 1].observation != kind ||
      calls_[depth_ - 1].object != object || calls_[depth_ - 1].body_reached ||
      count > 8) {
    Fault(Reason::UnpairedScope, "native_body_pair");
    return 0;
  }
  auto &call = calls_[depth_ - 1];
  call.body_reached = true;
  call.count = count;
  for (std::uint32_t i = 0; i < count; ++i)
    call.args[i] = args[i];
  if (kind == Observation::GridSpawn || kind == Observation::GridFree ||
      kind == Observation::Door) {
    const auto &c = layout::LayoutConstants();
    const auto limit = kind == Observation::Door ? c.room_door_slot_count
                                                 : c.room_grid_slot_count;
    const auto offset = kind == Observation::Door ? c.room_door_slot_offset
                                                  : c.room_grid_slot_offset;
    call.before = 0;
    if (count && args[0] < limit &&
        !Value(std::uint64_t(object) + offset + args[0] * 4, call.before))
      Fault(Reason::ReadFailed, "native_grid_before");
    if (kind == Observation::Door && count && args[0] < limit && !call.before) {
      if (!Value(std::uint64_t(object) + c.room_door_grid_map_offset +
                     args[0] * 4,
                 call.grid_index) ||
          call.grid_index >= c.room_grid_slot_count ||
          !Value(std::uint64_t(object) + c.room_grid_slot_offset +
                     call.grid_index * 4,
                 call.before_grid))
        Fault(Reason::ReadFailed, "door_grid_before");
      if (call.before_grid) {
        std::uint32_t vptr = 0;
        bool base = false;
        if (Classify(ObjectDomain::Grid, call.before_grid, vptr, base) !=
            Class::Grid)
          Fault(Reason::UnknownClass, "door_grid_type");
        std::size_t size = 0;
        const auto *classes = layout::Classes(size);
        for (std::size_t i = 0; i < size; ++i)
          if (classes[i].rtti == layout::RttiClass::Door)
            call.before_grid_door =
                vptr == memory_.image_base + classes[i].vtable_rva;
      }
    }
  }
  if (auto *id = identities_.Find(ObjectDomain::Entity, object)) {
    if (entity::Morph(kind) || kind == Observation::ClearReferences) {
      if (id->revision == UINT64_MAX)
        Fault(Reason::SequenceExhausted, "native_revision");
      else
        ++id->revision;
      id->RevokeCurrentWitness();
    }
    if (kind == Observation::Remove)
      id->MarkRemoveRequested();
    if (kind == Observation::Destroy) {
      id->lifecycle = static_cast<std::uint32_t>(Lifecycle::Retiring);
      id->readiness = 0;
    }
  }
  Event(EventKind::NativeBody, Reason::None, nullptr, &call, true);
  return call.id;
}
void Collector::NativeLeave(std::uint32_t call_id, std::uint32_t result,
                            bool normal) noexcept {
  if (!call_id)
    return;
  if (!depth_ || calls_[depth_ - 1].id != call_id ||
      !calls_[depth_ - 1].body_reached) {
    Fault(Reason::UnpairedScope, "native_body_return");
    return;
  }
  calls_[depth_ - 1].body_returned = normal;
  calls_[depth_ - 1].body_result = result;
  auto &call = calls_[depth_ - 1];
  if (normal && (call.observation == Observation::GridSpawn ||
                 call.observation == Observation::GridFree ||
                 call.observation == Observation::Door)) {
    const auto &c = layout::LayoutConstants();
    const bool door = call.observation == Observation::Door;
    const auto count = door ? c.room_door_slot_count : c.room_grid_slot_count;
    const auto offset =
        door ? c.room_door_slot_offset : c.room_grid_slot_offset;
    if (call.count && call.args[0] < count) {
      call.body_slot_known =
          Value(std::uint64_t(call.object) + offset + call.args[0] * 4,
                call.body_after);
      if (!call.body_slot_known)
        Fault(Reason::ReadFailed, "native_grid_after");
    }
  }
  Event(EventKind::NativeBody, normal ? Reason::None : Reason::UnpairedScope,
        nullptr, &calls_[depth_ - 1], false, !normal, result);
}
void Collector::ObserveLeave(std::uint32_t call_id, std::uint32_t result,
                             bool normal) noexcept {
  if (!call_id)
    return;
  if (!depth_ || calls_[depth_ - 1].id != call_id) {
    Fault(Reason::UnpairedScope, "observation_return");
    return;
  }
  const auto call = calls_[depth_ - 1];
  const auto kind = call.observation;
  const auto domain =
      grid::ObservationFor(kind) ? ObjectDomain::Grid : ObjectDomain::Entity;
  Identity *id = nullptr;
  const bool completed = normal && call.body_reached && call.body_returned;
  if (completed && !stopping_ && storage_.Ready()) {
    if (kind == Observation::Factory || kind == Observation::GridFactory ||
        kind == Observation::PlayerCtor) {
      const auto address =
          kind == Observation::PlayerCtor ? call.object : call.body_result;
      id = Find(domain, address);
      if (id) {
        const auto owner = CreationOwner(domain);
        // Factory 包含 ctor 时，同一次真实取得只计一次 use。
        if (!(kind == Observation::Factory && id->candidate &&
              id->owner_scope == owner &&
              id->lifecycle ==
                  static_cast<std::uint32_t>(Lifecycle::Constructed))) {
          if (!identities_.Acquire(*id, kind, kind == Observation::PlayerCtor,
                                   owner))
            Fault(Reason::SequenceExhausted, "acquire");
        }
        if (kind == Observation::PlayerCtor)
          id->lifecycle = static_cast<std::uint32_t>(Lifecycle::Constructed);
      }
    } else if (kind == Observation::Publish) {
      id = Find(ObjectDomain::Entity, call.args[0]);
      if (id)
        id->MarkPublished();
    } else if (kind == Observation::Init || kind == Observation::GridInit ||
               entity::Morph(kind) || kind == Observation::Remove ||
               kind == Observation::ClearReferences ||
               kind == Observation::Destroy ||
               kind == Observation::PlayerRestore) {
      id = identities_.Find(domain, call.object);
      if (id) {
        if (kind == Observation::Init || kind == Observation::GridInit)
          id->MarkInitReturned();
        if (entity::Morph(kind)) {
          id->readiness =
              static_cast<std::uint32_t>(Readiness::CreationOwnerCompleted);
          id->current_public_ready = true;
        }
        if (kind == Observation::Destroy)
          id->MarkDestroyed();
      }
    }
    if (kind == Observation::GridSpawn || kind == Observation::Door ||
        kind == Observation::GridFree) {
      const auto &c = layout::LayoutConstants();
      const auto limit = kind == Observation::Door ? c.room_door_slot_count
                                                   : c.room_grid_slot_count;
      const auto offset = kind == Observation::Door ? c.room_door_slot_offset
                                                    : c.room_grid_slot_offset;
      std::uint32_t after = 0;
      if (call.count && call.args[0] < limit) {
        if (!Value(std::uint64_t(call.object) + offset + call.args[0] * 4,
                   after))
          Fault(Reason::ReadFailed, "grid_after");
        else if (call.body_slot_known && after != call.body_after) {
          Event(EventKind::GapOrUnknown, Reason::Continuity, nullptr, &call);
        } else if (kind != Observation::GridFree &&
                   (kind != Observation::GridSpawn ||
                    (result && call.body_result)) &&
                   after) {
          id = Find(ObjectDomain::Grid, after);
          if (id && kind == Observation::Door && !call.before &&
              !call.before_grid_door && call.grid_index != kNoGridIndex) {
            std::uint32_t grid = 0;
            if (!Value(std::uint64_t(call.object) + c.room_grid_slot_offset +
                           call.grid_index * 4,
                       grid) ||
                grid != after)
              Fault(Reason::SourceChanged, "door_alias_after");
            else if (!id->candidate &&
                     !identities_.Acquire(*id, kind, true, call_id))
              Fault(Reason::SequenceExhausted, "door_acquire");
          }
          if (id && id->candidate && id->owner_scope == call_id)
            Birth(*id, kind);
        }
        // 槽真实改变后还需检查全部 Grid/door 别名，保留对象不冒称释放。
        const auto old_address =
            kind == Observation::Door ? call.before_grid : call.before;
        const bool native_replaced =
            call.body_slot_known && after == call.body_after &&
            after != old_address &&
            (kind != Observation::GridSpawn || call.body_result);
        if (old_address && native_replaced) {
          bool retained = false, readable = true;
          for (std::uint32_t i = 0;
               i < c.room_grid_slot_count + c.room_door_slot_count; ++i) {
            std::uint32_t value = 0;
            if (!Value(std::uint64_t(call.object) + c.room_grid_slot_offset +
                           i * 4,
                       value)) {
              readable = false;
              break;
            }
            retained = retained || value == old_address;
          }
          if (!readable)
            Fault(Reason::ReadFailed, "grid_aliases");
          else if (!retained)
            if (auto *old = identities_.Find(ObjectDomain::Grid, old_address)) {
              old->MarkDestroyed();
              Event(EventKind::GridPhysicalRetired, Reason::None, old, &call);
            }
        }
      }
    }
    if ((entity::CreationOwner(kind)||grid::CreationOwner(kind)) && kind != Observation::GridSpawn &&
        kind != Observation::Door)
      CompleteOwner(call_id, kind, result == call.body_result ? result : 0);
    if ((kind == Observation::GridSpawn || kind == Observation::Door) &&
        (kind == Observation::GridSpawn && !result)) {
      identities_.ForEach([&](Identity &value) noexcept {
        if (value.owner_scope == call_id) {
          value.candidate = false;
          value.readiness = 0;
        }
      });
    }
  }
  if (normal && !completed) {
    Event(EventKind::GapOrUnknown, Reason::NotReady, nullptr, &call);
    identities_.ForEach([&](Identity &value) noexcept {
      if (value.owner_scope == call_id) {
        value.owner_scope = 0;
        value.candidate = false;
        value.readiness = 0;
      }
    });
  }
  Event(EventKind::NativeHook, normal ? Reason::None : Reason::UnpairedScope,
        id, &call, false, !normal, result);
  --depth_;
  storage_.Diag().callbacks.fetch_sub(1, std::memory_order_relaxed);
  if (call.lifecycle)
    LifecycleLeave(normal);
  if (!normal) {
    storage_.Diag().callback_faults.fetch_add(1, std::memory_order_relaxed);
    Fault(Reason::UnpairedScope, "native_exception");
  }
  CloseIfDrained();
}
} // namespace cs::world
