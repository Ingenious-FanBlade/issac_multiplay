# 8：Run、Presentation 与 Random：生成布局

[返回布局总索引](../README.md) · [对应总计划步骤](../../8-run-presentation.md)

按布局编号排列；共享类型通过引用复用，不重复生成。未知字节与完整标志只描述布局事实，不表示采集、应用或验收已完成。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 完整 |
|---|---|---|---|---|
| [RunSeeds](RunSeeds.md) | 230 | 92 | 3 | 否 |
| [RunAmbush](RunAmbush.md) | 231 | 76 | 3 | 否 |
| [RunChallengeParam](RunChallengeParam.md) | 232 | 164 | 19 | 否 |
| [RunDailyChallenge](RunDailyChallenge.md) | 233 | 180 | 22 | 否 |
| [RunScoreSheet](RunScoreSheet.md) | 234 | 144 | 3 | 否 |
| [RunColorModState](RunColorModState.md) | 235 | 24 | 0 | 是 |
| [RunGameStateFlags](RunGameStateFlags.md) | 236 | 8 | 0 | 是 |
| [GameRun](GameRun.md) | 237 | 426216 | 150600 | 否 |
| [RunSeedEffectNode](RunSeedEffectNode.md) | 238 | 20 | 2 | 否 |
| [PlayerHUDHeart](PlayerHUDHeart.md) | 310 | 16 | 3 | 否 |
| [PlayerHUDActiveItem](PlayerHUDActiveItem.md) | 311 | 28 | 0 | 否 |
| [PlayerHUDTrinket](PlayerHUDTrinket.md) | 312 | 24 | 0 | 是 |
| [PlayerHUDPocket](PlayerHUDPocket.md) | 313 | 284 | 14 | 否 |
| [InventoryVisual](InventoryVisual.md) | 314 | 12 | 0 | 是 |
| [PlayerHUD](PlayerHUD.md) | 315 | 1760 | 150 | 否 |
| [StatHUDValue](StatHUDValue.md) | 316 | 20 | 0 | 是 |
| [StatHUDPlayer](StatHUDPlayer.md) | 317 | 204 | 0 | 是 |
| [StatHUD](StatHUD.md) | 318 | 688 | 17 | 否 |
| [HUDMessage](HUDMessage.md) | 319 | 500 | 167 | 否 |
| [HistoryHUDItem](HistoryHUDItem.md) | 320 | 40 | 0 | 是 |
| [HistoryHUDPlayer](HistoryHUDPlayer.md) | 321 | 20 | 4 | 否 |
| [HistoryHUD](HistoryHUD.md) | 322 | 44 | 11 | 否 |
| [HUD](HUD.md) | 323 | 23696 | 3388 | 否 |
| [MinimapConfig](MinimapConfig.md) | 324 | 352 | 26 | 否 |
| [Minimap](Minimap.md) | 325 | 1576 | 109 | 否 |
| [ColorModState](ColorModState.md) | 326 | 24 | 0 | 是 |
| [GenericPrompt](GenericPrompt.md) | 327 | 308 | 18 | 否 |
| [ItemOverlayDefinition](ItemOverlayDefinition.md) | 328 | 88 | 4 | 否 |
| [ItemOverlay](ItemOverlay.md) | 329 | 4704 | 216 | 否 |
| [MusicStream](MusicStream.md) | 330 | 52 | 4 | 否 |
| [MusicStreamSet](MusicStreamSet.md) | 331 | 388 | 31 | 否 |
| [MusicEntry](MusicEntry.md) | 332 | 96 | 2 | 否 |
| [MusicLayerDefinition](MusicLayerDefinition.md) | 333 | 52 | 0 | 是 |
| [Music](Music.md) | 334 | 872 | 67 | 否 |
| [SoundSample](SoundSample.md) | 335 | 56 | 4 | 否 |
| [SoundEffect](SoundEffect.md) | 336 | 440 | 30 | 否 |
| [SoundEffects](SoundEffects.md) | 337 | 24 | 0 | 是 |
| [Font](Font.md) | 338 | 76 | 14 | 否 |
| [FontSettings](FontSettings.md) | 339 | 28 | 8 | 否 |
| [ImageBase](ImageBase.md) | 340 | 128 | 53 | 否 |
| [RoomTransition](RoomTransition.md) | 341 | 572 | 242 | 否 |
| [StageTransition](StageTransition.md) | 342 | 32 | 16 | 否 |
| [NightmareScene](NightmareScene.md) | 343 | 1516 | 107 | 否 |
| [GameOver](GameOver.md) | 344 | 304 | 34 | 否 |
| [PauseScreen](PauseScreen.md) | 345 | 3572 | 1286 | 否 |
| [GameScreenEffects](GameScreenEffects.md) | 346 | 36 | 6 | 否 |
| [GameFadeView](GameFadeView.md) | 347 | 52 | 7 | 否 |
| [GameColorView](GameColorView.md) | 348 | 144 | 47 | 否 |
| [NightmareSnapshotQueue](NightmareSnapshotQueue.md) | 349 | 20 | 0 | 是 |
| [NightmareSnapshot](NightmareSnapshot.md) | 350 | 32 | 0 | 是 |
| [BossOverlayDefinition](BossOverlayDefinition.md) | 351 | 56 | 3 | 否 |
| [BossOverlay](BossOverlay.md) | 352 | 564 | 29 | 否 |
| [VisiblePlayerHUDHeart](VisiblePlayerHUDHeart.md) | 353 | 0 | 0 | 是 |
| [StatHUDRows](StatHUDRows.md) | 354 | 0 | 0 | 是 |
| [MapDisplaySprite](MapDisplaySprite.md) | 355 | 276 | 14 | 否 |
| [PresentationDrawClock](PresentationDrawClock.md) | 356 | 0 | 0 | 是 |
| [PickupRandomAlphaResult](PickupRandomAlphaResult.md) | 357 | 0 | 0 | 是 |
| [FamiliarRandomAlphaResult](FamiliarRandomAlphaResult.md) | 358 | 0 | 0 | 是 |
| [PillDisplayResult](PillDisplayResult.md) | 359 | 0 | 0 | 是 |
| [PickupPreviewItem](PickupPreviewItem.md) | 360 | 0 | 0 | 是 |
| [PickupPreviewResult](PickupPreviewResult.md) | 361 | 0 | 0 | 是 |
| [PlayerDrawImageBindings](PlayerDrawImageBindings.md) | 362 | 0 | 0 | 是 |
| [HudBodySelection](HudBodySelection.md) | 363 | 0 | 0 | 是 |
| [ControlGroupHudSelection](ControlGroupHudSelection.md) | 364 | 0 | 0 | 是 |
| [PresentationSpriteContribution](PresentationSpriteContribution.md) | 365 | 0 | 0 | 否 |
| [PresentationImageContribution](PresentationImageContribution.md) | 366 | 0 | 0 | 是 |
| [ScreenBloomResult](ScreenBloomResult.md) | 367 | 0 | 0 | 是 |
| [ScreenPixelationResult](ScreenPixelationResult.md) | 368 | 0 | 0 | 是 |
| [ScreenRetrovisionResult](ScreenRetrovisionResult.md) | 369 | 0 | 0 | 是 |
| [ScreenHallucinationResult](ScreenHallucinationResult.md) | 370 | 0 | 0 | 是 |
| [RoomGlitchResult](RoomGlitchResult.md) | 371 | 0 | 0 | 是 |
| [ScreenFadeContribution](ScreenFadeContribution.md) | 372 | 0 | 0 | 是 |
| [ScreenEffectsResult](ScreenEffectsResult.md) | 373 | 0 | 0 | 是 |
| [BossVersusResult](BossVersusResult.md) | 374 | 0 | 0 | 否 |
| [NightmareSceneResult](NightmareSceneResult.md) | 375 | 0 | 0 | 否 |
| [GameOverResult](GameOverResult.md) | 376 | 0 | 0 | 否 |
| [StageTransitionResult](StageTransitionResult.md) | 377 | 0 | 0 | 是 |
| [ItemOverlayResult](ItemOverlayResult.md) | 378 | 0 | 0 | 否 |
| [BossOverlayResult](BossOverlayResult.md) | 379 | 0 | 0 | 否 |
| [GenericPromptResult](GenericPromptResult.md) | 380 | 0 | 0 | 否 |
| [AudioVoiceParameters](AudioVoiceParameters.md) | 381 | 0 | 0 | 是 |
| [AudioVoiceState](AudioVoiceState.md) | 382 | 0 | 0 | 是 |
| [AudioWorldEvent](AudioWorldEvent.md) | 383 | 0 | 0 | 是 |
| [PickupRenderRNG](PickupRenderRNG.md) | 384 | 16 | 0 | 是 |
| [FamiliarRenderRNG](FamiliarRenderRNG.md) | 385 | 16 | 0 | 是 |
| [RenderRNGInitializationGuard](RenderRNGInitializationGuard.md) | 386 | 4 | 0 | 是 |
| [MTRNG](MTRNG.md) | 387 | 2500 | 0 | 是 |
| [GlobalRandomMTWords](GlobalRandomMTWords.md) | 388 | 2496 | 0 | 是 |
| [GlobalRandomMTIndex](GlobalRandomMTIndex.md) | 389 | 4 | 0 | 是 |
| [GlobalRandomMode](GlobalRandomMode.md) | 390 | 1 | 0 | 是 |
| [ItemPool_Item](ItemPool_Item.md) | 391 | 52 | 0 | 否 |
| [PoolItem](PoolItem.md) | 392 | 24 | 0 | 否 |
| [TrinketPoolItem](TrinketPoolItem.md) | 393 | 8 | 2 | 否 |
| [ItemPool](ItemPool.md) | 394 | 2096 | 259 | 否 |
| [BossPool_Entry](BossPool_Entry.md) | 395 | 20 | 0 | 否 |
| [BossPool_Pool](BossPool_Pool.md) | 396 | 60 | 0 | 否 |
| [BossPool](BossPool.md) | 397 | 2252 | 0 | 否 |
| [GameStateItemPool](GameStateItemPool.md) | 398 | 692 | 11 | 否 |
| [WeightedOutcomePicker](WeightedOutcomePicker.md) | 399 | 12 | 0 | 否 |
| [WeightedOutcomePicker_Outcome](WeightedOutcomePicker_Outcome.md) | 400 | 8 | 0 | 否 |
| [LootListEntry](LootListEntry.md) | 401 | 20 | 0 | 否 |
| [MsvcVectorBool32](MsvcVectorBool32.md) | 402 | 16 | 0 | 否 |
| [RepentogonMsvcMTModel](RepentogonMsvcMTModel.md) | 403 | 5000 | 0 | 否 |
| [LuaMathRandomPayload](LuaMathRandomPayload.md) | 404 | 32 | 0 | 否 |
| [CrtRandStateWord](CrtRandStateWord.md) | 405 | 4 | 0 | 否 |
| [SFXSampleSelectionRNG](SFXSampleSelectionRNG.md) | 406 | 16 | 0 | 是 |
| [SFXSeedSoundReplacementRNG](SFXSeedSoundReplacementRNG.md) | 407 | 16 | 0 | 是 |
| [DisplayTextRun](DisplayTextRun.md) | 422 | 0 | 0 | 是 |
| [FortuneText](FortuneText.md) | 423 | 0 | 0 | 是 |
| [LocalHistoryOverlay](LocalHistoryOverlay.md) | 424 | 0 | 0 | 是 |
| [PlayerHUDLocalClock](PlayerHUDLocalClock.md) | 435 | 0 | 0 | 是 |
| [HudMantleEffects](HudMantleEffects.md) | 436 | 24 | 2 | 否 |

公开目录：[run-catalog run域全部 191 行](run-catalog-run-members.md)。

公开目录：[stream-catalog presentation域全部 141 行](stream-catalog-presentation-members.md)。

公开目录：[stream-catalog random域全部 54 行](stream-catalog-random-members.md)。
