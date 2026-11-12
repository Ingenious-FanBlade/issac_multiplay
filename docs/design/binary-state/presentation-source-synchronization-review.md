# 第8步：资源、屏幕／转场和音频来源的同步职责复核

**2026-11-12 HUD代码迁移，版本70**：现行HUD布局70及纯值模块已落地；原始来源、字节、UNKNOWN与历史决定保留，不据此宣称正常来源／原生绘制已经接通。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-11 当前决定优先级修订**：[资源本地生成合同](local-media-presentation.md)覆盖下方旧“程序内容精确传送”“文字图／Host前后surface／反馈图必需”判定。默认传引用、文字、有限描述与音频控制，Client直接切房、hallucination使用本端历史。旧53项表及JSON原条目保留为2026-11-10决定与证据；机器目录附current_policy_override，不能以旧投影恢复原始媒体。其他最终结果／事件、唯一owner和两序列合同保持。

2026-11-10；目标仍为官方REPENTOGON 1.1.3／J273 Win32。本文复核[资源／屏幕来源](presentation-resource-screen-sources.md)23项与[音频来源](presentation-audio-sources.md)30项，逐来源机器决定见[JSON](presentation-source-synchronization-review.json)。**来源已核不等于该来源的全部字段都应下行。** 本轮只修订同步职责及设计账本，不改生产`layout_version=57`，不构建或测试；原生字节、指令和UNKNOWN不因裁剪而消失。

本轮采用当前结果adapter：屏幕／转场消费具名有序绘制结果，音频消费每个逻辑voice的最终参数。**生产输入与最终结果只保留一份网络表达。** 以后若改为参数化adapter，必须说明其具体消费者和替代关系，不能将两条路线同时列成必发字段。

## 1. 共同判定规则

| 类别 | 判定 | 例子 |
|---|---|---|
| 当前world表现状态 | 影响共享内容且不能由同批已下行数据唯一派生，需同步 | 当前surface引用、有效pass强度、voice当前逻辑gain |
| 一次性world事件 | 要保留发生／重启／终止及顺序，需去重事件 | 已接受的选样本Start、明确world Stop／retrigger；状态相同不代表发生次数相同 |
| 资源声明／内容 | 被当前结果引用时按需准备；绑定改变时更新引用 | 文件图、已选音频文件、完整recipe或程序内容；不每帧重发完整库 |
| 已有唯一来源／纯本地派生 | 复用或派生，不增第二字段 | Run.current颜色、GameFrame、D0相位、中心坐标、shader固定格式 |
| Host生产输入 | 只在Host决定最终结果，不下行 | 随机流、队列、倒计时、目标值、分层mask／平滑器 |
| 不应同步的本地或隔离内容 | 不进入world状态／事件，不由远端覆写 | 指针／GPU／设备、个人Options、后端失败、原版Online、存档与解锁命令 |
| UNKNOWN | 原生事实或必要结果来源未核，继续列缺口 | 不补0、不改Null、不因为不发送未知字节就声称显示来源完整 |

资源未就绪／Host某pass被个人画质选项跳过，不是world效果关闭；反过来，原生owner非Null也不是整块内容必须同步。字段无变化不重发，未被当前用途消费的scratch不下发；alpha/gain为0也不自动表示实例结束。带有未来变化的逻辑实例与本次零输出必须分开。

## 2. 资源：同步被采用的内容，不复制资源系统

| 范围 | 必要网络内容 | 不需同步／应排除 |
|---|---|---|
| 文件Image、ANM2、音频 | 当前实际选中资源引用；首次出现时声明可解析的不可变资源，资源变化时更新绑定 | 全目录、加载次数、预载策略、旧路径比较历史；文件已由资源合同提供时不重复发像素／PCM |
| 程序图／surface | 当前确被消费的内容版本；**完整已核recipe或精确内容选一条表示**，含必要图像几何和源坐标空间 | 同时强制发recipe和像素；逐tick整屏截图；未消费的FBO／scratch；`.procedural`名称不能充当内容 |
| HUD消息、fortune、库存／crafting图 | 当前图像及实际顺序／owner／绑定；文字像素或完整文字recipe已有时复用 | 另发同一图像的排版生产输入／全部字体度量；确有独立语义文本消费者时才另保留文字 |
| shader | 实际world用途所选固定shader及其必要参数，资源号／固定类型按正式表统一解释 | AACAB0二十个本地指针、shader程序／属性指针、任意stride／参数字典、完整shader表每帧复制；HQ4X等个人画质选择留本地 |
| blend／sampler／crop／clip | 真正改变world内容的非默认选择在对应固定pass中承接；固定常量由adapter派生 | 原样发GL枚举／viewport scissor；已有Layer或pass承接时再发一份Image状态；不能把world裁剪当个人窗口裁剪删掉 |
| Image尺寸 | 程序内容的逻辑几何／有意义的padding按资源声明一次；pixel／UV转换依赖保留 | Host窗口尺寸／缩放偏好／设备分配细节进入world；ScreenSize／Ratio按Client实际资源分配派生 |
| 实际Image与路径元数据 | 独立表达仍有消费者的两个选择；路径已变但旧Image还在用时保持旧图引用 | 将新路径自动等同实际图像；只为保持任意Getter相等而多发旧名字／原生string |

