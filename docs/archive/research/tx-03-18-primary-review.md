# tx-03-18：主智能体复核与整体静态验收记录

日期：2026-10-23（Asia/Shanghai）。目标是把独立 Sprite 的资源准备与固定姿态对应到有限的适配接口，而非实现或运行 Replica。用户要求继续 GPT-6 Luna / max 子智能体研究并由主智能体审阅；本轮子任务为 `/root/sprite_ready_tx0318`，模型 `gpt-6-luna`、reasoning effort=`max`。

资料入口：[现行规范](../refactor/project-conventions.md)、[主报告](tx-03-18-single-sprite-adapter-contract.md)、[Luna 报告](tx-03-18-luna-resource-readiness.md)。所有事实、静态推导和设计建议按主报告口径区分，没有以旧计划、gate、哈希或独立角色要求限制推进。

## 对子结论的复核

主智能体独立调用 IDA 只读导出，获得 [primary-readiness-review.json](../../../artifacts/cs-t0/tx-03-18-static/primary-readiness-review.json)，重读 `40C000`、`40C550`、`408590`、`408640`、`40C3B0`、`40C6B0`、`A123B0`、`A12B80`、`A69B30`，以及 PNG／control 两张表。没有只凭子报告摘要给出结论，也没有执行任何目标函数。

| 子结论 | 主审依据与处理 |
| --- | --- |
| layer+30 不代表最终 pair 成功 | 采纳。主审 raw 在 `40C080` 先记 candidate loaded；`40C106` 才调 control tryretain，`40C10A` 的 false 分支跳过 pair 填充；`40C146` 仍写之前的标志。没有把静态分支说成已发生的正常运行失败 |
| `40C550` 返回值不是成功 bool | 采纳。目标 pair 先清零，源 control 非空且 tryretain 为真才复制；返回目的地址不能替代检查两个字段 |
| `408590` 不可作纯 readiness getter | 采纳。主审原始 `4085D2: FF 50 44` 与 `40860B: FF 50 48` 在复制 pair 前调用 sampler 槽，可能改共享 image／GL 状态；不因随后 retain 失败就撤销前缀 |
| PNG loaded 只代表逻辑位 | 采纳。PNG slot0→`A123B0` 读 image+8 bit0；`A12B85` 先置位；`A69BD6: B0 01` 固定成功，不含 GL error/readback 成功合同 |
| 最终 pair／control 一致性可构成有限条件 | 限定采纳为设计。有效 owner、正常顺序、已知本地后端、代次和线程／context 必须先成立；vptr、计数、image 指针相等不证明失效指针安全、ABA 或跨线程寿命 |
| 尺寸或 texture id 可直接证明可绘制 | 不作此结论。尺寸可另作支持清单范围，非零纹理名只可诊断，不加入 GPU 成功语义 |

交流中子任务最初把 `40C073`／`40C0FD` 概括为调用位置，主审用 raw 明确：前者为读取 vptr，后者为 control 非空测试；实际调用为 `40C077: FF D0` 与 `40C106: FF D0`。子报告已改为精确指令角色，不把读取地址或测试地址当 callsite。

子任务采集有两项限制，均保留原始记录：

1. 首份 [luna-image-readiness.json](../../../artifacts/cs-t0/tx-03-18-static/luna-image-readiness.json) 复制旧 exporter 时留下 `tx=tx-03-17`。修正脚本后，以新文件 [luna-image-readiness-v2.json](../../../artifacts/cs-t0/tx-03-18-static/luna-image-readiness-v2.json) 记录正确本轮标签；旧文件不覆盖、不作为本轮合格元数据主证据。没有发生验收重跑。
2. 子导出对 PNG 表读了 25 个字，额外 `+60` 的 bytes=`B4 14 BB 00` 指向 `BB14B4`，当前 IDA 标识为 `ImageJpg` RTTI Complete Object Locator。它是相邻数据指针，不是 PNG 第 25 个函数槽；报告明确排除，主审导出为正确的 24 槽 `+00..+5C`，子 exporter 默认也改为 24。额外地址的 VA/RVA 仍是数据指针换算，不能用来证明函数槽。

## 主线新研究及合同收敛

