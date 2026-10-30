#include "world/compatibility/encode.h"
#include <sstream>
#include <vector>
namespace cs::world::compatibility {
std::string EncodeDomain(const BatchHeader &h,const RawObject *objects,const RawMembership *members,ObjectDomain domain,bool &truncated) {
  truncated=false;
  std::vector<std::uint32_t> head(h.object_count,UINT32_MAX),tail(h.object_count,UINT32_MAX),next(h.membership_count,UINT32_MAX);
  for(std::uint32_t m=0;m<h.membership_count;++m) {
    const auto index=members[m].object; if(index>=h.object_count) { truncated=true; continue; }
    if(head[index]==UINT32_MAX) head[index]=m; else next[tail[index]]=m;
    tail[index]=m;
  }
  std::ostringstream s;
  s<<"{\"compatibility\":{\"profile\":\"min-read-v1\",\"attempt\":\""<<h.attempt
   <<"\",\"capture\":\""<<h.capture<<"\",\"complete\":"<<(h.complete?"true":"false")
   <<",\"reason\":"<<h.reason<<",\"identity_mapping\":\"same-epoch-local-id-and-use\"},\"objects\":[";
  bool first=true;
  constexpr char hex[]="0123456789abcdef";
  for(std::uint32_t i=0;i<h.object_count;++i) {
    const auto &o=objects[i]; if(o.domain!=unsigned(domain)) continue;
    if(s.tellp()>std::streampos((16u<<20)-65536)) { truncated=true; break; }
    if(!first) s<<','; first=false;
    s<<"{\"local_id\":\""<<o.local_id<<"\",\"use\":\""<<o.use<<"\",\"birth\":\""<<o.birth
     <<"\",\"revision\":\""<<o.revision<<"\",\"class\":"<<o.kind<<",\"origin\":"<<o.origin
     <<",\"readiness\":"<<o.readiness<<",\"lifecycle\":"<<o.lifecycle<<",\"birth_known\":"<<o.birth_known
     <<",\"participant_ordinal\":";
    if(o.ordinal<4) s<<o.ordinal; else s<<"null";
    s<<",\"fields\":[";
    for(std::uint32_t f=0;f<o.field_count;++f) {
      const auto &v=o.fields[f]; if(f) s<<',';
      s<<"{\"id\":"<<v.id<<",\"status\":"<<unsigned(v.status)<<",\"encoding\":"<<unsigned(v.encoding)<<",\"width\":"<<v.width;
      if(v.encoding==Encoding::Reference) s<<",\"resolution\":"<<unsigned(v.relation)<<",\"target\":\""<<v.target_id<<"\",\"target_use\":\""<<v.target_use<<'"';
      else if(v.status==FieldStatus::Value) {
        s<<",\"bytes_le\":\"";
        for(std::uint32_t b=0;b<v.width;++b) s<<hex[v.raw[b]>>4]<<hex[v.raw[b]&15];
        s<<'"';
      }
      s<<'}';
    }
    s<<"],\"memberships\":[";
    bool member_first=true;
    for(auto m=head[i];m!=UINT32_MAX;m=next[m]) {
      if(!member_first) s<<','; member_first=false;
      s<<"{\"source\":"<<members[m].source<<",\"index\":"<<members[m].index<<'}';
    }
    s<<"]}";
  }
  s<<"],\"truncated\":"<<(truncated?"true":"false")<<'}'; return s.str();
}
}
