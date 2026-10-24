# tx-01-45 唯一一次整体验收记录

日期：2026-10-16。整体结果 **FAIL**；实机阶段 **NOT_RUN**。

用户已确认“场景已准备好；允许全部操作”，涵盖待确认问题中列出的隔离 Offline 初始仅 A、注入/Hook、A 物理与 B 脚本、暂停/换房/死亡复活、B 冲突退休、menu teardown、DLL 常驻，以及本次隔离存档和设备绑定配置等持久写入范围。实际未进入这些游戏操作。

## 唯一入口与实际顺序

通过 `pwsh -NoProfile -File cs-t0/accept-virtual-device-e2e.ps1` 调用一次；目标为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`，新产物目录为 `artifacts/cs-t0/tx-01-45-live-01`，脚本为 `cs-t0/testdata/tx-01-45-coop-script.json`。完整参数与授权补记在产物 `invocation.json`；它记录本次已发生的调用，没有再次执行入口。

| 阶段 | 结果 | 直接证据与边界 |
| --- | --- | --- |
| Rust release 构建 | PASS | `Finished release profile ... in 2m 03s`；这仅说明构建成功 |
| Native Release/Win32 源码编译 | PASS | 新增及受影响 cpp 编译完成并进入链接代码生成 |
| Native 链接 | FAIL | `LINK : fatal error LNK1101: MSPDB140.DLL 的版本不正确；请重新检查此产品的安装` |
| Helper 构建 | NOT_RUN | 入口在 Native 构建返回 1 后停止 |
| 控制器启动/注入/设备创建与注册/原生 Join | NOT_RUN | 未到控制器调用；没有本次 native capture |
| A/B 输入隔离、held/edge、重复/非法/过期/缺输入 | NOT_RUN | 没有向游戏发送输入，没有 Authority/query/world 事件 |
| pause/loading、换房、死亡/复活、失效与重绑 | NOT_RUN | 未进入实机场景 |
| B-only retirement、A 继续、menu/run teardown | NOT_RUN | 未进入实机场景 |
| pair 缺席、47 槽字节恢复、回调排空、DLL 常驻 | NOT_RUN | 本次没有安装 Hook 或注入 DLL，不能据此声称恢复用例通过 |

这是链接器报告的工具链组件版本问题；尚未定位实际加载的 MSPDB140.DLL 来源，不能声称已修复，也不能从编译成功推导运行时正确性。已保留错误和现有源码；没有修改工具链、补丁重建、重跑、强制卸载或重新注入。

## 产物

- [build.log](../../../../../artifacts/cs-t0/tx-01-45-live-01/build.log)：唯一调用的原始构建输出。
- [result.json](../../../../../artifacts/cs-t0/tx-01-45-live-01/result.json)：入口生成的 FAIL / runtime NOT_RUN / automaticRetry=false 原始结果。
- [invocation.json](../../../../../artifacts/cs-t0/tx-01-45-live-01/invocation.json)、[operations.jsonl](../../../../../artifacts/cs-t0/tx-01-45-live-01/operations.jsonl)：失败后补记的真实参数、用户授权与执行顺序。
- [scenario-input.json](../../../../../artifacts/cs-t0/tx-01-45-live-01/scenario-input.json)：本次指定脚本的副本，不是已发送输入的声明。

`capture/events.jsonl`、`loop-events.jsonl`、`helper.log` 与 `restoration.json` 未生成，因为游戏阶段未启动。没有计算或校验哈希，没有将未观察场景填为 PASS。T0-05-05 的生产闭环仍未验收完成。
