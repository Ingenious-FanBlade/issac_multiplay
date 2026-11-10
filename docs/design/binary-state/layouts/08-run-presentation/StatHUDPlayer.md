# StatHUDPlayer

由唯一布局源生成。布局编号 317；布局版本 58；原生大小 0xCC；未知 0B；完整：是。

 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。 真实物理10槽全部展开；线上固定0..7，官方星象房第8行按Host最终资格存在，不发第9行和可伪装空列表的count。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：实际身体Stat列 | subject：body key/use/shape＋具体HUD实例用途 | producer：83CFD0／83D210正常完成的0..8行最终三值及资格 | adapter：stat_h_u_d_player_fixed_adapter | dependencies：所属组选择、身体key/use/shape、必要P0—P6绘制拓扑和图像／动画资源；不依赖全地图。 | dependency_types：EntityPlayer | sequence：body | atomic_scope：body-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：必须取得所有控制组的合法来源，不能仅采Host当前两槽／八内嵌槽，不调用Update／Recompute补值。 |
|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤4194304B；body组≤65536B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 0x0 Entity_Player* player【引用】实际显示身体（4B；已核） | 0x4 f32 values[0].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0x8 f32 values[0].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0xC f32 values[0].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0x10 f32 values[0].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0x14 i32 values[0].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0x18 f32 values[1].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0x1C f32 values[1].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0x20 f32 values[1].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0x24 f32 values[1].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0x28 i32 values[1].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0x2C f32 values[2].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0x30 f32 values[2].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0x34 f32 values[2].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0x38 f32 values[2].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0x3C i32 values[2].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0x40 f32 values[3].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0x44 f32 values[3].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0x48 f32 values[3].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0x4C f32 values[3].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0x50 i32 values[3].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0x54 f32 values[4].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0x58 f32 values[4].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0x5C f32 values[4].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0x60 f32 values[4].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0x64 i32 values[4].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0x68 f32 values[5].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0x6C f32 values[5].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0x70 f32 values[5].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0x74 f32 values[5].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0x78 i32 values[5].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0x7C f32 values[6].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0x80 f32 values[6].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0x84 f32 values[6].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0x88 f32 values[6].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0x8C i32 values[6].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0x90 f32 values[7].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0x94 f32 values[7].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0x98 f32 values[7].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0x9C f32 values[7].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0xA0 i32 values[7].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0xA4 f32 values[8].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0xA8 f32 values[8].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0xAC f32 values[8].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0xB0 f32 values[8].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0xB4 i32 values[8].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） | 0xB8 f32 values[9].value【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：当前显示值（4B；已核） | 0xBC f32 values[9].delta【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字（4B；已核） | 0xC0 f32 values[9].previous【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时计算差值的历史值（4B；已核） | 0xC4 f32 values[9].threshold【权威】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：更新时是否触发提示的阈值（4B；已核） | 0xC8 i32 values[9].countdown【状态】十个物理槽位；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：StatHUDRows固定0..7及资格有效时第8行；物理第9行无消费者，不发送：差值文字alpha／位移的当前相位（4B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 99—111B，不含第 9 步记录头）：

块 0：StatHUDPlayer具名当前结果（body，99—111B）：

| +0x0 u16 player：实际显示身体 | +0x2 StatHUDRows（见该类型状态表）（伴随源，无根对象偏移） rows：固定已核行；禁止缺行填0，资格不成立时不读第8行残留。 |
|---|---|

证据：

- `player`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[0].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[0].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[0].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[0].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[0].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[1].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[1].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[1].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[1].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[1].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[2].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[2].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[2].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[2].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[2].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[3].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[3].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[3].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[3].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[3].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[4].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[4].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[4].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[4].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[4].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[5].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[5].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[5].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[5].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[5].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[6].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[6].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[6].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[6].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[6].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[7].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[7].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[7].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[7].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[7].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[8].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[8].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[8].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[8].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[8].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[9].value`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[9].delta`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[9].previous`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[9].threshold`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁
- `values[9].countdown`：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁；嵌套字段：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁

伴随来源（不属于上述原生对象字节）：

- `rows` → `StatHUDRows`：固定已核行；禁止缺行填0，资格不成立时不读第8行残留。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
