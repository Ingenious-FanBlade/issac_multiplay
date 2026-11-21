# Reader、局部公式与历史准入规格

更新时间：2026-10-16；整理：tx-01-44。执行约定以[现行项目规范](../../../project-conventions.md)为准。

## 可复用结论

admission v1–v6 是累计研究与离线规格，不是六套并列当前契约，也不是当前操作资格。v5 局部确认 manager guard 公式；v6 局部确认 GameRoot slot 到 ordinary spawn receiver 的定义。两项局部 STATIC_CONFIRMED 不推出 caller 入口与重读对象属于同一存活 generation。

读取失败、双读漂移、绑定冲突和无法排除的身份替换应停止相关 apply；不得把未来 B 读回当作创建前前提，也不得把有限合成模型通过写成原生 ABA 或完整传播证明。

publication identity、fresh-B source preservation、完整同步传播、manager context、原生 reader/ABA 和生产 runtime 仍有未决项。旧 inspector/verifier 的 BLOCKED 或非零退出按原记录保留；其哈希校验和准入流程不再执行。

## 追溯入口

- [v6 实施与局部 receiver 公式](../archive/03-admission/native-join-admission-implementation-v6.md)。
- [v6 历史测试结果](../archive/native-join-admission-v6-test-results-v1.md)。
- [v1–v6 完整版本索引](../archive/03-admission/README.md)。
- [Init / Fresh-B](../04-init-closure/README.md)、[manager identity](../06-manager-identity/README.md)与[持久化窗口](../05-persistence/README.md)。

本次仅归档说明文档，未改 cs-t0/admission_v*、JSON contract 或任何旧工具。
