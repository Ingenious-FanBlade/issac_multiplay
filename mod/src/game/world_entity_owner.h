#pragma once
#include "world/observe.h"
#include <memory>

namespace cs::game {
struct WorldCapture;
// J273 Delirium专属AI的只读owner。只在真实Init进入后才形成重配，普通AI不制造Morph。
// 强持有原采集context至原调用收口；停用、异常或换use不恢复当前字段资格。
class WorldEntityReconfigurationScope final {
public:
  WorldEntityReconfigurationScope(void *object,world::HookCounters counters) noexcept;
  ~WorldEntityReconfigurationScope() noexcept;
  void Returned() noexcept;
private:
  std::shared_ptr<WorldCapture> capture_;
  world::HookCounters counters_{};
  std::uint32_t token_=0;
  int exceptions_=0;
  bool returned_=false;
};
}
