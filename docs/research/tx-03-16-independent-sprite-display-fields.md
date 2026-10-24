# tx-03-16：独立 Sprite 的最小展示字段与支持范围

日期：2026-10-23（Asia/Shanghai）。继续 tx-03-15 的建议，本轮解释独立 Sprite 的 main／overlay、目标帧与 layer 索引、变换颜色及特殊视觉依赖，形成有限展示描述候选。只做静态逆向与文档，不实现或运行 Replica。用户指定 GPT-6 Luna / max 子任务研究 draw 字段，主智能体独立研究动画路径并审阅子结论，见 [主审记录](tx-03-16-primary-review.md)和 [Luna 报告](tx-03-16-luna-sprite-fields.md)。

## 结论

“资源路径＋动画名字＋总帧”不能还原任意原生 Sprite。main 与 overlay 有各自的 descriptor 和逐 track 关键帧索引；`SetLayerFrame` 可只改 main 的一个索引，不改变总帧。因此在支持独立 layer 覆盖时，只按总帧重新计算会丢失姿态。

本轮建议首版收敛到**固定姿态的独立 Sprite**：Authority 给出资源声明、动画与选中关键帧、实例外观；客户端不调用 Sprite.Update，不复制原生 event masks，不保留未知特殊 flags。这个候选只覆盖声明的视觉集合，不承诺完整 Entity／Player 外观；它是 `DESIGN_PROPOSAL`，还没有 wire schema、采集或 apply 实现。

原生 setter 本身也不是任意参数下安全的写回 API：目标帧 helper 会写事件位，`SetLayerFrame` 对无效 track 映射只诊断后继续访问，非 reset 动画切换还有数组复制／目的字段异常分支。即使名字为 Render，也会改变图形全局状态，特殊分支还依赖共享 RNG 和 Game 全局。

## 证据、地址与原始记录

活动 IDB 输入路径为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，imagebase=`0x400000`，`RVA = VA - 0x400000`。字段偏移以表中各对象为基址，不能混作 RVA；路径相同不证明 IDB 与磁盘文件内容相同。没有计算或校验任何哈希，没有执行原生函数。

| 主审 artifact | 内容 |
| --- | --- |
| [primary-animation-initial.json](../../artifacts/cs-t0/tx-03-16-static/primary-animation-initial.json) | state reset／目标帧／推进／事件，Sprite Render 包装与具名注册区间 |
| [primary-named-animation.json](../../artifacts/cs-t0/tx-03-16-static/primary-named-animation.json) | Play、SetAnimation、SetFrame、SetLayerFrame、overlay、查询、binding helper |
| [primary-state-selection.json](../../artifacts/cs-t0/tx-03-16-static/primary-state-selection.json) | 具名选择、track 索引推进、draw 调度、实际 Lua closures |
| [primary-animation-rebind.json](../../artifacts/cs-t0/tx-03-16-static/primary-animation-rebind.json) | 非 reset 切换、Lua 参数读取；包含两处内部地址请求的实际函数起点，见主审限制 |
| [primary-animation-binding.json](../../artifacts/cs-t0/tx-03-16-static/primary-animation-binding.json) | SetAnimation 的实际 Lua closure 与 boolean 返回 |
| [primary-draw-review.json](../../artifacts/cs-t0/tx-03-16-static/primary-draw-review.json) | 主审独立导出的三段 draw 函数与 SetAnimation 注册 helper |
| [primary-property-review.json](../../artifacts/cs-t0/tx-03-16-static/primary-property-review.json) | 主审独立导出的具名属性 getter／setter、44-byte Color 及当前颜色栈项 helper |
| [primary-color-composition.json](../../artifacts/cs-t0/tx-03-16-static/primary-color-composition.json) | 确认 bit `0x01` 路线是颜色选路与分量组合，不是矩阵选择 |

`STATIC_CONFIRMED` 为当前指令及数据流；`STATIC_DERIVED` 为依赖分析；支持集合与 DTO 均为设计候选。没有运行时姿态、像素、资源成功率或副作用观察。原始文件含完整 bytes、伪代码和静态 xrefs；只读可重复提取见 [export_static.py](../../artifacts/cs-t0/tx-03-16-static/export_static.py)，须选择新的输出路径，不覆盖原始记录。

