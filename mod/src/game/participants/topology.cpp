#include "game/participants/topology.h"
#include "binding/offsets.h"
#include "game/common/bridge.h"
#include "game/participants/device_k.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "runtime.h"
#include "core/capture_cost.h"
#include "core/replication_memory.h"
#include "game/common/context.h"
#include "world/common/frame_read_regions.h"
#include "world/collector.h"
#include "world/observe.h"
#include <algorithm>
#include <limits>

namespace cs::game {
bool ReadMemory(const void *source,void *output,std::size_t size) noexcept {
  return IsMain()&&world::CheckedNativeCopy(reinterpret_cast<std::uintptr_t>(source),output,size,world::FrameRegionReader::ReadMemory);
}
const char *TopologyFaultName(TopologyFault fault) noexcept {
  switch (fault) {
  case TopologyFault::None:
    return "none";
  case TopologyFault::NotRun:
    return "not_run";
  case TopologyFault::PlayerList:
    return "player_list";
  case TopologyFault::Unreadable:
    return "unreadable";
  case TopologyFault::Identity:
    return "identity";
  case TopologyFault::Ambiguous:
    return "ambiguous";
  case TopologyFault::Room:
    return "room";
  case TopologyFault::Device:
    return "device";
  case TopologyFault::Ownership:
    return "ownership";
  }
  return "UNKNOWN";
}
namespace {
bool DecodeVectorRange(const std::array<uintptr_t, 3> &range, size_t maximum,
                       uintptr_t &begin, size_t &count) noexcept {
  if (range[1] < range[0] || range[2] < range[1] ||
      (range[0] % alignof(void *)) || (range[1] - range[0]) % sizeof(void *))
    return false;
  count = (range[1] - range[0]) / sizeof(void *);
  begin = range[0];
  return count <= maximum && (!count || begin);
}
} // namespace

// 只枚举已知 vector 的活动区间，容量终点不作为额外设备读取。
bool VectorRange(const void *object, size_t offset, size_t maximum,
                 uintptr_t &begin, size_t &count) noexcept {
  std::array<uintptr_t, 3> range{};
  return ReadAt(object, offset, range) &&
         DecodeVectorRange(range, maximum, begin, count);
}
bool RunActive() noexcept {
  bool run = false;
  return ReadAt(GamePointer(), offset::RunActive, run) && run;
}
// 受限读取：不检查设备注册，只确认 Player vector 与该 slot 的实体身份。
// 读取失败时区分不可读与身份不符，调用方据此决定中性或 PeerLost。
bool ReadPlayerSlot(unsigned ordinal, PlayerSlot &out) noexcept {
  out = {};
  if (!IsMain() || !Base || ordinal >= core::kMaximumParticipants)
    return false;
  InputLease lease(HostBusy());
  if (!lease.Ready())
    return false;
  auto *game = GamePointer();
  bool run = false;
  if (!ReadAt(game, offset::RunActive, run)) {
    out.fault = TopologyFault::Unreadable;
    out.reason = "run_unreadable";
    return false;
  }
  out.known = true;
  if (!run) {
    out.fault = TopologyFault::NotRun;
    out.reason = "not_in_run";
    return true;
  }
  uintptr_t players = 0;
  size_t count = 0;
  if (!VectorRange(game, offset::PlayerManager, 64, players, count)) {
    out.fault = TopologyFault::PlayerList;
    out.reason = "player_list_unknown";
    return false;
  }
  out.count = static_cast<unsigned>(count);
  // ordinal 只选择已确认控制组的根；原生列表重排不改变参与者身份。
  auto *runtime = Runtime();
  void *expected = runtime ? runtime->controlled_player[ordinal] : nullptr;
  if (!expected && ordinal == 0 && count)
    ReadMemory(reinterpret_cast<void *>(players), &expected, sizeof(expected));
  for (size_t i = 0; i < count; ++i) {
    void *candidate = nullptr;
    if (!ReadMemory(reinterpret_cast<void *>(players + i * sizeof(void *)),
                    &candidate, sizeof(candidate)))
      return false;
    if (candidate == expected)
      out.player = candidate;
  }
  if (!out.player) {
    out.fault = TopologyFault::PlayerList;
    out.reason = "player_slot_missing";
    return false;
  }
  if (!ReadAt(out.player, offset::PlayerIndex, out.index) ||
      !ReadAt(out.player, offset::EntityType, out.type) ||
      !ReadAt(out.player, offset::EntityVariant, out.variant) ||
      !ReadAt(out.player, offset::PlayerType, out.character) ||
      !ReadAt(out.player, offset::ControllerIndex, out.controller) ||
      !ReadAt(out.player, offset::EntityValid, out.valid) ||
      !ReadAt(out.player, offset::EntityExists, out.exists) ||
      !ReadAt(out.player, offset::EntityDead, out.dead) ||
      !ReadAt(out.player, offset::EntityRemoved, out.removed) ||
      !ReadAt(out.player, offset::CoopGhost, out.ghost)) {
    out.fault = TopologyFault::Unreadable;
    out.reason = "player_fields_unreadable";
    return false;
  }
  if (out.type != 1 || out.variant != 0) {
    out.fault = TopologyFault::Identity;
    out.reason = "player_slot_identity";
    return false;
  }
  out.identity = true;
  out.fault = TopologyFault::None;
  out.reason = "slot_known";
  return true;
}
namespace {
constexpr unsigned kRosterLimit = 64, kDeviceLimit = 256;
constexpr std::size_t kRelations[] = {0x1D84, 0x1E4C, 0x1E50, 0x267C};
constexpr BodyRole kRelationRoles[] = {BodyRole::SubPlayer, BodyRole::Twin,
                                      BodyRole::Backup, BodyRole::Replaced};

// 保留本批的失败与成本计量。具体页权限由 Manager 帧区表供给。
class TopologyReader {
public:
  void Begin() noexcept {
    counts = {};
    stale = false;
  }

