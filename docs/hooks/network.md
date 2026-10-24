# 传输与原版联机历史观察：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

本模块含原版/旧研究观察点与现行C/S通信来源，二者明确隔离。原版channel0、Isaac Sync channel0xFD、Online Manager、NetInputDevice和FrameStore均不供新状态同步复用。动态SteamNetworking006的slot0/1/2在steamclient.dll内解析，主模块中没有固定site RVA，当前外部ABI列UNKNOWN。

当前T09使用独立UDP relay和wire IPC；远端worker收发协议，不写游戏对象。Kind::StateHash是历史名称，实际载荷只有PartialValues/Gap（最多24值），不计算状态哈希，也不是baseline/delta。tx-03-07 P1/F0正式通过仅证明10,000tick ACK/Gap STATE及输入/恢复链，不证明Replica或双世界相等。

RVA0x5110AB→0x50D640是旧dispatch调用点；0x50B8E5→0x510980是旧ready观察。旧原版位置仅供查阅，不因文档收录而授权运行。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| boundary_runtime.cpp [7] | REL32_CALL | 0x005110AB → 0x0050D640 | 原版 online dispatch；仅历史研究，C/S不复用 | E8 90 C5 FF FF / 5 | [boundary_runtime.cpp:42](../../cs-t0/native/src/boundary_runtime.cpp) |
| runtime.cpp [Frame] | REL32_CALL | 0x0050B8E5 → 0x00510980 | 原版 ready/frame 观察 | E8 96 50 00 00 / 5 | [runtime.cpp:23](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [Dispatch] | REL32_CALL | 0x005110AB → 0x0050D640 | 原版 dispatch 观察 | E8 90 C5 FF FF / 5 | [runtime.cpp:24](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [SteamContext] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | SteamContext 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [runtime.cpp:649](../../cs-t0/native/src/runtime.cpp) |
| offline_attestation_runtime.cpp [SteamContext] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | SteamContext 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [offline_attestation_runtime.cpp:422](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| runtime.cpp [0] | DYNAMIC_VTABLE_SLOT | 动态/自有表 → 按导入名/运行时解析 | SteamNetworking006 slot 0: SendP2PPacket；外部模块地址/ABI动态UNKNOWN | 动态/未声明 / 动态 | [runtime.cpp:720](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [1] | DYNAMIC_VTABLE_SLOT | 动态/自有表 → 按导入名/运行时解析 | SteamNetworking006 slot 1: IsP2PPacketAvailable；外部模块地址/ABI动态UNKNOWN | 动态/未声明 / 动态 | [runtime.cpp:720](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [2] | DYNAMIC_VTABLE_SLOT | 动态/自有表 → 按导入名/运行时解析 | SteamNetworking006 slot 2: ReadP2PPacket；外部模块地址/ABI动态UNKNOWN | 动态/未声明 / 动态 | [runtime.cpp:720](../../cs-t0/native/src/runtime.cpp) |
| offline_attestation_runtime.cpp [0] | DYNAMIC_VTABLE_SLOT | 动态/自有表 → 按导入名/运行时解析 | SteamNetworking006 slot 0: SendP2PPacket；外部模块地址/ABI动态UNKNOWN | 动态/未声明 / 动态 | [offline_attestation_runtime.cpp:361](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| offline_attestation_runtime.cpp [1] | DYNAMIC_VTABLE_SLOT | 动态/自有表 → 按导入名/运行时解析 | SteamNetworking006 slot 1: IsP2PPacketAvailable；外部模块地址/ABI动态UNKNOWN | 动态/未声明 / 动态 | [offline_attestation_runtime.cpp:361](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| offline_attestation_runtime.cpp [2] | DYNAMIC_VTABLE_SLOT | 动态/自有表 → 按导入名/运行时解析 | SteamNetworking006 slot 2: ReadP2PPacket；外部模块地址/ABI动态UNKNOWN | 动态/未声明 / 动态 | [offline_attestation_runtime.cpp:361](../../cs-t0/native/src/offline_attestation_runtime.cpp) |

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x00283580 | 0x00683580 | sub_683580 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x004FE3A0 | 0x008FE3A0 | sub_8FE3A0 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x004FF600 | 0x008FF600 | sub_8FF600 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x004FFC70 | 0x008FFC70 | sub_8FFC70 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x005015B0 | 0x009015B0 | sub_9015B0 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x00501B70 | 0x00901B70 | sub_901B70 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x00502950 | 0x00902950 | sub_902950 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x0050B150 | 0x0090B150 | sub_90B150 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x0050D640 | 0x0090D640 | sub_90D640 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x0050D950 | 0x0090D950 | sub_90D950 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x00510980 | 0x00910980 | sub_910980 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x00516770 | 0x00916770 | sub_916770 | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0051ACC0 | 0x0091ACC0 | sub_91ACC0 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。

## 源码原生定位常量补录

以下逐项保留当前源码的RVA/对象偏移命名和值，包括全局槽、vtable/COL、返回位置和只读诊断定位。常量不全是实际patch，旧命名不覆盖模块正文消歧；同一值在不同family中重复保留。`OBJECT_OFFSET`不加模块基址。

| 原始符号 | 类型 | 值 | 源码位置 |
| --- | --- | --- | --- |
| kNetworkBackendSlotRva | RVA | `0x00879A58` | [boundary_runtime.cpp:21](../../cs-t0/native/src/boundary_runtime.cpp) |
| kSteamBackendVtableRva | RVA | `0x007A2440` | [boundary_runtime.cpp:22](../../cs-t0/native/src/boundary_runtime.cpp) |
| kSyncManagerOffset | OBJECT_OFFSET | `0x0004B3D8` | [boundary_runtime.cpp:24](../../cs-t0/native/src/boundary_runtime.cpp) |
| kOnlineFrameOffset | OBJECT_OFFSET | `0x0000000C` | [boundary_runtime.cpp:25](../../cs-t0/native/src/boundary_runtime.cpp) |
| kOnlineManagerSlotRva | RVA | `0x0087169C` | [coop_passive_runtime.cpp:35](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kNetworkBackendSlotRva | RVA | `0x00879A58` | [coop_passive_runtime.cpp:36](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kOnlineRosterOffset | OBJECT_OFFSET | `0x0004B3D8` | [coop_passive_runtime.cpp:41](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kOnlineFrameOffset | OBJECT_OFFSET | `0x0004B3E4` | [coop_passive_runtime.cpp:42](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kSteamBackendVtableRva | RVA | `0x007A2440` | [coop_passive_runtime.cpp:43](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kNetworkBackendSlotRva | RVA | `0x00879A58` | [offline_attestation_runtime.cpp:22](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kSteamContextDescriptorRva | RVA | `0x0085C51C` | [offline_attestation_runtime.cpp:23](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kSteamContextInitializerRva | RVA | `0x00676090` | [offline_attestation_runtime.cpp:24](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kSyncManagerOffset | OBJECT_OFFSET | `0x0004B3D8` | [offline_attestation_runtime.cpp:35](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kFrameCallRva | RVA | `0x0050B8E5` | [runtime.cpp:23](../../cs-t0/native/src/runtime.cpp) |
| kDispatchCallRva | RVA | `0x005110AB` | [runtime.cpp:24](../../cs-t0/native/src/runtime.cpp) |
| kNetworkBackendSlotRva | RVA | `0x00879A58` | [runtime.cpp:26](../../cs-t0/native/src/runtime.cpp) |
| kSteamBackendVtableRva | RVA | `0x007A2440` | [runtime.cpp:27](../../cs-t0/native/src/runtime.cpp) |
| kSteamContextDescriptorRva | RVA | `0x0085C51C` | [runtime.cpp:28](../../cs-t0/native/src/runtime.cpp) |
| kSteamContextInitializerRva | RVA | `0x00676090` | [runtime.cpp:29](../../cs-t0/native/src/runtime.cpp) |
| kSyncManagerOffset | OBJECT_OFFSET | `0x0004B3D8` | [runtime.cpp:39](../../cs-t0/native/src/runtime.cpp) |
| kOnlineFrameOffset | OBJECT_OFFSET | `0x0000000C` | [runtime.cpp:40](../../cs-t0/native/src/runtime.cpp) |
| kOnlineManagerSlotRva | RVA | `0x0087169C` | [coop_passive_main.cpp:170](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kNetworkBackendSlotRva | RVA | `0x00879A58` | [coop_passive_main.cpp:171](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kOnlineRosterOffset | OBJECT_OFFSET | `0x0004B3D8` | [coop_passive_main.cpp:176](../../cs-t0/native/tests/coop_passive_main.cpp) |
