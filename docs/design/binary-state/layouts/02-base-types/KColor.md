# KColor

由唯一布局源生成。布局编号 3；布局版本 60；原生大小 0x14；未知 0B；完整：是。

KColor+0x10已由J273颜色空间转换核为u32标记，不是padding或无语义opaque32；0=RGB、1=sRGB、2=HSL已直接观察，未将三标签当作穷举合法值集合。四个f32分量按颜色空间解释，第四分量为Alpha。 当前原生安装仅接受已核标签0/1/2；其他标签应报未支持缺口并补证，不据此宣称它们在全程序中非法。

原生字节归属：

| 0x0 f32 red【状态】红色分量（4B；已核） | 0x4 f32 green【状态】绿色分量（4B；已核） | 0x8 f32 blue【状态】蓝色分量（4B；已核） | 0xC f32 alpha【状态】透明度分量（4B；已核） | 0x10 u32 color_space【状态】颜色空间标记：已核0=RGB、1=sRGB、2=HSL；不将当前观察标签冒充穷举集合（4B；已核） |
|---|---|---|---|---|

状态二进制布局（总计 20—20B，不含第 9 步记录头）：

块 0：颜色分量与颜色空间（body，20—20B）：

| +0x0 f32 red：红色分量 | +0x4 f32 green：绿色分量 | +0x8 f32 blue：蓝色分量 | +0xC f32 alpha：透明度分量 | +0x10 u32 color_space：颜色空间标记：已核0=RGB、1=sRGB、2=HSL；不将当前观察标签冒充穷举集合 |
|---|---|---|---|---|

证据：

- `red`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\KColor.zhl；docs/design/tx-04-19/entity-application.json 中 KColor RGBA+opaque32；J273 VA0x9E39F0转换前三个f32并复制+0xC Alpha，+0x10颜色空间另行处理
- `green`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\KColor.zhl；docs/design/tx-04-19/entity-application.json 中 KColor RGBA+opaque32；J273 VA0x9E39F0转换前三个f32并复制+0xC Alpha，+0x10颜色空间另行处理
- `blue`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\KColor.zhl；docs/design/tx-04-19/entity-application.json 中 KColor RGBA+opaque32；J273 VA0x9E39F0转换前三个f32并复制+0xC Alpha，+0x10颜色空间另行处理
- `alpha`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\KColor.zhl；docs/design/tx-04-19/entity-application.json 中 KColor RGBA+opaque32；J273 VA0x9E39F0转换前三个f32并复制+0xC Alpha，+0x10颜色空间另行处理
- `color_space`：J273 VA0x9E39F0：+0x9E3A01检查输入space=1，+0x9E3A37写输出0；VA0x9E37B0检查RGB0，VA0x9E3BA0检查HSL2/输出0；imagebase 0x400000；主审已复读；VA0x9E3DF0成功转换分支在VA0x9E3EB0写结果标签1；分派表VA0xAB1E00有三组三项；其他标签未作穷举排除
