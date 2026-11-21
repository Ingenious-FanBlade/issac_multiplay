# RE-10：音频入口、读取需求与显示根调度

日期：2026-11-12。`STATIC` 为 J273 指令／本地源码事实，`DESIGN` 为第 8／10／13 步的接线交接；都没有运行验证。VA 基址 `0x400000`。

## 1. 首要资料与 ABI

- 原版 Wiki：[MusicManager](F:/projects_isaac/IsaacDocs/docs/MusicManager.md)、[SFXManager](F:/projects_isaac/IsaacDocs/docs/SFXManager.md)、[SeedEffect](F:/projects_isaac/IsaacDocs/docs/enums/SeedEffect.md)、[RoomDescriptor 枚举](F:/projects_isaac/IsaacDocs/docs/enums/RoomDescriptor.md)。Wiki 对 FrameDelay 明确说明为再次播放间隔；音乐 API 的 UpdateVolume 是应用本机选项，不是本轮找到的总 tick。
- SDK：[Music.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Music.zhl)、[SoundEffects.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/SoundEffects.zhl)、[Manager.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Manager.zhl)、[OptionsConfig.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/OptionsConfig.zhl)。`repentogon/LuaInterfaces/LuaOptions.cpp` 的 `Lua_Options_Fix_SetSFXVolume` 在修改本机音量后调用 ClearVolumeModifier，正面支持它是混音刷新而非播放 tick。
- 既有[音频来源](../../presentation-audio-sources.md)、[RE-8 入口](../RE-8/sources-and-payloads.md)的对象／sample／voice 事实继续复用；旧“Client 不运行完整 Music Update／只同步最终混音”的路线已被显示委托取代，不能反过来阻止当前任务。尚未实施的选样本／恢复能力也没有因此自动完成。

| 入口 VA／RVA | 本轮核定 ABI | 指令依据 |
| --- | --- | --- |
| `7D7DA0 / 3D7DA0`，音乐 tick；SDK 未声明 | 无显式参数，ECX=this=`Manager+29DF8`；外层可用 `void __thiscall(Music*)` 语义，EAX 返回不消费 | caller `92EB43 lea ecx,[edi+29DF8] → 92EB49 call`，无 push；尾 `7D834A—7D8350`恢复对齐栈并裸 `retn`。入口 `53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55` 是动态对齐序言，伪码的 EBP 参数不是第二个业务参数 |
| `908340 / 508340`，ClearVolumeModifier | SDK `__thiscall void SoundEffects::ClearVolumeModifier()`；ECX=this=`Manager+2A160`，无栈参数 | 入口 `55 8B EC 83 EC 0C 8B C1 33 C9` 与 zhl 声明匹配；`908469 retn`。伪码 EAX 的遍历计数不升级为成功返回值 |
| `9EC130 / 5EC130`，既有输入泵 | 复用 `ClientPollInput` 已有 wrapper／offset，不另建 ABI | 依平台资格调用 `A2A390` 或报错、随后 `A31960 → 9EDCC0`；尾转入同一 InputManager owner |

## 2. SFX：不是通用逐帧 tick

`STATIC`：Manager 原始次序为：

```text
92EAD7  有 Game？否则直接去音乐
92EADB  Game::IsPaused；paused=true 直接去音乐
92EAE4  modifier = SoundEffects+10h
92EB01  modifier == 1 → 去音乐
92EB03  step = SoundEffects+14h
92EB12  step == 0 → 去音乐
92EB14—92EB39  |modifier-1| <= step 时置 1；否则朝 1 加／减 step
92EB3E  ClearVolumeModifier(this=Manager+2A160)
92EB49  Music tick(this=Manager+29DF8)
```

这些等值说明针对正常有限值；原指令为 SSE 浮点比较，不能把 NaN 当作合法音频输入。构造的 `+14h` 初值为 `0.005`，不表示每次实际都用这个值；更不在 Client 遇到未知来源时补默认步长。

**乘数取值范围（2026-11-12 审阅补充，`STATIC`）**：`0x908340` 共 8 个静态 caller，除 Manager 外：

| caller | 所在函数 | 对乘数／音量的写入 |
| --- | --- | --- |
| `41CB23`、`41CC26`、`9329C7` | `sub_41CAE0`、`sub_41CB40`、`cleanup_current_state` | 乘数≠1 时直接写 1.0 再 Clear（Host 生命周期重置） |
| `4A8CF7` | `sub_4A7380`（唯一 caller `sub_6BEB50`，玩法路径） | 乘数≠-10 时写 **-10.0**（`0xC1200000`，比较常量 `B40BE0`）再 Clear；同段先 `Music::Crossfade(48,0.5)` |
| `87D109`、`98A180` | `sub_87D0C0`、`sub_989970`（暂停菜单选项路径） | 写本机 Options.sfxVolume `Manager+2A194` 后 Clear |
| `8FE7CE` | `sub_8FE470`（读 `SFXVolume` 配置） | 本机配置载入后 Clear |

