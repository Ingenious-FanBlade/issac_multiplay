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
Ring<Event, 512> events;
std::atomic<uint64_t> sequence{}, dropped{}, truncated{};
std::atomic<bool> stopping{};
HANDLE worker = nullptr;
HANDLE output = INVALID_HANDLE_VALUE;
std::string path;
LARGE_INTEGER frequency{};

// 最坏每个字节转为六字节转义，缓冲容量覆盖整个 Event。
// 不使用 CRT 文件锁、堆分配或 ZHL，允许在进程终止通知中尽力落盘。
void WriteEvent(const Event &e) noexcept {
  if (output == INVALID_HANDLE_VALUE)
    return;
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
  if (!WriteFile(output, line, DWORD(p - line), &written, nullptr) || written != DWORD(p - line))
    ++dropped;
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
      Sleep(10);
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
        uint64_t(now.QuadPart % frequency.QuadPart) * 1000000 / frequency.QuadPart;
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
  path = directory + "/boot-" + std::to_string(GetCurrentProcessId()) + ".jsonl";
  output = CreateFileA(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL, nullptr);
  if (output == INVALID_HANDLE_VALUE)
    throw std::runtime_error("trace_open_failed");
  worker = CreateThread(nullptr, 0, WriteLoop, nullptr, 0, nullptr);
  if (!worker) {
    CloseHandle(output);
    output = INVALID_HANDLE_VALUE;
    throw std::runtime_error("trace_thread_failed");
  }
}
void Trace(const char *ev, const char *format, ...) noexcept {
  if (!worker || stopping.load())
    return;
  Event e = MakeEvent(ev);
  va_list args;
  va_start(args, format);
  const auto formatted = _vsnprintf_s(e.detail, sizeof(e.detail), _TRUNCATE, format, args);
  va_end(args);
  if (formatted < 0) {
    e.truncated = true;
    ++truncated;
    ++dropped;
  }
  if (!events.Push(e))
    ++dropped;
}
uint64_t TraceDropped() noexcept {
  return dropped.load();
}
uint64_t TraceTruncated() noexcept {
  return truncated.load();
}
const std::string &TracePath() noexcept {
  return path;
}

// 关闭配置时只留下 boot；正常线程环境下等待后台写完后关闭句柄。
void FinishTrace() noexcept {
  stopping = true;
  if (worker) {
    if (WaitForSingleObject(worker, 2000) != WAIT_OBJECT_0)
      return;
    CloseHandle(worker);
    worker = nullptr;
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
              "trace_dropped=%llu trace_truncated=%llu",
              dropped.load(), truncated.load());
  WriteEvent(e);
  CloseHandle(output);
  output = INVALID_HANDLE_VALUE;
}
} // namespace cs
