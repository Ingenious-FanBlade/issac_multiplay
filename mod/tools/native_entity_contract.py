"""19逐成员应用合同；目录只声明来源和恢复责任，不把Getter变成远程RPC。"""

# 白名单只登记已阅读本地Wiki/官方Lua实现的最终值入口，不按Get/Set名字猜写能力。
SETTERS = {
    "Entity": {
        **{"Get" + name: "Set" + name for name in (
            "BaitedCountdown", "BleedingCountdown", "BossStatusEffectCooldown",
            "BrimstoneMarkCountdown", "BurnCountdown", "BurnDamageTimer",
            "CharmedCountdown", "ConfusionCountdown", "DamageCountdown",
            "FearCountdown", "FireDamageCooldown", "FreezeCountdown", "IceCountdown",
            "KnockbackCountdown", "KnockbackDirection", "MagnetizedCountdown",
            "MidasFreezeCountdown", "PauseTime", "PoisonCountdown", "PoisonDamageTimer",
            "ShadowSize", "ShrinkCountdown", "SlowingCountdown", "SpeedMultiplier",
            "WaterClipFlags", "WeaknessCountdown", "ColorParams")},
        "IsDead": "SetDead", "IsInvincible": "SetInvincible",
    },
    "EntityNPC": {"GetControllerId": "SetControllerId", "GetFlyingOverride": "SetFlyingOverride",
                  "GetShieldStrength": "SetShieldStrength"},
    # 两个Getter都由精确Cycle位域决定；SetRedMode(true)会把多个bit全置1，不能重复写派生bool。
    "EntityDelirium": {},
    "EntityTear": {"IsMultidimensionalTouched": "SetMultidimensionalTouched", "IsPrismTouched": "SetPrismTouched"},
    "EntityBomb": {"Get" + name: "Set" + name for name in (
        "FallAcceleration", "FallSpeed", "RocketAngle", "RocketSpeed", "Scale")},
    "EntityLaser": {"Get" + name: "Set" + name for name in (
        "DamageMultiplier", "DisableFollowParent", "NumChainedLasers", "Scale", "Shrink", "Timeout")},
    "EntityKnife": {"Get" + name: "Set" + name for name in (
        "IsSpinAttack", "IsSwinging", "KnifeDistance", "KnifeVelocity", "HitboxParentKnife")},
    "EntityPickup": {"GetAlternatePedestal": "SetAlternatePedestal", "GetDropDelay": "SetDropDelay",
                     "GetVarData": "SetVarData", "IsBlind": "SetForceBlind"},
    # SetLilDelirium(true)会delirium_morph，不能作为后态恢复入口。
    "EntityFamiliar": {"GetMoveDelayNum": "SetMoveDelayNum"},
    "EntityEffect": {"GetTearFlags": "SetTearFlags"},
    "EntitySlot": {"Get" + name: "Set" + name for name in (
        "DonationValue", "PrizeCollectible", "PrizeType", "ShellGameAnimationIndex",
        "State", "Timeout", "Touch", "TriggerTimerNum")},
}

