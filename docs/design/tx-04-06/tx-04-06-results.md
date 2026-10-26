# tx-04-06 E2E 结果

> 最新状态（2026-10-25）：**tx-04-06 完成（用户接受，D28）**。用户决定不再执行第三次 E2E，将第二次 E2E（`tx-04-06-e2e-02`）算作验收通过：V1–V11 PASS；V12 只因第一、三档当日备份（`20261025.*`）找不到历史基线而 FAIL，这两份备份只改了 D20 的 Counter/Checksum 字段，D27 起每日备份已不参与判定。原报告不改写，各次失败记录原样保留。

下文地址都是 RVA（模块基址为 0），与[点位核对](../hooks/repentogon-binding.md)一致。IDA 数据库的基址是 `0x400000`，本次崩溃进程中 `isaac-ng.exe` 的基址是 `0x510000`。

## 第一次 E2E（`artifacts/repentogon/tx-04-06-e2e-01/`）

结果：**FAIL**。只启动了一次游戏，没有自动重跑。

- 入口：`out/repentogon/harness.exe e2e tx-04-06 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-06-e2e-01`。控制台输出存于 `out/repentogon/tx-04-06-e2e-01-console.log`。
- 使用的构建是 [ops build-04](../../artifacts/repentogon/tx-04-06-ops/README.md)（DLL 15:05，harness 15:08）。之后只改过 `mod/README.md`，没有重新构建。
- 启动前已告知用户：只启动一次，harness 接管键盘并在本机运行远端客户端，期间不要操作电脑。用户确认准备好后才启动。
- 按用户选择，运行前在 Download Helper（`repentogonreleasesbackup_3643104060`）目录临时创建 `disable.it`（15:17:45），运行后删除（15:18:15），它已恢复为启用。
- 运行结束后，测试 mod 已由 harness 移除，DLL 配置已写回关闭状态。新 DLL 保留在游戏目录，与 tx-04-05 相同。

### 已执行部分的运行时观察

以下内容来自 DLL trace、Lua 观测和游戏日志，不是 harness 的判定结论。

| 场景 | 观察 |
| --- | --- |
| S0 | 键位、环境检查通过；部署完成；只启动一次 |
| S1 | Lua 夹具在主菜单执行 `cs host`：`Idle → Hosting`（端口 27090，角色 0） |
| S2 | Endpoint `Negotiating → Established`（rx=2，tx=1），`Hosting → Warmup`。远端只建立一次握手，duplicate=0，stale=0 |
| S3 | A 稳定绑定（loop 557）。K 依次 allocated、constructed（id=1）、initialized、registered。Join 第一次 pass 为 Triggered(19)，三处 mode predicate 的原调用各一次，结果都是 0。第二次 pass 为 Triggered(14)，经过 site 1/3 后在创建点调用一次，角色 0。B 在 Join 返回后绑定（`join_return`）；Join 完成，passes=2；`Warmup → Active`（BindingsReady） |
| S3 Lua | `player_count=2`。B 为 Type 1 / Variant 0，Isaac，controller=1，不是 ghost |
| S3 持久观测 | Join 期间有两次 `PersistentGameData::TryUnlock`（index 381、363），前后值都是 1，dirty 没有变化 |
| tick 1 | OddPlayer，A、B 两个 ordinal 都完整执行了 pre → 原生调用 → commit |
| 崩溃 | 紧接着（loop 559–560）REPENTOGON 日志出现 `Exception thrown`，并写出 minidump |

远端第 1 轮的摘要：tx=61，rx=539，建立 1 次，duplicate=0，stale=0，没有收到 Bye，因为会话没有正常结束。

### 崩溃分析（minidump）

转储为 `crash/isaacv1.9.7.12.J273-20261025-151804-23536-31432.dmp`，附带同名 `.txt`。两份都是从用户的 `Documents/My Games/Binding of Isaac Repentance+/crash_dumps/` 复制来的，原文件没有删除。

