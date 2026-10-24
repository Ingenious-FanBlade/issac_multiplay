> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 独立验证结果 v1

2026-08-31。结果为 **`PASS_BLOCKED_GATE_VERIFICATION`**，工作包仍为 **`BLOCKED_BY_EVIDENCE / STATIC_PREREQUISITE`**。通过的是静态前置证据、拒绝行为和继承回归；没有实现或验收主动 join runtime，没有实机 source，也没有开放 05-05。

## 1. 完整门与证据

独立测试在 Windows、Win32/x86 Native Debug/Release、批准的 unpacked target 上执行：

```powershell
pwsh -NoProfile -File .\cs-t0\verify-coop-native-join.ps1 -GameExe <approved-disk-target> -RepositoryRoot <repository-root>
```

目标仅以磁盘文件读取，SHA-256 为 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。未启动、打开、附着或注入目标游戏进程。完整 v3 命令退出 **0**；其中当前 Release CLI 的合法 preflight 按约定退出 **1** 并输出 BLOCKED，而非被当作主动成功。

| 必需检查 | 实际结果 |
| --- | --- |
| 新公开 CLI 黑盒 | 8/8 PASS；6 个依赖真实磁盘 target 的组均有实际执行标记，未用 env 缺失/空执行计 PASS |
| 新拒绝边界 | catalog/static/plan/predecessor/当前引用/历史 snapshot 篡改、重复字段/开关、旧 arm/smoke 入口、不同 target、source 写入、create-new、ADS、DOS 设备别名、Unicode 上标及末尾点/空格均拒绝 |
| 独立磁盘 PE 验证 | 53/53 原始锚点、2 个对齐 PUSH callback operand，包含持久化反证关键字节 |
| 冻结历史 | 164/164 snapshot 文件与原 149 项基线对应关系通过；5 份 PASS 与原 exit BLOCKED 完整重放 |
| Rust | 285 单元，C0 9、device contract 7、passive contract 11、applicability contract 18，以及原有跨语言套件通过；fmt/check/clippy 与 Release 构建通过 |
| 旧 Native | Debug/Release 各 39 legacy、262 Active、407 passive case，以及 device-v2 套件通过 |
| 旧真实 IPC/恢复 | passive 53 槽、device 56 槽的实际 mapping、逐槽故障矩阵、原始调用 ABI、drain/Unsafe 和生产 Shutdown→公开 CLI 桥接通过 |
| 继承门 | 原 bootstrap、static、C0、T0-04、MCP（15 单元与其集成测试）、旧 live source 和 live-10 BLOCKED 字节重放通过；保持旧脚本的完整重复回归，不跳过深重放 |
| 当前 Release | 53 anchors、55 个静态候选槽、164 历史文件、5 PASS+exit BLOCKED；七项证据缺口保留，所有主动能力为 false |
| 发布后只读复核 | verification 的 16 条 input、3 条 production、1 条 diagnostic 绑定，20/20 SHA-256 匹配 |

正式报告：[verification v1](../../../../../../cs-t0/coop-native-join-verification-v1.json)，SHA-256 `5F2CE26D88D27B3869DE9B01B92DB79737F2A17369A990781BE96FAECDE61CE4`。完整日志：[automatic gate v3](../../../../../../out/cs-t0/native-join-blocked-automatic-gate-20260831-v3.log)，SHA-256 `2FB9CF59D51FF0D5EAEC22C769998BB979E23A6CC31B241974D5552730B0AE19`。

当前 Rust EXE 为 `474E8FBD5D6690F6EBB559B7FF424F1A444211FF41E285292B1EA2412BF9943D`；Native/Helper 分别仍为冻结的 `768CDDEBE490236789F6C542AAA7C9F84B31778EE1A3655DC4F23934AD4F720A` 和 `3170D805B6F1F2D0BDE4E4F93D38D982A71AC9F65BC9E973DF00B42F63D03972`。旧 production 文件已在 predecessor snapshot 保留，旧 verification 不作为新 EXE 的验证凭据。

## 2. 失败、中止与未执行项

- 开发期 fixture 漏拷 static 的当前树前置绑定，首次 8 组为 5 通过/3 失败，随后单例诊断确认 `missing join binding`。首轮 negative 的通过不采信；补齐冻结输入，并让 mutation 组先验证合法 BLOCKED baseline 后，8/8 从头复跑通过。[原失败记录](../../../../../../out/cs-t0/native-join-test-development-failures-v1.json) 保留。
- [完整 v1 日志](../../../../../../out/cs-t0/native-join-blocked-automatic-gate-20260831-v1.log) 因 sandbox 拒绝读取 Windows SDK 目录导致 MSBuild MSB4184，中止且未发布报告。通过工具的 sandbox 审批后，[SDK/build 检查](../../../../../../out/cs-t0/native-join-sdk-build-20260831-v1.log) 退出 0。
- [完整 v2 日志](../../../../../../out/cs-t0/native-join-blocked-automatic-gate-20260831-v2.log) 在主线发现诊断输出的 Windows ADS/设备别名边界后主动中止，未发布报告。生产收紧 basename，独立补充用例通过后，完整 v3 从头运行；没有复用 v1/v2 的部分结果充当完整 PASS。
- **未实现、未执行**：新 55 槽 join runtime/ABI/Shutdown、join 专用真实 IPC/arm/claim、实际两次 UI edge、无持久污染实机证明以及 active source/analyzer/replay。旧 53/56 槽测试不替代这些项目。

报告中的 `mappingCreated=false`、`claimConsumed=false` 等字段限定于 **native-join/目标游戏路径**。继承测试确实在独立测试进程创建 Win32 mapping，并消费 synthetic fixture token；这些没有产生游戏会话、live 授权或真实 active source。

## 3. 补核纪要的独立磁盘验证

完整门结束后，另外验证 [followup v1](native-join-active-followup-v1.md) 的固定 SHA、全部 43 条表格字节窗口、3 条正文明确字节、26 个 CALL rel32、5 个条件 rel32 和 2 项虚表 image pointer；全部与同一磁盘目标相符。

结果 [followup independent audit v1](../../../../../../out/cs-t0/native-join-followup-independent-audit-v1.json) 为 `PASS_STATIC_BYTES_ONLY`，SHA-256 `F14BB99584391AA730BFA079A41F517A21E256E440526461CD2C1246A38B9BB9`。可重放的只读核对脚本为 [audit script](../../../../../../out/cs-t0/native-join-followup-independent-audit-v1.ps1)，输出仍为 create-new。

此补核确认具体指令及目标计算，没有执行 prejoin 稳定性验证，也没有证明无持久写。原 v1 的七项机器缺口、全部零主动能力、`csT00505Unlocked=false` 与 `t005Unlocked=false` 均保持不变；有条件的静态 prejoin 依据须经新的版本化契约和独立门才能升级资格。
