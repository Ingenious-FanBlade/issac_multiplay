// tx-04-08 world 文件出口实现：JSONL 编码、manifest 与终态报告。
//
// 线程约定：只有 writer 线程访问文件与编码 scratch；主线程只读取原子状态。
// 编码只读取 storage 交付的 owned values，不接触游戏地址、getter、Lua 或 RNG。

// clang-format off
#include <windows.h>
// clang-format on

#include "world/writer.h"

#include <algorithm>
#include <charconv>
#include <cstring>
#include <cwchar>
#include <limits>
#include <new>
#include <string_view>
#include <utility>

namespace cs::world {
namespace {

// 每行 flush 周期：只是把用户缓冲交给文件，不是持久化保证。
constexpr std::uint32_t kFlushIntervalMs = 250;
// captures/events 各 1 MiB；diagnostics 与 manifest/result 使用小块缓冲。
constexpr std::size_t kRecordBufferBytes = kIoBufferBytes;
constexpr std::size_t kDiagnosticBufferBytes = 64u << 10;
// 成员关系分组的 scratch：排序数组 + 起始偏移 + 游标。
constexpr std::size_t kMembershipScratchBytes =
    std::size_t(kMembershipBudget) * sizeof(RawMembership);
constexpr std::size_t kMembershipIndexBytes =
    std::size_t(kObjectBudget + 1) * sizeof(std::uint32_t) * 2;
constexpr const char *kFloatComparison =
    "finite exact bits; normalize signed zero; retain raw bits";
constexpr const char *kHpEvidence =
    "USER_ACCEPTED_ASSUMPTION: generic Entity HP represents life; Player hearts "
    "remain separate";
constexpr const char *kCurrentPlayerRule =
    "exact pair remains valid across a normal native PlayerUpdate in an "
    "Authority simulation window; FIRST_OBSERVED and missing birth history "
    "remain unchanged";
constexpr const char *kUnconfirmedFields[6] = {
    "Player.State", "Player.Timeout",  "Tear.State",
    "Tear.Timeout", "Projectile.State", "Projectile.Timeout"};
constexpr const char *kNativeDiagnosticsExcluded[5] = {
    "address", "vptr", "thread", "container_descriptor", "wall_clock"};

std::uint64_t NowMilliseconds() noexcept { return GetTickCount64(); }

// 受限 ASCII label：只允许字母数字和 . _ -，且禁止 "."/".."。
bool SafeLabel(std::string_view label) noexcept {
  return ValidWorldLabel(label);
}
bool SafeSession(std::string_view session) noexcept {
  if (session.empty() || session.size() > 128)
    return false;
  for (const char c : session) {
    const auto byte = static_cast<unsigned char>(c);
    if (byte < 0x20 || byte >= 0x7F)
      return false;
  }
  return true;
}
std::wstring WidenAscii(std::string_view text) {
  std::wstring out;
  out.reserve(text.size());
  for (const char c : text)
    out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(c)));
  return out;
}

// 逐级创建目录；已存在且仍是目录时继续。最终输出目录另行 create-new。
// 只支持本地路径（含相对游戏工作目录的路径）；UNC 共享路径不在本轮范围。
bool EnsureDirectory(const std::wstring &path) {
  if (path.empty())
    return false;
  std::wstring partial;
  partial.reserve(path.size());
  for (std::size_t i = 0; i < path.size(); ++i) {
    const auto c = path[i];
    partial.push_back(c);
    const bool separator = c == L'\\' || c == L'/';
    if (!separator)
      continue;
    const auto probe = partial.substr(0, partial.size() - 1);
    if (probe.empty() || (probe.size() == 2 && probe[1] == L':'))
      continue;
    if (!CreateDirectoryW(probe.c_str(), nullptr) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
  }
  if (!CreateDirectoryW(path.c_str(), nullptr)) {
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
    const auto attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  return true;
}

// 输出目录必须不存在：create-new，绝不覆盖已有产物。
bool CreateOutputDirectory(const std::wstring &path) noexcept {
  if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
    return false;
  return CreateDirectoryW(path.c_str(), nullptr) != 0;
}

// ===========================================================================
// FileSink：CREATE_NEW + 有界缓冲 + 显式 flush/sync
// ===========================================================================
class FileSink final {
public:
  FileSink() noexcept = default;
  ~FileSink() { Close(); }
  FileSink(const FileSink &) = delete;
  FileSink &operator=(const FileSink &) = delete;

  bool Open(const std::wstring &path, std::size_t buffer_bytes) noexcept {
    if (handle_ != INVALID_HANDLE_VALUE)
      return false;
    handle_ = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                          CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
      last_error_ = GetLastError();
      ok_.store(false, std::memory_order_relaxed);
      return false;
    }
    if (!buffer_ || capacity_ < buffer_bytes) {
      buffer_.reset(new (std::nothrow) char[buffer_bytes]);
      if (!buffer_) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
        last_error_ = ERROR_NOT_ENOUGH_MEMORY;
        ok_.store(false, std::memory_order_relaxed);
        return false;
      }
      capacity_ = buffer_bytes;
    }
    used_ = 0;
    ok_.store(true, std::memory_order_relaxed);
    last_error_ = 0;
    return true;
  }
  bool Write(const char *data, std::size_t bytes) noexcept {
    if (handle_ == INVALID_HANDLE_VALUE) {
      // 句柄已被关闭（含 E2E 故障注入）：保留真实错误码而不是伪造成功。
      DWORD written = 0;
      write_calls_.fetch_add(1, std::memory_order_relaxed);
      WriteFile(INVALID_HANDLE_VALUE, data, static_cast<DWORD>((std::min)(bytes, std::size_t(4096))), &written, nullptr);
      last_error_ = GetLastError();
      ok_.store(false, std::memory_order_relaxed);
      return false;
    }
    while (bytes) {
      const auto room = capacity_ - used_;
      const auto take = (std::min)(room, bytes);
      std::memcpy(buffer_.get() + used_, data, take);
      used_ += take;
      data += take;
      bytes -= take;
      if (used_ == capacity_ && !Drain())
        return false;
    }
    return true;
  }
  // 把用户缓冲交给文件；成功只表示 WriteFile 成功。
  bool Flush() noexcept { return Drain(); }
  // 持久化边界：FlushFileBuffers 成功后才报告 files_flushed。
  bool Sync() noexcept {
    if (!Drain())
      return false;
    if (handle_ == INVALID_HANDLE_VALUE) {
      last_error_ = ERROR_INVALID_HANDLE;
      ok_.store(false, std::memory_order_relaxed);
      return false;
    }
    if (!FlushFileBuffers(handle_)) {
      last_error_ = GetLastError();
      ok_.store(false, std::memory_order_relaxed);
      return false;
    }
    return true;
  }
  bool Close() noexcept {
    bool good = true;
    if (handle_ != INVALID_HANDLE_VALUE) {
      good = Sync();
      if (!CloseHandle(handle_)) {
        if (good) {
          last_error_ = GetLastError();
          ok_.store(false, std::memory_order_relaxed);
        }
        good = false;
      }
      handle_ = INVALID_HANDLE_VALUE;
    }
    used_ = 0;
    return good;
  }
  // 仅 E2E：关闭自有句柄，使下一次真实写入以 ERROR_INVALID_HANDLE 失败。
  void DropHandleForTest() noexcept {
    if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
      handle_ = INVALID_HANDLE_VALUE;
    }
    used_ = 0;
    ok_.store(false, std::memory_order_relaxed);
  }
  bool Ok() const noexcept { return ok_.load(std::memory_order_relaxed); }
  bool IsOpen() const noexcept { return handle_ != INVALID_HANDLE_VALUE; }
  std::uint32_t LastError() const noexcept { return last_error_; }
  std::uint64_t WriteCalls() const noexcept {
    return write_calls_.load(std::memory_order_relaxed);
  }
  std::uint64_t BytesWritten() const noexcept {
    return bytes_written_.load(std::memory_order_relaxed);
  }

private:
  bool Drain() noexcept {
    if (handle_ == INVALID_HANDLE_VALUE) {
      last_error_ = ERROR_INVALID_HANDLE;
      ok_.store(false, std::memory_order_relaxed);
      return false;
    }
    if (!used_)
      return true;
    DWORD written = 0;
    if (!WriteFile(handle_, buffer_.get(), static_cast<DWORD>(used_), &written,
                   nullptr) ||
        written != used_) {
      last_error_ = GetLastError();
      ok_.store(false, std::memory_order_relaxed);
      return false;
    }
    write_calls_.fetch_add(1, std::memory_order_relaxed);
    bytes_written_.fetch_add(written, std::memory_order_relaxed);
    used_ = 0;
    return true;
  }

  HANDLE handle_ = INVALID_HANDLE_VALUE;
  std::unique_ptr<char[]> buffer_;
  std::size_t capacity_ = 0, used_ = 0;
  // 计数器与 ok 由 writer 线程更新、状态查询线程读取，故使用原子。
  std::atomic<std::uint64_t> write_calls_{0}, bytes_written_{0};
  std::atomic<bool> ok_{true};
  std::uint32_t last_error_ = 0;
};

