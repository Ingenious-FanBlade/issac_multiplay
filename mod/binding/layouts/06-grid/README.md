# 6：GridEntity：生成布局

[返回布局总索引](../README.md) · [对应总计划步骤](../../../../docs/archive/design/binary-state/6-grid.md)

按布局编号排列；共享类型通过引用复用，不重复生成。未知字节保留原生事实；需求覆盖只说明清单字段已生成计划，不表示采集、应用或验收已完成。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 需求覆盖 |
|---|---|---|---|---|
| [GridEntityDesc](GridEntityDesc.md) | 198 | 32 | 3 | 是 |
| [GridEntity](GridEntity.md) | 199 | 340 | 17 | 是 |
| [GridLayerState](GridLayerState.md) | 200 | 160 | 4 | 是 |
| [GridSprite](GridSprite.md) | 201 | 276 | 14 | 是 |
| [GridFloorSprite](GridFloorSprite.md) | 202 | 276 | 14 | 是 |
| [GridFireEmberAnimation](GridFireEmberAnimation.md) | 203 | 32 | 3 | 是 |
| [GridFireEmberSprite](GridFireEmberSprite.md) | 204 | 276 | 14 | 是 |
| [GridFireEmber](GridFireEmber.md) | 205 | 24 | 0 | 是 |
| [GridEntityDecoration](GridEntityDecoration.md) | 206 | 340 | 17 | 是 |
| [GridEntityGravity](GridEntityGravity.md) | 207 | 340 | 17 | 是 |
| [GridEntityStairs](GridEntityStairs.md) | 208 | 340 | 17 | 是 |
| [GridEntityTeleporter](GridEntityTeleporter.md) | 209 | 340 | 17 | 是 |
| [GridEntityWall](GridEntityWall.md) | 210 | 340 | 17 | 是 |
| [GridEntityWeb](GridEntityWeb.md) | 211 | 340 | 17 | 是 |
| [GridEntityTrapDoor](GridEntityTrapDoor.md) | 212 | 344 | 17 | 是 |
| [GridEntityFire](GridEntityFire.md) | 213 | 912 | 45 | 是 |
| [GridRockLayerState](GridRockLayerState.md) | 214 | 160 | 4 | 是 |
| [GridRockSprite](GridRockSprite.md) | 215 | 276 | 14 | 是 |
| [GridStatueUnusedSprite](GridStatueUnusedSprite.md) | 216 | 276 | 14 | 是 |
| [GridEntityLock](GridEntityLock.md) | 217 | 344 | 20 | 是 |
| [GridEntityPit](GridEntityPit.md) | 218 | 344 | 20 | 是 |
| [GridEntitySpikes](GridEntitySpikes.md) | 219 | 344 | 17 | 是 |
| [GridEntityStatue](GridEntityStatue.md) | 220 | 344 | 17 | 是 |
| [GridEntityTNT](GridEntityTNT.md) | 221 | 344 | 17 | 是 |
| [GridEntityPoop](GridEntityPoop.md) | 222 | 412 | 19 | 是 |
| [GridEntityRock](GridEntityRock.md) | 223 | 408 | 17 | 是 |
| [GridNonrandomLightDesc](GridNonrandomLightDesc.md) | 224 | 32 | 3 | 是 |
| [GridDoorTint](GridDoorTint.md) | 225 | 44 | 0 | 是 |
| [GridDoorSprite](GridDoorSprite.md) | 226 | 276 | 14 | 是 |
| [GridEntityPressurePlate](GridEntityPressurePlate.md) | 227 | 636 | 31 | 是 |
| [GridEntityDoor](GridEntityDoor.md) | 228 | 752 | 32 | 是 |
| [GridEntityPlus](GridEntityPlus.md) | 229 | 12 | 0 | 是 |

公开目录：[grid-catalog 全部 119 行](grid-catalog-members.md)。
