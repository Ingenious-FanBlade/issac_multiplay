# T0-01 C/S 独立证据基线

状态：`PASS`。成功的只读运行证据为 `20260818T142940Z-t0-01-baseline`。该文档只陈述 T0-01 结论；后续 T0-02 的实现与待执行实机门见 [模式隔离说明](mode-isolation.md)，不会反向扩大本基线的证据范围。

## 固定镜像

| 项 | 值 |
| --- | --- |
| PE machine | `0x014C` (x86) |
| timestamp | `0x69E6E3A7` |
| image size | `0x0092C000` |
| preferred image base | `0x00400000` |
| 当前双补丁 `.text` SHA-256 | `197175428117EC352324F22FA44CF42D60511F0A4887BF6DC5FECFBF9A6EE758` |
| Steam 原始备份 `.text` SHA-256 | `79F75061EAF803CB67F5CA037C5236016506622550626AF7BE040B0E3F98F106` |

完整文件 SHA-256、section 表、补丁状态、完整 masked features、原始字节、目标 RVA、call/IAT target、ABI、线程证据和来源均以 [机器可读账本](ledger/isaac-cs-t0-target-ledger.json) 为准。

## 已知目标

- manager slot、network backend slot、Steam context descriptor；
- Steam/EOS backend vtable，其中 Steam Receive/Send 分别由 `+0x08/+0x14` 槽定位；
- 输入推进 `RVA 0x50B8E5`、身份匹配 `RVA 0x50B754`、现有 online dispatch `RVA 0x5110AB`；
- Lua 注册入口、`lua_newstate`、`luaL_newstate`、`lua_close`、simulation `lua_pcallk`。

上述项目当前仅为 `STATIC_CONFIRMED`。一次 smoke 可以把本次构建/进程样本升级为 `RUNTIME_OBSERVED`，不能自动形成跨版本契约。

## 未知与拒绝项

`gameplay tick`、`input sample`、`entity update`、`render`、`local co-op manager`、`entity container` 均保持 `UNKNOWN`。旧的宽泛 online input window 被登记为不能证明通用 gameplay tick 的 `REJECTED_HYPOTHESIS`。

framestate、checksum、RNG history 只登记为诊断入口；完整 snapshot/apply 能力保持 `UNKNOWN`。

## PASS 门

当前补丁版本离线 inspect、只读启动/退出 smoke、artifact sequence/hash/RVA 重定位/IAT owner/无调试注入/目标字节前后一致均已通过：

- `authority.jsonl` 共 6 条事件，sequence 缺口为 0；`client.jsonl` 为空；
- 13 个目标的定位、语义解析、IAT/vtable owner 与前后字节一致；
- 模块基址为 `0x006D0000`，所有捕获地址均满足 `moduleBase + RVA`；
- `CheckRemoteDebuggerPresent` 始终为 false，禁止模块为空，安装 Hook 数为 0；
- 游戏完整文件 SHA-256 和 `.text` SHA-256 在启动/退出前后未变化；
- artifact 输入文件哈希全部通过重放，`firstDivergence = null`；
- artifact 未包含 Windows 用户名、绝对游戏路径或完整 SteamID。

第一次 smoke 因观察器在进程启动时使用 Toolhelp 模块快照得到瞬时 `ACCESS_DENIED` 而失败，失败 run `20260818T141920Z-t0-01-baseline` 已保留。观察器随后改用只读查询句柄上的标准模块枚举并增加有界启动稳定窗口；成功 run 使用工具 SHA-256 `E4923845B1398E6026B276EE83A6D665926223E9A82D0052B5AFF0C801E3997B`。
