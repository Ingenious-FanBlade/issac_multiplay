# 生成布局索引

只修改 `mod/binding/state/`。未知和未核声明照实保留，不能据本表宣称原生同步已可用。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 完整 |
|---|---|---|---|---|
| [Vector](Vector.md) | 1 | 8 | 0 | 是 |
| [ColorMod](ColorMod.md) | 2 | 44 | 0 | 是 |
| [KColor](KColor.md) | 3 | 20 | 0 | 是 |
| [BlendMode](BlendMode.md) | 4 | 20 | 0 | 是 |
| [AnimationState](AnimationState.md) | 5 | 32 | 3 | 否 |
| [LayerState](LayerState.md) | 6 | 160 | 4 | 否 |
| [ANM2](ANM2.md) | 7 | 276 | 14 | 否 |
| [RNG](RNG.md) | 8 | 16 | 0 | 是 |
| [BitSet128](BitSet128.md) | 9 | 16 | 0 | 是 |
| [EntityPtr](EntityPtr.md) | 10 | 4 | 0 | 是 |
| [ColorParams](ColorParams.md) | 11 | 60 | 2 | 否 |
| [ActiveItemDesc](ActiveItemDesc.md) | 12 | 32 | 0 | 是 |
| [Weapon](Weapon.md) | 13 | 68 | 20 | 否 |
| [TemporaryEffect](TemporaryEffect.md) | 14 | 16 | 4 | 否 |
| [EntityPlus](EntityPlus.md) | 15 | 16 | 5 | 否 |
| [EntityFamiliarPlus](EntityFamiliarPlus.md) | 16 | 28 | 5 | 否 |
| [EntityKnifePlus](EntityKnifePlus.md) | 17 | 20 | 5 | 否 |
| [FamiliarIsaacsBodyData](FamiliarIsaacsBodyData.md) | 18 | 1400 | 70 | 否 |
| [FamiliarAbyssLocustData](FamiliarAbyssLocustData.md) | 19 | 88 | 36 | 否 |
| [EntityTearPlus](EntityTearPlus.md) | 20 | 24 | 5 | 否 |
| [EntityPickupPlus](EntityPickupPlus.md) | 21 | 20 | 7 | 否 |
| [EntityLaserPlus](EntityLaserPlus.md) | 22 | 28 | 8 | 否 |
| [HomingLaserBezier](HomingLaserBezier.md) | 23 | 24 | 0 | 是 |
| [HomingLaser](HomingLaser.md) | 24 | 84 | 0 | 是 |
| [EffectMrMeData](EffectMrMeData.md) | 25 | 296 | 30 | 否 |
| [EffectTrailNode](EffectTrailNode.md) | 26 | 64 | 0 | 否 |
| [EffectSpriteTrailData](EffectSpriteTrailData.md) | 27 | 24 | 0 | 否 |
| [EffectPickupGhostData](EffectPickupGhostData.md) | 28 | 1112 | 56 | 否 |
| [Entity](Entity.md) | 100 | 1040 | 335 | 否 |
| [EntityEffect](EntityEffect.md) | 101 | 1216 | 438 | 否 |
| [EntityEffect_TearFlags](EntityEffect_TearFlags.md) | 102 | 1216 | 422 | 否 |
| [EntityEffect_GridDesc](EntityEffect_GridDesc.md) | 103 | 1216 | 409 | 否 |
| [EntityEffect_MrMe](EntityEffect_MrMe.md) | 104 | 1216 | 438 | 否 |
| [EntityEffect_SpriteTrail](EntityEffect_SpriteTrail.md) | 105 | 1216 | 438 | 否 |
| [EntityEffect_PickupGhost](EntityEffect_PickupGhost.md) | 106 | 1216 | 438 | 否 |
| [EntityFamiliar](EntityFamiliar.md) | 120 | 3888 | 906 | 否 |
| [EntityKnife](EntityKnife.md) | 121 | 1312 | 521 | 否 |
| [EntityBomb](EntityBomb.md) | 122 | 2520 | 441 | 否 |
| [EntityFamiliar_IsaacsBody](EntityFamiliar_IsaacsBody.md) | 123 | 3888 | 906 | 否 |
| [EntityFamiliar_AbyssLocust](EntityFamiliar_AbyssLocust.md) | 124 | 3888 | 906 | 否 |
| [EntityTear](EntityTear.md) | 125 | 2120 | 478 | 否 |
| [EntityPickup](EntityPickup.md) | 126 | 1776 | 448 | 否 |
| [EntityLaser](EntityLaser.md) | 127 | 1384 | 421 | 否 |
| [EntitySlot](EntitySlot.md) | 128 | 1360 | 367 | 否 |
| [EntityProjectile](EntityProjectile.md) | 129 | 1144 | 379 | 否 |
