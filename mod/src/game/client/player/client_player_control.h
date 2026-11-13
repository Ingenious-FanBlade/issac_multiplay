#pragma once
#include "replica/model.h"
#include "replica/subject_key.h"

namespace cs::game {
// 已提交A的控制组纯值视图；不带原生地址，身体shape和连接/绑定代次各自独立。
struct ClientPlayerControlView final {
  replica::SubjectKey participant{},group{};
  std::uint64_t revision=0,control_session=0,binding_revision=0;
  unsigned ordinal=0,bodies=0,active=0,consumers=0;
  bool present=false;
};
// 只比较真实控制身份；换装/shape/roster重排不制造一次新连接或重发旧边沿。
bool ClientPlayerControlSame(const ClientPlayerControlView &,const ClientPlayerControlView &) noexcept;
}
