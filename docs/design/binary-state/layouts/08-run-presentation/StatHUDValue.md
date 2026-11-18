# StatHUDValue

由唯一布局源生成。布局编号 316；布局版本 84；原生大小 0x14；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

记录编码预算≤4194304B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 0x0 f32 value【状态】当前显示值（4B；已核） | 0x4 f32 delta【状态】差值文字（4B；已核） | 0x8 f32 previous【权威】更新时计算差值的历史值（4B；已核） | 0xC f32 threshold【权威】更新时是否触发提示的阈值（4B；已核） | 0x10 i32 countdown【状态】差值文字alpha／位移的当前相位（4B；已核） |
|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：StatHUDValue具名当前结果（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
