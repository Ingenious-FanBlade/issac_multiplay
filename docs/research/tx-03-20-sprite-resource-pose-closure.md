# tx-03-20：具体 Sprite 资源与连续姿态闭环

日期：2026-10-23（Asia/Shanghai）。承接 [tx-03-19 的绘制窗口](tx-03-19-sprite-render-entry-window.md)，本轮选择具体 ANM／PNG 文件样本，解释同一个 Authority 完整采集窗口、同资源姿态更新和资源变更时的正常顺序。GPT-6 Luna / max 研究采集／更新合同，主智能体独立复核，并研究本地资源、路径合成、native parser 和 owner 应用，见 [Luna 报告](tx-03-20-luna-pose-capture-update.md)和 [主审记录](tx-03-20-primary-review.md)。

本轮为静态逆向、现成资源文件读取与适配设计；没有修改游戏资源、解包、执行 PNG 解码、原生加载、游戏／调试、构建或运行代码测试。读取 XML／PNG 文件头的脚本仅导出资料，没有 expected-result 断言或通过判定；最后只做一次整体静态文档验收。

## 结果

本轮已收敛一个具体单 Sprite fixture：**`gfx/1000.011_Bullet Poof Static.anm2` 的 `Poof` 动画**，对应 `gfx/effects/Effect_003_BloodTearStatic.png`。读到的 XML 只有一个 layer、一个 animation、15 个 delay=1 的关键帧，无 null／event／trigger／插值；PNG 文件头为 256×256、8-bit RGBA、非交错。它适合进入受限独立 Sprite 的实现，不代表 Effect Entity 或完整世界外观已支持。

同一个本地资源、main／overlay descriptor 和数组关系保持稳定时，**每次完整姿态可在自有 Sprite 上原地写入完整 key 向量及外观，不再 Load／reset／SetFrame**。资源或 descriptor 改变则准备新候选、检查后切换关联，再退休旧实例。客户端不 Update，也不消费 native event masks；它显示 Authority 已采到的姿态。

Authority 端在现有完整 `Capture` 的同一个 batch 内追加只读姿态，在最终稳定性复核／FinishBatch 前完成；当前代码尚无这项扩展。现有 Entity 基类 draw 的内嵌 Sprite 位于 +48，但资源样本不自动获得 Entity 生命周期资格。采到的是声明的持久 Sprite 姿态，并非把所有 Entity 绘制临时修饰都还原了。

以下 `STATIC_CONFIRMED` 指静态文件／指令／源码直接事实；`STATIC_DERIVED` 指有限正常顺序推导；支持声明、DTO、owner 窗口及更新算法为 `DESIGN_PROPOSAL`。实际资源命中、native descriptor、线程／context、姿态、像素和 GPU 结果均未运行观察。

## 证据

活动 IDB：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，input path 为同目录 EXE，imagebase=`0x400000`；`RVA = VA - 0x400000`。对象偏移、虚槽 offset 和地址各自解释。没有比较磁盘文件身份，没有计算或校验任何哈希。

| 本轮资料 | 范围 |
| --- | --- |
| [sample-resource-observation.json](../../artifacts/cs-t0/tx-03-20-static/sample-resource-observation.json) | 现成 ANM XML 元数据／逐 key 属性、PNG 前29字节／IHDR、entities2.xml 中一条配置关联 |
| [sample-support-declaration.json](../../artifacts/cs-t0/tx-03-20-static/sample-support-declaration.json) | 手工命名的单 Sprite 支持声明、预期 native shape、显式初始姿态及更新／重建规则；不是 wire 或运行状态 |
| [primary-resource-binding.json](../../artifacts/cs-t0/tx-03-20-static/primary-resource-binding.json) | ANM parser／cache bind／Load、layer 初始化、逐层图像处理与路径 resolver |
| [primary-sample-parse-detail.json](../../artifacts/cs-t0/tx-03-20-static/primary-sample-parse-detail.json) | 相对路径 helper、key选择／draw／reset／动画选择及 PNG decoder 静态分支 |
| [primary-pose-owner-review.json](../../artifacts/cs-t0/tx-03-20-static/primary-pose-owner-review.json) | 主审独立采集 update→render 来源、Entity draw／临时修饰、Sprite Update、资源加载及正常清理 |
| [primary-png-profile.json](../../artifacts/cs-t0/tx-03-20-static/primary-png-profile.json) | PNG loaded／不同属性、indexed 判定、image draw 与 PNG 24 槽表；不把相邻 RTTI 当函数槽 |
| [primary-capture-source-v2.json](../../artifacts/cs-t0/tx-03-20-static/primary-capture-source-v2.json) | UpdateHook、Authority witness、完整 Capture／尾部复核和 Player 采集范围的一基源码摘录 |

