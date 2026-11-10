#include "runtime.h"
#include "ops/e2e_ui.h"
#include "game/common/bridge.h"
#include "game/client/session/client.h"
#include "game/common/context.h"
#include "game/authority/tick.h"
#include "game/participants/topology.h"
#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace cs {
namespace {
namespace fs=std::filesystem;
constexpr size_t Limit=16384;
// worker终身只拥有值、句柄和路径，不持有游戏/Lua/窗口对象；DLL与线程随进程常驻。
struct Request {std::string action,data;bool resumes=false,nudged=false;};
struct State {
 std::mutex mutex;std::deque<Request> requests;std::string snapshot="{}",error;
 // 只由主线程使用的待交接队列，worker从不接触它。
 std::deque<Request> pending;
 std::atomic<ULONGLONG> heartbeat{};HANDLE wake=nullptr;
 std::atomic<bool> worker_idle{true};std::atomic<unsigned> queued{};
 fs::path root;HANDLE controller=nullptr;bool busy=false;bool ready=false;
};
State *shared=nullptr; // 主线程惰性创建；未打开面板时不启动文件worker。
std::uint64_t rendered=0,seen_rendered=0,blocked_frames=0;
unsigned clear_frames=0;
std::string Json(const rapidjson::Value &value){rapidjson::StringBuffer out;rapidjson::Writer<rapidjson::StringBuffer> writer(out);value.Accept(writer);return {out.GetString(),out.GetSize()};}
std::string Hex(std::string_view value){static constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(value.size()*2);for(unsigned char b:value){out+=digits[b>>4];out+=digits[b&15];}return out;}
std::string Unhex(std::string_view value){
 if(value.size()%2||value.size()>384)throw std::runtime_error("chunk_size");
 auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("chunk_hex");};
 std::string out;for(size_t i=0;i<value.size();i+=2)out+=char(digit(value[i])*16+digit(value[i+1]));return out;
}
// 信箱限于编译时工程目录，拒绝out/信箱/response重解析路径；不接受任意脚本地址。
void Directory(const fs::path &path){
 fs::create_directories(path);
 const auto attributes=GetFileAttributesW(path.c_str());
 if(attributes==INVALID_FILE_ATTRIBUTES||(attributes&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("ui_reparse_directory");
}
rapidjson::Document Read(const fs::path &path){
 rapidjson::Document d;d.SetObject();
 HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE){if(GetLastError()==ERROR_FILE_NOT_FOUND||GetLastError()==ERROR_PATH_NOT_FOUND)return d;throw std::runtime_error("ui_read_open");}
 std::string text(131073,'\0');DWORD count=0;const bool ok=ReadFile(file,text.data(),DWORD(text.size()),&count,nullptr)!=FALSE;CloseHandle(file);
 if(!ok)throw std::runtime_error("ui_read");text.resize(count);
 if(text.size()>131072)throw std::runtime_error("ui_file_limit");d.Parse(text.data(),text.size());
 if(d.HasParseError()||!d.IsObject())d.SetObject();return d;
}
void Write(const fs::path &path,const std::string &text){
 const fs::path temporary=path.wstring()+L".native-new";
 {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);out.write(text.data(),std::streamsize(text.size()));if(!out)throw std::runtime_error("ui_write");}
 if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("ui_replace");
}
// PID可能被系统复用；连同实际进程创建时间确认旧记录，不把无关进程当作编排owner。
bool Alive(DWORD pid,const std::string &created){
 if(!pid||created.empty())return false;auto process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process)return false;
 FILETIME start{},exit{},kernel{},user{};const bool times=GetProcessTimes(process,&start,&exit,&kernel,&user)!=FALSE;
 ULARGE_INTEGER value{};value.LowPart=start.dwLowDateTime;value.HighPart=start.dwHighDateTime;
 const bool alive=times&&std::to_string(value.QuadPart)==created&&WaitForSingleObject(process,0)==WAIT_TIMEOUT;CloseHandle(process);return alive;
}
bool Busy(State &s){
 if(s.controller){if(WaitForSingleObject(s.controller,0)==WAIT_TIMEOUT)return true;CloseHandle(s.controller);s.controller=nullptr;}
 auto life=Read(s.root/L"lifecycle.json");return life.HasMember("busy")&&life["busy"].IsBool()&&life["busy"].GetBool()&&life.HasMember("pid")&&life["pid"].IsUint()&&life.HasMember("start_filetime")&&life["start_filetime"].IsString()&&Alive(life["pid"].GetUint(),life["start_filetime"].GetString());
}
// 只持久化界面约定的五个字段，忽略未知JSON成员；文字不进入shell语法。
void Profile(State &s,const std::string &text){
 rapidjson::Document d;d.Parse(text.data(),text.size());if(d.HasParseError()||!d.IsObject())throw std::runtime_error("profile_json");
 rapidjson::Document profile;profile.SetObject();auto &a=profile.GetAllocator();
 for(const char *key:{"topology","run_directory","rerun","reuse_reason"}){
  if(!d.HasMember(key)||!d[key].IsString()||d[key].GetStringLength()>2048||std::strlen(d[key].GetString())!=d[key].GetStringLength())throw std::runtime_error("profile_field");
  rapidjson::Value name(key,a),value(d[key],a);profile.AddMember(name,value,a);
 }
 if(!d.HasMember("resume")||!d["resume"].IsBool())throw std::runtime_error("profile_resume");
 profile.AddMember("resume",d["resume"].GetBool(),a);Write(s.root/L"profile.json",Json(profile));
}
void Launch(State &s){
 const fs::path repository=fs::path(ISAACCS_REPOSITORY_ROOT);
 // 路径来自固定脚本与系统安装目录；无-Command、cmd.exe或用户文字插值。
 const auto script=repository/L"scripts/start-tx-04-21-ui.ps1";
 wchar_t programFiles[32768]{};const auto count=GetEnvironmentVariableW(L"ProgramW6432",programFiles,32768);
 if(!count||count>=32768)throw std::runtime_error("powershell7_not_found");
 const auto executable=fs::path(programFiles)/L"PowerShell/7/pwsh.exe";
 if(!fs::is_regular_file(executable)||!fs::is_regular_file(script))throw std::runtime_error("powershell7_or_fixed_script_missing");
 std::wstring line=L"\""+executable.wstring()+L"\" -NoProfile -STA -WindowStyle Hidden -File \""+script.wstring()+L"\" -BootstrapPid "+std::to_wstring(GetCurrentProcessId());
 STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION process{};
 if(!CreateProcessW(executable.c_str(),line.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,repository.c_str(),&startup,&process))throw std::runtime_error("ui_controller_launch");
 CloseHandle(process.hThread);s.controller=process.hProcess;
}
void Reply(State &s,const std::string &text){
 rapidjson::Document reply;reply.Parse(text.data(),text.size());
 if(reply.HasParseError()||!reply.IsObject()||!reply.HasMember("id")||!reply["id"].IsString()||!reply.HasMember("answer")||!reply["answer"].IsString())throw std::runtime_error("ui_reply_json");
 const std::string id=reply["id"].GetString();if(id.empty()||id.size()>100||id.find_first_not_of("0123456789abcdef-")!=std::string::npos)throw std::runtime_error("ui_prompt_id");
 const auto current=Read(s.root/L"state.json");
 if(!current.HasMember("pending")||!current["pending"].IsBool()||!current["pending"].GetBool()||!current.HasMember("id")||!current["id"].IsString()||id!=current["id"].GetString())throw std::runtime_error("ui_prompt_changed");
 const auto path=s.root/L"responses"/fs::u8path(id+".json");const auto bytes=Json(reply);
 HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("ui_reply_already_sent_or_io");
 DWORD written=0;const bool ok=WriteFile(file,bytes.data(),DWORD(bytes.size()),&written,nullptr)&&written==bytes.size();CloseHandle(file);if(!ok)throw std::runtime_error("ui_reply_write");
}
// 有界请求每批最多8个；等待/磁盘/启动只发生于此线程，正常逻辑启停不卸载它。
DWORD WINAPI Serve(void *context) noexcept{
 auto &s=*static_cast<State *>(context);std::string error;
 for(;;){
  std::deque<Request> requests;{std::lock_guard lock(s.mutex);requests.swap(s.requests);}
  auto recent=s.heartbeat.load();
  if(requests.empty()&&(!recent||GetTickCount64()-recent>=1500)){
   // 不显示界面且没有工作时完全休眠，避免后台文件轮询污染自动采样。
   s.heartbeat.compare_exchange_strong(recent,0);s.worker_idle.store(true);WaitForSingleObject(s.wake,INFINITE);continue;
  }
  s.worker_idle.store(false);
  try{
   Directory(fs::path(ISAACCS_REPOSITORY_ROOT)/L"out");Directory(s.root);Directory(s.root/L"responses");
   if(!fs::exists(s.root/L"profile.json")){
    rapidjson::Document defaults;defaults.SetObject();auto &a=defaults.GetAllocator();
    defaults.AddMember("topology","",a);
    defaults.AddMember("run_directory","",a);defaults.AddMember("resume",false,a);defaults.AddMember("rerun","",a);defaults.AddMember("reuse_reason","",a);Write(s.root/L"profile.json",Json(defaults));
   }
   for(const auto &request:requests){
    try{
     if(request.action=="error")throw std::runtime_error(request.data);
     if(request.action=="reply")Reply(s,request.data);
     else{if(Busy(s))throw std::runtime_error("controller_busy");Profile(s,request.data);if(request.action=="start")Launch(s);}
     error.clear();
    }catch(const std::exception &e){error=e.what();}
   }
   const auto heartbeat=s.heartbeat.load();if(heartbeat&&GetTickCount64()-heartbeat<1500)Write(s.root/L"game-heartbeat","visible");
   rapidjson::Document snapshot;snapshot.SetObject();auto &a=snapshot.GetAllocator();
   for(const char *name:{"profile","state","lifecycle"}){auto document=Read(s.root/(std::string(name)+".json"));rapidjson::Value key(name,a),value(document,a);snapshot.AddMember(key,value,a);}
   const bool busy=Busy(s);snapshot.AddMember("busy",busy,a);snapshot.AddMember("error",rapidjson::Value(error.c_str(),a),a);
   auto text=Json(snapshot);if(text.size()>131072)throw std::runtime_error("ui_snapshot_limit");
   {std::lock_guard lock(s.mutex);s.snapshot=std::move(text);s.busy=busy;s.ready=true;s.error.clear();}
  }catch(const std::exception &e){std::lock_guard lock(s.mutex);s.error=e.what();}
  s.queued.fetch_sub(unsigned(requests.size()));s.worker_idle.store(true);
  WaitForSingleObject(s.wake,400);
 }
}
State &Shared(){
 if(!shared){auto value=new State;value->root=fs::u8path(Settings.e2e_ui_directory);value->wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!value->wake)throw std::runtime_error("ui_wake");auto thread=CreateThread(nullptr,0,Serve,value,0,nullptr);if(!thread)throw std::runtime_error("ui_thread");CloseHandle(thread);shared=value;}return *shared;
}
} // namespace
void E2eUiManagerReturned() noexcept{
 if(!IsMain()||game::TopOwner()!=Owner::Manager||game::HasOwner(Owner::Render)||!game::ContextHealthy()||game::InBlockedContext())return;
 try{
  const bool blocked=game::ClientUiBlocked();
  if(blocked&&blocked_frames!=UINT64_MAX)++blocked_frames;
  clear_frames=blocked||seen_rendered!=rendered?0:(std::min)(clear_frames+1,2u);seen_rendered=rendered;
  if(!shared||shared->pending.empty())return;
  auto &s=*shared;auto &request=s.pending.front();
  if(request.resumes&&clear_frames<2){
   if(blocked&&!request.nudged){
    // Hide只改menuShown。WM_NULL让官方WndProc自行解除Console state=2；不写原生状态、不发键、不抢焦点。
    void *dc=nullptr,*context=nullptr;
    if(game::ClientRendererHandles(dc,context))request.nudged=PostMessageW(WindowFromDC(static_cast<HDC>(dc)),WM_NULL,0,0)!=FALSE;
   }
   return;
  }
  Request ready=request;
  if(ready.action=="start"&&(game::RunActive()||game::Busy()||game::ClientBusy()))ready={"error","return_to_idle_menu_first"};
  std::unique_lock lock(s.mutex,std::try_to_lock);if(!lock||s.requests.size()>=8)return;
  s.requests.push_back(std::move(ready));s.queued.fetch_add(1);s.pending.pop_front();SetEvent(s.wake);
 }catch(...){Trace("e2e_ui_error","manager_handoff_failed");}
}
std::string E2eUiFields(){
 return "ui.blocked="+std::to_string(game::ClientUiBlocked())+"\nui.clear_frames="+std::to_string(clear_frames)+
  "\nui.blocked_frames="+std::to_string(blocked_frames)+"\nui.render_callbacks="+std::to_string(rendered)+
  "\nui.handoff_pending="+std::to_string(shared?shared->pending.size():0)+
  "\nui.worker_idle="+std::to_string(!shared||(shared->worker_idle.load()&&shared->queued.load()==0&&shared->heartbeat.load()==0))+
  "\nui.counters_ok="+std::to_string(IsMain()&&game::TopOwner()==Owner::Manager&&game::ContextHealthy()&&!game::InBlockedContext()&&rendered!=UINT64_MAX&&blocked_frames!=UINT64_MAX)+"\n";
}
std::string E2eUiCommand(std::istream &in){
 try{
  std::string verb,arg,extra;in>>verb>>arg>>extra;
  if(!extra.empty())return "ok=0\nerror=ui_arguments\n";
  // 无文件worker的固定计数入口也用于远端E2E普通Session面板；产品不链接本单元。
  if(verb=="rendered"&&arg.empty()){if(rendered!=UINT64_MAX)++rendered;return "ok=1\n";}
  if(!Settings.e2e_ui_enabled)return "ok=0\nerror=e2e_ui_disabled\n";
  auto &s=Shared();
  if(verb=="poll"&&arg.empty()){
   s.heartbeat.store(GetTickCount64());SetEvent(s.wake);std::unique_lock lock(s.mutex,std::try_to_lock);if(!lock)return "ok=0\nerror=ui_busy\n";
   return "ok=1\nready="+std::to_string(s.ready)+"\nerror="+s.error+"\ndata="+Hex(s.snapshot)+"\n";
  }
  // 上传有总量和块长上限；同一Lua主线程的完整提交才进入worker，不传原生对象。
  static std::string upload,action;
  if(verb=="begin"&&(arg=="save"||arg=="start"||arg=="reply")){upload.clear();action=arg;return "ok=1\n";}
  if(verb=="chunk"&&!action.empty()){auto bytes=Unhex(arg);if(upload.size()+bytes.size()>Limit){action.clear();upload.clear();throw std::runtime_error("ui_upload_limit");}upload+=bytes;return "ok=1\n";}
  if(verb=="commit"&&arg.empty()&&!action.empty()){
   Request request{action,upload};
   if(action=="reply"){
    rapidjson::Document reply;reply.Parse(upload.data(),upload.size());
    if(reply.HasParseError()||!reply.IsObject()||!reply.HasMember("answer")||!reply["answer"].IsString())throw std::runtime_error("ui_reply_json");
    const std::string answer=reply["answer"].GetString();request.resumes=answer!="stop"&&answer.rfind("fail ",0)!=0;
   }
   std::unique_lock lock(s.mutex,std::try_to_lock);if(!lock||s.queued.load()+s.pending.size()>=8)return "ok=0\nerror=ui_queue_busy\n";
   if(action!="reply"&&s.busy)return "ok=0\nerror=controller_busy\n";
   s.pending.push_back(std::move(request));
   if(action=="reply")s.heartbeat.store(0);
   upload.clear();action.clear();return "ok=1\nqueued=1\n";
  }
  return "ok=0\nerror=ui_arguments\n";
 }catch(const std::exception &e){return std::string("ok=0\nerror=")+e.what()+"\n";}
}
} // namespace cs
