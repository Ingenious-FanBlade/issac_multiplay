// 预测插值阶段 1：显示覆盖层、世界显示时间、远端插值与外推（设计第 4、6、7 节）。
// 只产生显示值：权威值照常由 World／Body 安装写入原生对象，本层只在相机与 Render 作用域内暂时替换 pos。
#include "game/client/presentation/client_motion_overlay.h"
#include "replica/stream_metrics.h"
#include "runtime.h"
#include "state/client_shadow.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace cs::game {
namespace {
#include "state_host_fields.inc"
namespace F=host_field;
using replica::stream::Duration;
using replica::stream::MetricMicros;

// —— 常量（单位：h＝半帧＝1/60 秒；距离为游戏世界单位，一格 40）——
constexpr double kHalfFramesPerMicro=60.0/1'000'000.0;
constexpr double kDelayMin=2.0,kDelayMax=6.0;      // P4：D 限 2h—6h
constexpr double kDelayWarmup=4.0;                 // 到达样本不足 8 个时的 D 初值
constexpr double kChaseRate=0.05;                  // T_w 追赶速率上限 ±5%
constexpr double kTimelineJump=6.0;                // 偏离超过 6h 直接跳
constexpr double kMaxStep=6.0;                     // 单次推进最多 6h（卡顿后不一次冲过头）
constexpr float kGrid=40.f;
constexpr float kFuseLimit=1.5f*kGrid;             // P4：直接跳阈值 1.5 格
constexpr std::uint64_t kFuseMicros=100'000;       // 7.2：融合偏移约 100ms 衰减到 0
constexpr std::uint64_t kStalePrepareMicros=50'000;// 写入表超过 50ms 未刷新即不再使用
constexpr unsigned kArrivalWindow=64;              // 时钟与 D 统计窗口
constexpr unsigned kHistory=4;                     // 每对象 4 份权威样本（6.1）
constexpr unsigned kEntityCapacity=4096;           // 单次遍历上限；超出计数并截断
constexpr std::size_t kTrackLimit=4096;            // 历史表上限（每项约 160B，合计约 640KiB 上界）

// 外推上限（7.1）：泪弹、敌弹 200ms；敌怪、其他玩家等 100ms；拾取物与静止效果不外推。
double ExtrapolationCap(std::uint32_t type) noexcept {
  switch(type){
    case 2:case 9:return 12.0;                 // 泪弹、敌弹
    case 1:case 3:case 4:case 8:return 6.0;    // 玩家、跟班、炸弹、刀
    default:return type>=10&&type<1000?6.0:0.0;// 敌怪 100ms；拾取物、效果等不外推
  }
}
// 速度单位（RE-P1）：玩家按半帧，其他实体按整帧，换算成每 h 的位移系数。
float VelocityPerHalfFrame(std::uint32_t type) noexcept {return type==1?1.f:0.5f;}

struct Sample final {double h=0;float x=0,y=0,vx=0,vy=0;};
// 每个对象的权威历史与显示状态，按原生地址作键并核对身份。
struct Track final {
  ClientMotionIdentity identity{};
  std::array<Sample,kHistory> samples{};  // 旧→新
  unsigned count=0;
  std::uint64_t seen=0;                   // 最近一次 World 采样遍历的序号，用于清除已离场对象
  bool fresh=false;                       // 上次 Prepare 之后到了新样本
  bool cut=false;                         // 新样本与前一份不连续（传送、出生等），不插值不融合
  bool shown=false,extrapolating=false;   // 上一帧是否给出显示值、是否处于外推
  float shown_x=0,shown_y=0;
  float fuse_x=0,fuse_y=0;std::uint64_t fuse_us=0;
};
enum class Mode : unsigned {Before,Interp,Extrap,Capped,Hold,Count};
constexpr double kProjectileLeadCap=30.0;          // P6：敌弹外推到 T_p 时的外推上限 500ms

state::PresentationMember MemberOf(const ClientMotionMember &m) noexcept {
  state::PresentationMember out;out.space=m.space;out.layout=m.layout;out.id=m.id;out.use=m.use;out.shape=m.shape;return out;
}
// 影子记录中一个定长标量：平铺记录按名称取，普通记录按字段号取；缺失或长度不符返回 false。
template<class T>bool RecordValue(const state::ClientShadowRecordView &r,std::uint16_t field,const char *name,T &value) noexcept {
  std::span<const std::byte> bytes;
  if(r.Flat())bytes=r.FlatValue(name);
  else if(const auto *f=r.Field(field);f&&f->present)bytes=f->bytes;
  if(bytes.size()!=sizeof(T))return false;
  std::memcpy(&value,bytes.data(),sizeof(T));return true;
}
} // namespace