## 三层索引与两套动画 state

| 对象／字段 | 当前静态含义 |
| --- | --- |
| Sprite+`0x30 / 0x50` | main／overlay animation state，各 `0x20` bytes |
| state+`0x00` | 本地 Sprite owner 指针 |
| state+`0x04` | ANM animation descriptor 借用指针 |
| state+`0x08 / 0x0C` | visual track／另一类 track 的实例 selected-key 索引数组 |
| state+`0x10` | f32 动画位置；不是每层单独的位置 |
| state+`0x14` | playing byte |
| state+`0x18 / 0x1C` | 当前／历史事件位 |
| descriptor+`0x18 / 0x1C` | visual track 表指针／数量，表项 stride=`0x10` |
| descriptor+`0x20 / 0x24` | 第二类 track 表指针／数量；直接 Sprite Render 不遍历它们 |
| descriptor+`0x30 / 0x34` | 动画长度／循环 byte，供设置和推进使用 |
| descriptor+`0x38 + 4*layerOrdinal` | Sprite layer ordinal 到本动画 visual track index 的映射 |
| visual track+`0x00 / 0x04 / 0x08 / 0x0C` | Sprite layer ordinal／关键帧记录表／关键帧数／active byte；关键帧记录 stride=`0x6C` |

descriptor、track 和关键帧表来自共享 ANM 资源，不能作为实例包发送或随意改写。Sprite layer ordinal、动画 track index、该 track 内 selected-key index 是三个不同的整数域；资源文件外部 spritesheet ID 也不能混入它们。

`40A030` 的 raw 在 `40A058` 从 descriptor+`0x18` 装 ECX，`40A05E` 加 track byte offset，再在 `40A066` 调 `409120`；`40A06F` 以 `0x10` 步进。因此 `409120` 的 ECX 是 visual track，不是 animation state。它检查 track active、selected-key index 和记录可见性，再按 track 的 layer ordinal 从 Sprite 实例 layer 表取外观。

## 具名接口的真实行为

Sprite 注册区间见 primary-animation-initial 的 ranges。下表登记 native VA／RVA；这些 native 成员地址不是直接接收 lua_State 的 Lua C 函数。

| 接口 | native VA／RVA | 正常路径 |
| --- | --- | --- |
| Play | `0x40A360 / 0xA360` | 转到 `40A380`，需要重播时强制 reset，`40A1B0` 初帧，playing=1；同名非 force 不保证重新开始 |
| SetAnimation | `0x40A5B0 / 0xA5B0` | 名字匹配后按 reset 参数选 `408830` 或 `408970`，不等价于 Play |
| SetFrame（名字、整数帧） | `0x40A690 / 0xA690` | 强制 reset 到 main descriptor，再以 XMM1 设置目标帧，停止 main playing |
| SetFrame_1（整数帧） | `0x40A720 / 0xA720` | 仅对已有 main descriptor 调目标帧 helper；不 reset、不停止 playing |
| SetLayerFrame | `0x40A7E0 / 0xA7E0` | layer ordinal 映射到 main track，只重算该 track 的 selected-key index |
| SetLastFrame | `0x40A760 / 0xA760` | 经 `408D00` 前进到 length-1，会走跨帧事件路径，不是直接赋一个标量 |
| PlayOverlay | `0x40A920 / 0xA920` | 对 overlay 执行对应的 reset／初帧／playing=1 |
| SetOverlayAnimation | `0x40AAD0 / 0xAAD0` | overlay descriptor 选择，reset 与非 reset 路线分开 |
| SetOverlayFrame | `0x40ABB0 / 0xABB0` | 强制 reset overlay 后调目标帧；reset 已令 playing=0，不因缺少另一条显式停止指令就称其继续播放 |
| RemoveOverlay | `0x40AC60 / 0xAC60` | `408830(state=Sprite+50, descriptor=0)` 清 overlay 及索引数组 |
| Update | `0x409100 / 0x9100` | Sprite+109 非零才顺序推进 main 和 overlay；不是只读或只 draw |

