# tx-04-08 D–G 工程续作与主审

> **后续执行记录，2026-10-28**：首次整体入口因第三方 mod 启用在启动前失败；用户明确许可重新执行并临时禁用该mod后，e2e-02在第一进程的首条counters启动期超时，统一归约的L7另发现legacy gap空来源头差异；[结果与清理](tx-04-08-e2e-results.md)。已修正先等既有菜单就绪和显式legacy的15个零头读取，Rust纯编译成功，尚未为修正再运行。以下“尚未执行”保留编写时点；没有自动重跑，完整功能仍未验收。

日期：2026-10-28，Asia/Singapore。继续同一 tx-04-08，按[现行规范](../../refactor/project-conventions.md)执行。本轮尚未部署、启动游戏、运行 replay/compare 或任何测试；纯编译与静态阅读不代表运行通过。A/B/C 前序事实见[ABC续作](tx-04-08-abc-implementation.md)。

## C 补充：自然 Item 输入与两侧对象证据

夹具已接普通 Isaac A、Type=1/Variant=0、primary slot0 的公开准备：记录原道具/充能，执行一次 `AddCollectible(703,1,false,0,0)`，核对真实持有、slot与一格充能，再由 harness 按一次 SPACE。Lua 不直接调用 UseActiveItem，不写 pending item/Player 字段，不创建 B。原 primary 的替换或掉落由原生 AddCollectible 处理，不能声称对当前 run 没有影响。

该动作安排在 S10 Continue 已完成、A/B 已唯一绑定且先有健康观察之后，仍属于 S5 的 Replace 覆盖；不新增启动或验收。前序 2,000/256 稳定段、S6、故障和 S9 保持原序。Esau 保留的旧 Player 不自动获得新的读取授权，因此动作后允许保留其 NOT_READY/Incomplete，不能以此删来源或补字段。

主审复读现有 SDK `ControlActiveItem` 入口 VA `0x789E50`，保存[完整反编译](../../../artifacts/repentogon/tx-04-08-de-static/main-review-control-active-item.json)。`0x78B47F` 使用真实槽的 collectible、slot 和 owned flags 调 UseActiveItem；`0x78B4E8–0x78B4F4` 按返回低字节直接调 helper。这条 consumer 路径无需内部 `0x5BED54` 的非零条件；它仍受共享收尾的 ItemState 条件影响，不能误称完全不依赖该状态。旧直接 Lua Use/charge0 草案已撤回。代理详细推导见[原始路径记录](../../../artifacts/repentogon/tx-04-08-de-static/player-control-active-route.md)。被排除的 `0x7A9440` 候选阅读保存在 `main-review-7a9440-rejected-route.json`，不用于证明该输入链。

主审另外回读[flags/分支原码](../../../artifacts/repentogon/tx-04-08-de-static/main-review-use-flags.json)和[getter/返回/跳表原值](../../../artifacts/repentogon/tx-04-08-de-static/main-review-item-state-return.json)：703 减0x11B得到0x1A4，`0x5C7700` 的0x5F再索引 `0x5C7514` 得0x5B4E3F。该块写的是 sparkle 状态 var_5BD，不是结果低字节；初步将其当返回标志的推断已纠正。真正结果 var_5E4 从低字节1开始，PostUseItem 回调可改变它；`0x5C7028–0x5C7049` 只在 ItemState 为0或0x2F时保留非零。公开 getter 的 Lua 绑定 `0x8502C2→0x431860` 对应 `mov eax,[ecx+15F4h]; retn`。夹具在 Add 前后均只读要求 `GetItemState()==0`，把实际值随 input_prepared 保存，不调用 Set/ResetItemState。普通原生使用的 flags=4（可选强制位未置位）及 Variant0 路径静态可达；回调覆盖、容量不足、运行状态改变仍必须由真实成功返回与两侧证据裁定，不宣称无条件成功。

Lua 只观察准备之后的真实 `MC_POST_PLAYER_INIT`、当前 A/旧 userdata 与公开状态，不要求猜定的 PlayerType 变化。Lua arrived 仅结束等待，不证明交换成功。V5 另要求同一 action_id 的准备、同录制 watermark 内唯一成功 Replace outer/body、原始 old/replacement 两参数分别对应前后 ordinal0，且旧对象后来确在 source9 Esau 槽并退出 ordinal。episode 保留两侧字段/资格/成员原值；对象进入保留槽不算物理销毁。默认 scenario 已启用此限定动作，尚未执行。

## D：完整复制窗口的局部 RNG 见证

沿用 ST3 的 Entity dropRNG `+0x3DC..+0x3EB`、Grid RNG `+0x2C..+0x3B`，均为四个原始 uint32。没有增加 Game/全局 RNG 的读取授权或布局声明。

