// Winsock先于Windows，网络模块不依赖SDK或runtime。
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include "net/live.h"
#include "net/queue.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace cs::net::live {
namespace r = replica;
namespace s = replica::stream;
namespace {
[[noreturn]] void Fail(const char *reason) { throw r::ContractError(reason); }
std::string String(const r::Value &v) {
  if (!v.IsString()) Fail("live_string");
  return {v.GetString(), v.GetStringLength()};
}
const r::Value &At(const r::Value &v, const char *key) {
  if (!v.IsObject() || !v.HasMember(key)) Fail("live_missing_field");
  return v[key];
}
std::uint64_t Number(const r::Value &v, const char *key) { return r::Unsigned(At(v,key)); }
void N(r::Value &v, const char *key, std::uint64_t x, r::Allocator &a) {
  r::Put(v,key,r::Decimal(x,a),a);
}
// Document的allocator不随子Value转移；跨消息复制到目标allocator，避免悬空值树。
void Object(r::Value &v,const char *key,const r::Value &source,r::Allocator &a) {
  r::Value owned;owned.CopyFrom(source,a);r::Put(v,key,std::move(owned),a);
}
void Reason(Event &e, const char *reason) noexcept {
  strncpy_s(e.reason, reason, _TRUNCATE);
}
// 现阶段真实socket只授权小消息；完整世界预算在I14/I21独立授权，不能由schema能力扩大。
r::Json GameCapabilities(){auto caps=s::Capabilities();const auto l=Limits();caps["limits"]["bytes"].SetUint(l.bytes);caps["limits"]["datagram"].SetUint(l.datagram);caps["limits"]["candidates"].SetUint(l.candidates);caps["limits"]["fragments"].SetUint(l.fragments);caps["limits"]["timeout_ms"].SetUint(l.timeout_ms);return caps;}
r::Limits AgreedLimits(const r::Value &agreed){auto l=Limits();const auto &v=At(agreed,"limits");if(Number(v,"datagram")!=l.datagram||Number(v,"bytes")<l.bytes||Number(v,"fragments")<l.fragments)Fail("live_transport_budget");l.candidates=unsigned((std::min)(std::uint64_t(l.candidates),Number(v,"candidates")));l.timeout_ms=unsigned((std::min)(std::uint64_t(l.timeout_ms),Number(v,"timeout_ms")));return l;}
// roster是主线程快照的完整最终值；零进度合法，不冒充InputAck或AppliedAck。
r::Json ProgressValue(const protocol::Ack &p) {
  r::Json v; v.SetObject(); auto &a=v.GetAllocator();
  N(v,"server_tick",p.server_tick,a); N(v,"participant",p.client_id-1,a);
  N(v,"accepted",p.accepted,a); N(v,"processed",p.processed,a);
  N(v,"processed_tick",p.processed_tick,a); N(v,"processed_source",p.processed_source,a);
  N(v,"generation",p.generation,a); N(v,"state",unsigned(p.session_state),a);
  N(v,"roster",p.roster_mask,a); N(v,"rejected",p.rejected_count,a);
  N(v,"rejection_mask",p.rejection_mask,a); r::Put(v,"input_enabled",r::Value(p.input_enabled),a);return v;
}
protocol::Ack ReadProgress(const r::Value &v, unsigned ordinal) {
  if (!v.IsObject() || v.MemberCount()!=16 || Number(v,"participant")!=ordinal)
    Fail("live_roster_identity");
  protocol::Ack p; p.client_id=ordinal+1;
  if(!At(v,"input_enabled").IsBool())Fail("live_input_eligibility");
  p.input_enabled=v["input_enabled"].GetBool();
  p.server_sample_ms=Number(v,"server_sample_ms");
  p.server_tick=Number(v,"server_tick"); p.accepted=Number(v,"accepted");
  p.processed=Number(v,"processed"); p.processed_tick=Number(v,"processed_tick");
  p.processed_source=Number(v,"processed_source"); p.generation=Number(v,"generation");
  const auto state=Number(v,"state"), mask=Number(v,"roster");
  const auto rejection=Number(v,"rejection_mask");
  if (state>unsigned(core::SessionState::Failed) || mask>15 || !(mask&1) ||
      !(mask&(1u<<ordinal)) || rejection>UINT32_MAX || p.processed>p.accepted ||
      p.processed_tick>p.server_tick || p.processed_source>p.processed)
    Fail("live_roster_range");
  p.session_state=core::SessionState(state);p.roster_mask=unsigned(mask);
  p.rejected_count=Number(v,"rejected");p.rejection_mask=unsigned(rejection);return p;
}
r::Message Hello(std::uint64_t nonce) {
  r::Message m; m.kind=r::Kind::Hello;m.sequence=1;m.body.SetObject();auto &a=m.body.GetAllocator();
  Object(m.body,"capabilities",GameCapabilities(),a);N(m.body,"nonce",nonce,a);
  r::Put(m.body,"role",std::string("Client"),a);return m;
}
}
r::Limits Limits() noexcept {
  return s::HandshakeLimits();
}
std::uint64_t Random() {
  std::uint64_t n=0;
  if (BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&n),sizeof(n),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0 || !n)
    Fail("live_random");
  return n;
}
bool IsPacket(std::span<const std::uint8_t> b) noexcept {
  return b.size()>=4 && b[0]=='I' && b[1]=='C' && b[2]=='R' && (b[3]=='F'||b[3]=='S'||b[3]=='B');
}
Packets Pack(const r::Message &m,std::uint64_t &transfer) {
  if (transfer==UINT64_MAX) Fail("live_transfer_exhausted");
  s::Message wire;wire.sequence=m.sequence;wire.body=r::Copy(m.body);
  if(m.kind==r::Kind::Hello)wire.kind=s::Kind::Hello;
  else {
    const auto scope=r::ReadScope(At(m.body,"scope"));wire.connection=scope.connection;wire.route=1;wire.body.RemoveMember("scope");
    if(m.kind==r::Kind::Welcome){wire.kind=s::Kind::Welcome;N(wire.body,"session",scope.session,wire.body.GetAllocator());}
    else if(m.kind==r::Kind::Input)wire.kind=s::Kind::Input;
    else if(m.kind==r::Kind::InputAck)wire.kind=s::Kind::InputAck;
    else if(m.kind==r::Kind::Error)wire.kind=s::Kind::Error;
    else if(m.kind==r::Kind::Control){
      const auto op=String(At(m.body,"operation"));wire.body=r::Copy(At(m.body,"parameters"));
      if(op=="ready")wire.kind=s::Kind::Ready;else if(op=="roster")wire.kind=s::Kind::Roster;
      else if(op=="disconnect"||op=="stop"){wire.kind=op=="stop"?s::Kind::Stop:s::Kind::Disconnect;if(!wire.body.HasMember("reason"))r::Put(wire.body,"reason",std::string("client_disconnected"),wire.body.GetAllocator());}
      else Fail("live_v2_control_unimplemented");
    } else Fail("live_v2_world_not_connected");
  }
  return s::Fragment(wire,++transfer,Limits());
}
bool Reassembler::Receive(std::span<const std::uint8_t> bytes,std::uint64_t now,r::Message &out){
  // header先限制到握手预算；I13/I21接入正式路由授权后才扩大世界候选。
  s::Message wire;if(!wire_.Receive(bytes,now,wire))return false;
  s::ValidateReceive(wire,authority_?s::Direction::ToAuthority:s::Direction::ToClient,
    wire.kind==s::Kind::Hello||wire.kind==s::Kind::Welcome?s::State::Handshake:s::State::Waiting);
  out.sequence=wire.sequence;out.memory=wire.memory;
  if(wire.kind==s::Kind::Hello){out.kind=r::Kind::Hello;out.body=std::move(wire.body);return true;}
  if(wire.kind==s::Kind::Error&&!scope_.connection){out.kind=r::Kind::Error;out.body=std::move(wire.body);return true;}
  if(wire.kind==s::Kind::Welcome){scope_={Number(wire.body,"session"),wire.connection,1,1,1,1,1};out.kind=r::Kind::Welcome;out.body=std::move(wire.body);out.body.RemoveMember("session");}
  else {
    if(wire.connection!=scope_.connection||wire.route!=1)Fail("live_v2_route_mismatch");
    if(wire.kind==s::Kind::Ready||wire.kind==s::Kind::Roster||wire.kind==s::Kind::Stop||wire.kind==s::Kind::Disconnect){
      const auto op=wire.kind==s::Kind::Ready?"ready":wire.kind==s::Kind::Roster?"roster":wire.kind==s::Kind::Stop?"stop":"disconnect";
      out=Control(scope_,wire.sequence,op,std::move(wire.body));out.memory=std::move(wire.memory);return true;
    }
    out.kind=wire.kind==s::Kind::Input?r::Kind::Input:wire.kind==s::Kind::InputAck?r::Kind::InputAck:wire.kind==s::Kind::Error?r::Kind::Error:r::Kind::Baseline;
    if(out.kind==r::Kind::Baseline)Fail("live_v2_world_not_connected");out.body=std::move(wire.body);
  }
  r::Put(out.body,"scope",r::ScopeValue(scope_,out.body.GetAllocator()),out.body.GetAllocator());return true;
}
r::Message Control(const r::Scope &scope,std::uint64_t sequence,const char *operation,r::Json parameters) {
  if(!sequence)Fail("live_control_exhausted");
  r::Message m;m.kind=r::Kind::Control;m.sequence=sequence;m.body.SetObject();auto &a=m.body.GetAllocator();
  r::Put(m.body,"scope",r::ScopeValue(scope,a),a);N(m.body,"control_id",sequence,a);
  N(m.body,"required_state",0,a);r::Put(m.body,"operation",std::string(operation),a);
  Object(m.body,"parameters",parameters,a);return m;
}
HostPeer::HostPeer(std::uint64_t session,unsigned ordinal):ordinal_(ordinal) {
  // 等待世界期间的scope不冒充原生run；世界事务在后续任务分配新scope。
  scope_={session,Random(),1,1,1,1,1};server_nonce_=Random();
  assembler_.Scope(scope_);
}
bool HostPeer::Receive(std::span<const std::uint8_t> bytes,std::uint64_t now,
                       ReceivedInput &out,const HostIdentity &identity) {
  r::Message m;
  if(!assembler_.Receive(bytes,now,m))return false;
  if(m.kind==r::Kind::Hello) {
    const auto nonce=Number(m.body,"nonce");
    if(nonce_ && nonce_!=nonce)Fail("live_nonce_conflict");
    auto agreed=s::Negotiate(GameCapabilities(),At(m.body,"capabilities"));
    // 当前连接控制只实现标准1200字节预算，不能静默违反更小的协商值。
    assembler_.Configure(AgreedLimits(agreed));assembler_.Bind(scope_);
    nonce_=nonce;welcome_.kind=r::Kind::Welcome;welcome_.sequence=1;welcome_.body.SetObject();
    auto &a=welcome_.body.GetAllocator();Object(welcome_.body,"capabilities",agreed,a);
    r::Put(welcome_.body,"scope",r::ScopeValue(scope_,a),a);
    N(welcome_.body,"client_nonce",nonce_,a);N(welcome_.body,"server_nonce",server_nonce_,a);
    N(welcome_.body,"participant",ordinal_,a);r::Put(welcome_.body,"role",std::string("Authority"),a);
    reply_=true;last_receive_=now;return false;
  }
  if(!nonce_ || r::ReadScope(At(m.body,"scope"))!=scope_)Fail("live_stale_connection");
  if(m.kind==r::Kind::Control) {
    const auto op=String(At(m.body,"operation"));const auto &p=At(m.body,"parameters");
    if(op=="ready") {
      if(!p.IsObject()||p.MemberCount()!=3||Number(p,"client_nonce")!=nonce_||Number(p,"server_nonce")!=server_nonce_)
        Fail("live_ready_identity");
      if(m.sequence<=client_control_)return false;
      client_control_=m.sequence;echo_client_ms_=Number(p,"sent_ms");echo_server_ms_=now;
      established_=true;last_receive_=now;return false;
    }
    if(op=="disconnect"||op=="stop") {stopped_=true;last_receive_=now;return false;}
    Fail("live_client_control");
  }
  if(!established_ || m.kind!=r::Kind::Input || Number(m.body,"participant")!=ordinal_)
    Fail("live_input_identity");
  last_receive_=now;
  // 绑定代次显式携带；过期代次不重新贴当前标签。主线程还有实际绑定/窗口校验。
  ReceivedInput input{};auto &cmd=input.input.command;
  cmd.session_id=identity.session;cmd.client_id=identity.clients[ordinal_];
  cmd.sequence=Number(m.body,"input_sequence");cmd.target_tick=Number(m.body,"target_tick");
  cmd.state.move_x=std::int16_t(m.body["movement"][0].GetInt());cmd.state.move_y=std::int16_t(m.body["movement"][1].GetInt());
  cmd.state.aim_x=std::int16_t(m.body["aim"][0].GetInt());cmd.state.aim_y=std::int16_t(m.body["aim"][1].GetInt());
  cmd.state.held_flags=std::uint32_t(Number(m.body,"held"));cmd.state.edge_flags=std::uint32_t(Number(m.body,"edge"));
  input.packet_sequence=m.sequence;input.received_ms=now;
  input.acknowledged_generation=Number(m.body,"generation");input.acknowledged_state=sent_.session_state;
  input.ack_stamp_known=input.acknowledged_generation==sent_.generation && sent_.generation!=0;
  out=input;return true;
}
Packets HostPeer::Reply() {
  if(!reply_)return {};reply_=false;return Pack(welcome_,transfer_);
}
Packets HostPeer::Progress(const protocol::Ack &p) {
  if(roster_==UINT64_MAX)Fail("live_roster_exhausted");
  sent_=p;auto values=ProgressValue(p);auto &a=values.GetAllocator();
  N(values,"server_sample_ms",p.server_sample_ms,a);N(values,"server_send_ms",GetTickCount64(),a);
  N(values,"echo_client_ms",echo_client_ms_,a);N(values,"echo_server_ms",echo_server_ms_,a);
  auto packets=Pack(Control(scope_,++roster_,"roster",std::move(values)),transfer_);
  if(p.processed && p.processed_tick && p.generation) {
    auto ack=Pack(r::InputProgress(scope_,ordinal_,p.generation,p.processed,p.processed_tick),transfer_);
    packets.insert(packets.end(),std::make_move_iterator(ack.begin()),std::make_move_iterator(ack.end()));
  }
  return s::Bundle(packets);
}
Packets HostPeer::Bye(const char *reason) {
  if(!nonce_)return {};r::Json p;p.SetObject();r::Put(p,"reason",std::string(reason),p.GetAllocator());
  return Pack(Control(scope_,++control_,"disconnect",std::move(p)),transfer_);
}
Packets HostPeer::Reject(const char *reason) {
  r::Message m;m.kind=r::Kind::Error;m.sequence=++control_;m.body.SetObject();auto &a=m.body.GetAllocator();
  r::Put(m.body,"scope",r::ScopeValue(scope_,a),a);r::Put(m.body,"code",std::string(reason),a);
  r::Put(m.body,"recoverable",r::Value(false),a);r::Put(m.body,"detail",std::string("Client negotiation or connection rejected"),a);
  return Pack(m,transfer_);
}

