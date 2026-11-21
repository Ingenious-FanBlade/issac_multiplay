# 游戏循环、模拟阶段与生命周期：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

Authority tick、普通完整世界采样、render frame与墙钟分开。普通模拟和奇数帧Player-only helper是不同路径，后者确实执行玩家模拟但不形成完整世界样本。旧30Hz样本值不能写成跨版本协议常量。

完整采样要求同一主线程/outer token的普通owner、Room和标准尾部均正常完成，顶层original返回后、coop TopExit之前、render之前复制。pause/loading/teardown、重入、错误线程和已知mutation使该批作废；不能以最近tick给跨期样本贴标签。

LoopClockHook __m128d 返回保持XMM0，调用者只消费低64位。27个rel32＋2个poll槽＋3个Windows IAT构成基础32槽；当前coop后缀19槽为51，可选world再加19为70。不同family组合不把所有点一次安装。

tx-03-10补充了[Replica更新与展示分派研究](../research/tx-03-10-replica-update-presentation-boundary.md)。普通owner之外还有driver的staged位置恢复、odd helper的特殊FSM与普通Player分支，以及额外位置写入；render child也调用Room和Lua回调。已知render owner不证明其纯展示，当前仍没有可直接启用的完整Replica安全边界。

tx-03-11[进一步分离研究](../research/tx-03-11-logic-presentation-separation.md)区分了 Entity 两类绘制 pass、Sprite 目标帧与自主推进；动画事件位可被原生攻击消费者读取并走到 Game::Spawn。render 保留的条件 Lua、动态资源目标及额外对象向量仍须单独解释，不能仅停止输入或只拦普通 owner 后继续全部原生展示逻辑。

tx-03-12[派生绘制补充](../research/tx-03-12-derived-render-sprite-lifecycle.md)已确认 NPC／Tear／Projectile 各有独立于外层 post-render 的条件 Lua 执行；只处理全局分派仍有遗漏。Sprite 资源与共享绘制缓冲的存在也不赋予网络线程访问对象或并发绘制的资格。

tx-03-14[renderer／context 研究](../research/tx-03-14-renderer-context-lifecycle.md)连接 `_main → 图形初始化 → 正常 render`，current 窗口由 TLS 记录；WGL 解绑／删除在正常图形缓存清理之后。另有 `0x8FC32C`（RVA `0x4FC32C`）加载等待循环直接调 render owner，不经过当前仅替换 `0x9312F1`（RVA `0x5312F1`）的 RenderHook；不能据 render 次数建立 Authority tick，亦未新增资源处理或 Hook。

tx-03-19[独立 Sprite 实际绘制窗口](../research/tx-03-19-sprite-render-entry-window.md)定位 `0x9560D0`（RVA `0x5560D0`，`E8 AB 30 0C 00`）最终 A19180 调用之前的受限屏幕叠加候选。现有 RenderHook 的退出已在最终批次提交／可能的 swap 之后；phase/depth 不等于 armed／主线程／不重入。未来 guard 需健康状态、outer token、generation/loading、自有 depth、TLS window 与 OS current HDC/HGLRC；未新增 Hook 或改变槽数。

