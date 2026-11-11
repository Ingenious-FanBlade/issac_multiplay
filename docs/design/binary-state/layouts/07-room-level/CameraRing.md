# CameraRing

由唯一布局源生成。布局编号 251；布局版本 60；原生大小 0x10；未知 0B；完整：是。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 Vector* data【本地】本地环形缓冲管理；同步决定：不上线；focus projection仅在实际count>0且边界合法时取最后有效Vector（4B；已核） | 0x4 i32 capacity【本地】本地环形缓冲管理；同步决定：不上线；focus projection仅在实际count>0且边界合法时取最后有效Vector（4B；已核） | 0x8 i32 count【本地】本地环形缓冲管理；同步决定：不上线；focus projection仅在实际count>0且边界合法时取最后有效Vector（4B；已核） | 0xC i32 head【本地】本地环形缓冲管理；同步决定：不上线；focus projection仅在实际count>0且边界合法时取最后有效Vector（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `data`：91EF60/91EFE0；91C730；91E920
- `capacity`：91EF60/91EFE0；91C730；91E920
- `count`：91EF60/91EFE0；91C730；91E920
- `head`：91EF60/91EFE0；91C730；91E920
