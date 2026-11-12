# SelectedRayGroup

由唯一布局源生成。布局编号 291；布局版本 71；原生大小 0x0；未知 0B；完整：否。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—40991B，不含第 9 步记录头）：

块 0：SelectedRayGroup当前结果（world，1—40991B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：仅实际选择的非空ray组；合法无组为0，坏索引／空组为Gap | +0x1 RayGroup（见该类型状态表）（伴随源，无根对象偏移）（仅selectedraygroup_present_present(present)成立时存在） group：实际选中geometry与有序rays；phase缓存和资源同批 |
|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `group` → `RayGroup`：实际选中geometry与有序rays；phase缓存和资源同批；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
