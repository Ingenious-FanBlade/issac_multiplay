#pragma once
#include "state/client_receive.h"

namespace cs::state {
// 完整候选的公共验证：逐实际节点检查引用用途、精确资源闭包与物化、跨流P0/P6拓扑。
// 此函数不授予未实现的原生owner资格；调用方还须执行具名域校验和安装覆盖检查。
StateReadiness ValidateClientStateTarget(const ClientStatePreparedTarget &,const ClientStateTables &);
// 计划构建已经持有工作区时，递归校验与闭包扫描共同使用该租约。
StateReadiness ValidateClientStateTarget(const ClientStatePreparedTarget &,const ClientStateTables &,const ClientStateWorkspace &);
struct ClientStateResourceNeed {ClientResourceSnapshotRef resources;ResourceKey key;};
// 已闭合候选的实际资源闭包，按依赖先于使用者排序；不加载资源，不等待无关历史目录。
ClientStateVector<ClientStateResourceNeed> ClientStateRequiredResources(const ClientStatePreparedTarget &,const ClientStateTables &);
ClientStateVector<ClientStateResourceNeed> ClientStateRequiredResources(const ClientStatePreparedTarget &,const ClientStateTables &,const ClientStateWorkspace &);
// 仅来自本地布局／采集合同的正式根允许集；没有把所有带字段的嵌套类型开放成网络根。
std::vector<ClientShadowLayoutBinding> ClientStateRootBindings();
} // namespace cs::state
