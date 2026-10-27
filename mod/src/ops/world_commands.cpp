#include "ops/world_commands.h"
#include "runtime.h"
#include "binding/world_layout.h"
#include "game/world_bridge.h"
#include "world/observe.h"
#include <array>
#include <memory>

namespace cs {
namespace {
// ready 发布前主线程不读 recording；后台的本地强引用保护其完整生命周期。
// 主线程撤销桥和 job 引用后才准许后台释放大数组，DllMain 不参与等待。
struct WorldJob final {
  std::uint64_t request = 0;
  std::uint32_t generation = 0, timeout = 10000;
  std::string label, session, version, root;
  world::MemoryReader memory{};
  std::array<bool, world::kObservationCount> observations{};
  std::shared_ptr<game::WorldRecording> recording;
  std::atomic<bool> ready{false}, cancel{false}, producer_done{false};
  std::atomic<bool> closed{false}, retired{false}, close_timeout{false};
  std::atomic<std::uint64_t> stop_at{0};
  std::atomic<world::Reason> fault{world::Reason::None};
  std::atomic<const char *> stage{""};
  HANDLE test_thread = nullptr; // 主线程创建；producer_done 发布后由准备 worker 等待真实退出。
  bool attached = false, stopping = false; // 仅主线程访问。
};
std::shared_ptr<WorldJob> job;
std::string snapshot = "ok=1\nworld_status_version=1\nworld_state=Off\nreason=NONE\n";

// Job 首因与 storage 首因分别保留，不用 Closing/Closed 覆盖错误。
void Fail(WorldJob &j, world::Reason reason, const char *stage) noexcept {
  auto expected = world::Reason::None;
  if (j.fault.compare_exchange_strong(expected, reason)) j.stage.store(stage);
}
std::wstring RootPath(const std::string &value) {
  const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
      value.data(), static_cast<int>(value.size()), nullptr, 0);
  if (!size) throw std::runtime_error("world_output_dir_utf8");
  std::wstring result(static_cast<size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
      static_cast<int>(value.size()), result.data(), size);
  return result;
}

// 版本字符串由 Job 持有到 writer 结束；布局表是静态值，构造 collector 不执行 copy。
bool Prepare(WorldJob &j, game::WorldRecording &r) {
  world::StorageConfig storage;
  storage.ledger = &r.ledger;
  if (!Settings.world_e2e_token.empty()) {
    storage.faults = &r.faults;
    if (j.label == "fault-events") storage.limits.event_capacity = 64;
    // VirtualAlloc 保留粒度为 64 KiB，提交一页也按整个保留范围计账。
    if (!r.ledger.Charge(65536)) return false;
    r.faults.unreadable = static_cast<std::uint32_t>(reinterpret_cast<uintptr_t>(
        VirtualAlloc(nullptr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS)));
    if (!r.faults.unreadable) { r.ledger.Release(65536); return false; }
  }
  if (!r.storage.Prepare(storage)) return false;
  if (j.cancel.load()) return false;
  if (!r.identities.Initialize(r.ledger)) {
    r.storage.Diag().Fail(world::Reason::Budget, world::FailSource::Prepare, 0, "identity_prepare");
    return false;
  }
  if (j.cancel.load()) return false;
  world::ManifestInput manifest;
  manifest.session = j.session.c_str();
  manifest.game_version = "Repentance+ v1.9.7.12.J273";
  manifest.repentogon_version = j.version.c_str(); manifest.build = Version;
  manifest.hook_manifest = world::layout::HookManifestReference();
  manifest.recording_generation = j.generation;
  manifest.observations = j.observations.data();
  manifest.layout_fields = world::layout::ManifestFields(manifest.layout_field_count);
  manifest.layout_sources = world::layout::ManifestSources(manifest.layout_source_count);
  manifest.layout_classes = world::layout::ManifestClasses(manifest.layout_class_count);
  manifest.assumptions = world::layout::Assumptions(manifest.assumption_count);
  manifest.unknowns = world::layout::Unknowns(manifest.unknown_count);
  const auto limits = r.storage.Status().limits;
  manifest.slot_count = limits.slots; manifest.event_capacity = limits.event_capacity;
  manifest.gap_capacity = limits.gap_capacity; manifest.queue_bytes = limits.queue_bytes;
  manifest.queue_items = limits.queue_items;
  world::WriterConfig writer;
  writer.root = RootPath(j.root); writer.label = j.label; writer.session = j.session;
  writer.close_timeout_ms = j.timeout;
  if (!r.writer.Open(writer, manifest, r.storage)) return false;
  r.collector = std::make_unique<world::Collector>(r.storage, r.identities, j.memory, j.generation);
  return true;
}

DWORD WINAPI Work(void *parameter) noexcept {
  std::unique_ptr<std::shared_ptr<WorldJob>> start(
      static_cast<std::shared_ptr<WorldJob> *>(parameter));
  auto current = std::move(*start);
  // 即使主线程已归还 job->recording，本地强引用也使最终释放留在后台。
  std::shared_ptr<game::WorldRecording> recording;
  bool prepared = false;
  try {
    recording = std::make_shared<game::WorldRecording>();
    current->recording = recording;
    prepared = Prepare(*current, *recording);
    if (!prepared && !current->cancel.load()) {
      const auto reason = recording->storage.Status().fault.reason;
      Fail(*current, reason == world::Reason::None ? world::Reason::PrepareFailed : reason, "prepare");
    }
  } catch (...) { Fail(*current, world::Reason::PrepareFailed, "prepare_exception"); }
  if (!prepared && recording && recording->storage.Ready() &&
      current->fault.load() != world::Reason::None)
    recording->storage.Diag().Fail(current->fault.load(), world::FailSource::Prepare, 0, "prepare");
  current->ready.store(true, std::memory_order_release);
  // 期限包含 stop 等待在途 producer 的时间；超时只记自有故障并保留 owner。
  const auto note_timeout = [&] {
    const auto since = current->stop_at.load();
    if (since && GetTickCount64() - since >= current->timeout &&
        !current->close_timeout.exchange(true)) {
      Fail(*current, world::Reason::ThreadFailed, "close_timeout");
      if (recording && recording->storage.Status().state != world::StorageState::Idle)
        recording->storage.Diag().Fail(world::Reason::ThreadFailed,
            world::FailSource::Prepare, 0, "close_timeout");
    }
  };
  // 未 Attach 的失败/取消由后台收口；成功准备后必须等主线程归还 producer。
  while (prepared && (!current->producer_done.load(std::memory_order_acquire) ||
                     !recording->faults.worker_done.load(std::memory_order_acquire))) {
    note_timeout(); Sleep(10);
  }
  std::uint64_t unset = 0;
  current->stop_at.compare_exchange_strong(unset, GetTickCount64());
  if (current->test_thread) {
    while (WaitForSingleObject(current->test_thread, 10) != WAIT_OBJECT_0) note_timeout();
    CloseHandle(current->test_thread); current->test_thread = nullptr;
  }
  if (recording) {
    recording->storage.RequestStop(world::Reason::Stopped);
    const bool writer_started = recording->writer.Status().opened;
    // reader 先结束生产，writer 自然 drain；提前 RequestStop 会使空队列早退。
    while (!recording->storage.Closed() || (writer_started && !recording->writer.Closed())) {
      note_timeout();
      Sleep(10);
    }
    // done 标记可能早于线程函数真正返回；必须取得真实退出，才能释放外部 ledger。
    if (recording->storage.Status().state != world::StorageState::Idle)
      while (!recording->storage.Close(10)) { note_timeout(); Sleep(10); }
    if (writer_started)
      while (!recording->writer.Close(10)) { note_timeout(); Sleep(10); }
  }
  current->closed.store(true, std::memory_order_release);
  while (!current->retired.load(std::memory_order_acquire)) Sleep(10);
  return 0;
}

// 快照只含主线程已有值和原子后台计数，尺寸远小于控制口上限。
void Publish(WorldJob &j, bool closed) {
  world::StorageStatus storage;
  world::WriterStatus writer;
  const bool ready = j.ready.load(std::memory_order_acquire);
  if (ready && j.recording) {
    storage = j.recording->storage.Status(); writer = j.recording->writer.Status();
  }
  const auto reason = storage.fault.reason != world::Reason::None
      ? storage.fault.reason : j.fault.load();
  const char *state = closed ? "Closed" : j.stopping ? "Closing"
      : reason != world::Reason::None ? "Faulted"
      : j.attached && j.recording->collector->Recording() ? "Recording" : "Preparing";
  snapshot = "ok=1\nworld_status_version=1\nworld_state=" + std::string(state) +
      "\nreason=" + world::ReasonName(reason) + "\nfault_stage=" +
      (storage.fault.reason != world::Reason::None ? storage.fault.stage : j.stage.load()) +
      "\nrequest=" + std::to_string(j.request) + "\nrecording=" + j.session +
      "\ngeneration=" + std::to_string(j.generation) + "\nlabel=" + j.label +
      "\ndirectory=" + j.root + "/" + j.label +
      "\nattempt=" + std::to_string(storage.attempts) +
      "\ncomplete=" + std::to_string(storage.complete_captures) +
      "\ngap=" + std::to_string(storage.gaps) +
      "\nbackpressure=" + std::to_string(storage.backpressure) +
      "\ncallbacks=" + std::to_string(storage.callbacks) +
      "\ncheckpoint_produced=" + std::to_string(ready && j.recording &&
          storage.state != world::StorageState::Idle ? j.recording->storage.Diag().checkpoints_produced.load() : 0) +
      "\ncheckpoint_written=" + std::to_string(ready && j.recording &&
          storage.state != world::StorageState::Idle ? j.recording->storage.Diag().checkpoints_written.load() : 0) +
      "\nreader=" + world::ThreadStateName(storage.reader) +
      "\nwriter=" + world::ThreadStateName(writer.thread) +
      "\nfiles_flushed=" + std::to_string(writer.files_flushed) +
      "\nresult_written=" + std::to_string(writer.result_written) +
      "\ndrain_complete=" + std::to_string(writer.drain_complete) +
      "\nclose_timeout=" + std::to_string(j.close_timeout.load()) +
      "\noutcome=" + world::WriterOutcomeName(writer.outcome) + "\n";
  if (ready && j.recording) {
    const auto &f = j.recording->faults;
    snapshot += "test_mode=" + std::to_string(f.mode.load()) +
        "\ntest_applied=" + std::to_string(f.applied.load()) +
        "\ntest_before=" + std::to_string(f.before.load()) +
        "\ntest_after=" + std::to_string(f.after.load()) + "\n";
  }
}
} // namespace

