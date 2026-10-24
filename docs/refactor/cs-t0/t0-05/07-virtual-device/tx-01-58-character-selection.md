# tx-01-58：B 加入时选择角色

日期：2026-10-17。用户确认范围：A 沿用游戏开局选择，控制器为 B 指定加入时的角色。本次补齐控制器参数、隔离 IPC、原生普通工厂参数和结果记录；最后两项实机测试留待功能完成之后。分析、逆向与修改期间不运行测试。

## 实现前记录的失败方式

- 参数拼写错误、负数、溢出、重复参数被静默解释成默认角色；或者参数错误后仍枚举/打开游戏、注入或创建捕获目录。
- Rust/native IPC 字段、版本或偏移不一致；共享内存被后续修改导致同一 session 的角色改变。
- 把角色 ID 当作 selector 索引；传入 possessor 的 -1；绕过当前原生可选列表，或触碰解锁/存档。
- 修改 A、其他物理设备或非自有 K 的创建；在未知线程/上下文下写参数；额外调用工厂、改变已有 Join 两 pass/单次 owner。
- naked wrapper 定位栈参数错误，破坏返回地址、寄存器或拒绝分支的栈补偿。
- 只记录请求而未核对实际 B subtype，或者角色不匹配仍接受 B 输入。
- 角色产生额外 Main Player 时放宽既有 A/B 身份和两人拓扑；将编译或静态依据写成实机通过。

## 设计与静态依据

IDA 数据库 `isaac-ng.exe`，x86，image base `0x400000`。普通 Join 的 RVA `0x5BB40E` 为 `push edx`，`0x5BB415` 调 `0x5B9CD0`；角色为该 thiscall 的一个 4 字节栈参数。`0x5B9F12` 将同一参数传给 Player Init，`0x5B9F1B/1D` 压入 Variant=0/Type=1。selector 的选项数组在 PlayerManager `+0x70/+0x74`，selected index 位于 row `+0x14+20*i`；数组值才是角色 ID（`0x5BB32A/33D`）。

复用已有创建 Hook，只对唯一自有 K、主线程、同一 Game/manager/A、健康 exact pair 的受控 Join 改写栈参数；A 和其他设备走原参数。显式角色必须存在于当前原生可选数组，失败中性停用且不调用工厂；不添加解锁路径。默认 `native` 保留原生选择。配置初始化后由 native 复制冻结，不提供加入后换角。

角色名称/ID 取自本机 `resources/scripts/enums.lua` 的 PlayerType。名称目录只帮助选择，实际可选性以本次游戏原生数组为准。当前 T0 仍限定 Main vector 恰为 A/B；会生成额外 Main Player 的角色不属于已支持的双人拓扑，不放宽身份接纳。

## 验收范围

本次只安排一次统一的构建与命令行整体验收，产物写入新的 `artifacts/cs-t0/tx-01-58-build-01`。不注入、不发送游戏输入，也不提前执行最后两项实机测试。实机角色、A 健康、双端输入与恢复结果留待后续授权场景记录；不以构建通过代替 E2E。

## 使用方式

统一实机入口及现有专项包装入口新增 `-BCharacter`，例如在原命令上追加 `-BCharacter azazel` 或 `-BCharacter 7`。默认值为 `native`。控制器对应参数为 `--b-character azazel`；`PLAYER_AZAZEL`、`azazel-b`、`tainted-azazel` 等名称也可解析。

```powershell
.\cs-t0\target\release\IsaacCsT0.exe coop-active --list-characters
.\cs-t0\target\release\IsaacCsT0.exe coop-active --describe-character azazel
```

上述目录与解释命令只读本地名称表，不接触游戏。数字可表示 Mod 角色，native 仍按本次原生可选列表决定是否接受。名称表也包含原版内部形态，列出不代表当前可加入。

隔离 CoopActive IPC 升为 v3；`b_character` 复用旧 reserved 的偏移100，Header仍360字节、ScriptStep仍40字节、Event仍128字节。V2 canonical input的24/64字节、原版/Isaac Sync ABI及协议不变。控制器与 native 必须使用同轮新构建，旧v2 native会拒绝v3初始化。

新增 `CharacterSelection=30` 事件：action1是冻结请求（decision为请求ID，UINT_MAX代表native），action2是工厂前校验（query为原生参数，decision为选择ID，detail为显式请求时的可选数组长度，boolean为接受与否），action3是创建后核对（query为实际subtype，decision为工厂参数，after为A/B地址、Type、Variant，boolean为匹配）。`capture/result.json` 新增 `bCharacter` 请求/选择/实际值及对应事件。角色不匹配在绑定前中性失败，未观察不计PASS。

## 唯一整体验收结果

统一入口 `accept-character-selection-build.ps1` 执行一次，目录 `artifacts/cs-t0/tx-01-58-build-01`。Rust Release构建成功（2m07s）；Native/Helper均经VS2022 v143 Hostx64→Win32新目录构建与链接成功。CLI和入口检查后已进入source-snapshot，源码快照均存在，但最终汇总把PowerShell ErrorRecord对象递归展开，ConvertTo-Json提示超过depth12后内存不足，入口退出1，result.json未写入。**本轮整体FAIL**，不自动重跑；失败记录为 `acceptance-failure.json`。

角色选择生产代码保持本轮已构建内容；后续只修复报告链路，见[tx-01-59](tx-01-59-reporting-repair.md)。未操作游戏，native角色选择、A状态及恢复仍为实机未观察；最后两项实机测试尚未开始。
