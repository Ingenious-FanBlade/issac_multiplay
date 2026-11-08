#pragma once
#include "world/observe.h"
#include <memory>

namespace cs::game {
struct WorldCapture;
// 已核实JoinPass直接创建Effect的完整owner；只持有观察上下文，不改变Join或游戏对象。
class WorldJoinEffectSourceScope final {
public:
  WorldJoinEffectSourceScope() noexcept;
  ~WorldJoinEffectSourceScope() noexcept;
  void Returned() noexcept {returned_=true;}
private:
  std::shared_ptr<WorldCapture> capture_;
  std::uint32_t token_=0;
  int exceptions_=0;
  bool returned_=false;
};
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
// 包具名创建／换槽owner及实际Player Update中的hologram转backup，original恰一次。
// 强持有原上下文，异常不发布子体初始化证言，Host/Off游戏行为保持原链。
class WorldPlayerChildSourceScope final {
public:
  WorldPlayerChildSourceScope(void *,unsigned,world::HookCounters,unsigned slot=0) noexcept;
  ~WorldPlayerChildSourceScope() noexcept;
  void Returned(bool body_reached=true) noexcept;
private:
  std::shared_ptr<WorldCapture> capture_;
  world::HookCounters counters_{};
  std::uint32_t token_=0;
  int exceptions_=0;
  bool returned_=false;
  bool body_reached_=true;
};
}