// ===========================================================================
// JsonOut：只追加，不格式化浮点，不分配。字符串按 ASCII 转义。
// ===========================================================================
class JsonOut final {
public:
  explicit JsonOut(FileSink &file) noexcept : file_(&file) {}
  JsonOut &Raw(const char *data, std::size_t size) noexcept {
    file_->Write(data, size);
    return *this;
  }
  JsonOut &Lit(const char *text) noexcept {
    return Raw(text, std::strlen(text));
  }
  JsonOut &Str(std::string_view text) noexcept {
    Raw("\"", 1);
    for (const char c : text) {
      const auto byte = static_cast<unsigned char>(c);
      switch (byte) {
      case '"':
        Raw("\\\"", 2);
        break;
      case '\\':
        Raw("\\\\", 2);
        break;
      case '\n':
        Raw("\\n", 2);
        break;
      case '\r':
        Raw("\\r", 2);
        break;
      case '\t':
        Raw("\\t", 2);
        break;
      default:
        if (byte < 0x20 || byte >= 0x7F) {
          char escape[6] = {'\\', 'u', '0', '0', '0', '0'};
          constexpr char hex[] = "0123456789abcdef";
          escape[4] = hex[(byte >> 4) & 0x0F];
          escape[5] = hex[byte & 0x0F];
          Raw(escape, sizeof(escape));
        } else {
          Raw(&c, 1);
        }
        break;
      }
    }
    return Raw("\"", 1);
  }
  JsonOut &Key(const char *key) noexcept {
    Str(key);
    return Raw(":", 1);
  }
  // u64 一律写成规范十进制字符串。
  JsonOut &Dec(std::uint64_t value) noexcept {
    char buffer[24];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return Str(std::string_view(buffer,
                               static_cast<std::size_t>(result.ptr - buffer)));
  }
  JsonOut &Num(std::uint64_t value) noexcept {
    char buffer[24];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return Raw(buffer, static_cast<std::size_t>(result.ptr - buffer));
  }
  // 固定宽小写十六进制字符串。
  JsonOut &Hex(std::uint64_t value, unsigned digits) noexcept {
    char buffer[3 + 16];
    buffer[0] = '"';
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned i = 0; i < digits; ++i)
      buffer[1 + i] = hex[(value >> (4 * (digits - 1 - i))) & 0x0F];
    buffer[1 + digits] = '"';
    return Raw(buffer, 2 + digits);
  }
  JsonOut &HexBytes(const std::uint8_t *data, std::size_t size) noexcept {
    char buffer[3 + 32];
    constexpr char hex[] = "0123456789abcdef";
    std::size_t used = 0;
    buffer[used++] = '"';
    for (std::size_t i = 0; i < size; ++i) {
      buffer[used++] = hex[(data[i] >> 4) & 0x0F];
      buffer[used++] = hex[data[i] & 0x0F];
    }
    buffer[used++] = '"';
    return Raw(buffer, used);
  }
  JsonOut &Bool(bool value) noexcept {
    return value ? Lit("true") : Lit("false");
  }
  JsonOut &Null() noexcept { return Lit("null"); }
  bool Ok() noexcept { return file_->Ok(); }

private:
  FileSink *file_;
};

void Indent(JsonOut &j, int depth) noexcept {
  for (int i = 0; i < depth; ++i)
    j.Lit("  ");
}
// 便于把“逗号 + 换行 + 缩进”写成固定模式。
void Next(JsonOut &j, int depth) noexcept {
  j.Lit(",\n");
  Indent(j, depth);
}
void Begin(JsonOut &j, int depth) noexcept {
  j.Lit("{\n");
  Indent(j, depth);
}
void End(JsonOut &j, int depth) noexcept {
  j.Lit("\n");
  Indent(j, depth);
  j.Lit("}");
}

// ===========================================================================
// WriterState
// ===========================================================================
struct WriterState final {
  WriterConfig config{};
  ManifestInput manifest{};
  std::wstring directory;
  WorldStorage *storage = nullptr;
  ValueQueue *queue = nullptr;
  Diagnostics *diag = nullptr;
  BudgetLedger *ledger = nullptr;

  FileSink captures, events, diagnostics, small;
  std::unique_ptr<RawMembership[]> sorted_memberships;
  std::unique_ptr<std::uint32_t[]> membership_start;
  std::unique_ptr<std::uint32_t[]> membership_cursor;
  std::uint64_t charged = 0;

  std::atomic<bool> stop{false};
  std::atomic<bool> opened{false};
  std::atomic<std::uint32_t> thread_state{
      static_cast<std::uint32_t>(ThreadState::NotStarted)};
  std::atomic<bool> done{false}, flushed{false}, drain_complete{false};
  std::atomic<bool> result_written{false};
  std::atomic<bool> write_failed{false};
  std::atomic<std::uint64_t> records_written{0}, events_written{0},
      gaps_written{0}, diagnostics_written{0}, discarded{0};
  std::atomic<std::uint32_t> last_error{0};
  // 只有 writer 线程读写：逐原因统计在收尾时一次写成 result.json。
  std::array<std::uint64_t, kReasonCount> reason_counts{};

  void Fail(Reason reason, std::uint32_t code, const char *stage) noexcept {
    if (diag)
      diag->Fail(reason, FailSource::Writer, code, stage);
    last_error.store(code, std::memory_order_relaxed);
    write_failed.store(true, std::memory_order_relaxed);
  }
  std::uint32_t LastError() const noexcept {
    return last_error.load(std::memory_order_relaxed);
  }
};

// ===========================================================================
// manifest.json
// ===========================================================================

void WriteManifestFields(JsonOut &j, int depth) noexcept {
  Indent(j, depth);
  j.Lit("\"fields\":[\n");
  const auto fields = ProfileFields();
  for (std::size_t i = 0; i < fields.size(); ++i) {
    const auto &field = fields[i];
    Indent(j, depth + 1);
    j.Lit("{\"field_id\":");
    j.Hex(field.id, 4);
    j.Lit(",\"name\":");
    j.Str(field.name);
    j.Lit(",\"width\":");
    j.Num(field.width);
    j.Lit(",\"encoding\":");
    j.Str(EncodingName(field.encoding));
    j.Lit("}");
    j.Lit(i + 1 == fields.size() ? "\n" : ",\n");
  }
  Indent(j, depth);
  j.Lit("],\n");
}
void WriteManifestRequired(JsonOut &j, int depth) noexcept {
  Indent(j, depth);
  j.Lit("\"required\":{\n");
  for (std::uint32_t value = 0; value < kClassCount; ++value) {
    const auto kind = static_cast<Class>(value);
    const auto ids = ProfileFieldsFor(kind);
    Indent(j, depth + 1);
    j.Str(ClassName(kind));
    j.Lit(":[");
    for (std::size_t i = 0; i < ids.size(); ++i) {
      j.Hex(ids[i], 4);
      if (i + 1 != ids.size())
        j.Lit(",");
    }
    j.Lit("]");
    j.Lit(value + 1 == kClassCount ? "\n" : ",\n");
  }
  Indent(j, depth);
  j.Lit("},\n");
}
void WriteManifestLayout(JsonOut &j, const ManifestInput &input,
                         int depth) noexcept {
  const bool declared = input.layout_field_count > 0;
  const bool complete = input.layout_field_count >= ProfileFields().size();
  Indent(j, depth);
  j.Lit("\"layout\":{\n");
  Indent(j, depth + 1);
  j.Lit("\"state\":");
  j.Str(declared ? (complete ? "DECLARED" : "INCOMPLETE") : "UNDECLARED");
  j.Lit(",\n");
  Indent(j, depth + 1);
  j.Lit("\"declared_fields\":");
  j.Num(input.layout_field_count);
  j.Lit(",\n");
  Indent(j, depth + 1);
  j.Lit("\"fields\":[");
  for (std::size_t i = 0; i < input.layout_field_count; ++i) {
    const auto &field = input.layout_fields[i];
    j.Lit(i ? ",\n" : "\n");
    Indent(j, depth + 2);
    j.Lit("{\"field_id\":");
    j.Hex(field.id, 4);
    j.Lit(",\"offset\":");
    j.Num(field.offset);
    j.Lit(",\"evidence\":");
    j.Str(field.evidence ? field.evidence : "");
    j.Lit("}");
  }
  j.Lit("],\n");
  Indent(j, depth + 1);
  j.Lit("\"sources\":[");
  for (std::size_t i = 0; i < input.layout_source_count; ++i) {
    const auto &source = input.layout_sources[i];
    j.Lit(i ? ",\n" : "\n");
    Indent(j, depth + 2);
    j.Lit("{\"source\":");
    j.Num(source.source);
    j.Lit(",\"location\":");
    j.Str(source.location ? source.location : "");
    j.Lit(",\"capacity\":");
    j.Num(source.capacity);
    j.Lit(",\"evidence\":");
    j.Str(source.evidence ? source.evidence : "");
    j.Lit("}");
  }
  j.Lit("],\n");
  Indent(j, depth + 1);
  j.Lit("\"classes\":[");
  for (std::size_t i = 0; i < input.layout_class_count; ++i) {
    const auto &declaration = input.layout_classes[i];
    j.Lit(i ? ",\n" : "\n");
    Indent(j, depth + 2);
    j.Lit("{\"class\":");
    j.Str(ClassName(declaration.kind));
    j.Lit(",\"descriptor_rva\":");
    j.Str(declaration.descriptor_rva ? declaration.descriptor_rva : "");
    j.Lit(",\"evidence\":");
    j.Str(declaration.evidence ? declaration.evidence : "");
    j.Lit("}");
  }
  j.Lit("]\n");
  Indent(j, depth);
  j.Lit("},\n");
}

