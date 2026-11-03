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
        "参数查询无当前Entity/Player应用器消费者；完整观测保留，19/20采用新字段时显式修订本清单",
        "client_objects.cpp:Object/FieldsSupported只读取具名公共字段及代表Player统计；未通过参数查询构造或应用对象",
        """
        BitSet128.Get EffectList.Get
        Entity.GetCollisionCapsule Entity.GetDebugShape Entity.GetNullCapsule
        Entity.GetNullOffset Entity.GetPredictedTargetPosition Entity.HasCommonParentWithEntity
        Entity.HasEntityFlags Entity.IsActiveEnemy Entity.IsFrame
        EntityBomb.GetCostumeLayerSprite EntityBomb.HasTearFlags
        EntityConfigEntity.HasCustomTag EntityConfigEntity.HasEntityTags EntityConfigEntity.HasGibFlags
        EntityEffect.HasTearFlags EntityFamiliar.GetOrbitPosition EntityKnife.HasTearFlags
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
        "资源参数查询无消费者；本地动画数据及已保留逐实例姿态足以支持现有资源应用",
        "client_resource_model.cpp:Layer/ClientParseResourcePose读取GetAllLayers和pose_runtime；client_resources_native.cpp:ExactPhase/SetPose安装精确selector。官方LuaSprite.cpp:LuaGetLayerFrameDataInternal仅按同一selector查本地AnimationData帧",
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
        "client_presentation.cpp/client_presentation_native.cpp消费正式presentation_runtime而非这些按参查询；完整HUD/整局应用归25/27，当前没有参数结果消费者",
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

OMITTED_MEMBERS = {
    member: (reason, consumer)
    for reason, consumer, members in OMITTED_GROUPS
    for member in members.split()
}


def consumer_review(member):
    """将显式复核结论逐成员写入目录，新成员不能自动继承省略结论。"""
    if member in OMITTED_MEMBERS:
        return OMITTED_MEMBERS[member][1]
    return RETAINED_QUERIES.get(member, "未声明省略；沿用原归属，新增消费者须逐成员复核")
