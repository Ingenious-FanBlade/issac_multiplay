# FortuneText

由唯一布局源生成。布局编号 423；布局版本 71；原生大小 0x0；未知 0B；完整：是。

设计编号及预算，不推导为原生读取／运行adapter已实现。

记录编码预算≤16586B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—16586B，不含第 9 步记录头）：

块 0：FortuneText有界显示描述（world，1—16586B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：当前fortune文字是否有效；0仅1B且不读旧行；1时保留实际有序行，包括有效空行。 | +0x1 u8 count + count×DisplayTextRun（上限8）（C/S自有元数据，无原生偏移）（仅fortunetext_present_is_one(present)成立时存在） lines：最多8行有序文字；不排序、不合并；总行数／文字／HUD及帧预算同时适用。 |
|---|---|

证据：


