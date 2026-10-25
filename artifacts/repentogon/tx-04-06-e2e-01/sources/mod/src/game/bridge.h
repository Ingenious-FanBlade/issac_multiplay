#pragma once
#include <array>
#include <cstdint>

namespace cs::game {
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
// 主线程按“物理后端锁 -> 原生 vector 锁”顺序访问设备；本线程嵌套只增加深度。
// 使用同一原生 Mutex，不在公共更新已持 vector 锁后首次反向取得后端锁。
class InputLease final {
public:
  explicit InputLease(bool required) noexcept;
  ~InputLease() noexcept;
  bool Ready() const noexcept { return ready_; }
  InputLease(const InputLease &) = delete;
  InputLease &operator=(const InputLease &) = delete;

private:
  bool joined_ = false, ready_ = false;
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
  bool active_ = false;
  int exceptions_ = 0;
};
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
bool ReadPhysicalActions(void *device, PhysicalActions &actions);
bool OverrideQuery(void *device, int action, QueryKind kind,
                   float &result) noexcept;
void ObserveConsole(bool blocked) noexcept;
void NewRoomReturned() noexcept;
void RestoreBeginning() noexcept;
void RunTeardownBeginning() noexcept;
void ForeignCall() noexcept;
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
  PersistenceScope(const char *entry, void *data, int index = -1, int amount = 0) noexcept;
  ~PersistenceScope() noexcept;
private:
  const char *entry_, *operation_;
  void *data_;
  int index_, amount_, before_ = 0, exceptions_;
  unsigned char dirty_ = 0, readonly_ = 0, loaded_ = 0;
  bool known_ = false;
};
} // namespace cs::game
