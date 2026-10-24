// Production-link integration acceptance. Real named Windows mapping and real
// protocol endpoints; controller ring transfer is a single-threaded fixture.
// No game process, Hook installation, UDP, or world simulation is exercised.
// Failure modes were registered in tx-03-06-publication-boundary.md first.
#include "isaac_cs_t0/t09_authority_link.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace p=isaac_cs_t0::protocol;
namespace w=isaac_cs_t0::t09;
namespace c=isaac_cs_t0::coop_active;

namespace {
void Require(bool value,const char* reason) {
    if(!value)throw std::runtime_error(reason);
}
class NewLog final {
public:
    explicit NewLog(const std::filesystem::path& path) {
        file_=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        Require(file_!=INVALID_HANDLE_VALUE,"create-new artifact");
    }
    ~NewLog(){if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);}
    void Write(const std::string& text) {
        DWORD written=0;
        Require(WriteFile(file_,text.data(),static_cast<DWORD>(text.size()),&written,nullptr) &&
            written==text.size(),"artifact write");
    }
private:
    HANDLE file_=INVALID_HANDLE_VALUE;
};
struct Evidence {
    NewLog checks,packets,events;
    unsigned passed=0,failed=0;
    explicit Evidence(const std::filesystem::path& dir):checks(dir/L"checks.jsonl"),
        packets(dir/L"packets.jsonl"),events(dir/L"events.jsonl"){}
    void Check(const std::string& name,bool ok) {
        checks.Write("{\"name\":\""+name+"\",\"pass\":"+(ok?"true":"false")+"}\n");
        if(ok)++passed;else ++failed;
    }
    void Packet(const std::string& name,const char* direction,const p::Packet& packet,
        const p::Message& message,std::uint64_t slot_tick) {
        constexpr char digits[]="0123456789abcdef";
        std::string hex;hex.reserve(packet.size*2);
        for(auto byte:packet.View()){hex+=digits[byte>>4];hex+=digits[byte&15];}
        packets.Write("{\"case\":\""+name+"\",\"direction\":\""+direction+
            "\",\"slotTick\":"+std::to_string(slot_tick)+",\"kind\":"+
            std::to_string(static_cast<unsigned>(message.header.kind))+",\"sequence\":"+
            std::to_string(message.header.sequence)+",\"serverTick\":"+
            std::to_string(message.ack.server_tick)+",\"stateTick\":"+
            std::to_string(message.state.tick)+",\"hex\":\""+hex+"\"}\n");
    }
};
// The controller owns the opposite ends of the A mapping's rings. WireIpc is
// intentionally native-role constrained; the fixture implements only transfer,
// using the same published/token/counter layout as the production Rust bridge.
class Mapping final {
public:
    Mapping(const std::wstring& name,const p::Config& config,const p::Id& nonce) {
        mapping_=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,
            static_cast<DWORD>(w::kWireMappingSize),name.c_str());
        const auto error=GetLastError();
        Require(mapping_ && error!=ERROR_ALREADY_EXISTS,"fresh mapping");
        view_=static_cast<std::uint8_t*>(MapViewOfFile(mapping_,FILE_MAP_ALL_ACCESS,0,0,w::kWireMappingSize));
        Require(view_!=nullptr,"mapping view");
        std::memset(view_,0,w::kWireMappingSize);
        header=new(view_) w::WireHeader{};header->role=1;
        std::memcpy(header->run_nonce,nonce.data(),16);
        std::memcpy(header->session,config.session.data(),16);
        std::memcpy(header->authority_nonce,config.local_nonce.data(),16);
        std::memcpy(header->remote_nonce,config.peer_nonce.data(),16);
        std::memcpy(header->build,config.build.data(),32);
        std::copy(config.clients.begin(),config.clients.end(),header->clients);
        header->run_state=w::kRunBootstrap;
    }
    ~Mapping(){if(view_)UnmapViewOfFile(view_);if(mapping_)CloseHandle(mapping_);}
    bool Read(p::Packet& packet,std::uint64_t& tick) {
        constexpr unsigned lane=1;
        const auto produced=InterlockedCompareExchange64(&header->produced[lane],0,0);
        const auto consumed=InterlockedCompareExchange64(&header->consumed[lane],0,0);
        Require(produced>=consumed && produced-consumed<=static_cast<LONG64>(w::kWireRingCapacity),"controller read bounds");
        if(produced==consumed)return false;
        auto* slot=Slot(lane,static_cast<std::uint64_t>(consumed));
        Require(InterlockedCompareExchange64(&slot->published,0,0)==consumed+1 &&
            slot->token==static_cast<std::uint64_t>(consumed+1) && slot->direction==lane &&
            slot->length>=64 && slot->length<=p::kMaximumPacket,"controller read envelope");
        packet.size=slot->length;std::copy_n(slot->packet,packet.size,packet.bytes.begin());tick=slot->tick;
        MemoryBarrier();InterlockedExchange64(&header->consumed[lane],consumed+1);return true;
    }
    void Write(const p::Packet& packet,std::uint64_t tick) {
        constexpr unsigned lane=0;
        const auto produced=InterlockedCompareExchange64(&header->produced[lane],0,0);
        const auto consumed=InterlockedCompareExchange64(&header->consumed[lane],0,0);
        Require(!header->closed && produced>=consumed && produced-consumed<static_cast<LONG64>(w::kWireRingCapacity),
            "controller write bounds");
        auto* slot=Slot(lane,static_cast<std::uint64_t>(produced));
        InterlockedExchange64(&slot->published,0);slot->token=static_cast<std::uint64_t>(produced+1);
        slot->generation=7;slot->tick=tick;slot->direction=lane;slot->length=static_cast<std::uint32_t>(packet.size);
        std::copy_n(packet.bytes.begin(),packet.size,slot->packet);std::memset(slot->reserved,0,8);
        MemoryBarrier();InterlockedExchange64(&slot->published,produced+1);
        InterlockedExchange64(&header->produced[lane],produced+1);
    }
    w::WireHeader* header=nullptr;
