// T0-09-B offline independent endpoint driver. This executable owns exactly
// one production protocol::Endpoint and one Wire IPC v1 endpoint. It is a test
// driver, not a real game/VM result.
#include "isaac_cs_t0/authority_session.hpp"
#include "isaac_cs_t0/coop_input.hpp"
#include "isaac_cs_t0/minimal_protocol.hpp"
#include "isaac_cs_t0/t09_wire_ipc.hpp"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using isaac_cs_t0::coop_active::AuthoritySession;
using isaac_cs_t0::coop_active::Decision;
using isaac_cs_t0::coop_active::InputCmdDraftV2;
using isaac_cs_t0::coop_active::InputStateDraftV2;
using isaac_cs_t0::coop_active::Participant;
using isaac_cs_t0::coop_active::SessionIdentity;
using isaac_cs_t0::coop_active::SessionState;
using isaac_cs_t0::protocol::Ack;
using isaac_cs_t0::protocol::Endpoint;
using isaac_cs_t0::protocol::Error;
using isaac_cs_t0::protocol::Kind;
using isaac_cs_t0::protocol::Message;
using isaac_cs_t0::protocol::Packet;
using isaac_cs_t0::protocol::PeerState;
using isaac_cs_t0::protocol::Role;
using isaac_cs_t0::t09::Direction;
using isaac_cs_t0::t09::OpenConfig;
using isaac_cs_t0::t09::OpenResult;
using isaac_cs_t0::t09::ReadResult;
using isaac_cs_t0::t09::WireIpc;

