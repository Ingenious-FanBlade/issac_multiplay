#include "replica/stream_e2e.h"
#include "replica/stream_clock_e2e.h"
#include "replica/stream_resource_e2e.h"
#include "replica/stream_encoding_e2e.h"
#include "replica/stream_codec.h"
#include "replica/stream_materialize.h"
#include "replica/stream_capture.h"
#include <algorithm>
#include <atomic>
#include <deque>
#include <mutex>
#include <cstring>
#include <thread>
#include <set>
namespace cs::replica::stream {
namespace {
Value RangeValue(const RangeId &id,Allocator &a){auto versions=VersionValues({{id,1}},a);Value value;value.CopyFrom(versions[0],a);value.RemoveMember("revision");return value;}
void CopyField(Value &row,const char *name,const Value &source,Allocator &a){Value value;value.CopyFrom(source,a);Put(row,name,std::move(value),a);}
constexpr unsigned kEvents=65536,kSnapshots=16,kChunk=262144;
constexpr std::size_t kEventBytes=32u<<20;
constexpr std::uint64_t kBytes=64ull<<20,kTtlUs=10000000;
std::mutex mutex;
std::atomic<bool> enabled{false};
std::atomic<bool> encoding_saved{false};
std::atomic<bool> drop_next{false};
std::atomic<std::uint64_t> missed{0};
std::atomic<bool> visual_requested{false};
std::atomic<std::uint64_t> visual_until{0};
std::atomic<std::uint64_t> visual_neutral_until{0};
std::string wire_armed,wire_hit;
unsigned wire_hits=0;
// 紧凑不变正文避免每条Json各自保留默认64KiB分配块；分页在锁外解析、编码。
struct Row {std::uint64_t cursor=0;std::shared_ptr<const std::string> value;};
std::deque<Row> events;
std::size_t event_bytes=0;
std::uint64_t serial=0;
struct Saved {unsigned id=0;std::uint64_t at=0,connection=0,route=0;Range root;std::string text,render;std::vector<Version> commit_vector;std::uint64_t read_started=0,last_read=0;};
std::deque<Saved> snapshots;
unsigned next_id=0,kind_filter=0;
std::uint64_t revision_filter=0,armed_at=0,bytes=0;
std::string role_filter;
std::uint64_t arm_generation=0,source_selected_arm=0;
std::atomic<bool> source_snapshot_working{false};
void Add(Json row) noexcept {
 if(!enabled.load(std::memory_order_relaxed))return;
 if(drop_next.exchange(false)){++missed;return;}
 // 编码在锁外，实际保留量同时受记录数和32MiB正文约束，未读溢出仍明确计丢失。
 try{
  Put(row,"local_us",Decimal(MetricMicros(),row.GetAllocator()),row.GetAllocator());
  auto owned=std::make_shared<const std::string>(Text(row));const auto size=owned->size();
  if(size>kEventBytes){++missed;return;}
  std::lock_guard lock(mutex);
  while(!events.empty()&&(events.size()>=kEvents||event_bytes>kEventBytes-size)){
    event_bytes-=events.front().value->size();events.pop_front();++missed;
  }
  if(serial>=UINT64_MAX-1){++missed;return;}
  events.push_back({serial+1,std::move(owned)});++serial;event_bytes+=size;
 }catch(...){++missed;}
}
Json Base(const char *kind,const Metrics *m=nullptr){Json row(rapidjson::kObjectType);auto &a=row.GetAllocator();Put(row,"event",kind,a);if(m){Put(row,"owner",Decimal(std::uintptr_t(m),a),a);Put(row,"world",WorldValue(m->scope,a),a);Put(row,"route",Decimal(m->route,a),a);}return row;}
// 索引领取后给跨节点命令留出有界间隔；总读取寿命仍为120秒，数量和总字节上限不变。
void Expire(){const auto now=MetricMicros();while(!snapshots.empty()){const auto &s=snapshots.front();const bool reading=s.read_started&&now-s.read_started<=120000000&&now-s.last_read<=30000000;if((reading||now-s.at<=kTtlUs)&&snapshots.size()<=kSnapshots&&bytes<=kBytes)break;bytes-=s.text.size();snapshots.pop_front();}}
}
void E2EStreamProgress(const char *site,const std::array<std::uint64_t,8> &values) noexcept {
 if(!enabled)return;try{auto row=Base("stream-progress");auto &a=row.GetAllocator();Put(row,"site",site,a);
 constexpr const char *names[]={"revision","sequence","phase","prefix","started_ms","now_ms","native_target","age_ms"};
 for(unsigned i=0;i<values.size();++i)Put(row,names[i],Decimal(values[i],a),a);Add(std::move(row));}catch(...){++missed;}
}
void E2ETrace(bool on) noexcept {try{std::lock_guard lock(mutex);enabled=on;encoding_saved=false;events.clear();event_bytes=0;snapshots.clear();bytes=0;missed=0;role_filter.clear();armed_at=0;visual_requested=false;visual_until=0;visual_neutral_until=0;drop_next=false;}catch(...){++missed;}}
void E2EEncodingSource(const RangeData &range,const char *reason) noexcept {
 if(!enabled||encoding_saved.exchange(true))return;
 try{
  std::uint64_t total=0;
  for(unsigned record=0;record<range.records.size();++record)for(const auto &field:range.records[record].fields){
   const auto tree=std::get_if<std::shared_ptr<const Json>>(&field.value);if(!tree||!*tree)continue;
   const auto payload=Text(**tree);
   if(payload.size()>(4u<<20)-total){auto row=Base("encoding-source-truncated");Put(row,"reason","diagnostic_value_budget",row.GetAllocator());Add(std::move(row));return;}
   total+=payload.size();
   for(std::size_t offset=0;offset<payload.size();){
    auto end=(std::min)(offset+4096,payload.size());
    // JSON字符串按UTF-8边界分页，不能截断多字节字符形成无效诊断。
    while(end<payload.size()&&(std::uint8_t(payload[end])&0xc0)==0x80)--end;
    auto row=Base("encoding-source");auto &a=row.GetAllocator();
    Put(row,"range",RangeValue(range.id,a),a);Put(row,"revision",Decimal(range.revision,a),a);
    Put(row,"reason",std::string(reason),a);Put(row,"record",Value(record),a);Put(row,"field",Value(field.id),a);
    Put(row,"offset",Decimal(offset,a),a);Put(row,"total",Decimal(payload.size(),a),a);
    Put(row,"payload",payload.substr(offset,end-offset),a);Add(std::move(row));offset=end;
   }
  }
  auto complete=Base("encoding-source-complete");auto &allocator=complete.GetAllocator();
  Put(complete,"records",Value(unsigned(range.records.size())),allocator);Put(complete,"bytes",Decimal(total,allocator),allocator);Add(std::move(complete));
 }catch(...){++missed;}
}
void E2EObservationLoss() noexcept {drop_next=true;}
void E2EClockRejected(std::uint64_t echo,std::uint64_t now,std::uint64_t received,std::uint64_t sent,std::uint64_t sample) noexcept {
 if(!enabled)return;try{auto row=Base("clock-rejected");auto &a=row.GetAllocator();Put(row,"client_echo_ms",Decimal(echo,a),a);Put(row,"client_received_ms",Decimal(now,a),a);Put(row,"server_received_ms",Decimal(received,a),a);Put(row,"server_sent_ms",Decimal(sent,a),a);Put(row,"server_sample_ms",Decimal(sample,a),a);Add(std::move(row));}catch(...){++missed;}
}
void E2EResourceChange(std::uint32_t address,std::uint64_t generation,bool reading,bool retired,bool constructing,bool copying,const char *path,bool path_available,bool loaded,const char *operation) noexcept {
 if(!enabled)return;try{auto row=Base("resource-change");auto &a=row.GetAllocator();
 Put(row,"address",Decimal(address,a),a);Put(row,"generation",Decimal(generation,a),a);
 Put(row,"operation",std::string(operation),a);
 Put(row,"reading",Value(reading),a);Put(row,"retired",Value(retired),a);
 Put(row,"constructing",Value(constructing),a);Put(row,"copying",Value(copying),a);
 Put(row,"path_available",Value(path_available),a);if(path_available){Put(row,"path",std::string(path),a);Put(row,"loaded",Value(loaded),a);}
 Add(std::move(row));}catch(...){++missed;}
}
void E2ESourceLifecycle(const std::array<std::uint64_t,12> &values,const std::array<std::uint32_t,8> &arguments,unsigned count) noexcept {
 if(!enabled)return;try{auto row=Base("source-lifecycle");auto &a=row.GetAllocator();
 constexpr const char *names[]={"kind","reason","observation","local","use","birth","readiness","call","parent","entering","address","result"};
 for(unsigned i=0;i<values.size();++i)Put(row,names[i],Decimal(values[i],a),a);
 Value args(rapidjson::kArrayType);for(unsigned i=0;i<count&&i<arguments.size();++i)args.PushBack(arguments[i],a);
 Put(row,"arguments",std::move(args),a);Add(std::move(row));}catch(...){++missed;}
}
void E2EVisual(bool on) noexcept {visual_requested=on;visual_until=0;visual_neutral_until=on?MetricMicros()+120000000:0;}
bool E2EVisualHeld() noexcept {const auto until=visual_until.load();return until&&MetricMicros()<until;}
bool E2EVisualNeutral() noexcept {const auto until=visual_neutral_until.load();return until&&MetricMicros()<until;}
void E2EInput(const char *event,const InputWitness &w) noexcept {
 if(!enabled)return;try{const auto add=[&](const char *name,unsigned edge){auto row=Base(name);auto &a=row.GetAllocator();Put(row,"participant",Value(w.participant),a);Put(row,"body",Value(w.body),a);Put(row,"authority_kind",Value(w.authority_kind),a);Put(row,"edge",Value(edge),a);Put(row,"generation",Decimal(w.generation,a),a);Put(row,"sequence",Decimal(w.sequence,a),a);Put(row,"target_tick",Decimal(w.target,a),a);Put(row,"authority_tick",Decimal(w.tick,a),a);Value axes(rapidjson::kArrayType);for(auto axis:w.axes)axes.PushBack(Value(axis),a);Put(row,"axes",std::move(axes),a);Add(std::move(row));};add(event,w.edge);if(!std::strcmp(event,"physical-input")){add("input-target",0);for(unsigned bit=1;bit<=8;bit<<=1)if(w.edge&bit)add("edge-issued",bit);}if(!std::strcmp(event,"native-consumer"))for(unsigned bit=1;bit<=8;bit<<=1)if(w.edge&bit)add("edge-consumed",bit);}catch(...){++missed;}
}
void E2EAge(std::uint64_t authority,std::uint64_t source,std::uint64_t elapsed,std::uint64_t error,bool stale) noexcept {
 if(!enabled)return;try{auto row=Base("age");auto &a=row.GetAllocator();Put(row,"authority_tick",Decimal(authority,a),a);Put(row,"source_tick",Decimal(source,a),a);Put(row,"anchor_elapsed_ms",Decimal(elapsed,a),a);Put(row,"error_ms",Decimal(error,a),a);Put(row,"stale",Value(stale),a);Add(std::move(row));}catch(...){++missed;}
}
void E2EWireArm(const std::string &name){
 static constexpr const char *known[]={"wire","stream-schema","compact-schema","resource-declaration","session-identity","framing","capability-missing"};
 if(std::find(std::begin(known),std::end(known),name)==std::end(known))throw ContractError("e2e_wire_named_fault");
 std::lock_guard lock(mutex);wire_armed=name;wire_hit.clear();wire_hits=0;
}
std::string E2EWireStatus(){std::lock_guard lock(mutex);return "ok=1\narmed="+wire_armed+"\nhit="+wire_hit+"\nhits="+std::to_string(wire_hits)+"\n";}
// 仅供菜单容量负向的无世界握手探针；不会生成Ready/Input/Range或替代真实游戏Client。
std::string E2EHelloPackets(std::uint64_t nonce,const Value &caps){
 if(!nonce||nonce==UINT64_MAX)throw ContractError("e2e_hello_nonce");Message m;m.kind=Kind::Hello;m.sequence=1;m.body.SetObject();auto &a=m.body.GetAllocator();Value capabilities;capabilities.CopyFrom(caps,a);Put(m.body,"capabilities",std::move(capabilities),a);Put(m.body,"nonce",Decimal(nonce,a),a);Put(m.body,"role","Client",a);const auto packets=Fragment(m,nonce,HandshakeLimits());std::string out="ok=1\npurpose=negative_capacity_only_no_game_client\n";unsigned count=0;constexpr char h[]="0123456789abcdef";
 for(const auto &packet:packets){if(out.size()+packet.size()*2>60000)throw ContractError("e2e_hello_reply_budget");out+="packet."+std::to_string(count++)+"=";for(auto b:packet){out+=h[b>>4];out+=h[b&15];}out+='\n';}return out;
}
bool E2EWirePayload(const Message &m,Limits limits,std::vector<std::uint8_t> &payload){
 std::string fault;{std::lock_guard lock(mutex);if(wire_armed.empty()||m.kind!=(wire_armed=="session-identity"?Kind::Welcome:Kind::Hello))return false;fault=wire_armed;wire_hit=fault;wire_armed.clear();++wire_hits;}
 Message bad;bad.kind=m.kind;bad.sequence=m.sequence;bad.connection=m.connection;bad.route=m.route;bad.body=Copy(m.body);auto &a=bad.body.GetAllocator();
 if(fault=="session-identity")bad.body["client_nonce"]=Decimal(Unsigned(bad.body["client_nonce"])+1,a);
 else if(fault=="compact-schema"){payload=EncodePayload(m,limits);payload[0]=255;return true;}
 else {auto &caps=bad.body["capabilities"];if(fault=="wire")caps["protocol"].SetUint(999);else if(fault=="framing")caps["framing"].SetUint(999);else if(fault=="stream-schema")caps["stream_schema"].SetUint(999);else if(fault=="resource-declaration")caps["resources"]["base"].SetString("e2e-incompatible-resource",a);else if(fault=="capability-missing")caps["supported"].Clear();}
 payload=EncodePayload(bad,limits);return true;
}
void E2EIsolation(const char *kind,const char *entry,bool main,bool guarded,bool context,unsigned owner,const std::array<std::uint64_t,9> &identity) noexcept {
 if(!enabled)return;try{auto row=Base("isolation");auto &a=row.GetAllocator();Put(row,"kind",kind,a);Put(row,"entry",entry,a);Put(row,"main_thread",Value(main),a);Put(row,"persistence_blocked",Value(guarded),a);Put(row,"context_valid",Value(context),a);Put(row,"owner",Value(owner),a);constexpr const char *keys[]={"local_connection","connection","route","scene_generation","instance","resource","lua","context","phase"};for(unsigned i=0;i<identity.size();++i)Put(row,keys[i],Decimal(identity[i],a),a);Add(std::move(row));}catch(...){++missed;}
}
void E2EFlow(const Metrics *m,const RangeId &id,std::uint64_t revision,const Sample &s,FlowStage stage) noexcept {
 if(!enabled)return;try{auto row=Base("flow",m);auto &a=row.GetAllocator();Put(row,"range",RangeValue(id,a),a);Put(row,"revision",Decimal(revision,a),a);Put(row,"stage",Value(unsigned(stage)),a);Put(row,"sequence",Decimal(s.sequence,a),a);Put(row,"tick",Decimal(s.tick,a),a);Put(row,"native_tick",Decimal(s.last_native_update_tick,a),a);Put(row,"binding",Decimal(s.binding_revision,a),a);Put(row,"authority_kind",Value(unsigned(s.kind)),a);Add(std::move(row));}catch(...){++missed;}
}
void E2ETime(const Metrics *m,CpuStage stage,std::uint64_t wall,std::uint64_t cpu) noexcept {if(!enabled)return;try{auto row=Base("duration",m);auto &a=row.GetAllocator();Put(row,"stage",Value(unsigned(stage)),a);Put(row,"wall_us",Decimal(wall,a),a);if(cpu==UINT64_MAX)Put(row,"cpu_us",Value(),a);else Put(row,"cpu_us",Decimal(cpu,a),a);Add(std::move(row));}catch(...){++missed;}}
void E2EPacket(std::uint64_t connection,std::uint64_t route,std::uint64_t wire,const std::array<std::uint64_t,4> &lanes,const char *direction) noexcept {if(!enabled)return;try{auto row=Base("packet");auto &a=row.GetAllocator();Put(row,"connection",Decimal(connection,a),a);Put(row,"route",Decimal(route,a),a);Put(row,"wire_bytes",Decimal(wire,a),a);Put(row,"direction",direction,a);Value v(rapidjson::kArrayType);for(auto n:lanes)v.PushBack(Decimal(n,a),a);Put(row,"lanes",std::move(v),a);Add(std::move(row));}catch(...){++missed;}}
void E2EBody(std::uint64_t local,std::uint64_t use,std::uint64_t shape,std::uint64_t binding,std::uint64_t tick,unsigned stage) noexcept {if(!enabled)return;try{auto row=Base("body-witness");auto &a=row.GetAllocator();Put(row,"local",Decimal(local,a),a);Put(row,"use",Decimal(use,a),a);Put(row,"shape",Decimal(shape,a),a);Put(row,"binding",Decimal(binding,a),a);Put(row,"tick",Decimal(tick,a),a);Put(row,"stage",Value(stage),a);Add(std::move(row));}catch(...){++missed;}}
std::string E2ETracePage(std::uint64_t cursor){
 std::vector<Row> selected;selected.reserve(128);
 std::uint64_t first=0,available_tail=cursor,lost=0;bool active=false;
 // 单一读取者的下一次cursor确认上一页已取回；释放已确认项不算丢失。
 // 未读溢出仍由Add记录missed；重试同一cursor仍能得到原来未确认的页。
 {
  std::lock_guard lock(mutex);
  if(cursor>serial)throw ContractError("stream_trace_future_cursor");
  while(!events.empty()&&events.front().cursor<=cursor){event_bytes-=events.front().value->size();events.pop_front();}
  first=events.empty()?serial+1:events.front().cursor;
  available_tail=events.empty()?cursor:events.back().cursor;lost=missed.load();active=enabled.load();
  for(const auto &e:events){if(selected.size()==128)break;selected.push_back(e);}
 }
 Json out(rapidjson::kObjectType);auto &a=out.GetAllocator();Value rows(rapidjson::kArrayType);
 std::uint64_t next=cursor;std::size_t page_bytes=0;
 for(const auto &e:selected){
  const auto decoded=Parse(*e.value);Value row;row.CopyFrom(decoded,a);Put(row,"cursor",Decimal(e.cursor,a),a);
  const auto size=Text(row).size()+1;
  // 控制回复还要转义一层JSON，24KiB正文为64KiB控制口留出明确余量。
  if(rows.Size()>=128||page_bytes+size>24*1024){if(rows.Empty())throw ContractError("stream_trace_row_budget");break;}
  page_bytes+=size;rows.PushBack(std::move(row),a);next=e.cursor;
 }
 const bool more=available_tail>next;
 Put(out,"rows",std::move(rows),a);Put(out,"more",Value(more),a);Put(out,"next",Decimal(next,a),a);
 Put(out,"lost",Decimal(lost,a),a);Put(out,"first",Decimal(first,a),a);
 Put(out,"enabled",Value(active),a);return "ok=1\ndata="+Text(out)+"\n";
}
void E2EArm(const std::string &role,unsigned kind,std::uint64_t revision){if((role!="source"&&role!="native")||kind<1||kind>6)throw ContractError("stream_snapshot_filter");std::lock_guard lock(mutex);++arm_generation;role_filter=role;kind_filter=kind;revision_filter=revision;armed_at=MetricMicros();snapshots.clear();bytes=0;}
bool E2EWants(const char *role,const RangeData &r) noexcept {try{std::lock_guard lock(mutex);return enabled&&armed_at&&MetricMicros()-armed_at<=kTtlUs&&role_filter==role&&kind_filter==unsigned(r.id.kind)&&(!revision_filter||revision_filter==r.revision);}catch(...){++missed;return false;}}
void E2ESourceSelected(const Range &root,const std::vector<Range> &ranges,std::uint64_t connection,std::uint64_t route) noexcept {
 if(!root||!E2EWants("source",*root)||source_snapshot_working.exchange(true))return;
 try{
  std::uint64_t arm=0;
  {std::lock_guard lock(mutex);arm=arm_generation;if(source_selected_arm==arm){source_snapshot_working=false;return;}source_selected_arm=arm;}
  // 线程只持有不可变Range引用；既不访问Capture Store，也不接触Lua/游戏对象。
  std::thread([root,ranges,connection,route,arm]{
   try{
    std::vector<Range> closure;std::set<Version> seen;
    const auto visit=[&](const auto &self,const Range &r)->void{
     if(!seen.insert({r->id,r->revision}).second)return;
     if(seen.size()>128)throw ContractError("stream_snapshot_closure_capacity");
     const auto &dependencies=r->id.kind==RangeKind::Manifest?r->required:r->dependencies;
     for(const auto &d:dependencies){const auto i=std::find_if(ranges.begin(),ranges.end(),[&](const auto &v){return v->id==d.id&&v->revision==d.revision;});if(i==ranges.end())throw ContractError("stream_snapshot_selected_dependency");self(self,*i);}
     closure.push_back(r);
    };
    visit(visit,root);E2ESnapshot("source",root,closure,connection,route,{}, {},arm);
   }catch(...){++missed;}
   source_snapshot_working=false;
  }).detach();
 }catch(...){source_snapshot_working=false;++missed;}
}
void E2ESnapshot(const char *role,const Range &root,const std::vector<Range> &closure,std::uint64_t connection,std::uint64_t route,const std::string &native,const std::vector<Version> &commit_vector,std::uint64_t arm) noexcept {
 if(!root||!E2EWants(role,*root))return;
 try{Json row(rapidjson::kObjectType);auto &a=row.GetAllocator();Put(row,"role",role,a);Put(row,"connection",Decimal(connection,a),a);Put(row,"route",Decimal(route,a),a);Put(row,"world",WorldValue(root->world,a),a);CopyField(row,"root",RangeMessage(*root,1,1).body,a);Value ranges(rapidjson::kArrayType);std::uint64_t upper=0;Store values(root->world);for(const auto &r:closure){upper+=Footprint(*r);if(upper>kBytes/4)throw ContractError("stream_snapshot_budget");const auto admitted=values.Admit(r,CaptureLayouts());if(!admitted&&admitted.status!=Admission::WaitingDependencies)throw ContractError(admitted.reason);Value v;v.CopyFrom(RangeMessage(*r,1,1).body,a);ranges.PushBack(std::move(v),a);}Put(row,"closure",std::move(ranges),a);Put(row,"native",native,a);
  // 指定版本的来源值供独立比较；只有纯值，绝不回填到native栏目。
  if(root->id.kind==RangeKind::Manifest||root->id.kind==RangeKind::WorldDynamic){CopyField(row,"entity",DomainValue(values,*root,"entity"),a);CopyField(row,"grid",DomainValue(values,*root,"grid"),a);}
  else if(root->id.kind==RangeKind::PlayerFast){Value members(rapidjson::kArrayType);const auto &tree=std::get<std::shared_ptr<const Json>>(root->records.front().fields.front().value);Value v;v.CopyFrom(*tree,a);Value key(rapidjson::kObjectType);Put(key,"run",Decimal(root->id.body.run,a),a);Put(key,"id",Decimal(root->id.body.id,a),a);Put(key,"space",Value(root->id.body.space),a);Put(v,"key",std::move(key),a);members.PushBack(std::move(v),a);Put(row,"fast_bodies",std::move(members),a);}
  Put(row,"commit_vector",VersionValues(commit_vector,a),a);
  auto text=Text(row);if(text.size()>kBytes/2)throw ContractError("stream_snapshot_budget");std::lock_guard lock(mutex);
  // 旧worker不能消耗新一次arm，也不能在Trace关闭后重新发布快照。
  if(!enabled||(arm&&arm!=arm_generation)||!armed_at||role_filter!=role||kind_filter!=unsigned(root->id.kind)||(revision_filter&&revision_filter!=root->revision))return;
  Expire();while(!snapshots.empty()&&(snapshots.size()>=kSnapshots||bytes+text.size()>kBytes)){bytes-=snapshots.front().text.size();snapshots.pop_front();++missed;}bytes+=text.size();snapshots.push_back({++next_id,MetricMicros(),connection,route,root,std::move(text),{},commit_vector});if(std::strcmp(role,"native")==0&&visual_requested.exchange(false))visual_until=MetricMicros()+120000000;armed_at=0;role_filter.clear();
 }catch(const std::exception &e){
  // 快照组合失败必须保留真实原因；缺快照仍失败，不以诊断替代原生回读。
  ++missed;try{auto failure=Base("snapshot-failure");auto &a=failure.GetAllocator();Put(failure,"role",role,a);Put(failure,"range",RangeValue(root->id,a),a);Put(failure,"revision",Decimal(root->revision,a),a);Put(failure,"reason",std::string(e.what()),a);Add(std::move(failure));}catch(...){}
 }catch(...){++missed;}
}
void E2ERender(std::uint64_t connection,std::uint64_t route,const std::vector<Version> &versions,std::uint64_t revision) noexcept {try{std::lock_guard lock(mutex);Expire();for(auto &s:snapshots)if(s.connection==connection&&s.route==route&&s.render.empty()&&!s.commit_vector.empty()&&s.commit_vector==versions&&std::find(versions.begin(),versions.end(),Version{s.root->id,s.root->revision})!=versions.end())s.render="render_local_us="+std::to_string(MetricMicros())+";render_revision="+std::to_string(revision);}catch(...){++missed;}}
std::string E2ESnapshotPage(unsigned slot,unsigned page){std::lock_guard lock(mutex);Expire();if(!slot){Json rows(rapidjson::kArrayType);for(auto &s:snapshots){s.last_read=MetricMicros();if(!s.read_started)s.read_started=s.last_read;Value v(rapidjson::kObjectType);auto &a=rows.GetAllocator();Put(v,"id",Value(s.id),a);Put(v,"revision",Decimal(s.root->revision,a),a);Put(v,"range",RangeValue(s.root->id,a),a);Put(v,"bytes",Value(unsigned(s.text.size())),a);Put(v,"render",s.render,a);rows.PushBack(std::move(v),a);}return "ok=1\ndata="+Text(rows)+"\n";}
 for(auto &s:snapshots)if(s.id==slot){const auto offset=std::uint64_t(page)*kChunk;if(offset>=s.text.size())throw ContractError("stream_snapshot_page");s.last_read=MetricMicros();if(!s.read_started)s.read_started=s.last_read;const auto count=(std::min)(std::uint64_t(kChunk),s.text.size()-offset);std::string part;part.reserve(std::size_t(count)*2);constexpr char hex[]="0123456789abcdef";for(std::uint64_t i=0;i<count;++i){const auto byte=static_cast<unsigned char>(s.text[std::size_t(offset+i)]);part+=hex[byte>>4];part+=hex[byte&15];}return "ok=1\nbytes="+std::to_string(s.text.size())+"\nencoding=hex-utf8-bytes\nrender="+s.render+"\ndata="+part+"\n";}return "ok=0\nerror=stream_snapshot_expired_or_missing\n";
}
}
