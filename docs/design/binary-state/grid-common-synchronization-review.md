# Grid 公共部分同步职责复核

> **版本53子类收窄**：[PressurePlate／Door](grid-pressure-door-evidence.md)用224描述排除本类无临时Light RNG消费者的SpawnSeed，G0为20B；公共198及其它Grid仍按本文合同。Door主颜色改225/226，仅发覆写前的alpha正值入口布尔，其余从同world墙色提供；extra仍201保留完整颜色。Pressure主202用地板色、Timer201保留本色。本文版本50的公共事实保留，不能以旧通用Sprite再恢复这些子类已裁原值。

日期：2026-11-10（Asia/Singapore）。范围为第 6 步公共 `[0,0x154)` 及其主 Sprite／图层展示视图。最终布局版本 **50**；版本49完成首批裁剪后，继续按GPU实际分量核对并裁掉冠军色载体后七值。本轮只做静态分析、布局修订、生成与允许的纯编译，不运行测试、游戏、校验脚本或修改 IDB。原[版本48字节证据](grid-common-evidence.md)保留其原始发现及编译记录；本文取代其中的 CollisionClass 直接传输和完整通用 ANM2 复用决定。

## 1. 判定标准与失败方式

Client 接收最终世界与展示状态，并执行已允许的原生绘制；不运行 Host 的物理／碰撞／奖励／Init／Update／随机选型。公开 getter、旧目录曾发送、字段有名字或不能从别的字段推导，都不单独构成同步理由。

本轮针对的失败方式：

- 只看到中间原生 getter 读取 CollisionClass，漏看 C/S wrapper 会覆盖最终输出，导致把玩法碰撞原值写入 Client。
- 不传 CollisionClass 后留下工厂未初始化的 +3C，使 original-exactly-once 的 Lock 查询读到未定义本地值；必须由本地构造 owner 设置中性 0。
- 不理解 REP nullable 覆盖，把无 override、有值 0 与无法读取混同；最终值应按 Host 实际类型和来源合成，缺来源报 Gap。
- 为复制 Sprite 的 API 原值而发送默认动画名／待加载图层路径，甚至在 Client 调用 LoadGraphics 提前切到 Host 尚未实际使用的图像。
- 把 loaded、指针、控制块、事件位或 UNKNOWN 连同 ANM2 整块复制；或者因“停止 Update”错误删除绘制插值所需的 phase／is_playing／speed。
- 继续让 Dataminer 的纹理故障方向取决于 Client 本地 ANM2 地址，或反过来传输 Host 指针／重放玩法 RNG。
- 按 18 个 RTTI 的布局号猜 type，或把成员表中的不透明 id 当成 grid_index，删除仍需在线表示的结构／几何信息。

## 2. 公共原生字节的最终决定

| 对象偏移 | 字段 | 决定 | 具体依据 |
| --- | --- | --- | --- |
| `+000` | vptr | **不应同步**：本地地址／分派 | 用真实本地工厂／RTTI 选布局，不写网络虚表。 |
| `+004` | desc.type | **同步** | Room 的 pass／Light 用 type，Rock 一个 RTTI 对应多种 type，不能从布局号唯一还原。`801C24`、`7149E0` 等。 |
| `+008` | desc.variant | **同步** | Decoration 的隐藏、Poop 特殊绘制、Rock 碎块与 PressurePlate 文字、Light 都有真实分支。 |
| `+00C` | desc.state | **同步** | Fire ember 绘制、Rock 碎块、Poop 下层 pass 与 Room Light 消费；水面结果改为最终值并未消除这些其它消费者。 |
| `+010` | SpawnCount | **不应同步**：Host 初始化／随机选型 | Init 增量、Fire Init 选择随机变体，Client 不重演生产过程。 |
| `+014` | SpawnSeed | **同步** | Light `801BFF/801C07/801C0D` 用此值加同 world Host GameFrame 建立栈上绘制 RNG。它不是对象 RNG 游标，也不能由 identity id 代替。 |
| `+018` | VarData | **不应同步**：Host 玩法输入／进度 | 伤害奖励／TrapDoor Update 消费，最终状态和画面另传。 |
| `+01C` | Initialized | **不应同步**：Host Init 资格 | 来源资格不能覆盖 Client 自己的工厂／资源／发布状态。 |
| `+01D—01F` | 间隙 3B | **不读取、不传**：UNKNOWN | 没有语义或填充证明。 |
| `+020` | VariableSeed | **不应同步**：Host 玩法保存种子 | 不等于 Light 使用的 SpawnSeed，不在 Client 建立玩法随机。 |
| `+024` | grid_index | **同步** | 绘制世界位置／屏幕换算／Textured Pit 需要。第 9 步成员表只有不透明 run/id/use/shape 等，没有可还原的格子序号；门槽映射只覆盖 8 槽，也不能覆盖所有 Grid。 |
| `+028` | SpawnFrame | **不应同步**：Host Update 帧基准 | TrapDoor Update 比较 Room 帧基准，当前展示不消费。 |
| `+02C—03B` | 对象 RNG 四字 | **不应同步**：Host 玩法随机状态 | 不进入 Client；Light 和故障绘制的纯值选择分别处理。 |
| `+03C` | CollisionClass | **撤销直接同步，归 Host** | 上轮保留理由只到 Lock 原生 WaterClip，未追踪 C/S wrapper 最终输出。改为发送下文最终 `water_clip_flags`；Client 本地 owner 将此字段初始化为 0，不重建 Host 碰撞。 |
| `+040—153` | 主 Sprite | **按展示视图同步** | 复用已核字节事实，使用 GridSprite201／GridLayerState200 收紧职责，不发送整个 ANM2 原始内存。 |

