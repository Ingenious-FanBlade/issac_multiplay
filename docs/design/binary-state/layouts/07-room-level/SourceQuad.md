# SourceQuad

由唯一布局源生成。布局编号 269；布局版本 75；原生大小 0x24；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 f32 points.topLeft.x【权威】Quad recipe源四点：surface内局部四点，非Host屏幕坐标：X 分量（4B；SDK声明） | 0x4 f32 points.topLeft.y【权威】Quad recipe源四点：surface内局部四点，非Host屏幕坐标：Y 分量（4B；SDK声明） | 0x8 f32 points.topRight.x【权威】Quad recipe源四点：surface内局部四点，非Host屏幕坐标：X 分量（4B；SDK声明） | 0xC f32 points.topRight.y【权威】Quad recipe源四点：surface内局部四点，非Host屏幕坐标：Y 分量（4B；SDK声明） | 0x10 f32 points.bottomLeft.x【权威】Quad recipe源四点：surface内局部四点，非Host屏幕坐标：X 分量（4B；SDK声明） | 0x14 f32 points.bottomLeft.y【权威】Quad recipe源四点：surface内局部四点，非Host屏幕坐标：Y 分量（4B；SDK声明） | 0x18 f32 points.bottomRight.x【权威】Quad recipe源四点：surface内局部四点，非Host屏幕坐标：X 分量（4B；SDK声明） | 0x1C f32 points.bottomRight.y【权威】Quad recipe源四点：surface内局部四点，非Host屏幕坐标：Y 分量（4B；SDK声明） | 0x20 i32 coordinateSpace【权威】检查0/1后转u8 source_space；不得丢弃或混用pixel/UV（4B；SDK声明） |
|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `points.topLeft.x`：官方SourceQuad.zhl；Wiki renderer/SourceQuad；嵌套字段：官方DestinationQuad.zhl；Wiki renderer/DestinationQuad；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `points.topLeft.y`：官方SourceQuad.zhl；Wiki renderer/SourceQuad；嵌套字段：官方DestinationQuad.zhl；Wiki renderer/DestinationQuad；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `points.topRight.x`：官方SourceQuad.zhl；Wiki renderer/SourceQuad；嵌套字段：官方DestinationQuad.zhl；Wiki renderer/DestinationQuad；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `points.topRight.y`：官方SourceQuad.zhl；Wiki renderer/SourceQuad；嵌套字段：官方DestinationQuad.zhl；Wiki renderer/DestinationQuad；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `points.bottomLeft.x`：官方SourceQuad.zhl；Wiki renderer/SourceQuad；嵌套字段：官方DestinationQuad.zhl；Wiki renderer/DestinationQuad；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `points.bottomLeft.y`：官方SourceQuad.zhl；Wiki renderer/SourceQuad；嵌套字段：官方DestinationQuad.zhl；Wiki renderer/DestinationQuad；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `points.bottomRight.x`：官方SourceQuad.zhl；Wiki renderer/SourceQuad；嵌套字段：官方DestinationQuad.zhl；Wiki renderer/DestinationQuad；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `points.bottomRight.y`：官方SourceQuad.zhl；Wiki renderer/SourceQuad；嵌套字段：官方DestinationQuad.zhl；Wiki renderer/DestinationQuad；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `coordinateSpace`：官方SourceQuad.zhl；Wiki renderer/SourceQuad
