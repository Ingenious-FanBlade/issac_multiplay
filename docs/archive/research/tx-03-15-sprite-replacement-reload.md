# tx-03-15：Sprite 换图、Reload 与 ANM 引用

日期：2026-10-23（Asia/Shanghai）。范围：继续逻辑／展示分离研究，解释 `ReplaceSpritesheet`、`Reload`、`LoadGraphics` 的 Lua 绑定、原生 ABI、逐层状态、ANM 重绑定与正常失败分支。本轮只读当前 IDB 并维护文档，不实现 Replica 或资源 adapter。按用户要求由 GPT-6 Luna / max 研究换图路径，主智能体独立取证并审阅，见 [主审记录](tx-03-15-primary-review.md)和 [Luna 报告](tx-03-15-luna-sprite-replacement.md)。

## 结果与影响

三个入口不是可互换的“强制重载”。`ReplaceSpritesheet` 仅接受不同的实例 layer 路径并清该层 `+0x30`；`LoadGraphics` 只处理这个字节为零的层。正常成功的 `Reload` 则销毁并重建实例 layer，从 ANM 缓存定义恢复默认路径，再处理图像，因此覆盖此前实例换图。已命中的 ANM 不保证从磁盘重读。

正常图像解析失败没有旧 pair 回滚；多层加载也没有整体成功返回或原子提交。`Load` 换到无法打开的新 ANM 还存在条件性的旧引用减少／借用指针保留问题。重复 `Reload` 在稳定缓存节点上增加 ANM 计数，已读包装层和重建 helper 没有对应减少。后两项属于有条件的静态推导，未观察实际泄漏、悬空访问或崩溃。

这使独立 Sprite 仍是候选路线：后续 adapter 必须管理自己的资源描述、实例状态及正常退休，不能靠循环 Reload、整体 IsLoaded 或失败后沿用旧图像来获得正确性。本轮没有增加 Hook 或写世界的实现。

## 证据与地址

活动 IDB 的输入路径为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，imagebase=`0x400000`。以下 `RVA = VA - 0x400000`；对象字段偏移、表起点与 ordinal 均不属于 RVA。输入路径不证明 IDB 与磁盘文件内容相同，没有计算或检查任何哈希。

主审原始证据均包含完整函数伪代码、原始指令 bytes 与当前 IDA xrefs：

| 文件 | 内容 |
| --- | --- |
| [primary-reload-initial.json](../../../artifacts/cs-t0/tx-03-15-static/primary-reload-initial.json) | Load／Reload／LoadGraphics、ANM 绑定、Reset、注册指令区间 |
| [primary-anm-cache.json](../../../artifacts/cs-t0/tx-03-15-static/primary-anm-cache.json) | ANM 解析、引用增减、零引用清理、动画 reset、layer 初始化、Replace |
| [primary-replacement-flow.json](../../../artifacts/cs-t0/tx-03-15-static/primary-replacement-flow.json) | 路径比较、pair 获取／替换、ANM payload 清理、draw 几何依赖 |
| [primary-loader-failure-context.json](../../../artifacts/cs-t0/tx-03-15-static/primary-loader-failure-context.json) | 图像 resolver 空结果、pair 构造和图像缓存条件退休 |
| [primary-binding-context.json](../../../artifacts/cs-t0/tx-03-15-static/primary-binding-context.json) | layer 析构、两个实际 Lua 注册 helper |
| [primary-lua-wrappers.json](../../../artifacts/cs-t0/tx-03-15-static/primary-lua-wrappers.json) | 两个 Lua closure 的参数转换、native 调用和结果数量 |

`STATIC_CONFIRMED` 指当前静态指令与数据流；`STATIC_DERIVED` 指限定条件下的计数／寿命推导；没有新增 `RUNTIME_OBSERVED`。取证脚本 [export_static.py](../../../artifacts/cs-t0/tx-03-15-static/export_static.py)支持选择集与新输出路径，拒绝覆盖原始文件。没有执行目标原生 helper，也没有执行其路径 key 算法。

## 具名绑定与 ABI

注册函数 VA `0x866960`、RVA `0x466960` 中，以下指令把 native 成员函数交给 binding helper；这些 native 地址不是直接接收 `lua_State*` 的 Lua C 函数。

| 方法 | native VA／RVA | 注册点与 bytes | 实际 Lua closure |
| --- | --- | --- | --- |
| Reload | `0x40BEA0 / 0xBEA0` | `86730A: 68 A0 BE 40 00`；`86730F → 89F030` | `8A9740` |
| ReplaceSpritesheet | `0x40BF30 / 0xBF30` | `86731F: 68 30 BF 40 00`；`867324 → 89EF10` | `8A99D0` |
| LoadGraphics | `0x40BFF0 / 0xBFF0` | `867334: 68 F0 BF 40 00`；`867339 → 89F030` | `8A9740` |

