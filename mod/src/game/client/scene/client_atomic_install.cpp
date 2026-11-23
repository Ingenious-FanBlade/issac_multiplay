#include "game/client/scene/client_atomic_install.h"
#include "game/client/scene/client_atomic_bindings.h"
#include "game/client/scene/client_atomic_bindings_native.h"
#include "game/client/scene/client_flat_install.h"
#include "game/client/scene/client_lifetime.h"
#include "game/client/scene/client_read_window.h"
#include "game/client/display/client_display_native.h"
#include "game/client/room/client_room_description.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "game/tests/client_entity_e2e.h"
#include "runtime.h"
#include "state/client_memory_resource.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <map>
#include <tuple>

namespace cs::game {
namespace {
namespace S=state;
#include "state_host_fields.inc"
namespace F=host_field;
S::ClientStateNativeStep Ready(bool written,const char *why){return {true,written,false,why};}
S::ClientStateNativeStep Failed(bool written,const char *why){return {false,written,true,why};}
[[noreturn]]void Bad(const char *why){throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,why);}
template<class T>T Number(std::span<const std::byte> bytes){if(bytes.size()!=sizeof(T))Bad("binary_atomic_scalar_width");T v;std::memcpy(&v,bytes.data(),sizeof(v));return v;}
bool Rows(ClientStagedFieldKind k){return k==ClientStagedFieldKind::Rows;}
// 只绑定已核的原位同宽转换；其余转换必须有具名owner，不能仅因宽度相同就复制。
bool RawConversion(std::string_view h){return h=="playercostumemap_layer_id_read"||h=="playeractiveitemdesc_charge_read"||
  h=="playeractiveitemdesc_battery_charge_read"||h=="playeractiveitemdesc_sub_charge_read"||h=="playeractiveitemdesc_partial_charge_read"||
  h=="playeractiveitemdesc_var_data_read"||h=="playertemporaryeffect_cooldown_read"||h=="drawrecordnode_main_animation_frame_read"||
  h=="drawrecordnode_second_animation_frame_read"||h=="entityplayer_inventory_suppressed_read"||
  h=="f_x_mask_read"||h=="f_x_blend_mode_read"||h=="ray_phase_read"||
  h=="level_game_prefix_greed_wave_read"||h=="room_descriptor_spawn_seed_read"||h=="room_hell_backdrop_active_read"||
  h=="room_hell_backdrop_display_offset_read"||h=="room_hell_accumulated_offset_read"||h=="sfx_volume_modifier_read";}
// 具名叶ABI只依赖生成定义；数字分类不缓存原生地址、引用值或安装资格。
enum class LeafHandler : std::uint8_t {Unsupported,Raw,Reference,Expire,CostumeIndex,QueueFlags,Flying,Water};
LeafHandler ClassifyLeafHandler(const S::Field &field) noexcept {
  const std::string_view h=field.handler;
  if(h=="entity_pointer_to_member_index"||h=="entity_reference_to_member"||h=="entity_reference_to_identity"||
     h=="weapon_whip_main_to_member_index"||h=="player_tail_44_to_member"||h=="player__twinPlayer_to_member"||
     h=="read_npc_composite_references")return LeafHandler::Reference;
  if(h=="ExpireFrame")return LeafHandler::Expire;
  if(h=="playercostumemap_index_read")return LeafHandler::CostumeIndex;
  if(h=="queueitemdata_flags_read")return LeafHandler::QueueFlags;
  if(h=="entity_plus_flying_to_value")return LeafHandler::Flying;
  if(h=="entity_plus_water_to_value")return LeafHandler::Water;
  return RawConversion(h)?LeafHandler::Raw:LeafHandler::Unsupported;
}
LeafHandler BoundLeafHandler(std::uint32_t definition) noexcept {
  struct Index {
    memory::Lease charge;
    std::vector<LeafHandler> entries;
    Index():charge(memory::Reserve(memory::Pool::Values,128+S::Fields().size()*sizeof(LeafHandler),"client_leaf_abi_index")),
      entries(S::Fields().size()){
      for(std::size_t i=0;i<entries.size();++i)entries[i]=ClassifyLeafHandler(S::Fields()[i]);
      charge->Resize(sizeof(Index)+128+entries.capacity()*sizeof(LeafHandler));
    }
  };
  // 索引只接收不可变安装计划已验证的definition；分配失败仍使用完整原分类。
  try {static const Index index;return index.entries[definition];}
  catch(...){return ClassifyLeafHandler(S::Fields()[definition]);}
}
// 纯值叶的原生字节与写法。事务 Leaf 与快速写程序共用同一换算，保证两条路径写出相同的 ABI 值。
struct LeafBytes {
  std::span<const std::byte> out;
  ClientAtomicWriteKind kind=ClientAtomicWriteKind::Bytes;
  std::uint32_t mask=0;
};
// 按生成字段定义换算一条叶；scratch 承载换算后的字节。返回失败原因，成功返回 nullptr。
const char *ConvertLeaf(const S::Field &d,std::uint32_t definition,std::span<const std::byte> bytes,std::uint32_t frame,
    std::array<std::byte,8> &scratch,LeafBytes &result) noexcept {
  result={bytes};
  const auto number=[&](auto &v) noexcept {
    if(bytes.size()!=sizeof(v))return false;
    std::memcpy(&v,bytes.data(),sizeof(v));return true;
  };
  if(d.operation==S::Operation::Convert){
    const auto handler=BoundLeafHandler(definition);
    if(d.value_mask&&bytes.size()==4){result.kind=ClientAtomicWriteKind::Masked32;result.mask=d.value_mask;}
    else if(handler==LeafHandler::Expire){
      std::uint32_t value=0;if(!number(value))return "binary_atomic_scalar_width";
      const auto remaining=value-frame;std::memcpy(scratch.data(),&remaining,4);result.out=std::span(scratch).first(4);
    }else if(handler==LeafHandler::CostumeIndex){
      std::uint16_t value=0;if(!number(value))return "binary_atomic_scalar_width";
      const std::int32_t index=value==S::kNullIndex?-1:value;std::memcpy(scratch.data(),&index,4);result.out=std::span(scratch).first(4);
    }else if(handler==LeafHandler::QueueFlags){
      std::uint8_t value=0;if(!number(value))return "binary_atomic_scalar_width";
      const std::uint32_t flag=value?2u:0u;std::memcpy(scratch.data(),&flag,4);result.out=std::span(scratch).first(4);
      result.kind=ClientAtomicWriteKind::Masked32;result.mask=2;
    }else if(handler==LeafHandler::Flying){
      std::int8_t value=0;if(!number(value))return "binary_atomic_scalar_width";
      if(value < -1 || value > 1)return "binary_atomic_flying_optional";
      scratch[0]=std::byte(value==1);scratch[1]=std::byte(value!=-1);result.out=std::span(scratch).first(2);
    }else if(handler==LeafHandler::Water){
      std::uint64_t value=0;if(!number(value))return "binary_atomic_scalar_width";
      if(value>>32>1||((value>>32)==0&&value))return "binary_atomic_water_optional";
      std::memcpy(scratch.data(),&value,5);result.out=std::span(scratch).first(5);
    }else if(handler!=LeafHandler::Raw)return "binary_atomic_conversion_owner_missing";
  }else if(d.operation!=S::Operation::Copy&&d.operation!=S::Operation::Container)return "binary_atomic_leaf_operation";
  return nullptr;
}
}
// 保存首次写入前的根标量。重用同一地址时保留最早字节，最后一批实际撤场时反序归还。
class ClientSceneAtomicInstaller::RootScalars final {
  struct Entry {void *address=nullptr;std::uint32_t offset=0,size=0;};
  using Index=std::pair<std::uintptr_t,std::uint32_t>;
  S::ClientStateVector<Entry> entries_;
  S::ClientStateVector<std::byte> bytes_;
  std::map<Index,bool,std::less<Index>,S::ClientStateAllocator<std::pair<const Index,bool>>> saved_;
public:
  explicit RootScalars(const std::shared_ptr<S::ClientStateMemory> &m):entries_(S::ClientStateAllocator<Entry>(m)),
    bytes_(S::ClientStateAllocator<std::byte>(m)),saved_(S::ClientStateAllocator<std::pair<const Index,bool>>(m)) {}
  bool NeedsSnapshot(const ClientNativeRoots &roots,void *p,std::size_t size) const noexcept {
    return ClientAtomicRootScalar(roots,p,size)&&!saved_.contains({reinterpret_cast<std::uintptr_t>(p),std::uint32_t(size)});
  }
  bool Save(const ClientAtomicWrite &write,ClientAtomicReadRegion &region) noexcept {
    const Index index{reinterpret_cast<std::uintptr_t>(write.destination),std::uint32_t(write.size)};
    try{
      const auto start=std::uint32_t(bytes_.size());
      if(write.size>UINT32_MAX-start)return false;
      bytes_.resize(start+write.size);
      if(!ClientAtomicWriteCapture(write,bytes_.data()+start,region)){bytes_.resize(start);return false;}
      entries_.push_back({write.destination,start,index.second});saved_.emplace(index,true);return true;
    }catch(...){return false;}
  }
  bool Restore(const ClientNativeRoots &roots) noexcept {
    while(!entries_.empty()){
      const auto &entry=entries_.back();
      if(!ClientAtomicRestoreRootScalar(roots,entry.address,bytes_.data()+entry.offset,entry.size))return false;
      entries_.pop_back();
    }
    bytes_.clear();saved_.clear();return true;
  }
};
std::shared_ptr<ClientSceneAtomicInstaller::RootScalars> ClientBinaryRootScalars(const std::shared_ptr<S::ClientStateMemory> &m){
  return std::allocate_shared<ClientSceneAtomicInstaller::RootScalars>(S::ClientStateAllocator<ClientSceneAtomicInstaller::RootScalars>(m),m);
}
// 成员写程序表（设计 10.6 节）。条目来自最近一份发布的 World：事务描述时由 Leaf 登记，
// 静止／被吸收记录沿用上一份条目。表只保存地址与承载句柄的见证，不拥有任何原生对象；
// 使用前由调用者核对象安装实例和承载链。所有下标都是记录内序号，跨计划只要记录同形即可沿用。
class ClientSceneMemberStore final {
public:
  // 一条纯值叶的写法。
  struct Leaf {
    void *destination=nullptr;                    // 静态叶的活对象地址；晚绑定叶为空，执行时重新定位
    std::uint32_t local=S::kClientPlanNone;        // 记录内值序号；Time 叶没有值序号
    std::uint32_t field_local=S::kClientPlanNone;  // 记录内字段序号：晚绑定重定位、Time 取锚点字节
    std::uint32_t element=S::kClientPlanNone;      // 内嵌行元素序号（晚绑定）
    std::uint32_t definition=0;                    // 生成字段定义下标，用于 ABI 换算
    std::uint32_t stride=0;
    std::uint32_t size=0;
    bool late=false,display=false,time=false;
  };
  // 静态叶所在的私有承载：header 槽位和承载句柄。执行前逐个核仍接在活对象上。
  struct Carrier {void *slot=nullptr;ClientStagedField *native=nullptr;std::uint32_t field_local=S::kClientPlanNone;};
  // 一个成员记录的写程序。marks 中先放 value_count 个值标记，再放 field_count 个字段标记；
  // 1 表示纯值（由写程序全量写，字节可变），0 表示必须与上次写入的计划逐字节相等。
  struct Record {
    S::PresentationMember identity;
    std::uint64_t instance=0;
    void *address=nullptr;
    std::uint32_t record=S::kClientPlanNone;       // 在 plan 中的记录下标
    std::uint32_t first_leaf=0,leaf_count=0,first_carrier=0,carrier_count=0,first_mark=0;
    std::uint32_t value_count=0,field_count=0;
    bool display=false;                            // 含显示路径叶（Room 构图后写）
  };
  explicit ClientSceneMemberStore(const std::shared_ptr<S::ClientStateMemory> &m):memory(m),
    records(S::ClientStateAllocator<Record>(m)),leaves(S::ClientStateAllocator<Leaf>(m)),
    carriers(S::ClientStateAllocator<Carrier>(m)),marks(S::ClientStateAllocator<std::uint8_t>(m)) {}
  std::shared_ptr<S::ClientStateMemory> memory;
  // 条目对应的计划（最近一份发布的 World），比较“必须相等”字节时作为上次写入值。
  std::shared_ptr<const ClientSceneInstallPlan> plan;
  S::StateRoute route{};
  std::uint32_t version=0;
  // 每个有原生地址的成员记录都有条目时为真；整批快速帧要求为真。
  bool complete=false;
  // 每次刷新或清空递增；事务在分类时记下，安装时核对未变。
  std::uint64_t generation=0;
  S::ClientStateVector<Record> records;          // 按成员身份排序
  S::ClientStateVector<Leaf> leaves;
  S::ClientStateVector<Carrier> carriers;
  S::ClientStateVector<std::uint8_t> marks;
  void Clear() noexcept {
    plan.reset();route={};version=0;complete=false;++generation;
    records.clear();leaves.clear();carriers.clear();marks.clear();
  }
  bool Usable(const S::StateRoute &r,std::uint32_t base)const noexcept {return plan&&version&&route==r&&version==base;}
  const Record *Find(const S::PresentationMember &id)const noexcept {
    const auto at=std::lower_bound(records.begin(),records.end(),id,[](const auto &e,const auto &key){return e.identity<key;});
    return at!=records.end()&&at->identity==id?&*at:nullptr;
  }
  bool ValuePure(const Record &e,std::uint32_t local)const noexcept {return marks[e.first_mark+local]!=0;}
  bool FieldPure(const Record &e,std::uint32_t local)const noexcept {return marks[e.first_mark+e.value_count+local]!=0;}
  // next 的记录 n 中所有“必须相等”的值和字段，字节与条目所记计划的同一记录相同。调用者已核两记录同形。
  bool SameFixed(const S::ClientStateInstallPlan &next,std::uint32_t n,const Record &e)const noexcept {
    try {
      const auto &old=plan->State();
      const auto &a=old.Records()[e.record];const auto &b=next.Records()[n];
      if(a.value_count!=e.value_count||b.value_count!=e.value_count||a.field_count!=e.field_count||b.field_count!=e.field_count)return false;
      const auto same=[](std::span<const std::byte> x,std::span<const std::byte> y){
        return x.size()==y.size()&&(x.empty()||!std::memcmp(x.data(),y.data(),x.size()));
      };
      for(std::uint32_t local=0;local<e.value_count;++local)if(!ValuePure(e,local)&&
          !same(old.Bytes(old.Value(e.record,local).bytes),next.Bytes(next.Value(n,local).bytes)))return false;
      for(std::uint32_t local=0;local<e.field_count;++local)if(!FieldPure(e,local)&&
          !same(old.Bytes(old.Field(a.first_field+local).bytes),next.Bytes(next.Field(b.first_field+local).bytes)))return false;
      return true;
    }catch(...){return false;}
  }
};
std::shared_ptr<ClientSceneMemberStore> ClientMakeMemberStore(const std::shared_ptr<S::ClientStateMemory> &m){
  return std::allocate_shared<ClientSceneMemberStore>(S::ClientStateAllocator<ClientSceneMemberStore>(m),m);
}
void ClientClearMemberStore(ClientSceneMemberStore &store) noexcept {store.Clear();}
// 场景发布的字段索引。所有下标均是批内或记录内编号，既不拥有旧计划也不保留原生地址。
class ClientSceneFieldIndex final {
public:
  struct Record {
    S::PresentationMember identity;
    std::uint64_t instance=0;
    S::BodyStream stream=S::BodyStream::Value;
    bool has_previous_write=false;
    // 仅 Manager 使用。漂移后的下一次应用重写整根，其他记录仍可按块求差。
    mutable bool rewrite=false;
    std::shared_ptr<ClientFlatNativeState> flat;
    bool flat_full=false;
  };
  struct ReadbackEntry {
    std::uint64_t key;
    std::uint32_t operation;
    std::uint32_t Record()const noexcept{return std::uint32_t(key>>8);}
  };
  // 完整整数键逐项相等才共享执行顺序、读回分组。分组项定位当前安装租约的指令槽。
  struct Order {
    S::ClientStateVector<std::uint64_t> keys;
    S::ClientStateVector<std::uint32_t> execution;
    S::ClientStateVector<ReadbackEntry> readback;
    explicit Order(const std::shared_ptr<S::ClientStateMemory> &m):keys(S::ClientStateAllocator<std::uint64_t>(m)),
      execution(S::ClientStateAllocator<std::uint32_t>(m)),readback(S::ClientStateAllocator<ReadbackEntry>(m)) {}
  };
  S::StateRoute route{};
  std::array<std::uint32_t,2> versions{};
  S::ClientStateVector<Record> records;
  S::ClientStateVector<std::uint32_t> by_identity;
  S::ClientStateVector<std::uint32_t> flat_records;
  std::shared_ptr<const Order> order;
  explicit ClientSceneFieldIndex(const std::shared_ptr<S::ClientStateMemory> &m):
    records(S::ClientStateAllocator<Record>(m)),by_identity(S::ClientStateAllocator<std::uint32_t>(m)),
    flat_records(S::ClientStateAllocator<std::uint32_t>(m)) {}
  const Record *Find(S::BodyStream stream,const S::PresentationMember &identity) const noexcept {
    const auto key=std::tie(stream,identity);
    const auto at=std::lower_bound(by_identity.begin(),by_identity.end(),key,[&](auto index,const auto &wanted){
      const auto &r=records[index];return std::tie(r.stream,r.identity)<wanted;
    });
    if(at==by_identity.end())return nullptr;
    const auto &r=records[*at];
    return std::tie(r.stream,r.identity)==key?&r:nullptr;
  }
};
struct ClientSceneAtomicInstaller::Impl {
  struct Op {
    ClientAtomicWrite write;
    std::uint32_t field=0,value=0,order=0,element=S::kClientPlanNone,staged_element=S::kClientPlanNone,reference=S::kClientPlanNone;
    // 本记录内的引用叶。保留完整成员键所在的位置，挂起项的数量由字段计划限定。
    std::uint32_t reference_value=S::kClientPlanNone;
    std::uint64_t deferred_target_instance=0;
    S::ClientInstallPhase phase=S::ClientInstallPhase::Values;
    unsigned priority=5;
    // 数组起点定位仍用单元素步长；合并纯拷贝后write.extent是整段长度。
    std::size_t element_stride=0;
    bool delegated=false,late=false,display=false,duplicate=false,done=false,restored=false;
    bool borrowed_value=false;
  };
  std::shared_ptr<S::ClientStateMemory> memory;ClientSceneMainPreparation &preparation;
  std::unique_ptr<ClientAtomicInstallBindings> owned_bindings;
  ClientAtomicInstallBindings &bindings;
  std::shared_ptr<RootScalars> root_scalars;
  std::array<ClientSceneFieldIndexRef,2> published_fields;
  std::shared_ptr<ClientSceneFieldIndex> field_index;
  const std::thread::id thread=std::this_thread::get_id();
  S::ClientStateApplyBatchRef batch;std::shared_ptr<const ClientSceneInstallPlan> scene;
  S::ClientStateVector<ClientAtomicObject> objects;S::ClientStateVector<ClientAtomicField> fields;
  S::ClientStateVector<ClientAtomicBindingField> binding_fields;
  S::ClientStateVector<ClientAtomicResource> resources;S::ClientStateVector<Op> operations;
  S::ClientStateVector<std::byte> values;
  S::ClientStateVector<std::uint32_t> resolved_writes,resolved_scratch;
  // 静态地址在Describe之后保持不变；仅保留当前候选的有界索引，不能跨候选借用。
  S::ClientStateVector<std::uint32_t> static_writes;
  S::ClientStateVector<std::uint32_t> field_attachments;
  // 封存本批真正有附件或晚绑定的操作下标，提交段不再扫描全部标量叶。
  S::ClientStateVector<std::uint32_t> attachment_operations;
  std::array<S::ClientStateVector<std::uint32_t>,2> late_operations;
  // 只在整批描述期间保留字段分类和操作计数。
  struct Description {
    S::ClientStateVector<std::uint8_t> delegated;
    std::uint64_t late_count=0,delegated_count=0;
    Description(const std::shared_ptr<S::ClientStateMemory> &m,std::size_t fields):
      delegated(fields,0,S::ClientStateAllocator<std::uint8_t>(m)) {}
  };
  // 描述完成后，晚绑定到场景里的访问按当前 owner 核定，不能沿用暂存阶段的私有借用。
  bool described=false;
  // costume独立分组：两流共享容器随最新批次退休，旧流只检查仍属于自己的普通字段。
  // 切换写入器时沿用Manager游标，避免高频收包让尾部饥饿。
  // 完整保留 costume / stream / record / block 的字典序，不截断记录号。
  // 计划不可变；缓存最后一个 object 对同一 canonical 字段的归属，不含原生地址。
  struct FieldOwner {std::uint32_t object=S::kClientPlanNone,field=S::kClientPlanNone;};
  S::ClientStateVector<FieldOwner> field_owners;
  // 每条数字路径的静态归属只解析一次；不缓存原生地址或跨窗口有效性。
  struct PathFacts {
    std::uint8_t root_block=0,depth=0;
    bool late=false,display=false,costume=false;
  };
  S::ClientStateVector<PathFacts> path_facts;
  // 按记录下标的静止标记（设计 10.5 节）：同一实例、上次已写、本批相对应用基线无任何块变化。
  // 静止记录只生成必须逐批维持的指令，其余纯值叶和可省的具名字段整体不进入描述与写段。
  S::ClientStateVector<std::uint8_t> quiet;
  // 静止记录每隔这么多份 World 版本轮到一次照常描述，供漂移扫描与具名写入器自愈。
  static constexpr std::uint32_t kQuietRefreshPeriod=32;
  // 仅显式纯读窗口复用父链。退出窗口即清空，写入／回调后的地址重新解析。
  struct InstanceAddress {void *address=nullptr;std::size_t extent=0;std::uint32_t object=S::kClientPlanNone;};
  S::ClientStateVector<InstanceAddress> description_addresses;
  // 仅登记本纯读段实际访问的父路径；退出时逐项清空，不反复初始化全部叶路径。
  S::ClientStateVector<std::uint32_t> description_touched;
  // 写程序表与本批吸收（设计 10.6 节）。store 跨安装器共享；其余只属于本批描述。
  std::shared_ptr<ClientSceneMemberStore> store;
  bool absorb=false;
  // 分类时的写程序表代次；安装时不同就拒绝本批（条目下标已失效）。
  std::uint64_t store_generation=0;
  struct CaptureLeaf {std::uint32_t record=0;ClientSceneMemberStore::Leaf leaf;};
  struct CaptureCarrier {std::uint32_t record=0;ClientSceneMemberStore::Carrier carrier;};
  S::ClientStateVector<CaptureLeaf> capture_leaves;
  S::ClientStateVector<CaptureCarrier> capture_carriers;
  // 每条记录：0 未登记，1 已登记，2 登记失败（本批不为它建条目）。
  S::ClientStateVector<std::uint8_t> capture_state;
  // 每个计划字段／值：1 表示纯（写程序全量写，字节可变），0 表示必须相等。
  S::ClientStateVector<std::uint8_t> field_pure,value_pure;
  // 被吸收记录：(本计划记录号, 写程序表条目下标)。
  S::ClientStateVector<std::pair<std::uint32_t,std::uint32_t>> absorbed;
  // 整批快速帧换过计划：纯值由每帧全量写覆盖，漂移扫描只看委托字段。
  bool fast_swapped=false;
  // 最近一张已写入的快速票据：提供当前计划的发布目标与决议；根与承载仍属于 batch。
  S::ClientStateApplyBatchRef fast_ticket;
  ClientNativeReadWindow *description_reads=nullptr;
  ClientNativeRoots roots{};std::uint64_t manager_token=0;
  bool staged=false,lease=false,install_entered=false,installed=false,display_installed=false,publish_entered=false,published=false,written=false;
  bool deferred_display=false;
  bool invalid=false,quiesced=false,retired=false,lease_released=false,uncertain=false,notified=false;
  bool final_scene=false;
  std::uint64_t read_checks=0,read_bytes=0;
  std::uint64_t pure_write_bytes=0,skipped_world_bytes=0;
  std::uint32_t flat_drift_cursor=0;
  const char *reason="binary_atomic_idle";
  std::array<char,192> failure{};
  Impl(std::shared_ptr<S::ClientStateMemory> m,ClientSceneMainPreparation &p,ClientAtomicInstallBindings *b,
      std::shared_ptr<RootScalars> scalars={},const std::array<ClientSceneFieldIndexRef,2> &published={},
      std::shared_ptr<ClientSceneMemberStore> member_store={},bool absorbing=false):
    memory(std::move(m)),preparation(p),owned_bindings(b?nullptr:ClientMakeAtomicInstallBindings(memory,p)),
    bindings(b?*b:*owned_bindings),root_scalars(std::move(scalars)),
    published_fields(published),
    objects(S::ClientStateAllocator<ClientAtomicObject>(memory)),fields(S::ClientStateAllocator<ClientAtomicField>(memory)),
    binding_fields(S::ClientStateAllocator<ClientAtomicBindingField>(memory)),
    resources(S::ClientStateAllocator<ClientAtomicResource>(memory)),operations(S::ClientStateAllocator<Op>(memory)),
    values(S::ClientStateAllocator<std::byte>(memory)),
    resolved_writes(S::ClientStateAllocator<std::uint32_t>(memory,memory::Pool::Workspace)),
    resolved_scratch(S::ClientStateAllocator<std::uint32_t>(memory,memory::Pool::Workspace)),
    static_writes(S::ClientStateAllocator<std::uint32_t>(memory,memory::Pool::Workspace)),
    field_attachments(S::ClientStateAllocator<std::uint32_t>(memory)),
    attachment_operations(S::ClientStateAllocator<std::uint32_t>(memory)),
    late_operations{S::ClientStateVector<std::uint32_t>(S::ClientStateAllocator<std::uint32_t>(memory)),
      S::ClientStateVector<std::uint32_t>(S::ClientStateAllocator<std::uint32_t>(memory))},
    field_owners(S::ClientStateAllocator<FieldOwner>(memory)),
    path_facts(S::ClientStateAllocator<PathFacts>(memory)),
    quiet(S::ClientStateAllocator<std::uint8_t>(memory)),
    description_addresses(S::ClientStateAllocator<InstanceAddress>(memory)),
    description_touched(S::ClientStateAllocator<std::uint32_t>(memory)),
    store(std::move(member_store)),absorb(absorbing&&store!=nullptr),
    capture_leaves(S::ClientStateAllocator<CaptureLeaf>(memory)),capture_carriers(S::ClientStateAllocator<CaptureCarrier>(memory)),
    capture_state(S::ClientStateAllocator<std::uint8_t>(memory)),field_pure(S::ClientStateAllocator<std::uint8_t>(memory)),
    value_pure(S::ClientStateAllocator<std::uint8_t>(memory)),
    absorbed(S::ClientStateAllocator<std::pair<std::uint32_t,std::uint32_t>>(memory)){}
  bool Thread()const noexcept{return thread==std::this_thread::get_id();}
  void ReleaseInstallScratch() noexcept {
    // 发布后不再重进本批写段。保留漂移索引、字段附件和退休责任，立即归还排序和纯值撤销区。
    const auto release=[](auto &buffer){
      std::remove_reference_t<decltype(buffer)> empty(buffer.get_allocator());
      buffer.swap(empty);
    };
    release(resolved_writes);release(resolved_scratch);release(static_writes);
    release(attachment_operations);release(binding_fields);
    for(auto &indices:late_operations)release(indices);
    release(capture_leaves);release(capture_carriers);release(capture_state);release(field_pure);release(value_pure);release(absorbed);
  }
  Op &Ordered(std::size_t position) noexcept {return operations[field_index->order->execution[position]];}
  // 各阶段互不嵌套，只聚合当前调用栈的真实页查询；失败前缀同样计入成本。
  struct PageMetrics {
    Impl &owner;ClientNativePageCounters before=client_native_page_counters;
    explicit PageMetrics(Impl &s) noexcept:owner(s){}
    ~PageMetrics(){
      auto &metrics=owner.memory->ApplyMetrics();
      const auto streams=S::StateTargetStreams(owner.scene->State().Target());
      const auto &after=client_native_page_counters;
      metrics.Volume(streams,S::ClientApplyVolume::ReadPageQueries,after.read_queries-before.read_queries);
      metrics.Volume(streams,S::ClientApplyVolume::ReadPageFull,after.read_full-before.read_full);
      metrics.Volume(streams,S::ClientApplyVolume::WritePageQueries,after.write_queries-before.write_queries);
      metrics.Volume(streams,S::ClientApplyVolume::WritePageFull,after.write_full-before.write_full);
    }
  };
  // Describe已预留全部路径容量；后续纯读批次只借用该容量，不在安装或修复中分配。
  // 此作用域不得包含原生setter、Init、显示推进或修复写入。
  struct PurePathWindow {
    Impl &owner;ClientNativeReadWindow reads;bool ready=false;
    explicit PurePathWindow(Impl &s) noexcept:owner(s){
      const auto count=s.scene->State().Paths().size();
      if(s.description_reads||!s.description_touched.empty()||s.description_addresses.size()!=count||
          s.description_touched.capacity()<count)return;
      s.description_reads=&reads;ready=true;
    }
    ~PurePathWindow(){if(ready)owner.ClearDescriptionWindow();}
  };
  // 地址见证仍在每个窗口结束时实际清零，绝不跨写入、回调或Manager边界复用。
  void ClearDescriptionWindow() noexcept {
    description_reads=nullptr;
    for(const auto path:description_touched)description_addresses[path]={};
    description_touched.clear();
  }
  struct ReadMetrics {
    Impl &owner;std::uint8_t streams;std::uint64_t checks,bytes;
    S::ClientApplyTimer time;
    ReadMetrics(Impl &s,std::uint8_t mask):owner(s),streams(mask),checks(s.read_checks),bytes(s.read_bytes),
      time(s.memory->ApplyMetrics(),mask,S::ClientApplyStage::Readback) {}
    ~ReadMetrics(){
      auto &metrics=owner.memory->ApplyMetrics();
      metrics.Volume(streams,S::ClientApplyVolume::Checks,owner.read_checks-checks);
      metrics.Volume(streams,S::ClientApplyVolume::ReadBytes,owner.read_bytes-bytes);
    }
  };
  // 首因使用生成字段和完整成员身份，可跨移除旧安装器后继续定位。
  void FieldIssue(const Op &op,const char *phase,const char *reason)const noexcept {
    const auto &p=scene->State();const auto &f=p.Field(op.field);const auto &r=p.Records()[f.record];
    const auto &field=S::Fields()[f.definition];
    S::ClientApplyMetrics::Issue issue;issue.route=batch->ticket.route.route;issue.ticket=batch->ticket.serial;
    issue.target_version=r.context.target_version;issue.resource_epoch=r.context.resource_epoch;
    issue.layout=field.layout;issue.field=field.index;
    if(r.object<p.Objects().size()){
      const auto &id=p.Objects()[r.object].member.identity;issue.member=id.id;issue.instance=id.use;
    }
    auto path=f.path;
    while(p.Paths()[path].parent!=S::kClientPlanNone)path=p.Paths()[path].parent;
    const auto &root=p.Paths()[path].step;
    if(const auto *definition=S::FindField(root.layout,root.field))issue.block=definition->block;
    issue.expected=reinterpret_cast<std::uintptr_t>(op.write.destination);
    memory->ApplyMetrics().FirstIssue(std::uint8_t(1u<<unsigned(r.stream)),issue,phase,reason);
  }
  ClientAtomicInputs Inputs(){
    return {*scene,objects,fields,resources,this,[](void *owner,std::uint32_t record,std::uint32_t path,void *&address,std::size_t &extent){
      return static_cast<Impl *>(owner)->Instance(record,path,address,extent);
    },description_reads,binding_fields,[](void *owner,std::uint32_t object,ClientMemberFailure failure,const char *reason) noexcept {
      return static_cast<Impl *>(owner)->RejectObject(object,failure,reason);
    }};
  }
  std::uint32_t ObjectOf(const Op &op) const noexcept {
    const auto &plan=scene->State();return plan.Records()[plan.Field(op.field).record].object;
  }
  bool Rejected(std::uint32_t object) const noexcept {
    return object<objects.size()&&objects[object].failure!=ClientMemberFailure::None;
  }
  bool RejectObject(std::uint32_t n,ClientMemberFailure failure,const char *why) noexcept {
    using Domain=S::ClientInstallDomain;
    if(n>=objects.size()||!Current(*batch))return false;
    const auto &source=scene->State().Objects()[n];
    if(source.domain!=Domain::Entity&&source.domain!=Domain::Player&&source.domain!=Domain::Grid)return false;
    auto &object=objects[n];
    if(Rejected(n))return true;
    object.failure=failure;
    S::ClientApplyMetrics::Issue issue;
    issue.route=batch->ticket.route.route;issue.ticket=batch->ticket.serial;issue.member=source.member.identity.id;
    issue.layout=source.member.identity.layout;issue.instance=object.instance;
    memory->ApplyMetrics().FirstIssue(S::StateTargetStreams(scene->State().Target()),issue,"object_apply",why);
    Trace("client_error","event=binary_object_rejected member=%llu use=%llu shape=%llu stream=%u kind=%u reason=%s",
      source.member.identity.id,source.member.identity.use,source.member.identity.shape,
      unsigned(S::StateTargetStreams(scene->State().Target())),unsigned(failure),why);
    return true;
  }
  bool RejectField(const Op &op,const char *why) noexcept {
    if(!RejectObject(ObjectOf(op),ClientMemberFailure::Native,why))return false;
    FieldIssue(op,"object_field",why);return true;
  }
  bool Current(const S::ClientStateApplyBatch &b)const noexcept {
    return Thread()&&batch.get()==&b&&!invalid&&!uncertain&&preparation.Current(b)&&bindings.Current(b);
  }
  S::ClientStateNativeStep Fail(const char *why) noexcept {
    if(!why)why="binary_atomic_unspecified_failure";const auto size=(std::min)(std::strlen(why),failure.size()-1);
    std::memmove(failure.data(),why,size);failure[size]=0;reason=failure.data();invalid=true;
    if(written&&batch&&!notified){notified=true;bindings.Invalidate(*batch,reason);}return Failed(written,reason);
  }
  std::uint32_t FieldAt(std::uint32_t object,std::uint32_t path) {
    const auto key=scene->State().Paths()[path].field_path;if(key==S::kClientPlanNone)return S::kClientPlanNone;
    auto &cached=field_owners[key];
    if(cached.object!=object){
      cached={object,scene->State().CanonicalField(object,key)};
    }
    return cached.field;
  }
  void *FieldElement(ClientStagedField *owner,std::uint32_t element) const noexcept {
    // Describe 已取得全批只读见证；安装、回读和退休阶段按当前承载重新核元素。
    return described?ClientStagedFieldElement(owner,element):ClientInspectedFieldElement(owner,element);
  }
  // 父链逐层定位；动态容器元素取候选私有承载，固定内嵌对象保留其真实地址。
  bool Instance(std::uint32_t record,std::uint32_t path,void *&address,std::size_t &extent,unsigned depth=0) {
    const auto object=scene->State().Records()[record].object;
    if(description_reads&&path<description_addresses.size()){
      auto &cached=description_addresses[path];
      if(cached.address&&cached.object==object){address=cached.address;extent=cached.extent;return true;}
      if(!ResolveInstance(record,path,address,extent,depth))return false;
      if(!cached.address)description_touched.push_back(path);
      cached={address,extent,object};return true;
    }
    return ResolveInstance(record,path,address,extent,depth);
  }
  bool ResolveInstance(std::uint32_t record,std::uint32_t path,void *&address,std::size_t &extent,unsigned depth) {
    const auto &plan=scene->State();const auto object=plan.Records()[record].object;
    if(object>=objects.size()||depth>=S::kCodecMaxDepth)return false;
    if(path==S::kClientPlanNone){address=objects[object].address;const auto *l=S::FindLayout(plan.Objects()[object].member.identity.layout);extent=l?l->native_size:0;return address&&extent;}
    const auto &p=plan.Paths()[path];const auto *d=S::FindField(p.step.layout,p.step.field);if(!d||(!d->nested_layout&&!p.step.choice_layout))return false;
    const auto i=FieldAt(object,path);
    if(i!=S::kClientPlanNone) {auto &f=fields[i];
      if(f.native&&f.Input().kind!=ClientStagedFieldKind::Group&&!Rows(f.Input().kind)) {
        const auto element=p.step.element==S::kNullIndex?0u:p.step.element;address=FieldElement(f.native,element);
        const auto *l=S::FindLayout(p.step.choice_layout?p.step.choice_layout:d->nested_layout);extent=l?l->native_size:0;return address&&extent;
      }
    }
    void *parent=nullptr;std::size_t parent_size=0;if(!Instance(record,p.parent,parent,parent_size,depth+1))return false;
    if(bindings.LatePath(*d)||d->source!=S::FieldSource::Object||S::FindLayout(p.step.layout)->protocol)
      return bindings.Resolve(Inputs(),record,path,parent,address,extent)&&address&&extent;
    const auto *l=S::FindLayout(p.step.choice_layout?p.step.choice_layout:d->nested_layout);if(!l)return false;
    const auto element=p.step.element==S::kNullIndex?0u:p.step.element;
    const auto offset=std::uint64_t(d->native_offset)+std::uint64_t(element)*d->native_stride;
    if(offset>parent_size||l->native_size>parent_size-offset)return false;
    address=static_cast<std::byte *>(parent)+std::size_t(offset);extent=l->native_size;return true;
  }
  bool Late(std::uint32_t path)const noexcept {
    return path!=S::kClientPlanNone&&path_facts[path].late;
  }
  bool LiveParent(std::uint32_t object,std::uint32_t path) {
    const auto &plan=scene->State();
    for(;path!=S::kClientPlanNone;path=plan.Paths()[path].parent){
      const auto owner=FieldAt(object,path);
      if(owner==S::kClientPlanNone)continue;
      const auto &input=fields[owner].Input();
      if(input.resize_live_sequence||input.create_live_owner)return true;
    }
    return false;
  }
  // 构图会重置Backdrop Sprite，地狱初始化也会覆盖活动标量，因此这些路径最后安装。
  bool DisplayField(std::uint32_t path)const noexcept {
    return path!=S::kClientPlanNone&&path_facts[path].display;
  }
  bool Destination(std::uint32_t field,void *&address,std::size_t &extent) {
    const auto &plan=scene->State();const auto &f=plan.Field(field);const auto &d=S::Fields()[f.definition];const auto &p=plan.Paths()[f.path];
    const auto kind=fields[field].Input().kind;const bool records=kind==ClientStagedFieldKind::Records;
    if((d.source!=S::FieldSource::Object&&!records)||S::FindLayout(d.layout)->protocol)return false;
    const auto size=records?12u:kind==ClientStagedFieldKind::Seeds?8u:d.native_size;
    const auto offset=records?0u:d.native_offset;
    void *parent=nullptr;std::size_t parent_size=0;if(!Instance(f.record,p.parent,parent,parent_size)||offset>parent_size||size>parent_size-offset)return false;
    address=static_cast<std::byte *>(parent)+offset;extent=size;return address&&extent;
  }
  // 候选指令在最终数组中原位构造；完成前不调用Init或写原生内存。
  // 任何后续校验失败仍使整个候选失败；暂存预算与压缩后的262144条预算保持。
  Op &NewOperation() {
    if(operations.size()>=scene->State().FieldCount()+scene->State().ValueCount())Bad("binary_atomic_operation_budget");
    const auto order=std::uint32_t(operations.size());
    auto &op=operations.emplace_back();op.order=order;return op;
  }
  void Add(Op &op,std::span<const std::byte> bytes={},bool borrow=false) {
    if(!op.delegated){
      if(!op.write.size||op.write.size>op.write.extent)Bad("binary_atomic_destination_range");
      if(!op.late){
      if(!op.write.destination)Bad("binary_atomic_destination_range");
      const auto begin=reinterpret_cast<std::uintptr_t>(op.write.destination);if(op.write.size>UINTPTR_MAX-begin)Bad("binary_atomic_address_overflow");
      }
      if(!borrow&&bytes.size()>UINT32_MAX-values.size())Bad("binary_atomic_journal_budget");
      // 原样字段直接借引用表中的字节；只有 ABI 转换需要独立存储。
      op.borrowed_value=borrow;
      if(borrow)op.write.value=bytes.data();
      else {op.value=std::uint32_t(values.size());values.insert(values.end(),bytes.begin(),bytes.end());}
    }
  }
  // 先排序紧凑的地址索引，再核所有原始单元素区间；此时尚未合并数组、写世界或分配撤销日志。
  // 同地址保持原Add顺序，重复投影保留首项。部分重叠或不同目标仍在任何Init之前拒绝。
  std::uint64_t IndexWrites() {
    resolved_writes.clear();resolved_writes.reserve(operations.size());resolved_scratch.reserve(operations.size());
    for(std::uint32_t i=0;i<operations.size();++i){
      auto &op=operations[i];
      // 所有叶已收集完成，转换字节缓冲此后不再扩容；排序和压缩不改变其归属。
      if(Pure(op)&&!op.borrowed_value)op.write.value=values.data()+op.value;
      if(!op.delegated&&!op.late)resolved_writes.push_back(i);
    }
#ifdef ISAACCS_E2E
    // 仅改候选计划中的地址／掩码，不写原生对象；下一段必须在Init前拒绝重叠与目标冲突。
    const bool overlap=ClientEntityTestFault("binary-write-overlap");
    const bool conflict=ClientEntityTestFault("binary-write-conflict");
    if(overlap||conflict){
      Op *first=nullptr,*second=nullptr;
      for(auto n:resolved_writes)if(Pure(operations[n])&&operations[n].write.size>=2){
        if(!first)first=&operations[n];else {second=&operations[n];break;}
      }
      if(!second)Bad("binary_e2e_write_candidates_missing");
      second->write.destination=static_cast<std::byte *>(first->write.destination)+(overlap?1:0);
      second->write.size=overlap?1:first->write.size;
      second->write.mask=first->write.mask^1u;
    }
#endif
    SortWriteAddresses();
    std::uint32_t previous=S::kClientPlanNone;
    for(auto n:resolved_writes){
      auto &op=operations[n];const auto begin=reinterpret_cast<std::uintptr_t>(op.write.destination);
      if(previous!=S::kClientPlanNone){
        auto &old=operations[previous];const auto other=reinterpret_cast<std::uintptr_t>(old.write.destination);
        if(begin<other+old.write.size){
          bool same=begin==other&&op.write.size==old.write.size&&op.write.kind==old.write.kind&&op.write.mask==old.write.mask&&
            op.write.reference==old.write.reference&&op.write.field==old.write.field;
          if(same&&Pure(op)){
            const auto *next=op.borrowed_value?op.write.value:values.data()+op.value;
            const auto *prior=old.borrowed_value?old.write.value:values.data()+old.value;
            same=std::memcmp(next,prior,op.write.size)==0;
          }
          if(!same)Bad("binary_atomic_projection_write_conflict");
          op.duplicate=true;continue;
        }
      }
      previous=n;
    }
    // 复用已核冲突的地址顺序执行真实页／附件预检，避免按执行顺序随机探测相同区域。
    // 尚未压缩的每个区间均独立检查；后续只合并严格连续的纯字节，不扩大已核地址集合。
    // 本段没有Init／原生写入；晚绑定地址仍在原来的安装阶段重新解析和检查。
    const auto ranges_begin=S::ClientApplyMetrics::Now();
    ClientAtomicReadRegion read_region;std::size_t checked=0;
    for(auto n:resolved_writes){
      if(checked++%128==0&&!preparation.StagedCurrent(*batch))Bad("binary_atomic_description_revoked");
      const auto &op=operations[n];if(op.duplicate)continue;
      if(!ClientAtomicWritePreflight(op.write,read_region)&&!RejectField(op,"binary_atomic_native_preflight"))
        Bad("binary_atomic_native_preflight");
    }
    const auto ranges_time=S::ClientApplyMetrics::Now()-ranges_begin;
    memory->ApplyMetrics().Mark(S::StateTargetStreams(scene->State().Target()),S::ClientApplyStage::AtomicRanges,ranges_time);
    // 按原插入顺序压缩，再合并同字段的连续元素。重复投影不占日志，也不改变回调顺序。
    // 原始序号到压缩后序号的映射：重复或合并掉的项不再作为独立地址区间。
    resolved_scratch.assign(operations.size(),S::kClientPlanNone);
    std::size_t kept=0;
    for(std::size_t i=0;i<operations.size();++i){
      if(kept>=262144)Bad("binary_atomic_operation_budget");
      auto &op=operations[i];if(op.duplicate)continue;
      if(!op.delegated&&kept&&!op.late&&op.borrowed_value&&op.write.kind==ClientAtomicWriteKind::Bytes&&
          (op.element!=S::kClientPlanNone||op.staged_element!=S::kClientPlanNone)){
        auto &old=operations[kept-1];
        if(!old.delegated&&!old.late&&old.borrowed_value&&old.write.kind==ClientAtomicWriteKind::Bytes&&
            old.field==op.field&&old.phase==op.phase&&old.element_stride==op.element_stride&&
            reinterpret_cast<std::uintptr_t>(old.write.destination)+old.write.size==reinterpret_cast<std::uintptr_t>(op.write.destination)&&
            old.write.value+old.write.size==op.write.value){
          old.write.size+=op.write.size;old.write.extent=old.write.size;continue;
        }
      }
      op.order=std::uint32_t(kept);
      // 写入位置只向前移动，不覆盖尚未访问的指令；没有空洞时不复制自身。
      if(kept!=i)operations[kept]=op;
      resolved_scratch[i]=std::uint32_t(kept);++kept;
    }
    static_writes.reserve(resolved_writes.size());
    for(auto n:resolved_writes)if(resolved_scratch[n]!=S::kClientPlanNone)static_writes.push_back(resolved_scratch[n]);
    operations.resize(kept);resolved_writes.clear();resolved_scratch.clear();
    return ranges_time;
  }
  // 稳定地址基数排序只搬运整数索引；两个有界工作区与操作数组一起计入Values池。
  void SortWriteAddresses() {
    if(resolved_writes.empty())return;
    const auto address=[&](std::uint32_t n){return reinterpret_cast<std::uintptr_t>(operations[n].write.destination);};
    std::uintptr_t varying=0;const auto first=address(resolved_writes.front());
    for(auto n:resolved_writes)varying|=address(n)^first;
    if(resolved_scratch.capacity()<resolved_writes.size())Bad("binary_atomic_resolved_capacity");
    resolved_scratch.resize(resolved_writes.size());
    for(unsigned shift=0;shift<sizeof(std::uintptr_t)*8;shift+=8){
      if(!((varying>>shift)&255))continue;
      std::array<std::size_t,256> offsets{};
      for(auto n:resolved_writes)++offsets[(address(n)>>shift)&255];
      std::size_t next=0;for(auto &offset:offsets){const auto count=offset;offset=next;next+=count;}
      for(auto n:resolved_writes)resolved_scratch[offsets[(address(n)>>shift)&255]++]=n;
      resolved_writes.swap(resolved_scratch);
    }
    resolved_scratch.clear();
  }
  void Leaf(std::uint32_t field,const S::ClientInstallValue &v,std::uint32_t local) {
    const auto &plan=scene->State();const auto &f=plan.Field(field);const auto &d=S::Fields()[f.definition];const auto bytes=plan.Bytes(v.bytes);
    // 滚屏偏移由显示 owner 在构造相机时初始化。后续滚屏随本地相机推进，包括开启移动预测时。
    // 快速路径允许初始化输入变化，但不登记原生写入或漂移检查指令。
    if(d.layout==F::Room::kLayout&&d.index==F::Room::render_scroll){
      if(store){field_pure[field]=1;value_pure[plan.Records()[v.record].first_value+local]=1;}
      return;
    }
    const auto parent=plan.Paths()[v.path].parent;
    if(parent!=S::kClientPlanNone){const auto i=v.parent_field;
      if(i!=S::kClientPlanNone){const auto kind=fields[i].Input().kind;
        if((kind==ClientStagedFieldKind::Map&&d.layout==177)||(kind==ClientStagedFieldKind::Innate&&d.layout==178)||
           (kind==ClientStagedFieldKind::Seeds&&d.layout==238))return;}}
    // 调整长度或换原生 holder 后，后代地址才可确定。
    bool late=Late(parent)||LiveParent(plan.Records()[f.record].object,parent);
    void *address=nullptr;std::size_t extent=0;
    std::uint32_t inline_element=S::kClientPlanNone,staged_element=S::kClientPlanNone;
    const auto &staged_field=fields[field];
    if(d.operation==S::Operation::Container) {
      if(staged_field.staged){const auto kind=staged_field.Input().kind;
        // 文本／键／聚合已由StageFields从同一叶构造，键不可在有序容器中再次原位修改。
        if(kind==ClientStagedFieldKind::String||kind==ClientStagedFieldKind::CString||kind==ClientStagedFieldKind::Blocked||
           kind==ClientStagedFieldKind::Map||kind==ClientStagedFieldKind::Innate||kind==ClientStagedFieldKind::Seeds)return;
        const auto element=plan.Paths()[v.path].step.element;if(element==S::kNullIndex)Bad("binary_atomic_scalar_element");
        if(Rows(kind)){
          void *base=nullptr;std::size_t capacity=d.native_size;
          if((!late&&!Destination(field,base,capacity))||std::uint64_t(element+1)*staged_field.Input().stride>capacity)Bad("binary_atomic_inline_element_range");
          inline_element=element;
          if(!late)address=static_cast<std::byte *>(base)+std::size_t(element)*staged_field.Input().stride;
        }else {
          staged_element=element;
          const auto &input=staged_field.Input();
          const bool live=input.resize_live_sequence||input.create_live_owner;
          late|=live;
          if(!live)address=FieldElement(staged_field.native,element);
        }
        extent=staged_field.Input().stride;
      }
    }else if(late)extent=d.native_size;
    else if(!Destination(field,address,extent))Bad("binary_atomic_field_address_owner");
    if((!address&&!late)||!extent)Bad("binary_atomic_scalar_storage");
    auto &op=NewOperation();op.field=field;op.phase=f.phase;op.write.destination=address;op.write.extent=extent;
    op.late=late&&!address;op.element=inline_element;op.staged_element=staged_element;op.element_stride=extent;
    // 同批全部路径已在 Describe 的 PathFacts 阶段核过生成字段格式；
    // 叶只使用已验证定义与实际字节，不再为每个元素复制一份未消费的格式结构。
    if(v.operation==S::Operation::Reference||v.operation==S::Operation::Resource) {
      const auto link=plan.BindingForValue(v.record,local);
      if(!link||link->is_resource)Bad("binary_atomic_resource_owner_missing");
      if(BoundLeafHandler(f.definition)!=LeafHandler::Reference)Bad("binary_atomic_reference_abi_owner");
      // 完整目标保留待挂边的身份。目标暂缺时先清空原生槽，后续帧按同一身份再解析。
      const bool bound=!link->is_null&&link->object<objects.size()&&objects[link->object].address;
      op.write.kind=ClientAtomicWriteKind::EntityReference;op.write.size=sizeof(void *);op.write.reference=bound?objects[link->object].address:nullptr;
      op.reference=bound?link->object:S::kClientPlanNone;
      if(!link->is_null)op.reference_value=local;
      op.phase=S::ClientInstallPhase::Relations;Add(op);return;
    }
    std::array<std::byte,8> converted;LeafBytes leaf;
    if(const auto *why=ConvertLeaf(d,f.definition,bytes,plan.Records()[f.record].context.game_frame,converted,leaf))Bad(why);
    const auto output=leaf.out;
    if(leaf.kind==ClientAtomicWriteKind::Masked32){op.write.kind=leaf.kind;op.write.mask=leaf.mask;}
    if(output.size()!=extent)Bad("binary_atomic_native_value_width");op.write.size=output.size();Add(op,output,output.data()==bytes.data());
    // 只有不在动态容器元素内的纯值叶进入写程序（设计 10.6 节失败方式 1）。
    if(op.staged_element==S::kClientPlanNone)Capture(field,v,local,op);
  }
  // 登记一条纯值叶到本批写程序；静态叶另记其所在私有承载链。
  void Capture(std::uint32_t field,const S::ClientInstallValue &v,std::uint32_t local,const Op &op) {
    if(!store)return;
    const auto &plan=scene->State();const auto record=v.record;const auto &r=plan.Records()[record];
    if(r.stream!=S::BodyStream::World||r.event!=S::kClientPlanNone||capture_state[record]==2||Quiet(record))return;
    ClientSceneMemberStore::Leaf leaf;
    leaf.local=local;leaf.field_local=field-r.first_field;leaf.definition=plan.Field(field).definition;
    leaf.size=std::uint32_t(op.write.size);leaf.late=op.late;leaf.destination=op.late?nullptr:op.write.destination;
    leaf.element=op.element;leaf.stride=std::uint32_t(op.element_stride);leaf.display=DisplayField(plan.Field(field).path);
    if(!leaf.late&&!CaptureCarriers(record,plan.Paths()[v.path].parent))return;
    capture_leaves.push_back({record,leaf});
    value_pure[r.first_value+local]=1;
    capture_state[record]=1;
  }
  // 静态叶父链上的私有承载（非内嵌 Group／Rows 的暂存字段）登记 header 槽位，执行写程序前逐个核仍接在活对象上。
  bool CaptureCarriers(std::uint32_t record,std::uint32_t path) {
    const auto &plan=scene->State();const auto object=plan.Records()[record].object;
    for(;path!=S::kClientPlanNone;path=plan.Paths()[path].parent){
      const auto owner=FieldAt(object,path);
      if(owner==S::kClientPlanNone)continue;
      const auto &field=fields[owner];const auto kind=field.Input().kind;
      if(!field.native||kind==ClientStagedFieldKind::Group||Rows(kind))continue;
      void *slot=nullptr;std::size_t extent=0;
      if(!Destination(owner,slot,extent)||!slot){capture_state[record]=2;return false;}
      capture_carriers.push_back({record,{slot,field.native,owner-plan.Records()[record].first_field}});
    }
    return true;
  }
  // 内嵌叶封存相对偏移；动态叶只封存形状，写前重新核 ANM2 图层或 Plus holder／类型。
  // 只有同一个原生实例曾发布该记录，下一批才能省略不变块。
  void DescribeLeaves(const Description &work) {
    const auto &plan=scene->State();
    std::size_t visited=0;
    for(std::uint32_t n=0;n<plan.Records().size();++n){
      const auto &r=plan.Records()[n];
      if(r.event!=S::kClientPlanNone||r.object>=objects.size()||!objects[r.object].address)continue;
      const auto &object=objects[r.object];const auto stream=unsigned(r.stream);
      const auto &source=published_fields[stream];
      const auto *prior=source&&source->route==plan.Target().route&&source->versions[stream]==batch->application_bases[stream]?
        source->Find(r.stream,object.identity):nullptr;
      field_index->records[n]={object.identity,object.instance,r.stream,prior&&prior->instance==object.instance&&!prior->rewrite};
      field_index->by_identity.push_back(n);
      if(r.flat){
        auto &applied=field_index->records[n];
        applied.flat=std::allocate_shared<ClientFlatNativeState>(S::ClientStateAllocator<ClientFlatNativeState>(memory),memory);
        applied.flat->stream=r.stream;
        if(prior&&prior->instance==object.instance&&prior->flat){
          applied.flat->written=prior->flat->written;
          applied.flat->costume_topology=prior->flat->costume_topology;
          applied.flat->edges=prior->flat->edges;
          applied.flat->references_uncertain=prior->flat->references_uncertain;
        }
        applied.flat_full=r.flat->full||!object.borrowed||!applied.has_previous_write||
          applied.flat->written!=r.flat->compare_base;
        field_index->flat_records.push_back(n);
        continue;
      }
      // 静止记录只保留逐批必须维持的叶：实体引用（退休拆边要找到它）和到期帧换算（剩余帧随展示帧变化）。
      const bool still=Quiet(n);
      for(std::uint32_t local=0;local<r.value_count;++local){
        if(visited++%128==0&&!preparation.StagedCurrent(*batch))Bad("binary_live_description_revoked");
        const auto &value=plan.Value(n,local);
        if(value.layout)continue;
        // 到期帧按字段定义的绑定处理器判定（与 Leaf 的换算口径一致），不看值本身的操作类别。
        if(still&&value.operation!=S::Operation::Reference&&value.operation!=S::Operation::Resource&&
            (value.field>=plan.FieldCount()||BoundLeafHandler(plan.Field(value.field).definition)!=LeafHandler::Expire))continue;
        if(value.field>=plan.FieldCount())Bad("binary_live_value_field_missing");
        if(!work.delegated[value.field])Leaf(value.field,value,local);
      }
    }
    std::sort(field_index->by_identity.begin(),field_index->by_identity.end(),[&](auto a,auto b){
      const auto &x=field_index->records[a],&y=field_index->records[b];
      return std::tie(x.stream,x.identity)<std::tie(y.stream,y.identity);
    });
  }
  bool InspectStaged() noexcept {
    if(described&&preparation.StagedCurrent(*batch))return true;
    // 跨窗口或原生回调后重验同一批承载。已描述的操作内含原生地址，换址必须拒绝该候选。
    return preparation.InspectStaged(*batch,this,
      [](void *p,std::uint32_t i,void *address,const S::PresentationMember &identity,std::uint64_t instance,bool borrowed)noexcept{
        auto &s=*static_cast<Impl *>(p);
        if(i>=s.objects.size())return false;
        if(s.described)return s.objects[i].address==address&&s.objects[i].identity==identity&&
          s.objects[i].instance==instance&&s.objects[i].borrowed==borrowed;
        s.objects[i]={address,identity,false,instance,borrowed};return true;
      },
      [](void *p,S::ResourceKey key,void *address)noexcept{
        auto &s=*static_cast<Impl *>(p);
        if(s.described){
          const auto at=std::lower_bound(s.resources.begin(),s.resources.end(),key,[](const auto &r,auto k){return r.key<k;});
          return at!=s.resources.end()&&at->key==key&&at->address==address;
        }
        if(s.resources.size()==s.resources.capacity())return false;
        s.resources.push_back({key,address});return true;
      },
      [](void *p,std::uint32_t i,const ClientStagedFieldInput &input,ClientStagedField *native)noexcept{
        auto &s=*static_cast<Impl *>(p);
        if(i>=s.fields.size())return false;
        if(s.described)return s.fields[i].native==native&&s.fields[i].input==&input;
        s.fields[i]={&input,native,true};return true;
      });
  }
  void BeginDescription() {
    auto stage_start=S::ClientApplyMetrics::Now();
    const auto stage=[&](S::ClientApplyStage value){
      const auto now=S::ClientApplyMetrics::Now();
      memory->ApplyMetrics().Mark(S::StateTargetStreams(scene->State().Target()),value,now-stage_start);stage_start=now;
    };
    const auto &plan=scene->State();objects.resize(plan.Objects().size());fields.resize(plan.FieldCount());
    field_index=std::allocate_shared<ClientSceneFieldIndex>(S::ClientStateAllocator<ClientSceneFieldIndex>(memory),memory);
    field_index->route=plan.Target().route;
    for(unsigned i=0;i<plan.Target().count;++i){
      const auto &section=plan.Target().sections[i];
      field_index->versions[unsigned(section.stream)]=section.target_version;
    }
    resources.reserve(plan.Resources().size());
    if(!InspectStaged())Bad("binary_atomic_staged_inputs");
    description_addresses.resize(plan.Paths().size());
    description_touched.reserve(plan.Paths().size());
    std::sort(resources.begin(),resources.end(),[](const auto &a,const auto &b){return a.key<b.key;});
    stage(S::ClientApplyStage::AtomicInputs);
    path_facts.reserve(plan.Paths().size());
    for(const auto &path:plan.Paths()){
      const auto &p=path.step;const auto *definition=S::FindField(p.layout,p.field);
      if(!definition)Bad("binary_atomic_path_definition");
      PathFacts facts;
      if(path.parent!=S::kClientPlanNone){
        if(path.parent>=path_facts.size())Bad("binary_atomic_path_parent_order");
        facts=path_facts[path.parent];
        ++facts.depth;
      }else{facts.root_block=definition->block;facts.costume=S::ClientCostumeProjection(p.layout,p.field);}
      facts.late|=bindings.LatePath(*definition);
      facts.display|=(p.layout==F::Room::kLayout&&p.field==F::Room::hell_backdrop)||
        (p.layout==F::Backdrop::kLayout&&(p.field==F::Backdrop::floor_sprite||p.field==F::Backdrop::wall_detail_sprite||
          p.field==F::Backdrop::controls_sprite||p.field==F::Backdrop::controls_buttons_sprite));
      path_facts.push_back(facts);
    }
    // 热路径的指令预留移到静止判定之后（Describe），按非静止记录的字段与叶计算。
    // 规范父链及完整对象／字段索引由后台同一计划封存；只留本窗口的查询游标。
    field_owners.resize(plan.NormalizedPathCount());
    field_index->records.resize(plan.Records().size());
    if(store){
      capture_state.assign(plan.Records().size(),0);
      field_pure.assign(plan.FieldCount(),0);value_pure.assign(plan.ValueCount(),0);
    }
    stage(S::ClientApplyStage::AtomicIndex);
  }
  // 判定静止记录（设计 10.5 节条件 1—6）。只读数字计划、已核暂存输入和上一份已发布索引，
  // 不访问原生内存。任一条件不满足都按原路径完整描述，宁可多写也不漏写。
  // 返回静止记录省下的叶数，供指令预留扣减。
  std::size_t ClassifyQuiet() {
    const auto &plan=scene->State();
    quiet.assign(plan.Records().size(),0);absorbed.clear();
    constexpr auto world=unsigned(S::BodyStream::World);
    std::uint32_t version=0;
    for(unsigned i=0;i<plan.Target().count;++i)
      if(plan.Target().sections[i].stream==S::BodyStream::World)version=plan.Target().sections[i].target_version;
    // 上一份已发布的 World 索引必须与本批应用基线是同一版本，块掩码才等于“相对上次写入”。
    const auto &source=published_fields[world];
    if(!version||!source||source->route!=plan.Target().route||!batch->application_bases[world]||
        source->versions[world]!=batch->application_bases[world])return 0;
    // 写程序表在用时，静止和吸收都要求本记录已有同实例、同形的条目（失败方式 9）；
    // 表不可用（刚恢复、版本不符）时本批全部照常描述，发布后重新登记。
    const bool storing=store!=nullptr;
    const bool usable=storing&&store->Usable(plan.Target().route,batch->application_bases[world]);
    if(storing&&!usable)return 0;
    store_generation=storing?store->generation:0;
    const auto *stored=usable?&store->plan->State():nullptr;
    std::uint64_t count=0,absorbed_count=0;std::size_t saved=0;
    for(std::uint32_t n=0;n<plan.Records().size();++n){
      const auto &r=plan.Records()[n];
      if(r.flat||r.event!=S::kClientPlanNone||r.stream!=S::BodyStream::World||!r.context.base_version||r.object>=objects.size())continue;
      if(r.structure_mask|r.dependency_mask)continue;
      // 值块有变化的记录只能被吸收（设计 10.6 节二期），强制刷新事务不吸收。
      if(r.value_mask&&!absorb)continue;
      const auto domain=plan.Objects()[r.object].domain;
      if(domain!=S::ClientInstallDomain::Entity&&domain!=S::ClientInstallDomain::Grid)continue;
      const auto &object=objects[r.object];
      if(!object.address||!object.borrowed||object.unbound||object.failure!=ClientMemberFailure::None)continue;
      // 轮转刷新：本批轮到的静止记录照常描述，进入漂移扫描并重放具名写入器。
      if((n+version)%kQuietRefreshPeriod==0)continue;
      const auto *prior=source->Find(r.stream,object.identity);
      if(!prior||prior->instance!=object.instance||!prior->has_previous_write||prior->rewrite)continue;
      // 原生容器必须是沿用的旧承载且本批不改值、不变长、不新建；否则元素叶仍须写入。
      bool stable=true;
      for(std::uint32_t f=r.first_field;stable&&f<r.first_field+r.field_count;++f){
        if(f>=fields.size()){stable=false;break;}
        const auto &field=fields[f];
        if(!field.native)continue;
        const auto &input=field.Input();
        stable=input.retained&&!input.update_live&&!input.resize_live_sequence&&!input.create_live_owner;
      }
      if(!stable)continue;
      const ClientSceneMemberStore::Record *entry=nullptr;
      if(storing){
        entry=store->Find(object.identity);
        if(!entry||entry->instance!=object.instance||entry->address!=object.address||
           !plan.SameRecordShape(n,*stored,entry->record))continue;
        // 条目登记的承载必须是本批沿用的同一原生句柄（失败方式 8）。
        bool carriers=true;
        for(std::uint32_t c=0;carriers&&c<entry->carrier_count;++c){
          const auto &carrier=store->carriers[entry->first_carrier+c];
          const auto f=r.first_field+carrier.field_local;
          carriers=f<fields.size()&&fields[f].native==carrier.native&&fields[f].Input().retained;
        }
        if(!carriers)continue;
      }
      if(r.value_mask){
        // 吸收只走非显示写段；含显示路径叶的记录照常描述。
        if(entry->display||!store->SameFixed(plan,n,*entry))continue;
        quiet[n]=2;++absorbed_count;saved+=r.value_count;
        absorbed.push_back({n,std::uint32_t(entry-store->records.data())});
        continue;
      }
      quiet[n]=1;++count;saved+=r.value_count;
    }
    const auto streams=S::StateTargetStreams(plan.Target());
    memory->ApplyMetrics().Volume(streams,S::ClientApplyVolume::QuietRecords,count);
    if(absorbed_count)memory->ApplyMetrics().Volume(streams,S::ClientApplyVolume::AbsorbedRecords,absorbed_count);
    return (std::min)(saved,plan.ValueCount());
  }
  bool Quiet(std::uint32_t record)const noexcept {return record<quiet.size()&&quiet[record];}
  // 先完成所有附件归属，再生成叶指令，避免漏掉跨记录依赖。
  void DescribeFields(Description &work) {
    const auto &plan=scene->State();
    // 每个完整字段先取得唯一归属，再建立原生指令；委托不能只覆盖一个标量却遗失空／缺席操作。
    for(std::uint32_t n=0;n<plan.FieldCount();++n){
      if(n%128==0&&!preparation.StagedCurrent(*batch))Bad("binary_atomic_description_revoked");
      const auto &f=plan.Field(n);const auto &r=plan.Records()[f.record];
      if(r.event!=S::kClientPlanNone||r.object>=objects.size()||!objects[r.object].address)continue;
      const auto &d=S::Fields()[f.definition];
      const auto use=bindings.DescribeField(Inputs(),n);
      if(store){
        // 写程序分类：具名、委托或私有承载字段的字节必须相等；其余字段的字节由纯值叶覆盖。
        const auto &staged=fields[n];const auto kind=staged.Input().kind;
        field_pure[n]=!use.covered&&!use.kind&&(!staged.native||kind==ClientStagedFieldKind::Group||Rows(kind));
        if(bindings.FrameField(use)&&!Quiet(f.record))CaptureTime(n);
      }
      // 静止记录省掉可省的具名字段：不登记给绑定器，也不生成委托指令；其叶仍按委托标记跳过。
      const bool skip=Quiet(f.record)&&bindings.QuietSkippable(use);
      if((use.kind||use.display)&&!skip)binding_fields.push_back({n,use.kind,use.display});
      if(skip&&use.covered){work.delegated[n]=1;continue;}
      if(use.covered){
        work.delegated[n]=1;
        if(use.install){auto &op=NewOperation();op.delegated=true;op.field=n;op.phase=f.phase;Add(op);}
        continue;
      }
      if(!f.present&&fields[n].Input().kind!=ClientStagedFieldKind::Records)continue;
      if(d.nested_layout||d.operation==S::Operation::Container||d.operation==S::Operation::Choice){
        auto &field=fields[n];
        if(!field.staged)Bad("binary_atomic_field_not_staged");
        const auto size=ClientInspectedFieldAttachmentSize(field.native);
        if(size){void *address=nullptr;std::size_t extent=size;
          const auto parent=plan.Paths()[f.path].parent;
          const bool late=Late(parent)||LiveParent(r.object,parent);
          if((!late&&!Destination(n,address,extent))||extent!=size)Bad("binary_atomic_container_owner_missing");
          auto &op=NewOperation();op.field=n;op.phase=f.phase;op.late=late;
          op.write={ClientAtomicWriteKind::FieldAttachment,address,extent,size,nullptr,nullptr,0,field.native,field.Input().retained};
          Add(op);
        }else if(field.Input().kind==ClientStagedFieldKind::Group){
          // 内嵌Group无独立header；协议聚合和不透明owner必须明确委托，不能空操作冒充安装。
          if(d.source!=S::FieldSource::Object||S::FindLayout(d.layout)->protocol)Bad("binary_atomic_group_owner_missing");
        }else if(Rows(field.Input().kind)){
          S::BodySourceFieldInfo format;if(!S::DescribeBodySourceField(d.layout,d.index,format)||d.source!=S::FieldSource::Object||
            S::FindLayout(d.layout)->protocol||(d.operation==S::Operation::Container&&(format.storage!=S::BodyStorage::Array||
            (!d.fixed_count&&std::string_view(d.handler)!="read_effect_ghost_active_sprites"&&std::string_view(d.handler)!="rail_manager_grid_flags_read")||
            std::uint64_t(field.Input().count)*field.Input().stride>d.native_size)))Bad("binary_atomic_rows_owner_missing");
        }else Bad("binary_atomic_container_attachment_abi");
      }
    }
  }
  // Time 计时器按“本批帧号减锚点”写入（设计 10.6 节失败方式 3）；父承载必须是静态地址。
  void CaptureTime(std::uint32_t n) {
    const auto &plan=scene->State();const auto &f=plan.Field(n);const auto &r=plan.Records()[f.record];
    if(r.stream!=S::BodyStream::World||r.event!=S::kClientPlanNone||capture_state[f.record]==2)return;
    const auto &d=S::Fields()[f.definition];const auto parent=plan.Paths()[f.path].parent;
    void *base=nullptr;std::size_t extent=0;
    if(Late(parent)||LiveParent(r.object,parent)||d.native_size!=4||!Instance(f.record,parent,base,extent)||
       d.native_offset>extent||4>extent-d.native_offset){capture_state[f.record]=2;return;}
    ClientSceneMemberStore::Leaf leaf;
    leaf.field_local=n-r.first_field;leaf.definition=f.definition;leaf.size=4;leaf.time=true;
    leaf.destination=static_cast<std::byte *>(base)+d.native_offset;
    if(!CaptureCarriers(f.record,parent))return;
    capture_leaves.push_back({f.record,leaf});capture_state[f.record]=1;
  }
  void ClassifyOperations(Description &work) {
    const auto &plan=scene->State();
    std::size_t visited=0;
    for(auto &op:operations){
      if(visited++%128==0&&!preparation.StagedCurrent(*batch))Bad("binary_atomic_description_revoked");
      const auto &f=plan.Field(op.field);
      const auto &d=S::Fields()[f.definition];
      op.display=DisplayField(f.path);
      const auto order=op.delegated?bindings.Order(d):bindings.ValueOrder(d);
      op.priority=op.delegated?order:op.write.kind==ClientAtomicWriteKind::EntityReference?7u:order==0?0u:5u;
      // 委托的文件及动画仍按原优先级安装，但必须先接好本字段所需的新父容器。
      if(op.delegated&&order>0&&order<=3)op.late=LiveParent(plan.Records()[f.record].object,plan.Paths()[f.path].parent);
      work.late_count+=op.late;work.delegated_count+=op.delegated;
      deferred_display|=op.display;
    }
  }
  // 在一个原生只读窗口内完成整批描述，取消和承载失效检查仍贯穿各阶段。
  void Describe() {
    PageMetrics pages(*this);
    BeginDescription();
    const auto quiet_values=ClassifyQuiet();
    {
      // 热路径预留：静止记录只生成少量引用／到期帧／附件指令，其纯值叶不占预留；不足时向量正常增长。
      // 完整基线与前一完整场景共存时，仍在下方按实际可安装叶计算容量。
      const auto &plan=scene->State();
      if(plan.Target().count!=2||plan.Target().sections[0].base_version)
        operations.reserve((std::min)(std::size_t(262144),plan.FieldCount()+plan.ValueCount()-quiet_values));
    }
    PurePathWindow description_window(*this);
    if(!description_window.ready)Bad("binary_atomic_description_path_capacity");
    const auto &plan=scene->State();
    Description work(memory,plan.FieldCount());
    const auto streams=S::StateTargetStreams(plan.Target());auto &metrics=memory->ApplyMetrics();
    {
      S::ClientApplyTimer time(metrics,streams,S::ClientApplyStage::AtomicFields);
      DescribeFields(work);
    }
    if(plan.Target().count==2&&!plan.Target().sections[0].base_version){
      auto possible=operations.size();
      for(std::uint32_t n=0;n<plan.Records().size();++n){
        const auto &r=plan.Records()[n];
        if(r.event!=S::kClientPlanNone)continue;
        for(std::uint32_t local=0;local<r.value_count;++local){
          const auto value=plan.Value(n,local);
          possible+=!value.layout&&value.field<work.delegated.size()&&!work.delegated[value.field];
        }
      }
      operations.reserve((std::min)(std::size_t(262144),possible));
    }
    const auto leaves_start=S::ClientApplyMetrics::Now();
    DescribeLeaves(work);
    const auto raw_count=operations.size();
    // 地址预检单独记 AtomicRanges，叶处理扣除同一段，避免重复归因。
    const auto ranges=IndexWrites();
    metrics.Mark(streams,S::ClientApplyStage::AtomicLeaves,S::ClientApplyMetrics::Now()-leaves_start-ranges);
    {
      S::ClientApplyTimer time(metrics,streams,S::ClientApplyStage::AtomicOrder);
      ClassifyOperations(work);
      BindOrdering();
      // 组装完成即释放历史索引借用。新索引只共享仍生效的纯片段。
      published_fields={};
    }
    // 指令规模按完整描述批次聚合，区分静态预检与晚绑定排序的实际工作量。
    // 固定四次计数，不记录逐字段日志；两流联合批次沿用既有按目标流记账的口径。
    metrics.Volume(streams,S::ClientApplyVolume::RawOperations,raw_count);
    metrics.Volume(streams,S::ClientApplyVolume::NativeOperations,operations.size());
    metrics.Volume(streams,S::ClientApplyVolume::LateOperations,work.late_count);
    metrics.Volume(streams,S::ClientApplyVolume::DelegatedOperations,work.delegated_count);
    described=true;
  }
  std::uint64_t ReadbackKey(const Op &op)const noexcept {
    const auto &plan=scene->State();
    const auto &f=plan.Field(op.field);const auto &facts=path_facts[f.path];
    return (std::uint64_t(facts.costume)<<48)|(std::uint64_t(plan.Records()[f.record].stream)<<40)|
      (std::uint64_t(f.record)<<8)|facts.root_block;
  }
  // 只有执行键和分组完全一致才沿用排序结果。物理写区重叠仍按本批实际地址验证。
  void BindOrdering() {
    using Order=ClientSceneFieldIndex::Order;
    constexpr unsigned phases=unsigned(S::ClientInstallPhase::Retire)+1;
    const auto key=[&](const Op &op){
      return (ReadbackKey(op)<<12)|(std::uint64_t(op.display)<<11)|(std::uint64_t(op.priority)<<8)|unsigned(op.phase);
    };
    const bool bounded=std::all_of(operations.begin(),operations.end(),[](const auto &op){
      return op.priority<8&&unsigned(op.phase)<phases;
    });
    for(unsigned stream=0;stream<2&&bounded;++stream){
      const auto &prior=published_fields[stream];
      if(!field_index->versions[stream]||!prior||prior->route!=field_index->route||
          !batch->application_bases[stream]||prior->versions[stream]!=batch->application_bases[stream]||
          prior->order->keys.size()!=operations.size())continue;
      if(std::equal(operations.begin(),operations.end(),prior->order->keys.begin(),[&](const auto &op,auto before){return key(op)==before;})){
        field_index->order=prior->order;
        memory->ApplyMetrics().Volume(S::StateTargetStreams(scene->State().Target()),S::ClientApplyVolume::NativeReusedOrder);
        break;
      }
    }
    if(!field_index->order){
      auto order=std::allocate_shared<Order>(S::ClientStateAllocator<Order>(memory),memory);
      order->execution.resize(operations.size());
      if(bounded){
        order->keys.reserve(operations.size());
        std::array<std::size_t,2*8*phases> offsets{};
        const auto bucket=[](const Op &op){return (unsigned(op.display)*8+op.priority)*phases+unsigned(op.phase);};
        for(const auto &op:operations){order->keys.push_back(key(op));++offsets[bucket(op)];}
        std::size_t next=0;for(auto &offset:offsets){const auto count=offset;offset=next;next+=count;}
        for(std::uint32_t i=0;i<operations.size();++i)order->execution[offsets[bucket(operations[i])]++]=i;
      }else{
        for(std::uint32_t i=0;i<operations.size();++i)order->execution[i]=i;
        std::sort(order->execution.begin(),order->execution.end(),[&](auto a,auto b){
          const auto &x=operations[a],&y=operations[b];
          return std::tie(x.display,x.priority,x.phase,a)<std::tie(y.display,y.priority,y.phase,b);
        });
      }
      BuildReadback(*order);
      field_index->order=std::move(order);
    }
    field_attachments.resize(fields.size(),S::kClientPlanNone);
    for(std::uint32_t n=0;n<operations.size();++n){
      if(n%128==0&&!preparation.StagedCurrent(*batch))Bad("binary_atomic_description_revoked");
      const auto index=field_index->order->execution[n];
      auto &op=operations[index];op.order=n;
      if(op.delegated)continue;
      if(op.late)late_operations[op.display].push_back(index);
      if(op.write.kind==ClientAtomicWriteKind::FieldAttachment){
        field_attachments[op.field]=n;
        attachment_operations.push_back(index);
      }
    }
    const auto &plan=scene->State();
    std::sort(attachment_operations.begin(),attachment_operations.end(),[&](auto a,auto b){
      const auto x=path_facts[plan.Field(operations[a].field).path].depth;
      const auto y=path_facts[plan.Field(operations[b].field).path].depth;
      return std::tie(x,operations[a].order)<std::tie(y,operations[b].order);
    });
  }
  // 读回按流、服装、记录、块排列。同组保留原生执行顺序，跨批只保留整数槽号。
  void BuildReadback(ClientSceneFieldIndex::Order &order) {
    const auto records=scene->State().Records().size();
    const auto buckets=std::uint64_t(records)*128;
    const auto count=operations.size();
    bool dense=records&&buckets<=count*2&&buckets<=resolved_scratch.capacity();
    if(dense)resolved_scratch.resize(std::size_t(buckets),0);
    const auto bucket=[&](std::uint64_t key) noexcept {
      return (std::size_t((key>>48)*2+((key>>40)&255))*records+std::uint32_t(key>>8))*32+(key&255);
    };
    order.readback.reserve(count);
    const auto add=[&](ClientSceneFieldIndex::ReadbackEntry entry){
      order.readback.push_back(entry);
      if(!dense)return;
      const auto slot=bucket(entry.key);
      if(slot>=buckets)dense=false;else ++resolved_scratch[slot];
    };
    for(std::uint32_t i=0;i<operations.size();++i){
      if(i%128==0&&!preparation.StagedCurrent(*batch))Bad("binary_atomic_description_revoked");
      const auto raw=order.execution[i];
      add({order.keys.empty()?ReadbackKey(operations[raw]):order.keys[raw]>>12,i});
    }
    if(dense){
      std::uint32_t next=0;
      for(auto &offset:resolved_scratch){const auto count=offset;offset=next;next+=count;}
      decltype(order.readback) sorted(order.readback.get_allocator());sorted.resize(order.readback.size());
      for(const auto &entry:order.readback)sorted[resolved_scratch[bucket(entry.key)]++]=entry;
      order.readback.swap(sorted);
    }else if(order.readback.size()>1){
      // 稀疏记录号走原先的稳定基数分组。只重排数字槽，同组按执行顺序保留。
      std::uint64_t varying=0;
      const auto first=order.readback.front().key;
      for(const auto &entry:order.readback)varying|=entry.key^first;
      if(varying){
        decltype(order.readback) sorted(order.readback.get_allocator());sorted.resize(order.readback.size());
        static_assert(std::endian::native==std::endian::little);
        for(unsigned byte=0;byte<7;++byte){
          const auto digit=[&](const std::uint64_t &key){return reinterpret_cast<const unsigned char *>(&key)[byte];};
          if(!digit(varying))continue;
          std::array<std::size_t,256> offsets{};
          for(const auto &entry:order.readback)++offsets[digit(entry.key)];
          std::size_t next=0;for(auto &offset:offsets){const auto count=offset;offset=next;next+=count;}
          for(const auto &entry:order.readback)sorted[offsets[digit(entry.key)]++]=entry;
          order.readback.swap(sorted);
        }
      }
    }
    resolved_scratch.clear();
  }
  // 装载稳定图层地址后再查实际区间。所有索引空间已在Stage保留，不在写入中扩容。
  void ResolveWrites(bool display) {
    // 静态区间的重叠和一致投影已由 IndexWrites 核定，只有晚绑定地址加入时才需要重排。
    const auto &pending=late_operations[display];
    if(pending.empty())return;
    // Init／附件安装之后重新取得地址；这一轮只定位与检查区间，不执行原生回调。
    // 同一纯读窗口内复用父链和页见证，退出前清空，后续写入和回读不能沿用它们。
    // 容量已在Describe保留，真正写段不再分配。
    PurePathWindow window(*this);
    if(!window.ready)Bad("binary_atomic_late_path_capacity");
    resolved_writes.clear();
    for(const auto i:pending){
      auto &op=operations[i];
      if(Rejected(ObjectOf(op)))continue;
      void *address=nullptr;std::size_t extent=0;
      const auto &input=fields[op.field].Input();
      if(op.staged_element!=S::kClientPlanNone&&(input.resize_live_sequence||input.create_live_owner)){
        address=FieldElement(fields[op.field].native,op.staged_element);
        extent=op.element_stride;
      }else Destination(op.field,address,extent);
      const auto offset=op.element==S::kClientPlanNone?0:std::size_t(op.element)*op.element_stride;
      if(!address||offset>extent||op.write.size>extent-offset){
        if(RejectField(op,"binary_atomic_late_field_address"))continue;
        Bad("binary_atomic_late_field_address");
      }
      op.write.destination=static_cast<std::byte *>(address)+offset;
      resolved_writes.push_back(i);
    }
#ifdef ISAACCS_E2E
    // 故障只改候选晚绑定地址，不写原生对象；合并检查须拒绝并撤销此前的Init写入。
    const bool overlap=ClientEntityTestFault("binary-late-overlap");
    const bool conflict=ClientEntityTestFault("binary-late-conflict");
    if(overlap||conflict){
      Op *first=nullptr,*second=nullptr;
      for(auto n:static_writes)if(operations[n].display==display&&Pure(operations[n])&&operations[n].write.size>=2){first=&operations[n];break;}
      for(auto n:resolved_writes)if(Pure(operations[n])&&operations[n].write.size>=2){second=&operations[n];break;}
      if(!first||!second)Bad("binary_e2e_late_candidates_missing");
      second->write.destination=static_cast<std::byte *>(first->write.destination)+(overlap?1:0);
      second->write.size=overlap?1:first->write.size;
      second->write.mask=first->write.mask^1u;
    }
#endif
    const auto address=[&](std::uint32_t n){return reinterpret_cast<std::uintptr_t>(operations[n].write.destination);};
    SortWriteAddresses();
    // 只排序本次得到的晚绑定地址，再与同候选静态区间归并。相同起点按当前执行序号
    // 打破平局，与原来全量稳定地址排序一致；后面的逐区间重叠／目标一致性检查保持原样。
    resolved_scratch.clear();std::size_t fixed=0,late=0;
    while(fixed<static_writes.size()||late<resolved_writes.size()){
      while(fixed<static_writes.size()&&(operations[static_writes[fixed]].display!=display||
            Rejected(ObjectOf(operations[static_writes[fixed]]))))++fixed;
      if(fixed==static_writes.size()&&late==resolved_writes.size())break;
      bool take_fixed=late==resolved_writes.size();
      if(!take_fixed&&fixed<static_writes.size()){
        const auto a=static_writes[fixed],b=resolved_writes[late];
        take_fixed=address(a)<address(b)||(address(a)==address(b)&&operations[a].order<operations[b].order);
      }
      resolved_scratch.push_back(take_fixed?static_writes[fixed++]:resolved_writes[late++]);
    }
    resolved_writes.swap(resolved_scratch);resolved_scratch.clear();
    std::uint32_t previous=S::kClientPlanNone;
    for(auto n:resolved_writes){
      auto &w=operations[n].write;const auto start=address(n);
      if(!start||w.size>UINTPTR_MAX-start)Bad("binary_atomic_resolved_range");
      if(previous!=S::kClientPlanNone&&address(previous)+operations[previous].write.size>start){
        const auto &old=operations[previous].write;
        if(address(previous)!=start||old.size!=w.size||old.kind!=w.kind||old.reference!=w.reference||old.field!=w.field||old.mask!=w.mask||
           (old.value&&(!w.value||std::memcmp(old.value,w.value,w.size))))Bad("binary_atomic_resolved_projection_conflict");
        // 同址的晚绑定投影仍需执行时，由真实写项接管该区间。
        operations[n].duplicate=true;
      }else previous=n;
    }
  }
  bool Step(S::ClientStateNativeStep s) noexcept {
    written|=s.written;if(!s.ready||s.failed){Fail(s.reason);return false;}
    if(!Current(*batch)){Fail("binary_atomic_lifecycle_after_callback");return false;}return true;
  }
  static bool Pure(const Op &op) noexcept {
    return !op.delegated&&(op.write.kind==ClientAtomicWriteKind::Bytes||op.write.kind==ClientAtomicWriteKind::Masked32);
  }
  // 新承载只在当前写段接入，安装失败交本对象隔离及正常退休处理。
  bool CreateLiveFieldOwner(Op &op,bool display,ClientAtomicReadRegion &region) {
    const auto &input=fields[op.field].Input();
    if(op.display!=display||op.duplicate||op.done||!input.create_live_owner)return true;
    const auto object=ObjectOf(op);
    if(!Current(*batch)||object>=objects.size())return false;
    if(Rejected(object))return true;
    const auto &root=objects[object];
    if(root.borrowed&&!preparation.PendingObjectCurrent(*batch,object,root.address,root.instance))
      return RejectObject(object,ClientMemberFailure::Stale,"binary_live_field_owner_changed");
    std::size_t extent=0;
    if(!Destination(op.field,op.write.destination,extent)||extent<op.write.size)
      return RejectField(op,"binary_live_field_owner_destination");
    if(ClientStagedFieldAttached(op.write.field,op.write.destination)){op.duplicate=true;return true;}
    if(!ClientAtomicWritePreflight(op.write,region)||!ClientStagedFieldMaterialize(op.write.field,input))
      return RejectField(op,"binary_live_field_owner_materialize");
    written=true;
    if(!ClientAtomicWriteApply(op.write)){
      if(ClientStagedFieldAttached(op.write.field,op.write.destination))op.done=true;
      region.Clear();
      return RejectField(op,"binary_live_field_owner_attach");
    }
    op.done=true;
    region.Clear();
    return true;
  }
  // 文件、动画等早期委托可能在新 Costume 或 Weapon 内。按父到子接入必要承载，
  // 其他字段仍在 Sprite 文件与动画就绪后统一解析。
  bool CreateEarlyParents(const Op &op) {
    const auto &plan=scene->State();
    const auto object=ObjectOf(op);
    std::array<std::uint32_t,S::kCodecMaxDepth> parents{};
    std::size_t count=0;
    for(auto path=plan.Paths()[plan.Field(op.field).path].parent;path!=S::kClientPlanNone;path=plan.Paths()[path].parent){
      const auto field=FieldAt(object,path);
      if(field==S::kClientPlanNone||!fields[field].Input().create_live_owner)continue;
      const auto attached=field_attachments[field];
      if(attached==S::kClientPlanNone||count==parents.size())return false;
      parents[count++]=attached;
    }
    ClientAtomicReadRegion region;
    while(count)if(!CreateLiveFieldOwner(Ordered(parents[--count]),op.display,region))return false;
    return true;
  }
  bool CreateLiveFieldOwners(bool display) {
    ClientAtomicReadRegion region;
    for(const auto n:attachment_operations)if(!CreateLiveFieldOwner(operations[n],display,region))return false;
    return true;
  }
  // 写程序登记的承载仍接在活对象上（设计 10.6 节失败方式 1）。只读 header 槽位。
  bool CarriersAttached(const ClientSceneMemberStore::Record &e) const noexcept {
    for(std::uint32_t c=0;c<e.carrier_count;++c){
      const auto &carrier=store->carriers[e.first_carrier+c];
      if(!ClientStagedFieldAttached(carrier.native,carrier.slot))return false;
    }
    return true;
  }
  // 按写程序全量写一条记录的纯值叶（设计 10.6 节）。plan 是提供字节的计划，n 是其中的记录号；
  // 晚绑定叶用本安装器当前的路径定位重新取地址。调用者已在写段入口核复制窗口、对象实例和承载链。
  // 返回 false 表示字节不合法、定位失败或写入触发结构异常。
  bool RunRecord(const ClientSceneMemberStore::Record &e,const S::ClientStateInstallPlan &plan,std::uint32_t n,
      bool display,std::uint64_t &count) noexcept {
    try {
      const auto &r=plan.Records()[n];const auto frame=r.context.game_frame;
      for(std::uint32_t i=0;i<e.leaf_count;++i){
        const auto &leaf=store->leaves[e.first_leaf+i];
        if(leaf.display!=display)continue;
        std::array<std::byte,8> scratch;LeafBytes bytes;
        if(leaf.time){
          // Time：锚点在字段字节里，原生值是本批帧号减锚点。
          const auto anchor_bytes=plan.Bytes(plan.Field(r.first_field+leaf.field_local).bytes);
          if(anchor_bytes.size()!=4)return false;
          std::uint32_t anchor=0;std::memcpy(&anchor,anchor_bytes.data(),4);
          const auto next=frame-anchor;std::memcpy(scratch.data(),&next,4);bytes.out=std::span(scratch).first(4);
        }else{
          const auto value=plan.Value(n,leaf.local);
          if(ConvertLeaf(S::Fields()[leaf.definition],leaf.definition,plan.Bytes(value.bytes),frame,scratch,bytes))return false;
        }
        if(bytes.out.size()!=leaf.size)return false;
        void *destination=leaf.destination;
        if(leaf.late){
          void *base=nullptr;std::size_t extent=0;
          if(!Destination(r.first_field+leaf.field_local,base,extent)||!base)return false;
          const auto offset=leaf.element==S::kClientPlanNone?std::size_t{0}:std::size_t(leaf.element)*leaf.stride;
          if(offset>extent||leaf.size>extent-offset)return false;
          destination=static_cast<std::byte *>(base)+offset;
        }
        // 首次写到 Game／Room 根标量前补快照，撤场仍能还原菜单值（失败方式 11）。
        if(root_scalars&&root_scalars->NeedsSnapshot(roots,destination,leaf.size)){
          ClientAtomicWrite write;write.destination=destination;write.extent=write.size=leaf.size;write.value=bytes.out.data();
          ClientAtomicReadRegion region;
          if(!root_scalars->Save(write,region))return false;
        }
        if(!ClientAtomicFastWrite(destination,bytes.out.data(),bytes.out.size(),bytes.mask))return false;
        ++count;
      }
      return true;
    }catch(...){return false;}
  }
  // 事务内被吸收记录的纯值写入（设计 10.6 节二期）：每条记录核实例与承载后全量写；单条失败按对象隔离。
  S::ClientStateNativeStep RunAbsorbed() noexcept {
    if(absorbed.empty())return Ready(written,"binary_absorb_empty");
    S::ClientApplyTimer measured(memory->ApplyMetrics(),S::StateTargetStreams(scene->State().Target()),S::ClientApplyStage::FastWrite);
    if(!Current(*batch)||!ClientAtomicCopyWindow())return Fail("binary_absorb_copy_window");
    const auto &plan=scene->State();
    std::uint64_t count=0;
    for(const auto &[n,index]:absorbed){
      const auto object=plan.Records()[n].object;
      if(object>=objects.size())return Fail("binary_absorb_object_index");
      if(Rejected(object)||objects[object].unbound)continue;
      const auto &root=objects[object];
      if(root.borrowed&&!preparation.PendingObjectCurrent(*batch,object,root.address,root.instance)){
        if(RejectObject(object,ClientMemberFailure::Stale,"binary_absorb_object_replaced"))continue;
        return Fail("binary_absorb_object_replaced");
      }
      const auto &entry=store->records[index];
      if(!CarriersAttached(entry)){
        if(RejectObject(object,ClientMemberFailure::Native,"binary_absorb_carrier_detached"))continue;
        return Fail("binary_absorb_carrier_detached");
      }
      const auto before=count;
      const bool ok=RunRecord(entry,plan,n,false,count);
      written|=count!=before;
      if(!ok){
        if(RejectObject(object,ClientMemberFailure::Native,"binary_absorb_write"))continue;
        return Fail("binary_absorb_write");
      }
    }
    memory->ApplyMetrics().Volume(S::StateTargetStreams(plan.Target()),S::ClientApplyVolume::FastLeaves,count);
    return Ready(written,"binary_absorb_written");
  }
  struct FlatWindow {Impl &owner;std::uint32_t object;bool installed=false;};
  bool FlatCurrent(std::uint32_t index,bool installed,bool allow_absent_world_entity=false) noexcept {
    if(!Current(*batch)||index>=objects.size())return false;
    const auto &o=objects[index];
    if(!o.address||o.unbound||Rejected(index))return false;
    if(!installed)return preparation.PendingObjectCurrent(*batch,index,o.address,o.instance,true,allow_absent_world_entity);
    std::uint64_t instance=0;
    return preparation.InstalledObject(*batch,index,&instance)==o.address&&instance==o.instance;
  }
  ClientFlatNativeContext FlatContext(FlatWindow &window,const S::ClientInstallRecord &record) noexcept {
    const auto &object=objects[window.object];
    const S::ClientFlatImage *player_world=nullptr;
    if(record.layout==197&&record.stream==S::BodyStream::Body)
      if(const auto world=scene->State().Effective(S::BodyStream::World))
        if(const auto player=world->Find(object.identity))player_world=player->Flat().get();
    return {roots,resources,record.context.game_frame,record.stream,!object.borrowed,player_world,&window,[](void *p) noexcept {
      // exists 可在本轮值写入中翻转；后续特种字段仍写同一池槽。引用目标继续要求活对象。
      auto &w=*static_cast<FlatWindow *>(p);return w.owner.FlatCurrent(w.object,w.installed,true);
    },[](void *p,const S::PresentationMember &id) noexcept {
      auto &w=*static_cast<FlatWindow *>(p);const auto plan=w.owner.scene->State().Objects();
      const auto it=std::lower_bound(plan.begin(),plan.end(),id,[](const auto &obj,const auto &key){return obj.member.identity<key;});
      if(it==plan.end()||it->member.identity!=id)return ClientFlatTarget{id};
      const auto index=std::uint32_t(it-plan.begin());
      if(!w.owner.FlatCurrent(index,w.installed))return ClientFlatTarget{id};
      const auto &o=w.owner.objects[index];
      return ClientFlatTarget{id,o.address,o.instance};
    },[](void *p,const ClientAtomicWrite &write,ClientAtomicReadRegion &region) noexcept {
      auto &w=*static_cast<FlatWindow *>(p);auto &s=w.owner;
      return !s.root_scalars||!s.root_scalars->NeedsSnapshot(s.roots,write.destination,write.size)||s.root_scalars->Save(write,region);
    }};
  }
  bool FlatIssue(std::uint32_t n,const ClientFlatNativeResult &result) noexcept {
    written|=result.written;
    pure_write_bytes+=result.bytes;
    if(!result.reason)return true;
    const auto &record=scene->State().Records()[n];
    field_index->records[n].rewrite=true;
    S::ClientApplyMetrics::Issue issue;
    issue.route=batch->ticket.route.route;issue.ticket=batch->ticket.serial;
    issue.member=objects[record.object].identity.id;issue.instance=objects[record.object].instance;
    issue.layout=result.layout;issue.field=result.field;
    memory->ApplyMetrics().FirstIssue(std::uint8_t(1u<<unsigned(record.stream)),issue,"flat_write",result.reason);
    Trace("client_error","event=flat_native_failure stream=%u layout=%u field=%u reason=%s",
      unsigned(record.stream),unsigned(result.layout),unsigned(result.field),result.reason);
    return RejectObject(record.object,ClientMemberFailure::Native,result.reason);
  }
  enum class FlatPass { Values, Graphics, References };
  bool InstallFlat(FlatPass pass) noexcept {
    for(const auto n:field_index->flat_records){
      const auto &plan=scene->State().Records()[n];
      const auto index=plan.object;
      if(Rejected(index)||objects[index].unbound)continue;
      auto &entry=field_index->records[n];
      const auto *layout=S::FlatRecordLayout(plan.context.layout_version,plan.layout);
      if(!layout)return false;
      auto &object=objects[index];
      if(plan.flat->skip){
        entry.rewrite=true;
        if(!object.borrowed&&!RejectObject(index,ClientMemberFailure::Stale,"flat_skipped_without_live_instance"))return false;
        continue;
      }
      if(!FlatCurrent(index,false,true)){
        if(!RejectObject(index,ClientMemberFailure::Stale,"flat_live_instance_changed"))return false;
        continue;
      }
#ifdef ISAACCS_E2E
      // 连续三次故障只命中同一颗泪弹的 World 记录，不能把 Body 和 World 的失败混计。
      if(pass==FlatPass::Values&&plan.stream==S::BodyStream::World&&
          scene->State().Objects()[index].domain==S::ClientInstallDomain::Entity&&
          (ClientEntityTestFault("binary-object-write")||
           (layout->id==F::EntityTear::kLayout&&ClientTearWriteTestFault(object.identity.id)))){
        if(!RejectObject(index,ClientMemberFailure::Native,"binary_e2e_flat_object_write"))return false;
        entry.rewrite=true;
        continue;
      }
#endif
      FlatWindow window{*this,index};
      const auto context=FlatContext(window,plan);
      if(pass==FlatPass::Values){
        if(!FlatIssue(n,ClientWriteFlat(context,object.address,*layout,*plan.flat,entry.flat_full,*entry.flat)))return false;
      }else if(pass==FlatPass::Graphics){
        if(!FlatIssue(n,ClientCheckFlat(context,object.address,*layout,*plan.flat,*entry.flat,0,true,entry.flat_full)))return false;
      }else{
        if(!FlatIssue(n,ClientBindFlat(context,object.address,*layout,*plan.flat,entry.flat_full,*entry.flat)))return false;
        if(!Rejected(index))entry.flat->written=plan.flat->image;
      }
    }
    return true;
  }
  S::ClientStateNativeStep InstallNow(bool display=false) noexcept {
    PageMetrics pages(*this);
    // 被吸收记录的条目下标在分类时取得；写程序表其后若被刷新或清空，本批在写入前拒绝。
    if(!display&&!absorbed.empty()&&(!store||store->generation!=store_generation))return Fail("binary_absorb_store_changed");
    if(!display)install_entered=true;
    // 只细分主安装段，每批四个聚合计时；显示后的单独恢复不混入此分母。
    const auto metric_streams=display?std::uint8_t{0}:S::StateTargetStreams(scene->State().Target());
    bool initialized=false,resolved=false,owners_prepared=false;
    ClientAtomicReadRegion read_region;
    unsigned copies_left=0;
    std::uint32_t checked_copy_object=S::kClientPlanNone;
    const auto callback_boundary=[&]{
      read_region.Clear();copies_left=0;checked_copy_object=S::kClientPlanNone;return Current(*batch);
    };
    for(std::size_t position=0;position<operations.size();++position){
      auto &op=Ordered(position);
      if(op.display!=display)continue;
      if(!display&&op.priority>0&&!initialized){
        if(!callback_boundary())return Fail("binary_atomic_lifecycle_during_install");
        S::ClientApplyTimer measured(memory->ApplyMetrics(),metric_streams,S::ClientApplyStage::NativeInitialize);
        if(!Step(bindings.Initialize(*batch,Inputs())))return Failed(written,reason);initialized=true;
      }
      if(!display&&op.priority>0&&!owners_prepared){
        S::ClientApplyTimer measured(memory->ApplyMetrics(),metric_streams,S::ClientApplyStage::NativeOwners);
        if(!Step(bindings.PrepareFieldOwners(*batch,Inputs())))return Failed(written,reason);owners_prepared=true;
      }
      if(op.priority>3&&!resolved){try{
        // 先装载 Sprite 和原生父 holder，再创建容器并核定最终落点。
        if(!CreateLiveFieldOwners(display)||!callback_boundary())return Fail("binary_live_field_owner_create");
        // 先在真容器上改变长度，再定位本批元素叶，避免持有重分配前的地址。
        for(const auto n:attachment_operations){
          auto &attachment=operations[n];
          const auto &input=fields[attachment.field].Input();
          if(attachment.display!=display||attachment.duplicate||!input.resize_live_sequence)continue;
          const auto &plan=scene->State();
          const auto index=plan.Records()[plan.Field(attachment.field).record].object;
          if(index>=objects.size())return Fail("binary_live_sequence_object");
          if(Rejected(index))continue;
          const auto &object=objects[index];
          if(!object.borrowed||!preparation.PendingObjectCurrent(*batch,index,object.address,object.instance)){
            if(RejectObject(index,ClientMemberFailure::Stale,"binary_live_sequence_owner_changed"))continue;
            return Fail("binary_live_sequence_owner_changed");
          }
          if(attachment.late){
            std::size_t extent=0;
            if(!Destination(attachment.field,attachment.write.destination,extent)||extent<attachment.write.size){
              if(RejectField(attachment,"binary_live_sequence_attachment"))continue;
              return Fail("binary_live_sequence_attachment");
            }
          }
          if(!ClientAtomicWritePreflight(attachment.write,read_region)){
            if(RejectField(attachment,"binary_live_sequence_preflight"))continue;
            return Fail("binary_live_sequence_owner_changed");
          }
          written=true;
          if(!ClientStagedFieldInstallValues(attachment.write.field,input)){
            if(RejectField(attachment,"binary_live_sequence_resize")){read_region.Clear();continue;}
            return Fail("binary_live_sequence_resize");
          }
          if(!callback_boundary())
            return Fail("binary_live_sequence_resize");
        }
        S::ClientApplyTimer measured(memory->ApplyMetrics(),metric_streams,S::ClientApplyStage::NativeResolve);
        ResolveWrites(display);resolved=true;
      }catch(const S::ClientShadowError &e){return Fail(e.what());}catch(...){return Fail("binary_atomic_late_field_path");}}
      if(op.duplicate||(op.done&&fields[op.field].Input().create_live_owner))continue;
      {
        const auto &plan=scene->State();const auto &field=plan.Field(op.field);
        const auto &record=plan.Records()[field.record];const auto index=record.object;
        if(index>=objects.size())return Fail("binary_live_object_index");
        if(Rejected(index)||objects[index].unbound)continue;
#ifdef ISAACCS_E2E
        if(Pure(op)&&record.stream==S::BodyStream::World&&
            plan.Objects()[index].domain==S::ClientInstallDomain::Entity&&
            (ClientEntityTestFault("binary-object-write")||
             (objects[index].identity.layout==F::EntityTear::kLayout&&ClientTearWriteTestFault(objects[index].identity.id)))&&
            RejectField(op,"binary_e2e_object_write"))continue;
#endif
        const auto mask=std::uint32_t{1}<<path_facts[field.path].root_block;
        // 差分基于该流上次应用的值。保留附件的用途登记及显示重建字段仍交原 owner。
        if(!display&&record.context.base_version&&objects[index].borrowed&&field_index->records[field.record].has_previous_write&&
            Pure(op)&&!op.late&&BoundLeafHandler(field.definition)!=LeafHandler::Expire&&
            !(mask&(record.value_mask|record.structure_mask|record.dependency_mask))){
          bool retained=op.staged_element==S::kClientPlanNone||fields[op.field].Input().retained;
          for(auto path=plan.Paths()[field.path].parent;retained&&path!=S::kClientPlanNone;path=plan.Paths()[path].parent){
            const auto owner=FieldAt(index,path);
            if(owner!=S::kClientPlanNone&&fields[owner].native&&!fields[owner].Input().retained)retained=false;
          }
          if(retained){if(record.stream==S::BodyStream::World)skipped_world_bytes+=op.write.size;continue;}
        }
        const auto current=[&](std::uint32_t object_index){
          const auto &object=objects[object_index];
          return !object.borrowed||preparation.PendingObjectCurrent(*batch,object_index,object.address,object.instance);
        };
        // 纯值拷贝不会改换原生拥有者。同一连续段复用已核对的地址，其他操作重新见证。
        if(!Pure(op))checked_copy_object=S::kClientPlanNone;
        if(checked_copy_object!=index||!copies_left){
          if(!current(index)){
            if(RejectObject(index,ClientMemberFailure::Stale,"binary_live_object_replaced"))continue;
            return Fail("binary_live_object_replaced");
          }
          if(Pure(op))checked_copy_object=index;
        }
        if(op.reference!=S::kClientPlanNone&&(op.reference>=objects.size()||
            Rejected(op.reference)||objects[op.reference].unbound||!current(op.reference))){
          // 编号仍指向纯值计划里的完整键，原生槽先写空，目标重新出现时可补挂。
          op.reference=S::kClientPlanNone;op.write.reference=nullptr;op.deferred_target_instance=0;
        }
      }
      const bool managed_attachment=!op.delegated&&op.write.kind==ClientAtomicWriteKind::FieldAttachment;
      if(Pure(op)){
        if(!copies_left){
          // 区分后台撤销票据、原生对象／图形上下文失效和仍在借用纯值的提交队列。
          if(!Current(*batch))return Fail("binary_live_copy_target_changed");
          if(!ClientAtomicScopeActive())return Fail("binary_live_copy_scope_lost");
          if(!ClientAtomicQueuesEmpty())return Fail("binary_live_copy_cpu_borrow");
          copies_left=128;
        }
        --copies_left;
        if(!ClientAtomicWritePreflight(op.write,read_region)){
          checked_copy_object=S::kClientPlanNone;
          if(RejectField(op,"binary_live_write_preflight"))continue;
          return Fail("binary_live_write_preflight");
        }
        // 只保存撤场需要的原版 Game／Room 输入，实体纯值直接写入。
        if(root_scalars&&root_scalars->NeedsSnapshot(roots,op.write.destination,op.write.size)&&
            !root_scalars->Save(op.write,read_region))return Fail("binary_live_restore_root_snapshot");
        bool attempted=false;
        if(!ClientAtomicWriteApply(op.write,&attempted)){
          // 写入作用域在进入拷贝前失效时，本字段没有修改；保留此前真实写入的状态。
          if(!attempted)return Fail("binary_live_native_write_precondition");
          checked_copy_object=S::kClientPlanNone;
          written=true;
          if(RejectField(op,"binary_live_native_write"))continue;
          return Fail("binary_live_native_write");
        }
        written=true;
        op.done=true;pure_write_bytes+=op.write.size;
      }else if(!(managed_attachment?Current(*batch):callback_boundary())){
        return Fail("binary_atomic_lifecycle_during_install");
      }else if(op.delegated){
        if(op.late&&op.priority<=3){
          try {
            if(!CreateEarlyParents(op))return Fail("binary_live_early_field_parents");
          }catch(...){return Fail("binary_live_early_field_parent_path");}
          if(Rejected(ObjectOf(op)))continue;
          if(!callback_boundary())return Fail("binary_atomic_lifecycle_after_parent");
        }
        op.done=true;
        const auto result=bindings.InstallField(*batch,Inputs(),op.field);
#ifdef ISAACCS_E2E
        if(!result.ready){
          const auto &field=S::Fields()[scene->State().Field(op.field).definition];
          Trace("client_error","event=binary_live_delegated_failure layout=%u field=%u order=%u reason=%s",
            unsigned(field.layout),unsigned(field.index),op.priority,result.reason);
        }
#endif
        if(!result.ready&&RejectField(op,result.reason)){written|=result.written;continue;}
        if(!Step(result))return Failed(written,reason);
      }else{
        const bool attachment=op.write.kind==ClientAtomicWriteKind::FieldAttachment;
        const bool reference=op.write.kind==ClientAtomicWriteKind::EntityReference;
        if(!ClientAtomicWritePreflight(op.write,read_region)){
          if(RejectField(op,"binary_atomic_write_preflight_changed"))continue;
          return Fail("binary_atomic_write_preflight_changed");
        }
        // 已相等的引用无需重复更新原生反向树。
        if(reference&&ClientAtomicWriteCheck(op.write)){op.done=true;continue;}
        const bool was_written=written,was_done=op.done;
        if(!op.write.retained_attachment){written=true;op.done=true;}
        bool attempted=false;
        if(!ClientAtomicWriteApply(op.write,&attempted)){
          // 失败现场：字段身份、写入类别，以及作用域／原生图像队列是否仍成立，供定位真实原因。
          {const auto &field=S::Fields()[scene->State().Field(op.field).definition];
            Trace("client_error","event=binary_atomic_write_failed layout=%u field=%u kind=%u attempted=%u scope=%u queues_empty=%u order=%u",
              unsigned(field.layout),unsigned(field.index),unsigned(op.write.kind),unsigned(attempted),
              unsigned(ClientAtomicScopeActive()),unsigned(ClientAtomicQueuesEmpty()),op.priority);}
          if(!attempted){
            // 未进入原生写入：对象未改动，恢复登记，按安装期生命周期变化干净失败，
            // 不把引用标成“不确定”——否则批次永远无法排空，旧场景退休卡死（operation_execution_no_progress）。
            written=was_written;op.done=was_done;
            return Fail("binary_atomic_lifecycle_during_install");
          }
          if(attachment&&!ClientStagedFieldAttached(op.write.field,op.write.destination))op.done=false;
          if(RejectField(op,"binary_atomic_native_write_failed")){read_region.Clear();continue;}
          if(reference)uncertain=true;
          return Fail("binary_atomic_native_write_failed");
        }
        if(attachment&&fields[op.field].Input().update_live&&!fields[op.field].Input().resize_live_sequence){
          written=true;op.done=true;
          if(!ClientStagedFieldInstallValues(op.write.field,fields[op.field].Input())){
            if(RejectField(op,"binary_live_container_values")){read_region.Clear();continue;}
            return Fail("binary_live_container_values");
          }
        }
        op.done=true;
        if(!(managed_attachment?Current(*batch):callback_boundary()))return Fail("binary_atomic_lifecycle_after_callback");
      }
    }
    if(!Current(*batch))return Fail("binary_atomic_lifecycle_during_install");
    if(!display){
      // 纯值段之后写被吸收记录；它们都是借用的 Entity／Grid，不经过 Init，也没有显示路径叶。
      if(!callback_boundary())return Fail("binary_atomic_lifecycle_during_install");
      const auto result=RunAbsorbed();
      if(!result.ready)return result;
      if(!initialized){
        if(!Step(bindings.Initialize(*batch,Inputs())))return Failed(written,reason);
        initialized=true;
      }
      if(!InstallFlat(FlatPass::Values))return Fail("flat_native_value_window");
    }
    {
      S::ClientApplyTimer measured(memory->ApplyMetrics(),metric_streams,S::ClientApplyStage::NativeFinish);
      if(!Step(bindings.FinishFields(Inputs(),S::StateTargetStreams(scene->State().Target()))))return Failed(written,reason);
    }
    if(display){display_installed=true;return Ready(written,"binary_live_display_fields_installed");}
    if(!InstallFlat(FlatPass::Graphics)||!InstallFlat(FlatPass::References))return Fail("flat_native_finish_window");
    if(!Step(bindings.InstallMembers(*batch,Inputs())))return Failed(written,reason);
    installed=true;display_installed=!deferred_display;reason="binary_atomic_installed";return Ready(written,reason);
  }
  S::ClientStateNativeStep PublishNow() noexcept {
    PageMetrics pages(*this);
    publish_entered=true;if(!Step(bindings.Publish(*batch,Inputs())))return Failed(written,reason);
#ifdef ISAACCS_E2E
      // 诊断只抽样，记录失配而不改写、撤显或干预接收 ACK。
      if(!operations.empty()&&manager_token%16==0)try {
        ReadMetrics measured(*this,S::StateTargetStreams(scene->State().Target()));
        PurePathWindow window(*this);
        if(window.ready)for(std::size_t n=0;n<(std::min)(operations.size(),std::size_t{16});++n){
          const auto &op=Ordered((manager_token/16+n*(operations.size()/16+1))%operations.size());
          if(!op.done||op.restored||!Pure(op))continue;
          const auto read=ReadOp(op,false);
          if(read==ClientAtomicReadback::Unsafe||read==ClientAtomicReadback::Drift)
            FieldIssue(op,"publish_sample","binary_live_publish_sample");
        }
      }catch(...){Trace("client_error","event=binary_publish_sample_failed");}
#endif
    // 本批按对象隔离失败的对象可能只写了一部分。发布前把它们在索引里标为待重写，
    // 下一批既不判为静止记录，也不走未变块跳过，整对象完整写一次（设计 10.5 节失败方式 10）。
    if(field_index){
      const auto &plan=scene->State();
      for(std::uint32_t n=0;n<field_index->records.size()&&n<plan.Records().size();++n){
        const auto object=plan.Records()[n].object;
        if(object<objects.size()&&objects[object].failure!=ClientMemberFailure::None)field_index->records[n].rewrite=true;
      }
    }
    return Ready(written,"binary_live_values_published");
  }
  // 发布成功后刷新写程序表（设计 10.6 节）：本批描述过的记录用本批登记，静止／被吸收记录沿用旧条目，
  // 其余（失败、缺条目）不建条目并把表标为不完整。只含 Body 的批次不改表。任何分配失败都清空整表。
  void RefreshStore() noexcept {
    if(!store)return;
    const auto &plan=scene->State();
    if(!(S::StateTargetStreams(plan.Target())&1))return;
    try {
      const auto members=plan.MemberRecordCount();
      if(members==S::kClientPlanNone){store->Clear();return;}
      auto &old=*store;
      const auto *previous=old.plan?&old.plan->State():nullptr;
      decltype(old.records) records(old.records.get_allocator());
      decltype(old.leaves) leaves(old.leaves.get_allocator());
      decltype(old.carriers) carriers(old.carriers.get_allocator());
      decltype(old.marks) marks(old.marks.get_allocator());
      // 登记按记录分组；同一记录内保持登记顺序（Time 在前，叶按值序）。
      const auto by_record=[](const auto &a,const auto &b){return a.record<b.record;};
      std::stable_sort(capture_leaves.begin(),capture_leaves.end(),by_record);
      std::stable_sort(capture_carriers.begin(),capture_carriers.end(),by_record);
      bool complete=true;
      for(std::uint32_t n=0;n<members;++n){
        const auto &r=plan.Records()[n];
        if(r.flat){complete=false;continue;}
        if(r.stream!=S::BodyStream::World||r.object>=objects.size())continue;
        const auto &o=objects[r.object];
        if(!o.address)continue;
        if(o.unbound||o.failure!=ClientMemberFailure::None){complete=false;continue;}
        ClientSceneMemberStore::Record e;
        e.identity=o.identity;e.instance=o.instance;e.address=o.address;e.record=n;
        e.value_count=r.value_count;e.field_count=r.field_count;
        e.first_leaf=std::uint32_t(leaves.size());e.first_carrier=std::uint32_t(carriers.size());e.first_mark=std::uint32_t(marks.size());
        if(!Quiet(n)&&n<capture_state.size()&&capture_state[n]!=2){
          for(std::uint32_t local=0;local<r.value_count;++local)marks.push_back(value_pure[r.first_value+local]);
          for(std::uint32_t local=0;local<r.field_count;++local)marks.push_back(field_pure[r.first_field+local]);
          const auto key=CaptureLeaf{n,{}};
          for(auto [at,end]=std::equal_range(capture_leaves.begin(),capture_leaves.end(),key,by_record);at!=end;++at){
            leaves.push_back(at->leaf);e.display|=at->leaf.display;
          }
          const auto ckey=CaptureCarrier{n,{}};
          for(auto [at,end]=std::equal_range(capture_carriers.begin(),capture_carriers.end(),ckey,by_record);at!=end;++at){
            const auto first=carriers.begin()+e.first_carrier;
            if(std::none_of(first,carriers.end(),[&](const auto &c){return c.native==at->carrier.native;}))carriers.push_back(at->carrier);
          }
        }else if(Quiet(n)&&previous){
          const auto *before=old.Find(o.identity);
          if(!before||before->instance!=o.instance||before->address!=o.address||!plan.SameRecordShape(n,*previous,before->record)){
            complete=false;continue;
          }
          marks.insert(marks.end(),old.marks.begin()+before->first_mark,
            old.marks.begin()+before->first_mark+before->value_count+before->field_count);
          leaves.insert(leaves.end(),old.leaves.begin()+before->first_leaf,old.leaves.begin()+before->first_leaf+before->leaf_count);
          carriers.insert(carriers.end(),old.carriers.begin()+before->first_carrier,
            old.carriers.begin()+before->first_carrier+before->carrier_count);
          e.display=before->display;
        }else{complete=false;continue;}
        e.leaf_count=std::uint32_t(leaves.size())-e.first_leaf;
        e.carrier_count=std::uint32_t(carriers.size())-e.first_carrier;
        records.push_back(e);
      }
      std::sort(records.begin(),records.end(),[](const auto &a,const auto &b){return a.identity<b.identity;});
      std::uint32_t version=0;
      for(unsigned i=0;i<plan.Target().count;++i)
        if(plan.Target().sections[i].stream==S::BodyStream::World)version=plan.Target().sections[i].target_version;
      old.records.swap(records);old.leaves.swap(leaves);old.carriers.swap(carriers);old.marks.swap(marks);
      old.plan=scene;old.route=plan.Target().route;old.version=version;old.complete=complete;++old.generation;
      memory->ApplyMetrics().Volume(1,S::ClientApplyVolume::StoreRecords,old.records.size());
    }catch(...){store->Clear();}
  }
  S::ClientStateNativeStep CommitFields() noexcept {
    for(const auto n:attachment_operations){
      const auto &op=operations[n];
      if(op.duplicate||op.restored||Rejected(ObjectOf(op))||op.write.retained_attachment)continue;
      const auto result=ClientStagedFieldCommitCheckedAttachment(op.write.field,op.write.destination);
      // 已完成的附件折叠与尚未折叠的前驱链均能按现有逆序拆卸。
      if(result==ClientAttachmentCommit::ScopeLost)return Fail("binary_atomic_attachment_commit_scope_lost");
      if(result==ClientAttachmentCommit::InvalidChain){
        uncertain=true;
        return Fail("binary_atomic_attachment_commit_failed");
      }
    }
    return Ready(written,"binary_atomic_fields_committed");
  }
  // 原生装载和显示可能更换晚绑定承载；提交时必须先确认观察的是当前地址。
  bool WriteSlotCurrent(const Op &op) noexcept {
    try {
      void *slot=nullptr;
      std::size_t extent=0;
      if(!Destination(op.field,slot,extent))return false;
      if(op.staged_element!=S::kClientPlanNone){
        auto *field=fields[op.field].native;
        if(!ClientStagedFieldAttached(field,slot))return false;
        return ClientStagedFieldElement(field,op.staged_element)==op.write.destination;
      }
      const auto offset=op.element==S::kClientPlanNone?0:std::size_t(op.element)*op.element_stride;
      return offset<=extent&&op.write.size<=extent-offset&&static_cast<std::byte *>(slot)+offset==op.write.destination;
    }catch(...){return false;}
  }
  ClientAtomicReadback ReadOp(const Op &op,bool check_owner=true) {
    using R=ClientAtomicReadback;
    if(op.duplicate)return R::Untracked;
    const auto &plan=scene->State();const auto &f=plan.Field(op.field);
    const auto object=plan.Records()[f.record].object;
    if(S::FindLayout(plan.Records()[f.record].layout)->protocol)return R::Untracked;
    if(object>=objects.size())return R::Unsafe;
    if(op.restored||objects[object].unbound||!objects[object].address)return R::Untracked;
    if(check_owner&&!preparation.InstalledObjectCurrent(*batch,object,objects[object].address))return R::Unsafe;
    if(check_owner&&op.reference!=S::kClientPlanNone&&
        !preparation.InstalledObjectCurrent(*batch,op.reference,op.write.reference))return R::Unsafe;
    // 动态子地址只有在仍由本批容器承载时可读。任一父层换址都交完整恢复。
    for(auto p=plan.Paths()[f.path].parent;p!=S::kClientPlanNone;p=plan.Paths()[p].parent){
      const auto n=FieldAt(object,p);
      if(n==S::kClientPlanNone)continue;
      const auto attachment=field_attachments[n];
      if(attachment==S::kClientPlanNone)continue;
      const auto &owner=Ordered(attachment).write;
      void *slot=nullptr;std::size_t extent=0;
      if(!Destination(n,slot,extent)||slot!=owner.destination||!ClientStagedFieldAttached(owner.field,slot))return R::Unsafe;
    }
    if(op.delegated){++read_checks;return bindings.ReadField(Inputs(),op.field);}
    if(!WriteSlotCurrent(op))return R::Unsafe;
    ++read_checks;read_bytes+=op.write.size;
    return ClientAtomicWriteCheck(op.write)?R::Match:R::Drift;
  }
  S::ClientStateNativeStep ScanDrift(const ClientSceneFieldIndex &published,S::BodyStream stream,std::size_t &cursor,bool costumes) noexcept {
    using R=ClientAtomicReadback;
    const auto streams=std::uint8_t(1u<<unsigned(stream));ReadMetrics measured(*this,streams);
    try {
      if(!costumes&&stream==S::BodyStream::World&&!published.flat_records.empty()){
        const auto tick=std::uint64_t(published.versions[0])+flat_drift_cursor++;
        const auto index=published.flat_records[tick%published.flat_records.size()];
        const auto &current=published.records[index];
        const auto &record=scene->State().Records()[index];
        if(!current.rewrite&&current.flat&&current.flat->written&&FlatCurrent(record.object,true)){
          FlatWindow window{*this,record.object,true};
          const auto context=FlatContext(window,record);
          const auto *layout=S::FindFlatLayout(record.layout);
          const auto probe=ClientCheckFlat(context,objects[record.object].address,*layout,*record.flat,
            *current.flat,std::uint32_t(tick/published.flat_records.size()),false,true);
          if(probe.reason){current.rewrite=true;memory->ApplyMetrics().Volume(streams,S::ClientApplyVolume::Drift);}
        }
      }
      const auto &readback=published.order->readback;
      const auto group=(std::uint64_t(costumes)<<8)|std::uint64_t(stream);
      const auto first=std::lower_bound(readback.begin(),readback.end(),group,[](const auto &e,auto s){return (e.key>>40)<s;});
      const auto last=std::upper_bound(first,readback.end(),group,[](auto s,const auto &e){return s<(e.key>>40);});
      const auto count=std::size_t(last-first);
      if(!count)return Ready(false,"binary_drift_empty");
      PurePathWindow window(*this);
      if(!window.ready)return Fail("binary_drift_read_capacity");
      for(std::size_t n=0;n<(std::min)(count,std::size_t{128});++n){
        const auto &entry=first[cursor%count];++cursor;
        const auto &record=published.records[entry.Record()];
        if(record.rewrite)continue;
        if(!Current(*batch))return Fail("binary_drift_revoked");
        // 快速帧换过计划后，纯值指令里的旧字节已过时；纯值由每帧全量写覆盖，只读回委托字段。
        // Time 的具名写入器记的是旧批帧号，快速帧已按新帧号改写，同样不读回。
        if(fast_swapped){
          const auto &op=Ordered(entry.operation);
          if(Pure(op)||(op.delegated&&bindings.FrameField(bindings.DescribeField(Inputs(),op.field))))continue;
        }
        const auto result=ReadOp(Ordered(entry.operation));
        if(result!=R::Drift&&result!=R::Unsafe)continue;
        record.rewrite=true;
        memory->ApplyMetrics().Volume(streams,S::ClientApplyVolume::Drift);
      }
      return Ready(false,"binary_drift_scanned");
    }catch(...){return Fail("binary_drift_exception");}
  }
  bool UnbindRetiring(std::span<const ClientLiveRetirement> leaving) noexcept {
    const auto retiring=[&](const ClientAtomicObject &object){
      const auto at=std::lower_bound(leaving.begin(),leaving.end(),object.identity,
        [](const auto &entry,const auto &id){return entry.identity<id;});
      return at!=leaving.end()&&at->identity==object.identity&&at->instance==object.instance&&at->address==object.address;
    };
    const auto &plan=scene->State();
    if(field_index)for(const auto n:field_index->flat_records){
      const auto &record=plan.Records()[n];
      const auto &object=objects[record.object];
      if(!object.address||object.unbound)continue;
      auto &state=*field_index->records[n].flat;
      const bool root_leaving=retiring(object);
      const bool external=(!leaving.empty()&&state.references_uncertain)||
        std::any_of(state.edges.begin(),state.edges.end(),[&](const auto &edge){
          const auto at=std::lower_bound(leaving.begin(),leaving.end(),edge.identity,
            [](const auto &entry,const auto &id){return entry.identity<id;});
          return at!=leaving.end()&&at->identity==edge.identity&&at->address==edge.address&&at->instance==edge.instance;
        });
      if(!root_leaving&&!external)continue;
      // 失败根会在首批发布前回收，此时证言来自准备中的保管账。
      if(published){
        std::uint64_t instance=0;
        if(preparation.InstalledObject(*batch,record.object,&instance)!=object.address||instance!=object.instance)return false;
      }else if(root_leaving){
        if(!preparation.OwnsFailedObject(*batch,record.object,object.address,object.instance))return false;
      }else if(!preparation.PendingObjectCurrent(*batch,record.object,object.address,object.instance,true))return false;
      const auto *layout=S::FindFlatLayout(record.layout);
      if(!layout||!ClientClearFlat(object.address,*layout,state,root_leaving,leaving))return false;
    }
    // 先拆边，再按安装逆序摘附件。父容器恢复原头部后，旧元素地址可能已失效。
    for(auto &op:operations){
      if(!op.done||op.restored||op.duplicate)continue;
      const auto i=plan.Records()[plan.Field(op.field).record].object;
      if(!objects[i].address)continue;
      if(op.delegated){
        if(!bindings.ClearReferenceTo(Inputs(),op.field,leaving))return false;
      }else if(op.write.kind==ClientAtomicWriteKind::EntityReference){
        if(!op.write.reference)continue;
        const bool target_leaving=op.reference!=S::kClientPlanNone?retiring(objects[op.reference]):
          std::any_of(leaving.begin(),leaving.end(),[&](const auto &entry){
            return entry.address==op.write.reference&&entry.instance==op.deferred_target_instance;
          });
        if(!retiring(objects[i])&&!target_leaving)continue;
        if(WriteSlotCurrent(op)&&!ClientAtomicClearRetiringReference(op.write.destination,op.write.reference,true))return false;
        op.write.reference=nullptr;op.reference=S::kClientPlanNone;
        op.deferred_target_instance=0;
      }
    }
    for(std::size_t n=operations.size();n>0;--n){
      auto &op=Ordered(n-1);
      if(!op.done||op.restored)continue;
      const auto i=plan.Records()[plan.Field(op.field).record].object;
      if(!retiring(objects[i]))continue;
      if(op.delegated&&!bindings.UndoField(*batch,Inputs(),op.field))return false;
      if(!op.delegated&&op.write.kind==ClientAtomicWriteKind::FieldAttachment&&
          !ClientAtomicDetachField(op.write))return false;
      op.restored=true;
    }
    for(auto &object:objects)if(retiring(object))object.unbound=true;
    return bindings.UnbindMemberOverlays(leaving);
  }
  void ResolveDeferredBodyReferences(std::span<const ClientMorphedMember> morphed) noexcept {
    const auto &plan=scene->State();
    for(auto &op:operations){
      if(op.restored||!op.done||op.duplicate||op.reference_value==S::kClientPlanNone||op.write.reference)continue;
      const auto &field=plan.Field(op.field);const auto &record=plan.Records()[field.record];
      if(record.stream!=S::BodyStream::Body||objects[record.object].unbound)continue;
      if(!Current(*batch))return;
      const auto link=plan.BindingForValue(field.record,op.reference_value);
      if(!link||!preparation.InstalledObjectCurrent(*batch,record.object,objects[record.object].address)||!WriteSlotCurrent(op))continue;
      std::uint64_t instance=0;
      auto *address=preparation.FindLiveMember(link->member,instance);
      if(!address){
        const auto found=std::lower_bound(morphed.begin(),morphed.end(),link->member,
          [](const auto &entry,const auto &identity){return entry.before<identity;});
        if(found!=morphed.end()&&found->before==link->member){
          address=preparation.FindLiveMember(found->after,instance);
          if(address!=found->address||instance!=found->instance)address=nullptr;
        }
      }
      if(!address)continue;
      auto write=op.write;write.reference=address;
      ClientAtomicReadRegion region;
      if(!ClientAtomicWritePreflight(write,region)||!ClientAtomicWriteApply(write))continue;
      op.write.reference=address;op.deferred_target_instance=instance;
    }
  }
  // 整批快速帧资格（设计 10.6 节条件 1—5）：只读数字计划和本安装器状态，返回首个不满足的原因。
  // 可变状态部分：本安装器仍是当前发布者、写程序表完整且版本对得上。写段入口只复查这一部分，
  // 结构比较在准备阶段对同一张不可变票据做过一次。
  const char *FastStateNow(const S::ClientStateApplyBatch &ticket) noexcept {
    if(!batch||!published||invalid||uncertain||!Current(*batch))return "binary_fast_writer_not_current";
    if(S::StateTargetStreams(batch->target.candidate->Target())!=1)return "binary_fast_writer_joint";
    if(!store||!store->complete||store->plan!=scene)return "binary_fast_store_incomplete";
    if(!store->Usable(scene->State().Target().route,ticket.application_bases[0]))return "binary_fast_store_version";
    for(const auto &o:objects)if(o.failure!=ClientMemberFailure::None||(o.unbound&&o.address))return "binary_fast_object_failure";
    return nullptr;
  }
  const char *FastCheckNow(const S::ClientStateApplyBatch &ticket) noexcept {
    try {
      if(const auto *why=FastStateNow(ticket))return why;
      const auto next=std::static_pointer_cast<const ClientSceneInstallPlan>(ticket.native);
      if(!next||&next->State()!=ticket.target.install.get())return "binary_fast_plan_type";
      const auto &a=scene->State();const auto &b=next->State();
      if(b.Target().route!=a.Target().route||S::StateTargetStreams(b.Target())!=1)return "binary_fast_route";
      if(!b.SameStructure(a))return "binary_fast_structure";
      // 显示目标同房、无转场，房型与构图注入不变（失败方式 4）。
      if(!next->Display()||!scene->Display()||!next->Rooms()||!scene->Rooms())return "binary_fast_display_missing";
      {
        const auto x=ClientDisplayMeta(scene->Display()),y=ClientDisplayMeta(next->Display());
        if(x.scope.run!=y.scope.run||x.scope.floor!=y.scope.floor||x.scope.dimension!=y.scope.dimension||x.scope.room!=y.scope.room||
           x.scope.scene!=y.scope.scene||x.room_use!=y.room_use||x.room_shape!=y.room_shape||x.grid!=y.grid||x.dimension!=y.dimension||
           x.spawn_seed!=y.spawn_seed||x.transition_mode||y.transition_mode)return "binary_fast_display_room";
        const auto p=ClientDisplayInputs(scene->Display()),q=ClientDisplayInputs(next->Display());
        if(p.room_graphics_flags!=q.room_graphics_flags||p.backdrop_wall_style!=q.backdrop_wall_style||
           p.backdrop_tutorial!=q.backdrop_tutorial||p.water!=q.water)return "binary_fast_display_graphics";
        const auto u=scene->Rooms()->Room().Values(),v=next->Rooms()->Room().Values();
        if(u.type!=v.type||u.grid_width!=v.grid_width||u.grid_height!=v.grid_height||u.bounds!=v.bounds||
           u.descriptor_slot!=v.descriptor_slot||u.dimension!=v.dimension)return "binary_fast_room_values";
      }
      for(const auto &need:b.Resources()){
        const auto at=std::lower_bound(resources.begin(),resources.end(),need.key,[](const auto &r,auto k){return r.key<k;});
        if(at==resources.end()||at->key!=need.key)return "binary_fast_resource";
      }
      const auto members=b.MemberRecordCount();
      for(std::uint32_t n=0;n<members;++n){
        const auto &r=b.Records()[n];
        if(r.stream!=S::BodyStream::World)return "binary_fast_record_stream";
        if(r.structure_mask|r.dependency_mask)return "binary_fast_record_structure";
        if(!r.value_mask)continue;
        if(r.object>=objects.size()||!objects[r.object].address)return "binary_fast_logical_record_changed";
        using Domain=S::ClientInstallDomain;
        const auto domain=b.Objects()[r.object].domain;
        if(domain==Domain::Level||domain==Domain::Descriptor||domain==Domain::Participant||domain==Domain::ControlGroup)
          return "binary_fast_root_record_changed";
        const auto &o=objects[r.object];
        const auto *entry=store->Find(o.identity);
        if(!entry||entry->record!=n||entry->instance!=o.instance||entry->address!=o.address)return "binary_fast_store_entry";
        if(!store->SameFixed(b,n,*entry))return "binary_fast_named_value_changed";
      }
      return nullptr;
    }catch(...){return "binary_fast_check_exception";}
  }
  // 写前核原生现场（条件 7）：根未换，全部有地址的对象仍是同一安装实例，写程序登记的承载仍接在活对象上。
  const char *FastValidateNow() noexcept {
    const auto current=ClientPreparationRoots();
    if(current.game!=roots.game||current.room!=roots.room)return "binary_fast_roots";
    for(std::uint32_t n=0;n<objects.size();++n){
      const auto &o=objects[n];
      if(!o.address)continue;
      std::uint64_t instance=0;
      if(preparation.InstalledObject(*batch,n,&instance)!=o.address||instance!=o.instance)return "binary_fast_object_replaced";
    }
    for(const auto &e:store->records)if(!CarriersAttached(e))return "binary_fast_carrier_detached";
    return nullptr;
  }
  // 快速帧写段：写全部非显示纯值叶、Time 和帧号，然后换成新计划并更新字段索引版本。
  S::ClientStateNativeStep FastInstallNow(const S::ClientStateApplyBatchRef &ticket,
      const std::shared_ptr<const ClientSceneInstallPlan> &next) noexcept {
    try {
      if(!ClientAtomicCopyWindow())return Failed(false,"binary_fast_copy_window");
      if(const auto *why=FastValidateNow())return Failed(false,why);
      S::ClientApplyTimer measured(memory->ApplyMetrics(),1,S::ClientApplyStage::FastWrite);
      const auto &plan=next->State();
      std::uint64_t count=0;
      for(const auto &e:store->records){
        const auto before=count;
        const bool ok=RunRecord(e,plan,e.record,false,count);
        written|=count!=before;
        if(!ok)return Failed(true,"binary_fast_write");
      }
      written=true;
      if(!bindings.AdvancePublished(*next,next->Display()->Metadata().frame))return Failed(true,"binary_fast_frame");
      // 换计划：原票据仍持有旧计划，旧指令借用的字节保持存活（失败方式 5）。
      scene=next;fast_swapped=true;fast_ticket=ticket;
      for(unsigned i=0;i<plan.Target().count;++i)
        if(plan.Target().sections[i].stream==S::BodyStream::World)field_index->versions[0]=plan.Target().sections[i].target_version;
      memory->ApplyMetrics().Volume(1,S::ClientApplyVolume::FastLeaves,count);
      return Ready(true,"binary_fast_installed");
    }catch(...){return Failed(written,"binary_fast_exception");}
  }
  // 显示安装之后写显示路径上的纯值叶（Room 构图会重置这些位置）。
  S::ClientStateNativeStep FastDisplayNow() noexcept {
    if(!ClientAtomicCopyWindow())return Failed(true,"binary_fast_display_copy_window");
    S::ClientApplyTimer measured(memory->ApplyMetrics(),1,S::ClientApplyStage::FastWrite);
    std::uint64_t count=0;
    for(const auto &e:store->records)if(e.display&&!RunRecord(e,scene->State(),e.record,true,count))
      return Failed(true,"binary_fast_display_write");
    memory->ApplyMetrics().Volume(1,S::ClientApplyVolume::FastLeaves,count);
    return Ready(true,"binary_fast_display_installed");
  }
  S::ClientStateNativeStep DrainNow() noexcept {
    if(!install_entered&&!publish_entered)return Ready(false,"binary_atomic_staged_only");
    if(uncertain)return Failed(written,"binary_atomic_uncertain_reference_retained");
    // 退休责任取当前保管账，不按本批当初Join／Reuse推断；成功后根可跨批转交。
    if(!preparation.VisitRetirementObjects(*batch,this,[](void *p,std::uint32_t n,bool retire_root)noexcept{
      auto &s=*static_cast<Impl *>(p);if(n>=s.objects.size())return false;s.objects[n].retire_root=retire_root;return true;
    }))return Failed(written,"binary_atomic_retirement_ownership_unavailable");
    if(!quiesced){if(!bindings.Quiesce(*batch,Inputs()))return {false,false,false,"binary_atomic_quiescing"};quiesced=true;}
    if(field_index)for(const auto n:field_index->flat_records){
      const auto &record=scene->State().Records()[n];
      const auto &object=objects[record.object];
      auto &state=*field_index->records[n].flat;
      if(state.undo.storage||state.undo.uncertain){
        if(!ClientBinaryCommitFlatSpecial(state.interrupted,state.undo))return Failed(written,"flat_native_retire_special");
        state.special_charge.reset();
      }
      if(!object.retire_root||!object.address||object.unbound)continue;
      const auto *layout=S::FindFlatLayout(record.layout);
      if(!layout||!ClientClearFlat(object.address,*layout,state,true,{}))return Failed(written,"flat_native_retire_references");
    }
    // 装载／选择器可能交换图层数组。按照安装顺序统一反向拆除普通与特殊字段。
    for(std::size_t n=operations.size();n>0;--n){auto &op=Ordered(n-1);if(!op.done||op.restored)continue;
      if(op.delegated){
        if(!bindings.UndoField(*batch,Inputs(),op.field))return Failed(written,"binary_atomic_special_retirement_failed");
        op.restored=true;continue;
      }
      if(op.write.kind==ClientAtomicWriteKind::FieldAttachment&&
          (!bindings.RetirementCurrent(*batch,Inputs())||!ClientAtomicDetachField(op.write)))
        return Failed(written,"binary_live_attachment_retirement");
      op.restored=true;
    }
    if(!retired){if(!bindings.Retire(*batch,Inputs()))return {false,false,false,"binary_atomic_native_retiring"};retired=true;}
    if(final_scene&&root_scalars&&!root_scalars->Restore(roots))return Failed(written,"binary_atomic_root_scalar_retirement");
    return Ready(written,"binary_atomic_native_drained");
  }
};
ClientSceneAtomicInstaller::ClientSceneAtomicInstaller(std::shared_ptr<S::ClientStateMemory> memory,ClientSceneMainPreparation &p,
    std::shared_ptr<RootScalars> scalars,const std::array<ClientSceneFieldIndexRef,2> &published,
    std::shared_ptr<ClientSceneMemberStore> store,bool absorb){
  if(!memory)throw S::ClientTableError(S::ClientTableFailure::Internal,"binary_atomic_memory");
  impl_=std::make_unique<Impl>(std::move(memory),p,nullptr,std::move(scalars),published,std::move(store),absorb);
}
const char *ClientSceneAtomicInstaller::FastCheck(const S::ClientStateApplyBatch &ticket) noexcept {
  auto &s=*impl_;
  if(!s.Thread())return "binary_fast_wrong_thread";
  return s.FastCheckNow(ticket);
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::FastInstall(const S::ClientStateApplyBatchRef &ticket) noexcept {
  auto &s=*impl_;
  if(!ticket||!s.Thread()||!s.batch)return Failed(false,"binary_fast_install_owner");
  // 写前在 Manager 写段内复查可变状态：准备与写入之间可能已有回调改了本安装器状态。
  if(const auto *why=s.FastStateNow(*ticket))return Failed(false,why);
  if(!ticket->native||&static_cast<const ClientSceneInstallPlan &>(*ticket->native).State()!=ticket->target.install.get())
    return Failed(false,"binary_fast_plan_type");
  const auto next=std::static_pointer_cast<const ClientSceneInstallPlan>(ticket->native);
  struct Work {Impl &owner;const S::ClientStateApplyBatchRef &ticket;const std::shared_ptr<const ClientSceneInstallPlan> &next;};
  Work work{s,ticket,next};
  const auto result=s.preparation.ApplyInstallation(*s.batch,&s,false,&work,[](void *context) noexcept {
    auto &w=*static_cast<Work *>(context);return w.owner.FastInstallNow(w.ticket,w.next);
  });
  if(result.ready&&!result.failed)return result;
  // 已写的失败使本安装器失效；调用方随后撤显并整场景退休，下一份完整基线重建。
  if(result.written){s.written=true;s.Fail(result.reason);}
  return Failed(result.written,result.reason);
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::FastDisplay(const S::ClientStateApplyBatch &ticket) noexcept {
  auto &s=*impl_;
  if(!s.Thread()||!s.batch||s.fast_ticket.get()!=&ticket)return Failed(true,"binary_fast_display_owner");
  const auto result=s.preparation.ApplyInstallation(*s.batch,&s,false,&s,[](void *p) noexcept {
    return static_cast<Impl *>(p)->FastDisplayNow();
  });
  if(result.ready&&!result.failed)return result;
  s.Fail(result.reason);return Failed(true,result.reason);
}
void ClientSceneAtomicInstaller::FastPublished(const S::ClientStateApplyBatch &ticket) noexcept {
  auto &s=*impl_;
  if(!s.Thread()||!s.store||s.fast_ticket.get()!=&ticket)return;
  // 同形换计划：条目的记录号、标记与叶不变，只把“上次写入值”的来源换成本票据的计划。
  const auto &target=s.scene->State().Target();
  std::uint32_t version=0;
  for(unsigned i=0;i<target.count;++i)if(target.sections[i].stream==S::BodyStream::World)version=target.sections[i].target_version;
  s.store->plan=s.scene;s.store->version=version;++s.store->generation;
}
bool ClientSceneAtomicInstaller::VisitPublished(void *context,StagedVisitor visit) noexcept {
  auto &s=*impl_;
  return visit&&s.Thread()&&s.published&&s.batch&&s.Current(*s.batch)&&visit(context,s.Inputs());
}
bool ClientSceneAtomicInstaller::Absorbing()const noexcept {return impl_->absorb;}
ClientSceneAtomicInstaller::~ClientSceneAtomicInstaller(){if(impl_&&(!impl_->Thread()||impl_->batch))impl_.release();}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Stage(S::ClientStateApplyBatchRef b) noexcept {
  auto &s=*impl_;if(!s.Thread()||!b||!b->target.install||b->target.install->Memory()!=s.memory||s.invalid)return Failed(s.written,"binary_atomic_stage_lifecycle");
  if(s.batch&&s.batch!=b)return Failed(s.written,"binary_atomic_batch_busy");
  if(!s.batch){s.batch=std::move(b);s.scene=std::dynamic_pointer_cast<const ClientSceneInstallPlan>(s.batch->native);
    if(!s.scene||&s.scene->State()!=s.batch->target.install.get())return s.Fail("binary_atomic_plan_type");}
  if(!s.Current(*s.batch))return s.Fail("binary_atomic_stage_revoked");
  if(s.staged)return Ready(false,"binary_atomic_staged");
  const auto streams=S::StateTargetStreams(s.scene->State().Target());
  // 主线程子阶段分别聚合，避免把描述构图、原生暂存与真正写入混为同一种成本。
  {
    S::ClientApplyTimer time(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::MainObjects);
    const auto roots=s.preparation.StageObjects(*s.batch);if(!roots.ready||roots.failed)return roots;
  }
  {
    S::ClientApplyTimer time(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::MainFields);
    const auto fields=s.preparation.StageFields(*s.batch);if(!fields.ready||fields.failed)return fields;
  }
  try {
    S::ClientApplyTimer time(s.memory->ApplyMetrics(),streams,S::ClientApplyStage::AtomicDescribe);
    s.Describe();
    for(std::uint32_t n=0;n<s.objects.size();++n){
      const auto failure=s.preparation.StagedFailure(*s.batch,n);
      if(failure==ClientMemberFailure::None)continue;
      s.objects[n].identity=s.scene->State().Objects()[n].member.identity;
      s.objects[n].failure=failure;
    }
    s.staged=true;
    return Ready(false,"binary_atomic_staged");
  }catch(const S::ClientShadowError &e){return s.Fail(e.what());}
   catch(const S::ClientTableError &e){
    S::ClientApplyMetrics::Issue issue;issue.route=s.batch->ticket.route.route;issue.ticket=s.batch->ticket.serial;
    issue.expected=e.Requested();issue.actual=e.Available();
    s.memory->ApplyMetrics().FirstIssue(streams,issue,"atomic_describe_budget",e.what());
    return s.Fail("binary_atomic_budget");
   }
   catch(...){return s.Fail("binary_atomic_description_exception");}
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Install(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Thread())return Failed(false,"binary_atomic_wrong_thread");if(s.batch.get()!=&b)return Failed(false,"binary_atomic_other_ticket");
  if(!s.Current(b)||!s.staged||s.install_entered)return s.Fail("binary_live_install_not_current");
  if(!s.InspectStaged())return s.Fail("binary_live_install_inputs_changed");
  const auto ready=s.bindings.Preflight(b,s.Inputs());
  if(!ready.ready||ready.failed||ready.written){s.written|=ready.written;return s.Fail(ready.reason);}
  s.roots=ClientPreparationRoots();
  if(!s.preparation.BeginLiveUse(b,&s))return s.Fail("binary_live_use_unavailable");
  s.lease=true;
  S::ClientApplyTimer time(s.memory->ApplyMetrics(),S::StateTargetStreams(s.scene->State().Target()),S::ClientApplyStage::NativeWrite);
  s.manager_token=OwnerToken(Owner::Manager);if(!s.manager_token)return s.Fail("binary_atomic_manager_token");
  const auto result=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->InstallNow();});
  s.written|=result.written;
  if(result.failed)return s.Fail(result.reason);return result;
}
bool ClientSceneAtomicInstaller::VisitStaged(const S::ClientStateApplyBatch &b,void *context,StagedVisitor visit) noexcept {
  auto &s=*impl_;
  return visit&&s.Current(b)&&s.staged&&!s.install_entered&&s.InspectStaged()&&visit(context,s.Inputs())&&s.preparation.StagedCurrent(b);
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::Publish(const S::ClientStateApplyBatch &b) noexcept {
  auto &s=*impl_;if(!s.Thread())return Failed(false,"binary_atomic_wrong_thread");if(s.batch.get()!=&b)return Failed(false,"binary_atomic_other_ticket");
  if(!s.Current(b)||!s.installed||!s.display_installed)return s.Fail("binary_atomic_publish_not_installed");
  if(s.published)return Ready(s.written,"binary_atomic_published");
  if(s.publish_entered||!s.manager_token||s.manager_token!=OwnerToken(Owner::Manager))return s.Fail("binary_atomic_publish_crossed_manager_window");
  S::ClientApplyTimer time(s.memory->ApplyMetrics(),S::StateTargetStreams(s.scene->State().Target()),S::ClientApplyStage::NativePublish);
  const auto before_read=s.read_bytes;
  const auto result=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->PublishNow();});s.written|=result.written;
  if(!result.ready||result.failed)return s.Fail(result.reason);
  if(!s.preparation.PublishLiveUse(b,&s))return s.Fail("binary_atomic_custody_commit_failed");
  const auto attachments=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->CommitFields();});
  if(!attachments.ready||attachments.failed){
    // 换房可能在转交保管责任后撤销票据。只有 CommitFields 发现真实链异常才停住原生释放。
    Trace("client_error","event=binary_attachment_finalize_interrupted route=%llu serial=%llu cancelled=%u withdrawn=%u uncertain=%u reason=%s",
      b.ticket.route.route,b.ticket.serial,unsigned(b.cancelled.load(std::memory_order_acquire)),unsigned(b.Withdrawn()),
      unsigned(s.uncertain),attachments.reason);
    return s.Fail(attachments.reason);
  }
  s.bindings.Committed(b,s.Inputs());
  s.RefreshStore();
  const auto streams=S::StateTargetStreams(s.scene->State().Target());
  auto &metrics=s.memory->ApplyMetrics();
  metrics.Volume(streams,S::ClientApplyVolume::PureWriteBytes,s.pure_write_bytes);
  metrics.Volume(streams,S::ClientApplyVolume::PublishReadBytes,s.read_bytes-before_read);
  if(streams&1){
    metrics.Volume(1,S::ClientApplyVolume::UnchangedWorldBytes,s.skipped_world_bytes);
    metrics.Volume(1,S::ClientApplyVolume::WorldWriteOperations,s.operations.size());
  }
  s.published=true;
  s.ReleaseInstallScratch();
  s.reason="binary_atomic_published";return Ready(s.written,s.reason);
}
S::ClientStateNativeStep ClientSceneAtomicInstaller::InstallDisplayFields(const S::ClientStateApplyBatch &b,bool rebuilt) noexcept {
  auto &s=*impl_;
  if(!s.Current(b)||!s.installed||s.manager_token!=OwnerToken(Owner::Manager))return s.Fail("binary_atomic_display_install_window");
  if(s.display_installed&&!rebuilt)return Ready(s.written,"binary_atomic_display_fields_installed");
  const auto result=s.preparation.ApplyInstallation(b,&s,false,&s,[](void *p) noexcept {
    return static_cast<Impl *>(p)->InstallNow(true);
  });
  s.written|=result.written;
  return result.ready&&!result.failed?result:s.Fail(result.reason);
}
void ClientSceneAtomicInstaller::Invalidate(const S::ClientStateApplyBatch &b,const char *why) noexcept {
  auto &s=*impl_;if(s.Thread()&&s.batch.get()==&b)s.Fail(why);
}
ClientSceneFieldBinding ClientSceneAtomicInstaller::PublishedFields()const noexcept {
  auto &s=*impl_;
  if(!s.published)return {};
  return {s.field_index,&s,[](void *p,const ClientSceneFieldIndex &published,const S::ClientShadowStateRef &target,
      S::BodyStream stream,std::size_t &cursor,bool costumes) noexcept {
    auto &native=*static_cast<Impl *>(p);
    // 快速帧之后当前发布目标属于最新的快速票据，根与承载仍由原票据保管。
    const auto *current=native.fast_ticket?native.fast_ticket.get():native.batch.get();
    if(!native.published||!native.batch||native.field_index.get()!=&published||
        unsigned(stream)>1||current->target.candidate->State(stream)!=target)
      return Failed(false,"binary_readback_target_not_current");
    if(native.fast_ticket&&native.batch->resolution.load(std::memory_order_acquire)!=S::ClientStateApplyResolution::Committed)
      return Failed(false,"binary_readback_target_not_current");
    const auto resolution=current->resolution.load(std::memory_order_acquire);
    if(resolution==S::ClientStateApplyResolution::Pending)
      return Ready(false,"binary_readback_publication_pending");
    if(resolution!=S::ClientStateApplyResolution::Committed||!native.Current(*native.batch))
      return Failed(false,"binary_readback_target_not_current");
    struct Work {Impl &owner;const ClientSceneFieldIndex &index;S::BodyStream stream;std::size_t &cursor;bool costumes;};
    Work work{native,published,stream,cursor,costumes};
    return native.preparation.ApplyInstallation(*native.batch,&native,false,&work,[](void *context) noexcept {
      const auto &w=*static_cast<Work *>(context);return w.owner.ScanDrift(w.index,w.stream,w.cursor,w.costumes);
    });
  }};
}
bool ClientSceneAtomicInstaller::Drain(const S::ClientStateApplyBatch &b,bool final_scene) noexcept {
  auto &s=*impl_;if(!s.Thread())return false;if(!s.batch)return true;if(s.batch.get()!=&b)return false;
  s.final_scene|=final_scene;
  if(s.uncertain&&!s.lease){s.reason="binary_atomic_unleased_write_retained";return false;}
  // 已提交使用者接管根，未提交候选仍阻止旧owner退休；不因长寿命根固定整个历史批次。
  if(s.lease&&!s.lease_released&&!s.preparation.BeginInstallationRetirement(b)){s.reason="binary_atomic_root_retirement_waiting";return false;}
  if(!s.invalid&&s.written)s.Fail("binary_atomic_retiring");
  if(s.lease&&!s.lease_released){const auto result=s.preparation.ApplyInstallation(b,&s,true,&s,[](void *p)noexcept{return static_cast<Impl *>(p)->DrainNow();});
    s.reason=result.reason;if(!result.ready||result.failed)return false;
    if(!s.preparation.ReleaseLiveUse(b,&s))return false;s.lease_released=true;}
  if(!s.preparation.DrainBatch(b))return false;
  s.batch.reset();s.scene.reset();s.objects.clear();s.fields.clear();s.resources.clear();s.operations.clear();s.values.clear();
  s.field_owners.clear();s.path_facts.clear();s.quiet.clear();s.resolved_writes.clear();s.resolved_scratch.clear();
  s.field_index.reset();s.published_fields={};s.field_attachments.clear();s.binding_fields.clear();
  s.fast_ticket.reset();s.fast_swapped=false;s.absorbed.clear();
  s.attachment_operations.clear();for(auto &indices:s.late_operations)indices.clear();
  s.lease=false;s.reason="binary_atomic_drained";return true;
}
bool ClientSceneAtomicInstaller::SelectRetiring(const S::ClientStateApplyBatch &target,
    S::ClientStateVector<ClientLiveRetirement> &leaving) const {
  const auto &s=*impl_;
  if(!s.Thread()||!s.published||!s.batch||!s.scene)return false;
  const auto current=s.scene->State().Objects(),next=target.target.install->Objects();
  const auto &future=static_cast<const ClientSceneInstallPlan &>(*target.native);
  const auto &old_room=s.scene->Rooms()->Room().Descriptor();
  const bool same_room=future.Rooms()&&
    old_room.SourceSlot()==future.Rooms()->Room().Descriptor().SourceSlot()&&
    s.scene->State().World().dimension==target.target.install->World().dimension;
  for(std::uint32_t i=0;i<current.size();++i){
    const auto &object=current[i];
    using Domain=S::ClientInstallDomain;
    if(object.domain!=Domain::Entity&&object.domain!=Domain::Player&&object.domain!=Domain::Grid)continue;
    const auto &id=object.member.identity;
    const auto at=std::lower_bound(next.begin(),next.end(),id,[](const auto &entry,const auto &key){return entry.member.identity<key;});
    std::uint64_t instance=0;
    if(auto *address=s.preparation.InstalledObject(*s.batch,i,&instance)){
      if(s.objects[i].address!=address||s.objects[i].instance!=instance)return false;
      if(at!=next.end()&&at->member.identity==id&&
          s.preparation.PendingObjectCurrent(target,std::uint32_t(at-next.begin()),address,instance,false,true))continue;
      auto &entry=leaving.emplace_back(ClientLiveRetirement{id,address,instance});
      if(!same_room||object.domain!=Domain::Entity||
          (id.layout!=F::EntityPickup::kLayout&&id.layout!=F::EntityNPC::kLayout)||
          s.preparation.FindLiveMember(id,instance)!=address||instance!=entry.instance)continue;
      const auto newer=std::upper_bound(next.begin(),next.end(),id,
        [](const auto &key,const auto &o){return key<o.member.identity;});
      if(newer==next.end()||newer->domain!=Domain::Entity)continue;
      auto previous=newer->member.identity;previous.shape=id.shape;
      if(previous==id&&newer->description<target.target.install->Entities().size())
        entry.morph_target=std::uint32_t(newer-next.begin());
    }
  }
  return true;
}
bool ClientSceneAtomicInstaller::MorphMembers(const S::ClientStateApplyBatch &target,
    std::span<ClientLiveRetirement> leaving) noexcept {
  auto &s=*impl_;
  if(std::none_of(leaving.begin(),leaving.end(),[](const auto &entry){return entry.morph_target!=S::kClientPlanNone;}))return true;
  struct Work {Impl &owner;const S::ClientStateApplyBatch &target;std::span<ClientLiveRetirement> leaving;} work{s,target,leaving};
  const auto result=s.preparation.ApplyInstallation(*s.batch,&s,false,&work,[](void *context) noexcept {
    auto &w=*static_cast<Work *>(context);auto &s=w.owner;
    ClientBinaryInitPoolSnapshot before;
    if(!ClientBinaryRememberInitPools(s.roots,before))return Ready(false,"binary_morph_pool_unavailable");
    const auto &plan=*w.target.target.install;
    for(auto &entry:w.leaving){
      if(entry.morph_target==S::kClientPlanNone)continue;
      if(!s.Current(*s.batch)||!s.preparation.Current(w.target))return Failed(true,"binary_morph_lifecycle");
      const auto &object=plan.Objects()[entry.morph_target];
      const auto &entity=plan.Entities()[object.description];
      const unsigned category=entry.identity.layout==F::EntityPickup::kLayout?7u:1u;
      int champion=-1;
      if(category==1){
        const auto world=plan.Effective(S::BodyStream::World);
        const auto record=world?world->Find(object.member.identity):std::nullopt;
        // 平铺记录没有逐字段视图，按字段名从镜像取值；块记录仍按字段号读取。
        if(record){
          std::span<const std::byte> bytes;
          if(record->Flat())bytes=record->FlatValue("champion_color_index");
          else if(const auto *value=record->Field(F::EntityNPC::champion_color_index);value&&value->present)bytes=value->bytes;
          if(bytes.size()==sizeof(champion))std::memcpy(&champion,bytes.data(),sizeof(champion));
        }
      }
      entry.morphed=ClientBinaryEntityMorph(entry.address,category,entity.type,entity.variant,
        entity.subtype,entity.init_seed,champion);
      if(entry.morphed&&!s.preparation.AdoptMorph(w.target,entry.morph_target,entry.identity,
          entry.address,entry.instance,entity))entry.morphed=false;
      Trace("client","event=binary_native_morph member=%llu use=%llu before_shape=%llu after_shape=%llu instance=%llu address=%p reused=%u",
        entry.identity.id,entry.identity.use,entry.identity.shape,object.member.identity.shape,
        entry.instance,entry.address,unsigned(entry.morphed));
    }
    bool cleaned=false;
    if(!ClientBinaryCleanupInitPools(s.roots,before,nullptr,[](const void *,const void *) noexcept {return false;},cleaned))
      return Failed(true,"binary_morph_auxiliary_cleanup");
    return Ready(true,"binary_morph_complete");
  });
  return result.ready&&!result.failed;
}
bool ClientSceneAtomicInstaller::UnbindRetiring(std::span<const ClientLiveRetirement> leaving) noexcept {
  auto &s=*impl_;
  if(!s.lease||!s.batch)return false;
  struct Work {Impl &owner;std::span<const ClientLiveRetirement> leaving;} work{s,leaving};
  const auto result=s.preparation.ApplyInstallation(*s.batch,&s,true,&work,[](void *p) noexcept {
    auto &w=*static_cast<Work *>(p);
    return w.owner.UnbindRetiring(w.leaving)?Ready(true,"binary_live_retirement_unbound"):Failed(true,"binary_live_retirement_unbind");
  });
  return result.ready&&!result.failed;
}
void ClientSceneAtomicInstaller::FailedMembers(S::ClientStateVector<ClientLiveRetirement> &leaving) const {
  const auto &s=*impl_;
  const bool body_only=S::StateTargetStreams(s.scene->State().Target())==2;
  for(std::uint32_t i=0;i<s.objects.size();++i){
    const auto &o=s.objects[i];
    if(!o.address||o.unbound||o.failure==ClientMemberFailure::None)continue;
    // 单独 Body 只借 World 的活 Player。失败记在 Body 并在下一帧补写，名册仍保有该原生实例。
    if(body_only&&o.borrowed&&o.identity.layout==F::EntityPlayer::kLayout)continue;
    // 先核实际构造责任，旧数字计划和失败时的指针不足以准许清理别的池实例。
    if(!s.preparation.OwnsFailedObject(*s.batch,i,o.address,o.instance))
      Bad("binary_failed_member_owner_changed");
    leaving.push_back({o.identity,o.address,o.instance});
  }
}
void ClientSceneAtomicInstaller::VisitFailures(void *context,FailureVisitor visit) const {
  for(const auto &o:impl_->objects)
    if(o.failure!=ClientMemberFailure::None)visit(context,o.identity,o.failure);
}
bool ClientSceneAtomicInstaller::ForgetRetired(std::span<const ClientLiveRetirement> leaving) noexcept {
  return impl_->bindings.ForgetMembers(leaving);
}
bool ClientSceneAtomicInstaller::ReclaimFailed(std::span<const ClientLiveRetirement> leaving) noexcept {
  auto &s=*impl_;
  struct Work {Impl &owner;std::span<const ClientLiveRetirement> leaving;} work{s,leaving};
  const auto remove=s.preparation.ApplyInstallation(*s.batch,&s,true,&work,[](void *p) noexcept {
    auto &w=*static_cast<Work *>(p);auto &s=w.owner;const auto objects=s.scene->State().Objects();
    for(const auto &entry:w.leaving){
      const auto at=std::lower_bound(objects.begin(),objects.end(),entry.identity,
        [](const auto &o,const auto &key){return o.member.identity<key;});
      if(at==objects.end()||at->member.identity!=entry.identity)return Failed(true,"binary_failed_member_key");
      const auto n=std::uint32_t(at-objects.begin());const auto &object=s.objects[n];
      if(!object.unbound||object.address!=entry.address||object.instance!=entry.instance||
          !ClientBinaryRetireLiveMember(s.roots,entry.address,at->domain==S::ClientInstallDomain::Grid,
            at->domain==S::ClientInstallDomain::Player))return Failed(true,"binary_failed_member_detach");
    }
    return Ready(true,"binary_failed_members_detached");
  });
  if(!remove.ready||remove.failed)return false;
  const auto objects=s.scene->State().Objects();
  for(const auto &entry:leaving){
    const auto at=std::lower_bound(objects.begin(),objects.end(),entry.identity,
      [](const auto &o,const auto &key){return o.member.identity<key;});
    const auto n=std::uint32_t(at-objects.begin());
    if(!s.preparation.ReleaseFailedObject(*s.batch,n,entry.address,entry.instance))return false;
    s.objects[n].address=nullptr;
  }
  return s.bindings.ForgetMembers(leaving);
}
bool ClientSceneAtomicInstaller::ReclaimRetiring(std::span<const ClientLiveRetirement> leaving) noexcept {
  auto &s=*impl_;
  if(!s.published||!s.batch)return false;
  struct Work {Impl &owner;std::span<const ClientLiveRetirement> leaving;} work{s,leaving};
  const auto remove=s.preparation.ApplyInstallation(*s.batch,&s,true,&work,[](void *p) noexcept {
    const auto &w=*static_cast<Work *>(p);auto &s=w.owner;
    const auto objects=s.scene->State().Objects();
    using Domain=S::ClientInstallDomain;
    for(const auto &entry:w.leaving){
      const auto at=std::lower_bound(objects.begin(),objects.end(),entry.identity,
        [](const auto &o,const auto &key){return o.member.identity<key;});
      if(at==objects.end()||at->member.identity!=entry.identity)return Failed(true,"binary_live_retirement_identity");
      const auto i=std::uint32_t(at-objects.begin());
      std::uint64_t instance=0;
      if(s.preparation.InstalledObject(*s.batch,i,&instance)!=entry.address||instance!=entry.instance)
        return Failed(true,"binary_live_retirement_instance");
      std::array<std::uint32_t,3> shape{};
      const auto entities=s.scene->State().Entities();
      const bool entity=at->domain==Domain::Entity&&at->description<entities.size();
      if(entity){
        const auto &e=entities[at->description];shape={e.type,e.variant,e.subtype};
      }
      if(!ClientBinaryRetireLiveMember(s.roots,entry.address,at->domain==Domain::Grid,at->domain==Domain::Player,entity?&shape:nullptr)||
          !s.bindings.ForgetMembers({&entry,1}))
        return Failed(true,"binary_live_retirement_native");
    }
    return Ready(true,"binary_live_retirement_detached");
  });
  if(!remove.ready||remove.failed)return false;
  const auto objects=s.scene->State().Objects();
  for(const auto &entry:leaving){
    const auto at=std::lower_bound(objects.begin(),objects.end(),entry.identity,
      [](const auto &o,const auto &key){return o.member.identity<key;});
    if(!s.preparation.ReleaseRetiredObject(*s.batch,std::uint32_t(at-objects.begin()),entry.instance))return false;
  }
  return true;
}
void ClientSceneAtomicInstaller::ResolveDeferredBodyReferences(std::span<const ClientMorphedMember> morphed) noexcept {
  auto &s=*impl_;
  if(!s.published||!s.batch||!s.Current(*s.batch))return;
  struct Work {Impl &owner;std::span<const ClientMorphedMember> morphed;} work{s,morphed};
  s.preparation.ApplyInstallation(*s.batch,&s,false,&work,[](void *p) noexcept {
    const auto &w=*static_cast<Work *>(p);w.owner.ResolveDeferredBodyReferences(w.morphed);
    return Ready(false,"binary_live_body_references_resolved");
  });
}
bool ClientSceneAtomicInstaller::Current(const S::ClientStateApplyBatch &b)const noexcept{return impl_->Current(b);}
bool ClientSceneAtomicInstaller::Installed(const S::ClientStateApplyBatch &b)const noexcept{return impl_->installed&&impl_->Current(b);}
bool ClientSceneAtomicInstaller::Published(const S::ClientStateApplyBatch &b)const noexcept{return impl_->published&&impl_->Current(b);}
bool ClientSceneAtomicInstaller::Drained()const noexcept{return impl_->Thread()&&!impl_->batch;}
const char *ClientSceneAtomicInstaller::Reason()const noexcept{return impl_->Thread()?impl_->reason:"binary_atomic_wrong_thread";}
} // namespace cs::game
