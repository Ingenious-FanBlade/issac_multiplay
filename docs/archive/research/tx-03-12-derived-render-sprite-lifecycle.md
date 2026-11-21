# tx-03-12：派生实体绘制与 Sprite 资源生命周期

日期：2026-10-22（Asia/Shanghai）。类型：静态逆向。延续 [tx-03-11](tx-03-11-logic-presentation-separation.md)，本轮研究 Entity_NPC、Entity_Tear、Entity_Projectile 的绘制入口，以及独立 Sprite 的构造、加载、引用和释放链。GPT-6 Luna / max 子智能体研究派生绘制，主智能体研究资源并重新取证复核；结论与审阅见 [主审记录](tx-03-12-primary-review.md)及 [Luna 报告](tx-03-12-luna-derived-render.md)。不包含 Player 组合外观、所有实体变体、Mod 或完整 GPU 后端闭包，不实现 Replica。

## 本轮结论

**原生实体 render 与独立 Sprite render 是两条不同的适配路线。** 三类派生实体的 `+0x14` 都会条件性执行各自 Lua dispatcher，不能只处理外层 post-render；它们还包括额外 Sprite、变换或类型专用绘制。另一方面，原生 `Sprite()` 构造链确实能创建不经 Entity 工厂的 ANM2 展示对象，适合作为受限首版的进一步研究路线。但 Sprite 不是可直接复制的内存块，也不是用 `IsLoaded` 一个标志就能确认资源完备。

当前可作出的设计推导是：先为明确支持的视觉对象建立独立 Sprite 和资源 ID 映射，再应用服务器裁决的动画／外观结果；对需要原生派生 render 才能表达的对象，继续逐条解释其展示依赖、回调和状态写入。前者会丢失原生特殊图形，后者的副作用面更大。本轮没有选择全部类型通用的替代器，也没有证明任一路线已经可以实机启用。

## 数据库与证据口径

活动 IDB 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，input path 为同目录 `isaac-ng.exe`，imagebase `0x400000`。本轮只读当前 IDB，不断言磁盘输入与 IDB 内容相同，不计算或校验任何哈希。版本／旧磁盘元数据的限定沿用 tx-03-10；当前事实以本轮导出的指令、表项及调用上下文为准。

以下明确区分 `STATIC_CONFIRMED`（直接静态事实）、`STATIC_DERIVED`（基于这些事实的推导）及 `UNKNOWN`。静态可达不是实际运行记录。`RVA = VA - 0x400000`，虚槽与对象 offset 不是 RVA。Hex-Rays 签名中的 EBP、缺省参数及返回类型不能直接作为 native adapter ABI；涉及调用接口要同时读 ECX、栈、XMM 和清栈指令。

## 三类实体的绘制表项与依赖

主智能体独立导出 [三张虚表](../../../artifacts/cs-t0/tx-03-12-static/primary-derived-tables.json)和 [四个派生入口](../../../artifacts/cs-t0/tx-03-12-static/primary-derived-render.json)。表项 bytes 均为 IDA imagebase 下的指针编码，不是运行时 ASLR 匹配模板。

| 类别／vtable VA（RVA） | `+0x14` 目标 VA（RVA）／表项 bytes | `+0x18` 目标 | `+0x34` 目标 |
| --- | --- | --- | --- |
| NPC `0xB67468`（`0x767468`） | `0x6D2E50`（`0x2D2E50`）／`50 2E 6D 00` | `0x6D4840`（`0x2D4840`），专用分支后到基类 | `0x6B1000`（`0x2B1000`） |
| Tear `0xB64EAC`（`0x764EAC`） | `0x6790C0`（`0x2790C0`）／`C0 90 67 00` | 基类 `0x6B2000`（`0x2B2000`） | 基类 `0x6B1000` |
| Projectile `0xB64990`（`0x764990`） | `0x65CB60`（`0x25CB60`）／`60 CB 65 00` | 基类 `0x6B2000` | 基类 `0x6B1000` |

这只描述上述类表；不据类名承诺全部 Type／Variant 分支。`+0x14` 调用的 receiver 是当前 Entity，栈上传坐标向量指针；`+0x18` 是另一绘制 pass。三类排序槽共用基类并不代表其绘制相同。

