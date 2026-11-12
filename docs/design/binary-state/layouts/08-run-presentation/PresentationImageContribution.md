# PresentationImageContribution

由唯一布局源生成。布局编号 366；布局版本 71；原生大小 0x0；未知 0B；完整：是。

 版本63补齐ColorOffset的pixelation_amount及clip_plane共16B；当前213B，原字节尺寸为历史。

记录编码预算≤213B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 213—213B，不含第 9 步记录头）：

块 0：PresentationImageContribution具名当前结果（world，213—213B）：

| +0x0 u16（C/S自有元数据，无原生偏移） branch：所属具名owner固定分支／slot编号。 | +0x2 u16（C/S自有元数据，无原生偏移） image：实际内容／完整recipe资源；不是名称／FBO。 | +0x4 u16（C/S自有元数据，无原生偏移） shader：实际已映射的内置shader资源；无法映射报Gap，不猜默认。 | +0x6 u8（C/S自有元数据，无原生偏移） blend_view：第9步固定语义表的blend view，不接受原生GL整数或任意程序。 | +0x7 u8（C/S自有元数据，无原生偏移） sampler_view：第9步固定语义表的sampler view；资源padding与过滤语义保持。 | +0x8 f32[8]（C/S自有元数据，无原生偏移） destination：四顶点逻辑UI坐标，沿用已核topLeft/topRight/bottomLeft/bottomRight顺序；不传Host窗口像素。 | +0x28 f32[8]（C/S自有元数据，无原生偏移） source：实际图像取样四角；不得丢掉坐标空间。 | +0x48 u8（C/S自有元数据，无原生偏移） source_space：0=pixel，1=UV；按已核SourceQuad空间登记。 | +0x49 f32[11]（C/S自有元数据，无原生偏移） color：完整ColorMod十一值，不能一律压成RGBA。 | +0x75 f32[4]（C/S自有元数据，无原生偏移） vertex_color_0：实际第0顶点KColor RGBA。 | +0x85 u32（C/S自有元数据，无原生偏移） vertex_space_0：该顶点KColor颜色空间标签，不强制丢弃。 | +0x89 f32[4]（C/S自有元数据，无原生偏移） vertex_color_1：实际第1顶点KColor RGBA。 | +0x99 u32（C/S自有元数据，无原生偏移） vertex_space_1：该顶点KColor颜色空间标签，不强制丢弃。 | +0x9D f32[4]（C/S自有元数据，无原生偏移） vertex_color_2：实际第2顶点KColor RGBA。 | +0xAD u32（C/S自有元数据，无原生偏移） vertex_space_2：该顶点KColor颜色空间标签，不强制丢弃。 | +0xB1 f32[4]（C/S自有元数据，无原生偏移） vertex_color_3：实际第3顶点KColor RGBA。 | +0xC1 u32（C/S自有元数据，无原生偏移） vertex_space_3：该顶点KColor颜色空间标签，不强制丢弃。 | +0xC5 f32（C/S自有元数据，无原生偏移） pixelation_amount：ColorOffset实际提交上下文；不重用未来全局值 | +0xC9 f32[3]（C/S自有元数据，无原生偏移） clip_plane：ColorOffset实际clip normal.xy及threshold |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：


