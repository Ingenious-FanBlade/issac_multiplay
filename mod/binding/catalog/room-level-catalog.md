# tx-04-12 房间与楼层逐成员目录

由两套本地Wiki生成。目录与实际E2E覆盖分别报告。操作/随机/懒加载不轮询；显式参数查询保留签名，有限网格和门槽由域遍历。资源复用Entity公共值编码。

| 成员 | 分类 | 参数／原因 | 来源 |
| --- | --- | --- | --- |
| `Backdrop.GetControlsANM2` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Backdrop.md) L20 |
| `Backdrop.GetControlsButtonsANM2` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Backdrop.md) L24 |
| `Backdrop.GetDetailANM2` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Backdrop.md) L28 |
| `Backdrop.GetFloorANM2` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Backdrop.md) L32 |
| `Backdrop.GetFloorImage` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Backdrop.md) L36 |
| `Backdrop.GetWallImage` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Backdrop.md) L41 |
| `Camera.IsClampEnabled` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md) L20 |
| `Camera.IsPosVisible` | parameter_query |  Vector Pos   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md) L25 |
| `Camera.SetClampEnabled` | operation |  boolean Clamped   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md) L30 |
| `Camera.SetFocusPosition` | operation |  Vector Pos   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md) L35 |
| `Camera.SnapToPosition` | operation |  Vector Pos   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md) L44 |
| `Camera.Update` | operation |  boolean flag = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md) L56 |
| `ColorModifier.A` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L22 |
| `ColorModifier.B` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L27 |
| `ColorModifier.Brightness` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L31 |
| `ColorModifier.ColorModifier` | operation |  float R = 1, float G = 1, float B = 1, float A = 0, float Brightness = 0, float Contrast = 1   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L18 |
| `ColorModifier.Contrast` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L35 |
| `ColorModifier.G` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L39 |
| `ColorModifier.R` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L43 |
| `ColorModifier.__add` | operation |  ColorModifier right   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L48 |
| `ColorModifier.__div` | operation |  ColorModifier right   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L53 |
| `ColorModifier.__eq` | operation |  ColorModifier right   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L58 |
| `ColorModifier.__mul` | operation |  ColorModifier right   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L63 |
| `ColorModifier.__sub` | operation |  ColorModifier right   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorModifier.md) L68 |
| `FXLayers.AddPoopFx` | operation |  ColorMod Color   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXLayers.md) L20 |
| `FXParams.ColorModifier` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md) L19 |
| `FXParams.LightColor` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md) L25 |
| `FXParams.ShadowAlpha` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md) L29 |
| `FXParams.ShadowColor` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md) L33 |
| `FXParams.UseWaterV2` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md) L37 |
| `FXParams.WaterColor` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md) L42 |
| `FXParams.WaterColorMultiplier` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md) L46 |
| `FXParams.WaterEffectColor` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md) L50 |
| `LRoomAreaDesc.GetHighBottomRight` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomAreaDesc.md) L20 |
| `LRoomAreaDesc.GetHighTopLeft` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomAreaDesc.md) L25 |
| `LRoomAreaDesc.GetLowBottomRight` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomAreaDesc.md) L30 |
| `LRoomAreaDesc.GetLowTopLeft` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomAreaDesc.md) L35 |
| `LRoomTileDesc.GetHighBottomRight` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomTileDesc.md) L20 |
| `LRoomTileDesc.GetHighTopLeft` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomTileDesc.md) L25 |
| `LRoomTileDesc.GetLowBottomRight` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomTileDesc.md) L30 |
| `LRoomTileDesc.GetLowTopLeft` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomTileDesc.md) L35 |
| `LRoomTileDesc.GetRandomTile` | actual_call |  int Seed  random_selection_or_lazy_resource_load; never_poll | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LRoomTileDesc.md) L40 |
| `Level.AddAngelRoomChance` | operation |  float Chance   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L20 |
| `Level.AddCurse` | operation |  LevelCurse Curse, boolean ShowName   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L25 |
| `Level.ApplyBlueMapEffect` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L32 |
| `Level.ApplyCompassEffect` | operation |  boolean Persistent   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L37 |
| `Level.ApplyMapEffect` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L42 |
| `Level.CanOpenChallengeRoom` | parameter_query |  int RoomIndex   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L47 |
| `Level.CanPlaceRoom` | parameter_query |  RoomConfigRoom RoomConfigToPlace, int GridIndex, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L10 |
| `Level.CanPlaceRoom` | parameter_query |  RoomShape RoomShape, DoorMask DoorMask, int GridIndex, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L11 |
| `Level.CanPlaceRoomAtDoor` | parameter_query |  RoomConfigRoom RoomConfigToPlace, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L18 |
| `Level.CanPlaceRoomAtDoor` | parameter_query |  RoomShape RoomShape, DoorMask DoorMask, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L19 |
| `Level.CanSpawnDevilRoom` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L53 |
| `Level.CanSpawnDoorOutline` | parameter_query |  int RoomIdx, DoorSlot DoorSlot   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L26 |
| `Level.CanStageHaveCurseOfLabyrinth` | parameter_query |  LevelStage Stage   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L58 |
| `Level.ChangeRoom` | operation |  int RoomIndex, int Dimension = -1   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L63 |
| `Level.DisableDevilRoom` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L71 |
| `Level.DungeonReturnPosition` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L462 |
| `Level.DungeonReturnRoomIndex` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L467 |
| `Level.EnterDoor` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L472 |
| `Level.FindValidRoomPlacementLocations` | operation |  RoomConfigRoom RoomConfig, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L30 |
| `Level.FindValidRoomPlacementLocations` | operation |  RoomShape RoomShape = 1, DoorMask DoorMask = -1, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L31 |
| `Level.ForceHorsemanBoss` | actual_call |  int Seed  random_selection_or_lazy_resource_load; never_poll | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L76 |
| `Level.GetAbsoluteStage` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L81 |
| `Level.GetAngelRoomChance` | external_reference |   run:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L93 |
| `Level.GetCanSeeEverything` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L104 |
| `Level.GetCurrentRoom` | domain_value |   explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L109 |
| `Level.GetCurrentRoomDesc` | domain_value |   explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L114 |
| `Level.GetCurrentRoomIndex` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L127 |
| `Level.GetCurseName` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L137 |
| `Level.GetCurses` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L142 |
| `Level.GetDevilAngelRoomRNG` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L147 |
| `Level.GetDimension` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L40 |
| `Level.GetDungeonPlacementSeed` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L152 |
| `Level.GetEnterPosition` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L157 |
| `Level.GetForceSpecialQuest` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L45 |
| `Level.GetGenerationRNG` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L52 |
| `Level.GetGreedWavesClearedWithoutRedHeartDamage` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L56 |
| `Level.GetHeartPicked` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L162 |
| `Level.GetLastBossRoomListIndex` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L167 |
| `Level.GetLastRoomDesc` | domain_value |   explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L172 |
| `Level.GetMyosotisPickups` | external_reference |   room_save:tx-04-13 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L60 |
| `Level.GetName` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L177 |
| `Level.GetNeighboringRooms` | domain_value |  int GridIndex, RoomShape RoomShape, Dimension Dimension = -1  explicit_owned_membership_or_subject_reference | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L65 |
| `Level.GetNonCompleteRoomIndex` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L182 |
| `Level.GetPlanetariumChance` | external_reference |   run:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L187 |
| `Level.GetPreviousRoomIndex` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L193 |
| `Level.GetRandomRoomIndex` | actual_call |  boolean IAmErrorRoom, int Seed  random_selection_or_lazy_resource_load; never_poll | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L198 |
| `Level.GetRoomByIdx` | domain_value |  int RoomIdx, int Dimension = -1  explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L203 |
| `Level.GetRoomCount` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L226 |
| `Level.GetRooms` | domain_value |   explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L231 |
| `Level.GetStage` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L246 |
| `Level.GetStageType` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L251 |
| `Level.GetStartingRoomIndex` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L260 |
| `Level.GetStateFlag` | parameter_query |  LevelStateFlag LevelStateFlag  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L266 |
| `Level.GreedModeWave` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L481 |
| `Level.HasAbandonedMineshaft` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L90 |
| `Level.HasBossChallenge` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L271 |
| `Level.HasMirrorDimension` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L95 |
| `Level.HasPhotoDoor` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L100 |
| `Level.InitializeDevilAngelRoom` | operation |  boolean ForceAngel, boolean ForceDevil   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L276 |
| `Level.IsAltStage` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L289 |
| `Level.IsAscent` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L295 |
| `Level.IsDevilRoomDisabled` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L301 |
| `Level.IsNextStageAvailable` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L306 |
| `Level.IsPreAscent` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L312 |
| `Level.IsStageAvailable` | operation |  LevelStage Level, StageType Stage   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L105 |
| `Level.LeaveDoor` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L486 |
| `Level.MakeRedRoomDoor` | operation |  int CurrentRoomIdx, DoorSlot Slot   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L318 |
| `Level.PlaceRoom` | operation |  LevelGeneratorEntry Room, RoomConfigRoom RoomConfig, int Seed   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L110 |
| `Level.QueryRoomTypeIndex` | actual_call |  RoomType RoomType, boolean Visited, RNG rng, boolean IgnoreGroup = false  random_selection_or_lazy_resource_load; never_poll | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L328 |
| `Level.RemoveCompassEffect` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L333 |
| `Level.RemoveCurses` | operation |  LevelCurse Curses   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L338 |
| `Level.SetCanSeeEverything` | operation |  boolean Value   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L349 |
| `Level.SetForceSpecialQuest` | operation |  SpecialQuest Quest   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L119 |
| `Level.SetGreedWavesClearedWithoutRedHeartDamage` | operation |  int WavesCleared   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L126 |
| `Level.SetHeartPicked` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L354 |
| `Level.SetName` | operation |  string Name   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L130 |
| `Level.SetNextStage` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L359 |
| `Level.SetRedHeartDamage` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L364 |
| `Level.SetStage` | operation |  int StageOffset, int StageTypeOffset   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L369 |
| `Level.SetStateFlag` | operation |  LevelStateFlag LevelStateFlag, boolean Val   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L408 |
| `Level.ShowMap` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L413 |
| `Level.ShowName` | operation |  boolean Sticky   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L418 |
| `Level.TryPlaceRoom` | operation |  RoomConfigRoom RoomConfigToPlace, int GridIndex, Dimension Dimension = -1, int Seed = 0, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L134 |
| `Level.TryPlaceRoomAtDoor` | operation |  RoomConfigRoom RoomConfigToPlace, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, int Seed = 0, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Level.md) L198 |
| `Level.UncoverHiddenDoor` | operation |  int CurrentRoomIdx, DoorSlot Slot   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L423 |
| `Level.Update` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L428 |
| `Level.UpdateVisibility` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Level.md) L433 |
| `RailManager.GetRailsSprite` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RailManager.md) L20 |
| `Room.CanPickupGridEntity` | parameter_query |  int GridIndex   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L41 |
| `Room.CanSpawnObstacleAtPosition` | parameter_query |  int GridIndex, boolean Force   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L46 |
| `Room.CheckLine` | derived_query |  Vector Pos1, Vector Pos2, LinecheckMode Mode, int GridPathThreshold = 0, boolean IgnoreWalls = false, boolean IgnoreCrushable = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L21 |
| `Room.ClearBossHazards` | operation |  boolean IgnoreNPCs = true, Entity Source = nil   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L50 |
| `Room.DamageGrid` | operation |  int Index, int Damage   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L54 |
| `Room.DamageGridWithSource` | operation |  int Index, int Damage, EntityRef Source   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L59 |
| `Room.DestroyGrid` | operation |  int Index, boolean Immediate   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L63 |
| `Room.DestroyGridWithSource` | operation |  int Index, boolean Immediate, EntityRef Source   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L69 |
| `Room.DoLightningStrike` | operation |  int Seed = RandomSeed   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L55 |
| `Room.EmitBloodFromWalls` | operation |  int Duration, int Count   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L73 |
| `Room.FindFreePickupSpawnPosition` | operation |  Vector Pos, float InitialStep = 0, boolean AvoidActiveEntities = false, boolean AllowPits = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L78 |
| `Room.FindFreeTilePosition` | derived_query |  Vector Pos, float DistanceThreshold   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L83 |
| `Room.GetAliveBossesCount` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L88 |
| `Room.GetAliveEnemiesCount` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L93 |
| `Room.GetAwardSeed` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L98 |
| `Room.GetBackdrop` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L60 |
| `Room.GetBackdropType` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L103<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L9 |
| `Room.GetBossID` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L109 |
| `Room.GetBossVictoryJingle` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L65 |
| `Room.GetBottomRightPos` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L119 |
| `Room.GetBrokenWatchState` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L125 |
| `Room.GetCamera` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L69 |
| `Room.GetCenterPos` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L140 |
| `Room.GetChampionBossChance` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L74 |
| `Room.GetClampedGridIndex` | parameter_query |  Vector Position   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L146 |
| `Room.GetClampedPosition` | parameter_query |  Vector Pos, float Margin   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L152 |
| `Room.GetDecorationSeed` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L158 |
| `Room.GetDeliriumDistance` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L163 |
| `Room.GetDevilRoomChance` | external_reference |   run:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L168 |
| `Room.GetDoor` | domain_value |  DoorSlot Slot  explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L285 |
| `Room.GetDoorSlotPosition` | parameter_query |  DoorSlot Slot   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L291 |
| `Room.GetDungeonRockIdx` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L296 |
| `Room.GetEffects` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L79 |
| `Room.GetEnemyDamageInflicted` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L301 |
| `Room.GetEntities` | domain_value |   explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L309 |
| `Room.GetFXLayers` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L87 |
| `Room.GetFXParams` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L91 |
| `Room.GetFloorColor` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L83 |
| `Room.GetFrameCount` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L329 |
| `Room.GetGreedWaveTimer` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L95 |
| `Room.GetGridCollision` | parameter_query |  int GridIndex   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L335 |
| `Room.GetGridCollisionAtPos` | parameter_query |  Vector Pos   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L342 |
| `Room.GetGridEntity` | domain_value |  int Index  explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L348 |
| `Room.GetGridEntityFromPos` | domain_value |  Vector Position  explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L354 |
| `Room.GetGridHeight` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L360 |
| `Room.GetGridIndex` | parameter_query |  Vector Position   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L365 |
| `Room.GetGridIndexByTile` | parameter_query |  int GridRow, int GridColumn   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L99 |
| `Room.GetGridIndexByTile` | parameter_query |  int[2] Tile   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L100 |
| `Room.GetGridPath` | parameter_query |  int Index   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L370 |
| `Room.GetGridPathFromPos` | parameter_query |  Vector Position   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L388 |
| `Room.GetGridPosition` | parameter_query |  int GridIndex   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L405 |
| `Room.GetGridSize` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L410 |
| `Room.GetGridWidth` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L415 |
| `Room.GetItemPool` | actual_call |  int Seed = Random(), boolean Raw = false  random_selection_or_lazy_resource_load; never_poll | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L104 |
| `Room.GetLRoomAreaDesc` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L437<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L13 |
| `Room.GetLRoomTileDesc` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L444<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L18 |
| `Room.GetLaserTarget` | parameter_query |  Vector Pos, Vector Dir   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L420 |
| `Room.GetLavaIntensity` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L426 |
| `Room.GetLightingAlpha` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L432 |
| `Room.GetLightningIntensity` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L111 |
| `Room.GetNumRainSpawners` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L118 |
| `Room.GetRail` | parameter_query |  int GridIndex   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L126 |
| `Room.GetRailManager` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L130 |
| `Room.GetRainIntensity` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L134 |
| `Room.GetRandomPosition` | actual_call |  float Margin  random_selection_or_lazy_resource_load; never_poll | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L451 |
| `Room.GetRandomTileIndex` | actual_call |  int Seed  random_selection_or_lazy_resource_load; never_poll | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L457 |
| `Room.GetRedHeartDamage` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L462 |
| `Room.GetRenderMode` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L468 |
| `Room.GetRenderScrollOffset` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L473 |
| `Room.GetRenderSurfaceTopLeft` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L478 |
| `Room.GetRoomClearDelay` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L139 |
| `Room.GetRoomConfigStage` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L483 |
| `Room.GetRoomShape` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L530 |
| `Room.GetSecondBossID` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L535 |
| `Room.GetSeededCollectible` | actual_call |  int Seed, bool NoDecrease = false  random_selection_or_lazy_resource_load; never_poll | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L545 |
| `Room.GetShopItemPrice` | parameter_query |  int EntityVariant, int EntitySubType, int ShopItemID   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L143 |
| `Room.GetShopLevel` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L551 |
| `Room.GetSpawnSeed` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L556 |
| `Room.GetTintedRockIdx` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L561 |
| `Room.GetTopLeftPos` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L566 |
| `Room.GetType` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L571 |
| `Room.GetWallColor` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L148 |
| `Room.GetWaterAmount` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L152 |
| `Room.GetWaterColor` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L157 |
| `Room.GetWaterColorMultiplier` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L161 |
| `Room.GetWaterCurrent` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L576 |
| `Room.HasCurseMist` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L582 |
| `Room.HasLava` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L588 |
| `Room.HasSlowDown` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L597 |
| `Room.HasTriggerPressurePlates` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L608 |
| `Room.HasWater` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L617 |
| `Room.HasWaterPits` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L622 |
| `Room.InvalidatePickupVision` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L628 |
| `Room.IsAmbushActive` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L634 |
| `Room.IsAmbushDone` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L639 |
| `Room.IsChampionBossSeed` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L165 |
| `Room.IsClear` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L644 |
| `Room.IsCurrentRoomLastBoss` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L649 |
| `Room.IsDoorSlotAllowed` | parameter_query |  DoorSlot Slot   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L655 |
| `Room.IsFirstEnemyDead` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L665 |
| `Room.IsFirstVisit` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L670 |
| `Room.IsInitialized` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L675 |
| `Room.IsLShapedRoom` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L680 |
| `Room.IsMirrorWorld` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L685 |
| `Room.IsPersistentRoomEntity` | parameter_query |  EntityType Type, int Variant = 0   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L170 |
| `Room.IsPositionInRoom` | parameter_query |  Vector Pos, float Margin   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L691 |
| `Room.IsSacrificeDone` | query |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L697 |
| `Room.KeepDoorsClosed` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L702 |
| `Room.MamaMegaExplosion` | operation |  Vector Position   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L707 |
| `Room.MamaMegaExplosion` | operation |  Vector Position = Vector.Zero, EntityPlayer Player = nil   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L709 |
| `Room.PickupGridEntity` | operation |  int GridIndex   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L174 |
| `Room.PlayMusic` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L714 |
| `Room.RemoveDoor` | operation |  DoorSlot Slot   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L720 |
| `Room.RemoveGridEntity` | operation |  int GridIndex, int PathTrail, boolean KeepDecoration   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L725 |
| `Room.RemoveGridEntityImmediate` | operation |  int GridIndex, int PathTrail, boolean KeepDecoration   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L179 |
| `Room.Render` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L738 |
| `Room.RespawnEnemies` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L743 |
| `Room.SaveState` | operation |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L184 |
| `Room.ScreenWrapPosition` | derived_query |  Vector Pos, float Margin   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L748 |
| `Room.SetAmbushDone` | operation |  boolean Value   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L756 |
| `Room.SetBackdropType` | operation |  BackdropType, int Unknown   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L189 |
| `Room.SetBrokenWatchState` | operation |  int State   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L761 |
| `Room.SetCardAgainstHumanity` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L767 |
| `Room.SetClear` | operation |  boolean Clear   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L772 |
| `Room.SetFirstEnemyDead` | operation |  boolean Value   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L778 |
| `Room.SetFloorColor` | operation |  Color FloorColor   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L783 |
| `Room.SetGreedWaveTimer` | operation |  int Time   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L196 |
| `Room.SetGridPath` | operation |  int Index, int Value   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L795 |
| `Room.SetItemPool` | operation |  ItemPoolType PoolType   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L200 |
| `Room.SetLavaIntensity` | operation |  float Intensity   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L205 |
| `Room.SetLightningIntensity` | operation |  float Intensity   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L210 |
| `Room.SetPauseTimer` | operation |  int Duration   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L217 |
| `Room.SetRail` | operation |  int GridIndex, StbRailVariant RailVariant   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L221 |
| `Room.SetRainIntensity` | operation |  float Intensity   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L225 |
| `Room.SetRedHeartDamage` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L813 |
| `Room.SetRedHeartDamage` | operation |  boolean Value = true   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L23 |
| `Room.SetRoomClearDelay` | operation |  int Delay   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L230 |
| `Room.SetSacrificeDone` | operation |  boolean Done   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L818 |
| `Room.SetSlowDown` | operation |  int Duration   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L823 |
| `Room.SetWallColor` | operation |  Color WallColor   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L834 |
| `Room.SetWaterAmount` | operation |  float Amount   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L234 |
| `Room.SetWaterColor` | operation |  KColor WaterColor   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L239 |
| `Room.SetWaterColorMultiplier` | operation |  KColor WaterColor   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L243 |
| `Room.SetWaterCurrent` | operation |  Vector WaterCurrent   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L247 |
| `Room.ShopReshuffle` | operation |  boolean KeepCollectibleIdx, boolean ReselectSaleItem   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L846 |
| `Room.ShopRestockFull` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L851 |
| `Room.ShopRestockPartial` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L857 |
| `Room.SpawnClearAward` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L862 |
| `Room.SpawnGridEntity` | operation |  int GridIndex, GridEntityDesc Descriptor  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L29 |
| `Room.SpawnGridEntity` | operation |  int GridIndex, GridEntityType Type, int Variant = 0, int Seed = nil, int VarData = 0   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L28 |
| `Room.SpawnGridEntity` | operation |  int GridIndex, GridEntityType Type, int Variant, int Seed, int VarData   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L867 |
| `Room.StopRain` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L872 |
| `Room.TriggerClear` | operation |  boolean Silent = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L878 |
| `Room.TriggerOutput` | operation |  RoomEventOutput GroupIdx  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L255 |
| `Room.TriggerRestock` | operation |  int GridIndex, int ShopItemIdx  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L259 |
| `Room.TryGetShopDiscount` | operation |  int ShopItemIdx, int Price   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L264 |
| `Room.TryMakeBridge` | operation |  GridEntity pit, GridEntity rock   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L885 |
| `Room.TryPlaceLadder` | operation |  Vector PlayerPos, Vector PlayerVelocity, Entity Ladder   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L891 |
| `Room.TrySpawnBlueWombDoor` | operation |  boolean FirstTime = true, boolean IgnoreTime = false, boolean Force = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L897 |
| `Room.TrySpawnBossRushDoor` | operation |  boolean IgnoreTime = false, boolean Force = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L904 |
| `Room.TrySpawnDevilRoomDoor` | operation |  boolean Animate = false, boolean Force = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L911 |
| `Room.TrySpawnMegaSatanRoomDoor` | operation |  boolean Force = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L918 |
| `Room.TrySpawnSecretExit` | operation |  boolean Animate = false, boolean Force = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L925 |
| `Room.TrySpawnSecretShop` | operation |  boolean Force = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L932 |
| `Room.TrySpawnSpecialQuestDoor` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L939 |
| `Room.TrySpawnSpecialQuestDoor` | operation |  boolean IgnoreStageType = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L34 |
| `Room.TrySpawnTheVoidDoor` | operation |  boolean Force = false   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L945 |
| `Room.TurnGold` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L952 |
| `Room.Update` | operation |    | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L958 |
| `Room.UpdateColorModifier` | operation |  boolean Process = true, boolean Lerp = true, float Rate = 0.015   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md) L268 |
| `Room.WorldToScreenPosition` | derived_query |  Vector WorldPos   | [vanilla](F:/projects_isaac/IsaacDocs/docs/Room.md) L974 |
| `RoomConfig.AddRooms` | operation |  StbType Stage, int Mode, table[] Rooms   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfig.md) L21 |
| `RoomConfig.GetRandomRoom` | actual_call |  int Seed, boolean ReduceWeight, StbType Stage, RoomType Type, RoomShape Shape = RoomShape.NUM_ROOMSHAPES, int MinVariant = 0, int MaxVariant = -1, int MinDifficulty = 0, int MaxDifficulty = 10, int RequiredDoors = 0, int Subtype = -1, int Mode = -1  random_selection_or_lazy_resource_load; never_poll | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfig.md) L29 |
| `RoomConfig.GetRoomByStageTypeAndVariant` | derived_query |  StbType Stage, RoomType Type, int Variant, int Mode = -1  explicit_resource_lookup | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfig.md) L35 |
| `RoomConfig.GetStage` | derived_query |  StbType Stage  explicit_resource_lookup | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfig.md) L40 |
| `RoomConfig.LoadStb` | operation |  StbType Stage, int Mode, string StbFileName   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfig.md) L44 |
| `RoomConfigEntry.Subtype` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Entry.md) L26 |
| `RoomConfigEntry.Type` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Entry.md) L31 |
| `RoomConfigEntry.Variant` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Entry.md) L36 |
| `RoomConfigEntry.Weight` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Entry.md) L41 |
| `RoomConfigRoom.Difficulty` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L25 |
| `RoomConfigRoom.Doors` | domain_value |  explicit_owned_membership_or_subject_reference | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L31 |
| `RoomConfigRoom.Height` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L49 |
| `RoomConfigRoom.InitialWeight` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L54 |
| `RoomConfigRoom.Mode` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L59 |
| `RoomConfigRoom.Mode` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigRoom.md) L10 |
| `RoomConfigRoom.Name` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L66 |
| `RoomConfigRoom.OriginalVariant` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L71 |
| `RoomConfigRoom.Shape` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L76 |
| `RoomConfigRoom.SpawnCount` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L81 |
| `RoomConfigRoom.Spawns` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L86 |
| `RoomConfigRoom.StageID` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L91 |
| `RoomConfigRoom.Subtype` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L138 |
| `RoomConfigRoom.Type` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L143 |
| `RoomConfigRoom.Variant` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L148 |
| `RoomConfigRoom.Weight` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L153 |
| `RoomConfigRoom.Width` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Room.md) L158 |
| `RoomConfigSpawn.Entries` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Spawn.md) L32 |
| `RoomConfigSpawn.EntryCount` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Spawn.md) L38 |
| `RoomConfigSpawn.PickEntry` | operation |  float r   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Spawn.md) L26 |
| `RoomConfigSpawn.SumWeights` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Spawn.md) L43 |
| `RoomConfigSpawn.X` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Spawn.md) L48 |
| `RoomConfigSpawn.Y` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomConfig_Spawn.md) L53 |
| `RoomConfigStage.GetBackdrop` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L20 |
| `RoomConfigStage.GetBossSpot` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L25 |
| `RoomConfigStage.GetDisplayName` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L30 |
| `RoomConfigStage.GetID` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L35 |
| `RoomConfigStage.GetMusic` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L39 |
| `RoomConfigStage.GetPlayerSpot` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L44 |
| `RoomConfigStage.GetRoomSet` | actual_call |  int Mode  random_selection_or_lazy_resource_load; never_poll | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L49 |
| `RoomConfigStage.GetSuffix` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L55 |
| `RoomConfigStage.GetXMLName` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L60 |
| `RoomConfigStage.IsLoaded` | parameter_query |  int mode = 0   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L64 |
| `RoomConfigStage.SetBackdrop` | operation |  BackdropType Backdrop   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L68 |
| `RoomConfigStage.SetBossSpot` | operation |  string PngFilename   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L73 |
| `RoomConfigStage.SetDisplayName` | operation |  string Name   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L78 |
| `RoomConfigStage.SetMusic` | operation |  Music Music   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L83 |
| `RoomConfigStage.SetPlayerSpot` | operation |  string PngFilename   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L88 |
| `RoomConfigStage.SetSuffix` | operation |  string Suffix   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L93 |
| `RoomConfigStage.SetXMLName` | operation |  string name   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L98 |
| `RoomConfigStage.Unload` | operation |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomConfigStage.md) L102 |
| `RoomDescriptor.AddRestrictedGridIndex` | operation |  int GridIndex   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L30 |
| `RoomDescriptor.AllowedDoors` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L23<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L10 |
| `RoomDescriptor.AwardSeed` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L31 |
| `RoomDescriptor.BossDeathSeed` | field |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L147 |
| `RoomDescriptor.ChallengeDone` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L36 |
| `RoomDescriptor.Clear` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L41 |
| `RoomDescriptor.ClearCount` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L46 |
| `RoomDescriptor.CreateGroup` | operation |  string groupName   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L126 |
| `RoomDescriptor.Data` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L51 |
| `RoomDescriptor.DecorationSeed` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L56 |
| `RoomDescriptor.DeliriumDistance` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L61 |
| `RoomDescriptor.DisplayFlags` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L66 |
| `RoomDescriptor.Doors` | domain_value |  explicit_owned_membership_or_subject_reference | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L151 |
| `RoomDescriptor.Flags` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L100 |
| `RoomDescriptor.GetDecoSaveState` | external_reference |   room_save:tx-04-13 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L34 |
| `RoomDescriptor.GetDimension` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L38 |
| `RoomDescriptor.GetEntitiesSaveState` | external_reference |   room_save:tx-04-13 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L43 |
| `RoomDescriptor.GetErrorTrinketEffect` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L47 |
| `RoomDescriptor.GetGridEntitiesSaveState` | external_reference |   room_save:tx-04-13 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L53 |
| `RoomDescriptor.GetGroup` | query |   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L103 |
| `RoomDescriptor.GetGroupByName` | parameter_query |  string groupName   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L136 |
| `RoomDescriptor.GetNeighboringRooms` | domain_value |   explicit_owned_membership_or_subject_reference | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L57 |
| `RoomDescriptor.GetRestrictedGridIndexes` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L78 |
| `RoomDescriptor.GetTaintedKeeperCoinSpawns` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L82 |
| `RoomDescriptor.GetValidNeighborPlacementLocations` | parameter_query |  RoomConfigRoom RoomConfig, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L87 |
| `RoomDescriptor.GetValidNeighborPlacementLocations` | parameter_query |  RoomShape RoomShape = 1, DoorMask DoorMask = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L88 |
| `RoomDescriptor.GridIndex` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L105 |
| `RoomDescriptor.HasWater` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L141 |
| `RoomDescriptor.InitSeeds` | operation |  RNG RNG   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L95 |
| `RoomDescriptor.ListIndex` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L146 |
| `RoomDescriptor.NoReward` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L155 |
| `RoomDescriptor.OverrideData` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L160 |
| `RoomDescriptor.PitsCount` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L165 |
| `RoomDescriptor.PoopCount` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L170 |
| `RoomDescriptor.PressurePlatesTriggered` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L175 |
| `RoomDescriptor.SacrificeDone` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L180 |
| `RoomDescriptor.SafeGridIndex` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L185 |
| `RoomDescriptor.SetGroup` | operation |  int group   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L113 |
| `RoomDescriptor.SetTaintedKeeperCoinSpawns` | operation |  int Num   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomDescriptor.md) L99 |
| `RoomDescriptor.ShopItemDiscountIdx` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L198 |
| `RoomDescriptor.ShopItemIdx` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L226 |
| `RoomDescriptor.SpawnSeed` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L259 |
| `RoomDescriptor.SurpriseMiniboss` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L264 |
| `RoomDescriptor.VisitedCount` | field |   | [vanilla](F:/projects_isaac/IsaacDocs/docs/RoomDescriptor.md) L268 |
| `RoomTransition.GetPlayerExtraPortraitSprite` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomTransition.md) L22 |
| `RoomTransition.GetTransitionMode` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomTransition.md) L26 |
| `RoomTransition.GetVersusScreenSprite` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomTransition.md) L30 |
| `RoomTransition.IsRenderingBossIntro` | query |    | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomTransition.md) L34 |
| `RoomTransition.StartBossIntro` | operation |  BossType BossID1, BossType BossID2 = 0   | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/RoomTransition.md) L38 |