struct ClientMotionOverlay::Impl final {
  // —— 场景与样本 ——
  std::uint64_t scene=0;
  bool have_scene=false;
  std::unordered_map<void *,Track> tracks;
  std::uint64_t pass=0;
  std::vector<ClientMotionEntity> entities=std::vector<ClientMotionEntity>(kEntityCapacity);
  std::vector<ClientMotionPlayer> players;
  // Body 段头只有 game_frame，同帧两个半步同号（RE-P6）：同号第二份记为 2F+1。阶段 2 的相位字段到位后改用实值。
  std::uint32_t last_body_frame=0;double last_body_h=-1;
  // —— 世界显示时间 ——
  struct Arrival final {double t=0,h=0;};
  std::array<Arrival,kArrivalWindow> arrivals{};unsigned arrival_count=0,arrival_head=0;
  std::array<double,kArrivalWindow> staleness{};unsigned stale_count=0,stale_head=0;
  double latest_world_h=-1,latest_h=-1;
  double tw=0,rate=1,delay=kDelayWarmup;bool tw_valid=false,was_paused=false;
  std::uint64_t last_prepare_us=0,prepared_us=0;
  // —— 写入表 ——
  std::vector<ClientMotionWrite> writes;
  bool entered=false;
  // —— 阶段 2：本机身体预测（设计第 5 节）——
  ClientMotionPredictor predictor;
  ClientMotionTerrain terrain;
  ClientMotionBody body;
  // 寻路阻挡影子表（5.3.1）：按场景作键（换场景随 Clear 作废），记录来源 World 版本避免每帧重扫。
  std::array<std::uint8_t,kClientMotionGridCells/4> path_block{};
  bool path_valid=false;std::uint64_t path_version=0;
  // 本帧预测结果：本机身体地址与显示偏移 Δ（附着对象据此平移）。
  void *predicted_body=nullptr;float delta_x=0,delta_y=0;double lead_h=0;
  bool predict_paused=false;
  std::uint64_t predict_written=0,follow_written=0,path_updates=0,path_missing=0,
                body_missing=0,authority_missing=0,multi_body=0,terrain_missing=0,projectile_lead=0;
  // —— 诊断 ——
  Duration prepare_us,apply_us,delay_us,extrapolate_us;
  std::array<std::uint64_t,unsigned(Mode::Count)> modes{};
  std::uint64_t installs_world=0,installs_body=0,tw_jumps=0,tw_resets=0,resets=0,
                fusions=0,fuse_dropped=0,cuts=0,identity_resets=0,out_of_order=0,
                collect_overflow=0,track_overflow=0,apply_stale=0,apply_missing=0,
                restore_missing=0,restore_changed=0,disabled_frames=0,paused_frames=0;
  unsigned writes_last=0,writes_max=0,excluded_local=0,excluded_attached=0,excluded_laser=0;

