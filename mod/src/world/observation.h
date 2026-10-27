#pragma once
// tx-04-08 world_record 迁移：19 项观察语义的公共编号。
//
// 本头只包含 C++17 可表达的值类型，供 SDK-facing 的生成 Hook wrapper
// （按项目约定以 C++17 编译）与 C++20 的 world 实现共用。它不依赖 SDK、
// 游戏头、windows.h 或任何运行时状态。
//
// 编号属于文件域约定：lifecycle-events.jsonl 的 `observation` 列与
// manifest.json 的 `observations` 图例都使用这些数值，只追加不改写。
#include <cstdint>

namespace cs::world {

// 原型 19 项观察语义；它们是观察范围，不是 19 个独立安装器。
inline constexpr std::uint32_t kObservationCount = 19;
// 生命周期/成员关系边界事件不属于任何一个原生观察入口。
inline constexpr std::uint32_t kNoObservation = 0xFFFFFFFFu;

enum class Observation : std::uint32_t {
  Factory = 0,
  Init = 1,
  Publish = 2,
  Remove = 3,
  ClearReferences = 4,
  Destroy = 5,
  PlayerCtor = 6,
  PlayerCreate = 7,
  ManagerRestore = 8,
  Replace = 9,
  PlayerRestore = 10,
  NpcMorph = 11,
  PickupMorph = 12,
  GridFactory = 13,
  GridInit = 14,
  GridSpawn = 15,
  Door = 16,
  GridFree = 17,
  Spawn = 18
};

// 名称同时用于 lifecycle-events 的 hook 列与 manifest 图例；越界返回
// "unknown_observation"，不猜测最近值。
const char *ObservationName(Observation value) noexcept;

} // namespace cs::world