bool WorldCommandBusy() noexcept { return bool(job); }
std::string WorldRecordingId(std::uint32_t generation) {
  static const auto process_epoch = GetTickCount64();
  return "world-" + std::to_string(GetCurrentProcessId()) + "-" +
      std::to_string(process_epoch) + "-" + std::to_string(generation);
}
const char *WorldStartError() noexcept {
  if (!Settings.enabled || !Settings.world_record_enabled) return "world_disabled";
  if (!Capabilities[size_t(Group::World)] || !Capabilities[size_t(Group::Loop)] ||
      !Capabilities[size_t(Group::Player)]) return "world_capabilities_unavailable";
  bool mask[world::kObservationCount]{};
  world::layout::ObservationMask(mask, world::kObservationCount);
  for (const auto registered : mask) if (!registered) return "world_observations_unavailable";
  return nullptr;
}
bool WorldCanCheckpoint() noexcept {
  return job && job->attached && !job->stopping && job->recording->collector->Recording();
}
void StartWorldCommand(std::uint64_t request, std::uint32_t generation, const char *label) noexcept {
  try {
    if (job) return;
    job = std::make_shared<WorldJob>();
    job->request = request; job->generation = generation; job->label = label;
    job->root = Settings.world_output_dir; job->timeout = Settings.world_close_timeout_ms;
    job->version = RepentogonVersion; job->memory = game::WorldMemoryReader();
    job->session = WorldRecordingId(generation);
    world::layout::ObservationMask(job->observations.data(), job->observations.size());
    if (const auto error = WorldStartError()) {
      Fail(*job, world::Reason::PrepareFailed, error);
      job->ready.store(true); job->closed.store(true); return;
    }
    if (!job->memory.copy || !job->memory.image_base || !job->memory.image_size) {
      Fail(*job, world::Reason::PrepareFailed, "world_memory_reader");
      job->ready.store(true); job->closed.store(true); return;
    }
    auto start = std::make_unique<std::shared_ptr<WorldJob>>(job);
    const auto thread = CreateThread(nullptr, 0, Work, start.get(), 0, nullptr);
    if (!thread) {
      Fail(*job, world::Reason::ThreadFailed, "prepare_thread");
      job->ready.store(true); job->closed.store(true); return;
    }
    start.release(); CloseHandle(thread);
  } catch (...) {
    if (job) {
      Fail(*job, world::Reason::PrepareFailed, "start_exception");
      job->ready.store(true); job->closed.store(true);
    }
    Trace("world_command", "request=%llu error=start_exception", request);
  }
}
void StopWorldCommand(std::uint64_t request, std::uint32_t generation) noexcept {
  if (!job || job->generation != generation) return;
  job->stopping = true; job->cancel.store(true);
  std::uint64_t unset = 0;
  job->stop_at.compare_exchange_strong(unset, GetTickCount64());
  if (job->attached) game::StopWorldRecording();
  Trace("world_stop", "request=%llu generation=%u", request, generation);
}
void CheckpointWorldCommand(std::uint64_t request, std::uint32_t generation, const char *label) noexcept {
  const bool ok = WorldCanCheckpoint() && job->generation == generation &&
      job->recording->collector->Checkpoint(request, label);
  Trace("world_checkpoint", "request=%llu generation=%u label=%s accepted=%u",
        request, generation, label, unsigned(ok));
}
void PumpWorldCommands() noexcept {
  if (!job || !IsMain()) return;
  try {
    if (job->ready.load(std::memory_order_acquire) && !job->closed.load()) {
      auto &r = job->recording;
      if (job->attached && r && r->faults.pending == 9 && r->faults.reader_paused.load(std::memory_order_acquire)) {
        // reader 已确认暂停后才填真实环，最多 capacity+1 次，不等待 reader。
        auto &f = r->faults; f.pending = 0;
        world::LifecycleEvent diagnostic{};
        diagnostic.kind = world::EventKind::GapOrUnknown; diagnostic.result = 0x54455354;
        const auto capacity = r->storage.Status().limits.event_capacity;
        std::uint32_t count = 0;
        while (count <= capacity && r->storage.PublishEvent(diagnostic)) ++count;
        f.before.store(capacity); f.after.store(count);
        f.applied.store(r->storage.Diag().FaultReason() == world::Reason::EventOverflow);
      }
      if (job->fault.load() != world::Reason::None ||
          (r && r->storage.Status().fault.reason != world::Reason::None)) job->stopping = true;
      if (!job->stopping && !job->attached) {
        if (game::AttachPreparedWorld(r)) job->attached = true;
        else { Fail(*job, world::Reason::PrepareFailed, "attach"); job->stopping = true; }
      }
      if (job->stopping) {
        std::uint64_t unset = 0;
        job->stop_at.compare_exchange_strong(unset, GetTickCount64());
        if (r && r->storage.Ready() && job->fault.load() != world::Reason::None)
          r->storage.Diag().Fail(job->fault.load(), world::FailSource::Producer, 0, job->stage.load());
        if (job->attached) game::StopWorldRecording();
        if (!job->attached || r->collector->Drained()) job->producer_done.store(true);
      }
    }
    const bool closed = job->closed.load(std::memory_order_acquire);
    Publish(*job, closed);
    if (closed) {
      if (job->attached && !game::DetachClosedWorld(job->recording)) return;
      // worker 的本地强引用尚在；先归还主线程 owner，再准许后台释放。
      job->recording.reset();
      job->retired.store(true, std::memory_order_release);
      job.reset();
    }
  } catch (...) { Trace("world_command", "error=pump_exception"); }
}
const std::string &WorldCommandStatus() noexcept { return snapshot; }

