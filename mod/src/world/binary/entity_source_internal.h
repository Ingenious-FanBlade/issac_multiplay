#pragma once
#include "world/binary/entity_source.h"
#include "state/entity_counters.h"
#include "state/flat_layout.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <utility>

namespace cs::world::binary {
using namespace state;
// 所有特殊来源在冷准备绑定为数字指令；不在逐字段热路径解析handler名称。
enum class EntityRead : std::uint8_t {
  None,Raw,Mask,Bool,Order,Expire,Flying,Water,Reference,File,ImagePath,Image,Animation,Item,Config,
  QueueItem,QueueFlags,CostumeIndex,NpcControl,Incubus,Topology,DrawSecond,
  Membership,Roster,Group,Role,Control,DrawPresent,Vector,String,Fixed,Layers,LayerFrames,NullFrames,
  Deque,Rope,Trail,Ghost,Shared,Plus,Spoofs,Tree,Blocked,Innate,Weapon,DrawNode,DrawSprite,TrinketPresence,
  GridWater,Glitch,RockPose,RockPosition,RockRubble,RockAnimation,FireEmbers,DoorAlpha,DoorLight,Inline,
  Scene,RoomConfig,Run,Presentation,Display
};
enum class NativeSequence : std::uint32_t { Contiguous, Owned, Addresses, Weapon };
EntityRead GridRecipe(std::string_view) noexcept;
EntityRead SceneRecipe(std::string_view) noexcept;
EntityRead RunRecipe(std::string_view) noexcept;
EntityRead FindEntityRead(std::string_view) noexcept;
// 默认来源属于本根。Player 的同址不同用途／不同子 owner 必须保留独立证言。
enum class NativeEvidenceUse : std::uint8_t { Field, PlayerVtable, Loaded, FileName, Layers, Image, CostumeVector, CostumeItem,
  CostumeFlags, CostumeIndex, CostumeLayer, CostumeBody, CanFly, AnimationOwner, Animation, FrameCount, FrameData };
struct NativeEvidenceSource {std::uint32_t owner=0;NativeEvidenceUse use=NativeEvidenceUse::Field;};
// 仅保存读取过的具名小头和字段，根尾按原地址复读；未知负载不进入证言。
struct NativeReadWitness {
  std::uint32_t address=0,owner=0;
  std::uint16_t layout=0,field=kNullIndex;
  std::uint8_t size=0;
  NativeEvidenceUse use=NativeEvidenceUse::Field;
  std::array<std::byte,32> bytes{};
};
struct NativeWeaponSlot {std::uint32_t address=0,extent=0;std::uint16_t layout=0;};
struct NativeWeaponContract {std::uint32_t vtable=0,extent=0;std::uint16_t layout=0;};
const NativeWeaponContract *WeaponContract(std::uint32_t type) noexcept;
// 每个真实身体独立保留结构字节；版本只增，不计算摘要或将原生指针放进结构。
struct PlayerTopologyRevision {
  CaptureSubject subject;
  std::uint32_t revision=0,ordinary=0,size=0;
};
// 当前 Player 的具名结构投影，借用本次采集的 arena。PlayerPost 各自构造，ordinary 的两流共用。
struct PreparedPlayerSprite {
  std::uint32_t address=0,layer_data=0,layer_count=0;
  CaptureLink file;
  // 与拓扑比较共用自有 9B 句柄序列，按实际图层索引寻址。
  std::span<const std::byte> images;
};
struct PreparedPlayerCostume {PreparedPlayerSprite sprite;CaptureLink item;};
struct PreparedPlayerMap {std::int32_t layer=0;std::uint16_t index=kNullIndex;};
struct PreparedPlayerSource {
  std::array<PreparedPlayerSprite,14> sprites{};
  std::array<PreparedPlayerMap,15> map{};
  std::span<PreparedPlayerCostume> costumes;
  std::uint64_t read_serial=0;
  std::uint32_t costume_data=0;
  bool ready=false,shared_streams=false;
};
enum class NativeResourceSource : std::uint8_t { File, SelectedFile, Image, Item, Animation, ImagePath, Config };
// 以实际资源指针作键。动画的文件与 owner、图像的场景是来源的一部分。
struct NativeResourceKey {
  NativeResourceSource kind{};
  std::uint32_t address=0,owner=0,scene=0;
  std::uint64_t dependency=0;
  auto operator<=>(const NativeResourceKey &) const = default;
};
struct NativeResourceMemo {NativeResourceKey key;CaptureLink link;};
// 按原读取顺序保存确切字节。外部地址只在已核的当前读段内按原依赖顺序复查。
struct NativeInputRange {std::uint32_t address=0;std::uint16_t offset=0,size=0;};
struct NativeInputEvidence {
  std::uint16_t used=0,count=0;
  bool valid=false;
};
template<std::size_t Ranges,std::size_t Bytes>struct NativeInputStorage {
  NativeInputEvidence evidence;
  std::array<NativeInputRange,Ranges> ranges;
  std::array<std::byte,Bytes> bytes;
};
struct NativeInputView {
  NativeInputEvidence &evidence;
  std::span<NativeInputRange> ranges;
  std::span<std::byte> bytes;
  template<std::size_t Ranges,std::size_t Bytes> NativeInputView(NativeInputStorage<Ranges,Bytes> &s) noexcept:
    evidence(s.evidence),ranges(s.ranges),bytes(s.bytes) {}
  void Add(std::uint32_t address,std::span<const std::byte> input) noexcept;
};
struct NativeInputRecorder {
  NativeInputView input;
  NativeInputRecorder *parent;
  NativeInputRecorder(NativeInputRecorder *&slot,NativeInputView view) noexcept:
    input(view),parent(slot),slot_(slot){slot_=this;}
  ~NativeInputRecorder(){slot_=parent;}
private:
  NativeInputRecorder *&slot_;
};
struct NativeResourceReuse {
  NativeResourceKey key;
  StateScope scope;
  std::uint64_t epoch=0,generation=0,serial=0;
  CaptureLink link;
  NativeInputStorage<80,1536> input;
};
struct NativeRootReuse {
  CaptureSubject subject;
  StateScope scope;
  std::uint64_t epoch=0,revision=0;
  std::uint32_t address=0;
  std::uint16_t layout=0;
  // 引用语义可在指针未变时变化，因此另查当前成员身份。
  struct Reference {std::uint32_t address=0;CaptureSubject target;};
  std::array<Reference,16> references;
  std::uint8_t reference_count=0;
  NativeInputStorage<224,4096> input;
};
struct NativeSpriteDependency {std::uint32_t address=0,owner=0;std::uint64_t local=0,use=0;std::uint16_t space=0;};
struct NativeConfigOwner {std::uint32_t address=0;std::uint64_t run=0,owner=0;};
struct NativeHistoryGlyph {std::int32_t visual=0;CaptureLink image{CapturedReferenceKind::Null,0};};

// 具名补丁在冷准备时绑定。根和可达子布局共用按序排列的生成补丁表。
struct FlatSourceProgram {
  const FlatLayout *layout=nullptr;
  std::vector<EntityRead> reads;
  bool has_conditions=false;
};

struct EntitySource::Impl {
  MemoryReader memory;
  std::thread::id thread;
  EntityServices services;
  EntitySourceCapacity capacity;
  memory::Lease memory_charge;
  std::vector<EntityRead> reads;
  std::vector<FlatSourceProgram> flat_programs;
  std::vector<CaptureVtableBinding> vtables;
  std::unique_ptr<CapturePlan> plan;
  std::unique_ptr<CaptureExecutor> executor;
  std::vector<std::byte> scratch,resource_bytes,topology_bytes;
  std::vector<NativeReadWitness> witnesses;
  std::vector<std::uint32_t> witness_order,resource_order;
  std::vector<NativeResourceMemo> resource_memos;
  std::vector<NativeResource> resources;
  std::vector<NativeAnimationLayer> animation_layers;
  std::vector<PlayerTopologyRevision> topologies;
  PreparedPlayerSource player;
  std::vector<NativeSpriteDependency> sprite_dependencies;
  std::vector<NativeConfigOwner> config_owners;
  std::uint32_t config_owner_used=0;
  std::uint32_t memo_used=0;
  StateScope memo_scope;
  std::uint64_t memo_generation=0,memo_epoch=0;
  NativeSourceStats stats;
  memory::Lease reuse_charge;
  std::vector<NativeRootReuse> root_reuse;
  std::vector<NativeResourceReuse> resource_reuse;
  NativeInputRecorder *input_recorder=nullptr;
  NativeRootReuse *root_input=nullptr;
  std::uint64_t reuse_epoch=1;
  std::uint32_t scratch_used=0,witness_used=0,resource_used=0,resource_bytes_used=0,layer_used=0,topology_used=0;
  CaptureWindow window;
  EntityFacts facts;
  std::uint32_t topology_revision=0;
  std::uint32_t sprite_dependency_used=0;
  std::uint16_t root_layout=0;
  std::uint16_t evidence_layout=0,evidence_field=kNullIndex;
  std::uint32_t scene_scan_remaining=262144;
  bool map_ready=false;
  std::uint16_t map_mask=0;
  std::array<std::uint8_t,16> map_frames{};
  bool active=false;
  bool root_qualified=false;
  Impl(MemoryReader,std::thread::id,EntityServices,EntitySourceCapacity);
  EntityRead Recipe(const Field &f) const noexcept {return reads[FindLayout(f.layout)->first_field+f.index];}
  bool Reject(const CaptureFieldAccess &a,CaptureDiagnostic &d,CaptureFailure f) const noexcept {return FailCapture(d,f,a.root,a.field.index,a.location.parents);}
  bool Current(const CaptureWitness &,const CaptureRoot &) noexcept;
  bool ReadScopeCurrent() noexcept;
  bool Read(std::uint64_t,std::span<std::byte>,CaptureDiagnostic &) noexcept;
  void RecordInput(std::uint32_t,std::span<const std::byte>) noexcept;
  bool CopyInput(std::uint32_t,std::span<std::byte>) noexcept;
  bool InputsMatch(NativeInputView,std::uint32_t root=0,std::uint32_t extent=0) noexcept;
  void ReplayInput(NativeInputView) noexcept;
  bool MatchesRoot(NativeRootReuse &,const CaptureRoot &) noexcept;
  CaptureLink ReadReference(std::uint32_t) noexcept;
  void CheckReuseRecipe(EntityRead) noexcept;
  template<class T> bool Value(std::uint64_t p,T &value,CaptureDiagnostic &d) noexcept {return Read(p,std::as_writable_bytes(std::span(&value,1)),d);}
  bool Track(std::uint64_t,std::span<std::byte>,CaptureDiagnostic &,NativeEvidenceSource={}) noexcept;
  // 只接收本根完整键查找的插入位置；读取不改变证言表，实际容量与地址仍在此检查。
  bool InsertWitness(std::uint32_t,std::uint64_t,std::span<const std::byte>,CaptureDiagnostic &,NativeEvidenceSource) noexcept;
  bool Remember(std::uint64_t,std::span<const std::byte>,CaptureDiagnostic &,NativeEvidenceSource={}) noexcept;
  bool ReviewRoot() noexcept;
  bool Verify(const CaptureRoot &,CaptureDiagnostic &) noexcept;
  template<class T> bool Tracked(std::uint64_t p,T &value,CaptureDiagnostic &d,NativeEvidenceSource source={}) noexcept {
    return Track(p,std::as_writable_bytes(std::span(&value,1)),d,source);
  }
  std::span<std::byte> Allocate(std::uint64_t,unsigned align,CaptureDiagnostic &) noexcept;
  bool StringAt(std::uint32_t,std::span<std::byte>,std::uint32_t &,CaptureDiagnostic &,bool track=false,NativeEvidenceSource={}) noexcept;
  bool CStringAt(std::uint32_t,std::uint32_t,std::span<std::byte>,std::uint32_t &,CaptureDiagnostic &,bool track=false,std::uint32_t extent=0) noexcept;
  bool Leaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool BasicConversion(EntityRead,const CaptureRoot &,std::uint64_t address,std::uint32_t mask,
                       std::uint64_t &result,CaptureDiagnostic &) noexcept;
  bool ExistingPlus(const CaptureRoot &,std::uint32_t &holder,std::uint32_t &payload,CaptureDiagnostic &) noexcept;
  bool ExecuteFlatTear(const CaptureRoot &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  std::uint64_t BindFlatTear();
  bool Open(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool Element(const CaptureFieldAccess &,const CaptureTraversalSource &,std::uint16_t,CaptureRoot &,CaptureDiagnostic &) noexcept;
  bool Close(const CaptureFieldAccess &,const CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool ScalarContainer(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool NativeTree(std::uint32_t,std::span<std::uint32_t>,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool HashItems(std::uint32_t,bool,std::span<std::byte>,std::uint32_t &,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool Aggregate(std::uint32_t,std::span<std::byte>,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool Resource(const CaptureFieldAccess &,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool ItemResource(std::uint32_t,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool FileResource(std::uint32_t,bool,CaptureLink &,CaptureDiagnostic &,bool player_sprite=false) noexcept;
  bool ImageResource(std::uint32_t,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool ImagePathResource(std::uint32_t,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool ConfigResource(std::uint32_t,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool AnimationResource(std::uint32_t,std::uint32_t,CaptureLink &,CaptureDiagnostic &,const CaptureLink *prepared_file=nullptr) noexcept;
  bool Intern(NativeResource,std::span<const std::byte>,std::span<const NativeAnimationLayer>,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool ReuseResource(NativeResourceKey,CaptureLink &,void *,bool (*)(void *) noexcept,bool cross_frame) noexcept;
  template<class Parse> bool ResourceOnce(NativeResourceKey key,CaptureLink &out,Parse parse,bool cross_frame=true) noexcept {
    return ReuseResource(key,out,&parse,[](void *context) noexcept {return (*static_cast<Parse *>(context))();},cross_frame);
  }
  bool Weapon(std::uint32_t,NativeWeaponSlot &,std::uint32_t &,CaptureDiagnostic &,bool tracked=false) noexcept;
  bool Incubus(std::uint32_t,std::int32_t &,CaptureDiagnostic &) noexcept;
  bool PrepareTopology(CaptureDiagnostic &) noexcept;
  const PreparedPlayerSprite *PlayerSprite(std::span<const CodecPathStep>) const noexcept;
  const PreparedPlayerCostume *PlayerCostume(const CaptureFieldAccess &) const noexcept;
  const PreparedPlayerMap *PlayerMap(const CaptureFieldAccess &) const noexcept;
  bool PlayerLayerImage(const CaptureFieldAccess &,CaptureLink &) const noexcept;
  const PreparedPlayerSprite *PlayerAnimation(const CaptureFieldAccess &) const noexcept;
  bool ReadPlayerAnimation(const CaptureFieldAccess &,const PreparedPlayerSprite &,std::uint32_t &owner,
                           std::uint32_t &animation,CaptureDiagnostic &) noexcept;
  bool GridLeaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool GridOpen(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool RockAnimation(CaptureLink &,CaptureDiagnostic &) noexcept;
  bool SceneLeaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool SceneOpen(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool RunLeaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool RunOpen(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  const DisplayFieldValue *DisplayValue(const CaptureFieldAccess &,CaptureDiagnostic &) noexcept;
  bool DisplayLeaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool DisplayOpen(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool DisplayElement(const CaptureFieldAccess &,const CaptureTraversalSource &,std::uint16_t,CaptureRoot &,CaptureDiagnostic &) noexcept;
  bool SavedMapIcons(std::uint32_t,std::uint16_t &,std::array<std::uint8_t,16> &,CaptureDiagnostic &) noexcept;
  bool NamedAnimation(std::uint32_t,std::string_view,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool WaveSpriteMask(std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool ImageGeometry(std::uint32_t,std::array<std::uint32_t,4> &,CaptureDiagnostic &) noexcept;
  std::uint32_t FindTopology(CaptureSubject) const noexcept;
};
inline std::uint32_t NativeAddress(std::span<std::byte> b) noexcept {return std::uint32_t(reinterpret_cast<std::uintptr_t>(b.data()));}
}
