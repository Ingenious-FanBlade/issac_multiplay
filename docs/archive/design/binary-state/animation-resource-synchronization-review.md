# 动画、资源与颜色参数的同步职责复核

日期：2026-11-09（Asia/Singapore）。范围：第 2／3 步的 `AnimationState`、`LayerState`、`ANM2`、`ColorParams`，以及引用这些布局的主／附属 Sprite。当前布局版本 **32**。本轮只修订设计、唯一布局源和生成表；未接通第 8—14 步的正式执行器，没有构建或测试。历史字节事实、失败及 UNKNOWN 保留。

## 1. 判定与修改

Client 安装 Host 的最终姿态，仍运行已允许的原生绘制；不运行玩法 Update，不根据动画事件发射、生成、伤害或推进状态机。字段是否同步取决于这条路径的真实消费者。

| 内容 | 结论 | 本轮处理 |
| --- | --- | --- |
| AnimationState `+18/+1C` 两组事件位 | Host 的事件消费／历史；当前已核 Entity 展示路径无需复制 | 撤销 wire；块 3 显式预留；Client 本地清零，不比较原值、不重放事件 |
| AnimationState `+14 is_playing` | **保留**。官方 REPENTOGON 绘制插值读取它 | 保留 u8；它是绘制输入，不能因此放行 Update |
| AnimationState `+10 anim_frame`、两组 selector、当前动画选择 | **保留**。帧值和独立 selector 不能互相替代 | 不裁剪、不用 SetFrame 重算并覆盖逐层选择 |
| ANM2 `+104 playback_speed` | **保留**。官方绘制插值用它计算半帧比例 | 保留 f32，不据此在 Client 自行播放 |
| ANM2 `+109 loaded` 原始布尔字节 | 本地构造／加载结果；可渲染资源的有无仍须表达 | 撤销独立 wire／validator；并入已有 `file_name` 的可空资源号，见第 3 节 |
| LayerState `+30 sprite_sheet_loaded` | 本地加载／待重载标志，并非图像选择本身 | 撤销 wire／validator；实际 Image 与路径继续分别同步 |
| 文件、动画、图像、shader 的选择与图层外观 | 决定可见内容，不能当缓存删掉 | 保留稳定资源身份、绘制值和已核独立 selector；地址、控制块、旁表 key 不发送 |
| Entity `+1AC` 的 ColorParams 队列及六组元素输入 | Host 颜色覆盖／淡出合成时间线；Client 不重演合成 | 容器头及元素都不发送，Entity E7、ColorParams 块 0 显式预留；撤销颜色 lifespan 锚点及专用读取时钟 |
| Entity.Color、ANM2.color／champion_color、LayerState.color | 最终绘制颜色，**保留完整 ColorMod** | 11 个 f32：tint RGBA、colorize RGBA、offset RGB；不缩成普通 RGBA |

不把 `is_playing=false` 当成没有动画，不把 `loaded=false` 当成暂停播放，不把空 Image 当成 `visible=false`。这些含义各不相同。默认动画名的已有有界串此次未裁剪；其可写事实仍保留，不宣称可由文件无条件派生。Sprite／LayerState 的公开 owner 尚未完整接管，保持原来的未完成状态。

## 2. 动画事件与播放控制依据

