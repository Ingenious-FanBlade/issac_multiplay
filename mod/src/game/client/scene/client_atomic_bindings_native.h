#pragma once
#include "game/client/scene/client_preparation_native.h"
#include "game/client/scene/client_read_window.h"
#include <array>
#include <bitset>
#include <string_view>

namespace cs::game {
// 原生端只接收已定界的数字值及准备层实际借用的对象。
struct ClientBinaryInit {
  void *object=nullptr,*vptr=nullptr,*config=nullptr,*parent=nullptr,*spawner=nullptr;
  unsigned type=0,variant=0,subtype=0,seed=0,category=0,spawner_type=0,spawner_variant=0;
  int player_type=0;
  std::array<float,2> position{},velocity{};
  bool fresh=false,entered=false,returned=false;
};
struct ClientBinaryEntityMember {
  void *object=nullptr;
  unsigned type=0,variant=0,subtype=0,index=UINT32_MAX;
  bool valid=true,room=true,render=true,update=true,add_update=true;
};
struct ClientBinaryRoomMembers {
  void *descriptor=nullptr;
  std::array<void *,448> grids{};
  std::array<std::uint32_t,448> water_clip{};
  std::array<void *,8> doors{};
  std::array<void *,256> roster{};
  std::array<bool,527> descriptors{};
  std::size_t roster_count=0;
  unsigned width=0,height=0,type=0,dimension=0,descriptor_count=0;
  int entry=0,last_boss=-1,greed_treasure=-1;
  std::array<unsigned,507> occupancy{};
};
struct ClientBinaryRoomPrevious {
  ClientBinaryRoomMembers members;
  struct DescriptorBinding {void *data=nullptr,*override_data=nullptr;int list=0;};
  std::array<DescriptorBinding,527> descriptors{};
  unsigned last_entry=0,last_dimension=0,index=0;
  float sfx_volume=1.f;
  void *offsets=nullptr;
};
const char *ClientBinaryInitFailure(ClientBinaryInit &,ClientNativeReadWindow *reads=nullptr) noexcept;
bool ClientBinaryInitApply(ClientBinaryInit &,const char *&) noexcept;
// Init 窗口前后的池位差集。仅处理该窗口内新增且未被权威目标占用的池对象。
struct ClientBinaryInitPoolSnapshot {
  const void *factory=nullptr;
  std::bitset<4096> used;
};
bool ClientBinaryRememberInitPools(const ClientNativeRoots &,ClientBinaryInitPoolSnapshot &) noexcept;
bool ClientBinaryCleanupInitPools(const ClientNativeRoots &,const ClientBinaryInitPoolSnapshot &,
    const void *context,bool (*target)(const void *,const void *) noexcept,bool &changed) noexcept;
bool ClientBinaryGridInit(void *,unsigned) noexcept;
unsigned ClientBinaryLocalIndex(void *) noexcept;
unsigned ClientBinaryRetireCategory(void *) noexcept;
bool ClientBinaryEntityRetire(const ClientNativeRoots &,void *,unsigned) noexcept;
bool ClientBinaryRetireLiveMember(const ClientNativeRoots &,void *,bool grid,bool player,
    const std::array<std::uint32_t,3> *registered_shape=nullptr) noexcept;
bool ClientBinaryEntityMorph(void *,unsigned category,unsigned type,unsigned variant,unsigned subtype,unsigned seed,int champion) noexcept;
bool ClientBinaryInstallFrame(const ClientNativeRoots &,std::uint32_t,std::uint32_t &) noexcept;
bool ClientBinaryRestoreFrame(const ClientNativeRoots &,std::uint32_t) noexcept;
const char *ClientBinaryMemberFailure(const ClientNativeRoots &,void *const *,std::size_t,
    const ClientBinaryEntityMember *,std::size_t,const ClientBinaryRoomMembers *,const ClientBinaryRoomMembers *) noexcept;
bool ClientBinaryCaptureRoom(const ClientNativeRoots &,ClientBinaryRoomPrevious &) noexcept;
bool ClientBinarySetRoom(const ClientNativeRoots &,const ClientBinaryRoomMembers &) noexcept;
bool ClientBinaryRoomGeometryCurrent(const ClientNativeRoots &,const ClientBinaryRoomMembers &) noexcept;
// render_members 按地址排列。只有普通 World 可忽略实际 render 列表的绘制顺序。
bool ClientBinaryMemberSetsCurrent(const ClientNativeRoots &,const ClientBinaryEntityMember *,std::size_t,
    const ClientBinaryRoomMembers &,void *const *render_members,std::size_t render_count) noexcept;
bool ClientBinarySetMembers(const ClientNativeRoots &,const ClientBinaryEntityMember *,std::size_t,
    ClientBinaryEntityMember *,std::size_t,const ClientBinaryRoomMembers &) noexcept;
bool ClientBinaryRestoreRoom(const ClientNativeRoots &,const ClientBinaryRoomPrevious &) noexcept;
bool ClientBinaryDetachMember(const ClientNativeRoots &,void *,bool grid) noexcept;
bool ClientBinaryUnpublishWorld(const ClientNativeRoots &,const ClientBinaryEntityMember *,std::size_t,
    const ClientBinaryRoomMembers &) noexcept;
void *ClientBinaryRunClock(const ClientNativeRoots &) noexcept;
// 每个 Overlay 的 Player 槽具有原生反向登记；额外承载仅构造 Render 读取的两个 Sprite。
void *ClientBinaryGlobalOverlay(const ClientNativeRoots &) noexcept;
bool ClientBinaryCreateOverlay(void *&) noexcept;
bool ClientBinaryDestroyOverlay(void *&) noexcept;
void ClientBinaryRenderOverlay(void *) noexcept;
bool ClientBinaryShowOverlay(void *,int id,int delay,void *player) noexcept;
bool ClientBinaryOverlayPlayer(void *,void *player) noexcept;
void *ClientBinaryOverlayCurrentPlayer(void *) noexcept;
void ClientBinaryHideOverlay(void *) noexcept;
// 程序配置资源归准备表，原生负ID表和两张本地图集归当前world。
struct ClientBinaryProceduralDefinition {
  void *configuration=nullptr;
  int id=0;
  std::string_view name,description,gfx;
};
struct ClientBinaryProcedural;
bool ClientBinarySetProcedural(const ClientNativeRoots &,ClientBinaryProcedural *&,const ClientBinaryProcedural *,
    const ClientBinaryProceduralDefinition *,std::size_t,void *atlas,void *small_atlas) noexcept;
bool ClientBinaryRetireProcedural(const ClientNativeRoots &,ClientBinaryProcedural *&,bool restore,bool committed) noexcept;
bool ClientBinaryCheckProcedural(const ClientNativeRoots &,const ClientBinaryProcedural *) noexcept;
bool ClientBinaryCheckProceduralEntry(const ClientBinaryProcedural *,std::size_t,const ClientBinaryProceduralDefinition &) noexcept;
void ClientBinaryCommitProcedural(ClientBinaryProcedural *) noexcept;

// 特殊字段按原生类型赋值。先准备可回退的旧对象，再进入可能改变内部分配的调用。
enum class ClientBinarySpecial : unsigned {
  None,Metadata,Companion,File,Animation,WaveAnimation,LayerCount,LayerFrames,NullFrames,LayerPath,
  Image,LayerImage,Config,DescriptorConfig,BorrowedEntity,BorrowedPlayer,Rubble,Text,DefaultName,
  Widen,DescriptorFlags,DoorLight,DoorAlpha,FXCondition,FXBackdrop,VeinChildren,Daily,Ascent,Time,FXCount
};
struct ClientBinarySpecialWrite {
  ClientBinarySpecial kind=ClientBinarySpecial::None;
  void *parent=nullptr,*resource=nullptr;
  unsigned offset=0,size=0,count=0;
  const std::byte *bytes=nullptr;
  std::size_t length=0;
  std::uint32_t frame=0;
  // Player 主 Sprite 的图层归 World。普通 Body 只接管本次动画状态。
  bool shared_player_sprite=false;
};
struct ClientBinarySpecialUndo {
  // 原生Sprite、图像引用及字符串需按其自身的构造／析构规则保管。
  void *storage=nullptr;
  void *destination=nullptr;
  std::array<std::byte,16> original{};
  std::size_t original_size=0;
  bool entered=false,applied=false,uncertain=false,committed=false;
};
bool ClientBinaryApplySpecial(const ClientNativeRoots &,const ClientBinarySpecialWrite &,ClientBinarySpecialUndo &) noexcept;
// 供滚动漂移和 E2E 抽样使用，只读当前原生值。
// 0为无投影，1为相等，2为差异，3为承载已失效。
unsigned ClientBinaryReadSpecial(const ClientNativeRoots &,const ClientBinarySpecialWrite &) noexcept;
bool ClientBinaryRetireSpecial(const ClientBinarySpecialWrite &,ClientBinarySpecialUndo &) noexcept;
std::size_t ClientBinarySpecialCharge(const ClientBinarySpecialWrite &) noexcept;
void *ClientBinaryLayer(void *,unsigned) noexcept;
// 按最终 Sprite／Layer flags 配置本地图像顶点格式，并清理不兼容的原生空闲批次。
bool ClientBinaryLayerGraphics(void *layer,bool repair,bool &written) noexcept;
void *ClientBinaryPlus(void *,unsigned,ClientNativeReadWindow *reads=nullptr) noexcept;
bool ClientBinaryGridClip(const ClientNativeRoots &,const ClientBinaryRoomMembers &,const void *,std::uint32_t &) noexcept;
} // namespace cs::game