Host surface如果已经混入个人菜单、设备按钮提示、原版Online面板或本地画质后处理，不能因“能读回”就整图下发。必须确定其writer／内容边界，选择纯world内容或已核合成结果。**前后房间图像和hallucination反馈仍需要，但各自内容闭包、跨分辨率映射尚须落实；不能把此未知改成全部像素必发或全部图像可删。** 本轮不改已注册资源固定布局／kind编号。

## 3. 屏幕效果：保留有效输出，删除重复时钟与生产器

| 效果 | 需要同步或复用 | 不需要另发 |
|---|---|---|
| ColorMod／黑暗／闪电／眩晕 | 复用Run已有current六值及已核显示字段，按同world关联消费 | 同值的Presentation副本、target／lerp／changeSpeed |
| Bloom | 当前world有效性及最终f32 BloomAmount | t／T／strength生产三值、四顶点相同强度副本、Host画质开关；Ratio按本地padded尺寸算 |
| Pixelation | 当前有效pass及最终强度 | Room／Stage计时与两套FSM、Host ScreenSize；本地资源尺寸派生四维尺寸 |
| Hallucination | 最终Amount、两个不同用途的实际内容引用、各次叠加alpha／颜色和顺序 | 剩余／总时长、反馈字节、ShowHallucination；翻转由同批D0相位派生，不再发第二份翻转selector |
| OldTV | 复用Run的seed8显示资格与帧头GameFrame | 每帧Time、Host墙钟、另一份seed8 bool；Time按已核f32算式派生 |
| Retrovision | 最终强度覆盖药丸等短暂来源；seed78资格已有时复用 | 原剩余／总计时、临时BE7704地址；不能只保留seed78而漏短暂效果 |
| Glitch／clip | 当前world pass资格与不能派生的最终选择；复用已有逐用途Grid／Sprite选择及D0 | 同选择的额外结果副本、ANM2地址／RNG、完整Room TemporaryEffects；具体固定参数未核仍为Gap |
| Fade与白色覆盖 | 按实际顺序的最终颜色／alpha；白色固定RGB由adapter给出 | primary／secondary两组同时叠加、已选结果外再发branch和两套RGBA、speed／退出target；value/90被alpha承接后原value不发 |
| Fade loading图 | 仅共享world Fade要求展示时，当前实际图像引用／有效alpha；位置本地中心化，D0相位复用 | Host加载耗时／墙钟、个人等待／网络缓冲UI、为复刻原生载入而写Manager状态 |

新补证：`92B5C0`给Manager+4A9F4／+4A9FC加载`gfx/ui/loadimages/loadimages-%03u{,_2}.png`，Greed用对应子目录；`92B8B0`交替画图。它是加载插画来源，不能擅改名为存档／网络指示器，也不能仅凭属于Manager就删掉共享Fade中的画面。`92B5C0`写本地timeGetTime，Game正常Update还有加载等待分支；**这些等待时长不下行，更不让Client按Host墙钟阻塞世界。** 静态证据与语义决策分开记录。

pass存在性、顺序和当前参数须能区分“本批确认无输出”与Gap；不要求为同一个已完整参数化的固定pass再传全部顶点／绘制状态。Host个人画质不能决定Client world资格，本地用户仍可依其图形设置渲染。

## 4. 转场：选最终贡献，取消原生生产体的重复下发

