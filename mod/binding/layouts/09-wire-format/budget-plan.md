# 完整消息静态预算

布局版本 90；普通构建生成，仅整数公式，不运行编解码样例或测试。

| 合同 | 数值 |
|---|---|
| envelope_bytes | 40 |
| fragment_header_bytes | 64 |
| fragment_payload_bytes | 1136 |
| datagram_bytes | 1200 |
| bundle_header_bytes | 28 |
| bundle_entry_bytes | 22 |
| bundle_entries | 16 |
| ip_udp_bytes | 48 |
| link_bytes_per_second | 1250000 |
| host_utilization_percent | 90 |
| host_bytes_per_second | 1125000 |
| peer_bytes_per_second | 375000 |
| control_bytes_per_second | 25000 |
| world_bytes_per_second | 150000 |
| body_bytes_per_second | 150000 |
| reliable_bytes_per_second | 50000 |
| host_burst_bytes | 24576 |
| peer_burst_bytes | 8192 |
| control_burst_bytes | 2048 |
| world_burst_bytes | 4096 |
| body_burst_bytes | 4096 |
| reliable_burst_bytes | 8192 |
| world_hz | 30 |
| world_send_window | 8 |
| dynamic_send_bytes | 65536 |
| decode_snapshots | 8 |
| decode_failure_limit | 3 |
| capture_failure_limit | 30 |
| native_object_failure_limit | 3 |
| pool_object_missing_limit | 30 |
| body_hz | 60 |
| ordinary_message_bytes | 65536 |
| reliable_message_bytes | 33554432 |
| prepare_wire_bytes | 1048576 |
| prepare_send_ms | 8000 |
| prepare_total_ms | 10000 |
| progress_ms | 2000 |
| recovery_attempts | 2 |
| world_wait_ms | 100 |
| baseline_wait_ms | 10000 |
| body_send_ms | 50 |
| world_send_ms | 100 |
| body_stale_ms | 250 |
| world_stale_ms | 500 |
| ack_repeat_ms | 50 |
| resync_repeat_ms | 250 |
| sent_targets | 192 |
| sent_recovery_slots | 2 |
| sent_target_bytes | 67108864 |
| sent_age_ms | 500 |
| event_count | 4096 |
| event_bytes | 262144 |
| event_age_ms | 500 |
| result_cache_groups | 1024 |
| table_snapshots | 5 |
| peer_value_bytes | 134217728 |
| peer_wire_bytes | 25165824 |
| workspace_job_bytes | 100663296 |
| workspace_jobs | 1 |
| pool_value_bytes | 536870912 |
| pool_wire_bytes | 134217728 |
| pool_workspace_bytes | 134217728 |
| shared_value_bytes | 75497472 |
| shared_wire_bytes | 33554432 |
| world_body_zstd | 1 |
| zstd_level | 3 |
| zstd_window_bytes | 8388608 |

## 设计负载静态下界

不压缩、不计合包收益，IP／UDP按48B，三端各发一份。只计所列成员／资源；不含其余必要显示／资源／控制／重传，不是完整场景通过结论。数量是设计输入，不是原版硬上限或观察样本。

| 负载 | 正文B | 一端发出B | Hz | 一端B/s | 三端B/s |
|---|---|---|---|---|---|
| 空world | 104 | 256 | 30 | 7680 | 23040 |
| 空body | 104 | 256 | 60 | 15360 | 46080 |
| 四身体E1与P0标量下界 | 584 | 736 | 60 | 44160 | 132480 |
| 八身体E1与P0标量下界 | 1064 | 1216 | 60 | 72960 | 218880 |
| 八身体另带13Sprite动画投影下界 | 3352 | 3728 | 60 | 223680 | 671040 |
| 64颗Tear运动块 | 3688 | 4176 | 30 | 125280 | 375840 |
| 稠密运动设计负载 | 6376 | 7088 | 30 | 212640 | 637920 |
| 八身体全部body成员投影下界 | 18488 | 20432 | 0 | 0 | 0 |
| 四参与者八身体及稠密房间成员Baseline下界 | 124641 | 137001 | 0 | 0 | 0 |
| 上述成员表258项（另计） | 8574 | 9510 | 0 | 0 | 0 |
| 648项本地资源声明设计样例 | 41604 | 45788 | 0 | 0 | 0 |
| 单个100Quad纯值recipe资源 | 105 | 257 | 0 | 0 | 0 |
| 八身体独立HUD时钟完整消息设计下界 | 1464 | 1728 | 60 | 103680 | 311040 |

| 负载 | 组成 |
|---|---|
| 空world |  |
| 空body |  |
| 四身体E1与P0标量下界 | EntityPlayer×4 blocks=[1, 10] |
| 八身体E1与P0标量下界 | EntityPlayer×8 blocks=[1, 10] |
| 八身体另带13Sprite动画投影下界 | EntityPlayer×8 blocks=[1, 10, 20] |
| 64颗Tear运动块 | EntityTear×64 blocks=[1] |
| 稠密运动设计负载 | EntityTear×64 blocks=[1]; EntityProjectile×32 blocks=[1]; EntityNPC×16 blocks=[1] |
| 八身体全部body成员投影下界 | EntityPlayer×8 |
| 四参与者八身体及稠密房间成员Baseline下界 | PlayerParticipant×4; PlayerControlGroup×4; EntityPlayer×8; EntityTear×64; EntityProjectile×32; EntityNPC×16; GridEntity×128; Room×1; LevelGamePrefix×1 |
| 648项本地资源声明设计样例 | ResourcePath×100; ResourceImageFile×200; ResourceAnimation×100 + ResourceAnimationLayer×8/项; ResourceItemConfig×80; ResourceEntityConfig×64; RoomConfig_Room×20; ResourceBuiltinShader×20; ResourceAudio×64 |
| 单个100Quad纯值recipe资源 | SurfaceRecipeHeader×1 + SurfaceRecipeClear×1/项 + SurfaceRecipeQuad×100/项 |
| 八身体独立HUD时钟完整消息设计下界 | PlayerHUDLocalClock×8（具名结果组） + StateDependencyEntry×3/项 + StateContributionEntry×2/项 |

## 普通发送峰值的静态必要条件

只按不压缩、无其他排队竞争计算；是可行性的上限，不是每帧固定配额，也不是延迟保证。

| 序列 | 平均每次发出B | 可容纳正文B | 截止ms | 空桶可容纳正文B | 满通道突发桶可容纳正文B |
|---|---|---|---|---|---|
| world | 5000 | 4504 | 100 | 13592 | 17264 |
| body | 2500 | 2232 | 50 | 6776 | 10436 |

## 各类型整记录声明界限

仍不含公共帧／分段；上界只描述该类型record_limit，真正整帧必须再扣所有头／其他记录，条件下界不能视为完整合法样本。