  template <class T>
  bool At(const void *object, std::size_t offset, T &value) noexcept {
    const auto base = reinterpret_cast<std::uintptr_t>(object);
    return base && offset < world::kNativeAddressLimit - base
               ? Copy(base + offset, &value, sizeof(value))
               : Reject();
  }
  bool Copy(std::uintptr_t address, void *output, std::size_t size) noexcept {
    world::NativeReadCounts read;
    bool copy_fault=false;
    const bool ok=world::CheckedNativeCopy(address,output,size,world::FrameRegionReader::Topology,&read,&copy_fault);
    counts.region_queries+=read.queries;counts.region_hits+=read.hits;counts.region_fallbacks+=read.fallbacks;
    if(ok)return true;
    stale|=copy_fault;
    return Reject();
  }

  bool Range(const void *object, std::size_t offset, std::size_t maximum,
             std::uintptr_t &begin, std::size_t &count) noexcept {
    std::array<std::uintptr_t, 3> range;
    return At(object, offset, range) && DecodeVectorRange(range, maximum, begin, count);
  }

  capture::TopologyCounts counts{};
  bool stale = false;

private:
  bool Reject() noexcept {
    ++counts.read_failures;
    return false;
  }
};

struct GroupRead {
  unsigned ordinal;
  void *root;
  void *owned_device;
  Topology *out;
};
using GroupReads = std::array<GroupRead, core::kMaximumParticipants>;

// J273 7D1C90 为纯读叶，只读 Exists、PlayerType、twin 及双方索引。
// 官方 REPENTOGON 1.1.3 的 IsHologram 包装直达该入口，项目也未挂该点。
bool ReadHologram(void *player, bool &value) noexcept {
  __try {
    value = PlayerIsHologram(player);
    return true;
  } __except (world::NativeReadExceptionFilter(GetExceptionCode())) {
    return false;
  }
}

// 借用本次调用的运行期资格。即使 Game 帧号相同，也不跨原生 Update、
// 生命周期或绑定变更复用；lease 在交还结果之前已经释放。
struct TopologyBoundary {
  SessionRuntime *runtime = Runtime();
  std::uint64_t session = runtime ? SessionGeneration(*runtime) : 0;
  std::uint64_t query = runtime ? runtime->query_epoch : 0;
  std::uint64_t loop = ManagerLoop();
  std::uint64_t context_errors = ContextErrors();
  std::uint64_t owner_sequence = OwnerSequence();
  std::uint64_t observation_faults = world::ObservationFailureSequence();
  Owner owner = TopOwner();
  unsigned blocks = runtime ? runtime->blocks : 0;
  unsigned join = runtime ? runtime->join_ordinal : 0;
  unsigned lifecycle = runtime ? runtime->control_lifecycle_depth : 0;
  bool finished = runtime && runtime->finished;
  bool retiring = runtime && runtime->retiring;
  bool failure = runtime && runtime->failure;

