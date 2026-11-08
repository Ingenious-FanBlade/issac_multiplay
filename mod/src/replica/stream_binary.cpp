#include "replica/stream_codec.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <string_view>
#include <charconv>
#include <limits>
#include <iterator>

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
// 版本化名称表在构建时展开为只读数组；首次Hello不解析大目录、不动态分配名称字典。
struct ProtocolName {std::uint64_t code;const char *text;};
constexpr ProtocolName names_by_text[]={
#include "stream_names_by_text.inc"
};
constexpr ProtocolName names_by_code[]={
#include "stream_names_by_code.inc"
};
std::uint64_t NameCode(std::string_view name){
  const auto found=std::lower_bound(std::begin(names_by_text),std::end(names_by_text),name,
    [](const ProtocolName &entry,std::string_view value){return std::string_view(entry.text)<value;});
  return found!=std::end(names_by_text)&&std::string_view(found->text)==name?found->code:0;
}
std::string_view NameText(std::uint64_t code){
  const auto found=std::lower_bound(std::begin(names_by_code),std::end(names_by_code),code,
    [](const ProtocolName &entry,std::uint64_t value){return entry.code<value;});
  if(found==std::end(names_by_code)||found->code!=code)Fail("compact_key_schema");
  return found->text;
}
// 具名闭合结构只省略固定键名/类型标签，所有成员值仍逐项携带；额外/缺失成员走泛型树。
struct ObjectShape {const char *type;std::vector<const char *> keys;};
const std::vector<ObjectShape> &Shapes(){static const std::vector<ObjectShape> shapes={
  {"vector",{"x","y"}},
  {"color",{"R","G","B","A","RO","GO","BO","GetColorize","GetOffset","GetTint"}},
  {"table",{"entries"}},{"table",{"node","entries"}},
  {"table",{"entries","status"}},{"table",{"node","entries","status"}},
  {nullptr,{"fields","type"}},{nullptr,{"fields","pose_runtime","type"}},
  {nullptr,{"key","value"}},{nullptr,{"reason","status"}},
  {nullptr,{"layers","nulls","phase","playing"}},
  {nullptr,{"champion_color","loaded","main","overlay","overlay_first","version"}},
  {"graph_ref",{"node"}},{nullptr,{"R","G","B","A"}},{nullptr,{"x","y"}},{nullptr,{"status"}}
};return shapes;}
unsigned ShapeOf(const Value &v){
  for(unsigned i=0;i<Shapes().size();++i){const auto &shape=Shapes()[i];if(v.MemberCount()!=shape.keys.size()+unsigned(shape.type!=nullptr))continue;
    if(shape.type&&(!v.HasMember("type")||!v["type"].IsString()||v["type"].GetStringLength()!=std::strlen(shape.type)||std::strcmp(v["type"].GetString(),shape.type)))continue;
    if(std::all_of(shape.keys.begin(),shape.keys.end(),[&](const char *key){return v.HasMember(key);}))return i+1;
  }return 0;
}
std::uint64_t Zigzag(std::int64_t n){return (std::uint64_t(n)<<1)^(0ull-std::uint64_t(n<0));}
std::int64_t Unzigzag(std::uint64_t n){return std::bit_cast<std::int64_t>((n>>1)^(0ull-(n&1)));}
bool CanonicalInteger(const Value &v,std::int64_t &n){
  if(v.MemberCount()!=2||!v.HasMember("type")||!v["type"].IsString()||v["type"].GetStringLength()!=5||std::strcmp(v["type"].GetString(),"int64")||!v.HasMember("value")||!v["value"].IsString())return false;
  const auto &value=v["value"];const auto parsed=std::from_chars(value.GetString(),value.GetString()+value.GetStringLength(),n);
  if(parsed.ec!=std::errc{}||parsed.ptr!=value.GetString()+value.GetStringLength())return false;
  char text[32];const auto written=std::to_chars(text,text+sizeof(text),n);return written.ec==std::errc{}&&std::string_view(text,written.ptr-text)==std::string_view(value.GetString(),value.GetStringLength());
}
// 直接验证UTF-8，二进制热路径不借JSON序列化验证字符串。
bool Utf8(std::string_view s){
  for(std::size_t i=0;i<s.size();){const auto a=std::uint8_t(s[i++]);if(a<0x80)continue;
    unsigned n=0;std::uint32_t cp=0,min=0;if(a>=0xc2&&a<=0xdf){n=1;cp=a&31;min=0x80;}else if(a>=0xe0&&a<=0xef){n=2;cp=a&15;min=0x800;}else if(a>=0xf0&&a<=0xf4){n=3;cp=a&7;min=0x10000;}else return false;
    if(n>s.size()-i)return false;while(n--){const auto b=std::uint8_t(s[i++]);if((b&0xc0)!=0x80)return false;cp=(cp<<6)|(b&63);}if(cp<min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return false;
  }return true;
}
struct Writer final {
  Limits limits;std::vector<std::uint8_t> bytes;std::uint64_t nodes=0;
  // 失败诊断只计实际紧凑编码长度，不分配报文，也不扩大正式发送预算。
  bool measure=false;std::uint64_t measured=0;
  void Nodes(std::uint64_t n=1){if(n>limits.nodes-nodes)Fail("compact_node_budget");nodes+=n;}
  void U(std::uint64_t n,unsigned width=8){if(measure){measured+=width;return;}if(width>limits.bytes-bytes.size())Fail("compact_byte_budget");for(unsigned i=0;i<width;++i)bytes.push_back(std::uint8_t(n>>(8*i)));}
  void Var(std::uint64_t n){do{const auto b=std::uint8_t(n&127);n>>=7;U(b|(n?128:0),1);}while(n);}
  void String(std::string_view s){if(s.size()>limits.string_bytes||s.size()>UINT32_MAX||!Utf8(s))Fail("compact_string");U(s.size(),4);if(measure){measured+=s.size();return;}if(s.size()>limits.bytes-bytes.size())Fail("compact_byte_budget");bytes.insert(bytes.end(),s.begin(),s.end());}
  void Name(std::string_view name){if(name.size()>limits.string_bytes||!Utf8(name))Fail("compact_string");const auto code=NameCode(name);Var(code);if(!code)String(name);}
  void Key(SubjectKey k){Var(k.run);Var(k.id);U(k.space,1);}
  void World(WorldScope w){for(auto n:{w.session,w.run,w.floor,w.dimension,w.room,w.scene})Var(n);}
  void Versions(const std::vector<Version> &v){if(v.size()>64)Fail("compact_versions");U(v.size(),1);for(const auto &x:v){U(unsigned(x.id.kind),1);Name(x.id.name);Key(x.id.body);Var(x.revision);}}
  void MemberValue(const Member &m){Key(m.key);Var(m.use);Var(m.shape);Name(m.category);}
  void Tree(const Value &v,unsigned depth=0){
    if(depth>limits.depth)Fail("compact_depth");Nodes();
    if(v.IsNull())U(0,1);else if(v.IsBool())U(v.GetBool()?2:1,1);
    else if(v.IsUint64()){const auto n=v.GetUint64();if(n<=1)U(9+unsigned(n),1);else{U(3,1);Var(n);}}
    else if(v.IsInt64()){const auto n=v.GetInt64();if(n>=-1&&n<=1)U(n<0?16:14+unsigned(n),1);else{U(4,1);Var(Zigzag(n));}}
    else if(v.IsDouble()){const auto n=v.GetDouble();if(!std::isfinite(n))Fail("compact_nonfinite");
      if(n==0)U(std::signbit(n)?12:11,1);else if(n==1)U(13,1);
      else if(n>=-double(std::numeric_limits<float>::max())&&n<=double(std::numeric_limits<float>::max())){const auto single=float(n);if(double(single)==n){U(17,1);U(std::bit_cast<std::uint32_t>(single),4);}else{U(5,1);U(std::bit_cast<std::uint64_t>(n));}}
      else{U(5,1);U(std::bit_cast<std::uint64_t>(n));}
    }
    else if(v.IsString()){U(21,1);Name({v.GetString(),v.GetStringLength()});}
    else if(v.IsArray()){Nodes(v.Size());U(7,1);Var(v.Size());for(const auto &x:v.GetArray())Tree(x,depth+1);}
    else if(v.IsObject()){
      std::int64_t integer=0;
      if(CanonicalInteger(v,integer)){if(depth==limits.depth)Fail("compact_depth");if(limits.string_bytes<5||v["value"].GetStringLength()>limits.string_bytes)Fail("compact_string");Nodes(4);U(18,1);Var(Zigzag(integer));}
      else if(const auto id=ShapeOf(v)){const auto &shape=Shapes()[id-1];Nodes(v.MemberCount());if(shape.type){if(std::strlen(shape.type)>limits.string_bytes||limits.string_bytes<4)Fail("compact_string");Nodes();}U(20,1);Var(id);for(const auto key:shape.keys){if(std::strlen(key)>limits.string_bytes)Fail("compact_string");Tree(v[key],depth+1);}}
      else{Nodes(v.MemberCount());U(8,1);Var(v.MemberCount());for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){Name({i->name.GetString(),i->name.GetStringLength()});Tree(i->value,depth+1);}}
    }
    else Fail("compact_value_type");
  }
  void RangeValue(const RangeData &d){
    World(d.world);U(unsigned(d.id.kind),1);Name(d.id.name);Key(d.id.body);Var(d.revision);U(d.complete,1);U(d.membership_complete,1);
    const auto &s=d.sample;for(auto n:{s.epoch,s.sequence,s.tick,s.outer,s.owner,s.mutation,s.loss,s.last_native_update_tick,s.required_world_sequence,s.binding_revision})Var(n);U(unsigned(s.kind),1);
    Versions(d.dependencies);Versions(d.required);Var(d.native_domains);Var(d.retained_domains);Var(d.event_watermark);
    Nodes(d.members.size()+d.records.size());Var(d.members.size());for(const auto &m:d.members)MemberValue(m);
    Var(d.records.size());for(const auto &r:d.records){MemberValue(r.member);Name(r.layout);Nodes(r.fields.size());Var(r.fields.size());for(const auto &f:r.fields){Var(f.id);U(unsigned(f.status),1);U(f.value.index(),1);switch(f.value.index()){
      case 0:break;case 1:U(std::get<bool>(f.value),1);break;case 2:Var(std::get<std::uint64_t>(f.value));break;case 3:Var(Zigzag(std::get<std::int64_t>(f.value)));break;case 4:U(std::bit_cast<std::uint64_t>(std::get<double>(f.value)));break;case 5:String(std::get<std::string>(f.value));break;case 6:Tree(*std::get<std::shared_ptr<const Json>>(f.value));break;default:Fail("compact_atom_tag");
    }}}
    U(bool(d.components),1);if(d.components)Tree(*d.components);
  }
};
// 自有DocumentTree按SAX结束时一次分配容器；预扫描模拟同一4KiB arena，
// 并单列构造栈/查重集合的并发峰值，不按每个标量/键重复预占最坏容量。
struct TreeArena final : rapidjson::BaseReaderHandler<rapidjson::UTF8<>,TreeArena> {
  std::uint64_t capacity=0,remaining=0,stack=0,peak_stack=0,keys=0,peak_keys=0;
  std::array<std::uint64_t,66> object_keys{};unsigned depth=0;
  void Allocate(std::uint64_t n){if(!n)return;n=(n+7)&~7ull;if(n>remaining){const auto chunk=(std::max)(kJsonArenaChunk,n);capacity+=chunk;remaining=chunk;}remaining-=n;}
  void Push(){stack+=sizeof(Value);peak_stack=(std::max)(peak_stack,stack);}
  bool Null(){Push();return true;}bool Bool(bool){Push();return true;}
  bool Int(int){Push();return true;}bool Uint(unsigned){Push();return true;}
  bool Int64(std::int64_t){Push();return true;}bool Uint64(std::uint64_t){Push();return true;}
  bool Double(double){Push();return true;}
  bool String(const char *,rapidjson::SizeType n,bool copy){if(copy&&n>sizeof(Value)-3)Allocate(std::uint64_t(n)+1);Push();return true;}
  bool Key(const char *s,rapidjson::SizeType n,bool copy){String(s,n,copy);++object_keys[depth-1];++keys;peak_keys=(std::max)(peak_keys,keys);return true;}
  bool StartObject(){if(depth==object_keys.size())Fail("compact_depth");object_keys[depth++]=0;Push();return true;}
  bool StartArray(){return StartObject();}
  bool EndObject(rapidjson::SizeType n){keys-=object_keys[--depth];stack-=std::uint64_t(n)*2*sizeof(Value);Allocate(std::uint64_t(n)*sizeof(Value::Member));return true;}
  bool EndArray(rapidjson::SizeType n){--depth;stack-=std::uint64_t(n)*sizeof(Value);Allocate(std::uint64_t(n)*sizeof(Value));return true;}
  std::uint64_t Owned() const{return capacity+(capacity/kJsonArenaChunk+1)*64+sizeof(Json)+128;}
  // SDK栈按几何容量增长；4倍峰值覆盖旧/新Realloc并存，96字节覆盖每个活跃查重节点及堆头。
  std::uint64_t Temporary() const{return 4*(std::max)(std::uint64_t(1024),peak_stack)+4096+96*peak_keys;}
};
// 第一遍只读/计量；所有计数、深度和长度先验证，第二遍才分配DTO和DOM。
struct Reader final {
  std::span<const std::uint8_t> bytes;Limits limits;bool build=false;std::size_t p=0;std::uint64_t nodes=0,owned_bytes=65536;
  Json arena;
  memory::Lease lease;
  bool tree_scan=false;std::uint64_t tree_temporary=0;
  void Measure(std::uint64_t n){if(tree_scan)return;if(n>limits.value_bytes-owned_bytes)Fail("compact_value_budget");owned_bytes+=n;}
  void Nodes(std::uint64_t n=1){if(n>limits.nodes-nodes)Fail("compact_node_budget");nodes+=n;}
  std::uint64_t U(unsigned width=8){if(width>bytes.size()-p)Fail("compact_truncated");std::uint64_t n=0;for(unsigned i=0;i<width;++i)n|=std::uint64_t(bytes[p++])<<(i*8);return n;}
  // ULEB128只接纳最短形式，最多十字节；先拒绝溢出再做位移。
  std::uint64_t Var(){std::uint64_t out=0;for(unsigned i=0;i<10;++i){const auto b=U(1);if(i==9&&(b&0xfe))Fail("compact_varint_overflow");out|=(b&127)<<(7*i);if(!(b&128)){if(i&&!(b&127))Fail("compact_varint_noncanonical");return out;}}Fail("compact_varint_overflow");}
  bool Bool(){const auto n=U(1);if(n>1)Fail("compact_bool");return n!=0;}
  std::string_view View(){const auto n=U(4);if(n>limits.string_bytes||n>bytes.size()-p)Fail("compact_string_budget");std::string_view v(reinterpret_cast<const char *>(bytes.data()+p),std::size_t(n));p+=std::size_t(n);if(!Utf8(v))Fail("compact_utf8");Measure(2*n+64);return v;}
  std::string String(){const auto s=View();return build?std::string(s):std::string{};}
  std::string_view NameView(){const auto id=Var();if(!id)return View();const auto name=NameText(id);if(name.size()>limits.string_bytes)Fail("compact_string_budget");Measure(2*name.size()+64);return name;}
  std::string NamedString(){const auto s=NameView();return build?std::string(s):std::string{};}
  unsigned Count(unsigned max,std::size_t minimum,std::size_t item){const auto n=U(4);if(n>max||n>(bytes.size()-p)/minimum)Fail("compact_count");Nodes(n);Measure(n*item*2);return unsigned(n);}
  unsigned VarCount(unsigned max,std::size_t minimum,std::size_t item){const auto n=Var();if(n>max||n>(bytes.size()-p)/minimum)Fail("compact_count");Nodes(n);Measure(n*item*2);return unsigned(n);}
  unsigned TreeCount(unsigned max,std::size_t item){const auto n=Var();if(n>max||n>bytes.size()-p)Fail("compact_count");Nodes(n);Measure(n*item*2);return unsigned(n);}
  SubjectKey Key(){const auto run=Var(),id=Var(),space=U(1);if((!run||!id||space<1||space>14)&&(run||id||space))Fail("compact_key");return {run,id,unsigned(space)};}
  WorldScope World(){WorldScope w;w.session=Var();w.run=Var();w.floor=Var();w.dimension=Var();w.room=Var();w.scene=Var();return w;}
  std::vector<Version> Versions(){const auto n=U(1);if(n>64)Fail("compact_versions");Measure(n*sizeof(Version)*2);std::vector<Version> out;if(build)out.reserve(std::size_t(n));for(unsigned i=0;i<n;++i){const auto kind=U(1);auto name=NamedString();auto key=Key();const auto revision=Var();if(kind<1||kind>6||!revision)Fail("compact_version");if(build)out.push_back({{RangeKind(kind),std::move(name),key},revision});}return out;}
  Member MemberValue(){auto k=Key();const auto use=Var(),shape=Var();auto category=NamedString();return {k,use,shape,std::move(category)};}
  // SAX构造让对象/数组按最终成员数分配，避免每个小对象默认预留16个成员槽。
  template<class Handler> Value Emit(Value v,Handler *handler){if(handler){if(!v.Accept(*handler))Fail("compact_tree_handler");return Value();}return v;}
  template<class Handler=Json> Value Tree(unsigned depth=0,Handler *handler=nullptr){
    if(depth>limits.depth)Fail("compact_depth");Nodes();Measure(64);auto &a=arena.GetAllocator();
    switch(U(1)){
      case 0:return Emit(Value(),handler);case 1:return Emit(Value(false),handler);case 2:return Emit(Value(true),handler);case 3:return Emit(Value(Var()),handler);case 4:return Emit(Value(Unzigzag(Var())),handler);case 5:{const auto d=std::bit_cast<double>(U());if(!std::isfinite(d))Fail("compact_nonfinite");return Emit(Value(d),handler);}
      case 6:{const auto s=View();if(handler){handler->String(s.data(),rapidjson::SizeType(s.size()),true);return Value();}return build?Value(s.data(),rapidjson::SizeType(s.size()),a):Value();}
      case 7:{const auto n=TreeCount(limits.nodes,sizeof(Value));Value v(rapidjson::kArrayType);if(handler)handler->StartArray();else if(build)v.Reserve(n,a);for(unsigned i=0;i<n;++i){auto x=Tree(depth+1,handler);if(build&&!handler)v.PushBack(x,a);}if(handler)handler->EndArray(n);return v;}
      case 8:{const auto n=TreeCount(limits.nodes/2,sizeof(Value::Member));Value v(rapidjson::kObjectType);std::set<std::string_view> names;if(handler)handler->StartObject();for(unsigned i=0;i<n;++i){const auto name=NameView();if(handler){if(build&&!names.insert(name).second)Fail("compact_duplicate_key");handler->Key(name.data(),rapidjson::SizeType(name.size()),true);}auto x=Tree(depth+1,handler);if(build&&!handler){Value key(name.data(),rapidjson::SizeType(name.size()),a);if(v.HasMember(key))Fail("compact_duplicate_key");v.AddMember(key,x,a);}}if(handler)handler->EndObject(n);return v;}
      case 9:return Emit(Value(std::uint64_t(0)),handler);case 10:return Emit(Value(std::uint64_t(1)),handler);
      case 11:return Emit(Value(0.0),handler);case 12:return Emit(Value(-0.0),handler);case 13:return Emit(Value(1.0),handler);
      case 14:return Emit(Value(std::int64_t(0)),handler);case 15:return Emit(Value(std::int64_t(1)),handler);case 16:return Emit(Value(std::int64_t(-1)),handler);
      case 17:{const auto single=std::bit_cast<float>(std::uint32_t(U(4)));if(!std::isfinite(single))Fail("compact_nonfinite");return Emit(Value(double(single)),handler);}
      case 18:{
        if(depth==limits.depth)Fail("compact_depth");
        const auto n=Unzigzag(Var());char text[32];const auto written=std::to_chars(text,text+sizeof(text),n);if(written.ec!=std::errc{})Fail("compact_integer_text");
        const auto length=std::size_t(written.ptr-text);if(limits.string_bytes<5||length>limits.string_bytes)Fail("compact_string_budget");Nodes(4);Measure(2*sizeof(Value::Member)*2+2*64);
        for(const auto key:{"type","value"})Measure(2*std::strlen(key)+64);
        Measure(2*5+64);Measure(2*length+64);Value v(rapidjson::kObjectType);
        if(handler){handler->StartObject();handler->Key("type",4,true);handler->String("int64",5,true);handler->Key("value",5,true);handler->String(text,rapidjson::SizeType(length),true);handler->EndObject(2);}
        else if(build){v.AddMember(Value("type",a),Value("int64",a),a);v.AddMember(Value("value",a),Value(text,rapidjson::SizeType(length),a),a);}return v;
      }
      case 20:{
        const auto id=Var();if(!id||id>Shapes().size())Fail("compact_object_schema");const auto &shape=Shapes()[std::size_t(id-1)];
        const auto count=shape.keys.size()+unsigned(shape.type!=nullptr);Nodes(count);Measure(count*sizeof(Value::Member)*2);Value v(rapidjson::kObjectType);
        if(handler)handler->StartObject();
        if(shape.type){if(std::strlen(shape.type)>limits.string_bytes||limits.string_bytes<4)Fail("compact_string_budget");Nodes();Measure(64+2*4+64+2*std::strlen(shape.type)+64);if(handler){handler->Key("type",4,true);handler->String(shape.type,rapidjson::SizeType(std::strlen(shape.type)),true);}else if(build)v.AddMember(Value("type",a),Value(shape.type,a),a);}
        for(const auto key:shape.keys){if(std::strlen(key)>limits.string_bytes)Fail("compact_string_budget");Measure(2*std::strlen(key)+64);if(handler)handler->Key(key,rapidjson::SizeType(std::strlen(key)),true);auto x=Tree(depth+1,handler);if(build&&!handler)v.AddMember(Value(key,a),x,a);}if(handler)handler->EndObject(rapidjson::SizeType(count));return v;
      }
      case 21:{const auto s=NameView();if(handler){handler->String(s.data(),rapidjson::SizeType(s.size()),true);return Value();}return build?Value(s.data(),rapidjson::SizeType(s.size()),a):Value();}
      default:Fail("compact_value_tag");
    }
  }
  std::shared_ptr<const Json> DocumentTree(){
    if(!build){TreeArena measured;tree_scan=true;Tree(0,&measured);tree_scan=false;Measure(measured.Owned());tree_temporary=(std::max)(tree_temporary,measured.Temporary());return {};}
    auto tree=std::make_shared<Json>();
    auto generate=[&](Json &handler){Tree(0,&handler);return true;};
    tree->Populate(generate);return tree;
  }
  RangeData RangeValue(){
    RangeData d;d.world=World();const auto kind=U(1);if(kind<1||kind>6)Fail("compact_range_kind");d.id.kind=RangeKind(kind);d.id.name=NamedString();d.id.body=Key();d.revision=Var();d.complete=Bool();d.membership_complete=Bool();
    auto &s=d.sample;s.epoch=Var();s.sequence=Var();s.tick=Var();s.outer=Var();s.owner=Var();s.mutation=Var();s.loss=Var();s.last_native_update_tick=Var();s.required_world_sequence=Var();s.binding_revision=Var();const auto capture=U(1);if(capture>1)Fail("compact_capture_kind");s.kind=CaptureKind(capture);
    d.dependencies=Versions();d.required=Versions();d.native_domains=Var();d.retained_domains=Var();d.event_watermark=Var();
    const auto members=VarCount(16384,6,sizeof(Member));if(build)d.members.reserve(members);for(unsigned i=0;i<members;++i){auto m=MemberValue();if(build)d.members.push_back(std::move(m));}
    const auto records=VarCount(16384,8,sizeof(Record));if(build)d.records.reserve(records);for(unsigned i=0;i<records;++i){Record r;r.member=MemberValue();r.layout=NamedString();const auto fields=VarCount(32768,3,sizeof(Field));if(build)r.fields.reserve(fields);for(unsigned j=0;j<fields;++j){Field f;const auto field_id=Var();const auto status=U(1),tag=U(1);if(!field_id||field_id>UINT32_MAX||status>unsigned(FieldStatus::Truncated)||tag>6)Fail("compact_field");f.id=std::uint32_t(field_id);f.status=FieldStatus(status);switch(tag){
      case 0:break;case 1:f.value=Bool();break;case 2:f.value=Var();break;case 3:f.value=Unzigzag(Var());break;case 4:{const auto n=std::bit_cast<double>(U());if(!std::isfinite(n))Fail("compact_nonfinite");f.value=n;break;}case 5:f.value=String();break;case 6:{auto tree=DocumentTree();if(build){if(!tree->IsObject()&&!tree->IsArray())Fail("compact_atom_tree");f.value=std::move(tree);}break;}
    }if(build)r.fields.push_back(std::move(f));}if(build)d.records.push_back(std::move(r));}
    if(Bool()){d.components=DocumentTree();}return d;
  }
};
Value WorldJson(WorldScope w,Allocator &a){Value v(rapidjson::kObjectType);const std::uint64_t ns[]={w.session,w.run,w.floor,w.dimension,w.room,w.scene};unsigned i=0;for(auto k:{"session","run","floor","dimension","room","scene"})Put(v,k,Decimal(ns[i++],a),a);return v;}
Value VersionsJson(const std::vector<Version> &vs,Allocator &a){Value out(rapidjson::kArrayType);for(const auto &v:vs){Value x(rapidjson::kObjectType),key;Put(x,"kind",Decimal(unsigned(v.id.kind),a),a);Put(x,"name",v.id.name,a);if(v.id.body!=SubjectKey{}){key.SetObject();Put(key,"run",Decimal(v.id.body.run,a),a);Put(key,"id",Decimal(v.id.body.id,a),a);Put(key,"space",Decimal(v.id.body.space,a),a);}Put(x,"body",std::move(key),a);Put(x,"revision",Decimal(v.revision,a),a);out.PushBack(x,a);}return out;}
void ReadBody(Reader &r,Message &m){
  if(r.U(2)!=kCompactSchema)Fail("compact_payload_schema");
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
// 实际编码与计量共用同一写入路径，避免按协议最大值预占整个共享工作池。
void WritePayload(Writer &w,const Message &m){
  w.U(kCompactSchema,2);if(m.kind==Kind::Range){if(m.range)w.RangeValue(*m.range);else w.RangeValue(RangeDataFromMessage(m));}
  else for(const auto &slot:Schema(m.kind)){w.Nodes();const auto &v=At(m.body,slot.name);switch(slot.type){
    case 'u':w.U(Unsigned(v));break;case 'b':w.U(v.GetBool(),1);break;case 's':w.String({v.GetString(),v.GetStringLength()});break;
    case 'w':w.World({Unsigned(v["session"]),Unsigned(v["run"]),Unsigned(v["floor"]),Unsigned(v["dimension"]),Unsigned(v["room"]),Unsigned(v["scene"])});break;
    case 'v':{std::vector<Version> vs;for(const auto &x:v.GetArray()){SubjectKey k;if(!x["body"].IsNull())k={Unsigned(x["body"]["run"]),Unsigned(x["body"]["id"]),unsigned(Unsigned(x["body"]["space"]))};vs.push_back({{RangeKind(Unsigned(x["kind"])),std::string(x["name"].GetString(),x["name"].GetStringLength()),k},Unsigned(x["revision"])});}w.Versions(vs);break;}
    case 't':w.Tree(v);break;case 'a':for(const auto &x:v.GetArray())w.U(std::bit_cast<std::uint16_t>(std::int16_t(x.GetInt())),2);break;case 'x':for(const auto &x:v.GetArray())w.U(Unsigned(x));break;default:Fail("compact_slot");
  }}
}
}
std::uint64_t MeasureRangePayload(const RangeData &range){Writer w{Limits{}};w.measure=true;w.U(kCompactSchema,2);w.RangeValue(range);return w.measured;}
std::uint64_t MeasurePayload(const Message &m,Limits l){Writer w{l};w.measure=true;WritePayload(w,m);return w.measured;}
std::vector<std::uint8_t> EncodePayload(const Message &m,Limits l,std::uint64_t expected){
  if(!expected)expected=MeasurePayload(m,l);
  if(expected>l.bytes||expected>SIZE_MAX)Fail("compact_byte_budget");
  Writer w{l};w.bytes.reserve(std::size_t(expected));WritePayload(w,m);
  if(w.bytes.size()!=expected)Fail("compact_measured_size");
  return std::move(w.bytes);
}
void DecodePayload(std::span<const std::uint8_t> b,Message &m,Limits l){
  if(b.size()>l.bytes||l.value_bytes<65536)Fail("compact_byte_budget");Reader scan{b,l};ReadBody(scan,m);if(scan.p!=b.size())Fail("compact_trailing_bytes");
  scan.Measure(scan.tree_temporary);
  auto lease=memory::Reserve(memory::Pool::Values,scan.owned_bytes+sizeof(Message),"compact_decode");Reader reader{b,l,true};reader.lease=lease;ReadBody(reader,m);if(reader.p!=b.size())Fail("compact_trailing_bytes");
  // 预扫描额度只保护构造峰值；完成后共享租约只保留实际拥有的容量，不能永久占用最坏上界。
  const auto retained=(m.range?Footprint(*m.range):OwnedJsonBytes(m.body))+sizeof(Message)+128;
  if(retained>lease->Bytes())Fail("compact_decoded_footprint");lease->Resize(retained);
  if(!m.range)m.memory=std::move(lease);
}
} // namespace cs::replica::stream
