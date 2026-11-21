# tx-03-02：T0-09-B 统一验收失败 07

日期：2026-10-20（Asia/Shanghai）。事实记录；完整 artifact 原样保留，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-07 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

后台 job 一次执行；两 driver 正常做到 tick 10000，controller A/B 均 exit 0，relay exit 0。

## 结果

analyzer **38/41 PASS**。失败项：

1. `orchestrator.offline-pass`：orchestrator 在 A controller exit 0 后、尚未处理其
   STOP_COMPLETE/RETIRE_COMPLETE 时提前判失败：
   `endpoint controller exited before retirement: exit code: 0`。
2. `control.sequence-ack`：control log 的 `direction="closed"` 审计行被 analyzer 错当成
   sequenced T9C1 envelope。
3. `control.stop-retire-both`：A 的 STOP/RETIRE 未到达 orchestrator；A controller 本地日志
   显示已发送，随后进程快速退出，Windows TCP 表现为 10054 reset，可能丢弃接收端未读数据。

其余 38 项通过，包括完整 10,000 ACK/STATE、relay raw Counter、Wire overwrite=0、A/B
endpoint result PASS、B heartbeat 驱动输入、edge exactly-once 和 stop/retire（B 侧）。

artifact：`artifacts/cs-t0/tx-03-02-e2e-07/`。

## 根因与修正

1. controller 在写完最终 T9C1 行后直接结束进程，未显式 `shutdown(Write)`，快速进程退出在
   Windows 上可能造成 10054 并丢失对端尚未 drain 的最后消息。修正：flush 后
   `shutdown(Shutdown::Write)`，保留 100 ms settle 时间，再写本地 result/退出。
2. orchestrator 把 controller 的 exit 0 立即当失败。修正：对 exit 0 设置 5 秒有界消息
   settle 窗口，继续 drain reader channel；只有未退休或非零退出才失败。role 已 Retired 时
   正常连接关闭继续忽略。
3. analyzer 忽略 `direction="closed"` 的审计行，不再把它计入 T9C1 seq/ack 流。

该次是完整功能运行失败，不是环境中断。修正后需使用新的 create-new artifact 根再次执行；
原目录保持 FAIL，不拼接、不改写。