原生公共 UNKNOWN 仍 **17B**，没有新证据把它们变成已知／填充。图层数组在对象之外，GridLayerState 每元素原有 4B UNKNOWN 也保留，不混入根对象的 17B 计数。

## 3. 水面裁剪只传最终结果

当前真实 C/S 路径为 `mod/src/game/client/grid/client_grid_native.cpp:ClientGridWaterClip`，六个常驻 wrapper 的生成逻辑在 `mod/tools/gen_binding.py`：

1. 原链 `super(out)` 调用一次；
2. `ClientGridWaterClip(this,result)` 在同一 Client owner 中，按**实际实例**把 `bitFlags` 改成 `Entry.description.water_clip`；
3. Grid Main pass 与原生 Scene／反射消费者取得的是此最终值。

因此 `70CCE0` 读取 collision 只证明 **Host 原生计算输入**，不证明 Client 必须复制该输入。四字节合成字段 `water_clip_flags` 进入 G0，标记 `FieldSource::Synthetic`，没有游戏对象偏移，不能对 +3C 写入它。完整 u32 保留；REP setter 接受任意 uint32_t，不能沿用旧 JSON 的 0—255 限制。

Host 按已核的**实际 RTTI／虚表族**读取必要输入，在 `grid-current-window` 同窗合成，不额外调用 Lua／原生查询、Init 或 Update：

| 已核原生函数族 | 原始结果 |
| --- | --- |
| `705250`：Grid／Statue／Wall／Fire／Spikes／TNT／Gravity | 57 |
| `70F120`：Pit／Teleporter／TrapDoor／Web／Stairs／Decoration | 38 |
| `70C360`：Door | 0 |
| `95E940`：PressurePlate | 25 |
| `70CCE0`：Lock | collision==0 ? 38 : 57 |
| `711620`：Poop | state==1000 ? 38 : 57 |
| `718650`：Rock | state==2 ? 38 : 57 |

**版本54纠正版本50的概括**：上述六族应用官方 `GridEntityPlus.cpp/.h` 中按 index 共享的 nullable `waterClipInfoFlagsOverride`，无覆盖保留原值，有覆盖采用完整 u32（包括0）；**Lock不应用覆盖**，它的独立slot6没有调用基类，官方六Hook也不含Lock。按[旁表来源、ABI与生命周期核定](grid-plus-water-source-evidence.md)，官方1.1.3 DLL的448×12B数组在RVA62E650，先读slot+8 engaged，仅为1时读+4 payload；模块／索引／当前窗口失效报Gap，Lock不读旁表。Factory返回后和Init入口清同index共享项，Free／换房不假定自动清。未知类不能按desc.type猜规则，optional／填充／vptr／地址都不发Client；静态来源已核，实际reader仍待第10步。

Client把最终值安装进自身的per-instance owner；同index门与装饰未覆盖时仍有不同默认，Lock一直按自己的collision决定，不能共用index→final缓存。+3C先由本地owner写中性0，保证original有定义且不放行碰撞／Room Update。**现有六wrapper只能复用六族，第13步必须经统一清单补Lock第七覆盖点**，各链一次后覆盖bitFlags；不能靠写Host碰撞或官方共享表补缺。七族out+4 float由原链置0，不下发。正式reader／writer仍归第10／13步，本轮未切换旧JSON路径；先前“现行wrapper足够”的结论以本段纠正。

