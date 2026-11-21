# ImageStrongPair

由唯一布局源生成。布局编号 162；布局版本 90；原生大小 0x8；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 Image* image【权威】实际选中图像由第8步资源身份承接（4B；已核） | 0x4 Control* strong_control【本地】本地引用计数／析构控制块（4B；已核） |
|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `image`：mod/src/game/client/player/client_player_draw_native.cpp
- `strong_control`：mod/src/game/client/player/client_player_draw_native.cpp
