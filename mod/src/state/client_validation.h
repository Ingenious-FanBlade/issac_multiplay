#pragma once
#include "state/client_receive.h"

namespace cs::state {
// 接收阶段与应用阶段共用同一套纯值引用和资源语义检查。
std::optional<ResourceKey> ValidateClientDeclaredNode(const BodyNodeView &,StateMemberDirectory,ResourceDirectoryView);
struct ClientStateResourceNeed {ClientResourceSnapshotRef resources;ResourceKey key;};
// 固定输入的引用类型、共享投影及资源闭包已经核定。物化资格以当前本机缓存另查。
struct ClientTargetFacts {
  ClientStateVector<ClientStateResourceNeed> resources;
  explicit ClientTargetFacts(const std::shared_ptr<ClientStateMemory> &memory):
    resources(ClientStateAllocator<ClientStateResourceNeed>(memory)) {}
};
struct ClientTargetCheckWork;
std::shared_ptr<const ClientTargetFacts> InspectClientStateTargetStep(const ClientStatePreparedTarget &,const ClientStateTables &,
    std::shared_ptr<ClientTargetCheckWork> &,std::chrono::steady_clock::time_point);
// 仅来自本地布局／采集合同的正式根允许集；没有把所有带字段的嵌套类型开放成网络根。
std::vector<ClientShadowLayoutBinding> ClientStateRootBindings();
} // namespace cs::state
