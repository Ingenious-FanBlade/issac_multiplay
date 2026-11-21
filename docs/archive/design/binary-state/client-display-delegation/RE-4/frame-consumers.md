# RE-4：帧号消费者与分支证据

返回：[成果总览](README.md)。所有地址为 J273 VA，RVA=VA−`0x400000`。F=`Game+0x25A70`，M=`Manager+0x4A8B8`，R=F−`Room+0x11F0`，E=F−`Entity+0x328`。按帧号求值、按帧缓存和按调用推进是不同职责。

## 1. 直接相位求值

下表为 `J273_STATIC`，处理列为 `DESIGN`。这些判断不要求收到每一个中间 F，也不是应回放的事件队列。

| 消费者／位置 | 已核判断或运算 | 跳帧／停顿与处理 |
| --- | --- | --- |
| PlayerHUD::Update `0x832D38` | 满足低血及资格条件时，`r=F%45`；r<10 则按 `255*(9−r)/9` 写 `PlayerHUD+0xA` 的闪烁值；此前清该值 | F不变则此相位不变；跳帧可能跳过整个亮窗，直接显示目标相位，不为它补跑 PlayerHUD；同函数其他本地计数仍按调用变化 |
| PlayerHUD::RenderHearts `0x8350A8/0x835345` | F 转浮点，经常量缩放后调 `0x412AC0` 产生心形显示系数 | 保留原生当前相位；不以“每次 Render 都要变化”为由推进 F |
| Minimap::Render `0x9677C4—0x9677D9` | 时间值<=0 时，`F%8>=4` 选择隐藏／闪烁分支；正常时间读取独立 TimeCounter | 跳帧仅跳过闪烁画面；停包保持该分支。不能拿 M 或墙钟代替 TimeCounter／F |
| HUD::Render `0x97DF97—0x97DFBA` | `F%20>=10`，选择 coopMenuSprite overlay AnimationState（HUD `+0x5544`）帧0／1 | 状态求值；不发送“第10帧”事件，也不以补Update触发切帧 |
| AnimationLayer::RenderFrame `0x404C82/0x404C94`、`0x404D5F/0x404D71` | 特定 shader 按 flag 0x40 选 M/2 或 F，再取 `%256 /256` | 选择本机／游戏时间有明确标志；暂停时二者可不同，保持原生选择 |
| BeamRenderer::End `0x41A495`；PlayerHUD::RenderTrinket `0x83785A` | 特定材质同样以 F 的256帧周期给 shader 参数 | 可直接跳到目标材质相位，不恢复 Host RNG 游标 |
| HistoryHUD::Render `0x82FCD0` | 材质相位取 `M/2%256` | Host F停止时仍可随本地 M 前进；不从 D0 恢复 Host M |
| Room::Render `0x8057A9/0x805D1D` | 特定绘制参数取 `F%60 /60` | 当前值求相位，停止／跳跃均无需重演中间 Render |
| Room::Render `0x807787/0x80779B` 与 `0x807CC2/0x807CD6` | 暂停时 subframe=0，否则 `(M+1)%2`；与 `F%period` 组合后除120形成相位 | F停住时，本地半帧可能交替，不保证整幅画面完全静止；这是原生两种时间的组合，不是帧对齐失败 |
| Backdrop::RenderWater `0x91AC70` 内 F读取；Game::Render `0x6F8E05` | F分别进入水面时间量及全屏 shader 时间参数，后者乘0.05 | 原生 Render直接求值，不同步旧最终 packet，不借帧 |
| Game::Render `0x6F8B0A` | M%4 决定部分显示采样／抖动分支 | 跳 Host帧不妨碍本地采样；重复相同 M 得相同选择，不代表整个 Render 必须被丢弃 |
| GridPressurePlate::Render `0x95E381` | 条件满足时按 `F%4<=1` 分支选择外观 | 原生显示按目标F求值；不运行压力板 Update 补状态 |

HUD::Render `0x97DA79` 的本地 M 动画属于原版 WAITING/lobby 文字分支；它不构成接管原版 Online 状态的理由。C/S 继续保持原版 Online 关闭。

官方 [Anm2Extras.cpp:315](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/Anm2Extras.cpp:315) 同样按 IGNORE_GAME_TIME 选 F 或 M/2，给 Dogma／Gold shader 的256帧相位。[InterpolV2.cpp:54](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/InterpolV2.cpp:54) 在选项／资源／暂停菜单等条件成立时按 M奇偶取0或0.5插值量；[ExtraRenderSteps.cpp:117](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ExtraRenderSteps.cpp:117) 用 M−本地激活戳做错误文字淡化。它们是 `SOURCE`，不把 C++ wrapper 冒认成原版 IDB 入口。

