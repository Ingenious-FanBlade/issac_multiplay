#pragma once
// 实时协议、生成目录和发布清单共用版本；改版必须同时改变双方能力声明。
namespace cs::replica::stream {
inline constexpr unsigned kFraming=2,kSchema=1,kCompactSchema=3;
inline constexpr const char *kBuildContract="binary-state-v1";
}