Collector 先用有界预枚举建立观察集合。预枚举不在所证明的窗口内，也不提交 membership/birth。随后保存集合的 domain/address/local/use/revision/vptr/class 和前四字，再清除该次临时去重/成员位，执行正式完整来源枚举、字段复制、引用解析、动态 descriptor/逐元素指针/固定槽/door 映射复核及 root 复核，最后读同一仍有效集合的后四字。预枚举失败、前后集合变化、退休/复用/分类变化、root/source 污染或读取失败均保留不可确认；尾部身份不一致时不再读旧地址。Grid 的前置资格与原 CopyObject 一致：已确认初始化 byte、真实槽和 index；不补造 Grid Init 历史。

静态主审阅读了 `PreparePostState/BeginRngWindow/CopySources/CopyObject/CopyFields/ResolveReferences/EndRngWindow`、identity 的值操作与 game/world_bridge 的调用次序。窗口内只经主线程受保护原始复制、固定数组/身份查找和自有原子状态，不调用原生更新、Lua、getter/setter、Spawn 或 RNG 推进器。`std::sort` 仅整理 Collector 自有观察集合，无运行时分配。before/after 是有界扫描，不是假装全世界原子快照；外部未知 worker/mod 不在结论内。

`RawRng` 为40字节；RawObject 从2592变2632，BatchHeader从528变576，RawBatch从43,516,432变44,171,840。三槽增加1,966,224字节；预枚举工作表位于后台构造的 Collector，按 `sizeof(Collector)` 实际记账。编译期总预算断言继续约束256MiB，队列按实际 `sizeof(RawObject)` 计费，无游戏线程大栈数组。文件中每个对象保存两组四字及独立状态，capture 保存集合计数、两端实际线程、QPC/频率。replay 原值保留；出现的新 RNG 诊断严格检查四字和状态/值对应。历史没有此诊断的文件不会被补造见证。

V7 输出 `evidence/rng-window-index.jsonl`。完整普通样本逐个要求同集合、同线程、两端读取成功且四字相同；不完整 capture 另列不可确认及原值。两个边界相同本身不能排除中途改动后恢复，必须与上述路径静态分析一起阅读；不宣称全游戏 RNG 不变。

## E：原生关闭判断处的非阻塞收口

逆向由用户指定的 GPT-6 Luna/max 进行，见[代理静态报告](../../research/tx-04-08-de-exit-agent-notes.md)。主智能体独立复读 main 指令和原始字节，保存[主审指令](../../../artifacts/repentogon/tx-04-08-de-static/main-review-main.json)及[十字节原值](../../../artifacts/repentogon/tx-04-08-de-static/main-review-close-bytes.json)。接受范围如下：

- `0x90AF20` 为 `mov eax,[0xBAE270]`，`0x90AF25/0x90AF2C` 通过 graphics/display setup 条件进入 `0x90AFC4`。不将该全局 byte 解释成 close flag，也不写它。
- `0x90AFC4` 的4字节 CMP 与 `0x90AFC8` 的6字节 JNZ 原值为 `80 78 08 01 0F 85 73 FF FF FF`；不等时回 `0x90AF41`，等于1时落到 `0x90AFCE` 唯一已确认普通 Shutdown caller。Shutdown 后没有下一次 Manager。
- WM_CLOSE 和消息泵的 WM_QUIT 均可令原 main 比较满足退出请求。异常/强制终止，以及尚未独立核定的菜单 UI“退出”实现，不从该普通控制流外推。

统一 `hooks.json.byte_patches` 新增 `world_process_exit`；生成器分别生成 Host 与退出编译表，避免按旧数组位置把新补丁混入 Host/K。仍由首次 Manager 的 `InstallRuntimePatches` 单一 owner 安装，Hook/补丁常驻，world start 要求退出桥实际可用。安装时完整指令形状不匹配即关闭 world 能力，不修改第三方补丁、不自动重装。

`ExitCheckShim` 搬移完整 CMP/JNZ；每次原 CMP执行一次，未请求关闭时直接选择原循环目标。请求关闭时保存 GPR/EFLAGS 和16字节对齐 FXSAVE 状态后通知命令 owner；策略延后只清本次保存 flags 的 ZF，恢复全部上下文后选择原两个目标。没有写 GLFW window/close byte，也没有将不完整 SDK GLFW 结构当作布局契约。

主线程首次请求关闭即停新录制与 session publication，清除排队会话启动，StopRecording/StopSession 后仅查询自有状态。下一轮原 Manager 尾部退休 K/B/网络；背景 worker 继续排空、flush、等待自有 reader/writer 的真实退出。main helper 不等待、不调用 Sleep、不递归补跑 Manager，不在 DllMain join 或释放在途上下文。job 已归还且 session 已退休才放行原 Shutdown；自有期限耗尽也放行，但 trace 明确 timeout/未收口，现有失败前缀保留，不制造 Closed。K NEVER 不变。

