# tx-02-02：A物理输入与正常停止末帧发布

2026-10-20，Asia/Shanghai。用户要求修复tx-02-01的A全零输入和停止时Publication7，再继续测试。本轮完整目标：排除控制器/helper控制台抢焦点、保留真实物理源的前台证据；停止请求只阻止新tick，已进入模拟的tick完成post/ACK/STATE后再关闭采集与协议。完成全部修改和纯编译后，沿用一个统一入口只执行一次完整E2E，不自动重跑、不校验哈希、不新增单元测试。

实际 `tx-02-02-e2e-01` 完成唯一整体E2E，**29/31 FAIL**。停止末帧Publication7、trace3990/lost0、正常stop/清理及70槽恢复均通过；两项失败是A物理输入全零。事后确认连续输入在Authority激活前结束；后续由[tx-02-04](tx-02-04-owned-physical-input.md)修复并取得33/33 PASS。历史失败原样保留。

## 实施前失败方式

1. Python启动控制器、Rust启动注入/停止helper时创建可见控制台，在SendInput序列开始后夺走游戏前台，工具成功却游戏原生采样始终中性。
2. 只在输入开始时记录前台，无法分辨后来失焦、原生键位/active状态问题；把操作系统投递成功误当游戏动作成功。
3. 为补偿输入而直接写A世界、设备缓存或canonical，破坏A/B共同codec/queue语义。
4. helper线程在main-thread在途tick完成前world::Close，使本tick采集token/post身份不可用，commit已经保存而ACK/STATE缺失。
5. 仅延后Close，但stop请求后仍无限开始新tick，导致清理不能到达；或过早退役session导致已完成的native正常返回被当失败。
6. 丢弃最后真实commit、放宽逐commit ACK/STATE覆盖或忽略Publication7，将真实失败改写为通过。
7. 恢复重入/双stop、盲目重注入常驻旧DLL、强制卸载，或先跑独立输入探测再做E2E。
8. 改动窗口/采样诊断后更改IPC布局、原版或Isaac Sync行为；前台PID等临时观测误进入canonical/wire身份。

## 已有证据和设计

tx-02-01：工具输入68步返回成功，原生272个PhysicalSample和271个A Resolve全零；旧记录没有窗口焦点信息，**控制台失焦仅为待实机验证的原因，不能写成历史已观察事实**。控制器和helper的启动源码确实没有CREATE_NO_WINDOW。

停止失败直接证据为正常stop于tick269、tick271有commit/回执且随后Failure7，ACK/STATE均只覆盖1..270；Shutdown源码在请求stop前从helper线程调用world::Close。修复应移除此处提前关闭，在主线程Authority进入时看到stop就不开始新的模拟；已进入的模拟及TopExit完成照常，TopExit在发布后退休协议/关闭采集并设置stop_handled。保持单一协调器与70槽恢复。

实机一次E2E保留原corpus、历史缺恢复文件报告场景、固定seed/outage、A/B启动、B边沿、每commit ACK/STATE、世界身份、零trace丢失和70槽恢复检查。增加基于Native采样记录的A前台证据，不能替代非零输入/真实位移。构建与运行使用新目录；测试前先询问用户完整重启隔离安全新局。

## 实现与静态审阅

- Python统一入口的离线场景和Controller，以及Rust所有注入/正常停止/失败清理helper，统一使用CREATE_NO_WINDOW，保留stdin/stdout/stderr管道。没有模拟焦点成功或改写游戏输入。
- Native PhysicalObserved/PhysicalSample的query=2表示本次诊断语义：记录实际前台PID及是否属于游戏；只读临时诊断，不进入canonical/wire，不改变projection、物理poll只采样和统一Submit/Resolve路径。旧query0记录不改写。
- Shutdown helper只写stop请求；Authority在entering且stop已请求时不启动新模拟。已进入的native更新仍走原正常return/post和TopExit publication，之后TopExit退休protocol、BeginRetirement、world::Close和RetireB，最后标记stop_handled；helper等主线程处理后再恢复原70 Hook。
- 停止时刻的竞态分为：模拟前请求（不新建tick）；已进入后请求（完成本tick再关闭）；TopExit后请求（下一个outer停止）。不修改NativeReturned/FinishPostState的真实commit事实，不跳过最后commit或放宽ACK/STATE覆盖。ABI/IPC尺寸与wire版本保持不变。
- 统一报告器新增Native前台采样检查，仍要求outage内A非零Resolve和真实位置变化；失败退出报告优先保留nativeError数值，避免仅写generic capture failure。

本轮目标编号为tx-02-02。构建目录`out/cs-t0/tx-02-02-build`，唯一验收预定`artifacts/cs-t0/tx-02-02-e2e-01`。用户已在本会话明确确认完整重启隔离安全新局与WASD/前台准备。Rust纯编译已通过，Native/Helper/Scenario纯编译中；未执行产物。物理输入窗口内的ready及工具回执改由现有Node文件API原子写入，不再为写marker启动额外shell，减少输入期间的外部进程焦点干扰。

待最终纯编译成功后只运行下面这一次入口（失败不自动重跑）：

```powershell
python cs-t0/accept-protocol-e2e.py --transaction tx-02-02 --build-dir out/cs-t0/tx-02-02-build --artifact-dir artifacts/cs-t0/tx-02-02-e2e-01 --prior-failure-artifact artifacts/cs-t0/tx-02-00-e2e-03 --authorization "User requested fixing A input and stop publication, then E2E; fresh isolated safe WASD game confirmed in this chat."
```

状态：全部纯编译完成，Rust/Native/Helper/Scenario成功、构建退出0，准备本次唯一验收；此时尚未运行测试。构建日志与build.json保存在上述目录。
