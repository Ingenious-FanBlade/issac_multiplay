# tx-01-45 Virtual Device 最小生产闭环实现

后续 tx-01-48 已修复拓扑对 Lua engine 的误拒绝；一次实机确认 A/B 绑定、设备注册、原生 Join、B 退休及 A 继续、菜单 teardown/47 槽恢复，DLL 常驻。完整 E2E 仍为 NOT_OBSERVED（35 PASS、7 未观察项），见[最新结果](tx-01-48-binding-join-repair.md)。下面各轮记录保留当时事实。

后续 tx-01-47 已修复事件发布调用，实机初始化和正常 shutdown 成功、47 槽恢复完成；完整 E2E 仍为 NOT_OBSERVED（A/B 绑定、注册与 Join 未确认），见[当前记录](tx-01-47-event-publication-repair.md)。tx-01-46 的链接修复及初始化崩溃保留在[历史记录](tx-01-46-link-repair.md)。下面 tx-01-45 验收记录保留该轮事实。

日期：2026-10-16。状态：生产源码、构建接线、控制器和 E2E 入口已新增；**唯一一次整体验收 FAIL：Rust 构建成功，Native 在链接阶段报告 LNK1101 / MSPDB140.DLL 版本不正确，实机阶段 NOT_RUN**。不能据此标记 T0-05 完成。见[验收记录](tx-01-45-results.md)。

## 用户已确定的行为

自有 38 槽表、六个覆写槽、六槽 control block、真实 base ctor/Init、S2、原生 Join 路线 A；BASE_CONSTRUCTED 后 NEVER，base ctor 前 pin DLL，正常退休保留 root 并停在 UNREGISTERED。Authority-pre/current、normal-post/previous，poll 只返回 active；缺输入立即中性。正常顺序、有效分配/缓冲为本次范围；完整 OOM/EH/worker/并发镜像闭包没有完成。

## 代码与接口

- Native 新入口 `IsaacCsT0CoopActiveInitialize` / `IsaacCsT0CoopActiveShutdown`，helper 参数 `--inject-coop-active` / `--stop-coop-active`。resident stop 检查目标 DLL 仍在，不等待卸载，也不调用 FreeLibraryAndExitThread。
- `coop_input` 提供独立 V2 状态/命令及 encode/decode；尺寸 24/64，version=2，held/edge mask 均为 0x0F。仅复用 V1 的轴规范化和合法性算法，不修改 V1 wire/API。A/B 使用同一 Participant 实现，各自的 session/ClientId/ordinal、sequence、dedupe、queue、generation、planner、current/previous 隔离。
- `owned_virtual_device` 由真实分配器、base ctor、Init、control ctor 初始化原生前缀；只写已采纳的自有 vptr/control object 字段及扩展。运行时检查模板 32 项、实际 binding/双缓冲、key 和 exact pair。继承 Base 32 槽的有效状态不会被析构。
- `coop_active_runtime` 复用 `loop` 的同一安装/恢复事务：原有 32 槽 + 六 consumer call + Join pass/actual-create 两 call + 七 pointer slot，共 47 槽。strict transaction 的 digest 参数为空；本路径不调用任何 digest snapshot 或旧 active preflight。
- `IsaacCsT0 coop-active` 使用独立 mapping、nonce、target PID/creation time 和 ClientId 1→A0、2→B1。映射 Header=360，ScriptStep=40，Event=128，完整 mapping=4204904 字节，C++/Rust 均有编译期布局断言。

canonical 不包含 Player/device/control/K。generation 是本地提交 envelope 与绑定标签；换代丢弃 pending/物理历史并撤销可见性，但不重置 session sequence 与去重高水位。首个有效命令前 current 无效，所有查询中性；首次有效显式 edge 不依赖 previous。此后无输入的合法 neutral current 可以提交为 previous。

游戏对象仅由可信主线程边界操作。query 重新核对实际 Main Player、key 和 exact pair；原生跨 consumer 查询按许可表处理。A 的 physical query original 恰调用一次，受控失败路径继续中性至 Hook 恢复；UI/非受控域保持原生。B 没有虚构的 Base original。物理采样只读真实 keyboard/gamepad state，不读 synthetic action cache。

Join pass 的 command 配置与实际 query 事件分开；缺少实际调用记录为 NOT_CALLED。19/14 各使用一个独立 pass，stored-confirm 仍由原生控制。actual-create owner 仅匹配 B exact K/row 的 9BB415；deny 恢复 4-byte selectedType 栈责任并转 9BB561。只有 pass 返回后 Main B、正 K 和 exact pair 实际匹配才记 Joined。

B 退休发生在 TopUpdateExit：先中性/换代、native pair remove、确认 exact pair 缺席，再有条件调用普通 Main B erase；没有实际 B attempt 时不删除新出现的未知 Player。vector strong 由原生消费一次；正常 coordinator 不释放 root，也不调用旧 FinalRelease。意外 control 并发、计数 floor/溢出或析构尝试是终止性 fault；内存仍常驻，**不能把该检测称为完整并发安全算法**。