静态补核了 Room::GetGridCollision `7E6AF0`：有 Grid 时确实返回 +3C；但由 17 个 Grid／Light／batch／Textured Pit 绘制根向下四层的 208 个直接调用函数中没有到达它的路径。虚调的七个 WaterClip 族另行逐个核对。该有界依据不冒充全程序证明；后续发现新的已支持显示查询须接具名最终结果，不据旧 API 回读恢复全部 Host 物理状态。

## 4. ANM2／图层的保留与裁剪

`GridSprite`（201）与 `GridLayerState`（200）是主 Sprite 的场景视图，原生大小分别为 0x114／0xA0，复制第 2 步已核字节事实及原证据等级。通用 ANM2、其它 Entity／Player／NPC 的布局及行为保持各自现行合同；本轮不把局部结论扩大成通用资源 API 完成。

| 内容 | Grid 视图决定 | 依据 |
| --- | --- | --- |
| 可空 ANM2 文件身份、当前主／叠加动画资源 | **保留稳定资源号** | 用于创建正确资源、解析 layer／null-frame／animation；地址不发。 |
| 默认动画名 `ANM2+18` | **撤销传输，归 Host 控制元数据** | 可写不等于展示需要。Load／Reset 维护它；当前姿态有明确动画资源选择。`405F50 SetAnimation` 和 Rock `7146E0` 的显式 RubbleAnim 切换按传入名字查动画，不读取默认名。Client owner 只允许指定动画，不按来源默认名重新选动画／播放。不是“不可变／可由文件必然推导”的结论。 |
| 各层 spritesheet_path `LayerState+08` | **撤销传输，归 Host 待加载元数据** | `407CD0` 可先改路径、令 loaded=false 而保留旧 Image；真正绘制的 `403BE0 GetSpriteSheet` 只读实际 Image 和 filter／wrap。`407DA0 LoadGraphics` 才按路径重载，Client 不重演此生产请求。 |
| 各层实际 Image、filter／wrap | **保留** | Native GetSpriteSheet／batch 与官方 `Anm2Extras.cpp` 直接消费；不能用文件默认图像替代 Host 最终实际图像。 |
| 主／叠加独立层选择器、null-layer 选择器 | **保留** | Render／Light 直接读逐层或 null-frame；不能只发一个全局帧再 SetFrame 重算。 |
| anim_frame／is_playing／playback_speed | **保留** | Door Light 使用当前动画／帧；官方 InterpolV2 的 RenderFrame Hook 用三者计算半帧插值，停止 Update 不移除这些绘制消费者。 |
| offset／scale／rotation／flip、overlay 顺序 | **保留** | Native source/dest quad、主／叠加绘制顺序直接使用。 |
| Sprite／Layer 普通 ColorMod、flags／blend／crop／visible | **保留** | 官方 Anm2Extras 与 ANM2Utils 及 J273 `404570/404F60/405360` 有正面消费者；普通颜色仍为完整11个float，不因冠军载体裁剪连带缩成RGBA。 |
| champion_color 前16B／后28B | **只保留前4个float，后7值归Host** | J273 `404570` 仅把ANM2+D4/D8/DC/E0四值写入GPU顶点。官方 `ShaderUtils.hpp::ColorOffsetChampion::FillVertices` 同样只把championColor._tint[0..3]提交为vec4；参数名为ColorMod不代表其colorize[4]/offset[3]有消费者。GridSprite在+D4直接复制f32[4]，+E4—FF以具名f32[7]保留原生事实但不上线，不改通用ColorMod。 |
| loaded 原字节、动画 owner／layer_data／控制块、缓存表指针 | **不应同步** | 交 Client 资源／构造／退休 owner 维护。Null 资源与加载失败仍分开，不能裸写 loaded=true 或 memcpy string／智能指针。 |
| 两组主／叠加事件位 | **不应同步** | Host 动画事件消费／历史；Client 构造／换动画时本地清零，不重放。 |
| Root／图层 UNKNOWN | **不读、不传** | 保留原位置、大小及证据等级。 |

Room 会在部分 Grid Render 中覆盖 Sprite 颜色，但 Lock／Teleporter／Fire 直接用原色，Rock 会乘合成，Door 还先检查 alpha，Wall 的覆盖有条件。不能在公共视图中一律删除 color 或用 Room floorColor 代替所有实例颜色。

