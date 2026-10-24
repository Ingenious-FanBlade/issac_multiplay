# tx-01-61：实机入口进程可见性修复与 B=1 E2E

日期：2026-10-17。tx-01-60在注入前失败，原因是受限终端看不到目标进程；游戏确实运行，Isaac MCP和可访问宿主进程的只读命令均确认PID45496、creation_time_100ns=134367129629180556、路径F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/isaac-ng.exe。原失败不覆盖，无Hook或游戏输入残留。

实质修复：入口增加从Isaac MCP观察传入的ExpectedProcessId/ExpectedCreationTime，在注入前绑定同一进程，避免把跨执行环境的空进程表解释成游戏未启动；观察数不符时明确报告可见性。实机入口/controller/helper改在能够访问该宿主进程的授权执行环境运行。控制器和native生产二进制仍复用tx-01-58，不重构建、不修改Hook或输入语义。

分析/修复阶段不运行测试。新目标完成后只执行一次统一入口，目录`artifacts/cs-t0/tx-01-61-live-01`；使用相同B=1场景，用户确认B自带药丸，不需要生成或拾取资源。观察实际角色后，开启queries并发送同一session新sequence脚本，完成Pill/Card资源消费、held10纯读、双端输入与队列拒绝、B冲突退休/A继续、回菜单和51Hook正常恢复。继承tx-01-60预列的功能失败方式；新增失败方式为执行环境仍不可见、MCP与入口PID/创建时间不一致、错误进程注入、误把旧失败当作实机FAIL或PASS。

没有第二次注入，不进行完整Loading/死亡/复活专项重跑；失败停止并保留实际结果。

## 唯一整体验收结果

本次统一入口执行一次，退出0，专项 **32/32 PASS**。PID45496、creation_time_100ns `134367129629180556`，游戏`Repentance+ v1.9.7.17`；没有重构建、重新注入或强制卸载。493,550事件、10,374 Authority tick；nativeError=0、transportFailure=null，duplicateResolve/duplicateWorldCommit/duplicateConsumer/duplicateAccept/inconsistentQuery均为false。

- 角色请求#1、工厂前#115、创建后#122均为ID1，原生可选数组35项；一次注册、一次普通创建#116/发布#126，实际Type1/Variant0、exact owned pair。两pass内完成；A健康两次JoinOutcome #71/#121均保持 `[8,8,0,0]`，画面为A/B各四红心。
- B自带Full Health药丸，用户补充信息与画面一致。首次脚本在tick64、generation1、sequence33产生Pill消费#3432：before=`[0,5,0,0]`，after=`[8,0,1,0]`，held_flags=4、edge_flags=4。药丸资源槽5→0，实际变化与Triggered10 #3431对应，不仅是动作flag变化。
- 开启queries后同session新sequence脚本补齐held10：#200820–200822三次Pressed读取均为1，#200823确认值一致、完整Participant状态/queue深度不变。来源均为真实Pill consumer/action/exact-binding上下文的controlled companion（source2），不是引擎主动Pressed10。原生Triggered10及两端普通移动/射击消费另有实际事件。
- 非法、重复、过期、旧generation、wrong-session拒绝通过。同序号冲突#380759触发B-only退休#380762，随后A继续Resolve与实际移动/射击；画面仅A、健康保持四红心。
- 原生menu teardown #493495/#493496；51处目标字节全部恢复，回调关闭，root/DLL常驻。同一游戏进程仍活着，最终画面停在New Run/Continue主菜单；所有发送的物理键均已释放。

专项报告：[result.json](../../../../../artifacts/cs-t0/tx-01-61-live-01/result.json)；[原始事件](../../../../../artifacts/cs-t0/tx-01-61-live-01/capture/events.jsonl)、[恢复记录](../../../../../artifacts/cs-t0/tx-01-61-live-01/capture/restoration.json)、[操作记录](../../../../../artifacts/cs-t0/tx-01-61-live-01/capture/operations.jsonl)、[桌面操作补记](../../../../../artifacts/cs-t0/tx-01-61-live-01/desktop-operations.jsonl)、[进程结果](../../../../../artifacts/cs-t0/tx-01-61-live-01/process-after.json)。源快照、fixture和实际调用参数同目录保留。前序tx-01-60注入前可见性FAIL保留。

本轮只关闭B=1选角和此前Pill/Card/held10两项未观察，并覆盖必要输入/恢复依赖；完整控制器报告仍以原始`NOT_OBSERVED`保留在[full-feature-result.json](../../../../../artifacts/cs-t0/tx-01-61-live-01/full-feature-result.json)，controllerExit=1，因为本轮没有重做首次current edge、Bomb/Drop、Loading、room或死亡/复活等完整用例。专项入口对自己的32项给出PASS，不覆盖旧报告，也不外推完整T0-05、所有角色、OOM/EH/worker闭包。
