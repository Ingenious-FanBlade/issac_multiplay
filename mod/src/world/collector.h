#pragma once
// tx-04-08 主线程只读采集器。准备、文件和后台线程由录制 owner 管理；
// 本对象在后台准备完成后交给主线程，所有入口均零等待、无运行期分配。
#include "core/tick_pipeline.h"
#include "world/identity.h"
#include "world/common/native_source.h"
#include "world/common/membership.h"
#include "world/storage.h"
#include <span>

namespace cs::world {

// 只允许有界原始内存拷贝；实现必须捕获不可读地址，不能调用游戏接口。
struct MemoryReader final {
  bool (*copy)(std::uint32_t, void *, std::size_t) noexcept = nullptr;
  std::uint32_t image_base = 0, image_size = 0, main_thread = 0;
  // 后台只在冷准备读取当前模块的 PE/RTTI 静态元数据，游戏对象仍通过主线程 copy 读取。
  bool (*copy_image)(std::uint32_t, void *, std::size_t) noexcept = nullptr;
  // 正式原生读取器可提供单次纯读分类窗口；定制来源未提供时仍逐次走 copy。
  // 实现必须复用 ClassifyNative 的完整 RTTI 检查，不得缓存对象类别或跨回调页见证。
  Class (*classify)(ObjectDomain,std::uint32_t,std::uint32_t,std::uint32_t,
                    std::uint32_t &,bool &) noexcept = nullptr;
  // 单次纯读核Game指针及其Room／索引／维度；偏移来自当前可信布局。
  // 可复用本次调用内的页见证，但每个根值仍读取实际字节，不能跨调用缓存。
  bool (*roots)(std::uint32_t,const std::array<std::uint32_t,4> &,
                std::array<std::uint32_t,4> &) noexcept = nullptr;
};
// 本 tick 的真实凭据来自 NativeReturned，不能使用最近 completed tick。
struct Authority final {
  std::uint64_t loop = 0, outer = 0, tick = 0, scope = 0;
  AuthorityKind kind = AuthorityKind::None;
};
// A/B 当前 exact pair 和代次；地址仅用于本录制内关联。
struct PlayerAnchor final {
  std::uint32_t player = 0, device = 0;
  std::int32_t key = -1;
  std::uint64_t session = 0, generation = 0;
  bool valid = false;
  bool operator==(const PlayerAnchor &) const = default;
};
using PlayerAnchorSet = std::array<PlayerAnchor, kControlAnchorCapacity>;
// 当前outer内真实派生更新的逐身体证言；持久读取资格与本步是否更新是两件事。
struct PlayerPostWitness final {
  PlayerAnchor anchor{};
  std::uint64_t local=0,use=0,shape=0,tick=0,outer=0,owner=0,mutation=0,run=0,room=0;
  AuthorityKind kind=AuthorityKind::None;
  bool operator==(const PlayerPostWitness &) const = default;
};
// 正式二进制读取只借用已完成的主线程窗口；不依赖旧字段是否进入Recorder／Lua样本。
struct NativeCaptureWindow {
  std::uint64_t generation=0,run=0,room=0,outer=0,owner=0,tick=0,mutation=0,attempt=0,capture=0;
  std::uint32_t game=0,room_address=0,game_frame=0,body_slot=kNoOrdinal;
  bool ordinary=false;
  std::uint64_t wrong_thread=0,native_exception=0;
  bool production=false,production_outer=false,display_transition=false;
  bool operator==(const NativeCaptureWindow &) const = default;
};

// 采集服务的窄提交通知；录制器不拥有 Collector，也不能决定 Authority 是否提交。
struct CaptureSink final {
  void *owner=nullptr;
  void (*lua)(void *,const BatchHeader &,const RawObject *,const RawMembership *) noexcept=nullptr;
  void (*native)(void *,const BatchHeader &,const RawObject *,const RawMembership *) noexcept=nullptr;
  void (*commit)(void *,bool,bool) noexcept=nullptr;
  void (*event)(void *,const LifecycleEvent &) noexcept=nullptr;
  void (*gap)(void *,const GapRecord &) noexcept=nullptr;
};

class Collector final {
public:
  Collector(WorldStorage &storage, IdentityTable &identities,
            MemoryReader reader, std::uint32_t generation) noexcept;
  ~Collector();
  // 主线程接收已准备上下文时调用；失败不影响 Authority。
  bool Activate() noexcept;
  void SetCaptureSink(CaptureSink sink) noexcept { sink_=sink; }
  void SetLegacyDiagnostics(bool enabled) noexcept {
    if(enabled&&!legacy_diagnostics_)legacy_output_failed_=false;
    legacy_diagnostics_=enabled;
  }
  bool LegacyDiagnostics() const noexcept { return legacy_diagnostics_&&!LegacyOutputFailed(); }
  bool LegacyOutputFailed() const noexcept { return legacy_output_failed_||storage_.DiagnosticReaderFailed(); }
  std::uint64_t LegacyAttempts() const noexcept { return legacy_attempts_; }
  void CommitNativeMemberships(const NativeCaptureWindow &,std::span<const RawSource>,std::span<const std::uint32_t>,
                               std::span<const std::uint32_t>) noexcept;
  bool Recording() const noexcept;
  bool Participating() const noexcept { return active_; }
  bool Drained() const noexcept;
  void Stop(Reason reason = Reason::Stopped) noexcept;
  void Fault(Reason reason, const char *stage) noexcept;
  bool Checkpoint(std::uint64_t request, const char *label) noexcept;
  // Manager 最外层进入（在 BeginManager/命令之前）；阶段只能在同 outer 配对。
  void BeginOuter(std::uint64_t loop, std::uint64_t outer, bool run) noexcept;
  void EnterStage(Stage stage, std::uint64_t scope,
                  std::uint32_t object = 0) noexcept;
  void LeaveStage(Stage stage, std::uint64_t scope, bool normal) noexcept;
  void NativeReturned(const Authority &authority) noexcept;
  core::TickPostState PreparePostState(bool normal) noexcept;
  // clock.Complete 成功之后、PublishCompleted/Ack 之前释放唯一匹配批次。
  void PublishPostState(bool committed, const core::TickPostState &post) noexcept;
  // 无 Authority 的 Manager 同样保留 gap，不伪造 tick；异常只撤销 pending。
  void EndOuter() noexcept;
  // 生命周期嵌套合并一个 episode；进入立即污染当前窗口并撤销 witness。
  void LifecycleEnter(bool run_end = false) noexcept;
  // 实际 Start owner 进入即建立新 run 域，覆盖本次构造，不能等下帧再丢掉出生记录。
  void RunBeginning() noexcept;
  void RunOwnerReturned(bool normal) noexcept;
  void LifecycleLeave(bool normal) noexcept;
  void RevokePlayer(std::uint32_t ordinal) noexcept;
  void AnchorPlayer(std::uint32_t ordinal, const PlayerAnchor &anchor) noexcept;
  // 全体目标先判重；只撤销／安装变化槽。通知使用最终实际提交的锚点。
  unsigned ReplacePlayerAnchors(PlayerAnchorSet &desired,
      void (*retire)(std::uint32_t address) noexcept,
      void (*changed)(unsigned slot,const PlayerAnchor &before,const PlayerAnchor &after) noexcept) noexcept;
  static std::size_t AnchorRefreshWorkspaceBytes() noexcept;
  std::uint64_t EntityLocalId(std::uint32_t address) noexcept {
    auto *identity = identities_.Find(ObjectDomain::Entity, address);
    return identity && !identity->Retired() ? identity->local_id : 0;
  }
  // 事件只借用观察器已确认的上下文；加载中的旧 room 不能当作当前归属。
  std::uint64_t CurrentRunToken() const noexcept { return run_active_?run_:0; }
  std::uint64_t CurrentRoomToken() const noexcept { return run_active_&&!lifecycle_depth_&&!lifecycle_episode_?room_:0; }
  std::uint64_t CurrentOuterToken() const noexcept { return outer_active_?outer_:0; }
  std::uint64_t CurrentMutation() const noexcept { return mutation_; }
  PlayerAnchor CurrentAnchor(std::uint32_t slot) const noexcept {
    return slot < anchors_.size() ? anchors_[slot] : PlayerAnchor{};
  }
  void PlayerReturned(std::uint32_t ordinal, const PlayerAnchor &before,
                      const PlayerAnchor &after, std::uint64_t tick,
                      bool normal, bool body_reached) noexcept;
  bool PlayerPost(std::uint32_t slot,PlayerPostWitness &out) noexcept;
  bool NativeOrdinary(NativeCaptureWindow &,std::uint64_t completed_tick=0) noexcept;
  // 独立正常显示／音频观察窗口，不要求模拟tick；不据此授予任意Player字段资格。
  bool NativeProduction(NativeCaptureWindow &,std::uint64_t owner) noexcept;
  bool NativePostMatches(const core::TickPostState &) const noexcept;
  core::TickPostState NativePrepared(bool complete) noexcept;
  // 已复核的旧批次可给正式结构共享成员事实；缺任何槽或水位变化时回退独立读取。
  bool DiagnosticMembers(const NativeCaptureWindow &,std::span<RawSource>,std::span<std::uint32_t>) noexcept;
  bool NativeBody(std::uint32_t slot,NativeCaptureWindow &,PlayerPostWitness &) noexcept;
  bool NativeBodyInOrdinary(std::uint32_t slot,const NativeCaptureWindow &,NativeCaptureWindow &,PlayerPostWitness &) noexcept;
  bool NativeWindowCurrent(const NativeCaptureWindow &,bool recheck_production_roots=true) noexcept;
  bool NativeDisplayPlayer(const NativeCaptureWindow &,const Identity &) noexcept;
  PlayerLifeWitness BeginPlayerLife(std::uint32_t address,bool ghost) noexcept;
  void CompletePlayerLife(const PlayerLifeWitness &) noexcept;
  bool NativeCurrent(const NativeCaptureWindow &,std::uint32_t address,std::uint64_t local,
                     std::uint64_t use,std::uint64_t shape,ObjectDomain domain=ObjectDomain::Entity,
                     const NativeSource *reader=nullptr,bool grid_already_qualified=false,bool transition_roster_player=false) noexcept;
  // 20 项观察入口的 call/parent 值；错线程只在观察策略层置原子故障位。
  std::uint32_t ObserveEnter(Observation observation, std::uint32_t object,
                            const std::uint32_t *args, std::uint32_t count) noexcept;
  void ObserveLeave(std::uint32_t call, std::uint32_t result, bool normal) noexcept;
  // 内层只标记当前外层 call 的原生到达/返回；不会重复创建生命周期作用域。
  std::uint32_t NativeEnter(Observation observation, std::uint32_t object,
                           const std::uint32_t *args, std::uint32_t count) noexcept;
  void NativeLeave(std::uint32_t call, std::uint32_t result, bool normal) noexcept;
  // 仅49F3D0已核定的Type412重配owner调用；不按普通NPCUpdate推导私有字段资格。
  std::uint32_t BeginDeliriumReconfiguration(std::uint32_t object) noexcept;
  void EndDeliriumReconfiguration(std::uint32_t token,bool normal) noexcept;
  // JoinPass会绕过Game::Spawn直接创建烟雾；只接受本次实际Factory+Init+Publish闭包。
  std::uint32_t BeginJoinEffectSource() noexcept;
  void EndJoinEffectSource(std::uint32_t token,bool normal) noexcept;
  std::uint32_t BeginPlayerChildSource(std::uint32_t object,unsigned kind,unsigned slot) noexcept;
  void EndPlayerChildSource(std::uint32_t token,bool normal,bool body_reached=true) noexcept;
  // 必须在共享控制回调撤锚之前记录Replace前态；返回值仅用于不含地址的资格诊断。
  std::uint32_t BeforePlayerReplaceSource(std::uint32_t manager,std::uint32_t old_body,std::uint32_t replacement) noexcept;

private:
  void InstallPlayerAnchor(std::uint32_t ordinal,const PlayerAnchor &anchor) noexcept;
  bool NativeGridReady(const NativeCaptureWindow &,const Identity &,const NativeSource *,bool already_qualified) noexcept;
  struct Roots {
    std::uint32_t game = 0, room = 0, index = 0, dimension = 0;
    bool operator==(const Roots &) const = default;
  };
  struct Phase {
    std::uint64_t scope = 0;
    std::uint32_t entered = 0, returned = 0, object = 0;
    bool active = false;
  };
  // 固定回调栈；before 是 Grid/door 的真实旧槽，不把请求当释放结果。
  // Replace前已初始化的Lazarus旧活动体证言；只跟随同一次真实替换，不借新身体资格。
  struct PlayerReplaceState {std::uint32_t manager=0,old_body=0,new_body=0;unsigned old_type=0;std::uint64_t old_local=0,old_use=0,new_local=0,new_use=0,run=0;bool invalid=false;};
  struct Call {
    std::uint32_t id = 0, parent = 0, object = 0, before = 0;
    Observation observation = Observation::Factory;
    std::array<std::uint32_t, 8> args{};
    std::uint32_t count = 0;
    bool lifecycle = false;
    bool body_reached = false, body_returned = false;
    std::uint32_t body_result = 0;
    std::uint32_t body_after = 0, before_grid = 0, grid_index = kNoGridIndex;
    bool body_slot_known = false, before_grid_door = false;
    PlayerReplaceState player_replace{};
    // 只比较派生 Effect Init 入栈时的同一池对象，防止重入取得替换实例。
    std::uint64_t effect_local=0,effect_use=0;
  };
  void RememberPlayerReplaceState(Call &) noexcept;
  void CompletePlayerReplaceState(const Call &) noexcept;
  bool PlayerReplaceRoster(const Call &,bool completed) noexcept;
  bool Read(std::uint64_t address, void *value, std::size_t size) noexcept;
  template <class T> bool Value(std::uint64_t address, T &value) noexcept {
    return Read(address, &value, sizeof(value));
  }
  bool Image(std::uint32_t address, std::size_t bytes) const noexcept;
  bool ReadRoots(Roots &roots) noexcept;
  bool CurrentPlayerPost(std::uint32_t,PlayerPostWitness &) noexcept;
  Class Classify(ObjectDomain domain, std::uint32_t address,
                 std::uint32_t &vptr, bool &base_known) noexcept;
  void Incomplete(Reason reason) noexcept;
  Identity *Find(ObjectDomain domain, std::uint32_t address) noexcept;
  bool Event(EventKind kind, Reason reason, Identity *identity = nullptr,
             const Call *call = nullptr, bool entering = false,
             bool exceptional = false, std::uint32_t result = 0,
             std::uint64_t candidate_tick = 0) noexcept;
  std::uint32_t CreationOwner(ObjectDomain domain) const noexcept;
  void CompleteOwner(std::uint32_t call, Observation kind,
                     std::uint32_t result) noexcept;
  bool Birth(Identity &identity, Observation owner) noexcept;
  void CopySources() noexcept;
  // 先定义集合再开始见证；主复制重新枚举，不把单字段读前后冒充整个复制窗口。
  void BeginRngWindow() noexcept;
  void EndRngWindow(bool sources_stable) noexcept;
  void CopySource(std::uint32_t source) noexcept;
  std::int32_t Add(ObjectDomain domain, std::uint32_t address,
                   std::uint32_t source, std::uint32_t index) noexcept;
  void CopyObject(RawObject &object, Identity &identity) noexcept;
  NativeSource Source() noexcept;
  SubjectRegistry Registry() noexcept;
  void ResolveReferences() noexcept;
  void Memberships(bool commit) noexcept;
  void Gap(Reason reason, bool authority) noexcept;
  void CloseIfDrained() noexcept;

