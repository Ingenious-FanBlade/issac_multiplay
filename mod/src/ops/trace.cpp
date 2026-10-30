#include "runtime.h"
#include <cstdarg>
#include <cstdio>
#include <filesystem>

namespace cs {
namespace {
// 固定大小事件同时用于后台写入和进程退出时的补写。
struct Event {
  uint64_t t, seq;
  char ev[40], detail[3500];
  bool truncated;
};
// 启动时可在一个主线程调用内产生数千条持久入口记录；容量覆盖已观察的
// 突发，仍保持有界、逐事件保存，不采样或清除丢失计数（约 14 MiB 常驻）。
constexpr unsigned kTraceCapacity = 4096;
Ring<Event, kTraceCapacity> events;
std::atomic<uint64_t> sequence{}, dropped{}, truncated{};
std::atomic<uint64_t> queue_dropped{}, write_errors{}, wake_errors{};
std::atomic<DWORD> last_error{};
std::atomic<bool> stopping{};
HANDLE worker = nullptr;
// 自动复位事件：生产者只通知，文件写入始终留在后台线程。
HANDLE wake = nullptr;
HANDLE output = INVALID_HANDLE_VALUE;
std::string path;
LARGE_INTEGER frequency{};

// 最坏每个字节转为六字节转义，缓冲容量覆盖整个 Event。
// 不使用 CRT 文件锁、堆分配或 ZHL，允许在进程终止通知中尽力落盘。
void WriteEvent(const Event &e) noexcept {
  if (output == INVALID_HANDLE_VALUE) {
    last_error.store(ERROR_INVALID_HANDLE);
    ++write_errors;
    ++dropped;
    return;
  }
  char line[24000];
  char *p = line;
  auto literal = [&](const char *s) noexcept {
    while (*s)
      *p++ = *s++;
  };
  auto number = [&](uint64_t n) noexcept {
    char digits[20];
    unsigned count = 0;
    do {
      digits[count++] = char('0' + n % 10);
      n /= 10;
    } while (n);
    while (count)
      *p++ = digits[--count];
  };
  auto quoted = [&](const char *s) noexcept {
    constexpr char hex[] = "0123456789ABCDEF";
    *p++ = '"';
    for (; *s; ++s) {
      const auto c = static_cast<unsigned char>(*s);
      if (c == '"' || c == '\\') {
        *p++ = '\\';
        *p++ = char(c);
      } else if (c < 32) {
        literal("\\u00");
        *p++ = hex[c >> 4];
        *p++ = hex[c & 15];
      } else {
        *p++ = char(c);
      }
    }
    *p++ = '"';
  };
  literal("{\"t\":");
  number(e.t);
  literal(",\"seq\":");
  number(e.seq);
  literal(",\"ev\":");
  quoted(e.ev);
  literal(",\"detail\":");
  quoted(e.detail);
  literal(",\"truncated\":");
  literal(e.truncated ? "true" : "false");
  literal("}\n");
  DWORD written = 0;
  const bool written_ok =
      WriteFile(output, line, DWORD(p - line), &written, nullptr) != 0;
  if (!written_ok || written != DWORD(p - line)) {
    last_error.store(written_ok ? ERROR_WRITE_FAULT : GetLastError());
    ++write_errors;
    ++dropped;
  }
}

// 运行期间只有后台线程消费队列并写文件。
DWORD WINAPI WriteLoop(void *) noexcept {
  Event e{};
  for (;;) {
    if (events.Pop(e)) {
      WriteEvent(e);
    } else if (stopping.load()) {
      break;
    } else {
      // Push 完整发布后 SetEvent；即使通知先于等待发生，自动复位事件也保留
      // 信号，不再固定睡眠 10 ms 才发现启动突发。超时只作退出兜底。
      const auto waited = WaitForSingleObject(wake, 100);
      if (waited != WAIT_OBJECT_0 && waited != WAIT_TIMEOUT) {
        last_error.store(GetLastError());
        ++wake_errors;
        ++dropped;
        return 1;
      }
    }
  }
  return 0;
}

// 生产者只读系统时钟和原子计数，不接触输出句柄。
Event MakeEvent(const char *ev) noexcept {
  LARGE_INTEGER now{};
  QueryPerformanceCounter(&now);
  Event e{};
  e.t = uint64_t(now.QuadPart / frequency.QuadPart) * 1000000 +
        uint64_t(now.QuadPart % frequency.QuadPart) * 1000000 /
            frequency.QuadPart;
  e.seq = sequence.fetch_add(1) + 1;
  strncpy_s(e.ev, ev, _TRUNCATE);
  return e;
}
} // namespace

// 初始化发生在 ModInit；文件独占新建，避免覆盖前一次进程的记录。
void StartTrace(const std::string &directory) {
  std::filesystem::create_directories(directory);
  if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
    throw std::runtime_error("trace_clock_failed");
  path =
      directory + "/boot-" + std::to_string(GetCurrentProcessId()) + ".jsonl";
  output = CreateFileA(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                       CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (output == INVALID_HANDLE_VALUE)
    throw std::runtime_error("trace_open_failed");
  wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (!wake) {
    CloseHandle(output);
    output = INVALID_HANDLE_VALUE;
    throw std::runtime_error("trace_event_failed");
  }
  worker = CreateThread(nullptr, 0, WriteLoop, nullptr, 0, nullptr);
  if (!worker) {
    CloseHandle(wake);
    wake = nullptr;
    CloseHandle(output);
    output = INVALID_HANDLE_VALUE;
    throw std::runtime_error("trace_thread_failed");
  }
}
void Trace(const char *ev, const char *format, ...) noexcept {
  if (!worker || stopping.load())
    return;
  // 过滤发生在格式化/计时/入队之前；生产不运行逐tick、逐query及持久入口明细录制。
  if (!Settings.diagnostics) {
    bool important = std::strstr(ev, "error") || std::strstr(ev, "fault");
    for (const auto *name : {"boot", "config", "shutdown", "session", "peer",
                             "net", "tick_reject"})
      important |= std::strcmp(ev, name) == 0;
    if (!important)
      return;
  }
  Event e = MakeEvent(ev);
  va_list args;
  va_start(args, format);
  const auto formatted =
      _vsnprintf_s(e.detail, sizeof(e.detail), _TRUNCATE, format, args);
  va_end(args);
  if (formatted < 0) {
    e.truncated = true;
    ++truncated;
    ++dropped;
  }
  if (!events.Push(e)) {
    ++queue_dropped;
    ++dropped;
  } else if (!SetEvent(wake)) {
    last_error.store(GetLastError());
    ++wake_errors;
    ++dropped;
  }
}
uint64_t TraceDropped() noexcept { return dropped.load(); }
uint64_t TraceTruncated() noexcept { return truncated.load(); }
// 只读诊断区分队列容量、磁盘和唤醒失败；不改变 TraceDropped 的失败语义。
std::string TraceFields() {
  return "trace_capacity=" + std::to_string(kTraceCapacity) +
         "\ntrace_queue_dropped=" + std::to_string(queue_dropped.load()) +
         "\ntrace_write_errors=" + std::to_string(write_errors.load()) +
         "\ntrace_wake_errors=" + std::to_string(wake_errors.load()) +
         "\ntrace_native_error=" + std::to_string(last_error.load()) + "\n";
}
const std::string &TracePath() noexcept { return path; }

// 关闭配置时只留下 boot；正常线程环境下等待后台写完后关闭句柄。
void FinishTrace() noexcept {
  stopping = true;
  if (wake)
    SetEvent(wake);
  if (worker) {
    if (WaitForSingleObject(worker, 2000) != WAIT_OBJECT_0)
      return;
    CloseHandle(worker);
    worker = nullptr;
  }
  if (wake) {
    CloseHandle(wake);
    wake = nullptr;
  }
  if (output != INVALID_HANDLE_VALUE) {
    CloseHandle(output);
    output = INVALID_HANDLE_VALUE;
  }
}

// 只用于 DLL_PROCESS_DETACH 且 lpReserved 非空：系统已结束其他线程。
// 不等待 worker、不创建线程；补写剩余事件和真正的 shutdown。
void FinishTraceOnProcessExit() noexcept {
  if (output == INVALID_HANDLE_VALUE)
    return;
  stopping = true;
  Event e{};
  while (events.Pop(e))
    WriteEvent(e);
  e = MakeEvent("shutdown");
  _snprintf_s(e.detail, sizeof(e.detail), _TRUNCATE,
              "trace_dropped=%llu trace_truncated=%llu trace_capacity=%u "
              "trace_queue_dropped=%llu trace_write_errors=%llu "
              "trace_wake_errors=%llu trace_native_error=%lu",
              dropped.load(), truncated.load(), kTraceCapacity,
              queue_dropped.load(), write_errors.load(), wake_errors.load(),
              last_error.load());
  WriteEvent(e);
  CloseHandle(output);
  output = INVALID_HANDLE_VALUE;
  if (wake) {
    CloseHandle(wake);
    wake = nullptr;
  }
}
} // namespace cs
