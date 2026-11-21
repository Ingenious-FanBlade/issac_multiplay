# tx-03-12：主智能体复核与整体静态审阅

日期：2026-10-22（Asia/Shanghai）。用户要求继续游戏逻辑／展示分离的下一步逆向；沿用用户此前指定的 GPT-6 Luna / max 子智能体研究、主智能体审阅安排。本轮实际任务 `derived_render_tx0312` 使用 `gpt-6-luna`、reasoning effort `max`。目标为三类实体派生绘制与 Sprite 资源生命周期，最终研究见 [正文](tx-03-12-derived-render-sprite-lifecycle.md)。

## 主审做了什么

子智能体读取当前 IDB，形成 [Luna 报告](tx-03-12-luna-derived-render.md)与 `luna-*` 证据、只读导出脚本。主智能体另做资源研究，并从同一 IDB 独立导出三张表、四个派生入口、三个 Lua wrapper、两个实际 dispatcher、Tear 直接 Lua 路线和 NPC 几何 helper；没有只按子智能体摘要采纳结论。

| 审阅项 | 主审独立取证 | 结论与限定 |
| --- | --- | --- |
| NPC／Tear／Projectile `+0x14/+0x18/+0x34` | [primary-derived-tables.json](../../../artifacts/cs-t0/tx-03-12-static/primary-derived-tables.json) | 接受表项；只代表这三张类表，不能由共用排序槽或 Tear／Projectile 共用 `+0x18` 外推所有类型绘制相同。 |
| 三类主绘制的直接作用 | [primary-derived-render.json](../../../artifacts/cs-t0/tx-03-12-static/primary-derived-render.json) | 接受额外 Sprite、缓存／颜色、类型分支等事实。没有全闭包的 helper 保留 UNKNOWN；不将可见展示写入误命名为模拟步进，也不将 render 叫作只读。 |
| 三条 Lua 入口与 IDs 28／41／45 | [primary-render-callbacks.json](../../../artifacts/cs-t0/tx-03-12-static/primary-render-callbacks.json)、[primary-render-lua-chain.json](../../../artifacts/cs-t0/tx-03-12-static/primary-render-lua-chain.json) | 接受位图、filter、userdata 参数和实际 `lua_pcallk(4 args,1 result)`。回调绑定与 gameplay 效果 UNKNOWN；ID 是直接字面量，不依赖外部枚举。 |
| NPC 的曲线／点列几何 | [primary-npc-geometry.json](../../../artifacts/cs-t0/tx-03-12-static/primary-npc-geometry.json) | 接受 `4E9B10/59BED0 → 41E720 → 41E960`，其中写共享 buffer／global cursor。说明仅 Sprite 替代会损失特定几何，不宣称 GPU 深层无副作用。 |
| Projectile `+0x350` 的减后加回 | 主审重读 `0x65CB84/0x65CED1`、两个分支和 Lua 前的调用序列 | 接受正常路径的计算式恢复；限定为 PositionOffset.y。没有保存原位值后写回，不能声称 bit-exact、EH／重入或并发闭包。 |
| 初稿把 Tear／Projectile Lua filter 写成实体类型 | wrapper 分别读取 Entity+44（十进制），即 `+0x2C`；NPC 才读取 `+0x28`。 | 已修正子报告为 Variant；保留初稿错误和修正说明。 |
| 初稿把伪代码十进制 360 写为 `+0x360` | Projectile 指令／伪代码实际读 flags `+0x168`；`+0x360` 是 Velocity.x。 | 已修正子报告并在正文澄清，未据错误 offset 实现写入。 |
| 初稿将额外随机向量直接称为坐标／纹理抖动 | 主审重读实际四组栈参及 [primary-sprite-draw-parameters.json](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-draw-parameters.json)。 | 限定为主坐标外的两向量绘制参数；下传 `409120 → 4098A0/409CB0` 的完整语义仍 UNKNOWN，不以推测名称冒充效果闭合。 |

主审还限定：NPC 的 Lua 分派 `0x6D4006` 之后仍有额外绘制工作，不能将所谓 render callback 理解为整个函数或整个帧都已结束；Tear 的 Lua 在 exists／visible 保护内，Projectile 的特殊直接 Sprite 路线不能仅因普通分支调用基类就被赋予基类全部检查。当前回调 userdata 包装 Entity 指针，但本轮没有观察任何脚本实际写世界。

## 资源研究的证据与推导

