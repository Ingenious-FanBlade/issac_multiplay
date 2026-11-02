#pragma once
// tx-04-08 world_record 原有19项语义；tx-04-18追加Grid物理构造，旧编号不变。
//
// 本头只包含 C++17 可表达的值类型，供 SDK-facing 的生成 Hook wrapper
// （按项目约定以 C++17 编译）与 C++20 的 world 实现共用。它不依赖 SDK、
// 游戏头、windows.h 或任何运行时状态。
//
// 编号属于文件域约定：lifecycle-events.jsonl 的 `observation` 列与
// manifest.json 的 `observations` 图例都使用这些数值，只追加不改写。
#include <cstdint>

namespace cs::world {

// 观察范围与统一Hook清单对应，不各自建立安装器。
inline constexpr std::uint32_t kObservationCount = 20;
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
  Spawn = 18,
  GridCtor = 19
};

// 名称同时用于 lifecycle-events 的 hook 列与 manifest 图例；越界返回
// "unknown_observation"，不猜测最近值。
const char *ObservationName(Observation value) noexcept;

} // namespace cs::world
