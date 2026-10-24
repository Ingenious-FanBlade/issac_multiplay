> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 正常功能路径静态确认 v17

日期：2026-10-16  
推进：`tx-01-40`（W01–W04 同一目标）  
依据：[工作设计 v17](virtual-device-owned-abi-static-confirmation-next-work-design-v17.md)  
证据：[只读静态证据 v17](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md)

## 结论

在本报告声明的有效对象、主线程顺序操作、Offline/local-co-op、Online roster 为空、B 具有唯一正 K、创建 B 前 Main Player vector 只有 A 等前提内，本轮针对 Join 跨 pass、native remove 引用责任、S2 返回域和 writer 时序所需的原生静态依据**足够**。三类修正均有指定函数窗口或当前源码依据；候选把查询、创建尝试、vector 责任和 previous 写入分配给了明确 owner。

候选在自身范围内一致，但尚未采纳。它的 Authority-pre/current 与 StandardRoom normal-post/previous 和 contract v2 §4 的 `+0x78` poll 唯一 writer 冲突；本轮不改 contract、schema 或 production source。具体 K、selector byte、当前 mode 值、listener 是否实际登记/派发、exact pair 的运行时成员关系未观察。自有 ABI、JoinControl lane、双 participant S2、attempt owner 与 B retire coordinator 尚未接入 production。