# 本地Wiki明确为可写值字段；const、身份、容器及派生引用另走具名恢复，不能反射任意远端成员。
PROPERTIES = {
    "Entity": "Child CollisionDamage Color DepthOffset EntityCollisionClass FlipX Friction GridCollisionClass HitPoints Mass MaxHitPoints Parent Position RenderZOffset Size SizeMulti SortingLayer SpawnerEntity SpawnerType SpawnerVariant SplatColor SpriteOffset SpriteRotation SpriteScale Target TargetPosition Velocity Visible".split(),
    "EntityNPC": "CanShutDoors EntityRef GroupIdx I1 I2 ProjectileCooldown ProjectileDelay Scale State StateFrame V1 V2".split(),
    "EntityDelirium": "Angle AttackID Cycle RemainingAttacks StateD TransformationTimer".split(),
    "EntityTear": "Bounced CanTriggerStreakEnd ContinueVelocity FallingAcceleration FallingSpeed Height HomingFriction KnockbackMultiplier ParentOffset Rotation Scale StickDiff StickTarget StickTimer TearFlags WaitFrames".split(),
    "EntityProjectile": "Acceleration ChangeFlags ChangeTimeout ChangeVelocity CurvingStrength Damage DepthOffset FallingAccel FallingSpeed Height HomingStrength ProjectileFlags Scale WiggleFrameOffset".split(),
    "EntityBomb": "ExplosionDamage Flags IsFetus RadiusMultiplier".split(),
    "EntityLaser": "Angle AngleDegrees BlackHpDropChance BounceLaser CurveStrength DisableFollowParent EndPoint FirstUpdate GridHit HomingType IsActiveRotating LaserLength LastAngleDegrees MaxDistance OneHit ParentOffset Radius RotationDegrees RotationDelay RotationSpd SampleLaser Shrink StartAngleDegrees TearFlags Timeout".split(),
    "EntityKnife": "Charge MaxDistance PathFollowSpeed PathOffset Rotation RotationOffset Scale TearFlags".split(),
    "EntityPickup": "AutoUpdatePrice Charge OptionsPickupIndex Price ShopItemId State Timeout Touched Wait".split(),
    "EntityFamiliar": "Coins FireCooldown HeadFrameDelay Hearts IsDelayed IsFollower Keys LastDirection MoveDirection OrbitAngleOffset OrbitDistance OrbitLayer OrbitSpeed Player RoomClearCount ShootDirection State".split(),
    "EntityEffect": "DamageSource FallingAcceleration FallingSpeed IsFollowing LifeSpan MaxRadius MinRadius ParentOffset Rotation Scale State Timeout m_Height".split(),
}

RELATIONS = {
    "Entity": ("Parent", "Child", "Target", "SpawnerEntity", "GetMinecart"),
    "EntityNPC": ("ParentNPC", "ChildNPC", "EntityRef", "GetSirenPlayerEntity"),
    "EntityTear": ("StickTarget",), "EntityLaser": ("BounceLaser",),
    "EntityKnife": ("GetHitboxParentKnife",),
    "EntityPickup": ("GetMegaChestLeftCollectible", "GetMegaChestRightCollectible", "GetPickupGhost"),
    "EntityFamiliar": ("Player", "GetActiveWeaponEntity"),
}

# 实际Getter/const字段读回由同一主线程适配器消费；参数化派生查询不在普通目标中伪造调用。
READONLY = {
    "Entity": "Type Variant SubType InitSeed FrameCount SpawnGridIndex DropSeed PositionOffset Exists CollidesWithGrid GetEntityFlags GetMinecart IsFlying IsBoss".split(),
    "EntityNPC": "GetChampionColorIdx GetBossColorIdx GetDarkRedChampionRegenTimer GetDirtColor GetSirenPlayerEntity IsBoss".split(),
    "EntityDelirium": "BossType BossVariant IsRedMode GetTeleportationTimer".split(),
    "EntityTear": "BaseDamage BaseScale TearIndex PosDisplacement GetDeadEyeIntensity".split(),
    "EntityBomb": "GetExplosionCountdown IsLoadingCostumes IsPrismTouched".split(),
    "EntityLaser": "GetSamples GetNonOptimizedSamples GetEndPoint IsSampleLaser IsMultidimensionalTouched IsPrismTouched GetRenderZ".split(),
    "EntityKnife": "IsFlying IsPrismTouched IsMultidimensionalTouched GetRenderZ".split(),
    "EntityPickup": "GetCollectibleCycle HasFlipData GetFlipCollectible GetMegaChestLeftCollectible GetMegaChestRightCollectible GetPickupGhost".split(),
    "EntityFamiliar": "IsLilDelirium GetDirtColor GetWeapon GetActiveWeaponEntity GetActiveWeaponNumFired GetMultiplier GetFollowerPriority".split(),
}

