#pragma once
#include "replica/stream_capture.h"
#include "core/owned_source.h"

namespace cs::replica::stream {
// 主线程只投递自有值和有序控制；Registry、Store、版本与构建树只属于一个后台owner。
class CaptureWorker final {
public:
  CaptureWorker();~CaptureWorker();
  CaptureWorker(const CaptureWorker &)=delete;CaptureWorker &operator=(const CaptureWorker &)=delete;
  void Begin(std::uint64_t) noexcept;
  void Stop() noexcept;
  void Invalidate(const char *) noexcept;
  void RequestBaseline() noexcept;
  bool NeedsBaseline() const noexcept;
  void Ordinary(std::shared_ptr<const world::observation::LiveBatch>) noexcept;
  void PlayerPost(BodySample,capture::Source) noexcept;
  void BodyGap(const char *) noexcept;
  void PhysicalRetire(unsigned,std::uint64_t,std::uint64_t,std::uint64_t,std::uint64_t,std::uint64_t,std::uint64_t) noexcept;
  CaptureStatus Status() const noexcept;
  Metrics Statistics() const noexcept;
  std::shared_ptr<const Publication> Latest() const noexcept;
  std::shared_ptr<const Publication> Baseline() const noexcept;
  std::vector<std::shared_ptr<const Publication>> Bodies() const;
private:
  struct State;struct Output;
  std::shared_ptr<const Output> Read() const noexcept;
  void Control(unsigned,const char *,std::array<std::uint64_t,7> = {}) noexcept;
  std::shared_ptr<State> state_;
  mutable std::shared_ptr<const Output> view_;
  std::uint64_t generation_=0,order_=0,required_capture_=0;
  std::array<char,128> last_body_gap_{};
};
}
