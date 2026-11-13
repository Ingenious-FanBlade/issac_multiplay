# 7：Room、Level、RoomSave 与 Exploration：生成布局

[返回布局总索引](../README.md) · [对应总计划步骤](../../7-room-level.md)

按布局编号排列；共享类型通过引用复用，不重复生成。未知字节保留原生事实；需求覆盖只说明清单字段已生成计划，不表示采集、应用或验收已完成。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 需求覆盖 |
|---|---|---|---|---|
| [Room](Room.md) | 239 | 30872 | 610 | 是 |
| [Backdrop](Backdrop.md) | 240 | 21112 | 374 | 是 |
| [BackdropConfig](BackdropConfig.md) | 241 | 316 | 5 | 是 |
| [FXLayers](FXLayers.md) | 242 | 1432 | 109 | 是 |
| [FXParams](FXParams.md) | 243 | 160 | 3 | 是 |
| [FXPose](FXPose.md) | 244 | 20 | 2 | 是 |
| [FX](FX.md) | 245 | 76 | 5 | 是 |
| [Ray](Ray.md) | 246 | 20 | 0 | 是 |
| [RayGroup](RayGroup.md) | 247 | 44 | 2 | 是 |
| [Vein](Vein.md) | 248 | 52 | 7 | 是 |
| [RailManager](RailManager.md) | 249 | 728 | 14 | 是 |
| [Camera](Camera.md) | 250 | 148 | 3 | 是 |
| [CameraRing](CameraRing.md) | 251 | 16 | 0 | 是 |
| [RoomShockwave](RoomShockwave.md) | 252 | 28 | 0 | 是 |
| [RoomHellBackdrop](RoomHellBackdrop.md) | 253 | 296 | 17 | 是 |
| [EntityList](EntityList.md) | 254 | 1024 | 27 | 是 |
| [EntityListEL](EntityListEL.md) | 255 | 16 | 3 | 是 |
| [RoomTemporaryEffects](RoomTemporaryEffects.md) | 256 | 24 | 2 | 是 |
| [FXPoop](FXPoop.md) | 257 | 76 | 5 | 是 |
| [RoomOutputSetHeader](RoomOutputSetHeader.md) | 258 | 8 | 0 | 是 |
| [RoomCallbackVector](RoomCallbackVector.md) | 259 | 12 | 0 | 是 |
| [RainSpawnRegion](RainSpawnRegion.md) | 260 | 24 | 0 | 是 |
| [OriginalOnlineEntityFrame](OriginalOnlineEntityFrame.md) | 261 | 24 | 0 | 是 |
| [KilledNPCRecord](KilledNPCRecord.md) | 262 | 24 | 3 | 是 |
| [RoomConfig_Room](RoomConfig_Room.md) | 263 | 92 | 0 | 是 |
| [RoomConfig_Stage](RoomConfig_Stage.md) | 264 | 196 | 6 | 是 |
| [RoomSet](RoomSet.md) | 265 | 44 | 3 | 是 |
| [RoomSpawn](RoomSpawn.md) | 266 | 16 | 3 | 是 |
| [RoomEntry](RoomEntry.md) | 267 | 12 | 0 | 是 |
| [DestinationQuad](DestinationQuad.md) | 268 | 32 | 0 | 是 |
| [SourceQuad](SourceQuad.md) | 269 | 36 | 0 | 是 |
| [RoomDescriptor](RoomDescriptor.md) | 270 | 184 | 0 | 是 |
| [RoomDescriptorSavedPickupMapInput](RoomDescriptorSavedPickupMapInput.md) | 271 | 120 | 70 | 是 |
| [RoomDescriptorSavedGridMapInput](RoomDescriptorSavedGridMapInput.md) | 272 | 32 | 3 | 是 |
| [RoomDescriptorShopKinds](RoomDescriptorShopKinds.md) | 273 | 16 | 0 | 是 |
| [RoomDescriptorRestockCounts](RoomDescriptorRestockCounts.md) | 274 | 8 | 0 | 是 |
| [RoomDescriptorRestrictedSetHeader](RoomDescriptorRestrictedSetHeader.md) | 275 | 8 | 0 | 是 |
| [LevelGamePrefix](LevelGamePrefix.md) | 276 | 100624 | 22 | 是 |
| [LevelRoomOffsetEntry](LevelRoomOffsetEntry.md) | 277 | 4 | 0 | 是 |
| [LevelTemporaryCurseField](LevelTemporaryCurseField.md) | 278 | 4 | 0 | 是 |
| [LevelASM](LevelASM.md) | 279 | 4 | 0 | 是 |
| [LevelCustomStageNameABI](LevelCustomStageNameABI.md) | 280 | 24 | 0 | 是 |
| [RoomVeinSprite](RoomVeinSprite.md) | 281 | 276 | 14 | 是 |
| [RoomRailSprite](RoomRailSprite.md) | 282 | 276 | 14 | 是 |
| [BackdropBindingEntry](BackdropBindingEntry.md) | 283 | 0 | 0 | 是 |
| [BackdropBinding](BackdropBinding.md) | 284 | 0 | 0 | 是 |
| [WaterShaderResult](WaterShaderResult.md) | 285 | 0 | 0 | 是 |
| [WaterVisualPayload](WaterVisualPayload.md) | 286 | 0 | 0 | 是 |
| [WaterVisual](WaterVisual.md) | 287 | 0 | 0 | 是 |
| [RoomLightingResult](RoomLightingResult.md) | 288 | 0 | 0 | 是 |
| [RoomWaveResult](RoomWaveResult.md) | 289 | 0 | 0 | 是 |
| [ForcedCameraFocus](ForcedCameraFocus.md) | 290 | 0 | 0 | 是 |
| [SelectedRayGroup](SelectedRayGroup.md) | 291 | 0 | 0 | 是 |
| [PoopVisualPayload](PoopVisualPayload.md) | 292 | 0 | 0 | 是 |
| [PoopVisual](PoopVisual.md) | 293 | 0 | 0 | 是 |
| [VeinGraphPayload](VeinGraphPayload.md) | 294 | 0 | 0 | 是 |
| [VeinGraphVisual](VeinGraphVisual.md) | 295 | 0 | 0 | 是 |
| [FXScenePayload](FXScenePayload.md) | 296 | 0 | 0 | 是 |
| [SavedMapIconPayload](SavedMapIconPayload.md) | 297 | 0 | 0 | 是 |
| [FinalSavedMapIcons](FinalSavedMapIcons.md) | 298 | 0 | 0 | 是 |
| [CurrentDescriptorVisual](CurrentDescriptorVisual.md) | 299 | 0 | 0 | 是 |
| [MapFocusResult](MapFocusResult.md) | 300 | 0 | 0 | 是 |
| [MapFocusPair](MapFocusPair.md) | 301 | 0 | 0 | 是 |
| [MapBadgeList](MapBadgeList.md) | 302 | 0 | 0 | 是 |
| [MinimapQuestionImageBinding](MinimapQuestionImageBinding.md) | 303 | 0 | 0 | 是 |
| [ResourceEntryHeader](ResourceEntryHeader.md) | 304 | 0 | 0 | 是 |
| [SurfacePixelsDescriptor](SurfacePixelsDescriptor.md) | 305 | 0 | 0 | 是 |
| [ResourceBlobChunkHeader](ResourceBlobChunkHeader.md) | 306 | 0 | 0 | 是 |
| [SurfaceRecipeHeader](SurfaceRecipeHeader.md) | 307 | 0 | 0 | 是 |
| [SurfaceRecipeClear](SurfaceRecipeClear.md) | 308 | 0 | 0 | 是 |
| [SurfaceRecipeQuad](SurfaceRecipeQuad.md) | 309 | 0 | 0 | 是 |
| [RoomWaveSprite](RoomWaveSprite.md) | 434 | 276 | 11 | 是 |

公开目录：[stream-catalog room域全部 269 行](stream-catalog-room-members.md)。

公开目录：[stream-catalog level域全部 131 行](stream-catalog-level-members.md)。

公开目录：[stream-catalog room_save域全部 98 行](stream-catalog-room_save-members.md)。

公开目录：[stream-catalog exploration域全部 99 行](stream-catalog-exploration-members.md)。
