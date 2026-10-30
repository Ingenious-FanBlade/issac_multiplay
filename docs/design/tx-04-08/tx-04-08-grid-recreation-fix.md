# tx-04-08：按公开 API 修正 Grid 替换夹具

> **后续e2e-11实测（2026-10-29）**：[记录](../../../artifacts/repentogon/tx-04-08-e2e-11/execution-notes.md)。variant仅观察生效，ROCK创建、删除后同格创建POOP及移除均到达，旧退休sequence604→新出生618的原生归约单项通过。整体仍因新录制等待误判旧Closed而FAIL，第二进程/其余必需项未完成；不将Grid单项通过外推为tx-04-08整体通过。下方更早的未测描述保持原交付语境。

> **当前 variant 策略修正（2026-10-29）**：用户要求不严格比对 Grid variant。创建返回、跨帧到达、替换/删除前态及最终重建归约均取消实际variant与固定请求值的等值要求；variant不作身份依据，只记录请求值、创建后实际值、后续实际值和夹具先前观察值。保留位置、类型、显式seed、varData、自有标签/房间代次及原生退休→出生证据；原生调用实参仍与实际请求匹配。e2e-10失败保留，新策略只修改与纯编译，没有实机或离线测试。

> **后续e2e-10实测（2026-10-29）**：[原记录](../../../artifacts/repentogon/tx-04-08-e2e-10/execution-notes.md)。首次Grid创建空地检查通过，API实际返回boolean true，但岩石variant请求0、实际2，后态参数核对失败；尚未到达删除后创建POOP的场景。该修复的重建/关联路径仍未实测，不因编译或新日志而预记通过。失败后未改实现或自动重跑。

日期：2026-10-29，Asia/Singapore。用户指出完整岩石不能直接用生成 API 覆盖，并提供本地 Wiki `F:\projects_isaac\IsaacDocs`。范围为 tx-04-08 夹具、scenario 与同一动作的最终原生证据归约。本轮只阅读资料/源码、修改、静态审阅及纯编译，不部署、不启动游戏、不运行离线验收或单元测试。没有开展新的二进制逆向。

## API 核对与原错误

- [本地 Isaac.GridSpawn 文档](F:/projects_isaac/IsaacDocs/docs/Isaac.md:486)明确 forced 不是任意覆盖：不能覆盖完整岩石，可以覆盖部分已毁对象；应检查实际返回对象/类型，必要时删除后重新生成。它返回 GridEntity，而 `Room:SpawnGridEntity` 返回 bool，两个 API 的参数和返回契约分开。
- [本地 GetFreeNearPosition](F:/projects_isaac/IsaacDocs/docs/Isaac.md:264)返回最近空区域。本次只把它用作固定 index 的只读复核，不自动移动动作目标。
- [本地 GridEntity.ToPoop](F:/projects_isaac/IsaacDocs/docs/GridEntity.md:115)返回 GridEntityPoop 或 nil；它是取得派生接口的类型转换，不是将岩石原地变成便便的 Morph。目标类型来自[GRID_POOP=14](F:/projects_isaac/IsaacDocs/docs/enums/GridEntityType.md:22)。本地 REPENTOGON 的 ToDecoration/ToLock 等公开源码同样先检查已有描述类型再转换 Lua 接口，不创建对象。
- 本地 SDK `F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/Room/LuaRoom.cpp:33–63`：显式 seed 的 SpawnGridEntity 路由调用 Room owner 并 `lua_pushboolean`；RemoveGridEntityImmediate 为独立 void owner。既有原生分支资料只能说明存在换槽路径，不能证明任意被占用格都能通过前置检查。

[e2e-09](../../../artifacts/repentogon/tx-04-08-e2e-09/execution-notes.md)中的首次 ROCK 创建成功，随后在完整岩石上直接重复 Spawn 失败。将此作为有效替换的固定夹具是实现错误。旧失败和“未到达后续场景”保持原事实；本修复不补写该次返回或事件。

## 当前动作

scenario 第24项仍在固定空格 index32 创建 ROCK(type2, variant0, seed400809)。第25项明确 `replacement_mode=remove_then_spawn`、expected_type2、目标 POOP(type14, variant0, seed400810, varData0)、PathTrail0、keep_decoration=false。第26项独立移除更新后的 POOP，expected_type同步为14；自有标签名称rock仅为夹具标签，不代表当前类型。