因此 Host 当前乘数是**可为负的有限 f32**：-10 后按步长（初值 0.005）每个 Manager 尾部周期恢复，约 2000 周期仍 ≤0（Clear 按 `max(m,0)` 混成静音），之后才淡入到 1。线上校验只拒 NaN／无穷，不得限制到 [0,1] 或把负值改为 0 后再安装（否则 Play／AdjustVolume 等其他读取方看到的值与 Host 不同）。恢复期间 Host 值逐帧变化，该字段几乎每个 world 帧都会被增量检测为变化，属预期开销。


Clear 本体遍历 `SoundEffect` stride `1B8h`，每项样本数 `+190h`、请求音量 `+194h`、状态字节 `+198h`；样本 stride `38h`。有本地 source 时查询后端 active，可能清 sample 的 pending `+29h`，再为 active／pending 的 source 写：

```text
sample.backendVolume(+2Ch)
    = max(SoundEffects.modifier(+10h), 0)
      * SoundEffect.requestVolume(+194h)
      * 本机 Options.sfxVolume(Manager+2A194h)
```

`9083D4—908416` 支持该公式；本路径没有 Play／AdjustVolume 中的显式上限 1 clamp。它不把 modifier 清 1，不调 Play／Stop，不改 FrameDelay 截止字段，不做全体音效时间推进。后端 active／pending 和本机设备成败只属于本机，不变成 Host world Stop。

**DESIGN：第 8／10／13 步交接**：

- 补入“当前 SFX 全局乘数”的具体需求，消费方为上述 Clear 及原生 Play／AdjustVolume；源为 Host 同一正常主线程音频状态，不复制 `vector`／source／设备。原生恢复步长和 writer 历史继续留 Host，Client 不运行 `92EAE4—92EB39` 的恢复生产。
- world／音频状态安装使 `max(m,0)` 改变时置本地混音 dirty（负值区间内 m 变化但 `max(m,0)` 仍为 0 时 Clear 结果不变，不必逐帧扫描 sample）；本机 Options.sfxVolume 改变也置 dirty。Client 拦截了暂停菜单（`sub_989970`），C/S 自有 UI 改本机音量时由它负责置 dirty 或经官方 setter 刷新。显示根的 SFX 维护阶段在资格和 owner 有效时消费 dirty 调 Clear，一次即可；已经由官方 setter 完成的刷新要合并，不再按每个 Peer／Render 重复扫描。需要恢复全量循环声音时仍按 RE-8 的逻辑实例集合处理。
- 乘数先于本批回放 SFX／本地显示声音安装，使新声音直接用正确乘数；已有声音的 dirty 刷新保持在原生相机之后、音乐之前。没有新 world 不代表要重置乘数；暂停时收到新权威值仍须安装，不能只依赖原生“未暂停恢复”条件发现变化。
- **不要求无条件调用 Clear**。原任务“两个音频 tick 每帧运行”的意图落实为每个有效尾部周期调度音频阶段，其中音乐按周期更新、Clear 按脏变化刷新；不虚构一个必须递减的 SFX tick。

**FrameDelay 的补充限制**：`907D24—907D5A` 的 `now` 是本机 `Manager._framecount >> 1`，并非 Host game_frame；截止字段不会靠 Clear 递减。Q12 后回放仍传 FrameDelay=0，但它只影响**本次通过资格后写入的新截止值**，不会绕过调用前已有的 `now < next && next >= 0`。若同 id 先被本地显示路径设过非零截止，回放仍可能被吞。第 13 步应让 replay scope 在已验证合法 id／资源的前提下窄绕过这一已有节流判定，或由已核非随机执行 leaf 消费事件；不全局清 sound 截止缓存，不把普通本地播放一并取消节流。此项是静态更正，具体 shim 尚未实施。

## 3. 音乐 tick 的工作及前置状态

`STATIC`：`7D7DA0` 只有 `92EB49` 一个静态调用者。可达公共尾部时无 Game 也能更新本机 jingle／流组；有有效 Game 时才读取种子效果等。它不调用 Game／Room／Entity Update、生成／存档／解锁，也没有原版 Online owner 读取；其中 Music::Play 的官方 Lua callback 仍按 Q6 放行，本项目 Client 不注册玩法 callback。

原生内部顺序：

