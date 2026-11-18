#pragma once
#include "state/capture.h"

namespace cs::capture {struct BodyCloneCounts;}
namespace cs::game {class BinaryEntities;}
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
// 本根刚写入的自有值视图，执行器立即消费；不得跨写入或封存保存。
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
  // 延后绑定的请求仍保留原调用帧，不把当前ordinary帧冒充事件发生时刻。
  std::uint32_t event_game_frame=0;
  std::uint32_t call=0,parent=UINT32_MAX;
  std::uint16_t purpose=0,pass=0;
  // Host 内部整根复用凭据，只有后台已接管这一普通快照时才可使用。
  std::uint64_t reusable_base=0;
  CaptureFailure retained_reason=CaptureFailure::None;
  bool retained=false; // 对象仍在，由 Host 沿用同一实例的上次完整值。
  bool fast_body=false; // 已按实际正文长度核过快槽预算，仅供同段克隆。
  // 平铺根直接借用包内字节区，长度包括主区及全部附加段。
  std::uint32_t flat_offset=0,flat_bytes=0;
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
  // ordinary事件段覆盖闭区间[first_sequence,watermark]；空记录仍保留已丢外观的水位。
  // 两者均为0表示本包不携带事件交接，body快包不能代替ordinary确认。
  std::uint64_t event_first_sequence=0,event_watermark=0;
  bool request_baseline=false;
};
// 缓存的引用号属于原生产目录；克隆时按物理身份转入当前交接目录，资源handle只追加。
struct CaptureAppendContext {
  void *owner=nullptr;
  CaptureLink (*reference)(void *,const CaptureReferenceEntry &) noexcept=nullptr;
};
// 所有capacity在冷准备期给定并记入Values总账；逐帧只重置游标，不改变vector容量。
// 普通快照与后台单根索引使用同一上限；快速 Body 槽仍保留独立小容量。
inline constexpr std::uint32_t kOrdinaryCaptureCells=131072;
struct CaptureCapacity { std::uint32_t records=512,cells=16384,paths=32768,bytes=1u<<20,references=0; bool joint_clone_source=false; };
class CaptureBuffer final {
public:
  explicit CaptureBuffer(CaptureCapacity);
  static std::uint64_t EstimatedBytes(CaptureCapacity) noexcept;
  std::uint64_t RetainedBytes() const noexcept {return memory_->Bytes();}
  CaptureBuffer(const CaptureBuffer &)=delete;
  CaptureBuffer &operator=(const CaptureBuffer &)=delete;
private:
  friend class CaptureValueWriter;
  // 只在可作为联合来源的槽保留，记录的 cells 范围由 CaptureRecord 持有。
  // reference_first/end 定位非空引用条目的包络，间隙仍由 cell.kind 区分。
  struct BodyCloneRange {
    std::uint32_t path=UINT32_MAX,paths=0,byte=0,bytes=0,reference_first=UINT32_MAX,reference_end=0;
  };
  memory::Lease memory_;
  std::vector<CaptureRecord> records_;
  std::vector<CaptureCell> cells_;
  std::vector<CodecPathStep> paths_;
  std::vector<std::byte> bytes_;
  std::vector<CaptureReferenceEntry> references_;
  std::vector<BodyCloneRange> body_clone_ranges_;
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
  std::span<std::byte> BeginFlatRoot(CaptureSubject,std::uint16_t layout,std::uint32_t max_bytes) noexcept;
  bool FinishFlatRoot(std::uint32_t bytes) noexcept;
  void AbortRoot() noexcept;
  // 回卷同一对象的所有流，随后只提交身份标记。只由普通采集的对象根使用。
  struct RootCheckpoint {std::uint32_t records,cells,paths,bytes;std::uint64_t references;};
  RootCheckpoint Checkpoint() const noexcept {return {records_,cells_,paths_,bytes_,reference_limit_};}
  bool RetainRoot(RootCheckpoint,CaptureSubject,std::uint16_t layout,CaptureFailure) noexcept;
  bool ReuseRoot(CaptureSubject,std::uint16_t layout,std::uint64_t base) noexcept;
  void RequestBaseline() noexcept {request_baseline_=true;}
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
  bool EventRange(std::uint64_t first_sequence,std::uint64_t watermark) noexcept;
  // 克隆已经封存的纯值，不再访问原生对象；保留每根真实生产身份与修订。
  bool Append(const CapturePacket &,std::uint32_t record,CaptureAppendContext={}) noexcept;
  // 当前已完成根的自有值视图；调用方只在持有 writer 时借用。
  CapturePacket Completed(std::span<const CaptureReferenceEntry>) const noexcept;
  std::uint32_t WrittenRoots() const noexcept {return records_;}
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
  friend class CapturePlan;
  friend class CaptureExecution;
  friend class CaptureExecutor;
  friend class cs::game::BinaryEntities;
  // 每个执行器递归实例独占一个路径游标；路径由选中投影及已核元素构成。
  struct BoundPath {std::uint32_t offset=UINT32_MAX;};
  bool Put(const CodecLocation &,CaptureCellKind,ScalarKind,std::uint32_t,std::uint16_t,std::span<const std::byte>) noexcept;
  bool Store(const CodecLocation &,CaptureCellKind,ScalarKind,std::uint32_t,std::uint16_t,std::span<const std::byte>,BoundPath *) noexcept;
  bool PutScalarSlice(const CodecLocation &,ScalarKind,std::uint32_t,std::span<const std::byte>) noexcept;
  bool WriteBound(const CodecLocation &,BoundPath &,CaptureCellKind,std::uint32_t value,std::uint16_t aux=0) noexcept;
  bool WriteBoundScalar(const CodecLocation &,BoundPath &,ScalarKind,std::uint32_t width,std::uint32_t count,
                        std::span<const std::byte>,bool slice,CaptureWrittenValue &) noexcept;
  // 仅消费执行器冷绑定的连续纯标量模板，私有入口不向handler或网络解码开放。
  bool WriteBoundScalarBatch(const CodecLocation &,BoundPath &,std::span<const CaptureCell>,std::span<const std::byte>) noexcept;
  bool SetSlice(const CodecLocation &,BoundPath *,ScalarKind,std::uint32_t width,std::uint32_t count,
                std::span<const std::byte>,CaptureWrittenValue &) noexcept;
  bool FailCapacity(const CodecLocation &,const char *arena,std::size_t used,std::size_t limit,std::uint64_t requested) noexcept;
  bool OpenRoot(CaptureSubject,std::uint16_t,BodyStream,std::uint32_t,bool flat=false) noexcept;
  std::span<std::byte> ReserveScalars(std::uint32_t,const CodecLocation &) noexcept;
  void MarkFastBody(bool eligible,bool reviewed_pair) noexcept {
    buffer_.records_[records_].fast_body=eligible;
    shared_body_=eligible&&reviewed_pair;
  }
  bool CloneOrdinaryBody(const CaptureValueWriter &source,std::uint32_t index,CaptureSubject,std::uint16_t layout,
                      std::span<const CaptureReferenceEntry>,CaptureAppendContext,capture::BodyCloneCounts &) noexcept;
  CaptureBuffer &buffer_;
  CaptureWitness witness_;
  CaptureCatalogStamp catalog_;
  std::uint64_t reference_limit_=0;
  std::uint64_t event_first_sequence_=0,event_watermark_=0;
  CaptureDiagnostic diagnostic_;
  CaptureRoot root_;
  // 仅缓存本根已验证的纯值路径前缀与投影掩码；不缓存原生读取资格或对象状态。
  mutable std::array<CodecPathStep,kCodecMaxDepth> allowed_path_{};
  mutable std::array<std::uint16_t,kCodecMaxDepth> allowed_layout_{};
  mutable std::array<std::uint32_t,kCodecMaxDepth> allowed_mask_{};
  mutable std::size_t allowed_depth_=0;
  std::uint32_t records_=0,cells_=0,paths_=0,bytes_=0,mark_cells_=0,mark_paths_=0,mark_bytes_=0;
  std::uint32_t flat_capacity_=0;
  std::uint32_t first_reference_cell_=UINT32_MAX,last_reference_end_=0;
  bool shared_body_=false;
  bool open_=false,sealed_=false;
  bool catalog_set_=false,request_baseline_=false;
};
// 后台从精确成员／资源快照解析自有handle。false保留Pending／缺资源，不编码为Null。
struct CaptureLinkResolver {
  void *context=nullptr;
  bool (*resolve)(void *,CaptureSubject,bool resource,std::uint8_t table,CaptureLink,std::uint16_t &) noexcept=nullptr;
  // 只允许已证实缺少承载的沿用成员映射为空；字段本身仍须允许空引用。
  bool allow_retained_null=false;
};
// 仅借用仍由consumer票据持有的packet；scratch是后台自有可复用区，不包含原生对象。
// 此适配器只服务现有正文codec，缺值／错类型抛Gap发生在后台，不进入主线程热路径。
class CapturedBodySource final : public BodyValueSource {
public:
  CapturedBodySource(const CapturePacket &,std::uint32_t record,CaptureLinkResolver,std::span<std::byte> link_scratch,
                     std::span<std::uint32_t> index_scratch);
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
  std::span<const std::uint32_t> index_;
};
}
