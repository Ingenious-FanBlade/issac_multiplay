# tx-03-02：T0-09-B 统一验收失败 03

日期：2026-10-20（Asia/Shanghai）。事实记录；artifact 原样保留，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-03 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

## 结果

- 纯编译 exit 0；生成了 T09 runner 与 C++ endpoint scenario。
- 离线 F0 orchestrator **exit 2，未启动 relay/controller/driver 运行**。
- analyzer 也因入口脚本残留 `separate_ascii` 关键字失败，因此没有功能检查结果。
- artifact：`artifacts/cs-t0/tx-03-02-e2e-03/`；staging work 保留。
- 未启动游戏、未注入、未操作 Original Online/Isaac Sync、未重跑。

## 原始错误

`artifacts/cs-t0/tx-03-02-e2e-03/control/orchestrator.stderr.log`：

```text
IsaacCsT0T09: unknown T0-09-B command '--manifest'; use f0, endpoint, or relay
```

analyzer：

```text
JSONEncoder.__init__() got an unexpected keyword argument 'separate_ascii'
```

## 根因与修正

1. `IsaacCsT0T09` 是独立 binary，参数布局为 `exe <command> ...`；`t09::run_from_env` 错误地
   `skip(2)`，把实际 command `f0` 丢掉。修正为 `skip(1)`。
2. `report_t09_b.py` 尚有一处旧的 `separate_ascii` 参数；移除，只保留 `ensure_ascii=False`。

修正仅做纯编译和静态语法检查；后续统一验收必须使用新的 create-new artifact 根并单独记录，
不把本次结果回填为 PASS。
