#pragma once
#include <array>
namespace cs::replica {
// J273 987410/BD01B0常量表经主审核对：每bit对应2x2内的实际占用象限。
// 不调用含TLS静态初始化的原生helper；几何来源仍是Authority实际offset，表只用于写前边界核验。
inline bool RoomShapeCells(int grid,int shape,std::array<int,4> &cells,unsigned &count) noexcept {
  static constexpr unsigned masks[]={0,1,1,1,5,5,3,3,15,14,13,11,7};
  count=0;if(grid<0||grid>168||shape<1||shape>12)return false;
  const int x=grid%13,y=grid/13;
  for(unsigned quadrant=0;quadrant<4;++quadrant)if(masks[shape]&(1u<<quadrant)){
    const int dx=int(quadrant&1),dy=int(quadrant>>1);
    if(x+dx>12||y+dy>12)return false;cells[count++]=grid+dx+13*dy;
  }
  return true;
}
} // namespace cs::replica