`89EF10` 在 userdata 存入 `40BF30`，以一个 upvalue 安装 `8A99D0`。该 closure 从第 1 参数取得 Sprite 关联，从第 3 参数经 `luaL_checklstring` 及显式长度构造临时 native string，第 2 参数经 `luaL_checkinteger` 转 ordinal。`8A9B0E` 将 Sprite 放入 ECX，`8A9B10: FF D6` 间接调用 native；AL 被转换成 Lua boolean，`8A9B36: B8 01 00 00 00` 返回一个 Lua 结果。参数转换和临时 string 清理不等于加载图像。

`89F030` 存入传入的 native 指针，安装 `8A9740`。后者取得 Sprite 后以 ECX 调一次该指针（`8A9788: FF D0`），随后 `8A978B: 33 C0` 返回零个 Lua 结果；没有 ANM 引用平衡操作。这里闭合的是正常绑定调度，不声称所有非法 receiver、Lua error 或异常展开均已研究。

| native 入口 | 从原始指令确认的 ABI |
| --- | --- |
| `40BF30` | ECX=`Sprite*`；两个栈参数为 signed ordinal、native string 指针；AL 为 bool；`ret 8` |
| `40BEA0` | ECX=`Sprite*`；无显式栈参数；void／`ret`，伪代码的 EBP 参数不是新增 API 参数 |
| `40BFF0` | ECX=`Sprite*`；无显式栈参数；9-byte thunk `51 51 E8 09 00 00 00 59 C3` |
| `40C000` | ECX=`Sprite*`；另有一个当前函数体未使用的栈词，`ret 4`；不可因伪代码未使用而漏掉栈清理 |

## Replace：路径与实例 pair 分离

实例 layer 表指针在 Sprite+`0x7C`，数量在 Sprite+`0x80`，步长 `0xA0`。`40BF30` 先拒绝 `ordinal < 0` 或 `ordinal >= count`，在此之前不解引用路径参数。有效 ordinal 选择 `table + ordinal*0xA0`；不由此推断它等于 ANM 文件中任意外部 spritesheet ID。

每个实例 layer 的路径 string 在 `+0x08`，image/control pair 在 `+0x98/+0x9C`。string 长度／容量在其自身 `+0x10/+0x14`，容量阈值 `0x10` 区分 inline／heap，符合当前 MSVC string 风格布局。

`40CAE0` 比较新旧路径的长度和字节内容，不做大小写折叠、路径规范化或文件存在性检查。`40BF87: E8 54 0B 00 00` 调比较，非零在 `40BF91` 直接返回 false。不同内容才经 `40CCD0` 复制，`40BFB3: C6 44 08 30 00` 清 layer+`0x30`，返回 true。

| 情况 | 返回及状态 |
| --- | --- |
| ordinal 无效 | false，不改 layer |
| 路径字节相同 | false，不改路径或 `+0x30`；原为 0 仍可由 LoadGraphics 重试，原为 1 仍跳过 |
| 路径字节不同 | true，改路径、清 `+0x30`；不改 `+0x31` 或现有 pair |
| 空路径且不同 | 同样可被 native 接受；不代表 resolver 能加载空路径 |

自 string 别名检查位于内容相等检查之后，不能把它解释为“传同一 string 对象可强制加载同一路径”。Lua closure 使用临时 native string，也不能把 native 自别名分支当普通 Lua 技巧。

## LoadGraphics：逐层处理和失败后态

`40C000` 在 `40C043: 80 7C 03 30 00` 检查 layer+`0x30`；`40C048: 0F 85 61 01 00 00` 对非零层跳到循环尾，连 `+0x31` 清零也跳过。因此它不是无条件从磁盘刷新资源。

处理路径为：按 layer 路径调用 `9588A0 → A118D0` 取得候选 pair；候选 image 非空且其虚槽 `+0x00` 返回真时，本地结果字节为 1 并走 Sprite 图像缓存登记；否则为 0。之后取得旧 pair，旧 image 非空时先清 layer 当前 pair，再按旧 image+`0x44` 的资源路径调用 `40E740`；最后关联候选 pair。

