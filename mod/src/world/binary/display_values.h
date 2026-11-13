#pragma once
#include "state/capture_values.h"

namespace cs::world::binary {
// 只借用当前主线程自有值；children是同一node表的区间，不含Host原生关系指针。
struct DisplayFieldValue {
  std::uint16_t field=0;
  state::ScalarKind scalar=state::ScalarKind::None;
  std::uint32_t count=1,first_child=0;
  std::span<const std::byte> bytes;
  std::span<const state::CaptureLink> links;
};
// native只供已核ItemOverlay读取其原生Sprite；其余protocol节点用本方node地址作同步游标。
struct DisplayValueNode {
  std::uint16_t layout=0;
  std::uint32_t native=0;
  std::span<const DisplayFieldValue> fields;
};
}
