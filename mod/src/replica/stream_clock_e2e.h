#pragma once
#include <cstdint>
#ifdef ISAACCS_E2E
namespace cs::replica::stream {
// 仅在实际时钟拒绝时记录报文原值和本机接收时刻；观测失败不改变接纳或拒绝结果。
void E2EClockRejected(std::uint64_t echo,std::uint64_t now,std::uint64_t received,
                      std::uint64_t sent,std::uint64_t sample) noexcept;
}
#endif
