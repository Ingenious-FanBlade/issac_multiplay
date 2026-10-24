# tx-01-84：retained Player／Replace 与 GridFree 触发方法

日期：2026-10-19。目标：静态定位 tx-01-83 的两类场景缺口，修正可移除 fixture 和 Replace 身份取证，然后通过一个统一入口完成这两个场景的一次 E2E。准备过程中不查询或操作游戏，不运行 Lua/Python/验收或其它测试；旧 FAIL/NOT_COVERED 不改判。controller、70 槽、协议与 scheduler 不修改；Native 仅修正 Replace 观察分支的对象选择。

## 修改前登记的失败方式

- 道具 action 返回或 MC_USE_ITEM 回调并不证明实际 Replace；通用效果分派与原生活动槽消费混淆，会再次得到空覆盖。
- 用 CustomVarData=0 强改槽上下文、重新 AddCollectible 或在替换后持有旧 A userdata，会破坏 Esau 槽关联或操作已保留的对象；每次操作重新取得当前 ordinal 0，不写 VarData。
- 界面暂停、道具未充能、按键未经过 canonical Submit/Resolve、双击/长按导致重复效果，均必须保留实际输入与未触发结果；不自动重试。
- RemoveGridEntity 的延迟移除、SpawnGridEntity 的虚析构、立即释放入口不能互相冒充覆盖；仅凭格子变空或生成成功不足以判 GridFree 被调用。
- D12 会扫描整个房间，若还有非 fixture 的可重掷 Grid 就可能越出自建对象范围；操作前保守拒绝这种房间，不清理/覆盖原有 Grid 来满足条件。
- fixture Grid 在换房、被破坏、被替换或种子/类型变化后，不能继续按旧索引认定自有对象；新操作记录 room/frame/index/描述，失败立即停止本 run 的 fixture 操作。
- 准备、回调、自然帧等待或 Lua 日志异常必须进入明确 fixture_error；日志或标记不补造 native birth/use/membership。
- 同一动作只请求一次；等待原生操作的状态未完成时拒绝其它 fixture action；自然帧超时、多个 use 回调或来源不符保留错误，不改参数重试。
- Replace 的 ECX 是 manager，不能调用 Find(0, manager, true) 构造假 Entity 身份；只关联栈中 source/target 两个已有 Player 身份，保留物理 ID，记录 revision。缺身份继续明确未知。

## 当前静态依据

IDA MCP 已恢复，当前模块 isaac-ng.exe，imagebase=0x400000，输入路径与既有目标相同。只读反编译、xrefs 和原始指令保存在 [ida-evidence.json](../../../../artifacts/cs-t0/tx-01-84-static/ida-evidence.json)，没有哈希校验或 IDB 修改。0x5B39D0 全函数反编译失败如实保存；相关分支用原始指令及 IDA switch 映射核对。

1. Lua UseActiveItem 注册链 0x881F30 → 0x8ADF50 → 0x5B39D0；item 703 的 switch 目标为 0x5B83CF，进入通用收尾，没有 Esau 槽创建/交换。原生槽消费在 0x7AC7E0，703 分支创建/Init retained Player，0x7ACFD6 调用 Replace 0x9BEE80，返回后 0x7ACFF0 将旧 Player 保存到 Esau 槽。四槽位于 Game+0x1BB28，活动槽 VarData 位于 Player+0x159C+slot*0x20；只观察，不写这些字段。
2. Lua RemoveGridEntity 注册链 0x890CF0 → 0x81E930，仅排队延迟移除；被 Hook 的 GridFree 为 0x81E980，清槽后虚析构。SpawnGridEntity 0x7EBCA0 直接虚析构旧 Grid，也不经过 0x81E980。
3. D12 item 386 的 switch 目标为 0x5BC8CF；扫描 Grid 后 0x5BCA63 调用 0x81E980，后续替换分支还有 0x5BCC4E。普通新建 GRID_ROCK=2 属于前一个调用点覆盖的类型，且需 ToRock 非空。不直接调用内部函数，也不更换 Hook 目标来获得覆盖。

## 实现与验收入口

- fixture v1.1：Esau 三次请求改为有界等待正常物理道具键，保留实际 callback/flags/slot/VarData 及前后公开状态；每次重新取得当前 A，不覆盖输入、不改 VarData、不再直接 UseActiveItem(703)。GridFree 使用公开 D12 效果调用，先保守检查整个房间只含自建的可重掷岩石，并记录原始前后布局。异常后本 run 不能重新 enable。
- Native Replace 观察修正：ECX 继续仅作为 manager 参数由 wrapper 保持；身份从 args[0]/args[1] 关联已有 Player，更新 revision，不创建 manager 的 Entity 身份，不把保留源当析构或把目标重新发布当新生。原函数 exactly-once、70 槽、IPC 和 scheduler 不变。
- `cs-t0/accept-world-trigger-scenes.py`：复用既有进程识别、70 槽外部观察、探针消费与收尾，只执行一个新进程中的两个场景。判据包括三次实际成功 Replace、source roster→Esau-retained 且物理 ID/use/birth 不变、实际 canonical 道具边沿，以及自建 seed 岩石经过实际 Hook 17 后退休并从该槽退出。Lua settled 和字段值变化不能单独产生 PASS。旧总账只复制保留，不升级为本轮全阶段通过。
- Native 已直接纯编译成功，输出 `out/cs-t0/tx-01-84-build/native/IsaacCsT0.Native.dll`，完整日志 `out/cs-t0/tx-01-84-build/native-build.log`。仅编译/链接，没有执行程序或测试；Python/Lua 在唯一 E2E 开始前只读审阅，没有 py_compile、语法试跑或单元测试。

完整人工步骤见 [fixture README](../../../../test-mod/isaac-cs-world-e2e/README.md)。API 核对：[UseActiveItem](https://wofsauge.github.io/IsaacDocs/rep/EntityPlayer.html#useactiveitem)、[MC_USE_ITEM](https://wofsauge.github.io/IsaacDocs/rep/enums/ModCallbacks.html#mc_use_item)、[Grid GetSaveState/ToRock](https://wofsauge.github.io/IsaacDocs/rep/GridEntity.html)、[D12=386](https://wofsauge.github.io/IsaacDocs/rep/enums/CollectibleType.html)。触发路径以当前 IDA 原始指令为依据，文档 API 不替代原生覆盖证据。

准备阶段状态：实现和纯编译完成，当时唯一 E2E 尚未开始。此后已执行一次，结果 FAIL，见[专项结果](tx-01-84-e2e-results.md)。既有五项未覆盖和 W12 FAIL 均未因此改判。
