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
void ForeignCall() noexcept;
} // namespace cs::game