| 范围 | 当前路线需要的内容 | 当前路线不需要的内容 |
|---|---|---|
| RoomTransition | 真实前／后world surface引用；有序图像／遮罩贡献、当前必要位移／alpha及结构关联 | 原始mode／style／direction、s及smoothstep和最终位移全发；目标roomIndex／dimension重复编码；ChangeRoom／Update |
| versus／额外肖像 | 正常临时变换后的各次layer／主叠加资源与姿态、完整颜色、顺序，保留重复调用 | 原map节点／key地址；同一结果再发未变换Sprite全状态、旋转生产公式输入和完整生产树 |
| StageTransition | 最终遮罩颜色／alpha／存在性及顺序；ascent上下文确有其他消费者时复用Run | 原始mode、计时／总时长、render_kind或SDK同位置bool；本路线不调用Render重算这些输入 |
| Nightmare路线／气泡／肖像 | **最终有序贡献**覆盖Connector、Levels、Clear/NotClearFloor、指示器、四slot内容、程序道具及辅助Sprite；所需资源、姿态、局部几何、颜色／alpha与顺序 | `progress_map[14]`、progress_stage／flag、Scene.display_frame、Dogma／bubble标记、配置索引和队列，再与最终贡献一起全发 |
| BossOverlay | 当前有效贡献的存在性／资源／实际Sprite姿态；本地480×270比例 | selected_entry、五份配置、persist和生产Update；副作用涉及进度／音乐／切房，不在Client重放 |

Nightmare `8FAC20`实际把14项路线、场景帧和Game条件生产成Connector／Levels／ClearFloor等多次临时Sprite提交（同一个progress Sprite会逐项SetFrame）。因此“只发最后Sprite”不足；同理，**最终全部贡献已承接后再发14项生产数组也多余**。这是本轮修正，旧“路线／场景帧／Dogma必须独立同步”决定留在修订历史。

不删通用ANM2中仍有实际绘制／插值消费者的anim_frame／is_playing／playback_speed；它们与NightmareScene帧计数不是同一字段。位置／clip／source-quad必须采用已核的局部或视口锚定语义，不能把Host绝对屏幕像素直接当跨分辨率坐标。**完整贡献捕获、各用途坐标解释和正式adapter尚未实现**；没有完整输出时要报缺口，不能靠删路线输入宣称Nightmare已可用。

## 5. 音频：最终voice参数与事件，生产混音树不上线

### 5.1 需要保留的状态、事件和资源

| 范围 | 必要内容 | 不应重复发送的字段 |
|---|---|---|
| 资源 | 实际选中sample或音乐声部的文件／intro→main依赖及内容身份；固定通道／混音规则按资源／协议归属 | 全部sounds／music XML、未选sample、weight／sample_count、全部配置名称／ID；已有资源身份无需再同步sound ID Getter |
| 逻辑voice | 稳定实例／代次、资源引用、世界逻辑存续与pause、loop、当前Options前最终gain／pitch／必要pan | 本地source／control、后端playing／pending／loaded／active镜像、原生buffer／slot号 |
| 音乐crossfade／layer | 所有仍有逻辑用途的并存声部，分别有最终gain／pitch／loop；零gain而将来可继续的声部不能仅据音量删除 | 固定两组×7空槽；layer mask、mix_mode、config mul、layer当前权重、组current、ducking再全部下发 |
| Music总体pitch／enabled | 对实际voice参数的影响由当前最终pitch／gain或明确world暂停／终止承接 | 再发Music+314全局pitch、+328混合enabled、本地Mute；不能从后端静音推导世界停止 |
| 开始／重启／停止 | 通过world逻辑资格后的已选sample Start、明确retrigger、world Stop／有范围Clear；事件序号／逻辑时点／状态关联和水位 | FrameDelay作启动延迟、被PRE取消／被节流的Play、每帧SetAmbientSound当重播、每次更新都再发Start |
| 仍在播放的one-shot | 已有实例可接收必要gain／pitch／pan／world Stop，sound460的内联pan不能漏 | 对新加入者补播过去Start，或仅因它loop=false就丢掉后续控制 |
| jingle | world事件和仍有显示意义的当前逻辑实例；独立保留正确的本地SFX音量类别 | +32A官方ID当启动确认、+364倒计时当游标、+360 ducking在最终gain之外再发一次 |

本轮选择**最终voice gain**，所以MusicStream+18的mul、+1C权重、父组+174和Music+360 ducking只是Host计算该值的来源。Client不再用同一批生产输入重复混音。mask与mix_mode本身没有其他已核网络消费者，撤销原“必须发六位mask”。若某固定配置是Client打开intro→main所必需，则只在资源依赖声明保留；不得再把它重复乘入已合成gain。SFX不同正常来源的clamp规则仍需明确，最终world gain是**应用个人Options前**的量。

音乐Options前全部writer与逻辑包络仍未闭合，这是取得必需gain的缺口，不能改成不需要gain，也不能反除Host音量或补1。每voice已能完整表达pitch时不另发全局Music.pitch；同理，原生queued、targetPitch、targetVolume、fade rate、SFX modifier恢复步长、Room环境平滑器、jingle倒计时均留Host。Host维护原生组与C/S voice的映射，不把两组本地槽号当网络身份。

