# 4：Entity 子类：生成布局

[返回布局总索引](../README.md) · [对应总计划步骤](../../4-entity-classes.md)

按布局编号排列；共享类型通过引用复用，不重复生成。未知字节与完整标志只描述布局事实，不表示采集、应用或验收已完成。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 完整 |
|---|---|---|---|---|
| [EntityFamiliarPlus](EntityFamiliarPlus.md) | 16 | 28 | 5 | 否 |
| [EntityKnifePlus](EntityKnifePlus.md) | 17 | 20 | 5 | 否 |
| [FamiliarIsaacsBodyData](FamiliarIsaacsBodyData.md) | 18 | 1400 | 70 | 否 |
| [FamiliarAbyssLocustData](FamiliarAbyssLocustData.md) | 19 | 88 | 36 | 否 |
| [EntityTearPlus](EntityTearPlus.md) | 20 | 24 | 5 | 否 |
| [EntityPickupPlus](EntityPickupPlus.md) | 21 | 20 | 7 | 否 |
| [EntityLaserPlus](EntityLaserPlus.md) | 22 | 28 | 8 | 否 |
| [HomingLaserBezier](HomingLaserBezier.md) | 23 | 24 | 0 | 是 |
| [HomingLaser](HomingLaser.md) | 24 | 84 | 0 | 是 |
| [EffectMrMeData](EffectMrMeData.md) | 25 | 296 | 14 | 否 |
| [EffectTrailNode](EffectTrailNode.md) | 26 | 64 | 0 | 是 |
| [EffectSpriteTrailData](EffectSpriteTrailData.md) | 27 | 24 | 0 | 是 |
| [EffectPickupGhostData](EffectPickupGhostData.md) | 28 | 1112 | 56 | 否 |
| [EntityEffect](EntityEffect.md) | 101 | 1216 | 382 | 否 |
| [EntityEffect_TearFlags](EntityEffect_TearFlags.md) | 102 | 1216 | 366 | 否 |
| [EntityEffect_GridDesc](EntityEffect_GridDesc.md) | 103 | 1216 | 353 | 否 |
| [EntityEffect_MrMe](EntityEffect_MrMe.md) | 104 | 1216 | 382 | 否 |
| [EntityEffect_SpriteTrail](EntityEffect_SpriteTrail.md) | 105 | 1216 | 382 | 否 |
| [EntityEffect_PickupGhost](EntityEffect_PickupGhost.md) | 106 | 1216 | 382 | 否 |
| [EntityFamiliar](EntityFamiliar.md) | 120 | 3888 | 905 | 否 |
| [EntityKnife](EntityKnife.md) | 121 | 1312 | 497 | 否 |
| [EntityBomb](EntityBomb.md) | 122 | 2520 | 441 | 否 |
| [EntityFamiliar_IsaacsBody](EntityFamiliar_IsaacsBody.md) | 123 | 3888 | 905 | 否 |
| [EntityFamiliar_AbyssLocust](EntityFamiliar_AbyssLocust.md) | 124 | 3888 | 905 | 否 |
| [EntityTear](EntityTear.md) | 125 | 2120 | 478 | 否 |
| [EntityPickup](EntityPickup.md) | 126 | 1776 | 448 | 否 |
| [EntityLaser](EntityLaser.md) | 127 | 1384 | 421 | 否 |
| [EntitySlot](EntitySlot.md) | 128 | 1360 | 363 | 否 |
| [EntityProjectile](EntityProjectile.md) | 129 | 1144 | 350 | 否 |
| [EntityFamiliar_UmbilicalBaby](EntityFamiliar_UmbilicalBaby.md) | 130 | 3888 | 905 | 否 |
| [EntityFamiliar_Incubus](EntityFamiliar_Incubus.md) | 131 | 3888 | 905 | 否 |
| [UmbilicalRopeNode](UmbilicalRopeNode.md) | 132 | 120 | 108 | 否 |
| [FamiliarUmbilicalBabyData](FamiliarUmbilicalBabyData.md) | 133 | 344 | 58 | 否 |
| [EffectPickupGhostSprite](EffectPickupGhostSprite.md) | 134 | 276 | 14 | 否 |
| [EntityEffect_Propagation](EntityEffect_Propagation.md) | 137 | 1216 | 350 | 否 |
| [EntityEffect_Timer](EntityEffect_Timer.md) | 138 | 1216 | 370 | 否 |
| [EntityEffect_SpawnRequest](EntityEffect_SpawnRequest.md) | 139 | 1216 | 374 | 否 |
| [EntityEffect_DustCountdown](EntityEffect_DustCountdown.md) | 140 | 1216 | 378 | 否 |
| [EntityEffect_TearFlags113](EntityEffect_TearFlags113.md) | 141 | 1216 | 358 | 否 |
| [EntityNPC](EntityNPC.md) | 142 | 3936 | 586 | 否 |
| [EntityNPC_Delirium](EntityNPC_Delirium.md) | 143 | 3936 | 590 | 否 |
| [NpcOverlaySprite](NpcOverlaySprite.md) | 144 | 276 | 14 | 否 |
| [EntityText](EntityText.md) | 145 | 1328 | 339 | 否 |
| [EntityText_Scaled](EntityText_Scaled.md) | 146 | 1328 | 339 | 否 |
| [EntityDummy](EntityDummy.md) | 147 | 1040 | 335 | 否 |
| [EntityEffect_ChainLightning](EntityEffect_ChainLightning.md) | 148 | 1216 | 362 | 否 |
| [EntityEffect_InternalTearFlags](EntityEffect_InternalTearFlags.md) | 149 | 1216 | 366 | 否 |
