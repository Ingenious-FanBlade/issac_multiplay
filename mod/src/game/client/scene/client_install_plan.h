#pragma once
#include "state/client_plan.h"
#include "game/client/grid/client_grid_description.h"
#include "game/client/display/client_display.h"

namespace cs::game {
// 事件与音频的关联键在后台解析，主线程只用它借当前批次的原生对象。
struct ClientSceneDisplayLinks final {
  std::uint32_t player=state::kClientPlanNone;
  state::ResourceKey resource{255,state::kNullIndex};
};
// 二进制安装的格子显示消费者只需原生成员身份、槽位和裁剪位。
struct ClientSceneGridDisplay final {
  state::PresentationMember member;
  std::uint32_t index=0,clip=0;
  std::uint32_t Index()const noexcept{return index;}
  std::uint32_t WaterClipFlags()const noexcept{return clip;}
};
// 第12步的最终纯值输入：全域数字计划加已核关系的Grid／Room／Level／显示描述。
// 不持有游戏对象；生成它不代表任何字段已原生安装或事件已播放。
class ClientSceneInstallPlan final:public state::ClientStateNativePlan {
public:
  explicit ClientSceneInstallPlan(std::shared_ptr<const state::ClientStateInstallPlan>);
  const state::ClientStateInstallPlan &State()const noexcept{return *state_;}
  std::span<const ClientSceneGridDisplay> Grids()const noexcept{return grids_;}
  const std::optional<ClientRoomLevelDescription> &Rooms()const noexcept{return rooms_;}
  const ClientDisplayWorldRef &Display()const noexcept{return display_;}
  const ClientDisplayMusicRequest &Music()const noexcept{return music_;}
  std::span<const ClientDisplaySound> Sounds()const noexcept{return sounds_;}
  std::span<const ClientSceneDisplayLinks> SoundLinks()const noexcept{return sound_links_;}
  std::span<const ClientDisplayEvent> Events()const noexcept{return events_;}
  std::span<const ClientSceneDisplayLinks> EventLinks()const noexcept{return event_links_;}
  const ClientDisplayBoss &Boss()const noexcept{return boss_;}
  // 后台只预测目录身份。预通知还须在主线程核对上次发布的真实实例。
  std::span<const state::StateMemberEntry> ModLeaving()const noexcept{return mod_leaving_;}
  bool Releasable()const noexcept override;
private:
  friend struct ClientSceneDescribeWork;
  std::shared_ptr<const state::ClientStateInstallPlan> state_;
  state::ClientStateVector<ClientSceneGridDisplay> grids_;
  std::optional<ClientRoomLevelDescription> rooms_;
  ClientDisplayWorldRef display_;
  ClientDisplayMusicRequest music_;
  state::ClientStateVector<ClientDisplaySound> sounds_;
  state::ClientStateVector<ClientSceneDisplayLinks> sound_links_,event_links_;
  state::ClientStateVector<ClientDisplayEvent> events_;
  state::ClientStateVector<state::StateMemberEntry> mod_leaving_;
  ClientDisplayBoss boss_;
};
// 后续真实安装owner提供只读资格快照；后台不解引用游戏对象，也不在查询中物化资源。
// 没有该owner或其Available=false时，仍不能声明完整二进制安装能力。
class ClientSceneInstallReadiness {
public:
  virtual ~ClientSceneInstallReadiness()=default;
  virtual bool Available()const noexcept=0;
  virtual state::StateReadiness Inspect(const ClientSceneInstallPlan &,const state::ClientStateTables &)const=0;
};
// 正式准备接口的纯值实现：先生成全部描述，再委托真实owner核覆盖、资源和生命周期。
class ClientSceneStatePreparation final:public state::ClientStatePreparation {
public:
  explicit ClientSceneStatePreparation(std::shared_ptr<const ClientSceneInstallReadiness>);
  bool Available()const noexcept override;
  state::StateReadiness Inspect(const state::ClientStatePreparedTarget &,const state::ClientStateTables &)const override;
  std::shared_ptr<const state::ClientStateNativePlan> PrepareStep(const state::ClientStatePreparedTarget &,
      std::shared_ptr<state::ClientStateNativePlanWork> &,std::chrono::steady_clock::time_point)const override;
private:
  std::shared_ptr<const ClientSceneInstallReadiness> readiness_;
  // 仅工作线程访问；弱引用不延后正常退休。复用资格由完整身份和当前块决定。
  mutable std::weak_ptr<const ClientSceneInstallPlan> last_world_;
};
} // namespace cs::game
