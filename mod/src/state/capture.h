#pragma once
#include "state/body_codec.h"
#include "state/wire_scope.h"
#include "core/replication_memory.h"
#include <thread>

namespace cs::state {
// 采集身份只含自有句柄；原生地址仅存在于下面的当前主线程CaptureRoot中。
struct CaptureSubject {
  std::uint16_t space=0;
  std::uint64_t local=0,use=0,shape=0,binding=0,topology=0;
  auto operator<=>(const CaptureSubject &) const = default;
};
enum class CapturePhase : std::uint8_t { Ordinary, PlayerPost, Production };
// 来源证言与Peer传输版本分开；所有值必须由真实owner给出，不使用最近tick补缺。
struct CaptureWitness {
  StateScope scope;
  std::uint64_t generation=0,revision=0,batch=0,tick=0,outer=0,owner=0,mutation=0,required_revision=0;
  std::uint32_t game_frame=0;
  CapturePhase phase=CapturePhase::Ordinary;
  auto operator<=>(const CaptureWitness &) const = default;
};
// 只检查证言的值完整性；真正来源／线程／生命周期资格仍由CaptureWindow.current复核。
bool ValidCaptureWitness(const CaptureWitness &) noexcept;
// 固定诊断不分配字符串，失败路径同样可以定位到根、字段和递归父路径。
enum class CaptureFailure : std::uint8_t {
  None, WrongThread, InvalidWindow, ReadFailed, AddressRange, UnknownLayout, UnknownVtable,
  SelectorFailed, MissingQualification, QualificationFailed, MissingHandler, UnsafeCopy,
  ValueBudget, PathBudget, RecordBudget, InvalidValue, DuplicateValue, MissingValue,
  InvalidTransaction, Incomplete, ReferencePending, ResourcePending, QueueFull, ControlOverflow,
  Revoked, SequenceExhausted, NeedsOrdinaryJoint, CountMismatch, TraversalBudget, SourceChanged, Reentrant
};
struct CaptureDiagnostic {
  CaptureFailure reason=CaptureFailure::None;
  CaptureSubject subject;
  std::uint16_t layout=0,field=kNullIndex;
  std::uint8_t depth=0;
  std::array<CodecPathStep,kCodecMaxDepth> parents{};
};
// 根只借用到同步调用返回；extent由实际owner核定，不能拿base_prefix当动态分配大小。
struct CaptureRoot {
  CaptureSubject subject;
  std::uint32_t address=0,extent=0;
  std::uint16_t layout=0;
};
// current必须重新检查完整证言和当前根，read只做有界原始复制；均不调用Lua或游戏生产器。
struct CaptureWindow {
  std::thread::id thread;
  CaptureWitness witness;
  void *context=nullptr;
  bool (*current)(void *,const CaptureWitness &,const CaptureRoot &) noexcept=nullptr;
  bool (*read)(void *,std::uint32_t,std::span<std::byte>) noexcept=nullptr;
  bool Check(const CaptureRoot &,CaptureDiagnostic &) const noexcept;
  bool Copy(const CaptureRoot &,std::uint32_t offset,std::span<std::byte>,CaptureDiagnostic &) const noexcept;
};
class CaptureValueWriter;
struct CaptureTraversalSource;
// 字段访问只存在于主线程栈；path采用codec的canonical父路径，不能存入后台任务。
struct CaptureFieldAccess {
  const CaptureWindow &window;
  const CaptureRoot &root;
  const Field &field;
  BodySourceFieldInfo format;
  CodecLocation location;
  // 容器叶handler只借用执行器已经核定的当前游标，不能重开另一个来源或私自换count。
  const CaptureTraversalSource *traversal=nullptr;
};
using CaptureQualifier=bool (*)(const CaptureFieldAccess &) noexcept;
using CaptureFieldHandler=bool (*)(const CaptureFieldAccess &,CaptureValueWriter &,CaptureDiagnostic &) noexcept;
struct CaptureQualificationBinding { std::string_view name;CaptureQualifier check=nullptr; };
// 名称只在冷注册时匹配生成表；逐字段通过稳定数字索引分派。
struct CaptureHandlerBinding { std::string_view name;Operation operation;CaptureFieldHandler capture=nullptr; };
struct CaptureVtableBinding { std::uint32_t vtable=0;std::uint16_t layout=0; };
// 一根对象的一次同步遍历独占此缓存；不得跨窗口或跨线程保存scratch里的原生暂存。
struct CaptureCopyScratch {
  std::span<std::byte> bytes;
  std::uint32_t range=UINT32_MAX,address=0;
  CaptureSubject subject;
  CaptureWitness witness;
  void Reset() noexcept {range=UINT32_MAX;address=0;}
};
// 冷准备的不可变绑定表。缺绑定可保留到对应域实施，但真正访问时一律具名失败。
// 本类不建立读取资格，不扫描游戏对象，也不向快照写入任何原生地址。
class CapturePlan final {
public:
  CapturePlan(std::span<const CaptureQualificationBinding>,std::span<const CaptureHandlerBinding>,
              std::span<const CaptureVtableBinding>);
  CapturePlan(const CapturePlan &)=delete;
  CapturePlan &operator=(const CapturePlan &)=delete;
  // 仅真实RTTI对象调用Identify；无RTTI／协议视图由具名owner直接提供layout。
  bool Identify(const CaptureWindow &,CaptureRoot &,CaptureDiagnostic &) const noexcept;
  // 读取失败不回退基础臂；当前root必须已经退出临时改型owner。
  bool SelectArm(const CaptureWindow &,CaptureRoot &,CaptureDiagnostic &) const noexcept;
  // 分派一个已选中的字段；复杂条件／递归由具名处理器负责，不冒充完整域遍历器。
  bool Dispatch(const CaptureWindow &,const CaptureRoot &,std::uint16_t field,
                std::span<const CodecPathStep> parents,CaptureValueWriter &,CaptureCopyScratch &,
                CaptureDiagnostic &) const noexcept;
  // 复杂来源在open、元素遍历与close周围同样检查资格，不借叶字段Dispatch跳过owner。
  bool Qualify(const CaptureWindow &,const CaptureRoot &,std::uint16_t field,
               std::span<const CodecPathStep>,CaptureValueWriter &,CaptureDiagnostic &) const noexcept;
  const BodySourceFieldInfo *Format(std::uint16_t layout,std::uint16_t field) const noexcept;
  // 按生成块说明选流，不把body不适合发送误改为world。
  std::uint32_t Mask(std::uint16_t layout,BodyStream) const noexcept;
  std::uint64_t RetainedBytes() const noexcept {return memory_->Bytes();}
private:
  friend class CaptureExecutor;
  friend class CaptureExecution;
  bool DispatchSelected(const CaptureWindow &,const CaptureRoot &,std::uint16_t,
      std::span<const CodecPathStep>,CaptureValueWriter &,CaptureCopyScratch &,CaptureDiagnostic &,bool,
      const CaptureTraversalSource *source=nullptr) const noexcept;
  struct Binding {CaptureQualifier qualify=nullptr;CaptureFieldHandler handler=nullptr;BodySourceFieldInfo format;bool required=false,direct=false;std::uint32_t copy=UINT32_MAX;};
  memory::Lease memory_;
  std::vector<Binding> fields_;
  std::vector<CaptureVtableBinding> vtables_;
};
// 首次失败保留具体位置；热路径不格式化日志，也不覆盖导致整根失败的最初原因。
bool FailCapture(CaptureDiagnostic &,CaptureFailure,const CaptureRoot &,
                 std::uint16_t field=kNullIndex,std::span<const CodecPathStep> parents={}) noexcept;
std::uint32_t CaptureScalarWidth(ScalarKind) noexcept;
}
