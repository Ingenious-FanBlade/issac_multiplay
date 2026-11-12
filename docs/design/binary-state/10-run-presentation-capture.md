# 第10步第五项：Run与显示／音频生产缓存

日期：2026-11-12。布局版本71。**当前为实施中，第五项尚未完成。** 本文记录实际代码、静态来源与剩余工作；没有运行测试、启动游戏、部署DLL或执行实机实验。产品纯编译与生成不代替第15步统一整体验收。

## 当前停点：概率查询暂缓

2026-11-12，用户明确指示：“先暂缓概率查询这一块，将当前进度留在文档中。”从此停下概率查询部分的后续逆向、实现与运行操作；本节是恢复入口。**概率查询块未完成，第10步第五项整体也未完成。** 当前代码保留在工作区，没有回滚或部署；没有启动游戏，未实际执行概率查询，没有运行任何测试。此处暂停的是开发工作，不表示新增了运行期开关或已禁用某条部署路径。

已确定的需求与路线：每个参与者显示自己对应的属性面板和道具获得历史；详细面板按控制组及对应身体归属。概率部分优先复用原HUD当前有效结果，缺失时按用户已确认路线复用C++概率查询接口，不再重写完整概率算法。天使房接口返回修正值；恶魔房接口返回两种房共用总出现率；星象房接口直接返回概率，官方REPENTOGON的资格修正须保留。具体Wiki位置与J273证据见下节。

已保留的代码停点：

- `mod/src/game/observation/binary_entities.cpp`：`ProduceOwnedStats`已编写逐身体六属性／变化相位，以及正常HUD概率结果优先、缺失位集中查询的接线；当前结果仍只作为未验收实现。
- `mod/src/game/observation/binary_probability.{h,cpp}`：复用`Room::GetDevilRoomChance()`和`Game::GetPlanetariumChance()`，保留必要面板拆分；`GetAngelRoomChance()`对应的修正值从已核字段读取，不经Lua API。
- `mod/src/world/binary/hud_probability.cpp`：只保留总概率之外的楼层／出口资格投影；此前完整自有概率公式已移除。
- `docs/design/binary-state/hud-local-presentation.md`：已记录用户对这几个C++概率查询的具名例外，不放宽其它HUD／随机／Lua API约束；本次暂停不撤销已确定的来源选择。

恢复时先继续静态核对C++查询调用链、官方包装／回调影响、同窗资格、当前有效缓存与失败撤销，确认集中查询不受房主显示偏好影响，也不误读另一身体的六属性／历史。然后补齐剩余接线；不能将编译成功或Wiki有接口写成运行正确性通过。第15步仍是唯一统一整体验收，恢复前不补跑测试。

暂停前最后一次Release／Win32产品纯编译及链接**成功（退出码0）**，包括C++17 SDK桥设置；此后仅更新本文及入口文档，没有再次构建或测试。其余第五项来源、缺口及此前编译失败记录保留如下。

## 当前实现

`mod/src/state/production_cache.{h,cpp}` 提供主线程固定4MiB页池与1024项目录。缓存保存自有record/cell/path/bytes和实际引用的紧凑纯值目录；替换先复制后退休。ordinary交接复制内容并重映射成员引用，后台不借用可覆盖页或原生对象。记录保留真实正常生产窗口、来源修订、owner、实例／代次、用途、调用父子关系和事件序号，不以本次Authority tick冒充来源。持久结果在内容和依赖不变时保留初次生产修订；D0保留本次实际生产。失效、容量、线程、异常和来源变化均明确失败。

`binary_production.{h,cpp}` 的常驻wrapper经唯一Hook清单注册，只有正式Host主线程有效窗口启用；Client及停用透传，原链由wrapper调用一次。生产窗口可以位于真实Manager outer外，不编造Authority tick。Score、PlayerHUD／Stat、Screen／Room Render和音频正常控制各有具名owner；没有为采集额外调用Update、Render、Recompute或随机，也不经Lua API取值。用户在本次会话后续明确批准的C++概率查询是具名例外，见下节。

