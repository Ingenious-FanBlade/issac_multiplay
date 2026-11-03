#include "ops/world_commands.h"
#include "runtime.h"
#include "binding/world_layout.h"
#include "game/world_bridge.h"
#include "game/world_sources/lua_entity.h"
#include "game/tick.h"
#include "world/observe.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>

namespace cs {
namespace {
// Recorder 仅拥有文件和值队列；Capture 独立拥有身份、Collector、域与采集 epoch。
struct RecordingRequest final {
  std::uint64_t request=0;
  std::uint32_t generation=0;
  std::string label,session,version,root;
  std::atomic<bool> ready{false},stop{false},closed{false};
  std::atomic<world::Reason> fault{world::Reason::None};
  std::atomic<std::uint64_t> v2_written{0};
  std::uint64_t stop_attempt=0,stop_events=0,stop_gaps=0,start_loss=0,start_capture_loss=0;
  bool historical_gap=false,charged=false;
  std::unique_ptr<world::ValueQueue> queue;
  world::WorldWriter writer;
  std::ofstream v2;
};
struct WorldJob final {
  std::uint64_t request=0;
  std::uint32_t generation=0,timeout=10000;
  std::string label,session,version,root;
  world::MemoryReader memory{};
  std::array<bool,world::kObservationCount> observations{};
  std::shared_ptr<game::WorldRecording> recording;
  std::unique_ptr<RecordingRequest> consumer;
  std::atomic<RecordingRequest *> next_consumer{nullptr};
  std::atomic<bool> ready{false},cancel{false},producer_done{false},closed{false},retired{false};
  std::atomic<std::uint64_t> stop_at{0};
  std::atomic<world::Reason> fault{world::Reason::None};
  std::atomic<const char *> stage{""};
  HANDLE test_thread=nullptr;
  bool attached=false,stopping=false,formal=false;
};
std::shared_ptr<WorldJob> job;
bool process_exit_pending=false,process_exit_released=false;
std::uint64_t process_exit_started=0;
std::string snapshot="ok=1\nworld_status_version=1\nworld_state=Off\nreason=NONE\n";
std::uint64_t next_epoch=0;
constexpr std::uint64_t kObservationReserve=world::kObservationReserveBytes;
void Fail(WorldJob &j,world::Reason reason,const char *stage) noexcept {
  auto expected=world::Reason::None;
  if(j.fault.compare_exchange_strong(expected,reason)) j.stage.store(stage);
}
std::wstring RootPath(const std::string &value) {
  const auto size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),nullptr,0);
  if(!size) throw std::runtime_error("world_output_dir_utf8");
  std::wstring out(size,L'\0'); MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),out.data(),size); return out;
}
world::ManifestInput Manifest(WorldJob &j,RecordingRequest &q) {
  world::ManifestInput m;
  m.session=q.session.c_str(); m.game_version="Repentance+ v1.9.7.12.J273";
  m.repentogon_version=q.version.c_str(); m.build=Version; m.hook_manifest=world::layout::HookManifestReference();
  m.recording_generation=q.generation; m.observations=j.observations.data();
  m.layout_fields=world::layout::ManifestFields(m.layout_field_count);
  m.layout_sources=world::layout::ManifestSources(m.layout_source_count);
  m.layout_classes=world::layout::ManifestClasses(m.layout_class_count);
  m.assumptions=world::layout::Assumptions(m.assumption_count); m.unknowns=world::layout::Unknowns(m.unknown_count);
  const auto limits=j.recording->storage.Status().limits;
  m.slot_count=limits.slots; m.event_capacity=limits.event_capacity; m.gap_capacity=limits.gap_capacity;
  m.queue_bytes=limits.queue_bytes; m.queue_items=limits.queue_items; return m;
}
bool Prepare(WorldJob &j,game::WorldRecording &r) {
  r.source_memory=j.memory;
  world::StorageConfig config; config.ledger=&r.ledger;
  if(!Settings.world_e2e_token.empty()) {
    config.faults=&r.faults;
    if(!r.ledger.Charge(65536)) return false;
    r.faults.unreadable=std::uint32_t(reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS)));
    if(!r.faults.unreadable) { r.ledger.Release(65536); return false; }
  }
  if(!r.storage.Prepare(config)||j.cancel.load()||!r.identities.Initialize(r.ledger)||!r.ledger.Charge(kObservationReserve)) return false;
  r.reserve_charged=true;
  // reserve 同时覆盖最多三个 pending payload、两个最新值及编码在途值，发布队列另计。
  r.collector=std::make_unique<world::Collector>(r.storage,r.identities,j.memory,std::uint32_t(r.epoch));
  return true;
}
bool OpenRecorder(WorldJob &j,RecordingRequest &q) {
  auto &r=*j.recording;
  if(!r.ledger.Charge(world::kQueueBytes)) return false;
  q.charged=true; q.queue=std::make_unique<world::ValueQueue>(world::kQueueBytes,world::kRecorderQueueItems);
  world::WriterConfig config; config.root=RootPath(q.root); config.label=q.label; config.session=q.session; config.close_timeout_ms=j.timeout;
  if(!q.writer.Open(config,Manifest(j,q),r.storage,q.queue.get())) return false;
  const auto directory=std::filesystem::path(q.writer.Directory())/L"observation-v2";
  std::filesystem::create_directory(directory);
  q.v2.open(directory/L"records.jsonl",std::ios::binary|std::ios::out);
  std::ofstream manifest(directory/L"manifest.json",std::ios::binary|std::ios::out);
  manifest<<"{\"format\":\"isaac.cs.observation\",\"version\":2,\"session\":\""<<r.session_key
    <<"\",\"observation_epoch\":\""<<r.epoch<<"\",\"recording_generation\":"<<q.generation
    <<",\"catalog\":\"entity-catalog.json\",\"grid_catalog\":\"grid-catalog.json\",\"room_level_catalog\":\"room-level-catalog.json\",\"continuity_catalog\":\"continuity-catalog.json\",\"run_catalog\":\"run-catalog.json\",\"schemas\":[\"isaac.cs.entity\",\"isaac.cs.entity.native-v1\",\"isaac.cs.entity.lifecycle\",\"isaac.cs.entity.lua-event\",\"isaac.cs.entity.query\",\"isaac.cs.entity.actual-call\",\"isaac.cs.entity.render\",\"isaac.cs.entity.coverage\",\"isaac.cs.grid.legacy\",\"isaac.cs.grid.lifecycle\",\"isaac.cs.grid\",\"isaac.cs.grid.lua-event\",\"isaac.cs.grid.query\",\"isaac.cs.grid.render\",\"isaac.cs.room\",\"isaac.cs.level\",\"isaac.cs.level.transition\",\"isaac.cs.room.event\",\"isaac.cs.room.render\",\"isaac.cs.room.query\",\"isaac.cs.level.query\",\"isaac.cs.level.event\",\"isaac.cs.exploration\",\"isaac.cs.exploration.transition\",\"isaac.cs.room_save\",\"isaac.cs.room_save.transition\",\"isaac.cs.room_save.event\",\"isaac.cs.player\",\"isaac.cs.player.transition\",\"isaac.cs.player.event\",\"isaac.cs.player.query\",\"isaac.cs.player.control\",\"isaac.cs.run\",\"isaac.cs.run.event\",\"isaac.cs.run.transition\",\"isaac.cs.run.query\",\"isaac.cs.run.render\",\"isaac.cs.run.actual-call\",\"isaac.cs.random\",\"isaac.cs.random.event\",\"isaac.cs.random.transition\",\"isaac.cs.random.query\",\"isaac.cs.random.render\",\"isaac.cs.random.actual-call\",\"isaac.cs.random.native-global-block\",\"isaac.cs.random.native-global-call\",\"isaac.cs.random.native-call\",\"isaac.cs.random.native-state-change\",\"isaac.cs.random.native-retire\",\"isaac.cs.random.native-checkpoint\",\"isaac.cs.random.runtime-call\",\"isaac.cs.presentation\",\"isaac.cs.presentation.event\",\"isaac.cs.presentation.transition\",\"isaac.cs.presentation.query\",\"isaac.cs.presentation.render\",\"isaac.cs.presentation.actual-call\",\"isaac.cs.mod\",\"isaac.cs.mod.event\",\"isaac.cs.mod.transition\",\"isaac.cs.mod.query\",\"isaac.cs.mod.render\",\"isaac.cs.mod.actual-call\"],\"enabled_domains\":[1,2,3,4,5,6,7,8,9,10,11],\"unsupported_domains\":[],\"capture_plan\":{\"ordinary_interval\":30,\"lifecycle\":true,\"explicit_queries\":true},\"budgets\":{\"total_bytes\":"<<world::kWorldBudgetBytes<<",\"publication_bytes\":33554432,\"recorder_queue_items\":256,\"scope_bytes\":16777216,\"nodes\":262144,\"depth\":12},\"legacy_grid_range\":\"min-read-v1-only\",\"v1_identity_mapping\":\"same-observation-epoch-local-id-and-use\",\"v1_history_complete\":"<<(q.historical_gap?"false":"true")<<"}";
  // REPENTOGON 进程工作目录位于游戏的 Repentogon 子目录，资源在其兄弟 mods 目录。
  wchar_t executable[32768]{};
  const auto length=GetModuleFileNameW(nullptr,executable,DWORD(std::size(executable)));
  if(!length||length>=std::size(executable)) return false;
  const auto catalog=std::filesystem::path(executable).parent_path().parent_path()/L"mods"/L"isaac-cs-observation"/L"entity-catalog.json";
  std::filesystem::copy_file(catalog,directory/L"entity-catalog.json");
  std::filesystem::copy_file(catalog.parent_path()/L"grid-catalog.json",directory/L"grid-catalog.json");
  std::filesystem::copy_file(catalog.parent_path()/L"room-level-catalog.json",directory/L"room-level-catalog.json");
  std::filesystem::copy_file(catalog.parent_path()/L"continuity-catalog.json",directory/L"continuity-catalog.json");
  std::filesystem::copy_file(catalog.parent_path()/L"run-catalog.json",directory/L"run-catalog.json");
  manifest.flush(); q.start_loss=r.publications.LossSequence(); q.start_capture_loss=r.observation.LossSequence();
  return q.v2.good()&&manifest.good();
}
void MapRecording(RecordingRequest &q,world::QueueItem &item) {
  if(item.kind==world::ItemKind::Batch) {
    item.batch.header.recording_generation=q.generation;
    if(q.historical_gap) { item.batch.header.complete=0; item.batch.header.reason=unsigned(world::Reason::Continuity); }
  } else if(item.kind==world::ItemKind::Gaps) for(auto &v:item.gaps) v.recording_generation=q.generation;
  else if(item.kind==world::ItemKind::Checkpoints) for(auto &v:item.checkpoints) v.recording_generation=q.generation;
}
void Forward(RecordingRequest &q,world::QueueItem &item) {
  if(!q.queue||!q.ready.load()||q.fault.load()!=world::Reason::None) return;
  if(q.stop.load(std::memory_order_acquire)) {
    if(item.kind==world::ItemKind::Batch&&item.batch.header.attempt>q.stop_attempt) return;
    if(item.kind==world::ItemKind::Events) item.events.erase(std::remove_if(item.events.begin(),item.events.end(),[&](const auto &v){return v.sequence>q.stop_events;}),item.events.end());
    if(item.kind==world::ItemKind::Gaps) item.gaps.erase(std::remove_if(item.gaps.begin(),item.gaps.end(),[&](const auto &v){return v.attempt>q.stop_attempt;}),item.gaps.end());
  }
  MapRecording(q,item);
  const auto bytes=item.bytes;
  if(!q.queue->Reserve(bytes,10)) { q.fault.store(world::Reason::Backpressure); return; }
  if(!q.queue->Push(std::move(item))) { q.queue->Release(bytes); q.fault.store(world::Reason::Backpressure); }
}
void CloseRecorder(WorldJob &j,RecordingRequest &q) {
  if(q.queue) q.queue->CloseProducer();
  if(q.writer.Status().opened) while(!q.writer.Closed()) Sleep(5);
  if(q.writer.Status().opened) { q.writer.Close(100); if(q.writer.Status().fault!=world::Reason::None) q.fault.store(q.writer.Status().fault); }
  try { if(q.v2.is_open()) {
    q.v2.flush(); if(!q.v2.good()) q.fault.store(world::Reason::FileError); q.v2.close();
    // 扩展 Windows 路径不转换内部正斜杠，每个组成段分别拼接。
    std::ofstream result(std::filesystem::path(q.writer.Directory())/L"observation-v2"/L"result.json",std::ios::binary|std::ios::out);
    const auto lost=j.recording->publications.LossSequence()-q.start_loss;
    const auto scope_lost=j.recording->observation.LossSequence()-q.start_capture_loss;
    result<<"{\"format\":\"isaac.cs.observation\",\"version\":2,\"closed\":true,\"records\":"<<q.v2_written.load()<<",\"loss_sequence_delta\":"<<lost<<",\"capture_loss_sequence_delta\":"<<scope_lost<<",\"complete_delivery\":"<<(!lost&&!scope_lost&&q.fault.load()==world::Reason::None?"true":"false")<<",\"fault\":\""<<world::ReasonName(q.fault.load())<<"\"}";
    result.flush(); if(!result.good()) q.fault.store(world::Reason::FileError);
  }
  } catch(...) { q.fault.store(world::Reason::FileError); }
  if(q.charged) { j.recording->ledger.Release(world::kQueueBytes); q.charged=false; }
  j.next_consumer.store(nullptr,std::memory_order_release);
  q.closed.store(true,std::memory_order_release);
}
// 唯一后台分派线程：无录制时归还值内存；文件错误只退休 Recorder，不关闭 Capture。
DWORD WINAPI Work(void *parameter) noexcept {
  std::unique_ptr<std::shared_ptr<WorldJob>> start(static_cast<std::shared_ptr<WorldJob> *>(parameter));
  auto j=std::move(*start); bool prepared=false;
  try { prepared=Prepare(*j,*j->recording); } catch(...) { Fail(*j,world::Reason::PrepareFailed,"capture_prepare_exception"); }
  if(!prepared) Fail(*j,world::Reason::PrepareFailed,"capture_prepare");
  j->ready.store(true,std::memory_order_release);
  auto &r=*j->recording;
  RecordingRequest *current=nullptr;
  if(!prepared&&r.storage.Status().state==world::StorageState::Idle) { j->closed.store(true,std::memory_order_release); while(!j->retired.load()) Sleep(5); return 0; }
  if(!prepared) r.storage.RequestStop(world::Reason::Stopped);
  for(;;) {
    if(!current) {
      current=j->next_consumer.load(std::memory_order_acquire);
      if(current) {
        bool opened=false;
        try { opened=prepared&&OpenRecorder(*j,*current); }
        catch(const std::exception &e) { Trace("recorder_open","reason=%s",e.what()); opened=false; }
        catch(...) { Trace("recorder_open","reason=unknown_exception"); opened=false; }
        if(!opened) current->fault.store(world::Reason::PrepareFailed);
        current->ready.store(true,std::memory_order_release);
      }
    }
    world::QueueItem item;
    bool got=false;
    // 有界轮转：高密度v2事件不能让legacy值流长期得不到分派，反之亦然。
    for(unsigned batch=0;batch<4&&r.storage.HasQueue()&&r.storage.Queue().Pop(item,batch?0:1);++batch) {
      got=true; const auto bytes=item.bytes;
      if(current&&current->fault.load()==world::Reason::None) Forward(*current,item);
      item={}; r.storage.Queue().Release(bytes);
    }
    std::string value;
    unsigned values=0;
    while(values++<128&&r.publications.Pop(value)) {
      if(current&&current->ready.load()&&current->fault.load()==world::Reason::None) {
        current->v2<<value<<'\n';
        if(!current->v2.good()) current->fault.store(world::Reason::FileError);
        else ++current->v2_written;
      }
      value.clear();
    }
    if(current&&current->writer.Status().write_failed) current->fault.store(current->writer.Status().fault);
    if(current&&current->stop.load(std::memory_order_acquire)&&
       r.storage.PassedFence(current->stop_attempt,current->stop_events,current->stop_gaps)&&(!r.storage.HasQueue()||r.storage.Queue().Empty())&&r.publications.Empty()) {
      CloseRecorder(*j,*current); current=nullptr;
    }
    if(j->producer_done.load(std::memory_order_acquire)||!prepared) r.storage.RequestStop(world::Reason::Stopped);
    if(r.storage.Closed()&&(!r.storage.HasQueue()||r.storage.Queue().Empty())&&r.publications.Empty()) {
      if(current) { CloseRecorder(*j,*current); current=nullptr; }
      break;
    }
    if(!got) Sleep(1);
  }
  if(j->test_thread) { WaitForSingleObject(j->test_thread,INFINITE); CloseHandle(j->test_thread); j->test_thread=nullptr; }
  if(r.storage.Status().state!=world::StorageState::Idle) while(!r.storage.Close(10)) Sleep(5);
  j->closed.store(true,std::memory_order_release);
  while(!j->retired.load(std::memory_order_acquire)) Sleep(5);
  return 0;
}
const char *CaptureError() noexcept {
  if(process_exit_pending) return "process_exiting";
  if(!Settings.enabled) return "disabled";
  if(!WorldExitPatchInstalled()) return "world_exit_patch_unavailable";
  if(!Capabilities[size_t(Group::World)]||!Capabilities[size_t(Group::Loop)]||!Capabilities[size_t(Group::Player)]) return "world_capabilities_unavailable";
  return nullptr;
}
bool EnsureJob() {
  if(job) return !job->stopping;
  if(CaptureError()) return false;
  if(next_epoch==UINT32_MAX) return false;
  job=std::make_shared<WorldJob>(); job->recording=std::make_shared<game::WorldRecording>();
  job->recording->epoch=++next_epoch;
  static const auto process_started=GetTickCount64();
  job->recording->session_key=(std::uint64_t(GetCurrentProcessId())<<32)|(process_started&UINT32_MAX);
  job->root=Settings.world_output_dir; job->timeout=Settings.world_close_timeout_ms;
  job->version=RepentogonVersion; job->memory=game::WorldMemoryReader();
  world::layout::ObservationMask(job->observations.data(),job->observations.size());
  auto start=std::make_unique<std::shared_ptr<WorldJob>>(job);
  const auto thread=CreateThread(nullptr,0,Work,start.get(),0,nullptr);
  if(!thread) { Fail(*job,world::Reason::ThreadFailed,"capture_thread"); job->ready.store(true); job->closed.store(true); return false; }
  start.release(); CloseHandle(thread); return true;
}
void Publish(WorldJob &j) {
  const auto ready=j.ready.load(std::memory_order_acquire);
  const auto storage=ready?j.recording->storage.Status():world::StorageStatus{};
  auto *q=j.consumer.get(); const auto qready=q&&q->ready.load(std::memory_order_acquire);
  const auto writer=qready?q->writer.Status():world::WriterStatus{};
  const auto reason=q&&q->fault.load()!=world::Reason::None?q->fault.load():storage.fault.reason!=world::Reason::None?storage.fault.reason:j.fault.load();
  const char *state=!q?"Off":q->closed.load()?"Closed":q->stop.load()?"Closing":reason!=world::Reason::None?"Faulted":qready&&j.attached&&j.recording->collector->Recording()?"Recording":"Preparing";
  snapshot="ok=1\nworld_status_version=1\nworld_state="+std::string(state)+"\nreason="+world::ReasonName(reason)+
    "\nrequest="+std::to_string(j.request)+"\ngeneration="+std::to_string(j.generation)+"\nrecording="+j.session+
    "\nlabel="+j.label+"\ndirectory="+j.root+"/"+j.label+"\nattempt="+std::to_string(storage.attempts)+
    "\ncomplete="+std::to_string(storage.complete_captures)+"\ngap="+std::to_string(storage.gaps)+
    "\nbackpressure="+std::to_string(storage.backpressure)+"\ncallbacks="+std::to_string(storage.callbacks)+
    "\ncheckpoint_written="+std::to_string(ready&&storage.state!=world::StorageState::Idle?j.recording->storage.Diag().checkpoints_written.load():0)+
    "\ncheckpoint_produced="+std::to_string(ready&&storage.state!=world::StorageState::Idle?j.recording->storage.Diag().checkpoints_produced.load():0)+
    "\nwriter_records="+std::to_string(writer.records_written)+"\nwriter_events="+std::to_string(writer.events_written)+
    "\nwriter_gaps="+std::to_string(writer.gaps_written)+"\nwriter_result="+std::to_string(writer.result_written)+
    "\nobservation_epoch="+std::to_string(j.recording->epoch)+"\nobservation_active="+std::to_string(j.attached&&!j.stopping)+
    "\nv2_records="+std::to_string(q?q->v2_written.load():0)+"\n";
}
}
bool WorldCommandBusy() noexcept { return job&&job->consumer&&!job->consumer->closed.load(); }
std::string WorldRecordingId(std::uint32_t generation) { return "world-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(generation); }
const char *WorldStartError() noexcept { if(!Settings.world_record_enabled) return "world_disabled"; return CaptureError(); }
bool WorldCanCheckpoint() noexcept { return job&&job->attached&&job->consumer&&job->consumer->ready.load()&&!job->consumer->closed.load()&&!job->consumer->stop.load(); }
void StartWorldCommand(std::uint64_t request,std::uint32_t generation,const char *label) noexcept {
  try {
    if(WorldStartError()||WorldCommandBusy()||!EnsureJob()) return;
    job->consumer=std::make_unique<RecordingRequest>(); auto &q=*job->consumer;
    job->request=q.request=request; job->generation=q.generation=generation; job->label=q.label=label;
    job->session=q.session=WorldRecordingId(generation); q.root=job->root; q.version=job->version;
    q.historical_gap=job->attached; job->next_consumer.store(&q,std::memory_order_release);
  } catch(...) { if(job) Fail(*job,world::Reason::PrepareFailed,"recorder_request"); }
}
void StopWorldCommand(std::uint64_t,std::uint32_t generation) noexcept {
  if(!job||!job->consumer||job->generation!=generation||job->consumer->stop.load()) return;
  auto &r=*job->recording; auto &q=*job->consumer;
  if(job->attached) r.SetConsumers(job->formal,false);
  if(job->ready.load(std::memory_order_acquire)) { q.stop_attempt=r.storage.Diag().last_attempt.load(); q.stop_events=r.storage.EventWatermark(); q.stop_gaps=r.storage.Diag().gaps_produced.load(); }
  q.stop.store(true,std::memory_order_release);
  if(!job->formal) job->stopping=true;
}
void CheckpointWorldCommand(std::uint64_t request,std::uint32_t generation,const char *label) noexcept {
  if(WorldCanCheckpoint()&&job->generation==generation) job->recording->collector->Checkpoint(request,label);
}
void PumpWorldCommands() noexcept {
  if(!IsMain()) return;
  game::EnsureObservationLuaBridge();
  if(!job) return;
  try {
    if(job->ready.load(std::memory_order_acquire)&&!job->closed.load()) {
      auto &r=*job->recording;
      if(job->fault.load()!=world::Reason::None||r.storage.Status().fault.reason!=world::Reason::None) job->stopping=true;
      auto *q=job->consumer.get(); const bool qready=q&&q->ready.load(std::memory_order_acquire);
      if(!job->attached&&!job->stopping) {
        if(game::AttachPreparedWorld(job->recording)) job->attached=true;
        else { Fail(*job,world::Reason::PrepareFailed,"capture_attach"); job->stopping=true; }
      }
      if(job->attached) r.SetConsumers(job->formal,qready&&!q->stop.load()&&!q->closed.load()&&q->fault.load()==world::Reason::None);
      if(qready&&q->fault.load()!=world::Reason::None&&!q->stop.load()) StopWorldCommand(0,q->generation);
      if(job->attached&&r.faults.pending==9&&r.faults.reader_paused.load()) {
        auto &f=r.faults; f.pending=0; world::LifecycleEvent e{}; e.kind=world::EventKind::GapOrUnknown; e.result=0x54455354;
        const auto capacity=r.storage.Status().limits.event_capacity; unsigned count=0;
        while(count<=capacity&&r.storage.PublishEvent(e)) ++count;
        f.before.store(capacity); f.after.store(count); f.applied.store(r.storage.Diag().FaultReason()==world::Reason::EventOverflow);
      }
      if(job->stopping) {
        std::uint64_t unset=0; job->stop_at.compare_exchange_strong(unset,GetTickCount64());
        if(job->attached) game::StopWorldRecording();
        if(!job->attached||r.collector->Drained()) job->producer_done.store(true,std::memory_order_release);
      }
    }
    Publish(*job);
    if(job->closed.load(std::memory_order_acquire)) {
      if(job->attached&&!game::DetachClosedWorld(job->recording)) return;
      job->retired.store(true,std::memory_order_release); job.reset();
    }
  } catch(...) { if(job) { Fail(*job,world::Reason::PrepareFailed,"capture_pump"); job->stopping=true; } }
}
const std::string &WorldCommandStatus() noexcept { return snapshot; }
std::string ObservationCommand(const std::string &action) {
  if(!IsMain()) return "ok=0\nerror=wrong_thread\n";
  if(action=="start") {
    if(!EnsureJob()) return "ok=0\nerror=capture_unavailable\n";
    job->formal=true; return "ok=1\nobservation_state=Preparing\n";
  }
  if(action=="stop") { if(job) { job->formal=false; if(!WorldCommandBusy()) job->stopping=true; } return "ok=1\nobservation_state=Stopping\n"; }
  if(action!="status") return "ok=0\nerror=unknown_observation_verb\n";
  if(!job) return "ok=1\nobservation_state=Off\n";
  auto &r=*job->recording;
  const bool ready=job->ready.load(std::memory_order_acquire);
  return "ok=1\nobservation_state="+std::string(job->stopping?"Stopping":job->attached?"Active":"Preparing")+
    "\nobservation_epoch="+std::to_string(r.epoch)+"\nformal_consumer="+std::to_string(job->formal)+
    "\nrecording_consumer="+std::to_string(ready&&r.recorder_consumer)+"\npublication_count="+std::to_string(ready?r.observation.Published():0)+
    "\nloss_sequence="+std::to_string(ready?r.observation.LossSequence():0)+"\nrecord_loss_sequence="+std::to_string(r.publications.LossSequence())+
    "\nlua_attempts="+std::to_string(ready?r.lua_attempts:0)+"\nlua_complete="+std::to_string(ready?r.lua_complete:0)+
    "\nlua_partial="+std::to_string(ready?r.lua_partial:0)+"\nentity_scope="+std::to_string(ready?r.latest_entity.context.scope:0)+
    "\nrun_scope="+std::to_string(ready?r.latest_continuity[3].context.scope:0)+"\nrun_coverage="+(ready?world::observation::CoverageName(r.latest_continuity[3].result.coverage):"NotScheduled")+"\nrandom_scope="+std::to_string(ready?r.latest_continuity[4].context.scope:0)+"\nrandom_coverage="+(ready?world::observation::CoverageName(r.latest_continuity[4].result.coverage):"NotScheduled")+"\npresentation_scope="+std::to_string(ready?r.latest_continuity[5].context.scope:0)+"\npresentation_coverage="+(ready?world::observation::CoverageName(r.latest_continuity[5].result.coverage):"NotScheduled")+"\nmod_scope="+std::to_string(ready?r.latest_continuity[6].context.scope:0)+"\nmod_coverage="+(ready?world::observation::CoverageName(r.latest_continuity[6].result.coverage):"NotScheduled")+"\nexploration_scope="+std::to_string(ready?r.latest_continuity[0].context.scope:0)+"\nexploration_coverage="+(ready?world::observation::CoverageName(r.latest_continuity[0].result.coverage):"NotScheduled")+"\nroom_save_scope="+std::to_string(ready?r.latest_continuity[1].context.scope:0)+"\nroom_save_coverage="+(ready?world::observation::CoverageName(r.latest_continuity[1].result.coverage):"NotScheduled")+"\nplayer_scope="+std::to_string(ready?r.latest_continuity[2].context.scope:0)+"\nplayer_coverage="+(ready?world::observation::CoverageName(r.latest_continuity[2].result.coverage):"NotScheduled")+"\nroom_scope="+std::to_string(ready?r.latest_room.context.scope:0)+"\nroom_coverage="+(ready?world::observation::CoverageName(r.latest_room.result.coverage):"NotScheduled")+"\nlevel_scope="+std::to_string(ready?r.latest_level.context.scope:0)+"\nlevel_coverage="+(ready?world::observation::CoverageName(r.latest_level.result.coverage):"NotScheduled")+"\ngrid_scope="+std::to_string(ready?r.latest_grid.context.scope:0)+"\ngrid_coverage="+(ready?world::observation::CoverageName(r.latest_grid.result.coverage):"NotScheduled")+"\nentity_coverage="+(ready?world::observation::CoverageName(r.latest_entity.result.coverage):"NotScheduled")+"\n";
}
bool WorldProcessExitPending() noexcept { return process_exit_pending; }
bool WorldProcessExitDecision() noexcept {
  if(!IsMain()||(!job&&!process_exit_pending)) return true;
  if(!process_exit_pending) {
    process_exit_pending=true; process_exit_started=GetTickCount64();
    if(job) { job->formal=false; if(job->consumer) StopWorldCommand(0,job->generation); job->stopping=true; }
    ClearPendingRunMutations(); game::StopSession(core::SessionCause::RunTeardown);
  }
  PumpWorldCommands(); const bool done=!job&&!game::Busy();
  const bool expired=GetTickCount64()-process_exit_started>=Settings.world_close_timeout_ms;
  if(!done&&!expired) return false;
  if(!process_exit_released) { process_exit_released=true; Trace("world_exit","event=release closed=%u timeout=%u",unsigned(done),unsigned(expired&&!done)); Trace("world_exit_status","%s",snapshot.c_str()); }
  return true;
}

#ifdef ISAACCS_E2E
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
#endif
} // namespace cs
