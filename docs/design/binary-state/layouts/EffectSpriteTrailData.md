# EffectSpriteTrailData

由唯一布局源生成。布局编号 27；布局版本 23；原生大小 0x18；未知 0B；完整：否。

原生字节归属：

| 0x0 u32 trail_vtable【本地】SpriteTrailData具体虚表（4B；已核） | 0x4 std::deque<EffectTrailNode> trail_nodes【容器】有序轨迹投影；当前64B记录前缀/步长仍待producer核定，不复制proxy/map/capacity/offset等进程状态（20B；推断） |
|---|---|

状态二进制布局（总计 2—1048578B，不含第 9 步记录头）：

块 0：轨迹头（仅根投影的present）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 1：轨迹节点（world，2—1048578B）：

| +0x0 u16 count + count×EffectTrailNode（上限16384） trail_nodes：有序轨迹投影；当前64B记录前缀/步长仍待producer核定，不复制proxy/map/capacity/offset等进程状态 |
|---|

证据：

- `trail_vtable`：docs/design/binary-state/slot-projectile-effect-evidence.md
- `trail_nodes`：docs/design/binary-state/slot-projectile-effect-evidence.md
