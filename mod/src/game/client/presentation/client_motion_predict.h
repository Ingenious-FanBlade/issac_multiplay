#pragma once
// 预测插值阶段 2：本机身体移动预测（设计第 5 节，P2=M-a）。
// 速度模型与格子碰撞响应按 J273 逆向结果用 C++ 复刻（RE-P1 movement.md、RE-P2 collision.md）；
// 原生只用来读参数与地形（覆盖层外的权威值），不调用 ControlMovement／PlayerCollideWithGrid。
// 本头文件同时被 C++17 的 SDK 单元和 C++20 的逻辑单元包含，只用 C++17 可用的类型。
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace cs::game {
constexpr unsigned kClientMotionGridCells=448;     // Room 格子总数（寻路表 i32[448]）
constexpr unsigned kClientMotionGridPoints=128;    // 碰撞采样点上限（Entity+0x178 vector，由 SetSize 0x6A7880 按 EntityConfig GridCollisionPoints 生成；原版 Player／Coplayer 为 40）

// 一个本机身体在半步移动中要用的权威参数与预测资格事实，覆盖层外从原生对象读取。
// 偏移见 RE-P3 eligibility.md 第 4 节与本阶段补核（minecart +0x3D4、freezeCountdown +0x218）。
struct ClientMotionBody final {
  void *address=nullptr;
  // —— 运动状态与参数 ——
  float x=0,y=0,vx=0,vy=0;                 // pos +0x33C、velocity +0x360（玩家速度单位为每半步）
  float move_speed=1,friction=1,time_scale=1;// MoveSpeed +0x155C、Friction +0x36C、时间缩放 +0x39C
  float size=10,size_mx=1,size_my=1;       // size +0x370、尺寸倍率 +0x374/+0x378
  std::uint64_t flags=0;                   // 实体标志 +0x168
  std::int32_t grid_class=0;               // EntityGridCollisionClass +0x184
  std::int32_t variant=0,player_type=0;    // Entity+0x2C、Player _playerType +0x13BC
  unsigned point_count=0;                  // 碰撞采样点个数（超过上限时视为不可预测）
  std::array<float,kClientMotionGridPoints> point_x{},point_y{};
  // —— 输入相关 ——
  // 以下标“Host 资格位”的成员客机原生不同步，ClientMotionReadBody 不读，由 ClientMotionApplyGate 按 Body predict_gate 填写。
  bool controls=true;                      // ControlsEnabled +0x410（Host 资格位 bit0）
  std::int32_t cooldown=0;                 // ControlsCooldown +0x414（Host 资格位 bit1，只表达 >0）
  bool can_fly=false,coop_ghost=false;     // CanFly +0x1564、联机幽灵 +0x207D
  bool input_lock=false;                   // +0x1FE0 非零时输入清零（含义未知，RE-P3；Host 资格位 bit2）
  bool has_parent=false;                   // parent +0x3BC 非空（GetCollisionAt 的门规则）
  // GetMovementInput（0x771D70）的变换：混乱、反转种子 12、轴对齐种子 76、镜像世界取反 x。
  bool confused=false,reversed=false,axis_aligned=false,mirrored=false;
  // —— 资格事实（任一成立即不预测，见 ClientMotionIneligible）——
  bool airborne=false;                     // posOffset.y +0x350<0（已同步）或 +0x1E54<0（Host 资格位 bit3）
  bool has_target=false,in_minecart=false,parent_player=false;
  bool frozen=false;                       // PAUSE 临时效果 478（已同步）或 freezeCountdown +0x218>0（Host 资格位 bit5）
  std::int32_t suplex=0,pony=0;            // +0x1F08（已同步）、+0x1874（Host 资格位 bit4，只表达 >0）
  bool chariot=false,spin=false,mars=false,mega_mush=false; // NullItem 63／50，道具效果 593／625
  bool too_many_points=false;
  bool gate_missing=false;                 // Host 资格位缺失或无效：资格不可判，不预测
};

// 当前房间的地形快照：每格 GridEntity 的碰撞类别（无则 -1）与少量类型信息。
// 没有 GridEntity 的格子由 path_block（Host 同步的寻路 cost 阻挡类别，设计 5.3.1）决定。
struct ClientMotionTerrain final {
  bool valid=false;
  std::int32_t width=0,height=0,room_type=0,shape=0;
  float tl_x=0,tl_y=0,br_x=0,br_y=0;       // Room+0x14..+0x20 房间边界
  bool special_zero=false;                 // RoomConfig StageId 35 且 Type 16：GetGridCollision 恒 0（原样复刻，含义未核）
  std::array<std::int8_t,kClientMotionGridCells> entity_class{}; // GridEntity +0x3C；-1 无实体
  // 0 其他；1 普通便便（类型 14，变体不在 7—10）；2 大便便（变体 7—10）；3 门（类型 16）
  std::array<std::uint8_t,kClientMotionGridCells> entity_kind{};
  std::array<std::int16_t,kClientMotionGridCells> entity_index{}; // GridEntity 自身格号 +0x24
  // 设计 5.3.1：每格 2 位（0 无、1 PIT、3 SOLID），低位在前；path_valid=false 时按全无阻挡查询。
  std::array<std::uint8_t,kClientMotionGridCells/4> path_block{};
  bool path_valid=false;
};

// 预测中的运动状态：位置、速度（每半步）与本地维护的滑行计数（+0x1650 不同步，RE-P3 第 4 节）。
struct ClientMotionKinematic final {
  float x=0,y=0,vx=0,vy=0;
  std::int32_t slide=0;
};

// 原生侧（C++17 SDK 单元）：读本机身体参数。address 必须仍在 room 的 renderEL 中且为玩家，否则返回 false。
bool ClientMotionReadBody(void *room,void *address,ClientMotionBody &out) noexcept;
// 原生侧：读当前房间的格子实体表与边界；不读、不写原生寻路表 Room+0x76C。
bool ClientMotionReadTerrain(void *room,ClientMotionTerrain &out) noexcept;

// 不可预测的原因（静态名称），可预测时返回 nullptr（设计 5.5、RE-P3）。
const char *ClientMotionIneligible(const ClientMotionBody &body) noexcept;
// 一个玩家半步：Entity::Update 积分 → ControlMovement 速度 → PlayerCollideWithGrid（非静默）响应与推出。
// move_x／move_y 为输入命令的 move 分量（int16/32767，未变换），按 GetMovementInput 规则在内部变换。
void ClientMotionHalfStep(const ClientMotionBody &body,const ClientMotionTerrain &terrain,
                          float move_x,float move_y,ClientMotionKinematic &state) noexcept;
// 地形查询（复刻 0x6B1C20 对玩家的规则），返回 GridCollisionClass。供预测与诊断使用。
std::int32_t ClientMotionCollisionAt(const ClientMotionBody &body,const ClientMotionTerrain &terrain,
                                     float x,float y) noexcept;

// 一份本机身体的权威对账样本（设计 5.7）：Body 记录的三个合成字段。
struct ClientMotionAuthority final {
  std::uint32_t tick=0;      // Host Authority tick 低 32 位
  std::uint32_t sequence=0;  // 已处理的最高输入序号低 32 位
  std::uint8_t phase=0;      // bit0 奇半帧结束；bit1—2 本 tick 半步数（1 或 2）；0 表示未知
  // 预测资格位（Body predict_gate，2026-11-20）：bit0 ControlsEnabled、bit1 Cooldown>0、bit2 输入锁、bit3 跳跃、
  // bit4 小马冲刺、bit5 冻结、bit7 有效。这些原生成员不同步，客机本地值不可信，只能用 Host 同窗值。
  std::uint8_t gate=0;
};
// 用 Host 资格位覆盖身体中“客机本地不可信”的成员；gate 无效（bit7=0）时返回 false，调用方不预测。
bool ClientMotionApplyGate(ClientMotionBody &body,std::uint8_t gate) noexcept;
// 预测的显示结果：位置为预测值加平滑偏移 e，速度为预测速度（供相机前视），delta 为显示位置减权威位置。
struct ClientMotionDisplay final {
  float x=0,y=0,vx=0,vy=0,delta_x=0,delta_y=0;
  double lead_h=0;   // 预测领先权威的半帧数（(最新目标 tick−权威 tick)×每 tick 半步数），供 P6 敌弹外推
};

// 主线程唯一的本机身体预测器（设计 5.4）：待确认输入环、按 tick 重放、显示误差偏移 e 的平滑。
// 每次显示推进都从最新权威样本重放到最新已发送目标 tick（≤32 tick×2 半步），不保留原生状态。
class ClientMotionPredictor final {
public:
  ClientMotionPredictor();
  ~ClientMotionPredictor();
  ClientMotionPredictor(const ClientMotionPredictor &)=delete;
  ClientMotionPredictor &operator=(const ClientMotionPredictor &)=delete;
  // 换房、换连接、换输入代次时作废全部输入与平滑状态。
  void Reset() noexcept;
  // 记录一条已成功发送的输入意图；generation 变化清空环。
  void Input(std::uint64_t generation,std::uint64_t sequence,std::uint64_t target,
             std::int16_t move_x,std::int16_t move_y) noexcept;
  // 求本机身体的显示值。jump=true 表示本帧必须直接跳（不平滑），如身体换了地址或暂停恢复。
  // 返回 false 表示不预测（显示权威值），reason 给出静态原因名。
  bool Predict(const ClientMotionBody &body,const ClientMotionTerrain &terrain,
               const ClientMotionAuthority &authority,std::uint64_t now_us,bool jump,
               ClientMotionDisplay &out,const char *&reason) noexcept;
  // 诊断文本，前缀 stream.motion.predict_。
  std::string Fields() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace cs::game