### 5.2 必须修正：world退休与本地后端完成不是同一事件

- **明确world Stop／重触发／持续用途结束／会话退休需要同步。** 其声部集合应来自正常调用用途与逻辑实例；不能只以某个设备返回false决定world结束。
- **Host设备失效、资源未就绪、后端抢占、音源上限、窗口失焦和本地静音不下行成全体Stop。** Client设备故障也不修改服务器音频意图。这些是本地执行状态／诊断。
- 普通one-shot的后端自然播完由各端本地播放owner收尾；不要求把每次硬件完成回调当world事件。若Host据完成结果实际选择下一首音乐，发送它已经作出的新选择事件，不发送底层设备轮询历史。
- `907E9D`非空source的Play提交是已核**本地执行证据**；不能作为唯一world Start资格。需在通过原生节流且已经选sample的逻辑分支保存world意图，同时分开记录本地提交是否成功。Host无设备不能静默抹掉世界声音；来源暂不能区分时标Gap，不能补成功或补停止。
- `SetAmbientSound`的“后端未playing→Play”可能是本地恢复／重试，不能自动分配新world实例。须按world用途／代次判断持续需求、真实替换及显式重播；这部分用途归属和逻辑生命周期来源仍待落实。

此前专篇把“自然结束／偷占／设备失效统一通知”一起列为同步完整性的缺口，范围过大；本轮撤销把个人后端失效网络化的推导。仍未核的是**world用途／退休的完整来源，以及后端结果是否污染Host原生意图生产的隔离办法**，不是要求所有设备状态跨端一致。

### 5.3 baseline和相位

持续声／背景音乐baseline需要当前资源、逻辑实例和最终参数，重复state不重启；历史one-shot不重播，旧代次控制不作用新实例。正在进行的瞬态jingle若需恢复，必须有可解释的逻辑段落／起点及执行能力，不能从头Play后称为恢复。

**采样精确的Host硬件游标／设备延迟／PCM队列不属于必须同步的world状态。** 不要求两台音频设备逐采样相等。真正影响共享画面的逻辑段落／开始时点仍需要承接；当前segment、seek和可靠续接能力未核，保留该具体缺口，不把MusicStream+2C更新计数伪装成游标，也不以“全部本地”抹掉intro／main和共同事件顺序。

## 6. 明确排除与仍然未知

| 明确不应同步／执行 | 理由 |
|---|---|
| Host窗口／分辨率／个人画质、声音音量／静音／设备／预载与就绪 | 各端个人或资源后端状态；world意图与本地呈现分开 |
| 原生指针、strong control、GL／FBO／GPU命令、音频handle／采样buffer、STL节点、后端slot | 不是跨进程值或稳定身份；Client使用自身owner |
| Fadeout target、ChangeRoom、StartStageTransition／BossIntro、ShowHallucination、Nightmare Show、BossOverlay Update | 都是Host生产／流程命令，可能随机、切房或写持久状态；只同步其已产生结果 |
| 原版Deathmatch owner、Online密码／邀请／语音、Isaac Sync状态 | 已有C/S隔离范围；不建立旁路适配 |
| 个人菜单点击／等待动画、成就弹窗／解锁／存档／排行榜副作用 | 不能由Host画面或音频钩子扩展访客权限；共享world结算显示另按已核结果承接 |

保留的UNKNOWN包括：原生各孔／派生Image和后端完整ABI、surface纯world内容及坐标闭包、全部音乐Options前writer、world音频用途及退休来源、当前segment／可用seek与恢复语义、视频解码器独立音轨。**UNKNOWN与已知不应同步分开**；不增加新的未知字节结论，也不把已读字段裁剪包装成完整性已通过。

## 7. 回写范围与后续

两份来源目录每一行都关联本轮同步决定；Presentation字节账本修订5保留原字段事实及UNKNOWN数量，只改当前下行职责，旧决定留历史。公开成员回映射收紧Music IsLayerEnabled／pitch／ID等行；第8／9／10／13步按本轮结果路线接线。正式布局与预算仍待后续，不因本轮写入设计JSON就自动成为可接受报文。

本轮静态审阅只核资料、消费者、互斥表达与交接一致性，没有构建、游戏操作、校验脚本或测试。第15步再统一验证共享效果、不同个人设置、后端失败隔离、转场多次提交、重复／乱序事件和恢复边界。
