# 生命周期、设备重绑与退出

更新时间：2026-10-16；整理：tx-01-44。执行约定以[现行项目规范](../../../project-conventions.md)为准。

## 仍适用的技术结论

- A/B 固定 ordinal 0/1；Player/device/K 只绑定当前 generation。设备解除分配、room、death/respawn 等事件使临时关联失效，唯一重绑前不能 apply。
- device-rebind v2 以实际 wrapper self、主 Player vector 的唯一位置和 assignment 读回建立因果关系，不能从“B 按了暂停”猜测操作对象。
- 历史菜单重绑语法为 slot53 解除分配 → slot54 同一 Player 的提示确认 → slot55 重分配；断连语法为 slot54 → slot55。这些是当时场景与槽表中的观察契约，不是所有版本的通用协议。
- shutdown 私有清理不等同新游戏生命周期；terminal event、shared owner 与 manifest generation 的漂移曾导致 live-10 失败。live-11、menu live-12 的后续成功不抹去该失败。
- menu/run teardown 退休 session 和 generation；不复活旧输入，恢复遵守当前 Hook 协调 owner 和 NEVER/DLL 常驻约定。

05-03 的历史结果为五份适用 PASS source 与一项限域 N/A。ordinary Variant=0 下 native-exit 的限域不适用，不等于所有退出路径已通过，更不证明当前候选中的 B-only retirement 已生产实现。

## 原始资料

- [device-rebind v2 计划、因果语法与 shutdown 修复](../archive/01-passive-lifecycle/device-rebind-v2-evidence-plan.md)。
- [原生退出限域适用性记录](../archive/01-passive-lifecycle/native-exit-applicability-gate-v1.md)。
- [历史 source、live-10/11/12 与失败长日志](../archive/00-overview/isaac-cs-t0-05-status.md)。

旧授权安排、多组自动门与独立角色条款只作历史；实际操作按现行规范和用户实际授权执行。
