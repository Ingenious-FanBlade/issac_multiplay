"""模型v2的逐成员发送选择及消费者复核；不改变完整观测目录。"""

# 清单只声明已逐项复核的成员。未来新增parameter_query默认保留，不能按类别静默省略。
# 每组结论会展开到生成目录的每一行，包含被审阅的动态字段消费者。
OMITTED_GROUPS = (
    (
        "静态ANM2动画表由resource.logical_path引用本地同版本资源",
        "client_resource_model.cpp:ClientParseResourcePose/Layer和client_resources_native.cpp:SetPose只消费当前姿态、图层和pose_runtime，不读取整份动画表",
        "Sprite.GetAllAnimationData",
    ),
    (
        "I31逐类复核：非Player最终字段/关系/几何已有具名正式输入；下列派生按参查询不安装到Client；Player完整应用归20",
        "I31:client_entity_model.cpp的Installed/具名关系、client_entity_class_model.cpp的Rules/Resource/Collection/ParseFields、client_entity_links.inc逐槽及client_entity_draw_native.cpp已复核。Flags直接取完整位集；EntityList/VectorList/EffectList由完整表替代Get；GetNullOffset/CollisionCapsule/OrbitPosition均不被安装器或draw调用；Bomb五层另行保留；Player20再复核",
        """
        BitSet128.Get EffectList.Get
        Entity.GetCollisionCapsule Entity.GetDebugShape Entity.GetNullCapsule
        Entity.GetNullOffset Entity.GetPredictedTargetPosition Entity.HasCommonParentWithEntity
        Entity.HasEntityFlags Entity.IsActiveEnemy Entity.IsFrame
        EntityBomb.HasTearFlags
        EntityConfigEntity.HasCustomTag EntityConfigEntity.HasEntityTags EntityConfigEntity.HasGibFlags
        EntityEffect.HasTearFlags EntityFamiliar.GetOrbitPosition EntityFamiliar.GetOrbitDistance EntityKnife.HasTearFlags
        EntityLaser.HasTearFlags EntityList.Get EntityNPC.CanBeDamagedFromVelocity
        EntityPlayer.CanAddCollectible EntityPlayer.CanAddCollectibleToInventory
        EntityPlayer.CanOverrideActiveItem EntityPlayer.CanUsePill
        EntityPlayer.GetActiveCharge EntityPlayer.GetActiveItem EntityPlayer.GetActiveItemDesc
        EntityPlayer.GetActiveItemSlot EntityPlayer.GetActiveMaxCharge EntityPlayer.GetActiveMinUsableCharge
        EntityPlayer.GetActiveSubCharge EntityPlayer.GetBagOfCraftingSlot EntityPlayer.GetBatteryCharge
        EntityPlayer.GetCard EntityPlayer.GetCollectibleNum EntityPlayer.GetCostumeNullPos
        EntityPlayer.GetCustomCacheValue EntityPlayer.GetFootprintColor
        EntityPlayer.GetInnateCollectibleCount EntityPlayer.GetInnateCollectibleGroup
        EntityPlayer.GetInnateTrinketCount EntityPlayer.GetInnateTrinketGroup
        EntityPlayer.GetLaserOffset EntityPlayer.GetPill EntityPlayer.GetPlayerFormCounter
        EntityPlayer.GetPocketItem EntityPlayer.GetPoopSpell EntityPlayer.GetSmeltedTrinketDesc
        EntityPlayer.GetSmeltedTrinkets EntityPlayer.GetSpecialGridCollision
        EntityPlayer.GetTearMovementInheritance EntityPlayer.GetTotalActiveCharge
        EntityPlayer.GetTrinket EntityPlayer.GetTrinketMultiplier EntityPlayer.GetWeapon
        EntityPlayer.HasCollectible EntityPlayer.HasGoldenTrinket EntityPlayer.HasInvincibility
        EntityPlayer.HasPlayerForm EntityPlayer.HasTrinket EntityPlayer.HasWeaponType
        EntityPlayer.IsBlackHeart EntityPlayer.IsBoneHeart EntityPlayer.IsCollectibleAnimFinished
        EntityPlayer.IsCollectibleBlocked EntityPlayer.IsCollectibleCostumeVisible
        EntityPlayer.IsEntityValidTarget EntityPlayer.IsFootstepFrame EntityPlayer.IsItemCostumeVisible
        EntityPlayer.IsNullItemCostumeVisible EntityPlayer.IsPosInSpotLight
        EntityPlayer.IsTrinketBlocked EntityPlayer.NeedsCharge
        EntityProjectile.HasProjectileFlags EntityTear.HasTearFlags
        ItemConfigItem.HasCustomCacheTag ItemConfigItem.HasCustomTag ItemConfigItem.HasTags
        PathFinder.HasPathToPos
        TemporaryEffects.GetCollectibleEffect TemporaryEffects.GetCollectibleEffectNum
        TemporaryEffects.GetNullEffect TemporaryEffects.GetNullEffectNum
        TemporaryEffects.GetTrinketEffect TemporaryEffects.GetTrinketEffectNum
        TemporaryEffects.HasCollectibleEffect TemporaryEffects.HasNullEffect TemporaryEffects.HasTrinketEffect
        VectorList.Get Weapon.IsItemAnimFinished
        """,
    ),
    (
        "I31：主/辅助/owned Sprite均使用GetAllLayers+精确pose_runtime+本地同版本ANM；不发送可由同selector本地查询的静态帧表",
        "I31:ClientDescribeEntityClass::Pose/owned_sprite/NPC两ANM/Tear三ANM/Bomb五层/Knife DeadEye/Pickup双Sprite/Slot Prize与ClientEntityDrawScope均消费独立成员资源，未按path建立别名。client_resource_model.cpp:Layer/ClientParseResourcePose消费GetAllLayers/pose_runtime；client_resources_native.cpp:ExactPhase/SetPose安装selector；官方LuaSprite.cpp:LuaGetLayerFrameDataInternal按同selector读本地AnimationData帧",
        """
        AnimationData.GetLayer AnimationLayer.GetFrame Image.GetTexelRegion
        Sprite.GetAnimationData Sprite.GetLayer Sprite.GetLayerFrameData Sprite.GetNullFrame
        Sprite.GetOverlayLayerFrameData Sprite.GetOverlayNullFrame Sprite.GetSpritesheet Sprite.GetTexel
        Sprite.IsEventTriggered Sprite.IsFinished Sprite.IsOverlayEventTriggered
        Sprite.IsOverlayFinished Sprite.IsOverlayPlaying Sprite.IsPlaying
        """,
    ),
    (
        "Grid参数枚举无应用器消费者；不把查询结果作为Client玩法调用",
        "client_grids.cpp:Parse/Field的17类动态拼接分支已逐一核对：Door读具名状态及ExtraSprite，Rock读Anim/RubbleAnim/FrameCnt/GetBigRockFrame/GetSprite别名，不读IsRoomType或GetAltRockType",
        "GridEntityDoor.IsRoomType GridEntityRock.GetAltRockType",
    ),
    (
        "Room/Level参数查询无当前应用器消费者；路径、铁轨和门几何另有正式紧凑状态",
        "room_grid_target.cpp:ParseRoomGridTarget及client_room_native.cpp:ClientRoomStateApply使用application_runtime的grid_paths/rails与门/几何结构；client_map_native.cpp使用descriptor/map_application；查询不是Room私有状态。目录中未来任务说明不代表已有读取",
        """
        Camera.IsPosVisible Level.CanOpenChallengeRoom Level.CanPlaceRoom
        Level.CanPlaceRoomAtDoor Level.CanSpawnDoorOutline Level.CanStageHaveCurseOfLabyrinth
        Level.GetStateFlag Room.CanPickupGridEntity Room.CanSpawnObstacleAtPosition
        Room.GetClampedGridIndex Room.GetClampedPosition Room.GetDoorSlotPosition
        Room.GetGridCollision Room.GetGridCollisionAtPos Room.GetGridIndex Room.GetGridIndexByTile
        Room.GetGridPath Room.GetGridPathFromPos Room.GetGridPosition Room.GetLaserTarget
        Room.GetRail Room.GetShopItemPrice Room.IsDoorSlotAllowed Room.IsPersistentRoomEntity
        Room.IsPositionInRoom RoomConfigStage.IsLoaded RoomDescriptor.GetGroupByName
        RoomDescriptor.GetValidNeighborPlacementLocations
        """,
    ),
    (
        "跨房/历史参数查询无当前应用器消费者；不因未来Player/整局任务预先发送观测枚举",
        "client_scene.cpp及client_objects.cpp只消费同窗口Entity规范状态；History/PlayerManager派生参数查询没有当前读取，完整连续性观测保持原样",
        """
        History.SearchCollectibles History.SearchTrinkets
        PlayerManager.AnyPlayerTypeHasBirthright PlayerManager.AnyPlayerTypeHasCollectible
        PlayerManager.AnyPlayerTypeHasTrinket PlayerManager.AnyoneHasCollectible
        PlayerManager.AnyoneHasTrinket PlayerManager.AnyoneIsPlayerType
        PlayerManager.FirstBirthrightOwner PlayerManager.FirstCollectibleOwner
        PlayerManager.FirstPlayerByType PlayerManager.FirstTrinketOwner
        PlayerManager.GetNumCollectibles PlayerManager.GetTotalTrinketMultiplier
        """,
    ),
    (
        "整局/HUD/音画参数查询无当前应用器消费者；随机选择不在Client重放",
        "I31:client_presentation_native.cpp及client_entity_draw_native.cpp::Text消费presentation_runtime/同窗口application_draw/真实Font6；文本宽度在本地同版本Font按已知bytes计算，未消费观测Font按参枚举；HUD/整局归25/27",
        """
        Font.GetCharacterWidth Font.GetStringWidth Font.GetStringWidthUTF8
        Game.GetStateFlag Game.HasEncounteredBoss Game.IsErased
        HUD.GetPlayerHUD HUD.GetPlayerStreakSprite HUD.GetStackedStreakSprite
        HistoryHUD.GetCollectibleOffsets HistoryHUD.GetCollectibles HistoryHUD.GetItems
        HistoryHUD.GetPlayer HistoryHUD.GetTrinketOffsets HistoryHUD.GetTrinkets
        ItemPool.CanSpawnCollectible ItemPool.GetBibleUpgrades ItemPool.GetCollectiblesFromPool
        ItemPool.GetPillColor ItemPool.GetPillEffect ItemPool.GetPoolForRoom
        ItemPool.HasCollectible ItemPool.HasTrinket ItemPool.IsPillIdentified
        MusicManager.IsLayerEnabled PlayerHUD.GetHeartByIndex RNG.PhantomInt
        SFXManager.GetAmbientSoundVolume SFXManager.IsPlaying
        Seeds.CanAddSeedEffect Seeds.GetSeedEffect Seeds.GetStageSeed Seeds.HasSeedEffect
        Seeds.IsSeedComboBanned Seeds.IsSpecialSeed Seeds.IsStringValidSeed
        """,
    ),
)