v16 中“stored-confirm 跳过 action14 查询”“action19=false 必然无创建”“总 strong 变化等于 -1 且外部 R 恒定”“previous 到 TopUpdateExit 才提交”等说法在本轮明确勘误；v16 原报告、证据和既有审阅记录保留，详见[证据 v17 的勘误表](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md#e06--旧说法勘误与结论索引)。

## 范围和事实标签

正常路径的对象、缓冲、主线程、session、A0/B1 ordinal、ClientId→ordinal 固定映射及 pair/key 分离条件沿工作设计 v17。接受“原生 Join 会可靠创建并发布 B”为 `USER-ASSUMPTION / DESIGN_ONLY`；它不证明 action19 接线、actual call 或发布后置。Online roster、context vector、Main Player vector 与 InputManager pair vectors 分别判断。

候选保持 A/B 共用 canonical、sequence、target tick、queue、dedupe、Resolve/apply，没有房主即时旁路；C/S 与原版/Isaac Sync 隔离，不依赖 Online Manager、NetInputDevice、FrameStore 或全员 ready。正常 pause/loading/noRoom、重复 poll/query/pass、selector 跨 pass、B 退休与 A 下一查询均纳入有限路径说明。

本轮新增的 IDA 读取标为 `IDA-DIRECT`；当前仓库源码标为 `SOURCE-DIRECT`；v16 已有但本轮未重读的布局、ABI、vector tail/middle 责任和 query-family 表保留 `STATIC-INHERITED`；契约条文标 `CONTRACT`；本轮组合推理标 `STATIC-DERIVED`；新实现接口和时序约定标 `DESIGN_ONLY`。本报告没有把继承事实改标为本轮直接读取。

排除 OOM/EH、worker 并发、计数溢出/ABA、全映像 alias/observer 闭包、持久化、异常恢复、全部 Mod/Lua 行为与 05–06 完整网络证明。没有读取任何实机状态，没有测试、构建或游戏操作。

## W01：Join selector 与跨 pass 轨迹

`9BAC40`先逐个走 Manager key/admission；action19 在 `9BB064` 查询。若该 query 为 false，分支到 `9BB22F`，完成临时对象清理、推进外层 key 并回 `9BAC87`。key 循环结束后仍转到 `9BB26A`，遍历四条既有 selector row。因此 action19=false 只否定该 key 在本 pass 的新 admission；如果本 K 已有有效 row，它仍可继续到 action20/21、action14/15 和 `9BB415`。若本 K 没有有效 row，则不会凭 false 新建它。

每条 row 先在 `9BB274–9BB28F` 检查 K、selected index 和 `9BA980` guard。K 或 selected index 为负、或 `9BA980` 返回 true 时跳到 no-create continuation。有效 row 随后查询 action20；20 为 true 时更新 selector 并跳过本轮确认。20=false 才查询21；21 为 true 同样只导航并跳过本轮确认。只有 20/21 都不触发才进入 confirm 分支。

进入 confirm 分支后，`9BB39A` **先调用 action14**。14=true 直接到 `9BB3D8`；14=false 才在 `9BB3A0/9BB3A3` 比较 stored-confirm byte。stored 非零同样允许继续创建，但不会撤销已发生的 action14 查询。stored 为零时才查询 action15：15=true 把当前 selector 清为 `-1` 并不创建；15=false 保留 row 并结束当前 row 扫描。

actual-create attempt owner 只守精确 `9BB415` 的 `9B9CD0` callsite，且只认与 B own receiver/K 精确对应的 row；不把所有 `9B9CD0` caller 纳入 B owner。`9BB424` 的 `7A6450` 是后续 K bind，不是 create attempt 的起点或发布成功证明。拒绝该 callsite 时须补偿 native 已压入的 4-byte selectedType，并转 `9BB561→9BB567` 合法 no-create continuation。JoinControl scope 候选在 `6FADA1`、调用 `9BAC40` 前打开，在正常返回地址 `6FADAC` 关闭；不依赖 action19 成功，也不建立 B Player/consumer TLS。

每个 session 的自有 virtual-device allocate/base-construct/register owner 与 B native Player actual-create attempt owner 是两个责任；设备 owner至多创建一次自有设备，attempt owner至多 admission 一次 `9BB415`。不得以任一 owner 的一次成功代替另一个 owner 的终态。

手工跨 pass 轨迹、原始指令和逐行 owner 见[证据 E01–E02](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md#e01--join-分支的直接指令与条件)。query 是固定 pass snapshot 的纯读取；重复 read 不消费 sequence，也不推进 gameplay Resolve。action19、14/15、20/21 均按已有静态 ABI 作为 Triggered 查询处理；`+0x54` native writeback 不是 JoinControl acknowledgement。

## W02：native remove 的 vector 责任与外部引用

v16 E01 的 `A20EB0/A21570` 账本继续标 `STATIC-INHERITED`：临时 lookup/by-value/通知/shift ownership 各自 acquire/release 后 balanced；B 在 vector 尾部由 native tail release 消费一次，在中部由首个 shift 覆盖 release 消费一次。这个结论只描述本次 Manager vector 的一份 B responsibility。它不推出包括 pinned root 与其他外部 owner 在内的总 strong 变化，也不推出外部 `R` 恒定。

本轮 `IDA-DIRECT` 确认一条有限条件 listener 链：`906D60` 注册 `913760` 到 Manager `+0x34` list；`A1D610` 追加 callback/user-data node；`A212C0` 遍历该 list 并间接调用节点；`913760` 在匹配 NetInputDevice 或 source-pair key 条件下清 secondary/context 项或调用 `902F10(pair,0)`；`902F10` 对已有旧 pair control 调 `+0x0C` 后清该 pair association。

该链要求初始化与登记执行成功、node 仍在被遍历的 list、dispatch 参数及 secondary/source-pair 项满足匹配条件。它不是每次 `A20EB0` remove 都必达的副作用。source-pair 的持久引用若属于 B 的外部集合 `R_B`，匹配回调可改变该集合；它不是第二次 Manager vector decrement。没有该匹配时，本链不产生这项清理，但其他外部 owner 仍不在本轮闭包内。

以 `V` 表示 B 的 Manager vector responsibility，以 `R_B` 表示其他非 root、非 vector 的外部 strong 引用；pinned root 由既定 owner 保持、`Δroot=0`。在 v16 已读的 lookup/by-value/通知/shift 临时均结清的限定下，本次正常 remove 的总账写作 **`ΔStrong_B,total = -1 + ΔR_B`**。`-1` 仅表示 V 被 native 消费一次；`ΔR_B` 允许由上述条件 listener 或其他外部 owner 引起的变化，不能写成零。

退休候选在 `(device,control)` exact pair 复扫确认缺席后仅 `RecordVectorRefConsumedByNative` 并进 `UNREGISTERED`，不得再调用会执行 `port.ReleaseStrong` 的 `ReleaseClass`，也不调用 `FinalRelease` 或旧 `RemoveAndDestroy`。`BASE_CONSTRUCTED` 后对象、control 和有效 base 状态按 `NEVER` 保留，DLL 在 base ctor 前固定且常驻。此处是候选，不是 production coordinator 已实现。

具体 local ledger、匹配/不匹配 listener、B 尾/中部、重复退休及已缺席分支见[证据 E03](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md#e03--引用账本与有条件的listener链)。

## W03：S2 返回域和 writer 时序

候选按查询运行域给出唯一返回：活动 C/S 标准 gameplay consumer 的受控查询，必须由唯一有效的 participant identity、generation、exact pair 与 K 命中同一 S2；任何未绑定、stale、K/pair 不符、多重归属或不支持的受控 action 都返回 neutral（bool false、scalar 0.0），不把即时硬件结果透传进 gameplay。对 A 的物理 query wrapper，原生 `original` 仍按接口恰调用一次；这个调用责任与最终向 gameplay 返回 S2/neutral 分开。UI、非受控 consumer、C/S 未启用及完整恢复后的物理路径保留原生结果，original 同样恰调用一次。

B own receiver 没有可调用的 Base logical original：有效 JoinControl 优先处理 action19/14/15/20/21；其余受控 gameplay query 才读精确 B participant S2；身份不明、retired 或其他 action 返回 neutral。真实硬件 sampler 只从 keyboard/gamepad buffer 读 raw input，不读 synthetic query 输出，也不把 B 的输出回投到 Submit。上述是 `DESIGN_ONLY` 返回候选；当前 `active_runtime.cpp` 只有 A 的一组 dispatcher/adapter，且不合格 identity 会走 TerminalPassthrough 并返回 original，本轮不把它说成已实现。

writer 按两个不同源码边界说明。`StandardRoomHook` 调用 Authority(true) 后才执行 `g_standard_room(self)`；仅当 original 正常返回，才调用 Authority(false)。该 post 经 `EndAuthority(true)` 提交 previous 并关闭当前 visible authority window。完整 top update 则是 `UpdateHook` 先执行 `g_update()`，返回后才触发 `TopUpdateExit`。TopUpdateExit 处理 pause-gap/生命周期边界，不是正常 previous writer；一次完整 update 也可能没有 Authority。

候选字段规则和手工时间线见[证据 E04](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md#e04--s2返回域与writer时间线)。participant snapshot 绑定 session、ClientId、冻结 ordinal、participant generation、exact device/control/K 与 Authority tick；current/previous 各自带 tag。跨 generation、stale 或 invalid previous 一律不能产生 Triggered edge。room/pause/loading/binding lifecycle 使相关 participant generation 失效；B-only retire 只清 B ledger 与 scope，不清 A。正常 Authority post 才可将当期 current 提交为 previous；retire 只撤 visible/admission，不额外写 previous。

Authority-pre current / Authority-post previous / `+0x78` 仅作为 native poll-active 条件的候选与 contract v2 §4 的对象 `+0x78` 唯一 poll writer 冲突，维持 `DESIGN_ONLY / 未采纳`。本轮不修改 contract，也不要求以现场观测解决静态文档范围。

## W04：有限 mode 与可选 Lua callback

`954CD0` 的 `switch (*(n298+8))` 选择 case 2，本身静态意味着当次 `n298[2]==2`；case 内只有 `9C3990` 与 `9505E0` 两个 guard 都未命中才调用 `6FA540`。这澄清的是 case 选择条件，不给该数值附加 Offline/Online 语义，也不声称本轮现场值。`954B40` 的 setter 另在自身 `n298[2]==2` guard 内，且需要有效非 `-1` device K 与 Main Player vector[0]；沿用 `95BCA0` 的 context-empty、`v91==A` 与 exact identity 条件，不能将两条 setter 合并为必达 A rebind。

`956110/9561E0/9562D0` 在 normal native reader 前有 `n298[2]==2 && dword_C71690` 的可选 special-handler 条件。`864C30/864D80` 对 Lua state bit `0x2000`、callback id 13、有效 registry ref 与 non-nil 返回作条件处理；有 callback result 时 query helper 可提前返回，否则继续 native reader。读取只覆盖这段 gate 和 wrapper，不覆盖全部 Mod/Lua callback 行为；该行为不属于本轮目标。

模式字段、setter 和 callback 的直接证据及范围见[证据 E05](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v17.md#e05--有限mode条件与可选lua回调)。

## P01–P06 结论索引

| 主题 | 本轮原生静态结论 | 候选/采纳 | 现场值 | production |
| --- | --- | --- | --- | --- |
| P01 Register/enumerate | 本轮复用 keyed reader、Manager pair 与 HUD slot 分离事实；本轮范围内 Join selector 行为已足以定位。 | frozen ordinal 与 participant binding 为 DESIGN_ONLY。 | 实际 receiver/K/context hit 未观察。 | 自有 ABI 注册和双身份 reader 未实现。 |
| P02 Type/name/binding | `7A6450` 是 key writer；`954B40` 有明确 mode guard，`95BCA0` 有 context/v91/identity 条件。 | 38+6、S2 与 binding owner 依 contract/候选。 | K、v91、n298[2] 当前值未知。 | vptr/K 接线未实现。 |
| P03 Offline Join/Popup | action19 admission 与既有 row selector scan 分开；14先查、stored后查；合法 no-create/actual call 窗口明确。 | JoinControl/pass/AttemptOwner 为 DESIGN_ONLY。 | row、flags、命令及发布状态未观察。 | lane 与 attempt owner 未实现。 |
| P04 Key/B retirement | vector responsibility 恰消费一次；source-pair listener 只在登记、dispatch、key match 条件下清理。 | exact pair absent 后记 native-consumed 并 UNREGISTERED 为候选。 | listener dispatch 与 exact absence 未观察。 | retire coordinator 未实现。 |
| P05 Snapshot/input S2 | 原生三 query kind、consumer 路径与特殊可选 callback 条件有静态依据。 | participant S2 与 Authority writer 候选未采纳；与 `+0x78` contract 冲突。 | 实际 participant/query hit 未观察。 | 双 participant queue/reader 未实现。 |
| P06 Pause/cache/lifecycle | 当前源码分清 physical submit、Authority normal post 和完整 update TopUpdateExit。 | participant generation tags 与 B-only retire 为 DESIGN_ONLY。 | 实际 pause/loading/retire 时序未观察。 | 自有 Hook/lifecycle 集成未实现。 |

仍未直接读取的有限范围是全部 Lua/Mod callback 内容、所有 listener/observer/alias closure、worker/异常路径与全局 mode 字段 writer 语义；它们均在本轮明确排除，不构成本轮声明范围内的新静态缺口。可靠 native Join create/publish 继续只是用户规划假设。现场运行验收、production 编码和 contract 冲突处理仍是后续独立工作内容。

## tx-01-40 唯一整体人工静态文档审阅

结果：通过（文档准确性与声明边界一致性）。一次审阅覆盖本报告、证据 v17、README 当前入口及本轮引用的 source/contract；核对三类勘误、guard、owner、引用公式、返回域、writer 边界、事实标签、引用路径与历史保留，内容与本轮直接/继承证据及契约约束一致。未发现需修正项；没有进行自动修补或复审。

