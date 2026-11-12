#pragma once
#include "world/binary/entity_source.h"
#include "state/entity_counters.h"
#include <algorithm>
#include <bit>
#include <cstring>

namespace cs::world::binary {
using namespace state;
// 所有特殊来源在冷准备绑定为数字指令；不在逐字段热路径解析handler名称。
enum class EntityRead : std::uint8_t {
  None,Raw,Mask,Bool,Order,Expire,Flying,Water,Reference,File,ImagePath,Image,Animation,Item,Config,
  QueueItem,QueueFlags,CostumeIndex,NpcControl,Incubus,Topology,DrawSecond,
  Membership,Roster,Group,Role,Control,DrawPresent,Vector,String,Fixed,Layers,LayerFrames,NullFrames,
  Deque,Rope,Trail,Ghost,Shared,Plus,Spoofs,Tree,Blocked,Innate,Weapon,DrawNode,DrawSprite,TrinketPresence,
  GridWater,Glitch,RockPose,RockPosition,RockRubble,RockAnimation,FireEmbers,DoorAlpha,DoorLight,Inline,
  Scene,RoomConfig,Run,Presentation
};
enum class NativeSequence : std::uint32_t { Contiguous, Owned, Addresses, Weapon };
EntityRead GridRecipe(std::string_view) noexcept;
EntityRead SceneRecipe(std::string_view) noexcept;
EntityRead RunRecipe(std::string_view) noexcept;
EntityRead PresentationRecipe(const Field &) noexcept;
// 仅保存读取过的具名小头和字段，close按原地址复读；未知负载不进入证言。
struct NativeReadWitness {std::uint32_t address=0;std::uint8_t size=0;std::array<std::byte,32> bytes{};};
struct NativeWeaponSlot {std::uint32_t address=0,extent=0;std::uint16_t layout=0;};
struct NativeWeaponContract {std::uint32_t vtable=0,extent=0;std::uint16_t layout=0;};
const NativeWeaponContract *WeaponContract(std::uint32_t type) noexcept;
// 每个真实身体独立保留结构字节；版本只增，不计算摘要或将原生指针放进结构。
struct NativeTopology {CaptureSubject subject;std::uint32_t revision=0,ordinary=0,size=0;};
struct NativeSpriteDependency {std::uint32_t address=0;std::uint64_t local=0,use=0;std::uint16_t space=0;};
struct NativeConfigOwner {std::uint32_t address=0;std::uint64_t run=0,owner=0;};
// 当前Room根的一次纯值光照投影；颜色保持真实空间标签，不保存原生地址或渲染器句柄。
struct NativeLighting {
  std::array<std::uint32_t,5> shadow{},tint{};
  float alpha=0,lightning=0;
  std::uint8_t mask=0,special=0;
};
struct NativeSurface {std::uint32_t image=0;std::uint64_t resource=0;};
struct NativeHistoryGlyph {std::int32_t visual=0;CaptureLink image{CapturedReferenceKind::Null,0};};

struct EntitySource::Impl {
  MemoryReader memory;
  std::thread::id thread;
  EntityServices services;
  EntitySourceCapacity capacity;
  memory::Lease memory_charge;
  std::vector<EntityRead> reads;
  std::vector<CaptureVtableBinding> vtables;
  std::unique_ptr<CapturePlan> plan;
  std::unique_ptr<CaptureExecutor> executor;
  std::vector<std::byte> scratch,resource_bytes,topology_bytes;
  std::vector<NativeReadWitness> witnesses;
  std::vector<NativeResource> resources;
  std::vector<NativeAnimationLayer> animation_layers;
  std::vector<NativeTopology> topologies;
  std::vector<NativeSpriteDependency> sprite_dependencies;
  std::vector<NativeConfigOwner> config_owners;
  std::uint32_t config_owner_used=0;
  std::uint32_t scratch_used=0,witness_used=0,resource_used=0,resource_bytes_used=0,layer_used=0,topology_used=0;
  CaptureWindow window;
  EntityFacts facts;
  std::uint32_t topology_revision=0;
  std::uint32_t sprite_dependency_used=0;
  std::uint16_t root_layout=0;
  std::uint32_t scene_scan_remaining=262144;
  bool map_ready=false;
  bool lighting_ready=false;
  NativeLighting lighting;
  std::vector<NativeSurfaceOperation> surface_operations;
  std::array<NativeSurface,4> surfaces{};
  std::uint32_t production_owner=0,production_descriptor=0,production_depth=0,surface_used=0,surface_target=0;
  std::array<std::uint32_t,4> surface_geometry{};
  NativeSurfaceOperation pending_surface;
  std::uint32_t pending_vertices=0,pending_stride=0,pending_native_image=0;
  bool production_read=false,production_valid=false,surface_valid=false,surface_dirty=false,pending_image=false;
  std::uint16_t map_mask=0;
  std::array<std::uint8_t,16> map_frames{};
  bool active=false;
  Impl(MemoryReader,std::thread::id,EntityServices,EntitySourceCapacity);
  EntityRead Recipe(const Field &f) const noexcept {return reads[FindLayout(f.layout)->first_field+f.index];}
  bool Reject(const CaptureFieldAccess &a,CaptureDiagnostic &d,CaptureFailure f) const noexcept {return FailCapture(d,f,a.root,a.field.index,a.location.parents);}
  bool Current(const CaptureWitness &,const CaptureRoot &) noexcept;
  bool Read(std::uint64_t,std::span<std::byte>,CaptureDiagnostic &) noexcept;
  template<class T> bool Value(std::uint64_t p,T &value,CaptureDiagnostic &d) noexcept {return Read(p,std::as_writable_bytes(std::span(&value,1)),d);}
  bool Track(std::uint64_t,std::span<std::byte>,CaptureDiagnostic &) noexcept;
  template<class T> bool Tracked(std::uint64_t p,T &value,CaptureDiagnostic &d) noexcept {return Track(p,std::as_writable_bytes(std::span(&value,1)),d);}
  std::span<std::byte> Allocate(std::uint64_t,unsigned align,CaptureDiagnostic &) noexcept;
  bool StringAt(std::uint32_t,std::span<std::byte>,std::uint32_t &,CaptureDiagnostic &,bool track=false) noexcept;
  bool CStringAt(std::uint32_t,std::uint32_t,std::span<std::byte>,std::uint32_t &,CaptureDiagnostic &,bool track=false) noexcept;
  bool Leaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool Open(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool Element(const CaptureFieldAccess &,const CaptureTraversalSource &,std::uint16_t,CaptureRoot &,CaptureDiagnostic &) noexcept;
  bool Close(const CaptureFieldAccess &,const CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool ScalarContainer(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool NativeTree(std::uint32_t,std::span<std::uint32_t>,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool HashItems(std::uint32_t,bool,std::span<std::byte>,std::uint32_t &,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool Aggregate(std::uint32_t,std::span<std::byte>,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool Resource(const CaptureFieldAccess &,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool ItemResource(std::uint32_t,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool FileResource(std::uint32_t,bool,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool ImageResource(std::uint32_t,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool AnimationResource(std::uint32_t,std::uint32_t,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool Intern(NativeResource,std::span<const std::byte>,std::span<const NativeAnimationLayer>,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool Weapon(std::uint32_t,NativeWeaponSlot &,std::uint32_t &,CaptureDiagnostic &,bool tracked=false) noexcept;
  bool Incubus(std::uint32_t,std::int32_t &,CaptureDiagnostic &) noexcept;
  bool PrepareTopology(CaptureDiagnostic &) noexcept;
  bool GridLeaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool GridOpen(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool RockAnimation(CaptureLink &,CaptureDiagnostic &) noexcept;
  bool SceneLeaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool SceneOpen(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool RunLeaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool PresentationLeaf(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
  bool PresentationOpen(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool HudIcon(const CaptureFieldAccess &,std::uint8_t &,CaptureDiagnostic &) noexcept;
  bool HistoryGlyphs(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept;
  bool SavedMapIcons(std::uint32_t,std::uint16_t &,std::array<std::uint8_t,16> &,CaptureDiagnostic &) noexcept;
  bool WaterShader(std::array<std::uint32_t,8> &,CaptureDiagnostic &) noexcept;
  bool NamedAnimation(std::uint32_t,std::string_view,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool WaveSpriteMask(std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool Lighting(CaptureDiagnostic &) noexcept;
  bool QualifiedPlayer(std::uint32_t,CaptureDiagnostic &) noexcept;
  bool LightingCollectible(std::uint32_t,std::int32_t,bool &,CaptureDiagnostic &,unsigned depth=0,bool spoof=true) noexcept;
  bool NativeCounter(std::uint32_t,std::int32_t,std::int32_t &,CaptureDiagnostic &) noexcept;
  bool ImageGeometry(std::uint32_t,std::array<std::uint32_t,4> &,CaptureDiagnostic &) noexcept;
  bool ProgramImage(std::uint32_t,CaptureLink &,CaptureDiagnostic &) noexcept;
  bool FlushSurface(CaptureDiagnostic &) noexcept;
  bool SealSurface(CaptureDiagnostic &) noexcept;
  bool BackdropBindings(std::array<std::array<std::uint32_t,2>,7> &,std::uint32_t &,CaptureDiagnostic &) noexcept;
  bool Wave(unsigned,std::array<float,4> &,bool &,CaptureDiagnostic &) noexcept;
  std::uint32_t FindTopology(CaptureSubject) const noexcept;
};
inline std::uint32_t NativeAddress(std::span<std::byte> b) noexcept {return std::uint32_t(reinterpret_cast<std::uintptr_t>(b.data()));}
}
