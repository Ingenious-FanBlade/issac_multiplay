// 预测插值阶段 2：本机身体移动预测（设计第 5 节，P2=M-a）。
// 半步顺序与常数逐项对照 J273：Entity::Update 0x6A9CF0、ControlMovement 0x772540、GetMovementInput 0x771D70、
// PlayerCollideWithGrid 0x6A6A40（非静默）、GetGridCollisionAtPos 0x6B1C20、Room::GetGridCollision 0x7E6AF0、
// Room::GetGridIndex 0x808530、Room::GetClampedPosition 0x8087F0（含 L 形房 0x814B70 表 0xBAC490）。
// 全部用 float 按原式顺序计算，尽量与 Host 逐位一致；未复刻的分支在各处注明，由对账纠正。
#include "game/client/presentation/client_motion_predict.h"
#include "replica/stream_metrics.h"
#include "runtime.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace cs::game {
namespace {
// —— 实体标志位（Entity+0x168，64 位）——
constexpr std::uint64_t kFlagSlow=1ull<<7;          // FLAG_SLOW：滑行系数强制 0
constexpr std::uint64_t kFlagConfusion=1ull<<9;     // FLAG_CONFUSION：输入取反（Game+0x25B8C≥2 时）
constexpr std::uint64_t kFlagGravity=1ull<<15;      // FLAG_APPLY_GRAVITY：横版房
constexpr std::uint64_t kFlagSlippery=1ull<<27;     // FLAG_SLIPPERY_PHYSICS：滑行计数置 60
constexpr std::uint64_t kFlagKnockedBack=1ull<<47;  // FLAG_KNOCKED_BACK：击退由 Entity::Update 处理
constexpr std::uint64_t kFlagThrown=1ull<<58;       // FLAG_THROWN：摩擦 1.0
// —— 碰撞类别 ——
constexpr std::int32_t kGridNone=0,kGridSolid=3,kGridWall=4,kGridWallExceptPlayer=5;
constexpr std::int32_t kEntityWalls=3,kEntityGround=5;  // EntityGridCollisionClass
// L 形房间两块矩形的格坐标（0xBAC490 + 32×shape，shape 9—12），列行交替：R1 左上、R1 右下、R2 左上、R2 右下。
constexpr std::array<std::array<int,8>,4> kLShapes{{
  {14,1,26,7,1,8,26,14},{1,1,13,7,1,8,26,14},{1,1,26,7,14,8,26,14},{1,1,26,7,1,8,13,14}}};

constexpr int kPendingLimit=32;     // 设计 5.4：待确认 tick 上限（最新已发目标−权威 tick），超出停止预测
constexpr unsigned kInputRing=256;  // 输入环容量（按序号取模）；同一 tick 可有补发与重发多条命令
constexpr unsigned kChainLimit=64;  // 单次重放最多 tick 数（含起点）
constexpr float kSnapDistance=60.f; // P4：误差超过 1.5 格直接跳
constexpr double kHalfLifeMicros=50'000.0; // P4：平滑半衰期 50ms
constexpr float kHitEpsilon=0.01f;  // 对账命中：同一 tick 的旧预测与新重放之差小于此值
// 显示时钟（2026-11-20 实机修正）：显示 tick 不再直接取最新已发目标。目标由 GetTickCount64（约 16ms 量化）
// 投影，每帧前进 0、1 或 2 tick 不均匀，本机身体与跟随它的相机随之每帧走 0／9／18 单位，整屏抖动。
// 改为按 Host 速率（每秒 60 个半帧）在本地连续推进小数 tick，并以下列参数缓慢追赶“最新目标−滞后”。
constexpr double kDisplayLagTicks=1.0;        // 显示落后最新已发目标的 tick 数，给目标抖动留余量
constexpr double kDisplaySlewHalfLife=150'000.0; // 追赶误差的半衰期（微秒）
constexpr double kDisplayJumpTicks=6.0;       // 偏离超过此值直接跳

// 原生的 (int)((v-origin)/40+0.5)：截断取整，与 C 强转一致。
int Cell(float v,float origin) noexcept {return static_cast<int>((v-origin)/40.f+0.5f);}
// Room::GetGridIndex（0x808530）。
int GridIndex(const ClientMotionTerrain &t,float x,float y) noexcept {
  const int col=Cell(x,40.f);
  if(col<0||col>=t.width)return -1;
  const int row=Cell(y,120.f);
  if(row<0||row>=t.height)return -1;
  return col+row*t.width;
}
// Room::GetGridCollision（0x7E6AF0）：格子实体类别优先，其余按同步的寻路 cost 阻挡类别。
std::int32_t RoomCollision(const ClientMotionTerrain &t,int index) noexcept {
  if(t.special_zero)return kGridNone;
  if(index<0)return kGridWall;
  if(index>=int(kClientMotionGridCells))return kGridNone;
  if(t.entity_class[index]>=0)return t.entity_class[index];
  if(!t.path_valid)return kGridNone;  // 5.3.1：影子表未到时按无阻挡
  return (int(t.path_block[std::size_t(index/4)])>>((index%4)*2))&3;
}
// 格号 → 格中心（原生 (idx%w)*40+40、(idx/w)*40+120；idx=-1 时按 C 的取模与整除）。
float CellX(const ClientMotionTerrain &t,int index) noexcept {return float(index%t.width)*40.f+40.f;}
float CellY(const ClientMotionTerrain &t,int index) noexcept {return float(index/t.width)*40.f+120.f;}
int TableIndex(const ClientMotionTerrain &t,int col,int row) noexcept {
  return col<0||col>=t.width||row<0||row>=t.height?-1:col+row*t.width;
}
// Room::GetClampedPosition（0x8087F0）：边距 left/top/right/bottom；L 形房按 0x814B70 的两块矩形处理凹角。
void Clamp(const ClientMotionTerrain &t,float x,float y,float ml,float mt,float mr,float mb,float &ox,float &oy) noexcept {
  const float cx=(std::min)((std::max)(x,ml+t.tl_x),t.br_x-mr);
  float cy=(std::min)((std::max)(y,mt+t.tl_y),t.br_y-mb);
  ox=cx;oy=cy;
  if(t.shape<9||t.shape>12)return;
  const auto &s=kLShapes[std::size_t(t.shape-9)];
  const auto at=[&](int col,int row){return TableIndex(t,col,row);};
  const int i0=at(s[0],s[1]),i1=at(s[2],s[3]),i2=at(s[4],s[5]),i3=at(s[6],s[7]);
  const float r1l=CellX(t,i0)-20.f,r1r=CellX(t,i1)+20.f,r1b=CellY(t,i1)+20.f;
  const float r2l=CellX(t,i2)-20.f,r2t=CellY(t,i2)-20.f,r2r=CellX(t,i3)+20.f;
  const float v10=r1l+ml,v11=r1r-mr,v12=r2r-mr,v23=r2l+ml;
  float v13=r2t+mt;
  if((r2r-mr)-v23<=(r1r-mr)-v10)v13=r1b-mb;
  switch(t.shape){
    case 9:
      if(v10<=cx||v13<=cy)return;
      if(v13-cy>v10-cx){ox=v10;oy=cy;}else{ox=cx;oy=v13;}
      return;
    case 10:
      if(cx<=v11||v13<=cy)return;
      if(v13-cy>cx-v11){ox=v11;oy=cy;}else{ox=cx;oy=v13;}
      return;
    case 11:
      if(v23<=cx||cy<=v13)return;
      if(cy-v13>v23-cx){ox=v23;oy=cy;}else{ox=cx;oy=v13;}
      return;
    default:
      if(cx<=v12||cy<=v13)return;
      if(cy-v13>cx-v12){ox=v12;oy=cy;}else{ox=cx;oy=v13;}
      return;
  }
}
} // namespace

