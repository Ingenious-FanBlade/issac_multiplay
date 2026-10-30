#include "world/compatibility/min_read_v1.h"
#include "binding/world_layout.h"
#include <cstring>
namespace cs::world::compatibility {
namespace {
bool Finite(const RawField &field) noexcept { std::uint32_t bits=0; std::memcpy(&bits,field.raw.data(),sizeof(bits)); return (bits&0x7f800000u)!=0x7f800000u; }
}
void CopyNativeFields(const NativeSource &source,RawObject &object,const Identity &identity,
                           bool ready) noexcept {
  const auto kind = static_cast<Class>(object.kind);
  for (const auto id : ProfileFieldsFor(kind)) {
    if (object.field_count == kFieldBudget) {
      source.Fail(Reason::Budget);
      break;
    }
    auto &field = object.fields[object.field_count++];
    field.Reset();
    const auto *spec = layout::FindField(id);
    const auto *declaration = ProfileField(id);
    if (!spec || !declaration) {
      source.Fail(Reason::InvalidRecord);
      continue;
    }
    field.id = id;
    field.width = spec->width;
    field.encoding = spec->encoding;
    field.status = FieldStatus::Value;
    // 派生字段不能凭来源存在取得资格。Grid 由独立初始化标志和槽关联判定。
    if (!ready || identity.Retired() ||
        (id >= 0x0101 && id < 0x0800 && identity.readiness < 2)) {
      field.status = FieldStatus::NotReady;
      source.Fail(Reason::NotReady);
      continue;
    }
    if (spec->status == layout::FieldStatus::InitUnknown) {
      field.status = FieldStatus::Unconfirmed;
      source.Fail(Reason::NotReady);
      continue;
    }
    // 用户于 2026-10-27 采纳：初始化写入与本代当前字节的读取资格分别表达。
    // FIRST_OBSERVED 本身无资格；最终来源/root 复核失败仍撤销整批完整性。
    if (id == 0x000a && (!identity.current_public_ready ||
                         identity.readiness < 2 || source.polluted)) {
      field.status = FieldStatus::NotReady;
      source.Fail(Reason::NotReady);
      continue;
    }
    bool ok = true;
    if (spec->base == layout::FieldBase::Derived) {
      if (id == 0x001e) {
        std::uint32_t stamp = 0;
        ok = source.Value(std::uint64_t(object.address) +
                       layout::LayoutConstants().entity_frame_stamp_offset,
                   stamp);
        const auto value = source.game_frame - stamp;
        if (ok)
          std::memcpy(field.raw.data(), &value, 4);
      } else if (id == 0x0810 || id == 0x0811) {
        if (!source.grid_width || object.grid_index == kNoGridIndex) {
          field.status = FieldStatus::NotReady;
          source.Fail(Reason::NotReady);
          continue;
        }
        const auto &c = layout::LayoutConstants();
        const float value =
            id == 0x0810 ? c.grid_position_step *
                                   float(object.grid_index % source.grid_width) +
                               c.grid_position_origin_x
                         : c.grid_position_step *
                                   float(object.grid_index / source.grid_width) +
                               c.grid_position_origin_y;
        std::memcpy(field.raw.data(), &value, 4);
      } else
        ok = false;
    } else {
      const auto base =
          spec->base == layout::FieldBase::Game ? source.game : object.address;
      auto address = std::uint64_t(base) + spec->offset;
      if (auto *f = source.faults; f && f->pending == 5) {
        // 只替换本次读取的源为自有不可读页，SEH 路径与正常 Read 完全相同。
        f->before.store(static_cast<std::uint32_t>(address));
        f->after.store(f->unreadable);
        address = f->unreadable;
        f->pending = 0;
        ok = source.Read(address, field.raw.data(), field.width);
        f->applied.store(!ok);
      } else
        ok = source.Read(address, field.raw.data(), field.width);
    }
    if (!ok) {
      field.raw = {};
      field.status = FieldStatus::ReadFailed;
      source.Fail(Reason::ReadFailed);
    } else if (field.encoding == Encoding::F32 && !Finite(field))
      source.Fail(Reason::Nonfinite);
  }
}
}
