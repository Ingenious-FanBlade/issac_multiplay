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
std::atomic<std::uint64_t> missed{0};
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
 try{std::unique_lock lock(mutex,std::try_to_lock);if(!lock){++missed;return;}if(events.size()==kEvents){events.pop_front();++missed;}
 Put(row,"local_us",Decimal(MetricMicros(),row.GetAllocator()),row.GetAllocator());events.push_back({++serial,std::move(row)});}catch(...){++missed;}
}
Json Base(const char *kind,const Metrics *m=nullptr){Json row(rapidjson::kObjectType);auto &a=row.GetAllocator();Put(row,"event",kind,a);if(m){Put(row,"owner",Decimal(std::uintptr_t(m),a),a);Put(row,"world",WorldValue(m->scope,a),a);Put(row,"route",Decimal(m->route,a),a);}return row;}
void Expire(){const auto now=MetricMicros();while(!snapshots.empty()){const auto &s=snapshots.front();const bool reading=s.read_started&&now-s.read_started<=120000000&&now-s.last_read<=2000000;if((reading||now-s.at<=kTtlUs)&&snapshots.size()<=kSnapshots&&bytes<=kBytes)break;bytes-=s.text.size();snapshots.pop_front();}}
}
void E2ETrace(bool on) noexcept {try{std::lock_guard lock(mutex);enabled=on;events.clear();snapshots.clear();bytes=0;missed=0;role_filter.clear();armed_at=0;}catch(...){++missed;}}
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
  auto text=Text(row);if(text.size()>kBytes/2)throw ContractError("stream_snapshot_budget");std::lock_guard lock(mutex);Expire();while(!snapshots.empty()&&(snapshots.size()>=kSnapshots||bytes+text.size()>kBytes)){bytes-=snapshots.front().text.size();snapshots.pop_front();++missed;}bytes+=text.size();snapshots.push_back({++next_id,MetricMicros(),connection,route,root,std::move(text),{},commit_vector});armed_at=0;role_filter.clear();
 }catch(...){++missed;}
}
void E2ERender(std::uint64_t connection,std::uint64_t route,const std::vector<Version> &versions,std::uint64_t revision) noexcept {try{std::lock_guard lock(mutex);Expire();for(auto &s:snapshots)if(s.connection==connection&&s.route==route&&s.render.empty()&&!s.commit_vector.empty()&&s.commit_vector==versions&&std::find(versions.begin(),versions.end(),Version{s.root->id,s.root->revision})!=versions.end())s.render="render_local_us="+std::to_string(MetricMicros())+";render_revision="+std::to_string(revision);}catch(...){++missed;}}
std::string E2ESnapshotPage(unsigned slot,unsigned page){std::lock_guard lock(mutex);Expire();if(!slot){Json rows(rapidjson::kArrayType);for(const auto &s:snapshots){Value v(rapidjson::kObjectType);auto &a=rows.GetAllocator();Put(v,"id",Value(s.id),a);Put(v,"revision",Decimal(s.root->revision,a),a);Put(v,"range",RangeValue(s.root->id,a),a);Put(v,"bytes",Value(unsigned(s.text.size())),a);Put(v,"render",s.render,a);rows.PushBack(std::move(v),a);}return "ok=1\ndata="+Text(rows)+"\n";}
 for(auto &s:snapshots)if(s.id==slot){const auto offset=std::uint64_t(page)*kChunk;if(offset>=s.text.size())throw ContractError("stream_snapshot_page");s.last_read=MetricMicros();if(!s.read_started)s.read_started=s.last_read;const auto count=(std::min)(std::uint64_t(kChunk),s.text.size()-offset);std::string part;part.reserve(std::size_t(count)*2);constexpr char hex[]="0123456789abcdef";for(std::uint64_t i=0;i<count;++i){const auto byte=static_cast<unsigned char>(s.text[std::size_t(offset+i)]);part+=hex[byte>>4];part+=hex[byte&15];}return "ok=1\nbytes="+std::to_string(s.text.size())+"\nencoding=hex-utf8-bytes\nrender="+s.render+"\ndata="+part+"\n";}return "ok=0\nerror=stream_snapshot_expired_or_missing\n";
}
}
