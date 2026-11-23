# HistoryHUDPlayer

由唯一布局源生成。布局编号 321；布局版本 94；原生大小 0x14；未知 4B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：实际身体History列 | subject：body key/use/shape＋具体HUD实例用途 | producer：82FA10正常完成的实际有序列表 | adapter：history_h_u_d_player_fixed_adapter | dependencies：所属组选择、身体key/use/shape、必要P0—P6绘制拓扑和图像／动画资源；不依赖全地图。 | dependency_types：EntityPlayer | sequence：body | atomic_scope：body-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：必须取得所有控制组的合法来源，不能仅采Host当前两槽／八内嵌槽，不调用Update／Recompute补值。 | envelope：{'mode': 'group', 'subject_type': 'EntityPlayer', 'cacheable': True, 'clock': False, 'topology': True, 'paired_streams': 0, 'passes': ['Context', 'Value'], 'subject_field': 'player'} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤4194304B；body组≤65536B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 0x0 Entity_Player* player【引用】这列最终列表的实际身体（4B；已核） | 0x4 bytes[4] unknown_4_7【未知】未取得字段用途或填充证明（4B；未知） | 0x8 vector<HistoryHUDItem> items【容器】完整有序图像列表（12B；已核） |
|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：HistoryHUDPlayer具名当前结果（body，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `player`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `unknown_4_7`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `items`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
