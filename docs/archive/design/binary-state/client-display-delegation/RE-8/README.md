# RE-8：表现事件、状态与音效来源

日期：2026-11-12。状态：**J273 静态来源、载荷语义与播放交接完成；未实施、未测试**。依据[方案 6.4](../../client-display-delegation.md#64-事件与状态re-8)，先查本地 Wiki／SDK／官方实现，再复读当前 J273 IDB。VA 基址 `0x400000`，RVA＝VA−基址。

**同日 RE-10 后续完成／更正**：[音频调度](../RE-10/audio-and-schedule.md)确认 `0x908340` 是 SDK ClearVolumeModifier，非通用 SFX tick；音乐 tick 不受相机 paused 条件限制。下方审阅标注中“FrameDelay=0 不会被本机再次节流”只覆盖调用前未留非零截止的情况；`now=M>>1`，已有同 id 本地声音留下的 `next` 仍可先阻断，replay scope 还需窄绕过／执行 leaf。保留原始结论及修订经过，不将尚未实施的 shim 当作已完成。

成果：

- [采集／播放入口与载荷](sources-and-payloads.md)：SFX、循环声、音乐请求、横幅／fortune、ItemOverlay 与 Boss 开场。
- [Q7 来源作用域与已核音效点](audio-scopes.md)：同一放行表、混合路径暂停、C/S 回放标志，以及 Render 静态检查的实际范围。

本次未运行游戏、调用原生函数、改目标内存／IDB、安装 Hook、生成或编译代码，也未执行测试。线格式和运行接线仍待第 8—13 步，第15步唯一整体验收；旧的选 sample／逐 voice 音频研究保留，不把它们的未实现部分宣称已完成。

核定结论：

1. SFX Play=`0x907C60`，SDK `__thiscall void(int,float,int,bool,float,float)`，this 在 ECX、六个栈参数、`retn 0x18`。FrameDelay 是再次播放的间隔，不是启动延迟；官方 POST 回调也不证明声音实际启动。
2. 音乐 Play=`0x7D8360`、Crossfade=`0x7D8480`，还须覆盖 Queue=`0x7D8A80` 与正常 queued 消费。请求带递增版本／播放代次，不能只比较曲目 id，否则同曲重播丢失或每包重启。
3. 横幅优先捕获 item 身份与原始本地化键，fortune 捕获选定类别／索引和原始行。`HUD::ShowFortuneText 0x97BEE0` 消费 **8B MSVC list 头＋32B 字符串节点**，不是 vector 或 `char**`；必须使用兼容 owner 构造／清理，不能把网络数组强转进去。
4. ItemOverlay 按状态同步；除 id、state、delay、Player 关系和主 Sprite，还须承接 id45 的 MegaMush Player Sprite。以演出代次判断是否先 Show，不能只按 id 变化；Show 的 Player 槽有原生引用登记。
5. Boss 开场 J273 原生入口是 `0x823D10`，两个 u32 参数。isaac-lan 的“8 个 u32”不是 J273 ABI。该函数会选音乐、清环境声并写 Game 显示标记，必须在具名回放范围内隔离相应子调用；完成分支也不是纯动画，见 [RE-9](../RE-9/README.md)。
6. `RoomTransition::Update` 和其启动函数均为混合路径，不能整根标为显示后屏蔽其中所有 SFX。Q7 按 Client **实际执行的子路径**判定，玩法／被替换子路径暂停显示作用域；回放标志是 C/S 自有线程内状态，不给原生 Play 添加参数。

循环声多实例、world Stop 与本机设备失效、状态恢复、文字资源寿命及转场中断均有明确的实现限制，见两份成果；尚无 E37／E39 或性能通过结论。

> **2026-11-12 审阅后标注**（IDA J273 复核，原文保留）：
>
> - 结论 5 的“Game 显示标记” `Game+0x22E90`（`0x823DA9` 清 0）是 HUD `_isVisible`（`HUD.zhl:66`），按 Q8 同步 Host 当前值；BossReplay 的本地写入在下一次安装时被覆盖，只需隔离选曲 `0x823D8C` 与环境声清除 `0x823D9D`。
> - 结论 6：**Q11（用户决定）**后 Client 不运行 RoomTransition Start／Update 的任何部分，转场内三个声音点（608／609、sound 1、sound 214）全部作为 Host 事件。
> - 结论 1 补充：回放事件调用 Play 时 **FrameDelay 传 0**。节流 `0x907D2F—0x907D5A` 仅在 `now < next` 且 `next ≥ 0` 时跳过，播放后 `next = now + FrameDelay`；Host 已节流过，传 0 不会被本机再次节流。
> - **RE-10（新开）**：Client 跳过整个 Manager::Update，音乐逐帧 tick `0x7D7DA0`（唯一调用者 `0x92EB49`）与 `0x908340`（`0x92EB3E`，疑为 SFX tick）也不运行；须在显示推进根中运行，否则音乐淡入淡出／Queue 与 SFX 状态不前进。见方案第 6 节。
> - 第 15 步交接中的失败方式已并入方案第 12 节第 64—65 条。
