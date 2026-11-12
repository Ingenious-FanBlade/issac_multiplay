# J273 采集／播放入口与载荷语义

日期：2026-11-12。地址为 J273 VA；RVA＝VA−`0x400000`。本表是后续同源 schema／原生适配的输入，没有在本轮新增线格式或 Hook。

## 1. 首要资料

原版 [SFXManager.md](F:/projects_isaac/IsaacDocs/docs/SFXManager.md)、[MusicManager.md](F:/projects_isaac/IsaacDocs/docs/MusicManager.md)、[HUD.md](F:/projects_isaac/IsaacDocs/docs/HUD.md)；REPENTOGON [ItemOverlay.md](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemOverlay.md)、[RoomTransition.md](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomTransition.md)。SDK 为 `libzhl/functions/{SFXManager,SoundEffects,Music,HUD,ItemOverlay,RoomTransition,StringTable}.zhl`，官方实现重点为 `LuaInterfaces/CustomCallbacks.cpp`、`LuaMusicManager.cpp`、`Room/LuaRoomTransition.cpp`。旧细节参照[音频来源](../../presentation-audio-sources.md)，其原始事实保留，新事件／状态职责按本方案。

## 2. 入口及实际语义

| 内容 | Host 来源 VA（RVA） | Client 入口／限制 |
| --- | --- | --- |
| 非循环 SFX | `SFXManager／SoundEffects::Play 0x907C60`（`0x507C60`）；同址只设一个协调采集 owner | 同一 SDK Play，C/S replay scope 内调用。原生可能因节流、非法 id、空配置或后端不可用而返回，void 不作成功凭据；只能回放已接受的逻辑事件，不转发外层每次失败请求 |
| 循环声 | Play(loop=true)，正常 Stop `0x908140`、StopLoopingSounds `0x908210`、AdjustVolume `0x907F60`、AdjustPitch `0x908070`、Ambient `0x908530` | 有界状态集合按当前逻辑实例协调启停。Stop 按 sound id 影响其多 sample voice，不提供单 voice handle；不能只存一个 id 就宣称重建全部并存声部 |
| 音乐 | Play `0x7D8360`（`0x3D8360`）、Crossfade `0x7D8480`（`0x3D8480`）、Queue `0x7D8A80`（`0x3D8A80`） | SDK Play／Crossfade；Queue 原生 thiscall 一个 i32、`retn 4`，有效请求直接写 `Music+0x310`。仅在请求代次变化时调用，未变的状态包不重播；Client 房间／Boss 初始化的自选曲禁止覆盖 Host 请求 |
| 道具横幅 | `HUD::ShowItemText 0x97B080`（`0x57B080`），`(Player*,ItemConfig_Item*)`；CustomUTF8 `0x97B720`（`0x57B720`），`(Player*,char*,char*)` | 正常原生 HUD 路由会决定多人 mini text 或 stacked text。typed item 用 Client 配置及本机语言；自定义文字先按键解析再供 UTF8 入口。外层／嵌套 stacked 调用只登记一次逻辑事件 |
| fortune | `HUD::ShowFortuneText 0x97BEE0`（`0x57BEE0`），SDK thiscall 一个 `int**` 外观参数 | 真实参数是原生 list 头，详见下节。必须在图形主线程／正常 HUD 资源上下文创建本地文字图，不能把此调用派到网络线程 |
| ItemOverlay | Show `0x9848B0`（`0x5848B0`），SDK `(int id,int delay,Entity_Player*)`；当前状态从各实际 owner 读取 | 新演出代次先 Show，再安装 Host 的 state、delay 与两份必要 Sprite。完整 Update=`0x984110` 继续拦截；Render 已在 Game::Render 树内，不另绘制 |
| Boss 开场 | `RoomTransition::StartBossIntro 0x823D10`（`0x423D10`），SDK两个 u32，this=Game+1B83C | 具名 BossReplay 用本机 RoomTransition 和合法 room 描述，隔离／协调下文的子生产与直接标记写入后才可调用。官方 Lua 包装会先设置 `_roomIndex=Game._startingRoomIdx` 以防非法索引；C++直接调用不会自动执行这句，必须准备真实有效目标。不可传 Host 指针或 isaac-lan 的8参数结构 |
| 切房 | 原生 Start `0x823990`，五栈参数；Host 正常 ChangeRoom 后关联最终目标 | 见 [RE-9 载荷与拆分](../RE-9/client-split-contract.md)。原生 Start／Update 都不是整体无副作用的显示 leaf |