| VA（RVA） | bytes | 含义 |
| --- | --- | --- |
| `0x40C0C9`（`0xC0C9`） | `E8 72 C5 FF FF` | 旧 image 非空分支经 `408640` 接入空 image pair |
| `0x40C13B`（`0xC13B`） | `E8 00 C5 FF FF` | 经 `408640` 关联候选 pair |
| `0x40C146`（`0xC146`） | `88 4C 03 30` | 写候选 image 的先前检查结果 |
| `0x40C14D`（`0xC14D`） | `C6 44 03 31 00` | 处理层的 `+0x31=0` |
| `0x40C1B0`（`0xC1B0`） | `81 C3 A0 00 00 00` | 下一层步长 |

`408640` 接收按值 8-byte source pair，并可从 image 复制 sampler 到 layer+`0x20..0x2C`。内部 `40C3B0` 复制／retain source 后交换到目的 layer pair，释放原目的 pair；`408640` 尾部另释放自己按值持有的 source pair。两次释放的 owner 不同。control 的 `+0x0C` 与全局回调行为沿用 [tx-03-13](tx-03-13-sprite-image-resources.md)，不能把它们合并成一次“释放旧图像”。

`A118D0` 在资源 open/resolve `A17180` 返回空时产出空 image pair；它仍可能有 control。这个正常失败返回仍进入上述 layer 更新，没有恢复旧 pair 的分支。非空但 image loaded=false 的候选也会继续尝试关联。`+0x30` 来自关联前的检查，并不验证最终 retain／目的 pair；不能外推为 OOM、失效 control 或并发下的完整成功标志。处理完成的正常失败会留新路径、`+0x30=0`、`+0x31=0`，下次可重试。

旧 pair 的临时引用和图像缓存可以继续保活旧 image。`40E740` 仅在找到 key 且可见 strong count 不大于 2 的条件下移除 Sprite 图像索引，不能从换图推断 GL 纹理立即删除。各层顺序处理，前层可已更新而后层失败；native void／Lua 零结果不提供全层成功，也没有事务回滚。`+0x31` 在本函数只写零，含义及全部其它读写者仍未知，不命名为 dirty、锁或 generation。

## Reload：重建 layer、复用 ANM、保留部分旧状态

完整 Reload 函数很短：Sprite filename string 的长度（Sprite+`0x10`）为零即返回；否则 `40BEAB → 40DB90` 绑定 ANM，`40BEB1 → 40E110` 加引用，`40BEB9 → 40C000` 处理图像。没有旧 ANM 减引用、两套动画状态 reset 或 `Load` 尾部的 shadow／星号分类。以下为原始 call bytes：

| VA（RVA） | bytes | callee |
| --- | --- | --- |
| `0x40BEAB`（`0xBEAB`） | `E8 E0 1C 00 00` | `40DB90` |
| `0x40BEB1`（`0xBEB1`） | `E8 5A 22 00 00` | `40E110` |
| `0x40BEB9`（`0xBEB9`） | `E8 42 01 00 00` | `40C000` |

`40DB90` 按当前搜索前缀和路径 key 查 ANM。已有节点且节点+`0x14` 字节为零时复用；缺节点，或该字节非零且 resolve helper 返回非空时才调用 `40ED50`。字节的完整语义未定，不直接命名为“过期”或“强制刷新”。因此 Reload 不保证重读 ANM 文件。

找到可绑定节点后，非零层数且分配有效的正常路径销毁旧的 `0xA0` 实例 layer 数组（`40DE85` 选择 `407A60` 析构），新建数组并在 `40DF91` 调 `408310`。该 helper 从缓存 layer 定义的路径初始化实例 string，清图像 pair，并重置包括 `+0x30/+0x31` 在内的实例字段；owner+`0x04` 重新指向 Sprite。Sprite 的动画数组、名称和事件数组则借用该缓存节点的存储，default animation 名称也被复制。零层数据和分配失败的对象后态不从这条正常路径外推。

所以对于同一有效缓存：`Replace(P) → Reload()` 会重置为 ANM 路径；`Reload() → Replace(P) → LoadGraphics()` 才会把本次实例路径改为 P。这里只解释操作次序，不推荐重复 Reload 作为 adapter 日常行为。

`Load` 还会经 `408830` 清 Sprite+`0x30/+0x50` 两套动画 state，写 Sprite+`0x109=1`，重新扫描 shadow index（`+0x10C`）与星号相关字段（`+0x110`）。Reload 没有这些操作。实例 layer 重建不等于 Sprite 全部字段复位，缓存结构变化时不能保证旧动画 descriptor 或分类仍正确。

## ANM 计数与失败条件的静态推导

