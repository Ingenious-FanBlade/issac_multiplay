# Manager::Update 调用点分类

日期：2026-11-12。依据：J273 `0x92E3E0—0x92EC23` 指令、分支和被调用函数；SDK 名称优先，未声明入口保留地址名，不把反编译器猜测的 EBP／EDI 参数当作公开 ABI。仅静态分析，未实施／未测试。

## 1. 资料与分类含义

首要资料为 REPENTOGON `libzhl/functions/{Manager,Music,SoundEffects,Input,Console,Camera,MenuManager,Cutscene,NightmareScene,ANM2,OptionsConfig}.zhl`，以及本地 [MusicManager Wiki](F:/projects_isaac/IsaacDocs/docs/MusicManager.md)、[SFXManager Wiki](F:/projects_isaac/IsaacDocs/docs/SFXManager.md)。已有入口复用 [RE-1](../RE-1/README.md)、[tx-04-17 隔离研究](../../../tx-04-17/tx-04-17-update-isolation.md)及[生命周期／退出研究](../../../tx-04-17/tx-04-17-failure-retirement.md)。本轮用 zhl 特征字节与当前函数入口匹配名称，再读实际指令／消费者；未校验任何哈希。

下表的判定适用于 **C/S Client 活跃、Manager 原链被整体跳过时的自有显示根**，不是修改单机／Host 行为：

- **放行**：具名显示／音频子入口；按原生相对次序和下文前提调用。
- **本地**：保留本机 owner 或现有 C/S 路径；不代表将该原始 call site 原样搬入显示根。
- **拦截**：不由显示根调用。混合、Online、权限外持久写入和未核清 helper 均不因位于 Manager 内获得执行资格。

## 2. 全部业务 call site

共 50 行，按指令地址排序。Game 相关行也列出，以明确范围和调用顺序；其中 `IsPaused` 两行由 Q12 的同步位承接。