## 5. Dataminer 故障方向是遗漏的最终展示值

这不是要求传递地址或玩法随机。J273 Room Render `804500` 的 `8045FB—804612` 在 Room TemporaryEffects（+7310）存在 collectible effect 481（Dataminer）时置 `byte_BE72FC`，到 `807370—807388` 才撤销；Grid 绘制位于该房间绘制范围。Native `404570` 的 `4047FE` 检查此开关，再用 ANM2 地址作为本地 RNG 的种子；官方 `Utils/ANM2Utils.hpp::GetFrameSourceQuad` 也直接写了 `rng.SetSeed((uint32_t)&anm2,35)`、两次 `RandomInt(2)`，得到 ±90° 和水平翻转。两端同内容、不同地址不保证同结果。

GridSprite 的 G1 增加 1B 合成字段 `glitch_draw_bits`：bit0=第一次结果（0 对应 +90°，1 对应 -90°），bit1=第二次结果（1 翻转 X）；合法 0—3，其余位拒绝。Host 只在自身纯值 RNG 中按已核的地址种子／shift-index35规则取得两个**最终选择位**，不调用游戏 RNG，不读取／推进对象或共享 RNG，不把地址作为网络身份或传输地址。结果属于同 subject/use/shape 与 Sprite 角色，地址复用／资源角色变化撤销旧关联。

Client 在具名 RenderFrame／source-quad adapter 中消费这两个最终位，并使用同 world／同房间的实际 glitch pass 开关；不能再让官方／原生分支额外按 Client 地址抽一遍，不能用目标 quad 的旋转或 Sprite.FlipX 冒充纹理 source-quad 变换。原版及 REP 绘制两条路径、DataMiner 开关和临时状态精确恢复均属第 8/13 步实际接线；所需 Host 源、角色关联或 pass 不明时报 Gap。辅助 Grid Sprite 的相同问题仍须子类阶段逐个采用正确视图，本公共工作不宣称它们已覆盖。

## 6. 布局与后续边界

- Desc198 的四项 16B 不变，Grid199 的 G0 仍 24B：desc16＋index4＋最终 water_clip_flags4；CollisionClass 原值退出线状态。
- 主 Sprite 改为 GridSprite201，图层改为 GridLayerState200。G1 最终为固定 **94B**：原最小123B去掉默认名u16 count（及其变长字节），增加1B glitch最终选择，再去掉冠军色载体未消费的28B；G2最小22B、G3空数组2B。公共最小状态 **142B**，每个实际图层另少2B路径资源号。版本49中间结果为G1=122B／公共170B，不能再作为当前大小。实际图像、资源与未知计数不因裁剪减少。嵌套reader／writer必须按生成的nested_layout201／200递归，不能因复用通用handler名称又按布局7／6硬编码旧字段；冠军参数仅写前16B，不把线上16B当作44B ColorMod读取。
- 正式 Host reader／Client writer／绘制 adapter 仍待第 10/13 步；本轮只完成布局与职责修订，不表示新版同步已能运行。旧目录中 GetDefaultAnimation、GetSpritesheetPath、CollisionClass 的原值相等与旧八项 Desc／SpawnFrame 比较在第 14 步退出，不能为兼容回读恢复传输。
- 第 15 步才执行原 Grid／E09／E25／E27／E28／E29／E13／E17：覆盖最终裁剪值（包括有覆盖0、高位u32及同 index 多实例）、本地 +3C 初始化与原链各一次、图片路径先变而实际 Image 未变、显式动画／独立选择器、Dataminer 两端不同地址与开启／关闭／重建／退休、冠军前四值着色及普通颜色11值、临时状态恢复和全部 UNKNOWN。冠军16B截断／非有限值应拒绝，后28B不在网不能再构造其相等／非法负载测试。不运行负载已撤销字段的旧相等测试。

版本49已由现有CMake普通构建生成布局表并通过Release／Win32／v143的`isaac_cs_state`纯编译，得到中间公共170B。随后继续核对GPU实际分量，版本50将冠军后28B移出；最终版本50也已通过相同状态库纯编译，生成结果为G0=24B／G1=94B／G2最小22B／G3最小2B，共142B。+3C已无直接复制项，两项合成值明确为Synthetic／无原生偏移；冠军只有前16B直接复制／四值finite校验，GridLayerState每层117B，根与元素UNKNOWN保持。没有运行测试、游戏或校验脚本，没有构建完整DLL或部署，没有提交，也没有单独保留artifact。这些结果不代替第15步唯一运行验收。
