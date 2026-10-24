# Init、Fresh-B 与 P0：有效结论和未决项

更新时间：2026-10-16；整理：tx-01-44。执行约定以[现行项目规范](../../../project-conventions.md)为准。

## 技术摘要

Init closure v1–v8 逐步补充 ordinary Player 初始化、fresh-B reaching-definition、publication identity、manager guard/receiver；Cache/Familiar/P0 是其中一条支线。

已有局部结论是 managerReceiverFormulaStatus=STATIC_CONFIRMED，managerThisIdentityStatus 仍为 UNKNOWN。publication identity、fresh-B source preservation、desired/matching familiar、P0 topology、完整 manager return、自然更新窗口和 reader/ABA 没有被这些局部结论整体证明，不能把 synchronousPropagationComplete 写成 true。

原生 Join 创建/发布在当前规划中采用[用户假设](../02-active-join/native-join-active-user-planning-assumption-v1.md)；其余初始化与保存风险保持各自原始标签。v18 限定正常路径排除完整 Init/global manager-this 研究，不能把排除项误写成已经解决。

## 证据位置

- [Init closure v8](../archive/04-init-closure/archive/native-join-init-closure-v8.md)：累计 receiver 章节和未决前提。
- [Cache/Familiar/P0 v5 原文](../archive/native-join-init-v4-cache-p0-v5.md)。
- [Init v6 publication 到 dispatch 原文](../archive/native-join-init-closure-v6.md)。
- [完整版本索引](../archive/04-init-closure/archive/README.md)。

先前已存在的 04-init-closure/archive 层级随整批资料放入统一归档，保留原层级便于追溯；日常从本摘要进入，无需按版本逐篇恢复旧流程。