- 异常码为 `0xE06D7363`（C++ 异常），由 `VCRUNTIME140!__RTDynamicCast` 抛出。异常类型为 `std::__non_rtti_object`（继承自 `bad_typeid`），消息是 `Access violation - no RTTI data!`。没有代码捕获它。
- 调用栈：`_main`（`0x50AC60`）→ Hook 蹦床（其中经过 `zhlIsaacCS.dll+0x2AB40`）→ `0x52E3E0` → `0x2F66E0` → `0x52F880` → `__RTDynamicCast`。
- `0x52F880` 的行为：遍历所有 Player，读取 `+0x160C`（`_controllerIndex`），经 InputManager 取得对应设备，然后执行 `dynamic_cast<IsaacRepentancePlus::NetInputDevice*>(KAGE::Input::InputDeviceBase*)`。B 的 controller 是 K 的 id 1，所以这里取到的设备就是 K。
- **根因。** `mod/src/game/device_k.cpp` 把 K 的 vtable 指针设为 `uintptr_t table[38]`，而 `table[-1]` 处不是 RTTI Complete Object Locator。`__RTDynamicCast` 读 COL 时发生访问异常，MSVC 运行时把它转成 `__non_rtti_object` 抛出。
- **移植遗漏。** 原型 `cs-t0/native/src/owned_virtual_device.cpp` 的 `BuildTables` 在表头放了一个自建的 Locator：类型名为 `.?AVVirtualInputDevice@IsaacCsT0@@`，类层次为“自身 + 基类 COL 的 `bases[0]`”。移植到 tx-04-06 时漏掉了这一步，ST1–ST9 的静态确认也没有把 RTTI 消费者列为风险。项目规范第 9 节把 COL/RTTI 列为留给整体验收的未决项，本次验收暴露了它。

失败后又做了一次只读静态核对，没有修改 IDB：

- 设备基类 vtable `0x732F04` 的前一项是 COL `0x745F4C`。它的 TypeDescriptor 为 `0x7D11B8`（`.?AVInputDeviceBase@Input@KAGE@@`），类层次为 `0x744DB8`（单继承，1 个基类）。
- 以 `InputDeviceBase` 为源类型的 `__RTDynamicCast` 共有 6 处：`0x26F70`、`0x2F29A0`、`0x3B1090`、`0x3B3980`、`0x52F880`、`0x565DB0`，目标类型都是 `NetInputDevice`。K 有了合法的 COL 之后，这 6 处应当都返回 null，走与物理设备相同的非联机分支。这一点是静态推断，尚未经运行验证。

### harness 判定

| 编号 | 结果 | 说明 |
| --- | --- | --- |
| V1 | FAIL | 12 处字节补丁都恰好安装一次且成功；运行中的 `cs status` 也显示 registered=33、pointers=8、能力组全部为 1。判 FAIL 是因为崩溃后取不到最终的 `cs status` |
| V2 | FAIL | S9.raw 和 S10.raw 没有执行 |
| V3 | FAIL | 三轮会话没有完成，generation 证据缺失 |
| V4–V12 | NOT_RUN | 归约在读取 `remote-2.jsonl`（文件不存在）时中止，记为 `evaluation_error` |

对应预先列出的失败情况 F10（崩溃）。

### harness 的记录缺口（如实保留，没有修改工具）

- 报告的失败原因是 `os error 10054`。它只是游戏进程崩溃后控制口 TCP 连接被重置的结果，harness 没有把它识别为崩溃。
- 报告中 `crash_changes` 为空，因为 `crash_files` 只扫描游戏目录，而游戏把转储写在 `Documents/.../crash_dumps/`。转储文件只在 `save-changes.json` 中作为“不允许的变化”出现。
- 某一轮的远端日志缺失时，整个归约会中止，其余 V 项只能记为 NOT_RUN，不能用已有证据逐项判定。

## 待用户决定

1. **K 的 RTTI COL 怎么修。**
   - (a) 建议：按原型自建 TypeDescriptor、BaseClassDescriptor、Hierarchy 和 Locator。基类取 COL `0x745F4C` 的 `bases[0]`，把 Locator 放在表头，vtable 指针指向表头之后。
   - (b) 直接让 `table[-1]` 指向原生 `InputDeviceBase` 的 COL `0x745F4C`。
2. **harness 的三处记录缺口是否一并修正。** 包括：扫描 `crash_dumps`；进程已退出时把失败原因记为崩溃；某轮日志缺失时逐项记 NOT_RUN/FAIL，并继续归约其余 V 项。
3. **用户目录中的崩溃转储原件是否删除。** 两份已复制到 artifact。
4. **是否沿用 tx-04-04 的 D16 与 tx-04-05 的 D20 先例。** 即保留 tx-04-06 编号，修完后执行第二次 E2E（`tx-04-06-e2e-02`）。

## 用户决定（D26）与修正

2026-10-25，用户按上面的建议决定：1 选 (a)，2 修，3 删，4 保留编号。已完成的修改如下，只做了纯构建，见 [build-01.log](../../artifacts/repentogon/tx-04-06-fix/build-01.log)。DLL 与 Rust harness 都编译、链接成功，没有新增 warning；Hook 清单与 ops build-04 相同（33 个入口、8 个设备槽、12 处字节补丁）。

