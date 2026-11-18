#include "world/common/membership.h"
#include "binding/world_layout.h"
#include <cstring>
namespace cs::world {
void EnumerateSource(const NativeSource &reader,std::uint32_t source,RawSource &out,std::span<std::uint32_t> scratch,std::span<std::uint32_t> fixed,void *owner,MembershipSink sink,bool final_review_elsewhere) noexcept {
  const auto *spec = layout::FindSource(source);
  if (!spec) {
    out.status = static_cast<std::uint32_t>(Reason::NotReady);
    reader.Fail(Reason::NotReady);
    return;
  }
  if (spec->kind == layout::SourceKind::Reserved) {
    out.status = kSourceNotApplicable;
    return;
  }
  if (spec->kind == layout::SourceKind::Relation)
    return;
  const auto base =
      std::strcmp(spec->base, "game") == 0 ? reader.game : reader.room;
  const auto descriptor = std::uint64_t(base) + spec->offset;
  std::uint32_t data = 0, count = 0, bytes = 0;
  const auto fail = [&](Reason reason) noexcept {
    out.status = static_cast<std::uint32_t>(reason);
    reader.Fail(reason);
  };
  if (spec->kind == layout::SourceKind::EntityList ||
      spec->kind == layout::SourceKind::Roster) {
    bytes = spec->kind == layout::SourceKind::Roster ? 12u : 16u;
    if (!reader.Read(descriptor, out.descriptor.data(), bytes)) {
      fail(Reason::ReadFailed);
      return;
    }
    if (spec->kind == layout::SourceKind::Roster) {
      const auto begin = out.descriptor[0], end = out.descriptor[1],
                 capacity = out.descriptor[2];
      if (begin || end || capacity) {
        if (!begin || begin > end || end > capacity ||
            ((begin | end | capacity) & 3u)) {
          fail(Reason::ReadFailed);
          return;
        }
        data = begin;
        count = (end - begin) / 4;
      }
    } else {
      data = out.descriptor[1];
      count = out.descriptor[3];
      if (count > out.descriptor[2] || (count && !data) || (data & 3u)) {
        fail(Reason::ReadFailed);
        return;
      }
    }
  } else {
    if (descriptor > UINT32_MAX) {
      fail(Reason::ReadFailed);
      return;
    }
    data = static_cast<std::uint32_t>(descriptor);
    count = spec->count;
  }
  out.count = count;
  if (count > scratch.size() || (spec->kind==layout::SourceKind::FixedArray && count>fixed.size()) ||
      std::uint64_t(data) + std::uint64_t(count) * 4 > UINT32_MAX) {
    fail(Reason::Budget);
    return;
  }
  // 正式采集已安排段尾整表复核；两次各复制一次连续指针区。
  if(final_review_elsewhere&&count&&!reader.Read(data,scratch.data(),std::size_t(count)*4)){
    fail(Reason::ReadFailed);return;
  }
  for (std::uint32_t i = 0; i < count; ++i) {
    std::uint32_t address = final_review_elsewhere?scratch[i]:0;
    if (!final_review_elsewhere&&!reader.Value(std::uint64_t(data) + i * 4, address)) {
      fail(Reason::ReadFailed);
      return;
    }
    scratch[i] = address;
    if(spec->kind==layout::SourceKind::FixedArray) fixed[i]=address;
    if (!address) {
      if (spec->kind != layout::SourceKind::FixedArray)
        fail(Reason::NotReady);
      continue;
    }
    sink(owner,address,i);
  }
  if(final_review_elsewhere)return;
  for (std::uint32_t i = 0; i < count; ++i) {
    std::uint32_t value = 0;
    if (!reader.Value(std::uint64_t(data) + i * 4, value) ||
        value != scratch[i]) {
      fail(Reason::SourceChanged);
      return;
    }
  }
  if (bytes) {
    std::uint32_t after[4]{};
    if (!reader.Read(descriptor, after, bytes) ||
        std::memcmp(after, out.descriptor.data(), bytes))
      fail(Reason::SourceChanged);
  }
}
}
