# RE-10：Manager 非 Game 调用与音频阶段

日期：2026-11-12。状态：**J273 静态分类、前置状态与调度交接完成；未实施、未测试**。上级：[修改方案](../../client-display-delegation.md#6-新增逆向任务j273-静态反编译)。目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273；下文地址均为 VA，RVA＝VA−`0x400000`。

成果：

- [Manager 调用点分类](manager-call-sites.md)：按地址列全 50 个业务调用点，另列 3 个栈保护点；覆盖启动、输入、Online、菜单、相机、音频及光标，不只截取 Game::Update 后面的代码。
- [音频、前置状态与调度](audio-and-schedule.md)：两个音频入口的真实语义、ABI、读取需求、本地维护顺序和第 15 步覆盖。

本次先读本地 Wiki、REPENTOGON zhl／官方实现及已有研究，再静态反编译当前 J273 IDB。未运行游戏、调用目标函数、改目标内存／IDB、安装 Hook、生成或编译代码，也未运行测试；只交付研究文档，没有新增 wire／类型号／布局版本或保留临时取证包。

核定结论：

1. `Manager::Update 0x92E3E0` 内实际有 **53 个 call site**；去掉 3 个 security cookie 调用后为 50 个业务点，其中包括 Game 入口／查询。原任务“约 50 个尾部非 Game 调用”是概数，不能解释成 Game 返回后还有 50 个独立 helper。
2. `0x7D7DA0` 确为音乐更新，唯一静态 caller 是 `0x92EB49`。原生到达公共尾部时每次运行，**不受相机的 paused 条件限制**；处理 jingle、ducking、两组 Crossfade、pitch 和 Queue 接续。可以作为具名音频显示入口，前提是本地音乐 owner、资源和已同步的显示读取需求就绪。
3. **撤销“`0x908340` 疑为 SFX tick”的猜测**：SDK 明确声明它为 `SoundEffects::ClearVolumeModifier()`，用于按当前乘数及本机选项重新混音活动声音。Manager 只在有 Game、未暂停、乘数不等于 1 且恢复步长非 0 时，先推进乘数再调用它；它自身不递减 SFX 节流计时、不推进全体音效播放，也不把乘数重置为 1。
4. 第 13 步音频阶段须区分 **Host 当前 SFX 乘数**与 **Client 本地混音刷新**。建议随音频状态承接当前乘数、在安装变脏或本机 SFX 音量改变后调用 Clear；不把 Manager 的恢复生产段移到 Client，也不每帧无条件扫全部 sample。此为实现交接，不声称当前采集／布局已具备该字段。**审阅补充**：乘数可为负（玩法路径 `0x4A8CED` 写 -10.0），线上只拒 NaN／无穷；dirty 以 `max(m,0)` 变化为准。该乘数与红房位已登记入上级第 5／8 节需求清单，供第 4 步收缩布局。
5. 输入泵已由 `ClientPollInput → 0x9EC130` 承接，包含窗口事件、平台输入和 `InputManager::Update`；不能再补跑 Manager 中同一组三个调用。光标动画和闲置计时可作为本地维护；启动、设备重绑、原版 Online、共享弹窗回调、截图、完整控制台／暂停菜单、Cutscene／Nightmare 和 Game／Odd 路径继续拦截。
6. 音乐更新还直接读音乐种子效果 41—44、Q8 dizzy 和当前 RoomDescriptor.Flags bit `0x400`；这些是新增的具体消费方证据，不授权复制树指针、设备或运行种子／房间生成。读取需求与缺口见音频文档。

当前剩余工作均为第 8／10／13 步的状态需求、具名 adapter 与接线；未命名 helper 的业务名称、共享弹窗 callback 及原版 Online 子树不获得执行资格。第 15 步仍是唯一的统一整体验收，没有音频、输入或性能通过结论。