`world_fixture.lua` 在任何删除之前校验所有目标参数、房间代次、旧对象类型/variant/seed、自有标签、原始 PathTrail 与全部门槽排除关系。然后：

1. 记录旧视图，调用一次 RemoveGridEntityImmediate，记录其真实空槽后态，废弃旧标签。
2. 进入独立 pending 阶段，跨过一个真实 Game frame；不额外调用 Room:Update，不解引用被删除的旧 userdata。
3. 再核对房间代次、当前同格无对象、无碰撞、GetGridPath=0、GetFreeNearPosition 返回同格。任何失败停止动作，不寻找其他格。
4. 调用一次带显式 seed 的 SpawnGridEntity，先记录实际返回类型和值及当前后态，再判断返回是否为 true。
5. 复核 type/seed/varData，记录请求和实际variant；目标为POOP时才用 ToPoop 验证专用接口；更新自有标签。下一真实游戏帧再复核后态后写 arrived，variant变化不阻断到达。

删除后创建失败不会恢复原对象或重试。`route=remove_then_spawn`、remove_call/return、spawn_call/return、raw_return、removed与after均保留在日志中；Lua到达不冒充原生物理退休/出生。

## 原生归约与边界

`episodes.rs` 将本动作的 GridFree 三个实参及 GridSpawn 五个实参分别精确匹配，继承已有 Grid 标签的真实出生 local/use。删除前旧身份与新出生身份分开，新标签只绑定新出生；之后的独立 Remove 不混入旧身份。

`grid_recreation` 必须同时满足：同 call 的 wrapper/body 正常返回、旧非空槽→空槽、已标记旧身份的 GridPhysicalRetired、其后进入的新 GridSpawn、原生 true、空槽→非空槽、同 owner 新身份 BirthConfirmed，以及单一 Lua 终态的匹配后态。V5 的 grid_replace 现在要求这两次原生 owner 及此关联证明，不能仅见一个 Spawn 和任意退休事件就通过。

该动作验证同格删除后重建。`direct_native_overwrite_claimed=false`，没有把它登记为原生单次直接覆盖，也没有证明在已炸毁岩石等其他前态的直接覆盖分支。旧直接覆盖资料与 e2e-09 失败保留。

## 构建与尚未验证

只对修改后的 Rust harness 做离线 locked i686 release 纯编译，19.59 秒成功；Lua 采用静态阅读，没有运行语法/夹具自测。日志 [rust-build-grid-recreation.log](../../../artifacts/repentogon/tx-04-08-defg-build/rust-build-grid-recreation.log)，已更新 `out/repentogon/harness.exe`。DLL 未改。

空地查询、跨帧后 POOP 创建及对应原生关联仍须未来一次用户授权的整体 E2E 实际观察；本轮不预记通过或自动启动游戏。30秒、主250/对照32及其他生命周期/隔离规则保持。

## variant 仅观察的具体修订

`world_fixture.lua` 在立即创建后与下一帧到达时均不比较实际variant和请求variant；原来的 `expected_variant=0` 前态限制从 scenario 和夹具删除。标签记录实际variant，后续动作仅将先前值和当前值写入 `variant_observation`，不因variant变化而误判对象已复用；身份仍须同房间代次、同格、自有标签、预期类型及seed匹配。

`episodes.rs::grid_recreation` 同步移除后态variant等值判定，输出 `variant_policy=OBSERVE_ONLY`、requested_variant和observed_variant。GridSpawn原生调用的variant实参仍与scenario实际请求匹配，这用于关联发出的调用，不要求游戏最终描述保持该值。Entity Spawn/Morph的variant规则和跨文件字段原值保存不受此调整影响。

这项修改遵循用户要求，不据此认定接口已存在已证实的bug，也不把 e2e-10 的失败重新标为通过。新策略 i686 release 纯编译17.75秒成功，已更新 `out/repentogon/harness.exe`，DLL未改；日志：[rust-build-grid-variant-observation.log](../../../artifacts/repentogon/tx-04-08-defg-build/rust-build-grid-variant-observation.log)。未运行Lua语法/夹具自测、游戏或离线验收。
