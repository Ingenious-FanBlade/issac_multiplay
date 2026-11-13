#pragma once
#include "state/client_validation.h"

namespace cs::state {
inline constexpr std::uint32_t kClientPlanNone=UINT32_MAX;
// P6目录和P0 Sprite共用同一原生vector；安装与回读必须按完整容器归属。
bool ClientCostumeProjection(std::uint16_t layout,std::uint16_t field) noexcept;
// 域决定暂存顺序；所有对象先取得承载，引用最后统一绑定，允许合法的环与别名。
enum class ClientInstallDomain:std::uint8_t {Participant,ControlGroup,Run,Level,Descriptor,Room,Grid,Player,Entity,Sfx};
enum class ClientInstallAction:std::uint8_t {Reuse,Join,Reshape,Remap};
enum class ClientInstallPhase:std::uint8_t {Resources,NativeResources,CaptureOldRoom,StageObjects,StageFields,Values,RelationFields,Relations,Display,CaptureNewRoom,Events,Publish,Retire};
struct ClientInstallObject {
  StateMemberEntry member;
  ClientInstallDomain domain=ClientInstallDomain::Entity;
  ClientInstallAction action=ClientInstallAction::Reuse;
  std::uint8_t streams=0;
};
// 递归路径是紧凑的数字父链，包含元素／Weapon槽及choice布局；没有字段名或原生地址。
struct ClientInstallPath {std::uint32_t parent=kClientPlanNone;CodecPathStep step;};
struct ClientInstallSlice {std::uint32_t offset=0,size=0;};
struct ClientInstallBlock {std::uint32_t version=0,game_frame=0;std::uint64_t source_revision=0;ClientInstallSlice bytes;};
struct ClientInstallRecord {
  std::uint32_t object=kClientPlanNone,event=kClientPlanNone;
  std::uint16_t layout=0;
  BodyStream stream=BodyStream::World;
  // 仅补齐共享costume的配对投影，不代表该流版本已收到更新，也不参与ACK推进。
  bool paired_costume=false;
  PresentationBatchContext context;
  ClientMemberSnapshotRef members;
  ClientResourceSnapshotRef resources;
  std::array<ClientInstallBlock,32> blocks{};
  std::uint32_t mask=0,first_field=0,field_count=0;
  ClientInstallSlice bytes;
};
// 每个实际字段只登记一次；合法空集合也生成操作，条件缺席保留为显式清理语义。
// definition是可信Fields()索引，native写入仍由对应数字owner执行，不能直接按offset复制。
struct ClientInstallField {
  std::uint32_t record=0,path=0,definition=0,elements=0,slot_mask=0;
  ClientInstallSlice bytes;
  bool present=false,payload_present=false;
  ClientInstallPhase phase=ClientInstallPhase::Values;
};
// 嵌套实例和标量叶已在后台定界；供原生容器／Sprite owner按元素顺序直接消费。
struct ClientInstallValue {
  std::uint32_t record=0,path=kClientPlanNone;
  std::uint16_t layout=0,component=0;
  Operation operation=Operation::Copy;
  ClientInstallSlice bytes;
};
struct ClientInstallBinding {
  std::uint32_t value=0,object=kClientPlanNone;
  PresentationMember member;
  ResourceKey resource;
  bool is_resource=false,is_null=false;
};
struct ClientInstallDescriptor {std::uint32_t object=0;std::uint16_t source_slot=0,ordinary_ordinal=kNullIndex;};
struct ClientInstallGrid {
  std::uint32_t object=0,index=0,spawn_seed=0;
  std::int32_t type=0,variant=0,state=0;
  bool has_spawn_seed=false;
};
// 冷构造使用实际type／variant／subtype及已选择配置，不能按参与者ordinal猜Player类型。
struct ClientInstallEntity {
  std::uint32_t object=0,type=0,variant=0,subtype=0,init_seed=0,order=0;
  ResourceKey configuration;
};
struct ClientInstallPlayer {
  std::uint32_t object=0,group=kClientPlanNone,topology=0;
  std::int32_t player_type=0;
  std::uint16_t roster=kNullIndex;
  std::uint8_t membership=0,role=6,control=0;
};
struct ClientInstallGroup {
  std::uint32_t object=0,participant=0,first_anchor=0,anchor_count=0;
  std::uint64_t session=0,binding=0;
  std::uint8_t ordinal=0;
};
struct ClientInstallEvent {
  std::uint32_t record=0;
  std::uint64_t sequence=0,transition=0;
  std::uint32_t game_frame=0;
  StateEventKind kind=StateEventKind::Sfx;
  PresentationMember owner;
  bool has_owner=false;
  // Host已接纳的SFX不再次执行Client FrameDelay节流。
  std::int32_t replay_frame_delay=0;
};
// 开始／完成可独立出现（恢复可能只保留其中之一），不靠本地FSM或调用次数补事件。
struct ClientInstallTransition {
  std::uint64_t sequence=0;
  std::uint32_t start=kClientPlanNone,finish=kClientPlanNone;
};
enum class ClientInstallNativeNeedKind:std::uint8_t {Music,MusicJingle,BossIntro};
// 这些显示资源由原生具名owner从本地固定目录解析；数字请求不冒充已经加载的资源。
struct ClientInstallNativeNeed {
  ClientInstallNativeNeedKind kind;
  std::uint32_t record=0;
  std::int64_t id=0,second_id=0;
  std::uint64_t generation=0;
};
// index指向相应数组：StageObjects为object，StageFields／Values／RelationFields／Display为field，
// Relations为binding，Resources为资源需要，Events为event，Retire为retired。
struct ClientInstallCommand {ClientInstallPhase phase;std::uint32_t index=kClientPlanNone;};
struct ClientInstallWorld {
  std::uint32_t run=kClientPlanNone,level=kClientPlanNone,room=kClientPlanNone,sfx=kClientPlanNone,current_descriptor=kClientPlanNone;
  std::array<std::uint32_t,8> doors{};
  std::array<std::uint16_t,507> occupancy{};
  std::uint32_t grid_width=0,grid_height=0,topology=0;
  std::int32_t current_entry=0;
  std::uint8_t dimension=0;
  bool replace_world=false,replace_body=false;
};
// 不可变、无原生地址的完整安装输入。所有数组及正文副本计本Peer Values额度。
// Main只消费数字字段／已解码边界；实际物化、ABI覆盖与完成回执由后续原生owner提供。
class ClientStateInstallPlan final {
public:
  explicit ClientStateInstallPlan(const std::shared_ptr<ClientStateMemory> &);
  const StateTarget &Target() const noexcept{return candidate_->Target();}
  const ClientInstallWorld &World() const noexcept{return world_;}
  std::span<const ClientInstallObject> Objects()const noexcept{return objects_;}
  std::span<const StateMemberEntry> Retired()const noexcept{return retired_;}
  std::span<const ClientInstallRecord> Records()const noexcept{return records_;}
  std::span<const ClientInstallPath> Paths()const noexcept{return paths_;}
  std::span<const ClientInstallField> Fields()const noexcept{return fields_;}
  std::span<const ClientInstallValue> Values()const noexcept{return values_;}
  std::span<const ClientInstallBinding> Bindings()const noexcept{return bindings_;}
  std::span<const ClientStateResourceNeed> Resources()const noexcept{return resources_;}
  std::span<const ClientInstallDescriptor> Descriptors()const noexcept{return descriptors_;}
  std::span<const ClientInstallGrid> Grids()const noexcept{return grids_;}
  std::span<const ClientInstallEntity> Entities()const noexcept{return entities_;}
  std::span<const ClientInstallPlayer> Players()const noexcept{return players_;}
  std::span<const ClientInstallGroup> Groups()const noexcept{return groups_;}
  std::span<const std::uint32_t> Anchors()const noexcept{return anchors_;}
  std::span<const ClientInstallEvent> Events()const noexcept{return events_;}
  std::span<const ClientInstallTransition> Transitions()const noexcept{return transitions_;}
  std::span<const ClientInstallNativeNeed> NativeNeeds()const noexcept{return native_needs_;}
  std::span<const ClientInstallCommand> Commands()const noexcept{return commands_;}
  std::span<const std::byte> Bytes(ClientInstallSlice s)const;
  ClientShadowStateRef Effective(BodyStream)const;
  std::shared_ptr<ClientStateMemory> Memory()const noexcept{return bytes_.get_allocator().Memory();}
  bool Describes(const ClientStateCandidateRef &c)const noexcept{return candidate_==c;}
private:
  friend struct ClientInstallBuilder;
  ClientStateCandidateRef candidate_;
  std::array<ClientShadowStateRef,2> effective_;
  ClientInstallWorld world_;
  ClientStateVector<ClientInstallObject> objects_;
  ClientStateVector<StateMemberEntry> retired_;
  ClientStateVector<ClientInstallRecord> records_;
  ClientStateVector<ClientInstallPath> paths_;
  ClientStateVector<ClientInstallField> fields_;
  ClientStateVector<ClientInstallValue> values_;
  ClientStateVector<ClientInstallBinding> bindings_;
  ClientStateVector<ClientStateResourceNeed> resources_;
  ClientStateVector<ClientInstallDescriptor> descriptors_;
  ClientStateVector<ClientInstallGrid> grids_;
  ClientStateVector<ClientInstallEntity> entities_;
  ClientStateVector<ClientInstallPlayer> players_;
  ClientStateVector<ClientInstallGroup> groups_;
  ClientStateVector<std::uint32_t> anchors_;
  ClientStateVector<ClientInstallEvent> events_;
  ClientStateVector<ClientInstallTransition> transitions_;
  ClientStateVector<ClientInstallNativeNeed> native_needs_;
  ClientStateVector<ClientInstallCommand> commands_;
  ClientStateVector<std::byte> bytes_;
};
// 只由后台调用：递归校验、跨域关系、资源闭包、结构动作和有序执行描述一起封存。
// 抛具名ClientShadowError／预算失败；绝不返回空计划或以原生尚未实现为Ready。
std::shared_ptr<const ClientStateInstallPlan> BuildClientStateInstallPlan(const ClientStatePreparedTarget &,const ClientStateTables &);
} // namespace cs::state
