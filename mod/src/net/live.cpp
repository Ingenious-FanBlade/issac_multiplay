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
  Object(m.body,"capabilities",r::Capabilities(),a);N(m.body,"nonce",nonce,a);
  r::Put(m.body,"role",std::string("Client"),a);return m;
}
}
r::Limits Limits() noexcept {
  r::Limits l; l.bytes=65536;l.nodes=4096;l.depth=24;l.string_bytes=8192;
  l.fragments=64;l.candidates=2;l.timeout_ms=1000;return l;
}
std::uint64_t Random() {
  std::uint64_t n=0;
  if (BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&n),sizeof(n),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0 || !n)
    Fail("live_random");
  return n;
}
bool IsPacket(std::span<const std::uint8_t> b) noexcept {
  return b.size()>=4 && b[0]=='I' && b[1]=='C' && b[2]=='R' && (b[3]=='F'||b[3]=='S');
}
Packets Pack(const r::Message &m,std::uint64_t &transfer) {
  if (transfer==UINT64_MAX) Fail("live_transfer_exhausted");
  return r::Fragment(m,++transfer,Limits());
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
}
bool HostPeer::Receive(std::span<const std::uint8_t> bytes,std::uint64_t now,
                       ReceivedInput &out,const HostIdentity &identity) {
  r::Message m;
  if(!assembler_.Receive(bytes,now,m))return false;
  if(m.kind==r::Kind::Hello) {
    const auto nonce=Number(m.body,"nonce");
    if(nonce_ && nonce_!=nonce)Fail("live_nonce_conflict");
    auto agreed=r::Negotiate(r::Capabilities(),At(m.body,"capabilities"));
    // 当前连接控制只实现标准1200字节预算，不能静默违反更小的协商值。
    if(Number(agreed["limits"],"datagram")!=1200 || Number(agreed["limits"],"bytes")<65536 ||
       Number(agreed["limits"],"fragments")<64)Fail("live_transport_budget");
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
  sent_=p;auto values=ProgressValue(p);auto &a=values.GetAllocator();
  N(values,"server_sample_ms",p.server_sample_ms,a);N(values,"server_send_ms",GetTickCount64(),a);
  N(values,"echo_client_ms",echo_client_ms_,a);N(values,"echo_server_ms",echo_server_ms_,a);
  auto packets=Pack(Control(scope_,++control_,"roster",std::move(values)),transfer_);
  if(p.processed && p.processed_tick && p.generation) {
    auto ack=Pack(r::InputProgress(scope_,ordinal_,p.generation,p.processed,p.processed_tick),transfer_);
    packets.insert(packets.end(),std::make_move_iterator(ack.begin()),std::make_move_iterator(ack.end()));
  }
  return packets;
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
  r::Reassembler assembler{Limits()};
  r::Scope scope{};unsigned ordinal=0;
  std::uint64_t nonce=0,server_nonce=0,transfer=0,control=0,last_receive=0,last_send=0,roster_sequence=0;
  std::uint32_t timeout=3000;
  bool welcomed=false,have_progress=false;
  protocol::Ack progress{};
  std::array<Intent,4> history{};unsigned history_size=0;
  ~State(){if(socket!=INVALID_SOCKET)closesocket(socket);if(wsa)WSACleanup();}
  void Emit(Event e) {if(!events.TryPush(e))Fail("live_client_event_queue_full");}
  void SendPackets(const Packets &packets) {
    for(const auto &p:packets) {
      const auto n=send(socket,reinterpret_cast<const char *>(p.data()),int(p.size()),0);
      if(n!=int(p.size())) {
        const auto error=WSAGetLastError();
        if(error==WSAEWOULDBLOCK||error==WSAENOBUFS||error==WSAECONNRESET||error==WSAENETRESET)return;
        Fail("live_client_send");
      }
    }
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
      r::Negotiate(r::Capabilities(),At(m.body,"capabilities"));
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
        fd_set reads;FD_ZERO(&reads);FD_SET(socket,&reads);timeval wait{0,10000};
        const auto ready=select(0,&reads,nullptr,nullptr,&wait);
        if(ready==SOCKET_ERROR)Fail("live_client_select");
        if(ready>0)for(unsigned n=0;n<64;++n) {
          std::array<std::uint8_t,1201> bytes{};const auto count=recv(socket,reinterpret_cast<char *>(bytes.data()),int(bytes.size()),0);
          if(count==SOCKET_ERROR){const auto error=WSAGetLastError();if(error==WSAEWOULDBLOCK)break;
            if(error==WSAECONNRESET||error==WSAENETRESET||error==WSAEMSGSIZE)continue;Fail("live_client_receive");}
          r::Message m;if(assembler.Receive({bytes.data(),std::size_t(count)},GetTickCount64(),m))Receive(m);
        }
      }
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