只读源为 [export_static.py](../../artifacts/cs-t0/tx-03-20-static/export_static.py)和 [read_resource_metadata.ps1](../../artifacts/cs-t0/tx-03-20-static/read_resource_metadata.ps1)，输出均 create-new。初次源码摘录的单范围数组被展开，原始 [primary-capture-source.json](../../artifacts/cs-t0/tx-03-20-static/primary-capture-source.json) 保留，v2 修正其 loop 段；该采集问题、最初 rg 路径错误与本地目录发现范围见主审记录。没有以资料采集修正替代或重跑测试。

## 1. 具体资源与来源范围

本次搜索从游戏安装根目录发现已有 `extracted_resources/resources`，它位于先前只搜索的 `resources` 目录之外。tx-03-18 的“该次搜索未得到散装文件”仍是当时真实结果，不改写为全安装目录都没有资源。本轮直接读取已有文件，没有调用解包工具或修改搜索路径。

| 项目 | 直接读到的内容 |
| --- | --- |
| ANM 文件 | `extracted_resources/resources/gfx/1000.011_bullet poof static.anm2` |
| 配置逻辑名 | entities2.xml 第5336行：id=1000、variant=200、anm2path=`1000.011_Bullet Poof Static.anm2`；根 anm2root=`gfx/` |
| layer／sheet | layer Id0、Name=`head`、SpritesheetId0；sheet 路径=`effects\Effect_003_BloodTearStatic.png` |
| 动画 | 默认 `Poof`、FrameNum15、Loop=false、Fps30；root 一个恒定帧、Delay15 |
| XML keys | 15 个，每个 Delay1、64×64、position(-32,-32)、pivot(0,0)、scale(100,100)、Visible=true、RGBA tint255、offset0、rotation0、Interpolated=false |
| crop 序列 | X=0/64/128/192 按行变化，Y=0/64/128/192；最后一格为(128,192)，末行只使用三格 |
| PNG 文件 | `extracted_resources/resources/gfx/effects/effect_003_bloodtearstatic.png`，IHDR=256×256、bit depth8、color type6、interlace0 |
| 排除项 | XML 没有 Null／Events／Triggers；layer 名称不以 `*` 开头，也不名为 shadow；这只解释该文件声明，不证明运行对象后来没有被改 flags |

PNG 资料只读取文件头，未验证 CRC 或任何摘要，未解码 texels；尺寸与格式也不证明 driver 上传或画面正确。配置关联仅用于说明文件来源，**不调用 Spawn，不扩充现有七类 Entity 支持，不声称 Type1000／variant200 的 native lifecycle 已闭合**。首个实现目标是自有独立 Sprite fixture。

逻辑 ResourceKey 候选 `tx0320.poof.static.v1` 和 revision=1 由本地清单命名，不是内容哈希。两端相同名字／元数据不证明任意文件字节或像素相同；固定可信资源部署仍是后续实现的声明范围，不能由此宣称跨版本／Mod 兼容。

## 2. 从 XML 路径到 native 资源关系