  void ClearTimeline() noexcept {
    arrival_count=arrival_head=stale_count=stale_head=0;
    latest_world_h=latest_h=-1;tw_valid=false;delay=kDelayWarmup;rate=1;
    last_body_frame=0;last_body_h=-1;
  }
  void Clear() noexcept {
    tracks.clear();writes.clear();players.clear();ClearTimeline();have_scene=false;entered=false;++resets;
    predictor.Reset();path_valid=false;path_version=0;path_block={};predicted_body=nullptr;
  }
  // 5.3.1：从已发布 World 的 Room 记录取 grid_path_block（112 字节）存入影子表；不写原生 Room+0x76C。
  void RefreshPath(const state::ClientStateBlockTable *table) noexcept {
    if(!table||!table->streams[0])return;
    const auto &world=*table->streams[0];
    const auto version=world.Context().target_version;
    if(path_valid&&version==path_version)return;
    for(std::size_t i=0;i<world.RecordCount();++i){
      const auto r=world.Record(i);
      if(r.Identity().layout!=F::Room::kLayout)continue;
      std::span<const std::byte> bytes;
      if(r.Flat())bytes=r.FlatValue("grid_path_block");
      else if(const auto *f=r.Field(F::Room::grid_path_block);f&&f->present)bytes=f->bytes;
      if(bytes.size()!=path_block.size()){++path_missing;return;}
      std::memcpy(path_block.data(),bytes.data(),path_block.size());
      path_valid=true;path_version=version;++path_updates;
      return;
    }
    ++path_missing;
  }
  // 设计 5.7：本机身体在已发布 Body 表中的对账字段。
  bool Authority(const state::ClientStateBlockTable *table,const ClientMotionMember &member,ClientMotionAuthority &out) noexcept {
    if(!table||!table->streams[1])return false;
    const auto r=table->streams[1]->Find(MemberOf(member));
    if(!r)return false;
    return RecordValue(*r,F::EntityPlayer::input_tick,"input_tick",out.tick)&&
           RecordValue(*r,F::EntityPlayer::input_sequence,"input_sequence",out.sequence)&&
           RecordValue(*r,F::EntityPlayer::input_phase,"input_phase",out.phase);
  }
  // 本地单调时钟换成 h。
  static double LocalH(std::uint64_t us) noexcept {return double(us)*kHalfFramesPerMicro;}
  // Host 时间速率：窗口首尾的 Host 帧差除以本地时差；跨度不足 30h 或异常时按 1。
  void EstimateRate() noexcept {
    rate=1;
    if(arrival_count<8)return;
    const auto &oldest=arrivals[(arrival_head+kArrivalWindow-arrival_count)%kArrivalWindow];
    const auto &newest=arrivals[(arrival_head+kArrivalWindow-1)%kArrivalWindow];
    const double span=newest.t-oldest.t;
    if(span<30)return;
    rate=std::clamp((newest.h-oldest.h)/span,0.5,1.5);
  }
  // Host 现在时刻估计：按速率换算后取最早到达包络（最小偏移），过滤排队抖动。
  bool HostNow(double now,double &out) const noexcept {
    if(!arrival_count){if(latest_world_h<0)return false;out=latest_world_h;return true;}
    double best=1e300;
    for(unsigned i=0;i<arrival_count;++i){
      const auto &a=arrivals[(arrival_head+kArrivalWindow-1-i)%kArrivalWindow];
      best=(std::min)(best,a.t*rate-a.h);
    }
    out=now*rate-best;return true;
  }
  // D（6.2）：每次 World 到达前“最新样本已过去多久”的 p95，即 2h 名义间隔加到达抖动，限 2h—6h。
  void EstimateDelay() noexcept {
    if(stale_count<8){delay=kDelayWarmup;return;}
    std::array<double,kArrivalWindow> sorted{};
    std::copy_n(staleness.begin(),stale_count,sorted.begin());
    const auto at=sorted.begin()+std::ptrdiff_t((stale_count*95+99)/100-1);
    std::nth_element(sorted.begin(),at,sorted.begin()+stale_count);
    delay=std::clamp(*at,kDelayMin,kDelayMax);
  }
  // 追加一份权威样本：同时刻覆盖，倒退视为时间基准异常重建该对象，跳变超过阈值视为不连续。
  void Push(Track &t,double h,const ClientMotionEntity &e) noexcept {
    const Sample s{h,e.x,e.y,e.vx,e.vy};
    if(t.count){
      auto &newest=t.samples[t.count-1];
      if(h<newest.h){++out_of_order;t.count=0;}
      else if(h==newest.h){newest=s;t.fresh=true;return;}
      else{
        const float per=VelocityPerHalfFrame(e.identity.type);
        const float expected=std::hypot(newest.vx,newest.vy)*per*float(h-newest.h);
        if(std::hypot(e.x-newest.x,e.y-newest.y)>kGrid+1.5f*expected){++cuts;t.count=0;t.cut=true;t.fuse_x=t.fuse_y=0;}
      }
    }
    if(t.count==kHistory){std::move(t.samples.begin()+1,t.samples.end(),t.samples.begin());--t.count;}
    t.samples[t.count++]=s;t.fresh=true;
  }
  // 在 T_w 处求值（6.3、7.1）。
  Mode Evaluate(const Track &t,double at,std::uint32_t type,bool extrapolate,float &x,float &y) noexcept {
    const auto &first=t.samples[0];
    if(at<=first.h){x=first.x;y=first.y;return t.count==1&&at==first.h?Mode::Interp:Mode::Before;}
    for(unsigned i=1;i<t.count;++i){
      const auto &a=t.samples[i-1],&b=t.samples[i];
      if(at<=b.h){
        const float k=float((at-a.h)/(b.h-a.h));
        x=a.x+(b.x-a.x)*k;y=a.y+(b.y-a.y)*k;return Mode::Interp;
      }
    }
    const auto &last=t.samples[t.count-1];
    const double cap=extrapolate?ExtrapolationCap(type):0.0;
    if(cap<=0){x=last.x;y=last.y;return Mode::Hold;}
    const double ahead=at-last.h,used=(std::min)(ahead,cap);
    const float scale=VelocityPerHalfFrame(type)*float(used);
    x=last.x+last.vx*scale;y=last.y+last.vy*scale;
    extrapolate_us.Observe(std::uint64_t(used/kHalfFramesPerMicro));
    return ahead>cap?Mode::Capped:Mode::Extrap;
  }
  // P6：敌弹在 T_p 处求值。落在历史内仍插值；超过最新样本按速度外推，上限 kProjectileLeadCap。
  Mode EvaluateLead(const Track &t,double at,bool extrapolate,float &x,float &y) noexcept {
    const auto &last=t.samples[t.count-1];
    if(at<=last.h||!extrapolate)return Evaluate(t,at,9,extrapolate,x,y);
    const double ahead=at-last.h,used=(std::min)(ahead,kProjectileLeadCap);
    const float scale=VelocityPerHalfFrame(9)*float(used);
    x=last.x+last.vx*scale;y=last.y+last.vy*scale;
    extrapolate_us.Observe(std::uint64_t(used/kHalfFramesPerMicro));
    return ahead>kProjectileLeadCap?Mode::Capped:Mode::Extrap;
  }
};