目标 IDB 为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`。下列均为 VA，RVA = VA − `0x400000`；本轮没有改动或保存 IDB。

- `AnimationState::IsEventTriggered 0x406710`、`WasEventTriggered 0x4067B0` 分别读取 `+18/+1C`。`AdvancePosition 0x4041A0`、更新器 `0x404480` 维护播放与事件历史；这些 getter 的存在不证明 Client 必须拥有原值。
- `ANM2::Render 0x4059E0` 检查 `+109`，按 `+70 overlayRenderFirst` 分派主／叠加 `AnimationState::Render 0x405990`。后者读取当前 AnimationData 和逐层 selector，再调用 `AnimationLayer::RenderFrame 0x404570`，没有消费这两组事件位。
- 本轮从公共 Render／Shadow／PostRender `6AC4E0/6AD450/6AC660`，Player `784A60/7862A0`，NPC `6CEC90/6D0650`，Tear `675AC0`，Projectile `659460`，Pickup `6E35D0`，Familiar `61E360/61E320`，Effect `600E60`，Knife `93ECD0`，Bomb `6A0F90`，Laser `9514B0/955290/956F80`，Slot `6620E0`，Text `5CC580`，Dummy `6A3630` 和 Room Light `7FC110` 检索四层直接调用，涉及 436 个函数，没有通向上述两个事件查询或 Entity::Update 的直接调用路径。这是有界静态依据，**不是虚调用／全部 HUD／未来 Mod 的全程序闭包证明**。
- 旧 `client_resources_native.cpp` 的 SetPose 已在安装最终姿态后清零四个主／叠加事件掩码，其姿态后置检查也要求事件位为零。这是已有 Client 行为，不是新增实机结论。
- 官方 `REPENTOGON/Patches/InterpolV2.cpp` 的 `AnimationLayer::RenderFrame` Hook 在第 34 行读取 `_interpol_state->_isPlaying`，随后以 `_animFrame`、帧起点／duration、`animation->_playbackSpeed` 和 Manager 帧奇偶计算插值。主线程冻结 Update 并不移除该 Render Hook。因此 `is_playing`、`anim_frame` 和 `playback_speed` 继续保留；不得把它们一律归 Host 控制输入，也不为删字段而禁用官方绘制。

Client 构造、换资源或切动画时，由本地安装 owner 清理事件位；不得在采集／漂移路径把它们恢复为 Host 历史。不可靠状态帧中的位掩码本来就不能保证每次声音／闪光事件到达，真正的一次性音画事件仍按第 8 步的事件通道处理，不以掩码代替事件序列。第 5—8 步接入尚未核定的 Player／Grid／HUD owner 时仍须审阅其实际绘制消费者；发现依赖则补最终表现输入或具名适配，不能以本轮有界阴性检索声称那些 owner 已覆盖。

## 3. 资源选择与本地维护必须分开

### ANM2

`Load 0x407AF0` 在 `0x407B52` 写 `+109=1`；`Reset 0x403530` 清文件名、动画和图层等资源，并用覆盖 `+108/+109` 的 WORD 写零。Render 确实受 loaded 影响，不能只因它由加载器写入便抹去“有／无资源”的语义。

版本 32 把这一语义合并到已有 u16 `file_name`，不再另传布尔字节：

- Host 同一合法窗口内读取 ANM2 `+109`。false → `kNullIndex=0xFFFF`；true → 解析实际文件资源号，无法识别或资源不完整时报告 Gap，不能用 Null 掩盖失败。**不以路径是否非空猜 loaded**，也不要求所有历史／异常对象都满足两者等价。
- Null 是 Host 没有选择可渲染 ANM2；Client 通过资源 owner 安装已构造的空对象，正常退休旧资源，不能析构／裸清零内嵌 ANM2。
- 非 Null 表示目标资源存在。Client 等待、加载失败和加载成功是本地准备状态。只有资源、动画、图层、实际 Image 及姿态全部安装后才发布／回复 AppliedAck；不能直接写 `_loaded=true` 冒充成功，更不能将失败改成 Null 后 ACK。
- `anm2_selected_file_to_resource/from_resource` 已在布局源声明合同，具体执行器仍归第 8／10／13 步；它们尚未实现，不能沿用只读路径字符串的旧 handler 后声称接管完成。E4／E5／E6 及附属 Sprite 各投影须按同一目标资源版本提交。

`Sprite.IsLoaded` 的回映射改为成功应用后的资源选择／本地准备结果派生。它不要求加载过程中的跨进程原始字节相等。

### LayerState

- 官方 `LayerState.zhl::SetSpriteSheet` 替换 `+98` 的 Image smart pointer，按图像更新 filter／wrap，并令 `+30=(image != nullptr)`；不写 `+08` 的路径字符串。
- 更关键的反例是 J273 `ReplaceSpritesheet 0x407CD0`：更改路径后，在 `0x407D53/0x407D69` 令 `+30=0`，此时旧 Image 仍可能保留。`LoadGraphics 0x407DA0` 再根据该标志决定重载，并更新实际 Image 和标志。所以 **Host 的 +30 既不等价于 path 非空，也不总等价于 image 非空**。
- `RenderFrame 0x404570` 经 `GetSpriteSheet 0x403BE0` 取得实际 Image；该 getter 按 LayerState 的 filter／wrap 懒同步 Image，再返回 smart pointer，没有检查 `+30`。因此重现当前画面需要实际 Image 选择，不需要重演 Host 的待重载位。

Client 安装实际 `image_selection` 与路径元数据，保留 filter／wrap、变换、颜色、混合模式、可见和 render flags；本地加载标志交资源 owner 维护。路径改变而 Host 仍画旧 Image 时，Client 也保留对应图像，不能仅因路径变化调用 LoadGraphics 抢先换图。Host 实际 Image 变更后才替换相应本地引用；Null 图像是合法目标，Client 加载失败不等于这个 Null。地址、refcount/control block 不发送；Image 名字重名／程序图像身份与 shader 旁表接线仍是第 8 步真实未决，未被本轮解决。

## 4. ColorParams 只在 Host 合成，最终 ColorMod 全量保留

官方本地 `docs/docs/ColorParams.md` 与 [ColorParams 文档](https://repentogon.com/ColorParams.html)区分 Duration 和 Lifespan：前者参与淡出比例，后者是剩余更新次数。它们的原生语义保留，不能因不传输便改写成无语义字段或填充。

J273 直接链如下：

1. `Entity::Update 0x6A9CF0` 在 `0x6AA10E—0x6AA163` 遍历 `+1AC` 队列，对元素 `+34` Lifespan 无条件递减，仅结果为零时移除；负值也继续减。
2. `0x6AA18E—0x6AA211` 选择覆盖颜色，使用 `+38 fadeout`、`+30 Duration` 和 `+34 Lifespan` 计算颜色。`0x6AA198` 可取 Entity `+1B8` 的基础颜色参与合成，因此 **Entity.Color 不能独自替代主 Sprite 的合成结果**。
3. `0x6AA21E` 取 `Entity+48` 主 Sprite，`0x6AA221` 调用 `0x4029B0`，后者把 44B ColorMod 写入 `ANM2+0xA8`。Render 使用的这个结果已经在 E4 的 `sprite#0.color` 中发送；Entity `+1B8 Color` 也继续单独发送。
4. 上述 436 函数的有界直接展示调用集合没有 `+1AC/+1B0/+1B4` 队列头访问候选。旧 `client_entity_model.cpp` 已把 `Entity.GetColorParams` 定义为 Host 时间线、只安装当前 Color、不本地推进。

