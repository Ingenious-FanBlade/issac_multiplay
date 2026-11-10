# SurfaceRecipeQuad

由唯一布局源生成。布局编号 309；布局版本 58；原生大小 0x0；未知 0B；完整：是。

196B固定Quad值。shader/blend/sampler的具体已核枚举及上传leaf ABI仍归第9/13步，不把数值布局当成已实现GPU adapter。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 196—196B，不含第 9 步记录头）：

块 0：SurfaceRecipeQuad当前结果（world，196—196B）：

| +0x0 u8（C/S自有元数据，无原生偏移） op：1 Quad | +0x1 u16（C/S自有元数据，无原生偏移） image：实际image-table依赖 | +0x3 u16（C/S自有元数据，无原生偏移） shader：固定ColorOffset shader资源依赖 | +0x5 u8（C/S自有元数据，无原生偏移） blend_view：固定登记blend view，不接受任意GL值 | +0x6 u8（C/S自有元数据，无原生偏移） sampler_view：固定登记sampler view | +0x7 u8（C/S自有元数据，无原生偏移） source_space：0 pixel/1 UV | +0x8 f32[8]（C/S自有元数据，无原生偏移） source_points：SourceQuad源四点，保留坐标空间 | +0x28 f32[8]（C/S自有元数据，无原生偏移） destination_points：surface局部目标四点 | +0x48 f32[4]（C/S自有元数据，无原生偏移） vertex_color0：KColor0 RGBA | +0x58 u32（C/S自有元数据，无原生偏移） vertex_space0：KColor0标签 | +0x5C f32[4]（C/S自有元数据，无原生偏移） vertex_color1：KColor1 RGBA | +0x6C u32（C/S自有元数据，无原生偏移） vertex_space1：KColor1标签 | +0x70 f32[4]（C/S自有元数据，无原生偏移） vertex_color2：KColor2 RGBA | +0x80 u32（C/S自有元数据，无原生偏移） vertex_space2：KColor2标签 | +0x84 f32[4]（C/S自有元数据，无原生偏移） vertex_color3：KColor3 RGBA | +0x94 u32（C/S自有元数据，无原生偏移） vertex_space3：KColor3标签 | +0x98 f32[11]（C/S自有元数据，无原生偏移） color_mod：完整ColorMod |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：