ClientMotionOverlay::ClientMotionOverlay():impl_(std::make_unique<Impl>()) {}
ClientMotionOverlay::~ClientMotionOverlay()=default;

void ClientMotionOverlay::Reset() noexcept {impl_->Clear();}

void ClientMotionOverlay::SetPlayers(const ClientMotionPlayer *players,unsigned count) noexcept {
  auto &s=*impl_;
  s.players.assign(players,players+(players?count:0));
}

void ClientMotionOverlay::Installed(void *room,std::uint8_t streams,std::uint64_t scene,
                                    std::uint32_t world_frame,std::uint32_t body_frame,
                                    const state::ClientStateBlockTable *table) noexcept {
  auto &s=*impl_;
  if(!IsMain()||!room||!(streams&3))return;
  try{
    // 换房、换层、换维度都会换 scene：旧历史和时间基准一并作废，新房间不从旧位置滑入。
    if(!s.have_scene||s.scene!=scene){s.Clear();s.scene=scene;s.have_scene=true;}
    const auto now=MetricMicros();const double t=Impl::LocalH(now);
    double world_h=-1,body_h=-1;
    if(streams&1){
      world_h=2.0*world_frame;
      // Host 帧号大幅倒退只会出现在新一局；作为异常重建时间基准。
      if(s.latest_world_h>=0&&world_h+kTimelineJump<s.latest_world_h){s.ClearTimeline();++s.tw_resets;}
      double host=0;
      if(s.latest_world_h>=0&&s.HostNow(t,host)){
        s.staleness[s.stale_head]=(std::max)(0.0,host-s.latest_world_h);
        s.stale_head=(s.stale_head+1)%kArrivalWindow;s.stale_count=(std::min)(s.stale_count+1,kArrivalWindow);
      }
      if(world_h>s.latest_world_h){
        s.arrivals[s.arrival_head]={t,world_h};
        s.arrival_head=(s.arrival_head+1)%kArrivalWindow;s.arrival_count=(std::min)(s.arrival_count+1,kArrivalWindow);
        s.latest_world_h=world_h;
      }
      s.EstimateRate();s.EstimateDelay();
      ++s.installs_world;
    }
    if(streams&2){
      body_h=2.0*body_frame;
      // 阶段 2：Body 记录带半帧相位（设计 5.7），bit0 为 1 表示样本结束于奇半帧 2F+1；
      // 取任一已登记身体的相位（同一 tick 对所有身体相同）。读不到时沿用阶段 1 的同号推断。
      std::uint8_t phase=0;
      if(table&&table->streams[1])for(const auto &p:s.players){
        if(const auto r=table->streams[1]->Find(MemberOf(p.member));r&&RecordValue(*r,F::EntityPlayer::input_phase,"input_phase",phase)&&phase)break;
        phase=0;
      }
      if(phase)body_h+=double(phase&1u);
      else if(body_frame==s.last_body_frame&&s.last_body_h>=body_h)body_h=2.0*body_frame+1;
      s.last_body_frame=body_frame;s.last_body_h=body_h;
      ++s.installs_body;
    }
    s.latest_h=(std::max)({s.latest_h,world_h,body_h});
    bool overflow=false;
    const unsigned n=ClientMotionCollect(room,nullptr,0,s.entities.data(),unsigned(s.entities.size()),overflow);
    if(overflow)++s.collect_overflow;
    ++s.pass;
    for(unsigned i=0;i<n;++i){
      const auto &e=s.entities[i];
      if(e.laser)continue;
      auto at=s.tracks.find(e.address);
      if(at==s.tracks.end()){
        if(s.tracks.size()>=kTrackLimit){++s.track_overflow;continue;}
        at=s.tracks.emplace(e.address,Track{}).first;at->second.identity=e.identity;
      }else if(at->second.identity!=e.identity){
        // 地址复用或原地换形态：视为出生，不从旧对象位置插值。
        at->second=Track{};at->second.identity=e.identity;++s.identity_resets;
      }
      auto &track=at->second;
      if(streams&1)track.seen=s.pass;
      // 玩家运动组随 Body 流，其他实体随 World 流；World 安装不改玩家位置，不能给旧位置盖新时间。
      if(e.identity.type==1){
        if(body_h>=0)s.Push(track,body_h,e);
        else if(!track.count)s.Push(track,world_h,e);
      }else if(world_h>=0)s.Push(track,world_h,e);
    }
    // World 遍历覆盖整个渲染列表：未出现的对象已离场，清掉历史。
    if(streams&1)for(auto it=s.tracks.begin();it!=s.tracks.end();)
      if(it->second.seen!=s.pass)it=s.tracks.erase(it);else ++it;
  }catch(...){s.Clear();}
}

