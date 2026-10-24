> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 静态条件审查独立测试结果 v1

2026-08-31；独立测试实例执行，create-new。结果为 **PASS_STATIC_CONDITION_REVIEW_VERIFICATION**，工作包仍为 **BLOCKED_BY_EVIDENCE / STATIC_CONDITION_REVIEW**。未证明完整无污染条件，未解锁 active、live、05-05、T0-05 或 T0-06。

## 实际入口与环境

在 Windows、PowerShell 7.6.5、Python 3.13.7、GNU objdump 2.44 下执行。独立解码器 SHA-256 为 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`。仅读取磁盘 x86 PE；目标 SHA-256 为 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`，image base `0x400000`、image size `0x8FE000`。不记录目标绝对路径、账号或设备身份。

```powershell
pwsh -NoProfile -File ./cs-t0/verify-coop-native-join-static-closure.ps1 -GameExe '<disk-target>' -RepositoryRoot '<repository>' -Objdump '<independent-decoder>' -Python '<python-3.13.7>'
```

正式 attempt-v1 实际退出 0，未发生正式门失败。公开 inspector 的合法输入实际退出 1、输出 BLOCKED；它的退出码不作为测试门成功码。旧生产 preflight 也实际退出 1，仍保持 STATIC_PREREQUISITE 和原有七项缺口。

| 产物 | SHA-256 |
| --- | --- |
| [正式验证](../../../../../cs-t0/coop-native-join-static-closure-verification-v1.json) | `319C43D13A28A783976410C896DABE33E4D6E4BD4C17D47594EAFAFFDBF7B8A1` |
| [正式执行日志](../../../../../out/cs-t0/native-join-static-closure-verifier-attempt-v1.log) | `5B69A80A87F97ABE0F4B6F77789B6CE07953C165E673D54D11A3EA9CFF9D6224` |
| [封印汇总契约](../../../../../cs-t0/coop-native-join-static-closure-contract-v1.json) | `1E2C1D201AF26F8E97E08C871FF6284055DCD26400C1A562BA7402ED9938C7CD` |

验证绑定了 32 项实际输入，包括 inspector、测试脚本、两份证据及旧生产文件；执行前后全部哈希相符。本文在验证之后创建，供后续发布绑定，不回填进既有验证。

## 独立静态验收

| 证据 | 磁盘锚点 | 指令 | 直接边 | 常量内存引用 | float32 常量 | 局部条件条目 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Init closure v2 | 83 | 1154 | 251 | 5 | 4 | 11 |
| Persistence window v1 | 63 | 840 | 205 | 0 | 0 | 9 |
| 合计 | 146 | 1994 | 456 | 5 | 4 | 20 |

使用独立 PE 解析和 GNU objdump，从各声明函数入口连续解码至窗口结束，核对窗口与完整指令边界、大小、助记符、原始字节及完整直接边清单。REL32 和 REL8 分别按实际 opcode、有符号位移重新计算，间接调用不冒充 REL32。数据引用按原始绝对 displacement 与 image base 重算 RVA；四个 IEEE754 常量分别核对实际字节和值。函数入口前缀的线性解码不是所有分支可达性证明，也不表示 padding 被执行。

14 项独立工具对抗测试全部通过：递归重复字段及非有限数拒绝、路径/设备别名与哈希拒绝、PE 截断/架构/重叠映射拒绝、真实回调边界及 REL32/指针复算、半条指令拒绝、create-new、错误大小/助记符/字节拒绝、缺失/伪造边拒绝、短跳必须 REL8、数据与代码上下文隔离，以及 float 阈值不可用舍入抹平差异。

9 组公开黑盒、70 次断言全部通过：

| 分组 | 断言数 |
| --- | ---: |
| 原仓库与完整有效 fixture 建立同一 BLOCKED 基线 | 2 |
| 相同目标字节的副本 | 1 |
| 错误目标、目录目标、缺失目标 | 3 |
| 未知、重复、缺失命令参数 | 4 |
| create-new、旧 source/子目录、冻结目录、ADS、DOS 设备、尾点/空格、输出 junction 与仓库父级 junction | 18 |
| 契约身份、未知字段、权限、重复字段、非有限数、缺口与路径篡改 | 12 |
| Init handoff 篡改及恢复后的有效基线 | 14 |
| Persistence handoff 篡改及恢复后的有效基线 | 14 |
| 前驱字节篡改及恢复后的有效基线 | 2 |

