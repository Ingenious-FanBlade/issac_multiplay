# StageTransition

由唯一布局源生成。布局编号 342；布局版本 60；原生大小 0x20；未知 16B；完整：否。

 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 u32 mode【权威】2淡入/3淡出等分支（4B；已核） | 0x4 u32[2] progress_inputs【权威】Render unsigned current/duration（8B；已核） | 0xC bytes[4] unknown_C_F【未知】未取得字段用途或填充证明（4B；未知） | 0x10 i32 render_kind【权威】909E60按DWORD==5决定白色；SDK只声明_sameStage bool（4B；已核） | 0x14 bytes[12] unknown_14_1F【未知】未取得字段用途或填充证明（12B；未知） |
|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `mode`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/StageTransition.zhl；J273 909E60
- `progress_inputs`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/StageTransition.zhl；J273 909E60
- `unknown_C_F`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/StageTransition.zhl；J273 909E60
- `render_kind`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/StageTransition.zhl；J273 909E60
- `unknown_14_1F`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/StageTransition.zhl；J273 909E60