具名绑定来自 [primary-sprite-bindings.json](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-bindings.json)，包括 Reset、Load、Reload、LoadGraphics、IsLoaded、Render／RenderLayer 和构造目标。

- [primary-sprite-initial.json](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-initial.json)：构造／Reset／Load／Entity 析构。
- [primary-sprite-lifetime.json](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-lifetime.json)：独立 Sprite 析构、IsLoaded、Lua userdata 构造、layer 初始化等。
- [primary-sprite-resource.json](../../../artifacts/cs-t0/tx-03-12-static/primary-sprite-resource.json)：缓存 bind／add-ref／remove-ref、graphics、layer 析构。
- [primary-cache-lifecycle.json](../../../artifacts/cs-t0/tx-03-12-static/primary-cache-lifecycle.json)：零引用 cache 扫描、其他 cache 路线与 Entity 构造；未使用非相关诊断函数当作删除依据。
- [primary-resource-release.json](../../../artifacts/cs-t0/tx-03-12-static/primary-resource-release.json)：共享数据释放、图像关联／control、动画数组释放。

主审接受的关键推导是：独立 Sprite 正常构造不需要 Entity factory；Sprite 内部 self／owner、缓存数组和图像引用阻止按裸内存复制；`IsLoaded` 仅表示 byte `+0x109`，不能证明整个加载过程成功；Reset 和完整析构不同。失败 helper 正常返回后该标志仍可能被置 1 是条件性静态推导，不伪装为一次无效路径实验。Reload 的重复引用平衡未全闭合，未宣称已发现运行时泄漏。

未来 adapter 要解释实际支持资源、动画、layer、图像引用及失败后的停显／退休，而不是以这个标志替代实际可绘制状态。该功能约束不是新项目 gate，本轮没有阻止用户继续其它实质研究。

## 方法、失败与复现

活动 IDB 路径、input path 与 imagebase `0x400000` 已记录在每份主审原始 JSON；未将路径相同当作与磁盘文件内容一致。只读提取脚本为 [export_static.py](../../../artifacts/cs-t0/tx-03-12-static/export_static.py)，使用 `py_exec_file` 单一 namespace，输出 create-new。重复提取可从现有 JSON 的 requested_va／ranges／tables 选择集，向 `runpy.run_path` 的 init_globals 传相应 `TX0312_*` 参数和新路径；不覆盖本轮数据。脚本只读取当前 IDB 并写证据文件，不执行目标代码或校验哈希。

第一次主审字符串扫描把 generator expression 放在 `py_eval` 的分离命名空间，实际 NameError；随后改用文件脚本。一次 rg 通配路径被 Windows 拒绝，一次 PowerShell 指令格式化返回 exit1；均见 [采集失败记录](../../../artifacts/cs-t0/tx-03-12-static/primary-acquisition-failures.json)。它们是资料获取错误，不是游戏失败，也未作为“没有匹配”或目标测试结果。终端部分大输出截断，正文判断以完整落盘 JSON 为依据。

两个 Luna 导出脚本已经各自完成原始资料提取；主审为其加入 `TX0312_LUNA_OUTPUT` 可选新路径，便于重复提取且不覆盖旧输出。此默认行为不变的编辑没有再次执行，最终静态文档审阅只评价脚本与交付的可读一致性。

本轮双方全程只读 IDB，没有 patch／rename／set_type／保存；没有游戏输入、启动、进程附着、注入、断点或游戏资源实际加载。未修改生产源码，没有构建、单元测试、E2E 或其它实验；没有计算／校验任何哈希。原版 Online／Isaac Sync 的协议、会话和运行状态未触及。

## 唯一一次整体静态文档验收

全部研究和编辑结束后，使用 [review_documents.ps1](../../../artifacts/cs-t0/tx-03-12-static/review_documents.ps1)作为一次统一静态文档审阅入口：读取正文、本主审、Luna 报告、更新入口和 JSON，检查本轮新增引用、JSON 完整性、关键表项／指令及 VA/RVA 换算；结果为 [final-static-review.json](../../../artifacts/cs-t0/tx-03-12-static/final-static-review.json)。语义判断来自上述人工反编译／指令审阅，脚本只检查交付一致性，不证明游戏可运行或资源安全。

此验收仅评价 tx-03-12 的文档／静态证据交付。独立 Sprite 运行适配、完整资源 control ABI／释放线程、缓存重载、原生 EH／OOM、所有实体变体／Mod 和 Player 组合外观仍未完成，也没有新增可启用的 Replica 实现。