// 每个连接线程自有socket、重组器和历史；跨线程只使用两个固定容量队列与原子标志。
struct Client::State {
  SOCKET socket=INVALID_SOCKET;bool wsa=false;
  std::atomic<bool> stop{false},finished{false};
  SpscQueue<Intent,256> intents;SpscQueue<Event,256> events;
  Reassembler assembler;
  r::Scope scope{};unsigned ordinal=0;
  std::uint64_t nonce=0,server_nonce=0,transfer=0,control=0,last_receive=0,last_send=0,roster_sequence=0;
  std::uint32_t timeout=3000;
  bool welcomed=false,have_progress=false;
  protocol::Ack progress{};
  std::array<Intent,4> history{};unsigned history_size=0;
  struct Queued final {memory::Lease memory;std::vector<std::uint8_t> bytes;};
  std::deque<Queued> pending_send;
  ~State(){if(socket!=INVALID_SOCKET)closesocket(socket);if(wsa)WSACleanup();}
  void Emit(Event e) {if(!events.TryPush(e))Fail("live_client_event_queue_full");}
  void FlushPackets() {
    for(unsigned i=0;i<64&&!pending_send.empty();++i) {const auto &p=pending_send.front().bytes;
      const auto n=send(socket,reinterpret_cast<const char *>(p.data()),int(p.size()),0);
      if(n!=int(p.size())) {
        const auto error=WSAGetLastError();
        if(error==WSAEWOULDBLOCK||error==WSAENOBUFS||error==WSAECONNRESET||error==WSAENETRESET)return;
        Fail("live_client_send");
      }
      pending_send.pop_front();
    }
  }
  void SendPackets(const Packets &packets) {
    if(packets.size()>256-pending_send.size())Fail("live_client_send_queue_full");
    for(const auto &p:packets){if(p.empty()||p.size()>1200)Fail("live_client_datagram_budget");auto lease=memory::Reserve(memory::Pool::Wire,p.size()+sizeof(Queued)+64);pending_send.push_back({std::move(lease),p});}FlushPackets();
  }
  void SendMessage(const r::Message &m){SendPackets(Pack(m,transfer));}
  void Ready() {
    r::Json p;p.SetObject();auto &a=p.GetAllocator();N(p,"client_nonce",nonce,a);N(p,"server_nonce",server_nonce,a);
    N(p,"sent_ms",GetTickCount64(),a);
    SendMessage(Control(scope,++control,"ready",std::move(p)));
  }
  void Receive(const r::Message &m) {
    const auto now=GetTickCount64();
    if(m.kind==r::Kind::Error)Fail(String(At(m.body,"code")).c_str());
    if(m.kind==r::Kind::Welcome) {
      if(Number(m.body,"client_nonce")!=nonce)Fail("live_welcome_nonce");
      const auto next=r::ReadScope(At(m.body,"scope"));const auto index=Number(m.body,"participant");
      const auto server=Number(m.body,"server_nonce");
      if(index<1||index>3||!next.session||!next.connection)Fail("live_welcome_identity");
      if(welcomed&&(scope!=next||ordinal!=index||server_nonce!=server))Fail("live_welcome_conflict");
      auto agreed=s::Negotiate(GameCapabilities(),At(m.body,"capabilities"));assembler.Configure(AgreedLimits(agreed));assembler.Bind(next);
      if(!welcomed) {scope=next;ordinal=unsigned(index);server_nonce=server;welcomed=true;
        Event e;e.type=Event::Type::Welcome;e.scope=scope;e.ordinal=ordinal;Emit(e);}
      last_receive=now;Ready();return;
    }
    if(!welcomed||r::ReadScope(At(m.body,"scope"))!=scope)Fail("live_server_scope");
    if(m.kind==r::Kind::Control) {
      const auto op=String(At(m.body,"operation"));
      if(op=="disconnect"||op=="stop") {Event e;e.type=Event::Type::Stopped;Reason(e,"server_disconnected");Emit(e);stop=true;return;}
      if(op!="roster")Fail("live_server_control");
      if(m.sequence<=roster_sequence)return;
      const auto &parameters=At(m.body,"parameters");auto next=ReadProgress(parameters,ordinal);
      const auto sent=Number(parameters,"server_send_ms"),echo=Number(parameters,"echo_client_ms"),
                 received=Number(parameters,"echo_server_ms");
      // 两台机器的GetTickCount不能直接相减；只减各端自身区间，扣除服务器回显驻留时间。
      if(!echo||echo>now||next.server_sample_ms>sent||received>sent||
         ((sent-received)>(now-echo)&&(sent-received)-(now-echo)>2))
        Fail("live_clock_range");
      if(have_progress && !protocol::AckAdvances(progress,next))Fail("live_progress_regression");
      progress=next;have_progress=true;roster_sequence=m.sequence;last_receive=now;
      Event e;e.type=Event::Type::Progress;e.scope=scope;e.ordinal=ordinal;e.progress=progress;
      e.received_ms=now;e.round_trip_ms=(now-echo)>=(sent-received)?(now-echo)-(sent-received):0;
      e.publication_age_ms=sent-next.server_sample_ms;
      Emit(e);return;
    }
    if(m.kind==r::Kind::InputAck) {
      if(Number(m.body,"participant")!=ordinal)Fail("live_ack_identity");
      Event e;e.type=Event::Type::InputAck;e.scope=scope;e.ordinal=ordinal;
      e.progress.generation=Number(m.body,"generation");e.progress.processed=Number(m.body,"input_sequence");
      e.progress.processed_tick=Number(m.body,"processed_tick");Emit(e);last_receive=now;return;
    }
    // 尚未有原生applier，不接纳world，更不能据解码成功发送Applied ACK。
    Fail("live_unexpected_world_message");
  }
  void Run() noexcept {
    try {
      const auto hello=Hello(nonce);const auto hello_packets=Pack(hello,transfer);
      last_receive=GetTickCount64();
      while(!stop.load(std::memory_order_acquire)) {
        const auto now=GetTickCount64();
        FlushPackets();
        if(now-last_receive>=timeout)Fail("live_peer_timeout");
        if(now-last_send>=250) {if(welcomed)Ready();else SendPackets(hello_packets);last_send=now;}
        Intent intent;
        for(unsigned n=0;n<32&&intents.TryPop(intent);++n) {
          if(!welcomed||!have_progress||!progress.input_enabled||progress.session_state!=core::SessionState::Active||
             !intent.generation||intent.generation!=progress.generation)continue;
          if(history_size && history[history_size-1].generation!=intent.generation)history_size=0;
          if(history_size==history.size()){std::move(history.begin()+1,history.end(),history.begin());--history_size;}
          history[history_size++]=intent;
          // 同一命令原值冗余；重发边沿依靠Host统一命令去重实现exactly-once。
          for(unsigned i=0;i<history_size;++i) {
            const auto &v=history[i];
            if(v.target+8<progress.server_tick)continue;
            SendMessage(r::InputIntent(scope,ordinal,v.generation,v.sequence,v.target,v.state));
          }
        }
        fd_set reads;FD_ZERO(&reads);FD_SET(socket,&reads);timeval wait{0,2000};
        const auto ready=select(0,&reads,nullptr,nullptr,&wait);
        if(ready==SOCKET_ERROR)Fail("live_client_select");
        if(ready>0)for(unsigned n=0;n<64;++n) {
          std::array<std::uint8_t,1201> bytes{};const auto count=recv(socket,reinterpret_cast<char *>(bytes.data()),int(bytes.size()),0);
          if(count==SOCKET_ERROR){const auto error=WSAGetLastError();if(error==WSAEWOULDBLOCK)break;
            if(error==WSAECONNRESET||error==WSAENETRESET||error==WSAEMSGSIZE)continue;Fail("live_client_receive");}
          const std::span<const std::uint8_t> packet(bytes.data(),std::size_t(count));
          if(s::IsBundle(packet)){auto workspace=memory::Reserve(memory::Pool::Workspace,8192);const auto entries=s::Unbundle(packet);for(const auto &entry:entries){if(stop.load(std::memory_order_acquire))break;r::Message m;if(assembler.Receive(entry,GetTickCount64(),m))Receive(m);}}
          else {r::Message m;if(assembler.Receive(packet,GetTickCount64(),m))Receive(m);}
        }
      }
      pending_send.clear();
      if(welcomed) {r::Json p;p.SetObject();auto bye=Pack(Control(scope,++control,"disconnect",std::move(p)),transfer);
        for(unsigned i=0;i<3;++i)SendPackets(bye);}
    } catch(const std::exception &error) {Event e;e.type=Event::Type::Failed;Reason(e,error.what());events.TryPush(e);}
    catch(...) {Event e;e.type=Event::Type::Failed;Reason(e,"live_client_exception");events.TryPush(e);}
    finished.store(true,std::memory_order_release);
  }
  static DWORD WINAPI Entry(void *arg) noexcept {
    std::unique_ptr<std::shared_ptr<State>> owner(static_cast<std::shared_ptr<State> *>(arg));
    auto self=std::move(*owner);owner.reset();self->Run();return 0;
  }
};
Client::~Client(){Stop();if(thread_)CloseHandle(thread_);}
const char *Client::Start(const std::string &ipv4,std::uint16_t port,std::uint32_t timeout_ms) noexcept {
  if(state_||!port||timeout_ms<500||timeout_ms>60000)return "live_configuration";
  try {
    auto s=std::make_shared<State>();s->nonce=Random();s->timeout=timeout_ms;
    WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))return "live_winsock";s->wsa=true;
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(port);
    if(InetPtonA(AF_INET,ipv4.c_str(),&address.sin_addr)!=1||address.sin_addr.s_addr==INADDR_ANY||address.sin_addr.s_addr==INADDR_BROADCAST)
      return "live_ipv4";
    s->socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(s->socket==INVALID_SOCKET)return "live_socket";
    u_long nonblocking=1;if(ioctlsocket(s->socket,FIONBIO,&nonblocking))return "live_nonblocking";
    if(connect(s->socket,reinterpret_cast<const sockaddr *>(&address),sizeof(address)))return "live_connect";
    auto transfer=std::make_unique<std::shared_ptr<State>>(s);
    auto thread=CreateThread(nullptr,0,State::Entry,transfer.get(),0,nullptr);if(!thread)return "live_thread";
    transfer.release();thread_=thread;state_=std::move(s);return "";
  } catch(...) {return "live_allocation";}
}
bool Client::Send(const Intent &v) noexcept {
  return state_&&!state_->stop.load(std::memory_order_acquire)&&!Finished()&&state_->intents.TryPush(v);
}
bool Client::Poll(Event &e) noexcept {return state_&&state_->events.TryPop(e);}
void Client::Stop() noexcept {if(state_)state_->stop.store(true,std::memory_order_release);}
bool Client::Finished() const noexcept {return !state_||state_->finished.load(std::memory_order_acquire);}
} // namespace cs::net::live