std::int32_t ClientMotionCollisionAt(const ClientMotionBody &body,const ClientMotionTerrain &t,float x,float y) noexcept {
  const int index=GridIndex(t,x,y);
  std::int32_t n=RoomCollision(t,index);
  // 便便：普通便便只在格中心 18 单位内阻挡；大便便（变体 7—10）原生按 0x713970 求中心、半径 38，未复刻，按整格阻挡。
  if(n==kGridSolid&&index>=0&&index<int(kClientMotionGridCells)&&t.entity_kind[index]==1){
    const int own=t.entity_index[index];
    const float dx=x-(float(own%t.width)*40.f+40.f),dy=y-(float(own/t.width)*40.f+120.f);
    if(dy*dy+dx*dx>324.f)n=kGridNone;
  }
  // 墙上开着的门（+652&0x10 且若干条件）原生可改为 5；条件含未核函数，首版不复刻，保持 WALL。
  if(n==kGridWallExceptPlayer){
    // 0x6B1C20 LABEL_41：非 0 变体、联机幽灵、带 parent 的玩家仍视为墙；0x7D1C90 未核，不计。
    if(body.variant||body.coop_ghost||body.has_parent)return kGridWall;
  }
  return n;
}

const char *ClientMotionIneligible(const ClientMotionBody &b) noexcept {
  if(b.too_many_points)return "points";
  if(b.player_type==19||b.player_type==20)return "jacob_esau";       // 设计 5.5：首版不预测
  if(b.player_type==35||b.player_type==40)return "tainted_forgotten";
  if(b.airborne)return "airborne";
  if(b.has_target)return "target";
  if(b.in_minecart)return "minecart";
  if(b.parent_player)return "parent_player";
  if(b.frozen)return "frozen";
  if(b.suplex)return "suplex";
  if(b.pony>0)return "pony";
  if(b.chariot)return "reverse_chariot";
  if(b.spin)return "spin_to_win";
  if(b.mars)return "mars";
  if(b.mega_mush)return "mega_mush";
  if(b.flags&kFlagKnockedBack)return "knocked_back";
  if((b.flags&kFlagGravity)&&!b.can_fly)return "gravity";
  if(!(b.move_speed>0.f)||!std::isfinite(b.move_speed))return "move_speed";
  return nullptr;
}