因此整个 ColorParams 队列无需同步；priority、duration、lifespan、fadeout、shared 及其中 color 都作为 Host 合成输入归权威。Client 不重建、倒计时、删项或 CopyStatusEffects 来生成画面；已有本地空容器的正常构造／析构仍由 owner 维护。ColorParams 六个 getter 和 Entity.GetColorParams 回映射为 authority，不要求 Client 重建 Host 这些 API 原值。旧观测和第 3 步有符号计数证据保持事实，但颜色锚点、专用读取时钟和两个颜色容器 handler 不再属于正式同步。

这不意味着颜色只要四个数。`ColorMod` 的 tint[4]、colorize[4]、offset[3] 分别参与乘色、着色与加色，J273 `0x67E940` 分组处理全部 11 个 float，`RenderFrame 0x404570` 合成 Sprite／图层／帧颜色。最终 Entity.Color、Sprite.color、champion_color 和 LayerState.color 均保留其完整 44B，不假定某组恒零或从另一组派生。

## 5. 联动、体积与验收边界

- 唯一布局源 `base.json/entity.json` 与整数版本改为 32；派生／附属 Sprite 自然继承新职责，不维护第二份裁剪表。AnimationState 块 3、ColorParams 块 0、Entity E7 保留为空，不前移后续块号。UNKNOWN 数量不变。
- 每个 AnimationState 少 8B，最小值由 19B 降为 11B；每个 ANM2 的主／叠加事件位共少 16B、独立 loaded 少 1B，每个 LayerState 少 1B。Entity E7 还去掉 2B count 和每个旧 ColorParams 元素的 62B 线上投影。Entity 公共最小负载由版本31的 410B 降为 **391B**；这是布局计算，不是实测带宽或性能结果，图层／辅助 Sprite 的数量仍决定实际节省。
- 第 13／14 步接线时退役旧事件位原值比较、loaded 直接写入／相等要求和 ColorParams 时间线采集／重建／漂移比较；保留最终姿态、完整颜色、实际资源与正常本地 owner。官方绘制插值的临时资源帧改写／恢复仍须纳入完整绘制生命周期，不能靠删播放字段或放行 Update 处理。
- 第 15 步唯一统一 E2E 覆盖：主／叠加／逐层／空层姿态；播放／停止与不同速度的官方绘制插值；空资源↔非空资源及 Client 等待／加载失败；路径先变但仍保留旧 Image、实际图像随后替换和空图像；重叠颜色优先级、淡出、正／负 lifespan、shared 传播后的最终 Entity／Sprite／图层／costume 颜色；全部 11 个 ColorMod 分量。确认不重放事件、不运行本地颜色合成，失败不 ACK。

第 5—14 步执行器、资源身份未决与唯一功能整体验收仍未完成。此次仅完成静态资料核对与文档／布局职责修订，不宣称上述场景已实机通过。