// 顶层字符串数组字段（假设/未知/未确认）。
void WriteStringArray(JsonOut &j, const char *key, const char *const *values,
                      std::size_t count, int depth) noexcept {
  Indent(j, depth);
  j.Key(key);
  j.Lit("[");
  for (std::size_t i = 0; i < count; ++i) {
    if (i)
      j.Lit(",");
    j.Str(values[i] ? values[i] : "");
  }
  j.Lit("],\n");
}

bool WriteManifest(WriterState &state) {
  const auto path = state.directory + L"\\manifest.json";
  if (!state.small.Open(path, kDiagnosticBufferBytes))
    return false;
  const auto &input = state.manifest;
  const auto status = state.storage->Status();
  JsonOut j(state.small);
  j.Lit("{\n");
  Indent(j, 1);
  j.Key("domain");
  j.Str(kDomain);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("version");
  j.Num(kSchemaVersion);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("profile");
  j.Str(kProfile);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("recording_generation");
  j.Num(input.recording_generation);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("session");
  j.Str(state.config.session);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("phase");
  j.Str(input.phase ? input.phase : "POST_STANDARD_UPDATE_PRE_TOP_EXIT");
  j.Lit(",\n");
  Indent(j, 1);
  j.Lit("\"producer\":{\"component\":");
  j.Str("zhlIsaacCS.dll");
  j.Lit(",\"build\":");
  j.Str(input.build ? input.build : "");
  j.Lit(",\"game\":");
  j.Str(input.game_version ? input.game_version : "");
  j.Lit(",\"repentogon\":");
  j.Str(input.repentogon_version ? input.repentogon_version : "");
  j.Lit("},\n");
  Indent(j, 1);
  j.Key("hook_manifest");
  j.Str(input.hook_manifest ? input.hook_manifest : "");
  j.Lit(",\n");
  // 实际启用的观察入口：19 项语义中哪些本次登记为启用。
  Indent(j, 1);
  j.Lit("\"observations\":{\"declared\":");
  j.Num(kObservationCount);
  j.Lit(",\"enabled\":[");
  {
    bool first = true;
    for (std::uint32_t i = 0; i < kObservationCount; ++i) {
      if (!input.observations || !input.observations[i])
        continue;
      if (!first)
        j.Lit(",");
      j.Str(ObservationName(static_cast<Observation>(i)));
      first = false;
    }
  }
  j.Lit("]},\n");
  Indent(j, 1);
  j.Key("classes");
  j.Lit("[");
  for (std::uint32_t i = 0; i < kClassCount; ++i) {
    if (i)
      j.Lit(",");
    j.Str(ClassName(static_cast<Class>(i)));
  }
  j.Lit("],\n");
  Indent(j, 1);
  j.Key("sources");
  j.Lit("[");
  for (std::uint32_t i = 0; i < kSourceCount; ++i) {
    if (i)
      j.Lit(",");
    j.Str(SourceName(i));
  }
  j.Lit("],\n");
  WriteManifestFields(j, 1);
  WriteManifestRequired(j, 1);
  WriteManifestLayout(j, input, 1);
  // 预算：同时登记设计上限与本次实际生效值。
  Indent(j, 1);
  j.Lit("\"budgets\":{");
  j.Lit("\"objects_per_batch\":");
  j.Num(kObjectBudget);
  j.Lit(",\"memberships_per_batch\":");
  j.Num(kMembershipBudget);
  j.Lit(",\"fields_per_object\":");
  j.Num(kFieldBudget);
  j.Lit(",\"player_relation_nodes\":");
  j.Num(kRelationNodes);
  j.Lit(",\"batches\":");
  j.Num(status.limits.slots);
  j.Lit(",\"identities\":");
  j.Num(kIdentityBudget);
  j.Lit(",\"events\":");
  j.Num(status.limits.event_capacity);
  j.Lit(",\"gap_records\":");
  j.Num(status.limits.gap_capacity);
  j.Lit(",\"checkpoints\":");
  j.Num(kCheckpointCapacity);
  j.Lit(",\"close_timeout_ms\":");
  j.Num(state.config.close_timeout_ms);
  j.Lit(",\"queue_bytes\":");
  j.Num(status.limits.queue_bytes);
  j.Lit(",\"queue_items\":");
  j.Num(status.limits.queue_items);
  j.Lit(",\"io_buffer_bytes\":");
  j.Num(kIoBufferBytes);
  j.Lit(",\"world_budget_bytes\":");
  j.Num(kWorldBudgetBytes);
  j.Lit(",\"planned_bytes\":");
  j.Num(PlannedWorldBytes());
  j.Lit(",\"allocated_bytes\":");
  j.Num(status.budget_used);
  j.Lit("},\n");
  // 枚举图例：离线工具据此解释数值列，不依赖隐式约定。
  Indent(j, 1);
  j.Lit("\"event_kinds\":{");
  {
    bool first = true;
    for (std::uint32_t i = 0; i <= static_cast<std::uint32_t>(EventKind::NativeBody); ++i) {
      if (i == 5)
        continue; // 原型保留空号，不登记为有效类别
      if (!first)
        j.Lit(",");
      j.Lit("\"");
      j.Num(i);
      j.Lit("\":");
      j.Str(EventKindName(static_cast<EventKind>(i)));
      first = false;
    }
  }
  j.Lit("},\n");
  Indent(j, 1);
  j.Lit("\"readiness_evidence\":{");
  for (std::uint32_t i = 0; i < 4; ++i) {
    if (i)
      j.Lit(",");
    j.Lit("\"");
    j.Num(i);
    j.Lit("\":");
    j.Str(ReadinessName(static_cast<Readiness>(i)));
  }
  j.Lit("},\n");
  Indent(j, 1);
  j.Lit("\"lifecycle\":{");
  for (std::uint32_t i = 0; i < 7; ++i) {
    if (i)
      j.Lit(",");
    j.Lit("\"");
    j.Num(i);
    j.Lit("\":");
    j.Str(LifecycleName(static_cast<Lifecycle>(i)));
  }
  j.Lit("},\n");
  Indent(j, 1);
  j.Lit("\"origin\":{");
  for (std::uint32_t i = 0; i < 3; ++i) {
    if (i)
      j.Lit(",");
    j.Lit("\"");
    j.Num(i);
    j.Lit("\":");
    j.Str(OriginName(static_cast<Origin>(i)));
  }
  j.Lit("},\n");
  Indent(j, 1);
  j.Lit("\"relation\":[");
  for (std::uint32_t i = 0; i < 5; ++i) {
    if (i)
      j.Lit(",");
    j.Str(RelationName(static_cast<Relation>(i)));
  }
  j.Lit("],\n");
  Indent(j, 1);
  j.Lit("\"field_status\":[");
  for (std::uint32_t i = 0; i < 5; ++i) {
    if (i)
      j.Lit(",");
    j.Str(FieldStatusName(static_cast<FieldStatus>(i)));
  }
  j.Lit("],\n");
  Indent(j, 1);
  j.Lit("\"encoding\":[");
  for (std::uint32_t i = 0; i < 4; ++i) {
    if (i)
      j.Lit(",");
    j.Str(EncodingName(static_cast<Encoding>(i)));
  }
  j.Lit("],\n");
  Indent(j, 1);
  j.Key("float_comparison");
  j.Str(kFloatComparison);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("hp_evidence");
  j.Str(kHpEvidence);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("current_player_read_rule");
  j.Str(kCurrentPlayerRule);
  j.Lit(",\n");
  WriteStringArray(j, "unconfirmed", kUnconfirmedFields,
                   std::size(kUnconfirmedFields), 1);
  WriteStringArray(j, "assumptions", input.assumptions, input.assumption_count,
                   1);
  WriteStringArray(j, "unknowns", input.unknowns, input.unknown_count, 1);
  WriteStringArray(j, "native_diagnostics_excluded", kNativeDiagnosticsExcluded,
                   std::size(kNativeDiagnosticsExcluded), 1);
  Indent(j, 1);
  j.Key("runtime_acceptance");
  j.Str("NOT_RUN");
  j.Lit("\n}\n");
  if (!j.Ok())
    return false;
  // manifest 先于任何 capture 落盘：失败即准备失败，不开始录制。
  return state.small.Close();
}

// ===========================================================================
// captures.jsonl / lifecycle-events.jsonl / gap 记录
// ===========================================================================

// 成员关系按对象分组：计数 → 起始偏移 → 稳定填充，避免 O(对象×成员) 扫描。
bool GroupMemberships(WriterState &state, const OwnedBatch &batch) noexcept {
  const auto objects = batch.object_count;
  const auto memberships = batch.membership_count;
  auto *start = state.membership_start.get();
  auto *cursor = state.membership_cursor.get();
  std::fill_n(start, static_cast<std::size_t>(objects) + 1, 0u);
  for (std::uint32_t i = 0; i < memberships; ++i) {
    const auto object = batch.memberships[i].object;
    if (object >= objects)
      return false;
    ++start[object + 1];
  }
  for (std::uint32_t i = 1; i <= objects; ++i)
    start[i] += start[i - 1];
  std::copy_n(start, objects, cursor);
  for (std::uint32_t i = 0; i < memberships; ++i) {
    const auto &membership = batch.memberships[i];
    state.sorted_memberships[cursor[membership.object]++] = membership;
  }
  return true;
}