# 四个查询是资源写前拒绝自定义shader的必需输入，false和失败状态均不能被省略。
RETAINED_QUERIES = {
    member: "client_resource_model.cpp:ShaderAbsent；Sprite和Layer的自定义shader拒绝检查必需，保留参数及结果/失败状态"
    for member in (
        "Sprite.HasCustomShader", "Sprite.HasCustomChampionShader",
        "LayerState.HasCustomShader", "LayerState.HasCustomChampionShader",
    )
}
RETAINED_QUERIES["EntityBomb.GetCostumeLayerSprite"] = "tx-04-19 entity-target-v1/I10/I14：五个真实costume成员的独立资源和精确姿态是Bomb表现输入，参数0..4及Null/失败均保留"

OMITTED_MEMBERS = {
    member: (reason, consumer)
    for reason, consumer, members in OMITTED_GROUPS
    for member in members.split()
}


def consumer_review(member):
    """将显式复核结论逐成员写入目录，新成员不能自动继承省略结论。"""
    if member == "EntityPlayer.GetHeldEntity":
        return "tx-04-19 I24/I25：Effect136的原生Player持物draw按当前稳定Key/shape借用；完整Player状态由20负责"
    if member in {"EntityPlayer.GetBodySprite", "EntityPlayer.GetItemState", "EntityPlayer.GetSuplexState"}:
        return "tx-04-19 I24/I25：普通Isaac的136持物draw必需BodySprite/null-frame和当前状态；按来源安装或Render栈借用，不重放玩法"
    if member in OMITTED_MEMBERS:
        return OMITTED_MEMBERS[member][1]
    return RETAINED_QUERIES.get(member, "未声明省略；沿用原归属，新增消费者须逐成员复核")


# I31保留的紧凑正式来源：它们不是Sprite别名，也不是夹具补值。
ENTITY_COMPACT_CONSUMERS = {
    "Entity.application_draw": "ClientEntityFrameScope;ClientEntityDrawScope;ordinary-window frame/subframe and two render RNG states",
    "EntityNPC.application_npc": "ClientEntityNpcCollections;three complete independent deques, including typed EntityPtr links",
    "EntityNPC.application_resources": "NPC overlay/effect real members;each owns exact resource_pose",
    "Entity.application_links": "client_entity_links.inc;stable typed slots;Null kept;no EntityList.Get needed",
    "EntityLaser.HomingLaser.application_geometry": "four complete owned containers and native scalar values;GetSamples/GetNonOptimizedSamples aliases cross-checked, never SampleLaser RPC",
    "Entity.application_owned": "five typed ExtraData owners;independent ANM poses and SpriteTrail records",
    "EntityEffect.GetGridEntityDesc": "Effect136 full LiftedGridTarget;Grid18 owner;no inferred source key",
    "NativeText.application_text_bytes": "writer-witnessed bounded bytes+KColor20+ordinary application_draw;Font6 reads local metrics",
}
