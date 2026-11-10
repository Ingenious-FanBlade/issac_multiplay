# ImageBase

由唯一布局源生成。布局编号 340；布局版本 58；原生大小 0x80；未知 53B；完整：否。

**范围仅为已核前缀，不是动态对象分配大小；具体对象仍需完整布局。**

未声明ImagePNG/PIC/PCX/GL后端完整大小；不据ImageBase的80h前缀读取派生私有像素。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 void** vtable【本地】原生Image类虚表（4B；SDK声明） | 0x4 bytes[12] unknown_4_F【未知】未取得字段用途或填充证明（12B；未知） | 0x10 u64 flags【本地】文件图/程序图等资源种类；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：本地菜单／资源owner；共享world结果按具名独立记录承接（8B；SDK声明） | 0x18 bytes[8] unknown_18_1F【未知】未取得字段用途或填充证明（8B；未知） | 0x20 u32 shader【本地】Image自动选择/抑制shader的本地模式；3用于抑制，不等于ShaderType枚举（4B；已核） | 0x24 u32 min_filter【本地】实际图像采样参数；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：本地菜单／资源owner；共享world结果按具名独立记录承接（4B；SDK声明） | 0x28 u32 mag_filter【本地】实际图像采样参数；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：本地菜单／资源owner；共享world结果按具名独立记录承接（4B；SDK声明） | 0x2C u32 wrap_s【本地】实际图像采样参数；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：本地菜单／资源owner；共享world结果按具名独立记录承接（4B；SDK声明） | 0x30 u32 wrap_t【本地】实际图像采样参数；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：本地菜单／资源owner；共享world结果按具名独立记录承接（4B；SDK声明） | 0x34 u8* vertex_format【本地】本地顶点格式数组（4B；SDK声明） | 0x38 u8 attribute_count【本地】顶点格式属性数；不另发原始生产／重复负载，唯一归属：本地菜单／资源owner；共享world结果按具名独立记录承接（1B；SDK声明） | 0x39 bytes[1] unknown_39_39【未知】未取得字段用途或填充证明（1B；未知） | 0x3A u16 vertex_stride【本地】顶点宽度；不另发原始生产／重复负载，唯一归属：本地菜单／资源owner；共享world结果按具名独立记录承接（2B；SDK声明） | 0x3C char* name【本地】可能重复的图像名称；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：本地菜单／资源owner；共享world结果按具名独立记录承接（4B；SDK声明） | 0x40 vector<RenderBatch*> batches【本地】绘制批次（12B；SDK声明） | 0x4C RenderBatch* last_batch【本地】批次缓存（4B；SDK声明） | 0x50 vector<RenderBatch*> transparent_batches【本地】透明批次复用池（12B；SDK声明） | 0x5C RenderBatch* last_transparent_batch【本地】最近透明批次（4B；SDK声明） | 0x60 bytes[32] unknown_60_7F【未知】未取得字段用途或填充证明（32B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `vtable`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `unknown_4_F`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `flags`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `unknown_18_1F`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `shader`：官方ImageUtils.hpp::SubmitQuadForShader；KAGE_Graphics_Image.zhl；67DA3C→AACAB0
- `min_filter`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `mag_filter`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `wrap_s`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `wrap_t`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `vertex_format`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `attribute_count`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `unknown_39_39`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `vertex_stride`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `name`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `batches`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `last_batch`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `transparent_batches`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `last_transparent_batch`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
- `unknown_60_7F`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage
