#pragma once
#include "world/binary/entity_source.h"
#include "state/host_publish.h"
namespace cs::world::binary {
// 后台只读取票据保护的目录前缀；输出具有稳定编号和独立字节所有权。
std::vector<state::HostStateResource> ExportStateResources(NativeResourceView,
    std::span<const state::HostStateResource> previous);
}