bool WriteField(JsonOut &j, const RawField &field) noexcept {
  const auto *declaration = ProfileField(field.id);
  // 与 profile 不符的字段不能写进新 domain：这是采集器错误，不是未知值。
  if (!declaration || declaration->width != field.width ||
      declaration->encoding != field.encoding)
    return false;
  if (field.status != FieldStatus::Value &&
      field.status != FieldStatus::Unconfirmed &&
      field.status != FieldStatus::NotApplicable &&
      field.status != FieldStatus::NotReady &&
      field.status != FieldStatus::ReadFailed)
    return false;
  j.Lit("{\"field_id\":");
  j.Hex(field.id, 4);
  j.Lit(",\"status\":");
  j.Str(FieldStatusName(field.status));
  j.Lit(",\"encoding\":");
  j.Str(EncodingName(field.encoding));
  j.Lit(",\"width\":");
  j.Num(field.width);
  if (field.status == FieldStatus::Value) {
    if (field.encoding == Encoding::Reference) {
      if (static_cast<std::uint32_t>(field.relation) > 4)
        return false;
      j.Lit(",\"reference\":{\"status\":");
      j.Str(RelationName(field.relation));
      if (field.relation == Relation::KnownCurrent) {
        j.Lit(",\"target\":{\"local_id\":");
        j.Dec(field.target_id);
        j.Lit(",\"use_generation\":");
        j.Dec(field.target_use);
        j.Lit("}");
      }
      j.Lit("}");
      // 引用字段的原始地址只作诊断，不能当跨进程身份。
      std::uint32_t address = 0;
      std::memcpy(&address, field.raw.data(), sizeof(address));
      j.Lit(",\"diagnostics\":{\"native_address\":");
      j.Hex(address, 8);
      j.Lit("}");
    } else if (field.width == 4) {
      std::uint32_t value = 0;
      std::memcpy(&value, field.raw.data(), sizeof(value));
      j.Lit(",\"raw32\":");
      j.Hex(value, 8);
    } else if (field.width == 8) {
      std::uint64_t value = 0;
      std::memcpy(&value, field.raw.data(), sizeof(value));
      j.Lit(",\"raw64\":");
      j.Hex(value, 16);
    } else if (field.width > 0 && field.width <= 16) {
      j.Lit(",\"bytes_le\":");
      j.HexBytes(field.raw.data(), field.width);
    } else {
      return false;
    }
  }
  j.Lit("}");
  return true;
}

bool WriteCapture(WriterState &state, const OwnedBatch &batch) noexcept {
  const auto &header = batch.header;
  if (batch.object_count > kObjectBudget ||
      batch.membership_count > kMembershipBudget) {
    state.Fail(Reason::InvalidRecord, 0, "capture_budget");
    return false;
  }
  if (static_cast<std::uint32_t>(header.reason) >= kReasonCount) {
    state.Fail(Reason::InvalidRecord, 0, "capture_reason");
    return false;
  }
  if (!GroupMemberships(state, batch)) {
    state.Fail(Reason::InvalidRecord, 0, "membership_index");
    return false;
  }
  JsonOut j(state.captures);
  const auto phase =
      state.manifest.phase ? state.manifest.phase : "POST_STANDARD_UPDATE_PRE_TOP_EXIT";
  j.Lit("{\"record\":\"capture\",\"domain\":");
  j.Str(kDomain);
  j.Lit(",\"version\":");
  j.Num(kSchemaVersion);
  j.Lit(",\"profile\":");
  j.Str(kProfile);
  j.Lit(",\"session\":");
  j.Str(state.config.session);
  j.Lit(",\"recording_generation\":");
  j.Num(header.recording_generation);
  j.Lit(",\"run_epoch\":");
  j.Dec(header.run);
  j.Lit(",\"room_epoch\":");
  j.Dec(header.room);
  j.Lit(",\"mutation_epoch\":");
  j.Dec(header.mutation);
  j.Lit(",\"attempt_seq\":");
  j.Dec(header.attempt);
  j.Lit(",\"capture_seq\":");
  if (header.capture)
    j.Dec(header.capture);
  else
    j.Null();
  j.Lit(",\"outer_token\":");
  j.Dec(header.outer);
  j.Lit(",\"phase\":");
  j.Str(phase);
  j.Lit(",\"authority_tick\":");
  if (header.authority)
    j.Dec(header.tick);
  else
    j.Null();
  j.Lit(",\"authority_kind\":");
  j.Num(header.commit_kind);
  j.Lit(",\"scope_token\":");
  j.Dec(header.scope);
  j.Lit(",\"event_watermark\":");
  j.Dec(header.event_watermark);
  j.Lit(",\"complete\":");
  j.Bool(header.complete != 0);
  j.Lit(",\"reason\":");
  j.Str(ReasonName(static_cast<Reason>(header.reason)));
  // 来源：15 项固定顺序，NOT_APPLICABLE 与读取失败分开。
  j.Lit(",\"sources\":[");
  for (std::uint32_t i = 0; i < kSourceCount; ++i) {
    const auto &source = header.sources[i];
    if (i)
      j.Lit(",");
    j.Lit("{\"source\":");
    j.Num(i);
    j.Lit(",\"status\":");
    if (source.status == kSourceNotApplicable) {
      j.Str("NOT_APPLICABLE");
    } else if (source.status == 0) {
      j.Str("NONE");
    } else {
      j.Str(ReasonName(static_cast<Reason>(source.status)));
    }
    j.Lit(",\"count\":");
    j.Num(source.count);
    j.Lit(",\"diagnostics\":{\"descriptor\":[");
    for (std::uint32_t d = 0; d < 4; ++d) {
      if (d)
        j.Lit(",");
      j.Num(source.descriptor[d]);
    }
    j.Lit("]}}");
  }
  j.Lit("],\"objects\":[");
  for (std::uint32_t i = 0; i < batch.object_count; ++i) {
    const auto &object = batch.objects[i];
    if (i)
      j.Lit(",");
    if (object.kind >= kClassCount ||
        object.origin >= static_cast<std::uint32_t>(Origin::ObservedCreation) + 1 ||
        object.lifecycle > static_cast<std::uint32_t>(Lifecycle::Destroyed) ||
        object.readiness >
            static_cast<std::uint32_t>(Readiness::CurrentPlayerUpdateWitness) ||
        object.field_count > kFieldBudget) {
      state.Fail(Reason::InvalidRecord, 0, "object_schema");
      return false;
    }
    j.Lit("{\"local_id\":");
    j.Dec(object.local_id);
    j.Lit(",\"use_generation\":");
    j.Dec(object.use);
    j.Lit(",\"revision\":");
    j.Dec(object.revision);
    j.Lit(",\"object_domain\":");
    j.Num(object.domain);
    j.Lit(",\"class\":");
    j.Str(ClassName(static_cast<Class>(object.kind)));
    j.Lit(",\"origin\":");
    j.Str(OriginName(static_cast<Origin>(object.origin)));
    j.Lit(",\"lifecycle\":");
    j.Num(object.lifecycle);
    j.Lit(",\"init_evidence\":");
    j.Num(object.readiness);
    j.Lit(",\"birth_event\":");
    if (object.birth_known)
      j.Dec(object.birth);
    else
      j.Null();
    j.Lit(",\"birth\":{\"kind\":");
    j.Num(object.birth_kind);
    j.Lit(",\"entity_type\":");
    j.Num(object.birth_type);
    j.Lit(",\"variant\":");
    j.Num(object.birth_variant);
    j.Lit(",\"subtype\":");
    j.Num(object.birth_subtype);
    j.Lit(",\"seed\":");
    j.Num(object.birth_seed);
    j.Lit("},\"ordinal\":");
    if (object.ordinal <= 1)
      j.Num(object.ordinal);
    else
      j.Null();
    j.Lit(",\"grid_index\":");
    if (object.grid_index != kNoGridIndex)
      j.Num(object.grid_index);
    else
      j.Null();
    j.Lit(",\"fields\":[");
    for (std::uint32_t f = 0; f < object.field_count; ++f) {
      if (f)
        j.Lit(",");
      if (!WriteField(j, object.fields[f])) {
        state.Fail(Reason::InvalidRecord, 0, "field_schema");
        return false;
      }
    }
    j.Lit("],\"memberships\":[");
    const auto begin = state.membership_start[i];
    const auto end = state.membership_start[i + 1];
    for (std::uint32_t m = begin; m < end; ++m) {
      if (m != begin)
        j.Lit(",");
      const auto &membership = state.sorted_memberships[m];
      j.Lit("{\"source\":");
      j.Num(membership.source);
      j.Lit(",\"index\":");
      j.Num(membership.index);
      j.Lit("}");
    }
    j.Lit("],\"diagnostics\":{\"address\":");
    j.Hex(object.address, 8);
    j.Lit(",\"vptr\":");
    j.Hex(object.vptr, 8);
    j.Lit("}}");
  }
  j.Lit("],\"required_objects\":");
  j.Num(header.required_objects);
  j.Lit(",\"required_memberships\":");
  j.Num(header.required_memberships);
  j.Lit(",\"diagnostics\":{\"thread\":");
  j.Num(header.thread);
  j.Lit(",\"loop\":"); j.Dec(header.loop);
  j.Lit(",\"phase_witness\":[");
  for (std::size_t i = 0; i < header.phase_scopes.size(); ++i) {
    if (i) j.Lit(",");
    j.Lit("{\"stage\":"); j.Num(static_cast<std::uint32_t>(i + 1));
    j.Lit(",\"scope\":"); j.Dec(header.phase_scopes[i]);
    j.Lit(",\"entered\":"); j.Num(header.phase_entered[i]);
    j.Lit(",\"returned\":"); j.Num(header.phase_returned[i]);
    j.Lit("}");
  }
  j.Lit("]");
  j.Lit("}}\n");
  if (!j.Ok()) {
    state.Fail(Reason::FileError, state.captures.LastError(), "captures_write");
    return false;
  }
  state.records_written.fetch_add(1, std::memory_order_relaxed);
  ++state.reason_counts[static_cast<std::uint32_t>(header.reason)];
  return true;
}

