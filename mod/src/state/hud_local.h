#pragma once
#include "state/presentation_envelope.h"
#include "core/replication_memory.h"
#include <array>
#include <optional>
#include <thread>

namespace cs::state {
enum class HudIconSource : std::uint8_t { Empty, Local, Host };
enum class HudIconKind : std::uint8_t { Active, Trinket };
enum class HudQueueKind : std::uint8_t { Material, Spell };
// 所有键均为自有纯值；HUD实例、绑定、身体形态和资源代次不能相互替代。
struct HudLocalIdentity {
  PresentationScope scope;
  PresentationMember body,group;
  std::uint64_t binding=0,instance=0,generation=0;
  std::uint32_t topology=0;
  auto operator<=>(const HudLocalIdentity &) const = default;
};
struct HudLocalClock {
  std::uint64_t binding=0,update=0,mantle_revision=0,drop_anchor=0;
  std::uint32_t game_frame=0;
  bool critical_health=false,drop_held=false;
  std::uint8_t drop_phase=0;
  auto operator<=>(const HudLocalClock &) const = default;
};
struct HudClockWireValue {std::uint16_t player=kNullIndex;HudLocalClock clock;};
// 正式正文仍走同源通用编解码器，不维护第二份字节偏移或在旧版本删字节。
std::span<const std::byte> EncodeHudLocalClockInto(const HudClockWireValue &,std::span<std::byte>,BodyCodecScratch &);
HudClockWireValue DecodeHudLocalClock(std::span<const std::byte>,BodyCodecScratch &);
struct HudEffectIdentity { std::uint32_t category=0,id=0; std::int32_t count=0; };
struct HudEffectsInput {
  PresentationMember body;
  std::uint64_t revision=0;
  bool complete=false,disabled=false;
  std::span<const HudEffectIdentity> entries;
};
// 只复现已核32位求和及90A490存在性查询；不调用Player/效果/伤害逻辑。
bool HudCriticalHealth(std::int32_t red,std::int32_t eternal,std::int32_t soul,std::int32_t bone) noexcept;
bool HudHolyMantle(const HudEffectsInput &);
std::int16_t HudLowHealthFlash(bool critical,bool mantle,std::uint32_t game_frame) noexcept;
float HudDropScale(const HudLocalClock &);

// 观察者必须在正常HUD更新尾部提供实际消费结果与准确的更新后相位。
// source_revision是已有封存修订；update只数本HUD的正常完成，不充当第三确认序列。
struct HudNormalUpdate {
  HudLocalIdentity identity;
  std::uint64_t source_revision=0,mantle_revision=0;
  std::uint32_t game_frame=0;
  bool critical_health=false,drop_held=false;
  std::uint8_t completed_drop_phase=0;
};
class HudClockCapture final {
public:
  explicit HudClockCapture(std::thread::id owner=std::this_thread::get_id());
  HudLocalClock Observe(const HudNormalUpdate &);
  void Retire() noexcept;
private:
  memory::Charge charge_;
  std::thread::id thread_=std::this_thread::get_id();
  HudLocalIdentity identity_{};
  HudLocalClock clock_{};
  std::uint64_t source_revision_=0;
  bool present_=false;
};

// 普通映射由唯一JSON源生成；还须同槽P3/P4、真实配置类别及图像内容精确一致。
bool HudOrdinaryIcon(HudIconKind,std::int32_t id,unsigned slot) noexcept;
HudIconSource SelectHudIconSource(HudIconKind,unsigned slot,std::int32_t state_id,
    std::int32_t visual_id,bool genuinely_empty,std::uint32_t configuration_category,bool exact_config_image) noexcept;
struct HudImageKey {
  std::uint32_t epoch=0;
  std::uint16_t index=kNullIndex;
  std::uint64_t configuration=0,content=0;
  auto operator<=>(const HudImageKey &) const = default;
};
struct HudPreparedImage { HudImageKey key;float center_x=0,center_y=0; };
struct HudQueueKey {
  std::uint32_t epoch=0;
  std::uint16_t file=kNullIndex,animation=kNullIndex;
  std::uint64_t configuration=0,content=0;
  auto operator<=>(const HudQueueKey &) const = default;
};
// 本地资源owner仅返回已物化的精确内容；接口不允许用旧身体缓存或默认图像兜底。
class HudLocalResources {
public:
  virtual ~HudLocalResources()=default;
  virtual std::optional<HudPreparedImage> Ordinary(HudIconKind,std::int32_t,unsigned slot) const=0;
  virtual std::optional<HudPreparedImage> Special(std::uint16_t image) const=0;
  virtual bool Ready(const HudImageKey &) const=0;
  virtual std::optional<HudQueueKey> Queue(HudQueueKind,std::uint8_t frame,bool small) const=0;
  virtual bool QueueReady(const HudQueueKey &) const=0;
};
// alpha来自本地实际图像，最多32x32；返回整数中点，与J273中心取整一致。
// 全透明图没有可证明的中心，显式失败，不沿用旧图中心。
std::array<float,2> HudOpaqueCenter(std::span<const std::uint8_t> alpha,
    unsigned width,unsigned height,unsigned stride);
struct HudActiveInput {
  std::int32_t item=0,charge=0,battery_charge=0,sub_charge=0,var_data=0;
  float partial_charge=0;
  HudIconSource source=HudIconSource::Empty;
  std::int32_t special_visual=0,charge_flash=0;
  std::uint16_t special_image=kNullIndex,book_image=kNullIndex;
};
struct HudTrinketInput {
  std::uint32_t item=0;
  HudIconSource source=HudIconSource::Empty;
  bool tint_override=false;
  std::int32_t special_visual=0;
  std::uint16_t special_image=kNullIndex;
  std::array<float,2> special_center{};
};
struct HudLocalInputs {
  HudLocalIdentity identity;
  std::uint32_t target=0;
  // 完整目标装配者在P3/P4与435主体、目标、来源及窄依赖全部匹配后提供。
  bool complete=false;
  HudLocalClock clock;
  HudEffectsInput effects;
  std::array<HudActiveInput,4> active{};
  std::array<HudTrinketInput,2> trinkets{};
  std::array<std::uint8_t,8> materials{};
  std::array<std::uint8_t,6> spells{};
  std::int32_t poop_mana=0;
  std::array<float,6> stat_values{};
  unsigned stat_precision=2;
};
struct HudLocalIcon {
  std::int32_t visual=0;
  bool golden=false,tint_override=false,has_image=false;
  HudPreparedImage image;
};
// x/y为本地HUD单位，实际视口/缩放由排版owner提供；draw_order保留正常消费者顺序。
struct HudQueueGlyph {
  std::uint8_t frame=0,slot=0,draw_order=0;
  bool small=false;
  float x=0,y=0,alpha=1;
  HudQueueKey resource;
};
// 输出只含显示值；心格/三色相位/Pocket/库存/History/概率仍由原结果owner承接。
struct HudLocalFrame {
  HudLocalIdentity identity;
  std::uint32_t target=0;
  HudLocalClock clock;
  bool mantle=false;
  std::int16_t low_health_flash=0;
  float drop_scale=0;
  std::array<HudLocalIcon,4> active{};
  std::array<std::optional<HudPreparedImage>,4> books{};
  std::array<std::int32_t,4> charge_flash{};
  std::array<HudLocalIcon,2> trinkets{};
  std::array<std::uint8_t,8> materials{};
  std::array<std::uint8_t,6> spells{};
  std::array<HudQueueGlyph,8> material_draw{};
  std::array<HudQueueGlyph,6> spell_draw{};
  std::array<std::array<char,64>,6> stat_text{};
};
HudLocalFrame PrepareHudLocal(const HudLocalInputs &,const HudLocalResources &);
bool HudLocalResourcesReady(const HudLocalFrame &,const HudLocalResources &) noexcept;

// 候选与已提交实例分开。Commit不会生成ACK；整批原生安装成功仍由接受状态机确认。
// 外层writer/draw须持有真实资源owner借用，本类的借用只保护自有纯值缓存。
class HudLocalOwner final {
public:
  HudLocalOwner();
  std::uint64_t Prepare(const HudLocalInputs &,const HudLocalResources &);
  bool Commit(std::uint64_t ticket,const HudLocalIdentity &,std::uint32_t target,const HudLocalResources &);
  void Cancel() noexcept;
  const HudLocalFrame *Borrow() noexcept;
  void Release() noexcept;
  void Retire() noexcept;
  bool Drained() const noexcept {return thread_==std::this_thread::get_id()&&borrows_==0;}
  static constexpr std::size_t RetainedBytes() noexcept {return sizeof(HudLocalOwner);}
private:
  memory::Charge charge_;
  std::thread::id thread_=std::this_thread::get_id();
  HudLocalFrame committed_{},prepared_{};
  std::uint64_t ticket_=0;
  std::uint32_t borrows_=0;
  bool pending_=false,present_=false,retiring_=false;
};

// 布局只消费已提交的必要显示选择；ordinal来自C/S组，display_order不是roster顺序。
struct HudLayoutBody {
  PresentationMember body,group;
  std::uint8_t ordinal=0,roles=0,display_order=0;
  std::uint32_t history_count=0;
};
struct HudViewport {float width=0,height=0,inset=12,preferred_scale=1;bool no_hud=false,world_visible=true;};
struct HudLocalPlacement {
  PresentationMember body;
  float x=0,y=0,scale=1,clip_x=0,clip_y=0,clip_width=0,clip_height=0;
  std::uint8_t roles=0;
  std::uint32_t history_prefix=0;
};
// 输出区由调用者预留，失败不提交部分排版；本端组才有详细Pocket/History/Stat。
std::span<const HudLocalPlacement> PlanHudLocalLayout(std::span<const HudLayoutBody>,
    const PresentationMember &local_group,const HudViewport &,std::span<HudLocalPlacement> output);
} // namespace cs::state