控制器异常返回只尝试一次正常 resident stop。Native 同时持有控制器进程的 SYNCHRONIZE handle；控制器进程退出后中性、排程退休，再由单一 shutdown owner 尝试正常恢复。恢复不确定时保留模块和对象，不强制卸载或重新注入。当前研究入口每个游戏进程只初始化一次；停止后不提供盲目重新初始化。

## 本轮新增静态读取

IDA MCP survey/health 已直接连接 32-bit isaac-ng.exe IDB，base 0x400000、image size 0x8FE000；仅使用非哈希元数据，未改 IDB 或读取游戏进程。

- A1E050 的 key/vptr、40CC10 的 native control/Mutex 初始化、A1E210 的 slot5 与 `ret 4`；A20DF0/A20EB0 的按值 pair、`ret 8` 及 native release。
- 6FADA7 原始 `E8 94 FE 2B 00`；9BB415 `E8 B6 E8 FF FF`，前置 9BB40E push selectedType；9BB424 仅为后续 key setter。
- 785BCE push 0 → 785BD2 调 791A40：Item consumer 的 slot=0；785BD9 调 7A3D30 为无 stack 参数的 Pill/Card consumer。
- 417470 `8A 81 73 01 00 00 C3` 只读 Entity+0x173；42A320 `8A 81 A9 20 00 00 C3` 只读 Player+0x20A9。用于死亡/ghost 变化失效，不写这些字段。
- 792AD6 `or [ebx+1378h],10h` 与 7A3F3C `or [edi+1378h],8`、Pill/Card 17A0/17A4/17A8 原始访问支撑新增 consumer-state witness。Item 主 slot 的 ID/charge 字段来自当前反编译，保持静态来源边界。

其余正常路径继承落盘 [v18 证据](../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v18.md)及其指向的早期 ABI/勘误。工作区当前没有 v18 主报告链接目标，本次没有伪造或重建它。

## 唯一验收的使用方式

先读[实现前 E2E 场景](tx-01-45-e2e.md)。用户需准备授权范围内的隔离 Offline run，初始仅普通 A、Online roster 为空；使用能实际观察 Bomb、主 slot Item、Pill/Card 和 Drop 的资源/场景。程序不会为测试自动生成道具、修改存档或启动/重启游戏。

全部代码准备完并取得具体实机操作授权后，只执行一次：

```powershell
pwsh -NoProfile -File .\cs-t0\accept-virtual-device-e2e.ps1 `
  -GameExe 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe' `
  -ArtifactDirectory '.\artifacts\cs-t0\tx-01-45-live-01' `
  -Authorization '<用户明确允许的隔离场景、操作及持久写入范围>'
```

入口仅构建 Rust controller 和 Release|Win32 Native/helper，再进入同一次真实游戏 E2E。不会运行旧 build.ps1、Cargo tests、离线 harness、smoke 或哈希工具。编译失败也记录这次验收失败，不自动修补重跑。

控制器命令：`status` 查看状态；`mark <操作说明>` 记录人工操作；脚本结束后 `script` 使用新 sequence 再提交同一组意图（不再创建设备/Player）；最后用 `conflict-b` 验证同序号冲突中性并触发 B-only retirement，观察 A 继续；完成授权的 menu teardown 后输入 `stop`。`retire-b` 是不制造输入冲突的正常退休命令，不能替代冲突用例。EOF 不当作批准或停止指令。

默认脚本持续约 151 个 Authority tick，含移动/瞄准、held、四类 edge、duplicate/expired/invalid/old-generation/wrong-session。它不是实时网络协议，也没有无条件自动重复 edge。换房、死亡/复活后可按实际授权再次发送 `script`。

产物位于新目录：`build.log` 与 `capture/{invocation.json,events.jsonl,loop-events.jsonl,operations.jsonl,helper.log,restoration.json,result.json}`。根 result.json 是 capture 结果的副本。恢复记录含 47 个目标的原始期望/实际字节；consumer witness 为原始字段，非 hash。报告检测重复接受、Resolve/commit/consumer、重复查询不一致，列逐项结果及 event sequence；未观察的场景不能记 PASS。

consumer-state witness 不是任意道具效果、所有实体生成或全部 Mod 副作用的通用证明；最终报告必须保留此边界。完整 OOM/EH、worker 竞争、全 alias/observer、完整网络与所有 Mod 兼容未完成。

## 验收记录

2026-10-16，用户明确回复“场景已准备好；允许全部操作”后，通过统一入口执行了一次整体验收。分析、编辑期间未提前测试。

结果 FAIL：Rust release 构建成功；Native 源码编译及链接代码生成完成后，链接器报告 `LNK1101: MSPDB140.DLL 的版本不正确`。入口以 1 退出，未构建 helper、未启动控制器、未注入或向游戏发送输入。所有实机场景均为 NOT_RUN，无 Hook 或 DLL 运行时恢复事件。

保留原始 build.log 和入口生成的 result.json；补记本次参数、授权、场景配置与逐项阶段结果。未修改工具链、未自动修补或重跑。具体证据见[验收记录](tx-01-45-results.md)。
