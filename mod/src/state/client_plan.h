#pragma once
#include "state/client_validation.h"

namespace cs::state {
inline constexpr std::uint32_t kClientPlanNone=UINT32_MAX;
// P6目录和P0 Sprite共用同一原生vector；安装与回读必须按完整容器归属。
bool ClientCostumeProjection(std::uint16_t layout,std::uint16_t field) noexcept;
// 主线程复核共享承载，只比较实际玩家身份、拓扑与 World 服装目录。
bool ClientCostumeDirectoryMatches(const ClientShadowStateRef &,const ClientShadowStateRef &,const PresentationMember &) noexcept;
// 域决定暂存顺序；所有对象先取得承载，引用最后统一绑定，允许合法的环与别名。
enum class ClientInstallDomain:std::uint8_t {Participant,ControlGroup,Run,Level,Descriptor,Room,Grid,Player,Entity,Sfx};
enum class ClientInstallAction:std::uint8_t {Reuse,Join,Reshape,Remap};
enum class ClientInstallPhase:std::uint8_t {Resources,NativeResources,CaptureOldRoom,StageObjects,StageFields,Values,RelationFields,Relations,Display,CaptureNewRoom,Events,Publish,Retire};
struct ClientInstallObject {
  StateMemberEntry member;
  ClientInstallDomain domain=ClientInstallDomain::Entity;
  ClientInstallAction action=ClientInstallAction::Reuse;
  std::uint8_t streams=0;
  // 按 domain 指向实体、格子或房间描述符的纯值下标，只有该计划确实带对应描述时才填。
  std::uint32_t description=kClientPlanNone;
};
// 递归路径是紧凑的数字父链，包含元素／Weapon槽及choice布局；没有字段名或原生地址。
struct ClientInstallPath {
  std::uint32_t parent=kClientPlanNone;CodecPathStep step;
  // 后台按完整规范父链生成；field_path去除末级元素／Choice，缺少对应字段时保留None。
  // 仅含数字归属，主线程仍须重新定位实际承载与验证生命周期。
  std::uint32_t normalized=kClientPlanNone,field_path=kClientPlanNone;
  std::uint16_t canonical=0;
};
struct ClientInstallSlice {
  std::uint32_t offset=0,size=0;
  // 普通记录直接借本计划引用表中的不可变块；有序事件仍使用计划内偏移。
  const std::byte *block=nullptr;
};
struct ClientInstallBlock {std::uint32_t version=0,game_frame=0;std::uint64_t source_revision=0;ClientInstallSlice bytes;};
// World 只为实际存在的块存储三个结束下标，首块从记录起点开始。
struct ClientInstallBlockEnd {std::uint32_t field=0,value=0,binding=0;};
// 镜像记录独立于暂存字段；compare_base 供主线程核对是否确实写过计算差分所用的前值。
struct ClientFlatWritePlan {
  explicit ClientFlatWritePlan(const std::shared_ptr<ClientStateMemory> &memory)
    :groups(ClientStateAllocator<std::uint32_t>(memory)) {}
  ClientFlatImageRef image,compare_base;
  ClientStateVector<std::uint32_t> groups;
  bool full=false,skip=false;
};
struct ClientInstallRecord {
  std::uint32_t object=kClientPlanNone,event=kClientPlanNone;
  std::uint16_t layout=0;
  BodyStream stream=BodyStream::World;
  PresentationBatchContext context;
  ClientMemberSnapshotRef members;
  ClientResourceSnapshotRef resources;
  std::array<ClientInstallBlock,32> blocks{};
  std::uint32_t mask=0,first_field=0,field_count=0;
  // 每批逻辑区间的位置可变，记录内部的叶、字段和引用序号保持稳定。
  std::uint32_t first_value=0,value_count=0,first_binding=0,binding_count=0;
  std::uint32_t first_block_end=0;
  // 与真实已接受根相比。结构模板和实际引用独立于普通字节变化。
  std::uint32_t changed_mask=0;
  std::uint32_t value_mask=0,structure_mask=0,dependency_mask=0;
  ClientInstallSlice bytes;
  std::shared_ptr<const ClientFlatWritePlan> flat;
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
  // 后台按完整对象／规范路径封存归属；仅数字下标，不借用原生地址。
  std::uint32_t field=kClientPlanNone,parent_field=kClientPlanNone;
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
  // 平铺根所需的构造事实由后台从镜像直接读取，仍由现有构造与名册 owner 消费。
  struct FlatInit {
    std::array<float,2> position{},velocity{};
    std::uint64_t flags=0;
    std::uint32_t spawner_type=0,spawner_variant=0,parent=kClientPlanNone,spawner=kClientPlanNone;
  };
  std::optional<FlatInit> flat_init;
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
  // 事件可能属于已退出当前表的历史对象；此时不在主线程借用它的承载。
  std::uint32_t owner_object=kClientPlanNone;
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
// 独占 World 根的数字描述可由多份计划共享。字段归属和引用使用记录内序号，
// 字节由各计划当前的 shadow 块持有，不引用旧计划或原生地址。
struct ClientInstallRecordData final {
  explicit ClientInstallRecordData(const std::shared_ptr<ClientStateMemory> &m)
    :fields(ClientStateAllocator<ClientInstallField>(m)),values(ClientStateAllocator<ClientInstallValue>(m)),
     bindings(ClientStateAllocator<ClientInstallBinding>(m)),lookup(ClientStateAllocator<FieldSlot>(m)),
     stage_fields(ClientStateAllocator<std::uint32_t>(m)),
     presentation_fields(ClientStateAllocator<std::uint32_t>(m)),presentation_values(ClientStateAllocator<std::uint32_t>(m)),
     native_binding_values(ClientStateAllocator<std::uint32_t>(m)),native_display_values(ClientStateAllocator<std::uint32_t>(m)) {}
  struct FieldSlot {std::uint32_t path,local;};
  ClientStateVector<ClientInstallField> fields;
  ClientStateVector<ClientInstallValue> values;
  ClientStateVector<ClientInstallBinding> bindings;
  ClientStateVector<FieldSlot> lookup;
  ClientStateVector<std::uint32_t> stage_fields;
  ClientStateVector<std::uint32_t> presentation_fields,presentation_values,native_binding_values,native_display_values;
  bool shareable=true;
};
struct ClientInstallWorld {
  std::uint32_t run=kClientPlanNone,level=kClientPlanNone,room=kClientPlanNone,sfx=kClientPlanNone,current_descriptor=kClientPlanNone;
  std::array<std::uint32_t,8> doors{};
  std::array<std::uint16_t,507> occupancy{};
  std::uint32_t grid_width=0,grid_height=0;
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
  std::uint32_t RecordIndex(BodyStream,const PresentationMember &)const noexcept;
  // B4／B5 共享结构资格。后台另核块共享，主线程另核原生实例与承载。
  bool ReusableRecord(const ClientInstallRecord &,std::uint32_t previous_mask) const noexcept;
  // 快速路径（设计 10.6 节）：两份计划中同一成员记录的数字形状是否一致。
  // 比较字段（路径、定义、元素数、槽位掩码、存在位、阶段）、叶（路径、布局、分量、操作、记录内字段归属）
  // 和引用（记录内叶序号、成员、资源、空位），不比较字节。共享同一描述数据时直接判等。
  bool SameRecordShape(std::uint32_t record,const ClientStateInstallPlan &other,std::uint32_t other_record)const noexcept;
  // 快速路径整批资格：other 是当前已安装的计划。对象表、World 拓扑、实体、玩家、控制组、锚点、格子、
  // 描述符、原生需求、暂存对象逐项相同；非事件记录下标与形状相同，事件记录只在末尾；
  // other 的路径表是本计划路径表的前缀。只读数字计划，不访问原生内存。
  bool SameStructure(const ClientStateInstallPlan &other)const noexcept;
  // 非事件记录数；事件记录不全在末尾时返回 kClientPlanNone。
  std::uint32_t MemberRecordCount()const noexcept;
  std::span<const ClientInstallPath> Paths()const noexcept{return paths_;}
  std::uint32_t NormalizedPathCount()const noexcept{return normalized_path_count_;}
  // 完整(object,规范字段路径)查询；投影重合时保留原计划中最先登记的字段。
  std::uint32_t CanonicalField(std::uint32_t object,std::uint32_t path)const noexcept;
  std::size_t FieldCount()const noexcept{return field_records_.size();}
  std::size_t ValueCount()const noexcept{return value_records_.size();}
  ClientInstallField Field(std::uint32_t index)const noexcept;
  ClientInstallValue Value(std::uint32_t index)const noexcept;
  ClientInstallValue Value(std::uint32_t record,std::uint32_t local)const noexcept;
  std::span<const std::uint32_t> PresentationFields()const noexcept{return presentation_fields_;}
  std::span<const std::uint32_t> PresentationValues()const noexcept{return presentation_values_;}
  // 特殊原生字段实际消费的数字叶，按Values原序登记，不含原生地址。
  std::span<const std::uint32_t> NativeBindingValues()const noexcept{return native_binding_values_;}
  std::span<const std::uint32_t> NativeDisplayValues()const noexcept{return native_display_values_;}
  std::size_t BindingCount()const noexcept{return binding_records_.size();}
  ClientInstallBinding Binding(std::uint32_t index)const noexcept;
  std::optional<ClientInstallBinding> BindingForValue(std::uint32_t record,std::uint32_t local)const noexcept;
  std::span<const ClientStateResourceNeed> Resources()const noexcept {
    return facts_?std::span<const ClientStateResourceNeed>(facts_->resources):std::span<const ClientStateResourceNeed>{};
  }
  std::span<const ClientInstallDescriptor> Descriptors()const noexcept{return descriptors_;}
  std::span<const ClientInstallGrid> Grids()const noexcept{return grids_;}
  std::span<const ClientInstallEntity> Entities()const noexcept{return entities_;}
  std::span<const ClientInstallPlayer> Players()const noexcept{return players_;}
  std::span<const ClientInstallGroup> Groups()const noexcept{return groups_;}
  std::span<const std::uint32_t> Anchors()const noexcept{return anchors_;}
  std::span<const ClientInstallEvent> Events()const noexcept{return events_;}
  std::span<const ClientInstallTransition> Transitions()const noexcept{return transitions_;}
  std::span<const ClientInstallNativeNeed> NativeNeeds()const noexcept{return native_needs_;}
  std::span<const std::uint32_t> StageObjects()const noexcept{return stage_objects_;}
  std::span<const std::uint32_t> StageFields()const noexcept{return stage_fields_;}
  std::span<const std::byte> Bytes(ClientInstallSlice s)const;
  ClientShadowStateRef Effective(BodyStream)const;
  std::shared_ptr<ClientStateMemory> Memory()const noexcept{return bytes_.get_allocator().Memory();}
  bool Describes(const ClientStateCandidateRef &c)const noexcept{return candidate_==c;}
private:
  friend struct ClientInstallBuilder;
  ClientStateCandidateRef candidate_;
  std::array<ClientShadowStateRef,2> streams_;
  // 普通 World 只在安装前借 Body 姿态。发布的 World 不延长当时配对目录的寿命。
  std::array<std::weak_ptr<const ClientShadowState>,2> peer_context_;
  ClientInstallWorld world_;
  ClientStateVector<ClientInstallObject> objects_;
  ClientStateVector<StateMemberEntry> retired_;
  ClientStateVector<ClientInstallRecord> records_;
  ClientStateVector<ClientInstallBlockEnd> block_ends_;
  ClientStateVector<ClientInstallPath> paths_;
  std::uint32_t normalized_path_count_=0;
  bool has_flat_records_=false;
  ClientStateVector<std::shared_ptr<const ClientInstallRecordData>> data_;
  ClientStateVector<std::uint32_t> field_records_,value_records_,binding_records_,private_records_;
  ClientInstallBinding RebaseBinding(std::uint32_t record,ClientInstallBinding)const noexcept;
  // 每个对象独占小段；连续路径直接下标，稀疏路径完整比较，对象身份由独占段确定。
  // 构建计数阶段暂用first/base保存最小/最大路径；封存后base=none表示散列表。
  struct FieldRange {std::uint32_t first=kClientPlanNone,count=0,base=0;};
  struct FieldSlot {std::uint32_t path=0,field=kClientPlanNone;};
  ClientStateVector<FieldRange> field_ranges_;
  ClientStateVector<FieldSlot> field_lookup_;
  // 只给具名显示描述建立紧凑索引，避免再次扫整个 World 的 Sprite 与实体叶。
  ClientStateVector<std::uint32_t> presentation_fields_,presentation_values_;
  ClientStateVector<std::uint32_t> native_binding_values_;
  // Overlay／Procedural三类布局节点，保留原Values顺序及事件；不混入其它显示消费者索引。
  ClientStateVector<std::uint32_t> native_display_values_;
  std::shared_ptr<const ClientTargetFacts> facts_;
  ClientStateVector<ClientInstallDescriptor> descriptors_;
  ClientStateVector<ClientInstallGrid> grids_;
  ClientStateVector<ClientInstallEntity> entities_;
  ClientStateVector<ClientInstallPlayer> players_;
  ClientStateVector<ClientInstallGroup> groups_;
  ClientStateVector<std::uint32_t> anchors_;
  ClientStateVector<ClientInstallEvent> events_;
  ClientStateVector<ClientInstallTransition> transitions_;
  ClientStateVector<ClientInstallNativeNeed> native_needs_;
  ClientStateVector<std::uint32_t> stage_objects_,stage_fields_;
  ClientStateVector<std::byte> bytes_;
};
// 只由后台调用：递归校验、跨域关系、资源闭包、结构动作和有序执行描述一起封存。
// 抛具名ClientShadowError／预算失败；绝不返回空计划或以原生尚未实现为Ready。
std::shared_ptr<const ClientStateInstallPlan> BuildClientStateInstallPlan(const ClientStatePreparedTarget &,const ClientStateTables &);
struct ClientInstallPlanWork;
// 工作状态只借等待位中固定的 target。返回空时继续使用同一输入和原期限。
std::shared_ptr<const ClientStateInstallPlan> BuildClientStateInstallPlanStep(const ClientStatePreparedTarget &,const ClientStateTables &,
    std::shared_ptr<ClientInstallPlanWork> &,std::chrono::steady_clock::time_point,
    const std::array<std::weak_ptr<const ClientStateInstallPlan>,2> &accepted={});
} // namespace cs::state
