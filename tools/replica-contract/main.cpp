// tx-04-15统一E2E：真实11域观测→正式adapter→ICSR→正式解码→独立Rust核对。
// 错误注入基于这些真实模型；不是新建游戏模拟器，也不写单元测试。
#include "replica/codec.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <array>
#include <memory>

namespace r=cs::replica;
using r::Json;using r::Value;
namespace {
unsigned checks=0;
void Require(bool b,const char *why){if(!b)throw std::runtime_error(why);++checks;}
void Reject(const std::function<void()> &fn,const char *why){bool rejected=false;try{fn();}catch(const r::ContractError &){rejected=true;}Require(rejected,why);}
std::string Str(const Value &v){return v.IsString()?std::string(v.GetString(),v.GetStringLength()):std::string{};}
r::Message Message(r::Kind kind,std::uint64_t sequence,const Value &body){r::Message m;m.kind=kind;m.sequence=sequence;m.body=r::Copy(body);return m;}
void Write(const std::filesystem::path &path,const std::vector<std::uint8_t> &bytes){std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char *>(bytes.data()),std::streamsize(bytes.size()));if(!f)throw std::runtime_error("write wire");}
void Roundtrip(const r::Message &m,const std::filesystem::path &root,const std::string &name,std::ofstream &expected){
  auto wire=r::Encode(m);auto decoded=r::Decode(wire);Require(r::Encode(decoded)==wire,"canonical roundtrip");Require(decoded.body==m.body,"typed value roundtrip");
  Write(root/(name+".icsr"),wire);expected<<"{\"file\":\""<<name<<".icsr\",\"kind\":"<<unsigned(m.kind)<<",\"sequence\":\""<<m.sequence<<"\",\"body\":"<<r::Text(m.body)<<"}\n";
}
void Revision(Json &out,std::uint64_t revision){
 auto &a=out.GetAllocator();r::Put(out,"revision",r::Decimal(revision,a),a);
 for(auto i=out["components"].MemberBegin();i!=out["components"].MemberEnd();++i)r::Put(i->value,"version",r::Decimal(revision,a),a);
 auto dependencies=[&](auto &&self,Value &v)->void{if(v.IsArray())for(auto &x:v.GetArray())self(self,x);if(v.IsObject()){if(v.HasMember("required_revision"))r::Put(v,"required_revision",r::Decimal(revision,a),a);for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)self(self,i->value);}};dependencies(dependencies,out["components"]);
}
Json With(const Value &base,std::uint64_t revision,const char *field,Value value){auto out=r::Copy(base);Revision(out,revision);auto &a=out.GetAllocator();r::Put(out["components"]["entity"]["state"],field,std::move(value),a);return out;}
void ProtocolCases(const Json &real,const r::Scope &scope,const std::filesystem::path &root,std::ofstream &expected){
  auto a=r::Copy(real);auto &aa=a.GetAllocator();Revision(a,100);
  auto b=With(a,101,"e2e_final_value",Value(7));auto c=With(a,102,"e2e_final_value",Value(10));
  r::Put(a["components"]["entity"]["state"],"e2e_final_value",Value(10),aa);
  auto d1=r::Difference(a,b),d2=r::Difference(a,c);Require(r::Reconstruct(a,d1)==b,"delta first final value");Require(r::Reconstruct(a,d2)==c,"immutable baseline value return");
  auto deleted=With(a,103,"e2e_final_value",Value());deleted["components"]["entity"]["state"].RemoveMember("e2e_final_value");
  auto dd=r::Difference(a,deleted);Require(r::Reconstruct(a,dd)==deleted,"final field deletion survives omitted packet");
  auto bm=Message(r::Kind::Baseline,100,a),dm=Message(r::Kind::Delta,102,d2);Roundtrip(bm,root,"baseline-contract",expected);Roundtrip(dm,root,"delta-contract",expected);
  r::ModelWindow window(scope);Require(window.Stage(bm),"stage baseline");Require(window.Stage(Message(r::Kind::Delta,101,d1)),"stage delta");Require(window.Stage(dm),"skip lost delta");Require(window.Current()==c,"rebuilt target");Require(!window.Stage(dm),"duplicate state");Require(!window.Stage(Message(r::Kind::Baseline,101,b)),"old state");
  auto conflict=With(c,102,"e2e_final_value",Value(12));Reject([&]{window.Stage(Message(r::Kind::Baseline,102,conflict));},"same version conflict");
  r::ModelWindow empty(scope);Reject([&]{empty.Stage(dm);},"unknown baseline");
  auto wrong=r::Copy(a);r::Put(wrong["scope"],"scene",r::Decimal(scope.scene+1,wrong.GetAllocator()),wrong.GetAllocator());Reject([&]{window.Stage(Message(r::Kind::Baseline,100,wrong));},"old scene");
  auto partial=r::Copy(a);r::Put(partial,"publishable",Value(false),partial.GetAllocator());Reject([&]{r::Difference(a,partial);},"partial enumeration");
  auto parts=r::Fragment(bm,500);r::Reassembler reassembly;r::Message assembled;bool done=false;
  Require(!reassembly.Receive(parts.back(),1,assembled),"fragment partial");Require(!reassembly.Receive(parts.back(),2,assembled),"fragment duplicate");
  for(std::size_t i=parts.size()-1;i-->0;)if(reassembly.Receive(parts[i],3,assembled))done=true;
  Require(done&&assembled.body==bm.body,"reverse fragmented model");
  r::Reassembler lost;lost.Receive(parts.front(),1,assembled);lost.Expire(6000);Require(lost.Pending()==0,"fragment timeout");
  r::Reassembler bad;bad.Receive(parts.front(),1,assembled);auto corrupt=parts.front();corrupt.back()^=1;Reject([&]{bad.Receive(corrupt,2,assembled);},"conflicting fragment");
  r::Limits small;small.candidates=1;r::Reassembler full(small);full.Receive(parts.front(),1,assembled);auto other=r::Fragment(bm,501);Reject([&]{full.Receive(other.front(),2,assembled);},"candidate budget");
  auto bytes=r::Encode(bm);
  for(auto offset:{0u,4u,6u,7u,8u,10u,12u}){auto invalid=bytes;invalid[offset]=255;Reject([&]{r::Decode(invalid);},"header malformed");}
  auto truncated=bytes;truncated.pop_back();Reject([&]{r::Decode(truncated);},"wire truncation");auto trailing=bytes;trailing.push_back(0);Reject([&]{r::Decode(trailing);},"wire trailing bytes");
  r::Limits tiny;tiny.bytes=64;Reject([&]{r::Encode(bm,tiny);},"wire budget");
  Reject([]{r::Parse("{\"x\":1,\"x\":2}");},"duplicate JSON members");Reject([]{r::Parse("{\"x\":1e9999}");},"nonfinite");
  Reject([]{r::Parse(std::string(70,'[')+"0"+std::string(70,']'));},"depth before allocation");
  auto caps=r::Capabilities();auto negotiated=r::Negotiate(caps,caps);Require(negotiated["native_appliers"].Empty(),"no false applier claim");
  auto incompatible=r::Copy(caps);incompatible["protocol"].SetUint(99);Reject([&]{r::Negotiate(caps,incompatible);},"version negotiation");
  incompatible=r::Copy(caps);incompatible["required"].PushBack(Value("unknown",incompatible.GetAllocator()),incompatible.GetAllocator());Reject([&]{r::Negotiate(caps,incompatible);},"unknown required capability");
  incompatible=r::Copy(caps);r::Put(incompatible["resources"],"base","other-version",incompatible.GetAllocator());Reject([&]{r::Negotiate(caps,incompatible);},"resource mismatch");
  Json greeting(rapidjson::kObjectType);auto &ga=greeting.GetAllocator();Value advertised;advertised.CopyFrom(caps,ga);r::Put(greeting,"capabilities",std::move(advertised),ga);r::Put(greeting,"nonce",r::Decimal(1001,ga),ga);r::Put(greeting,"role","Client",ga);Roundtrip(Message(r::Kind::Hello,1,greeting),root,"hello",expected);
  Json welcome(rapidjson::kObjectType);auto &wa=welcome.GetAllocator();Value agreed;agreed.CopyFrom(negotiated,wa);r::Put(welcome,"capabilities",std::move(agreed),wa);r::Put(welcome,"scope",r::ScopeValue(scope,wa),wa);r::Put(welcome,"client_nonce",r::Decimal(1001,wa),wa);r::Put(welcome,"server_nonce",r::Decimal(2002,wa),wa);r::Put(welcome,"participant",Value(3),wa);r::Put(welcome,"role","Authority",wa);Roundtrip(Message(r::Kind::Welcome,2,welcome),root,"welcome",expected);
  auto invalid_caps=r::Copy(caps);invalid_caps["required"].SetObject();Reject([&]{r::Negotiate(caps,invalid_caps);},"malformed capabilities");
  auto input=r::InputProgress(scope,3,9,123,456);Roundtrip(input,root,"input-ack",expected);Reject([&]{r::Applied(scope,100,0,false);},"no applied ack before native commit");Roundtrip(r::Applied(scope,100,0,true),root,"applied-ack",expected);
  cs::core::InputStateDraftV2 intent;intent.move_x=32767;intent.aim_y=-32767;intent.held_flags=1;Roundtrip(r::InputIntent(scope,3,9,124,458,intent),root,"input-intent",expected);intent.aim_y=0;Reject([&]{r::InputIntent(scope,3,9,125,459,intent);},"shared canonical semantics");
  Json event(rapidjson::kObjectType);auto &ea=event.GetAllocator();r::Put(event,"scope",r::ScopeValue(scope,ea),ea);r::Put(event,"event_id",r::Decimal(1000,ea),ea);r::Put(event,"server_tick",r::Decimal(10,ea),ea);r::Put(event,"required_state",r::Decimal(100,ea),ea);r::Put(event,"expires_tick",r::Decimal(20,ea),ea);r::Put(event,"kind","sound",ea);
  auto parameters=r::Parse(R"({"id":10,"u64":"18446744073709551615","i64":"-9223372036854775808","bits128":{"low":"18446744073709551615","high":"9223372036854775808"},"statuses":[{"status":"Unknown"},{"status":"Null"},{"status":"Empty"},{"status":"NotApplicable"},{"status":"Unavailable"},{"status":"NotCaptured"},{"status":"ReadFailed"},{"status":"Nonfinite"},{"status":"Truncated"},{"status":"Deleted"}],"empty":[],"missing":{},"null":null,"negative":-9223372036854775808,"unsigned":18446744073709551615,"floating":0.125})");Value owned;owned.CopyFrom(parameters,ea);r::Put(event,"parameters",std::move(owned),ea);
  auto ev=Message(r::Kind::Event,1000,event);Roundtrip(ev,root,"event",expected);r::EventWindow events(scope);
  Require(events.Accept(ev,99,10)==r::EventDecision::Waiting,"event state dependency");Require(events.Accept(ev,100,9)==r::EventDecision::Waiting,"event time dependency");Require(events.Accept(ev,100,10)==r::EventDecision::Ready,"event ready");Require(events.Accept(ev,100,10)==r::EventDecision::Duplicate,"event exactly once");
  r::EventWindow expired(scope);Require(expired.Accept(ev,100,21)==r::EventDecision::Expired,"event expires");expired.Baseline(1000);Require(expired.Accept(ev,100,10)==r::EventDecision::Duplicate,"baseline watermark");
  auto event_conflict=Message(r::Kind::Event,1000,event);r::Put(event_conflict.body["parameters"],"id",Value(11),event_conflict.body.GetAllocator());Reject([&]{events.Accept(event_conflict,100,10);},"conflicting event");
  auto foreign=Message(r::Kind::Event,1000,event);r::Put(foreign.body["scope"],"scene",r::Decimal(scope.scene+1,foreign.body.GetAllocator()),foreign.body.GetAllocator());Require(events.Accept(foreign,100,10)==r::EventDecision::WrongScene,"event scene");
  Json control(rapidjson::kObjectType);auto &ca=control.GetAllocator();r::Put(control,"scope",r::ScopeValue(scope,ca),ca);r::Put(control,"control_id",r::Decimal(1,ca),ca);r::Put(control,"required_state",r::Decimal(100,ca),ca);r::Put(control,"operation","scene_prepare",ca);r::Put(control,"parameters",Value(rapidjson::kObjectType),ca);
  auto cm=Message(r::Kind::Control,1,control);r::ControlWindow controls(scope);Require(!controls.Accept(cm,99),"control cross lane dependency");Require(controls.Accept(cm,100),"ordered control");Require(!controls.Accept(cm,100),"control idempotence");Roundtrip(cm,root,"control",expected);
  auto conflict_control=Message(r::Kind::Control,1,control);r::Put(conflict_control.body,"operation","stop",conflict_control.body.GetAllocator());Reject([&]{controls.Accept(conflict_control,100);},"control conflict");
  Json receipt(rapidjson::kObjectType);auto &ra=receipt.GetAllocator();r::Put(receipt,"scope",r::ScopeValue(scope,ra),ra);r::Put(receipt,"transfer",r::Decimal(500,ra),ra);Value received(rapidjson::kArrayType);received.PushBack(Value(0),ra);r::Put(receipt,"received_parts",std::move(received),ra);Roundtrip(Message(r::Kind::ReceiveAck,3,receipt),root,"receive-ack",expected);
  Json error(rapidjson::kObjectType);auto &er=error.GetAllocator();r::Put(error,"scope",r::ScopeValue(scope,er),er);r::Put(error,"code","ResourceUnavailable",er);r::Put(error,"recoverable",Value(true),er);r::Put(error,"detail","candidate not published;request baseline",er);Roundtrip(Message(r::Kind::Error,4,error),root,"error",expected);
  r::Registry registry(scope.session,scope.run);Json memory(rapidjson::kObjectType);auto &ma=memory.GetAllocator();auto key1=registry.Key(1,"entity","1",ma);auto again=registry.Key(1,"entity","1",ma);auto reuse=registry.Key(1,"entity","2",ma);Require(key1==again&&key1!=reuse,"stable identity and pool use");
  auto entity=r::Parse("{\"space\":1,\"local_id\":\"10\",\"use\":\"1\",\"revision\":\"1\",\"resolution\":\"KnownCurrent\"}");auto first=registry.Project(entity,ma,7);entity["revision"].SetString("2",ma);auto morph=registry.Project(entity,ma,7);Require(first["target"]==morph["target"]&&first["shape_revision"]!=morph["shape_revision"],"morph identity");
  auto pointer=r::Parse("{\"pointer\":123}");Reject([&]{registry.Project(pointer,ma,7);},"pointer leak");
  entity.AddMember("epoch",Value("8",ma),entity.GetAllocator());Reject([&]{registry.Project(entity,ma,7);},"cross epoch reference");
}
}
int main(int argc,char **argv){
  try{
    if(argc!=3)throw std::runtime_error("replica-contract <observation-v2/records.jsonl> <temporary-output>");
    const std::filesystem::path root=argv[2];std::filesystem::create_directories(root);std::ifstream input(argv[1]);if(!input)throw std::runtime_error("capture unavailable");std::ofstream expected(root/"expected.jsonl");
    std::vector<Json> current;std::vector<Json> models;std::vector<Json> independent;std::set<std::string> kinds;std::string window;std::uint64_t lines=0,source_session=0;std::unique_ptr<r::Registry> registry; r::Scope scope;
    std::uint64_t actual_windows=0;
    auto flush=[&]{if(current.empty())return;auto model=r::CaptureRows(scope,actual_windows+1,current,*registry);if(model["components"].MemberCount()==11&&model["publishable"]==true){
      ++actual_windows;if(models.size()==4)models.erase(models.begin()+2);models.push_back(std::move(model));}
      current.clear();};
    std::string line;std::set<std::uint64_t> scopes;
    while(std::getline(input,line)){
      ++lines;auto row=r::Parse(line);const auto &c=row["context"];if(!scopes.insert(r::Unsigned(c["scope"])).second)throw std::runtime_error("duplicate actual scope");
      if(!source_session){source_session=r::Unsigned(c["session"]);scope={source_session,1,1,1,1,1,1};registry=std::make_unique<r::Registry>(source_session,1);}
      const auto schema=Str(row["schema"]);const auto phase=Str(c["phase"]);
      if(phase=="OrdinaryPost"&&schema=="isaac.cs."+std::string(std::array<const char *,12>{"invalid","entity","grid","room","level","exploration","room_save","player","run","random","presentation","mod"}.at(unsigned(r::Unsigned(row["domain"]))))){
        const auto w=r::Text(c["authority_tick"])+r::Text(c["outer"])+r::Text(c["owner"]);if(!window.empty()&&w!=window)flush();window=w;
        current.push_back(std::move(row));
      }else if(phase!="OrdinaryPost"&&kinds.size()<40&&kinds.insert(schema+":"+phase).second)independent.push_back(std::move(row));
    }
    flush();Require(models.size()>=3,"at least three complete actual windows required");
    for(const auto &model:models){const auto revision=r::Unsigned(model["revision"]);Roundtrip(Message(r::Kind::Baseline,revision,model),root,"real-"+std::to_string(revision),expected);}
    unsigned n=0;for(const auto &row:independent){auto message=r::Independent(scope,row,*registry);Roundtrip(message,root,"independent-"+std::to_string(++n),expected);}Require(n>0,"independent scopes required");
    auto delta=r::Difference(models.front(),models.back());Require(r::Reconstruct(models.front(),delta)==models.back(),"actual world delta");Roundtrip(Message(r::Kind::Delta,r::Unsigned(models.back()["revision"]),delta),root,"real-delta",expected);
    ProtocolCases(models.back(),scope,root,expected);
    // 原始窗口错误注入通过正式adapter，不以重新写的投影器代替被验实现。
    auto broken=r::Copy(independent.front());r::Put(broken["context"],"authority_tick",r::Decimal(1,broken.GetAllocator()),broken.GetAllocator());Reject([&]{r::Independent(scope,broken,*registry);},"independent tick contamination");
    const auto &catalog=r::Catalog();Require(catalog["members"].Size()>3000,"full member directory");
    std::cout<<"{\"pass\":true,\"checks\":"<<checks<<",\"source_records\":"<<lines<<",\"actual_windows\":"<<actual_windows<<",\"independent_scopes\":"<<n<<",\"mapped_subjects\":"<<registry->Size()<<",\"catalog_members\":"<<catalog["members"].Size()<<",\"catalog_callbacks\":"<<catalog["callbacks"].Size()<<"}\n";return 0;
  }catch(const std::exception &e){std::cerr<<"tx-04-15 contract E2E: "<<e.what()<<"\n";return 1;}
}
