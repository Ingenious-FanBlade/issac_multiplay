#include "game/bridge.h"
#include "game/context.h"
#include "game/topology.h"
#include "game/tick.h"
#include "runtime.h"
#include <cstring>
#include <exception>

namespace cs::game {
namespace {
thread_local const char *operation = "none";
// 仅读取 ST8 与官方结构均已确认的字段；无效下标不解引用。
bool Value(const char *entry, void *data, int index, int &value) noexcept {
  if (std::strcmp(entry, "pgd_unlock") == 0) {
    unsigned char unlocked = 0;
    if (index < 0 || index >= 641 || !ReadAt(data, 0x38 + index, unlocked)) return false;
    value = unlocked;
    return true;
  }
  if (std::strcmp(entry, "pgd_counter") == 0)
    return index >= 0 && index < 523 && ReadAt(data, 0x2BC + index * 4, value);
  value = 0;
  return true;
}
}
PersistenceOperation::PersistenceOperation(const char *name) noexcept : previous_(operation) { operation = name; }
PersistenceOperation::~PersistenceOperation() noexcept { operation = previous_; }
PersistenceScope::PersistenceScope(const char *entry, void *data, int index, int amount) noexcept
    : entry_(entry), operation_(operation), data_(data), index_(index), amount_(amount),
      exceptions_(std::uncaught_exceptions()) {
  if (!IsMain()) return;
  // Join 原生 Init/官方回调发生在 Join owner 内，也覆盖嵌套的 PGD 调用。
  if (std::strcmp(operation_, "none") == 0 && HostBusy() && HasOwner(Owner::Join)) operation_ = "join";
  if (std::strcmp(entry_, "button_maps") == 0) {
    known_ = true;
    Trace("persistent", "phase=enter entry=%s operation=%s known=1 loop=%llu", entry_, operation_, ManagerLoop());
    return;
  }
  known_ = ReadAt(data_, 0, dirty_) && ReadAt(data_, 1, readonly_) &&
           ReadAt(data_, 0xF8C, loaded_) && Value(entry_, data_, index_, before_);
  Trace("persistent", "phase=enter entry=%s operation=%s known=%u dirty=%u readonly=%u loaded=%u index=%d amount=%d value=%d loop=%llu",
        entry_, operation_, known_, dirty_, readonly_, loaded_, index_, amount_, before_, ManagerLoop());
}
PersistenceScope::~PersistenceScope() noexcept {
  if (!IsMain()) return;
  unsigned char dirty = 0;
  int after = 0;
  const bool maps = std::strcmp(entry_, "button_maps") == 0;
  const bool known = known_ && (maps || (ReadAt(data_, 0, dirty) && Value(entry_, data_, index_, after)));
  Trace("persistent", "phase=return entry=%s operation=%s known=%u normal=%u dirty_before=%u dirty_after=%u readonly=%u loaded=%u index=%d amount=%d before=%d after=%d loop=%llu",
        entry_, operation_, known, exceptions_ == std::uncaught_exceptions(), dirty_, dirty,
        readonly_, loaded_, index_, amount_, before_, after, ManagerLoop());
}
} // namespace cs::game