| 入口 | 本轮直接可见行为 | 直接 Sprite 替代会遗漏的内容 |
| --- | --- | --- |
| NPC `0x6D2E50` | 大量 Type／Variant 条件、额外 Sprite／相关对象／专用 helper、绘制坐标缓存和颜色等状态写入；普通分支可达基类 `0x6B10F0`，另有独立 Lua 分派。 | 类型专用几何／关联对象、附加部件、变换和多层绘制。未全追这些 helper，不将整函数标成纯展示。 |
| NPC `0x6D4840` | Type=886 调 `0x4E9B10(coords,1)`，Type=19 且 Variant=3 调 `0x59BED0(coords,1)`；随后调用基类 `0x6B2000`。 | 这些特殊分支的额外 pass。共享基类不能覆盖 NPC 的全部 `+0x18` 行为。 |
| Tear `0x6790C0` | exists／visible 条件；绘制主 Sprite 外还按条件绘制 Entity+`0x4AC`、`0x5C0`、`0x6D4` 的附加 Sprite；保存／恢复颜色参数；读相关 Player 及状态；最后到本类 Lua 分派。 | 附加 Sprite 的资源／动画、叠放顺序、颜色变化与关系依赖。仅同步主 Sprite 不足以表达这些分支。 |
| Projectile `0x65CB60` | 条件性从 PositionOffset.y `+0x350` 减去 `+0x410`，正常路径加回；普通路径调基类，另一路根据 `+0x460/+0x464`、Game 计数和局部伪随机算两组附加绘制向量，再调 Sprite render；最后到 Lua 分派。 | 主坐标之外的两组绘制参数。局部随机计算不等于 gameplay RNG 推进；尚未证明其完整产品语义。 |

Projectile 的两个真实写点 `0x65CB84`、`0x65CED1` 均为 `F3 0F 11 86 50 03 00 00`。前者在原绘制前写，后者在正常控制流中恢复计算；并非位级保存旧浮点值后原样回写，不证明跨异常／重入或舍入后完全无变化。它写的是 PositionOffset，不是 Position `+0x33C/+0x340`。不能把这种展示用途写入自动命名为模拟步进，也不能称整个 render 只读。

主审还独立展开 [NPC 几何辅助链](../../../artifacts/cs-t0/tx-03-12-static/primary-npc-geometry.json)：`0x4E9B10/0x59BED0` 根据点列或关联对象计算曲线，进入 `0x41E720` 向共享绘制缓冲写 68 字节记录并推进全局游标 `0xC78F00`，随后调 `0x41E960`。这是主 Sprite 之外的具体几何依赖；缓冲、临时状态和下层资源共享，不能因为有独立 Sprite 对象就推导这些 helper 可并发或重入。完整 GPU 提交／所有 helper 未闭合。

Projectile 的特殊分支检查 EntityFlags `+0x168` 的 `0x20`，不是十六进制 `+0x360`；后者是 Velocity.x，初稿的十进制／十六进制混淆已经主审纠正。主审另读 [Sprite 绘制参数下传](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-draw-parameters.json)：两组额外向量经 `0x409120` 再交 `0x4098A0/0x409CB0`，不能仅因它们来自随机值就称为已确认的位置或纹理抖动；这些下层的完整参数语义仍 UNKNOWN。

## 新闭合的实体 render → Lua 路径

本轮明确发现了外层 `0x863680` 之外的三条路径。主智能体证据：[wrapper](../../../artifacts/cs-t0/tx-03-12-static/primary-render-callbacks.json)和 [实际 Lua 调用](../../../artifacts/cs-t0/tx-03-12-static/primary-render-lua-chain.json)。

| 派生调用点 VA / RVA／bytes | wrapper 与条件 | 实际执行点／参数 |
| --- | --- | --- |
| NPC `0x6D4006 / 0x2D4006`，`E8 95 CB 18 00` | `0x860BA0` 写分派 ID=28，filter 取 Entity Type；检查分派位图 word0 的 `0x10000000`。 | `0x872B10 → 0x872C02` 的 `FF 15 D8 83 B1 00` 调 `lua_pcallk`，4 入参、1 返回值。 |
| Tear `0x6793E7 / 0x2793E7`，`E8 94 97 1E 00` | `0x862B80` 本体准备 ID=41，filter 取 Variant；检查位图 word1 的 `0x200`。 | wrapper 本体直接 `lua_pcallk`，4 入参、1 返回值，随后 ref／settop／unref。 |
| Projectile `0x65CEDB / 0x25CEDB`，`E8 90 57 20 00` | `0x862670` 写 ID=45，filter 取 Variant；检查位图 word1 的 `0x2000`。 | `0x873740 → 0x873832` 的 `FF 15 D8 83 B1 00` 调 `lua_pcallk`，4 入参、1 返回值。 |

三路都可见两个整数分派参数、包装当前 Entity 指针的 `UserdataPtr`（或适用时 nil）及复制坐标的独立 `Vector2` userdata。Lua 得到 Entity 指针包装，不能把回调本身归类为不接触世界。当前注册到 registry 的具体 dispatcher、用户 callback、执行结果及其世界写入仍 UNKNOWN，不声称已经观察到实际 Mod 写世界。ID 使用当前二进制的直接数值，不借外部枚举补造 API 契约。