- **`mod/src/game/device_k.cpp`。** K 的表改为 `DeviceTable{locator, slots[38]}`，对象的 vtable 指针指向 `slots`，所以 `slots[-1]` 就是自建的 COL。
  - 自建的 RTTI 与原型 `BuildTables` 相同：TypeDescriptor 名为 `.?AVRemoteInputDevice@IsaacCs@@`，沿用基类 type_info 的 vtable；自身 BaseClassDescriptor 的 contained=1、attributes=0x40；Hierarchy 有 2 项，即自身加上基类 COL 的 `bases[0]`；Locator 的 offset 为 0。
  - 准备表时读取 `DeviceBaseVtable-4` 处的基类 COL，并核对 signature/offset 为 0、基类层次只有 1 项、类型名为 `.?AVInputDeviceBase@Input@KAGE@@`。任何一项不符就记录 `device_k event=rtti_invalid`，不注册 K。
  - 13 个覆写槽及其他行为都没有改。
- **harness。**
  - `crash_files` 增加扫描存档根下的 `crash_dumps/`，路径前缀为 `documents/crash_dumps/`。
  - 收尾时如果发现新的 `.dmp`，记录 `failure_cause={"kind":"game_crash","dumps":[…]}`，并写入 report.md；原始错误仍保留在 `failure`。
  - V4 中某轮 `remote-N.jsonl` 缺失或损坏时，只把该轮记为 NOT_RUN 并判为不通过，其余各项继续归约。
  - `counters.json` 缺失时 V12 判为不通过，并记录 `counters_present=false`，不再中止归约。
- **崩溃转储。** 两份原件与 artifact 中的副本逐字节一致，已从 `Documents/My Games/Binding of Isaac Repentance+/crash_dumps/` 删除。

第一次 E2E 的报告和失败记录保持原样，没有改写。

## 第二次 E2E（`artifacts/repentogon/tx-04-06-e2e-02/`）

结果：**FAIL**。S0–S11 全部执行完毕；V1–V11 PASS，只有 V12 FAIL。游戏正常退出（退出码 0），没有新增崩溃转储，也没有新增截图。没有自动重跑。

- 入口与 e2e-01 相同，输出目录为 `tx-04-06-e2e-02`；控制台输出存于 `out/repentogon/tx-04-06-e2e-02-console.log`。使用 D26 修复后的构建（`tx-04-06-fix/build-01.log`，DLL 15:29:21，harness 15:29:29）。
- 启动前已告知用户，用户确认准备好后才启动。Download Helper 在 15:30:41 临时禁用，15:31:26 恢复；测试 mod 已移除。
- 最终状态：`trace_dropped=0`、`context_faults=0`、`foreign=0`、`queue_dropped=0`，`session_error` 和 `last_error` 都为空，REPENTOGON 日志中没有 ERROR/FATAL。

### 通过项摘要

| 编号 | 关键观察 |
| --- | --- |
| V1 | 能力组全部通过；33 个入口、8 个设备槽、12 处字节补丁，各恰好安装一次 |
| V2 | 在 Idle（S9.raw，向左）和 Hosting（S10.raw，向右）两个状态下，A 都能原生移动 |
| V3 | 三轮会话：`Idle→Hosting→Warmup→Active→Retiring→Idle`，结束原因依次为 Stop、PeerTimeout、RunTeardown；generation 依次为 1、2、3；没有 `tick_reject` |
| V4 | 三轮中，主机侧和远端的收发包数都能对齐（第 1 轮主机 rx=504、tx=1237）。每轮只建立一次握手，stale=0，foreign=0，两个队列的丢弃都是 0；每轮 Bye 尝试 3 次、成功 3 次。远端每轮收到 2 份字节相同的副本，按规则计数，不算错误 |
| V5 | 每轮 K 恰好创建一次，Join 恰好产生一个完整的 Isaac B。第 1 轮三处 predicate 的原值都是 0；第 2、3 轮原值为 1（原生会走 Baby 路径），全部被改写为 0，最终状态 `join_baby_overrides=5`。B 仍是完整的 Type 1 / Variant 0 / Isaac |
| V6 | S4.right 和 S4.up 各有 23 个有效 tick，松开后在 lead+1 内回到中性 |
| V7 | S5 共 68 个 tick，同一 tick 内 A、B 的输入互不串扰 |
| V8 | bomb edge 被接受 1 次、Resolve 1 次，炸弹数减一 |
| V9 | 浮层期间被阻断；跳帧、切房（loading 区间 loop 1200–1218）期间没有 StandardRoom tick；切房后 A、B 都重新绑定，B 之后还能移动 |
| V10 | 三轮结束时 B 都回到中性；K 的第 0 槽各被调用一次（3 个不同对象，NEVER，不释放）；Player 数回到 1，网络线程已停止 |
| V11 | 共 709 个 tick、803 个 Odd 配对，没有故障，两种 tick 都出现过 |

