# 持久化副作用与完整停止窗口

更新时间：2026-10-16；整理：tx-01-44。执行约定以[现行项目规范](../../../project-conventions.md)为准。

## 仍适用的结论

正常菜单路径可以原生建立 readonly，并排除已定位的部分写入口；readonly 不支配全部 direct save。最终 dirty=0 或文件未变化，均不能说明整个操作窗口没有保存、统计、Cloud/Steam 或其他持久副作用。

两次干净采样之间可能出现 dirty=1 → flush 清零 → 后端写出。历史资料将此作为“观测不足”的反例，没有证明固定候选实际发生污染，也没有证明候选安全。

窗口覆盖创建前、Init/同步传播、后续自然更新和 disable/drain/退休/Hook 恢复/terminal；不能以两个 UI edge、端点采样或经验秒数替代完整生命周期理解。历史三 manager pass 的最小安排属于当时观测语法，不恢复为逐场景测试流程。

完整 Init/tail、Room/Grid/player 调用、Lua、native state restore 与 readonly/guard 的保持仍存在未决传播。不得直接改 readonly、压制保存、清统计或删除对象来制造安全结果。具体操作保持在用户实际授权范围内。

## 原始资料

- [完整自然更新与停止窗口 v2](../archive/native-join-persistence-window-v2.md)。
- [readonly / dirty / save / Cloud / Steam 初始清单 v1](../archive/native-join-persistence-window-v1.md)。
- [限域静态审查实施记录](../archive/native-join-static-closure-implementation-v1.md)和[历史结果](../archive/native-join-static-closure-test-results-v1.md)。

本主题保留功能风险，不把旧审查链或哈希字段变成当前推进前提；没有新增“零持久污染已通过”的结论。
