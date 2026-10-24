# Lua VM 与回调：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

Lua IAT通过导入名解析，表中CallRva是调用指令而非IAT槽。lua_newstate/luaL_newstate/lua_close/lua_pcallk外部DLL目标地址没有稳定主模块RVA。lua_close为VM退休边界；脚本与游戏对象仅在已理解的主线程操作。

原生getter、ToXXX和注册closure可能创建userdata、表或引用；仅把它们作为布局/类型证据，不在只读采集时调用。未来Authority执行gameplay回调，Replica只执行明确获准的presentation逻辑；角色分离尚未实现，不能靠所有Lua callback一刀切跳过维持画面。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| loop_runtime.cpp [3] | REL32_CALL | 0x002FBBA5 → 0x004607A0 | 普通更新尾部 MC_POST_UPDATE | E8 F6 4B 16 00 / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [12] | REL32_CALL | 0x00559B82 → 0x00464EE0 | alternate transaction tail | E8 59 B3 F0 FF / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [18] | REL32_CALL | 0x002F580B → 0x00464EE0 | alternate completion 0 | E8 D0 F6 16 00 / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [19] | REL32_CALL | 0x002F61FF → 0x00464EE0 | alternate completion 1 | E8 DC EC 16 00 / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [20] | REL32_CALL | 0x002F72C9 → 0x00464EE0 | alternate completion 2 | E8 12 DC 16 00 / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [21] | REL32_CALL | 0x002F7721 → 0x00464EE0 | alternate completion 3 | E8 BA D7 16 00 / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [22] | REL32_CALL | 0x002F7A10 → 0x00464EE0 | alternate completion 4 | E8 CB D4 16 00 / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| boundary_runtime.cpp [4] | REL32_CALL | 0x002FBBA5 → 0x004607A0 | post-update callback | E8 F6 4B 16 00 / 5 | [boundary_runtime.cpp:42](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [6] | REL32_CALL | 0x002FC0A6 → 0x00463680 | post-render callback | E8 D5 75 16 00 / 5 | [boundary_runtime.cpp:42](../../cs-t0/native/src/boundary_runtime.cpp) |
| runtime.cpp [0] | REL32_CALL | 0x002F580B → 0x00464EE0 | PostGameStart caller 0 | E8 D0 F6 16 00 / 5 | [runtime.cpp:46](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [1] | REL32_CALL | 0x002F61FF → 0x00464EE0 | PostGameStart caller 1 | E8 DC EC 16 00 / 5 | [runtime.cpp:46](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [2] | REL32_CALL | 0x002F72C9 → 0x00464EE0 | PostGameStart caller 2 | E8 12 DC 16 00 / 5 | [runtime.cpp:46](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [3] | REL32_CALL | 0x002F7721 → 0x00464EE0 | PostGameStart caller 3 | E8 BA D7 16 00 / 5 | [runtime.cpp:46](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [4] | REL32_CALL | 0x002F7A10 → 0x00464EE0 | PostGameStart caller 4 | E8 CB D4 16 00 / 5 | [runtime.cpp:46](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [5] | REL32_CALL | 0x00559B82 → 0x00464EE0 | PostGameStart caller 5 | E8 59 B3 F0 FF / 5 | [runtime.cpp:46](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [LuaNewstate] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaNewstate 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [runtime.cpp:649](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [LuaNewstate caller] | IAT_CALL_DEPENDENCY | 0x0046057B → 按导入名/运行时解析 | LuaNewstate 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [runtime.cpp:30](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [LuaLNewstate] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaLNewstate 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [runtime.cpp:649](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [LuaLNewstate caller] | IAT_CALL_DEPENDENCY | 0x004605A3 → 按导入名/运行时解析 | LuaLNewstate 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [runtime.cpp:31](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [LuaClose] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaClose 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [runtime.cpp:649](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [LuaClose caller] | IAT_CALL_DEPENDENCY | 0x0046E5C0 → 按导入名/运行时解析 | LuaClose 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [runtime.cpp:32](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [LuaPcall] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaPcall 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [runtime.cpp:649](../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [LuaPcall caller] | IAT_CALL_DEPENDENCY | 0x004729CB → 按导入名/运行时解析 | LuaPcall 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [runtime.cpp:33](../../cs-t0/native/src/runtime.cpp) |
| boundary_runtime.cpp [LuaNewstate] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaNewstate 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [boundary_runtime.cpp:560](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [LuaNewstate caller] | IAT_CALL_DEPENDENCY | 0x0046057B → 按导入名/运行时解析 | LuaNewstate 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [boundary_runtime.cpp:33](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [LuaLNewstate] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaLNewstate 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [boundary_runtime.cpp:560](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [LuaLNewstate caller] | IAT_CALL_DEPENDENCY | 0x004605A3 → 按导入名/运行时解析 | LuaLNewstate 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [boundary_runtime.cpp:35](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [LuaClose] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaClose 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [boundary_runtime.cpp:560](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [LuaClose caller] | IAT_CALL_DEPENDENCY | 0x0046E5C0 → 按导入名/运行时解析 | LuaClose 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [boundary_runtime.cpp:31](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [LuaPcall] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaPcall 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [boundary_runtime.cpp:560](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [LuaPcall caller] | IAT_CALL_DEPENDENCY | 0x004729CB → 按导入名/运行时解析 | LuaPcall 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [boundary_runtime.cpp:37](../../cs-t0/native/src/boundary_runtime.cpp) |
| offline_attestation_runtime.cpp [LuaClose] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaClose 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [offline_attestation_runtime.cpp:422](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| offline_attestation_runtime.cpp [LuaClose caller] | IAT_CALL_DEPENDENCY | 0x0046E5C0 → 按导入名/运行时解析 | LuaClose 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [offline_attestation_runtime.cpp:25](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| offline_attestation_runtime.cpp [LuaPcall] | IAT_SYMBOL | 动态/自有表 → 按导入名/运行时解析 | LuaPcall 导入槽（按符号动态定位） | 动态/未声明 / 动态 | [offline_attestation_runtime.cpp:422](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| offline_attestation_runtime.cpp [LuaPcall caller] | IAT_CALL_DEPENDENCY | 0x004729CB → 按导入名/运行时解析 | LuaPcall 间接调用位置；不直接patch该6字节call | 动态/未声明 / 6 | [offline_attestation_runtime.cpp:26](../../cs-t0/native/src/offline_attestation_runtime.cpp) |

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x00017360 | 0x00417360 | Entity GetMaxHitPoints raw member getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00017370 | 0x00417370 | Entity GetHitPoints raw member getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00035D20 | 0x00435D20 | Entity SetHitPoints raw member setter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00035D40 | 0x00435D40 | Entity SetMaxHitPoints raw member setter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x0045BFD0 | 0x0085BFD0 | lua_pushinteger_ | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |
| 0x0045BFF0 | 0x0085BFF0 | lua_pushinteger__0 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。

## 源码原生定位常量补录

以下逐项保留当前源码的RVA/对象偏移命名和值，包括全局槽、vtable/COL、返回位置和只读诊断定位。常量不全是实际patch，旧命名不覆盖模块正文消歧；同一值在不同family中重复保留。`OBJECT_OFFSET`不加模块基址。

| 原始符号 | 类型 | 值 | 源码位置 |
| --- | --- | --- | --- |
| kLuaCloseCallRva | RVA | `0x0046E5C0` | [boundary_runtime.cpp:31](../../cs-t0/native/src/boundary_runtime.cpp) |
| kLuaCloseReturnRva | RVA | `0x0046E5C6` | [boundary_runtime.cpp:32](../../cs-t0/native/src/boundary_runtime.cpp) |
| kLuaNewstateCallRva | RVA | `0x0046057B` | [boundary_runtime.cpp:33](../../cs-t0/native/src/boundary_runtime.cpp) |
| kLuaNewstateReturnRva | RVA | `0x00460581` | [boundary_runtime.cpp:34](../../cs-t0/native/src/boundary_runtime.cpp) |
| kLuaLNewstateCallRva | RVA | `0x004605A3` | [boundary_runtime.cpp:35](../../cs-t0/native/src/boundary_runtime.cpp) |
| kLuaLNewstateReturnRva | RVA | `0x004605A9` | [boundary_runtime.cpp:36](../../cs-t0/native/src/boundary_runtime.cpp) |
| kLuaPcallCallRva | RVA | `0x004729CB` | [boundary_runtime.cpp:37](../../cs-t0/native/src/boundary_runtime.cpp) |
| kLuaPcallReturnRva | RVA | `0x004729D1` | [boundary_runtime.cpp:38](../../cs-t0/native/src/boundary_runtime.cpp) |
| kLuaCloseCallRva | RVA | `0x0046E5C0` | [offline_attestation_runtime.cpp:25](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kLuaPcallCallRva | RVA | `0x004729CB` | [offline_attestation_runtime.cpp:26](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kLuaPcallReturnRva | RVA | `0x004729D1` | [offline_attestation_runtime.cpp:27](../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kLuaNewstateCallRva | RVA | `0x0046057B` | [runtime.cpp:30](../../cs-t0/native/src/runtime.cpp) |
| kLuaLNewstateCallRva | RVA | `0x004605A3` | [runtime.cpp:31](../../cs-t0/native/src/runtime.cpp) |
| kLuaCloseCallRva | RVA | `0x0046E5C0` | [runtime.cpp:32](../../cs-t0/native/src/runtime.cpp) |
| kLuaPcallCallRva | RVA | `0x004729CB` | [runtime.cpp:33](../../cs-t0/native/src/runtime.cpp) |
| kLuaPcallReturnRva | RVA | `0x004729D1` | [runtime.cpp:34](../../cs-t0/native/src/runtime.cpp) |
