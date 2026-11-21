> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device 原生证据闭合下一步 v1

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

日期：2026-09-08  
状态：`PLANNED_READ_ONLY / NOT_STARTED`

## 1. 当前基线

当前正式入口是 `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v4.json`。
离线 lifecycle 原型已经证明七态状态机、引用类别/上限、线程拒绝、drain、
vector absence、13 个 rollback 以及 exactly-once delete/free 的代码侧义务；
该结论只具有 `OFFLINE_IMPLEMENTATION_CONFIRMED` 等级。

真实 virtual device 尚未实现，也未接入生产 DLL。对象 owner、创建/析构与
引用计数三项的 `nativeEvidenceStatus` 和 `formalStatus` 都仍为 `UNKNOWN`；
`formalDecision=null`、`implementationEligibility=false`、
`workPackageStatus=BLOCKED_BY_EVIDENCE`。

## 2. 下一步固定目标

下一步只闭合三条原生静态证据，不编写或注入真实 virtual device：

1. `NATIVE-ALIAS-CLOSURE`：从两个 keyed-lookup vtable 入口出发，穷尽间接
   caller、返回 shared-pair 的所有 copy/move/reset/escape 和最终释放点；证明
   manager remove 前所有 reader/callback alias 都可被关闭或 drain，或者记录首个
   无法排除的反例。
2. `NATIVE-FACTORY-CLOSURE`：寻找非 OriginalOnline 的可复用 InputDevice
   创建/析构形状，闭合 allocation、member construction、control bind、insert、
   notification、publication、rollback、deleting destructor 与 deallocation 的全部
   正常/失败边。若当前 PE 不存在该 factory，结论必须保持 `UNKNOWN`，不得以离线
   coordinator 或 `NetInputDevice` 替代。
3. `NATIVE-RELEASE-THREAD-CLOSURE`：穷尽 strong/weak 间接 release caller、
   vector grow/erase/clear、callback/reader 临时副本及 shutdown/destructor 路径；
   给出 `uint16` 最大并发引用数及 overflow/underflow/double-release 排除，并确定
   last-strong/last-weak 的调用线程和 main-loop phase。

执行顺序固定为 alias universe → factory/rollback → release/thread。前一项的 alias
集合是后两项 cardinality 和 final-releaser 证明的输入，不能并行猜测闭合。

## 3. 交付物与证据门

- 独立 TEST 角色先封印覆盖要求和失败反例格式；独立 REVERSE 角色随后只读分析
  固定 Win32/x86 PE，两个角色不得互相代验。
- REVERSE 交接必须记录 PE SHA-256、machine、RVA/bytes 或 image-pointer anchor、
  完整 caller/path 分类、线程来源、首个未闭合边和每项 verdict。
- 新决策必须 create-new，并哈希绑定 v4 决策/最终验证、TEST preseal、REVERSE
  交接及新增 anchors。只有三条原生证据和离线义务同时闭合时，对应 formal 状态
  才可提升；规范文字、fake port 和 harness 不能充当 PE 事实。
- 任一间接目标、alias escape、失败边或线程来源不能穷尽时，该项继续
  `UNKNOWN`；不得为了 promotion 放宽 coverage flag。

## 4. 压缩验证

只保留四项：一次固定 PE/hash/新增 anchors 批次、一次 create-new 保守决策验证、
一次 verify-existing、一个参数化负向批次。负向批次覆盖遗漏 indirect caller、
alias 逃逸、factory/rollback 缺边、未知 final-release thread、计数边界未闭合、
用离线证据伪造 native/formal promotion、哈希/权限/解锁漂移及覆盖已有输出。

不运行 Cargo 全量、完整 T0 build、生产 Native/Debug 矩阵、历史宽回归或重复
self-test。

## 5. 运行时与注入边界

本下一步不启动或访问游戏进程，不使用 x32dbg，不创建 IPC、Hook、注入或 canary。
只有原生 ABI/lifecycle 证据闭合、05-04 正式门通过、另行实现生产候选并获得新的
单次授权后，才可单独规划一个 passive canary；passive 通过后才考虑一个最小
active canary。当前所有权限及 05-06/T0-05/T0-06 unlock 继续为 `false`。