IDENTITY = {"Entity.Type", "Entity.Variant", "Entity.SubType", "Entity.InitSeed", "Entity.GetType"}
NATIVE = {
    "Entity.FrameCount": "J273出生帧相对同窗口Game帧恢复，禁止推进Update",
    "Entity.PositionOffset": "SDK Entity::_posOffset；只读Lua值不能当可写属性",
    "Entity.Exists": "原生寿命与成员owner；不以HP/可见性猜删除",
    "Entity.SpawnGridIndex": "J273 +32C当前来源格；不写远端Entity.Index",
    "Entity.DropSeed": "J273 +3DC RNG当前seed；不调用Next",
    "EntityLaser.GetSamples": "J273 +480原生vector<Vector>；游戏CRT分配及本方Factory析构",
    "EntityLaser.GetNonOptimizedSamples": "J273 +48C原生vector<Vector>；不调用CalculateSamples重推曲线",
    "EntityPickup.GetCollectibleCycle": "原生8槽当前集合；不能重新抽池",
    "EntityPickup.GetFlipCollectible": "原生保存态及资源owner；不能靠重新Flip重演",
    "EntityPickup.HasFlipData": "原生保存态存在性；与显式Null分别解释",
    "NativeText.counter": "J273 Text +0x410当前计数；需要合法Text owner",
    "NativeText.duration": "J273 Text +0x414当前寿命；不能靠Update推进",
    "NativeText.text": "J273 Text +0x418至多255字节正文及终止符；不读取未初始化尾部",
}
NATIVE.update({"EntityTear."+name: "native_entity_state.inc::NativeEntityCommon，原生当前值恢复" for name in ("BaseDamage","BaseScale","TearIndex","PosDisplacement","GetDeadEyeIntensity")})
NATIVE.update({"EntityNPC."+name: "native_entity_state.inc::NativeEntityDerived，原生当前值/规范引用恢复" for name in ("GetChampionColorIdx","GetBossColorIdx","GetDarkRedChampionRegenTimer","GetDirtColor","GetSirenPlayerEntity","IsBoss","IsChampion")})
NATIVE.update({"EntityDelirium."+name: "NPC +BC8/+BCC专用形态状态，与同key shape及Sprite一起恢复" for name in ("BossType","BossVariant")})
NATIVE.update({"EntityBomb."+name: "native_entity_state.inc::NativeEntityDerived；官方Lua对应SDK成员，不引爆/加载随机服装" for name in ("GetExplosionCountdown","IsLoadingCostumes","IsPrismTouched")})
NATIVE.update({"EntityKnife."+name: "native_entity_state.inc::NativeEntityDerived；官方Lua对应SDK bool" for name in ("IsPrismTouched","IsMultidimensionalTouched")})
NATIVE.update({"EntityLaser."+name: "native_entity_state.inc::NativeEntityDerived；官方Lua对应SDK bool" for name in ("IsPrismTouched","IsMultidimensionalTouched","IsSampleLaser")})
NATIVE.update({"EntityFamiliar."+name: "native_entity_state.inc::NativeEntityDerived；不调用delirium_morph或AI" for name in ("GetDirtColor","IsLilDelirium","GetItemConfig")})
NATIVE.update({"NativeText."+name: "J273 +518 KColor四个float与第5个32位原值；CheckNativeText实际读回" for name in ("red","green","blue","alpha","color_word")})
NATIVE.update({"EntityFamiliar."+name: "NativeEntityWeapon恢复真实weapon/current EntityPtr槽；原生Getter读回，不重新发射" for name in ("GetWeapon","GetActiveWeaponEntity","GetActiveWeaponNumFired")})
NATIVE["EntityFamiliar.GetMultiplier"]="REPENTOGON EntityFamiliarPlus真实optional缓存及本方Player key；不执行属性重算"
NATIVE["Entity.GetMinecart"]="公共EntityPtr SetReference；同scope当前shape解析及退休撤销"
NATIVE["EntityKnife.IsFlying"]="J273原版绑定606C10读取独立+464 bool，不能与Entity.IsFlying混用"
NATIVE["EntityKnife.GetRenderZ"]="原生Getter从最终Position/公共偏移派生；排序另恢复私有+434 float"
NATIVE["EntityLaser.GetRenderZ"]="最终Position/RenderZOffset与原生Getter；曲线样本原生vector恢复"