`40ED50` 在 `40EF89` 调 `67F670`，以 ANM 名和 sheet Path 合成图像路径。该 helper 分别将两条输入中的 `\` 改为 `/`（例如 `67F717/67F71C`、`67F7D7/67F7DC`），拆除 ANM 文件尾项、处理相对目录并合并输出；当前已读指令不在这里做统一大小写折叠。本样本正常合成的逻辑名为 `gfx/effects/Effect_003_BloodTearStatic.png`，不是把 sheet 路径误当安装根的 `effects/...`。

这只是合成路径的静态推导。ANM bind 还使用当前 prefix/cache 关系；`9588A0` 把实例路径交给 `8F5AD0`，后者按已注册来源／Mod 路径查找并缓存解析结果，再进入图像 loader。读取 Windows 文件成功不证明 active game resolver 命中了同一 extracted 文件；不能将 extracted 目录自动视为游戏已注册的资源根，也不据文件名大小写差异修改原生 string 或缓存。

`40ED50` 会生成插值／root-frame 调整后的 native key records，诊断分支包括 “too many frames after interpolation/root frame adjustment”。因此通常不能用 XML `<Frame>` 数量直接决定 native selected-key 范围。本样本恒定 root、每 key Delay1、无插值，使预期单 visual track／15 native keys 很简单；**本支持声明仍要求在实际完整构造后读取 descriptor／track，并按该预期 shape 接纳或报告不支持**，没有运行确认它必为15。

PNG 的 indexed 判定也不能由 loaded 位替代：PNG table+8 指向 `A12420`，raw `A12420: 83 79 28 00` 检查 **image+28 format==0**；`A123D0` 读 image+8 bit1，是另一属性，不能将它改名为 indexed。decoder 在 palette 分支写 format0，四通道分支 `A64F01` 写 format8。本文件 RGBA8 提供非 indexed 的预期，实际候选仍须检查 native format／后端和 [tx-03-18 的最终 pair 条件](tx-03-18-single-sprite-adapter-contract.md)，不能从 IHDR 替代 native 后态。

## 3. Authority 的同窗口采集

源码 [UpdateHook](../../cs-t0/native/src/loop_runtime.cpp#L1020) 在 `g_update()` 正常返回后同步关闭 Stage::Outer，再调用 top-exit observation。`world::StageEvent` 检查本 token 的 owner／room／tail、mutation／lifecycle、commit kind，随后执行 [Capture](../../cs-t0/native/src/world_record.cpp#L1670)。原始 `_main` 的 `9312EC: E8 DF A3 07 00` 调 update，紧接 `9312F1` 调 render；因此不能把 render 后的另一次读取贴成先前的 Authority tick。

新姿态采样位置候选为：在同一个普通完整 Capture 中、相同 token／source owner 保护下复制声明的 Sprite 值；扩展读取结束后再次检查该 owner／generation／资源关系，并纳入现有 roots/mutation/lifecycle 的最终复核，最后才 FinishBatch。不是等 FinishBatch 已确认完整后另读 Sprite 再拼接。缺失、越界、背压或关系不稳定时标记该声明样本不完整，不用旧姿态补齐或前进 AppliedAck。

现有 `kPlayer`／CopyObject 和 world batch 没有 Sprite 名称、descriptor、key 向量或外观字段；本轮没有修改它们。未来 Sprite source 必须来自明确的自有构造 owner，或已解释当前有效来源的受支持对象，绑定 session/run/room 与本地 source generation。资源 fixture 的 Type1000 配置不自动产生这个来源，也不将任意 Player／Effect 指针加一个偏移当成有效 source。

主审 fresh `6B10F0` 在 `6B120B` 形成 Entity+48 receiver，`6B124D` 调 Sprite Render；这确认当前基类内嵌 payload 的位置。`6B2000` 同样使用 +48，但 shadow pass 在 `6B215D` 把 layer+74 设1、临时改 Sprite Color，正常尾部恢复 Color，`6B21F5` 却直接把该 visible 写0，没有保存任意旧 visible 值。故“采在 render 前”定义的是声明的持久状态，不等同最终 Entity 组合外观；本 fixture 没有 shadow layer，仍不由它外推 Player／派生几何。

姿态只读复制应包括本地支持清单对应的 resource key/revision、精确 main／overlay 名、逐 visual-track key、overlay 次序和完整声明外观。native filename／descriptor string 按实际 native string 长度／容量做有界复制，拒绝内嵌 NUL 和不支持名字；数组按本地已解释 count／stride 检查。source owner 与关系在复制前后复读；不调用 Render、Update、SetFrame、Replace、LoadGraphics 或有 sampler 副作用的 `408590` 来“查询”。指针、vptr、native string 布局和 control 不进入逻辑 DTO。

source frame／playing／event masks 可供本地诊断，但不作为客户端游戏事件或姿态重建指令。原生 frame 在 direct draw 中不是逐层 key 的替代。首版 full-pose DTO 是本地设计草案，不修改既有 ISCS5／ABI5／七项 Lua API，也没有复用原版 Online／Isaac Sync。

## 4. 同 descriptor 的连续姿态更新

以下算法只适用于 adapter 自有、已准备、未退休的 Sprite；共享 cache／资源结构稳定，主线程内没有 draw 重入、原生 callback 或其它使用者观察中间写入。它不是硬件原子事务、并发 memcpy 或 arbitrary native 对象修复。

1. **先完整规划。** 在第一次写前，将整份入站 pose 解码到有界自有内存；检查 source revision、resource declaration、两个 descriptor 的本地身份、数组地址／count、owner／generation、完整 key 向量、每个 key 的范围、所有声明外观与 flags。实际 main／overlay 必须仍是本候选被管理的 state，playing=0、事件=0，资源最终关联仍满足支持范围。意外的原生推进不靠清位悄悄修复。
2. **只执行普通字段写入。** 把本地 registry 的 pose 状态标为 Updating，停止该实例新 draw；再写完整 main／overlay visual selected-key 数组和完整声明外观。此段不分配、不调 native setter／loader／Lua／资源 helper，不 yield，也不改变 descriptor、数组容量、ANM 引用或 image pair。没有把数组复制到 Authority 原生对象上。
3. **固定本地停止语义。** native frame marker 保持初始化的 -1、playing=0、事件=0；第二类 track 维持 reset 的 -1。Authority 分数 frame 若保留，放在逻辑诊断元数据中，不调 `408E00` 重建 key，不从客户端墙钟推进动画。
4. **完成后再发布 revision。** 复读完整已写字段及同一组 owner／generation／descriptor／数组关系，全部满足后，最后提交本地 applied pose revision 并恢复可见关联。此 revision 不是整批 baseline 的 AppliedAck；整批仍须满足其全部对象与引用。若中途异常／读回不符，保留已执行前缀，停止该实例发布和显示，不把旧值冒充目标已应用，也不猜测回滚未知 native 状态。

本样本若实际 native shape 符合声明，key 向量只有一项；连续姿态可依次选择0..14或明确的 -1。重复 draw 同一已应用 pose 不调用 Update；同 revision 同规范字段直接比较后幂等，旧 revision 不覆盖已应用的新值，同 revision 内容冲突则拒绝。包到达时间、render 帧数与 Authority tick 分开。

绕开的入口有具体副作用：reset=true 会重新分配／清索引与事件；非 reset 仍有 tx-03-16 的数组复制风险；SetFrame 重算全套索引／事件；SetLayerFrame 依总帧再选择某一 track 且 mapping 错误后仍可能写；`409100→409030` 才是本地推进。它们都不等于应用一份已确定的完整 key 向量。

## 5. 资源／descriptor 变更与 owner 服务窗口

| 变化 | 首版处理 |
| --- | --- |
| key、position、允许的外观改变，resource／两个 descriptor／数组关系不变 | 采用上面的有界原地提交；本样本不需要每帧 Load／reset |
| ANM、sheet 路径、resource revision 改变 | 新的未发布候选，按一次 Load(false)→需要的覆盖→LoadGraphics→动画 reset→完整姿态→Inspect 准备 |
| main 或 overlay 名／descriptor、overlay 有无改变 | 同样准备新候选；不在 live Sprite 上逐个 reset 或 RemoveOverlay 后再补其它字段 |
| 原生 cache 重解析、外部 Mod 热替换、未知结构／owner 变化 | 不沿同 descriptor 优化继续写；报告不支持／未知并停显，不凭指针相等认为内容未变 |

本样本初始姿态明确给 Offset0、Scale1、Rotation0、Color11分量 `(1,1,1,1,0,0,0,0,0,0,0)`、Flip=false、layer visible=true／scale1／其它偏移0、main key0、overlay不存在和零 crop。它们是自有 fixture 的设计值，不把进程全局提供的构造默认当常量，也不清原对象未知 flags 冒充同样视觉。支持声明不接纳其它动画／sheet；上表变更分支是后续支持清单扩充的调用规则。

结合 tx-03-19，未来最小实现的服务顺序为：

```text
网络／控制线程：只解码、限额入队
主线程 RenderHook 进入后的自有服务点：
  已知 owner/context + 上一轮队列已空 + 无在途自有 draw
  -> 处理有界 Prepare/Inspect 或同资源完整 pose 规划／提交
  -> 不手工 flush/swap 修复未知队列
