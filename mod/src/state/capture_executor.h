#pragma once
#include "state/capture_values.h"

namespace cs::state {
// 来源游标仅在当前同步调用借用。opaque供具名owner保存头／holder／修订证言，
// 绝不写入CapturePacket；open／element／close均只读、零分配且不调用游戏生产器。
struct CaptureTraversalSource {
  std::array<std::uint64_t,4> opaque{};
  // 当前root/use内同一原生容器／holder的非零证言；各投影必须一致，仅同步栈借用。
  std::uint64_t identity=0;
  std::uint32_t count=0,slot_mask=0;
  bool present=true;
};
struct CaptureTraversalBinding {
  std::string_view name;
  Operation operation=Operation::Nested;
  bool (*open)(const CaptureFieldAccess &,CaptureTraversalSource &,CaptureDiagnostic &) noexcept=nullptr;
  // nested使用element=FFFF；container为逻辑序号；choice为实际槽号并返回真实动态view布局。
  bool (*element)(const CaptureFieldAccess &,const CaptureTraversalSource &,std::uint16_t element,
                  CaptureRoot &child,CaptureDiagnostic &) noexcept=nullptr;
  // 复核原头、共享holder、实际动态类／资源与同use来源，不能用恒真回调代替。
  bool (*close)(const CaptureFieldAccess &,const CaptureTraversalSource &,CaptureDiagnostic &) noexcept=nullptr;
};
// 704条件消费当前资源句柄的已核配置；不在主线程提前分配Peer资源序号或调用getter。
struct CaptureExecutionContext {
  void *context=nullptr;
  bool (*item)(void *,const CaptureWitness &,std::uint8_t table,CaptureLink,CodecItemConfig &) noexcept=nullptr;
};
// 与正文codec保持相同保护范围；reliable只供完整ordinary交接，不扩大网络配额。
struct CaptureExecutionLimits {
  std::uint32_t bytes=65536,field_visits=262144;
  std::uint8_t depth=32;
  bool reliable=false;
};
// 冷准备容量与每次执行预算分开；不得在逐帧路径调整或销毁这些保留区。
struct CaptureExecutorCapacity {std::uint32_t copy_bytes=65536;std::uint8_t depth=32;};
// 冷准备一次分配全部递归状态和copy scratch，单一主线程owner逐根复用；plan须活得更久。
// 它只解释计划，真实J273来源／身份预约与游戏桥由各域owner接入，缺绑定直接失败。
class CaptureExecutor final {
public:
  CaptureExecutor(const CapturePlan &,std::span<const CaptureTraversalBinding>,std::thread::id owner,
                  CaptureExecutorCapacity={},CaptureExecutionContext={});
  CaptureExecutor(const CaptureExecutor &)=delete;
  CaptureExecutor &operator=(const CaptureExecutor &)=delete;
  // RTTI根先Identify／SelectArm，无RTTI／protocol由具名owner指定。采集整个所选流，
  // 完整成功才FinishRoot；任何字段失败回卷整根且writer永久保留整包失败。
  bool Execute(const CaptureWindow &,CaptureRoot,BodyStream,CaptureValueWriter &,CaptureDiagnostic &,
               CaptureExecutionLimits={}) noexcept;
  std::uint64_t RetainedBytes() const noexcept {return memory_->Bytes();}
private:
  friend class CaptureExecution;
  // 名称在构造期消解，此表的逐帧访问只按生成全局字段索引进行。
  struct Binding {
    decltype(CaptureTraversalBinding::open) open=nullptr;
    decltype(CaptureTraversalBinding::element) element=nullptr;
    decltype(CaptureTraversalBinding::close) close=nullptr;
  };
  // 每个递归实例独占一片字段状态；selector／配置句柄和来源游标不跨根复用。
  struct FieldState {
    CaptureTraversalSource source;
    CaptureLink link;
    std::uint64_t bits=0;
    ScalarKind scalar=ScalarKind::None;
    bool opened=false,seen=false,integer=false,linked=false;
  };
  const CapturePlan &plan_;
  std::thread::id owner_;
  CaptureExecutorCapacity capacity_;
  CaptureExecutionContext context_;
  memory::Lease memory_;
  std::vector<Binding> bindings_;
  std::vector<FieldState> states_;
  std::vector<std::byte> copy_;
  bool active_=false;
};
}
