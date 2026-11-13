#pragma once
#include "state/body_codec.h"
#include "state/wire_scope.h"

namespace cs::state {
// 此上下文来自公共头／分段头；不在线上再复制一份，也不增加Presentation确认序列。
using PresentationScope=StateScope;
struct PresentationBatchContext {
  PresentationScope scope;
  std::uint16_t layout_version=0;
  BodyStream stream=BodyStream::World;
  std::uint32_t member_epoch=0, resource_epoch=0, base_version=0, target_version=0;
  std::uint64_t source_revision=0, presentation_batch=0;
  std::uint32_t game_frame=0;
  // 仅由外层实际共同目标提供；0表示没有配对，不能从来源修订推算Peer目标。
  std::array<std::uint32_t,2> paired_targets{};
  std::uint64_t installed_event_watermark=0, event_watermark=0;
};
struct PresentationAudioEvent {
  std::uint64_t sequence=0;
  std::uint32_t logical_frame=0;
  std::uint16_t owner=kNullIndex, purpose=382;
  std::uint64_t voice=0, generation=0;
  std::uint8_t clear_scope=0, flags=0;
  std::uint16_t layout=383;
  std::span<const std::byte> payload;
  std::uint32_t consumed=0, field_visits=0;
};
struct PresentationMember {
  std::uint32_t space=0;
  std::uint16_t layout=0;
  std::uint64_t id=0, use=0, shape=0;
  auto operator<=>(const PresentationMember &) const = default;
};
// 帧分段只携带有序事件；旧 group_count 槽仍按保留零值读写。
struct StateFrameEventView {
  std::span<const PresentationAudioEvent> events;
  std::uint32_t consumed=0;
  std::uint64_t first_sequence=0;
};
} // namespace cs::state