| 来源 | 已接线的行为 | 具体边界 |
|---|---|---|
| Run237 | 当前seed树七项显示投影、daily、ascent、TimeCounter帧基准及原有声明字段 | 树有界80项、稳定头与键；不读生成流或全部Game |
| Score234 | 9BAC90正常完成后的15个u32与+4C实际时间，64B | 当前Game缓存和证言同scope且数值一致才有资格；不是+40 vector或Leaderboard副本 |
| HUD进度图标 | 97C300实际短路分支的Seeds／Manager返回与同窗custom、rerun、challenge | 不额外查询解锁或将构造默认值当最终结果 |
| PlayerHUD315／435／436 | 832C10正常结束的心格／特殊结果采集、最小critical_health、实际Drop返回、更新序号与有限锚点；90A490正常查询点封存P8 | 436保留实际已消费的那一份效果修订；冻结分支不采当前P8替代；315必要特殊来源仍按具体handler失败 |
| Stat317 | 每身体自有十行Host显示缓存；六项最终属性在Host转换成原生显示单位，独立delta／previous／threshold／countdown | 以83C860实际正常调用推进，不用网络包／Client帧；线上只取0—7及获准的8行，previous／threshold留Host |
| History321 | 每身体有序History只读生成最新在前的显示列表，保留重复项、黄金位及饰品标记，过滤实际Null配置与主动道具 | 不传原始时间／房间／池，不LoadImage；程序道具缺精确atlas／recipe时仍失败 |
| D0 356 | Screen原链正常IsPaused返回与同点Manager+4A8B8 | 没有真实查询、同批查询冲突或窗口变化时明确缺源，不额外调用IsPaused |
| 367／368／370 | Bloom、Pixelation、Hallucination的已核同窗纯值包络 | 跳过未知／未激活计数；分母或有限性非法失败，Host画质不改world意图 |
| 369／371 | Room正常481查询点立即复制本次临时BE7704和实际glitch结果 | Room Render尾部会清零，不能尾部补读 |
| BossOverlay379 | Game当前active与实际ANM2资源／姿态 | 不运行41CB40；第13步本地重建480×270视口比例 |
| SFX381—383 | 正常节流后实际选sample、最终pan尾叶及Options前gain／pitch／loop；独立实例与可靠事件 | 同sample重启退休旧实例；后端source为空不抹掉逻辑Start；非零pan有效分支仍缺通道证明 |
| SFX控制 | 正常AdjustVolume／Pitch、Stop、StopLoopingSounds、ClearVolumeModifier；ambient子Play关联实际改号 | 控制自有world实例，不额外IsPlaying；重试保持既有ambient实例，资源变更不明时报缺口 |

音频`mix_rule`新增至381，布局70→71：0对应Play／Adjust的本地Options后上限1，1对应正常ClearVolumeModifier无显式min。不能从含Host Options的后端值反除。当前固定512项逻辑声部、1024项可靠事件；one-shot保留控制关联但不放入持续声baseline，历史事件释放和重传水位由第11步实现，容量不足不能覆盖旧事件。音量类别和资源使用本地文件路径，不发送PCM或文件字节。

## 每个参与者自己的详细面板

本次会话明确要求：每个参与者显示自己对应的面板和道具获得历史栏。实现按实际participant／control-group／身体use、shape、binding与topology绑定缓存，原生Stat／History两槽不是C/S人数上限。每端在第13步选择自己的组，Jacob／Esau按实际身体关系显示；其他组公共健康HUD保持完整，位置／缩放／裁剪和History可见前缀在本地生成。

六项属性从Host已核最终值生产，仅生成显示结果，不恢复Client的Player玩法属性或EvaluateItems。自有差值更新遵循83DC60的150／135／30帧规则，倒计时随真正Stat调用推进；新实例和实际退休重置其自有历史。History逐身体倒序读取28B原始条目的isTrinket／item身份，并沿配置过滤生成320；不从当前库存counts推导获得史。

概率优先复用83CFD0本次flags证明实际写过的6／7／8行；不能仅因函数返回便采空身体／未更新行。HUD关闭只是不更新其最终缓存，游戏仍维护和查询概率世界状态。根据用户后续明确选择，缺少当前有效结果时直接复用Host的C++查询接口，集中查询一次供所有组使用：

