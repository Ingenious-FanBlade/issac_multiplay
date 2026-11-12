# DisplayTextRun

由唯一布局源生成。布局编号 422；布局版本 71；原生大小 0x0；未知 0B；完整：是。

设计编号及预算，不推导为原生读取／运行adapter已实现。 原生wchar_t为J273的UTF-16；读取上限与合法窗口由来源owner负责，转码拒绝未配对代理项、控制码及超预算，不替换或截断。EntityText和其他原始字节串不应用此规则。

记录编码预算≤2073B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—2073B，不含第 9 步记录头）：

块 0：DisplayTextRun有界显示描述（world，1—2073B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：0为Host确认无此文字槽，仅1B；1允许实际空串；未知来源不能编码为0。 | +0x1 u8（C/S自有元数据，无原生偏移）（仅displaytextrun_present_is_one(present)成立时存在） font：自有字体profile：0 Latin、1 Japanese、2 Korean、3 SimplifiedChinese、4 TraditionalChinese、5 Cyrillic；固定本地字体映射缺失报Gap，非原生Font编号。 | 顺序变长偏移 u8（C/S自有元数据，无原生偏移）（仅displaytextrun_present_is_one(present)成立时存在） style：自有样式：0 BannerMain、1 BannerSubtitle、2 CurseMain、3 CurseSubtitle、4 Fortune、5 PromptTitle、6 PromptBody；特殊原生首码须先按具名映射转成允许样式，未知不得原样当指令或丢弃。 | 顺序变长偏移 f32[4]（C/S自有元数据，无原生偏移）（仅displaytextrun_present_is_one(present)成立时存在） color：实际必要KColor的RGBA，保留有限浮点位模式；不是Host缓存像素。 | 顺序变长偏移 u32（C/S自有元数据，无原生偏移）（仅displaytextrun_present_is_one(present)成立时存在） color_space：同实际KColor的颜色空间标记；不把全部文字颜色强制转成线性。 | 顺序变长偏移 u16 count + count×u8（上限2048）（C/S自有元数据，无原生偏移）（仅displaytextrun_present_is_one(present)成立时存在） utf8：严格UTF-8字节串，u16字节数，最多2048B；无NUL、C0/C1控制码、代理项、过长编码或尾部终止符；保留实际Host文字，不重查翻译。 |
|---|---|---|---|---|---|

证据：