void ClientMotionOverlay::RecordInput(std::uint64_t generation,std::uint64_t sequence,std::uint64_t target,
                                      std::int16_t move_x,std::int16_t move_y) noexcept {
  if(!IsMain())return;
  impl_->predictor.Input(generation,sequence,target,move_x,move_y);
}

void ClientMotionOverlay::Prepare(void *room,unsigned local_ordinal,bool paused,const state::ClientStateBlockTable *table) noexcept {
  auto &s=*impl_;
  if(!IsMain()||s.entered)return;
  const auto start=MetricMicros();
  try{
    s.writes.clear();s.writes_last=0;
    const auto dt_us=s.last_prepare_us&&start>s.last_prepare_us?start-s.last_prepare_us:0;
    s.last_prepare_us=start;
    s.predicted_body=nullptr;s.delta_x=s.delta_y=0;s.lead_h=0;
    if(!room||!s.have_scene){return;}
    // —— 阶段 2：本机身体预测（4.3 第 4 步，先于远端求值以便附着对象取 Δ）——
    s.RefreshPath(table);
    std::array<const ClientMotionPlayer *,8> mine{};unsigned mine_count=0;
    for(const auto &p:s.players)if(p.ordinal==local_ordinal&&p.address&&mine_count<mine.size())mine[mine_count++]=&p;
    ClientMotionDisplay predicted{};
    // 设计 5.5：Host 暂停时停止预测并清空待确认输入；恢复后从新输入重新开始并直接跳。
    // P9 预测开关关闭时同样处理，重新打开时不从旧状态平滑。
    if(paused||!Settings.motion.prediction){if(!s.predict_paused)s.predictor.Reset();s.predict_paused=true;}
    else if(mine_count){
      bool jump=s.predict_paused;s.predict_paused=false;
      ClientMotionAuthority authority;const char *reason=nullptr;
      if(mine_count!=1){++s.multi_body;}  // 多身体角色首版不预测（设计 5.5）
      else if(!ClientMotionReadBody(room,mine[0]->address,s.body))++s.body_missing;
      else if(!s.Authority(table,mine[0]->member,authority))++s.authority_missing;
      else{
        if(!ClientMotionReadTerrain(room,s.terrain))++s.terrain_missing;
        s.terrain.path_block=s.path_block;s.terrain.path_valid=s.path_valid;
        if(s.predictor.Predict(s.body,s.terrain,authority,start,jump,predicted,reason)){
          s.predicted_body=s.body.address;s.delta_x=predicted.delta_x;s.delta_y=predicted.delta_y;s.lead_h=predicted.lead_h;
        }
      }
    }
    const bool interpolate=Settings.motion.interpolation&&s.latest_h>=0;
    // P9：插值开关关闭时远端对象不覆盖，显示即权威值；本机预测仍按自己的开关进行。
    if(!interpolate){
      if(!Settings.motion.interpolation)++s.disabled_frames;
      for(auto &[address,track]:s.tracks){track.shown=track.extrapolating=false;track.fuse_x=track.fuse_y=0;}
    }
    // Host 暂停：T_w 停在原处；恢复后清空到达窗口重新建立时钟，避免把暂停时长当成网络延迟。
    if(paused){++s.paused_frames;s.was_paused=true;}
    else if(s.was_paused){s.was_paused=false;s.arrival_count=s.arrival_head=s.stale_count=s.stale_head=0;s.tw_valid=false;++s.tw_resets;}
    bool jumped=false;
    if(interpolate&&!paused){
      double host=0;
      if(s.HostNow(Impl::LocalH(start),host)){
        const double target=host-s.delay;
        if(!s.tw_valid){s.tw=target;s.tw_valid=true;jumped=true;}
        else{
          const double step=(std::min)(double(dt_us)*kHalfFramesPerMicro*s.rate,kMaxStep);
          s.tw+=step;
          const double error=target-s.tw;
          if(std::abs(error)>kTimelineJump){s.tw=target;++s.tw_jumps;jumped=true;}
          else s.tw+=std::clamp(error,-kChaseRate*step,kChaseRate*step);
        }
      }
      s.delay_us.Observe(std::uint64_t(s.delay/kHalfFramesPerMicro));
    }
    // 本机身体：同一控制组序号的全部身体（多身体角色一并排除）。
    std::array<void *,8> locals{};unsigned local_count=0;
    for(unsigned i=0;i<mine_count;++i)locals[local_count++]=mine[i]->address;
    bool overflow=false;
    const unsigned n=ClientMotionCollect(room,locals.data(),local_count,s.entities.data(),unsigned(s.entities.size()),overflow);
    if(overflow)++s.collect_overflow;
    s.excluded_local=s.excluded_attached=s.excluded_laser=0;
    const bool extrapolate=Settings.motion.extrapolation;
    for(unsigned i=0;i<n;++i){
      const auto &e=s.entities[i];
      auto at=s.tracks.find(e.address);
      if(e.local||e.attached||e.laser){
        e.local?++s.excluded_local:e.attached?++s.excluded_attached:++s.excluded_laser;
        if(at!=s.tracks.end()){at->second.shown=at->second.extrapolating=false;at->second.fuse_x=at->second.fuse_y=0;}
        ClientMotionWrite w;w.address=e.address;w.identity=e.identity;w.slot=e.slot;
        if(e.local&&e.address==s.predicted_body){
          // 本机身体：预测位置＋平滑偏移，并写预测速度供相机前视（RE-P4）。
          w.display_x=e.x+s.delta_x;w.display_y=e.y+s.delta_y;
          w.velocity=true;w.display_vx=predicted.vx;w.display_vy=predicted.vy;
          s.writes.push_back(w);++s.predict_written;
        }else if(e.follow&&e.owner&&e.owner==s.predicted_body&&(s.delta_x!=0||s.delta_y!=0)){
          // 5.6：环绕跟班、举起物、Parent 指向本机身体的对象随身体平移同一个 Δ。
          w.display_x=e.x+s.delta_x;w.display_y=e.y+s.delta_y;
          s.writes.push_back(w);++s.follow_written;
        }
        continue;
      }
      if(!interpolate||!s.tw_valid)continue;
      if(at==s.tracks.end()||at->second.identity!=e.identity||!at->second.count)continue;
      auto &t=at->second;
      float x=0,y=0;
      // P6（7.3）：开关打开且本机正在预测时，敌弹单独外推到 T_p（T_w＋D＋预测领先量），上限放宽到 500ms。
      const bool lead=Settings.motion.projectile_lead&&e.identity.type==9&&s.predicted_body;
      const auto mode=lead?s.EvaluateLead(t,s.tw+s.delay+s.lead_h,extrapolate,x,y):s.Evaluate(t,s.tw,e.identity.type,extrapolate,x,y);
      if(lead)++s.projectile_lead;
      ++s.modes[unsigned(mode)];
      // 7.2：外推中途到了新样本，把旧显示位置与新求值之差作为偏移，约 100ms 衰减到 0。
      if(t.fresh&&t.shown&&t.extrapolating&&!t.cut&&!jumped){
        const float fx=t.shown_x-x,fy=t.shown_y-y;
        if(std::hypot(fx,fy)<=kFuseLimit){t.fuse_x=fx;t.fuse_y=fy;t.fuse_us=start;++s.fusions;}
        else{t.fuse_x=t.fuse_y=0;++s.fuse_dropped;}
      }
      if(jumped||t.cut){t.fuse_x=t.fuse_y=0;}
      t.fresh=t.cut=false;
      if(t.fuse_x!=0||t.fuse_y!=0){
        const auto age=start-t.fuse_us;
        if(age>=kFuseMicros)t.fuse_x=t.fuse_y=0;
        else{const float k=1.f-float(age)/float(kFuseMicros);x+=t.fuse_x*k;y+=t.fuse_y*k;}
      }
      t.shown=true;t.shown_x=x;t.shown_y=y;t.extrapolating=mode==Mode::Extrap||mode==Mode::Capped;
      // 与当前权威值逐位相同就不写，静止对象不产生写入。
      if(x==e.x&&y==e.y)continue;
      ClientMotionWrite w;w.address=e.address;w.identity=e.identity;w.slot=e.slot;w.display_x=x;w.display_y=y;
      s.writes.push_back(w);
    }
    s.writes_last=unsigned(s.writes.size());s.writes_max=(std::max)(s.writes_max,s.writes_last);
    s.prepared_us=start;
  }catch(...){s.writes.clear();}
  s.prepare_us.Observe(MetricMicros()-start);
}

