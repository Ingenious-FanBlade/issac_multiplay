#pragma once
#include "game/client_native_world.h"
#include "replica/stream_codec.h"
#include "replica/stream_materialize.h"
#include <deque>
#include "replica/stream_config.h"

namespace cs::game {
bool ClientDescribeSceneValues(const replica::Value &,
                               const replica::RoomGridTarget &,
                               ClientNativeSceneDescription &, const char *&);
// 主线程不可变提交事实；范围、attempt、实例/资源代次与世界提交分别保留。
struct ClientStreamCommit final {
  replica::Scope scope{};
  std::uint64_t route = 0, attempt = 0, instance = 0, resource = 0,
                native_domains = 0, retained_domains = 0, event_watermark = 0;
  replica::stream::Version version;
};
// 同Scene的纯值接收端；只接纳一个正式route，未执行动态最多每范围一个候选。
struct ClientStreamState final {
  explicit ClientStreamState(replica::Scope s, std::uint64_t r, std::uint64_t m,
                             std::uint64_t g)
      : scope(s), route(r), manifest_revision(m), generation(g),
        store(replica::stream::World(s)) {}
  replica::Scope scope;
  replica::stream::RuntimeConfig config;
  std::uint64_t route = 0, manifest_revision = 0, generation = 0, control = 0,
                ack_at = 0, progress_prefix = 0, progress_sequence = 0;
  replica::stream::Store store;
  replica::stream::Range manifest, inflight;
  std::map<replica::stream::RangeId, replica::stream::Range> pending;
  std::map<replica::stream::RangeId, replica::stream::Range> available;
  std::map<replica::SubjectKey, replica::stream::Range> bodies;
  std::map<replica::SubjectKey, memory::Lease> body_memory;
  std::map<replica::stream::RangeId, ClientStreamCommit> acknowledgements;
  std::map<replica::stream::RangeId, ClientStreamCommit> committed;
  std::deque<std::shared_ptr<const replica::stream::Message>> controls;
  std::unique_ptr<replica::Json> skeleton;
  memory::Lease metadata = memory::Reserve(memory::Pool::Values, 65536),
                model_memory, installed_memory;
  bool cold_started = false, baseline_committed = false,
       structure_rebuild = false;
};
} // namespace cs::game
