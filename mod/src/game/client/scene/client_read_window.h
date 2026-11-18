#pragma once
#include "world/common/checked_read.h"
#include <array>

namespace cs::game {
// 仅主线程调用栈累计实际页查询及定长表满后的回退；不记录原生地址或逐字段日志。
// 原子安装阶段按差值归入现有两流计数，纯读窗口的寿命不受统计影响。
struct ClientNativePageCounters {
  std::uint64_t read_queries=0,read_full=0,write_queries=0,write_full=0;
};
inline thread_local ClientNativePageCounters client_native_page_counters;
// 只用于一次显式纯读调用栈的页见证；不得跨原生写入、回调或Manager边界。
// 地址和动态类型仍每次读取，缓存只省略同一只读窗口内重复的VirtualQuery。
class ClientNativeReadWindow final {
  // 实体／Plus／图层可分布在多个堆区；固定有序表避免小环反复淘汰同一纯读段的页见证。
  std::array<world::NativeReadRegion,64> regions_{};
  world::NativeReadRegion recent_{};
  unsigned used_=0;
public:
  bool Copy(const void *,void *,std::size_t) noexcept;
  // 查找只遍历有效前缀；撤销计数和最近页即可使全部旧见证不可达，无须逐页擦写。
  void Clear()noexcept {recent_={};used_=0;}
};
} // namespace cs::game