// 此命令在 port 请求的 Manager 主线程分派处执行，不进入 UDP/玩家 Lua 接口。
std::string TestWorldCommand(const char *source, const std::string &token,
    std::uint32_t generation, const std::string &mode, std::uint32_t value) {
  if (!IsMain() || !source || std::strcmp(source, "port") ||
      Settings.world_e2e_token.empty() || token != Settings.world_e2e_token)
    return "ok=0\nerror=world_test_disabled_or_token\n";
  if (!WorldCanCheckpoint() || job->generation != generation)
    return "ok=0\nerror=world_test_recording_generation\n";
  static constexpr const char *names[] = {"reader_pause", "objects", "memberships", "relations",
      "read", "source", "root", "reference", "events", "thread", "file"};
  std::uint32_t code = 0;
  for (std::uint32_t i = 0; i < std::size(names); ++i) if (mode == names[i]) code = i + 1;
  auto &r = *job->recording; auto &f = r.faults;
  if (mode == "resume" && value == 1 && f.mode.load() == 1) {
    f.pause_reader.store(false); return "ok=1\nresumed=1\n";
  }
  if (!code || (code == 1 ? value < 100 || value > 500 : value != 1))
    return "ok=0\nerror=world_test_arguments\n";
  if (f.mode.load()) return "ok=0\nerror=world_test_already_used\n";
  f.value.store(value); f.attempt.store(r.storage.Diag().last_attempt.load());
  f.watermark.store(r.storage.EventWatermark()); f.mode.store(code);
  if (code == 1 || code == 9) {
    f.pause_until.store(GetTickCount64() + (code == 1 ? value : 500));
    f.pause_reader.store(true);
  }
  if (code == 1) f.applied.store(1);
  else if (code == 2) { f.object_limit = value; f.pending = code; }
  else if (code == 3) { f.membership_limit = value; f.pending = code; }
  else if (code == 4) { f.relation_limit = value; f.pending = code; }
  else if (code <= 8) f.pending = code;
  else if (code == 9) {
    f.pending = code; // 下一 Manager 从暂停确认继续，无主线程等待。
  } else if (code == 10) {
    auto parameter = std::make_unique<std::shared_ptr<game::WorldRecording>>(job->recording);
    f.worker_done.store(false);
    const auto thread = CreateThread(nullptr, 0, [](void *p) -> DWORD {
      std::unique_ptr<std::shared_ptr<game::WorldRecording>> owner(
          static_cast<std::shared_ptr<game::WorldRecording> *>(p));
      auto &state = (**owner).faults;
      // main=false 由实际线程身份计算；这里只进 observer 策略检查，绝不调用 original。
      { world::ObservationScope notification(world::Observation::Factory, IsMain(), nullptr, {}, {}); }
      state.applied.store(1); state.worker_done.store(true, std::memory_order_release);
      return 0;
    }, parameter.get(), 0, nullptr);
    if (!thread) {
      f.worker_done.store(true); f.mode.store(0);
      return "ok=0\nerror=world_test_thread_create\n";
    }
    parameter.release(); job->test_thread = thread;
  } else if (code == 11) f.drop_capture_file.store(true);
  Trace("world_test", "injected=TEST_INJECTED generation=%u recording=%s mode=%s value=%u attempt=%llu watermark=%llu",
      generation, job->session.c_str(), mode.c_str(), value, f.attempt.load(), f.watermark.load());
  return "ok=1\ninjected=TEST_INJECTED\ngeneration=" + std::to_string(generation) +
      "\nrecording=" + job->session + "\nmode=" + mode + "\nvalue=" + std::to_string(value) + "\n";
}
} // namespace cs