1. `7D7DC9` 将本机 SFX 音量放入全局临时混音量 `BE49F8`；`7D7DDA → 7D7B90(Music+32C,true)` 更新 jingle 流。
2. `7D7DDF—7D7E82` 维护 jingle countdown `+364`、背景 ducking `+360`；活跃且 countdown>0 时趋向 0.1，普通恢复趋向 1，保留 countdown=-100 的特殊原生分支。不是把 jingle 自身 gain 与背景 ducking 重复相乘到网络最终 gain 上。
3. `7D7E78—7D7EAD` 按 Music.enabled `+328` 和 Manager state 生成背景总乘数；`7D7EBE` 两次调用 `7D5E90`，流组为 `Music+4`、`Music+188`，stride `184h`。两组各有主流和最多六层，推进组／层淡化、active／pending 与旧组退休；不再从 Host 发送这套本地混音结果反向覆盖它。
4. `7D7EED—7D7FE8` 查种子效果 42、41、43、44，分别将当前 pitch 置为 0.15、0.75、1.25、2.5；多个效果同时存在时按这个原生优先级。
5. Manager state=2 时，`7D7FFA` 读 dizzy，`7D810A—7D811B` 读当前 RoomDescriptor.Flags `&0x400`，结合本机 M 的正弦相位产生音高变化；随后 `7D8202—7D828D` 按 targetPitch `+318` 平滑并向两组／jingle 的本地 source 提交 pitch。
6. `7D8296—7D833E` 当前组不再 active 且 queued 非零时检查合法 entry，`7D8303 → Music::Play(queued,0)`，再直接设置当前组 gain=0、target=本机 enabled／musicVolume、fadeRate=0.08，最后清 queued。Queue 接续是本地播放维护，不重新执行房间选曲。

**所需读取与安装顺序**：

| 读取需求 | 已核消费方 | 第 8／10／13 步处理 |
| --- | --- | --- |
| 本机 Music owner、两组／jingle 强引用及曲目配置、queued／enabled／pitch／targetPitch 等实际播放控制 | 上述全部；SDK Music 大小 `368h` | 使用本机正常构造与资源配置；请求带代次，变更时操作，未变包不重复 Play；jingle／layer 等控制不能因只传曲目 id 而丢失。不 memcpy Host source、vector 或整块 Music |
| 本机 Music／SFX Options | `7D7DBE`、`7D82C0` 及各请求 setter | 保留本机音量；Host 原参数若已混入 Host Options，要从正常请求来源取得逻辑参数，不能反除 Host 音量 0 |
| 音乐种子效果 41—44 的存在性 | `7D7EED—7D7FE8`，读取 `Game+1BBD8` 的原生树 | 追加到 Run 的显示需求；这是 Seeds `Game+1BB84` 内 `+54h` 的树 owner，不能复制 Host 树根地址。与其他已核种子显示需求统一物化最小本地集合；不运行 SetStartSeed／世界生成。当前收缩后的实际安装未接通 |
| 当前 dizzy `Game+669A0` | `7D7FFA`、`7D80C8` | Q8 已要求同步当前值；不在 Client 运行 dizzy target／speed 生产 |
| 当前 `Room+4 → RoomDescriptor.Flags+44h` bit `0x400` | `7D810A—7D811B` | Wiki 名为 `FLAG_RED_ROOM`；按现有 descriptor Flags 的需求承接该位，同一 world 提供有效 Room／descriptor，不能改读 Data 或 OverrideData 的 flags |
| 本机 Manager state、Mpre | `7D7E96`、`7D7FEB`、`7D800F/7D8124` | 使用本机 state／M；不伪造 state=3 保持音乐音量，也不借 Host Manager 帧。声音相位因 M 不同的差异属本地表现，不据此运行 Host 玩法 |

`BE49F8` 是内部短时混音量，先用于 jingle 的 SFX 类别、再用于背景；不是网络状态，也不能在它之外再乘一次旧 Host 最终混音。`7D7DA0` 对 Seeds 的查找是内联原生树读取，只挂 `Seeds::HasSeedEffect` getter 无法覆盖它。树／资源没有正常 owner 时不调用，具体构造／退休交实际安装器；不拿空树或默认 enabled 冒充已满足需求。

Queue 的内部 `Music::Play` 要带具名本地音频维护 scope：不生成新的 Host 网络事件，不每包重复调用；新的 Host 音乐请求仍以请求代次为准。两个设备自然播完的时点可以不同，不反向修改 Host 播放代次或 world；切房／重连依据当前请求收敛，不声称恢复精确硬件游标。

## 4. Client 调度交接

