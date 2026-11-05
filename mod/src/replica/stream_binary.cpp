#include "replica/stream_codec.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <string_view>

namespace cs::replica::stream {
namespace {
[[noreturn]] void Fail(const char *s){throw ContractError(s);}
const Value &At(const Value &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Fail("compact_missing_field");return v[k];}
// 字段表是协议定义，不是运行时子树字典；控制数值不再发送键名/十进制字符串。
struct Slot {const char *name;char type;};
using Slots=std::vector<Slot>;
const Slots &Schema(Kind kind){
  static const std::map<Kind,Slots> all={
    {Kind::Hello,{{"capabilities",'t'},{"nonce",'u'},{"role",'s'}}},
    {Kind::Welcome,{{"capabilities",'t'},{"session",'u'},{"client_nonce",'u'},{"server_nonce",'u'},{"participant",'u'},{"role",'s'}}},
    {Kind::Ready,{{"client_nonce",'u'},{"server_nonce",'u'},{"sent_ms",'u'}}},
    {Kind::ScenePrepare,{{"world",'w'},{"next_route",'u'},{"manifest",'u'},{"generation",'u'},{"native_domains",'u'},{"retained_domains",'u'},{"contract",'s'}}},
    {Kind::ScenePrepared,{{"world",'w'},{"manifest",'u'},{"generation",'u'},{"native_domains",'u'},{"retained_domains",'u'},{"contract",'s'}}},
    {Kind::ReceiveAck,{{"transfer",'u'},{"part_count",'u'},{"window",'u'},{"bitmap",'x'},{"complete",'b'}}},
    {Kind::ContextReady,{{"world",'w'},{"versions",'v'}}},
    {Kind::AppliedAck,{{"world",'w'},{"versions",'v'},{"results",'t'},{"native_domains",'u'},{"retained_domains",'u'},{"event_watermark",'u'}}},
    {Kind::Resync,{{"world",'w'},{"failed_revision",'u'},{"reason",'s'}}},
    {Kind::Cancel,{{"transfer",'u'},{"reason",'s'}}},
    {Kind::OperationAck,{{"operation",'u'},{"operation_sequence",'u'},{"stage",'u'},{"progress_sequence",'u'},{"reason",'s'}}},
    {Kind::Retire,{{"reason",'s'}}},
    {Kind::Stop,{{"reason",'s'}}},{Kind::Disconnect,{{"reason",'s'}}},
    {Kind::Error,{{"code",'s'},{"recoverable",'b'},{"detail",'s'}}},
    {Kind::Roster,{{"eligibility_revision",'u'},{"server_tick",'u'},{"participant",'u'},{"accepted",'u'},{"processed",'u'},{"processed_tick",'u'},{"processed_source",'u'},{"generation",'u'},{"state",'u'},{"roster",'u'},{"rejected",'u'},{"rejection_mask",'u'},{"input_enabled",'b'},{"server_sample_ms",'u'},{"server_send_ms",'u'},{"echo_client_ms",'u'},{"echo_server_ms",'u'}}},
    {Kind::TickProgress,{{"eligibility_revision",'u'},{"server_tick",'u'},{"kind",'u'},{"paused",'b'},{"generation",'u'},{"input_enabled",'b'},{"sample_ms",'u'},{"send_ms",'u'},{"echo_client_ms",'u'},{"echo_server_ms",'u'}}},
    {Kind::InputAck,{{"participant",'u'},{"generation",'u'},{"input_sequence",'u'},{"processed_tick",'u'}}},
    {Kind::Input,{{"participant",'u'},{"generation",'u'},{"input_sequence",'u'},{"target_tick",'u'},{"movement",'a'},{"aim",'a'},{"held",'u'},{"edge",'u'}}}
  };const auto i=all.find(kind);if(i==all.end())Fail("compact_unknown_schema");return i->second;
}
// 原始复杂树保持每个值和显式状态；仅版本固定的常用键有编号，无值/子树去重或继承。
const std::vector<std::string> &Names(){static const std::vector<std::string> names={
  "type","status","value","fields","schema","version","members","subject","key","run","id","space","use","revision","shape","category","position","velocity","x","y","z","r","g","b","a","offset","scale","rotation","frame","animation","overlay","layers","resource_generation","required_revision","component","target","relation","references","parameters","native","layout","source","coverage","reason","name","index","flags","width","height","complete","membership_complete","kind","owner","seed","data","values","entries","Null","Empty"};return names;}
// 直接验证UTF-8，二进制热路径不借JSON序列化验证字符串。
bool Utf8(std::string_view s){
  for(std::size_t i=0;i<s.size();){const auto a=std::uint8_t(s[i++]);if(a<0x80)continue;
    unsigned n=0;std::uint32_t cp=0,min=0;if(a>=0xc2&&a<=0xdf){n=1;cp=a&31;min=0x80;}else if(a>=0xe0&&a<=0xef){n=2;cp=a&15;min=0x800;}else if(a>=0xf0&&a<=0xf4){n=3;cp=a&7;min=0x10000;}else return false;
    if(n>s.size()-i)return false;while(n--){const auto b=std::uint8_t(s[i++]);if((b&0xc0)!=0x80)return false;cp=(cp<<6)|(b&63);}if(cp<min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return false;
  }return true;
}
struct Writer final {
  Limits limits;std::vector<std::uint8_t> bytes;std::uint64_t nodes=0;
  void Nodes(std::uint64_t n=1){if(n>limits.nodes-nodes)Fail("compact_node_budget");nodes+=n;}
  void U(std::uint64_t n,unsigned width=8){if(width>limits.bytes-bytes.size())Fail("compact_byte_budget");for(unsigned i=0;i<width;++i)bytes.push_back(std::uint8_t(n>>(8*i)));}
  void String(std::string_view s){if(s.size()>limits.string_bytes||s.size()>UINT32_MAX||!Utf8(s))Fail("compact_string");U(s.size(),4);if(s.size()>limits.bytes-bytes.size())Fail("compact_byte_budget");bytes.insert(bytes.end(),s.begin(),s.end());}
  void Key(SubjectKey k){U(k.run);U(k.id);U(k.space,1);}
  void World(WorldScope w){for(auto n:{w.session,w.run,w.floor,w.dimension,w.room,w.scene})U(n);}
  void Versions(const std::vector<Version> &v){if(v.size()>64)Fail("compact_versions");U(v.size(),1);for(const auto &x:v){U(unsigned(x.id.kind),1);String(x.id.name);Key(x.id.body);U(x.revision);}}
  void MemberValue(const Member &m){Key(m.key);U(m.use);U(m.shape);String(m.category);}
  void Tree(const Value &v,unsigned depth=0){
    if(depth>limits.depth)Fail("compact_depth");Nodes();
    if(v.IsNull())U(0,1);else if(v.IsBool())U(v.GetBool()?2:1,1);else if(v.IsUint64()){U(3,1);U(v.GetUint64());}else if(v.IsInt64()){U(4,1);U(std::bit_cast<std::uint64_t>(v.GetInt64()));}else if(v.IsDouble()){if(!std::isfinite(v.GetDouble()))Fail("compact_nonfinite");U(5,1);U(std::bit_cast<std::uint64_t>(v.GetDouble()));}
    else if(v.IsString()){U(6,1);String({v.GetString(),v.GetStringLength()});}
    else if(v.IsArray()){Nodes(v.Size());U(7,1);U(v.Size(),4);for(const auto &x:v.GetArray())Tree(x,depth+1);}
    else if(v.IsObject()){Nodes(v.MemberCount());U(8,1);U(v.MemberCount(),4);for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){const std::string_view name(i->name.GetString(),i->name.GetStringLength());const auto &names=Names();const auto at=std::find(names.begin(),names.end(),name);U(at==names.end()?0:1+std::size_t(at-names.begin()),1);if(at==names.end())String(name);Tree(i->value,depth+1);}}
    else Fail("compact_value_type");
  }
  void RangeValue(const RangeData &d){
    World(d.world);U(unsigned(d.id.kind),1);String(d.id.name);Key(d.id.body);U(d.revision);U(d.complete,1);U(d.membership_complete,1);
    const auto &s=d.sample;for(auto n:{s.epoch,s.sequence,s.tick,s.outer,s.owner,s.mutation,s.loss,s.last_native_update_tick,s.required_world_sequence,s.binding_revision})U(n);U(unsigned(s.kind),1);
    Versions(d.dependencies);Versions(d.required);U(d.native_domains);U(d.retained_domains);U(d.event_watermark);
    Nodes(d.members.size()+d.records.size());U(d.members.size(),4);for(const auto &m:d.members)MemberValue(m);
    U(d.records.size(),4);for(const auto &r:d.records){MemberValue(r.member);String(r.layout);Nodes(r.fields.size());U(r.fields.size(),4);for(const auto &f:r.fields){U(f.id,4);U(unsigned(f.status),1);U(f.value.index(),1);switch(f.value.index()){
      case 0:break;case 1:U(std::get<bool>(f.value),1);break;case 2:U(std::get<std::uint64_t>(f.value));break;case 3:U(std::bit_cast<std::uint64_t>(std::get<std::int64_t>(f.value)));break;case 4:U(std::bit_cast<std::uint64_t>(std::get<double>(f.value)));break;case 5:String(std::get<std::string>(f.value));break;case 6:Tree(*std::get<std::shared_ptr<const Json>>(f.value));break;default:Fail("compact_atom_tag");
    }}}
    U(bool(d.components),1);if(d.components)Tree(*d.components);
  }
};
// 第一遍只读/计量；所有计数、深度和长度先验证，第二遍才分配DTO和DOM。
struct Reader final {
  std::span<const std::uint8_t> bytes;Limits limits;bool build=false;std::size_t p=0;std::uint64_t nodes=0,owned_bytes=65536;
  Json arena;
  memory::Lease lease;
  void Measure(std::uint64_t n){if(n>limits.value_bytes-owned_bytes)Fail("compact_value_budget");owned_bytes+=n;}
  void Nodes(std::uint64_t n=1){if(n>limits.nodes-nodes)Fail("compact_node_budget");nodes+=n;}
  std::uint64_t U(unsigned width=8){if(width>bytes.size()-p)Fail("compact_truncated");std::uint64_t n=0;for(unsigned i=0;i<width;++i)n|=std::uint64_t(bytes[p++])<<(i*8);return n;}
  bool Bool(){const auto n=U(1);if(n>1)Fail("compact_bool");return n!=0;}
  std::string_view View(){const auto n=U(4);if(n>limits.string_bytes||n>bytes.size()-p)Fail("compact_string_budget");std::string_view v(reinterpret_cast<const char *>(bytes.data()+p),std::size_t(n));p+=std::size_t(n);if(!Utf8(v))Fail("compact_utf8");Measure(2*n+64);return v;}
  std::string String(){const auto s=View();return build?std::string(s):std::string{};}
  unsigned Count(unsigned max,std::size_t minimum,std::size_t item){const auto n=U(4);if(n>max||n>(bytes.size()-p)/minimum)Fail("compact_count");Nodes(n);Measure(n*item*2);return unsigned(n);}
  SubjectKey Key(){const auto run=U(),id=U(),space=U(1);if((!run||!id||space<1||space>14)&&(run||id||space))Fail("compact_key");return {run,id,unsigned(space)};}
  WorldScope World(){WorldScope w;w.session=U();w.run=U();w.floor=U();w.dimension=U();w.room=U();w.scene=U();return w;}
  std::vector<Version> Versions(){const auto n=U(1);if(n>64)Fail("compact_versions");Measure(n*sizeof(Version)*2);std::vector<Version> out;if(build)out.reserve(std::size_t(n));for(unsigned i=0;i<n;++i){const auto kind=U(1);auto name=String();auto key=Key();const auto revision=U();if(kind<1||kind>6||!revision)Fail("compact_version");if(build)out.push_back({{RangeKind(kind),std::move(name),key},revision});}return out;}
  Member MemberValue(){auto k=Key();const auto use=U(),shape=U();auto category=String();return {k,use,shape,std::move(category)};}
  Value Tree(unsigned depth=0){
    if(depth>limits.depth)Fail("compact_depth");Nodes();Measure(64);auto &a=arena.GetAllocator();
    switch(U(1)){
      case 0:return Value();case 1:return Value(false);case 2:return Value(true);case 3:return Value(U());case 4:return Value(std::bit_cast<std::int64_t>(U()));case 5:{const auto d=std::bit_cast<double>(U());if(!std::isfinite(d))Fail("compact_nonfinite");return Value(d);}
      case 6:{const auto s=View();return build?Value(s.data(),rapidjson::SizeType(s.size()),a):Value();}
      case 7:{const auto n=Count(limits.nodes,1,sizeof(Value));Value v(rapidjson::kArrayType);if(build)v.Reserve(n,a);for(unsigned i=0;i<n;++i){auto x=Tree(depth+1);if(build)v.PushBack(x,a);}return v;}
      case 8:{const auto n=Count(limits.nodes/2,2,sizeof(Value::Member));Value v(rapidjson::kObjectType);for(unsigned i=0;i<n;++i){const auto id=U(1);std::string_view name;if(id){if(id>Names().size())Fail("compact_key_schema");name=Names()[std::size_t(id-1)];}else name=View();Measure(2*name.size()+64);auto x=Tree(depth+1);if(build){Value key(name.data(),rapidjson::SizeType(name.size()),a);if(v.HasMember(key))Fail("compact_duplicate_key");v.AddMember(key,x,a);}}return v;}
      default:Fail("compact_value_tag");
    }
  }
  RangeData RangeValue(){
    RangeData d;d.world=World();const auto kind=U(1);if(kind<1||kind>6)Fail("compact_range_kind");d.id.kind=RangeKind(kind);d.id.name=String();d.id.body=Key();d.revision=U();d.complete=Bool();d.membership_complete=Bool();
    auto &s=d.sample;s.epoch=U();s.sequence=U();s.tick=U();s.outer=U();s.owner=U();s.mutation=U();s.loss=U();s.last_native_update_tick=U();s.required_world_sequence=U();s.binding_revision=U();const auto capture=U(1);if(capture>1)Fail("compact_capture_kind");s.kind=CaptureKind(capture);
    d.dependencies=Versions();d.required=Versions();d.native_domains=U();d.retained_domains=U();d.event_watermark=U();
    const auto members=Count(16384,37,sizeof(Member));if(build)d.members.reserve(members);for(unsigned i=0;i<members;++i){auto m=MemberValue();if(build)d.members.push_back(std::move(m));}
    const auto records=Count(16384,45,sizeof(Record));if(build)d.records.reserve(records);for(unsigned i=0;i<records;++i){Record r;r.member=MemberValue();r.layout=String();const auto fields=Count(32768,6,sizeof(Field));if(build)r.fields.reserve(fields);for(unsigned j=0;j<fields;++j){Field f;f.id=unsigned(U(4));const auto status=U(1),tag=U(1);if(!f.id||status>unsigned(FieldStatus::Truncated)||tag>6)Fail("compact_field");f.status=FieldStatus(status);switch(tag){
      case 0:break;case 1:f.value=Bool();break;case 2:f.value=U();break;case 3:f.value=std::bit_cast<std::int64_t>(U());break;case 4:{const auto n=std::bit_cast<double>(U());if(!std::isfinite(n))Fail("compact_nonfinite");f.value=n;break;}case 5:f.value=String();break;case 6:{auto v=Tree();if(build){if(!v.IsObject()&&!v.IsArray())Fail("compact_atom_tree");auto tree=std::make_shared<Json>();tree->Swap(arena);static_cast<Value &>(*tree).Swap(v);f.value=std::shared_ptr<const Json>(std::move(tree));}Measure(65536);break;}
    }if(build)r.fields.push_back(std::move(f));}if(build)d.records.push_back(std::move(r));}
    if(Bool()){auto v=Tree();if(build){auto tree=std::make_shared<Json>();tree->Swap(arena);static_cast<Value &>(*tree).Swap(v);d.components=std::move(tree);}Measure(65536);}return d;
  }
};
Value WorldJson(WorldScope w,Allocator &a){Value v(rapidjson::kObjectType);const std::uint64_t ns[]={w.session,w.run,w.floor,w.dimension,w.room,w.scene};unsigned i=0;for(auto k:{"session","run","floor","dimension","room","scene"})Put(v,k,Decimal(ns[i++],a),a);return v;}
Value VersionsJson(const std::vector<Version> &vs,Allocator &a){Value out(rapidjson::kArrayType);for(const auto &v:vs){Value x(rapidjson::kObjectType),key;Put(x,"kind",Decimal(unsigned(v.id.kind),a),a);Put(x,"name",v.id.name,a);if(v.id.body!=SubjectKey{}){key.SetObject();Put(key,"run",Decimal(v.id.body.run,a),a);Put(key,"id",Decimal(v.id.body.id,a),a);Put(key,"space",Decimal(v.id.body.space,a),a);}Put(x,"body",std::move(key),a);Put(x,"revision",Decimal(v.revision,a),a);out.PushBack(x,a);}return out;}
void ReadBody(Reader &r,Message &m){
  if(r.U(2)!=1)Fail("compact_payload_schema");
  if(m.kind==Kind::Range){auto d=r.RangeValue();if(r.build){d.memory=r.lease;m.range=std::make_shared<const RangeData>(std::move(d));}return;}
  if(r.build)m.body.SetObject();auto &a=r.arena.GetAllocator();for(const auto &slot:Schema(m.kind)){Value v;r.Nodes();r.Measure(256);switch(slot.type){
    case 'u':{const auto n=r.U();if(r.build)v=Decimal(n,a);break;}case 'b':v.SetBool(r.Bool());break;case 's':{const auto s=r.View();if(r.build)v.SetString(s.data(),rapidjson::SizeType(s.size()),a);break;}
    case 'w':{const auto w=r.World();if(r.build)v=WorldJson(w,a);break;}case 'v':{auto vs=r.Versions();if(r.build)v=VersionsJson(vs,a);break;}case 't':v=r.Tree();break;
    case 'a':case 'x':{v.SetArray();const auto count=slot.type=='a'?2u:4u;for(unsigned i=0;i<count;++i){const auto n=r.U(slot.type=='a'?2:8);if(r.build){if(slot.type=='a')v.PushBack(Value(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(n)))),a);else v.PushBack(Value(n),a);}}break;}
    default:Fail("compact_slot");
  }if(r.build)Put(m.body,slot.name,std::move(v),a);}
  // 把整个allocator一次转移给消息；任何Value都不得引用Reader析构后的arena。
  if(r.build){Value root;root.Swap(m.body);m.body.Swap(r.arena);static_cast<Value &>(m.body).Swap(root);}
}
}
std::vector<std::uint8_t> EncodePayload(const Message &m,Limits l){
  Writer w{l};w.U(1,2);if(m.kind==Kind::Range){if(m.range)w.RangeValue(*m.range);else w.RangeValue(RangeDataFromMessage(m));}
  else for(const auto &slot:Schema(m.kind)){w.Nodes();const auto &v=At(m.body,slot.name);switch(slot.type){
    case 'u':w.U(Unsigned(v));break;case 'b':w.U(v.GetBool(),1);break;case 's':w.String({v.GetString(),v.GetStringLength()});break;
    case 'w':w.World({Unsigned(v["session"]),Unsigned(v["run"]),Unsigned(v["floor"]),Unsigned(v["dimension"]),Unsigned(v["room"]),Unsigned(v["scene"])});break;
    case 'v':{std::vector<Version> vs;for(const auto &x:v.GetArray()){SubjectKey k;if(!x["body"].IsNull())k={Unsigned(x["body"]["run"]),Unsigned(x["body"]["id"]),unsigned(Unsigned(x["body"]["space"]))};vs.push_back({{RangeKind(Unsigned(x["kind"])),std::string(x["name"].GetString(),x["name"].GetStringLength()),k},Unsigned(x["revision"])});}w.Versions(vs);break;}
    case 't':w.Tree(v);break;case 'a':for(const auto &x:v.GetArray())w.U(std::bit_cast<std::uint16_t>(std::int16_t(x.GetInt())),2);break;case 'x':for(const auto &x:v.GetArray())w.U(Unsigned(x));break;default:Fail("compact_slot");
  }}return std::move(w.bytes);
}
void DecodePayload(std::span<const std::uint8_t> b,Message &m,Limits l){
  if(b.size()>l.bytes||l.value_bytes<65536)Fail("compact_byte_budget");Reader scan{b,l};ReadBody(scan,m);if(scan.p!=b.size())Fail("compact_trailing_bytes");
  auto lease=memory::Reserve(memory::Pool::Values,scan.owned_bytes+sizeof(Message));Reader reader{b,l,true};reader.lease=lease;ReadBody(reader,m);if(reader.p!=b.size())Fail("compact_trailing_bytes");
  if(!m.range)m.memory=std::move(lease);
}
} // namespace cs::replica::stream
