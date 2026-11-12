# TemporaryInnateItem

由唯一布局源生成。布局编号 172；布局版本 71；原生大小 0xC；未知 0B；完整：是。

原生字节归属：

| 0x0 i32 id【权威】定时管理条目ID（4B；已核） | 0x4 i32 count【权威】定时管理条目份数，正式投影使用cachedTotals（4B；已核） | 0x8 i32 timer【权威】到期管理timer，不使P3每tick变化（4B；已核） |
|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `id`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h
- `count`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h
- `timer`：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h
