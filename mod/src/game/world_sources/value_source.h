#pragma once
// 跨 SDK 边界只传故障计数及逻辑启停，不携带游戏对象。
#include <cstdint>
namespace cs::game {
struct ValueSourceFailures final { std::uint64_t wrong_thread=0,exception=0,budget=0; };
void ArmObservationValueSources(bool armed) noexcept;
ValueSourceFailures ObservationValueSourceFailures() noexcept;
class ActualValueScope final {
public:
  ActualValueScope() noexcept;
  ~ActualValueScope();
  bool main=false;
private:
  bool armed_=false;
  int exceptions_=0;
};
}