以下是 **DESIGN，未接线**，在 [RE-1／RE-3／RE-4](../RE-1/manager-render-cadence.md) 的 Mpre／Mpost 合同上补充，不能通过调完整 Manager 实现：

1. 进入已有主线程 Manager 边界，完成可提交的 world／资源／库存／注入／F／Q8／Q11／Q12／音频状态安装；安装失败或原生 owner 已退休则不进入其显示／音频调用。
2. 保留现有 `ClientPollInput` 的唯一调用和 canonical 采样／阻断，不再加 `A2A390/A31960/9EDCC0` 第二组。它是现有本地输入维护，不因显示快返重复或停止接收松键；本轮不改它的实现位置。
3. 用 Mpre 和本机插值／控制字节判显示推进资格：插值关闭的奇数快返，以及普通路径的 A／B 保持条件均要处理（[分支](manager-call-sites.md#4-分支不能合并成一个-paused)）。正常的 Host paused 不等于本机 Manager 尾部停止。
4. 合格偶数周期运行 Q12 所选 30Hz 批次；奇数周期不运行 Odd／staged 位置恢复。
5. 相机按 Q12 paused、Q11 MINECART 和本机 CameraStyle 二选一；仍依 RE-1 清 override。**相机被 paused 跳过，不跳过其后的音乐**。
6. SFX 维护：消费已安装乘数／本机 Options 的 dirty，具名调用 Clear；不运行 Host 恢复生产段。随后 **音乐 tick 一次**，使用仍未加一的 Mpre。世界 F 相同也不以到包次数跳过音乐；跨过多帧也不按 ΔF 补跑。
7. 本机 mouse control 开启且窗口／cursor owner 有效时维护光标 Clicked／Idle 与闲置计数。只按本机鼠标，不走 Host canonical；查询可能消费鼠标过渡态，须置于现有输入 owner 之后并保持一次。缺本地光标资源时不为它调用 Manager 资源／启动链。
8. 本机 M 由唯一 owner 推进／保持一次，再交原生 Manager::Render 按 Mpost 判断；不增加额外 Game::Render。会话停止后各 wrapper 透传，不能让原链与自有音频阶段同周期各执行一次。

默认不运行完整 Console／PauseScreen／共享弹窗队列。若第 13 步决定增加本机菜单，需继续使用现有自有 UI 或明确的显示拆分；不因这里分类“本地”而放行任意登记 callback、全局热键保存或原版 Online。

## 5. 第 15 步覆盖与未决

仅补统一验收内容，不现在执行：

- **E36**：50 点分类实际接线；原链整体不运行；窗口／输入只泵一次、旧输入及时中性；本机 A／B 保持条件、插值开关、Mpre 音频／Mpost Render 相位和停用恢复没有双增／双调；不进入 Online、启动、重绑、Console 命令、共享回调和截图文件写入。
- **E37**：Play／同曲重播／Crossfade／Queue／jingle／layer、暂停、网络空窗、切房／重连下继续与收敛；SFX 乘数恢复随 Host 当前值收敛、暂停时更新及本机音量变化刷新、最终回到 1 不残留静音；不每个 Render 扫全部 sample。
- **E37**：音乐种子 41—44、dizzy、红房标志及本地音量 0；FrameDelay=0 的同 id 批量事件，尤其前一条本地显示 Play 留下非零 next 时的回放，不能只验“本机截止原本为 0”。本地设备失败不使所有 Peer 发生 world Stop；jingle 仍按本机 SFX 类别混音。
- **E28／性能**：保留本地混音／cursor／backend pending 的职责，不作为 Host world 漂移反复覆盖；音频扫描、资源及当前乘数安装的开销计入原预算。

未知和未实现如实保留：四槽环形平滑的全部绘制用途、本机 UI 阻断合并、音频需求的正式 schema／采集／本地树及资源物化、回放节流窄绕过、sample 多实例适配、必要段落／重连恢复和真实后端行为。上述不是新增静态闭合任务或测试前置门；当前调用点已可按放行／拦截／本地落实第 13 步，游戏、音频效果及性能是否正确仍由第 15 步验收。

## 6. 2026-11-21 修复：Crossfade 速率（静态分析，仅编译，未测试）

Host `BinaryDisplayCapture::BootstrapMusic` 原把 Crossfade(id,faderate) 的速率写进 `parameter`、`fade` 留 0，Client 按 `Crossfade(id,fade)` 以速率0交叉淡化：旧曲不淡出、新曲不淡入，表现为 Client 从标题进局后仍是标题曲。现 Crossfade 的速率写入 `fade`（`parameter` 固定 1，Client 不使用）。Play／Fadein／Fadeout／Queue 接续的参数位不变。

