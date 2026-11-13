#pragma once
#include "replica/stream_model.h"
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>

namespace cs::replica::stream {
std::string LimitsFields(const Limits &,const std::string &prefix);
enum class CpuStage : unsigned { Capture, Encode, Compress, Queue, Decode, NativePrepare, NativeApply, Draw, Network, Ipc, NativeRetire, Build, MainCapture, FrameInterval, SourceLua, SourceNative, SourceCopy, SourceControl, SourceHandoff, Count };
// 固定对数桶保留计数/总量/最大值；生产不保存每tick日志或无界历史。
struct Duration final {
  std::uint64_t count=0,total_us=0,max_us=0;
  std::array<std::uint64_t,32> buckets{};
  void Observe(std::uint64_t us) noexcept;
};
struct Metrics final {
  WorldScope scope{};
  std::uint64_t route=0,missing=0,replacements=0,stale_rejections=0;
  std::uint64_t packets=0,payload_bytes=0,wire_bytes=0,bundle_messages=0;
  std::array<std::uint64_t,4> lane_bytes{},lane_packets{},lane_encoded{},lane_compressed{};
  std::uint64_t queue_items=0,queue_high=0,candidates=0,retained=0;
  std::uint64_t first_prepare_ms=0,first_receive_ms=0,display_ready_ms=0;
  std::uint64_t encoded_bytes=0,compressed_bytes=0;
  std::array<Duration,unsigned(CpuStage::Count)> duration{},cpu{};
  void Bind(WorldScope,std::uint64_t epoch=0) noexcept;
  void Queue(std::uint64_t size) noexcept {queue_items=size;if(size>queue_high)queue_high=size;}
  std::string Fields(const std::string &prefix,unsigned page=0) const;
};
// 诊断邮箱使用try_lock；采样缺失可观察，但诊断失败从不阻止输入/同步生效。
class MetricsMailbox final {
public:
  void Publish(const Metrics &v) noexcept {if(mutex_.try_lock()){value_=v;mutex_.unlock();}else ++missed_;}
  Metrics Read() const noexcept {std::lock_guard lock(mutex_);auto out=value_;out.missing+=missed_.load();return out;}
private:
  mutable std::mutex mutex_;
  Metrics value_{};
  std::atomic<std::uint64_t> missed_{0};
};
std::uint64_t ThreadCpuMicros() noexcept;
#ifdef ISAACCS_E2E
void E2ETime(const Metrics *,CpuStage,std::uint64_t,std::uint64_t) noexcept;
#endif
inline std::uint64_t MetricMicros() noexcept {return std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
// CPU窗口和墙钟区间分别命名；不把不同设备绝对时钟相减。
class Measure final {
public:
  Measure(Metrics &m,CpuStage stage) noexcept:metrics_(m),stage_(stage),start_(MetricMicros()),cpu_start_(ThreadCpuMicros()){}
  ~Measure(){const auto wall=MetricMicros()-start_,end=ThreadCpuMicros();const auto cpu=end!=UINT64_MAX&&cpu_start_!=UINT64_MAX&&end>=cpu_start_?end-cpu_start_:UINT64_MAX;metrics_.duration[unsigned(stage_)].Observe(wall);if(cpu!=UINT64_MAX)metrics_.cpu[unsigned(stage_)].Observe(cpu);else ++metrics_.missing;
#ifdef ISAACCS_E2E
    E2ETime(&metrics_,stage_,wall,cpu);
#endif
  }
private:
  Metrics &metrics_;CpuStage stage_;std::uint64_t start_,cpu_start_;
};
} // namespace cs::replica::stream