SFX 的原生参数为 id i32、volume f32、FrameDelay i32、loop bool（占栈槽）、pitch f32、pan f32。ECX 是本机 SoundEffects，通常 `Manager+0x2A160`；没有原生 listener／peer 参数，听众是 C/S 附加的用途／可见性信息。

音乐 this 通常 `Manager+0x29DF8`。`Music+0x30C` 是 current、`+0x310` 是 queued；队列空值不得由“GetQueuedMusicID 返回 current”的 API 表象反推。`0x7D82A9／0x7D82E4` 正常消费 queued，`0x7D833E` 清零。只挂 Play／Crossfade 会漏直接 Queue 与消费后状态，因此须在正常写点或同窗原生字段读出中维护请求缓存。13B 只是参照工程的压紧请求体，不是已核的完整 C/S 网络包大小。

## 3. 事件与状态载荷

下表是**语义字段**，整数宽度、总限额和版本72生成表随后统一定义；不把本表写成已部署协议。

| 载荷 | 必需信息 | 去重／恢复 |
| --- | --- | --- |
| 共同事件头 | session／scope 身份、单调事件序号、Host game_frame、种类、长度；相关 Player 使用稳定成员身份，绝不下发地址 | 每 Peer 有界串行水位；确认接收与实际可播放目标分开，遵守 world 提交依赖；过期纯外观事件可丢并计数 |
| SFX one-shot | 接受后的最终 sound id、volume、FrameDelay、pitch、pan、听众／world 用途；必要的已选资源依赖按现行音频合同 | 只登记玩法来源；回放标志阻止二次采集。FrameDelay 不转成将来开始时间，也不把重传当再次 Play；选样本细节若要求保持，复用旧正常选样本来源，不在 Client 重抽来冒充同一 sample |
| 循环声状态 | 集合版本、逻辑播放代次、sound id、loop、当前逻辑 gain／pitch／pan、用途和必要资源 | 显式 world Stop／替换才退休 world 实例。断设备／Host后端 source 空不等于所有 Client 应停；完整空集合意味着停止，缺状态不等于空。按 id 聚合的 Stop 后要重建同组仍保留实例；单 voice 适配还未实施 |
| 音乐请求状态 | 请求版本、播放代次、模式 Play／Crossfade／必要的静音／退休意图、当前 id、原参数、queued id 与有效性、world用途 | 同 id 的新播放代次仍重播；未变版本不重播。Crossfade 的旧／新两组由本地原生处理；重连收敛到当前请求，不声称恢复精确硬件游标。jingle／layer 等已核控制不能因精简请求而静默丢失 |
| 横幅 | item／trinket 等类型与 id，Player 身份，title／subtitle 的本地化类别＋键（或索引），原始／最终文字后备、路由／样式 | typed item 使用已准备的 Client ItemConfig；不同时把外层 ShowItemText 和内层 Custom／stacked 记成两条。自定义无键文本原样显示；两端字符串表不一致按后备文本处理 |
| fortune | 每行独立长度与字节，原始选中类别／索引或键、文字后备，必要样式 | 不只传 `|` 拼接字符串，避免混淆原有竖线、空行与行分隔。Host 抽出的索引不在 Client 重抽；重连不重放旧 fortune |
| ItemOverlay 状态 | owner／controller或稳定Player关系、演出代次、state、id、delay、主 Sprite 与 MegaMush Player Sprite 的资源／姿态 | 新代次即使 id 相同也先 Show；同代次幂等更新不反复 Show；state=0 仍是有效清除。没有新状态时不运行玩法 Update 推进它 |
| Boss 开场 | 事件序号、frame、当前 room／dimension／transition 关联、bossID1／bossID2、必要的已选肖像／资源依赖 | 两 id 是原生参数，不是整个载荷；额外肖像 map 由实际 owner 准备。已活跃演出的重连恢复需当前演出状态／Sprite，不能只重放陈旧 Start 事件 |

