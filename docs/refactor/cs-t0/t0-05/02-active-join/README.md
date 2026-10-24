# 原生 Join 路径与创建/发布假设

更新时间：2026-10-16；整理：tx-01-44。执行约定以[现行项目规范](../../../project-conventions.md)为准。

## 当前用途与边界

原生 local-co-op Join 是自有虚拟设备路线 A 的功能依据。用户已接受[可靠创建并发布 B 的规划假设](native-join-active-user-planning-assumption-v1.md)：fresh B、Main Player vector ordinal 1、assignment、HUD/index 和 Lua GetPlayer(1) 在同 generation 一致。该记录仍是 USER-ASSUMPTION / DESIGN_ONLY，末尾旧流程已由现行规范覆盖；它不证明完整 Init/持久化安全或实际运行已成功。

[v18 报告](../07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v18.md)补充了正常路径的实际 query 轨迹：已有同 K 的有效 selector 且尚无 B Main Player 时，两处受条件控制的 action19 查询可均为 NOT_CALLED，后续仍可继续 20/21、14/stored/15 和创建。配置 19=false 不等于原生实际查询返回 false。

JoinControl scope 独立于尚未存在的 B Player TLS；设备创建 owner 与 B actual-create attempt owner 是不同职责。相关候选尚未生产实现，不能把旧 preflight 或规划后置当成实际接线结果。

## 原始资料

- [原生主动路径静态交接](../archive/native-join-active-reverse-v1.md)与[有界补核](../archive/native-join-active-followup-v1.md)：原始 UI、selector、ordinary Join 路线与剩余边界。
- [历史 preflight 实施记录](../archive/02-active-join/native-join-active-implementation-v1.md)和[历史测试结果](../archive/02-active-join/native-join-active-test-results-v1.md)。
- [Virtual Device 当前入口](../07-virtual-device/README.md)：最新 JoinControl、attempt、S2 与退休说明。

保存、副作用、同代身份和恢复的未决事实见相应主题；不要求先完成旧 gate/解锁链才可分析或实现新的完整目标。
