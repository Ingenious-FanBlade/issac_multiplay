# HudBodySelection

由唯一布局源生成。布局编号 363；布局版本 71；原生大小 0x0；未知 0B；完整：是。

记录编码预算≤4B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 4—4B，不含第 9 步记录头）：

块 0：HudBodySelection具名当前结果（world，4—4B）：

| +0x0 u16（C/S自有元数据，无原生偏移） body：当前显示身体成员引用；use/shape由已提交成员上下文确认。 | +0x2 u8（C/S自有元数据，无原生偏移） roles：bit0公共HUD、bit1本组详细、bit2History、bit3Stat、bit4必要关联健康显示；其余为0。 | +0x3 u8（C/S自有元数据，无原生偏移） display_order：组内已确定顺序，不等同roster index或Peer ordinal。 |
|---|---|---|

证据：