bool WriteGap(WriterState &state, const GapRecord &gap) noexcept {
  if (static_cast<std::uint32_t>(gap.reason) >= kReasonCount) {
    state.Fail(Reason::InvalidRecord, 0, "gap_reason");
    return false;
  }
  JsonOut j(state.captures);
  j.Lit("{\"record\":\"gap\",\"domain\":");
  j.Str(kDomain);
  j.Lit(",\"version\":");
  j.Num(kSchemaVersion);
  j.Lit(",\"profile\":");
  j.Str(kProfile);
  j.Lit(",\"session\":");
  j.Str(state.config.session);
  j.Lit(",\"recording_generation\":");
  j.Num(gap.recording_generation);
  j.Lit(",\"run_epoch\":");
  j.Dec(gap.run);
  j.Lit(",\"room_epoch\":");
  j.Dec(gap.room);
  j.Lit(",\"mutation_epoch\":");
  j.Dec(gap.mutation);
  j.Lit(",\"attempt_seq\":");
  j.Dec(gap.attempt);
  j.Lit(",\"capture_seq\":null,\"outer_token\":");
  j.Dec(gap.outer);
  j.Lit(",\"phase\":");
  j.Str(state.manifest.phase ? state.manifest.phase
                             : "POST_STANDARD_UPDATE_PRE_TOP_EXIT");
  j.Lit(",\"authority_tick\":null,\"authority_kind\":");
  j.Num(gap.authority_kind);
  j.Lit(",\"scope_token\":");
  j.Dec(gap.scope);
  j.Lit(",\"event_watermark\":");
  j.Dec(gap.event_watermark);
  j.Lit(",\"complete\":false,\"reason\":");
  j.Str(ReasonName(gap.reason));
  j.Lit(",\"sources\":[],\"objects\":[],\"required_objects\":0,"
        "\"required_memberships\":0,\"diagnostics\":{\"candidate_tick\":");
  j.Dec(gap.tick);
  j.Lit(",\"thread\":");
  j.Num(gap.thread);
  j.Lit("}}\n");
  if (!j.Ok()) {
    state.Fail(Reason::FileError, state.captures.LastError(), "gap_write");
    return false;
  }
  state.gaps_written.fetch_add(1, std::memory_order_relaxed);
  ++state.reason_counts[static_cast<std::uint32_t>(gap.reason)];
  return true;
}

bool WriteEvent(WriterState &state, const LifecycleEvent &event) noexcept {
  if (event.observation != kNoObservation &&
      event.observation >= kObservationCount) {
    state.Fail(Reason::InvalidRecord, 0, "event_observation");
    return false;
  }
  if (static_cast<std::uint32_t>(event.reason) >= kReasonCount) {
    state.Fail(Reason::InvalidRecord, 0, "event_reason");
    return false;
  }
  const auto kind = static_cast<std::uint32_t>(event.kind);
  if (kind > static_cast<std::uint32_t>(EventKind::NativeBody)) {
    state.Fail(Reason::InvalidRecord, 0, "event_kind");
    return false;
  }
  JsonOut j(state.events);
  j.Lit("{\"record\":\"lifecycle_event\",\"sequence\":");
  j.Dec(event.sequence);
  j.Lit(",\"run_epoch\":");
  j.Dec(event.run);
  j.Lit(",\"room_epoch\":");
  j.Dec(event.room);
  j.Lit(",\"outer_token\":");
  j.Dec(event.outer);
  j.Lit(",\"local_id\":");
  if (event.local_id)
    j.Dec(event.local_id);
  else
    j.Null();
  j.Lit(",\"use_generation\":");
  if (event.local_id)
    j.Dec(event.use);
  else
    j.Null();
  j.Lit(",\"birth_event\":");
  if (event.birth)
    j.Dec(event.birth);
  else
    j.Null();
  j.Lit(",\"revision\":");
  if (event.local_id)
    j.Dec(event.revision);
  else
    j.Null();
  j.Lit(",\"call\":");
  if (event.call != kNoCall)
    j.Dec(event.call);
  else
    j.Null();
  j.Lit(",\"parent\":");
  if (event.parent != kNoCall)
    j.Dec(event.parent);
  else
    j.Null();
  j.Lit(",\"observation\":");
  if (event.observation == kNoObservation)
    j.Null();
  else
    j.Str(ObservationName(static_cast<Observation>(event.observation)));
  j.Lit(",\"entering\":");
  j.Bool(event.entering != 0);
  j.Lit(",\"exceptional\":");
  j.Bool(event.exceptional != 0);
  j.Lit(",\"event_kind\":");
  j.Num(kind);
  j.Lit(",\"reason\":");
  j.Str(ReasonName(event.reason));
  j.Lit(",\"memberships\":");
  j.Num(event.memberships);
  j.Lit(",\"diagnostics\":{\"address\":");
  j.Hex(event.address, 8);
  j.Lit(",\"result\":");
  j.Hex(event.result, 8);
  j.Lit(",\"mutation\":"); j.Dec(event.mutation);
  j.Lit(",\"candidate_tick\":"); j.Dec(event.tick);
  j.Lit(",\"thread\":"); j.Num(event.thread);
  j.Lit(",\"domain\":"); j.Num(event.domain);
  j.Lit(",\"lifecycle\":"); j.Num(event.lifecycle);
  j.Lit(",\"readiness\":"); j.Num(event.readiness);
  j.Lit(",\"ordinal\":"); j.Num(event.ordinal);
  j.Lit(",\"session\":"); j.Dec(event.session);
  j.Lit(",\"participant_generation\":"); j.Dec(event.participant_generation);
  j.Lit(",\"assignment_key\":");
  if (event.assignment_key < 0) j.Lit("-1"); else j.Num(static_cast<std::uint32_t>(event.assignment_key));
  j.Lit(",\"device\":"); j.Hex(event.device, 8);
  j.Lit(",\"slot_before\":"); j.Hex(event.slot_before, 8);
  j.Lit(",\"slot_after\":"); j.Hex(event.slot_after, 8);
  j.Lit(",\"args\":[");
  for (std::uint32_t i = 0; i < event.arg_count && i < event.args.size(); ++i) {
    if (i) j.Lit(","); j.Hex(event.args[i], 8);
  }
  j.Lit("]");
  j.Lit("}}\n");
  if (!j.Ok()) {
    state.Fail(Reason::FileError, state.events.LastError(), "events_write");
    return false;
  }
  state.events_written.fetch_add(1, std::memory_order_relaxed);
  return true;
}

// 诊断行：与常规 trace 分离，只描述 world 自有状态。
template <class Body>
bool WriteDiagnostic(WriterState &state, const char *kind, Body &&body) noexcept {
  JsonOut j(state.diagnostics);
  j.Lit("{\"record\":\"diagnostic\",\"kind\":");
  j.Str(kind);
  j.Lit(",\"at_ms\":");
  j.Num(NowMilliseconds());
  j.Lit(",\"recording_generation\":");
  j.Num(state.manifest.recording_generation);
  body(j);
  j.Lit("}\n");
  if (!j.Ok())
    return false;
  state.diagnostics_written.fetch_add(1, std::memory_order_relaxed);
  return true;
}

// ===========================================================================
// result.json
// ===========================================================================
void WriteReadiness(JsonOut &j, const WriterState &state,
                    WriterOutcome outcome) noexcept {
  const auto status = state.storage->Status();
  j.Lit("{\"producer\":");
  j.Str(status.fault.reason == Reason::None ? "READY" : "FAULTED");
  j.Lit(",\"reader\":");
  j.Str(ThreadStateName(status.reader));
  j.Lit(",\"writer\":");
  j.Str(ThreadStateName(
      static_cast<ThreadState>(state.thread_state.load(std::memory_order_relaxed))));
  j.Lit(",\"drain\":");
  j.Str(state.drain_complete.load(std::memory_order_relaxed) ? "COMPLETE"
                                                             : "INCOMPLETE");
  j.Lit(",\"flush\":");
  j.Str(state.flushed.load(std::memory_order_relaxed) ? "FLUSHED"
                                                      : "NOT_FLUSHED");
  j.Lit(",\"outcome\":");
  j.Str(WriterOutcomeName(outcome));
  j.Lit("}");
}

