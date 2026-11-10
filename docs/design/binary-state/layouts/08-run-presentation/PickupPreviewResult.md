# PickupPreviewResult

由唯一布局源生成。布局编号 361；布局版本 58；原生大小 0x0；未知 0B；完整：是。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：Pickup预览显示 | subject：pickup | producer：正常Guppy Eye／已核预览消费点 | adapter：pickup_preview_fixed_adapter | dependencies：当前Pickup key/use/shape、具体pass／贡献及状态／图像资源修订。 | dependency_types：EntityPickup | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：全部预览正常来源覆盖仍需第10步核定；256仅保护上限，超限报Gap不截断。 |
|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤4096B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—1539B，不含第 9 步记录头）：

块 0：PickupPreviewResult具名当前结果（world，1—1539B）：

| +0x0 u8（C/S自有元数据，无原生偏移） visible：Host已确认预览资格；隐藏不读取旧列表。 | +0x1 u16 count + count×PickupPreviewItem（上限256）（C/S自有元数据，无原生偏移）（仅pickup_preview_result_visible_visible__1(visible)成立时存在） items：真实已产生的完整有序预览；不调用GetLootList(false)，不解引用Loot RNG*。 |
|---|---|

证据：