原生 render owner 原调用
最终9560D0的pre窗口：只绘制已准备、已提交实例
原生A19180正常返回并结束该轮使用：
  复核context与队列/owner -> 切换关联或退休不再使用的旧实例
```

这些 callback／队列／guard 尚未实现。现有 outer RenderHook 没有上述服务回调，普通 source 健康状态也不代替 renderer/window／TLS／WGL guard。资源工作不混入最终 draw 回调；若 owner 提前返回、没有最终 helper witness、context 丢失或退出已越过正常资源清理时段，不能只凭 RenderHook post 盲目析构。正常显式停用应在图形 context 仍存活、引用与队列顺序明确的 owner 中结束；完整退出／异常／worker 恢复仍不由此保证。

新候选失败时未改变旧 Sprite 的内存，但不得继续把旧姿态报告成目标 revision 已完成；按当前同步设计停止相关发布／显示并记录失败前缀。退休沿原生引用计数和 batch/cache 持有关系，不手工删除纹理、不 Reset 其它 owner、不把 virtual device 的 NEVER 策略移植过来。整体 baseline/delta apply、回滚和任意 Entity 创建都未在本轮实现。

## 完成范围与下一步

对于这个受限单 Sprite 样本，本轮已有具体文件、支持声明、Authority 同窗口采集位置、same-descriptor 无 native 调用的更新方案、候选替换与正常 owner 服务顺序。**建议下一轮直接实现最小独立 Sprite adapter 和本地 fixture**，把 actual load／descriptor、context／pass、连续 key、正常替换／停用和像素结果纳入实现完成后的一次整体 E2E，不再为已解释的基础链另开重复逆向。

仍未知的实际 resolver 命中、native key 数、资源／GL 结果需要实现与运行验收回答。当前没有 adapter、fixture source、姿态采集扩展、registry、服务回调、新 Hook、wire codec 或 E2E；Player 组合、房间内遮挡、特殊视觉以及完整 OOM/EH／worker／热重载仍各自有范围。本次没有把这些未知提升为新 gate，也未触及原版 Online／Isaac Sync。

唯一一次静态文档验收见 [主审记录](tx-03-20-primary-review.md)和 [final-static-review.json](../../artifacts/cs-t0/tx-03-20-static/final-static-review.json)。
