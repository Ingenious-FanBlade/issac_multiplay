# tx-04-03 静态核对的脚本与中间数据

结论见 [docs/hooks/repentogon-binding.md](../../../docs/hooks/repentogon-binding.md)。这里的文件全部来自静态分析：用 IDAPython 读取两个 IDB，以及读取 REPENTOGON 的源码和运行日志。没有运行游戏，也没有计算或校验任何哈希。

## 输入

- **v1.9.7.17 IDB**：游戏根目录的 `isaac-ng.exe.i64`。
- **v1.9.7.12 IDB**：先把 `<游戏目录>\Repentogon\isaac-ng.exe` 复制一份，用 Steamless 3.1.0.5 的 CLI 脱壳，再用 IDA 打开脱壳产物。
- **REPENTOGON 运行日志**：`<游戏目录>\Repentogon\repentogon.log`，2026-10-24 00:40，模块基址 `0x00BA0000`。
- **REPENTOGON 源码**：`D:\projects\REPENTOGON`，即 `libzhl/functions/*.zhl` 和 `repentogon/**/*.cpp`。

## 运行顺序

脚本通过 ida-mcp 的 `py_exec_file` 执行。脚本里的 `SP` 路径写死成了当时会话的 scratchpad，重跑前先改成这个目录。

1. 在 v1.9.7.17 IDB 上运行：`extract_v17.py`，生成 `v17_fingerprints.json`。
2. 在 v1.9.7.12 IDB 上依次运行：
   - `stage1_patterns.py`、`stage2_graph.py`：函数匹配，结果写入 `v12_state.json`；
   - `stage3_sites.py`：调用点、返回地址和全局变量；
   - `stage4_manual.py`；
   - `stage_vt2.py`、`stage_rtti_names.py`、`stage5_device.py`、`stage6_input.py`：vtable 与输入设备；
   - `stage7_fields.py`：字段偏移；
   - `stage8_misc.py`：控制台判断与 ExecuteCommand；
   - `stage9_overlap.py`：REPENTOGON 补丁重叠检查；
   - `stage10_callers.py`：调用方统计。
3. 本地运行 `make_binding.py`，生成 `docs/hooks/repentogon-binding.json`。

## data/ 目录

| 文件 | 内容 |
| --- | --- |
| `repentogon_anchor_rva.json` | 从运行日志解析出的 1267 个 REPENTOGON 定义的 RVA |
| `repentogon_values.json` | 运行日志中的 58 个变量值（绝对地址） |
| `repentogon_asm_sigs.json` | 343 个 REPENTOGON ASM/内联特征串 |
| `v17_fingerprints.json` | v1.9.7.17 原型点位的特征 |
| `v12_*.json` | v1.9.7.12 各阶段的输出 |
