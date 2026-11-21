# tx-03-03：Steam 脱壳新版实验 F0 失败 05

日期：2026-10-21（Asia/Shanghai）。事实记录；artifact 原样保留，不自动重跑。

> tx-03-04 静态复核补充（2026-10-21）：以下历史文本把笼统 `Error::Hook=3`
> 直接解释为 exact site 检查拒绝，超出了当时产物可支持的结论。
> `StartEmbeddedObserver` 的 mapping、image、conflict 及 patch transaction 失败都会被上层
> 归为同一个 `Error::Hook`；该次产物没有保存原始返回码及具体拒绝阶段。
> 当前文件的主循环、C/S 动作/Join、虚表与 world 入口静态检查尚未发现点位漂移。
> 因此 live-05 的具体根因仍为 `UNKNOWN`；下文保留历史判断及失败事实，不据此放松检查。
> 本轮新的全量证据和诊断修正见 [tx-03-04](../tx-03-04-hook-calibration.md)。

## 实际调用

用户对两端 Steam 脱壳后确认第五次实验：

```powershell
python cs-t0/accept-t09-b-live.py `
  --artifact-dir artifacts/cs-t0/tx-03-03-live-05 `
  --authorization "user confirmed both games Steam-unpacked and fifth live T0-09-B F0; ..." `
  --vm-share "D:\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --vm-guest-share "\\tsclient\D\projects\isaac_hook_reserach\cs-t0\vm_share" `
  --await-b-seconds 900
```

## 实际 host image

第五次 host 游戏已变为：

- size `9176064`
- loaded base `0x00DB0000`
- module memory size `0x008FE000`
- in-memory: 5 sections、timestamp `0x69E6E3A7`、ImageBase=loaded base、
  Entry `0x006EFC46`、SizeOfImage `0x008FE000`

这符合既有 `Profile::ApprovedUnpacked`；之前的新 build relocated profiles 或
`RelocatedCanonicalExperimental` 不再参与本次匹配。Image check 已通过。

## 实际失败

A `CoopActiveInitialize` 返回8。controller 提前退出，VM start script **未运行**。

只读打开仍存活的 SharedV7 mapping 后确认：

```text
state=5
error=3
restored=0
produced=6
```

`coop_active::Error=3` 是 **Hook**。事件证据保存在：

`artifacts/cs-t0/tx-03-03-live-05/host/native-init-events-readonly.json`

其中：

- kind31 AuthoritySessionTransition：Dormant→Negotiating→Warmup；
- kind36 InputRuntimeConfig；
- kind30 CharacterSelection；
- kind16 Failure detail=3（Hook）。

`StartEmbeddedObserver` 的 exact hook/site 准备检查拒绝了该 Steam 脱壳镜像。Image metadata
相似不等于 hook target bytes/RVA/ABI 一致；不能为继续测试而跳过这些检查。

## 当前状态

- 未进入 READY/ARMED/RUN_START。
- 未形成 10,000 tick/ACK/STATE/STOP/RETIRE。
- 未运行 VM script、VM 未被本次注入。
- Host 游戏已 resident 加载 Native DLL；下一次任何尝试前必须正常重启 host 游戏。
- 无 T09 controller/relay 进程残留。
- 没有 T0-09-B/P1 PASS。

## 继续条件

Steam 脱壳产物仍不是既有 approved unpacked hook baseline。可选：

1. 使用与项目原 approved unpacked image **字节/目标 site 一致**的旧文件；
2. 或把当前脱壳 build 作为新的 T0-01/T0-05 baseline 重新批准：
   - 重新定位所有 hook target、RVA、原始字节、ABI、对象布局、线程与恢复策略；
   - 这是新的实现/研究范围，不是 T0-09-B 内的绕过或 smoke；
   - 未完成前真实 P1 保持 NOT_RUN。

不得通过关闭 hook target 校验或猜测 RVA 继续写 world。
