#include "replica/stream_codec.h"
#include <algorithm>
#include <cstring>
#include <set>
#include "core/session.h"

namespace cs::replica::stream {
namespace {
[[noreturn]] void Fail(const char *s){throw ContractError(s);}
const Value &At(const Value &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Fail("stream_missing_field");return v[k];}
std::uint64_t N(const Value &v,const char *k){return Unsigned(At(v,k));}
std::string S(const Value &v){if(!v.IsString())Fail("stream_string");return {v.GetString(),v.GetStringLength()};}
void Members(const Value &v,std::initializer_list<const char *> keys){if(!v.IsObject()||v.MemberCount()!=keys.size())Fail("stream_schema");for(auto k:keys)At(v,k);}
void Text(const Value &v,const char *k,std::size_t max=128){const auto s=S(At(v,k));if(s.empty()||s.size()>max)Fail("stream_text_range");}
void Positive(const Value &v,std::initializer_list<const char *> keys){for(auto k:keys)if(!N(v,k)||N(v,k)==UINT64_MAX)Fail("stream_sequence_range");}
void WorldValue(const Value &v){Members(v,{"session","run","floor","dimension","room","scene"});Positive(v,{"session","run","floor","dimension","room","scene"});}
void Versions(const Value &v){if(!v.IsArray()||v.Size()>64)Fail("stream_versions");std::set<std::string> seen;for(const auto &x:v.GetArray()){Members(x,{"kind","name","body","revision"});if(N(x,"kind")<1||N(x,"kind")>6)Fail("stream_range_kind");Text(x,"name");Positive(x,{"revision"});if((N(x,"kind")==6)==x["body"].IsNull())Fail("stream_version_body");if(!x["body"].IsNull()){Members(x["body"],{"run","id","space"});Positive(x["body"],{"run","id","space"});if(N(x["body"],"space")>14)Fail("stream_version_space");}if(!seen.insert(std::to_string(N(x,"kind"))+":"+S(x["name"])+":"+replica::Text(x["body"])).second)Fail("stream_duplicate_version");}}
void U(std::vector<std::uint8_t> &b,std::uint64_t n,unsigned width){for(unsigned i=0;i<width;++i)b.push_back(std::uint8_t(n>>(8*i)));}
struct Reader {std::span<const std::uint8_t> b;std::size_t p=0;std::uint64_t U(unsigned n){if(n>b.size()-p)Fail("stream_truncated");std::uint64_t x=0;for(unsigned i=0;i<n;++i)x|=std::uint64_t(b[p++])<<(8*i);return x;}};
bool KnownKind(Kind k){return unsigned(k)>=1&&unsigned(k)<=20;}
void Scope(const Value &b){WorldValue(At(b,"world"));}
}
Lane MessageLane(const Message &m){
  if(!KnownKind(m.kind))Fail("stream_unknown_kind");
  if(m.kind==Kind::Input||m.kind==Kind::InputAck)return Lane::Input;
  if(m.kind!=Kind::Range)return Lane::Control;
  const auto kind=m.range?unsigned(m.range->id.kind):N(m.body,"range_kind");if(kind<1||kind>6)Fail("stream_range_kind");return kind==1?Lane::Baseline:kind<=4?Lane::Context:kind==5?Lane::WorldDynamic:Lane::PlayerFast;
}
void ValidateMessage(const Message &m){
  if(!m.range)CheckValue(m.body);if(!KnownKind(m.kind)||!m.sequence||m.sequence==UINT64_MAX)Fail("stream_kind_sequence");
  if(m.kind==Kind::Hello){if(m.connection||m.route)Fail("stream_hello_route");}else if(!m.connection||!m.route||m.route==UINT64_MAX)Fail("stream_route");
  const auto &b=m.body;
  if(m.range){if(m.kind!=Kind::Range||!b.IsNull())Fail("stream_range_representation");ValidateRangePayload(m);return;}
  switch(m.kind){
    case Kind::Hello:Members(b,{"capabilities","nonce","role"});Positive(b,{"nonce"});if(S(b["role"])!="Client")Fail("stream_hello_role");Negotiate(Capabilities(),b["capabilities"]);break;
    case Kind::Welcome:Members(b,{"capabilities","session","client_nonce","server_nonce","participant","role"});Positive(b,{"session","client_nonce","server_nonce"});if(N(b,"participant")<1||N(b,"participant")>3||S(b["role"])!="Authority")Fail("stream_welcome");Negotiate(Capabilities(),b["capabilities"]);break;
    case Kind::Ready:Members(b,{"client_nonce","server_nonce","sent_ms"});Positive(b,{"client_nonce","server_nonce"});N(b,"sent_ms");break;
    case Kind::ScenePrepare:Members(b,{"world","next_route","manifest","generation","native_domains","retained_domains","contract"});Scope(b);Positive(b,{"next_route","manifest","generation"});if(N(b,"next_route")!=m.route+1||S(b["contract"])!="complete-ranges-schema-v1"||(N(b,"native_domains")&N(b,"retained_domains"))||(N(b,"native_domains")|N(b,"retained_domains"))!=2047)Fail("stream_prepare_contract");break;
    case Kind::ScenePrepared:Members(b,{"world","manifest","generation","native_domains","retained_domains","contract"});Scope(b);Positive(b,{"manifest","generation"});if(S(b["contract"])!="complete-ranges-schema-v1"||(N(b,"native_domains")&N(b,"retained_domains"))||(N(b,"native_domains")|N(b,"retained_domains"))!=2047)Fail("stream_prepared_contract");break;
    case Kind::Range:Members(b,{"world","range_kind","name","revision","schema","dependencies","complete","payload"});Scope(b);Text(b,"name");Positive(b,{"revision"});if(N(b,"schema")!=kSchema||b["complete"]!=true||!b["payload"].IsObject())Fail("stream_range_schema");Versions(b["dependencies"]);MessageLane(m);ValidateRangePayload(m);break;
    case Kind::ReceiveAck:{Members(b,{"transfer","part_count","window","bitmap","complete"});Positive(b,{"transfer","part_count"});if(N(b,"part_count")>32768||N(b,"window")%256||N(b,"window")>=N(b,"part_count")||!b["bitmap"].IsArray()||b["bitmap"].Size()!=4||!b["complete"].IsBool())Fail("stream_receive_ack");const auto remaining=N(b,"part_count")-N(b,"window");bool all=true;for(unsigned i=0;i<4;++i){const auto bits=Unsigned(b["bitmap"][i]);const auto start=std::uint64_t(i)*64,valid=remaining>start?(std::min)(64ull,remaining-start):0ull;const auto mask=valid==64?UINT64_MAX:valid?((1ull<<valid)-1):0;if(bits&~mask)Fail("stream_receive_ack_tail");if(bits!=mask)all=false;}if(b["complete"].GetBool()&&(remaining>256||!all))Fail("stream_receive_ack_completion");break;}
    case Kind::ContextReady:Members(b,{"world","versions"});Scope(b);Versions(b["versions"]);if(b["versions"].Empty())Fail("stream_context_ready_empty");break;
    case Kind::AppliedAck:{Members(b,{"world","versions","results","native_domains","retained_domains","event_watermark"});Scope(b);Versions(b["versions"]);if(b["versions"].Empty()||!b["results"].IsArray()||b["results"].Size()!=b["versions"].Size()||(N(b,"native_domains")&N(b,"retained_domains"))||((N(b,"native_domains")|N(b,"retained_domains"))&~2047ull))Fail("stream_applied_scope");
      std::uint64_t native=0,retained=0,watermark=0;for(unsigned i=0;i<b["results"].Size();++i){const auto &r=b["results"][i];Members(r,{"version","attempt","instance","resource","native_domains","retained_domains","event_watermark"});if(r["version"]!=b["versions"][i])Fail("stream_applied_result_version");Positive(r,{"attempt","instance","resource"});if((N(r,"native_domains")&N(r,"retained_domains"))||((N(r,"native_domains")|N(r,"retained_domains"))&~2047ull))Fail("stream_applied_result_domains");native|=N(r,"native_domains");retained|=N(r,"retained_domains");watermark=(std::max)(watermark,N(r,"event_watermark"));}if(native!=N(b,"native_domains")||retained!=N(b,"retained_domains")||watermark!=N(b,"event_watermark"))Fail("stream_applied_result_union");break;}
    case Kind::Resync:Members(b,{"world","failed_revision","reason"});Scope(b);N(b,"failed_revision");Text(b,"reason");break;
    case Kind::Cancel:Members(b,{"transfer","reason"});Positive(b,{"transfer"});Text(b,"reason");break;
    case Kind::OperationAck:Members(b,{"operation","operation_sequence","stage","progress_sequence","reason"});Positive(b,{"operation_sequence","progress_sequence"});if(N(b,"operation")<1||N(b,"operation")>20||N(b,"stage")<1||N(b,"stage")>6)Fail("stream_operation_result");Text(b,"reason");break;
    case Kind::Retire:case Kind::Stop:case Kind::Disconnect:Members(b,{"reason"});Text(b,"reason");break;
    case Kind::Error:Members(b,{"code","recoverable","detail"});Text(b,"code");Text(b,"detail",1024);if(!b["recoverable"].IsBool())Fail("stream_error_schema");break;
    case Kind::Roster:Members(b,{"eligibility_revision","server_tick","participant","accepted","processed","processed_tick","processed_source","generation","state","roster","rejected","rejection_mask","input_enabled","server_sample_ms","server_send_ms","echo_client_ms","echo_server_ms"});
      Positive(b,{"eligibility_revision"});for(auto i=b.MemberBegin();i!=b.MemberEnd();++i)if(S(i->name)!="input_enabled")Unsigned(i->value);
      if(!b["input_enabled"].IsBool()||N(b,"participant")<1||N(b,"participant")>3||N(b,"roster")>15||!(N(b,"roster")&1)||!(N(b,"roster")&(1ull<<N(b,"participant")))||N(b,"state")>unsigned(core::SessionState::Failed)||N(b,"rejection_mask")>UINT32_MAX||N(b,"processed")>N(b,"accepted")||N(b,"processed_tick")>N(b,"server_tick")||N(b,"processed_source")>N(b,"processed"))Fail("stream_roster");break;
    case Kind::TickProgress:Members(b,{"eligibility_revision","server_tick","kind","generation","input_enabled","sample_ms","send_ms","echo_client_ms","echo_server_ms"});Positive(b,{"eligibility_revision","server_tick","generation"});if(N(b,"kind")>1||!b["input_enabled"].IsBool()||N(b,"sample_ms")>N(b,"send_ms")||N(b,"echo_server_ms")>N(b,"send_ms"))Fail("stream_tick_progress");N(b,"echo_client_ms");break;
    case Kind::InputAck:Members(b,{"participant","generation","input_sequence","processed_tick"});if(N(b,"participant")<1||N(b,"participant")>3)Fail("stream_participant");Positive(b,{"generation","input_sequence","processed_tick"});if(N(b,"input_sequence")!=m.sequence)Fail("stream_input_sequence");break;
    case Kind::Input:{Members(b,{"participant","generation","input_sequence","target_tick","movement","aim","held","edge"});if(N(b,"participant")<1||N(b,"participant")>3||N(b,"held")>15||N(b,"edge")>15)Fail("stream_input_range");Positive(b,{"generation","input_sequence","target_tick"});if(N(b,"input_sequence")!=m.sequence)Fail("stream_input_sequence");for(const auto *k:{"movement","aim"}){if(!b[k].IsArray()||b[k].Size()!=2)Fail("stream_axis");for(const auto &x:b[k].GetArray())if(!x.IsInt()||x.GetInt() < -32767||x.GetInt()>32767)Fail("stream_axis");}core::InputStateDraftV2 v;v.move_x=std::int16_t(b["movement"][0].GetInt());v.move_y=std::int16_t(b["movement"][1].GetInt());v.aim_x=std::int16_t(b["aim"][0].GetInt());v.aim_y=std::int16_t(b["aim"][1].GetInt());v.held_flags=unsigned(N(b,"held"));v.edge_flags=unsigned(N(b,"edge"));if(!core::Valid(v))Fail("stream_noncanonical_input");break;}
    default:Fail("stream_unknown_kind");
  }
}
void ValidateReceive(const Message &m,Direction dir,State state){
  ValidateMessage(m);if(state==State::Stopped)Fail("stream_stopped");
  const bool to_host=m.kind==Kind::Hello||m.kind==Kind::Ready||m.kind==Kind::ScenePrepared||m.kind==Kind::ReceiveAck||m.kind==Kind::ContextReady||m.kind==Kind::AppliedAck||m.kind==Kind::Resync||m.kind==Kind::Input||m.kind==Kind::Disconnect;
  const bool both=m.kind==Kind::Stop||m.kind==Kind::Disconnect||m.kind==Kind::Error||m.kind==Kind::Cancel||m.kind==Kind::Retire||m.kind==Kind::OperationAck;
  if(!both&&m.kind!=Kind::ScenePrepared&&(to_host!=(dir==Direction::ToAuthority)))Fail("stream_wrong_direction");
  if(state==State::Handshake&&m.kind!=Kind::Hello&&m.kind!=Kind::Welcome&&m.kind!=Kind::Error&&m.kind!=Kind::Stop&&m.kind!=Kind::Disconnect)Fail("stream_handshake_state");
  if((m.kind==Kind::Range||m.kind==Kind::ContextReady||m.kind==Kind::AppliedAck||m.kind==Kind::Resync)&&state!=State::Preparing&&state!=State::Active)Fail("stream_world_state");
  if(state==State::Retiring&&m.kind!=Kind::Stop&&m.kind!=Kind::Disconnect&&m.kind!=Kind::Cancel&&m.kind!=Kind::Error&&m.kind!=Kind::ReceiveAck&&m.kind!=Kind::OperationAck&&m.kind!=Kind::Retire)Fail("stream_retiring_state");
}
Json Capabilities(){
  auto c=replica::Capabilities();c["protocol"].SetUint(2);auto &a=c.GetAllocator();c["required"].Clear();c["supported"].Clear();
  for(const auto *name:{"typed-values-v1","stable-keys-v1","replica-projection-v2","entity-target-v1","player-target-v2","icsrf-v2","stream-schema-v1","complete-ranges-schema-v1","receive-ack-schema-v1","precise-applied-schema-v1"}){c["required"].PushBack(Value(name,a),a);c["supported"].PushBack(Value(name,a),a);}
  for(const auto *name:{"compact-payload-v1","small-bundle-v1","selective-receive-v1","reliable-operations-v1","native-range-commit-v1","world-dynamic-layout-v2"}){c["required"].PushBack(Value(name,a),a);c["supported"].PushBack(Value(name,a),a);}
  Put(c,"framing",Value(2),a);Put(c,"stream_schema",Value(kSchema),a);Value compression(rapidjson::kArrayType);compression.PushBack(Value("none",a),a);compression.PushBack(Value("zstd-v1",a),a);Put(c,"compression",std::move(compression),a);
  Value world(rapidjson::kObjectType);const auto l=WorldLimits();for(const auto &[key,n]:std::initializer_list<std::pair<const char *,std::uint64_t>>{{"bytes",l.bytes},{"nodes",l.nodes},{"depth",l.depth},{"string_bytes",l.string_bytes},{"fragments",l.fragments},{"candidates",l.candidates},{"value_bytes",l.value_bytes},{"zstd_window_bytes",8ull<<20}})Put(world,key,Value(n),a);Put(c,"world_limits",std::move(world),a);return c;
}
Json Negotiate(const Value &a,const Value &b){
  for(const auto *v:{&a,&b}){if(!v->IsObject()||v->MemberCount()!=16||N(*v,"protocol")!=2||N(*v,"framing")!=2||N(*v,"stream_schema")!=kSchema)Fail("stream_capability_version");if(N(At(*v,"limits"),"datagram")<=64||N((*v)["limits"],"datagram")>1200)Fail("stream_datagram_capability");const auto &codecs=At(*v,"compression");if(!codecs.IsArray()||codecs.Empty()||codecs.Size()>2)Fail("stream_compression_capability");std::set<std::string> seen;for(const auto &x:codecs.GetArray()){const auto s=S(x);if((s!="none"&&s!="zstd-v1")||!seen.insert(s).second)Fail("stream_compression_capability");}if(!seen.contains("none"))Fail("stream_compression_required");}
  for(const auto *v:{&a,&b}){const auto &w=At(*v,"world_limits");Members(w,{"bytes","nodes","depth","string_bytes","fragments","candidates","value_bytes","zstd_window_bytes"});if(N(w,"zstd_window_bytes")!=(8ull<<20))Fail("stream_zstd_window_capability");const auto ceiling=WorldLimits();for(const auto &[key,max]:std::initializer_list<std::pair<const char *,std::uint64_t>>{{"bytes",ceiling.bytes},{"nodes",ceiling.nodes},{"depth",ceiling.depth},{"string_bytes",ceiling.string_bytes},{"fragments",ceiling.fragments},{"candidates",ceiling.candidates},{"value_bytes",ceiling.value_bytes}})if(!N(w,key)||N(w,key)>max)Fail("stream_world_capability_budget");}
  auto x=Copy(a),y=Copy(b);for(auto *v:{&x,&y}){v->RemoveMember("framing");v->RemoveMember("stream_schema");v->RemoveMember("compression");v->RemoveMember("world_limits");}
  auto c=replica::Negotiate(x,y);auto &alloc=c.GetAllocator();Put(c,"framing",Value(2),alloc);Put(c,"stream_schema",Value(kSchema),alloc);Value codecs(rapidjson::kArrayType);for(const auto &l:a["compression"].GetArray())for(const auto &r:b["compression"].GetArray())if(l==r){Value v;v.CopyFrom(l,alloc);codecs.PushBack(v,alloc);}Put(c,"compression",std::move(codecs),alloc);Value world(rapidjson::kObjectType);for(auto i=a["world_limits"].MemberBegin();i!=a["world_limits"].MemberEnd();++i){const auto key=S(i->name);Put(world,key.c_str(),Value((std::min)(Unsigned(i->value),N(b["world_limits"],key.c_str()))),alloc);}Put(c,"world_limits",std::move(world),alloc);return c;
}
std::vector<std::uint8_t> Encode(const Message &m,Limits l){if(!m.range)CheckValue(m.body,l);ValidateMessage(m);const auto payload=EncodePayload(m,l);if(payload.size()>l.bytes||payload.size()+40>l.bytes)Fail("stream_wire_budget");std::vector<std::uint8_t> b={'I','C','S','R'};b.reserve(payload.size()+40);U(b,2,2);U(b,40,2);U(b,unsigned(m.kind),2);U(b,unsigned(MessageLane(m)),1);U(b,0,1);U(b,payload.size(),4);U(b,m.sequence,8);U(b,m.connection,8);U(b,m.route,8);b.insert(b.end(),payload.begin(),payload.end());return b;}
Message Decode(std::span<const std::uint8_t> b,Limits l){if(b.size()<40||b.size()>l.bytes||std::memcmp(b.data(),"ICSR",4))Fail("stream_wire_header");Reader r{b,4};if(r.U(2)!=2||r.U(2)!=40)Fail("stream_wire_version");Message m;m.kind=Kind(r.U(2));const auto lane=r.U(1);if(r.U(1))Fail("stream_wire_flags");const auto n=r.U(4);m.sequence=r.U(8);m.connection=r.U(8);m.route=r.U(8);if(n!=b.size()-40)Fail("stream_wire_length");DecodePayload(b.subspan(40),m,l);ValidateMessage(m);if(lane!=unsigned(MessageLane(m)))Fail("stream_wire_lane");return m;}
Header ReadHeader(std::span<const std::uint8_t> b,Limits l){
  if(b.size()<64||b.size()>l.datagram||std::memcmp(b.data(),"ICRF",4))Fail("stream_fragment_header");Reader r{b,4};if(r.U(2)!=2||r.U(2)!=64)Fail("stream_fragment_version");Header h;h.connection=r.U(8);h.route=r.U(8);h.transfer=r.U(8);h.wire_bytes=unsigned(r.U(4));h.decoded_bytes=unsigned(r.U(4));h.index=unsigned(r.U(4));h.count=unsigned(r.U(4));h.kind=Kind(r.U(2));h.lane=Lane(r.U(1));h.codec=Codec(r.U(1));h.payload=std::uint16_t(r.U(2));if(r.U(2)||r.U(8))Fail("stream_fragment_reserved");
  if(!KnownKind(h.kind)||!h.transfer||h.transfer==UINT64_MAX||!h.count||h.count>l.fragments||h.index>=h.count||!h.wire_bytes||h.wire_bytes>l.bytes||h.decoded_bytes<40||h.decoded_bytes>l.bytes||unsigned(h.codec)>1||unsigned(h.lane)<1||unsigned(h.lane)>6||!h.payload||h.payload!=b.size()-64||l.datagram<=64||l.datagram>1200)Fail("stream_fragment_range");
  if(h.kind==Kind::Range?(h.lane<Lane::Baseline||h.lane>Lane::PlayerFast):h.lane!=((h.kind==Kind::Input||h.kind==Kind::InputAck)?Lane::Input:Lane::Control))Fail("stream_fragment_lane");
  if(h.kind==Kind::Hello?(h.connection||h.route):(!h.connection||!h.route))Fail("stream_fragment_route");
  const auto chunk=l.datagram-64;if(h.count!=(std::uint64_t(h.wire_bytes)+chunk-1)/chunk||h.payload!=(h.index+1<h.count?chunk:h.wire_bytes-std::uint64_t(h.index)*chunk))Fail("stream_fragment_geometry");if(h.codec==Codec::None&&h.wire_bytes!=h.decoded_bytes)Fail("stream_fragment_decoded_size");return h;
}
// 兼容小消息调用端；大可靠范围直接持有共享Blob并逐片生成，避免整份分片副本。
std::vector<std::vector<std::uint8_t>> Fragment(const Message &m,std::uint64_t transfer,Limits l){
  if(l.bytes>65536||MessageLane(m)==Lane::Baseline||MessageLane(m)==Lane::Context)Fail("stream_large_transfer_requires_shared_blob");
  const auto blob=EncodeBlob(m,false,l);const auto count=(blob->bytes.size()+l.datagram-65)/(l.datagram-64);
  if(count>l.fragments)Fail("stream_fragment_count");std::vector<std::vector<std::uint8_t>> out;
  for(unsigned i=0;i<count;++i)out.push_back(Datagram(*blob,{m.connection,m.route,transfer},i,l));return out;
}
} // namespace cs::replica::stream