tx-03-20[采集与连续更新合同](../research/tx-03-20-sprite-resource-pose-closure.md)将 Sprite 读取限定在完整 Authority Capture 的同一 token／batch 内、最终稳定性复核和 FinishBatch 之前。普通 `_main` 的 update 后紧接 render，render 后补读不能冒充同 tick。候选资源服务在自有主线程/context／已空队列窗口，最终 present 前只 draw；这些扩展尚未实现。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| loop_runtime.cpp [0] | REL32_CALL | 0x00531231 → 0x006266A0 | 主循环时钟：XMM0 返回，低64位供调用者使用 | E8 6A 54 0F 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [1] | REL32_CALL | 0x005312EC → 0x005AB6D0 | 顶层更新 owner | E8 DF A3 07 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [2] | REL32_CALL | 0x00555231 → 0x002FADC0 | 普通 gameplay 分支 owner | E8 8A 5B DA FF / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [4] | REL32_CALL | 0x00559696 → 0x002FA0C0 | run teardown caller 1 | E8 25 0A DA FF / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [5] | REL32_CALL | 0x005AB84A → 0x002FA0C0 | run teardown caller 2 | E8 71 E8 D4 FF / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [7] | REL32_CALL | 0x005312F1 → 0x005555C0 | 顶层 render owner；不能据此认定全部 presentation 无副作用 | E8 CA 42 02 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [10] | REL32_CALL | 0x00554D5E → 0x00559720 | alternate/loading owner | E8 BD 49 00 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [13] | REL32_CALL | 0x002FADFA → 0x0028BBA0 | 普通分支 pause/FSM predicate | E8 A1 0D F9 FF / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [14] | REL32_CALL | 0x002FAE09 → 0x00308630 | standard body 内部 anchor | E8 22 D8 00 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [17] | REL32_CALL | 0x00559B21 → 0x002F8140 | alternate 原生状态应用；不是 C/S snapshot apply | E8 1A E6 D9 FF / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| boundary_runtime.cpp [1] | REL32_CALL | 0x00555231 → 0x002FADC0 | gameplay owner | E8 8A 5B DA FF / 5 | [boundary_runtime.cpp:42](../../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [5] | REL32_CALL | 0x005559C1 → 0x002FBC10 | render 内部 owner | E8 4A 62 DA FF / 5 | [boundary_runtime.cpp:42](../../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [9] | REL32_CALL | 0x00559696 → 0x002FA0C0 | teardown caller 1 | E8 25 0A DA FF / 5 | [boundary_runtime.cpp:42](../../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [10] | REL32_CALL | 0x005AB84A → 0x002FA0C0 | teardown caller 2 | E8 71 E8 D4 FF / 5 | [boundary_runtime.cpp:42](../../../cs-t0/native/src/boundary_runtime.cpp) |
| runtime.cpp [PreGameExit] | CALL_DEPENDENCY | 0x002FA0E2 → 0x004650A0 | 退出分派调用依赖（校验，不在10个relative patch中） | E8 B9 AF 16 00 / 5 | [runtime.cpp:37](../../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [0] | REL32_CALL | 0x00559696 → 0x002FA0C0 | RunTeardown caller 0 | E8 25 0A DA FF / 5 | [runtime.cpp:58](../../../cs-t0/native/src/runtime.cpp) |
| runtime.cpp [1] | REL32_CALL | 0x005AB84A → 0x002FA0C0 | RunTeardown caller 1 | E8 71 E8 D4 FF / 5 | [runtime.cpp:58](../../../cs-t0/native/src/runtime.cpp) |

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x0028BBA0 | 0x0068BBA0 | sub_68BBA0 | [root-ida-abi.json](../../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x002FA0C0 | 0x006FA0C0 | sub_6FA0C0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x002FADC0 | 0x006FADC0 | sub_6FADC0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00308630 | 0x00708630 | sub_708630 | [root-ida-abi.json](../../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x00531050 | 0x00931050 | _main | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00554CD0 | 0x00954CD0 | sub_954CD0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x005555C0 | 0x009555C0 | sub_9555C0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00559720 | 0x00959720 | sub_959720 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x005AB6D0 | 0x009AB6D0 | Outer update wrapper: exactly one direct 954CD0 call | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x006266A0 | 0x00A266A0 | sub_A266A0 | [root-ida-abi.json](../../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |

## 证据与限制

[最近固定点位与ABI复核](../refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../refactor/project-conventions.md)。全量历史研究保留于 `docs/archive/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。

## 源码原生定位常量补录

以下逐项保留当前源码的RVA/对象偏移命名和值，包括全局槽、vtable/COL、返回位置和只读诊断定位。常量不全是实际patch，旧命名不覆盖模块正文消歧；同一值在不同family中重复保留。`OBJECT_OFFSET`不加模块基址。

| 原始符号 | 类型 | 值 | 源码位置 |
| --- | --- | --- | --- |
| kGameRootSlotRva | RVA | `0x00871678` | [manager_identity_reader.hpp:8](../../../cs-t0/native/include/isaac_cs_t0/manager_identity_reader.hpp) |
| kCanonicalEntryPointRva | RVA | `0x008FE310` | [target_image.hpp:15](../../../cs-t0/native/include/isaac_cs_t0/target_image.hpp) |
| kUnpackedEntryPointRva | RVA | `0x006EFC46` | [target_image.hpp:16](../../../cs-t0/native/include/isaac_cs_t0/target_image.hpp) |
| kGameRootRva | RVA | `0x00871678` | [active_runtime.cpp:37](../../../cs-t0/native/src/active_runtime.cpp) |
| kManagerSlotRva | RVA | `0x0087169C` | [boundary_runtime.cpp:20](../../../cs-t0/native/src/boundary_runtime.cpp) |
| kGameSlotRva | RVA | `0x00871678` | [boundary_runtime.cpp:23](../../../cs-t0/native/src/boundary_runtime.cpp) |
| kMainLoopStateOffset | OBJECT_OFFSET | `0x00000008` | [boundary_runtime.cpp:29](../../../cs-t0/native/src/boundary_runtime.cpp) |
| kGameplayDispatchFlagsOffset | OBJECT_OFFSET | `0x0004ABBC` | [boundary_runtime.cpp:30](../../../cs-t0/native/src/boundary_runtime.cpp) |
| kGameRootSlotRva | RVA | `0x00871678` | [coop_passive_runtime.cpp:34](../../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kRunActiveOffset | OBJECT_OFFSET | `0x0002658A` | [coop_passive_runtime.cpp:38](../../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPressedRva | RVA | `0x0061E8D0` | [loop_runtime.cpp:27](../../../cs-t0/native/src/loop_runtime.cpp) |
| kTriggeredRva | RVA | `0x0061E970` | [loop_runtime.cpp:28](../../../cs-t0/native/src/loop_runtime.cpp) |
| kScalarRva | RVA | `0x0061EA20` | [loop_runtime.cpp:29](../../../cs-t0/native/src/loop_runtime.cpp) |
| kManagerSlotRva | RVA | `0x0087169C` | [offline_attestation_runtime.cpp:21](../../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kGameSlotRva | RVA | `0x00871678` | [offline_attestation_runtime.cpp:33](../../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kRunActiveOffset | OBJECT_OFFSET | `0x0002658A` | [offline_attestation_runtime.cpp:34](../../../cs-t0/native/src/offline_attestation_runtime.cpp) |
| kGameRootRva | RVA | `0x00871678` | [raw_apply_runtime.cpp:24](../../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kManagerSlotRva | RVA | `0x0087169C` | [runtime.cpp:25](../../../cs-t0/native/src/runtime.cpp) |
| kGameSlotRva | RVA | `0x00871678` | [runtime.cpp:35](../../../cs-t0/native/src/runtime.cpp) |
| kRunActiveOffset | OBJECT_OFFSET | `0x0002658A` | [runtime.cpp:36](../../../cs-t0/native/src/runtime.cpp) |
| kPreGameExitCallRva | RVA | `0x002FA0E2` | [runtime.cpp:37](../../../cs-t0/native/src/runtime.cpp) |
| kPreGameExitDispatcherRva | RVA | `0x004650A0` | [runtime.cpp:38](../../../cs-t0/native/src/runtime.cpp) |
| kGameRootSlotRva | RVA | `0x00871678` | [coop_passive_main.cpp:169](../../../cs-t0/native/tests/coop_passive_main.cpp) |
| kRunActiveOffset | OBJECT_OFFSET | `0x0002658A` | [coop_passive_main.cpp:173](../../../cs-t0/native/tests/coop_passive_main.cpp) |
| kGuardRva | RVA | `0x005BAC6D` | [coop_passive_main.cpp:1009](../../../cs-t0/native/tests/coop_passive_main.cpp) |
