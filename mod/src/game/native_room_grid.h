#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace cs::game {
// C++20调和层与C++17 SDK层只传自有值，绝不把SDK对象交给后台或协议。
struct NativeRoomGridTransaction final {
  std::string model;
  std::uint64_t connection=0,revision=0,scene=0,session=0,run=0,floor=0,dimension=0,room=0;
  unsigned shape=0,grids=0;
  bool duplicate=false,stale=false,reset=false;
  std::vector<std::uint64_t> remove,create,change,rebuild;
  // 跨域目标仍为规范key依赖，不在18中写Entity/Player裸地址或冒称整世界完成。
  std::vector<std::string> external_dependencies;
};
NativeRoomGridTransaction PrepareNativeRoomGridTransaction(const std::string &current,
    const std::string &target,bool healthy);
// 正式来源使用同窗口的Room/Grid观测consumer，不依赖Recorder或E2E控制口。
std::string CaptureNativeRoomGridSource();
std::string NativeRoomGridSourceFields();
std::string ReconstructNativeRoomGridTarget(const std::string &baseline,const std::string &target);
// SDK只读补充：游戏上下文与必要门目标描述，和公开域状态在同一复制窗口内提交。
std::string CaptureNativeRoomContext();
bool NativeRoomGridBegin(std::uint64_t connection) noexcept;
const char *NativeRoomGridSubmit(const std::string &complete_target) noexcept;
void NativeRoomGridStop() noexcept;
bool NativeRoomGridDrained() noexcept;
bool NativeRoomGridOwnsDraw() noexcept;
void NativeRoomGridDraw() noexcept;
void NativeRoomGridQueueReturned(bool reset) noexcept;
void NativeRoomGridRenderReturned() noexcept;
std::string NativeRoomGridFields();
// 有界原生调用上下文；清单wrapper仅在此上下文拦截玩法和重定向描述查询。
bool NativeRoomGridContextActive() noexcept;
// 原生教程仍自行绘制；只把菜单角色查询替换成Authority角色类型的纯值。
int NativeRoomGridControlCharacter() noexcept;
// 仅E2E实际参考帧隔离Entity灯光；正常Host/Off和正式同步始终返回false。
bool NativeRoomGridReferenceOnly() noexcept;
// 墙缓存只保存原生装饰判别值及私有RNG状态；不会记录像素、顶点或图形调用。
void NativeBackdropWallsBegin(void *backdrop);
void NativeBackdropWallsEnd(void *backdrop,bool success);
void NativeBackdropEntityChoice(void *backdrop,int x,int y,unsigned layer,int kind,
                                float px,float py,bool flipx,bool flipy,bool returned);
std::uint32_t NativeRoomGridRandom() noexcept;
void NativeRoomGridSeed(std::uint32_t seed) noexcept;
bool NativeRoomGridRandomObjectAllowed(const void *object) noexcept;
void *NativeRoomGridDescriptor(int index,int dimension);
void *NativeRoomGridCurrentDescriptor() noexcept;
// 显式测试入口仍调用上述生产来源、调和、应用及原生读回，没有另一个应用器。
std::string NativeRoomGridTest(const std::string &token,const std::string &action);
} // namespace cs::game
