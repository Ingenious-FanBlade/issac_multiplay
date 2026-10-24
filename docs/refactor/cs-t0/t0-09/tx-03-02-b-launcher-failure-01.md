# tx-03-02：T0-09-B 统一入口启动失败 01

日期：2026-10-20（Asia/Shanghai）。事实记录，不重判为测试 PASS，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-01 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

## 原始结果

入口在 Python import 阶段退出 1：

```text
ModuleNotFoundError: No module named 'report_t09_b'
```

- 未执行 build-t09-b.ps1。
- 未启动 T09 runner、relay、endpoint controller 或 driver。
- 未创建 `artifacts/cs-t0/tx-03-02-e2e-01` 或对应 work 目录。
- 未运行游戏、未注入、未操作 Original Online/Isaac Sync。
- 因而本次不是功能验收 FAIL，也没有可分析运行 artifact；它是入口自身的模块解析缺陷。

## 原因与修正

`accept-t09-b.py` 在定义脚本目录之前直接 `import report_t09_b`；当前 Python 启动环境未把
`cs-t0/` 放入 `sys.path`。修正为在计算 `TOOLS` 后显式 `sys.path.insert(0, str(TOOLS))`，
再做同目录模块导入。修正属于验收入口的实质修复；后续调用需作为新的启动事实记录，不能
把本记录改写成成功，也不能把修好后的结果回填到本次失败。
