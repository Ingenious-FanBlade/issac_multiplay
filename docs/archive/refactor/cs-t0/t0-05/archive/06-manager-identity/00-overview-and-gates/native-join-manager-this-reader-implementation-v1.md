> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this production reader implementation v1

记录日期：2026-09-01

状态：`PRODUCTION_COMPONENT_IMPLEMENTED / STATIC_LOCAL_EPOCH_CLOSURE_PENDING`

本记录描述 create-new 的 `IsaacCsT0.ManagerIdentityReader.dll`。它实现同代身份的生产判定组件与公开 C ABI，但不把组件装入游戏、不修改冻结的 `IsaacCsT0.Native.dll`、不安装 Hook，也不授予 runtime、active、live、05-05、T0-05 总体或 T0-06 权限。manager-this 能否从 `UNKNOWN` 提升，仍取决于独立静态 local-epoch 闭包与独立验收。

## 1. 新增实现

- `cs-t0/native/include/isaac_cs_t0/manager_identity_reader.hpp`：常量、状态机、失效原因、snapshot 和 C ABI；
- `cs-t0/native/src/manager_identity_reader.cpp`：同一生产实现；
- `cs-t0/native/IsaacCsT0.ManagerIdentityReader.vcxproj` 与 `.def`：独立 Win32 DLL；
- `cs-t0/native/tests/manager_identity_reader_main.cpp` 与 `IsaacCsT0.ManagerIdentityReaderTests.vcxproj`：直接编译同一生产源，并额外通过 `LoadLibrary/GetProcAddress` 调用实际构建 DLL；
- `cs-t0/build-manager-identity-reader.ps1`：只构建并执行这个 create-new 组件的 Debug/Release 门。

旧 v1-v6 admission、旧 Native project/source/build 脚本与既有二进制均不修改。新 DLL 必须由后续明确授权的 adapter 集成层显式调用；导出存在不等于旧生产入口已经加载它。

## 2. 冻结身份与读取预算

reader 在 `Create` 时冻结当前进程 PID、`GetProcessTimes` creation time 和主线程 ID。每个观察点都重新读取进程创建身份并要求线程不变；PID 复用、creation-time 漂移或错误线程立即永久失能。

一次候选 epoch 的最大读取量固定为 8 次、32 字节：

1. caller 入口双读 InputManager vptr，要求运行时派生 vtable 为 image RVA `0x00782950`；base/purecall vtable、读取失败或两次漂移均拒绝；
2. caller 入口双读 GameRoot slot RVA `0x00871678`；
3. manager entry 再双读同一 slot，并校验原始 `ECX == root + 0x1BAA8`；
4. ordinary reload/spawn 前第三次双读同一 slot，并校验 receiver 与冻结 root/generation。

所有地址为 Win32 `uintptr_t`，加法先检查溢出。reader 不读取 GameRoot 或 PlayerManager 对象内部字段；成功路径只保留当前进程内的临时地址，不写入 artifact。

## 3. 状态机与发布门

成功顺序严格为：

```text
Ready
  -> InputManager validated
  -> CallerSampled
  -> ManagerEntered
  -> OrdinarySampled
  -> SpawnValidated
  -> ManagerExited
  -> Eligible
```

`ConfirmSpawn` 只到 `SpawnValidated`，此时 `publication_eligible=false`。只有精确 manager normal exit wrapper 和三个 caller normal return wrapper 中的一个依次被观察到，才进入 `Eligible` 并允许发布先前缓冲的证据。缺少任一 normal-exit、乱序、重复调用或异常后重新进入，都会因 stale state 永久失能；不能用 spawn 调用前的地址相等抢先发布。

## 4. generation 与 fail-closed

reader 拥有单调 `generation`，caller 首样本冻结 `epoch_generation`。slot writer、computed alias writer、owner replacement、lifecycle、callback reentry、exception resume、读取失败、双读漂移、进程复用、线程漂移、错误顺序、receiver 不一致、预算超限或 generation overflow 均清除 publication eligibility，并进入不可复活的 `PermanentlyDisarmed`。

`IsaacCsT0ManagerIdentityInvalidate` 是 writer/lifecycle wrapper 的 pre-store 入口；调用者必须在原始写入或可重入 callback 前调用，无论新旧指针是否相等。`UncertainStop` 进入 `UnsafeRetainModule`；此状态下 `Destroy` 故意不释放 handle，防止不确定停止被伪装成安全卸载。安全生命周期结束可调用 `Retire`，其 generation 同样不再复用。

## 5. 当前构建证据与边界

2026-09-01 的本地 Win32 Debug/Release 构建均通过；每个配置执行 410 项断言，并实际动态加载对应 DLL、解析全部 12 个导出、完成正向 epoch，验证 spawn 后仍不可发布、两个 normal-exit 后才可发布，以及错误线程、永久失能、generation overflow、预算超限和 Unsafe 路径。

这些测试证明生产状态机和公开 ABI 的有限实现行为，不证明目标程序的所有 alias writer、callback transitive effect 或异常路径已经静态闭合。最终 v7 必须同时满足：精确目标字节/调用图闭包、local epoch 的 all-writer/ABA 排除、此生产 reader 的 source/project/hash 绑定，以及不同测试智能体的独立对抗验收。任何一项缺失，`managerThisIdentityStatus` 必须保持 `UNKNOWN`。
