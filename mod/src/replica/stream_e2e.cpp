#include "replica/stream_e2e.h"
#include "replica/stream_codec.h"
#include "replica/stream_materialize.h"
#include "replica/stream_capture.h"
#include <algorithm>
#include <atomic>
#include <deque>
#include <mutex>
#include <cstring>
namespace cs::replica::stream {
namespace {
Value RangeValue(const RangeId &id,Allocator &a){auto versions=VersionValues({{id,1}},a);Value value;value.CopyFrom(versions[0],a);value.RemoveMember("revision");return value;}
void CopyField(Value &row,const char *name,const Value &source,Allocator &a){Value value;value.CopyFrom(source,a);Put(row,name,std::move(value),a);}
constexpr unsigned kEvents=16384,kSnapshots=16,kChunk=8192;
constexpr std::uint64_t kBytes=64ull<<20,kTtlUs=10000000;
std::mutex mutex;
std::atomic<bool> enabled{false};
std::atomic<bool> drop_next{false};
std::atomic<std::uint64_t> missed{0};
std::atomic<bool> visual_requested{false};
std::atomic<std::uint64_t> visual_until{0};
std::atomic<std::uint64_t> visual_neutral_until{0};
std::string wire_armed,wire_hit;
unsigned wire_hits=0;
struct Row {std::uint64_t cursor=0;Json value;};
std::deque<Row> events;
std::uint64_t serial=0;
struct Saved {unsigned id=0;std::uint64_t at=0,connection=0,route=0;Range root;std::string text,render;std::vector<Version> commit_vector;std::uint64_t read_started=0,last_read=0;};
std::deque<Saved> snapshots;
unsigned next_id=0,kind_filter=0;
std::uint64_t revision_filter=0,armed_at=0,bytes=0;
std::string role_filter;
void Add(Json row) noexcept {
 if(!enabled.load(std::memory_order_relaxed))return;
 if(drop_next.exchange(false)){++missed;return;}
 try{std::unique_lock lock(mutex,std::try_to_lock);if(!lock){++missed;return;}if(events.size()==kEvents){events.pop_front();++missed;}
 Put(row,"local_us",Decimal(MetricMicros(),row.GetAllocator()),row.GetAllocator());events.push_back({++serial,std::move(row)});}catch(...){++missed;}
}
Json Base(const char *kind,const Metrics *m=nullptr){Json row(rapidjson::kObjectType);auto &a=row.GetAllocator();Put(row,"event",kind,a);if(m){Put(row,"owner",Decimal(std::uintptr_t(m),a),a);Put(row,"world",WorldValue(m->scope,a),a);Put(row,"route",Decimal(m->route,a),a);}return row;}
void Expire(){const auto now=MetricMicros();while(!snapshots.empty()){const auto &s=snapshots.front();const bool reading=s.read_started&&now-s.read_started<=120000000&&now-s.last_read<=2000000;if((reading||now-s.at<=kTtlUs)&&snapshots.size()<=kSnapshots&&bytes<=kBytes)break;bytes-=s.text.size();snapshots.pop_front();}}
}
void E2ETrace(bool on) noexcept {try{std::lock_guard lock(mutex);enabled=on;events.clear();snapshots.clear();bytes=0;missed=0;role_filter.clear();armed_at=0;visual_requested=false;visual_until=0;visual_neutral_until=0;drop_next=false;}catch(...){++missed;}}
void E2EObservationLoss() noexcept {drop_next=true;}
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
std::string E2EHelloPackets(std::uint64_t nonce){
 if(!nonce||nonce==UINT64_MAX)throw ContractError("e2e_hello_nonce");Message m;m.kind=Kind::Hello;m.sequence=1;m.body.SetObject();auto &a=m.body.GetAllocator();auto caps=Capabilities();Value capabilities;capabilities.CopyFrom(caps,a);Put(m.body,"capabilities",std::move(capabilities),a);Put(m.body,"nonce",Decimal(nonce,a),a);Put(m.body,"role","Client",a);const auto packets=Fragment(m,nonce,HandshakeLimits());std::string out="ok=1\npurpose=negative_capacity_only_no_game_client\n";unsigned count=0;constexpr char h[]="0123456789abcdef";
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
std::string E2ETracePage(std::uint64_t cursor){std::lock_guard lock(mutex);Json out(rapidjson::kObjectType);auto &a=out.GetAllocator();Value rows(rapidjson::kArrayType);std::uint64_t next=cursor;for(const auto &e:events)if(e.cursor>cursor&&rows.Size()<32){Value row;row.CopyFrom(e.value,a);Put(row,"cursor",Decimal(e.cursor,a),a);rows.PushBack(std::move(row),a);next=e.cursor;}Put(out,"rows",std::move(rows),a);Put(out,"next",Decimal(next,a),a);Put(out,"lost",Decimal(missed.load(),a),a);Put(out,"first",Decimal(events.empty()?serial+1:events.front().cursor,a),a);Put(out,"enabled",Value(enabled.load()),a);return "ok=1\ndata="+Text(out)+"\n";}
void E2EArm(const std::string &role,unsigned kind,std::uint64_t revision){if((role!="source"&&role!="native")||kind<1||kind>6)throw ContractError("stream_snapshot_filter");std::lock_guard lock(mutex);role_filter=role;kind_filter=kind;revision_filter=revision;armed_at=MetricMicros();snapshots.clear();bytes=0;}
bool E2EWants(const char *role,const RangeData &r) noexcept {try{std::lock_guard lock(mutex);return enabled&&armed_at&&MetricMicros()-armed_at<=kTtlUs&&role_filter==role&&kind_filter==unsigned(r.id.kind)&&(!revision_filter||revision_filter==r.revision);}catch(...){++missed;return false;}}
void E2ESnapshot(const char *role,const Range &root,const std::vector<Range> &closure,std::uint64_t connection,std::uint64_t route,const std::string &native,const std::vector<Version> &commit_vector) noexcept {
 if(!root||!E2EWants(role,*root))return;
 try{Json row(rapidjson::kObjectType);auto &a=row.GetAllocator();Put(row,"role",role,a);Put(row,"connection",Decimal(connection,a),a);Put(row,"route",Decimal(route,a),a);Put(row,"world",WorldValue(root->world,a),a);CopyField(row,"root",RangeMessage(*root,1,1).body,a);Value ranges(rapidjson::kArrayType);std::uint64_t upper=0;Store values(root->world);for(const auto &r:closure){upper+=Footprint(*r);if(upper>kBytes/4)throw ContractError("stream_snapshot_budget");const auto admitted=values.Admit(r,CaptureLayouts());if(!admitted&&admitted.status!=Admission::WaitingDependencies)throw ContractError(admitted.reason);Value v;v.CopyFrom(RangeMessage(*r,1,1).body,a);ranges.PushBack(std::move(v),a);}Put(row,"closure",std::move(ranges),a);Put(row,"native",native,a);
  // 指定版本的来源值供独立比较；只有纯值，绝不回填到native栏目。
  if(root->id.kind==RangeKind::Manifest||root->id.kind==RangeKind::WorldDynamic){CopyField(row,"entity",DomainValue(values,*root,"entity"),a);CopyField(row,"grid",DomainValue(values,*root,"grid"),a);}
  else if(root->id.kind==RangeKind::PlayerFast){Value members(rapidjson::kArrayType);const auto &tree=std::get<std::shared_ptr<const Json>>(root->records.front().fields.front().value);Value v;v.CopyFrom(*tree,a);Value key(rapidjson::kObjectType);Put(key,"run",Decimal(root->id.body.run,a),a);Put(key,"id",Decimal(root->id.body.id,a),a);Put(key,"space",Value(root->id.body.space),a);Put(v,"key",std::move(key),a);members.PushBack(std::move(v),a);Put(row,"fast_bodies",std::move(members),a);}
  Put(row,"commit_vector",VersionValues(commit_vector,a),a);
  auto text=Text(row);if(text.size()>kBytes/2)throw ContractError("stream_snapshot_budget");std::lock_guard lock(mutex);Expire();while(!snapshots.empty()&&(snapshots.size()>=kSnapshots||bytes+text.size()>kBytes)){bytes-=snapshots.front().text.size();snapshots.pop_front();++missed;}bytes+=text.size();snapshots.push_back({++next_id,MetricMicros(),connection,route,root,std::move(text),{},commit_vector});if(std::strcmp(role,"native")==0&&visual_requested.exchange(false))visual_until=MetricMicros()+120000000;armed_at=0;role_filter.clear();
 }catch(...){++missed;}
}
void E2ERender(std::uint64_t connection,std::uint64_t route,const std::vector<Version> &versions,std::uint64_t revision) noexcept {try{std::lock_guard lock(mutex);Expire();for(auto &s:snapshots)if(s.connection==connection&&s.route==route&&s.render.empty()&&!s.commit_vector.empty()&&s.commit_vector==versions&&std::find(versions.begin(),versions.end(),Version{s.root->id,s.root->revision})!=versions.end())s.render="render_local_us="+std::to_string(MetricMicros())+";render_revision="+std::to_string(revision);}catch(...){++missed;}}
std::string E2ESnapshotPage(unsigned slot,unsigned page){std::lock_guard lock(mutex);Expire();if(!slot){Json rows(rapidjson::kArrayType);for(const auto &s:snapshots){Value v(rapidjson::kObjectType);auto &a=rows.GetAllocator();Put(v,"id",Value(s.id),a);Put(v,"revision",Decimal(s.root->revision,a),a);Put(v,"range",RangeValue(s.root->id,a),a);Put(v,"bytes",Value(unsigned(s.text.size())),a);Put(v,"render",s.render,a);rows.PushBack(std::move(v),a);}return "ok=1\ndata="+Text(rows)+"\n";}
 for(auto &s:snapshots)if(s.id==slot){const auto offset=std::uint64_t(page)*kChunk;if(offset>=s.text.size())throw ContractError("stream_snapshot_page");s.last_read=MetricMicros();if(!s.read_started)s.read_started=s.last_read;const auto count=(std::min)(std::uint64_t(kChunk),s.text.size()-offset);std::string part;part.reserve(std::size_t(count)*2);constexpr char hex[]="0123456789abcdef";for(std::uint64_t i=0;i<count;++i){const auto byte=static_cast<unsigned char>(s.text[std::size_t(offset+i)]);part+=hex[byte>>4];part+=hex[byte&15];}return "ok=1\nbytes="+std::to_string(s.text.size())+"\nencoding=hex-utf8-bytes\nrender="+s.render+"\ndata="+part+"\n";}return "ok=0\nerror=stream_snapshot_expired_or_missing\n";
}
}
