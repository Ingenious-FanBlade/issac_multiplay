# PickupPreviewItem

由唯一布局源生成。布局编号 360；布局版本 58；原生大小 0x0；未知 0B；完整：是。

记录编码预算≤6B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 6—6B，不含第 9 步记录头）：

块 0：PickupPreviewItem具名当前结果（world，6—6B）：

| +0x0 i32（C/S自有元数据，无原生偏移） visual_id：真实有序预览visual_id；不从Loot seed／权重重抽。 | +0x4 u16（C/S自有元数据，无原生偏移） image：本次实际预览图像；程序内容缺失报Gap。 |
|---|---|

证据：


