# Windows 消息与平台接口：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

PeekMessageW、TranslateMessage、DispatchMessageW是USER32导入槽，loop中记录消息与焦点/泵进度，不把消息次数或墙钟当Authority tick。菜单pump RVA0x65E660，历史Peek调用标记RVA0x65E68A、返回0x65E68C仅为所选静态指令位置，不是新的gameplay边界。

网络线程、文件线程与消息线程不得凭获得地址就读写游戏对象。平台和设备handle仅作当前进程临时关联。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| loop_runtime.cpp [Peek] | IAT_SLOT | 0x00718714 → 按导入名/运行时解析 | PeekMessage | 动态/未声明 / 4 | [loop_runtime.cpp:32](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [Translate] | IAT_SLOT | 0x00718650 → 按导入名/运行时解析 | TranslateMessage | 动态/未声明 / 4 | [loop_runtime.cpp:33](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [Dispatch] | IAT_SLOT | 0x007185B8 → 按导入名/运行时解析 | DispatchMessage | 动态/未声明 / 4 | [loop_runtime.cpp:34](../../cs-t0/native/src/loop_runtime.cpp) |
| offline_attestation_runtime.cpp [PeekMessage] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | PeekMessage 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [offline_attestation_runtime.cpp:422](../../cs-t0/native/src/offline_attestation_runtime.cpp) |

## 已知静态函数入口索引

目前没有额外的独立函数导出；已有虚表/安装点已在上文列明。未知入口不填猜测地址。

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。

## 源码原生定位常量补录

以下逐项保留当前源码的RVA/对象偏移命名和值，包括全局槽、vtable/COL、返回位置和只读诊断定位。常量不全是实际patch，旧命名不覆盖模块正文消歧；同一值在不同family中重复保留。`OBJECT_OFFSET`不加模块基址。

| 原始符号 | 类型 | 值 | 源码位置 |
| --- | --- | --- | --- |
| kPeekIatRva | RVA | `0x00718714` | [loop_runtime.cpp:32](../../cs-t0/native/src/loop_runtime.cpp) |
| kTranslateIatRva | RVA | `0x00718650` | [loop_runtime.cpp:33](../../cs-t0/native/src/loop_runtime.cpp) |
| kDispatchIatRva | RVA | `0x007185B8` | [loop_runtime.cpp:34](../../cs-t0/native/src/loop_runtime.cpp) |
| kGenericPumpRva | RVA | `0x0065E660` | [offline_attestation_runtime.cpp:28](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kSecurityCookieRva | RVA | `0x007F93B4` | [offline_attestation_runtime.cpp:29](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kPeekMessageIatRva | RVA | `0x00718714` | [offline_attestation_runtime.cpp:30](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kMenuPeekCallRva | RVA | `0x0065E68A` | [offline_attestation_runtime.cpp:31](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kMenuPeekReturnRva | RVA | `0x0065E68C` | [offline_attestation_runtime.cpp:32](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
