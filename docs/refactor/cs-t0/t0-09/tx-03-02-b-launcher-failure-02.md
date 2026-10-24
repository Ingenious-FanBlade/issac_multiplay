# tx-03-02：T0-09-B 统一入口启动失败 02

日期：2026-10-20（Asia/Shanghai）。事实记录，不重判为测试 PASS，不自动重跑。

## 实际调用

在修复 01 的 `sys.path` 插入后，使用新的 create-new artifact 根：

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-02 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

## 原始结果

入口仍在 Python import 阶段退出 1：

```text
ModuleNotFoundError: No module named 'report_t09_b'
```

根因是分析器文件名使用短横线 `report-t09-b.py`，而 import 名称使用下划线
`report_t09_b`；Python 普通 import 不接受该文件名。`sys.path` 修复本身没有错。

- 未执行 build-t09-b.ps1。
- 未启动 T09 runner、relay、endpoint controller 或 driver。
- 未创建 `artifacts/cs-t0/tx-03-02-e2e-02` 或对应 work 目录。
- 未运行游戏、未注入、未操作 Original Online/Isaac Sync。

## 修正

将分析器文件改为可导入的 `report_t09_b.py`，同步修改统一入口 source snapshot 与实现文档。
修正后仅做静态导入检查；实际统一验收必须使用新的 create-new artifact 根并单独记录结果。
