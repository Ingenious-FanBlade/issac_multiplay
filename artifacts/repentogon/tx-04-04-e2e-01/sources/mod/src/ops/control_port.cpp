#include "runtime.h"
#include <cstring>
namespace cs {
namespace {
std::atomic<bool> stop{};
SOCKET listener=INVALID_SOCKET;
bool SendAll(SOCKET s,const std::string& text) noexcept {
    size_t sent=0;
    while(sent<text.size()) { int n=send(s,text.data()+sent,int(text.size()-sent),0); if(n<=0)return false;sent+=n; }
    return true;
}
DWORD WINAPI Serve(void*) noexcept {
    uint64_t sequence=0;
    // Only one client is accepted at a time. Socket timeouts bound stalled peers.
    while(!stop) {
        fd_set fds;FD_ZERO(&fds);FD_SET(listener,&fds);timeval timeout{0,200000};
        if(select(0,&fds,nullptr,nullptr,&timeout)<=0) continue;
        auto peer=accept(listener,nullptr,nullptr);if(peer==INVALID_SOCKET)continue;
        DWORD ms=1000;setsockopt(peer,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&ms),sizeof(ms));
        setsockopt(peer,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<char*>(&ms),sizeof(ms));
        std::string line;line.reserve(513);
        bool connected=true;
        while(connected && !stop) {
            char c;int n=recv(peer,&c,1,0);
            if(n<=0) { if(n<0 && WSAGetLastError()==WSAETIMEDOUT)continue;break; }
            if(c=='\r') continue;
            if(c!='\n') {
                if(line.size()==512 || c==0) { SendAll(peer,"ok=0\nerror=line_too_long\n.\n");break; }
                line.push_back(c);continue;
            }
            PortRequest req{};req.id=++sequence;strncpy_s(req.text,line.c_str(),_TRUNCATE);line.clear();
            if(!QueuePort(req)) { connected=SendAll(peer,"ok=0\nerror=queue_full\n.\n");continue; }
            const auto deadline=GetTickCount64()+5000;
            PortReply reply{};bool received=false;
            while(!stop && GetTickCount64()<deadline) {
                if(TakeReply(reply)) { if(reply.id==req.id) {received=true;break;} }
                else Sleep(5);
            }
            connected=SendAll(peer,received?std::string(reply.text)+".\n":"ok=0\nerror=main_thread_timeout\n.\n");
        }
        closesocket(peer);
    }
    return 0;
}
}
void StartControlPort() {
    WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data)) throw std::runtime_error("winsock_startup");
    listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(listener==INVALID_SOCKET) throw std::runtime_error("control_socket");
    BOOL exclusive=TRUE;setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive));
    sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);addr.sin_port=htons(u_short(Settings.control_port));
    if(bind(listener,reinterpret_cast<sockaddr*>(&addr),sizeof(addr)) || listen(listener,1)) {
        closesocket(listener);listener=INVALID_SOCKET;throw std::runtime_error("control_bind");
    }
    auto thread=CreateThread(nullptr,0,Serve,nullptr,0,nullptr);
    if(!thread) {closesocket(listener);throw std::runtime_error("control_thread");}
    CloseHandle(thread);
    Trace("control_port","address=127.0.0.1 port=%u",Settings.control_port);
}
void StopControlPort() noexcept { stop=true; }
}
