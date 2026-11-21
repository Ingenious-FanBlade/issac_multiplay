# tx-03-20 Luna：Authority 同窗口 Sprite 姿态采集与连续更新

日期：2026-10-23（Asia/Shanghai）。本轮仅做静态逆向和 C/S 源码读取，目标是确定完整 Authority 世界采集与 Sprite 姿态采样的同窗口关系、main/overlay 选中 key 与外观的读取路径，以及同资源连续更新和描述符变化的写入边界。没有改生产代码、Hook、IDB 或主线报告；没有运行游戏、资源加载、native/OS helper、构建或测试。

本文区分 `STATIC_CONFIRMED`（当前 IDB 指令或源码）与 `STATIC_DERIVED`（正常顺序下的控制流推导）及 `DESIGN_PROPOSAL`。没有运行时线程、资源命中、Sprite 状态或像素观察。

## 结论

完整 Authority 采集的合适姿态采样点，是现有 `world::Capture` 同一普通完整捕获窗口内、`FinishBatch` 前的本地扩展位置。`UpdateHook` 在原生 top-level update 返回后先调用 `StageEvent(Outer,false)`，该阶段关闭 world token 并执行 `Capture`；当前调用点后面紧邻普通 top-level render callsite。IDB 原始指令为 `0x9312EC` 调 `0x9AB6D0`，下一条 `0x9312F1` 调 `0x9555C0`。因此静态顺序表明，完整采集可在本轮 render 进入前读取姿态；不得在 render 后另取一帧并把它标成同一 Authority tick。

现有 world capture **尚未采集 Sprite 姿态**。它对 Player 读取 `kPlayer` 中列出的字段，并处理有限的对象关系；没有记录 Sprite animation descriptor、main/overlay selected-key 向量或 Sprite/layer 外观。新姿态数据需要受本地 owner 与代次保护的 Sprite 来源，并在同一完整 batch 中记录，不能从 Player、Effect 类型或资源 XML 反推一个 Sprite 指针。

固定姿态的 direct draw 由 `Sprite+0x30` main state、`Sprite+0x50` overlay state 中各自的 descriptor 和 selected-key 数组驱动。连续更新同一资源、同一 main/overlay descriptor 时，可由自有 adapter 在稳定实例上原地写入完整 selected-key 向量和受支持的实例外观；应跳过动画选择、SetFrame、SetLayerFrame、Play 与 Update 这类会重置、重算、推进索引或事件位的原生入口。资源、main descriptor、overlay descriptor 或 overlay 存在状态改变时，按候选新建、完整检查、再替换旧实例处理。

## Authority 同窗口与采样时序

| 位置 | 静态证据 | 结论 |
| --- | --- | --- |
| Update / Render 两个 owner callsite | `loop_runtime.cpp` 第 45–53 行；fresh IDA 范围见原始 artifact | `0x9312EC / RVA 0x5312EC` bytes `E8 DF A3 07 00` 调 `0x9AB6D0 / RVA 0x5AB6D0`；紧邻 `0x9312F1 / RVA 0x5312F1` bytes `E8 CA 42 02 00` 调 `0x9555C0 / RVA 0x5555C0`。 |
| Authority normal-return witness | `coop_active_runtime.cpp` 第 1373 行起、第 1487–1489 行 | `StandardRoomHook` 在原生 `g_standard_room` 正常返回后调用 Authority post；post 通过 `world::Commit(candidate, kind)` 记录 native-return witness。注释明确它不是 completed tick，world collection 随后发生，publication 等待 TopExit。 |
| outer update close | `loop_runtime.cpp` 第 1020–1033 行 | `UpdateHook` 先开 `Stage::Outer`，调用 `g_update()`，再关 `Stage::Outer`，之后才调用 top-update-exit observation。 |
| 完整采集资格 | `world_record.cpp` 第 1961–2009 行 | `StageEvent(Outer,false)` 检查 token 的 owner/room/tail 各恰好一次、均不活跃、无 lifecycle/mutation/continuity 问题；然后同步调用 `Capture(reason)`。未满足时记录 gap，不冒充完整样本。 |
| batch 身份和早退 | `world_record.cpp` 第 1660–1707 行 | batch 记录 `g_token.commits==1` 的 Authority 标记、tick、outer、attempt 和初始完整状态；`initial!=None` 时直接 `FinishBatch`，不会进入 world 字段复制。 |
| world/source 稳定性 | `world_record.cpp` 第 1551–1592、1778–1813 行 | descriptor 与成员指针在读取后复读；root/room、mutation 和 lifecycle 也在复制尾部复核，再决定完整度。 |
| Player 字段范围 | `world_record.cpp` 第 1442–1450 行及 `CopyObject` 第 1490–1518 行 | `kPlayer` 不含 Sprite descriptor、selected-key 或外观字段。Player 的既有成员关系不能被解释成已映射的 Sprite source。 |

