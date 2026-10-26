#pragma once
// tx-04-08 world 文件出口：manifest/captures/lifecycle-events/diagnostics/result
// 的写入，以及 reader 之后独立的编码写盘线程。
//
// 线程与所有权：writer 线程只处理 storage 队列里的 owned values，不解引用
// 任何游戏地址、不调用 getter/Lua/RNG/setter；所有文件操作都在该线程。
//
// 文件格式（新的 isaac.cs.world-record v1 domain，与旧 t0 domain 区分）：
//   manifest.json          domain/schema/profile、布局声明与证据、字段/来源/
//                          预算、假设/未知、Hook 清单引用
//   captures.jsonl         完整/不完整 capture 与 gap 控制记录
//   lifecycle-events.jsonl 生命周期事件（含 call/parent、成员位集）
//   diagnostics.jsonl      准备/背压范围/关闭/耗时/内存/队列与自有故障
//   result.json            终态、逐原因统计、生产/复制/写入数、未交付范围
//
// 输出目录由固定根目录 + 受限 ASCII label 组成，最终目录使用 create-new，
// 不覆盖已有产物；所有文件同样 create-new。

#include "world/model.h"
#include "world/storage.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace cs::world {

// 终态：完整收口、world 自有故障、或排空/写入未确认（不能冒充 Closed）。
enum class WriterOutcome : std::uint32_t { Closed, Error, Unconfirmed };
const char *WriterOutcomeName(WriterOutcome value) noexcept;

struct WriterConfig final {
  std::wstring root;   // 固定根目录（绝对或相对游戏工作目录；不支持 UNC）
  std::string label;   // 受限 ASCII 标签，作为 create-new 的最终目录
  std::string session; // 会话标识（受限 ASCII）；写入 manifest 与每条记录
  // 正常关闭的后台期限；由调用方在 Close(wait_ms) 时传入（默认取此值）。
  // 只用于收口，不推进 Authority。
  std::uint32_t close_timeout_ms = 10000;
};

struct WriterStatus final {
  bool opened = false;
  bool files_flushed = false;   // 三个 JSONL 都完成 flush + FlushFileBuffers
  bool producer_done = false;   // reader 已关闭队列生产端
  bool drain_complete = false;  // writer 看到队列排空且生产端已结束
  bool write_failed = false;
  std::uint64_t records_written = 0, events_written = 0, gaps_written = 0,
                diagnostics_written = 0, discarded_items = 0;
  std::uint64_t write_calls = 0, bytes_written = 0;
  std::uint32_t last_error = 0;
  ThreadState thread = ThreadState::NotStarted;
  WriterOutcome outcome = WriterOutcome::Unconfirmed;
  Reason fault = Reason::None;
};

// 编码线程状态（writer.cpp）。对外只暴露值查询，不暴露文件/线程细节。
struct WriterRuntime;

class WorldWriter final {
public:
  WorldWriter() noexcept;
  ~WorldWriter();
  WorldWriter(const WorldWriter &) = delete;
  WorldWriter &operator=(const WorldWriter &) = delete;

  // 在 Preparing 后台线程调用，且必须在 storage.Prepare 成功之后：
  // 创建目录与文件、写 manifest、启动编码线程。storage 必须比 writer 活得久
  // （writer 线程在 Close 之前一直读取它的队列/诊断/故障开关）。
  bool Open(const WriterConfig &config, const ManifestInput &manifest,
            WorldStorage &storage) noexcept;
  // 只发停止信号，不等待；主线程路径必须用它。
  void RequestStop() noexcept;
  // 有界等待 writer 线程；wait_ms=0 时只返回当前是否已收口。
  bool Close(std::uint32_t wait_ms) noexcept;
  bool Closed() const noexcept;
  WriterStatus Status() const noexcept;
  // 实际输出目录；未打开时为空串。
  const std::wstring &Directory() const noexcept;

private:
  std::shared_ptr<WriterRuntime> impl_;
};

} // namespace cs::world