  bool Current() const noexcept {
    return OwnerSequence() == owner_sequence && TopOwner() == owner &&
           ContextErrors() == context_errors && !ForeignCallPending() &&
           world::ObservationFailureSequence() == observation_faults &&
           Runtime() == runtime && ManagerLoop() == loop &&
           (!runtime || (SessionGeneration(*runtime) == session &&
                         runtime->query_epoch == query && runtime->blocks == blocks &&
                         runtime->join_ordinal == join &&
                         runtime->control_lifecycle_depth == lifecycle &&
                         runtime->finished == finished && runtime->retiring == retiring &&
                         runtime->failure == failure));
  }
};

// 只在当前同步调用中以真实地址索引 Player 事实。roster 外的已知关系可入表，
// 仍须通过完整 Player 身份检查；事实在批次结束后失去读取资格。
class TopologyReadBatch {
public:
  static std::size_t WorkspaceBytes() noexcept {
    // 固定工作表和同时在用的投影／提交暂存共用一个额度，读取热路径不重复记账。
    return sizeof(TopologyReadBatch) + sizeof(TopologyGroups) + sizeof(GroupReads) +
           world::Collector::AnchorRefreshWorkspaceBytes();
  }
  void Read(std::span<const GroupRead> groups) noexcept {
    if (busy_) {
      revoked_ = true;
      return;
    }
    busy_ = true;
    revoked_ = false;
    reader_.Begin();
    auto &counts = reader_.counts;
    counts.batches = 1;
    counts.groups = groups.size();
    common_ = {};
    player_count_ = fact_count_ = device_count_ = 0;
    esau_read_ = 0;
    device_ready_ = false;
    device_fault_ = nullptr;
    const TopologyBoundary boundary;
    {
      // 只在批次租约入口计时，单独观察墙钟等待和线程 CPU。
      const bool required=HostBusy();
      auto lease=[required]{capture::Cost cost(capture::CostPart::TopologyLease,required);return InputLease(required);}();
      counts.lease_acquires = lease.Acquired();
      if (!lease.Ready() || !boundary.Current())
        revoked_ = true;
      if (!revoked_) {
        if (ReadCommon()) {
          capture::Cost cost(capture::CostPart::TopologyGroups);
          for (const auto &group : groups) {
            if (revoked_ || reader_.stale)
              break;
            ReadGroup(group, boundary);
          }
        } else {
          for (const auto &group : groups)
            static_cast<TopologyCommon &>(*group.out) = common_;
        }
      }
    }
    if (revoked_ || reader_.stale || !boundary.Current()) {
      for (const auto &group : groups)
        *group.out = {};
      counts.revoked = 1;
    }
    capture::CountTopology(counts);
    busy_ = false;
  }

private:
  struct PlayerFact {
    void *player = nullptr;
    ControlBody body{};
    unsigned type = 0, variant = 0;
    bool controller_read = false, identity_read = false, body_read = false;
    bool relations_read = false, actives_read = false;
    std::array<int, 4> esau_associations{};
  };

  bool ReadCommon() noexcept;
  void ReadGroup(const GroupRead &, const TopologyBoundary &) noexcept;
  PlayerFact *Fact(void *player) noexcept;
  bool Body(PlayerFact &) noexcept;
  bool Controller(PlayerFact &) noexcept;
  bool Relations(PlayerFact &) noexcept;
  bool EsauSlots(PlayerFact &, Topology &) noexcept;
  void ReadDevices() noexcept;
  bool BindDevice(const GroupRead &, Topology &) noexcept;