**设计影响**：只绕过全局 post-render 分派不能隔离保留下来的各类实体回调；只绕过这三处也不证明专用 helper／嵌套对象的脚本入口全部消失。自有 Sprite 路线可避开这些 Entity 层 wrapper，但其资源／绘制子树还需按实际支持集合理解。这里是新识别的静态依赖，不是新增 Hook 或跳过原调用的实现。

## Sprite 的独立对象与加载状态

原生注册表 `0x866960` 在 `0x8672A1` 注册 `Reset → 0x407F10`，在 `0x8672F5` 注册 `Load → 0x40BD50`，在 `0x867334` 注册 `LoadGraphics → 0x40BFF0`，在 `0x867349` 注册 `IsLoaded → 0x420160`；`Render → 0x40A0D0`、`RenderLayer → 0x40A140` 也分别具名绑定。这些是 [注册指令证据](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-bindings.json)，不是本轮运行 Lua。

`Sprite()` 的注册目标 `0x8BB720` 调 `lua_newuserdata(L,284)`，前 8 字节为 userdata wrapper／对象指针，把后面的 payload 交给 `0x4074C0`。当前构造字段覆盖到 `+0x110` 的 DWORD，与 276（`0x114`）字节 ANM2 payload 相符。该直接链没有 Entity factory／publish，是独立展示对象可行性的静态依据，不能外推任意对齐、分配器、跨版本大小或错误恢复。证据见 [构造／加载初始链](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-initial.json)与 [生命周期](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-lifetime.json)。

`0x4074C0` 初始化原生字符串、两组 animation state 的自指针／数组及其它字段，再调 Reset。两个 state 在 Sprite+`0x30/+0x50`，其首字段指向所属 Sprite；layer 初始化后也有指向 Sprite 的 owner 字段。因此即使不考虑资源计数，直接 memcpy 整个 Sprite 也会复制错误的内部 owner／self 指针。不能把原生布局直接作为网络状态或可重定位快照。

### Load 的实际顺序与失败含义

`0x40BD50`：ECX=Sprite，两个栈词分别为路径原生字符串对象的地址、load-graphics 标志（读取其低 byte）；末尾 `C2 08 00`。路径对象有 length `+0x10`／capacity `+0x14` 和 inline／heap 两种表示，不是可直接传入的 C 字符串。

1. 旧文件名非空时，先 `0x40E2B0` 减旧缓存引用；复制新路径。
2. `0x40DB90` 查共享 ANM 缓存，必要时调 `0x40ED50` 解析；重建实例 layer 数组并关联共享动画／名字数据。
3. `0x40E110` 为解析到的缓存节点加引用。若请求 graphics，调用 `0x40C000`。
4. 两组 state 以 descriptor=0 重置；`0x40BDB2` 的 `C6 86 09 01 00 00 01` 直接把 Sprite+`0x109` 设 1；后续识别 shadow layer／特殊名字。

`0x420160` 仅 `mov al,[ecx+0x109]` 返回该 byte。Load 外层没有根据 cache bind／graphics 成功结果来决定是否设置该位；其 EAX 最后也不是已证实的统一成功 bool。**STATIC_DERIVED**：若某个失败 helper 正常返回，`IsLoaded` 仍可能为真；故它不能证明 descriptor、layer 和图像全部有效。这里没有用无效路径实际运行失败实验。

此外 `Reload 0x40BEA0` 会再次调用 bind、加缓存引用与 graphics 路线，函数体内未见与本次加引用配对的减引用。尚未分析所有调用者及替换路径，不能据此宣布内存泄漏已发生；同样不能把 Reload 规定为可无限幂等的每帧刷新接口。

## 共享 ANM 数据、实例 layer 与图像引用

原始证据：[缓存绑定／引用](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-resource.json)、[缓存清理](../../../artifacts/cs-t0/tx-03-12-static/primary-cache-lifecycle.json)、[资源释放 helper](../../../artifacts/cs-t0/tx-03-12-static/primary-resource-release.json)。下列均是对象相对 offset。

