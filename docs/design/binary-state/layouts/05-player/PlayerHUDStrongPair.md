# PlayerHUDStrongPair

由唯一布局源生成。布局编号 163；布局版本 58；原生大小 0x8；未知 0B；完整：是。

原生字节归属：

| 0x0 PlayerHUD* hud【本地】本地动态HUD承载，状态在第8步（4B；已核） | 0x4 Control* control【本地】本地control vtable／引用计数（4B；已核） |
|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `hud`：mod/src/game/client/player/client_player_hud_native.cpp
- `control`：mod/src/game/client/player/client_player_hud_native.cpp