**STATIC_DERIVED。** 对一个普通完整 Authority tick，姿态读取应发生在 `Capture` 处理同一个 `g_token` 和同一个 batch 的过程内，在 `FinishBatch` 前完成；建议仅当 `initial==None`、Authority 标记成立、当前 batch 仍完整、owner/source/generation 稳定时追加本地拥有的 Sprite pose。缺任一条件时将 pose 标为缺失/不完整，不能取 render callback 或下一帧来补数。

当前 `world::Capture` 只复制既有 source 表，没有上述 Sprite pose 扩展；本轮没有编辑该实现。候选 Sprite owner 必须由后续 adapter 明确提供，不通过实体工厂、`Spawn`、Player 关系字段或普通 Effect 类别临时找对象。

## 新鲜 IDA 证据与姿态字段

本轮 fresh IDA metadata：活动数据库路径 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，input path 为同目录 `isaac-ng.exe`，imagebase `0x400000`。未比较磁盘身份，未计算或校验哈希。`RVA = VA - 0x400000`。

原始函数 bytes、伪代码、指定指令范围和 IDA 当前静态 xrefs 在 [luna-pose-capture.json](../../../artifacts/cs-t0/tx-03-20-static/luna-pose-capture.json)；只读采集脚本为 [luna-export-pose-capture.py](../../../artifacts/cs-t0/tx-03-20-static/luna-export-pose-capture.py)。本次 20 个函数 seed 全部解析；artifact 只表示 IDA 静态视图，不能证明磁盘构建身份或动态调用全集。

| VA / RVA | 直接证据与正常职责 |
| --- | --- |
| `0x408830 / 0x8830` | state reset。将 descriptor 写入 state+`0x04`；重置/重建 state+`0x08/+0x0C` 数组并以 `-1` 初始化；state+`0x10` 设为 `-1`，`+0x14` playing 和 `+0x18/+0x1C` 事件清零。分配后直接 memset，完整 OOM/EH 安全性未知。 |
| `0x40A5D0 / 0xA5D0` | main 名字选择。遍历 Sprite+`0x74` 表、count 在 +`0x78`，descriptor stride `0x13C`，用 `strcmp` 精确匹配；`0x40A668: E8 C3 E1 FF FF` 调 `408830`，`0x40A678: E8 F3 E2 FF FF` 调 `408970`。找到名称不等同 image/descriptor 完整就绪。 |
| `0x40AAF0 / 0xAAF0` | overlay 名字选择，使用同样的名字/descriptor 查找，state receiver 为 Sprite+`0x50`；`0x40AB88: E8 A3 DC FF FF` 调 `408830`，`0x40AB98: E8 D3 DD FF FF` 调 `408970`。 |
| `0x40A7E0 / 0xA7E0` | SetLayerFrame。main descriptor 的 +`0x38` 映射把 Sprite layer ordinal 转 visual track index；`0x40A84D: C7 04 B0 00 00 00 00` 先写 selected key 0，再于 `0x40A854` 调 `408B90` 依给定帧重算该 track。映射错误会先诊断，但后续仍可能写数组，不作为安全的网络写入 API。 |
| `0x408E00 / 0x8E00` | SetFrame 核心。裁剪并写 frame，清事件位，清并重算两类索引；会遍历重算 key/事件，不能保留逐 track Authority 姿态。 |
| `0x409100 / 0x9100` | Sprite Update。只有 Sprite+`0x109` 非零时，顺序更新 main 和 overlay 两个 state，调用 `409030`。 |
| `0x409030 / 0x9030` | 仅在 state descriptor 非空且 playing 时推进 frame；读取 Sprite+`0x104` playback speed，进入 `408D00` 重算 key/事件或循环/停止分支。它不是姿态读取函数。 |
| `0x40A030 / 0xA030` | state draw loop。按 descriptor+`0x1C` 个 visual track、stride `0x10` 遍历；每次从 state+`0x08` 取该 track selected key，再调用 `409120`。 |
| `0x40A0D0 / 0xA0D0` | Sprite Render。要求 Sprite+`0x109` 非零，按 Sprite+`0x70` 决定 overlay/main 次序，再对 Sprite+`0x30/+0x50` 两个 state 调 `40A030`。没有调用 `409100`。 |
| `0x409120 / 0x9120` | 单 visual-track 绘制入口；读取 track selected key、keyframe、layer 和 Sprite 参数，经 `4098A0` / `409CB0` 准备几何/变换，再进入图形 draw。检查代码路径未看到对 state selected-key 数组的写回；它会改 renderer/图形全局状态，flags bit `0x02` 的几何路线还更新共享 RNG。不可用 Render 充当无副作用 getter。 |
| `0x4098A0 / 0x98A0`, `0x409CB0 / 0x9CB0` | geometry / appearance 读取路径。二者读取 keyframe、实例 layer 与 Sprite 的变换/flip/flags，构造 draw quad/位置；其输入来自现有对象字段，不是把绘制状态回写为 Authority pose 的接口。 |

