# LocalHistoryOverlay

由唯一布局源生成。布局编号 424；布局版本 71；原生大小 0x0；未知 0B；完整：是。

设计编号及预算，不推导为原生读取／运行adapter已实现。

记录编码预算≤46B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 46—46B，不含第 9 步记录头）：

块 0：LocalHistoryOverlay有界显示描述（world，46—46B）：

| +0x0 u16（C/S自有元数据，无原生偏移） branch：0为6F7EF1振荡叠加、1为6F8425白色叠加；列表保留Host实际pass顺序。只消费Client本端Snapshot，不传Host图像／坐标／GPU指令。 | +0x2 f32[11]（C/S自有元数据，无原生偏移） color：正常消费的最终ColorMod十一值；保留两次独立贡献，不恢复Host RNG／历史像素；本地取景范围由adapter固定。 |
|---|---|

证据：


