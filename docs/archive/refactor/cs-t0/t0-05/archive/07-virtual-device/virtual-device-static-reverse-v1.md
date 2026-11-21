> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device 静态逆向交接 v1

## Scope / target

- 角色：独立 `REVERSE`；仅为 05-05 的只读决策提供证据。
- 目标：固定 Win32/x86 `isaac-ng.exe`，IDA image base `0x00400000`。
- IDB：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`。
- 目标 SHA-256：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- 地址换算：`VA = RVA + 0x00400000`。
- 本次只做磁盘/IDB 静态读取；未启动游戏、未附加或打开游戏进程、未使用 x32dbg、未修改 IDB、未注入、未创建 IPC、未运行测试。
- 05-04 的 native Join 仅是 `DESIGN_ONLY` 用户假设，本交接不提高其证据等级。

## STATIC_CONFIRMED（6 项）

### SC-01：assignment 与 Player consumer

- `Player::SetControllerId`：RVA `0x3A6450`，ABI
  `void __thiscall(Player*, int32 controllerId, uint32 propagate)`，`ret 8`。
- controller id 写入 `Player + 0x1618`。
- 静态调用点：menu unassign `0x5B27EB`，bytes `E8 60 3C DF FF`；
  disconnect `0x55B87F`，bytes `E8 CC AB E4 FF`；
  reassign `0x55BCA0`，bytes `E8 AB A7 E4 FF`。
- Player update：RVA `0x382AF0`；函数指针槽 RVA `0x76BDDC`。
- consumer callsites：movement `0x385BC2`、bomb `0x385BC9`、item `0x385BD2`、
  pill `0x385BD9`、shoot `0x385BE0`、drop `0x385BE7`。

### SC-02：selected key 必须解析到具体 InputDevice

- InputManager singleton/object：RVA `0x857B18`。
- keyed query helper：RVA `0x620390`；加锁后寻找首个 `device + 0x0C == key` 的设备。
- 设备 query：RVA `0x620FB0`；查询依赖已解析出的具体 device。
- 因此只有 controller id / selected-device key，而没有对应具体 device，不能完成 native query 链。

### SC-03：InputManager active shared_ptr vector

- vector begin/end/capacity 位于 manager `+0x08/+0x0C/+0x10`，元素 stride `8`。
- 元素布局：object pointer `+0`，reference-count control block `+4`；mutex 位于 manager `+0x14`。
- 插入函数 RVA `0x620DF0`：锁 mutex、push 8-byte pair，必要时调用 grow RVA `0x621630`，
  解锁后调用 manager vtable `+0x78` notification。

### SC-04：physical poll 到 logical input 的必经链

- manager tick callsite RVA `0x554F7D`，bytes `E8 7E AC 0C 00`；update RVA `0x61FC00`。
- per-device common RVA `0x61F280`；RVA `0x61F307` bytes `8B 40 78 FF D0`
  调用 InputDevice vtable `+0x78`；poll return anchor RVA `0x61F30C`。
- logical vtable `+0x30/+0x34`：`bool __thiscall(device*, uint32 action)`，`ret 4`，结果在 `AL`。
- logical vtable `+0x38`：`float __thiscall(device*, uint32 action)`，`ret 4`，结果在 x87 `ST(0)`。
- 该链把具体 device 的 poll/query 结果送往 SC-01 的 Player consumers。

### SC-05：native NetInputDevice 存在，但当前 adapter 不接受它

- `IsaacRepentancePlus::NetInputDevice` vtable RVA `0x778030`，38 slots，152-byte SHA-256
  `C36AD094D3F5785848C0BF225134A0593B8B0BB5F35A2806336881068B510088`。
- vtable `[-1]` bytes `78 EF BA 00`，指向 COL RVA `0x7AEF78`；
  TypeDescriptor 名为 `.?AVNetInputDevice@IsaacRepentancePlus@@`。
- logical `+0x30/+0x34/+0x38` 分别指向 RVA `0x501480/0x5014C0/0x5026A0`。
- poll `+0x78` 指向 RVA `0x309C30`，bytes `B0 01 C3`。
- native add-player 路径 RVA `0x2F6500`，两处 `alloc(0x3F8)` 后调用 Net ctor
  RVA `0x501930` 或 `0x5015B0`，再经 wrapper RVA `0x505F60` 注册。
- 当前 adapter 只识别 Keyboard vtable RVA `0x7A1678` 与 Gamepad vtable RVA `0x782C80`，
  并要求具体设备、真实 poll return、owner/lifecycle/thread、selected key 一致；
  无具体 device 或 Net/未知 family 均不会提交覆盖输入。

### SC-06：native Net allocation/registration/assignment 的局部形状

- base ctor RVA `0x61E050` 写入 base vtable，并令 `device + 0x0C = global[C7362C]` 后递增 key。
- deleting destructor RVA `0x5017C0` 最终释放 `0x3F8` 字节对象。
- wrapper RVA `0x505F60` 分配 `0x18` 字节 `ReferenceCount<InputDeviceBase>`，
  control block vtable RVA `0x778014`，object 写入 `+0x14`，初值 dword `+4 = 0x00010001`，
  再调用 SC-03 的 manager insert RVA `0x620DF0`。
- add-player assignment callsite RVA `0x2F68B2`，bytes
  `6A 01 56 8B C8 E8 94 FB 0A 00`，以 device key 调用 SC-01，`propagate=1`。
- 这些事实只证明原生网络路径存在局部构造/注册/assignment 形状，不证明可复用 virtual eligibility。

## EXISTING_RUNTIME_EVIDENCE（不升级为静态证明）

- 已有 05-03 live-11 记录在物理手柄断开后出现 B `BINDING_UNBOUND`，并走 unassign/reassign 事件。
- 该记录支持“第二实体设备消失时 B 会失去绑定”；不证明 virtual device 安全、可创建或可 teardown。

## UNKNOWN / missingEvidence（10 项）

1. `UNKNOWN`：脱离原生 network add-player transaction 后，virtual device 的唯一 owner 与责任边界。
2. `UNKNOWN`：allocation context、constructor 前置条件、失败 rollback、重入规则及异常/早退清理。
3. `UNKNOWN`：完整 ReferenceCount ABI、强弱计数语义、最后一次 release 路径与销毁线程。
4. `UNKNOWN`：创建、插入、poll、assignment、移除各自允许的线程与 main-loop phase。
5. `UNKNOWN`：device key 的并发、回绕、碰撞、保留、复用和进程生命周期规则。
6. `UNKNOWN`：active vector 的精确 remove API、锁顺序、遍历者同步及 erase 后 notification。
7. `UNKNOWN`：38 个 vtable slots 的完整签名、语义、所有调用点与 ABI；现有 slot 绑定并非穷尽证明。
8. `UNKNOWN`：Net state ingestion 的消息格式、合法 producer、frame window、checksum 与状态发布规则。
9. `UNKNOWN`：device insert、Player assignment、adapter generation、join generation 是否构成原子事务。
10. `UNKNOWN`：disconnect、player exit、shutdown、部分初始化失败及模块 unload 的 exactly-once teardown。

以上任一项未闭合都不足以填写 `REQUIRED_ELIGIBLE`；不得以局部 ctor/vtable 事实替代生命周期证明。

## Decision boundary

- `necessityAssessment = REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`
- `formalDecision = null`
- `workPackageStatus = BLOCKED_BY_EVIDENCE`
- 理由：Endpoint B 必须在 Authority 没有第二实体手柄时持续工作；native-only adapter 又要求 selected key
  解析到受支持的具体物理 device。virtual device 因而“必要”，但上述 ABI/owner/lifecycle 证据未闭合。
- `csT00506Unlocked = false`。
- 所有 runtime、active、mutation、process-access、injection、IPC、game-launch 权限必须为 `false`。
- 本交接只供阻断决策绑定，不授权实现、运行时验证、passive/active canary 或任何状态变更。

## 前序证据绑定（SHA-256）

- T0-04 reverse handoff：`01E250DDC2B40C6FAD861FEACF7E5D0721F2857DEAF7312F64D6FE33EDEF3952`
- 05-03 device-rebind evidence plan：`6220868EC02F9C7661ACA381507DAB6A06DBB022A7B75E7DCDC3D69FAB0D3E18`
- 05-03 contract v2：`444141651E42886AC6DC9807D41A429A8DA89B8CB12E2855E659E6DFEA01D2E0`
- 05-03 static evidence：`57C8A805488EE28384F04EE7AD959A51B58734A1CEF7768842C39A9A1D491D53`
- 05-03 live-11 events：`795986B50E0BE59378E20B9CA74C425C3EE49D984A1510C0A45D8BB45DE4F650`
- 05-04 planning assumption：`816D4AC883A3234922498167649159590E6711891C3AA724957E2EE867BA535B`

