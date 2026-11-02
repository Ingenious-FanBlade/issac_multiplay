#pragma once
#include <array>
namespace cs::replica {
// 官方RailType与IsValidRailType的纯值等价表；不能把0..255全部当作可绘制轨道。
inline bool RoomRailTypeValid(int rail) noexcept {
  return (rail>=0&&rail<=10)||rail==16||rail==17||rail==32||rail==33||
    (rail>=80&&rail<=85)||(rail>=96&&rail<=101)||rail==112||rail==113||
    rail==144||rail==145||rail==160||rail==161||rail==255;
}
// J273 814DE0从BAC490逐8个DWORD复制；主审读取12项原始表。
// 每项为两个闭区间内部矩形；窄房仍使用完整15/28列数组，不能缩短stride。
inline std::array<int,8> RoomShapeTiles(int shape) noexcept {
  static constexpr int tiles[13][8]={
    {},{1,1,13,7,1,1,13,7},{1,3,13,5,1,3,13,5},{5,1,9,7,5,1,9,7},
    {1,1,13,14,1,1,13,14},{5,1,9,14,5,1,9,14},{1,1,26,7,1,1,26,7},{1,3,26,5,1,3,26,5},
    {1,1,26,14,1,1,26,14},{14,1,26,7,1,8,26,14},{1,1,13,7,1,8,26,14},
    {1,1,26,7,14,8,26,14},{1,1,26,7,1,8,13,14}};
  std::array<int,8> out{};if(shape>0&&shape<13)for(unsigned i=0;i<8;++i)out[i]=tiles[shape][i];return out;
}
inline int RoomShapeGridWidth(int shape) noexcept {return shape>=1&&shape<=5?15:shape>=6&&shape<=12?28:0;}
inline int RoomShapeGridHeight(int shape) noexcept {return shape>=1&&shape<=3?9:shape==4||shape==5?16:shape==6||shape==7?9:shape>=8&&shape<=12?16:0;}
// 有效存储含两矩形的一格边界；内部交集不包含墙。L缺口的远端格不会被矩形并集补满。
inline bool RoomShapeGridCell(int shape,int index,bool walls) noexcept {
  const int width=RoomShapeGridWidth(shape),height=RoomShapeGridHeight(shape);if(!width||index<0||index>=width*height)return false;
  const auto t=RoomShapeTiles(shape);const int x=index%width,y=index/width,pad=walls?1:0;
  for(unsigned i=0;i<8;i+=4)if(x>=t[i]-pad&&x<=t[i+2]+pad&&y>=t[i+1]-pad&&y<=t[i+3]+pad)return true;return false;
}
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
