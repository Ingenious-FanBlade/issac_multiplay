#pragma once
#include <cstdint>
#include <memory>
#include <string>

namespace cs::game {
// 正式Authority资源请求与owned快照；不依赖录制或E2E文件。只在主线程调用。
bool RequestOriginalRoomGrid() noexcept;
std::string OriginalRoomGridSnapshot();
std::string OriginalRoomGridFields();
void OriginalRoomGridRenderReturned() noexcept;
// 唯一Hook清单的只读上下文。kind: 0 Room，1 Grid，2 RockTop，3/4/5 Backdrop，6 GridLight，7 Entity排除，8 FX，11 Room.PreRender。
class OriginalRoomGridScope final {
public:
  OriginalRoomGridScope(unsigned kind,void *owner) noexcept;
  ~OriginalRoomGridScope() noexcept;
  void Returned() noexcept;
private:
  unsigned kind_=0;bool active_=false,returned_=false;std::uint64_t previous_=0;
};
class OriginalLayerScope final {
public:
  explicit OriginalLayerScope(void *animation) noexcept;
  ~OriginalLayerScope() noexcept;
private: bool active_=false;unsigned previous_=0;
};
void OriginalImageBefore() noexcept;
void OriginalImageReturned(void *image,float *vertices) noexcept;
// SDK图形端的自有图像／最终顶点消费，不调用ANM时间或Grid玩法。
struct OriginalRoomGraphics;
std::shared_ptr<OriginalRoomGraphics> PrepareOriginalRoomGraphics(const std::string &descriptor);
void DrawOriginalRoomGraphics(OriginalRoomGraphics &);
unsigned OriginalGraphicsImages(const OriginalRoomGraphics &) noexcept;
unsigned OriginalGraphicsDraws(const OriginalRoomGraphics &) noexcept;
// C++20纯值桥；guard为source生命周期标记，SDK单元不包含world/replica的原生对象布局。
struct OriginalRoomSource final {
  std::string model;
  std::uint64_t session=0,epoch=0,run=0,room=0,mutation=0,revision=0;
};
bool ReadOriginalRoomSource(OriginalRoomSource &) noexcept;
std::string OriginalRoomSourceFields();
bool OriginalRoomSourceStable(const OriginalRoomSource &) noexcept;
std::string OriginalGridKey(const void *grid) noexcept;
// E2E只将正式owned值输出到本次临时目录；世界修改属于独立的玩法夹具。
std::string OriginalRoomGridTest(const std::string &token,const std::string &action);
} // namespace cs::game
