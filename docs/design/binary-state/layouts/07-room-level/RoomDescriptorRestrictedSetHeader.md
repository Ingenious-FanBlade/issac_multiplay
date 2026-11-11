# RoomDescriptorRestrictedSetHeader

由唯一布局源生成。布局编号 275；布局版本 60；原生大小 0x8；未知 0B；完整：是。

真实8B头，生成限制set不进入本轮下行；不据此推定全部节点ABI已独立核定。 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 native pointer head【本地】红黑树head，ctor分配14h节点；同步决定：不发地址/树头（4B；已核） | 0x4 u32 size【权威】限制集合元素数；同步决定：不发数量/元素（4B；已核） |
|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `head`：6EBB10；6EBF40
- `size`：6EBB10；733010；LuaRoomDescriptor.cpp遍历set
