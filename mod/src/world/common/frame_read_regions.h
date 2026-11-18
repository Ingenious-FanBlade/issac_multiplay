#pragma once
#include "world/common/checked_read.h"
#include <span>
#include <string>

namespace cs::world {
enum class FrameRegionReader : unsigned { ReadMemory, Consumer, Topology, Roots, Classify, Capture, Count };
enum class FrameRegionReset : unsigned { Frame, Fault, Guard, Overlap, Revoke, Count };
// 临时读者可累加自己的查询成本。整帧表另按读者累计进程诊断。
struct NativeReadCounts {
  std::uint64_t lookups=0,queries=0,hits=0,fallbacks=0;
};
void InitializeFrameReadRegions(void (*first_guard)() noexcept) noexcept;
// 空闲重载期间归还区表租约，读取计数及守护页停用状态留在进程内。
void SetFrameReadRegionCache(bool enabled) noexcept;
void InvalidateFrameReadRegions(FrameRegionReset cause) noexcept;
bool CheckedNativeCopy(std::uint64_t address,void *output,std::size_t size,FrameRegionReader,
                       NativeReadCounts *counts=nullptr,bool *copy_fault=nullptr) noexcept;
bool CheckedNativeStringBlock(std::uint64_t address,std::span<std::byte> output,std::size_t &size,
                              FrameRegionReader,NativeReadCounts *counts=nullptr) noexcept;
std::string FrameReadRegionFields();
}