void ClientMotionHalfStep(const ClientMotionBody &b,const ClientMotionTerrain &t,
                          float move_x,float move_y,ClientMotionKinematic &k) noexcept {
  // —— 1. Entity::Update：按旧速度积分（击退分支不预测，资格已排除）——
  {
    float f=b.friction;const float ts=b.time_scale;
    if(ts!=1.f)f=f/((f+ts)-(f*ts));
    k.vx=f*k.vx;k.vy=f*k.vy;
    k.y=ts*k.vy+k.y;k.x=ts*k.vx+k.x;
  }
  // 滑行计数：FLAG_SLIPPERY_PHYSICS 置 60，否则每次更新减 1（不低于 0，RE-P1 第 2 节第 8 条）。
  k.slide=(b.flags&kFlagSlippery)?60:(std::max)(0,k.slide-1);
  // —— 2. ControlMovement：由输入算新速度 ——
  const bool gravity=(b.flags&kFlagGravity)&&!b.can_fly;
  float ix=0,iy=0;
  if(b.controls&&b.cooldown<=0){
    // GetActionValue：LEFT/RIGHT/UP/DOWN = max(∓move,0)/32767（core/participant.cpp Magnitude）。
    const float left=float((std::max)(-int(move_x),0))/32767.f,right=float((std::max)(int(move_x),0))/32767.f;
    const float up=float((std::max)(-int(move_y),0))/32767.f,down=float((std::max)(int(move_y),0))/32767.f;
    float x=right+(0.f-left),y=down+(0.f-up);
    if(b.confused){x=x*-1.f;y=y*-1.f;}
    if(b.reversed){x=x*-1.f;y=y*-1.f;}
    if(b.axis_aligned){
      const float len=std::sqrt(y*y+x*x);
      if(std::fabs(x)<=std::fabs(y)){x=0.f*len;y=(y>=0.f?1.f:-1.f)*len;}
      else{y=0.f*len;x=(x>=0.f?1.f:-1.f)*len;}
    }
    if(b.mirrored)x=x*-1.f;
    if(gravity)y=0.f;
    const float len=std::sqrt(x*x+y*y);
    if(len>1.f){x=x/len;y=y/len;}
    ix=x;iy=y;
  }
  if(b.input_lock||b.suplex){ix=0;iy=0;}
  const float scale=b.move_speed*0.25f+0.75f;
  ix=ix*scale;iy=scale*iy;
  float s=float(k.slide)/60.f;
  if(b.flags&kFlagSlow)s=0.f;
  // 垂直阻尼：输入方向（未归一化的缩放输入）以外的速度分量按 sqrt(0.2s+0.8) 衰减。
  if((ix!=0.f||iy!=0.f)&&!gravity){
    const float d=(ix*k.vx)+(k.vy*iy);
    const float px=k.vx-(ix*d),py=k.vy-(d*iy);
    const float damp=std::sqrt(s*0.2f+0.8f);
    k.vx=px*damp+(ix*d);k.vy=py*damp+(d*iy);
  }
  const float base_x=k.vx,base_y=k.vy;
  float along=((s*0.7f)-0.2f)*((base_x*ix)+(base_y*iy));
  if(along<=0.f)along=0.f;
  const float a=(along/b.move_speed+3.f)*0.4f;
  float friction;
  if(b.flags&kFlagThrown)friction=1.f;
  else if(b.chariot||b.suplex>1)friction=std::sqrt(s*0.51f+0.45f);
  else friction=std::sqrt(s*0.185f+0.775f);
  const float cap=b.move_speed*4.41f;
  const float m=((a*0.25f)-a)*s+a;
  float nx=((ix*0.5f)*m)+base_x;
  float ny=((iy*0.5f)*m)+base_y;
  if(s>0.f){
    const float sq=(nx*nx)+(ny*ny);
    if(sq>cap*cap){
      // 超过上限时缩回到加速前的速度大小（不是缩到上限），与原式一致。
      const float before=std::sqrt((base_x*base_x)+(base_y*base_y));
      const float len=std::sqrt(sq);
      if(len>0.f){const float r=before/len;nx=r*nx;ny=ny*r;}
    }
  }
  k.vx=nx*friction;
  k.vy=gravity?ny+0.225f:friction*ny;
  // —— 3. PlayerCollideWithGrid(p,0)：房间钳制、采样点法线、速度响应、二分推出 ——
  if(!b.grid_class||!b.point_count||b.in_minecart||!t.valid)return;
  if(t.special_zero){
    float cx=0,cy=0;
    Clamp(t,k.x-0.f,k.y-20.f,b.size,b.size,b.size,b.size,cx,cy);
    k.x=cx+0.f;k.y=cy+20.f;
  }else{
    float cx=0,cy=0;
    Clamp(t,k.x,k.y,0.f,0.f,0.f,0.f,cx,cy);
    if((GridIndex(t,k.x,k.y)<0||ClientMotionCollisionAt(b,t,k.x,k.y)==kGridWall)&&t.room_type!=16){k.x=cx;k.y=cy;}
  }
  float sum_x=0.f,sum_y=0.f;
  for(unsigned i=0;i<b.point_count;++i){
    const float px=k.x+b.point_x[i],py=b.point_y[i]+k.y;
    const int index=GridIndex(t,px,py);
    if(index<0)continue;
    const std::int32_t c=ClientMotionCollisionAt(b,t,px,py);
    const bool door=index<int(kClientMotionGridCells)&&t.entity_class[index]>=0&&t.entity_kind[index]==3;
    bool hit=false;
    if(b.grid_class==kEntityWalls)hit=c==kGridWall||(c==kGridSolid&&door);
    else if(b.grid_class==kEntityGround)hit=c!=kGridNone&&c!=kGridWallExceptPlayer;
    // 原生在非静默时另有推石头、门回调、Mega Mush 破坏等分支会跳过累加；均不复刻，由对账纠正。
    if(hit){sum_x=sum_x+b.point_x[i];sum_y=sum_y+b.point_y[i];}
  }
  const float sq=(sum_y*sum_y)+(sum_x*sum_x);
  if(sq<0.001f)return;
  float nx2=sum_x,ny2=sum_y;
  const float len=std::sqrt(sq);
  if(len>0.f){const float inv=1.f/len;nx2=sum_x*inv;ny2=sum_y*inv;}
  const float into=((-nx2)*k.vx)-(k.vy*ny2);
  if(into<0.f){
    const float r=(into*0.9f)-0.1f;
    const float vx=(r*nx2)+k.vx;
    k.vy=(ny2*r)+k.vy;k.vx=vx;
  }
  const float reach=std::sqrt(((ny2*b.size_my)*(ny2*b.size_my))+((nx2*b.size_mx)*(nx2*b.size_mx)))*b.size;
  float hi=reach,lo=reach-(std::max)(std::sqrt((k.vx*k.vx)+(k.vy*k.vy)),2.f);
  const bool ground=b.grid_class==kEntityGround;
  for(int n=ground?12:6;n>0;--n){
    const float mid=((hi-lo)*0.5f)+lo;
    const float qx=(nx2*mid)+k.x,qy=(ny2*mid)+k.y;
    const std::int32_t c=ClientMotionCollisionAt(b,t,qx,qy);
    const bool free=ground?(c==kGridNone||c==kGridWallExceptPlayer):c!=kGridWall;
    if(free)lo=mid;else hi=mid;
  }
  const float back=lo-reach;
  k.x=(nx2*back)+k.x;k.y=(ny2*back)+k.y;
}

