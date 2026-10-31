#include "replica/codec.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <set>

namespace cs::replica {
namespace {
[[noreturn]] void Fail(const char *c){throw ContractError(c);}
std::string Str(const Value &v){return v.IsString()?std::string(v.GetString(),v.GetStringLength()):std::string{};}
const Value &At(const Value &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Fail("missing_field");return v[k];}
void U(std::vector<std::uint8_t> &v,std::uint64_t n,unsigned bytes){for(unsigned i=0;i<bytes;++i)v.push_back(std::uint8_t(n>>(8*i)));}
struct Reader {
  std::span<const std::uint8_t> bytes;std::size_t p=0;Limits l;std::uint64_t nodes=0;
  std::uint64_t U(unsigned n){if(n>bytes.size()-p)Fail("truncated_wire");std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t(bytes[p++])<<(8*i);return v;}
  Value String(Allocator &a){const auto n=U(4);if(n>l.string_bytes||n>bytes.size()-p)Fail("wire_string_budget");Value v(reinterpret_cast<const char *>(bytes.data()+p),rapidjson::SizeType(n),a);p+=std::size_t(n);return v;}
  Value Read(Allocator &a,unsigned depth=0){
    if(depth>l.depth||++nodes>l.nodes)Fail("wire_value_budget");
    switch(U(1)){
      case 0:return Value();case 1:return Value(false);case 2:return Value(true);
      case 3:return Value(U(8));case 4:return Value(std::bit_cast<std::int64_t>(U(8)));
      case 5:{const auto d=std::bit_cast<double>(U(8));return Value(d);}
      case 6:return String(a);
      case 7:{const auto count=U(4);if(count>l.nodes-nodes||count>bytes.size()-p)Fail("wire_array_budget");Value v(rapidjson::kArrayType);for(std::uint64_t i=0;i<count;++i)v.PushBack(Read(a,depth+1),a);return v;}
      case 8:{const auto count=U(4);if(count>(l.nodes-nodes)/2||count>(bytes.size()-p)/5)Fail("wire_object_budget");Value v(rapidjson::kObjectType);std::set<std::string> seen;
        for(std::uint64_t i=0;i<count;++i){auto k=String(a);++nodes;if(!seen.insert(Str(k)).second)Fail("duplicate_field");auto x=Read(a,depth+1);v.AddMember(k,x,a);}return v;}
      default:Fail("unknown_value_tag");
    }
  }
};
void String(std::vector<std::uint8_t> &out,const Value &v){U(out,v.GetStringLength(),4);const auto *b=reinterpret_cast<const std::uint8_t *>(v.GetString());out.insert(out.end(),b,b+v.GetStringLength());}
void Write(std::vector<std::uint8_t> &out,const Value &v,const Limits &limits){
  if(v.IsNull())U(out,0,1);else if(v.IsBool())U(out,v.GetBool()?2:1,1);
  else if(v.IsUint64()){U(out,3,1);U(out,v.GetUint64(),8);}else if(v.IsInt64()){U(out,4,1);U(out,std::bit_cast<std::uint64_t>(v.GetInt64()),8);}
  else if(v.IsDouble()){U(out,5,1);U(out,std::bit_cast<std::uint64_t>(v.GetDouble()),8);}
  else if(v.IsString()){U(out,6,1);String(out,v);}
  else if(v.IsArray()){U(out,7,1);U(out,v.Size(),4);for(const auto &x:v.GetArray())Write(out,x,limits);}
  else if(v.IsObject()){
    U(out,8,1);U(out,v.MemberCount(),4);std::vector<const Value::Member *> sorted;
    for(auto it=v.MemberBegin();it!=v.MemberEnd();++it)sorted.push_back(&*it);
    std::sort(sorted.begin(),sorted.end(),[](auto *a,auto *b){return Str(a->name)<Str(b->name);});
    for(auto *it:sorted){String(out,it->name);Write(out,it->value,limits);}
  }else Fail("unknown_value");
  if(out.size()>limits.bytes)Fail("wire_byte_budget");
}
void Members(const Value &v,std::initializer_list<const char *> keys){
  if(!v.IsObject()||v.MemberCount()!=keys.size())Fail("message_schema");for(auto k:keys)At(v,k);
}
void Schema(const Message &m){
  if(!m.sequence)Fail("zero_sequence");CheckValue(m.body);const auto &b=m.body;
  if(m.kind==Kind::Hello||m.kind==Kind::Welcome){
    if(m.kind==Kind::Hello){Members(b,{"capabilities","nonce","role"});if(Str(b["role"])!="Client"||!Unsigned(b["nonce"]))Fail("hello_identity");}
    else{Members(b,{"capabilities","scope","client_nonce","server_nonce","participant","role"});ReadScope(b["scope"]);if(Str(b["role"])!="Authority"||!Unsigned(b["client_nonce"])||!Unsigned(b["server_nonce"])||Unsigned(b["participant"])>3)Fail("welcome_identity");}
    Negotiate(Capabilities(),b["capabilities"]);return;
  }
  ReadScope(At(b,"scope"));
  switch(m.kind){
    case Kind::Baseline:
      Members(b,{"format","scope","revision","components","missing_domains","publishable","event_watermark","native_applied"});
      if(Str(b["format"])!="isaac.cs.replica.model"||!Unsigned(b["revision"])||!b["components"].IsObject()||!b["missing_domains"].IsArray()||!b["publishable"].IsBool()||b["native_applied"]!=false)Fail("baseline_schema");
      if(m.sequence!=Unsigned(b["revision"]))Fail("baseline_sequence");ValidateModel(b);break;
    case Kind::Delta:
      Members(b,{"scope","base","target","header","replace","remove"});
      if(!b["replace"].IsObject()||!b["remove"].IsArray()||!b["header"].IsObject()||!Unsigned(b["base"])||Unsigned(b["target"])<=Unsigned(b["base"])||m.sequence!=Unsigned(b["target"]))Fail("delta_schema");break;
    case Kind::Event:{
      Members(b,{"scope","event_id","server_tick","required_state","expires_tick","kind","parameters"});
      if(Unsigned(b["event_id"])!=m.sequence||!Unsigned(b["server_tick"])||Unsigned(b["expires_tick"])<Unsigned(b["server_tick"]))Fail("event_range");
      const auto k=Str(b["kind"]);if(k!="sound"&&k!="flash"&&k!="particle"&&k!="text"&&k!="animation"&&k!="shake")Fail("unsupported_event");
      Unsigned(b["required_state"]);break;}
    case Kind::Control:{
      Members(b,{"scope","control_id","required_state","operation","parameters"});
      if(Unsigned(b["control_id"])!=m.sequence)Fail("control_sequence");Unsigned(b["required_state"]);
      const auto k=Str(b["operation"]);if(k!="scene_prepare"&&k!="scene_retire"&&k!="resync"&&k!="stop"&&k!="roster"&&k!="ui_intent"&&k!="ready"&&k!="start"&&k!="disconnect")Fail("unsupported_control");break;}
    case Kind::InputAck:
      Members(b,{"scope","participant","generation","input_sequence","processed_tick"});
      if(Unsigned(b["participant"])>3||!Unsigned(b["generation"])||Unsigned(b["input_sequence"])!=m.sequence||!Unsigned(b["processed_tick"]))Fail("input_ack_schema");break;
    case Kind::AppliedAck:
      Members(b,{"scope","snapshot","event_watermark"});if(Unsigned(b["snapshot"])!=m.sequence)Fail("applied_ack_schema");Unsigned(b["event_watermark"]);break;
    case Kind::ReceiveAck:
      Members(b,{"scope","transfer","received_parts"});if(!Unsigned(b["transfer"])||!b["received_parts"].IsArray()||b["received_parts"].Size()>32768)Fail("receive_ack_schema");
      {std::set<std::uint64_t> seen;for(const auto &v:b["received_parts"].GetArray())if(Unsigned(v)>=32768||!seen.insert(Unsigned(v)).second)Fail("receive_ack_parts");}break;
    case Kind::Component:
      Members(b,{"scope","domain","source_schema","capture_kind","source_scope","required_state","coverage","value"});
      if(Unsigned(b["domain"])<1||Unsigned(b["domain"])>11||!b["source_schema"].IsString()||!b["coverage"].IsString()||!Unsigned(b["source_scope"]))Fail("component_schema");
      {const auto phase=Str(b["capture_kind"]);if(phase!="Render"&&phase!="PlayerPost"&&phase!="Lifecycle"&&phase!="Transition"&&phase!="ActualCall")Fail("component_phase");}Unsigned(b["required_state"]);break;
    case Kind::Input:
      Members(b,{"scope","participant","generation","input_sequence","target_tick","movement","aim","held","edge"});
      if(Unsigned(b["participant"])>3||!Unsigned(b["generation"])||Unsigned(b["input_sequence"])!=m.sequence||!Unsigned(b["target_tick"])||Unsigned(b["held"])>15||Unsigned(b["edge"])>15)Fail("input_schema");
      for(const char *key:{"movement","aim"}){if(!b[key].IsArray()||b[key].Size()!=2)Fail("input_axis");for(const auto &axis:b[key].GetArray())if(!axis.IsInt()||axis.GetInt() < -32767||axis.GetInt()>32767)Fail("input_axis");}
      {core::InputStateDraftV2 state;state.move_x=std::int16_t(b["movement"][0].GetInt());state.move_y=std::int16_t(b["movement"][1].GetInt());state.aim_x=std::int16_t(b["aim"][0].GetInt());state.aim_y=std::int16_t(b["aim"][1].GetInt());state.held_flags=std::uint32_t(Unsigned(b["held"]));state.edge_flags=std::uint32_t(Unsigned(b["edge"]));if(!core::Valid(state))Fail("noncanonical_input");}break;
    case Kind::Error:
      Members(b,{"scope","code","recoverable","detail"});if(!b["code"].IsString()||!b["recoverable"].IsBool()||!b["detail"].IsString())Fail("error_schema");break;
    default:Fail("unknown_kind");
  }
}
void SameScope(const Value &b,const Scope &s){if(ReadScope(At(b,"scope"))!=s)Fail("stale_scope");}
std::string Signature(const Message &m){const auto bytes=Encode(m);return {reinterpret_cast<const char *>(bytes.data()),bytes.size()};}
}
Lane MessageLane(Kind k){
  switch(k){case Kind::Input:case Kind::InputAck:return Lane::Input;case Kind::Baseline:return Lane::Recovery;case Kind::Delta:case Kind::Component:return Lane::State;case Kind::Event:return Lane::Presentation;
    case Kind::Hello:case Kind::Welcome:case Kind::Control:case Kind::AppliedAck:case Kind::Error:case Kind::ReceiveAck:return Lane::Control;default:Fail("unknown_kind");}
}
std::vector<std::uint8_t> Encode(const Message &m,Limits l){
  CheckValue(m.body,l);Schema(m);std::vector<std::uint8_t> out={'I','C','S','R'};
  U(out,1,2);U(out,unsigned(m.kind),1);U(out,unsigned(MessageLane(m.kind)),1);U(out,0,4);U(out,0,4);U(out,m.sequence,8);
  Write(out,m.body,l);const auto n=std::uint32_t(out.size()-24);for(unsigned i=0;i<4;++i)out[12+i]=std::uint8_t(n>>(8*i));return out;
}
Message Decode(std::span<const std::uint8_t> bytes,Limits l){
  if(bytes.size()<24||bytes.size()>l.bytes||std::memcmp(bytes.data(),"ICSR",4))Fail("wire_header");
  Reader r{bytes,4,l};if(r.U(2)!=1)Fail("wire_version");Message m;m.kind=Kind(r.U(1));const auto lane=r.U(1);
  if(lane!=unsigned(MessageLane(m.kind))||r.U(4)!=0)Fail("wire_flags_lane_reserved");const auto size=r.U(4);m.sequence=r.U(8);
  if(size!=bytes.size()-24)Fail("wire_length");auto v=r.Read(m.body.GetAllocator());static_cast<Value &>(m.body).Swap(v);
  if(r.p!=bytes.size())Fail("wire_trailing_bytes");CheckValue(m.body,l);
  // 验证直接来自二进制的UTF-8，不接受替换字符悄悄修复非法字符串。
  auto validated=Parse(Text(m.body),l);(void)validated;Schema(m);return m;
}
std::vector<std::vector<std::uint8_t>> Fragment(const Message &m,std::uint64_t transfer,Limits l){
  if(!transfer||l.datagram<=32||l.datagram>65507)Fail("fragment_budget");const auto wire=Encode(m,l);const auto chunk=l.datagram-32;
  const auto count=std::uint32_t((wire.size()+chunk-1)/chunk);if(count>l.fragments)Fail("fragment_count");
  std::vector<std::vector<std::uint8_t>> parts;
  for(std::uint32_t i=0;i<count;++i){
    const auto begin=std::size_t(i)*chunk;const auto n=(std::min)(std::size_t(chunk),wire.size()-begin);
    std::vector<std::uint8_t> p={'I','C','R','F'};U(p,1,2);U(p,0,2);U(p,transfer,8);U(p,wire.size(),4);U(p,i,4);U(p,count,4);U(p,n,4);p.insert(p.end(),wire.begin()+begin,wire.begin()+begin+n);parts.push_back(std::move(p));
  }return parts;
}
void Reassembler::Expire(std::uint64_t now){for(auto i=pending_.begin();i!=pending_.end();)if(now<i->second.started||now-i->second.started>=limits_.timeout_ms)i=pending_.erase(i);else ++i;}
bool Reassembler::Receive(std::span<const std::uint8_t> bytes,std::uint64_t now,Message &out){
  Expire(now);if(bytes.size()<32||bytes.size()>limits_.datagram||std::memcmp(bytes.data(),"ICRF",4))Fail("fragment_header");
  Reader r{bytes,4,limits_};if(r.U(2)!=1||r.U(2)!=0)Fail("fragment_version_reserved");
  const auto id=r.U(8);const auto total=std::uint32_t(r.U(4)),index=std::uint32_t(r.U(4)),count=std::uint32_t(r.U(4)),n=std::uint32_t(r.U(4));
  if(!id||!count||count>limits_.fragments||index>=count||total<24||total>limits_.bytes||n!=bytes.size()-32||!n||limits_.datagram<=32)Fail("fragment_range");
  const auto chunk=limits_.datagram-32;if(count!=(std::uint64_t(total)+chunk-1)/chunk||n!=(index+1<count?chunk:total-std::uint64_t(index)*chunk))Fail("fragment_geometry");
  auto i=pending_.find(id);if(i==pending_.end()){
    if(pending_.size()>=limits_.candidates)Fail("candidate_budget");
    Candidate c;c.started=now;c.total=total;c.count=count;c.parts.resize(count);i=pending_.emplace(id,std::move(c)).first;
  }
  auto &c=i->second;if(c.total!=total||c.count!=count){pending_.erase(i);Fail("fragment_conflict");}
  std::vector<std::uint8_t> part(bytes.begin()+32,bytes.end());auto &slot=c.parts[index];
  if(!slot.empty()){if(slot!=part){pending_.erase(i);Fail("fragment_conflict");}return false;}
  slot=std::move(part);c.bytes+=n;if(c.bytes!=total)return false;
  std::vector<std::uint8_t> wire;wire.reserve(total);for(const auto &p:c.parts)wire.insert(wire.end(),p.begin(),p.end());pending_.erase(i);out=Decode(wire,limits_);return true;
}
bool ModelWindow::Stage(const Message &m){
  Schema(m);SameScope(m.body,scope_);Json target;
  if(m.kind!=Kind::Baseline&&m.kind!=Kind::Delta)Fail("not_state_message");
  // 已接纳目标的副本不再依赖其旧基线；保留原字节而非摘要以识别冲突。
  if(!current_.IsNull()&&m.sequence<=Unsigned(current_["revision"])){
    if(m.kind==Kind::Baseline){if(auto it=baselines_.find(m.sequence);it!=baselines_.end()&&it->second!=m.body)Fail("same_version_conflict");}
    else if(auto it=seen_.find(m.sequence);it!=seen_.end()){if(it->second!=Encode(m))Fail("same_version_conflict");}
    else if(m.sequence==Unsigned(current_["revision"]))Fail("same_version_conflict");
    return false;
  }
  if(m.kind==Kind::Baseline){if(m.body["publishable"]!=true)Fail("incomplete_model");target=Copy(m.body);}
  else if(m.kind==Kind::Delta){auto it=baselines_.find(Unsigned(m.body["base"]));if(it==baselines_.end())Fail("missing_baseline");target=Reconstruct(it->second,m.body);}
  else Fail("not_state_message");
  const auto version=Unsigned(target["revision"]);
  if(auto it=baselines_.find(version);it!=baselines_.end()&&it->second!=target)Fail("same_version_conflict");
  if(!current_.IsNull()&&version<=Unsigned(current_["revision"]))return false;
  // 纯值保留最多两份和64MiB；真实Peer发送窗口／原生apply在21—22接线。
  if(!current_.IsNull()&&ReadScope(target["scope"])!=ReadScope(current_["scope"]))Fail("stale_scope");
  current_=Copy(target);baselines_.emplace(version,std::move(target));seen_[version]=Encode(m);
  while(baselines_.size()>2)baselines_.erase(baselines_.begin());while(seen_.size()>2)seen_.erase(seen_.begin());return true;
}
void EventWindow::Baseline(std::uint64_t watermark){if(watermark<watermark_)Fail("event_watermark_regression");watermark_=watermark;for(auto i=played_.begin();i!=played_.end();)if(i->first<=watermark_)i=played_.erase(i);else ++i;}
EventDecision EventWindow::Accept(const Message &m,std::uint64_t applied,std::uint64_t tick){
  Schema(m);if(m.kind!=Kind::Event)Fail("not_event");if(ReadScope(m.body["scope"])!=scope_)return EventDecision::WrongScene;
  const auto id=Unsigned(m.body["event_id"]);if(id<=watermark_)return EventDecision::Duplicate;
  const auto text=Signature(m);if(auto i=played_.find(id);i!=played_.end()){if(i->second!=text)Fail("event_conflict");return EventDecision::Duplicate;}
  if(tick>Unsigned(m.body["expires_tick"]))return EventDecision::Expired;
  if(applied<Unsigned(m.body["required_state"])||tick<Unsigned(m.body["server_tick"]))return EventDecision::Waiting;
  if(played_.size()>=4096)Fail("event_budget");played_[id]=text;return EventDecision::Ready;
}
bool ControlWindow::Accept(const Message &m,std::uint64_t applied){
  Schema(m);if(m.kind!=Kind::Control)Fail("not_control");SameScope(m.body,scope_);
  const auto text=Signature(m);if(auto i=history_.find(m.sequence);i!=history_.end()){if(i->second!=text)Fail("control_conflict");return false;}
  if(m.sequence<next_)Fail("retired_control");if(m.sequence!=next_||Unsigned(m.body["required_state"])>applied)return false;
  if(next_==UINT64_MAX)Fail("control_exhausted");history_[next_++]=text;while(history_.size()>256)history_.erase(history_.begin());return true;
}
} // namespace cs::replica
