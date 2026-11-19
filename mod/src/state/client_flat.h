#pragma once
#include "state/client_tables.h"
#include "state/flat_capture.h"

namespace cs::state {
// 已解析槽保留完整目标身份，安装时按这份身份查活对象和资源，不能用新目录重解释旧编号。
struct ClientFlatLink {
  std::uint32_t offset=0;
  std::uint16_t field=kNullIndex,index=kNullIndex;
  PresentationMember member;
  ResourceKey resource;
  bool is_resource=false;
  bool operator==(const ClientFlatLink &) const = default;
};
// 子布局和附加段的位置只在通过校验的本地镜像中使用。
struct ClientFlatChild {
  std::uint32_t owner=0,main=0;
  std::uint16_t layout=0,element=0;
};
struct ClientFlatExtra {
  std::uint32_t owner=0;
  FlatTailSpan bytes;
};
struct ClientFlatImage {
  struct alignas(32) Block {std::byte bytes[32];};
  explicit ClientFlatImage(const std::shared_ptr<ClientStateMemory> &);
  ClientStateVector<Block> storage;
  ClientStateVector<FlatTailSpan> tails;
  ClientStateVector<ClientFlatLink> links;
  ClientStateVector<ClientFlatChild> children;
  ClientStateVector<ClientFlatExtra> extras;
  ClientStateVector<std::uint32_t> inactive;
  std::uint32_t size=0;
  std::span<const std::byte> Bytes() const noexcept {return std::as_bytes(std::span(storage)).first(size);}
  std::span<const std::byte> Tail(std::uint16_t index) const noexcept;
  const ClientFlatLink *Link(std::uint32_t offset) const noexcept;
  std::span<const std::byte> Extra(std::uint32_t offset) const noexcept;
  std::span<const ClientFlatChild> Children(std::uint32_t offset) const noexcept;
  bool Active(std::uint32_t offset) const noexcept;
};
using ClientFlatImageRef=std::shared_ptr<const ClientFlatImage>;

// 失败报告跟随目标的纯值状态。损坏根继续计数且仅接受全量记录重新建立基线。
struct ClientFlatFault {
  PresentationMember identity;
  std::uint16_t member=kNullIndex,layout=0,field=kNullIndex;
  std::uint32_t count=0;
  std::array<char,96> reason{};
};

ClientFlatImageRef DecodeClientFlat(const FlatLayout &,const StateRecordPayload &,const StateSectionView &,
    const ClientFlatImageRef &base,const ClientMemberSnapshotRef &,const ClientResourceSnapshotRef &,
    const std::shared_ptr<ClientStateMemory> &,std::uint32_t &remaining);
bool ClientFlatLinksCurrent(const ClientFlatImage &,const ClientMemberSnapshotRef &,const ClientResourceSnapshotRef &) noexcept;
void ClientFlatWriteGroups(const FlatLayout &,const ClientFlatImage &,const ClientFlatImage *,
    ClientStateVector<std::uint32_t> &groups,const std::shared_ptr<ClientStateMemory> &);
std::span<const std::byte> ClientFlatNamed(const FlatLayout &,const ClientFlatImage &,std::string_view) noexcept;
// Player 的映射在 World，所引用的 Sprite 向量在 Body；两边须来自同一个目录修订。
bool ClientFlatPlayerTopologyMatches(const ClientFlatImage &world,const ClientFlatImage &body) noexcept;
// 由同一镜像选择标量条件。资源条件在 FlatReader 中解释。
bool ClientFlatSelected(const FlatLayout &,const FlatPatch &,std::span<const std::byte> main) noexcept;
}