`40E110` 对找到的节点+`0x50` 加一，原始点 `40E26E: FF 46 50`。`40E2B0` 在非零时减一，`40E42C: 89 46 50` 写回。`40E470` 的 `40E4A0: 83 7E 50 00` 只让零引用节点进入 payload 清理和删除。layer 析构 `407A60` 只释放 image/control 和路径 string，没有 ANM 减引用。

**STATIC_DERIVED：稳定节点上的额外引用。** 假定其它 owner 贡献为 c，搜索前缀／key 不变、节点持续命中、不发生额外平衡／并发、正常返回：一次 Load(F) 得 `c+1`；k 次 Reload 得 `c+1+k`；随后 Reset 仅减一次得 `c+k`。Reload 的 Lua closure 也没有另做平衡。k>0 的增量可阻止正常零引用 sweep，属于局部静态计数问题；没有测量运行时内存增长，也不声称所有外部调用者都无法补偿。

**STATIC_DERIVED：Load(G) 失败后旧借用关系可能失配。** 限定旧 F 有效、该 Sprite 是最后一个 F 引用，G 对应不同且不存在的缓存节点、open 返回空、搜索关系稳定且正常返回：

1. `Load` 先在 `40BD60` 调 `40E2B0`，F 的计数可变为零，再把 filename 改为 G。
2. `40ED50` 的资源 open 失败在 `414500` 创建／取得节点之前返回；`40DB90` 随后仍查不到 G，只记录 `AnmCache failed to load`，不替换 Sprite 的旧 layer／借用数组字段。
3. G 的加引用只记录 cannot add reference。Load 仍清动画 state 并写 `+0x109=1`，没有恢复 F filename／引用，也没有按上述绑定失败提前中止。
4. 若随后正常零引用 sweep 删除 F，其共享 layer 定义、动画／名称／事件存储可失效，而 Sprite 的部分旧字段仍曾指向它们。

这不证明立即 draw、崩溃或所有加载失败均有此结果；动画 state 已被清理、其它 F owner 可保活、同 key／已有 G 节点会改变路径。它证明不能把失败后的 IsLoaded 或旧数组存在作为继续发布 Replica 的依据。

另一个重解析分支中，`40ED50` 在取得节点 payload 后先调用 `40D850` 清原有共享数组，再解析 XML；该 helper 不先等待所有借用者退休，也没有通过节点引用数实现版本化替换。正常缓存引用计数并不赋予原地重解析的事务性。完整 XML 失败、OOM、EH 与 worker 并发仍未闭合。

## 换图到 draw 的有限补充

`409120` 的已读分支要求有效帧／可见性和非空 layer image，再进入几何 helper 与 image 虚槽绘制；`409120/4098A0/409CB0` 本体未读 layer+`0x30/+0x31` 来自动加载新路径。因此单独 Replace 之后旧 pair 仍可在其它条件满足时进入绘制路径，不能认定新路径已经显示，也不保证实际像素结果。

`4098A0` 还有条件性共享状态写入：当 `(layer+0x8C | Sprite+0x110) & 2` 非零时，用全局 `C33914` 和 shift 参数生成 quad 扰动；`409AF0: 89 3D 14 39 C3 00` 与 `409B7D: A3 14 39 C3 00` 两次写回该全局。它不是纯读坐标。该 RNG 的全部下游用途未知，不能称为已确认 gameplay RNG，也不能把它与此前派生弹体的局部随机路径混同。首版视觉支持范围必须明确处理此类特殊标志。

## 后续适配约束与剩余研究

后续实现应保存逻辑资源描述而非发送 string 内部指针／image/control／ANM 借用地址；资源操作在 [tx-03-14](tx-03-14-renderer-context-lifecycle.md)解释的主线程与有效 context 生命周期内调度。正常路径来源不代替运行时 context guard。换图失败需停止相关候选的展示发布，记录已执行前缀，并按已理解 owner 正常退休；本轮没有证明可把原对象回滚到旧状态。

建议下一轮收敛首版独立 Sprite 的最小展示字段：主／overlay 动画与目标帧、layer ordinal 和资源覆盖、颜色／变换／flip／可见性，以及 draw 扰动等特殊标志的支持边界。随后再接独立 Sprite 资源应用／退休、Player 组合绘制、其余实体和 Room／Grid 的原生副作用隔离。renderer 重入、异常恢复与完整 Replica 创建／应用仍未实现。

本轮唯一一次验收为全部编辑后的整体静态文档审阅，见 [主审记录](tx-03-15-primary-review.md)及 [结果 artifact](../../../artifacts/cs-t0/tx-03-15-static/final-static-review.json)。它不运行游戏、构建、单元测试、E2E 或资源实验，也不把静态文档一致性当作运行时功能通过。
