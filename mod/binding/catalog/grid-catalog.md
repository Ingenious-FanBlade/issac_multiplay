# tx-04-11 Grid 逐成员目录

Sprite／图层／动画／资源的共用规范字段见[资源成员目录](entity-catalog.md)，本目录JSON的shared_resources声明复用范围。

继承公共字段由 GridEntity 管理；17 个派生接口分别登记，空的派生接口仍保存转换结果和类型。Desc 与 GetSaveState 共用规范字段。操作不在采集时执行，真实更新／生成／破坏与 native 生命周期分 scope 记录。全参数查询见签名；目录不等于全部场景运行覆盖。

| 规范成员 | 原成员及参数 | 实现 | 来源 |
| --- | --- | --- | --- |
| `GridEntity.CollisionClass` | `CollisionClass()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L158 |
| `GridEntity.GetSaveState` | `Desc()` | alias:GridEntity.GetSaveState  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L163 |
| `GridEntity.Destroy` | `Destroy( boolean Immediate )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L23 |
| `GridEntity.DestroyWithSource` | `DestroyWithSource( boolean Immediate, EntityRef Source )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L28 |
| `GridEntity.GetGridIndex` | `GetGridIndex( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L33 |
| `GridEntity.GetRNG` | `GetRNG( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L38 |
| `GridEntity.GetRenderPosition` | `GetRenderPosition( )` | readonly_query official Lua wrapper leaks temporary Vector; native stack-buffer adapter | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L16 |
| `GridEntity.GetSaveState` | `GetSaveState( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L46 |
| `GridEntity.GetSprite` | `GetSprite( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L53 |
| `GridEntity.GetType` | `GetType( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L58 |
| `GridEntity.GetVariant` | `GetVariant( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L63 |
| `GridEntity.GetWaterClipFlags` | `GetWaterClipFlags( )` | readonly_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L20 |
| `GridEntity.Hurt` | `Hurt( int Damage )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L68 |
| `GridEntity.HurtDamage` | `HurtDamage( Entity Entity, int PlayerDamage, int DamageFlags, float Damage, boolean ignoreGridCollision )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L27 |
| `GridEntity.HurtSurroundings` | `HurtSurroundings(float enemyDistance, float playerDistance, float enemyDamage, int playerDamage, int damageFlags, boolean ignoreGridCollision)` | actual_callback_event_only official public binding; absent from local Wiki; actual event only | [official-binding](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/GridEntities/LuaGridEntity.cpp) L18 |
| `GridEntity.HurtWithSource` | `HurtWithSource( int Damage, EntityRef Source )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L73 |
| `GridEntity.Init` | `Init( int Seed )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L78 |
| `GridEntity.IsBreakableRock` | `IsBreakableRock( )` | readonly_query Wiki void typo; official LuaGridEntity.cpp returns boolean | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L31 |
| `GridEntity.Position` | `Position()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L171 |
| `GridEntity.PostInit` | `PostInit( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L83 |
| `GridEntity.Render` | `Render( Vector Offset )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L88 |
| `GridEntity.ResetWaterClipFlags` | `ResetWaterClipFlags( )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L35 |
| `GridEntity.SetType` | `SetType( GridEntityType Type )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L93 |
| `GridEntity.SetVariant` | `SetVariant( int Variant )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L98 |
| `GridEntity.SetWaterClipFlags` | `SetWaterClipFlags( WaterClipFlag Flags )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L40 |
| `GridEntity.State` | `State()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L176 |
| `GridEntity.ToDecoration` | `ToDecoration( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L49 |
| `GridEntity.ToDoor` | `ToDoor( )` | all_cast_results  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L103 |
| `GridEntity.ToFire` | `ToFire( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L58 |
| `GridEntity.ToGravity` | `ToGravity( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L67 |
| `GridEntity.ToLock` | `ToLock( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L76 |
| `GridEntity.ToPit` | `ToPit( )` | all_cast_results  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L110 |
| `GridEntity.ToPoop` | `ToPoop( )` | all_cast_results  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L117 |
| `GridEntity.ToPressurePlate` | `ToPressurePlate( )` | all_cast_results  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L124 |
| `GridEntity.ToRock` | `ToRock( )` | all_cast_results  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L131 |
| `GridEntity.ToSpikes` | `ToSpikes( )` | all_cast_results  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L138 |
| `GridEntity.ToStairs` | `ToStairs( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L84 |
| `GridEntity.ToStatue` | `ToStatue( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L93 |
| `GridEntity.ToTNT` | `ToTNT( )` | all_cast_results  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L145<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L9 |
| `GridEntity.ToTeleporter` | `ToTeleporter( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L102 |
| `GridEntity.ToTrapDoor` | `ToTrapDoor( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L109 |
| `GridEntity.ToWall` | `ToWall( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L118 |
| `GridEntity.ToWeb` | `ToWeb( )` | all_cast_results  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntity.md) L127 |
| `GridEntity.Update` | `Update( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L152 |
| `GridEntity.VarData` | `VarData()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntity.md) L186 |
| `GridEntityDesc.Initialized` | `Initialized()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md) L21 |
| `GridEntityDesc.SpawnCount` | `SpawnCount()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md) L26 |
| `GridEntityDesc.SpawnSeed` | `SpawnSeed()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md) L31 |
| `GridEntityDesc.State` | `State()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md) L36 |
| `GridEntityDesc.Type` | `Type()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md) L41 |
| `GridEntityDesc.VarData` | `VarData()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md) L46 |
| `GridEntityDesc.VariableSeed` | `VariableSeed()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md) L51 |
| `GridEntityDesc.Variant` | `Variant()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDesc.md) L56 |
| `GridEntityDoor.Bar` | `Bar( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L21 |
| `GridEntityDoor.Busted` | `Busted()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L102 |
| `GridEntityDoor.CanBlowOpen` | `CanBlowOpen( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L26 |
| `GridEntityDoor.Close` | `Close( boolean Force )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L31 |
| `GridEntityDoor.CloseAnimation` | `CloseAnimation()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L107 |
| `GridEntityDoor.CurrentRoomType` | `CurrentRoomType()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L112 |
| `GridEntityDoor.Direction` | `Direction()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L117 |
| `GridEntityDoor.ExtraSprite` | `ExtraSprite()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L122 |
| `GridEntityDoor.ExtraVisible` | `ExtraVisible()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L127 |
| `GridEntityDoor.ExtraSprite` | `GetExtraSprite( )` | readonly_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityDoor.md) L30 |
| `GridEntityDoor.GetSpriteOffset` | `GetSpriteOffset( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L36 |
| `GridEntityDoor.IsBusted` | `IsBusted( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L41 |
| `GridEntityDoor.IsKeyFamiliarTarget` | `IsKeyFamiliarTarget( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L46 |
| `GridEntityDoor.IsLocked` | `IsLocked( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L51 |
| `GridEntityDoor.IsOpen` | `IsOpen( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L56 |
| `GridEntityDoor.IsRoomType` | `IsRoomType( RoomType Type )` | finite_enum_or_explicit_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L61 |
| `GridEntityDoor.IsTargetRoomArcade` | `IsTargetRoomArcade( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L66 |
| `GridEntityDoor.LockedAnimation` | `LockedAnimation()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L132 |
| `GridEntityDoor.Open` | `Open( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L71 |
| `GridEntityDoor.OpenAnimation` | `OpenAnimation()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L137 |
| `GridEntityDoor.OpenLockedAnimation` | `OpenLockedAnimation()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L142 |
| `GridEntityDoor.PlayAnimation` | `PlayAnimation( )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityDoor.md) L35 |
| `GridEntityDoor.PreviousState` | `PreviousState()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L147<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityDoor.md) L10 |
| `GridEntityDoor.PreviousVariant` | `PreviousVariant()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L154<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityDoor.md) L18 |
| `GridEntityDoor.SetLocked` | `SetLocked( boolean Locked )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L76 |
| `GridEntityDoor.SetRoomTypes` | `SetRoomTypes( RoomType CurrentRoomType, RoomType TargetRoomType )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L81 |
| `GridEntityDoor.Slot` | `Slot()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L161 |
| `GridEntityDoor.SpawnDust` | `SpawnDust( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L86 |
| `GridEntityDoor.TargetRoomIndex` | `TargetRoomIndex()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L166 |
| `GridEntityDoor.TargetRoomType` | `TargetRoomType()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L172 |
| `GridEntityDoor.TryBlowOpen` | `TryBlowOpen( boolean FromExplosion, Entity source )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L91 |
| `GridEntityDoor.TryUnlock` | `TryUnlock(EntityPlayer player, boolean Force )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityDoor.md) L96 |
| `GridEntityLock.TryUnlock` | `TryUnlock( EntityPlayer Player, boolean TryPayToPlay )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityLock.md) L15 |
| `GridEntityPit.HasLadder` | `HasLadder()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPit.md) L36 |
| `GridEntityPit.MakeBridge` | `MakeBridge( GridEntity parentEntity)` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPit.md) L20 |
| `GridEntityPit.SetLadder` | `SetLadder( boolean Value )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPit.md) L25 |
| `GridEntityPit.UpdateCollision` | `UpdateCollision( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPit.md) L30 |
| `GridEntityPoop.ReduceSpawnRate` | `ReduceSpawnRate( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPoop.md) L20 |
| `GridEntityPoop.ReducedSpawnRate` | `ReducedSpawnRate()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPoop.md) L31 |
| `GridEntityPoop.RespawnRedPoop` | `RespawnRedPoop( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPoop.md) L25 |
| `GridEntityPoop.ReviveTimer` | `ReviveTimer()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPoop.md) L36 |
| `GridEntityPoop.StateAnimation` | `StateAnimation()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPoop.md) L41 |
| `GridEntityPoop.UnderPlayer` | `UnderPlayer()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPoop.md) L46 |
| `GridEntityPressurePlate.GreedModeRNG` | `GreedModeRNG()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPressurePlate.md) L34 |
| `GridEntityPressurePlate.NextGreedAnimation` | `NextGreedAnimation()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPressurePlate.md) L40 |
| `GridEntityPressurePlate.Reward` | `Reward( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPressurePlate.md) L20 |
| `GridEntityPressurePlate.TimerPlate` | `TimerPlate()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityPressurePlate.md) L48 |
| `GridEntityRock.Anim` | `Anim()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityRock.md) L47 |
| `GridEntityRock.FrameCnt` | `FrameCnt()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityRock.md) L52 |
| `GridEntityRock.GetAltRockType` | `GetAltRockType( BackdropType Backdrop = 0 )` | finite_enum_or_explicit_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L10 |
| `GridEntityRock.GetBigRockFrame` | `GetBigRockFrame( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityRock.md) L20 |
| `GridEntityRock.RubbleAnim` | `GetRubbleAnim( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityRock.md) L25 |
| `GridEntityRock.GetSprite` | `GetSprite( )` | readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityRock.md) L30 |
| `GridEntityRock.PlayBreakSound` | `PlayBreakSound( GridEntityType Type, BackdropType Backdrop = 0 )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L14 |
| `GridEntityRock.RegisterRockDestroyed` | `RegisterRockDestroyed( GridEntityType Type )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L18 |
| `GridEntityRock.RenderTop` | `RenderTop( Vector Offset )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L22 |
| `GridEntityRock.RubbleAnim` | `RubbleAnim()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityRock.md) L57 |
| `GridEntityRock.SetBigRockFrame` | `SetBigRockFrame( int Frame )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityRock.md) L36 |
| `GridEntityRock.SpawnDrops` | `SpawnDrops( Vector Position, GridEntityType Type, int GridVariant, int Seed, boolean Unknown, BackdropType Backdrop = 0 )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L26 |
| `GridEntityRock.TrySpawnLadder` | `TrySpawnLadder( )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L30 |
| `GridEntityRock.TrySpawnWorms` | `TrySpawnWorms( )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L34 |
| `GridEntityRock.UpdateAnimFrame` | `UpdateAnimFrame( )` | actual_callback_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityRock.md) L41 |
| `GridEntityRock.UpdateCollision` | `UpdateCollision( )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L38 |
| `GridEntityRock.UpdateNeighbors` | `UpdateNeighbors( )` | actual_callback_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityRock.md) L42 |
| `GridEntitySpikes.Timeout` | `Timeout()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntitySpikes.md) L20 |
| `GridEntityTNT.FrameCnt` | `FrameCnt()` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/GridEntityTNT.md) L20 |