循环集合不能用额外 `IsPlaying` 扫描伪装纯读采集：`0x908470` 会依后端状态清 pending，且只按 id 聚合。旧正常查询的观察可以复用，但 world 意图登记、声音后端与重连状态必须区分。旧 sample／voice 来源和仍需控制的 one-shot 限制见[音频来源](../../presentation-audio-sources.md)，本轮没有将这些问题标成“已实现”。

## 4. 文字与本地化的正面来源

`ShowItemText 0x97B080` 在 `0x97B0E4／0x97B0FA` 通过 `0x72A5B0／0x72A690` 读取 ItemConfig name／desc；字符串以 `#` 开头时，两函数调用 StringTable 的 Items 类别按本机语言解析。原始键位于 ItemConfig `name+0x08／desc+0x20`，不能只在翻译后 UTF16 图像里找键。函数内还有显示变体／文字转换路径，Host 已选的文本结果与普通 item 身份分别保留，不由 Client 重演 Host 的玩法选择。

原生 fortune 正常来源 `0x958C30`：`0x958CB6` 先抽选索引；`0x958CC1` this=`Manager+0x4A61C` StringTable，`0x958CCD→0x9F2D90` 按**类别、语言、选中索引**取文字，然后按字符 124（`|`）拆行，`0x958F36` 交 ShowFortuneText。因此采集应在同一正常选中窗口保留类别／索引，Client 只做本机语言查询，不能再次 RNG。`0x9F2D90` 对无语言／无类别／未翻译分别返回明确文本，不能无界遍历表。

ShowFortuneText 参数：`[head+0]` 为循环哨兵、`[head+4]` 为 count；每节点 next／prev 各4B、`std::string` 位于 `node+8`，节点共0x20B。`0x97BFCC` 取首节点，`0x97BFF2／0x97C1FB` 沿 next，`node+0x1C` 是 string capacity。原版 Lua adapter `0x8452A0` 明确 `operator new(0x20)`、字符串构造／复制和正常清理，最多接受32个字符串实参。网络行数组不能直接传成 `int**`；本地 C++ list 的 allocator／运行库兼容及异常清理仍须由已有原生内存 owner 落实。

ShowFortuneText 自身会创建本地文字图、flush 图形队列并在部分分支进入 present，因此它不是任意线程的纯 setter。应在现有图形主线程合法窗口调用一次，服从正常上下文和在途寿命；本轮没有验证实际 GPU／窗口行为。

## 5. ItemOverlay 与 Boss 的混合副作用

ItemOverlay 的 state/id 在 +0/+4，主 ANM2 +8，delay +0x1144，Player 槽 +0x1148，MegaMush ANM2 +0x114C。Show `0x98491D—0x98494B` 对 Player 槽先撤旧登记、再登记新引用，不能用裸 Player* 赋值代替；`0x984AFE／0x984B00` 按 delay 写 state 与 delay。Show 后再安装精确 Host 姿态，避免 Show 把已装状态重置。

完整 Update 在 `0x984296／0x984352／0x98447B／0x9845E7` 发 SFX，还调玩家治疗／形态 helper、进度 `0x903A50`、`0x7408B0`、房间 helper 与选曲。因此这些 Host 音效继续走事件，不能因“ItemOverlay 是显示”而放进显示作用域排除。只同步 state 也不会自动恢复可见玩家；玩家可见性由同目标 Host Entity 状态承担。

Boss Start `0x823D8C` 调 Music::Play(81)，`0x823D9D` 清 Room 环境音，`0x823DA9` 清 `Game+0x22E90`，尾部 `0x825457` 进入 mode2。Client BossReplay 的音乐子调用由音乐状态 owner 控制，环境循环声由同步集合控制；不能让一次 Boss 事件另起第二条未经版本管理的音乐。Boss 动画结束时的玩法标记／保护计数修改由 [RE-9](../RE-9/native-transition.md)隔离，原生 Update 不能整体放行。

`Game+0x22E90` 是本体直接写入，暂停音乐／环境声子调用并不会挡住它。实现必须明确它与当前 Host 显示状态安装的关系，采用具名窄拆分或合法批次内的局部保护；不能笼统宣称两个子调用钩子已隔离完整 StartBossIntro。此项未接通前，完整 Boss Start 同样不获得通用放行资格。

所有需要新增的 wrapper／shim 经统一 Hook 清单常驻，Host 正常原函数 exactly-once；未激活时原样透传。当前没有实现这些采集／播放 owner，也没有 E37／E39 通过结论。
