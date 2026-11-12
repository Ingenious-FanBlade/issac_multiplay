# 显示封装数字合同

布局版本 71；由唯一源生成，不是测试结果。

| 用途／布局 | 序列 | 模式 | subject布局（0为world） | 可引用缓存 | D0依赖 | 绘制拓扑 | 允许配对mask | pass mask |
|---|---|---|---|---|---|---|---|---|
| 315 PlayerHUD | body | group | 197 | True | False | True | 0 | 3 |
| 317 StatHUDPlayer | body | group | 197 | True | False | True | 0 | 3 |
| 318 StatHUD | world | group | 0 | True | False | False | 2 | 3 |
| 321 HistoryHUDPlayer | body | group | 197 | True | False | True | 0 | 3 |
| 323 HUD | world | group | 0 | True | True | False | 0 | 3 |
| 325 Minimap | world | group | 0 | True | False | False | 0 | 3 |
| 356 PresentationDrawClock | world | group | 0 | False | False | False | 0 | 3 |
| 357 PickupRandomAlphaResult | world | group | 126 | False | True | False | 0 | 449 |
| 358 FamiliarRandomAlphaResult | world | group | 120 | False | True | False | 0 | 449 |
| 359 PillDisplayResult | body | group | 197 | True | False | True | 0 | 3 |
| 361 PickupPreviewResult | world | group | 126 | True | False | False | 0 | 3 |
| 362 PlayerDrawImageBindings | body | group | 197 | True | False | True | 0 | 3 |
| 364 ControlGroupHudSelection | world | group | 196 | True | False | False | 2 | 3 |
| 367 ScreenBloomResult | world | group | 0 | True | True | False | 0 | 3 |
| 368 ScreenPixelationResult | world | group | 0 | True | True | False | 0 | 3 |
| 369 ScreenRetrovisionResult | world | group | 0 | True | True | False | 0 | 3 |
| 370 ScreenHallucinationResult | world | group | 0 | True | True | False | 0 | 3 |
| 371 RoomGlitchResult | world | group | 0 | True | True | False | 0 | 3 |
| 373 ScreenEffectsResult | world | group | 0 | False | True | False | 0 | 51 |
| 374 BossVersusResult | world | group | 0 | False | True | False | 0 | 7 |
| 375 NightmareSceneResult | world | group | 0 | False | True | False | 0 | 15 |
| 376 GameOverResult | world | group | 0 | False | True | False | 0 | 15 |
| 377 StageTransitionResult | world | group | 0 | False | True | False | 0 | 35 |
| 378 ItemOverlayResult | world | group | 0 | True | True | False | 2 | 3 |
| 379 BossOverlayResult | world | group | 0 | True | True | False | 0 | 3 |
| 380 GenericPromptResult | world | group | 0 | True | True | False | 0 | 3 |
| 382 AudioVoiceState | world | group | 0 | True | False | False | 0 | 3 |
| 383 AudioWorldEvent | world | event | 0 | False | False | False | 0 | 0 |
| 435 PlayerHUDLocalClock | body | group | 197 | True | False | True | 0 | 3 |
| 436 HudMantleEffects | body | group | 197 | True | False | True | 0 | 3 |

聚合owner的具名结果依赖（包括实际嵌套owner）：

| 用途 | 必需结果用途 |
|---|---|
| 315 | 435 |
| 318 | 364 |
| 323 | 356 |
| 357 | 356 |
| 358 | 356 |
| 367 | 356 |
| 368 | 356 |
| 369 | 356 |
| 370 | 356 |
| 371 | 356 |
| 373 | 356 |
| 374 | 356 |
| 375 | 356 |
| 376 | 356 |
| 377 | 356 |
| 378 | 356 |
| 379 | 356 |
| 380 | 356 |

贡献类别编号见protocol-constants.md。Context仅定位；多次独立贡献保持原数组顺序，不按位置排序重写payload。所有资源、成员、拓扑和缓存就绪由第12／13步正式owner提供，结构解码不提交或ACK。
