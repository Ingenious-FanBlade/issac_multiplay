# ScreenFadeContribution

由唯一布局源生成。布局编号 372；布局版本 71；原生大小 0x0；未知 0B；完整：是。

记录编码预算≤19B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 17—19B，不含第 9 步记录头）：

块 0：ScreenFadeContribution具名当前结果（world，17—19B）：

| +0x0 u8（C/S自有元数据，无原生偏移） kind：0=最终solid覆盖、1=实际加载插画；不发送退出／保存target。 | +0x1 f32[4]（C/S自有元数据，无原生偏移） rgba：本次最终RGBA；primary优先，否则secondary，不同时发送未选路径。 | +0x11 u16（C/S自有元数据，无原生偏移）（仅screen_fade_contribution_kind_kind__1(kind)成立时存在） image：92B8B0实际所选插画，中心位置本地派生。 |
|---|---|---|

证据：


