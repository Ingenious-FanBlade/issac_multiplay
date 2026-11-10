# ImageStrongPair

由唯一布局源生成。布局编号 162；布局版本 56；原生大小 0x8；未知 0B；完整：是。

原生字节归属：

| 0x0 Image* image【权威】实际选中图像由第8步资源身份承接（4B；已核） | 0x4 Control* strong_control【本地】本地引用计数／析构控制块（4B；已核） |
|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `image`：mod/src/game/client/player/client_player_draw_native.cpp
- `strong_control`：mod/src/game/client/player/client_player_draw_native.cpp