bool WriteResult(WriterState &state, WriterOutcome outcome) {
  const auto path = state.directory + L"\\result.json";
  if (!state.small.Open(path, kDiagnosticBufferBytes))
    return false;
  const auto status = state.storage->Status();
  const auto queue = state.queue->Read();
  const auto fault = state.diag->Fault();
  JsonOut j(state.small);
  j.Lit("{\"status\":");
  switch (outcome) {
  case WriterOutcome::Closed:
    j.Str("CAPTURE_CLOSED");
    break;
  case WriterOutcome::Error:
    j.Str("CAPTURE_ERROR");
    break;
  case WriterOutcome::Unconfirmed:
    j.Str("CAPTURE_UNCONFIRMED");
    break;
  }
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("domain");
  j.Str(kDomain);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("version");
  j.Num(kSchemaVersion);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("profile");
  j.Str(kProfile);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("session");
  j.Str(state.config.session);
  j.Lit(",\n");
  Indent(j, 1);
  j.Key("recording_generation");
  j.Num(state.manifest.recording_generation);
  j.Lit(",\n");
  Indent(j, 1);
  j.Lit("\"readiness\":");
  WriteReadiness(j, state, outcome);
  j.Lit(",\n");
  // 计数：生产/复制/写入分离，避免把“已生产”当作“已落盘”。
  Indent(j, 1);
  j.Lit("\"counts\":{");
  j.Lit("\"attempts\":");
  j.Dec(status.attempts);
  j.Lit(",\"captures\":");
  j.Dec(status.captures);
  j.Lit(",\"complete_captures\":");
  j.Dec(status.complete_captures);
  j.Lit(",\"gaps\":");
  j.Dec(status.gaps);
  j.Lit(",\"batches_copied\":");
  j.Dec(status.batches_copied);
  j.Lit(",\"batches_written\":");
  j.Dec(state.records_written.load(std::memory_order_relaxed));
  j.Lit(",\"events_produced\":");
  j.Dec(status.events_produced);
  j.Lit(",\"events_copied\":");
  j.Dec(status.events_consumed);
  j.Lit(",\"events_written\":");
  j.Dec(state.events_written.load(std::memory_order_relaxed));
  j.Lit(",\"gaps_produced\":");
  j.Dec(status.gaps_produced);
  j.Lit(",\"gaps_copied\":");
  j.Dec(status.gaps_consumed);
  j.Lit(",\"gaps_written\":");
  j.Dec(state.gaps_written.load(std::memory_order_relaxed));
  j.Lit(",\"backpressure\":");
  j.Dec(status.backpressure);
  j.Lit(",\"budget_waits\":");
  j.Dec(status.budget_waits);
  j.Lit(",\"callbacks\":");
  j.Dec(status.callbacks);
  j.Lit(",\"checkpoints_produced\":");
  j.Dec(state.storage->Diag().checkpoints_produced.load());
  j.Lit(",\"checkpoints_written\":");
  j.Dec(state.storage->Diag().checkpoints_written.load());
  j.Lit(",\"callback_faults\":");
  j.Dec(status.callback_faults);
  j.Lit(",\"discarded_items\":");
  j.Dec(state.discarded.load(std::memory_order_relaxed));
  j.Lit("},\n");
  // 逐原因统计：抓取与 gap 共用同一原因枚举。
  Indent(j, 1);
  j.Lit("\"reasons\":{");
  {
    bool first = true;
    for (std::uint32_t i = 0; i < kReasonCount; ++i) {
      if (!state.reason_counts[i])
        continue;
      if (!first)
        j.Lit(",");
      j.Str(ReasonName(static_cast<Reason>(i)));
      j.Lit(":");
      j.Dec(state.reason_counts[i]);
      first = false;
    }
  }
  j.Lit("},\n");
  // 未交付范围：把“生产了但没写下去”的部分显式列出。
  Indent(j, 1);
  j.Lit("\"undelivered\":{");
  j.Lit("\"last_attempt\":");
  j.Dec(status.last_attempt);
  j.Lit(",\"last_capture\":");
  j.Dec(status.last_capture);
  j.Lit(",\"last_event\":");
  j.Dec(status.last_event);
  j.Lit(",\"events_not_written\":");
  j.Dec(status.events_produced >=
                state.events_written.load(std::memory_order_relaxed)
            ? status.events_produced -
                  state.events_written.load(std::memory_order_relaxed)
            : 0);
  j.Lit(",\"gaps_not_written\":");
  j.Dec(status.gaps_produced >=
                state.gaps_written.load(std::memory_order_relaxed)
            ? status.gaps_produced -
                  state.gaps_written.load(std::memory_order_relaxed)
            : 0);
  j.Lit(",\"batches_not_written\":");
  j.Dec(status.batches_copied >= state.records_written.load(
                                     std::memory_order_relaxed)
            ? status.batches_copied -
                  state.records_written.load(std::memory_order_relaxed)
            : 0);
  j.Lit(",\"checkpoints_not_written\":");
  j.Dec(state.storage->Diag().checkpoints_produced.load() -
        state.storage->Diag().checkpoints_written.load());
  j.Lit(",\"queue_items\":");
  j.Dec(queue.held_items);
  j.Lit(",\"queue_bytes\":");
  j.Dec(queue.held_bytes);
  j.Lit("},\n");
  // 故障：首因 + writer 自己的文件错误分开记录。
  Indent(j, 1);
  j.Lit("\"failure\":");
  if (fault.reason == Reason::None && !state.write_failed.load()) {
    j.Null();
  } else {
    const bool file_failure = fault.reason == Reason::None;
    j.Lit("{\"reason\":");
    j.Str(file_failure ? "FILE_ERROR" : ReasonName(fault.reason));
    j.Lit(",\"source\":");
    j.Str(file_failure ? "WRITER" : FailSourceName(fault.source));
    j.Lit(",\"code\":");
    j.Num(fault.code);
    j.Lit(",\"stage\":");
    j.Str(fault.stage ? fault.stage : "");
    j.Lit(",\"at_ms\":");
    j.Dec(fault.at_ms);
    j.Lit(",\"write_error\":");
    j.Num(state.LastError());
    j.Lit("}");
  }
  j.Lit(",\n");
  Indent(j, 1);
  j.Lit("\"diagnostics\":{");
  j.Lit("\"budget_used\":");
  j.Dec(status.budget_used);
  j.Lit(",\"budget_peak\":");
  j.Dec(status.budget_peak);
  j.Lit(",\"budget_limit\":");
  j.Dec(status.budget_limit);
  j.Lit(",\"queue_peak_items\":");
  j.Dec(queue.peak_items);
  j.Lit(",\"queue_peak_bytes\":");
  j.Dec(queue.peak_bytes);
  j.Lit(",\"queue_budget_waits\":");
  j.Dec(queue.budget_waits);
  j.Lit(",\"copy_max_ns\":");
  j.Dec(status.copy_max_ns);
  j.Lit(",\"write_calls\":");
  j.Dec(state.captures.WriteCalls());
  j.Lit(",\"bytes_written\":");
  j.Dec(state.captures.BytesWritten());
  j.Lit(",\"diagnostics_written\":");
  j.Dec(state.diagnostics_written.load(std::memory_order_relaxed));
  j.Lit(",\"files_flushed\":");
  j.Bool(state.flushed.load(std::memory_order_relaxed));
  j.Lit("},\n");
  Indent(j, 1);
  j.Key("runtime_acceptance");
  j.Str("NOT_RUN");
  j.Lit("\n}\n");
  if (!j.Ok())
    return false;
  return state.small.Close();
}

// ===========================================================================
// writer 线程
// ===========================================================================

void FlushDiagnosticsAt(WriterState &state, const char *kind) noexcept {
  WriteDiagnostic(state, kind, [&](JsonOut &j) {
    const auto status = state.storage->Status();
    const auto queue = state.queue->Read();
    j.Lit(",\"reader\":");
    j.Str(ThreadStateName(status.reader));
    j.Lit(",\"state\":");
    j.Str(StorageStateName(status.state));
    j.Lit(",\"attempts\":");
    j.Dec(status.attempts);
    j.Lit(",\"captures\":");
    j.Dec(status.captures);
    j.Lit(",\"gaps\":");
    j.Dec(status.gaps);
    j.Lit(",\"backpressure\":");
    j.Dec(status.backpressure);
    j.Lit(",\"queue_items\":");
    j.Dec(queue.held_items);
    j.Lit(",\"queue_bytes\":");
    j.Dec(queue.held_bytes);
    j.Lit(",\"producer_done\":");
    j.Bool(queue.producer_done);
  });
}

// 背压范围与未交付量：与常规 trace 分离，单独写进 diagnostics.jsonl。
void FlushBackpressureAt(WriterState &state) noexcept {
  const auto status = state.storage->Status();
  const auto queue = state.queue->Read();
  if (!status.backpressure && !queue.budget_waits)
    return;
  WriteDiagnostic(state, "backpressure", [&](JsonOut &j) {
    j.Lit(",\"attempts\":");
    j.Dec(status.attempts);
    j.Lit(",\"slots_exhausted\":");
    j.Dec(status.backpressure);
    j.Lit(",\"budget_waits\":");
    j.Dec(status.budget_waits);
    j.Lit(",\"queue_peak_items\":");
    j.Dec(queue.peak_items);
    j.Lit(",\"queue_peak_bytes\":");
    j.Dec(queue.peak_bytes);
    j.Lit(",\"queue_limited_items\":");
    j.Num(state.queue->ItemLimit());
    j.Lit(",\"queue_limited_bytes\":");
    j.Dec(state.queue->ByteLimit());
    j.Lit(",\"last_attempt\":");
    j.Dec(status.last_attempt);
    j.Lit(",\"captures_not_written\":");
    j.Dec(status.batches_copied >=
                  state.records_written.load(std::memory_order_relaxed)
              ? status.batches_copied -
                    state.records_written.load(std::memory_order_relaxed)
              : 0);
    j.Lit(",\"events_not_written\":");
    j.Dec(status.events_produced >=
                  state.events_written.load(std::memory_order_relaxed)
              ? status.events_produced -
                    state.events_written.load(std::memory_order_relaxed)
              : 0);
    j.Lit(",\"gaps_not_written\":");
    j.Dec(status.gaps_produced >=
                  state.gaps_written.load(std::memory_order_relaxed)
              ? status.gaps_produced -
                    state.gaps_written.load(std::memory_order_relaxed)
              : 0);
  });
}

