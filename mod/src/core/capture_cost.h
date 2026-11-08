#pragma once
#include <cstdint>
#include <string>
namespace cs::capture {
enum class CostPart : unsigned { Native, Lua, Copy, Control, Handoff };
// 只在已激活的Host主线程记录固定聚合；嵌套计时做排他归属，总计不重复相加。
void FrameBegin(std::uint64_t outer,bool active) noexcept;
void FrameEnd() noexcept;
bool CostBegin(CostPart) noexcept;
void CostEnd() noexcept;
std::string CostFields(unsigned page);
class Cost final {
public:
  explicit Cost(CostPart part) noexcept:active_(CostBegin(part)){}
  ~Cost(){if(active_)CostEnd();}
private:bool active_;
};
}
