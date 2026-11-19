#pragma once
#include "game/client/room/client_backdrop_native.h"
#include "game/observation/binary_production.h"
#include <array>
#include <cstdint>
#include <string>
#include <memory>

namespace cs::game {
struct WorldCapture;
void BinaryBackdropChanged(void *,bool current) noexcept;
// SDK公开ANM2资源写入的只读观察：original恰好一次，进入即撤旧资格，异常保持撤销。
class StreamResourceScope final {
public:
  explicit StreamResourceScope(void *object,bool retired=false,bool constructing=false,void *copy_source=nullptr,const char *operation="unspecified") noexcept;
  ~StreamResourceScope() noexcept;
  void Returned() noexcept {returned_=true;}
private:
  bool returned_=false;
  bool qualified_writer_=false;
  bool foreign_writer_=false;
  int exceptions_=0;
#ifdef ISAACCS_E2E
  std::shared_ptr<WorldCapture> capture_;
  bool retired_=false;
  bool constructing_=false;
  bool affects_source_=false;
  std::uint64_t copied_generation_=0;
  std::uint32_t address_=0;
#endif
};
#ifdef ISAACCS_E2E
std::uint64_t StreamResourceGeneration(void *object,bool copied_value=false) noexcept;
#endif
bool StreamResourceHooksInstalled() noexcept;
// 通知 Host 原生共享缓存的释放边界；外线程全程计入外部写水位。
class StreamSharedResourceScope final {
public:
  explicit StreamSharedResourceScope(bool animation) noexcept;
  ~StreamSharedResourceScope() noexcept;
private:
  bool foreign_=false;
};
unsigned StreamSharedResourceHooks() noexcept;
bool BinaryDisplayCaptureHooksInstalled() noexcept;
std::string ClientTestExerciseNative(const std::string &token) noexcept;
// E2E专有独立VM，只用于实际不同VM身份的受控拒绝，不替换游戏Lua根。
void *ClientTestCreateVm() noexcept;
void ClientTestDestroyVm(void *) noexcept;
// SDK 公开只读 Getter 的 C++17 桥；只在主线程读取已确认的 Player。
bool PlayerIsHologram(void *player) noexcept;
// SDK-facing 的 world 作用域只传值，不包含 C++20 核心或 SDK 对象方法。
enum class WorldBoundary : unsigned { Owner, Room, Tail, Lifecycle, RunEnd, RunStart };
  // Level::Init整条正常链的只读生命周期证言；只撤销来源，不改变原函数调用。
  class BinaryFloorScope final {
  public:
    BinaryFloorScope() noexcept;
    ~BinaryFloorScope() noexcept;
  private:
    int exceptions_=0;
    bool active_=false;
  };
class WorldBoundaryScope final {
public:
  WorldBoundaryScope(WorldBoundary kind, const void *object = nullptr,
                     bool enabled = true) noexcept;
  ~WorldBoundaryScope() noexcept;
private:
  WorldBoundary kind_;
  std::uint64_t scope_ = 0;
  bool active_ = false;
  int exceptions_ = 0;
};
// 此头只用 C++17 可表达的值类型，供 SDK-facing 的生成 wrapper 调用。
enum class ConsumerKind : unsigned {
  None,
  Movement,
  Bomb,
  Item,
  Pill,
  Shooting,
  Drop
};
enum class QueryKind : unsigned { Pressed = 1, Triggered, Scalar };
enum class SimulationKind : unsigned { Room, Odd };
struct PhysicalActions {
  std::array<float, 12> values{};
  std::array<bool, 12> pressed{}, triggered{};
};
// 真实Client在完整Manager原链之前泵消息并独立更新输入后端，不启动本地模拟。
bool ClientManagerBoundary() noexcept;
bool ClientBusy() noexcept;
void ClientPollInput();
// 原生插值选项：0=关，1=开，2=非主线程或Manager未就绪；只读诊断。
unsigned NativeInterpolationState() noexcept;
bool ClientUiBlocked() noexcept;
bool ClientLocalOrdinal(unsigned &ordinal) noexcept;
// 仅在当前相机原生更新栈内投影本端控制组，并在返回时恢复全局名册及中心缓存。
class LocalCameraScope final {
public:
  explicit LocalCameraScope(void *camera) noexcept;
  ~LocalCameraScope() noexcept;
  LocalCameraScope(const LocalCameraScope &)=delete;
  LocalCameraScope &operator=(const LocalCameraScope &)=delete;
private:
  void *roster_=nullptr;
  std::array<void *,256> players_{};
  std::array<float,2> center_{};
  std::uint32_t frame_=0;
  unsigned count_=0;
};
bool HostOwnsCameraPlayer(const void *room,const void *player) noexcept;
bool ClientSnapLocalCamera(void *camera);
// 只在已安装客机世界的原生 Minimap::Update 调用期间开放地图输入。
class ClientMinimapInputScope final {
public:
  ClientMinimapInputScope() noexcept;
  ~ClientMinimapInputScope() noexcept;
  ClientMinimapInputScope(const ClientMinimapInputScope &)=delete;
  ClientMinimapInputScope &operator=(const ClientMinimapInputScope &)=delete;
};
bool ClientMinimapMapPressed(int action,bool &pressed) noexcept;
// StatHUD 有两个原生显示槽，按本端 C/S 控制组登记；渲染时临时隐藏过期槽。
class FoundHudScope final {
public:
  FoundHudScope(void *hud,bool rendering) noexcept;
  ~FoundHudScope() noexcept;
  FoundHudScope(const FoundHudScope &)=delete;
  FoundHudScope &operator=(const FoundHudScope &)=delete;
  void Returned() noexcept;
private:
  void *hud_=nullptr;
  std::array<void *,2> previous_{};
  bool rendering_=false;
};
bool FoundHudSlotAllowed(const void *hud,const void *player) noexcept;
bool HostOwnsFoundHudPlayer(const void *player) noexcept;
void ClearClientFoundHudSlots(void *hud) noexcept;
// 仅当前原生HUD draw作用域供应服务器最终显示条件，不放行成就或任何持久写入。
bool ClientMenuAvailable() noexcept;
void ClientInputUnavailable() noexcept;
void ClientMenuRendered() noexcept;
// 只替代 Manager Render 内的菜单绘制；保留外层真实清屏、提交与交换缓冲。
bool ClientRenderTransitionHold() noexcept;
bool ClientProcessExitDecision() noexcept;
bool ClientShutdownProgressBlocked() noexcept;
// classes_registered 仅 RegisterClasses 外层正常返回可设，证明 vanilla main.lua 前已保护。
void ClientGuardLuaBindings(bool classes_registered=false) noexcept;
bool ClientLuaGuardReady() noexcept;
void ClientLuaGuardClosed() noexcept;
void ClientGuardCallbackBindings(const char *path) noexcept;
bool ClientCallbackGuardReady() noexcept;
// main_ex返回、官方清除_LuaBindings之前保留同VM的真实EntityManager清理闭包。
bool ClientObjectStagedPoolNotification(void *,bool execute,bool creating,const char *&reason) noexcept;
bool ClientObjectSealEssmDeleteBinding(void *state) noexcept;
void ClientCallbackFault(const char *reason) noexcept;
bool ClientCallbackMenuAllowed() noexcept;
void ClientCallbackSkipped() noexcept;
void ClientCallbackOwnExecuted() noexcept;
// main_ex.lua 重载开始即撤销旧保护资格；正常返回后重装，异常/失败阻止活跃 Client 继续。
class ClientCallbackScriptScope final {
public:
  ClientCallbackScriptScope(bool main,const char *path) noexcept;
  ~ClientCallbackScriptScope() noexcept;
  ClientCallbackScriptScope(const ClientCallbackScriptScope &)=delete;
  ClientCallbackScriptScope &operator=(const ClientCallbackScriptScope &)=delete;
  bool AllowOriginal() const noexcept { return allowed_; }
  void Returned(bool success) noexcept;
private:
  bool tracked_=false,returned_=false,allowed_=true;
};
// I3 入口分派只标记实际用途和原调用处置；不会把观察 Hook 的 skip 计作原调用返回。
enum class ClientOperation : unsigned { Build, Apply, PresentMaintenance, PresentDraw, Retire, BinaryPreparation, BinaryPreparationRetire, BinaryInstall, BinaryInstallRetire };
enum class ClientEntry : unsigned {
  GameUpdate, PostUpdate, RoomUpdate, EntityListUpdate, PlayerUpdate, OddHelper,
  LifecycleFSM, DoorUpdate, DoorNested, NewRoom, RoomInit, GameRender,
  RoomPreRender, RoomRender, EntityListRenderSort, GameStart,
  NativeBuild, NativeApply, NativeTeardown, PresentationMaintenance, PresentationDraw, MenuRender,
  BackdropEffects,
  DisplayHudUpdate, DisplayHudPostUpdate, DisplayMinimapUpdate, DisplayMinimapRefresh,
  DisplaySort, DisplayFxUpdate, DisplayBackground, DisplayTransition,
  DisplayCamera, DisplaySfxRefresh, DisplayMusic, DisplaySfxReplay, DisplayCursor, TransitionChangeRoom, NativePreparation, NativeAtomicInstall,
  Count
};
enum class ClientEntryDecision : unsigned { CallOriginal, ExpectedSkip, FaultSkip };
// 仅二进制准备 owner 当前主线程栈授予；不借用旧 JSON target 作为资源准备资格。
bool ClientBinaryPreparationScopeActive(bool retirement) noexcept;
bool ClientBinaryInstallationScopeActive(bool retirement) noexcept;
bool ClientBinaryGridWaterClip(const void *,std::uint32_t &) noexcept;
// 完整原生 Entity Init 依赖随机与音频隔离 Hook。
bool ClientEntityInitIsolationInstalled() noexcept;
bool ClientBinaryDrawOrder(const void *,std::uint32_t &) noexcept;
// 返回当前安装／已发布 roster 的 C/S 控制组槽位，不借用原版设备编号。
bool ClientBinaryPlayerOrdinal(const void *,unsigned &,bool active_only=false) noexcept;
constexpr std::uint64_t ClientEntryMask(ClientEntry entry) noexcept {
  return unsigned(entry)<64?std::uint64_t{1}<<unsigned(entry):0;
}
// 适配器须声明唯一根入口与明确预期跳过的嵌套更新；作用域持有 I6 票据直到原调用退出。
class ClientDispatchScope final {
public:
  ClientDispatchScope(ClientOperation operation, ClientEntry root,
                      std::uint64_t expected_skips = 0) noexcept;
  ~ClientDispatchScope() noexcept;
  ClientDispatchScope(const ClientDispatchScope &)=delete;
  ClientDispatchScope &operator=(const ClientDispatchScope &)=delete;
  bool Ready() const noexcept { return active_; }
  bool Succeeded() const noexcept;
private:
  std::uint64_t frame_=0;
  bool active_=false;
  int exceptions_=0;
};
ClientEntryDecision ClientDispatchBefore(ClientEntry entry) noexcept;
void ClientDispatchReturned(ClientEntry entry) noexcept;
// 实际安装的常驻外层入口和 Lua 回调保护齐备后才允许原生适配器使用。
bool ClientDispatchIsolationInstalled() noexcept;
// C++17 Hook 桥仅传纯值通知；场景/模型类型不暴露给 SDK-facing wrapper。
// 只观测 Manager::Render 在途窗口；不授予完整 native Render 或 Graphics 使用资格。
class ClientRenderScope final {
public:
  explicit ClientRenderScope(bool enabled) noexcept;
  ~ClientRenderScope() noexcept;
  ClientRenderScope(const ClientRenderScope &)=delete;
  ClientRenderScope &operator=(const ClientRenderScope &)=delete;
private:
  bool active_=false;
  int exceptions_=0;
};
void ClientLuaClosing() noexcept;
void ClientShutdownBeginning() noexcept;
void ClientContextClosing() noexcept;
bool ClientPersistenceBlocked() noexcept;
// 主动拒绝与 original exactly-once 观察分开计数；只接受固定入口名，不保存 Lua 值。
void ObserveClientPersistenceDenied(const char *entry,bool lua=false) noexcept;
// SDK-facing 只在主线程返回当前原生 owner 身份；不跨线程传递这些值。
bool ClientRendererHandles(void *&dc,void *&context) noexcept;
void *ClientLuaStateIdentity() noexcept;
// 主线程按“物理后端锁 -> 原生 vector 锁”顺序访问设备；本线程嵌套只增加深度。
// 使用同一原生 Mutex，不在公共更新已持 vector 锁后首次反向取得后端锁。
class InputLease final {
public:
  explicit InputLease(bool required) noexcept;
  ~InputLease() noexcept;
  bool Ready() const noexcept { return ready_; }
  bool Acquired() const noexcept { return acquired_; }
  InputLease(const InputLease &) = delete;
  InputLease &operator=(const InputLease &) = delete;

private:
  bool joined_ = false, ready_ = false, acquired_ = false;
};
bool HostBusy() noexcept;
void InputLeaseUnavailable() noexcept;
// Join 的整个原生 pass 必须在同一固定动作快照中完成，异常只观察并收尾。
class JoinScope final {
public:
  explicit JoinScope(void *manager) noexcept;
  ~JoinScope() noexcept;

private:
  InputLease lease_;
  unsigned ordinal_ = 0;
  bool active_ = false;
  int exceptions_ = 0;
};
// 常驻汇编补丁只传递原生地址、整数和 bool；决策实现在 C++20 的 join.cpp。
// SDK-facing 单元经此接口调用，不包含带 Binding/Topology 状态的 join.h。
bool JoinMode(bool original, void *manager, int key, void *row,
              unsigned site) noexcept;
bool JoinCreate(void *manager, void *row, int *role) noexcept;
// 观测作用域不拥有原函数；异常由调用方自然向上展开。
class SimulationScope final {
public:
  SimulationScope(SimulationKind kind, void *object = nullptr) noexcept;
  ~SimulationScope() noexcept;

private:
  SimulationKind kind_;
  uint64_t token_ = 0;
  bool started_ = false, main_ = false;
  int exceptions_ = 0;
};
// Player 内外层配对，分别记录到达入口与到达原生子类 body。
class PlayerScope final {
public:
  explicit PlayerScope(void *player) noexcept;
  ~PlayerScope() noexcept;
  void *player = nullptr;
  PlayerScope *previous = nullptr;
  uint64_t tick = 0, generation = 0;
  int ordinal = -1;
  unsigned consumer_mask = 0; // 仅用于逐身体原生活动见证，不消费或复制输入。
  // world 读取依据的前快照；SDK-facing 头只保存 C++17 值，不包含 collector。
  void *world_device = nullptr;
  std::int32_t world_key = -1;
  std::uint64_t world_session = 0;
  bool world_valid = false;
  bool native_reached = false, controlled = false, main = false;
  int exceptions = 0;
};
void PlayerNativeReached(void *player) noexcept;
// 仅 Shooting 可主动抑制；其它 consumer 都保持原调用一次。
class ConsumerScope final {
public:
  ConsumerScope(void *player, ConsumerKind kind) noexcept;
  ~ConsumerScope() noexcept;
  bool CallOriginal() const noexcept { return call_original_; }
  void *player = nullptr;
  ConsumerKind kind = ConsumerKind::None;
  ConsumerScope *previous = nullptr;
#ifdef ISAACCS_E2E
  // 同一次真实consumer中多次Triggered读取不冒充多次动作。
  unsigned e2e_edge_queries=0;
#endif

private:
  InputLease lease_;
  bool main_ = false, call_original_ = true;
  int exceptions_ = 0;
};
class DeviceScope final {
public:
  explicit DeviceScope(void *device) noexcept;
  ~DeviceScope() noexcept;
  void
  Returned(bool success); // 保存原 query 可能抛异常，不在 noexcept 析构中调用。
  void *device = nullptr;
  DeviceScope *previous = nullptr;
  uint64_t loop = 0, token = 0;
  unsigned polls = 0;
  bool poll_ok = false, main = false;
  int exceptions = 0;
};
void PollReturned(void *device, bool success) noexcept;
void InputFrameFinished() noexcept;
bool ReadPhysicalActions(void *device, PhysicalActions &actions,bool *map_pressed=nullptr);
bool OverrideQuery(void *device, int action, QueryKind kind,
                   float &result) noexcept;
void ObserveConsole(bool blocked) noexcept;
void NewRoomReturned() noexcept;
// native_exit 只在真正的 Game::Exit 观察点为 true，用于打开 run_exit 持久窗口；
// Restore 的退休分支复用同一停止请求但不伪造退出窗口。
void RunTeardownBeginning(bool native_exit = false) noexcept;
void ForeignCall() noexcept;
// 仅原生 RoomTransition::ChangeRoom 的 Hourglass 分支授予本局回溯资格。
class HourglassRoomScope final {
public:
  explicit HourglassRoomScope(const void *transition) noexcept;
  ~HourglassRoomScope() noexcept;
  HourglassRoomScope(const HourglassRoomScope &)=delete;
  HourglassRoomScope &operator=(const HourglassRoomScope &)=delete;
private:
  bool previous_=false;
};
bool HourglassRoomActive() noexcept;
bool HourglassRestoreActive() noexcept;
// tx-04-07：一次 Restore 决策处理两个停止点。构造时决定保留首次菜单 Host 还是
// 退休已有 run，并完成停止/排队清理；析构观察 original 的正常返回与异常。
// 整个 original 处于 Restore owner。
class RestoreScope final {
public:
  RestoreScope(void *game, void *state, bool start_game) noexcept;
  ~RestoreScope() noexcept;
  bool Preserved() const noexcept { return preserve_; }
  bool Active() const noexcept { return active_; }

private:
  void *game_ = nullptr;
  void *state_ = nullptr;
  bool start_game_ = false, preserve_ = false, active_ = false,
       main_ = false;
  int exceptions_ = 0;
};
// Start/Lifecycle owner 的观测作用域：构造时记录进入（Start 上固定首次菜单
// 加载资格），析构记录正常/异常返回并把稳定入口延后到后续 loop。
// ExecuteStartGame 每帧可空调用；只有 Manager.starting 非零才构成启动操作。
bool StartGameRequested(void *manager) noexcept;
class LoadingOwnerScope final {
public:
  explicit LoadingOwnerScope(bool start_owner, bool enabled = true) noexcept;
  ~LoadingOwnerScope() noexcept;

private:
  unsigned bit_ = 0;
  bool main_ = false;
  int exceptions_ = 0;
};
// 跨 loop 持久操作窗口；固定容量、允许嵌套，只记录值，不写世界。
enum class PersistenceWindowKind : unsigned {
  None,
  BDeath,
  BRevive,
  StageChange,
  Continue,
  RunExit,
  Count
};
const char *PersistenceWindowKindName(PersistenceWindowKind kind) noexcept;
struct PersistenceWindow {
  uint64_t id = 0, parent = 0, caller = 0;
  PersistenceWindowKind kind = PersistenceWindowKind::None;
  unsigned ordinal = 0; // 0为整局窗口，1..3为独立远端生命窗口。
  uint64_t start_loop = 0, end_loop = 0, session_generation = 0;
  uint64_t loading_episode = 0, death_event = 0;
  const char *source = "", *end_reason = "open";
  bool active = false, normal = false;
  bool call_returned = false, call_normal = false;
};
uint64_t OpenPersistenceWindow(PersistenceWindowKind kind, const char *source,
                               uint64_t death_event = 0, unsigned ordinal = 0) noexcept;
bool ClosePersistenceWindow(PersistenceWindowKind kind, const char *reason,
                            bool normal, unsigned ordinal = 0) noexcept;
bool PersistenceWindowActive(PersistenceWindowKind kind, unsigned ordinal = 0) noexcept;
// 该窗口的同步原生调用是否已正常返回；只读诊断，不改窗口状态。
bool PersistenceWindowCallNormal(PersistenceWindowKind kind, unsigned ordinal = 0) noexcept;
// 该窗口的同步原生调用是否已经返回（正常或异常）。ST2 要求复活动作正常返回
// 前不得仅凭 ghost=false 推进代次。
bool PersistenceWindowCallReturned(PersistenceWindowKind kind, unsigned ordinal = 0) noexcept;
void CloseAllPersistenceWindows(const char *reason, bool normal) noexcept;
// 同步持久入口：记录 call_id/parent_call_id、同步 operation 与有效 window ids。
class PersistenceCallScope final {
public:
  PersistenceCallScope(const char *entry, void *object, bool enabled = true) noexcept;
  ~PersistenceCallScope() noexcept;
  void Returned(bool value) noexcept; // 有 bool 返回值的入口
  void Returned() noexcept;           // void 入口

private:
  const char *entry_ = "";
  void *object_ = nullptr;
  uint64_t call_ = 0, parent_ = 0;
  bool main_ = false, recorded_ = false;
  int exceptions_ = 0;
};
// 持久写入只读观测：记录原生入口、条件和前后值；不修改 readonly/dirty。
class PersistenceOperation final {
public:
  explicit PersistenceOperation(const char *name) noexcept;
  ~PersistenceOperation() noexcept;

private:
  const char *previous_;
};
class PersistenceScope final {
public:
  PersistenceScope(const char *entry, void *data, int index = -1,
                   int amount = 0) noexcept;
  ~PersistenceScope() noexcept;
  // TryUnlock 等 bool 入口的正常返回值；不虚构异常路径的返回值。
  void Returned(bool value) noexcept;

private:
  const char *entry_, *operation_;
  void *data_;
  int index_, amount_, before_ = 0, exceptions_;
  unsigned char dirty_ = 0, readonly_ = 0, loaded_ = 0;
  uint64_t call_ = 0, parent_ = 0;
  bool known_ = false, recorded_ = false;
};
// tx-04-07 统一清单新增入口的只读观察；original 仍 exactly-once。
void ObserveEntityKill(void *entity, void *source) noexcept;
void ObserveReviveGhost(void *player) noexcept;
void ObserveStageTransition(void *game, bool same_stage, int param2,
                            void *player) noexcept;
void ObserveLoadGameState(void *manager, int saveslot) noexcept;
std::string PersistenceFields();
std::string PersistenceWindowFields();
} // namespace cs::game
