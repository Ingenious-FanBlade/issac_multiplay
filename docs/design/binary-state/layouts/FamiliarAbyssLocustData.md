# FamiliarAbyssLocustData

由唯一布局源生成。布局编号 19；布局版本 23；原生大小 0x58；未知 36B；完整：否。

原生字节归属：

| 0x0 u32 locust_vtable【本地】AbyssLocustData原生虚表；由具名owner构造（4B；已核） | 0x4 bytes[24] unknown_4_1b【未知】尚未核定的原生字节；不读取、不复制、不作填充（24B；未知） | 0x1C tree-header locust_gameplay_tree【权威】Host玩法树；Client现有owner只建本地空sentinel，不浅拷贝节点（8B；已核） | 0x24 std::vector<ANM2> locust_sprites_a【容器】第一组完整Sprite元素；两组合计不超过16384元素；各投影count必须一致，每个投影另受128MiB编码预算与全局池预算约束，超限报Gap，不截断（12B；已核） | 0x30 std::vector<ANM2> locust_sprites_b【容器】第二组完整Sprite元素；两组合计不超过16384元素；各投影count必须一致，每个投影另受128MiB编码预算与全局池预算约束，超限报Gap，不截断（12B；已核） | 0x3C bytes[4] unknown_3c_3f【未知】尚未核定的原生字节；不读取、不复制、不作填充（4B；未知） | 0x40 vector-header locust_gameplay_vector【权威】Host专用4字节元素容器；元素语义未闭合，不在Client重建玩法集合（12B；已核） | 0x4C EntityPtr locust_external_link【权威】外部反向引用关联；Client专用payload保持为空，不复制Host关系（4B；已核） | 0x50 u8 locust_unknown_50【未知】旧研究记录为bool，未闭合实际消费者与含义，不升级为可复制状态（1B；未知） | 0x51 bytes[7] unknown_51_57【未知】尚未核定的原生尾部；池步长不证明填充（7B；未知） |
|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 12—805306368B，不含第 9 步记录头）：

块 0：蝗虫Sprite外观（world，4—268435456B）：

| +0x0 u16 count + count×ANM2（上限16384） 子块0 locust_sprites_a#0：第一组完整Sprite元素；两组合计不超过16384元素；各投影count必须一致，每个投影另受128MiB编码预算与全局池预算约束，超限报Gap，不截断 | 顺序变长偏移 u16 count + count×ANM2（上限16384） 子块0 locust_sprites_b#0：第二组完整Sprite元素；两组合计不超过16384元素；各投影count必须一致，每个投影另受128MiB编码预算与全局池预算约束，超限报Gap，不截断 |
|---|---|

块 1：蝗虫Sprite动画（world，4—268435456B）：

| +0x0 u16 count + count×ANM2（上限16384） 子块1 locust_sprites_a#1：第一组完整Sprite元素；两组合计不超过16384元素；各投影count必须一致，每个投影另受128MiB编码预算与全局池预算约束，超限报Gap，不截断 | 顺序变长偏移 u16 count + count×ANM2（上限16384） 子块1 locust_sprites_b#1：第二组完整Sprite元素；两组合计不超过16384元素；各投影count必须一致，每个投影另受128MiB编码预算与全局池预算约束，超限报Gap，不截断 |
|---|---|

块 2：蝗虫Sprite图层（world，4—268435456B）：

| +0x0 u16 count + count×ANM2（上限16384） 子块2 locust_sprites_a#2：第一组完整Sprite元素；两组合计不超过16384元素；各投影count必须一致，每个投影另受128MiB编码预算与全局池预算约束，超限报Gap，不截断 | 顺序变长偏移 u16 count + count×ANM2（上限16384） 子块2 locust_sprites_b#2：第二组完整Sprite元素；两组合计不超过16384元素；各投影count必须一致，每个投影另受128MiB编码预算与全局池预算约束，超限报Gap，不截断 |
|---|---|

证据：

- `locust_vtable`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `unknown_4_1b`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `locust_gameplay_tree`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `locust_sprites_a`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `locust_sprites_b`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `unknown_3c_3f`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `locust_gameplay_vector`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `locust_external_link`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `locust_unknown_50`：docs/design/binary-state/familiar-knife-bomb-evidence.md
- `unknown_51_57`：docs/design/binary-state/familiar-knife-bomb-evidence.md

容器计数约束（同一容器各投影count相同，按容器去重求和）：

- `locust_sprites_total`：locust_sprites_a + locust_sprites_b ≤ 16384；mod/src/game/client_entity_class_model.cpp::Owned，AbyssLocust两组合计<=16384