// 自有故障：首因写进 diagnostics.jsonl，供离线归因，不冒充游戏故障。
void FlushFaultAt(WriterState &state) noexcept {
  const auto fault = state.diag->Fault();
  if (fault.reason == Reason::None && !state.write_failed.load())
    return;
  WriteDiagnostic(state, "fault", [&](JsonOut &j) {
    j.Lit(",\"reason\":");
    j.Str(ReasonName(fault.reason));
    j.Lit(",\"source\":");
    j.Str(FailSourceName(fault.source));
    j.Lit(",\"code\":");
    j.Num(fault.code);
    j.Lit(",\"stage\":");
    j.Str(fault.stage ? fault.stage : "");
    // 外层 diagnostic 已有数值 at_ms；首因时间另列，避免重复 JSON 键丢事实。
    j.Lit(",\"fault_at_ms\":");
    j.Dec(fault.at_ms);
    j.Lit(",\"write_error\":");
    j.Num(state.LastError());
  });
}

bool WriteItem(WriterState &state, const QueueItem &item) noexcept {
  switch (item.kind) {
  case ItemKind::Batch:
    return WriteCapture(state, item.batch);
  case ItemKind::Events:
    for (const auto &event : item.events)
      if (!WriteEvent(state, event))
        return false;
    return true;
  case ItemKind::Gaps:
    for (const auto &gap : item.gaps)
      if (!WriteGap(state, gap))
        return false;
    return true;
  case ItemKind::Checkpoints:
    for (const auto &marker : item.checkpoints) {
      if (!WriteDiagnostic(state, "checkpoint", [&](JsonOut &j) {
        j.Lit(",\"label\":"); j.Str(marker.label);
        j.Lit(",\"request\":"); j.Dec(marker.request);
        j.Lit(",\"sequence\":"); j.Dec(marker.sequence);
        j.Lit(",\"run\":"); j.Dec(marker.run);
        j.Lit(",\"room\":"); j.Dec(marker.room);
        j.Lit(",\"outer\":"); j.Dec(marker.outer);
        j.Lit(",\"mutation\":"); j.Dec(marker.mutation);
        j.Lit(",\"attempt\":"); j.Dec(marker.attempt);
        j.Lit(",\"capture\":"); j.Dec(marker.capture);
        j.Lit(",\"event_watermark\":"); j.Dec(marker.event_watermark);
      })) {
        state.Fail(Reason::FileError, state.diagnostics.LastError(), "checkpoint_write");
        return false;
      }
      state.storage->Diag().checkpoints_written.fetch_add(1);
    }
    return true;
  }
  return false;
}

bool CloseArtifacts(WriterState &state) noexcept {
  // 显式 flush + FlushFileBuffers：成功后才能报告 files_flushed。
  bool all_ok = state.captures.Sync();
  if (!all_ok)
    state.Fail(Reason::FileError, state.captures.LastError(), "captures_sync");
  if (!state.events.Sync()) {
    state.Fail(Reason::FileError, state.events.LastError(), "events_sync");
    all_ok = false;
  }
  if (!state.diagnostics.Sync()) {
    state.Fail(Reason::FileError, state.diagnostics.LastError(),
               "diagnostics_sync");
    all_ok = false;
  }
  state.flushed.store(all_ok, std::memory_order_release);
  state.captures.Close();
  state.events.Close();
  state.diagnostics.Close();
  return all_ok;
}

WriterOutcome TerminalOutcome(const WriterState &state) noexcept {
  const auto fault = state.diag->Fault();
  const bool write_failed = state.write_failed.load(std::memory_order_relaxed);
  if (fault.reason != Reason::None || write_failed)
    return WriterOutcome::Error;
  if (!state.drain_complete.load(std::memory_order_relaxed) ||
      state.storage->DrainIncomplete() ||
      !state.flushed.load(std::memory_order_relaxed))
    return WriterOutcome::Unconfirmed;
  return WriterOutcome::Closed;
}

} // namespace

// 线程只持有自己的状态强引用；WorldWriter 句柄关闭后线程仍能安全收尾。
struct WriterRuntime final {
  WriterState state;
  std::atomic<WriterOutcome> outcome{WriterOutcome::Unconfirmed};
  HANDLE thread = nullptr;

  ~WriterRuntime() {
    // 打开失败/未关闭时归还 writer 自己的预算占用；线程已结束时才可能执行。
    if (state.ledger && state.charged) {
      state.ledger->Release(state.charged);
      state.charged = 0;
    }
  }
};

void WriterLoop(WriterRuntime &impl) {
  WriterState &state = impl.state;
  state.thread_state.store(static_cast<std::uint32_t>(ThreadState::Running),
                           std::memory_order_release);
  state.diag->writer_state.store(static_cast<std::uint32_t>(ThreadState::Running),
                                 std::memory_order_release);
  std::uint64_t last_flush = NowMilliseconds();
  bool discarding = false;
  for (;;) {
    // E2E 文件故障：关闭自有句柄，下一次真实写/刷以 ERROR_INVALID_HANDLE 失败。
    if (auto *faults = state.storage->Faults()) {
      if (faults->drop_capture_file.exchange(false)) {
        state.captures.DropHandleForTest();
        faults->applied.store(1);
        WriteDiagnostic(state, "fault", [](JsonOut &j) {
          j.Lit(",\"injected\":\"TEST_INJECTED\"");
          j.Lit(",\"target\":\"captures.jsonl\"");
          j.Lit(",\"effect\":\"handle_closed\"");
        });
        // 不在注入点伪造失败：让下一次真实 Write/Flush 以 ERROR_INVALID_HANDLE
        // 失败，并沿同一条错误路径记录 FileError。
      }
    }
    QueueItem item;
    const bool got = state.queue->Pop(item, 100);
    if (got) {
      const auto bytes = item.bytes;
      if (!discarding) {
        const bool written = WriteItem(state, item);
        if (!written) {
          // 具体原因已由编码函数记录；这里只兜底，避免未记录就静默丢弃。
          if (!state.write_failed.load(std::memory_order_relaxed))
            state.Fail(Reason::InvalidRecord, 0, "encode");
          discarding = true; // 已写前缀保留，其余只排空并计入丢弃
        }
      }
      if (discarding)
        state.discarded.fetch_add(1, std::memory_order_relaxed);
      // 先释放 owned 内存再归还预算：预算代表真实在途字节。
      item = QueueItem{};
      state.queue->Release(bytes);
      continue;
    }
    if (state.queue->ProducerDone() && state.queue->Empty()) {
      state.drain_complete.store(true, std::memory_order_release);
      break;
    }
    if (state.stop.load(std::memory_order_acquire) && state.queue->Empty()) {
      // 停止后队列已空但生产端尚未结束：不能声称完整排空。
      state.drain_complete.store(false, std::memory_order_release);
      break;
    }
    if (NowMilliseconds() - last_flush >= kFlushIntervalMs) {
      state.captures.Flush();
      state.events.Flush();
      state.diagnostics.Flush();
      last_flush = NowMilliseconds();
      if (!state.captures.Ok()) {
        state.Fail(Reason::FileError, state.captures.LastError(),
                   "periodic_flush");
        discarding = true;
      }
    }
  }
  state.diag->writer_state.store(static_cast<std::uint32_t>(ThreadState::Draining),
                                 std::memory_order_release);
  // 收尾顺序：先写背压/故障/关闭诊断，再 flush/sync 数据文件，最后 result。
  FlushBackpressureAt(state);
  FlushFaultAt(state);
  if (const auto *f = state.storage->Faults(); f && f->mode.load()) {
    WriteDiagnostic(state, "test_injection", [&](JsonOut &j) {
      j.Lit(",\"injected\":\"TEST_INJECTED\",\"mode\":"); j.Num(f->mode.load());
      j.Lit(",\"session\":"); j.Str(state.config.session);
      j.Lit(",\"value\":"); j.Num(f->value.load());
      j.Lit(",\"applied\":"); j.Num(f->applied.load());
      j.Lit(",\"rejection\":"); j.Str(ReasonName(static_cast<Reason>(f->rejection.load())));
      j.Lit(",\"before\":"); j.Num(f->before.load());
      j.Lit(",\"after\":"); j.Num(f->after.load());
      j.Lit(",\"attempt\":"); j.Dec(f->attempt.load());
      j.Lit(",\"event_watermark\":"); j.Dec(f->watermark.load());
    });
  }
  FlushDiagnosticsAt(state, "close");
  CloseArtifacts(state);
  auto outcome = TerminalOutcome(state);
  if (WriteResult(state, outcome)) state.result_written.store(true);
  else {
    state.Fail(Reason::FileError, state.small.LastError(), "result_write");
    outcome = WriterOutcome::Error;
  }
  impl.outcome.store(outcome, std::memory_order_release);
  state.thread_state.store(
      static_cast<std::uint32_t>(outcome == WriterOutcome::Error
                                     ? ThreadState::Failed
                                     : ThreadState::Done),
      std::memory_order_release);
  state.diag->writer_state.store(state.thread_state.load(),
                                 std::memory_order_release);
  state.done.store(true, std::memory_order_release);
}