主智能体通过 [pose／ANM 导出](../../../artifacts/cs-t0/tx-03-18-static/primary-pose-validation.json)、[descriptor／几何导出](../../../artifacts/cs-t0/tx-03-18-static/primary-descriptor-origin.json)及 [mapping 构造导出](../../../artifacts/cs-t0/tx-03-18-static/primary-mapping-bound.json)，补充下列具体边界：

- `40BEC0` 在 descriptor+38 填 64 个 DWORD；`40BEC6` 的 40h 次数与 parser 单独交换 +138，说明 stride13C 末尾不能当作第 65 项映射。首版支持 1..64 层与唯一 ordinal 是设计限制，不声称所有原版资源都满足。
- main／overlay helper 按 C 字符串精确比较、descriptor stride13C 查找第一个匹配，reset=true 分别对 Sprite+30／50 调 `408830`。AL=true 只说明所选名字匹配；内嵌 NUL、重复名字及本地 descriptor 对应关系需在 adapter 合同中处理。
- `408830` 依据两套 descriptor count 创建本地索引数组、填 -1，并写 frame=-1、playing=0、事件=0。分配后直接 memset 的代码不提供完整 OOM 恢复；本轮只解释正常构造和 helper 返回的对象。
- `40A030` 从 state+8 读 key，`409120` 检查 key 范围后却直接按 track.ordinal 访问实例 layer，不能仅验证网络 key 而放过资源 mapping。零 crop 也仍执行除法，frame×layer scale 分母必须有限且非零。
- 完整 key 向量经事先验证后写入未发布候选，保持停止／零事件，是适配方案而非现有功能。不调用 SetFrame／Update 模拟姿态，不改共享 ANM 记录、不写 Entity／Room，也不把 native swap 当完整提交。

合同明确 Plan、Prepare、Inspect、Publish 与 Retire 的 owner／前缀语义，本地 logical key／revision 和不复用 instance token 负责关联；未来按实际模块基址使用 RVA。高频 live pose 事务、整批 baseline apply、资源具体内容匹配和运行 guard 均未实现。文件／资源元数据不冒充内容相同，也不计算资源哈希。

资源目录静态搜索仅得到 `packed`／`scripts`，没有散装 `.anm2/.png`；`resources-dlc3` 不存在的读取错误保存在 [采集说明](../../../artifacts/cs-t0/tx-03-18-static/primary-acquisition-notes.json)。没有解包、加载或虚构一个已可用样本。

## 修改范围与未决项

新增主报告、Luna 报告、本审阅记录及 `tx-03-18-static` 的只读证据／采集源／整体审阅入口；更新 `docs/archive/README.md`、`docs/archive/hooks/README.md`、`docs/archive/hooks/entity.md` 与 `docs/archive/design/state-synchronization.md`。生产代码、Hook、机器索引与 IDB 未修改；历史报告及 tx-03-16 原 FAIL 不改写、不重跑。

原生 final pair 一致与逻辑 loaded 不证明 GL／像素成功。实际线程／context、renderer draw 接入、具体资源清单、Authority 采集、Native adapter、registry、OOM／EH／worker／热重载、Player 组合与 Room／Grid 副作用仍未完成。运行时 guard 未实现不改变当前静态研究结论的有限性质；后续直接按研究目标推进，不引入 gate 或旧解锁字段。

## 唯一一次整体静态验收

全部研究、子结论复核与文件编辑后，统一执行 [review_documents.ps1](../../../artifacts/cs-t0/tx-03-18-static/review_documents.ps1) 一次，结果以 [final-static-review.json](../../../artifacts/cs-t0/tx-03-18-static/final-static-review.json) 为准。脚本检查本轮三篇报告与四处入口、相对引用、明确 VA/RVA 声明、原始证据地址和关键指令，并直接比较主／子独立取证的共同函数及正确表项；语义复核由上文说明，不以字节一致代替语义或运行行为。

审阅器明确区分旧误标元数据与当前 v2、相邻 RTTI 数据与 PNG 函数槽；它不删除或掩盖采集异常。没有执行旧验收，也没有检查任何哈希。若本轮审阅失败，保留结果，不自动重跑；本文不预先宣称结果通过。

本轮整个分析和编辑期间未运行其它测试、程序实验、构建、游戏、调试或 native helper。下一项建议继续实际绘制接入窗口的静态逆向，将加载循环旁路、caller 参数／pass 状态与未来 thread/context guard 对应到现有 C/S owner。
