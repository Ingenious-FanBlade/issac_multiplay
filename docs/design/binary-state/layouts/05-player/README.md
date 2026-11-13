# 5：EntityPlayer：生成布局

[返回布局总索引](../README.md) · [对应总计划步骤](../../5-player.md)

按布局编号排列；共享类型通过引用复用，不重复生成。未知字节保留原生事实；需求覆盖只说明清单字段已生成计划，不表示采集、应用或验收已完成。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 需求覆盖 |
|---|---|---|---|---|
| [PlayerTemporaryEffects](PlayerTemporaryEffects.md) | 150 | 24 | 2 | 是 |
| [PlayerCostumeMap](PlayerCostumeMap.md) | 151 | 16 | 3 | 是 |
| [CostumeSpriteDesc](CostumeSpriteDesc.md) | 152 | 304 | 16 | 是 |
| [PocketItem](PocketItem.md) | 153 | 8 | 0 | 是 |
| [QueueItemData](QueueItemData.md) | 154 | 16 | 0 | 是 |
| [SmeltedTrinketDesc](SmeltedTrinketDesc.md) | 155 | 4 | 0 | 是 |
| [AfterImageFrame](AfterImageFrame.md) | 156 | 12 | 0 | 是 |
| [History](History.md) | 157 | 20 | 3 | 是 |
| [HistoryItem](HistoryItem.md) | 158 | 28 | 3 | 是 |
| [BagOfCraftingOutput](BagOfCraftingOutput.md) | 159 | 8 | 0 | 是 |
| [EntityDescHost](EntityDescHost.md) | 160 | 28 | 3 | 是 |
| [ConsumableData](ConsumableData.md) | 161 | 32 | 2 | 是 |
| [ImageStrongPair](ImageStrongPair.md) | 162 | 8 | 0 | 是 |
| [PlayerHUDStrongPair](PlayerHUDStrongPair.md) | 163 | 8 | 0 | 是 |
| [OptionalDrawSequence](OptionalDrawSequence.md) | 164 | 292 | 18 | 是 |
| [DrawRecordNode](DrawRecordNode.md) | 165 | 40 | 8 | 是 |
| [SavedHistory](SavedHistory.md) | 166 | 20 | 8 | 是 |
| [GameStatePlayer](GameStatePlayer.md) | 167 | 1468 | 1408 | 是 |
| [FamiliarData](FamiliarData.md) | 168 | 52 | 12 | 是 |
| [NativeCommunicationDisplay](NativeCommunicationDisplay.md) | 169 | 2056 | 102 | 是 |
| [ItemSpoof](ItemSpoof.md) | 170 | 44 | 0 | 是 |
| [InnateItems](InnateItems.md) | 171 | 76 | 0 | 是 |
| [TemporaryInnateItem](TemporaryInnateItem.md) | 172 | 12 | 0 | 是 |
| [PlayerItemSpoofs](PlayerItemSpoofs.md) | 173 | 92 | 0 | 是 |
| [EntityPlayerPlus](EntityPlayerPlus.md) | 174 | 168 | 9 | 是 |
| [EntitySaveStateStrongPair](EntitySaveStateStrongPair.md) | 175 | 8 | 0 | 是 |
| [EntitySaveState](EntitySaveState.md) | 176 | 120 | 74 | 是 |
| [PlayerItemCount](PlayerItemCount.md) | 177 | 8 | 0 | 是 |
| [PlayerInnateCount](PlayerInnateCount.md) | 178 | 8 | 0 | 是 |
| [PlayerActiveItemDesc](PlayerActiveItemDesc.md) | 179 | 32 | 0 | 是 |
| [PlayerTemporaryEffect](PlayerTemporaryEffect.md) | 180 | 16 | 4 | 是 |
| [PlayerWeaponTears](PlayerWeaponTears.md) | 181 | 80 | 28 | 是 |
| [PlayerWeaponBrimstone](PlayerWeaponBrimstone.md) | 182 | 76 | 20 | 是 |
| [PlayerWeaponLaser](PlayerWeaponLaser.md) | 183 | 80 | 24 | 是 |
| [PlayerWeaponKnife](PlayerWeaponKnife.md) | 184 | 84 | 16 | 是 |
| [PlayerWeaponBombs](PlayerWeaponBombs.md) | 185 | 72 | 16 | 是 |
| [PlayerWeaponRockets](PlayerWeaponRockets.md) | 186 | 72 | 16 | 是 |
| [PlayerWeaponMonstrosLung](PlayerWeaponMonstrosLung.md) | 187 | 68 | 16 | 是 |
| [PlayerWeaponLudovico](PlayerWeaponLudovico.md) | 188 | 72 | 16 | 是 |
| [PlayerWeaponTechX](PlayerWeaponTechX.md) | 189 | 68 | 16 | 是 |
| [PlayerWeaponBoneAxe](PlayerWeaponBoneAxe.md) | 190 | 96 | 20 | 是 |
| [PlayerWeaponUrn](PlayerWeaponUrn.md) | 191 | 84 | 24 | 是 |
| [PlayerWeaponSpiritSword](PlayerWeaponSpiritSword.md) | 192 | 80 | 24 | 是 |
| [PlayerWeaponFetus](PlayerWeaponFetus.md) | 193 | 68 | 16 | 是 |
| [PlayerWeaponWhip](PlayerWeaponWhip.md) | 194 | 72 | 16 | 是 |
| [PlayerParticipant](PlayerParticipant.md) | 195 | 0 | 0 | 是 |
| [PlayerControlGroup](PlayerControlGroup.md) | 196 | 0 | 0 | 是 |
| [EntityPlayer](EntityPlayer.md) | 197 | 11936 | 3707 | 是 |