`408830` 重建索引数组并初始化为 -1，frame=-1、playing=0、两组事件位=0。`40A1B0` 则设 frame=0、清事件位后重算初帧事件，两个索引数组置零。SetFrame 的名字查找失败只警告并保留旧状态，不提供事务恢复；不能靠旧 descriptor 仍非空就认定新名字已接入。

`408E00` 的 ECX=state、XMM1=f32 请求帧，无额外帧栈参数；它用 nextafter(length,0) 做上限裁剪，清当前事件位、写总帧、重算事件和两类索引。传给 `408B90/408C10` 的 XMM2 仍是原请求帧，未改为裁剪后的值。负数、非有限值与资源不合法不是完整拒绝契约。`408B90` 按 visual keyframe+`0x68` 的时间边界前进索引；总帧只是默认索引重建的输入，并非 draw 唯一依据。

`SetLayerFrame` 不改总帧、playing 或事件位。`40A81F` 映射 layer 到 track，`40A84D` 先清该 selected-key，再 `40A854 → 408B90` 根据传入整数帧选 key。因此两个 Sprite 可以拥有相同名字、相同总帧，却绘制不同关键帧。全层目标帧设置会重算这些索引，覆盖先前独立层姿态。

`GetFrame(40A780)` 对 main f32 取 floor，`GetOverlayFrame(40AC50)` 转整数；二者均不返回完整分数相位或全部逐层姿态。不能用查询得到的两个整数重建任意完整 state。直接 draw 当前没有读 playing、speed 或 event masks；这些字段主要服务推进与消费者，固定姿态方案可以不复制它们，但不能把同一原生对象的 gameplay 消费者继续保留。

### Lua 包装层与 native ABI

`89F150` 为 Play／PlayOverlay 安装 `8A9520`，`8BA6D0` 读取第 2 参数 string、第 3 参数 truthiness；native 返回内容被丢弃，Lua 返回零个结果。`89ED10` 为 SetAnimation／SetOverlayAnimation 安装 `8A9C20`，同样解析参数，但将 native AL 转为一个 Lua boolean。

`89F0F0` 安装 `8A95F0`，`8BA5E0` 读取名字和整数帧；`89F090` 为 SetFrame_1 安装 `8A96D0`，读取一个整数；`89EC10` 为 SetLayerFrame 安装 `8A9D70`，读取两个整数。这三类 closure 都调用 native 后返回零个 Lua 结果。没有从 SetFrame 的 native 残余 EAX 值发明 Lua 成功结果，也没有证明脚本端存在另一套自动重载调度。

`40A690/40ABB0/40A7E0` 均 ECX=Sprite，两个栈参数，`ret 8`；`40A720` 为一个整数栈参数。`40A360/40AAD0` 原始指令保留 ECX 并 tail-jump 到真正成员 helper，反编译器显示 `__stdcall` 不表示它们不需要 Sprite receiver。可调用 ABI 仍以这些 raw 指令为准。

## 不宜直接复用的边界分支

`40A7E0` 先保护外部 layer ordinal 的范围，但取到 track mapping 后，`40A837` 的 `id out of bounds` 日志后仍执行 `40A84D` 数组写入。`408B90` 的诊断路径也继续取数组。正确 layer ordinal 不保证该动画包含此层；若 mapping=-1 或越界，不能把日志当安全拒绝。这是静态可达风险，没有运行此非法调用。

非 reset helper `408970` 还保留 frame、playing、events，并在计数相同时保留旧 key indices。计数不同时，按新 visual count 分配，`408A62` 却按旧 count 复制；old>new>0 时存在复制长度超出新容量的条件推导。第二类 track 重分配中，`408B63` 清 state+`0x0C`，`408B6A: 89 5E 08` 却把新数组写到 state+`0x08`。此分支可破坏两组索引关系；不把它当安全的“无损换动画”原语。未观察实际触发、内存破坏或崩溃，也未修补原生代码。

## draw 的姿态依赖

