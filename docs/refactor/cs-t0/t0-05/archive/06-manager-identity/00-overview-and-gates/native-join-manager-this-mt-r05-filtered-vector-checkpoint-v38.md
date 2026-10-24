> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this MT-R05 filtered-vector checkpoint v38

日期：2026-09-06  
状态：`SINGLE_ROW_V3_ADJUDICATED / 0 POSITIVE / 1 UNKNOWN / BLOCKED_EXACT_99`

## 1. 当前结论

在 v37 的 exact5 裁决后，单独继续 `MT-R05 {0x0033570B}` 的有界静态逆向。
冻结 PE 与 IDA 数据库均绑定 SHA-256
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。

新证据确认：

- `0x00735220-0x00735367` 在正常继续路径构造 auxiliary vector，精确追加整数
  `1..14`；
- `0x00735368-0x0073537C` 随后把 filtered vector 初始化为空；
- 源候选数为零时，`0x00735399-0x0073539B` 直接进入空 filtered merge；即使源
  非空，`0x007353D1-0x0073546D` 的完整 reject 谓词也允许零次 append；
- `0x007355F1-0x0073560C` 只检查 auxiliary 非空；`0x0073567B-0x0073570B`
  没有 filtered nonempty guard，零计数仍形成 raw length `0xFFFFFFFC`；
- 七个直接 caller 的有界前驱窗口没有提供“至少一个候选必通过过滤”的支配条件。

因此独立 TEST 只接受失败安全的 `UNKNOWN`：first-open 仍为
`countAndNoWrap / R05_AUX_NONEMPTY_FILTERED_EMPTY_CONJUNCTION_OPEN`，精确
countermodel 继续是
`auxiliaryRangeCount>0 && filteredCount==0 yields raw transfer length 0xFFFFFFFC`。
`frontierCompositionEligible=false`，不得 promotion，也不创建空 successor。

## 2. 证据与作者分离验收

REVERSE：

- [v3 有界逆向交接](../02-copy-move/native-join-manager-this-mt-r05-filtered-vector-v3-handoff.md)；
- candidate SHA-256：
  `DEB2E1301025DA0AB46DFBA2E02017F0BA6A8F27C7EAEE2C7F4FD2C15B9291A6`；
- builder SHA-256：
  `318C5FFFFF55FAE2C94E51C64C751F25EDD849332B922EB41A29264B56185281`。

独立 TEST 在 candidate 出现前封印规则，并在 candidate 发布后用同一物理 contract
验收；没有读取或 import REVERSE builder：

- [候选前封印交接](../02-copy-move/native-join-manager-this-mt-r05-filtered-vector-v3-sealed-preparation-v1-handoff.md)；
- [候选后独立审计](../02-copy-move/native-join-manager-this-mt-r05-filtered-vector-v3-sealed-independent-audit-v1-handoff.md)；
- sealed spec SHA-256：
  `8872E11ED61E153C2B27A394764FC4959C4E1C6035ABE28D40AED69891BF7759`；
- independent audit SHA-256：
  `37195EFEE870E371B8915264F3E1AB45906B54508BE08E985B92501B19E73765`。

审计 verdict 为
`PASS_SEALED_MT_R05_V3_SINGLE_ROW_CANDIDATE / 0 positive / 1 UNKNOWN`。
这里的 `PASS` 只表示候选满足预封印的失败安全分类规则，不表示该行已闭合。

## 3. 压缩测试

REVERSE 作者自检仅执行一次 create-new、一次 verify-existing 和三项核心 mutation。
独立 TEST 仅执行：

- preparation 一次 create-new、一次 verify-existing、四类决策 mutation 和一次
  wrong-spec-hash fail-before-write；
- candidate audit 一次 create-new、一次 verify-existing、同四类决策 mutation和一次
  wrong-candidate-hash fail-before-write。

没有运行 Cargo、全量 T0 build、历史宽回归、重复 self-test、游戏、进程访问、
x32dbg、注入或 IDB 修改。

## 4. 当前停止点

若将来继续该行，首个有意义的静态任务是闭合
`n298+0x2A434/+0x2A438` 的 producer/lifetime fixed point，并证明对所有到达目标的
执行、每个仍有 auxiliary 项的轮次至少有一个源候选必通过全部 reject 谓词。普通
运行时采样最多增加存在性观察，不能替代这个全称正向命题。

v33 frontier 继续为 copy/move `323/405`、memset `225/242`、总计 `548/647`、
remaining `99`。manager-this、whole-image raw v2、formal v7、active/live、runtime
mutation、05-05、T0-05 总门和 T0-06 全部保持关闭。