| # | call site → target（VA） | 来源／已核工作 | Client 判定与理由 |
| --- | --- | --- | --- |
| 1 | `92E46E → 932A20` | SDK `Manager::ExecuteStartGame` | **拦截**：新局、恢复、资源／世界生命周期，不是显示维护 |
| 2 | `92E475 → 933060` | 菜单请求转换 helper；cleanup、state=1、必要时创建 MenuManager，前段含 NetplayManager | **拦截**：既有研究已识别混合生命周期及 Online；不从函数中部进入 |
| 3 | `92E495 → 932970` | SDK `Manager::cleanup_current_state` | **拦截**：清理当前 Game／菜单／演出 owner |
| 4 | `92E4A6 → 937BB0` | SDK `Cutscene::Show`，this=`Manager+20C48` | **拦截**：Manager 状态切换前的演出启动，不是 Client 游戏内纯显示根 |
| 5 | `92E4E0 → 8FBE00` | SDK `NightmareScene::Show`，this=`Manager+21498` | **拦截**：原生 Nightmare 启动／状态切换 |
| 6 | `92E57C → 933060` | Cutscene 完成后的菜单转换，同 #2 | **拦截**：不复制完成后的菜单／启动请求 |
| 7 | `92E599 → 92E260` | 枚举原生设备，选设备 id，条件调用 `79E1B0` 重绑 Player | **拦截**：设备归属／Player 绑定由 C/S 统一 owner 维护 |
| 8 | `92E5A7 → 9E35B0` | ARGB `FF000000` 转颜色，用于该完成分支内联 fade 写入 | **拦截**：所在原生完成分支不执行；Game fade 取 Q8 当前值，不补演 |
| 9 | `92E633 → 9E0DF0` | SDK KAGE 日志，`Cutscene End` | **本地**：诊断而非显示依赖，不为补一条日志执行 Cutscene 完成链 |
| 10 | `92E650 → 7D8480` | SDK `Music::Crossfade(63,0.08)`，完成后 state=1 时触发 | **拦截**：这是选曲请求生产；由 Host 音乐请求／本机合法菜单 owner 决定，不能与尾部音乐 tick 混同 |
| 11 | `92E672 → A2F820` | 平台未初始化的错误分支；外层还要求 Update 参数为 true | **本地**：原始错误报告不是必调的 tick；缺平台资格时不伪造初始化 |
| 12 | `92E67C → A2A390` | Win32 窗口消息与按键事件泵，`PeekMessage/DispatchMessage` | **本地**：现有 `ClientPollInput → 9EC130` 已调用；不重复抽取事件 |
| 13 | `92E681 → A31960` | 平台输入／窗口事件缓存整理 | **本地**：由同一 `9EC130` 输入泵执行一次，不随新增显示批次重复 |
| 14 | `92E68B → 9EDCC0` | InputManager 更新，this=`BCE438`；包含设备更新、失效移除／通知 | **本地**：已在 `9EC130`；沿用统一输入与失效处理，不另建物理采样旁路 |
| 15 | `92E692 → 9346B0` | 设备断连／重新选择、Player 设备写入、Online 本地设备归属等 | **拦截**：混合输入绑定／提示；不是“补输入泵”所需的一部分 |
| 16 | `92E69D → 999990` | this=`Manager+4CD50`；遍历共享弹窗队列，推进 `9975E0`，销毁完成项 | **拦截**：子更新查询按键、超时提交，并在 `997884` 调任意登记 callback；不能按纯 UI 放行整个队列 |
| 17 | `92E6AC → 41FC20` | 依 Manager state／Player／Netplay 选控制设备的 helper | **拦截**：不为两槽提示位置维护读取原版 Online／设备身份 |
| 18 | `92E702 → 41FB30` | 两槽循环中的提示坐标计算；读取 `Manager+4A64C` 附近的本地布局值 | **本地**：纯本机提示布局，无 world 状态输出；当前根不重建该控制器提示，不单独补调 |
| 19 | `92E736 → 92E030` | 全局热键；含音乐／全屏设置，也含 Steam 成就读取和 PGD 写入／保存路径 | **拦截**：不能因其中有本地音量键而整体放行；本机选项走已授权 UI |
| 20 | `92E742 → 9EEC40` | SDK 名 `InputManager::GetDeviceType(0)`，该处实际将返回值作为设备对象 | **拦截**：此处只为原生截图热键获取设备，不增加输入采集入口 |
| 21 | `92E757 → EAX` | 上述设备 vtable `+18h`，查询键 298（F9） | **拦截**：截图分支；保留间接目标语义，不伪造一个固定函数地址 |
| 22 | `92E76C → 41B800` | 格式化本机 `screenshots` 路径 | **拦截**：截图写文件前段，不是 world 显示必需 |
| 23 | `92E77B → [CreateDirectoryA]` | 创建截图目录 | **拦截**：不会因显示根重放而创建目录 |
| 24 | `92E786 → 9BBDA0` | 读取本机 framebuffer 并写 PNG／文件名冲突处理 | **拦截**：截图有持久副作用，任务没有要求恢复此快捷键 |
| 25 | `92E793 → 8E75A0` | this=`Manager+4AFB8` NetplayManager；Online 请求／延迟启动／run status 等 | **拦截**：C/S 与原版 Online 隔离，不调用来取得其布尔结果 |
| 26 | `92E7B9 → 686850` | SDK `Console::ProcessInput` | **拦截**：可执行命令；Client 输入阻断读取现有 Console 状态，不消费原生玩法命令 |
| 27 | `92E7CA → 687020` | Console 显示计数、滑入／滑出；还读 Netplay 列表 | **拦截**：已有 `ClientUiBlocked` 只读路径不补调该 helper；如需原生控制台动画另作具名拆分 |
| 28 | `92E7D1 → 6F58E0` | Game 的 UI／fade／transition 窄条件查询 | **拦截**：只决定 Online 早退路径是否运行暂停菜单；不作为 Client 终结分支选择器 |
| 29 | `92E7E4 → 98CCC0` | PauseScreen 输入，this=`Game+2363C`；含退出／重启／设置／Online 路由 | **拦截**：Host 同步 paused 不授权 Client 消费原生暂停菜单玩法操作 |
| 30 | `92E7F5 → 98D4E0` | PauseScreen 更新；还可能扫描／Remove 实体、改 HUD 请求及菜单 owner | **拦截**：不整体当成纯动画更新 |
| 31 | `92E845 → 90B780` | SDK `AchievementOverlay::Update` | **拦截**：本机成就提示／弹出 owner，不是同步游戏 HUD；没有本次额外本机成就展示需求 |
| 32 | `92E88B → 999E10` | 带锁读取共享弹窗活动状态，this=`Manager+4CD50` | **本地**：本机 UI 阻断交 C/S UI owner 合并；不复制 Host 弹窗队列或用它重算 Q12 分支 |
| 33 | `92E896 → 929E50` | NetplayManager 两个 modal 状态的非零／active 查询 | **拦截**：不触及原版 Online，C/S 不执行其谓词 |
| 34 | `92E89F → 6F66E0` | SDK `Game::ProcessInput` | **拦截**：玩法输入；真实输入走统一 C/S scheduler |
| 35 | `92E911 → 6F6D30` | SDK `Game::Update`，M 偶数；之前内联 staged 位置恢复 | **拦截**：由具名显示批次替代；不补跑 staged 位置恢复 |
| 36 | `92E921 → 6F9340` | M 奇数的 Odd helper | **拦截**：含 Player 半帧玩法，不用它补相机或音频 |
| 37 | `92E94A → 6F92A0` | SDK `Game::IsPaused`，相机条件 | **本地**：Q12 已同步的 paused 位；MINECART 例外读 Q11 的 mode／animation |
| 38 | `92E980 → 91E5E0` | SDK `Camera::UpdateDrag2` | **放行**：本机 CameraStyle=2，沿用 RE-1 资格／override 清理 |
| 39 | `92E998 → 91D840` | SDK `Camera::UpdateUltrasmooth(Mpre&1)` | **放行**：CameraStyle!=2，与 #38 二选一 |
| 40 | `92EA8C → 999E10` | state=1 的共享弹窗活动查询，同 #32 | **本地**：本机菜单／UI owner，不补跑完整 Manager 菜单分派 |
| 41 | `92EAA7 → 929E50` | state=1 的 Online modal 查询，同 #33 | **拦截**：不接入原版 Online |
| 42 | `92EAB0 → 961CB0` | SDK `MenuManager::Update` | **拦截**：Client 活跃时不执行完整原生菜单输入／启动路由；C/S UI 保留独立 owner |
| 43 | `92EABD → 9373D0` | state=3 的 Cutscene 更新；含 Online 等待和完成处理 | **拦截**：不是 Client 游戏内的纯显示子入口 |
| 44 | `92EACA → 8FA880` | state=5 的 NightmareScene 更新 | **拦截**：更新独立演出及完成标志；不伪造 Manager state 切入 |
| 45 | `92EADB → 6F92A0` | 公共尾部的 `Game::IsPaused`，SFX 恢复条件 | **本地**：Q12 同步位；与相机 MINECART 例外不同，SFX 无该例外 |
| 46 | `92EB3E → 908340` | SDK `SoundEffects::ClearVolumeModifier` | **本地**：条件性音频混音刷新；不是无条件 SFX tick，详见[音频合同](audio-and-schedule.md#2-sfx不是通用逐帧-tick) |
| 47 | `92EB49 → 7D7DA0` | 音乐 tick，this=`Manager+29DF8` | **放行**：每次到达音频尾部一次；paused 不单独阻断，前置与 Queue scope 见音频合同 |
| 48 | `92EB63 → 9F26E0` | 本机窗口鼠标键 0 查询；可能清鼠标过渡态 3 | **本地**：仅本机开启 mouse control 时用于光标；不是纯只读接口，不重复查询／先于既有输入 owner 消费 |
| 49 | `92EB80 → 405D00` | SDK `ANM2::Play(Clicked/Idle,false)`，this=`Manager+4AE88` | **本地**：光标资源有效时更新；不发送 Host 光标动画／鼠标位置 |
| 50 | `92EB9A → A2F820` | 光标路径的平台未初始化错误 | **本地**：缺有效本机窗口时停止该维护，不为进入成功路径调用错误入口 |

栈保护点为 `92E452`、`92E805`、`92EC1A → A86BBB`。它们属于编译器函数收尾，不是显示业务／调度入口，不从外层单独调用。

## 3. 没有 call 的相邻工作

仅列 call 会漏掉以下内联段；本轮一并区分，不增加通用 Manager 放行：

| 原生范围 | 工作 | 交接 |
| --- | --- | --- |
| `92E45D—92E469`、`92E4AE—92E655` | Manager state／完成请求、fade 写入等 | 生命周期／选路留原 owner；Game 显示标量按 Q8 安装，不复制整个控制段 |
| `92E6D0—92E715` | 两个本机提示槽的当前位置向目标位置靠拢，最大步长 6；随后坐标 helper | 本机提示布局，与 #17／18 一起不作为 world 必需根；不由此恢复设备／Online 提示 |
| `92E71B` | 清本机 `Manager+4A8C1` | 不能从后半截复制一条清零指令，否则破坏本机 A／B 控制与 M 保持语义；唯一 M owner 统一处理，两个字节的具体业务名称仍不猜测 |
| `92E8C1—92E90A` | staged Entity 位置恢复并清标志 | 继续拦截，最终姿态由 world 安装，不以画面平滑为由执行 |
| `92E9A4—92EA7E` | 四槽 `Game+1BAB4`、stride `14h` 的环形索引平滑，写各槽 `+Ch`；环长来自 `Game+1BB18/+1BB1C` vector | 属 PlayerManager 区域；本轮未核定其全部绘制用途，不加逐帧根、不复制整个 PlayerManager。未知语义照实保留，若第 15 步出现该 UI 差异再补需求；不可脱离有效非零环长搬运该循环 |
| `92EAE4—92EB39` | SFX 乘数按 `+14h` 步长向 1 恢复 | **Host 生产**与本地 Clear 刷新分开，不把它称为 Clear 函数自身行为 |
| `92EBB0—92EC01` | 本机鼠标位置／闲置计数 `Manager+4AF9C/+4AFA0/+4AFA4`；移动置 300，不动且计数正则减一 | 本地光标维护，按 M 周期一次；不当作网络帧／Host 状态 |
| `92EC09` | 本机 M 加一 | 唯一显示 owner；只在本次实际到达该尾部时一次，不由音乐 helper 或 Render 再加 |

## 4. 分支不能合并成一个 paused

1. 插值关闭且 Mpre 为奇数的入口快返，在所有上述业务调用之前；M 的增量／保持见 [RE-1 真值表](../RE-1/manager-render-cadence.md)。
2. `92E793` Online 更新返回 false、Manager state 为 2／5 且有 Game 时，原生经 Console／条件 PauseScreen 后在 `92E80D` 或 `92EC0F` 返回，既不到相机／音频，也不递增 M。**C/S 不调用 Online Update 来重演这一分支**，不读取其状态决定是否推进显示。
3. `92E816—92E826` 再检查本机 `A=Manager+4A8C0`、`B=+4A8C1`；A 非零且 B 为零时普通路径也直接到 `92EC0F`，不运行 state 分派／相机／音频，也不递增 M。RE-1 关于入口奇数快返的保持例外不等于这里只有那一处保持；第 13 步须在唯一 M owner 内处理这个本机控制条件，不搬入 `92E71B` 或热键链来推算它。
4. 能到公共尾部时，相机自身受 paused／MINECART 条件控制；SFX 恢复仅要求未暂停，没有 MINECART 例外；音乐无 paused 判断。游戏终结分支提前 return 不意味着音乐 tick 要停。

主循环随后仍调用 Manager::Render；绘制资格取 Mpost 与本机插值设置。上述静态分支不授权 Client 再调用启动、Online、Console 输入或额外 Game::Render。