private:
    w::WireSlot* Slot(unsigned lane,std::uint64_t position) {
        return reinterpret_cast<w::WireSlot*>(view_+w::kWireHeaderSize+
            (lane*w::kWireRingCapacity+position%w::kWireRingCapacity)*w::kWireSlotSize);
    }
    HANDLE mapping_=nullptr;std::uint8_t* view_=nullptr;
};
p::Config Config() {
    p::Config value{};value.session.fill(1);value.local_nonce.fill(2);value.peer_nonce.fill(3);
    value.clients={101,202};value.build=p::TextId("tx-03-06-link-integration");return value;
}
struct Admission {
    std::uint64_t count=0,last_sequence=0,last_generation=0;
    static std::uint32_t Admit(const c::InputCmdDraftV2& input,std::uint64_t generation,void* context) noexcept {
        auto& self=*static_cast<Admission*>(context);
        ++self.count;self.last_sequence=input.sequence;self.last_generation=generation;
        return static_cast<std::uint32_t>(c::Decision::Accepted);
    }
};
class Flow final {
public:
    Flow(Evidence& evidence,const char* label,std::uint64_t ticks):out(evidence),name(label),config(Config()),
        mapping_name(L"Local\\IsaacCsT0.Publication."+std::to_wstring(GetCurrentProcessId())+L"."+
            std::wstring(name.begin(),name.end())),mapping(mapping_name,config,Nonce()) {
        Require(link.Initialize(config,mapping_name.c_str(),Nonce(),ticks),"link initialize");
        auto remote_config=config;remote_config.role=p::Role::Remote;
        std::swap(remote_config.local_nonce,remote_config.peer_nonce);
        Require(remote.Start(remote_config)==p::Error::None,"remote start");
        link.MarkGameReady();
    }
    void Handshake() {
        p::Message hello{};hello.header.kind=p::Kind::Hello;Send(hello,0);
        Drain();Require(link.PumpInputs(0,256,Admission::Admit,&admission,7),"handshake pump");
        Drain();Require(link.Established() && remote.State()==p::PeerState::Established,"established pair");
    }
    void Start(){InterlockedExchange(&mapping.header->run_state,w::kRunRunning);}
    bool Publish(std::uint64_t tick,int mismatch=0) {
        p::Ack ack{};ack.server_tick=tick;ack.client_id=202;
        ack.accepted=admission.last_sequence;ack.session_state=c::SessionState::Active;
        p::StateObservation state{};state.tick=tick;state.run=1;state.room=1;
        state.attempt=1;state.source_token=tick;state.status=p::StateStatus::Gap;state.reason=15;
        if(mismatch==1)++ack.server_tick;if(mismatch==2)++state.tick;
        return link.Publish(ack,state,tick);
    }
    void Input(std::uint64_t target,std::uint16_t redundant=0) {
        p::Message input{};input.header.kind=p::Kind::Input;input.input.redundancy_count=redundant;
        for(std::size_t i=0;i<=redundant;++i) {
            auto& command=i==redundant?input.input.command:input.input.redundant[i];
            command.session_id=config.session;command.client_id=202;command.sequence=++input_sequence;
            command.target_tick=target;command.state.move_x=4096;
        }
        Send(input,target);
    }
    bool Pump(std::uint64_t now){return link.PumpInputs(now,256,Admission::Admit,&admission,7);}
    void Drain(unsigned limit=256) {
        for(unsigned i=0;i<limit;++i) {
            p::Packet packet{};std::uint64_t tick=0;if(!mapping.Read(packet,tick))break;
            p::Message message{};Require(remote.Receive(packet.View(),message)==p::Error::None,"remote decode/receive");
            Require(message.header.sequence==++last_sequence,"outbound contiguous sequence");
            out.Packet(name,"authority-to-remote",packet,message,tick);
            if(message.header.kind==p::Kind::Ack && message.ack.purpose==p::AckPurpose::Progress) {
                ack_ticks.push_back(message.ack.server_tick);final_ack=message.ack;
                Require(tick==message.ack.server_tick,"ack envelope tick");
            } else if(message.header.kind==p::Kind::StateHash) {
                state_ticks.push_back(message.state.tick);Require(tick==message.state.tick,"state envelope tick");
            }
        }
    }
    void Check(const char* suffix,bool value){out.Check(name+"."+suffix,value);}
    void Snapshot(const char* phase,std::uint64_t simulated) {
        const auto& h=*mapping.header;
        out.events.Write("{\"case\":\""+name+"\",\"phase\":\""+phase+"\",\"simulatedTick\":"+
            std::to_string(simulated)+",\"heartbeat\":"+std::to_string(h.heartbeat_tick)+
            ",\"start\":"+std::to_string(h.run_start_tick)+",\"end\":"+std::to_string(h.target_tick)+
            ",\"produced\":"+std::to_string(h.produced[1])+",\"consumed\":"+std::to_string(h.consumed[1])+
            ",\"admissions\":"+std::to_string(admission.count)+",\"flags\":"+std::to_string(h.driver_flags)+
            ",\"error\":"+std::to_string(h.driver_error)+",\"closed\":"+std::to_string(h.closed)+"}\n");
    }
    Evidence& out;std::string name;p::Config config;std::wstring mapping_name;Mapping mapping;
    w::AuthorityWireLink link{};p::Endpoint remote{};Admission admission{};
    std::vector<std::uint64_t> ack_ticks,state_ticks;p::Ack final_ack{};
private:
    static p::Id Nonce(){p::Id nonce{};nonce.fill(4);return nonce;}
    void Send(const p::Message& message,std::uint64_t tick) {
        p::Packet packet{};Require(remote.Prepare(message,packet)==p::Error::None,"remote prepare");Commit(packet,tick);
    }
    void Commit(const p::Packet& packet,std::uint64_t tick) {
        p::Message decoded{};Require(p::Decode(packet.View(),decoded)==p::Error::None,"remote packet decode");
        mapping.Write(packet,tick);Require(remote.CommitSent(packet)==p::Error::None,"remote commit");
        out.Packet(name,"remote-to-authority",packet,decoded,tick);
    }
    std::uint64_t last_sequence=0,input_sequence=0;
};
void Normal(Evidence& out,const char* name,std::uint64_t start,std::uint64_t count,std::uint64_t delay) {
    Flow flow(out,name,count);
    flow.Check("before-handshake-no-publication",flow.Publish(start+1) && !flow.link.RunStarted());
    flow.Handshake();flow.Check("before-run-no-publication",flow.Publish(start+1) && !flow.link.RunStarted());
    flow.Start();const auto end=start+count;
    for(std::uint64_t tick=start+1;tick<=end;++tick) {
        if(tick==start+2) {
            flow.Input(tick);flow.Input(end+1);
            Require(flow.Pump(tick-1),"input pump");
            flow.Check("window-generation-and-target-filter",flow.admission.count==1 && flow.admission.last_generation==7);
        }
        Require(flow.Publish(tick),"publication success");flow.Drain();
        Require(flow.mapping.header->heartbeat_tick==static_cast<LONG64>(tick),"per-tick heartbeat");
        Require((flow.mapping.header->driver_flags&w::kDriverTargetReached)==(tick==end?w::kDriverTargetReached:0),
            "no premature completion");
    }
    std::vector<std::uint64_t> expected;for(std::uint64_t tick=start+1;tick<=end;++tick)expected.push_back(tick);
    flow.Check("exact-ack-and-state-window",flow.ack_ticks==expected && flow.state_ticks==expected);
    flow.Check("start-and-end",flow.link.RunStartTick()==start && flow.link.EndTick()==end &&
        flow.mapping.header->run_start_tick==static_cast<LONG64>(start) && flow.mapping.header->target_tick==static_cast<LONG64>(end));
    flow.Check("final-ack",flow.final_ack.server_tick==end && flow.final_ack.accepted==flow.admission.last_sequence);
    flow.Snapshot("target-reached",end);
    const auto produced=flow.mapping.header->produced[1],heartbeat_qpc=flow.mapping.header->driver_heartbeat_qpc;
    const auto admitted=flow.admission.count;
    // Real encoded post-window records include redundant commands and targets
    // both inside and outside the now-closed admission window.
    flow.Input(end,3);flow.Input(end+1);
    Require(flow.Pump(end),"late input drain");
    flow.Check("late-input-drained-without-admission",flow.admission.count==admitted &&
        flow.mapping.header->produced[0]==flow.mapping.header->consumed[0]);
    bool continued=true;
    for(std::uint64_t tick=end+1;tick<=end+delay;++tick)continued=flow.Publish(tick) && continued;
    continued=flow.Publish(end) && continued;flow.Drain();
    flow.Check("continued-simulation-no-new-wire",continued && flow.link.Healthy() &&
        flow.mapping.header->produced[1]==produced && flow.ack_ticks==expected && flow.state_ticks==expected);
    flow.Check("final-heartbeat-frozen",flow.mapping.header->heartbeat_tick==static_cast<LONG64>(end) &&
        flow.mapping.header->driver_heartbeat_qpc==heartbeat_qpc);
    flow.Check("await-stop-remains-open",!flow.mapping.header->closed &&
        flow.mapping.header->run_state==static_cast<LONG>(w::kRunRunning) &&
        flow.mapping.header->driver_flags==static_cast<LONG>(w::kDriverTargetReached|w::kDriverScriptComplete));
    flow.Snapshot("delayed-stop",end+delay);
    InterlockedExchange(&flow.mapping.header->run_state,w::kRunStopRequested);
    flow.link.Retire(end+delay);flow.link.Retire(end+delay+1);
    flow.Check("normal-retire",flow.mapping.header->closed && flow.mapping.header->run_state==static_cast<LONG>(w::kRunStopped) &&
        (flow.mapping.header->driver_state&w::kDriverStopped) && (flow.mapping.header->driver_flags&w::kDriverRestoreComplete) &&
        flow.mapping.header->heartbeat_tick==static_cast<LONG64>(end) && !flow.Publish(end+delay+2));
    flow.Check("no-loss-no-driver-error",!flow.mapping.header->overwritten[0] && !flow.mapping.header->overwritten[1] &&
        !flow.mapping.header->driver_error);
    flow.Snapshot("retired",end+delay);
}
void Invalid(Evidence& out,const char* name,std::uint64_t first,std::uint64_t second,int mismatch=0) {
    Flow flow(out,name,3);flow.Handshake();flow.Start();
    if(second){Require(flow.Publish(first),"valid first publication");flow.Drain();}
    const auto before=flow.mapping.header->produced[1];
    const auto invalid_tick=second?second:first;
    flow.Check("reject-invalid-window",!flow.Publish(invalid_tick,mismatch) && !flow.link.Healthy() &&
        flow.mapping.header->driver_error && !(flow.mapping.header->driver_flags&w::kDriverTargetReached) &&
        flow.mapping.header->produced[1]==before);
    flow.Check("failure-is-terminal",!flow.Publish(20));flow.Snapshot("rejected",invalid_tick);
    flow.link.Retire(invalid_tick);
}
void Saturated(Evidence& out,const char* name,bool state_failure) {
    Flow flow(out,name,65);flow.Handshake();flow.Start();
    for(std::uint64_t tick=1;tick<=64;++tick) {
        Require(flow.Publish(tick),"fill bounded outbound queue");
        if(state_failure && tick==1)flow.Drain(1); // Consume ACK only: leave 127 packets before final pair.
    }
    const auto before=flow.mapping.header->produced[1];
    flow.Check("final-send-fails",!flow.Publish(65) && !flow.link.Healthy());
    flow.Check("no-false-completion",flow.mapping.header->heartbeat_tick==64 &&
        !(flow.mapping.header->driver_flags&w::kDriverTargetReached) &&
        !(flow.mapping.header->driver_flags&w::kDriverScriptComplete));
    flow.Check("ack-or-state-failure-position",flow.mapping.header->produced[1]==before+(state_failure?1:0) &&
        flow.mapping.header->overwritten[1]==1 && flow.mapping.header->closed && flow.mapping.header->driver_error);
    flow.Drain();
    flow.Check("remote-observes-incomplete-final-pair",flow.ack_ticks.size()==(state_failure?65u:64u) &&
        flow.state_ticks.size()==64);
    flow.Check("no-retry-after-send-failure",!flow.Publish(65) && flow.mapping.header->overwritten[1]==1);
    flow.Snapshot("send-failed",65);flow.link.Retire(65);
}
void Lifecycle(Evidence& out) {
    w::AuthorityWireLink dormant{};p::Ack ack{};p::StateObservation state{};
    out.Check("uninitialized.publish-rejected",!dormant.Publish(ack,state,1));
    Flow early(out,"early-stop",100);early.Handshake();early.Start();Require(early.Publish(40),"early tick");early.Drain();
    InterlockedExchange(&early.mapping.header->run_state,w::kRunStopRequested);
    early.Check("no-fill-after-stop",early.Publish(41) && early.ack_ticks.size()==1);
    early.link.Retire(41);
    early.Check("no-false-target",!(early.mapping.header->driver_flags&w::kDriverTargetReached) &&
        early.mapping.header->heartbeat_tick==40 && early.mapping.header->closed);
    early.Snapshot("early-retire",41);
    Flow closed(out,"externally-closed",1);closed.Handshake();closed.Start();Require(closed.Publish(1),"one tick");closed.Drain();
    InterlockedExchange(&closed.mapping.header->closed,1);
    closed.Check("completion-cannot-mask-closed-wire",!closed.Publish(2));closed.Snapshot("external-close",2);
}
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=2)return 2;
    try {
        Evidence out(argv[1]);
        Normal(out,"ten-thousand",4795,10000,512);
        Normal(out,"single-tick",0,1,24);
        Normal(out,"offset-window",900,7,1024);
        Invalid(out,"duplicate-before-end",10,10);
        Invalid(out,"backward-tick",10,9);
        Invalid(out,"skipped-tick",10,12);
        Invalid(out,"ack-tick-mismatch",10,0,1);
        Invalid(out,"state-tick-mismatch",10,0,2);
        Invalid(out,"zero-tick",0,0);
        Invalid(out,"max-tick",UINT64_MAX,0);
        Invalid(out,"end-overflow",UINT64_MAX-1,0);
        Invalid(out,"end-is-max",UINT64_MAX-2,0);
        Saturated(out,"final-ack-ring-full",false);
        Saturated(out,"final-state-ring-full",true);
        Lifecycle(out);
        NewLog result(std::filesystem::path(argv[1])/L"scenario-result.json");
        result.Write("{\"pass\":"+std::string(out.failed?"false":"true")+",\"passed\":"+
            std::to_string(out.passed)+",\"failed\":"+std::to_string(out.failed)+"}\n");
        std::printf("Production publication integration: %u passed, %u failed\n",out.passed,out.failed);
        return out.failed?1:0;
    } catch(const std::exception& error) {
        std::fprintf(stderr,"Publication integration aborted: %s\n",error.what());return 2;
    }
}
