# tx-01-85：retained Player／Replace 与 GridFree 专项完成

2026-10-19 19:29:40–19:36:49（Asia/Shanghai）。`accept-gridfree-scene.py` 唯一入口执行完成，**专项 PASS：2 PASS / 0 FAIL**，进程退出码 0。准备期没有测试或试跑，本轮没有构建。

这是两个指定场景的结论。tx-01-84 的 FAIL 和 tx-01-83 的 25 PASS / 1 FAIL / 7 NOT_COVERED 总账保持原样；没有重跑三次 Esau，也没有把本轮解释为完整 T0-06 通过。

## 可核对产物

- [result.json](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/result.json)：S85.replace_retained / S85.gridfree 均 PASS。
- [finalization.json](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/finalization.json)、[收尾诊断](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/finalization-diagnostics.json)：COMPLETED、PASS，errors=[]。
- [Replace 证明及文件偏移](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/prior-replace-evidence/scene-proof.json)：只读解析 tx-01-84 原始 trace/world/events/fixture，明确保留旧 GridFree 拒绝。
- [新 GridFree 证明及文件偏移](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/new-gridfree-evidence/scene-proof.json)、[生命周期事件](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/captures/gridfree/capture/world/lifecycle-events.jsonl)：实际调用、旧 Grid 退休和槽成员变化。
- [独立 70 槽恢复](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/captures/gridfree/external/after-normal-stop.json)、[采集器结果](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/captures/gridfree/capture/result.json)、[探针结果](../../../../../artifacts/cs-t0/tx-01-85-gridfree-e2e-01/captures/gridfree/capture/e2e/result.json)。

manifest.json、binary-provenance.json、gridfree-plan.json、controller-invocation.json、sources/、binaries/、operator-actions.jsonl 和 fixture/final.log 保存使用版本、输入步骤与现场材料。复查可直接阅读这些现有文件，不要求再次运行游戏或分析器。没有计算或校验哈希。

## Replace／retained：沿用实际采集，修正汇总

tx-01-84 的真实三次正常道具按键，经 canonical Submit/Resolve 后分别在 outer 6176、9279、10270 调用 Replace。新汇总只取 subject=0 的入口参数，另核对 Entry/Original/Return 各为 3，三个返回 AL=1。

| 次序 | source local_id/use | target local_id/use | 正常使用的 generation / target tick |
| --- | --- | --- | --- |
| 1 | 1/1 | 49/1 | 10 / 2660 |
| 2 | 49/1 | 1/1 | 15 / 5150 |
| 3 | 1/1 | 49/1 | 20 / 5738 |

每次 source 都从 roster source 8 进入 Esau retained source 9 且不再属于 roster；target 成为 roster ordinal 0。物理 ID/use 不变，生命周期事件中的历史 birth_event 仍为 1 和 5，没有把 manager 当 Entity，也没有把重新发布视作新生。

输入按现行协议 ActiveItem mask=2、Accepted=1 解码；三个 Submit.target 均与同 generation 的 Resolve.tick 对应，原 command/tick/target 完整保存。旧脚本误用 Bomb mask=1 和双计参数副本的 FAIL 不改写。

第二、三次 Replace 内有实际 PlayerRestore。其后当前样本的 birth_event=null 继续表示映射可用性 UNKNOWN；旧出生事件仅证明物理历史，不能据此恢复跨 Restore 的可比较状态。专项通过的是实际替换、retained 成员关系及物理身份保持。

## GridFree：新进程的实际触发

用户手动准备全新进程 PID 33340，以撒 / Normal / RD2W XWC9；实际 seed=1998345781，room=84。复用 tx-01-84 Native、tx-01-83 controller、tx-01-79 Helper；只执行 fixture v1.2 的 enable / gridfree-spawn / gridfree-use。

1. frame 6932 在空槽 index 69 创建 seed=9730012、type=2、实际 variant=2 的岩石。v1.2 保存这个实际描述，而不是把请求 variant=0 当固定结果。
2. frame 7049 的 `gridfree_validation` 记录实际与期望均为 type=2/variant=2/seed=9730012，ToRock=true；整个房间没有其它 D12 可重掷 Grid。
3. 公开 D12 效果经过原生 0x5BCA63 → 0x81E980，Hook 17 的 Entry/Original/Return 各一次，outer=13304、index=69。
4. 旧岩石 local_id=48/use=1、birth_event=3、地址 3a6074dc，出现真实 `GRID_PHYSICAL_RETIRED`（event_kind=4、sequence=82）。outer=13306 的普通样本确认旧对象退出该槽；证明文件给出原始 byte offset。
5. 新 Grid factory/Init/Spawn 确认 local_id=49/use=1、birth_event=4、地址 3a607684；公开布局中 type=14/variant=0/seed=2304574500。没有用 Lua settled 或画面变化代替 Hook 证据。

## 健康与恢复

新采集 6,335 个普通样本全部匹配 owner/Room/tail/commit/TopExit 阶段，trace_integrity=true、errors=[]，没有 fixture_error 或字段/来源读取失败。15,820 批、93 个生命周期事件全部复制、写出并 flush，backpressure=0。1,185,534 条探针记录全部消费，overflow=0。复制耗时 0.0415–0.3511 ms；只读/RNG 观察未报告异常。

控制器正常停止，nativeError=0、transportFailure=null，world native_error=NONE；原生及独立字节观察均确认 70 槽恢复。模块与自有设备按既有策略常驻至游戏退出。测试 Mod v1.2 仍安装，正式游玩前应在游戏正常退出后禁用或移除。

## 复现入口与操作

执行入口为 [accept-gridfree-scene.py](../../../../../cs-t0/accept-gridfree-scene.py)，参数如下；必须使用新 artifact 目录，实际授权和人工新进程准备按现行规范处理：

```powershell
& 'C:\Users\114514\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' -u .\cs-t0\accept-gridfree-scene.py `
  --game-exe 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe' `
  --controller '.\out\cs-t0\tx-01-83-build\rust\release\IsaacCsT0.exe' `
  --native-dll '.\out\cs-t0\tx-01-84-build\native\IsaacCsT0.Native.dll' `
  --helper '.\artifacts\cs-t0\tx-01-79-live-remaining-01\binaries\IsaacCsT0.Helper.exe' `
  --catalogue '.\artifacts\cs-t0\tx-01-83-instrumented-e2e-01\captures\primary\capture\restoration.json' `
  --prior-evidence '.\artifacts\cs-t0\tx-01-84-trigger-e2e-01' `
  --game-log 'C:\Users\114514\Documents\My Games\Binding of Isaac Repentance+\log.txt' `
  --artifact-dir '<新的独立目录>' --authorization '<本次实际授权范围>'
```

准备 fixture v1.2 的同种子新 run，输入 `start gridfree`，待 B 和健康样本后，以 `lua Isaac.RunCallback(22, "cswe2e", "<action>")` 依次请求 enable、gridfree-spawn、gridfree-use；spawn 后关闭控制台运行约 2 秒，use 后约 4 秒。然后 observe / fixture-log / stop，等待一次统一汇总。完整 Esau 触发方法及失败即停止规则仍见 [fixture README](../../../../../test-mod/isaac-cs-world-e2e/README.md)。

## T0-06 剩余范围

本任务要求的 retained Player／Replace 和 GridFree 触发及直接取证已完成。完整窗口/gap 因果配对、短命对象完整退休、重复发布历史的其余要求、非空引用歧义及 W12 的原失败仍需后续处理；全量 replay 和真实 Online/Sync 隔离继续排除。没有把这些整项账本自动升级为 PASS。
