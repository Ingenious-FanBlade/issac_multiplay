# tx-01-78：修正外部观察器的主模块识别与失败取证

日期：2026-10-19。tx-01-77 唯一验收在注入前报 No unique main module，具体 Toolhelp 枚举内容缺失，不能声称确定底层原因。

本轮实质修正：外部观察器改用 `src/win32.rs` 已使用的 K32EnumProcessModulesEx/LIST_MODULES_32BIT 与 K32GetModuleInformation 路线；复用持有且已核对 PID/创建时间/完整路径的只读进程 handle，增加查询模块所需的 PROCESS_QUERY_INFORMATION，不增加游戏写权限。模块完整路径经 K32GetModuleFileNameExW 保存，主模块按本次已核对的可执行完整路径识别。固定最多 4096 模块，空列表/越界/截断/读取失败仍停止；保存 module-inventory-before.json 与恢复后的列表，不掩盖未来识别问题。

同时，W01.optional_70 的历史 PASS 来源只在真实完成新采集与检查后才改为 AUTOMATED，修正 tx-01-77 注入前失败报告的提前标记问题。tx-01-77 FAIL 不覆盖，未来新产物会保存该失败报告及修正说明。

生产控制器继续使用 tx-01-77 纯编译的元数据版本，Native/Helper 仍为 tx-01-75 冻结副本。本轮不需重编译任何二进制。场景、故障操作及[原先登记的失败方式](tx-01-77-live-remaining.md)不变；不运行全量 replay 或已通过离线用例。修正期间没有游戏查询、试跑、注入或测试。

运行更新：用户同意后执行了一次整体验收，主模块识别成功，随后因观察器的 IAT 归属条件错误在注入前 FAIL；未自动重试，见[实际结果](tx-01-78-e2e-results.md)。