没有 recording job 时，退出桥保持原始关闭分支透传，不发起新的 session 关闭流程；只在本录制的关闭协调已开始后持续推进该协调。

同一整体 E2E 的两次原定启动内，第一进程在 Continue Recording/Host 活动时请求 WM_CLOSE；第二进程在原比较录制正常 stop、Host stop、返回菜单后另开 exit-menu 录制，对本 harness 创建的 PID 主窗口线程投递一次 WM_QUIT。区分显式 stop、活动 run 关闭、菜单消息泵关闭、失败清理与强制终止。`direct-exit-request.json`、`world_exit request/release/status`、真实 recorder result/worker/flush 与进程结局共同参与 V9；未执行不通过。不把 WM_QUIT 测成“菜单 UI退出已覆盖”。

## F：最终判定和离线关联

新增 `tx08/obligations.rs`，在同一最终入口消费原始文件。V1 对照 legacy/world outer/world native 三份清单、能力、ABI、注册与实际 native 次序；停止前后保存实际 counter 窗口，零调用时不冒称观察到透传。原调用配对现在检查外/内层进出栈和 parent 顺序，数量相同不足以通过。注册不等于安装到达。

V2 扫描 primary/comparison/healthy/continue 的全部完整 capture，逐一关联同 loop/scope/tick 和三阶段正常返回。V3 同时要求 primary 2,000 与 comparison 默认256次，明确 odd-only 例外不变。V4 保存逐类固定字段、资格、完整来源、实际448 Grid槽和真实 door/Grid 别名。V5 根据 episode 的参数、parent/local/use/revision、真实 body/出生/移除/清引用/退休判定，复用/重复发布仍须实际观察，缺失不改 PASS。V6 回放成功 witness 和撤销事件，关联原生 Player body、Authority、exact pair、session/participant generation、线程与当前字段；FIRST_OBSERVED 不补出生。V7 见 D。V8 保留 ABC 的11种真实自有故障。V9 见 E。

V10/L1–L5 保持真实生产 CLI；L3 增加完整 run/room/tick/Player key/field 名称路径、唯一差异、first_difference 和 earlier_coverage_unknown 对照，不能只用同名 field_id。L6 增加双方实际 Submit/sequence/target/session/input-generation 与消费值核对；seed/CLI 成功不足以配对，未映射出生继续不可比。V11同时要求L6/L7。V12另输出输入/完成/Ack、Join/K NEVER与退休、loading/Continue、原生持久入口/跨帧窗口、目录通知和逐窗口文件变化，D18/D20/D27 原规则不变；已增加 room/death/revive/stage 各操作前后文件窗口。未知 cloud/ESSM 子结果保留 UNKNOWN，不拿父调用正常或最终文件净变化冒充所有中间操作成功。

性能索引保留原始 QPC/频率、reader copy ns、队列/内存峰值、背压、后台实际 WriteFile/FlushFileBuffers 耗时及离线CLI耗时/吞吐/flush。raw生命周期、RNG、Player 和 session/persistence 索引可回到录制名/attempt/event/trace 序号。报告仍区分执行失败、判定失败、未执行、未观察、清理错误和不可比较区域。

## G：完整 legacy 原始材料

用户本轮提供旧研究仓库 `D:/projects/isaac_hook_reserach`。静态阅读选用 `artifacts/cs-t0/tx-01-85-gridfree-e2e-01/captures/gridfree/capture/world`：六个原始文件 manifest.json(14,832字节)、captures.jsonl(596,907,184字节)、lifecycle-events.jsonl(26,716字节)、result.json(1,461字节)、consumer.json(748字节)、restoration.json(48,569字节)。这是原生录制目录，未选 replay 派生目录。consumer 原值显示 reader_done、files_flushed，实际解析/计数一致性仍留最终 L7。

scenario 保存绝对来源和可读说明；统一入口复制六个原件至本次 `legacy-input/`，保存来源/尺寸清单，最终 L7 明确 `--legacy-t0`。不启动旧 runtime/Helper，不修改原件，不校验哈希，不导入历史 PASS，不宣称跨版本语义 equal。原终态、consumer/restoration 随 replay 保留。

## 编译与当前未决

必要纯编译日志见[本轮构建目录](../../../artifacts/repentogon/tx-04-08-defg-build/README.md)。尚未执行唯一整体 E2E；每次启动仍须分别向用户说明实际动作及停止手动操作时机并取得准备确认。

剩余工程代码与限定参数已接通，完整 DLL 和 Rust 纯编译成功，out 中产物/清单/构建出处已更新。特殊 Player Replace、自然复用/重复发布、七类参数实际可用性、完整稳定段、所有故障、两条直接退出和所有最终判定均仍为运行未观察；不将静态可达或编译成功提前称作验收通过。下一步仅按统一入口执行一次整体 E2E，每次启动分别确认准备。
