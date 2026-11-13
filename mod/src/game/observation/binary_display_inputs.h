#pragma once
namespace cs::game {
// 原生查询只在Host同world的主线程后态执行；保留原始概率而非面板百分比。
struct BinaryDisplayInputs {
  float devil_total=0,angel_share=0,planetarium=0;
  bool progress_blocked=false;
};
bool QueryBinaryDisplayInputs(void *game,void *room,BinaryDisplayInputs &) noexcept;
}
