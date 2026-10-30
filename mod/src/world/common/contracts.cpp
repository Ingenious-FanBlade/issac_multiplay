#include "world/common/contracts.h"
#include <sstream>
namespace cs::world::observation {
const char *CoverageName(Coverage v) noexcept {
  constexpr const char *names[]={"Complete","Partial","Failed","NotApplicable","NotScheduled","Disabled","Unsupported"};
  return unsigned(v)<7 ? names[unsigned(v)] : "Unsupported";
}
const char *PhaseName(Phase v) noexcept {
  constexpr const char *names[]={"OrdinaryPost","PlayerPost","Lifecycle","Transition","Render","ActualCall"};
  return unsigned(v)<6 ? names[unsigned(v)] : "Unknown";
}
const char *RecordKindName(RecordKind v) noexcept {
  constexpr const char *names[]={"State","Event","Relation","Coverage","Gap","Catalog"};
  return unsigned(v)<6 ? names[unsigned(v)] : "Gap";
}
namespace {
std::string Quote(const std::string &s) {
  std::string out="\"";
  constexpr char hex[]="0123456789abcdef";
  for (unsigned char c:s) {
    if(c=='"'||c=='\\') { out+='\\'; out+=char(c); }
    else if(c<32) { out+="\\u00"; out+=hex[c>>4]; out+=hex[c&15]; }
    else out+=char(c);
  }
  return out+'"';
}
std::string Subject(const Optional<SubjectKey> &v) {
  if(v.presence==Presence::Unknown) return "{\"presence\":\"Unknown\"}";
  if(v.presence==Presence::NotApplicable) return "{\"presence\":\"NotApplicable\"}";
  const auto &k=v.value;
  return "{\"presence\":\"Known\",\"session\":\""+std::to_string(k.session)+"\",\"epoch\":\""+std::to_string(k.epoch)+"\",\"space\":"+std::to_string(unsigned(k.space))+",\"local_id\":\""+std::to_string(k.local)+"\"}";
}
}
// u64 使用十进制字符串；未知上下文显式写 null，不把本地房号冒充楼层身份。
std::string EncodeEnvelope(const ObservationEnvelope &e) {
  const auto &c=e.context;
  std::ostringstream s;
  s<<"{\"format\":\"isaac.cs.observation\",\"version\":2,\"domain\":"<<unsigned(e.domain)
   <<",\"schema\":"<<Quote(e.schema)<<",\"schema_version\":"<<e.version
   <<",\"kind\":\""<<RecordKindName(e.kind)<<"\",\"context\":{\"session\":\""<<c.session
   <<"\",\"epoch\":\""<<c.epoch<<"\",\"scope\":\""<<c.scope<<"\",\"parent_scope\":\""<<c.parent_scope
   <<"\",\"sequence\":\""<<c.sequence<<"\",\"phase\":\""<<PhaseName(c.phase)<<"\",\"authority_tick\":";
  if(c.authority_tick.presence==Presence::Known) s<<'"'<<c.authority_tick.value<<'"'; else s<<"null";
  s<<",\"run\":"<<Subject(c.run)<<",\"floor\":"<<Subject(c.floor)<<",\"dimension\":"<<Subject(c.dimension)
   <<",\"room_descriptor\":"<<Subject(c.descriptor)<<",\"room_instance\":"<<Subject(c.room)
   <<",\"outer\":\""<<c.outer<<"\",\"owner\":\""<<c.owner<<"\",\"mutation_before\":\""<<c.mutation_before
   <<"\",\"mutation_after\":\""<<c.mutation_after<<"\",\"event_watermark\":\""<<c.event_watermark
   <<"\",\"source\":"<<c.source<<",\"source_version\":"<<c.source_version<<",\"plan_revision\":"<<c.plan_revision
   <<"},\"coverage\":{\"status\":\""<<CoverageName(e.result.coverage)<<"\",\"range\":"<<Quote(e.result.range)
   <<",\"reasons\":[";
  for(std::size_t i=0;i<e.result.reasons.size();++i) { if(i) s<<','; s<<Quote(e.result.reasons[i]); }
  s<<"],\"objects\":"<<e.result.objects<<",\"nodes\":"<<e.result.nodes<<",\"bytes\":"<<e.result.bytes
   <<",\"budget\":{\"objects\":"<<e.result.budget.objects<<",\"nodes\":"<<e.result.budget.nodes
   <<",\"depth\":"<<e.result.budget.depth<<",\"bytes\":"<<e.result.budget.bytes<<"}},\"payload\":"
   <<(e.payload.empty()?"null":e.payload)<<'}';
  return s.str();
}
}