| 类型 | 布局 | 正文下界B | 正文声明上界B |
|---|---|---|---|
| Vector | 1 | 8 | 8 |
| ColorMod | 2 | 44 | 44 |
| KColor | 3 | 20 | 20 |
| BlendMode | 4 | 20 | 20 |
| AnimationState | 5 | 11 | 524291 |
| LayerState | 6 | 119 | 119 |
| ANM2 | 7 | 147 | 8912907 |
| RNG | 8 | 0 | 0 |
| BitSet128 | 9 | 16 | 16 |
| EntityPtr | 10 | 2 | 2 |
| ColorParams | 11 | 0 | 0 |
| ActiveItemDesc | 12 | 24 | 24 |
| Weapon | 13 | 16 | 16 |
| TemporaryEffect | 14 | 10 | 10 |
| EntityPlus | 15 | 9 | 9 |
| EntityFamiliarPlus | 16 | 9 | 9 |
| EntityKnifePlus | 17 | 9 | 9 |
| FamiliarIsaacsBodyData | 18 | 751 | 44564551 |
| FamiliarAbyssLocustData | 19 | 12 | 805306368 |
| EntityTearPlus | 20 | 9 | 9 |
| EntityPickupPlus | 21 | 9 | 9 |
| EntityLaserPlus | 22 | 9 | 9 |
| HomingLaserBezier | 23 | 0 | 0 |
| HomingLaser | 24 | 4 | 262148 |
| EffectMrMeData | 25 | 147 | 8912907 |
| EffectTrailNode | 26 | 64 | 64 |
| EffectSpriteTrailData | 27 | 2 | 8130 |
| EffectPickupGhostData | 28 | 6 | 35651570 |
| Entity | 100 | 391 | 8913151 |
| EntityEffect | 101 | 420 | 8913180 |
| EntityEffect_TearFlags | 102 | 420 | 8913180 |
| EntityEffect_GridDesc | 103 | 420 | 8913180 |
| EntityEffect_MrMe | 104 | 568 | 17826088 |
| EntityEffect_SpriteTrail | 105 | 422 | 8921310 |
| EntityEffect_PickupGhost | 106 | 427 | 44564751 |
| EntityFamiliar | 120 | 610 | 17826130 |
| EntityKnife | 121 | 420 | 8913180 |
| EntityBomb | 122 | 1127 | 53477687 |
| EntityFamiliar_IsaacsBody | 123 | 616 | 62390687 |
| EntityFamiliar_AbyssLocust | 124 | 616 | 823132504 |
| EntityTear | 125 | 854 | 35651894 |
| EntityPickup | 126 | 699 | 26738979 |
| EntityLaser | 127 | 439 | 9175343 |
| EntitySlot | 128 | 538 | 17826058 |
| EntityProjectile | 129 | 404 | 8913164 |
| EntityFamiliar_UmbilicalBaby | 130 | 620 | 26935657 |
| EntityFamiliar_Incubus | 131 | 614 | 17826134 |
| UmbilicalRopeNode | 132 | 12 | 12 |
| FamiliarUmbilicalBabyData | 133 | 149 | 9109517 |
| EffectPickupGhostSprite | 134 | 131 | 8912891 |
| WeaponUmbilicalWhip | 135 | 18 | 18 |
| TemporaryEffects | 136 | 3 | 163843 |
| EntityEffect_Propagation | 137 | 420 | 8913180 |
| EntityEffect_Timer | 138 | 420 | 8913180 |
| EntityEffect_SpawnRequest | 139 | 420 | 8913180 |
| EntityEffect_DustCountdown | 140 | 420 | 8913180 |
| EntityEffect_TearFlags113 | 141 | 420 | 8913180 |
| EntityNPC | 142 | 710 | 27033902 |
| EntityNPC_Delirium | 143 | 710 | 27033902 |
| NpcOverlaySprite | 144 | 139 | 8912899 |
| EntityText | 145 | 409 | 8913424 |
| EntityText_Scaled | 146 | 413 | 8913428 |
| EntityDummy | 147 | 391 | 8913151 |
| EntityEffect_ChainLightning | 148 | 420 | 8913180 |
| EntityEffect_InternalTearFlags | 149 | 420 | 8913180 |
| PlayerTemporaryEffects | 150 | 3 | 163843 |
| PlayerCostumeMap | 151 | 7 | 7 |
| CostumeSpriteDesc | 152 | 151 | 8912911 |
| PocketItem | 153 | 8 | 8 |
| QueueItemData | 154 | 3 | 3 |
| SmeltedTrinketDesc | 155 | 4 | 4 |
| AfterImageFrame | 156 | 12 | 12 |
| History | 157 | 0 | 0 |
| HistoryItem | 158 | 25 | 25 |
| BagOfCraftingOutput | 159 | 0 | 0 |
| EntityDescHost | 160 | 0 | 0 |
| ConsumableData | 161 | 0 | 0 |
| ImageStrongPair | 162 | 0 | 0 |
| PlayerHUDStrongPair | 163 | 0 | 0 |
| OptionalDrawSequence | 164 | 1 | 8912932 |
| DrawRecordNode | 165 | 24 | 24 |
| SavedHistory | 166 | 0 | 0 |
| GameStatePlayer | 167 | 0 | 0 |
| FamiliarData | 168 | 0 | 0 |
| NativeCommunicationDisplay | 169 | 0 | 0 |
| ItemSpoof | 170 | 4 | 196612 |
| InnateItems | 171 | 0 | 0 |
| TemporaryInnateItem | 172 | 0 | 0 |
| PlayerItemSpoofs | 173 | 8 | 393224 |
| EntityPlayerPlus | 174 | 17 | 393233 |
| EntitySaveStateStrongPair | 175 | 0 | 0 |
| EntitySaveState | 176 | 0 | 0 |
| PlayerItemCount | 177 | 8 | 8 |
| PlayerInnateCount | 178 | 8 | 8 |
| PlayerActiveItemDesc | 179 | 24 | 24 |
| PlayerTemporaryEffect | 180 | 6 | 10 |
| PlayerWeaponTears | 181 | 16 | 16 |
| PlayerWeaponBrimstone | 182 | 16 | 16 |
| PlayerWeaponLaser | 183 | 16 | 16 |
| PlayerWeaponKnife | 184 | 16 | 16 |
| PlayerWeaponBombs | 185 | 4 | 4 |
| PlayerWeaponRockets | 186 | 4 | 4 |
| PlayerWeaponMonstrosLung | 187 | 16 | 16 |
| PlayerWeaponLudovico | 188 | 4 | 4 |
| PlayerWeaponTechX | 189 | 16 | 16 |
| PlayerWeaponBoneAxe | 190 | 16 | 16 |
| PlayerWeaponUrn | 191 | 4 | 4 |
| PlayerWeaponSpiritSword | 192 | 16 | 16 |
| PlayerWeaponFetus | 193 | 16 | 16 |
| PlayerWeaponWhip | 194 | 6 | 6 |
| PlayerParticipant | 195 | 0 | 0 |
| PlayerControlGroup | 196 | 21 | 149 |
| EntityPlayer | 197 | 2809 | 33554432 |
| GridEntityDesc | 198 | 16 | 16 |
| GridEntity | 199 | 142 | 8716297 |
| GridLayerState | 200 | 117 | 117 |
| GridSprite | 201 | 118 | 8716273 |
| GridFloorSprite | 202 | 74 | 8716229 |
| GridFireEmberAnimation | 203 | 3 | 3 |
| GridFireEmberSprite | 204 | 110 | 8191985 |
| GridFireEmber | 205 | 16 | 16 |
| GridEntityDecoration | 206 | 98 | 8716253 |
| GridEntityGravity | 207 | 98 | 8716253 |
| GridEntityStairs | 208 | 98 | 8716253 |
| GridEntityTeleporter | 209 | 142 | 8716297 |
| GridEntityWall | 210 | 142 | 8716297 |
| GridEntityWeb | 211 | 98 | 8716253 |
| GridEntityTrapDoor | 212 | 98 | 8716253 |
| GridEntityFire | 213 | 372 | 25624637 |
| GridRockLayerState | 214 | 110 | 118 |
| GridRockSprite | 215 | 108 | 8781809 |
| GridStatueUnusedSprite | 216 | 0 | 0 |
| GridEntityLock | 217 | 142 | 8716297 |
| GridEntityPit | 218 | 98 | 8716253 |
| GridEntitySpikes | 219 | 98 | 8716253 |
| GridEntityStatue | 220 | 24 | 24 |
| GridEntityTNT | 221 | 98 | 8716253 |
| GridEntityPoop | 222 | 98 | 8716253 |
| GridEntityRock | 223 | 136 | 9306117 |
| GridNonrandomLightDesc | 224 | 12 | 12 |
| GridDoorTint | 225 | 1 | 1 |
| GridDoorSprite | 226 | 75 | 8716230 |
| GridEntityPressurePlate | 227 | 212 | 17432522 |
| GridEntityDoor | 228 | 223 | 17432533 |
| GridEntityPlus | 229 | 0 | 0 |
| RunSeeds | 230 | 7 | 327 |
| RunAmbush | 231 | 0 | 0 |
| RunChallengeParam | 232 | 0 | 0 |
| RunDailyChallenge | 233 | 1 | 1 |
| RunScoreSheet | 234 | 0 | 0 |
| RunColorModState | 235 | 24 | 24 |
| RunGameStateFlags | 236 | 1 | 1 |
| GameRun | 237 | 422 | 675853 |
| RunSeedEffectNode | 238 | 4 | 4 |
| Room | 239 | 613 | 33554432 |
| Backdrop | 240 | 21 | 35651649 |
| BackdropConfig | 241 | 0 | 0 |
| FXLayers | 242 | 0 | 0 |
| FXParams | 243 | 0 | 0 |
| FXPose | 244 | 10 | 10 |
| FX | 245 | 15 | 163855 |
| Ray | 246 | 10 | 10 |
| RayGroup | 247 | 30 | 40990 |
| Vein | 248 | 35 | 35 |
| RailManager | 249 | 138 | 8389066 |
| Camera | 250 | 0 | 0 |
| CameraRing | 251 | 0 | 0 |
| RoomShockwave | 252 | 28 | 28 |
| RoomHellBackdrop | 253 | 1 | 8912916 |
| EntityList | 254 | 0 | 0 |
| EntityListEL | 255 | 0 | 0 |
| RoomTemporaryEffects | 256 | 0 | 0 |
| FXPoop | 257 | 0 | 0 |
| RoomOutputSetHeader | 258 | 0 | 0 |
| RoomCallbackVector | 259 | 0 | 0 |
| RainSpawnRegion | 260 | 0 | 0 |
| OriginalOnlineEntityFrame | 261 | 0 | 0 |
| KilledNPCRecord | 262 | 0 | 0 |
| RoomConfig_Room | 263 | 15 | 15 |
| RoomConfig_Stage | 264 | 0 | 0 |
| RoomSet | 265 | 0 | 0 |
| RoomSpawn | 266 | 0 | 0 |
| RoomEntry | 267 | 0 | 0 |
| DestinationQuad | 268 | 0 | 0 |
| SourceQuad | 269 | 0 | 0 |
| RoomDescriptor | 270 | 55 | 70 |
| RoomDescriptorSavedPickupMapInput | 271 | 0 | 0 |
| RoomDescriptorSavedGridMapInput | 272 | 0 | 0 |
| RoomDescriptorShopKinds | 273 | 0 | 0 |
| RoomDescriptorRestockCounts | 274 | 0 | 0 |
| RoomDescriptorRestrictedSetHeader | 275 | 0 | 0 |
| LevelGamePrefix | 276 | 1036 | 1036 |
| LevelRoomOffsetEntry | 277 | 0 | 0 |
| LevelTemporaryCurseField | 278 | 0 | 0 |
| LevelASM | 279 | 0 | 0 |
| LevelCustomStageNameABI | 280 | 0 | 0 |
| RoomVeinSprite | 281 | 91 | 8912851 |
| RoomRailSprite | 282 | 136 | 8388616 |
| BackdropBindingEntry | 283 | 0 | 0 |
| BackdropBinding | 284 | 0 | 0 |
| WaterShaderResult | 285 | 0 | 0 |
| WaterVisualPayload | 286 | 0 | 0 |
| WaterVisual | 287 | 0 | 0 |
| RoomLightingResult | 288 | 0 | 0 |
| RoomWaveResult | 289 | 0 | 0 |
| ForcedCameraFocus | 290 | 0 | 0 |
| SelectedRayGroup | 291 | 0 | 0 |
| PoopVisualPayload | 292 | 0 | 0 |
| PoopVisual | 293 | 0 | 0 |
| VeinGraphPayload | 294 | 0 | 0 |
| VeinGraphVisual | 295 | 0 | 0 |
| FXScenePayload | 296 | 0 | 0 |
| SavedMapIconPayload | 297 | 2 | 11 |
| FinalSavedMapIcons | 298 | 1 | 12 |
| CurrentDescriptorVisual | 299 | 0 | 0 |
| MapFocusResult | 300 | 0 | 0 |
| MapFocusPair | 301 | 0 | 0 |
| MapBadgeList | 302 | 0 | 0 |
| MinimapQuestionImageBinding | 303 | 0 | 0 |
| ResourceEntryHeader | 304 | 8 | 8 |
| SurfacePixelsDescriptor | 305 | 0 | 0 |
| ResourceBlobChunkHeader | 306 | 0 | 0 |
| SurfaceRecipeHeader | 307 | 16 | 16 |
| SurfaceRecipeClear | 308 | 21 | 21 |
| SurfaceRecipeQuad | 309 | 0 | 0 |
| PlayerHUDHeart | 310 | 0 | 0 |
| PlayerHUDActiveItem | 311 | 0 | 0 |
| PlayerHUDTrinket | 312 | 0 | 0 |
| PlayerHUDPocket | 313 | 0 | 0 |
| InventoryVisual | 314 | 0 | 0 |
| PlayerHUD | 315 | 0 | 0 |
| StatHUDValue | 316 | 0 | 0 |
| StatHUDPlayer | 317 | 0 | 0 |
| StatHUD | 318 | 0 | 0 |
| HUDMessage | 319 | 0 | 0 |
| HistoryHUDItem | 320 | 0 | 0 |
| HistoryHUDPlayer | 321 | 0 | 0 |
| HistoryHUD | 322 | 0 | 0 |
| HUD | 323 | 0 | 0 |
| MinimapConfig | 324 | 0 | 0 |
| Minimap | 325 | 0 | 0 |
| ColorModState | 326 | 0 | 0 |
| GenericPrompt | 327 | 0 | 0 |
| ItemOverlayDefinition | 328 | 0 | 0 |
| ItemOverlay | 329 | 0 | 0 |
| MusicStream | 330 | 0 | 0 |
| MusicStreamSet | 331 | 0 | 0 |
| MusicEntry | 332 | 0 | 0 |
| MusicLayerDefinition | 333 | 0 | 0 |
| Music | 334 | 0 | 0 |
| SoundSample | 335 | 0 | 0 |
| SoundEffect | 336 | 0 | 0 |
| SoundEffects | 337 | 0 | 0 |
| Font | 338 | 0 | 0 |
| FontSettings | 339 | 0 | 0 |
| ImageBase | 340 | 0 | 0 |
| RoomTransition | 341 | 0 | 0 |
| StageTransition | 342 | 0 | 0 |
| NightmareScene | 343 | 0 | 0 |
| GameOver | 344 | 0 | 0 |
| PauseScreen | 345 | 0 | 0 |
| GameScreenEffects | 346 | 0 | 0 |
| GameFadeView | 347 | 0 | 0 |
| GameColorView | 348 | 0 | 0 |
| NightmareSnapshotQueue | 349 | 0 | 0 |
| NightmareSnapshot | 350 | 0 | 0 |
| BossOverlayDefinition | 351 | 0 | 0 |
| BossOverlay | 352 | 0 | 0 |
| VisiblePlayerHUDHeart | 353 | 0 | 0 |
| StatHUDRows | 354 | 0 | 0 |
| MapDisplaySprite | 355 | 0 | 0 |
| PresentationDrawClock | 356 | 0 | 0 |
| PickupRandomAlphaResult | 357 | 0 | 0 |
| FamiliarRandomAlphaResult | 358 | 0 | 0 |
| PillDisplayResult | 359 | 0 | 0 |
| PickupPreviewItem | 360 | 0 | 0 |
| PickupPreviewResult | 361 | 0 | 0 |
| PlayerDrawImageBindings | 362 | 0 | 0 |
| HudBodySelection | 363 | 0 | 0 |
| ControlGroupHudSelection | 364 | 0 | 0 |
| PresentationSpriteContribution | 365 | 0 | 0 |
| PresentationImageContribution | 366 | 0 | 0 |
| ScreenBloomResult | 367 | 0 | 0 |
| ScreenPixelationResult | 368 | 0 | 0 |
| ScreenRetrovisionResult | 369 | 0 | 0 |
| ScreenHallucinationResult | 370 | 0 | 0 |
| RoomGlitchResult | 371 | 0 | 0 |
| ScreenFadeContribution | 372 | 0 | 0 |
| ScreenEffectsResult | 373 | 0 | 0 |
| BossVersusResult | 374 | 0 | 0 |
| NightmareSceneResult | 375 | 0 | 0 |
| GameOverResult | 376 | 0 | 0 |
| StageTransitionResult | 377 | 0 | 0 |
| ItemOverlayResult | 378 | 0 | 0 |
| BossOverlayResult | 379 | 0 | 0 |
| GenericPromptResult | 380 | 0 | 0 |
| AudioVoiceParameters | 381 | 0 | 0 |
| AudioVoiceState | 382 | 0 | 0 |
| AudioWorldEvent | 383 | 0 | 0 |
| PickupRenderRNG | 384 | 0 | 0 |
| FamiliarRenderRNG | 385 | 0 | 0 |
| RenderRNGInitializationGuard | 386 | 0 | 0 |
| MTRNG | 387 | 0 | 0 |
| GlobalRandomMTWords | 388 | 0 | 0 |
| GlobalRandomMTIndex | 389 | 0 | 0 |
| GlobalRandomMode | 390 | 0 | 0 |
| ItemPool_Item | 391 | 0 | 0 |
| PoolItem | 392 | 0 | 0 |
| TrinketPoolItem | 393 | 0 | 0 |
| ItemPool | 394 | 0 | 0 |
| BossPool_Entry | 395 | 0 | 0 |
| BossPool_Pool | 396 | 0 | 0 |
| BossPool | 397 | 0 | 0 |
| GameStateItemPool | 398 | 0 | 0 |
| WeightedOutcomePicker | 399 | 0 | 0 |
| WeightedOutcomePicker_Outcome | 400 | 0 | 0 |
| LootListEntry | 401 | 0 | 0 |
| MsvcVectorBool32 | 402 | 0 | 0 |
| RepentogonMsvcMTModel | 403 | 0 | 0 |
| LuaMathRandomPayload | 404 | 0 | 0 |
| CrtRandStateWord | 405 | 0 | 0 |
| SFXSampleSelectionRNG | 406 | 0 | 0 |
| SFXSeedSoundReplacementRNG | 407 | 0 | 0 |
| StateCommonHeader | 408 | 50 | 50 |
| StateFrameHeader | 409 | 2 | 2 |
| StateSectionHeader | 410 | 52 | 52 |
| StateRecordHeader | 411 | 12 | 12 |
| StateTableHeader | 412 | 10 | 10 |
| StateMemberEntry | 413 | 33 | 33 |
| StateResultGroupHeader | 414 | 42 | 42 |
| StateDependencyEntry | 415 | 32 | 32 |
| StateContributionEntry | 416 | 16 | 16 |
| StateAudioEventHeader | 417 | 40 | 40 |
| StateAppliedAckHeader | 419 | 2 | 2 |
| StateAppliedAckEntry | 420 | 21 | 21 |
| StateResyncHeader | 421 | 11 | 11 |
| DisplayTextRun | 422 | 0 | 0 |
| FortuneText | 423 | 0 | 0 |
| LocalHistoryOverlay | 424 | 0 | 0 |
| ResourcePath | 425 | 2 | 4098 |
| ResourceImageFile | 426 | 10 | 4106 |
| ResourceBuiltinShader | 427 | 1 | 1 |
| ResourceAnimation | 428 | 12 | 5132 |
| ResourceName | 429 | 2 | 2050 |
| ResourceAnimationLayer | 430 | 6 | 6 |
| ResourceItemConfig | 431 | 16 | 16 |
| ResourceEntityConfig | 432 | 22 | 22 |
| ResourceAudio | 433 | 4 | 8198 |
| RoomWaveSprite | 434 | 133 | 8323078 |
| PlayerHUDLocalClock | 435 | 0 | 0 |
| HudMantleEffects | 436 | 0 | 0 |
| SFXVolumeState | 437 | 4 | 4 |
| DisplayInjectionState | 438 | 176 | 176 |
| MusicRequestState | 439 | 57 | 57 |
| LoopSoundState | 440 | 38 | 38 |
| LoopSoundSet | 441 | 10 | 19466 |
| OverlayDisplayState | 442 | 320 | 131072 |
| OverlayDisplaySet | 443 | 2 | 131074 |
| ProceduralDisplayDefinition | 444 | 12 | 12300 |
| ProceduralDisplaySet | 445 | 6 | 524292 |
| StateEventQueueHeader | 446 | 22 | 22 |
| StateEventHeader | 447 | 22 | 22 |
| SfxPlayEvent | 448 | 22 | 22 |
| ItemBannerEvent | 449 | 13 | 16397 |
| FortuneLine | 450 | 2 | 4098 |
| FortuneEvent | 451 | 8 | 131272 |
| BossIntroEvent | 452 | 24 | 24 |
| RoomTransitionEvent | 453 | 43 | 43 |
| BossDisplayState | 454 | 26 | 323 |
| BossDisplayPose | 455 | 9 | 9 |

