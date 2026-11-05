#include "game/client_grids.h"
#include "game/client_objects.h"
#include "game/client_presentation.h"
#include "game/client_scene.h"
#include "game/client.h"
#include "replica/stream_e2e.h"
#include "game/context.h"
#include "replica/stream_capture.h"
#include "runtime.h"
#include <algorithm>
#include <cstring>
#include <optional>
#include <limits>
#include <windows.h>
#ifdef ISAACCS_E2E
#include <rapidjson/pointer.h>
#endif

namespace cs::game {
namespace {
namespace stream = replica::stream;
std::shared_ptr<stream::Message> StreamMessage(ClientStreamState &s,
                                               stream::Kind kind) {
  if (s.control >= UINT64_MAX - 1)
    throw replica::ContractError("client_stream_control_exhausted");
  auto m = std::make_shared<stream::Message>();
  m->memory = memory::Reserve(memory::Pool::Values, 8192);
  m->kind = kind;
  m->sequence = ++s.control;
  m->connection = s.scope.connection;
  m->route = s.route;
  m->body.SetObject();
  replica::Put(
      m->body, "world",
      stream::WorldValue(stream::World(s.scope), m->body.GetAllocator()),
      m->body.GetAllocator());
  return m;
}
void QueueStream(ClientStreamState &s,
                 std::shared_ptr<const stream::Message> m) {
  if (s.controls.size() >= 32)
    throw replica::ContractError("client_stream_control_capacity");
  stream::ValidateMessage(*m);
  s.controls.push_back(std::move(m));
}
void StreamReady(ClientStreamState &s, const stream::Range &r) {
  auto m = StreamMessage(s, stream::Kind::ContextReady);
  replica::Put(
      m->body, "versions",
      stream::VersionValues({{r->id, r->revision}}, m->body.GetAllocator()),
      m->body.GetAllocator());
  QueueStream(s, std::move(m));
}
replica::Json StreamSkeleton(const replica::Value &model) {
  auto out = replica::Copy(model);
  for (auto i = out["components"].MemberBegin();
       i != out["components"].MemberEnd(); ++i)
    if (std::strcmp(i->name.GetString(), "random"))
      i->value.RemoveMember("state");
  return out;
}
bool StructuralDifference(const ClientStreamState &s,
                          const stream::RangeData &root) {
  for (const auto &r : stream::Closure(s.store, root))
    if (r->id.kind == stream::RangeKind::Structure ||
        r->id.kind == stream::RangeKind::Resource) {
      auto old = s.store.Applied().find(r->id);
      if (old == s.store.Applied().end() ||
          old->second->revision != r->revision)
        return true;
    }
  return false;
}
} // namespace
bool ClientScene::BeginStream(const replica::Scope &scope, std::uint64_t route,
                              std::uint64_t manifest,
                              std::uint64_t generation, stream::RuntimeConfig config,std::uint64_t prepare_received_ms) noexcept {
  if (!IsMain() || TopOwner() != Owner::Manager ||
      state_ != ClientSceneState::Waiting || waiting_scope_ != scope ||
      !route || !manifest || !generation)
    return false;
  try {
    if(*config.Validate())return false;
    stream_ =
        std::make_unique<ClientStreamState>(scope, route, manifest, generation);
    stream_->config=config;network_stream_started_=true;stream_write_failure_=false;stream_metrics_.Bind(stream::World(scope),route);stream_metrics_.first_prepare_ms=prepare_received_ms?prepare_received_ms:GetTickCount64();stream_last_commit_=0;
    return true;
  } catch (...) {
    return false;
  }
}
bool ClientScene::AdmitStream(const stream::Message &m,
                              const char *&reason) noexcept {
  if (!IsMain() || TopOwner() != Owner::Manager || !stream_ ||
      state_ == ClientSceneState::Retiring ||
      state_ == ClientSceneState::Failed ||
      m.connection != stream_->scope.connection || m.route != stream_->route) {
    reason = "client_stream_route_or_owner";
    return false;
  }
  try {
    auto range = stream::ReadRange(m, stream::CaptureLayouts(),stream_->config.value_pool_bytes);
    stream_metrics_.Note(*range,stream::FlowStage::Receive,GetTickCount64());
    auto &s = *stream_;
    // 丢Applied ACK后的重复目标只补已存在的真实提交事实，不重入任何native步骤。
    if (const auto old = s.committed.find(range->id);
        old != s.committed.end() &&
        old->second.version.revision == range->revision) {
      s.acknowledgements.insert_or_assign(range->id, old->second);
      if (range->id.kind == stream::RangeKind::Manifest)
        s.ack_at = 0;
      reason = "client_stream_committed_repeat";
      return true;
    }
    const auto decision = s.store.Admit(range, stream::CaptureLayouts());
    reason = decision.reason;
    if (decision.status == stream::Admission::Stale){++stream_metrics_.stale_rejections;return true;}
    if (decision.status != stream::Admission::Complete &&
        decision.status != stream::Admission::WaitingDependencies &&
        decision.status != stream::Admission::Duplicate)
      return false;
    if (range->id.kind == stream::RangeKind::Manifest) {
      if (range->revision != s.manifest_revision) {
        reason = "client_stream_prepare_manifest";
        return false;
      }
      if (!s.baseline_committed)
        s.manifest = range;
    } else if (range->id.kind == stream::RangeKind::WorldDynamic ||
               range->id.kind == stream::RangeKind::PlayerFast) {
      if (!s.pending.contains(range->id) && s.pending.size() >= s.config.dynamic_candidates) {
        reason = "client_stream_dynamic_capacity";
        return false;
      }
      auto i = s.pending.find(range->id);
      if (i == s.pending.end() || i->second->revision < range->revision){if(i!=s.pending.end())++stream_metrics_.replacements;s.pending[range->id] = range;}
    }
    // ContextReady只确认纯值接纳；这里没有任何原生Applied行为。
    if (stream::Reliable(stream::MessageLane(m))) {
      if (!range->components && range->id.kind != stream::RangeKind::Manifest) {
        if (!s.available.contains(range->id) && s.available.size() >= 128) {
          reason = "client_stream_dependency_scope_capacity";
          return false;
        }
        s.available.insert_or_assign(range->id, range);
      }
      StreamReady(s, range);
    }
    return true;
  } catch (const std::exception &e) {
    try {
      stream_reason_ = e.what();
      reason = stream_reason_.c_str();
    } catch (...) {
      reason = "client_stream_admission_allocation";
    }
    return false;
  } catch (...) {
    reason = "client_stream_admission_exception";
    return false;
  }
}
std::shared_ptr<const stream::Message>
ClientScene::StreamControl() const noexcept {
  return IsMain() && stream_ && !stream_->controls.empty()
             ? stream_->controls.front()
             : nullptr;
}
void ClientScene::StreamControlSent() noexcept {
  if (IsMain() && stream_ && !stream_->controls.empty())
    stream_->controls.pop_front();
}
void ClientScene::CommitStream(const stream::Range &root, bool baseline) {
  auto &s = *stream_;
  for (const auto &range : stream::Closure(s.store, *root)) {
    auto old = s.store.Applied().find(range->id);
    if (old != s.store.Applied().end() &&
        old->second->revision >= range->revision)
      continue;
    const auto d = s.store.Commit({range->id, range->revision});
    if (!d && d.status != stream::Admission::Duplicate)
      throw replica::ContractError(d.reason);
  }
  ClientStreamCommit result;
  result.scope = s.scope;
  result.route = s.route;
  result.version = {root->id, root->revision};
  result.attempt = result_.attempt;
  result.instance = generations_.instance;
  result.resource = generations_.resource;
  result.native_domains = root->id.kind == stream::RangeKind::PlayerFast
                              ? (1ull << 6)
                              : std::uint64_t(result_.committed_domains >> 1);
  result.retained_domains = baseline ? s.manifest->retained_domains : 0;
  result.event_watermark = baseline ? s.manifest->event_watermark : 0;
  if (baseline && result.native_domains != s.manifest->native_domains)
    throw replica::ContractError("client_stream_precise_native_domains");
  s.committed.insert_or_assign(root->id, result);
  s.acknowledgements.insert_or_assign(root->id, std::move(result));
  if (root->id.kind == stream::RangeKind::PlayerFast)
    s.bodies.insert_or_assign(root->id.body, root);
  const auto now=GetTickCount64();
  stream_metrics_.Note(*root,stream::FlowStage::Commit,now);
  if(baseline||root->id.kind==stream::RangeKind::WorldDynamic)stream_last_commit_=now;
  stream_metrics_.display_ready_ms=now;
#ifdef ISAACCS_E2E
  ClientStreamAgeWitness(root->sample.tick);
  // 精确提交当刻回读；诊断失败不得改变提交或制造ACK，缺测由E2E单独判定。
  if(stream::E2EWants("native",*root)){
    try{std::vector<stream::Version> versions;for(const auto &[id,commit]:s.committed)versions.push_back(commit.version);stream::E2ESnapshot("native",root,stream::Closure(s.store,*root),s.scope.connection,s.route,TestInspect(),versions);}catch(...){}
  }
#endif
  if (baseline) {
    entity_events_.BaselineEvents(s.manifest->event_watermark);
    s.baseline_committed = true;
    s.ack_at = 0;
    s.store.ReleaseCold();
    s.manifest.reset();
  } else if (s.structure_rebuild)
    s.ack_at = 0;
  s.structure_rebuild = false;
}
void ClientScene::PumpStream() noexcept {
  if (!IsMain() || TopOwner() != Owner::Manager || !stream_ || pumping_ ||
      state_ == ClientSceneState::Failed ||
      state_ == ClientSceneState::Retiring)
    return;
  auto &s = *stream_;
  // 视觉中性段只暂留native提交；接纳、确认及Collect仍推进，不能积累整个高频历史。
  const auto advance_native=[]() noexcept {
#ifdef ISAACCS_E2E
    return !stream::E2EVisualHeld();
#else
    return true;
#endif
  };
  try {
    if (s.inflight) {
      if (result_.native_committed && DisplayReady() &&
          result_.revision == s.inflight->revision) {
        CommitStream(s.inflight,
                     s.inflight->id.kind == stream::RangeKind::Manifest);
        s.inflight.reset();
        s.model_memory.reset();
      } else if (result_.executed_prefix > s.progress_prefix) {
        s.progress_prefix = result_.executed_prefix;
        auto m = StreamMessage(s, stream::Kind::OperationAck);
        m->body.RemoveMember("world");
        auto &a = m->body.GetAllocator();
        replica::Put(m->body, "operation",
                     replica::Decimal(unsigned(stream::Kind::Range), a), a);
        replica::Put(m->body, "operation_sequence",
                     replica::Decimal(s.manifest_revision, a), a);
        replica::Put(m->body, "stage", replica::Decimal(2, a), a);
        replica::Put(m->body, "progress_sequence",
                     replica::Decimal(++s.progress_sequence, a), a);
        replica::Put(m->body, "reason", "native_complete_step", a);
        QueueStream(s, std::move(m));
      }
    }
    if (advance_native() && !s.cold_started && s.manifest &&
        stream::DependenciesReady(s.store, *s.manifest)) {
      s.model_memory = memory::Reserve(memory::Pool::Values, 65536);
      stream::Measure measure(stream_metrics_,stream::CpuStage::NativePrepare);
      auto model = stream::ColdModel(s.store, *s.manifest, s.scope.connection);
      s.model_memory->Resize(replica::OwnedValueUpperBound(model));
      auto skeleton = std::make_unique<replica::Json>(StreamSkeleton(model));
      if (!AdmitTarget(s.scope, s.manifest_revision, model))
        throw replica::ContractError(result_.reason.data());
      s.skeleton = std::move(skeleton);
      s.cold_started = true;
      s.inflight = s.manifest;
      s.progress_prefix = 0;
      return;
    }
    // 冷/结构提交立即先交付不可变确认，不能被连续的新结构反复抢先。
    if (advance_native() && s.baseline_committed && !s.inflight && DisplayReady() &&
        !s.pending.empty() && !(s.ack_at == 0 && !s.acknowledgements.empty())) {
      const auto rebuild = [&](const stream::Range &root) {
        s.bodies.clear();
        s.body_memory.clear();
        for (auto i = s.committed.begin(); i != s.committed.end();)
          if (i->first.kind == stream::RangeKind::PlayerFast)
            i = s.committed.erase(i);
          else
            ++i;
        s.store.RevokeBodies();
        for (auto p = s.pending.begin(); p != s.pending.end();)
          if (p->first.kind == stream::RangeKind::PlayerFast)
            p = s.pending.erase(p);
          else
            ++p;
        s.model_memory = memory::Reserve(memory::Pool::Values, 65536);
        auto model = stream::StructuralModel(s.store, *root, *s.skeleton,
                                             s.scope.connection);
        s.model_memory->Resize(replica::OwnedValueUpperBound(model));
        s.structure_rebuild = true;
        if (!AdmitTarget(s.scope, root->revision, model)){s.structure_rebuild=false;throw replica::ContractError(result_.reason.data());}
        s.inflight = root;
        s.pending.erase(root->id);
        s.progress_prefix = 0;
      };
      for (auto it = s.pending.begin(); it != s.pending.end();) {
        auto root = it->second;
        const auto applied = s.store.Applied().find(root->id);
        if (applied != s.store.Applied().end() &&
            applied->second->revision >= root->revision) {
          it = s.pending.erase(it);
          continue;
        }
        if (!stream::DependenciesReady(s.store, *root)) {
          ++it;
          continue;
        }
        if (root->id.kind == stream::RangeKind::PlayerFast) {
          bool compatible = true;
          for (const auto &d : root->dependencies) {
            auto current = s.store.Applied().find(d.id);
            if (current == s.store.Applied().end() ||
                (d.id.kind == stream::RangeKind::WorldDynamic
                     ? current->second->revision < d.revision
                     : current->second->revision != d.revision))
              compatible = false;
          }
          if (!compatible) {
            ++it;
            continue;
          }
          auto world = s.store.Applied().find(
              {stream::RangeKind::WorldDynamic, "ordinary-room", {}});
          if (world != s.store.Applied().end() &&
              world->second->sample.tick >= root->sample.tick) {
            it = s.pending.erase(it);
            continue;
          }
        }
        if (root->id.kind == stream::RangeKind::WorldDynamic &&
            StructuralDifference(s, *root)) {
          rebuild(root);
          break;
        }
        if (!GraphicsWindowValid() || !DestructiveReady() ||
            !ClientNativeCpuQueuesEmpty())
          break;
        std::uint64_t dto_bound = 65536;
        for (const auto &r : stream::Closure(s.store, *root))
          dto_bound += 8 * stream::Footprint(*r);
        auto dto_memory = memory::Reserve(memory::Pool::Values, dto_bound);
        std::vector<ClientObjectDescription> entities;
        std::vector<ClientPlayerDescription> players;
        std::vector<ClientGridDescription> grid_values;
        std::optional<ClientPlayerFastValues> body;
        std::shared_ptr<const replica::RoomGridTarget> room;
        ClientNativeSceneDescription description;
        std::optional<replica::Json> partial;
        if (root->id.kind == stream::RangeKind::PlayerFast)
          body = objects_->CompileFast(s.store, *root);
        else {
          partial = stream::DomainModel(s.store, *root, *s.skeleton,
                                        {"room", "grid", "level", "run"});
          room = replica::ParseRoomGridTarget(
              *partial, {}, replica::RoomGridModelMode::StreamValues);
          const char *why = "stream_room_values";
          if (!ClientDescribeSceneValues(*partial, *room, description, why))
            throw replica::ContractError(why);
          if (!ClientNativeWorldCanReuse(native_world_, description) ||
              !grids_->StreamCompatible(*room)) {
            rebuild(root);
            break;
          }
          grid_values = grids_->CompileStream(*room);
          objects_->CompileStream(s.store, *root, *room, s.bodies, entities,
                                  players);
          if (!presentation_->PrepareTarget(
                  *partial, why, description.room_state, description.map))
            throw replica::ContractError(why);
          presentation_->SetStreamPlayers();
          std::vector<std::uint64_t> facts;
          for (const auto &d : description.descriptors) {
            facts.push_back(d.key_id);
            facts.push_back(unsigned(d.display_flags));
            facts.push_back(unsigned(d.flags));
            facts.push_back(unsigned(d.visited));
            facts.push_back(unsigned(d.clear_count));
          }
          // 原图标/config的完整值进入事实目录；不用哈希或本地最新缓存判等。
          for (const auto &d : description.map.descriptors) {
            facts.push_back(std::uint64_t(d.storage));
            facts.push_back(std::uint64_t(d.source_slot));
            for (const auto &icon : d.icons) {
              facts.push_back(unsigned(icon.type));
              facts.push_back(unsigned(icon.variant));
              facts.push_back(unsigned(icon.subtype));
              facts.push_back(unsigned(icon.storage3));
              std::uint32_t x = 0, y = 0;
              std::memcpy(&x, &icon.position.x, 4);
              std::memcpy(&y, &icon.position.y, 4);
              facts.push_back(x);
              facts.push_back(y);
            }
          }
          facts.push_back(description.map.state.game_flags[0]);
          facts.push_back(description.map.state.game_flags[1]);
          for (const auto &c : description.map.state.configs) {
            for (float value : {c.anchor.x, c.anchor.y, c.padding.x,
                                c.padding.y, c.size1.x, c.size1.y, c.size2.x,
                                c.size2.y, c.offset, c.border_scale, c.alpha}) {
              std::uint32_t bits = 0;
              std::memcpy(&bits, &value, 4);
              facts.push_back(bits);
            }
            facts.push_back(unsigned(c.width));
            facts.push_back(unsigned(c.height));
            facts.push_back(unsigned(c.icons));
          }
          presentation_->StreamMapFacts(std::move(facts));
          if (!presentation_->StreamCompatible()) {
            rebuild(root);
            break;
          }
        }
        stream::Measure apply_measure(stream_metrics_,stream::CpuStage::NativeApply);
        stream_write_failure_=false;
        const auto previous_result = result_;
        if (next_attempt_ == UINT64_MAX)
          throw replica::ContractError("client_stream_attempt_exhausted");
        result_.attempt = ++next_attempt_;
        result_.revision = root->revision;
        result_.scope = s.scope;
        result_.native_committed = false;
        state_ = ClientSceneState::Applying;
        applied_valid_ = false;
        ClientBorrowToken native{}, graphics{};
        if (!Borrow(ClientPurpose::NativeApply, native) ||
            !Borrow(ClientPurpose::Graphics, graphics)) {
          Release(native);
          throw replica::ContractError("client_stream_borrow");
        }
        bool ok = true, written = false, waiting = false;
        const char *why = "client_stream_native_checked";
        if (body) {
          auto step = objects_->InstallFast(*body);
          NoteObjectStep(step, "player_fast");
          written = step.written;stream_write_failure_=written;
          ok = step.done && !step.failed;
          waiting = !step.done && !step.failed && !step.written;
          why = step.reason;
          if (ok) {
            auto sorted =
                presentation_->FastSort(ClientNativeWorldRoots(native_world_));
            ok = sorted.done && !sorted.failed;
            why = sorted.reason;
          }
        } else {
          written = true;stream_write_failure_=true;
          auto refreshed = ClientNativeWorldRefresh(native_world_, description);
          ok = refreshed.state == ClientNativeStepState::Ready;
          why = refreshed.reason;
          if (ok) {
            auto step = grids_->InstallStream(
                ClientNativeWorldRoots(native_world_), grid_values);
            ok = step.done && !step.failed;
            why = step.reason;
            result_.grid_prefix = step.prefix;
          }
          if (ok) {
            auto step = objects_->InstallStream(
                ClientNativeWorldRoots(native_world_), entities, players);
            NoteObjectStep(step, "stream");
            ok = step.done && !step.failed;
            why = step.reason;
          }
          if (ok) {
            auto step = ClientNativeWorldFinalize(native_world_);
            ok = step.state == ClientNativeStepState::Ready;
            why = step.reason;
          }
          if (ok) {
            auto step =
                presentation_->Step(ClientNativeWorldRoots(native_world_));
            ok = step.done && !step.failed;
            why = step.reason;
          }
          if (ok) {
            auto step = ClientNativeWorldCheckCommit(native_world_);
            ok = step.state == ClientNativeStepState::Ready;
            why = step.reason;
          }
        }
        result_.native_written |= written;
        result_.old_display_valid = false;
        Release(graphics);
        Release(native);
        if (waiting) {
          result_ = previous_result;
          state_ = ClientSceneState::Displaying;
          applied_valid_ = true;
          stream_reason_ = why;
          break;
        }
        if (!ok) {
          FailureResult(written ? ClientResultKind::PartialNativeFailure
                                : ClientResultKind::UnsupportedRequirement,
                        "stream", why);
          state_ = ClientSceneState::Failed;
          return;
        }
        applied_valid_ = true;
        state_ = ClientSceneState::Displaying;
        FailureResult(ClientResultKind::NativeCommitted, "stream_committed",
                      "native_range_committed");
        result_.native_committed = true;
        result_.old_display_valid = true;
        SetCommittedRange();
        if (body)
          result_.committed_domains =
              1u << unsigned(world::observation::DomainId::Player);
        CommitStream(root, false);stream_write_failure_=false;
        s.pending.erase(root->id);
        if (room) {
          s.installed_memory = std::move(dto_memory);
          s.body_memory.clear();
          for (auto p = s.available.begin(); p != s.available.end();)
            if (p->second->sample.tick <= root->sample.tick &&
                std::none_of(root->dependencies.begin(),
                             root->dependencies.end(),
                             [&](const auto &d) { return d.id == p->first; }))
              p = s.available.erase(p);
            else
              ++p;
        } else
          s.body_memory.insert_or_assign(root->id.body, std::move(dto_memory));
        break;
      }
    }
    const auto now = GetTickCount64();
    if (!s.acknowledgements.empty() && now >= s.ack_at) {
      while (!s.acknowledgements.empty() && s.controls.size() < 31) {
        auto m = StreamMessage(s, stream::Kind::AppliedAck);
        auto &a = m->body.GetAllocator();
        std::vector<stream::Version> versions;
        replica::Value results(rapidjson::kArrayType);
        std::uint64_t native = 0, retained = 0, watermark = 0;
        for (unsigned n = 0; n < 2 && !s.acknowledgements.empty(); ++n) {
          auto it = s.acknowledgements.begin();
          const auto &r = it->second;
          versions.push_back(r.version);
          replica::Value v(rapidjson::kObjectType);
          auto vs = stream::VersionValues({r.version}, a);
          replica::Value version;
          version.CopyFrom(vs[0], a);
          replica::Put(v, "version", std::move(version), a);
          for (const auto &[name, value] :
               std::initializer_list<std::pair<const char *, std::uint64_t>>{
                   {"attempt", r.attempt},
                   {"instance", r.instance},
                   {"resource", r.resource},
                   {"native_domains", r.native_domains},
                   {"retained_domains", r.retained_domains},
                   {"event_watermark", r.event_watermark}})
            replica::Put(v, name, replica::Decimal(value, a), a);
          native |= r.native_domains;
          retained |= r.retained_domains;
          watermark = (std::max)(watermark, r.event_watermark);
          results.PushBack(v, a);
          s.acknowledgements.erase(it);
        }
        replica::Put(m->body, "versions", stream::VersionValues(versions, a),
                     a);
        replica::Put(m->body, "results", std::move(results), a);
        replica::Put(m->body, "native_domains", replica::Decimal(native, a), a);
        replica::Put(m->body, "retained_domains", replica::Decimal(retained, a),
                     a);
        replica::Put(m->body, "event_watermark", replica::Decimal(watermark, a),
                     a);
        QueueStream(s, std::move(m));
      }
      if (s.acknowledgements.empty())
        s.ack_at = now + s.config.applied_ack_ms;
    }
    std::vector<stream::Version> roots;
    if (s.manifest && !s.baseline_committed)
      roots.push_back({s.manifest->id, s.manifest->revision});
    if (s.inflight)
      roots.push_back({s.inflight->id, s.inflight->revision});
    for (const auto &[id, r] : s.pending)
      roots.push_back({id, r->revision});
    for (const auto &[id, r] : s.available)
      roots.push_back({id, r->revision});
    for (const auto &[key, r] : s.bodies)
      roots.push_back({r->id, r->revision});
    s.store.Collect(roots);
    stream_metrics_.Queue(s.controls.size());stream_metrics_.candidates=s.pending.size()+(s.inflight?1:0);stream_metrics_.retained=s.available.size()+s.bodies.size();
  } catch (const std::exception &e) {
    const auto policy=stream::ClassifyFailure(e.what(),stream_write_failure_);
    if(DisplayReady()&&!stream_write_failure_&&policy.waitable){
      // 写前依赖/预算暂缺时A仍有效；保留首因，超过状态年龄由连接级恢复收口。
      if(!result_.failure_reason[0]){std::snprintf(result_.failure_reason.data(),result_.failure_reason.size(),"%s",e.what());std::snprintf(result_.failure_phase.data(),result_.failure_phase.size(),"stream_prewrite");}
      return;
    }
    Fail(e.what());
  } catch (...) {
    Fail("client_stream_exception");
  }
}
#ifdef ISAACCS_E2E
// 原18—20写前失败在真实NetworkStream的同一Scene重放；全部输入来自已收到的不可变闭包。
// 不改变wire/网络身份、不伪造有效世界；编码/路由错误仍由真实报文代理另外覆盖。
std::string ClientScene::TestStreamReject(unsigned tx,const std::string &name) noexcept {
 if(!IsMain()||TopOwner()!=Owner::Manager||!stream_||!stream_->skeleton||!DisplayReady()||tx<18||tx>20)return "ok=0\nerror=network_negative_requires_committed_A\n";
 try {
  auto &s=*stream_;const auto world=s.store.Applied().find({stream::RangeKind::WorldDynamic,"ordinary-room",{}});
  if(world==s.store.Applied().end())throw replica::ContractError("negative_actual_world_missing");
  auto model=stream::StructuralModel(s.store,*world->second,*s.skeleton,s.scope.connection);auto &a=model.GetAllocator();
  const auto before=TestInspect();const auto old=AppliedRevision(),attempt=result_.attempt;
  if(before.rfind("ok=1\n",0)!=0)throw replica::ContractError("negative_actual_native_inspect_unavailable");
  if(old>=UINT64_MAX-2)throw replica::ContractError("negative_revision_exhausted");
  model["revision"]=replica::Decimal(old+1,a);
  const auto at=[&](const char *path)->replica::Value &{auto *v=rapidjson::Pointer(path).Get(model);if(!v)throw replica::ContractError(std::string("negative_actual_prerequisite:")+path);return *v;};
  const auto entity=[&](bool player)->replica::Value &{auto &rows=at("/components/entity/state/objects");for(auto &row:rows.GetArray())if((std::strcmp(row["class"].GetString(),"EntityPlayer")==0)==player)return row;throw replica::ContractError("negative_actual_entity_class_missing");};
  const auto unknown=[&](replica::Value &v){if(!v.IsObject()||!v.HasMember("target"))throw replica::ContractError("negative_actual_reference_missing");v["target"]["id"]=replica::Decimal(999999999,a);};
  bool compatibility=false,rejected=false;std::string reason;
  if(name.rfind("caps-",0)==0){
   auto caps=replica::Capabilities();auto &ca=caps.GetAllocator();const auto suffix=name.substr(5);
   if(suffix=="model")caps["model"].SetUint(999);
   else if(suffix=="catalog")caps["model_categories"][tx==20?"player_target":"entity_target"]["catalog"].SetUint(0);
   else if(suffix=="id"||suffix=="source")replica::Put(caps["model_categories"]["player_target"],suffix.c_str(),"e2e-incompatible",ca);
   else throw replica::ContractError("negative_capability_name");
   try{replica::Negotiate(replica::Capabilities(),caps);}catch(const std::exception &e){rejected=true;reason=e.what();}compatibility=true;
  }else if(name=="retired-scope")at("/scope/scene")=replica::Decimal(0,a);
  else if(name=="old"||name=="old-revision")model["revision"]=replica::Decimal(old?old-1:0,a);
  else if(name=="conflict"||name=="same-revision-conflict"||name=="same-version-conflict"){model["revision"]=replica::Decimal(old,a);replica::Put(at("/components/entity/state"),"e2e_conflict",replica::Value(true),a);}
  else if(name=="partial")at(tx==20?"/components/player/coverage":"/components/entity/coverage").SetString("Partial",a);
  else if(name=="source-version")at(tx==20?"/components/player/state/application_source/version":"/components/entity/state/application_source/version").SetUint(999);
  else if(name=="bad-route")at("/components/room/state/room_index").SetInt(-100);
  else if(name=="bad-budget"){replica::Value rows(rapidjson::kArrayType);for(unsigned i=0;i<32769;++i)rows.PushBack(replica::Value(rapidjson::kObjectType),a);replica::Value budget(rapidjson::kObjectType);replica::Put(budget,"embers",std::move(rows),a);replica::Put(at("/components/grid/state"),"application_runtime_budget",std::move(budget),a);}
  else if(name=="bad-pose"||name=="bad-resource")entity(false)["common"]["fields"]["Entity.GetSprite"]["fields"]["Sprite.GetFilename"].SetString("../invalid.anm2",a);
  else if(name=="bad-class")entity(false)["class"].SetString("UnsupportedEntity",a);
  else if(name=="bad-number")entity(false)["common"]["fields"]["Entity.Position"].SetDouble(std::numeric_limits<double>::infinity());
  else if(name=="field-read-failed"||name=="field-truncated"||name=="shader-read-failed"){auto &fields=entity(false)["common"]["fields"];replica::Value v(rapidjson::kObjectType);replica::Put(v,"status",name=="field-truncated"?"Truncated":"ReadFailed",a);replica::Put(v,"reason","E2E real-network derived negative",a);if(name=="shader-read-failed")replica::Put(fields["Entity.GetSprite"]["fields"],"Sprite.HasCustomShader",std::move(v),a);else replica::Put(fields,"Entity.Position",std::move(v),a);}
  else if(name=="bad-reference"||name=="missing-owner"){auto &row=entity(false);replica::Value reference;reference.CopyFrom(row["subject"],a);unknown(reference);if(name=="bad-reference")reference["target"]["space"].SetUint(2);replica::Put(row["common"]["fields"],"Entity.Parent",std::move(reference),a);}
  else if(name=="held-owner-missing"||name=="held-owner-wrong-class"){auto &row=entity(true);replica::Value reference;reference.CopyFrom(row["subject"],a);if(name=="held-owner-missing")unknown(reference);replica::Put(row["body"]["fields"],"EntityPlayer.GetHeldEntity",std::move(reference),a);}
  else if(name=="dangling-graph"){replica::Value v(rapidjson::kObjectType);replica::Put(v,"type","graph_ref",a);replica::Put(v,"node",replica::Value(999999999u),a);replica::Put(at("/components/entity/state"),"negative_graph",std::move(v),a);}
  else if(name=="fx-blend-range"||name=="fx-blend-missing"){auto &layers=at("/components/room/state/presentation_runtime/room_fx/layers");bool found=false;for(auto &row:layers.GetArray())if(row.HasMember("blend_mode")){if(name=="fx-blend-range")row["blend_mode"].SetInt(3);else row.RemoveMember("blend_mode");found=true;break;}if(!found)throw replica::ContractError("negative_actual_generic_fx_missing");}
  else if(name=="bad-bitset")replica::Put(entity(false)["common"]["fields"],"Entity.GetEntityFlags",replica::Value("invalid-bits",a),a);
  else if(name=="player-scope")entity(true)["body"]["fields"]["EntityPlayer.GetPlayerType"].SetInt(20);
  else if(name=="cross-scope")at("/components/player/state/bodies/0/subject/target/run").SetString("999999999",a);
  else if(name=="mixed-window")at("/components/player/capture/owner").SetString("999999999",a);
  else if(name=="missing-body")at("/components/player/state/bodies").PopBack();
  else if(name=="missing-twin"){
   auto &ps=at("/components/player/state");replica::Value key;bool found=false;
   for(auto &body:ps["bodies"].GetArray())if(body["relations"]["in_public_roster"].IsBool()&&body["relations"]["in_public_roster"].GetBool()&&replica::Unsigned(body["relations"]["player_type"])==40){key.CopyFrom(body["subject"]["target"],a);found=true;break;}
   if(!found)throw replica::ContractError("negative_actual_tainted_soul_twin_missing");
   for(auto i=ps["bodies"].Begin();i!=ps["bodies"].End();)if((*i)["subject"]["target"]==key)i=ps["bodies"].Erase(i);else ++i;
   ps["membership"]["count"].SetUint(ps["bodies"].Size());
   auto &roster=ps["roster"];for(auto i=roster.Begin();i!=roster.End();)if((*i)["body"]["target"]==key)i=roster.Erase(i);else ++i;for(unsigned i=0;i<roster.Size();++i)roster[i]["index"].SetUint(i);
   for(auto &group:ps["control_groups"].GetArray()){
    auto &members=group["memberships"];for(auto i=members.Begin();i!=members.End();)if((*i)["subject"]["target"]==key)i=members.Erase(i);else ++i;
    auto &bodies=group["bodies"];for(auto i=bodies.Begin();i!=bodies.End();)if((*i)["target"]==key)i=bodies.Erase(i);else ++i;
   }
   auto &es=at("/components/entity/state");auto &objects=es["objects"];for(auto i=objects.Begin();i!=objects.End();)if((*i)["subject"]["target"]==key)i=objects.Erase(i);else ++i;es["membership"]["count"].SetUint(objects.Size());
  }
  else if(name=="duplicate-body"||name=="budget"||name=="pool-budget"){auto &rows=at(tx==20?"/components/player/state/bodies":"/components/entity/state/objects");if(rows.Empty())throw replica::ContractError("negative_actual_members_missing");auto v=replica::Copy(tx==20?rows[0]:entity(false));for(unsigned i=0;i<(name=="duplicate-body"?1u:name=="pool-budget"?513u:258u);++i){replica::Value copy;copy.CopyFrom(v,a);if(name=="pool-budget")copy["subject"]["target"]["id"]=replica::Decimal(700000+i,a);rows.PushBack(std::move(copy),a);}at(tx==20?"/components/player/state/membership/count":"/components/entity/state/membership/count").SetUint(rows.Size());if(name=="pool-budget")at("/components/entity/state/membership/native_count").SetUint(rows.Size());}
  else if(name=="unknown-role")at("/components/player/state/bodies/0/relations/player_type").SetUint(999);
  else if(name=="width")at("/components/player/state/bodies/0/relations/membership/ordinal").SetUint64(4294967296ull);
  else if(name=="unresolved"){auto &v=at("/components/player/state/bodies/0/relations/other_twin");v.SetObject();replica::Put(v,"status","Unresolved",a);replica::Put(v,"reason","E2E actual dependency rejected",a);}
  else if(name=="backup-slot")at("/components/player/state/backup_slots/0/index").SetUint(4);
  else if(name=="wrong-shape"){auto &v=at("/components/player/state/bodies/0/subject/shape_revision");v=replica::Decimal(replica::Unsigned(v)+1,a);}
  else if(name=="wrong-group"){auto &groups=at("/components/player/state/control_groups");if(groups.Size()<2)throw replica::ContractError("negative_actual_second_group_missing");replica::Value other;other.CopyFrom(groups[1]["subject"]["target"],a);at("/components/player/state/bodies/0/relations/membership/control_group_local_id")=std::move(other);}
  else if(name=="missing-final-state"||name=="missing-resource"){auto &body=entity(true)["body"];if(!body.RemoveMember(name=="missing-final-state"?"application_player_state":"application_player_resources"))throw replica::ContractError("negative_actual_player_application_missing");}
  else if(name=="history-gap"){auto &v=at("/components/player/state/bodies/0/history");v.SetObject();replica::Put(v,"status","ReadFailed",a);replica::Put(v,"reason","E2E",a);}
  else if(name=="self-held"){auto &row=entity(true);replica::Value ref;ref.CopyFrom(row["subject"],a);replica::Put(row["body"]["fields"],"EntityPlayer.GetHeldEntity",std::move(ref),a);}
  else if(name=="native-claim")replica::Put(model,"native_applied",replica::Value(true),a);
  else return "ok=0\nerror=network_negative_requires_named_stage_owner\n";
  if(!compatibility){try{replica::ValidateModel(model);rejected=!AdmitTarget(s.scope,replica::Unsigned(model["revision"]),model);reason=result_.reason.data();}catch(const std::exception &e){rejected=true;reason=e.what();}}
  const bool preserved=DisplayReady()&&AppliedRevision()==old&&result_.attempt==attempt&&TestInspect()==before;
  return std::string("ok=1\nrejected=")+(rejected?"1":"0")+"\nA_preserved="+(preserved?"1":"0")+"\norigin=real_network_applied_closure_no_file_import\nlayer="+(compatibility?"capability_contract":"native_model_prewrite")+"\nreason="+reason+"\n";
 }catch(const std::exception &e){return "ok=0\nerror="+std::string(e.what())+"\n";}catch(...){return "ok=0\nerror=network_negative_diagnostic_failure\n";}
}
#endif
} // namespace cs::game
