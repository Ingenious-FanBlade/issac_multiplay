#include "state/client_memory.h"
#include <atomic>
#include <algorithm>
#include <bit>
#include <cstring>
#include <iterator>

namespace cs::state {
namespace {std::atomic_flag client_workspace=ATOMIC_FLAG_INIT;}
#ifdef ISAACCS_E2E
namespace {
constexpr const char *recipe_misses[]={"fresh_member","no_index_or_base","instance","container","structure_or_dependency",
    "block_value","revoked","no_recipe"};
constexpr const char *skip_misses[]={"joint_or_baseline","no_display","boss","event","event_sfx","event_item_banner","event_fortune","display_state","display_roots",
    "display_transition","display_music","display_graphics","display_poop_fx","display_hud","display_key",
    "callback_world","callback_members","callback_procedural","callback_geometry","callback_entity_init",
    "callback_grid_init","callback_overlay","native_member_sets","no_eligible_operation","callback_write_or_snapshot"};
}
void ClientApplyMetrics::CadenceReason(bool recipe,const char *reason) noexcept {
  if(!reason)return;
  const auto count=[&](const auto &names,auto &values){
    static_assert(std::size(names)==std::tuple_size_v<std::remove_reference_t<decltype(values)>>);
    for(unsigned i=0;i<std::size(names);++i)if(std::strcmp(names[i],reason)==0){values[i].fetch_add(1,std::memory_order_relaxed);break;}
  };
  if(recipe)count(recipe_misses,recipe_misses_);else count(skip_misses,skip_misses_);
}
void ClientApplyMetrics::FirstD1(Issue detail) noexcept {
  if(!issue_mutex_.try_lock())return;
  if(!first_d1_.reason[0]){
    std::memcpy(detail.reason.data(),"captured_value_differs",sizeof("captured_value_differs"));
    first_d1_=detail;
  }
  issue_mutex_.unlock();
}
#endif
void ClientApplyMetrics::Mark(std::uint8_t streams,ClientApplyStage step,std::uint64_t elapsed) noexcept {
#ifdef ISAACCS_E2E
  if(step==ClientApplyStage::Manager&&elapsed>8000)Wait(streams,ClientApplyWait::ManagerOver8ms);
#endif
  const auto now=Now();
  for(unsigned i=0;i<2;++i)if(streams&(1u<<i)){
    auto &s=streams_[i].stages[unsigned(step)];
    s.count.fetch_add(1,std::memory_order_relaxed);s.total.fetch_add(elapsed,std::memory_order_relaxed);
    const auto bucket=(std::min)(std::size_t(31),std::size_t(std::bit_width(elapsed)));
    s.buckets[bucket].fetch_add(1,std::memory_order_relaxed);
    if(elapsed>2000)s.over_2ms.fetch_add(1,std::memory_order_relaxed);
    if(elapsed>4000)s.over_4ms.fetch_add(1,std::memory_order_relaxed);
    auto maximum=s.maximum.load(std::memory_order_relaxed);
    while(maximum<elapsed&&!s.maximum.compare_exchange_weak(maximum,elapsed,std::memory_order_relaxed)){}
    const auto last=s.last.exchange(now,std::memory_order_relaxed);
    if(last&&now>=last)s.interval.store(now-last,std::memory_order_relaxed);
  }
}
void ClientApplyMetrics::Wait(std::uint8_t streams,ClientApplyWait wait) noexcept {
  for(unsigned i=0;i<2;++i)if(streams&(1u<<i))streams_[i].waits[unsigned(wait)].fetch_add(1,std::memory_order_relaxed);
}
void ClientApplyMetrics::Volume(std::uint8_t streams,ClientApplyVolume volume,std::uint64_t n) noexcept {
  for(unsigned i=0;i<2;++i)if(streams&(1u<<i))streams_[i].volumes[unsigned(volume)].fetch_add(n,std::memory_order_relaxed);
}
void ClientApplyMetrics::FirstIssue(std::uint8_t streams,Issue issue,const char *phase,const char *reason,bool display) noexcept {
  if(!issue_mutex_.try_lock())return;
  const auto copy=[](auto &dst,const char *src){
    if(src)std::memcpy(dst.data(),src,(std::min)(dst.size()-1,std::strlen(src)));
  };
  copy(issue.phase,phase);copy(issue.reason,reason);
  if(display){if(!issues_[2].reason[0])issues_[2]=issue;}
  else for(unsigned i=0;i<2;++i)if((streams&(1u<<i))&&!issues_[i].reason[0])issues_[i]=issue;
  issue_mutex_.unlock();
}
void ClientApplyMetrics::LastWait(std::uint8_t streams,Issue issue,const char *phase,const char *reason) noexcept {
  if(!issue_mutex_.try_lock())return;
  if(phase)std::memcpy(issue.phase.data(),phase,(std::min)(issue.phase.size()-1,std::strlen(phase)));
  if(reason)std::memcpy(issue.reason.data(),reason,(std::min)(issue.reason.size()-1,std::strlen(reason)));
  for(unsigned i=0;i<2;++i)if(streams&(1u<<i))last_wait_[i]=issue;
  issue_mutex_.unlock();
}
void ClientApplyMetrics::Holds(std::uint64_t published,std::uint64_t pending,std::uint64_t retired) noexcept {
  holds_[0].store(published,std::memory_order_relaxed);holds_[1].store(pending,std::memory_order_relaxed);
  holds_[2].store(retired,std::memory_order_relaxed);
}
void ClientApplyMetrics::BackgroundHolds(std::uint64_t waiting,std::uint64_t in_flight,std::uint64_t retired) noexcept {
  holds_[3].store(waiting,std::memory_order_relaxed);holds_[4].store(in_flight,std::memory_order_relaxed);
  holds_[5].store(retired,std::memory_order_relaxed);
}
void ClientApplyMetrics::SetBlockSample(const std::array<std::uint64_t,3> &bytes)noexcept {
  for(unsigned i=0;i<3;++i)block_bytes_[i].store(bytes[i],std::memory_order_relaxed);
  sample_time_.store(Now(),std::memory_order_release);
}
std::array<ClientApplyMetrics::InstallTiming,2> ClientApplyMetrics::ReadInstallTiming() const noexcept {
  std::array<InstallTiming,2> timing{};
  for(unsigned i=0;i<timing.size();++i){
    const auto &stage=streams_[i].stages[unsigned(ClientApplyStage::Install)];
    timing[i]={stage.count.load(std::memory_order_relaxed),stage.total.load(std::memory_order_relaxed)};
  }
  return timing;
}
std::string ClientApplyMetrics::Fields(unsigned page)const {
  if(page>1)return {};
  constexpr const char *stages[]={"receive","decode_slice","validation_slice","plan_slice","describe_slice","admit","main_prepare",
    "install","readback","publish","feedback","accept","ack","retire","end_to_end","manager",
    "main_objects","main_fields","atomic_describe","native_write","native_publish","main_queue",
    "atomic_inputs","atomic_index","atomic_fields","atomic_leaves","atomic_order","atomic_ranges",
    "native_initialize","native_owners","native_resolve","native_finish",
    "plan_import","plan_walk","plan_paths","plan_counts","plan_ranges","plan_index","plan_rules","plan_commands",
    "decode_ring_base","decode_normalize","background_gap","world_busy_packet","fast_check","fast_write"};
  constexpr const char *waits[]={"own_stream","joint","dependency","resources","budget","expired","stale","mailbox","cancelled","old_base","duplicate",
    "body_queued","body_replaced","body_queue_expired","body_rebased","resource_yield_frame","table_wait_expired",
    "world_queue_replaced","world_queue_expired","cpu_queue_borrow","manager_over_8000_us"};
  constexpr const char *volumes[]={"value_blocks","structure_blocks","dependency_blocks","reused_blocks","drift_blocks","checks","readback_bytes","built_containers","retained_containers",
    "raw_operations","native_operations","late_operations","delegated_operations",
    "read_page_queries","read_page_full","write_page_queries","write_page_full","single_frames","joint_frames",
    "pure_write_bytes","publish_readback_bytes","unchanged_world_bytes",
    "plan_reused_records","native_reused_order","world_ring_base_decoded",
    "plan_reused_world_blocks",
    "world_write_operations",
    "write_operations","write_segments",
    "plan_shared_records",
    "pool_members_deferred",
    "quiet_records",
    "fast_path_hits","fast_path_misses","absorbed_records","store_records","fast_write_leaves","mod_records_rejected"};
  constexpr const char *holds[]={"published","pending","retired_native","waiting","in_flight","retired_values"};
  static_assert(std::size(stages)==unsigned(ClientApplyStage::Count)&&std::size(waits)==unsigned(ClientApplyWait::Count)&&
                std::size(volumes)==unsigned(ClientApplyVolume::Count));
  std::string out;
  const auto add=[&](std::string key,std::uint64_t value){out+=key+"="+std::to_string(value)+"\n";};
  const auto prefix=std::string("stream.binary.")+(page?"body.":"world.");
  const auto &stream=streams_[page];
  for(unsigned i=0;i<std::size(stages);++i){
    const auto &s=stream.stages[i];const auto key=prefix+stages[i];
    const auto count=s.count.load(std::memory_order_relaxed);
    const auto percentile=[&](unsigned percent){
      const auto rank=(count*percent+99)/100;std::uint64_t seen=0;
      for(unsigned j=0;j<s.buckets.size();++j){
        seen+=s.buckets[j].load(std::memory_order_relaxed);
        if(seen>=rank)return (std::uint64_t(1)<<j)-1;
      }
      return s.maximum.load(std::memory_order_relaxed);
    };
    add(key+".n",count);add(key+".sum_us",s.total.load(std::memory_order_relaxed));
    add(key+".max_us",s.maximum.load(std::memory_order_relaxed));add(key+".last_interval_us",s.interval.load(std::memory_order_relaxed));
    if(i==unsigned(ClientApplyStage::Install)||i==unsigned(ClientApplyStage::Manager)||i==unsigned(ClientApplyStage::EndToEnd)){
      add(key+".p95_bucket_us",percentile(95));add(key+".p99_bucket_us",percentile(99));
      add(key+".over_2000_us",s.over_2ms.load(std::memory_order_relaxed));
      add(key+".over_4000_us",s.over_4ms.load(std::memory_order_relaxed));
    }
  }
  for(unsigned i=0;i<std::size(waits);++i)add(prefix+"wait."+waits[i],stream.waits[i].load(std::memory_order_relaxed));
  for(unsigned i=0;i<std::size(volumes);++i)add(prefix+volumes[i],stream.volumes[i].load(std::memory_order_relaxed));
#ifdef ISAACCS_E2E
  if(!page){
    for(unsigned i=0;i<std::size(recipe_misses);++i)add(prefix+"recipe_miss."+recipe_misses[i],recipe_misses_[i].load(std::memory_order_relaxed));
    for(unsigned i=0;i<std::size(skip_misses);++i)add(prefix+"skip_miss."+skip_misses[i],skip_misses_[i].load(std::memory_order_relaxed));
  }
#endif
  for(unsigned i=0;i<std::size(holds);++i)add(std::string("stream.binary.holds.")+holds[i],holds_[i].load(std::memory_order_relaxed));
  const auto sample=sample_time_.load(std::memory_order_acquire);
  add("stream.binary.shadow_blocks.sample_us",sample);
  if(sample){
    add("stream.binary.shadow_blocks.accepted_bytes",block_bytes_[0].load(std::memory_order_relaxed));
    add("stream.binary.shadow_blocks.candidate_bytes",block_bytes_[1].load(std::memory_order_relaxed));
    add("stream.binary.shadow_blocks.retired_bytes",block_bytes_[2].load(std::memory_order_relaxed));
  }
  std::lock_guard lock(issue_mutex_);
  const auto issue=[&](const Issue &v,const std::string &key){
    if(!v.reason[0])return;
    out+=key+".phase="+v.phase.data()+"\n"+key+".reason="+v.reason.data()+"\n";
    add(key+".route",v.route);add(key+".ticket",v.ticket);add(key+".member",v.member);add(key+".instance",v.instance);
    add(key+".layout",v.layout);add(key+".field",v.field);add(key+".block",v.block);
    add(key+".target_version",v.target_version);
    add(key+".resource_epoch",v.resource_epoch);add(key+".resource_key",v.resource_key);
    add(key+".expected",v.expected);add(key+".actual",v.actual);
  };
  issue(issues_[page],prefix+"first");issue(issues_[2],"stream.binary.display.first");
  issue(last_wait_[page],prefix+"last_wait");
#ifdef ISAACCS_E2E
  if(!page)issue(first_d1_,prefix+"first_d1");
#endif
  return out;
}
ClientStateWorkspace::ClientStateWorkspace() {
  if(client_workspace.test_and_set(std::memory_order_acquire))throw ClientTableError(ClientTableFailure::WorkspaceBusy,"client_state_workspace_busy");
}
ClientStateWorkspace::~ClientStateWorkspace() {client_workspace.clear(std::memory_order_release);}
void ClientStateWorkspace::RequireCurrentThread() const {
  if(thread_!=std::this_thread::get_id())throw ClientTableError(ClientTableFailure::WrongThread,"client_state_workspace_wrong_thread");
}
ClientStateMemory::ClientStateMemory() {
  const auto &policy=StateBudgets();limits_={policy.peer_value_bytes,policy.peer_wire_bytes,policy.workspace_job_bytes};
  // 账户及三份既有Charge的管理对象冷分配保守预留；容器及其控制块另外按实际分配收费。
  charges_={memory::Reserve(memory::Pool::Values,sizeof(*this)+1024,"client_state_values"),
      memory::Reserve(memory::Pool::Wire,0,"client_state_wire"),memory::Reserve(memory::Pool::Workspace,0,"client_state_workspace")};
}
std::shared_ptr<ClientStateMemory> ClientStateMemory::Create() {
  try {return std::shared_ptr<ClientStateMemory>(new ClientStateMemory);}
  catch(const std::bad_alloc &) {throw ClientTableError(ClientTableFailure::Budget,"client_memory_allocation");}
  catch(const std::runtime_error &) {throw ClientTableError(ClientTableFailure::Budget,"client_memory_pool");}
}
void *ClientStateMemory::Allocate(memory::Pool pool,std::size_t bytes,std::size_t alignment) {
  std::lock_guard lock(mutex_);const auto index=unsigned(pool);
  if(index>=charges_.size() || !alignment || (alignment&(alignment-1)))
    throw ClientTableError(ClientTableFailure::Internal,"client_allocation_arguments");
  auto &charge=*charges_[index];const auto before=charge.Bytes();
  const auto available=before<=limits_[index]?limits_[index]-before:0;
  if(before>limits_[index] || bytes>available)throw ClientTableError(ClientTableFailure::Budget,"client_peer_quota",bytes,available,pool);
  try {charge.Resize(before+bytes);}
  catch(const std::runtime_error &) {const auto shared=memory::ReplicationBudget()->Snapshot()[index];
    throw ClientTableError(ClientTableFailure::Budget,"client_shared_pool",bytes,shared.limit-shared.current,pool);}
  try {
    if(alignment>__STDCPP_DEFAULT_NEW_ALIGNMENT__)return ::operator new(bytes,std::align_val_t(alignment));
    return ::operator new(bytes);
  } catch(const std::bad_alloc &) {
    charge.Resize(before);throw ClientTableError(ClientTableFailure::Budget,"client_native_allocation",bytes,available,pool);
  }
}
void ClientStateMemory::Deallocate(memory::Pool pool,void *value,std::size_t bytes,std::size_t alignment) noexcept {
  if(alignment>__STDCPP_DEFAULT_NEW_ALIGNMENT__)::operator delete(value,std::align_val_t(alignment));else ::operator delete(value);
  std::lock_guard lock(mutex_);auto &charge=*charges_[unsigned(pool)];charge.Resize(charge.Bytes()-bytes);
}
std::array<std::uint64_t,3> ClientStateMemory::Usage() const noexcept {
  std::lock_guard lock(mutex_);return {charges_[0]->Bytes(),charges_[1]->Bytes(),charges_[2]->Bytes()};
}
void ClientStateMemory::ChargeNative(std::size_t bytes) {
  std::lock_guard lock(mutex_);auto &charge=*charges_[unsigned(memory::Pool::Values)];const auto before=charge.Bytes();
  const auto available=before<=limits_[0]?limits_[0]-before:0;
  if(before>limits_[0]||bytes>available)throw ClientTableError(ClientTableFailure::Budget,"client_native_peer_quota",bytes,available);
  try {charge.Resize(before+bytes);}catch(const std::runtime_error &){throw ClientTableError(ClientTableFailure::Budget,"client_native_shared_pool",bytes,available);}
}
void ClientStateMemory::UnchargeNative(std::size_t bytes) noexcept {
  std::lock_guard lock(mutex_);auto &charge=*charges_[unsigned(memory::Pool::Values)];charge.Resize(charge.Bytes()-bytes);
}
} // namespace cs::state
