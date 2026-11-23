# DrawRecordNode

由唯一布局源生成。布局编号 165；布局版本 94；原生大小 0x28；未知 8B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 Node*[2] links【本地】next／prev仅维护本机链（8B；已核） | 0x8 f32 position[0].x【状态】当前可绘节点位置；未选节点不读负载：X 分量（4B；已核） | 0xC f32 position[0].y【状态】当前可绘节点位置；未选节点不读负载：Y 分量（4B；已核） | 0x10 bytes[8] unknown_10_17【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（8B；未知） | 0x18 i32[2] main_animation_frame【换算】animation负时整节点无绘制，不读其余负载（8B；已核） | 0x20 i32[2] second_animation_frame【换算】第二animation负时规范-1／0，不读无效frame（8B；已核） |
|---|---|---|---|---|---|

状态二进制布局（总计 24—24B，不含第 9 步记录头）：

块 0：展示值（world，24—24B）：

| +0x0 Vector position：当前可绘节点位置；未选节点不读负载 | +0x8 i32[2] main_animation_frame：animation负时整节点无绘制，不读其余负载 | +0x10 i32[2] second_animation_frame：第二animation负时规范-1／0，不读无效frame |
|---|---|---|

证据：

- `links`：5-player-p6-p8.md
- `position[0].x`：5-player-p6-p8.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `position[0].y`：5-player-p6-p8.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `unknown_10_17`：5-player-p6-p8.md
- `main_animation_frame`：5-player-p6-p8.md
- `second_animation_frame`：5-player-p6-p8.md
