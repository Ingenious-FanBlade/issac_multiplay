# tx-01-77 运行结果：外部观察器在注入前失败

日期：2026-10-19。用户确认采集后拾取跟班道具、本轮纳入自有消费暂停和控制器失联，并允许新增只读取证元数据。控制器直接 `cargo build --release --locked --offline --bin IsaacCsT0` 纯编译成功（2m42s），没有构建 Native/Helper 或执行单元测试。

唯一一次统一入口在 `start primary` 后失败，`result.json` 为 **FAIL**，error=`No unique main module`。失败发生于外部只读观察器的注入前模块识别，尚未创建 controller 子进程，未安装 Hook，未执行拾取、线程暂停或控制器终止。游戏 PID=41328，已保存 process-before.json 的 .NET 模块列表确实含 isaac-ng.exe；独立观察器没有找到唯一主模块。这是观察器识别结果不一致，不能据此判断采集实现故障。

旧观察器未保存它实际枚举的模块列表，因此本次材料不足以确定 Toolhelp 结果是空缺、重复、跨位数枚举差异还是结构解释问题。禁止把推测写作已证实原因。随后只读检查了已有文件与源代码，没有在游戏上再次查询或试跑。

产物：[原始 FAIL](../../../../artifacts/cs-t0/tx-01-77-live-remaining-01/result.json)、[完成收尾](../../../../artifacts/cs-t0/tx-01-77-live-remaining-01/finalization.json)、[注入前进程](../../../../artifacts/cs-t0/tx-01-77-live-remaining-01/captures/primary/process-before.json)。源代码、二进制与目录清单保留，未覆盖原报告，未自动重试。子项里的 19 项 PASS 仍全部来自 tx-01-76 及更早材料，没有新的实机 PASS；其中 W01.optional_70 的 method 在开始场景前被提前改为 AUTOMATED，属于报告标记问题，本说明予以更正，原 JSON 原样保留。

修正将使用项目现有控制器已采用的 PSAPI 32-bit 模块枚举路线，绑定已核对身份的进程 handle、按完整可执行路径识别主映像并保存模块枚举诊断，同时修正提前更改 PASS 来源标记的问题。见[tx-01-78 修正范围](tx-01-78-observer-fix.md)。新整体验收须用户明确同意后进行，当前尚未重新执行。