姿态的状态布局承接 tx-03-16/18 的已确认字段：Sprite+`0x30/+0x50` 是 main/overlay state；每个 state+`0x04` 是 descriptor，+`0x08` 是 visual selected-key 索引数组，+`0x0C` 是另一类 track 数组，+`0x10` frame，+`0x14` playing，+`0x18/+0x1C` 事件位。descriptor+`0x18/+0x1C` 指向 visual track 表和数量，track stride 为 `0x10`；track+`0x04/+0x08` 是 key 记录表和 key 数量。Sprite main descriptor 列表来自 +`0x74/+0x78`。

外观没有另一个可安全调用的聚合 getter。draw helpers 直接读取 Sprite+`0x94/+0x98` Offset、+`0x9C/+0xA0` Scale、+`0xA4` Rotation、+`0xA8` 44-byte Color、+`0x100/+0x101` Flip、+`0x110` flags；layer 外观读取 layer+`0x32/+0x33` flip、+`0x34/+0x38` scale、+`0x3C` rotation、+`0x40/+0x44` offset、+`0x48` Color、+`0x74` visible、+`0x8C` flags、+`0x90/+0x94` quad offset。字段用途沿 tx-03-16 受限 profile，不据此扩成所有颜色/shader 组合。采样应在固定 owner 下直接复制已声明字段并做有界/有限值解释；不要调用会切图形资源或改共享状态的 `408590` 取外观。

## 同资源连续更新合同

**DESIGN_PROPOSAL。** 先对已发布实例做完整只读规划：固定本地 ResourceDeclaration、main/overlay descriptor 身份、track 数量、完整 key 向量长度、每个 key 的 `-1` 或该 track 内范围、layer 映射、实例外观和支持 flags。所有尺寸与乘加须有界；确认同实例仍属于当前 session/run/room generation，且无在途 draw/retire 后，才原地写入完整向量及完整声明外观。写完复读同一组字段；失败则停止该实例发布，不用旧姿态冒充新 revision。

对外身份使用本地支持清单中的精确资源声明和 main/overlay 动画名；native descriptor 地址只在当前实例寿命内作只读定位，不进入 Authority DTO 或远端身份。

