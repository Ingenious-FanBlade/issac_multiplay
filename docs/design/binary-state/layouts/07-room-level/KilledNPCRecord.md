# KilledNPCRecord

由唯一布局源生成。布局编号 262；布局版本 57；原生大小 0x18；未知 3B；完整：否。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 f32 position.x【权威】死亡记录位置；同步决定：历史留Host：X 分量（4B；已核） | 0x4 f32 position.y【权威】死亡记录位置；同步决定：历史留Host：Y 分量（4B；已核） | 0x8 i32 type【权威】记录NPC类型，部分类型归一；同步决定：历史留Host（4B；已核） | 0xC i16 variant【权威】记录variant窄值；同步决定：历史留Host（2B；已核） | 0xE i16 subtype【权威】记录subtype窄值；同步决定：历史留Host（2B；已核） | 0x10 u32 seed【权威】Entity+3EC的记录seed；同步决定：不下发历史／重生输入（4B；已核） | 0x14 bool friendly【权威】flags的友好状态投影；同步决定：历史留Host；当前实体另有后态（1B；已核） | 0x15 bytes[3] unknown_15_17【未知】尚未解释；不能凭对齐、清零或检索未命中认作填充；同步决定：不读不发；窄字段读写跳过该范围，保留未决（3B；未知） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `position.x`：815FB0；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `position.y`：815FB0；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `type`：815FB0
- `variant`：815FB0
- `subtype`：815FB0
- `seed`：815FB0
- `friendly`：815FB0
- `unknown_15_17`：815FB0构建的24B栈payload高3B尚无独立来源，不能补零读取