`40A0D0/40A140` 首先读 Sprite+`0x109`，但它只是已知标志而非资源完备证明。Sprite+`0x70` 为非零时先 overlay 后 main，为零时先 main 后 overlay；顺序是姿态描述的一部分。RenderLayer 也会按映射分别尝试两套 state，不代表只绘制 main。

每条轨道还需 active、selected-key 有效、关键帧记录可见、实例 layer+`0x74` 可见及非空 image。下层从 ANM 关键帧记录取固定 rect／位置／scale／颜色等数据，再叠加 Sprite／layer 实例外观。只读偏移不自动具有 setter 或网络写回资格。

| 实例对象／字段 | 最小描述中的用途 |
| --- | --- |
| Sprite+`0x94/+0x98` | Offset 二维量，与 draw 位置参数分开 |
| Sprite+`0x9C/+0xA0`、`+0xA4` | Scale 二维量、Rotation |
| Sprite+`0xA8` | 44-byte Color 结构；颜色乘合和 shader 选择还受 flags／全局条件影响 |
| Sprite+`0x100/+0x101` | FlipX／FlipY，与负 scale 及 layer flip 共同决定顶点／变换 |
| Sprite+`0x104` | PlaybackSpeed；固定姿态无本地 Update 时不作为 draw 驱动输入 |
| Sprite+`0x110` | 特殊视觉 flags，不能无条件原样开放 |
| layer+`0x32/+0x33` | 实例层 flip bytes |
| layer+`0x34/+0x38`、`+0x3C` | 实例层 scale 二维量、rotation |
| layer+`0x40/+0x44`、`+0x48` | 实例层位置偏移、44-byte Color |
| layer+`0x74`、`+0x8C` | 可见性、特殊 flags |
| layer+`0x90/+0x94` | 额外 quad 偏移；不能误作 Sprite Offset |
| layer+`0x08`、`+0x98/+0x9C` | 逻辑路径覆盖与原生 image/control 分开；加载／退休沿用 tx-03-15 |

这些字段的具名属性和 draw 证据由 Luna 提供并由主审重读关键函数。完整属性 getter/setter、颜色／符号缩放／特殊分支的细节与原始导出见其报告；未确认的字段不改名为产品语义。

`4098A0` 根据 frame 与 layer scale 处理 crop，存在除法；Sprite／layer flip 和 scale 符号共同影响顶点排列，不能把一个 FlipX 位当作全部镜像行为。`409CB0` 在特殊 bit `0x08` 下改变缩放组合方式；还读画面镜像全局和条件 Game offset。`409120` 会临时写／恢复 renderer 的 sampler 全局，并切换图形状态；正常尾部恢复不证明异常或重入安全。

bit `0x01` 选择 layer Color，否则选 Sprite Color 或当前颜色栈项，再与 frame Color 组合；它不是矩阵开关。`4071C0` 是 44-byte Color 复制，`4072E0 → 6831C0` 对前四分量相乘、中间四分量条件组合、后三分量相加。`407170` 直接初始化为前四个 1、其余七个 0；这能说明该 helper 的内容，不等于已追到所有 Sprite 构造默认或所有 shader 的相同语义。

特殊标志合并值为 `layer+0x8C | Sprite+0x110`。已读 bit `0x02` 使 `4098A0` 两次更新共享 RNG `C33914` 并扰动 quad；bit `0x04` 依赖 renderer 全局才放行；其它着色分支会依赖第二颜色、Game 帧计数／shader 等。该 RNG 的 gameplay 用途仍未知。清未知 bit 虽可得到另一种画面，却不能称为忠实复制原对象。

## 固定姿态候选的最小逻辑描述

以下只针对声明的 ANM／图像资源、无未知特殊 flags 的独立 Sprite，字段名字是设计候选，未写入现有协议。没有新增“通过后解锁”流程或 gate。

