#pragma once
#include "state/capture.h"

namespace cs::state {
enum class CaptureCellKind : std::uint8_t { Scalar, Count, Presence, SlotMask, SlotLayout, References, Resources };
// 引用数组保留Null／Identity／Pending；handle由主线程身份owner分配，绝不借用原生指针。
struct CaptureLink { CapturedReferenceKind kind=CapturedReferenceKind::Invalid;std::uint64_t handle=0; };
struct CaptureCell {
  std::uint32_t path=0,offset=0,bytes=0,count=0;
  std::uint16_t layout=0,field=0,aux=0;
  std::uint8_t depth=0;
  CaptureCellKind kind=CaptureCellKind::Scalar;
  ScalarKind scalar=ScalarKind::None;
};
// 当前事务最后一次写入的借用视图，仅供执行器立即核验handler输出，不能跨写入／封存保存。
struct CaptureWrittenValue {
  ScalarKind scalar=ScalarKind::None;
  std::uint32_t count=0;
  std::uint16_t aux=0;
  std::span<const std::byte> bytes;
};
// 每根记录是一个不可分割事务；一份packet可包含同窗口的多根记录或同批显示值。
struct CaptureRecord {
  CaptureSubject subject;
  std::uint16_t layout=0;
  BodyStream stream=BodyStream::Value;
  std::uint32_t mask=0,first_cell=0,cell_count=0;
  // 正常生产修订不被ordinary交接修订覆盖；0表示普通对象状态。
  CaptureWitness production;
  CaptureSubject owner;
  std::uint64_t instance=0,generation=0,event_sequence=0;
  std::uint32_t call=0,parent=UINT32_MAX;
  std::uint16_t purpose=0,pass=0;
};
// 资源目录的不可变前缀及本包引用表尺寸；随packet交接，不借用主线程可变计数。
struct CaptureCatalogStamp {std::uint32_t references=0,resources=0,resource_bytes=0,resource_layers=0;};
// source_slot与order仅为结构owner的纯值事实；物理描述槽和活动顺序不能在worker按ListIndex重猜。
struct CaptureReferenceEntry {std::uint16_t space=0;std::uint64_t local=0,use=0;std::uint16_t source_slot=kNullIndex,order=kNullIndex;};
struct CapturePacket {
  CaptureWitness witness;
  std::span<const CaptureRecord> records;
  std::span<const CaptureCell> cells;
  std::span<const CodecPathStep> paths;
  std::span<const std::byte> bytes;
  CaptureCatalogStamp catalog;
  std::span<const CaptureReferenceEntry> references;
};
// 缓存的引用号属于原生产目录；克隆时按物理身份转入当前交接目录，资源handle只追加。
struct CaptureAppendContext {
  void *owner=nullptr;
  CaptureLink (*reference)(void *,const CaptureReferenceEntry &) noexcept=nullptr;
};
// 所有capacity在冷准备期给定并记入Values总账；逐帧只重置游标，不改变vector容量。
struct CaptureCapacity { std::uint32_t records=512,cells=16384,paths=32768,bytes=1u<<20,references=0; };
class CaptureBuffer final {
public:
  explicit CaptureBuffer(CaptureCapacity);
  std::uint64_t RetainedBytes() const noexcept {return memory_->Bytes();}
  CaptureBuffer(const CaptureBuffer &)=delete;
  CaptureBuffer &operator=(const CaptureBuffer &)=delete;
private:
  friend class CaptureValueWriter;
  memory::Lease memory_;
  std::vector<CaptureRecord> records_;
  std::vector<CaptureCell> cells_;
  std::vector<CodecPathStep> paths_;
  std::vector<std::byte> bytes_;
  std::vector<CaptureReferenceEntry> references_;
};
// writer独占一份buffer。整包失败后不能Seal，AbortRoot回卷全部游标但不抹去失败原因。
class CaptureValueWriter final {
public:
  CaptureValueWriter(CaptureBuffer &,CaptureWitness) noexcept;
  CaptureValueWriter(const CaptureValueWriter &)=delete;
  CaptureValueWriter &operator=(const CaptureValueWriter &)=delete;
  CaptureValueWriter(CaptureValueWriter &&)=default;
  // Begin固定根身份、流与真实块mask；Finish只封闭值事务，尚不代表正文或完整world合法。
  bool BeginRoot(CaptureSubject,std::uint16_t layout,BodyStream,std::uint32_t mask) noexcept;
  bool FinishRoot() noexcept;
  void AbortRoot() noexcept;
  bool PutScalar(const CodecLocation &,ScalarKind,std::uint32_t count,std::span<const std::byte>) noexcept;
  bool PutCount(const CodecLocation &,std::uint32_t) noexcept;
  bool PutPresence(const CodecLocation &,bool) noexcept;
  bool PutSlotMask(const CodecLocation &,std::uint32_t) noexcept;
  bool PutSlotLayout(const CodecLocation &,std::uint8_t slot,std::uint16_t layout) noexcept;
  bool PutLinks(const CodecLocation &,bool resource,std::uint8_t table,std::span<const CaptureLink>) noexcept;
  // 必须给出枚举owner预期的根数；整包任何失败或未结束根都会拒绝封存。
  bool Seal(std::uint32_t expected_roots,CapturePacket &) noexcept;
  // 引用目录复制进本buffer；资源目录只追加，可凭stamp安全借用其不可变前缀。
  bool Catalog(CaptureCatalogStamp,std::span<const CaptureReferenceEntry>) noexcept;
  // 克隆已经封存的纯值，不再访问原生对象；保留每根真实生产身份与修订。
  bool Append(const CapturePacket &,std::uint32_t record,CaptureAppendContext={}) noexcept;
  bool Production(const CaptureRecord &) noexcept;
  bool Fail(CaptureFailure,std::uint16_t field=kNullIndex,std::span<const CodecPathStep> parents={}) noexcept;
  bool Fail(const CaptureDiagnostic &) noexcept;
  const CaptureDiagnostic &Diagnostic() const noexcept {return diagnostic_;}
  const CaptureWitness &Witness() const noexcept {return witness_;}
  const CaptureRoot &Root() const noexcept {return root_;}
  bool Allows(const CodecLocation &) const noexcept;
  std::uint32_t WrittenCells() const noexcept {return cells_;}
  bool LastValue(const CodecLocation &,CaptureCellKind,CaptureWrittenValue &) const noexcept;
private:
  bool Put(const CodecLocation &,CaptureCellKind,ScalarKind,std::uint32_t,std::uint16_t,std::span<const std::byte>) noexcept;
  CaptureBuffer &buffer_;
  CaptureWitness witness_;
  CaptureCatalogStamp catalog_;
  std::uint64_t reference_limit_=0;
  CaptureDiagnostic diagnostic_;
  CaptureRoot root_;
  std::uint32_t records_=0,cells_=0,paths_=0,bytes_=0,mark_cells_=0,mark_paths_=0,mark_bytes_=0;
  bool open_=false,sealed_=false;
};
// 后台从精确成员／资源快照解析自有handle。false保留Pending／缺资源，不编码为Null。
struct CaptureLinkResolver {
  void *context=nullptr;
  bool (*resolve)(void *,CaptureSubject,bool resource,std::uint8_t table,CaptureLink,std::uint16_t &) noexcept=nullptr;
};
// 仅借用仍由consumer票据持有的packet；scratch是后台自有可复用区，不包含原生对象。
// 此适配器只服务现有正文codec，缺值／错类型抛Gap发生在后台，不进入主线程热路径。
class CapturedBodySource final : public BodyValueSource {
public:
  CapturedBodySource(const CapturePacket &,std::uint32_t record,CaptureLinkResolver,std::span<std::byte> link_scratch);
  std::span<const std::byte> Scalar(const CodecLocation &,ScalarKind,std::uint32_t) const override;
  std::uint32_t Count(const CodecLocation &) const override;
  bool SharedPresent(const CodecLocation &) const override;
  std::uint32_t SlotMask(const CodecLocation &) const override;
  std::uint16_t SlotLayout(const CodecLocation &,std::uint8_t) const override;
private:
  const CaptureCell &Find(const CodecLocation &,CaptureCellKind,std::uint16_t aux=0) const;
  std::span<const std::byte> Bytes(const CaptureCell &) const;
  CapturePacket packet_;
  CaptureRecord record_;
  CaptureLinkResolver resolver_;
  std::span<std::byte> scratch_;
};
}
