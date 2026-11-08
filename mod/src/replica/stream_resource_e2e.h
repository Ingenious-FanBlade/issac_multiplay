#pragma once
#include <cstdint>
#include <array>
#ifdef ISAACCS_E2E
namespace cs::replica::stream {
// 仅E2E记录实际来源资源失效的纯值；不读取或持有原生资源对象。
void E2EResourceChange(std::uint32_t address,std::uint64_t generation,bool reading,bool retired,bool constructing,bool copying,const char *path,bool path_available,bool loaded,const char *operation) noexcept;
// 仅复制Collector已经形成的创建/Init调用证言，不由诊断器补出生或读取游戏对象。
void E2ESourceLifecycle(const std::array<std::uint64_t,12> &,const std::array<std::uint32_t,8> &,unsigned count) noexcept;
}
namespace cs::game {
// 仅主线程已知owner入口调用；SDK字段读取留在C++17适配单元，环只接收复制值。
void E2EResourceChangeFromNative(void *object,std::uint32_t address,std::uint64_t generation,bool reading,bool retired,bool constructing,bool copying,const char *operation) noexcept;
}
#endif