// —— 预测器 ——
struct ClientMotionPredictor::Impl final {
  struct Command final {std::uint64_t generation=0,sequence=0,target=0;std::int16_t x=0,y=0;};
  struct Link final {std::uint64_t tick=0;ClientMotionKinematic state{};};
  std::array<Command,kInputRing> ring{};
  std::uint64_t generation=0,latest_target=0;
  std::array<Link,kChainLimit> chain{},previous{};
  unsigned chain_count=0,previous_count=0;
  // 上一帧显示的纯预测（不含 e）及其 tick；e 为显示误差偏移（5.4 第 5 步）。
  bool have_shown=false;double shown_tick=0;float shown_x=0,shown_y=0;
  // 显示时钟：小数 tick（绝对 64 位 tick 的 double 值），见 kDisplayLagTicks。
  bool display_valid=false;double display_tick=0;
  std::uint64_t display_jumps=0,misses_odd=0,misses_even=0;
  float ex=0,ey=0;std::uint64_t last_us=0;
  std::uint32_t last_authority_tick=0;bool have_authority=false;
  void *last_body=nullptr;
  // 诊断
  replica::stream::Duration replay_us,error_milli;
  std::uint64_t frames=0,predicted=0,ineligible=0,hits=0,misses=0,gaps=0,snaps=0,jumps=0,
                authority_new=0,pending_overflow=0,inputs=0,input_conflicts=0,generation_resets=0,
                neutral_ticks=0,replay_ticks=0,replay_max=0,phase_unknown=0;
  const char *last_reason="";