## 2. 严格相等与年龄阈值

### 2.1 周期 HUD 请求不是无条件本地 tick

Game::Update `0x6F789D` 写回新 F 后，整数运算求 F%30；`0x6F78BA` 非零则跳过。若 HUD type（Game `+0x22E94`）不为2，`0x6F78C5/0x6F78CF` 写 type=1／countdown=2。

- 29→31：两个到达值都不满足 `%30==0`，朴素移植会漏请求。
- F=30 停住：本地30Hz批次继续执行，朴素移植会每批把 countdown 重新置2。
- HUD::Update `0x97ACF9—0x97AD18` 仅在 consumed byte `HUD+0x54EC` 为真时减 countdown 并清该 byte，到期清 type；不能为避免上述重复而改成无条件减，也不能人工伪造 consumed。

采用跨周期／变脏合并请求，详见[处理方式](handling.md#2-周期-hud-刷新)；不重新运行原生 F递增段。

### 2.2 RoomAge==30：历史 surface 捕获

Game::Render 原始判断：

```text
0x6F7C90  cmp byte ptr [Room],0
0x6F7C9A  mov eax,[Game+25A70h]
0x6F7CA0  sub eax,[Room+11F0h]
0x6F7CA6  cmp eax,1Eh
0x6F7CA9  jnz 0x6F7D0E
```

`0x6F7CA0` 起11字节：`2B 81 F0 11 00 00 83 F8 1E 75 63`。年龄为30时读取 descriptor `+0x5C`，以临时 xorshift结果 `%3` 和 Game byte `+0x25AB8` 决定是否进入捕获。byte=0xFF 另有强制进入路径：

```text
0x6F7D0E  cmp byte ptr [Game+25AB8h],0FFh
0x6F7D1D  ; 捕获分支
0x6F7D21  call 0x67EDD0              ; 图形状态保存
0x6F7D28  push [0xBE72B4]            ; 本地 surface
0x6F7D33  call 0x9E80F0              ; 选择渲染目标
0x6F7D38  mov byte ptr [Game+25AB8h],1
```

局部捕获标志使后续进入该 surface、结束后恢复并绘制；同一 surface 还在 hallucination 相关绘制中消费。没有在此写入全局玩法 RNG 游标、发放奖励或推进世界。

该分支要求真实有效的descriptor输入：`+0x5C` 为0时 `0x6F7CCA—0x6F7CD9` 记录 RNG Seed is zero 并进入 int3。不能给原生Render一个裸零配置；必要源由冷构建／需求清单提供。本次没有新增seed writer，也不因此同步全局RNG游标。

**重要区别**：写 byte=1 并不保证在年龄30时只进一次。临时随机 `%3==0` 时，停在30的多次 Render 仍可重复捕获；29→31则略过年龄分支。这是本地历史画面的采样差异，不是应补回的玩法事件。不能统一把所有 `==某帧` 改成 `>=某帧`。

byte 与 surface 归本地 owner。冷加入／资源失效不能假定已经历本地年龄30；`0x6F08D0` 在 `0x6F092A` 清该 byte，本次显式位移检索未取得其他生产者的完整闭包，不把未命中的批量／别名写入当作不存在。后续本地刷新请求可利用已核0xFF分支，限于资源owner与下一次真实Render，不调用有玩法副作用的 ShowHallucination。

### 2.3 年龄<=1 与短时光照

| 消费者 | 已核阈值 | 风险／处理 |
| --- | --- | --- |
| StatHUD::RecomputeStats `0x83D000—0x83D00F` | `F−Player[+0x328]>1` 传给显示值 helper `0x83DC60` | 这是是否产生属性变化提示的资格，不是“上次调用距今恰好一帧”。晚加入时不能借Host年龄判断本地缓存是否已初始化 |
| helper `0x83DC60` | flag为假时清delta／提示计时并写current；flag为真且变化达阈值可启动150计时，已有提示还有30／135分支 | 当前／previous／delta／提示计时是本地历史；首次本地绑定先初始化，再正常接变化，不改 F／实体锚点 |
| NPC::Render `0x6CEC90` 的年龄判断 | E>1 再允许特定状态外观／OverlayEffect分支 | 跳过0／1直接按目标年龄显示；不人为插入“出生帧”或补Entity Update |
| Room::RenderEntityLight `0x7FC110` 内多处 | E<7、E<10等缩放淡入／短时光照 | 直接算目标强度；错过整个短窗时不在Client重新创造闪光实体，必要瞬时表现归既定事件／实体状态 |
| FX Render `0x96CA3B/0x96CA41` | 相关背景条件下颜色系数 `clamp(R/30,0,1)` | 跳过30不会丢完成动作；这是连续钳位求值，R>=30自然达到最终强度 |

StatHUD单值记录的本次已核相对布局：current `+0`、delta `+4`、previous `+8`、threshold `+0xC`、countdown `+0x10`；首个记录从 StatHUD `+0x118` 开始，槽跨度 `0xCC`。只用于具名显示缓存初始化，不当整块复制布局。SDK StatHUD 声明不包含这些成员，不能用错误的普通C原型强转调用其 XMM float helper。

## 3. 真正的按帧缓存

平均位置 helper `0x995B30`，原生 caller 使用当前 PlayerManager、Vector输出指针和 useCache bool；`retn 8`。SDK本次读取资料没有独立具名声明，原型不能从 Hex-Rays 的临时寄存器推导。

```text
0x995B36  cmp byte ptr [ebp+arg_4],0  ; useCache
0x995B42  jz 0x995B7C                ; false 强制重算
0x995B49  mov eax,[Manager+4A8B8h]
0x995B4F  cmp eax,[0xBD01E8]          ; 上次计算的 M
0x995B55  jnz 0x995B7C
0x995B5A  movss xmm0,[0xC0136C]       ; 缓存坐标
0x995B66  movss xmm0,[0xC01370]
...
0x995D24  mov eax,[Manager+4A8B8h]
0x995D2A  mov [0xBD01E8],eax
```

重算路径尾写坐标缓存 `0x995DE7/0x995DEF`。重算必须使用真实当前 roster；缓存命中只返回旧坐标，不能证明它仍对应当前 roster。不启用原版 Online 来取得平均值。

FXLayers 更新在零初始位置／直接初始化条件下传false；常规平滑路径传true，再对结果以0.1趋近。故第二次相同M的调用既可能取旧缓存，又可能再推进一次FX平滑；**缓存去重不等于整个函数幂等**。M是进程本地计数，不带 world身份；重建、同M换目标或 M回到旧值时需要本地资源／缓存owner处理，不把缓存戳同步为Host值。

## 4. 没有通用 F 去重的调用

- SDK ANM2::Update `0x404550` 有资源才调主／overlay `0x404480`；后者以 playbackSpeed 推进 animFrame。调用两次可推进两次，不检查“F是否已经更新过”。
- `0x4041A0` 清 AnimationState `+0x18` 当前事件位，再按动画游标推进，更新层／null层。`0x406710` 只测试当前事件位，不将它消费掉；重复查询不是 exactly-once 机制。Wiki 的 IsEventTriggered／WasEventTriggered 对当前／历史事件也作了区分。
- HUD::Update／PostUpdate、PlayerHUD、Minimap、FXLayers 的具体调用计数见 [RE-3 目录](../RE-3/display-counters.md)。Minimap `0x965FEC` 先增加本地holdTime，再在 `0x965FEF` 比较是否恰好10并触发楼层文字；它不是按 F 的十帧阈值。
- SFXManager::Play `0x907C60` 以 `M>>1` 对比每音效的重播期限，允许时保存本次时间与 `本次时间+FrameDelay`。Wiki明确FrameDelay是重播间隔。取0时同一时刻再次调用仍可满足 `>=`，不代替调度／Q7事件去重；默认重播间隔也不能保证不同来源不会双响。完整来源判定归 RE-8。

## 5. 排除错误的帧替换

- 原生 render_sort `0x4150C0` 按 F奇偶选择渐进方向／工作预算，另有空表和scratch限制。主方案已决定用自有有界全排，不通过伪造F奇偶补齐排序。
- Player伤害闪烁 `0x784BCA—0x784C07` 读 `Player+0x13B8`：正值、余6小于3及其他资格满足时跳过绘制。它不读F／M。Wiki特别注明其计数单位是render帧；沿用[既有补证](../../../tx-04-20/player-i39-source.md:99)，不能机械地每个Host GameFrame减一。
- 实体／Grid Update、房间规则、保存／恢复中其他 F判断不因此放行。Q8 的 Game／Room 内联标量也不在本地推进。本次在已采用显示入口中没有发现可充当所有显示调用去重的“F必须差1”通用保护；不把这个限定结论外推到整个游戏。