def application(member, ownership):
    """每行携带来源、入口/依赖、读回与缺口；实现状态独立于目录分类。"""
    owner = member.get("class", member.get("owner", ""))
    name, category = member.get("member", ""), member.get("category", "")
    if not (owner.startswith("Entity") or owner in (
            "Sprite", "LayerState", "BlendMode", "ColorParams", "NativeText", "NativeDummy")):
        return None
    rule = {"task": 19, "source_version": member.get("target", "REPENTOGON 1.1.3/J273"),
            "applicable_class": owner, "readback": member["id"],
            "units": member.get("units", "API原单位"),
            "source_status": "Value/Null/NotApplicable/NotCaptured/ReadFailed/Truncated各自保留",
            "implementation": "native_state_adapter;unified_e2e_pending"}
    if ownership != "ReplicatedState":
        rule.update(action=ownership, entry=None, dependency="不在Client执行原生玩法/随机/持久操作")
    elif owner == "EntityPlayer":
        rule.update(action="PlayerOwner", entry=None, dependency="19仅必要合法载体/引用；完整状态由20接管同key")
    elif owner.startswith("EntityConfig"):
        rule.update(action="ConfigurationReadback", entry="本地同版本原生配置/资源Getter",
                    dependency="由Type/Variant及内容版本选择；禁止将只读配置表作为可写游戏状态")
    elif member["id"] in IDENTITY:
        rule.update(action="Identity", entry="合法原生创建/形态重建", dependency="先核对实际派生类型及shape revision")
    elif name in RELATIONS.get(owner, ()):
        entry = (owner + "." + SETTERS[owner][name]) if name in SETTERS.get(owner, {}) else (
            member["id"] if name in PROPERTIES.get(owner, ()) else "game/native_entity_state.inc;同类Parent/Child原生派生Getter")
        rule.update(action="Reference", entry=entry,
                    dependency="同scope、当前generation、正确shape；Owner先行，普通关系第二阶段")
    elif name in SETTERS.get(owner, {}):
        rule.update(action="Setter", entry=owner + "." + SETTERS[owner][name],
                    dependency="最终值恢复；不调用Add状态效果、Shoot、Morph或Update重演结果")
    elif member["id"] in NATIVE:
        rule.update(action="NativeRestore", entry="game/native_entity_state.inc", dependency=NATIVE[member["id"]])
    elif member.get("returns") == "Sprite" or owner in ("Sprite", "LayerState", "BlendMode"):
        rule.update(action="ResourcePose", entry="18共享Sprite/LayerState资源恢复",
                    dependency="主/overlay/layer、资源版本、换图、连续姿态及源事件mask；不推进时钟或派发玩法事件")
    elif name in PROPERTIES.get(owner, ()):
        rule.update(action="Property", entry=member["id"],
                    dependency="公开可写字段白名单；引用解析当前代次，纯值恢复不执行玩法动作")
    elif owner=="EntityLaser" and name=="HomingLaser":
        rule.update(action="DerivedReadback",entry="EntityLaser.GetSamples/GetNonOptimizedSamples/GetEndPoint",
                    dependency="不传HomingLaser私有指针；实际语义曲线从公开容器恢复，内部重算向量不推进")
    elif category == "field":
        rule.update(action="DerivedReadback", entry="同类最终决定状态/原生Getter",
                    dependency="const/原生借用字段不反射写入；从具名状态或原生owner读回")
    else:
        rule.update(action="DerivedReadback", entry=None,
                    dependency="由实际决定状态及原生维护恢复；没有通用setter；参数查询保留独立scope")
    return rule