## 容量边界单价

| 对象 | 正文B | 不压缩一端发出B |
|---|---|---|
| 单项接收 ACK | 73 | 225 |
| joint AppliedAck | 94 | 246 |
| Resync | 61 | 213 |
| 最大成员表 | 2162715 | 2376003 |
| 4096条最大19B音频事件区域 | 241664 | 265560 |
| 普通完整消息保护 | 65496 | 72032 |
| 可靠完整消息保护 | 33554392 | 36862688 |

## 逐块声明上下界

下界可能要求不同条件同时缺席，不能直接宣称可构成一个合法完整对象。上界受记录／整消息／字段访问预算共同约束，不代表允许按此大小持续发送。

| 类型 | 布局／块 | 序列 | 下界B | 声明上界B |
|---|---|---|---|---|
| Vector | 1/0 | body | 8 | 8 |
| ColorMod | 2/0 | body | 44 | 44 |
| KColor | 3/0 | body | 20 | 20 |
| BlendMode | 4/0 | body | 20 | 20 |
| AnimationState | 5/0 | body | 2 | 2 |
| AnimationState | 5/1 | body | 4 | 524284 |
| AnimationState | 5/2 | body | 5 | 5 |
| AnimationState | 5/3 | body | 0 | 0 |
| LayerState | 6/0 | body | 4 | 4 |
| LayerState | 6/1 | body | 115 | 115 |
| ANM2 | 7/0 | body | 123 | 65658 |
| ANM2 | 7/1 | body | 22 | 1048582 |
| ANM2 | 7/2 | body | 2 | 7798667 |
| BitSet128 | 9/0 | body | 16 | 16 |
| EntityPtr | 10/0 | body | 2 | 2 |
| ColorParams | 11/0 | body | 0 | 0 |
| ActiveItemDesc | 12/0 | body | 24 | 24 |
| Weapon | 13/0 | body | 16 | 16 |
| TemporaryEffect | 14/0 | body | 10 | 10 |
| EntityPlus | 15/0 | world | 9 | 9 |
| EntityFamiliarPlus | 16/0 | world | 9 | 9 |
| EntityKnifePlus | 17/0 | world | 9 | 9 |
| EntityKnifePlus | 17/1 | world | 0 | 0 |
| FamiliarIsaacsBodyData | 18/0 | world | 631 | 328306 |
| FamiliarIsaacsBodyData | 18/1 | world | 110 | 5242910 |
| FamiliarIsaacsBodyData | 18/2 | world | 10 | 38993335 |
| FamiliarAbyssLocustData | 19/0 | world | 4 | 268435456 |
| FamiliarAbyssLocustData | 19/1 | world | 4 | 268435456 |
| FamiliarAbyssLocustData | 19/2 | world | 4 | 268435456 |
| EntityTearPlus | 20/0 | world | 9 | 9 |
| EntityPickupPlus | 21/0 | world | 9 | 9 |
| EntityLaserPlus | 22/0 | world | 9 | 9 |
| HomingLaserBezier | 23/0 | world | 0 | 0 |
| HomingLaser | 24/0 | world | 0 | 0 |
| HomingLaser | 24/1 | world | 2 | 131074 |
| HomingLaser | 24/2 | world | 2 | 131074 |
| HomingLaser | 24/3 | world | 0 | 0 |
| HomingLaser | 24/4 | world | 0 | 0 |
| EffectMrMeData | 25/0 | world | 123 | 65658 |
| EffectMrMeData | 25/1 | world | 22 | 1048582 |
| EffectMrMeData | 25/2 | world | 2 | 7798667 |
| EffectTrailNode | 26/0 | world | 64 | 64 |
| EffectSpriteTrailData | 27/0 | world | 0 | 0 |
| EffectSpriteTrailData | 27/1 | world | 2 | 8130 |
| EffectPickupGhostData | 28/0 | world | 2 | 262570 |
| EffectPickupGhostData | 28/1 | world | 2 | 4194330 |
| EffectPickupGhostData | 28/2 | world | 2 | 31194670 |
| Entity | 100/0 | world | 34 | 34 |
| Entity | 100/1 | world | 44 | 44 |
| Entity | 100/2 | world | 35 | 35 |
| Entity | 100/3 | world | 8 | 8 |
| Entity | 100/4 | world | 227 | 65762 |
| Entity | 100/5 | world | 22 | 1048582 |
| Entity | 100/6 | world | 2 | 7798667 |
| Entity | 100/7 | world | 0 | 0 |
| Entity | 100/8 | world | 10 | 10 |
| Entity | 100/9 | world | 9 | 9 |
| EntityEffect | 101/0 | world | 34 | 34 |
| EntityEffect | 101/1 | world | 44 | 44 |
| EntityEffect | 101/2 | world | 35 | 35 |
| EntityEffect | 101/3 | world | 8 | 8 |
| EntityEffect | 101/4 | world | 227 | 65762 |
| EntityEffect | 101/5 | world | 22 | 1048582 |
| EntityEffect | 101/6 | world | 2 | 7798667 |
| EntityEffect | 101/7 | world | 0 | 0 |
| EntityEffect | 101/8 | world | 10 | 10 |
| EntityEffect | 101/9 | world | 9 | 9 |
| EntityEffect | 101/10 | world | 16 | 16 |
| EntityEffect | 101/11 | world | 13 | 13 |
| EntityEffect | 101/12 | world | 0 | 0 |
| EntityEffect_TearFlags | 102/0 | world | 34 | 34 |
| EntityEffect_TearFlags | 102/1 | world | 44 | 44 |
| EntityEffect_TearFlags | 102/2 | world | 35 | 35 |
| EntityEffect_TearFlags | 102/3 | world | 8 | 8 |
| EntityEffect_TearFlags | 102/4 | world | 227 | 65762 |
| EntityEffect_TearFlags | 102/5 | world | 22 | 1048582 |
| EntityEffect_TearFlags | 102/6 | world | 2 | 7798667 |
| EntityEffect_TearFlags | 102/7 | world | 0 | 0 |
| EntityEffect_TearFlags | 102/8 | world | 10 | 10 |
| EntityEffect_TearFlags | 102/9 | world | 9 | 9 |
| EntityEffect_TearFlags | 102/10 | world | 16 | 16 |
| EntityEffect_TearFlags | 102/11 | world | 13 | 13 |
| EntityEffect_TearFlags | 102/12 | world | 0 | 0 |
| EntityEffect_GridDesc | 103/0 | world | 34 | 34 |
| EntityEffect_GridDesc | 103/1 | world | 44 | 44 |
| EntityEffect_GridDesc | 103/2 | world | 35 | 35 |
| EntityEffect_GridDesc | 103/3 | world | 8 | 8 |
| EntityEffect_GridDesc | 103/4 | world | 227 | 65762 |
| EntityEffect_GridDesc | 103/5 | world | 22 | 1048582 |
| EntityEffect_GridDesc | 103/6 | world | 2 | 7798667 |
| EntityEffect_GridDesc | 103/7 | world | 0 | 0 |
| EntityEffect_GridDesc | 103/8 | world | 10 | 10 |
| EntityEffect_GridDesc | 103/9 | world | 9 | 9 |
| EntityEffect_GridDesc | 103/10 | world | 16 | 16 |
| EntityEffect_GridDesc | 103/11 | world | 13 | 13 |
| EntityEffect_GridDesc | 103/12 | world | 0 | 0 |
| EntityEffect_MrMe | 104/0 | world | 34 | 34 |
| EntityEffect_MrMe | 104/1 | world | 44 | 44 |
| EntityEffect_MrMe | 104/2 | world | 35 | 35 |
| EntityEffect_MrMe | 104/3 | world | 8 | 8 |
| EntityEffect_MrMe | 104/4 | world | 227 | 65762 |
| EntityEffect_MrMe | 104/5 | world | 22 | 1048582 |
| EntityEffect_MrMe | 104/6 | world | 2 | 7798667 |
| EntityEffect_MrMe | 104/7 | world | 0 | 0 |
| EntityEffect_MrMe | 104/8 | world | 10 | 10 |
| EntityEffect_MrMe | 104/9 | world | 9 | 9 |
| EntityEffect_MrMe | 104/10 | world | 16 | 16 |
| EntityEffect_MrMe | 104/11 | world | 13 | 13 |
| EntityEffect_MrMe | 104/12 | world | 0 | 0 |
| EntityEffect_MrMe | 104/13 | world | 124 | 65659 |
| EntityEffect_MrMe | 104/14 | world | 22 | 1048582 |
| EntityEffect_MrMe | 104/15 | world | 2 | 7798667 |
| EntityEffect_SpriteTrail | 105/0 | world | 34 | 34 |
| EntityEffect_SpriteTrail | 105/1 | world | 44 | 44 |
| EntityEffect_SpriteTrail | 105/2 | world | 35 | 35 |
| EntityEffect_SpriteTrail | 105/3 | world | 8 | 8 |
| EntityEffect_SpriteTrail | 105/4 | world | 227 | 65762 |
| EntityEffect_SpriteTrail | 105/5 | world | 22 | 1048582 |
| EntityEffect_SpriteTrail | 105/6 | world | 2 | 7798667 |
| EntityEffect_SpriteTrail | 105/7 | world | 0 | 0 |
| EntityEffect_SpriteTrail | 105/8 | world | 10 | 10 |
| EntityEffect_SpriteTrail | 105/9 | world | 9 | 9 |
| EntityEffect_SpriteTrail | 105/10 | world | 16 | 16 |
| EntityEffect_SpriteTrail | 105/11 | world | 13 | 13 |
| EntityEffect_SpriteTrail | 105/12 | world | 0 | 0 |
| EntityEffect_SpriteTrail | 105/13 | world | 0 | 0 |
| EntityEffect_SpriteTrail | 105/14 | world | 2 | 8130 |
| EntityEffect_PickupGhost | 106/0 | world | 34 | 34 |
| EntityEffect_PickupGhost | 106/1 | world | 44 | 44 |
| EntityEffect_PickupGhost | 106/2 | world | 35 | 35 |
| EntityEffect_PickupGhost | 106/3 | world | 8 | 8 |
| EntityEffect_PickupGhost | 106/4 | world | 227 | 65762 |
| EntityEffect_PickupGhost | 106/5 | world | 22 | 1048582 |
| EntityEffect_PickupGhost | 106/6 | world | 2 | 7798667 |
| EntityEffect_PickupGhost | 106/7 | world | 0 | 0 |
| EntityEffect_PickupGhost | 106/8 | world | 10 | 10 |
| EntityEffect_PickupGhost | 106/9 | world | 9 | 9 |
| EntityEffect_PickupGhost | 106/10 | world | 16 | 16 |
| EntityEffect_PickupGhost | 106/11 | world | 13 | 13 |
| EntityEffect_PickupGhost | 106/12 | world | 0 | 0 |
| EntityEffect_PickupGhost | 106/13 | world | 3 | 262571 |
| EntityEffect_PickupGhost | 106/14 | world | 2 | 4194330 |
| EntityEffect_PickupGhost | 106/15 | world | 2 | 31194670 |
| EntityFamiliar | 120/0 | world | 34 | 34 |
| EntityFamiliar | 120/1 | world | 44 | 44 |
| EntityFamiliar | 120/2 | world | 35 | 35 |
| EntityFamiliar | 120/3 | world | 8 | 8 |
| EntityFamiliar | 120/4 | world | 227 | 65762 |
| EntityFamiliar | 120/5 | world | 22 | 1048582 |
| EntityFamiliar | 120/6 | world | 2 | 7798667 |
| EntityFamiliar | 120/7 | world | 0 | 0 |
| EntityFamiliar | 120/8 | world | 10 | 10 |
| EntityFamiliar | 120/9 | world | 9 | 9 |
| EntityFamiliar | 120/10 | world | 8 | 8 |
| EntityFamiliar | 120/11 | world | 16 | 16 |
| EntityFamiliar | 120/12 | world | 4 | 4 |
| EntityFamiliar | 120/13 | world | 44 | 44 |
| EntityFamiliar | 120/14 | world | 123 | 65658 |
| EntityFamiliar | 120/15 | world | 22 | 1048582 |
| EntityFamiliar | 120/16 | world | 2 | 7798667 |
| EntityKnife | 121/0 | world | 34 | 34 |
| EntityKnife | 121/1 | world | 44 | 44 |
| EntityKnife | 121/2 | world | 35 | 35 |
| EntityKnife | 121/3 | world | 8 | 8 |
| EntityKnife | 121/4 | world | 227 | 65762 |
| EntityKnife | 121/5 | world | 22 | 1048582 |
| EntityKnife | 121/6 | world | 2 | 7798667 |
| EntityKnife | 121/7 | world | 0 | 0 |
| EntityKnife | 121/8 | world | 10 | 10 |
| EntityKnife | 121/9 | world | 9 | 9 |
| EntityKnife | 121/10 | world | 9 | 9 |
| EntityKnife | 121/11 | world | 20 | 20 |
| EntityKnife | 121/12 | world | 0 | 0 |
| EntityKnife | 121/13 | world | 0 | 0 |
| EntityKnife | 121/14 | world | 0 | 0 |
| EntityBomb | 122/0 | world | 34 | 34 |
| EntityBomb | 122/1 | world | 44 | 44 |
| EntityBomb | 122/2 | world | 35 | 35 |
| EntityBomb | 122/3 | world | 8 | 8 |
| EntityBomb | 122/4 | world | 227 | 65762 |
| EntityBomb | 122/5 | world | 22 | 1048582 |
| EntityBomb | 122/6 | world | 2 | 7798667 |
| EntityBomb | 122/7 | world | 0 | 0 |
| EntityBomb | 122/8 | world | 10 | 10 |
| EntityBomb | 122/9 | world | 9 | 9 |
| EntityBomb | 122/10 | world | 0 | 0 |
| EntityBomb | 122/11 | world | 1 | 1 |
| EntityBomb | 122/12 | world | 615 | 328290 |
| EntityBomb | 122/13 | world | 110 | 5242910 |
| EntityBomb | 122/14 | world | 10 | 38993335 |
| EntityBomb | 122/15 | world | 0 | 0 |
| EntityFamiliar_IsaacsBody | 123/0 | world | 34 | 34 |
| EntityFamiliar_IsaacsBody | 123/1 | world | 44 | 44 |
| EntityFamiliar_IsaacsBody | 123/2 | world | 35 | 35 |
| EntityFamiliar_IsaacsBody | 123/3 | world | 8 | 8 |
| EntityFamiliar_IsaacsBody | 123/4 | world | 227 | 65762 |
| EntityFamiliar_IsaacsBody | 123/5 | world | 22 | 1048582 |
| EntityFamiliar_IsaacsBody | 123/6 | world | 2 | 7798667 |
| EntityFamiliar_IsaacsBody | 123/7 | world | 0 | 0 |
| EntityFamiliar_IsaacsBody | 123/8 | world | 10 | 10 |
| EntityFamiliar_IsaacsBody | 123/9 | world | 9 | 9 |
| EntityFamiliar_IsaacsBody | 123/10 | world | 8 | 8 |
| EntityFamiliar_IsaacsBody | 123/11 | world | 16 | 16 |
| EntityFamiliar_IsaacsBody | 123/12 | world | 4 | 4 |
| EntityFamiliar_IsaacsBody | 123/13 | world | 44 | 44 |
| EntityFamiliar_IsaacsBody | 123/14 | world | 123 | 65658 |
| EntityFamiliar_IsaacsBody | 123/15 | world | 22 | 1048582 |
| EntityFamiliar_IsaacsBody | 123/16 | world | 2 | 7798667 |
| EntityFamiliar_IsaacsBody | 123/17 | world | 2 | 328308 |
| EntityFamiliar_IsaacsBody | 123/18 | world | 2 | 5242912 |
| EntityFamiliar_IsaacsBody | 123/19 | world | 2 | 38993337 |
| EntityFamiliar_AbyssLocust | 124/0 | world | 34 | 34 |
| EntityFamiliar_AbyssLocust | 124/1 | world | 44 | 44 |
| EntityFamiliar_AbyssLocust | 124/2 | world | 35 | 35 |
| EntityFamiliar_AbyssLocust | 124/3 | world | 8 | 8 |
| EntityFamiliar_AbyssLocust | 124/4 | world | 227 | 65762 |
| EntityFamiliar_AbyssLocust | 124/5 | world | 22 | 1048582 |
| EntityFamiliar_AbyssLocust | 124/6 | world | 2 | 7798667 |
| EntityFamiliar_AbyssLocust | 124/7 | world | 0 | 0 |
| EntityFamiliar_AbyssLocust | 124/8 | world | 10 | 10 |
| EntityFamiliar_AbyssLocust | 124/9 | world | 9 | 9 |
| EntityFamiliar_AbyssLocust | 124/10 | world | 8 | 8 |
| EntityFamiliar_AbyssLocust | 124/11 | world | 16 | 16 |
| EntityFamiliar_AbyssLocust | 124/12 | world | 4 | 4 |
| EntityFamiliar_AbyssLocust | 124/13 | world | 44 | 44 |
| EntityFamiliar_AbyssLocust | 124/14 | world | 123 | 65658 |
| EntityFamiliar_AbyssLocust | 124/15 | world | 22 | 1048582 |
| EntityFamiliar_AbyssLocust | 124/16 | world | 2 | 7798667 |
| EntityFamiliar_AbyssLocust | 124/17 | world | 2 | 268435458 |
| EntityFamiliar_AbyssLocust | 124/18 | world | 2 | 268435458 |
| EntityFamiliar_AbyssLocust | 124/19 | world | 2 | 268435458 |
| EntityTear | 125/0 | world | 34 | 34 |
| EntityTear | 125/1 | world | 44 | 44 |
| EntityTear | 125/2 | world | 35 | 35 |
| EntityTear | 125/3 | world | 8 | 8 |
| EntityTear | 125/4 | world | 227 | 65762 |
| EntityTear | 125/5 | world | 22 | 1048582 |
| EntityTear | 125/6 | world | 2 | 7798667 |
| EntityTear | 125/7 | world | 0 | 0 |
| EntityTear | 125/8 | world | 10 | 10 |
| EntityTear | 125/9 | world | 9 | 9 |
| EntityTear | 125/10 | world | 0 | 0 |
| EntityTear | 125/11 | world | 20 | 20 |
| EntityTear | 125/12 | world | 0 | 0 |
| EntityTear | 125/13 | world | 2 | 2 |
| EntityTear | 125/14 | world | 0 | 0 |
| EntityTear | 125/15 | world | 369 | 196974 |
| EntityTear | 125/16 | world | 66 | 3145746 |
| EntityTear | 125/17 | world | 6 | 23396001 |
| EntityPickup | 126/0 | world | 34 | 34 |
| EntityPickup | 126/1 | world | 44 | 44 |
| EntityPickup | 126/2 | world | 35 | 35 |
| EntityPickup | 126/3 | world | 8 | 8 |
| EntityPickup | 126/4 | world | 227 | 65762 |
| EntityPickup | 126/5 | world | 22 | 1048582 |
| EntityPickup | 126/6 | world | 2 | 7798667 |
| EntityPickup | 126/7 | world | 0 | 0 |
| EntityPickup | 126/8 | world | 10 | 10 |
| EntityPickup | 126/9 | world | 9 | 9 |
| EntityPickup | 126/10 | world | 4 | 4 |
| EntityPickup | 126/11 | world | 4 | 4 |
| EntityPickup | 126/12 | world | 6 | 6 |
| EntityPickup | 126/13 | world | 0 | 0 |
| EntityPickup | 126/14 | world | 246 | 131316 |
| EntityPickup | 126/15 | world | 44 | 2097164 |
| EntityPickup | 126/16 | world | 4 | 15597334 |
| EntityLaser | 127/0 | world | 34 | 34 |
| EntityLaser | 127/1 | world | 44 | 44 |
| EntityLaser | 127/2 | world | 35 | 35 |
| EntityLaser | 127/3 | world | 8 | 8 |
| EntityLaser | 127/4 | world | 227 | 65762 |
| EntityLaser | 127/5 | world | 22 | 1048582 |
| EntityLaser | 127/6 | world | 2 | 7798667 |
| EntityLaser | 127/7 | world | 0 | 0 |
| EntityLaser | 127/8 | world | 10 | 10 |
| EntityLaser | 127/9 | world | 9 | 9 |
| EntityLaser | 127/10 | world | 16 | 16 |
| EntityLaser | 127/11 | world | 26 | 26 |
| EntityLaser | 127/12 | world | 0 | 0 |
| EntityLaser | 127/13 | world | 2 | 2 |
| EntityLaser | 127/14 | world | 0 | 0 |
| EntityLaser | 127/15 | world | 2 | 131074 |
| EntityLaser | 127/16 | world | 2 | 131074 |
| EntityLaser | 127/17 | world | 0 | 0 |
| EntityLaser | 127/18 | world | 0 | 0 |
| EntityLaser | 127/19 | world | 0 | 0 |
| EntitySlot | 128/0 | world | 34 | 34 |
| EntitySlot | 128/1 | world | 44 | 44 |
| EntitySlot | 128/2 | world | 35 | 35 |
| EntitySlot | 128/3 | world | 8 | 8 |
| EntitySlot | 128/4 | world | 227 | 65762 |
| EntitySlot | 128/5 | world | 22 | 1048582 |
| EntitySlot | 128/6 | world | 2 | 7798667 |
| EntitySlot | 128/7 | world | 0 | 0 |
| EntitySlot | 128/8 | world | 10 | 10 |
| EntitySlot | 128/9 | world | 9 | 9 |
| EntitySlot | 128/10 | world | 0 | 0 |
| EntitySlot | 128/11 | world | 0 | 0 |
| EntitySlot | 128/12 | world | 123 | 65658 |
| EntitySlot | 128/13 | world | 22 | 1048582 |
| EntitySlot | 128/14 | world | 2 | 7798667 |
| EntityProjectile | 129/0 | world | 34 | 34 |
| EntityProjectile | 129/1 | world | 44 | 44 |
| EntityProjectile | 129/2 | world | 35 | 35 |
| EntityProjectile | 129/3 | world | 8 | 8 |
| EntityProjectile | 129/4 | world | 227 | 65762 |
| EntityProjectile | 129/5 | world | 22 | 1048582 |
| EntityProjectile | 129/6 | world | 2 | 7798667 |
| EntityProjectile | 129/7 | world | 0 | 0 |
| EntityProjectile | 129/8 | world | 10 | 10 |
| EntityProjectile | 129/9 | world | 9 | 9 |
| EntityProjectile | 129/10 | world | 4 | 4 |
| EntityProjectile | 129/11 | world | 0 | 0 |
| EntityProjectile | 129/12 | world | 9 | 9 |
| EntityFamiliar_UmbilicalBaby | 130/0 | world | 34 | 34 |
| EntityFamiliar_UmbilicalBaby | 130/1 | world | 44 | 44 |
| EntityFamiliar_UmbilicalBaby | 130/2 | world | 35 | 35 |
| EntityFamiliar_UmbilicalBaby | 130/3 | world | 8 | 8 |
| EntityFamiliar_UmbilicalBaby | 130/4 | world | 227 | 65762 |
| EntityFamiliar_UmbilicalBaby | 130/5 | world | 22 | 1048582 |
| EntityFamiliar_UmbilicalBaby | 130/6 | world | 2 | 7798667 |
| EntityFamiliar_UmbilicalBaby | 130/7 | world | 0 | 0 |
| EntityFamiliar_UmbilicalBaby | 130/8 | world | 10 | 10 |
| EntityFamiliar_UmbilicalBaby | 130/9 | world | 9 | 9 |
| EntityFamiliar_UmbilicalBaby | 130/10 | world | 8 | 8 |
| EntityFamiliar_UmbilicalBaby | 130/11 | world | 16 | 16 |
| EntityFamiliar_UmbilicalBaby | 130/12 | world | 6 | 6 |
| EntityFamiliar_UmbilicalBaby | 130/13 | world | 44 | 44 |
| EntityFamiliar_UmbilicalBaby | 130/14 | world | 123 | 65658 |
| EntityFamiliar_UmbilicalBaby | 130/15 | world | 22 | 1048582 |
| EntityFamiliar_UmbilicalBaby | 130/16 | world | 2 | 7798667 |
| EntityFamiliar_UmbilicalBaby | 130/17 | world | 2 | 65660 |
| EntityFamiliar_UmbilicalBaby | 130/18 | world | 2 | 1048584 |
| EntityFamiliar_UmbilicalBaby | 130/19 | world | 2 | 7798669 |
| EntityFamiliar_UmbilicalBaby | 130/20 | world | 2 | 196612 |
| EntityFamiliar_Incubus | 131/0 | world | 34 | 34 |
| EntityFamiliar_Incubus | 131/1 | world | 44 | 44 |
| EntityFamiliar_Incubus | 131/2 | world | 35 | 35 |
| EntityFamiliar_Incubus | 131/3 | world | 8 | 8 |
| EntityFamiliar_Incubus | 131/4 | world | 227 | 65762 |
| EntityFamiliar_Incubus | 131/5 | world | 22 | 1048582 |
| EntityFamiliar_Incubus | 131/6 | world | 2 | 7798667 |
| EntityFamiliar_Incubus | 131/7 | world | 0 | 0 |
| EntityFamiliar_Incubus | 131/8 | world | 10 | 10 |
| EntityFamiliar_Incubus | 131/9 | world | 9 | 9 |
| EntityFamiliar_Incubus | 131/10 | world | 8 | 8 |
| EntityFamiliar_Incubus | 131/11 | world | 16 | 16 |
| EntityFamiliar_Incubus | 131/12 | world | 4 | 4 |
| EntityFamiliar_Incubus | 131/13 | world | 44 | 44 |
| EntityFamiliar_Incubus | 131/14 | world | 123 | 65658 |
| EntityFamiliar_Incubus | 131/15 | world | 22 | 1048582 |
| EntityFamiliar_Incubus | 131/16 | world | 2 | 7798667 |
| EntityFamiliar_Incubus | 131/17 | world | 4 | 4 |
| UmbilicalRopeNode | 132/0 | world | 12 | 12 |
| FamiliarUmbilicalBabyData | 133/0 | world | 123 | 65658 |
| FamiliarUmbilicalBabyData | 133/1 | world | 22 | 1048582 |
| FamiliarUmbilicalBabyData | 133/2 | world | 2 | 7798667 |
| FamiliarUmbilicalBabyData | 133/3 | world | 2 | 196610 |
| EffectPickupGhostSprite | 134/0 | body | 107 | 65642 |
| EffectPickupGhostSprite | 134/1 | body | 22 | 1048582 |
| EffectPickupGhostSprite | 134/2 | body | 2 | 7798667 |
| WeaponUmbilicalWhip | 135/0 | body | 16 | 16 |
| WeaponUmbilicalWhip | 135/1 | body | 2 | 2 |
| TemporaryEffects | 136/0 | body | 1 | 1 |
| TemporaryEffects | 136/1 | body | 2 | 163842 |
| EntityEffect_Propagation | 137/0 | world | 34 | 34 |
| EntityEffect_Propagation | 137/1 | world | 44 | 44 |
| EntityEffect_Propagation | 137/2 | world | 35 | 35 |
| EntityEffect_Propagation | 137/3 | world | 8 | 8 |
| EntityEffect_Propagation | 137/4 | world | 227 | 65762 |
| EntityEffect_Propagation | 137/5 | world | 22 | 1048582 |
| EntityEffect_Propagation | 137/6 | world | 2 | 7798667 |
| EntityEffect_Propagation | 137/7 | world | 0 | 0 |
| EntityEffect_Propagation | 137/8 | world | 10 | 10 |
| EntityEffect_Propagation | 137/9 | world | 9 | 9 |
| EntityEffect_Propagation | 137/10 | world | 16 | 16 |
| EntityEffect_Propagation | 137/11 | world | 13 | 13 |
| EntityEffect_Propagation | 137/12 | world | 0 | 0 |
| EntityEffect_Timer | 138/0 | world | 34 | 34 |
| EntityEffect_Timer | 138/1 | world | 44 | 44 |
| EntityEffect_Timer | 138/2 | world | 35 | 35 |
| EntityEffect_Timer | 138/3 | world | 8 | 8 |
| EntityEffect_Timer | 138/4 | world | 227 | 65762 |
| EntityEffect_Timer | 138/5 | world | 22 | 1048582 |
| EntityEffect_Timer | 138/6 | world | 2 | 7798667 |
| EntityEffect_Timer | 138/7 | world | 0 | 0 |
| EntityEffect_Timer | 138/8 | world | 10 | 10 |
| EntityEffect_Timer | 138/9 | world | 9 | 9 |
| EntityEffect_Timer | 138/10 | world | 16 | 16 |
| EntityEffect_Timer | 138/11 | world | 13 | 13 |
| EntityEffect_Timer | 138/12 | world | 0 | 0 |
| EntityEffect_SpawnRequest | 139/0 | world | 34 | 34 |
| EntityEffect_SpawnRequest | 139/1 | world | 44 | 44 |
| EntityEffect_SpawnRequest | 139/2 | world | 35 | 35 |
| EntityEffect_SpawnRequest | 139/3 | world | 8 | 8 |
| EntityEffect_SpawnRequest | 139/4 | world | 227 | 65762 |
| EntityEffect_SpawnRequest | 139/5 | world | 22 | 1048582 |
| EntityEffect_SpawnRequest | 139/6 | world | 2 | 7798667 |
| EntityEffect_SpawnRequest | 139/7 | world | 0 | 0 |
| EntityEffect_SpawnRequest | 139/8 | world | 10 | 10 |
| EntityEffect_SpawnRequest | 139/9 | world | 9 | 9 |
| EntityEffect_SpawnRequest | 139/10 | world | 16 | 16 |
| EntityEffect_SpawnRequest | 139/11 | world | 13 | 13 |
| EntityEffect_SpawnRequest | 139/12 | world | 0 | 0 |
| EntityEffect_DustCountdown | 140/0 | world | 34 | 34 |
| EntityEffect_DustCountdown | 140/1 | world | 44 | 44 |
| EntityEffect_DustCountdown | 140/2 | world | 35 | 35 |
| EntityEffect_DustCountdown | 140/3 | world | 8 | 8 |
| EntityEffect_DustCountdown | 140/4 | world | 227 | 65762 |
| EntityEffect_DustCountdown | 140/5 | world | 22 | 1048582 |
| EntityEffect_DustCountdown | 140/6 | world | 2 | 7798667 |
| EntityEffect_DustCountdown | 140/7 | world | 0 | 0 |
| EntityEffect_DustCountdown | 140/8 | world | 10 | 10 |
| EntityEffect_DustCountdown | 140/9 | world | 9 | 9 |
| EntityEffect_DustCountdown | 140/10 | world | 16 | 16 |
| EntityEffect_DustCountdown | 140/11 | world | 13 | 13 |
| EntityEffect_DustCountdown | 140/12 | world | 0 | 0 |
| EntityEffect_TearFlags113 | 141/0 | world | 34 | 34 |
| EntityEffect_TearFlags113 | 141/1 | world | 44 | 44 |
| EntityEffect_TearFlags113 | 141/2 | world | 35 | 35 |
| EntityEffect_TearFlags113 | 141/3 | world | 8 | 8 |
| EntityEffect_TearFlags113 | 141/4 | world | 227 | 65762 |
| EntityEffect_TearFlags113 | 141/5 | world | 22 | 1048582 |
| EntityEffect_TearFlags113 | 141/6 | world | 2 | 7798667 |
| EntityEffect_TearFlags113 | 141/7 | world | 0 | 0 |
| EntityEffect_TearFlags113 | 141/8 | world | 10 | 10 |
| EntityEffect_TearFlags113 | 141/9 | world | 9 | 9 |
| EntityEffect_TearFlags113 | 141/10 | world | 16 | 16 |
| EntityEffect_TearFlags113 | 141/11 | world | 13 | 13 |
| EntityEffect_TearFlags113 | 141/12 | world | 0 | 0 |
| EntityNPC | 142/0 | world | 34 | 34 |
| EntityNPC | 142/1 | world | 44 | 44 |
| EntityNPC | 142/2 | world | 35 | 35 |
| EntityNPC | 142/3 | world | 8 | 8 |
| EntityNPC | 142/4 | world | 227 | 65762 |
| EntityNPC | 142/5 | world | 22 | 1048582 |
| EntityNPC | 142/6 | world | 2 | 7798667 |
| EntityNPC | 142/7 | world | 0 | 0 |
| EntityNPC | 142/8 | world | 10 | 10 |
| EntityNPC | 142/9 | world | 9 | 9 |
| EntityNPC | 142/10 | world | 25 | 25 |
| EntityNPC | 142/11 | world | 4 | 32772 |
| EntityNPC | 142/12 | world | 2 | 131074 |
| EntityNPC | 142/13 | world | 2 | 131074 |
| EntityNPC | 142/14 | world | 238 | 131308 |
| EntityNPC | 142/15 | world | 44 | 2097164 |
| EntityNPC | 142/16 | world | 4 | 15597334 |
| EntityNPC | 142/17 | world | 0 | 0 |
| EntityNPC | 142/18 | world | 0 | 0 |
| EntityNPC_Delirium | 143/0 | world | 34 | 34 |
| EntityNPC_Delirium | 143/1 | world | 44 | 44 |
| EntityNPC_Delirium | 143/2 | world | 35 | 35 |
| EntityNPC_Delirium | 143/3 | world | 8 | 8 |
| EntityNPC_Delirium | 143/4 | world | 227 | 65762 |
| EntityNPC_Delirium | 143/5 | world | 22 | 1048582 |
| EntityNPC_Delirium | 143/6 | world | 2 | 7798667 |
| EntityNPC_Delirium | 143/7 | world | 0 | 0 |
| EntityNPC_Delirium | 143/8 | world | 10 | 10 |
| EntityNPC_Delirium | 143/9 | world | 9 | 9 |
| EntityNPC_Delirium | 143/10 | world | 25 | 25 |
| EntityNPC_Delirium | 143/11 | world | 4 | 32772 |
| EntityNPC_Delirium | 143/12 | world | 2 | 131074 |
| EntityNPC_Delirium | 143/13 | world | 2 | 131074 |
| EntityNPC_Delirium | 143/14 | world | 238 | 131308 |
| EntityNPC_Delirium | 143/15 | world | 44 | 2097164 |
| EntityNPC_Delirium | 143/16 | world | 4 | 15597334 |
| EntityNPC_Delirium | 143/17 | world | 0 | 0 |
| EntityNPC_Delirium | 143/18 | world | 0 | 0 |
| NpcOverlaySprite | 144/0 | body | 115 | 65650 |
| NpcOverlaySprite | 144/1 | body | 22 | 1048582 |
| NpcOverlaySprite | 144/2 | body | 2 | 7798667 |
| EntityText | 145/0 | world | 34 | 34 |
| EntityText | 145/1 | world | 44 | 44 |
| EntityText | 145/2 | world | 35 | 35 |
| EntityText | 145/3 | world | 8 | 8 |
| EntityText | 145/4 | world | 227 | 65762 |
| EntityText | 145/5 | world | 22 | 1048582 |
| EntityText | 145/6 | world | 2 | 7798667 |
| EntityText | 145/7 | world | 0 | 0 |
| EntityText | 145/8 | world | 10 | 10 |
| EntityText | 145/9 | world | 9 | 9 |
| EntityText | 145/10 | world | 16 | 16 |
| EntityText | 145/11 | world | 2 | 257 |
| EntityText | 145/12 | world | 0 | 0 |
| EntityText_Scaled | 146/0 | world | 34 | 34 |
| EntityText_Scaled | 146/1 | world | 44 | 44 |
| EntityText_Scaled | 146/2 | world | 35 | 35 |
| EntityText_Scaled | 146/3 | world | 8 | 8 |
| EntityText_Scaled | 146/4 | world | 227 | 65762 |
| EntityText_Scaled | 146/5 | world | 22 | 1048582 |
| EntityText_Scaled | 146/6 | world | 2 | 7798667 |
| EntityText_Scaled | 146/7 | world | 0 | 0 |
| EntityText_Scaled | 146/8 | world | 10 | 10 |
| EntityText_Scaled | 146/9 | world | 9 | 9 |
| EntityText_Scaled | 146/10 | world | 16 | 16 |
| EntityText_Scaled | 146/11 | world | 2 | 257 |
| EntityText_Scaled | 146/12 | world | 4 | 4 |
| EntityDummy | 147/0 | world | 34 | 34 |
| EntityDummy | 147/1 | world | 44 | 44 |
| EntityDummy | 147/2 | world | 35 | 35 |
| EntityDummy | 147/3 | world | 8 | 8 |
| EntityDummy | 147/4 | world | 227 | 65762 |
| EntityDummy | 147/5 | world | 22 | 1048582 |
| EntityDummy | 147/6 | world | 2 | 7798667 |
| EntityDummy | 147/7 | world | 0 | 0 |
| EntityDummy | 147/8 | world | 10 | 10 |
| EntityDummy | 147/9 | world | 9 | 9 |
| EntityEffect_ChainLightning | 148/0 | world | 34 | 34 |
| EntityEffect_ChainLightning | 148/1 | world | 44 | 44 |
| EntityEffect_ChainLightning | 148/2 | world | 35 | 35 |
| EntityEffect_ChainLightning | 148/3 | world | 8 | 8 |
| EntityEffect_ChainLightning | 148/4 | world | 227 | 65762 |
| EntityEffect_ChainLightning | 148/5 | world | 22 | 1048582 |
| EntityEffect_ChainLightning | 148/6 | world | 2 | 7798667 |
| EntityEffect_ChainLightning | 148/7 | world | 0 | 0 |
| EntityEffect_ChainLightning | 148/8 | world | 10 | 10 |
| EntityEffect_ChainLightning | 148/9 | world | 9 | 9 |
| EntityEffect_ChainLightning | 148/10 | world | 16 | 16 |
| EntityEffect_ChainLightning | 148/11 | world | 13 | 13 |
| EntityEffect_ChainLightning | 148/12 | world | 0 | 0 |
| EntityEffect_InternalTearFlags | 149/0 | world | 34 | 34 |
| EntityEffect_InternalTearFlags | 149/1 | world | 44 | 44 |
| EntityEffect_InternalTearFlags | 149/2 | world | 35 | 35 |
| EntityEffect_InternalTearFlags | 149/3 | world | 8 | 8 |
| EntityEffect_InternalTearFlags | 149/4 | world | 227 | 65762 |
| EntityEffect_InternalTearFlags | 149/5 | world | 22 | 1048582 |
| EntityEffect_InternalTearFlags | 149/6 | world | 2 | 7798667 |
| EntityEffect_InternalTearFlags | 149/7 | world | 0 | 0 |
| EntityEffect_InternalTearFlags | 149/8 | world | 10 | 10 |
| EntityEffect_InternalTearFlags | 149/9 | world | 9 | 9 |
| EntityEffect_InternalTearFlags | 149/10 | world | 16 | 16 |
| EntityEffect_InternalTearFlags | 149/11 | world | 13 | 13 |
| EntityEffect_InternalTearFlags | 149/12 | world | 0 | 0 |
| PlayerTemporaryEffects | 150/0 | world | 3 | 163843 |
| PlayerCostumeMap | 151/0 | world | 7 | 7 |
| CostumeSpriteDesc | 152/0 | world | 4 | 4 |
| CostumeSpriteDesc | 152/1 | body | 123 | 65658 |
| CostumeSpriteDesc | 152/2 | body | 22 | 1048582 |
| CostumeSpriteDesc | 152/3 | body | 2 | 7798667 |
| PocketItem | 153/0 | world | 8 | 8 |
| QueueItemData | 154/0 | world | 3 | 3 |
| SmeltedTrinketDesc | 155/0 | world | 4 | 4 |
| AfterImageFrame | 156/0 | world | 12 | 12 |
| HistoryItem | 158/0 | world | 25 | 25 |
| OptionalDrawSequence | 164/0 | world | 1 | 8912932 |
| DrawRecordNode | 165/0 | world | 24 | 24 |
| ItemSpoof | 170/0 | world | 4 | 196612 |
| PlayerItemSpoofs | 173/0 | world | 8 | 393224 |
| EntityPlayerPlus | 174/0 | world | 9 | 9 |
| EntityPlayerPlus | 174/1 | world | 8 | 393224 |
| PlayerItemCount | 177/0 | world | 8 | 8 |
| PlayerInnateCount | 178/0 | world | 8 | 8 |
| PlayerActiveItemDesc | 179/0 | world | 24 | 24 |
| PlayerTemporaryEffect | 180/0 | world | 6 | 10 |
| PlayerWeaponTears | 181/0 | world | 16 | 16 |
| PlayerWeaponBrimstone | 182/0 | world | 16 | 16 |
| PlayerWeaponLaser | 183/0 | world | 16 | 16 |
| PlayerWeaponKnife | 184/0 | world | 16 | 16 |
| PlayerWeaponBombs | 185/0 | world | 4 | 4 |
| PlayerWeaponRockets | 186/0 | world | 4 | 4 |
| PlayerWeaponMonstrosLung | 187/0 | world | 16 | 16 |
| PlayerWeaponLudovico | 188/0 | world | 4 | 4 |
| PlayerWeaponTechX | 189/0 | world | 16 | 16 |
| PlayerWeaponBoneAxe | 190/0 | world | 16 | 16 |
| PlayerWeaponUrn | 191/0 | world | 4 | 4 |
| PlayerWeaponSpiritSword | 192/0 | world | 16 | 16 |
| PlayerWeaponFetus | 193/0 | world | 16 | 16 |
| PlayerWeaponWhip | 194/0 | world | 6 | 6 |
| PlayerControlGroup | 196/0 | world | 21 | 149 |
| EntityPlayer | 197/0 | world | 34 | 34 |
| EntityPlayer | 197/1 | body | 44 | 44 |
| EntityPlayer | 197/2 | world | 35 | 35 |
| EntityPlayer | 197/3 | world | 8 | 8 |
| EntityPlayer | 197/4 | body | 227 | 65762 |
| EntityPlayer | 197/5 | body | 22 | 1048582 |
| EntityPlayer | 197/6 | body | 2 | 7798667 |
| EntityPlayer | 197/7 | world | 0 | 0 |
| EntityPlayer | 197/8 | world | 10 | 10 |
| EntityPlayer | 197/9 | world | 9 | 9 |
| EntityPlayer | 197/10 | body | 64 | 2112 |
| EntityPlayer | 197/11 | world | 25 | 25 |
| EntityPlayer | 197/12 | world | 77 | 77 |
| EntityPlayer | 197/13 | world | 90 | 790618 |
| EntityPlayer | 197/14 | world | 96 | 96 |
| EntityPlayer | 197/15 | world | 1 | 91 |
| EntityPlayer | 197/16 | world | 111 | 65647 |
| EntityPlayer | 197/17 | world | 21 | 21 |
| EntityPlayer | 197/18 | world | 6 | 9273385 |
| EntityPlayer | 197/19 | body | 1599 | 853554 |
| EntityPlayer | 197/20 | body | 286 | 13631566 |
| EntityPlayer | 197/21 | body | 26 | 33554432 |
| EntityPlayer | 197/22 | body | 2 | 33554432 |
| EntityPlayer | 197/23 | body | 2 | 33554432 |
| EntityPlayer | 197/24 | body | 2 | 33554432 |
| EntityPlayer | 197/25 | body | 10 | 10 |
| GridEntityDesc | 198/0 | world | 16 | 16 |
| GridEntity | 199/0 | world | 24 | 24 |
| GridEntity | 199/1 | world | 94 | 94 |
| GridEntity | 199/2 | world | 22 | 1048582 |
| GridEntity | 199/3 | world | 2 | 7667597 |
| GridLayerState | 200/0 | body | 2 | 2 |
| GridLayerState | 200/1 | body | 115 | 115 |
| GridSprite | 201/0 | body | 94 | 94 |
| GridSprite | 201/1 | body | 22 | 1048582 |
| GridSprite | 201/2 | body | 2 | 7667597 |
| GridFloorSprite | 202/0 | body | 50 | 50 |
| GridFloorSprite | 202/1 | body | 22 | 1048582 |
| GridFloorSprite | 202/2 | body | 2 | 7667597 |
| GridFireEmberAnimation | 203/0 | body | 3 | 3 |
| GridFireEmberSprite | 204/0 | body | 94 | 94 |
| GridFireEmberSprite | 204/1 | body | 14 | 524294 |
| GridFireEmberSprite | 204/2 | body | 2 | 7667597 |
| GridFireEmber | 205/0 | world | 16 | 16 |
| GridEntityDecoration | 206/0 | world | 24 | 24 |
| GridEntityDecoration | 206/1 | world | 50 | 50 |
| GridEntityDecoration | 206/2 | world | 22 | 1048582 |
| GridEntityDecoration | 206/3 | world | 2 | 7667597 |
| GridEntityGravity | 207/0 | world | 24 | 24 |
| GridEntityGravity | 207/1 | world | 50 | 50 |
| GridEntityGravity | 207/2 | world | 22 | 1048582 |
| GridEntityGravity | 207/3 | world | 2 | 7667597 |
| GridEntityStairs | 208/0 | world | 24 | 24 |
| GridEntityStairs | 208/1 | world | 50 | 50 |
| GridEntityStairs | 208/2 | world | 22 | 1048582 |
| GridEntityStairs | 208/3 | world | 2 | 7667597 |
| GridEntityTeleporter | 209/0 | world | 24 | 24 |
| GridEntityTeleporter | 209/1 | world | 94 | 94 |
| GridEntityTeleporter | 209/2 | world | 22 | 1048582 |
| GridEntityTeleporter | 209/3 | world | 2 | 7667597 |
| GridEntityWall | 210/0 | world | 24 | 24 |
| GridEntityWall | 210/1 | world | 94 | 94 |
| GridEntityWall | 210/2 | world | 22 | 1048582 |
| GridEntityWall | 210/3 | world | 2 | 7667597 |
| GridEntityWeb | 211/0 | world | 24 | 24 |
| GridEntityWeb | 211/1 | world | 50 | 50 |
| GridEntityWeb | 211/2 | world | 22 | 1048582 |
| GridEntityWeb | 211/3 | world | 2 | 7667597 |
| GridEntityTrapDoor | 212/0 | world | 24 | 24 |
| GridEntityTrapDoor | 212/1 | world | 50 | 50 |
| GridEntityTrapDoor | 212/2 | world | 22 | 1048582 |
| GridEntityTrapDoor | 212/3 | world | 2 | 7667597 |
| GridEntityFire | 213/0 | world | 24 | 24 |
| GridEntityFire | 213/1 | world | 94 | 94 |
| GridEntityFire | 213/2 | world | 22 | 1048582 |
| GridEntityFire | 213/3 | world | 2 | 7667597 |
| GridEntityFire | 213/4 | world | 94 | 94 |
| GridEntityFire | 213/5 | world | 22 | 1048582 |
| GridEntityFire | 213/6 | world | 2 | 7667597 |
| GridEntityFire | 213/7 | world | 94 | 94 |
| GridEntityFire | 213/8 | world | 14 | 524294 |
| GridEntityFire | 213/9 | world | 2 | 7667597 |
| GridEntityFire | 213/10 | world | 2 | 82 |
| GridRockLayerState | 214/0 | body | 2 | 2 |
| GridRockLayerState | 214/1 | body | 108 | 116 |
| GridRockSprite | 215/0 | body | 94 | 94 |
| GridRockSprite | 215/1 | body | 12 | 1048583 |
| GridRockSprite | 215/2 | body | 2 | 7733132 |
| GridStatueUnusedSprite | 216/0 | body | 0 | 0 |
| GridStatueUnusedSprite | 216/1 | body | 0 | 0 |
| GridStatueUnusedSprite | 216/2 | body | 0 | 0 |
| GridEntityLock | 217/0 | world | 24 | 24 |
| GridEntityLock | 217/1 | world | 94 | 94 |
| GridEntityLock | 217/2 | world | 22 | 1048582 |
| GridEntityLock | 217/3 | world | 2 | 7667597 |
| GridEntityPit | 218/0 | world | 24 | 24 |
| GridEntityPit | 218/1 | world | 50 | 50 |
| GridEntityPit | 218/2 | world | 22 | 1048582 |
| GridEntityPit | 218/3 | world | 2 | 7667597 |
| GridEntitySpikes | 219/0 | world | 24 | 24 |
| GridEntitySpikes | 219/1 | world | 50 | 50 |
| GridEntitySpikes | 219/2 | world | 22 | 1048582 |
| GridEntitySpikes | 219/3 | world | 2 | 7667597 |
| GridEntityStatue | 220/0 | world | 24 | 24 |
| GridEntityStatue | 220/1 | world | 0 | 0 |
| GridEntityStatue | 220/2 | world | 0 | 0 |
| GridEntityStatue | 220/3 | world | 0 | 0 |
| GridEntityTNT | 221/0 | world | 24 | 24 |
| GridEntityTNT | 221/1 | world | 50 | 50 |
| GridEntityTNT | 221/2 | world | 22 | 1048582 |
| GridEntityTNT | 221/3 | world | 2 | 7667597 |
| GridEntityPoop | 222/0 | world | 24 | 24 |
| GridEntityPoop | 222/1 | world | 50 | 50 |
| GridEntityPoop | 222/2 | world | 22 | 1048582 |
| GridEntityPoop | 222/3 | world | 2 | 7667597 |
| GridEntityRock | 223/0 | world | 24 | 24 |
| GridEntityRock | 223/1 | world | 94 | 94 |
| GridEntityRock | 223/2 | world | 12 | 1048583 |
| GridEntityRock | 223/3 | world | 2 | 7733132 |
| GridEntityRock | 223/4 | world | 4 | 524284 |
| GridNonrandomLightDesc | 224/0 | world | 12 | 12 |
| GridDoorTint | 225/0 | body | 1 | 1 |
| GridDoorSprite | 226/0 | body | 51 | 51 |
| GridDoorSprite | 226/1 | body | 22 | 1048582 |
| GridDoorSprite | 226/2 | body | 2 | 7667597 |
| GridEntityPressurePlate | 227/0 | world | 20 | 20 |
| GridEntityPressurePlate | 227/1 | world | 50 | 50 |
| GridEntityPressurePlate | 227/2 | world | 22 | 1048582 |
| GridEntityPressurePlate | 227/3 | world | 2 | 7667597 |
| GridEntityPressurePlate | 227/4 | world | 94 | 94 |
| GridEntityPressurePlate | 227/5 | world | 22 | 1048582 |
| GridEntityPressurePlate | 227/6 | world | 2 | 7667597 |
| GridEntityDoor | 228/0 | world | 20 | 20 |
| GridEntityDoor | 228/1 | world | 51 | 51 |
| GridEntityDoor | 228/2 | world | 22 | 1048582 |
| GridEntityDoor | 228/3 | world | 2 | 7667597 |
| GridEntityDoor | 228/4 | world | 10 | 10 |
| GridEntityDoor | 228/5 | world | 94 | 94 |
| GridEntityDoor | 228/6 | world | 22 | 1048582 |
| GridEntityDoor | 228/7 | world | 2 | 7667597 |
| RunSeeds | 230/0 | world | 5 | 5 |
| RunSeeds | 230/1 | world | 2 | 322 |
| RunDailyChallenge | 233/0 | world | 1 | 1 |
| RunScoreSheet | 234/0 | world | 0 | 0 |
| RunColorModState | 235/0 | world | 24 | 24 |
| RunGameStateFlags | 236/0 | world | 1 | 1 |
| GameRun | 237/0 | world | 21 | 341 |
| GameRun | 237/1 | world | 40 | 40 |
| GameRun | 237/2 | world | 0 | 0 |
| GameRun | 237/3 | world | 41 | 41 |
| GameRun | 237/4 | world | 37 | 37 |
| GameRun | 237/5 | world | 6 | 6 |
| GameRun | 237/6 | world | 176 | 176 |
| GameRun | 237/7 | world | 57 | 57 |
| GameRun | 237/8 | world | 10 | 19466 |
| GameRun | 237/9 | world | 2 | 131074 |
| GameRun | 237/10 | world | 6 | 524292 |
| GameRun | 237/11 | world | 26 | 323 |
| RunSeedEffectNode | 238/0 | world | 4 | 4 |
| Room | 239/0 | world | 36 | 36 |
| Room | 239/1 | world | 8 | 8 |
| Room | 239/2 | world | 104 | 104 |
| Room | 239/3 | world | 48 | 48 |
| Room | 239/4 | world | 21 | 35651649 |
| Room | 239/5 | world | 0 | 0 |
| Room | 239/6 | world | 0 | 0 |
| Room | 239/7 | world | 0 | 0 |
| Room | 239/8 | world | 0 | 0 |
| Room | 239/9 | world | 0 | 0 |
| Room | 239/10 | world | 138 | 8389066 |
| Room | 239/11 | world | 1 | 8912916 |
| Room | 239/12 | world | 1 | 8323079 |
| Room | 239/13 | world | 144 | 144 |
| Room | 239/14 | world | 112 | 112 |
| Backdrop | 240/0 | world | 21 | 35651649 |
| FXLayers | 242/0 | world | 0 | 0 |
| FXPose | 244/0 | world | 10 | 10 |
| FX | 245/0 | world | 15 | 163855 |
| Ray | 246/0 | world | 10 | 10 |
| RayGroup | 247/0 | world | 30 | 40990 |
| Vein | 248/0 | world | 35 | 35 |
| RailManager | 249/0 | world | 138 | 8389066 |
| RoomShockwave | 252/0 | world | 28 | 28 |
| RoomHellBackdrop | 253/0 | world | 1 | 8912916 |
| RoomConfig_Room | 263/0 | world | 15 | 15 |
| RoomDescriptor | 270/0 | world | 54 | 58 |
| RoomDescriptor | 270/1 | world | 1 | 12 |
| LevelGamePrefix | 276/0 | world | 18 | 18 |
| LevelGamePrefix | 276/1 | world | 1014 | 1014 |
| LevelGamePrefix | 276/2 | world | 0 | 0 |
| LevelGamePrefix | 276/3 | world | 4 | 4 |
| RoomVeinSprite | 281/0 | body | 67 | 65602 |
| RoomVeinSprite | 281/1 | body | 22 | 1048582 |
| RoomVeinSprite | 281/2 | body | 2 | 7798667 |
| RoomRailSprite | 282/0 | body | 123 | 65658 |
| RoomRailSprite | 282/1 | body | 11 | 524291 |
| RoomRailSprite | 282/2 | body | 2 | 7798667 |
| BackdropBindingEntry | 283/0 | world | 0 | 0 |
| BackdropBinding | 284/0 | world | 0 | 0 |
| WaterShaderResult | 285/0 | world | 0 | 0 |
| WaterVisualPayload | 286/0 | world | 0 | 0 |
| WaterVisual | 287/0 | world | 0 | 0 |
| RoomLightingResult | 288/0 | world | 0 | 0 |
| RoomWaveResult | 289/0 | world | 0 | 0 |
| ForcedCameraFocus | 290/0 | world | 0 | 0 |
| SelectedRayGroup | 291/0 | world | 0 | 0 |
| PoopVisualPayload | 292/0 | world | 0 | 0 |
| PoopVisual | 293/0 | world | 0 | 0 |
| VeinGraphPayload | 294/0 | world | 0 | 0 |
| VeinGraphVisual | 295/0 | world | 0 | 0 |
| FXScenePayload | 296/0 | world | 0 | 0 |
| SavedMapIconPayload | 297/0 | world | 2 | 11 |
| FinalSavedMapIcons | 298/0 | world | 1 | 12 |
| CurrentDescriptorVisual | 299/0 | world | 0 | 0 |
| MapFocusResult | 300/0 | world | 0 | 0 |
| MapFocusPair | 301/0 | world | 0 | 0 |
| MapBadgeList | 302/0 | world | 0 | 0 |
| MinimapQuestionImageBinding | 303/0 | world | 0 | 0 |
| ResourceEntryHeader | 304/0 | world | 8 | 8 |
| SurfacePixelsDescriptor | 305/0 | world | 0 | 0 |
| ResourceBlobChunkHeader | 306/0 | world | 0 | 0 |
| SurfaceRecipeHeader | 307/0 | world | 16 | 16 |
| SurfaceRecipeClear | 308/0 | world | 21 | 21 |
| SurfaceRecipeQuad | 309/0 | world | 0 | 0 |
| PlayerHUDHeart | 310/0 | world | 0 | 0 |
| PlayerHUDActiveItem | 311/0 | world | 0 | 0 |
| PlayerHUDTrinket | 312/0 | world | 0 | 0 |
| PlayerHUDPocket | 313/0 | world | 0 | 0 |
| InventoryVisual | 314/0 | world | 0 | 0 |
| PlayerHUD | 315/0 | body | 0 | 0 |
| StatHUDValue | 316/0 | world | 0 | 0 |
| StatHUDPlayer | 317/0 | body | 0 | 0 |
| StatHUD | 318/0 | world | 0 | 0 |
| HUDMessage | 319/0 | world | 0 | 0 |
| HistoryHUDItem | 320/0 | world | 0 | 0 |
| HistoryHUDPlayer | 321/0 | body | 0 | 0 |
| HUD | 323/0 | world | 0 | 0 |
| MinimapConfig | 324/0 | world | 0 | 0 |
| Minimap | 325/0 | world | 0 | 0 |
| VisiblePlayerHUDHeart | 353/0 | world | 0 | 0 |
| StatHUDRows | 354/0 | world | 0 | 0 |
| MapDisplaySprite | 355/0 | world | 0 | 0 |
| PresentationDrawClock | 356/0 | world | 0 | 0 |
| PickupRandomAlphaResult | 357/0 | world | 0 | 0 |
| FamiliarRandomAlphaResult | 358/0 | world | 0 | 0 |
| PillDisplayResult | 359/0 | body | 0 | 0 |
| PickupPreviewItem | 360/0 | world | 0 | 0 |
| PickupPreviewResult | 361/0 | world | 0 | 0 |
| PlayerDrawImageBindings | 362/0 | body | 0 | 0 |
| HudBodySelection | 363/0 | world | 0 | 0 |
| ControlGroupHudSelection | 364/0 | world | 0 | 0 |
| PresentationSpriteContribution | 365/0 | world | 0 | 0 |
| PresentationImageContribution | 366/0 | world | 0 | 0 |
| ScreenBloomResult | 367/0 | world | 0 | 0 |
| ScreenPixelationResult | 368/0 | world | 0 | 0 |
| ScreenRetrovisionResult | 369/0 | world | 0 | 0 |
| ScreenHallucinationResult | 370/0 | world | 0 | 0 |
| RoomGlitchResult | 371/0 | world | 0 | 0 |
| ScreenFadeContribution | 372/0 | world | 0 | 0 |
| ScreenEffectsResult | 373/0 | world | 0 | 0 |
| BossVersusResult | 374/0 | world | 0 | 0 |
| NightmareSceneResult | 375/0 | world | 0 | 0 |
| GameOverResult | 376/0 | world | 0 | 0 |
| StageTransitionResult | 377/0 | world | 0 | 0 |
| ItemOverlayResult | 378/0 | world | 0 | 0 |
| BossOverlayResult | 379/0 | world | 0 | 0 |
| GenericPromptResult | 380/0 | world | 0 | 0 |
| AudioVoiceParameters | 381/0 | world | 0 | 0 |
| AudioVoiceState | 382/0 | world | 0 | 0 |
| AudioWorldEvent | 383/0 | world | 0 | 0 |
| StateCommonHeader | 408/0 | world | 50 | 50 |
| StateFrameHeader | 409/0 | world | 2 | 2 |
| StateSectionHeader | 410/0 | world | 52 | 52 |
| StateRecordHeader | 411/0 | world | 12 | 12 |
| StateTableHeader | 412/0 | world | 10 | 10 |
| StateMemberEntry | 413/0 | world | 33 | 33 |
| StateResultGroupHeader | 414/0 | world | 42 | 42 |
| StateDependencyEntry | 415/0 | world | 32 | 32 |
| StateContributionEntry | 416/0 | world | 16 | 16 |
| StateAudioEventHeader | 417/0 | world | 40 | 40 |
| StateAppliedAckHeader | 419/0 | world | 2 | 2 |
| StateAppliedAckEntry | 420/0 | world | 21 | 21 |
| StateResyncHeader | 421/0 | world | 11 | 11 |
| DisplayTextRun | 422/0 | world | 0 | 0 |
| FortuneText | 423/0 | world | 0 | 0 |
| LocalHistoryOverlay | 424/0 | world | 0 | 0 |
| ResourcePath | 425/0 | world | 2 | 4098 |
| ResourceImageFile | 426/0 | world | 10 | 4106 |
| ResourceBuiltinShader | 427/0 | world | 1 | 1 |
| ResourceAnimation | 428/0 | world | 12 | 5132 |
| ResourceName | 429/0 | world | 2 | 2050 |
| ResourceAnimationLayer | 430/0 | world | 6 | 6 |
| ResourceItemConfig | 431/0 | world | 16 | 16 |
| ResourceEntityConfig | 432/0 | world | 22 | 22 |
| ResourceAudio | 433/0 | world | 4 | 8198 |
| RoomWaveSprite | 434/0 | world | 117 | 117 |
| RoomWaveSprite | 434/1 | world | 14 | 524294 |
| RoomWaveSprite | 434/2 | world | 2 | 7798667 |
| PlayerHUDLocalClock | 435/0 | body | 0 | 0 |
| HudMantleEffects | 436/0 | body | 0 | 0 |
| SFXVolumeState | 437/0 | world | 4 | 4 |
| DisplayInjectionState | 438/0 | world | 176 | 176 |
| MusicRequestState | 439/0 | world | 57 | 57 |
| LoopSoundState | 440/0 | world | 38 | 38 |
| LoopSoundSet | 441/0 | world | 10 | 19466 |
| OverlayDisplayState | 442/0 | world | 320 | 17825840 |
| OverlayDisplaySet | 443/0 | world | 2 | 131074 |
| ProceduralDisplayDefinition | 444/0 | world | 12 | 12300 |
| ProceduralDisplaySet | 445/0 | world | 6 | 524292 |
| StateEventQueueHeader | 446/0 | world | 22 | 22 |
| StateEventHeader | 447/0 | world | 22 | 22 |
| SfxPlayEvent | 448/0 | world | 22 | 22 |
| ItemBannerEvent | 449/0 | world | 13 | 16397 |
| FortuneLine | 450/0 | world | 2 | 4098 |
| FortuneEvent | 451/0 | world | 8 | 131272 |
| BossIntroEvent | 452/0 | world | 24 | 24 |
| RoomTransitionEvent | 453/0 | world | 43 | 43 |
| BossDisplayState | 454/0 | world | 26 | 323 |
| BossDisplayPose | 455/0 | world | 9 | 9 |
