# BossOverlayDefinition

由唯一布局源生成。布局编号 351；布局版本 84；原生大小 0x38；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

五份静态生产定义不下行；每份3B未知不称padding。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 i32 id【权威】bossoverlays.xml entry id（4B；已核） | 0x4 std::string anm2_path【权威】配置选择的ANM2路径（24B；已核） | 0x1C std::string animation_name【权威】配置选择的动画名（24B；已核） | 0x34 bool persist【权威】配置persist；Update据此决定结束（1B；已核） | 0x35 bytes[3] unknown_35_37【未知】未取得字段用途或填充证明（3B；未知） |
|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `id`：41C3D0 parser写+11C+38*id；41C2F0
- `anm2_path`：41C3D0 anm2root/anm2；41CD62
- `animation_name`：41C3D0 anim；41CD8F—41CE0D
- `persist`：41C3D0 persist；41CB74
- `unknown_35_37`：presentation-resource-screen-sources.md；J273 VA−RVA=0x400000；本轮仅静态核对