| 对象／字段 | 本轮确认的所有权或生命周期 |
| --- | --- |
| Sprite+`0x7C/+0x80` | 实例 layer 数组／数量。`0x40DB90` 按 160 字节 stride 创建 layer，每项由 `0x4081D0/0x408310` 初始化并关联缓存 layer 定义；不是整个 ANM 数据的私有复制。 |
| Sprite+`0x74/+0x78`、`+0x84/+0x88`、`+0x8C/+0x90` | 直接指向 cache node 中的动画、名字／事件名字等数组／数量；绑定时是赋指针，未深拷贝。其有效性依赖共享缓存生命周期。 |
| cache node+`0x50` | `0x40E26E` 的 `FF 46 50` 加引用；`0x40E42C` 的 `89 46 50` 在非零检查后写递减值。零值移除引用有诊断；未在该引用 helper 内释放节点。 |
| `0x40E470` 缓存扫描 | 仅对 `node+0x50==0` 的节点清理；`0x40E4BB → 0x40D850` 释放共享 layer／frame／事件数据，再摘节点、清字符串和释放节点。不能在仍有 Sprite 借用数组时自行删 cache。扫描的完整线程／调用时机未闭合。 |
| layer+`0x98/+0x9C` | image pointer／control 关联。layer 析构 `0x407A60` 通过 control 虚槽 `+0x0C` 和全局 `0xC7163C` 回调条件释放，再清路径字符串。具体 control 虚实现／释放线程及 GPU 生命周期仍 UNKNOWN。 |
| graphics `0x40C000` | 遍历 layer，调用 `0x9588A0` 取图像资源，绑定／维护图像引用，写 layer+`0x30` 的加载状态与 `+0x31`。涉及 cache、分配、虚调用、全局释放回调，不是只读查询。 |

`0x40C7F0` 可见分配 24 字节的 `ReferenceCount<KAGE::Graphics::ImageBase>` control block；本次仅解释局部创建／替换关联，未关闭其全部计数 ABI，也不把它套用到已有 owned virtual device 的不同 control 契约。共享 ANM 缓存引用和图像 control 是两层不同对象，不能以一个“Sprite refcount”概括。

## Reset 与析构的区别

`Reset 0x407F10` 在旧路径非空时减缓存引用，将两组 animation state 置空，清文件名／默认动画名；按 layer 数组头的数量调用 `0x407A60` 逐项销毁、释放数组，清缓存借用指针和数量，并恢复多组展示字段／playing／loaded／shadow 等默认状态。它会清理真实资源，不是纯粹复位动画帧。

独立 Sprite 析构入口 `0x408170` 先 Reset，再调用两组 state 的 `0x4087A0` 清数组、`0x40D040` 销毁两个原生字符串；与 Entity 析构 `0x6A8850` 对内嵌 Sprite 的对应清理链相符。Reset、释放 ANM 共享 cache、析构 Sprite、释放分配此对象的 wrapper 内存是不同阶段。不得把 `0x408170` 当通用 delete，也不得用 Reset 退出后仍让旧 layer／descriptor 指针继续绘制。

这些结论限定主线程上正常分配和正常返回路径的静态代码。OOM、解析失败、原生 EH、图像 worker／GPU 并发、cache 重载、错误资源及代际退休的完整恢复都未证明；没有执行任何资源加载／释放或游戏操作。

## 首批展示适配候选与剩余研究

| 路线 | 已取得的依据 | 必须补充的具体能力／画面限制 |
| --- | --- | --- |
| 独立 Sprite + 复制展示结果 | 原生有不经 Entity 工厂的构造链；Load／graphics／state／render／reset／析构已分别定位。可绕开本轮三类 Entity render wrapper。 | 需选定支持资源／动画／layer、建立本地资源 ID 映射和原生 owner；失败 Sprite 不发布到绘制集合。初版只支持能以 Sprite 表达的外观，不覆盖 NPC 专用几何、Tear 附层与所有武器。 |
| 保留三类原生派生 render | 能复用更多原版专用显示；已定位其差异、三个脚本入口及部分字段恢复。 | 实体壳安全创建、专用 helper／关系、回调分离和 resource／cache 的运行时约束仍未闭合；只让原更新停下不足以直接启用。 |

下一步宜沿一个明确普通视觉集合，继续核对独立 Sprite 的图像 control 虚表、资源 owner／释放线程，以及不用 Entity 规则就能应用的最小展示字段；需要 Player 时另做身体／装备组合绘制。原生 renderer 的深层能力不在本轮被宣布完成。

只读可重复提取脚本为 [export_static.py](../../../artifacts/cs-t0/tx-03-12-static/export_static.py)，以 IDA MCP `py_exec_file` 单一 namespace 执行，create-new 写 JSON。其它选择集用 `runpy.run_path` 的 `TX0312_OUTPUT/FUNCTIONS/RANGES/TABLES/STRING_TERMS` 指定；各 JSON 保留 requested_va、ranges 和 tables。采集失败见 [原始错误记录](../../../artifacts/cs-t0/tx-03-12-static/primary-acquisition-failures.json)，未将失败查询当作否定证据。

本轮未改生产源码／IDB，未启动或附着游戏／调试器，未构建、运行代码测试或实验。全部修订完成后只做一次整体静态文档审阅，记录在主审及对应结果 artifact；它不证明 Replica 已实现或原生适配已安全。
