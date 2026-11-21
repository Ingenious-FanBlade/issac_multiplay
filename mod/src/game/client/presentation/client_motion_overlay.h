#pragma once
// 预测插值阶段 1：客机显示覆盖层与远端插值／外推。
// 设计见 docs/archive/design/binary-state/prediction-interpolation.md 第 4、6、7 节。
// 本头文件同时被 C++17 的 SDK 单元和 C++20 的逻辑单元包含，只用 C++17 可用的类型。
#include "game/client/presentation/client_motion_predict.h"
#include <cstdint>
#include <memory>
#include <string>

namespace cs::state {struct ClientStateBlockTable;}
namespace cs::game {
// 影子表成员身份（同 state::PresentationMember 的纯值拷贝，C++17 单元不能包含该头）。
struct ClientMotionMember final {
  std::uint32_t space=0;
  std::uint16_t layout=0;
  std::uint64_t id=0,use=0,shape=0;
};
// 已发布 World 中一个玩家身体的原生地址、控制组序号与影子表身份；只在主线程、本次发布有效。
// 阶段 2 用 member 在已发布 Body 表中找该身体的对账字段（设计 5.7）。
struct ClientMotionPlayer final {
  void *address=nullptr;
  unsigned ordinal=0;
  ClientMotionMember member{};
};
// 原生实体身份：地址复用或原地换形态时据此判定“出生”，旧历史作废。
// 不含 Entity+0x20：客机上它是由 Host index 换算的绘制排序值，可随安装变化。
struct ClientMotionIdentity final {
  std::uint32_t type=0,variant=0,subtype=0,init_seed=0;
  bool operator==(const ClientMotionIdentity &o) const noexcept {
    return type==o.type&&variant==o.variant&&subtype==o.subtype&&init_seed==o.init_seed;
  }
  bool operator!=(const ClientMotionIdentity &o) const noexcept {return !(*this==o);}
};
// 渲染列表中一个实体的纯值快照（一次遍历内有效）。
// 排除标志由原生遍历给出：本机身体、附着在本机身体上的对象、激光（点表未核实，阶段 1 不覆盖）。
struct ClientMotionEntity final {
  void *address=nullptr;
  ClientMotionIdentity identity{};
  unsigned slot=0;            // 在 renderEL 中的下标，仅作定位提示
  float x=0,y=0,vx=0,vy=0;    // 当前原生 pos／velocity（覆盖层外读取，即权威值）
  bool local=false;           // 本机身体
  bool attached=false;        // Parent／跟班所属／举起物指向本机身体
  bool laser=false;           // 激光：pos 之外还有点表，阶段 1 一律不覆盖
  // 阶段 2（设计 5.6、RE-P5）：attached 对象所附着的本机身体，以及是否随该身体平移 Δ。
  // follow 只给环绕跟班（OrbitLayer≥0）、举起物与 Parent 指向本机身体的非激光对象；跟随队列跟班不平移。
  void *owner=nullptr;
  bool follow=false;
  // 阶段 3（设计 5.6）：SpawnerEntity（Entity+0x3C8）引用的对象地址，只用于与本机身体地址比较，不解引用。
  // 本机泪弹出生融合据此识别“本机身体发出的泪弹”。
  void *spawner=nullptr;
};
// 覆盖层的一次写入：进入作用域时写显示值并暂存原值，退出时写回原值。
struct ClientMotionWrite final {
  void *address=nullptr;
  ClientMotionIdentity identity{};
  unsigned slot=0;
  float display_x=0,display_y=0;  // 要显示的位置
  float saved_x=0,saved_y=0;      // 进入时暂存的原生值（权威值）
  bool applied=false;
  // 阶段 2：本机身体同时写预测速度（RE-P4：单人相机 UpdateUltrasmooth 读 velocity 做前视）。
  bool velocity=false;
  float display_vx=0,display_vy=0,saved_vx=0,saved_vy=0;
};
// 原生侧（C++17 SDK 单元）：遍历当前房间 renderEL。room 必须是 g_Game 当前房间。
// locals 是本机身体地址（只与列表成员比较，不单独解引用）。返回写入 out 的个数；超过 capacity 时截断并置 overflow。
unsigned ClientMotionCollect(void *room,void *const *locals,unsigned local_count,
                             ClientMotionEntity *out,unsigned capacity,bool &overflow) noexcept;
// 原生侧：apply=true 时写显示值并暂存原值；apply=false 时把暂存值写回。
// 每个地址先在 renderEL 中定位（先看 slot 提示，再线性查找）并核对身份，找不到或身份不符时不触碰该地址。
// missing 计找不到／身份不符的个数；changed 计写回时发现 pos 已不是本层写入值的个数（仍写回权威值）。
void ClientMotionSwap(void *room,ClientMotionWrite *writes,unsigned count,bool apply,
                      unsigned &missing,unsigned &changed) noexcept;

// 主线程唯一的覆盖层 owner，由 ClientScene 持有。
// 生命周期：Installed（每次 World／Body 发布后采样权威值）→ Prepare（每个显示推进一次，算显示时间与显示值）
// → Enter／Exit（相机阶段与 Game::Render 各一对，写入显示值后写回权威值）。
class ClientMotionOverlay final {
public:
  ClientMotionOverlay();
  ~ClientMotionOverlay();
  ClientMotionOverlay(const ClientMotionOverlay &)=delete;
  ClientMotionOverlay &operator=(const ClientMotionOverlay &)=delete;
  // 新连接、换路由、结束二进制场景时作废全部历史与时间基准。
  void Reset() noexcept;
  // World 发布后登记玩家身体与控制组序号，供 Prepare 识别本机身体。
  void SetPlayers(const ClientMotionPlayer *players,unsigned count) noexcept;
  // 一次原生发布之后采样。streams 位 0=World、位 1=Body；scene 变化即换房，全部作废。
  // table 为刚发布的两流影子表（可空），阶段 2 从中读 Body 的半帧相位。
  void Installed(void *room,std::uint8_t streams,std::uint64_t scene,
                 std::uint32_t world_frame,std::uint32_t body_frame,
                 const state::ClientStateBlockTable *table=nullptr) noexcept;
  // 阶段 2：客机成功发送一条输入意图后登记，供本机身体预测重放（设计 5.4）。
  void RecordInput(std::uint64_t generation,std::uint64_t sequence,std::uint64_t target,
                   std::int16_t move_x,std::int16_t move_y) noexcept;
  // 每个显示推进调用一次：推进世界显示时间 T_w，求各对象显示值，生成写入表。
  // table 为当前已发布的两流影子表（可空）：本机身体的对账字段与 Room 寻路阻挡类别从这里读取。
  void Prepare(void *room,unsigned local_ordinal,bool paused,const state::ClientStateBlockTable *table) noexcept;
  // 进入／退出覆盖作用域；Enter 返回 true 时必须配对 Exit。
  bool Enter(void *room) noexcept;
  void Exit(void *room) noexcept;
  // 诊断文本，前缀 stream.motion.。
  std::string Fields() const;
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace cs::game
