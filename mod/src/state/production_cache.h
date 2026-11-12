#pragma once
#include "state/capture_values.h"

namespace cs::state {
// 正常生产的主线程缓存：固定页池、固定目录；不借用原生对象，也不向后台暴露可覆盖页。
// ordinary发布必须Append复制到交接票据；本缓存不构成第三条确认序列。
class ProductionCache final {
public:
  ProductionCache(std::thread::id,std::uint32_t bytes=4u<<20,std::uint32_t groups=1024);
  bool Store(const CapturePacket &,CaptureDiagnostic &) noexcept;
  bool Append(const StateScope &,CaptureValueWriter &,std::uint32_t &roots,CaptureDiagnostic &,CaptureAppendContext) const noexcept;
  bool Contains(const StateScope &,std::uint16_t purpose) const noexcept;
  std::uint64_t Revision(CaptureSubject,std::uint16_t purpose,std::uint64_t instance) const noexcept;
  void Retire(CaptureSubject) noexcept;
  void RetirePurpose(CaptureSubject,std::uint16_t) noexcept;
  void Clear() noexcept;
  std::uint64_t RetainedBytes() const noexcept {return memory_->Bytes();}
private:
  struct Entry {std::uint32_t page=0,pages=0,records=0,cells=0,paths=0,bytes=0,references=0;CaptureWitness witness;CaptureCatalogStamp catalog;CaptureSubject subject;std::uint16_t purpose=0;std::uint64_t instance=0,generation=0;};
  CapturePacket View(const Entry &) const noexcept;
  void Release(Entry &) noexcept;
  std::thread::id thread_;
  memory::Lease memory_;
  std::vector<std::uint64_t> arena_;
  std::vector<std::uint8_t> pages_;
  std::vector<Entry> entries_;
  std::vector<std::uint32_t> reference_map_,reference_seen_;
  std::uint32_t reference_generation_=0;
};
}