- 原版Wiki `Room.md:168`：`Room:GetDevilRoomChance()`为天使／恶魔房共同总出现率，可能大于1且不包含全部楼层可生成资格；C++为`Room::GetDevilRoomChance()`。
- 原版Wiki `Level.md:93`：`Level:GetAngelRoomChance()`为天使房修正值，实际字段Game／Level+182F4；J273注册84D340绑定7DF730，该入口直接返回this[24765]，无需经过Lua。它不是最终面板百分比；只读该值并按7EF590／83CFD0的同义规则拆分，不重建整个概率生产系统。
- 原版Wiki `Level.md:187`：`Level:GetPlanetariumChance()`为0—1最终概率；C++为`Game::GetPlanetariumChance()`。官方`CustomDevilPlanetariumChance.cpp`还修正Telescope Lens可用性及禁止宝箱房的挑战资格，直接沿该现成查询取得结果。

`binary_probability.{h,cpp}`只承接已批准的C++查询和面板拆分，不经Lua API，不运行完整HUD Update／Recompute。原生总概率之外的楼层／出口资格由`hud_probability.cpp`沿743800只读投影，保留XL、alt、ascent、Greed、解锁与boss出口要求。输入在查询前后核相同当前窗口与具名值；变更不封存半份结果。C++查询保留官方包装语义；第三方回调改写仍不属于本轮原版范围，不能据此声称已支持任意Mod。

Host星象房406资格按9039B0的已核只读分支投影当前Host进度与实际daily／rerun许可，不查询访客进度。曾开始补充的完整自有概率公式已按用户选择移除，只保留资格及显示拆分。静态核对同时修正既有光照HasCollectible投影的native innate向量起点：9B5540的this来自Player+1FA4，其data在+0C，即Player+8112；原代码误读8128。该修正不改变其它P3归属或调用任何getter。

## 未完成的必要来源

以下是实际缺口，不以空列表或编译成功消除，也没有标成第五项完成：

- 原版程序道具History的实际atlas／recipe；315等剩余合成结果、普通映射的完整资源资格及全部必要组选择投影。概率接口接线已编写，运行正确性仍留第15步。
- HUD323的消息／fortune实际文字及字体来源、完整Minimap325与徽标／挑战32当前选择；瞬态alpha、药丸／预览及其它按次结果仍未全部接入。
- 373—378、380等全部正常贡献批次：两次Hallucination snapshot、实际Fade及加载图像、versus多层临时变换、Nightmare、GameOver、StageTransition遮罩、ItemOverlay、GenericPrompt文字等。
- 音乐Options前目标／全部内联writer与完整逻辑包络、并存组／六层／jingle，world用途及退休的完整分类，非mono有效pan、sound460内联后续控制和必要逻辑段落／恢复来源。当前SFX world判定仍限真实健康Authority模拟上下文，不能宣称所有正常菜单外声音已覆盖。
- 正常资源绑定／程序图／消息字体等完整内容来源和后续物化仍有缺口；第11步发布、第12步依赖校验、第13步adapter／本地资源owner与ACK均未因本次缓存实现而完成。

`ProductionInto`在必要身体来源或已登记world用途缺失时拒绝形成完整ordinary候选；Authority输入和原链继续运行。资源加载／替换进入即撤销相关HUD显示副本，历史P8保持真实冻结来源，不以新任意状态补造。

## 编译与失败记录

只执行`cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:1 /v:minimal /clp:ErrorsOnly`，E2E工具关闭，没有执行测试入口。实施途中出现过新增mix_rule后record_limit未扩展、混合宽度auto／局部名遮蔽、生成器显式ABI参数未传入及同RVA Play双wrapper目录约束的编译／生成问题，均已修正后续编译。新增C++概率SDK桥首次编译还暴露官方头在默认C++20模式下的C4596，已沿本仓库其他SDK桥相同规则将此TU限定为C++17。暂停前最后一次Release／Win32产品纯编译／链接成功，退出码0；没有性能、正确性或实机通过结论。
