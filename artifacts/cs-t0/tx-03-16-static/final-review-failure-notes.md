# tx-03-16：唯一静态审阅的失败记录

日期：2026-10-23（Asia/Shanghai）。最终审阅入口实际执行一次，exit code=1，结果为 `FAIL_STATIC_DOCUMENT_REVIEW`，127 项中 125 项通过、2 项失败。原始 [final-static-review.json](final-static-review.json)与 [review_documents.ps1](review_documents.ps1)保留，没有修改、重跑或创建替代通过结果。

失败项为两份报告的 `document-addresses`：主报告被识别为 17 对、Luna 报告为 7 对。主审在失败后仅阅读结果和相应文档行，发现新增的裸 slash 匹配规则 `` `0x... / 0x...` `` 没有要求显式 VA／RVA 标签：

- 主报告的 Sprite `0x30 / 0x50`、state 的 `0x08 / 0x0C` 等六组对象字段偏移被当作 VA/RVA。这些行原本明确以 Sprite／state／descriptor 为基址。
- Luna 报告的两个 wrapper VA、Flip setter 的两个 VA、两个 getter VA，以及 `0x20/0x80` 两个 flags 值也被当作 VA/RVA。

因此这两项失败是审阅脚本错误分类并列数值，不是据此发现新的二进制地址换算错误。实际 raw JSON 的函数／指令 VA/RVA、关键指令、共同函数 bytes、引用和入口检查在这次结果中均通过；没有用此说明把总结果改为 PASS，也没有再执行地址检查程序。

未完成项：本轮整体静态文档验收仍为 FAIL，审阅器的 slash 分类规则尚未修正。以后有实质研究推进时应同时将该规则改为读取明确标记的 VA/RVA，而非仅凭斜杠推断；不单为重复这次验收而另开 tx。研究正文和主审的语义结论保持各自静态限定，不因验收脚本失败升级或降格为运行时观察。

这是按项目约定追加的失败说明，不是第二次验收。没有运行游戏、构建、目标 helper、单元测试、E2E 或哈希检查。