namespace {
struct WriterStart final {
  std::shared_ptr<WriterRuntime> impl;
};
DWORD WINAPI WriterEntry(LPVOID parameter) noexcept {
  std::unique_ptr<WriterStart> start(static_cast<WriterStart *>(parameter));
  auto impl = std::move(start->impl);
  try {
    WriterLoop(*impl);
  } catch (...) {
    impl->state.Fail(Reason::ThreadFailed, 0, "writer_exception");
    impl->outcome.store(WriterOutcome::Error, std::memory_order_release);
    impl->state.thread_state.store(
        static_cast<std::uint32_t>(ThreadState::Failed),
        std::memory_order_release);
    impl->state.done.store(true, std::memory_order_release);
  }
  return 0;
}
} // namespace

// ===========================================================================
// WorldWriter
// ===========================================================================
const char *WriterOutcomeName(WriterOutcome value) noexcept {
  switch (value) {
  case WriterOutcome::Closed:
    return "CAPTURE_CLOSED";
  case WriterOutcome::Error:
    return "CAPTURE_ERROR";
  case WriterOutcome::Unconfirmed:
    return "CAPTURE_UNCONFIRMED";
  }
  return "UNKNOWN";
}

WorldWriter::WorldWriter() noexcept = default;
WorldWriter::~WorldWriter() {
  if (!impl_)
    return;
  impl_->state.stop.store(true, std::memory_order_release);
  if (impl_->thread) {
    // 不等待、不释放线程仍在使用的状态：线程持有自己的 shared_ptr。
    CloseHandle(impl_->thread);
    impl_->thread = nullptr;
  }
}

bool WorldWriter::Open(const WriterConfig &config, const ManifestInput &manifest,
                       WorldStorage &storage) noexcept try {
  if (impl_)
    return false;
  std::shared_ptr<WriterRuntime> impl;
  try {
    impl = std::make_shared<WriterRuntime>();
  } catch (...) {
    return false;
  }
  impl_ = impl;
  auto &state = impl->state;
  state.config = config;
  state.manifest = manifest;
  state.storage = &storage;

  if (!SafeLabel(config.label) || !SafeSession(config.session) ||
      config.root.empty()) {
    state.Fail(Reason::PrepareFailed, 0, "config");
    return false;
  }
  // 必须在 storage.Prepare 成功之后：writer 只消费已有的槽/队列/账本。
  if (!storage.Ready()) {
    state.Fail(Reason::PrepareFailed, 0, "storage_not_ready");
    return false;
  }
  state.queue = &storage.Queue();
  state.diag = &storage.Diag();
  state.ledger = &storage.Ledger();
  // 目录 = 固定根目录 + 受限 label；最终目录必须不存在（create-new）。
  const auto root = config.root;
  const auto separator = root.back() == L'\\' || root.back() == L'/';
  const auto directory =
      root + (separator ? L"" : L"\\") + WidenAscii(config.label);
  if (!EnsureDirectory(root)) {
    state.Fail(Reason::PrepareFailed, GetLastError(), "root_directory");
    return false;
  }
  if (!CreateOutputDirectory(directory)) {
    state.Fail(Reason::PrepareFailed, GetLastError(), "output_directory");
    return false;
  }
  state.directory = directory;

  // 成员关系分组 scratch 与 I/O 缓冲都记入同一 256 MiB 预算。
  const auto charge = kMembershipScratchBytes + kMembershipIndexBytes +
                      2 * std::uint64_t(kRecordBufferBytes) +
                      std::uint64_t(kDiagnosticBufferBytes) +
                      std::uint64_t(kDiagnosticBufferBytes);
  if (!state.ledger->Charge(charge)) {
    state.Fail(Reason::Budget, 0, "writer_budget");
    return false;
  }
  state.charged = charge;
  state.sorted_memberships =
      std::unique_ptr<RawMembership[]>(new (std::nothrow) RawMembership[kMembershipBudget]);
  state.membership_start = std::unique_ptr<std::uint32_t[]>(
      new (std::nothrow) std::uint32_t[kObjectBudget + 1]);
  state.membership_cursor = std::unique_ptr<std::uint32_t[]>(
      new (std::nothrow) std::uint32_t[kObjectBudget + 1]);
  if (!state.sorted_memberships || !state.membership_start ||
      !state.membership_cursor) {
    state.Fail(Reason::Budget, 0, "writer_scratch");
    return false;
  }
  const auto captures_path = directory + L"\\captures.jsonl";
  const auto events_path = directory + L"\\lifecycle-events.jsonl";
  const auto diagnostics_path = directory + L"\\diagnostics.jsonl";
  if (!state.captures.Open(captures_path, kRecordBufferBytes) ||
      !state.events.Open(events_path, kRecordBufferBytes) ||
      !state.diagnostics.Open(diagnostics_path, kDiagnosticBufferBytes)) {
    state.Fail(Reason::PrepareFailed, state.captures.LastError(), "open_files");
    return false;
  }
  if (!WriteManifest(state)) {
    state.Fail(Reason::PrepareFailed, state.small.LastError(), "manifest");
    return false;
  }
  WriteDiagnostic(state, "prepare", [&](JsonOut &j) {
    const auto status = storage.Status();
    j.Lit(",\"label\":");
    j.Str(config.label);
    j.Lit(",\"budgets\":{\"slots\":");
    j.Num(status.limits.slots);
    j.Lit(",\"events\":");
    j.Num(status.limits.event_capacity);
    j.Lit(",\"gaps\":");
    j.Num(status.limits.gap_capacity);
    j.Lit(",\"queue_bytes\":");
    j.Dec(status.limits.queue_bytes);
    j.Lit(",\"queue_items\":");
    j.Num(status.limits.queue_items);
    j.Lit("},\"allocated_bytes\":");
    j.Dec(status.budget_used);
    j.Lit(",\"planned_bytes\":");
    j.Dec(PlannedWorldBytes());
  });
  auto start = std::unique_ptr<WriterStart>(new (std::nothrow) WriterStart{impl});
  if (!start) {
    state.Fail(Reason::ThreadFailed, 0, "writer_start");
    return false;
  }
  const auto handle = CreateThread(nullptr, 0, WriterEntry, start.get(), 0, nullptr);
  if (!handle) {
    state.Fail(Reason::ThreadFailed, GetLastError(), "writer_thread");
    return false;
  }
  start.release();
  impl->thread = handle;
  state.opened.store(true, std::memory_order_release);
  return true;
} catch (...) {
  if (impl_) {
    impl_->state.Fail(Reason::Budget, ERROR_NOT_ENOUGH_MEMORY, "writer_prepare_exception");
    impl_->state.done.store(true, std::memory_order_release);
  }
  return false;
}

void WorldWriter::RequestStop() noexcept {
  if (impl_)
    impl_->state.stop.store(true, std::memory_order_release);
}

bool WorldWriter::Close(std::uint32_t wait_ms) noexcept {
  if (!impl_)
    return false;
  RequestStop();
  if (!wait_ms)
    return impl_->state.done.load(std::memory_order_acquire);
  if (impl_->thread) {
    const auto waited =
        WaitForSingleObject(impl_->thread, (std::min)(wait_ms, 15000u));
    if (waited == WAIT_OBJECT_0) {
      CloseHandle(impl_->thread);
      impl_->thread = nullptr;
    } else {
      return false;
    }
  }
  return impl_->state.done.load(std::memory_order_acquire);
}

bool WorldWriter::Closed() const noexcept {
  return !impl_ || impl_->state.done.load(std::memory_order_acquire);
}

WriterStatus WorldWriter::Status() const noexcept {
  WriterStatus status{};
  if (!impl_)
    return status;
  const auto &state = impl_->state;
  status.opened = state.opened.load(std::memory_order_acquire);
  status.files_flushed = state.flushed.load(std::memory_order_acquire);
  status.producer_done = state.queue && state.queue->ProducerDone();
  status.drain_complete = state.drain_complete.load(std::memory_order_acquire);
  status.write_failed = state.write_failed.load(std::memory_order_acquire);
  status.result_written = state.result_written.load(std::memory_order_acquire);
  status.records_written =
      state.records_written.load(std::memory_order_relaxed);
  status.events_written = state.events_written.load(std::memory_order_relaxed);
  status.gaps_written = state.gaps_written.load(std::memory_order_relaxed);
  status.diagnostics_written =
      state.diagnostics_written.load(std::memory_order_relaxed);
  status.discarded_items = state.discarded.load(std::memory_order_relaxed);
  status.write_calls = state.captures.WriteCalls();
  status.bytes_written = state.captures.BytesWritten();
  status.last_error = state.LastError();
  status.thread =
      static_cast<ThreadState>(state.thread_state.load(std::memory_order_acquire));
  status.outcome = impl_->outcome.load(std::memory_order_acquire);
  status.fault = state.diag ? state.diag->Fault().reason : Reason::None;
  return status;
}

const std::wstring &WorldWriter::Directory() const noexcept {
  static const std::wstring kEmpty;
  return impl_ ? impl_->state.directory : kEmpty;
}

} // namespace cs::world
