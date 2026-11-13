#pragma once
#include "state/capture_executor.h"
#include "world/collector.h"
#include "state/display_contract.h"
#include "world/binary/display_values.h"
#include <memory>

namespace cs::world::binary {
// 同窗结构只在主线程借用。物理槽、活动顺序和八门别名独立保存，绝不交后台。
struct NativeScene {
  std::uint32_t game=0,room=0,current_descriptor=0,ordinary_count=0,dimension=0,extent=0;
  std::array<std::uint32_t,456> grids{};
  std::array<std::uint32_t,527> descriptors{};
  std::array<std::uint16_t,507> occupancy{};
  std::array<std::uint16_t,527> descriptor_order{};
  std::array<std::uint64_t,456> grid_handles{};
  std::uint32_t descriptor_count=0,effective_curses=0;
  std::array<bool,80> seed_effects{};
  bool greed=false;
};
// 音频只保存正常Host已选资源及Options前参数；没有后端handle或硬件游标。
struct NativeAudioValue {
  state::CaptureLink asset{state::CapturedReferenceKind::Null,0};
  float gain=0,pitch=1,pan=0;
  std::uint32_t start_frame=0;
  std::uint8_t volume_class=0,segment=0,kind=0,mix_rule=0;
  bool loop=false,paused=false;
};
// Host自有显示owner的十行缓存；previous/threshold只参与显示相位生产，不发送玩法属性。
// 每个身体的结构事实来自同窗成员集合和真实C/S拓扑，不由raw valid／类型值猜测。
struct EntityFacts {
  state::CaptureSubject subject;
  std::uint32_t address=0,memberships=0,identity_address=0;
  std::uint16_t roster=state::kNullIndex;
  std::uint8_t role=6,control_flags=0,ordinal=255;
  bool ready=false,player_ready=false,text_ready=false,activation_qualified=false,group_known=false;
  std::uint64_t group=0,participant=0,control_session=0,binding_revision=0;
  const NativeScene *scene=nullptr;
  std::uint16_t layout=0,source_slot=state::kNullIndex;
  std::uint32_t grid_index=UINT32_MAX;
  bool score_ready=false;
  std::uint32_t manager=0;
  // Q12由同一ordinary后态读取，未准备时拒绝合成字段，不回落normal。
  bool display_ready=false,host_paused=false;
  state::DisplayBranch display_branch=state::DisplayBranch::Normal;
  std::span<const DisplayValueNode> display_values;
};
// 主线程owner重新核Collector窗口及subject/use/shape；原生引用先预约自有身份句柄。
struct EntityServices {
  void *owner=nullptr;
  bool (*current)(void *,const state::CaptureWitness &,const EntityFacts &) noexcept=nullptr;
  state::CaptureLink (*reference)(void *,std::uint32_t) noexcept=nullptr;
  // 伴随玩家的只读投影也必须满足同窗完整初始化资格，不凭可读地址访问Player专用字段。
  bool (*player_current)(void *,std::uint32_t) noexcept=nullptr;
  bool (*entity_current)(void *,std::uint32_t) noexcept=nullptr;
};
// 资源描述只含已选本地内容、有限数字与自有依赖handle；既不是地址也不是GPU ready。
struct NativeResource {
  std::uint64_t handle=0;
  // 仅自有配置owner的身份；相同内容不推导两个配置对象连续／相同，地址不进入目录。
  std::uint64_t source_owner=0;
  std::uint8_t table=255,kind=255;
  // false表示只有真实本地路径，文件几何仍待资源owner准备；禁止补零后宣称typed payload完整。
  bool metadata_known=true;
  std::array<std::uint64_t,3> links{};
  std::array<std::uint32_t,8> values{};
  std::uint32_t text=0,text_bytes=0,extra=0,extra_count=0;
};
struct NativeAnimationLayer {std::uint16_t id=0;std::uint32_t frames=0;};
struct NativeResourceView {
  std::span<const NativeResource> entries;
  std::span<const std::byte> bytes;
  std::span<const NativeAnimationLayer> layers;
};
// 程序背景只保存有界显示操作和自有资源handle；worker稍后将handle转Peer资源号。
// 明确紧排避免把编译器填隙或原生顶点／GPU字节当成资源描述。
#pragma pack(push,1)
struct NativeSurfaceOperation {
  std::uint8_t op=0,blend=0,sampler=0,source_space=0;
  std::uint64_t image=0,shader=0;
  std::array<float,8> source{},destination{};
  std::array<std::array<std::uint32_t,5>,4> colors{};
  std::array<float,11> modifier{};
  std::array<float,4> context{};
};
#pragma pack(pop)
// 所有动态区在后台准备；主线程只移动有界游标。容量失败不截断或返回伪空值。
struct EntitySourceCapacity {
  std::uint32_t scratch_bytes=2u<<20,witnesses=65536,resources=16384,resource_bytes=4u<<20,animation_layers=65536;
  std::uint32_t topology_bodies=256,topology_bytes_per_body=16384;
  // 官方静态旁表由SDK桥在冷准备期按已核模块／机器关系绑定，0保持具名缺源。
  std::uint32_t grid_plus=0,grid_plus_vtable=0;
};
class EntitySource final {
public:
  struct Impl;
  EntitySource(MemoryReader,std::thread::id,EntityServices,EntitySourceCapacity={});
  ~EntitySource();
  EntitySource(const EntitySource &)=delete;
  EntitySource &operator=(const EntitySource &)=delete;
  bool Capture(const state::CaptureWitness &,EntityFacts,state::BodyStream,state::CaptureValueWriter &,
               state::CaptureDiagnostic &,bool ordinary_joint=false) noexcept;
  // 仅实际ordinary外层成功提交后确认P6；尚无同拓扑ordinary时不能产生fast body。
  void OrdinaryCommitted(state::CaptureSubject) noexcept;
  void Invalidate() noexcept;
  void Retire(state::CaptureSubject) noexcept;
  // 只在owner线程借用；第11步必须复制成不可变资源快照后才能交后台。
  NativeResourceView Resources() const noexcept;
  // 只供持有有效consumer票据的worker借用其stamp前缀；这些预分配区只追加且从不覆盖旧项。
  NativeResourceView PublishedResources(state::CaptureCatalogStamp) const noexcept;
  std::uint32_t Topology(state::CaptureSubject) const noexcept;
  bool UsesSprite(std::uint32_t address) const noexcept;
  // 纯值种子／有效curse投影和结构读取在同一ordinary窗口完成，供Room／Level复用。
  bool ReadSceneContext(const state::CaptureWitness &,EntityFacts,NativeScene &,state::CaptureDiagnostic &) noexcept;
  // Wiki总出现率之外的楼层资格；概率本身由已授权C++查询接口提供。
  bool HudDealEligibility(const state::CaptureWitness &,EntityFacts,bool &,state::CaptureDiagnostic &) noexcept;
  bool ReadDisplayContext(const state::CaptureWitness &,EntityFacts &,state::CaptureDiagnostic &) noexcept;
  bool AudioResource(const state::CaptureWitness &,EntityFacts,std::uint32_t sample,state::CaptureLink &,state::CaptureDiagnostic &) noexcept;
  bool DisplayItemResource(const state::CaptureWitness &,EntityFacts,std::uint32_t config,state::CaptureLink &,state::CaptureDiagnostic &) noexcept;
  bool DisplayProceduralAtlas(const state::CaptureWitness &,EntityFacts,std::uint32_t,state::CaptureLink &,state::CaptureDiagnostic &) noexcept;
  bool DisplayAudioPathResource(const state::CaptureWitness &,EntityFacts,std::span<const std::byte>,state::CaptureLink &,state::CaptureDiagnostic &) noexcept;
  std::uint64_t RetainedBytes() const noexcept;
private:
  std::unique_ptr<Impl> impl_;
};
}
