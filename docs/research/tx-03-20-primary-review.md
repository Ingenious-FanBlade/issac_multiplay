# tx-03-20：主智能体审阅记录

日期：2026-10-23（Asia/Shanghai）。本轮完整目标为具体单 Sprite 资源、Authority 同窗口姿态采集、同资源连续更新和资源／descriptor 改变的正常替换顺序。按会话要求使用 **GPT-6 Luna / max** 子智能体研究采集／更新，主智能体阅读其[报告](tx-03-20-luna-pose-capture-update.md)与[原始资料](../../artifacts/cs-t0/tx-03-20-static/luna-pose-capture.json)，独立读取原生函数和现行 C/S 源码，并补充资源、PNG 和 Entity draw 的研究。[主报告](tx-03-20-sprite-resource-pose-closure.md)为本轮结论入口。

本轮只做静态逆向、已有 XML／PNG 文件头读取和文档编辑；没有实现 adapter、姿态采集、registry 或新 Hook，没有执行资源加载、PNG 解码、游戏／调试、构建或中间测试。生产代码、游戏资源和 IDB 未修改。支持声明中的预期 native shape、owner 服务点和写入算法仍为设计方案。

## 对子智能体结论的裁定

| 项目 | 主审依据与采纳范围 |
| --- | --- |
| 同一个 Authority 采集窗口 | 采纳。直接阅读 UpdateHook、world StageEvent／Capture、coop normal-return witness；fresh `_main` 的 update→render 两个相邻 callsite。扩展应置于同一个普通完整 Capture、最终稳定性检查及 FinishBatch 前；当前源码没有 Sprite pose 字段，不把建议位置写成已实现。 |
| 逐 track key 是 direct draw 的姿态输入 | 采纳受限结论。主审独立导出 `408B90`、`40A030`、`409120`，确认 descriptor／track／selected-key 的读取链；不是用总 frame、墙钟或客户端 Update 重算 Authority 姿态。Render 仍有图形状态及特殊几何副作用，不能充当 getter。 |
| 同 descriptor 可以连续更新 | 采纳为设计方案。主审另读 `40A5D0`、`40AAF0`、`408830`、`40A7E0`、`409100`、`409030`。原生 setter/reset/Update 的写入不等于完整 pose apply；仅在已管理的独立 Sprite、固定资源及数组关系、主线程无重入的正常顺序下规划完整向量与外观，再不调用 native helper 地写入、读回，最后提交本地 pose revision。不是硬件原子事务，也不保证 OOM/EH 恢复。 |
| 改动画／资源要新候选 | 采纳。live 实例不逐个 reset/RemoveOverlay 后补字段；候选在最终 draw 外准备，完成检查后切换关联。失败不把旧实例记成目标已应用，也不提前推进整批 AppliedAck。 |
| XML 15 帧对应 native 15 keys | 未作为事实采纳。子报告已保留预期限定；主审 `40ED50` 的 interpolation/root-frame 调整分支进一步支持该限制。只能在实际候选构造后读取 shape。本轮没有这种运行观察。 |
| 单 Sprite 姿态代表完整 Entity 画面 | 不作该外推。子任务未展开全部 Entity render；主审另外 fresh 读取 `6B10F0`、`6B2000`，确认 Entity+48 receiver、shadow pass 临时 Color 和 layer visible 强制写0。报告已补充该限制，持久 Sprite 采样不自动复现 Entity 组合。 |

主审与子任务共同 fresh 导出的九个函数见 [primary-sample-parse-detail.json](../../artifacts/cs-t0/tx-03-20-static/primary-sample-parse-detail.json)与 [primary-pose-owner-review.json](../../artifacts/cs-t0/tx-03-20-static/primary-pose-owner-review.json)。最终统一静态审阅会直接比较双方指令地址和原始 bytes 字符串；这只核对引用资料，不代替本表的语义裁定，不计算哈希。子任务本轮的20个函数请求均有实际函数资料，没有把函数请求次数当作动态覆盖率。

## 主审补充与修正

