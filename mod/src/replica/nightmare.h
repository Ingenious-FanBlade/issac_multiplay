#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace cs::replica {
inline constexpr unsigned kNightmareLayerLimit=64;
inline constexpr unsigned kNightmareWireLimit=8192;

// 楼层过场只传本地资源名与实际绘制值。帧内顺序由 Host 的原生绘制决定。
struct NightmareLayer final {
  std::string sprite,animation,sheet;
  unsigned layer=0,frame=0,flips=0;
  std::array<float,2> position{},top_left{},bottom_right{},offset{},scale{},layer_position{},layer_scale{},crop{};
  float rotation=0,layer_rotation=0;
  std::array<float,11> color{},champion_color{},layer_color{};
};
struct NightmareFrame final {
  float width=0,height=0,fade=0;
  std::vector<NightmareLayer> layers;
};
using NightmareFrameRef=std::shared_ptr<const NightmareFrame>;
bool NightmareAssetPath(const std::string &,const char *extension) noexcept;
}