  void ClearMotion() noexcept {
    chain_count=previous_count=0;have_shown=false;ex=ey=0;have_authority=false;last_body=nullptr;display_valid=false;
  }
  // tick 的输入：同代次中目标恰为该 tick 的最高序号命令；没有则为中性（与 Participant::Begin 相同）。
  bool InputFor(std::uint64_t tick,std::int16_t &x,std::int16_t &y) const noexcept {
    std::uint64_t best=0;
    for(const auto &c:ring)if(c.sequence&&c.generation==generation&&c.target==tick&&c.sequence>best){best=c.sequence;x=c.x;y=c.y;}
    return best!=0;
  }
  // Host tick 只携带低 32 位：取与最新已发目标最接近的 64 位值。
  std::uint64_t Expand(std::uint32_t low) const noexcept {
    std::uint64_t v=(latest_target&~0xFFFF'FFFFull)|low;
    if(v>latest_target+0x8000'0000ull&&v>=(1ull<<32))v-=1ull<<32;
    else if(v+0x8000'0000ull<latest_target)v+=1ull<<32;
    return v;
  }
  // 在当前链上按小数 tick 线性插值；超出链范围时返回 false。
  bool Sample(double at,ClientMotionKinematic &out) const noexcept {
    if(!chain_count)return false;
    const double first=double(chain[0].tick),last=double(chain[chain_count-1].tick);
    if(at<first-1e-6||at>last+1e-6)return false;
    double offset=at-first;if(offset<0)offset=0;
    auto i=unsigned(offset);if(i>=chain_count-1){out=chain[chain_count-1].state;return true;}
    const float f=float(offset-double(i));
    const auto &a=chain[i].state,&b=chain[i+1].state;
    out=a;out.x=a.x+(b.x-a.x)*f;out.y=a.y+(b.y-a.y)*f;out.vx=a.vx+(b.vx-a.vx)*f;out.vy=a.vy+(b.vy-a.vy)*f;
    return true;
  }
  const Link *Previous(std::uint64_t tick) const noexcept {
    for(unsigned i=0;i<previous_count;++i)if(previous[i].tick==tick)return &previous[i];
    return nullptr;
  }
};

ClientMotionPredictor::ClientMotionPredictor():impl_(std::make_unique<Impl>()) {}
ClientMotionPredictor::~ClientMotionPredictor()=default;

void ClientMotionPredictor::Reset() noexcept {
  auto &s=*impl_;s.ring={};s.latest_target=0;s.ClearMotion();
}

void ClientMotionPredictor::Input(std::uint64_t generation,std::uint64_t sequence,std::uint64_t target,
                                  std::int16_t move_x,std::int16_t move_y) noexcept {
  auto &s=*impl_;
  if(!sequence||!target)return;
  if(generation!=s.generation){s.ring={};s.latest_target=0;s.generation=generation;s.ClearMotion();++s.generation_resets;}
  auto &slot=s.ring[sequence%kInputRing];
  if(slot.sequence==sequence&&slot.generation==generation)++s.input_conflicts;
  slot={generation,sequence,target,move_x,move_y};
  s.latest_target=(std::max)(s.latest_target,target);
  ++s.inputs;
}

bool ClientMotionPredictor::Predict(const ClientMotionBody &body,const ClientMotionTerrain &terrain,
                                    const ClientMotionAuthority &authority,std::uint64_t now_us,bool jump,
                                    ClientMotionDisplay &out,const char *&reason) noexcept {
  auto &s=*impl_;
  ++s.frames;
  const auto dt=s.last_us&&now_us>s.last_us?now_us-s.last_us:0;s.last_us=now_us;
  reason=ClientMotionIneligible(body);
  const unsigned halves=(authority.phase>>1)&3u;
  if(!reason&&(!authority.phase||(halves!=1&&halves!=2))){reason="phase_unknown";++s.phase_unknown;}
  if(!reason&&!s.latest_target)reason="no_input";
  if(body.address!=s.last_body){jump=true;s.last_body=body.address;}
  const std::uint64_t tick=s.latest_target?s.Expand(authority.tick):0;
  if(!reason){
    // 待确认量按 tick 距离计：客机会为同一 tick 补发或重发多条命令（client.cpp 发送规则），条数不代表时长。
    const std::uint64_t pending=s.latest_target>tick?s.latest_target-tick:0;
    if(pending>std::uint64_t(kPendingLimit)){reason="pending_overflow";++s.pending_overflow;}
  }
  if(reason){
    // 不可预测：显示权威值；恢复资格时直接跳（5.4 第 6 步）。
    ++s.ineligible;s.last_reason=reason;s.ClearMotion();s.last_body=body.address;
    return false;
  }
  const auto start=replica::stream::MetricMicros();
  const bool fresh=!s.have_authority||authority.tick!=s.last_authority_tick;
  if(fresh){++s.authority_new;s.last_authority_tick=authority.tick;s.have_authority=true;}
  // 起点：权威位置与速度；滑行计数沿用上次重放中同一 tick 的本地值（不同步，RE-P3 第 4 节）。
  ClientMotionKinematic k{body.x,body.y,body.vx,body.vy,0};
  if(const auto *p=s.Previous(tick))k.slide=p->state.slide;
  else k.slide=(body.flags&kFlagSlippery)?60:0;
  std::swap(s.chain,s.previous);s.previous_count=s.chain_count;
  s.chain_count=0;s.chain[s.chain_count++]={tick,k};
  std::uint64_t last=tick;
  for(std::uint64_t t=tick+1;t<=s.latest_target&&s.chain_count<kChainLimit;++t){
    std::int16_t x=0,y=0;
    if(!s.InputFor(t,x,y))++s.neutral_ticks;
    for(unsigned h=0;h<halves;++h)ClientMotionHalfStep(body,terrain,float(x),float(y),k);
    s.chain[s.chain_count++]={t,k};last=t;
  }
  s.replay_ticks+=s.chain_count-1;s.replay_max=(std::max)(s.replay_max,std::uint64_t(s.chain_count-1));
  // 显示时钟：按 Host 速率连续推进小数 tick（每 tick halves 个半帧，每秒 60 个半帧），再追赶“最新目标−滞后”。
  const double goal=double(last)-kDisplayLagTicks;
  if(!s.display_valid||jump||std::fabs(goal-s.display_tick)>kDisplayJumpTicks){
    if(s.display_valid&&!jump)++s.display_jumps;
    s.display_tick=goal;s.display_valid=true;
  }else{
    s.display_tick+=double(dt)*60.0/1'000'000.0/double(halves);
    s.display_tick+=(goal-s.display_tick)*(1.0-std::pow(0.5,double(dt)/kDisplaySlewHalfLife));
  }
  // 夹在本条链范围内：不早于权威 tick，不晚于最新已发目标。
  s.display_tick=(std::max)(double(tick),(std::min)(double(last),s.display_tick));
  ClientMotionKinematic pure{};s.Sample(s.display_tick,pure);
  // 平滑（5.4 第 5 步）：上一帧显示的小数 tick 在新链中的值与旧值之差并入 e；e 按 50ms 半衰期衰减。
  if(s.have_shown&&!jump){
    if(dt){const float decay=float(std::pow(0.5,double(dt)/kHalfLifeMicros));s.ex*=decay;s.ey*=decay;}
    ClientMotionKinematic same{};
    if(s.Sample(s.shown_tick,same)){
      const float dx=s.shown_x-same.x,dy=s.shown_y-same.y;
      s.ex+=dx;s.ey+=dy;
      if(fresh){
        const float error=std::hypot(dx,dy);
        if(error<kHitEpsilon)++s.hits;
        else{++s.misses;(authority.phase&1u?s.misses_odd:s.misses_even)++;s.error_milli.Observe(std::uint64_t(error*1000.f));}
      }
    }else ++s.gaps;
    if(std::hypot(s.ex,s.ey)>kSnapDistance){s.ex=s.ey=0;++s.snaps;}
  }else{s.ex=s.ey=0;if(jump)++s.jumps;}
  s.have_shown=true;s.shown_tick=s.display_tick;s.shown_x=pure.x;s.shown_y=pure.y;
  out.x=pure.x+s.ex;out.y=pure.y+s.ey;out.vx=pure.vx;out.vy=pure.vy;
  out.delta_x=out.x-body.x;out.delta_y=out.y-body.y;out.lead_h=(s.display_tick-double(tick))*double(halves);
  ++s.predicted;s.last_reason="";
  s.replay_us.Observe(replica::stream::MetricMicros()-start);
  return true;
}

std::string ClientMotionPredictor::Fields() const {
  const auto &s=*impl_;
  std::string out;
  const auto add=[&](const char *name,std::uint64_t value){out+="stream.motion.predict_";out+=name;out+="=";out+=std::to_string(value);out+="\n";};
  const auto hist=[&](const char *name,const replica::stream::Duration &d){
    const std::string n(name);
    add((n+"_count").c_str(),d.count);add((n+"_p50").c_str(),d.Percentile(50));
    add((n+"_p95").c_str(),d.Percentile(95));add((n+"_max").c_str(),d.max_us);
  };
  add("enabled",Settings.motion.prediction);add("frames",s.frames);add("predicted",s.predicted);add("ineligible",s.ineligible);
  out+="stream.motion.predict_reason=";out+=s.last_reason;out+="\n";
  add("authority_new",s.authority_new);add("hits",s.hits);add("misses",s.misses);add("gaps",s.gaps);
  add("snaps",s.snaps);add("jumps",s.jumps);add("pending_overflow",s.pending_overflow);add("phase_unknown",s.phase_unknown);
  add("display_jumps",s.display_jumps);add("misses_odd",s.misses_odd);add("misses_even",s.misses_even);
  add("display_lead_milli_tick",std::uint64_t((s.display_valid&&s.chain_count?(s.display_tick-double(s.chain[0].tick)):0)*1000.0));
  add("inputs",s.inputs);add("input_conflicts",s.input_conflicts);add("generation_resets",s.generation_resets);
  add("neutral_ticks",s.neutral_ticks);add("replay_ticks",s.replay_ticks);add("replay_max",s.replay_max);
  hist("replay_us",s.replay_us);hist("error_milli",s.error_milli);
  return out;
}
} // namespace cs::game