  WorldStorage &storage_;
  CaptureSink sink_{};
  bool post_stable_=false;
  IdentityTable &identities_;
  MemoryReader memory_;
  const std::uint32_t generation_;
  bool charged_ = false;
  bool active_ = false, stopping_ = false, outer_active_ = false,
       prepared_ = false, polluted_ = false, run_active_ = false,
       lifecycle_episode_ = false, run_ended_ = false, run_start_pending_ = false,
       run_start_owner_ = false;
  std::uint32_t lifecycle_depth_ = 0, depth_ = 0, next_call_ = 0;
  std::uint64_t loop_ = 0, outer_ = 0, run_ = 0, room_ = 0, mutation_ = 0,
                attempt_ = 0, capture_ = 0;
  Roots roots_{};
  Reason outer_reason_ = Reason::None;
  Authority authority_{};
  std::uint32_t authority_count_ = 0, game_frame_ = 0, grid_width_ = 0;
  std::array<Phase, 4> phases_{};
  // 已正常更新的 Player 实例可跨房继续承载显示。此证言不恢复输入 slot 或 PlayerPost。
  struct PlayerDisplayWitness {
    std::uint64_t run=0,local=0,use=0,shape=0;
    std::uint32_t address=0,seed=0;
  };
  std::array<PlayerDisplayWitness,kControlAnchorCapacity> transition_players_{};
  std::array<Call, 64> calls_{};
  // 只跨同一ObservationScope构造中的control_enter→enter两步；按深度与实参消耗，不能复用。
  std::array<PlayerReplaceState,64> player_replace_before_control_{};
  // 地址只作同线程在途关联；local/use/birth/run/outer共同限定同一已观察出生。
  // entry不改身份；实际BaseInit才推进shape，正常helper返回才恢复派生读取。
  struct EntityReconfiguration final {
    std::uint32_t token=0,object=0,call_depth=0;
    std::uint64_t local=0,use=0,birth=0,run=0,outer=0;
    bool native_init=false;
  };
  std::array<EntityReconfiguration,8> entity_reconfigurations_{};
  std::uint32_t entity_reconfiguration_depth_=0,next_entity_reconfiguration_=0;
  struct JoinEffectWitness {std::uint32_t address=0;std::uint64_t local=0,use=0;bool initialized=false;};
  std::array<JoinEffectWitness,64> join_effect_witnesses_{};
  std::uint32_t join_effect_token_=0,next_join_effect_token_=0,join_effect_depth_=0,join_effect_count_=0;
  std::uint64_t join_effect_run_=0,join_effect_outer_=0;
  // 具名角色owner内真实Ctor/完整Init与存储迁移证言；嵌套正常收口并复核当前关系才授予4/5。
  struct PlayerChildOwner {std::uint32_t token=0,object=0,call_depth=0,first=0,replacement=0;unsigned kind=0,slot=0,source_type=0;std::uint64_t local=0,use=0,run=0,replacement_local=0,replacement_use=0;};
  // initialized要求同use真实完整Init或既有存储依据；esau_active另等实际替换和槽后态。
  struct PlayerChildWitness {std::uint32_t object=0,parent=0,offset=0,peer=0;std::uint64_t local=0,use=0,parent_local=0,parent_use=0;bool complete=false,state_owner=false,pair_owner=false,invalid=false,ctor_seen=false,initialized=false,esau_active=false;};
  std::array<PlayerChildOwner,16> player_child_owners_{};
  std::array<PlayerChildWitness,256> player_child_witnesses_{};
  unsigned player_child_depth_=0,player_child_count_=0;
  std::uint32_t next_player_child_=0;
  bool player_child_failed_=false;
  void RememberPlayerChildCtor(const Identity &) noexcept;
  bool PlayerChildRelation(const PlayerChildOwner &,PlayerChildWitness &) noexcept;
  bool PlayerChildStillCurrent(const PlayerChildWitness &) noexcept;
  bool PlayerEsauSlotCompleted(const PlayerChildOwner &,PlayerChildWitness &) noexcept;
  void RememberPlayerEsauReplacement(std::uint32_t manager,std::uint32_t old_body,std::uint32_t replacement) noexcept;
  bool PlayerEsauRestoreNoop(std::uint32_t object) noexcept;
  void CompletePlayerEsauReplacement(const PlayerChildOwner &) noexcept;
  bool PlayerLazarusRetired(const PlayerChildOwner &,PlayerChildWitness &) noexcept;
  EntityReconfiguration *EntityReconfigurationFor(std::uint32_t object) noexcept;
  PlayerAnchorSet anchors_{};
  // 当前exact pair是否已观察正常PlayerUpdate；独立于创建owner赋予的字段读取依据。
  std::array<PlayerAnchor, kControlAnchorCapacity> update_witnesses_{};
  std::array<PlayerPostWitness,kControlAnchorCapacity> player_posts_{};
  // 来源顺序两遍读取，包含每个指针值；大工作表属于堆上 Collector，计入管理余量。
  std::array<std::uint32_t, kMembershipBudget> source_addresses_{};
  std::array<std::uint32_t, 460> fixed_addresses_{}; // Esau 4 + Grid 448 + door 8。
  std::array<std::uint32_t, kRelationNodes> relation_nodes_{};
  struct RngInventory {
    std::uint64_t local = 0, use = 0, revision = 0;
    std::uint32_t domain = 0, address = 0, vptr = 0, kind = 0;
    RawRng value{};
  };
  std::array<RngInventory, kObjectBudget> rng_inventory_{};
  std::uint32_t rng_count_ = 0;
  BatchWriter pending_{};
  // 诊断租约与 Authority 后态凭据独立；每个 outer 只发送一次完成通知。
  bool legacy_diagnostics_=false,legacy_output_failed_=false,post_notified_=false;
  std::uint64_t legacy_attempts_=0;
  core::TickPostState post_{};
};
} // namespace cs::world