| 逻辑信息 | 是否需要／如何减少冗余 |
| --- | --- |
| session／room／visual ID、资源修订与支持声明 | 本地 owner／代次关联；不发送 native pointer、缓存节点或 vptr，不计算资源哈希 |
| ANM 资源名、逐 layer 图像路径覆盖 | 未改资源时可引用已声明本地描述；不能复制 string 内部布局或 image/control pair |
| main 动画名与逐 visual track selected-key index | draw 的直接姿态；key index 按本地相同资源／动画 track 定位并验证范围，不发送地址 |
| overlay 是否存在、动画名、selected-key index、绘制先后 | overlay 可完全缺省；不存在与保留上次 overlay 必须区分 |
| Sprite／layer 外观覆盖 | 变换、颜色、flip、可见性和额外偏移；与明确默认值相同可省略，省略表示恢复默认而非沿用任意旧状态 |
| draw 位置与 crop 两向量 | 由客户端相机／视口及复制的逻辑位置产生；零 crop 可作为首版固定约束，不把一台机器的屏幕坐标当通用网络坐标 |
| 总帧／fractional phase | 若只按完整 selected-key 姿态绘制，可作诊断元数据；若依赖帧值重建索引或未来本地推进，必须另定义准确相位，整数 getter 不够 |
| playing／speed／loop／event masks | 固定姿态本地不 Update，playing 维持停止；loop 来自资源。事件播放沿独立表现事件，不复制／消费 native masks 来裁决世界 |

选中 key indices 的方案并非可直接写数组的实现授权；未来 adapter 仍要按本地已构造且资源匹配的对象管理数组长度、owner、引用及失败停显。flags 全零时，`409120` 不走使用 layer+`0x48` Color 的 bit `0x01` 分支，该 profile 可省略 layer Color；第二 Sprite Color 与特殊 shader 参数也不进入此 profile 的实例描述。layer+`0x30` 图像处理标志和未知 `+0x31` 不与 flip bytes 打包复制。另一种受限描述可以只给动画帧，并明确拒绝所有独立 layer 覆盖；这会缩小支持范围，不能悄悄丢掉覆盖。

首版候选将特殊 flags 合并值限定为零，视觉索引允许明确的无帧值 -1 或本 track 范围内的 key，所有资源 track 到实例 layer 的映射必须有效，参与 crop 除法的 frame×layer scale 有限且非零。颜色的 44-byte native 布局用于定位，未来 wire 仍须逐分量定义支持语义，不能直接搬运结构体。外观默认值以本地构造／Load 及声明资源为准，不盲目把所有 bit 清零；例如 Load 的星号／shadow 分类可能表明该资源不属于本集合。字段缺失、资源不支持或 native apply 部分失败时停止相关候选发布，不沿旧姿态伪装应用成功。

这不消除 renderer 全局、镜像、Game offset、GL context 或资源寿命要求；它们是本地展示环境与 owner 约束，不是逐 Sprite 可复制指针。尤其颜色栈非空（`C78DC0`）会替代 Sprite Color，`C7977D/F` 条件分支也不受特殊 flags 全零消除。首版候选要求绘制 pass 明确采用无颜色栈覆盖、上述两 byte 为零且 Game offset／视口变换已定义的环境；不靠未知全局值获得偶然一致，也不授权直接清这些全局。首版直接 Sprite Render 不需要借用 Entity／Room 的 gameplay update，也不覆盖 Player 组合外观、派生几何、HUD 或其它持久副作用。

## 本轮完成范围与下一步

本轮已把固定姿态所需实例数据、资源派生数据、可省略的推进／事件状态和不支持的特殊分支分开，并记录原生接口的实际限制。当前 Authority 采集 profile 尚未包含这套描述；Native adapter、schema、资源 staging、失败退休和 E2E 像素／姿态验证都未实现。

建议下一轮研究独立 Sprite 的创建—资源准备—姿态应用—失败退休的完整正常 owner 路径，把本轮描述对应到可执行调用顺序；随后再选择有限资源／对象集合实现。Player 组合绘制、Room／Grid 副作用与 renderer 重入仍单独未决，不扩大本轮为全部引擎闭包。

唯一一次整体静态文档验收见 [主审记录](tx-03-16-primary-review.md)和 [结果 artifact](../../artifacts/cs-t0/tx-03-16-static/final-static-review.json)。本轮没有运行游戏、构建、单元测试、E2E、目标 helper 或调试实验，没有修改 IDB／生产代码。