  memory::Charge memory_{memory::Pool::Workspace, WorkspaceBytes(), "native_topology_batch"};
  TopologyReader reader_;
  TopologyCommon common_{};
  std::array<void *, kRosterLimit> roster_{};
  std::array<PlayerFact, kRosterLimit + core::kMaximumParticipants * kMaximumGroupBodies> facts_{};
  std::array<void *, kDeviceLimit> devices_{};
  std::array<int, kDeviceLimit> device_ids_{};
  std::array<void *, 4> esau_backups_{};
  std::size_t player_count_ = 0, fact_count_ = 0, device_count_ = 0;
  unsigned esau_read_ = 0;
  const char *device_fault_ = nullptr;
  bool device_ready_ = false, busy_ = false, revoked_ = false;
};

bool TopologyReadBatch::ReadCommon() noexcept {
  capture::Cost cost(capture::CostPart::TopologyCommon);
  ++reader_.counts.common_reads;
  auto &out = common_;
  if (!reader_.At(reinterpret_cast<void *>(Base), offset::Game, out.room.game) ||
      !reader_.At(out.room.game, offset::RunActive, out.run))
    return false;
  if (!out.run) {
    out.known = true;
    out.fault = TopologyFault::NotRun;
    out.reason = "not_in_run";
    return false;
  }
  void *manager = nullptr;
  std::array<unsigned, 2> stage;
  if (!reader_.At(reinterpret_cast<void *>(Base), offset::Manager, manager) ||
      !reader_.At(manager, offset::Focus, out.focused) ||
      !reader_.At(manager, offset::PauseOnFocusLost, out.pause_on_focus_lost) ||
      !reader_.At(manager, offset::Starting, out.starting) ||
      !reader_.At(out.room.game, offset::CurrentRoom, out.room.room) ||
      !reader_.At(out.room.game, 0, stage) ||
      !reader_.At(out.room.game, offset::RoomIndex, out.room.index) ||
      !reader_.At(out.room.game, offset::Dimension, out.room.dimension) ||
      !reader_.At(out.room.game, offset::PauseScreen, out.pause) ||
      !reader_.At(out.room.game, offset::RoomTransition, out.room_transition) ||
      !reader_.At(out.room.game, offset::StageTransition, out.stage_transition)) {
    out.fault = TopologyFault::Unreadable;
    out.reason = "game_fields_unreadable";
    return false;
  }
  out.room.stage = stage[0];
  out.room.stage_type = stage[1];
  out.known = true;
  ++reader_.counts.roster_reads;
  std::uintptr_t begin = 0;
  const bool range = reader_.Range(out.room.game, offset::PlayerManager,
                                   roster_.size(), begin, player_count_);
  out.player_count = static_cast<unsigned>(player_count_);
  if (!range || !player_count_) {
    out.fault = TopologyFault::PlayerList;
    out.reason = "player_list_unknown";
    return false;
  }
  if (!reader_.Copy(begin, roster_.data(), player_count_ * sizeof(void *))) {
    out.fault = TopologyFault::Unreadable;
    out.reason = "player_pointer_unreadable";
    return false;
  }
  return true;
}

TopologyReadBatch::PlayerFact *TopologyReadBatch::Fact(void *player) noexcept {
  for (std::size_t i = 0; i < fact_count_; ++i)
    if (facts_[i].player == player)
      return &facts_[i];
  if (!player || fact_count_ == facts_.size())
    return nullptr;
  auto &fact = facts_[fact_count_++];
  fact = {};
  fact.player = fact.body.player = player;
  return &fact;
}

bool TopologyReadBatch::Controller(PlayerFact &fact) noexcept {
  if (fact.controller_read) {
    ++reader_.counts.fact_hits;
    return true;
  }
  fact.controller_read = reader_.At(fact.player, offset::ControllerIndex, fact.body.controller);
  return fact.controller_read;
}

bool TopologyReadBatch::Body(PlayerFact &fact) noexcept {
  if (fact.body_read) {
    ++reader_.counts.fact_hits;
    return true;
  }
  if (!fact.identity_read) {
    static_assert(offset::EntityVariant == offset::EntityType + sizeof(unsigned));
    std::array<unsigned, 2> identity;
    if (!reader_.At(fact.player, offset::EntityType, identity))
      return false;
    fact.type = identity[0];
    fact.variant = identity[1];
    fact.identity_read = true;
  }
  if (fact.type != 1 || fact.variant != 0)
    return false;
  auto &body = fact.body;
  static_assert(offset::PlayerIndex == offset::ControllerIndex + sizeof(int));
  if (!fact.controller_read) {
    std::array<int, 2> indices;
    if (!reader_.At(fact.player, offset::ControllerIndex, indices))
      return false;
    body.controller = indices[0];
    body.index = indices[1];
    fact.controller_read = true;
  } else if (!reader_.At(fact.player, offset::PlayerIndex, body.index)) {
    return false;
  }
  fact.body_read = reader_.At(fact.player, offset::PlayerType, body.character) &&
                   reader_.At(fact.player, offset::EntityValid, body.valid) &&
                   reader_.At(fact.player, offset::EntityExists, body.exists) &&
                   reader_.At(fact.player, offset::EntityRemoved, body.removed) &&
                   reader_.At(fact.player, offset::EntityDead, body.dead) &&
                   reader_.At(fact.player, offset::CoopGhost, body.ghost);
  return fact.body_read;
}

bool TopologyReadBatch::Relations(PlayerFact &fact) noexcept {
  if (fact.relations_read) {
    ++reader_.counts.fact_hits;
    return true;
  }
  // 只合并紧邻的 twin／backup，不覆盖其他 Player 字段或关系之间的空洞。
  static_assert(kRelations[2] == kRelations[1] + sizeof(void *));
  std::array<void *, 2> twin_backup;
  auto &r = fact.body.relations;
  if (!reader_.At(fact.player, kRelations[0], r[0]) ||
      !reader_.At(fact.player, kRelations[1], twin_backup) ||
      !reader_.At(fact.player, kRelations[3], r[3]))
    return false;
  r[1] = twin_backup[0];
  r[2] = twin_backup[1];
  fact.relations_read = true;
  return true;
}

bool TopologyReadBatch::EsauSlots(PlayerFact &fact, Topology &out) noexcept {
  if (fact.actives_read) {
    ++reader_.counts.fact_hits;
    return true;
  }
  for (unsigned i = 0; i < fact.esau_associations.size(); ++i) {
    const auto stride = i * offset::ActiveItemSlotStride;
    int item = 0, association = 0;
    if (!reader_.At(fact.player, offset::ActiveItemSlotBase + stride, item)) {
      out.fault = TopologyFault::Unreadable;
      out.reason = "active_item_unreadable";
      return false;
    }
    if (item == 703) {
      if (!reader_.At(fact.player, offset::EsauJrAssociation + stride, association)) {
        out.fault = TopologyFault::Unreadable;
        out.reason = "esau_jr_slot_unreadable";
        return false;
      }
      if (association < 0 || association > static_cast<int>(esau_backups_.size())) {
        out.fault = TopologyFault::Identity;
        out.reason = "esau_jr_slot_invalid";
        return false;
      }
    }
    fact.esau_associations[i] = association;
  }
  fact.actives_read = true;
  return true;
}

void TopologyReadBatch::ReadDevices() noexcept {
  if (device_ready_)
    return;
  capture::Cost cost(capture::CostPart::TopologyCommon);
  device_ready_ = true;
  ++reader_.counts.device_reads;
  std::uintptr_t begin = 0;
  if (!reader_.Range(reinterpret_cast<void *>(Base + offset::InputManager),
                     offset::DeviceVector, devices_.size(), begin, device_count_)) {
    device_fault_ = "device_list_unknown";
    return;
  }
  if (device_count_ && !reader_.Copy(begin, devices_.data(), device_count_ * sizeof(void *))) {
    device_fault_ = "device_identity_unknown";
    return;
  }
  for (std::size_t i = 0; i < device_count_; ++i)
    if (!reader_.At(devices_[i], offset::DeviceId, device_ids_[i])) {
      device_fault_ = "device_identity_unknown";
      return;
    }
}

bool TopologyReadBatch::BindDevice(const GroupRead &group, Topology &out) noexcept {
  if (out.pair.key >= 0)
    ReadDevices();
  if (out.pair.key < 0 || device_fault_) {
    out.pair.player = nullptr;
    out.fault = TopologyFault::Device;
    out.reason = out.pair.key < 0 ? "device_list_unknown" : device_fault_;
    return false;
  }
  unsigned matches = 0;
  for (std::size_t i = 0; i < device_count_; ++i)
    if (device_ids_[i] == out.pair.key) {
      ++matches;
      out.pair.device = devices_[i];
    }
  std::uintptr_t vtable = 0;
  bool enabled = false;
  const bool readable = matches == 1 && reader_.At(out.pair.device, 0, vtable) &&
                        reader_.At(out.pair.device, offset::DeviceEnabled, enabled);
  // 本机只接受原生物理后端。远端必须仍由该 ordinal 自有的 K 持有。
  const bool ownership_ok =
      readable &&
      (group.ordinal == 0 ? (vtable == Base + offset::KeyboardVtable ||
                             vtable == Base + offset::GamepadVtable)
                          : (group.owned_device && out.pair.device == group.owned_device &&
                             IsDeviceK(out.pair.device)));
  if (ownership_ok && enabled)
    return true;
  out.pair.player = nullptr;
  if (matches > 1) {
    out.fault = TopologyFault::Ambiguous;
    out.reason = "ambiguous_device";
  } else if (group.ordinal != 0 && matches != 1) {
    out.fault = TopologyFault::Ownership;
    out.reason = "device_missing";
  } else if (group.ordinal != 0 && readable && !ownership_ok) {
    out.fault = TopologyFault::Ownership;
    out.reason = "device_ownership_mismatch";
  } else {
    out.fault = TopologyFault::Device;
    out.reason = "device_unavailable";
  }
  return false;
}

void TopologyReadBatch::ReadGroup(const GroupRead &group,
                                  const TopologyBoundary &boundary) noexcept {
  auto &out = *group.out;
  static_cast<TopologyCommon &>(out) = common_;
  const auto end = roster_.begin() + player_count_;
  const auto in_roster = [&](void *player) noexcept {
    return std::find(roster_.begin(), end, player) != end;
  };
  auto *root = group.root;
  if (!root && group.ordinal == 0)
    root = roster_[0];
  // 缺席旧根只作身份比较；从当前 roster 的四条已知关系寻找唯一替换。
  if (root && !in_roster(root)) {
    void *replacement = nullptr;
    for (auto it = roster_.begin(); it != end; ++it) {
      auto *fact = Fact(*it);
      if (!fact || !Relations(*fact)) {
        out.fault = TopologyFault::Unreadable;
        return;
      }
      const auto &r = fact->body.relations;
      if (std::find(r.begin(), r.end(), root) == r.end())
        continue;
      if (replacement && replacement != *it) {
        out.fault = TopologyFault::Ambiguous;
        return;
      }
      replacement = *it;
    }
    root = replacement;
  }
  if (!root) {
    out.fault = TopologyFault::Identity;
    out.reason = "group_root_missing";
    return;
  }

  auto *root_fact = Fact(root);
  if (!root_fact || !Body(*root_fact)) {
    const bool identity_mismatch = root_fact && root_fact->identity_read &&
                                   (root_fact->type != 1 || root_fact->variant != 0);
    out.fault = identity_mismatch ? TopologyFault::Identity : TopologyFault::Unreadable;
    out.reason = identity_mismatch ? "player_identity_unknown" : "player_fields_unreadable";
    return;
  }
  out.pair.player = root;
  out.pair.key = root_fact->body.controller;
  out.variant = root_fact->variant;
  out.character = root_fact->body.character;
  out.valid = root_fact->body.valid;
  out.exists = root_fact->body.exists;
  out.dead = root_fact->body.dead;
  out.removed = root_fact->body.removed;
  out.ghost = root_fact->body.ghost;

  const auto append = [&](void *player, BodyRole role) noexcept {
    if (!player || out.Contains(player, false))
      return true;
    if (out.body_count == out.bodies.size())
      return false;
    auto &body = out.bodies[out.body_count++];
    body.player = player;
    body.role = role;
    return true;
  };
  append(root, BodyRole::Primary);
  for (unsigned i = 0; i < out.body_count; ++i) {
    auto &body = out.bodies[i];
    auto *fact = i == 0 ? root_fact : Fact(body.player);
    if (!fact || !Body(*fact)) {
      out.fault = TopologyFault::Unreadable;
      out.reason = "group_body_unreadable";
      return;
    }
    const auto role = body.role;
    body = fact->body;
    body.role = role;
    body.roster = in_roster(body.player);
    if (body.valid && body.exists && !body.removed) {
      ++reader_.counts.hologram_calls;
      if (!ReadHologram(body.player, body.hologram)) {
        ++reader_.counts.read_failures;
        revoked_ = true;
      }
      if (revoked_ || !boundary.Current()) {
        revoked_ = true;
        return;
      }
    }
    const bool control_role = role == BodyRole::Primary || role == BodyRole::Twin ||
                              role == BodyRole::SubPlayer;
    body.active = control_role && body.valid && body.exists && !body.removed &&
                  body.controller == out.pair.key;
    // 非 roster 的 16/17 形态交换对象仍需要本绑定的实际更新见证。
    body.native_activity_required = role == BodyRole::SubPlayer && !body.roster &&
                                    (body.character == 16 || body.character == 17);
    if (body.native_activity_required) {
      if (auto *r = boundary.runtime; r && r->players[group.ordinal].BoundValid())
        for (const auto &witness : r->body_activity[group.ordinal])
          if (witness.player == body.player &&
              witness.generation == r->players[group.ordinal].Generation() &&
              witness.tick && witness.tick <= r->clock.Completed())
            body.native_activity_seen = true;
    }
    out.active_bodies += body.CanConsume();
    // 703 的备用槽来自该身体实际持物元数据。四个 Game 槽在同批按需复制。
    if (control_role) {
      if (!EsauSlots(*fact, out))
        return;
      for (const int association : fact->esau_associations) {
        if (!association)
          continue;
        const unsigned slot = association - 1, bit = 1u << slot;
        if (!(esau_read_ & bit) &&
            !reader_.At(out.room.game, offset::PlayerManager + offset::EsauJrStates +
                         slot * sizeof(void *), esau_backups_[slot])) {
          out.fault = TopologyFault::Unreadable;
          out.reason = "esau_jr_backup_unknown";
          return;
        }
        esau_read_ |= bit;
        if (!append(esau_backups_[slot], BodyRole::EsauJr)) {
          out.fault = TopologyFault::Unreadable;
          out.reason = "esau_jr_backup_unknown";
          return;
        }
      }
    }
    if (!Relations(*fact)) {
      out.fault = TopologyFault::Unreadable;
      out.reason = "group_relation_unknown";
      return;
    }
    body.relations = fact->body.relations;
    for (unsigned j = 0; j < body.relations.size(); ++j)
      if (!append(body.relations[j], control_role ? kRelationRoles[j] : BodyRole::Inactive)) {
        out.fault = TopologyFault::Ambiguous;
        out.reason = "group_relation_overflow";
        return;
      }
  }
  // 同 controller 的每一项 roster 都要由当前控制组解释，缓存只复用已读的值。
  for (auto it = roster_.begin(); it != end; ++it) {
    auto *fact = Fact(*it);
    if (!fact || !Controller(*fact)) {
      out.fault = TopologyFault::Unreadable;
      return;
    }
    if (fact->body.controller == out.pair.key && !out.Contains(*it, false)) {
      out.fault = TopologyFault::Ambiguous;
      out.reason = "unowned_shared_controller";
      return;
    }
  }
  if (!BindDevice(group, out))
    return;
  const auto body_end = out.bodies.begin() + out.body_count;
  const bool structural = std::any_of(out.bodies.begin(), body_end,
                                      [](const ControlBody &body) { return body.active; });
  out.unique = out.room.room && structural;
  out.fault = out.unique ? TopologyFault::None : TopologyFault::Room;
  out.reason = out.unique ? "unique" : "room_unknown";
}

void ReadBatch(std::span<const GroupRead> groups) noexcept {
  if (!IsMain() || !Base || groups.empty())
    return;
  // 常驻复用的是容量。只在一个同步调用内借用原生地址，嵌套读取撤销外批。
  try {
    static TopologyReadBatch workspace;
    workspace.Read(groups);
  } catch (...) {
    capture::TopologyCounts failed{};
    failed.batches = failed.revoked = 1;
    failed.groups = groups.size();
    capture::CountTopology(failed);
  }
}
} // namespace

std::size_t TopologyReadWorkspaceBytes() noexcept {
  return TopologyReadBatch::WorkspaceBytes();
}

Topology ReadTopology(unsigned ordinal, void *owned_device) noexcept {
  auto *r = Runtime();
  void *root = r && ordinal < core::kMaximumParticipants ? r->controlled_player[ordinal] : nullptr;
  return ReadTopologyForRoot(ordinal, root, owned_device);
}

Topology ReadTopologyForRoot(unsigned ordinal, void *root, void *owned_device) noexcept {
  Topology result;
  if (ordinal < core::kMaximumParticipants) {
    const GroupRead group{ordinal, root, owned_device, &result};
    ReadBatch(std::span(&group, 1));
  }
  return result;
}

// 多组共享公共事实，但一份身体（含非活动关系）只能归属一个参与者。
unsigned ReadTopologies(unsigned mask, const TopologyDevices &devices,
                        TopologyGroups &out) noexcept {
  GroupReads requests;
  std::size_t used = 0;
  auto *r = Runtime();
  for (unsigned i = 0; i < requests.size(); ++i)
    if (mask & (1u << i)) {
      out[i] = {};
      requests[used++] = {i, r ? r->controlled_player[i] : nullptr, devices[i], &out[i]};
    }
  ReadBatch(std::span(requests.data(), used));
  capture::Cost cost(capture::CostPart::TopologyGroups,used!=0);
  unsigned conflicts = 0;
  for (std::size_t i = 0; i < used; ++i)
    for (std::size_t j = i + 1; j < used; ++j) {
      const auto &left = *requests[i].out, &right = *requests[j].out;
      const auto end = left.bodies.begin() + left.body_count;
      const bool overlap = std::any_of(left.bodies.begin(), end, [&](const ControlBody &body) {
        return right.Contains(body.player, false);
      });
      if (overlap)
        conflicts |= (1u << requests[i].ordinal) | (1u << requests[j].ordinal);
    }
  for (unsigned i = 0; i < out.size(); ++i)
    if (conflicts & (1u << i)) {
      out[i].unique = false;
      out[i].fault = TopologyFault::Ambiguous;
      out[i].reason = "cross_participant_body_conflict";
    }
  return conflicts;
}
int DeviceOccurrences(const void *device) noexcept {
  if (!IsMain() || !device)
    return -1;
  InputLease lease(true);
  uintptr_t begin = 0;
  size_t count = 0;
  if (!lease.Ready() ||
      !VectorRange(reinterpret_cast<void *>(Base + offset::InputManager),
                   offset::DeviceVector, 256, begin, count))
    return -1;
  int matches = 0;
  for (size_t i = 0; i < count; ++i) {
    void *p = nullptr;
    if (!ReadMemory(reinterpret_cast<void *>(begin + i * 4), &p, sizeof(p)))
      return -1;
    if (p == device)
      ++matches;
  }
  return matches;
}
bool SameLife(const Topology &a, const Topology &b) noexcept {
  return a.pair.player == b.pair.player && a.valid == b.valid &&
         a.exists == b.exists && a.dead == b.dead && a.removed == b.removed &&
         a.ghost == b.ghost;
}
// 构造取得的原生 id 必须在注册前空闲，注册后恰好出现一次；不是 vector 下标。
int DeviceIdOccurrences(int id) noexcept {
  if (!IsMain() || id < 0)
    return -1;
  InputLease lease(true);
  uintptr_t begin = 0;
  size_t count = 0;
  if (!lease.Ready() ||
      !VectorRange(reinterpret_cast<void *>(Base + offset::InputManager),
                   offset::DeviceVector, 256, begin, count))
    return -1;
  int matches = 0;
  for (size_t i = 0; i < count; ++i) {
    void *device = nullptr;
    int current = -1;
    if (!ReadMemory(reinterpret_cast<void *>(begin + i * 4), &device, 4) ||
        !ReadAt(device, offset::DeviceId, current))
      return -1;
    if (current == id)
      ++matches;
  }
  return matches;
}
} // namespace cs::game

namespace cs::game {
bool SameControlGroup(const Topology &a, const Topology &b) noexcept {
  if (!(a.pair == b.pair) || a.body_count != b.body_count)
    return false;
  for (unsigned i = 0; i < a.body_count; ++i) {
    auto left = a.bodies[i], right = b.bodies[i];
    left.native_activity_seen = right.native_activity_seen = false;
    left.index = right.index =
        -1; // 原生RemoveCoPlayer重编号不改变其余参与者归属。
    if (!(left == right))
      return false;
  }
  return true;
}
} // namespace cs::game
