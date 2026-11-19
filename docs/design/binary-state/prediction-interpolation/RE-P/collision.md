# RE-P2：可单独调用的碰撞与地形函数

J273 静态结果。判定标准（设计 5.2 M-b）：不触发回调、不改其他对象、不消耗随机数，只写输出参数。

## 1. 纯读函数（可作 M-b）

| VA | 名称 | ABI | 说明 |
|---|---|---|---|
| `0x6B1C20` | GetGridCollisionAtPos | `thiscall(Entity* 或 null, Vector*)` → 碰撞类别 | 在 Room 查询基础上加玩家专属规则（门、坑、幽灵） |
| `0x7E6AF0` | Room::GetGridCollision(idx) | `thiscall(Room*, int)` | 见第 3 节 |
| `0x808530` | Room::GetGridIndex | | 坐标→格子序号 |
| `0x8087F0` | Room::GetClampedPosition | userpurge，xmm3 传边距 | 处理 L 形房间（经 `0x814B70`） |
| `0x713970`、`0x70B0B0`、`0x7C12C0`、`0x7D1C40`、`0x7D1C90`、`0x814B70` | 辅助 | | 均为只读几何／查询 |

这些函数读的是客机已安装的 Room／GridEntity 状态，调用前须确认被读字段都在客机上（见第 3 节缺口）。

## 2. PlayerCollideWithGrid `0x6A6A40(Player*, bool quiet)`

流程：

1. 房间边界钳制。
2. 按 `+0x178/+0x17C` 处向量中的采样点偏移逐点查询；根据 gridCollisionClass `+0x184` 判定哪些点碰撞，由碰撞点合成法线 n。
3. 速度响应：朝墙运动时 vel += n · (0.9 · (−n·v) − 0.1)。
4. 推出：用二分搜索（6 次，类别 5 为 12 次），半径取 size `+0x370` 与尺寸倍率 `+0x374/+0x378`。

副作用：

- quiet=0：Mega Mush（625）破坏或生成格子（`0x706100`、`0x705070`）；调用格子 OnCollide（vtable+20）；门 `0x70A8E0`；推石头 `0x813950`；写 `+0x190`（已碰撞）、`+0x194`（碰撞前速度）、`+0x19C`（法线）。
- quiet=1：跳过上述全部效果，只写 pos（不改速度、不写法线）。

结论：quiet=1 只写活对象的 pos，客机若调用须在自有临时对象或覆盖层作用域内进行；它也不给出速度响应，速度仍要自己算。因此 **M-b 实际只覆盖地形查询，速度与碰撞响应须 M-a 复刻**（采样点、法线合成、速度响应、二分推出按上面的流程实现）。

## 3. Room::GetGridCollision 的数据来源

```
if (this[1]+16 指向 type 35、variant 16 的实体) return 0;   // 场景未确认
if (idx < 0) return 4;
if (Room+0x24 起的格子实体表[idx] 非空) return gridEntity+0x3C;  // collision_class
cost = Room+0x76C 起的寻路表[idx];       // i32[448]，即布局 room.json 的 grid_paths
if (cost >= 3000) return cost <= 3999 ? 1 : 3;
return cost >= 1000 ? 3 : 0;
```

返回值按 GridCollisionClass：1=PIT、3=SOLID。即 cost 1000—2999 与 ≥4000 视为 SOLID，3000—3999 视为 PIT（飞行可越过），<1000 无碰撞。

这张表就是 Lua 的 `Room:GetGridPath／SetGridPath／GetGridPathFromPos`（本地 Wiki `IsaacDocs/docs/Room.md`、`PathFinder.md` FindGridPath 的 PathMarker 说明）。Wiki 的伪枚举：

| 值 | 来源 | 衰减 | 本函数结果 |
|---|---|---|---|
| 900 | 部分敌人走过时标记，降低寻路优先级 | 每次 −100 | 无碰撞 |
| 950 | 火堆 | 不衰减 | 无碰撞 |
| 1000 | 格子实体 | 不衰减 | SOLID |
| 3000 | 坑 | 不衰减 | PIT |
| 3999 | Grimace（每帧重置） | 降到 900 后每次 −100 | PIT |

Wiki 还说明 cost>0 会阻止在该格生成 GridEntity；REPENTOGON 的 `PRE/POST_*_GRID_COLLISION` 回调注明 cost≥1000 时实体可能与“空”格碰撞（GridEntity 为 nil）。

`grid_paths`（`Room+0x76C`，1792 字节）在布局中标为“留 Host；不为 Client 碰撞／寻路发送”。有 GridEntity 的格子走上面的第一分支，不受影响；差异只出现在**没有 GridEntity 但 cost≥1000** 的格子，已知来源是 Grimace 所在格（3999）以及 Lua `SetGridPath`／`FindGridPath` 写入的高值。火堆（950）和敌人足迹（900）不影响碰撞。预测碰撞实施前要二选一（并入 P7）：

1. 补同步 grid_paths 中 ≥1000 的阻挡位（可压缩为位图）；
2. 预测只看 GridEntity，把 cost 阻挡交给对账纠正。

> **2026-11-19 用户决定选 1**：派生 2 位类别随 World 同步，客机存影子表、不写原生表，见[设计 5.3.1](../../prediction-interpolation.md#531-寻路-cost-阻挡位同步p72026-11-19用户决定)。

## 4. 实体间碰撞

碰撞阶段 `sub_415880` 用 `sub_6ADA00/6ADD90` 做实体对碰撞（含双子身体对），会触发接触伤害、推挤等玩法。不可调用；按设计 5.3，首版只对静止阻挡实体做自写的圆形碰撞，其余交给对账。
