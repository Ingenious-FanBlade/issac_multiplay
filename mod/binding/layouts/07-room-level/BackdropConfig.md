# BackdropConfig

由唯一布局源生成。布局编号 241；布局版本 90；原生大小 0x13C；未知 5B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 u32 id【权威】本地已加载配置／构建参数；同步决定：稳定资源配置；不用这些参数在Client随机选纹理（4B；已核） | 0x4 std::string gfx【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0x1C std::string water_gfx【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0x34 std::string l_floor【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0x4C std::string n_floor【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0x64 u32 walls【权威】本地已加载配置／构建参数；同步决定：稳定资源配置；不用这些参数在Client随机选纹理（4B；已核） | 0x68 u32 wall_variants【权威】本地已加载配置／构建参数；同步决定：稳定资源配置；不用这些参数在Client随机选纹理（4B；已核） | 0x6C u32 floors【权威】本地已加载配置／构建参数；同步决定：稳定资源配置；不用这些参数在Client随机选纹理（4B；已核） | 0x70 u32 floor_variants【权威】本地已加载配置／构建参数；同步决定：稳定资源配置；不用这些参数在Client随机选纹理（4B；已核） | 0x74 bool full_image【权威】实际XML fullimage构建模式，非u32 unk1；同步决定：结果纹理路线不额外发构建模式；若使用recipe，须显式包含该模式，禁止读高3B（1B；已核） | 0x75 bytes[3] unknown_75_77【未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；同步决定：不读不发；窄字段读写跳过该范围，保留未决（3B；未知） | 0x78 std::string props【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0x90 std::string rocks【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0xA8 std::string pit【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0xC0 std::string water_pit【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0xD8 std::string bridge【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0xF0 std::string door【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0x108 std::string hole_in_wall【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0x120 std::string spikes【权威】配置路径；SSO／allocator本地；同步决定：实际采用资源走稳定目录；不复制字符串对象头（24B；SDK声明） | 0x138 u8 water_pits_mode【权威】实际水坑图形模式byte，SDK的u32过宽；同步决定：稳定配置／实际资源用途承接，不能打包邻接reverse为一个u32（1B；已核） | 0x139 bool reverse_water_gfx【权威】91AC70::RenderWater把GameFrame/2相位乘-1的真实绘制条件；同步决定：需要保留当前采用配置的这一个方向bool，或等价最终有符号水面相位；纹理像素本身不能代替（1B；已核） | 0x13A bytes[2] unknown_13A_13B【未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；同步决定：不读不发；窄字段读写跳过该范围，保留未决（2B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `id`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `gfx`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `water_gfx`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `l_floor`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `n_floor`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `walls`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `wall_variants`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `floors`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `floor_variants`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `full_image`：90B9C0只写+74 byte和+138 word；90C3D0解析fullimage/waterpitsmode/reversewatergfx；90E080分别复制三byte
- `unknown_75_77`：90B9C0只写+74 byte和+138 word；90C3D0解析fullimage/waterpitsmode/reversewatergfx；90E080分别复制三byte
- `props`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `rocks`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `pit`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `water_pit`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `bridge`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `door`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `hole_in_wall`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `spikes`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/BackdropConfig.zhl；917910/911310
- `water_pits_mode`：90B9C0只写+74 byte和+138 word；90C3D0解析fullimage/waterpitsmode/reversewatergfx；90E080分别复制三byte
- `reverse_water_gfx`：90B9C0只写+74 byte和+138 word；90C3D0解析fullimage/waterpitsmode/reversewatergfx；90E080分别复制三byte；91ACEB..91AD05读取Backdrop+id*13C+28D
- `unknown_13A_13B`：90B9C0只写+74 byte和+138 word；90C3D0解析fullimage/waterpitsmode/reversewatergfx；90E080分别复制三byte
