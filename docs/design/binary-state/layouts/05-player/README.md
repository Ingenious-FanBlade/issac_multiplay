# 5：EntityPlayer：生成布局

[返回布局总索引](../README.md) · [对应总计划步骤](../../5-player.md)

按布局编号排列；共享类型通过引用复用，不重复生成。未知字节与完整标志只描述布局事实，不表示采集、应用或验收已完成。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 完整 |
|---|---|---|---|---|
| [PlayerTemporaryEffects](PlayerTemporaryEffects.md) | 150 | 24 | 2 | 否 |
| [PlayerCostumeMap](PlayerCostumeMap.md) | 151 | 16 | 3 | 否 |
| [CostumeSpriteDesc](CostumeSpriteDesc.md) | 152 | 304 | 16 | 否 |
| [PocketItem](PocketItem.md) | 153 | 8 | 0 | 是 |
| [QueueItemData](QueueItemData.md) | 154 | 16 | 0 | 是 |
| [SmeltedTrinketDesc](SmeltedTrinketDesc.md) | 155 | 4 | 0 | 是 |
| [AfterImageFrame](AfterImageFrame.md) | 156 | 12 | 0 | 是 |
| [History](History.md) | 157 | 20 | 3 | 否 |
| [HistoryItem](HistoryItem.md) | 158 | 28 | 3 | 否 |
| [BagOfCraftingOutput](BagOfCraftingOutput.md) | 159 | 8 | 0 | 是 |
| [EntityDescHost](EntityDescHost.md) | 160 | 28 | 3 | 否 |
| [ConsumableData](ConsumableData.md) | 161 | 32 | 2 | 否 |
| [ImageStrongPair](ImageStrongPair.md) | 162 | 8 | 0 | 是 |
| [PlayerHUDStrongPair](PlayerHUDStrongPair.md) | 163 | 8 | 0 | 是 |
| [OptionalDrawSequence](OptionalDrawSequence.md) | 164 | 292 | 18 | 否 |
| [DrawRecordNode](DrawRecordNode.md) | 165 | 40 | 8 | 否 |
| [SavedHistory](SavedHistory.md) | 166 | 20 | 8 | 否 |
| [GameStatePlayer](GameStatePlayer.md) | 167 | 1468 | 1408 | 否 |
| [FamiliarData](FamiliarData.md) | 168 | 52 | 12 | 否 |
| [NativeCommunicationDisplay](NativeCommunicationDisplay.md) | 169 | 2056 | 102 | 否 |
| [ItemSpoof](ItemSpoof.md) | 170 | 44 | 0 | 是 |
| [InnateItems](InnateItems.md) | 171 | 76 | 0 | 是 |
| [TemporaryInnateItem](TemporaryInnateItem.md) | 172 | 12 | 0 | 是 |
| [PlayerItemSpoofs](PlayerItemSpoofs.md) | 173 | 92 | 0 | 是 |
| [EntityPlayerPlus](EntityPlayerPlus.md) | 174 | 168 | 9 | 否 |
| [EntitySaveStateStrongPair](EntitySaveStateStrongPair.md) | 175 | 8 | 0 | 是 |
| [EntitySaveState](EntitySaveState.md) | 176 | 120 | 74 | 否 |
| [PlayerItemCount](PlayerItemCount.md) | 177 | 8 | 0 | 是 |
| [PlayerInnateCount](PlayerInnateCount.md) | 178 | 8 | 0 | 是 |
| [PlayerActiveItemDesc](PlayerActiveItemDesc.md) | 179 | 32 | 0 | 是 |
| [PlayerTemporaryEffect](PlayerTemporaryEffect.md) | 180 | 16 | 4 | 否 |
| [PlayerWeaponTears](PlayerWeaponTears.md) | 181 | 80 | 28 | 否 |
| [PlayerWeaponBrimstone](PlayerWeaponBrimstone.md) | 182 | 76 | 20 | 否 |
| [PlayerWeaponLaser](PlayerWeaponLaser.md) | 183 | 80 | 24 | 否 |
| [PlayerWeaponKnife](PlayerWeaponKnife.md) | 184 | 84 | 16 | 否 |
| [PlayerWeaponBombs](PlayerWeaponBombs.md) | 185 | 72 | 16 | 否 |
| [PlayerWeaponRockets](PlayerWeaponRockets.md) | 186 | 72 | 16 | 否 |
| [PlayerWeaponMonstrosLung](PlayerWeaponMonstrosLung.md) | 187 | 68 | 16 | 否 |
| [PlayerWeaponLudovico](PlayerWeaponLudovico.md) | 188 | 72 | 16 | 否 |
| [PlayerWeaponTechX](PlayerWeaponTechX.md) | 189 | 68 | 16 | 否 |
| [PlayerWeaponBoneAxe](PlayerWeaponBoneAxe.md) | 190 | 96 | 20 | 否 |
| [PlayerWeaponUrn](PlayerWeaponUrn.md) | 191 | 84 | 24 | 否 |
| [PlayerWeaponSpiritSword](PlayerWeaponSpiritSword.md) | 192 | 80 | 24 | 否 |
| [PlayerWeaponFetus](PlayerWeaponFetus.md) | 193 | 68 | 16 | 否 |
| [PlayerWeaponWhip](PlayerWeaponWhip.md) | 194 | 72 | 16 | 否 |
| [PlayerParticipant](PlayerParticipant.md) | 195 | 0 | 0 | 是 |
| [PlayerControlGroup](PlayerControlGroup.md) | 196 | 0 | 0 | 是 |
| [EntityPlayer](EntityPlayer.md) | 197 | 11936 | 3707 | 否 |
