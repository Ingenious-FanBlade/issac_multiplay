# ScreenEffectsResult

由唯一布局源生成。布局编号 373；布局版本 58；原生大小 0x0；未知 0B；完整：是。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：屏幕有序覆盖 | subject：run／显示代次 | producer：6F7A80各具名正常pass与6F8FFD／6F90B5最终选择 | adapter：screen_effects_fixed_adapter | dependencies：GameRun当前颜色／darkness／lightning／dizzy与D0唯一源；OldTV资格由RunSeeds、时间由GameFrame派生。 | dependency_types：GameRun, PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：全部纯world图像内容与坐标解释仍待落实；无完整贡献不接受目标。 |
|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤8192B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 4—6612B，不含第 9 步记录头）：

块 0：ScreenEffectsResult具名当前结果（world，4—6612B）：

| +0x0 u16 count + count×PresentationImageContribution（上限32）（C/S自有元数据，无原生偏移） snapshot_overlays：6F7EF1／6F8425等各次snapshot实际内容／颜色及顺序，不合并最后alpha。 | 顺序变长偏移 u16 count + count×ScreenFadeContribution（上限16）（C/S自有元数据，无原生偏移） fades：实际选中fade／插画与白色覆盖clamp(value/90,0,1)的有序结果；无输出须Host确认。 |
|---|---|

证据：