namespace {
struct Options {
    Role role=Role::Authority;
    std::string mapping,events,build;
    std::array<std::uint8_t,16> session{},run_nonce{},authority_nonce{},remote_nonce{};
    std::array<std::uint64_t,2> clients{};
    std::uint64_t tick_interval_ms=4,lead_ticks=8;
};
struct Field {std::string name,value;};
std::string JsonString(const std::string& value) {
    std::string out="\"";
    constexpr char digits[]="0123456789abcdef";
    for(unsigned char c:value) {
        switch(c) {
        case '"':out+="\\\"";break;case '\\':out+="\\\\";break;case '\b':out+="\\b";break;
        case '\f':out+="\\f";break;case '\n':out+="\\n";break;case '\r':out+="\\r";break;
        case '\t':out+="\\t";break;default:
            if(c<0x20) {out+="\\u00";out+=digits[c>>4];out+=digits[c&15];}
            else out+=static_cast<char>(c);
        }
    }
    out+='"';return out;
}
std::uint64_t Qpc() noexcept {
    LARGE_INTEGER value{};QueryPerformanceCounter(&value);return static_cast<std::uint64_t>(value.QuadPart);
}
std::wstring Wide(const std::string& value) {
    if(value.empty())return {};
    const int count=MultiByteToWideChar(CP_UTF8,0,value.data(),static_cast<int>(value.size()),nullptr,0);
    if(count<=0)return {};
    std::wstring result(static_cast<std::size_t>(count),L'\0');
    if(!MultiByteToWideChar(CP_UTF8,0,value.data(),static_cast<int>(value.size()),result.data(),count))return {};
    return result;
}
class EventLog final {
public:
    ~EventLog(){if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);}
    bool Open(const std::string& path) noexcept {
        const auto wide=Wide(path);if(wide.empty())return false;
        file_=CreateFileW(wide.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,nullptr);
        return file_!=INVALID_HANDLE_VALUE;
    }
    bool Event(const char* name,std::uint64_t tick,
        std::initializer_list<Field> fields={}) noexcept {
        if(file_==INVALID_HANDLE_VALUE)return false;
        std::ostringstream row;
        row<<"{\"name\":"<<JsonString(name)<<",\"qpc\":"<<Qpc()
           <<",\"tick\":"<<tick<<",\"pid\":"<<GetCurrentProcessId();
        for(const auto& field:fields) row<<","<<JsonString(field.name)<<":"<<field.value;
        row<<"}\n";
        const auto text=row.str();
        DWORD written=0;
        return WriteFile(file_,text.data(),static_cast<DWORD>(text.size()),&written,nullptr) &&
            written==text.size();
    }
    bool Flush() noexcept {
        return file_!=INVALID_HANDLE_VALUE && FlushFileBuffers(file_);
    }
private:
    HANDLE file_=INVALID_HANDLE_VALUE;
};
bool ParseHex16(const std::string& text,std::array<std::uint8_t,16>& output) noexcept {
    if(text.size()!=32)return false;
    auto nibble=[](char c)->int {
        if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;
        if(c>='A'&&c<='F')return c-'A'+10;return -1;
    };
    for(std::size_t i=0;i<16;++i) {
        const int high=nibble(text[i*2]),low=nibble(text[i*2+1]);
        if(high<0||low<0)return false;output[i]=static_cast<std::uint8_t>(high*16+low);
    }
    return true;
}
bool ParseU64(const std::string& text,std::uint64_t& value) noexcept {
    if(text.empty())return false;std::uint64_t parsed=0;
    for(char c:text) {
        if(c<'0'||c>'9')return false;
        const auto digit=static_cast<std::uint64_t>(c-'0');
        if(parsed>((std::numeric_limits<std::uint64_t>::max)()-digit)/10)return false;
        parsed=parsed*10+digit;
    }
    value=parsed;return true;
}
bool ParseOptions(int argc,char** argv,Options& options) {
    std::string role;
    for(int i=1;i<argc;i+=2) {
        if(i+1>=argc)return false;const std::string key=argv[i],value=argv[i+1];
        if(key=="--role")role=value;
        else if(key=="--mapping")options.mapping=value;
        else if(key=="--events")options.events=value;
        else if(key=="--build")options.build=value;
        else if(key=="--session") {if(!ParseHex16(value,options.session))return false;}
        else if(key=="--run-nonce") {if(!ParseHex16(value,options.run_nonce))return false;}
        else if(key=="--authority-nonce") {if(!ParseHex16(value,options.authority_nonce))return false;}
        else if(key=="--remote-nonce") {if(!ParseHex16(value,options.remote_nonce))return false;}
        else if(key=="--client-a") {if(!ParseU64(value,options.clients[0]))return false;}
        else if(key=="--client-b") {if(!ParseU64(value,options.clients[1]))return false;}
        else if(key=="--tick-interval-ms") {if(!ParseU64(value,options.tick_interval_ms))return false;}
        else if(key=="--lead-ticks") {if(!ParseU64(value,options.lead_ticks))return false;}
        else return false;
    }
    if(role=="A")options.role=Role::Authority;
    else if(role=="B")options.role=Role::Remote;
    else return false;
    return !options.mapping.empty()&&!options.events.empty()&&!options.build.empty()&&
        options.build.size()<=32&&options.tick_interval_ms>=1&&options.tick_interval_ms<=1000&&
        options.lead_ticks>=1&&options.lead_ticks<=256&&options.clients[0]&&options.clients[1]&&
        options.clients[0]!=options.clients[1];
}
std::array<std::uint8_t,32> BuildText(const std::string& value) noexcept {
    std::array<std::uint8_t,32> output{};std::copy(value.begin(),value.end(),output.begin());return output;
}
bool SendMessage(Endpoint& endpoint,const Message& content,WireIpc& ipc,
    EventLog& log,std::uint64_t tick,const char* event_name,std::uint32_t role) {
    Packet packet{};auto result=endpoint.Prepare(content,packet);
    if(result!=Error::None) {
        log.Event("send-prepare-failed",tick,{{"error",std::to_string(static_cast<unsigned>(result))},
            {"kind",std::to_string(static_cast<unsigned>(content.header.kind))}});
        return false;
    }
    if(!ipc.Write(WireIpc::Outbound(role),packet.View(),ipc.LocalGeneration(),tick,false)) {
        log.Event("send-ipc-failed",tick,{{"length",std::to_string(packet.size)}});return false;
    }
    result=endpoint.CommitSent(packet);
    if(result!=Error::None) {
        log.Event("send-commit-failed",tick,{{"error",std::to_string(static_cast<unsigned>(result))}});return false;
    }
    log.Event(event_name,tick,{{"length",std::to_string(packet.size)},
        {"kind",std::to_string(static_cast<unsigned>(content.header.kind))}});
    return true;
}
bool ProcessAck(const Message& message,std::uint64_t& latest_ack_tick,
    std::uint64_t& latest_accepted,std::uint64_t& latest_processed,EventLog& log,
    std::uint64_t tick,std::uint32_t role) {
    if(message.header.kind!=Kind::Ack)return true;
    if(message.ack.server_tick<latest_ack_tick ||
       message.ack.accepted<latest_accepted || message.ack.processed<latest_processed) {
        log.Event("ack-regression",tick,{{"serverTick",std::to_string(message.ack.server_tick)},
            {"accepted",std::to_string(message.ack.accepted)},
            {"processed",std::to_string(message.ack.processed)}});
        return false;
    }
    latest_ack_tick=message.ack.server_tick;latest_accepted=message.ack.accepted;
    latest_processed=message.ack.processed;
    log.Event("ack-received",tick,{{"role",std::to_string(role)},
        {"serverTick",std::to_string(message.ack.server_tick)},
        {"accepted",std::to_string(message.ack.accepted)},
        {"processed",std::to_string(message.ack.processed)},
        {"purpose",std::to_string(static_cast<unsigned>(message.ack.purpose))}});
    return true;
}
struct InboundState {
    bool peer_hello_received=false,first_ack_received=false;
    std::uint64_t latest_ack_tick=0,latest_accepted=0,latest_processed=0,states_received=0;
};
bool DrainInbound(Endpoint& endpoint,WireIpc& ipc,EventLog& log,
    std::uint64_t tick,std::uint32_t role,InboundState& state,
    std::vector<Message>& newly_received) {
    newly_received.clear();
    for(;;) {
        std::vector<std::uint8_t> packet;std::uint64_t generation=0,packet_tick=0;
        const auto read=ipc.Read(WireIpc::Inbound(role),packet,generation,packet_tick,false);
        if(read==ReadResult::Empty)return true;
        if(read==ReadResult::Error) {
            log.Event("ipc-read-error",tick,{{"role",std::to_string(role)}});return false;
        }
        Message message{};const auto result=endpoint.Receive(packet,message);
        if(result==Error::Duplicate) {
            log.Event("duplicate-packet",tick,{{"length",std::to_string(packet.size())}});continue;
        }
        if(result!=Error::None) {
            log.Event("receive-rejected",tick,{{"error",std::to_string(static_cast<unsigned>(result))},
                {"length",std::to_string(packet.size())}});
            return false;
        }
        log.Event("packet-received",tick,{{"kind",std::to_string(static_cast<unsigned>(message.header.kind))},
            {"sequence",std::to_string(message.header.sequence)},{"length",std::to_string(packet.size())}});
        if(message.header.kind==Kind::Hello) {
            if(state.peer_hello_received)return false;
            state.peer_hello_received=true;
        } else if(message.header.kind==Kind::Ack) {
            if(message.ack.purpose==isaac_cs_t0::protocol::AckPurpose::First) {
                if(state.first_ack_received)return false;
                state.first_ack_received=true;
            }
            if(!ProcessAck(message,state.latest_ack_tick,state.latest_accepted,
                state.latest_processed,log,tick,role))return false;
        } else if(message.header.kind==Kind::StateHash) {
            ++state.states_received;
        } else if(role==2 && message.header.kind==Kind::Input) {
            log.Event("unexpected-input-at-remote",tick);return false;
        }
        newly_received.push_back(message);
    }
}
bool WaitEstablished(Endpoint& endpoint,WireIpc& ipc,EventLog& log,std::uint32_t role,
    InboundState& state) {
    const auto deadline=GetTickCount64()+20000;
    bool hello_sent=false,first_ack_sent=false;
    while(endpoint.State()!=PeerState::Established) {
        if(ipc.Closed()||GetTickCount64()>deadline) {
            log.Event("handshake-timeout",0,{{"endpointState",std::to_string(static_cast<unsigned>(endpoint.State()))}});
            return false;
        }
        std::vector<Message> messages;
        if(!DrainInbound(endpoint,ipc,log,0,role,state,messages))return false;
        if(!hello_sent) {
            Message hello{};hello.header.kind=Kind::Hello;
            if(!SendMessage(endpoint,hello,ipc,log,0,"hello-sent",role))return false;
            hello_sent=true;ipc.SetDriverState(isaac_cs_t0::t09::kDriverHandshake);
        }
        if(role==1 && endpoint.State()==PeerState::Negotiating && state.peer_hello_received && !first_ack_sent) {
            Packet packet{};auto result=endpoint.PrepareFirstAck(packet);
            if(result!=Error::None ||
               !ipc.Write(WireIpc::Outbound(role),packet.View(),ipc.LocalGeneration(),0,false)) {
                log.Event("first-ack-prepare-failed",0,{{"error",std::to_string(static_cast<unsigned>(result))}});
                return false;
            }
            result=endpoint.CommitSent(packet);
            if(result!=Error::None) {
                log.Event("first-ack-commit-failed",0,{{"error",std::to_string(static_cast<unsigned>(result))}});
                return false;
            }
            first_ack_sent=true;log.Event("first-ack-sent",0,{{"length",std::to_string(packet.size)}});
        }
        if(role==2 && state.first_ack_received && endpoint.State()==PeerState::Established)break;
        Sleep(1);
    }
    ipc.SetDriverState(isaac_cs_t0::t09::kDriverEstablished|isaac_cs_t0::t09::kDriverReady);
    log.Event("endpoint-established",0,{{"role",std::to_string(role)},
        {"state",std::to_string(static_cast<unsigned>(endpoint.State()))}});
    return true;
}
bool AdmitInput(const Message& message,Participant& participant,std::uint64_t generation,
    std::uint64_t completed,EventLog& log,std::uint64_t tick,std::uint64_t& accepted) {
    for(std::size_t i=0;i<=message.input.redundancy_count;++i) {
        const auto& command=i==message.input.redundancy_count?message.input.command:message.input.redundant[i];
        isaac_cs_t0::coop_active::EncodedInputCmdV2 encoded{};
        isaac_cs_t0::coop_active::Encode(command,encoded);
        const auto decision=participant.AcceptInput(std::span<const std::uint8_t>(encoded),generation,completed);
        if(decision==Decision::Accepted)++accepted;
        log.Event("input-admission",tick,{{"sequence",std::to_string(command.sequence)},
            {"targetTick",std::to_string(command.target_tick)},
            {"decision",std::to_string(static_cast<unsigned>(decision))},
            {"edgeFlags",std::to_string(command.state.edge_flags)},
            {"redundancy",std::to_string(message.input.redundancy_count)}});
        if(decision==Decision::SequenceConflict || decision==Decision::Invalid ||
           decision==Decision::WrongSession || decision==Decision::WrongClient ||
           decision==Decision::WrongGeneration) return false;
    }
    return true;
}
bool RunAuthority(WireIpc& ipc,EventLog& log,const Options& options,Endpoint& endpoint,
    InboundState& inbound) {
    SessionIdentity identity{options.session,options.clients};
    AuthoritySession session;
    if(!session.Start(identity)||!session.Prepared()) {
        log.Event("authority-session-start-failed",0);return false;
    }
    Participant participant;
    participant.Initialize(identity.session,identity.clients[1],1,&session);
    std::array<int,3> objects{};
    if(!participant.Bind({&objects[0],&objects[1],&objects[2],7})) {
        log.Event("participant-bind-failed",0);return false;
    }
    if(!session.Activate(true,true)) {log.Event("authority-activate-failed",0);return false;}
    std::uint64_t tick=ipc.RunStartTick()+1;
    if(!tick)tick=1;
    const auto target=ipc.TargetTick();
    std::uint64_t accepted=0,edges_processed=0;
    bool target_reached=false;
    for(;;) {
        if(ipc.Closed()||ipc.DriverError()) {
            log.Event("authority-ipc-closed",tick);return false;
        }
        const auto run_state=ipc.RunState();
        if(run_state==isaac_cs_t0::t09::kRunStopRequested||run_state==isaac_cs_t0::t09::kRunStopped)break;
        if(tick<=target) {
            std::vector<Message> messages;
            if(!DrainInbound(endpoint,ipc,log,tick,1,inbound,messages))return false;
            for(const auto& message:messages) {
                if(message.header.kind==Kind::Input) {
                    if(!AdmitInput(message,participant,ipc.LocalGeneration(),tick-1,log,tick,accepted))return false;
                } else if(message.header.kind!=Kind::Ack && message.header.kind!=Kind::StateHash) {
                    log.Event("unexpected-authority-message",tick,{{"kind",std::to_string(static_cast<unsigned>(message.header.kind))}});
                    return false;
                }
            }
            if(!participant.Begin(tick)) {log.Event("authority-begin-failed",tick);return false;}
            if(participant.Current().edge_flags)++edges_processed;
            participant.End(true);
            const auto progress=participant.Progress(tick);
            Ack ack{};
            ack.server_tick=tick;ack.client_id=identity.clients[1];ack.accepted=progress.last_accepted_sequence;
            ack.processed=progress.last_processed_sequence;ack.processed_tick=progress.last_processed_tick;
            ack.processed_source=progress.last_processed_source_sequence;
            ack.generation=progress.last_state_valid?progress.last_state_generation:progress.generation;
            ack.rejected_count=progress.rejected_count;
            for(std::size_t i=3;i<progress.rejections.size();++i)
                if(progress.rejections[i])ack.rejection_mask|=1u<<i;
            ack.session_state=SessionState::Active;ack.purpose=isaac_cs_t0::protocol::AckPurpose::Progress;
            Message ack_message{};ack_message.header.kind=Kind::Ack;ack_message.ack=ack;
            if(!SendMessage(endpoint,ack_message,ipc,log,tick,"ack-sent",1))return false;
            isaac_cs_t0::protocol::StateObservation state{};state.tick=tick;state.run=1;state.room=1;
            state.attempt=1;state.capture=0;state.status=isaac_cs_t0::protocol::StateStatus::Gap;
            state.reason=isaac_cs_t0::protocol::kValuesNotExported;state.source_token=tick;
            Message state_message{};state_message.header.kind=Kind::StateHash;state_message.state=state;
            if(!SendMessage(endpoint,state_message,ipc,log,tick,"state-sent",1))return false;
            if(tick>=target&&!target_reached) {
                target_reached=true;
                ipc.SetDriverFlags(isaac_cs_t0::t09::kDriverTargetReached|isaac_cs_t0::t09::kDriverScriptComplete);
                log.Event("target-reached",tick,{{"accepted",std::to_string(accepted)},
                    {"edgesProcessed",std::to_string(edges_processed)},
                    {"lastAccepted",std::to_string(progress.last_accepted_sequence)},
                    {"lastProcessed",std::to_string(progress.last_processed_sequence)}});
            }
            ipc.SetHeartbeat(tick,Qpc());
            ++tick;
            Sleep(static_cast<DWORD>(options.tick_interval_ms));
        } else {
            std::vector<Message> messages;
            if(!DrainInbound(endpoint,ipc,log,tick-1,1,inbound,messages))return false;
            Sleep(1);
        }
    }
    std::vector<Message> tail;
    DrainInbound(endpoint,ipc,log,tick,1,inbound,tail);
    const auto before=session.Status();
    if(!session.BeginRetirement(isaac_cs_t0::coop_active::SessionCause::Stop) ||
       !session.CompleteRetirement(true)) {
        log.Event("authority-retire-failed",tick,{{"state",std::to_string(static_cast<unsigned>(before.state))}});
        return false;
    }
    endpoint.Retire();
    ipc.SetDriverFlags(isaac_cs_t0::t09::kDriverRestoreComplete);
    ipc.SetDriverState(isaac_cs_t0::t09::kDriverStopped);
    ipc.SetRunState(isaac_cs_t0::t09::kRunStopped);
    log.Event("endpoint-retired",tick,{{"accepted",std::to_string(accepted)},
        {"lastTick",std::to_string(tick-1)},{"sessionState",
        std::to_string(static_cast<unsigned>(session.Status().state))}});
    return true;
}
bool RunRemote(WireIpc& ipc,EventLog& log,const Options& options,Endpoint& endpoint,
    InboundState& inbound) {
    Participant participant;
    participant.Initialize(options.session,options.clients[1],1,nullptr);
    const auto target_tick=ipc.TargetTick();
    std::uint64_t last_ack_tick=0,last_control_heartbeat=0,sequence_sent=0,target_sent=0;
    bool edge_sent=false;
    for(;;) {
        if(ipc.Closed()||ipc.DriverError()) {log.Event("remote-ipc-closed",0);return false;}
        const auto run_state=ipc.RunState();
        if(run_state==isaac_cs_t0::t09::kRunStopRequested||run_state==isaac_cs_t0::t09::kRunStopped)break;
        std::vector<Message> messages;
        if(!DrainInbound(endpoint,ipc,log,inbound.latest_ack_tick,2,inbound,messages))return false;
        if(inbound.latest_ack_tick>last_ack_tick)last_ack_tick=inbound.latest_ack_tick;
        const auto heartbeat=ipc.ControllerHeartbeatTick();
        if(heartbeat>last_control_heartbeat) {
            last_control_heartbeat=heartbeat;
            const auto target=heartbeat+options.lead_ticks;
            if(target<=target_tick) {
                InputStateDraftV2 state{};
                // Deterministic canonical B script input. The matrix script is
                // intentionally minimal in T0-09-B; T0-09-C owns the full script.
                if((heartbeat/100)%2==0)state.move_x=4096;
                else state.move_y=4096;
                if(!edge_sent && target>=4000) {state.edge_flags=8;edge_sent=true;}
                const auto command=participant.MakeCommand(state,target);
                if(!command.client_id||!command.sequence||!command.target_tick) {
                    log.Event("remote-command-invalid",heartbeat);return false;
                }
                Message input{};input.header.kind=Kind::Input;input.input.command=command;
                input.input.last_ack=endpoint.LastAckSequence();
                if(!SendMessage(endpoint,input,ipc,log,heartbeat,"input-sent",2))return false;
                sequence_sent=command.sequence;target_sent=command.target_tick;
                log.Event("input-produced",heartbeat,{{"sequence",std::to_string(command.sequence)},
                    {"targetTick",std::to_string(command.target_tick)},
                    {"edgeFlags",std::to_string(command.state.edge_flags)},
                    {"lastAck",std::to_string(input.input.last_ack)},
                    {"controllerHeartbeat",std::to_string(heartbeat)}});
            }
        }
        Sleep(1);
    }
    endpoint.Retire();
    ipc.SetDriverFlags(isaac_cs_t0::t09::kDriverScriptComplete|isaac_cs_t0::t09::kDriverRestoreComplete);
    ipc.SetDriverState(isaac_cs_t0::t09::kDriverStopped);
    ipc.SetRunState(isaac_cs_t0::t09::kRunStopped);
    log.Event("endpoint-retired",last_control_heartbeat,{{"sequenceSent",std::to_string(sequence_sent)},
        {"targetSent",std::to_string(target_sent)},
        {"acks",std::to_string(inbound.latest_ack_tick)},
        {"controllerHeartbeat",std::to_string(last_control_heartbeat)},
        {"states",std::to_string(inbound.states_received)}});
    return true;
}
int Run(const Options& options) {
    EventLog log;
    if(!log.Open(options.events))return 2;
    WireIpc ipc;
    OpenConfig expected{};
    expected.role=options.role==Role::Authority?1u:2u;
    expected.run_nonce=options.run_nonce;expected.session=options.session;
    expected.authority_nonce=options.authority_nonce;expected.remote_nonce=options.remote_nonce;
    expected.clients=options.clients;expected.build=BuildText(options.build);
    const auto opened=ipc.Open(Wide(options.mapping).c_str(),expected);
    if(opened!=OpenResult::Accepted) {
        log.Event("wire-open-failed",0,{{"openResult",std::to_string(static_cast<unsigned>(opened))}});
        return 3;
    }
    ipc.SetDriverPid(GetCurrentProcessId());
    ipc.SetDriverState(isaac_cs_t0::t09::kDriverOpen);
    log.Event("wire-opened",0,{{"role",std::to_string(expected.role)},
        {"mapping",JsonString(options.mapping)}});
    isaac_cs_t0::protocol::Config config{};
    config.role=options.role;config.session=options.session;config.clients=options.clients;
    config.build=isaac_cs_t0::protocol::TextId(options.build.c_str());
    config.local_nonce=options.role==Role::Authority?options.authority_nonce:options.remote_nonce;
    config.peer_nonce=options.role==Role::Authority?options.remote_nonce:options.authority_nonce;
    Endpoint endpoint;
    if(endpoint.Start(config)!=Error::None) {
        log.Event("endpoint-start-failed",0);ipc.SetDriverError(0xE201);return 4;
    }
    InboundState inbound{};
    if(!WaitEstablished(endpoint,ipc,log,expected.role,inbound)) {
        ipc.SetDriverError(0xE202);return 5;
    }
    const auto start_deadline=GetTickCount64()+60000;
    while(ipc.RunState()!=isaac_cs_t0::t09::kRunRunning) {
        if(ipc.Closed()||ipc.DriverError()||GetTickCount64()>start_deadline) {
            log.Event("run-start-timeout",0,{{"runState",std::to_string(ipc.RunState())}});
            ipc.SetDriverError(0xE203);return 6;
        }
        std::vector<Message> messages;
        if(!DrainInbound(endpoint,ipc,log,0,expected.role,inbound,messages)) {
            ipc.SetDriverError(0xE204);return 7;
        }
        Sleep(1);
    }
    ipc.SetDriverState(isaac_cs_t0::t09::kDriverRunning);
    log.Event("run-started",ipc.RunStartTick(),{{"role",std::to_string(expected.role)},
        {"targetTick",std::to_string(ipc.TargetTick())},
        {"generation",std::to_string(ipc.LocalGeneration())}});
    const bool ok=options.role==Role::Authority?
        RunAuthority(ipc,log,options,endpoint,inbound):
        RunRemote(ipc,log,options,endpoint,inbound);
    if(!ok) {
        ipc.SetDriverError(0xE205);
        log.Event("driver-failed",0);log.Flush();return 8;
    }
    log.Event("driver-stopped",ipc.HeartbeatTick(),{{"role",std::to_string(expected.role)}});
    log.Flush();
    ipc.MarkClosed();
    return 0;
}
} // namespace

int main(int argc,char** argv) {
    Options options{};
    if(!ParseOptions(argc,argv,options))return 2;
    return Run(options);
}