### V12 失败：当日备份的放行规则依赖日期

V12 的其他子项都通过：运行时计数、无转储、Lua 观测、ST8 持久观测（5218 次调用，`no_cs_added_writes=true`）、REPENTOGON 日志、退出码 0、S11 结束时 `run_active=0`。唯一失败的子项是 `save_changes_allowed=false`，涉及两个文件：

- `documents/save_backups/20261025.rgon_steam_persistentgamedata1.dat`
- `documents/save_backups/20261025.rgon_steam_persistentgamedata3.dat`

逐字节比较：两份文件长度都不变，只有距末尾第 8 字节和最后 4 字节发生了变化，也就是 D20 规定的 Counter/Checksum 字段。同一次运行中，第一、三档的云存档（`steam_remote/.../rgon_steam_persistentgamedata1/3.dat`）也只改了这两个字段，并已按 D20 放行。

原因：`apply_recorded_baseline_tx05` 只用完整路径到 e2e-08 的历史关闭配置区间里找同名文件。e2e-08 的当日备份名为 `20261024.*`，今天的是 `20261025.*`，所以找不到历史字段，无法放行。

- tx-04-05 的四次运行都在 2026-10-24，文件名恰好相同，所以当时能放行。
- 本次推进的第一次运行是今天第一次启动，`20261025.*` 是新创建的文件，按新文件放行了。
- 因此，这项失败来自 harness 规则对文件名日期的依赖，不是 C/S 新增的持久副作用。

报告 `report.json`/`report.md` 保持原样（FAIL），没有改写。

## 用户决定（D27）：存档判定排除每日备份

2026-10-25，用户决定：存档判定不看每日备份中的任何存档。

- `tools/harness/src/saves.rs` 的 `changes` 跳过 `documents/save_backups/` 下的所有文件，它们不再进入白名单、D18/D20 基线判定和 `save-changes.json`。`save_before`/`save_mid`/`save_after` 快照仍原样复制这些文件，作为原始证据。
- 同时删去随之失效的分支：`allowed_change` 里的备份规则（新建备份放行、已有备份只放行第二档），以及 `other_slot` 里去掉日期前缀的识别。第二档、附属文件、杂项和 D20 的其余规则不变。
- 只做纯构建，见 [build-02.log](../../artifacts/repentogon/tx-04-06-fix/build-02.log)：harness 编译成功，没有 warning；DLL 没有变化，没有重新链接。
- 第二次 E2E 的 `save-changes.json` 共 18 条，其中每日备份 3 条；去掉后剩下 15 条，全部为允许。这只是对已有账本的人工复核，没有重新执行归约，原报告保持 FAIL，没有改写。

## 用户决定（D28）：验收结论

2026-10-25，用户决定：不再执行第三次 E2E，直接将第二次 E2E 算作 tx-04-06 的验收通过。

- **证据等级。** V1–V11 由 harness 判定为 PASS。V12 由用户接受：运行时计数、无崩溃转储、Lua 观测、ST8 持久观测、REPENTOGON 日志、退出码这几项都由 harness 判定通过；唯一失败的存档项只涉及第一、三档的当日备份，逐字节比较表明它们只改了 D20 规定的末尾字段（见上文第二次 E2E 一节）。
- `artifacts/repentogon/tx-04-06-e2e-02/report.json`/`report.md` 保持原样（FAIL），没有改写。第一次 E2E 的崩溃记录与转储也原样保留。
- **未经运行验证的修改。** D27 的每日备份排除规则只做过纯构建，下一次使用该 harness 时才会第一次实际运行。
- **本次推进中的环境操作。**
  - 两次运行前都在 Download Helper 目录临时创建 `disable.it`，运行后删除，均已恢复。
  - 第一次运行的崩溃转储（`.dmp`/`.txt`）先存入 e2e-01 artifact，再从用户 Documents 删除。
  - 新的 DLL 留在游戏目录，配置已写回关闭。
