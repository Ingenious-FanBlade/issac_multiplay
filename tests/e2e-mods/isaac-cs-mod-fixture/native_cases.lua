-- 原生用例只用公开游戏接口产生及观察结果。Host 显式执行场景动作，客机读 HUD 和可见状态。
return function(mod, CS, config, emit)
    local tag = "isaaccs_e2e_stat"
    local swap, replacement = SoundEffect.SOUND_PLOP, SoundEffect.SOUND_THUMBSUP
    local cancel, localSound = SoundEffect.SOUND_CHOIR_UNLOCK, SoundEffect.SOUND_BEEP
    local music, musicReplacement = Music.MUSIC_BOSS_RUSH, Music.MUSIC_CREDITS
    local sfx = SFXManager()
    local audioOrigin, override, bossFill, stageName = nil, false, nil, false

    -- 两端都记录指定 ID，意外进入客机回放的回调也会被记录。
    mod:AddCallback(ModCallbacks.MC_PRE_SFX_PLAY, function(_, id, volume, delay, loop, pitch, pan)
        if id ~= swap and id ~= replacement and id ~= cancel and id ~= localSound then return end
        local action = id == swap and "swap" or id == cancel and "cancel" or "observe"
        emit("sfx-pre", {origin = audioOrigin or "native", id = id, action = action,
            final = id == swap and replacement or id, volume = volume, pitch = pitch, pan = pan})
        if id == swap then return {replacement, 0.65, delay, loop, 1.1, -0.25} end
        if id == cancel then return false end
    end)
    mod:AddCallback(ModCallbacks.MC_POST_SFX_PLAY, function(_, id, volume, _delay, _loop, pitch, pan)
        if id ~= swap and id ~= replacement and id ~= cancel and id ~= localSound then return end
        emit("sfx-post", {origin = audioOrigin or "native", id = id, volume = volume, pitch = pitch, pan = pan})
    end)
    mod:AddCallback(ModCallbacks.MC_PRE_MUSIC_PLAY, function(_, id, volume, fade)
        if id ~= music and id ~= musicReplacement then return end
        emit("music-pre", {origin = audioOrigin or "native", id = id, volume = volume, fade = fade})
        if id == music then return {musicReplacement, 0.55} end
    end)

    -- 自定义 cache 的结果来自游戏自己的 EvaluateItems 路径。
    mod:AddCallback(ModCallbacks.MC_EVALUATE_CUSTOM_CACHE, function(_, player, key, value)
        if key ~= tag then return end
        emit("cache-evaluate", {ordinal = CS.GetParticipant(player), value = value})
        return 37.25
    end, tag)
    mod:AddCallback(ModCallbacks.MC_PLAYER_GET_HEART_LIMIT, function(_, _player, limit)
        if override then return math.min(limit, 10) end
    end)
    mod:AddCallback(ModCallbacks.MC_PLAYER_GET_ACTIVE_MAX_CHARGE, function(_, _item, _player, _varData, maximum)
        if override then return maximum + 2 end
    end)
    mod:AddCallback(ModCallbacks.MC_PLAYER_GET_ACTIVE_MIN_USABLE_CHARGE, function(_, _slot, _player, minimum)
        if override then return minimum + 1 end
    end)

    local actions = {}
    function actions.audio()
        audioOrigin = "authority"
        local ok, err = pcall(function()
            sfx:Play(swap, 1, 0, false, 1, 0)
            sfx:Play(cancel, 1, 0, false, 1, 0)
            MusicManager():Play(music, 0.8)
        end)
        audioOrigin = nil
        if not ok then error(err, 0) end
        emit("audio-request", {swap = swap, replacement = replacement, cancel = cancel,
            music = music, music_replacement = musicReplacement})
    end
    function actions.native()
        override = true
        local player = assert(CS.GetLocalPlayer(), "需要一个本端玩家")
        player:AddCustomCacheTag(tag, true)
        player:SetForceCamoEffect(true)
        bossFill = 0.375
        Game():GetHUD():SetBossHPBarFill(bossFill)
        emit("native-request", {ordinal = CS.GetParticipant(player), cache = 37.25, boss_fill = bossFill})
    end
    function actions.floor()
        Game():GetLevel():SetName("IsaacCS level override")
        stageName = true
    end
    actions["floor-config"] = function()
        Game():GetLevel():SetName("")
        stageName = false
        Isaac.SetCurrentFloorName("IsaacCS floor config")
        Isaac.SetCurrentFloorBackdrop(BackdropType.CAVES)
    end
    actions["restore-look"] = function()
        local player = CS.GetLocalPlayer()
        if player then player:SetForceCamoEffect(false) end
        override = false
        bossFill = nil
        if stageName then Game():GetLevel():SetName("") end
        stageName = false
    end
    local function playCutscene(clear)
        emit("cutscene-request", {id = config.cutscene, clear = clear})
        Isaac.PlayCutscene(config.cutscene, clear)
    end
    function actions.cutscene() playCutscene(false) end
    actions["cutscene-clear"] = function() playCutscene(true) end

    -- 读取公开查询；progress 的各个字段可独立与 Host、客机接入前的本机读值作对照。
    local function observe(player)
        local game, pgd = Game(), Isaac.GetPersistentGameData()
        local slot = ActiveSlot.SLOT_PRIMARY
        return {
            cache = player:GetCustomCacheValue(tag), camo = player:HasForcedCamoEffect(),
            health_type = player:GetHealthType(), heart_limit = player:GetHeartLimit(),
            active_item = player:GetActiveItem(slot), max_charge = player:GetActiveMaxCharge(slot),
            min_charge = player:GetActiveMinUsableCharge(slot), boss_fill = game:GetHUD():GetBossHPBarFill(),
            floor = game:GetLevel():GetName(), backdrop = game:GetRoom():GetBackdropType(),
            music = MusicManager():GetCurrentMusicID(), unlocked = pgd:Unlocked(config.achievement),
            achievement = config.achievement, event_counter = pgd:GetEventCounter(1),
            challenge = pgd:IsChallengeCompleted(1), collection = pgd:IsItemInCollection(CollectibleType.COLLECTIBLE_SAD_ONION),
            boss_killed = pgd:IsBossKilled(1), encounters = pgd:GetBestiaryEncounterCount(EntityType.ENTITY_GAPER, 0),
            kills = pgd:GetBestiaryKillCount(EntityType.ENTITY_GAPER, 0),
            deaths = pgd:GetBestiaryDeathCount(EntityType.ENTITY_GAPER, 0),
        }
    end

    return {
        actions = actions,
        observe = observe,
        playLocal = function()
            audioOrigin = "local-render"
            local ok, err = pcall(sfx.Play, sfx, localSound, 0.8, 0, false, 1, 0)
            audioOrigin = nil
            if not ok then error(err, 0) end
            -- Play 没有返回值。记录原生播放状态，区分按键已处理和音效实际启动。
            emit("local-sfx", {id = localSound, playing = sfx:IsPlaying(localSound)})
        end,
        update = function()
            if bossFill then Game():GetHUD():SetBossHPBarFill(bossFill) end
            if override then
                local player = CS.GetLocalPlayer()
                if player then
                    -- 主动道具查询从 Host 正常 Lua 入口执行，客机 HUD 才有相同参数的权威结果。
                    player:GetHeartLimit()
                    player:GetActiveMaxCharge(ActiveSlot.SLOT_PRIMARY)
                    player:GetActiveMinUsableCharge(ActiveSlot.SLOT_PRIMARY)
                end
            end
        end,
        reset = function()
            if stageName then Game():GetLevel():SetName("") end
            audioOrigin, override, bossFill, stageName = nil, false, nil, false
        end,
    }
end