| 更新类型 | 合同 |
| --- | --- |
| 同一 ANM、同一 main descriptor、同一 overlay descriptor/存在状态 | 不 Load/Reload/Replace，不改变 state descriptor 或数组尺寸；在现有实例内写完整 selected-key 向量和外观。样本若 native descriptor 实际仅有一个 visual track，则向量为 1 项、该项选 0..14 或明确 `-1`；实际数量必须读本地构造后的 descriptor/track，不能由 XML 直接定值。 |
| 同一 ANM 但 main 名称/descriptor 改变 | 建候选、新 descriptor、新索引向量，完整检查后切换本地 registry 关联，再退休旧实例。不要在已发布实例上调用 `40A5D0`、`40A690` 或 `40A720` 试图做事务式换动画。 |
| overlay descriptor 或 overlay 有/无改变 | 同样建候选后切换。不要在已发布实例上调用 `40AAF0`、`40AAD0` 或 `40AC60` 半途重置/清除 overlay。 |
| key/外观变化但 descriptor 不变 | 只写本地实例选定的 vector 与已声明字段；不要调用 `40A7E0` 逐层重算，不调用 `408E00`/SetFrame，也不调用 `409100`/Update。 |

以上避用入口的依据是其静态副作用：`40A5D0/40AAF0` 在成功匹配后改 state descriptor 并进入 reset/rebind；`408830` 清理/重建索引，`408970` 走数组重分配/复制；`408E00` 根据一个总帧重算 key 与事件；`40A7E0` 只重算指定 track 并在映射无效时仍有写入；`409100→409030` 根据 playback speed 推进 frame、key 和事件。它们都不能代表“写入 Authority 已选中的全部 key 而不改其它状态”。

固定姿态应保持 state+`0x14` 为停止、事件位为零；`state+0x10` 仅作本地诊断或受支持值，direct draw 不靠它重算 key。外观变更必须给出完整默认语义，不能让缺字段静默继承任意上一版颜色、visible 或 flip。

## 资源 fixture 与边界

主线提供的静态 fixture 候选为 `gfx/1000.011_bullet poof static.anm2` 的 `Poof` 动画，以及 `effects/Effect_003_BloodTearStatic.png`（256×256、RGBA8、non-interlaced）；`entities2.xml` 的 id 1000 / variant 200 只作文件来源线索。该 fixture 是未来自有独立 Sprite 的资源样本，**不**意味着允许 `Spawn`、新开 Effect Entity profile 或已闭合游戏 Effect 生命周期。

XML 样本呈现一个 layer（Id 0、Name=head）、一个动画和 15 个 delay=1 的 key，root frame 恒定；未见 Null、Events、Triggers 等项目。主线 fresh 解析器 `0x40ED50` 已发现会做 interpolation/root frame 调整，因此这些 XML 数量只给预期 native key 数，不是解析后契约。只有本地候选成功构造后，读取实际 descriptor 的 visual-track count 与 track key count 才能确定向量/范围。资源路径大小写和 slash 风格不一致，文件存在不证明游戏 resolver 命中；本轮未解包、加载或运行该资源。

## 未知与范围

- 当前 C/S world snapshot 没有 Sprite/ANM/appearance 字段，也没有从现存 Entity identity 到受控独立 Sprite owner 的映射；本轮只给同窗口采样位置和字段合同。
- `0x40ED50` 的静态 parser 逻辑不能替代实际 loaded descriptor 结果；XML 15 keys 不等同原生 count 的运行时确认。
- resolver 对资源路径的实际命中、PNG final pair/GL 可绘制性、线程/context 运行值、draw 重入、对象寿命、OOM/EH、worker/hotreload 均 UNKNOWN。
- 单 Sprite direct Render 的静态路径未显示 selected-key 向量写回。本子任务没有展开所有 Entity/Player render；主审另外 fresh 核对 `6B10F0` 的 Entity+48 receiver，以及 `6B2000` shadow pass 的颜色恢复／layer visible 直接写0，见[主报告](tx-03-20-sprite-resource-pose-closure.md)。因此采到的持久 Sprite 状态不自动等于 Entity 合成外观，也不借未来/额外 render 帧补采样。
- 本轮只编辑本 Luna 报告与 `artifacts/cs-t0/tx-03-20-static/luna-*`。整体验收由主智能体执行；本子任务没有运行验收或其它测试。