拒绝用例必须非零退出、无 JSON 报告、无创建或覆盖输出。黑盒修改仅发生于临时完整 fixture，且先验证有效基线；未更改原证据或为篡改重新签发 hash pin。固定证据的篡改拒绝可能先由哈希门触发，不把它冒称为每个内部 schema 分支的单独覆盖；解析、边界与解码的对抗测试另行执行。

## 条件含义与未闭合边界

独立核对支持有限的静态条件事实：普通合法、非特殊显式 seed 可以原生建立 readonly；特定模式字段持续为零时可排除已定位的两个临时清 readonly 入口；Init 的局部 guard 和常量分支与原始指令一致。这些条件成立与否没有 runtime 读回。

共同 readonly 不能推出完整无污染：不经过 setter 的 direct persistent 写需要独立条件排除；GameStateSave 在检查 save-enabled 前已经更新状态/dirty；flush 可以先清 dirty 再进行文件、Cloud 或 Steam 路径，因此事后 dirty 为零、文件哈希相同或未来 readback 均不足以证明没有先前污染。自然 manager pass 后仍可能进入 GameUpdate；Init 与随后 tick 对 timers、request、fade、pending lifecycle 和 readonly 的状态传播仍未闭合。

因此保留 Init 三项 UNKNOWN、window 的 UNKNOWN 条件及六项 false 完成声明。汇总精确保留 12 项缺口：四项总门义务、四项 Init 缺口、四项 window 缺口；structured window 缺口的 UNKNOWN/class 未丢失。局部条件支持不被提升为 whole-init、窗口稳定或 active 资格。

## 旧证据保护与失败保留

正式门前后均实际核对：旧 publication 的 18 项绑定、原验证的 20 项绑定、继承的 16 项引用、164 个冻结 snapshot 文件、149 个原始 baseline 文件及三个现有生产二进制，全部原哈希不变。未重新构建生产，未重发旧生产验证，未重复旧全套自动门。新 inspector 明确不宣称独立解码、历史深重放或生产重新验收；这些字段保持 false，与本独立门自身完成的磁盘和哈希检查区分。

开发期失败按 create-new 保留：

- Init v1 attempt-v1 至 v4 均未改写。完整 [attempt-v4](../../../../../out/cs-t0/native-join-init-anchor-audit-attempt-v4.json)，SHA `313AF378783B3C5FA556FC3980D099F8092B6E7C53769F82B8087E208D270603`，拒绝 5 个锚点中的 22 条空助记符。更正 v2 仅修订 1 条 `90/nop`、21 条 `CC/int3`；独立比较确认原字节、边界、边、条件、权限均未改变。原 v1 拒绝结论不改判。
- Init v2 的初核 [attempt-v1](../../../../../out/cs-t0/native-join-init-v2-anchor-audit-attempt-v1.json)，SHA `FDCF982031C7D829DE824016C87CB6824ACA8679E3B83F801753F728E635BC66`，通过字节及边界核对，随后纳入正式门重验。
- Window 初核 [attempt-v1](../../../../../out/cs-t0/native-join-persistence-window-anchor-audit-attempt-v1.json)，SHA `0AEB3C61737FA1BA54F2DB4E96B1D47F3D299EF5022541684AF2D42C279EAF89`，是测试适配器误要求八位 RVA 的失败，发生在解码之前；已定义 schema 允许一至八位，不归因于 reverse 原始字节错误。修正适配器后 [attempt-v2](../../../../../out/cs-t0/native-join-persistence-window-anchor-audit-attempt-v2.json)，SHA `DEDDB1C89F48BEF43A1464B1EFC8968E3D68174C032C06C90DCF721F2E3CB439`，通过并纳入正式门重验。

本轮没有启动、枚举、打开或附着游戏进程，没有游戏输入、mapping、注入、callback override、arm、claim 或授权消费。55 槽 join runtime 未实现、未执行、未验收；没有进行污染快照、original-once、restore/Unsafe、Shutdown 或实机验证。历史 Native/真实 mapping/Shutdown 结果属于上一轮的旧实现回归，本轮不将其升级为 55 槽 runtime 结论。