bool ClientMotionOverlay::Enter(void *room) noexcept {
  auto &s=*impl_;
  if(!IsMain()||s.entered||s.writes.empty())return false;
  const auto start=MetricMicros();
  // 写入表只对本轮显示推进有效；显示推进没有运行（恢复、阻断）时不写旧值。
  if(start-s.prepared_us>kStalePrepareMicros){++s.apply_stale;s.writes.clear();return false;}
  unsigned missing=0,changed=0;
  ClientMotionSwap(room,s.writes.data(),unsigned(s.writes.size()),true,missing,changed);
  s.apply_missing+=missing;s.entered=true;
  s.apply_us.Observe(MetricMicros()-start);
  return true;
}

void ClientMotionOverlay::Exit(void *room) noexcept {
  auto &s=*impl_;
  if(!s.entered)return;
  const auto start=MetricMicros();
  unsigned missing=0,changed=0;
  ClientMotionSwap(room,s.writes.data(),unsigned(s.writes.size()),false,missing,changed);
  s.restore_missing+=missing;s.restore_changed+=changed;s.entered=false;
  s.apply_us.Observe(MetricMicros()-start);
}

std::string ClientMotionOverlay::Fields() const {
  const auto &s=*impl_;
  std::string out;
  const auto add=[&](const char *name,std::uint64_t value){out+="stream.motion.";out+=name;out+="=";out+=std::to_string(value);out+="\n";};
  const auto hist=[&](const char *name,const Duration &d){
    const std::string n(name);
    add((n+"_count").c_str(),d.count);add((n+"_p50").c_str(),d.Percentile(50));
    add((n+"_p95").c_str(),d.Percentile(95));add((n+"_max").c_str(),d.max_us);
  };
  add("interpolation",Settings.motion.interpolation);add("extrapolation",Settings.motion.extrapolation);
  add("tracks",s.tracks.size());add("track_overflow",s.track_overflow);add("collect_overflow",s.collect_overflow);
  add("installs_world",s.installs_world);add("installs_body",s.installs_body);add("resets",s.resets);
  add("rate_milli",std::uint64_t(s.rate*1000));add("delay_milli_h",std::uint64_t(s.delay*1000));
  hist("delay_us",s.delay_us);add("tw_jumps",s.tw_jumps);add("tw_resets",s.tw_resets);
  add("eval_before",s.modes[unsigned(Mode::Before)]);add("eval_interp",s.modes[unsigned(Mode::Interp)]);
  add("eval_extrap",s.modes[unsigned(Mode::Extrap)]);add("eval_extrap_capped",s.modes[unsigned(Mode::Capped)]);
  add("eval_hold",s.modes[unsigned(Mode::Hold)]);hist("extrapolate_us",s.extrapolate_us);
  add("fusions",s.fusions);add("fuse_dropped",s.fuse_dropped);add("cuts",s.cuts);
  add("identity_resets",s.identity_resets);add("out_of_order",s.out_of_order);
  add("writes_last",s.writes_last);add("writes_max",s.writes_max);
  add("excluded_local",s.excluded_local);add("excluded_attached",s.excluded_attached);add("excluded_laser",s.excluded_laser);
  hist("prepare_us",s.prepare_us);hist("apply_us",s.apply_us);
  add("apply_stale",s.apply_stale);add("apply_missing",s.apply_missing);
  add("restore_missing",s.restore_missing);add("restore_changed",s.restore_changed);
  add("disabled_frames",s.disabled_frames);add("paused_frames",s.paused_frames);
  add("prediction",Settings.motion.prediction);add("projectile_lead",Settings.motion.projectile_lead);
  add("predict_written",s.predict_written);add("follow_written",s.follow_written);
  add("path_valid",s.path_valid);add("path_updates",s.path_updates);add("path_missing",s.path_missing);
  add("body_missing",s.body_missing);add("authority_missing",s.authority_missing);add("multi_body",s.multi_body);
  add("terrain_missing",s.terrain_missing);add("projectile_lead_evals",s.projectile_lead);
  out+=s.predictor.Fields();
  return out;
}
} // namespace cs::game