本轮从安装根目录发现已有 `extracted_resources/resources`，选择 `gfx/1000.011_Bullet Poof Static.anm2` 的 Poof 和对应 PNG，保存[文件观察](../../artifacts/cs-t0/tx-03-20-static/sample-resource-observation.json)及[支持声明草案](../../artifacts/cs-t0/tx-03-20-static/sample-support-declaration.json)。XML 一层／一动画／15个无插值帧，PNG 前29字节为256×256、RGBA8、非交错；没有读取或验证 CRC，没有解码像素。entities2.xml 的 Type1000／variant200 只是文件关联，不获得新 Effect Entity 支持或 source owner 资格。tx-03-18 当时只搜索 resources 的未发现结果保留，不能将本次扩大搜索写成旧结果错误或游戏已加载该文件。

`67F670` 的实际路径合成会统一输入反斜杠、移除 ANM 文件尾项并处理相对目录；本样本推导为 `gfx/effects/Effect_003_BloodTearStatic.png`。没有从这段指令推导全局大小写折叠，也没有证明 active resolver 使用 extracted 文件。`9588A0→8F5AD0` 还依赖已注册资源／Mod 来源及缓存，文件存在不是运行命中。

主审 fresh PNG table 的24个槽与属性函数，纠正潜在的属性混淆：`A123B0` 读 image+8 bit0；`A123D0` 读 bit1；**indexed 是 table+8 的 `A12420` 检查 image+28 format==0**。decoder 的 palette 和四通道分支分别写 format0、format8。IHDR 只给预期，不能替代候选最终 image/control／native format。另读的 `A69B10` 是缓存的最大纹理尺寸查询；`A12EA0` 更新 image bookkeeping，不能把这两个函数改称直接 GL bind。详见 [primary-png-profile.json](../../artifacts/cs-t0/tx-03-20-static/primary-png-profile.json)。

为研究 owner 服务点读取的 `A18750` 实际管理 renderbuffer/FBO storage，**未采纳为通用 GL 状态恢复或 Sprite 服务 helper**。服务回调仍是 tx-03-19 窗口上的后续设计：有可信 context、明确空队列和 owner 顺序时准备／应用；最终 pre 窗口只 Render 已提交实例；正常最终 helper 返回后才按关系退休。普通 RenderHook post、未知队列或 context 丢失不能代替这些条件。

## 资料采集异常保留

1. 初次 Windows `rg` 对显式 `world*` 路径返回 os error123；随后改用实际目录及 `-g` 检索。原始失败如实记录在[采集说明](../../artifacts/cs-t0/tx-03-20-static/primary-acquisition-notes.json)，不是游戏失败或验收结果。
2. 初版 [primary-capture-source.json](../../artifacts/cs-t0/tx-03-20-static/primary-capture-source.json) 的单个 nested array 范围被 PowerShell 展开，loop 两段 end_line=null、lines为空。原文件保留，不用作完整 loop 源码证据；新建 [primary-capture-source-v2.json](../../artifacts/cs-t0/tx-03-20-static/primary-capture-source-v2.json)，改为显式范围对象，提供1020..1038行。其它源码段沿用原采集。该修正是在阅读资料时发现，不是验收失败重跑。
3. 反编译类型不直接当作 ABI、native key 时间字段或运行事实；XML count、PNG header、当前 IDB/input 路径亦不证明动态对象与磁盘身份。没有执行哈希比较或恢复旧 gate 流程。

## 修订范围与唯一验收

本轮新增主报告、Luna 报告及本文，维护 docs、hooks、Entity、game-loop 和 state-synchronization 五个入口；静态原始导出、读取脚本、源码摘录、资源观察和支持声明置于 `artifacts/cs-t0/tx-03-20-static`。没有改原版 Online／Isaac Sync、ISCS5／ABI5、七项 Lua API、C/S native 实现或历史验收记录。

研究、子结论审阅及全部编辑结束后，仅运行一次 [review_documents.ps1](../../artifacts/cs-t0/tx-03-20-static/review_documents.ps1)，结果以 [final-static-review.json](../../artifacts/cs-t0/tx-03-20-static/final-static-review.json) 为准。范围为文档引用、显式 VA/RVA、原始指令与PNG槽、双方导出、v2源码摘录及样本声明的一致性；不执行 acquisition 脚本、游戏或生产代码。本文不预写通过次数或运行成功结论；若失败保留结果、不自动重跑。

本样本的基础研究已足以进入**最小独立 Sprite adapter／本地 fixture 实现**。实际 resolver、native shape、线程／context、连续姿态、正常候选切换及最终像素留给实现完成后的一次整体 E2E。完整 Entity 外观、room occlusion、异常恢复和热重载不由本次静态结论保证，不为这些未决项设新的推进前置条件。
