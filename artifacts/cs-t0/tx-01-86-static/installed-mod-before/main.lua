local mod = RegisterMod("Isaac CS World E2E Fixture", 1)
local game = Game()
local json = require("json")
local enabled, queue, done, serial = false, nil, {}, 0
local nesting, nested, reuse, grid, alignAt = false, false, nil, nil, nil
local morph, inputFrame = nil, nil
local nativeUse, gridFree, failed = nil, nil, false
local esauSettled = 0
local TAG = "[CSWORLD-E2E-v1] "

local function log(kind, data)
    data = data or {}
    serial = serial + 1
    data.sequence, data.kind = serial, kind
    data.seed = game:GetSeeds():GetStartSeed()
    data.frame = game:GetFrameCount()
    data.room = game:GetLevel():GetCurrentRoomIndex()
    data.players = game:GetNumPlayers()
    Isaac.DebugString(TAG .. json.encode(data))
end

local function failFixture(request, message)
    enabled, failed, queue, nativeUse = false, true, nil, nil
    log("fixture_error", {action=request, message=tostring(message)})
end

local function playerView(player)
    return {index=player.Index, init_seed=player.InitSeed, controller=player.ControllerIndex,
        player_type=player:GetPlayerType(), primary=player:GetActiveItem(ActiveSlot.SLOT_PRIMARY),
        charge=player:GetActiveCharge(ActiveSlot.SLOT_PRIMARY)}
end

local function currentA()
    if game:GetNumPlayers() < 2 then error("Wait for A/B before this scene") end
    local player = Isaac.GetPlayer(0) -- reacquire after each native Replace
    if player.Type ~= EntityType.ENTITY_PLAYER or player.Variant ~= 0 then
        error("Scene needs a full current A Player")
    end
    return player
end

local function armEsau(request)
    if nativeUse ~= nil then error("Previous native use is still pending") end
    if tonumber(request:match("(%d)$")) ~= esauSettled+1 then error("Esau requests must settle in order 1, 2, 3") end
    local player = currentA()
    if player:GetActiveItem(ActiveSlot.SLOT_PRIMARY) ~= CollectibleType.COLLECTIBLE_ESAU_JR then
        error("Current A has no primary Esau Jr.; do not re-equip to reset VarData")
    end
    if player:NeedsCharge(ActiveSlot.SLOT_PRIMARY) then player:FullCharge(ActiveSlot.SLOT_PRIMARY) end
    if player:NeedsCharge(ActiveSlot.SLOT_PRIMARY) then error("Esau Jr. could not be charged") end
    nativeUse = {request=request, before=playerView(player), armed_at=game:GetFrameCount(),
        deadline=game:GetFrameCount()+1800, seen=nil, callbacks=0}
    log("native_use_armed", {action=request, before=nativeUse.before,
        deadline=nativeUse.deadline, input="USER_PHYSICAL_ITEM_BUTTON_THROUGH_CANONICAL_SCHEDULER",
        coverage="REQUIRES_NATIVE_REPLACE_AND_RETAINED_MEMBERSHIP"})
    -- No UseActiveItem(703), input override, VarData write, or internal call.
    -- The user closes the console and presses/releases the normal item button.
end

-- D12's current native switch includes these Grid types. Conservatively refuse
-- all non-fixture members, including types whose native predicate may skip them.
local D12_TYPES = {[2]=true,[3]=true,[4]=true,[6]=true,[8]=true,[9]=true,[10]=true,
    [11]=true,[12]=true,[14]=true,[22]=true,[24]=true,[25]=true,[27]=true}

local function gridView(room, index)
    local entity = room:GetGridEntity(index)
    if entity == nil then return {index=index, present=false} end
    local desc = entity:GetSaveState()
    return {index=index, present=true, type=entity:GetType(), variant=entity:GetVariant(),
        spawn_seed=desc.SpawnSeed, state=entity.State}
end

local function gridLayout(room, ownedIndex)
    local size, layout = room:GetGridSize(), {}
    if size < 1 or size > 448 then error("Unsupported room Grid size") end
    for index=0,size-1 do
        local view = gridView(room, index)
        if view.present then
            if D12_TYPES[view.type] and index ~= ownedIndex then
                error("D12 would touch non-fixture Grid at index " .. index)
            end
            layout[#layout+1] = view
        end
    end
    return layout
end

local function spawnGridFreeFixture()
    currentA()
    local room, level = game:GetRoom(), game:GetLevel()
    local before = gridLayout(room, nil)
    local index = room:GetGridIndex(room:GetCenterPos() + Vector(80, 0))
    if index < 0 or index >= room:GetGridSize() or room:GetGridEntity(index) ~= nil then
        error("GridFree fixture slot is unavailable; no overwrite")
    end
    local ok = room:SpawnGridEntity(index, GridEntityType.GRID_ROCK, 0, 9730012, 0)
    if not ok then error("GridFree fixture SpawnGridEntity returned false") end
    local view = gridView(room, index)
    if not view.present or view.type ~= GridEntityType.GRID_ROCK or view.spawn_seed ~= 9730012 then
        error("GridFree fixture descriptor does not match the explicit spawn")
    end
    -- Native Grid Init can select an actual rock variant even when the public
    -- spawn request used zero. Freeze the observed descriptor, not the request.
    gridFree = {index=index, type=view.type, variant=view.variant, seed=9730012, run_seed=game:GetSeeds():GetStartSeed(),
        room=level:GetCurrentRoomIndex(), stage=level:GetStage(), stage_type=level:GetStageType(),
        spawned_at=game:GetFrameCount(), observation_at=nil}
    log("gridfree_fixture_created", {grid=view, previous_layout=before})
end

local function useD12ForGridFree()
    if gridFree == nil then error("Run gridfree-spawn first") end
    local room, level, player = game:GetRoom(), game:GetLevel(), currentA()
    if game:GetSeeds():GetStartSeed() ~= gridFree.run_seed or level:GetCurrentRoomIndex() ~= gridFree.room
        or level:GetStage() ~= gridFree.stage or level:GetStageType() ~= gridFree.stage_type then
        error("GridFree fixture room/run changed")
    end
    if game:GetFrameCount() <= gridFree.spawned_at then error("Wait a natural frame after Grid spawn") end
    local view = gridView(room, gridFree.index)
    local entity = room:GetGridEntity(gridFree.index)
    local rockOk = entity ~= nil and entity:ToRock() ~= nil
    log("gridfree_validation", {grid=view, expected={index=gridFree.index, type=gridFree.type,
        variant=gridFree.variant, spawn_seed=gridFree.seed}, to_rock=rockOk})
    if not view.present or view.type ~= gridFree.type or view.variant ~= gridFree.variant
        or view.spawn_seed ~= gridFree.seed or not rockOk then
        error("GridFree fixture changed; refusing D12")
    end
    local before = gridLayout(room, gridFree.index)
    log("gridfree_before_d12", {grid=view, layout=before, player=playerView(player),
        coverage="REQUIRES_NATIVE_GRIDFREE_HOOK_17_FOR_THIS_INDEX"})
    -- D12's ordinary effect dispatcher reaches 5BCA63 -> 81E980. This is an
    -- explicit fixture world action, not a native-function or input shortcut.
    player:UseActiveItem(CollectibleType.COLLECTIBLE_D12,
        UseFlag.USE_NOANIM | UseFlag.USE_NOCOSTUME, -1, 0)
    gridFree.observation_at = game:GetFrameCount()+3
    log("gridfree_d12_return", {index=gridFree.index, grid=gridView(room, gridFree.index)})
end

mod:AddCallback(ModCallbacks.MC_USE_ITEM, function(_, item, _rng, player, flags, slot, varData)
    if not enabled or nativeUse == nil or item ~= CollectibleType.COLLECTIBLE_ESAU_JR then return end
    local ok, message = pcall(function()
        nativeUse.callbacks = nativeUse.callbacks+1
        log("native_use_callback", {action=nativeUse.request, item=item, flags=flags, slot=slot,
            var_data=varData, callback_count=nativeUse.callbacks, player=playerView(player)})
        if nativeUse.callbacks ~= 1 or slot ~= ActiveSlot.SLOT_PRIMARY
            or player.ControllerIndex ~= nativeUse.before.controller then
            error("Duplicate or mismatched Esau Jr. use; no retry")
        end
        nativeUse.seen = game:GetFrameCount()
    end)
    if not ok then failFixture("native-use-callback", message) end
    -- No return value: preserve native item discharge, animation and effects.
end, CollectibleType.COLLECTIBLE_ESAU_JR)

local function spawn(label, kind, variant, subtype, seed, parent)
    -- Explicit nonzero seed: no Random(), no seed-zero path in Isaac.Spawn.
    local entity = game:Spawn(kind, variant, Vector(400, 200), Vector(0, 0), parent, subtype, seed)
    if entity == nil then error("Spawn returned nil: " .. label) end
    log("spawn", {operation=label, type=entity.Type, variant=entity.Variant,
        subtype=entity.SubType, init_seed=entity.InitSeed, index=entity.Index})
    return entity
end

mod:AddCallback(ModCallbacks.MC_POST_PICKUP_INIT, function(_, pickup)
    if not enabled or not nesting or nested or pickup.InitSeed ~= 9730001 then return end
    nested = true
    log("nested_begin", {parent_seed=pickup.InitSeed})
    local child = spawn("nested_child", EntityType.ENTITY_EFFECT, 15, 0, 9730002, pickup)
    child:Remove()
    log("nested_end", {parent_seed=pickup.InitSeed})
end)

local function scene()
    nesting, nested = true, false
    local pickup = spawn("nested_parent", EntityType.ENTITY_PICKUP, PickupVariant.PICKUP_COIN, 1, 9730001, nil)
    nesting = false
    log("nested_result", {callback_seen=nested})
    pickup:Remove()
    local short = spawn("short_lived", EntityType.ENTITY_EFFECT, 15, 0, 9730003, nil)
    short:Remove()
    log("short_removed", {operation="short_lived"})
    local npc = spawn("npc_morph", EntityType.ENTITY_FLY, 0, 0, 9730004, nil):ToNPC()
    if npc == nil then error("NPC conversion failed") end
    local p = spawn("pickup_morph", EntityType.ENTITY_PICKUP, PickupVariant.PICKUP_COIN, 1, 9730005, nil):ToPickup()
    if p == nil then error("Pickup conversion failed") end
    morph = {npc=npc, pickup=p, at=game:GetFrameCount()+10, changed=false}
    local unknown = spawn("unknown_bomb", EntityType.ENTITY_BOMB, 0, 0, 9730006, nil)
    reuse = {entity=unknown, start=game:GetFrameCount(), count=0}
    log("scene_ready")
end

local function action(name)
    if done[name] then error("Action already used; no retry: " .. name) end
    done[name] = true
    log("action_begin", {action=name})
    if name == "scene" then scene()
    elseif name == "esau-equip" then
        local player = currentA()
        if player:GetActiveItem(ActiveSlot.SLOT_PRIMARY) == CollectibleType.COLLECTIBLE_ESAU_JR then
            error("Esau Jr. already equipped; do not reset its context")
        end
        player:AddCollectible(CollectibleType.COLLECTIBLE_ESAU_JR, 6, false, ActiveSlot.SLOT_PRIMARY, 0)
        log("esau_equipped", {player=playerView(player)})
    elseif name:match("^esau%-use%-[123]$") then armEsau(name)
    elseif name == "gridfree-spawn" then spawnGridFreeFixture()
    elseif name == "gridfree-use" then useD12ForGridFree()
    elseif name == "hourglass-equip" then
        Isaac.GetPlayer(0):AddCollectible(CollectibleType.COLLECTIBLE_GLOWING_HOUR_GLASS, 6, true, ActiveSlot.SLOT_PRIMARY)
    elseif name == "hourglass-use" then
        local p = Isaac.GetPlayer(0)
        if p:GetActiveItem(ActiveSlot.SLOT_PRIMARY) ~= CollectibleType.COLLECTIBLE_GLOWING_HOUR_GLASS then error("Hour Glass not equipped") end
        p:UseActiveItem(CollectibleType.COLLECTIBLE_GLOWING_HOUR_GLASS, UseFlag.USE_NOANIM | UseFlag.USE_OWNED, ActiveSlot.SLOT_PRIMARY)
    elseif name == "grid-spawn" then
        local room = game:GetRoom()
        grid = room:GetGridIndex(room:GetCenterPos() + Vector(80, 0))
        if room:GetGridEntity(grid) ~= nil then error("Fixture grid slot occupied; no overwrite") end
        local ok = room:SpawnGridEntity(grid, GridEntityType.GRID_ROCK, 0, 9730010, 0)
        log("grid_spawn_return", {index=grid, result=ok})
    elseif name == "grid-remove" then
        if grid == nil then error("No fixture grid") end
        game:GetRoom():RemoveGridEntity(grid, 0, false)
    elseif name == "grid-respawn" then
        if grid == nil or game:GetRoom():GetGridEntity(grid) ~= nil then error("Grid removal not complete") end
        log("grid_spawn_return", {index=grid, result=game:GetRoom():SpawnGridEntity(grid, GridEntityType.GRID_ROCK, 0, 9730011, 0)})
    elseif name == "witness" then log("witness")
    else error("Unknown fixture action: " .. name) end
    log("action_end", {action=name})
end

mod:AddCallback(ModCallbacks.MC_EXECUTE_CMD, function(_, command, parameters)
    if command ~= "cswe2e" then return end
    if parameters == "enable" then
        if failed then log("refused", {reason="fixture failed; a fresh run is required"}); return end
        if enabled then log("refused", {reason="already enabled"}); return end
        if game:GetNumPlayers() < 1 or game:GetSeeds():GetStartSeed() == 0 then return end
        enabled = true
        log("enabled", {fixture="tx-01-85", input_override=false})
        return
    end
    if not enabled then Isaac.ConsoleOutput("Run cswe2e enable in the authorized seeded local run first.\n"); return end
    if nativeUse ~= nil or (gridFree ~= nil and gridFree.observation_at ~= nil) then
        log("refused", {reason="native operation pending; wait for its recorded observation"}); return
    end
    local frame = parameters:match("^align (%d+)$")
    if frame then
        frame = tonumber(frame)
        if alignAt ~= nil or frame < game:GetFrameCount()+120 then log("refused", {reason="alignment frame too early or repeated"}); return end
        alignAt = frame
        log("alignment_armed", {target=frame})
        return
    end
    local allowed = {scene=true, ["esau-equip"]=true,["esau-use-1"]=true,["esau-use-2"]=true,["esau-use-3"]=true,
        ["hourglass-equip"]=true,["hourglass-use"]=true,["grid-spawn"]=true,["grid-remove"]=true,["grid-respawn"]=true,
        ["gridfree-spawn"]=true,["gridfree-use"]=true,witness=true}
    if queue ~= nil or not allowed[parameters] then log("refused", {reason="pending or unknown action"}); return end
    queue = parameters
end)

mod:AddCallback(ModCallbacks.MC_POST_UPDATE, function()
    -- Passive startup evidence precedes injection. No MC_INPUT_ACTION override;
    -- raw values and later canonical controller events remain separate sources.
    local frame = game:GetFrameCount()
    if inputFrame ~= frame then
        inputFrame = frame
        local players = {}
        for i=0,game:GetNumPlayers()-1 do
            local p, values = Isaac.GetPlayer(i), {}
            for a=0,11 do values[a+1] = Input.GetActionValue(a, p.ControllerIndex) end
            players[#players+1] = {ordinal=i, controller=p.ControllerIndex, values=values}
        end
        log("raw_input", {observed=players})
    end
    if not enabled then return end
    if queue ~= nil then
        local request = queue; queue = nil
        local ok, message = pcall(action, request)
        if not ok then failFixture(request, message); return end
    end
    if nativeUse ~= nil then
        local ok, message = pcall(function()
            if nativeUse.seen ~= nil and frame >= nativeUse.seen+90 then
                log("native_use_settled", {action=nativeUse.request, before=nativeUse.before,
                    after=playerView(currentA()), callbacks=nativeUse.callbacks,
                    coverage="LUA_OBSERVATION_ONLY_NATIVE_IDENTITY_EVIDENCE_STILL_REQUIRED"})
                esauSettled, nativeUse = esauSettled+1, nil
            elseif frame >= nativeUse.deadline then error("Native Esau use did not settle before the natural-frame deadline") end
        end)
        if not ok then failFixture("native-use-wait", message); return end
    end
    if gridFree ~= nil and gridFree.observation_at ~= nil and frame >= gridFree.observation_at then
        local ok, message = pcall(function()
            local level = game:GetLevel()
            if level:GetCurrentRoomIndex() ~= gridFree.room or level:GetStage() ~= gridFree.stage
                or level:GetStageType() ~= gridFree.stage_type then error("Room changed before D12 observation") end
            local room, layout = game:GetRoom(), {}
            for index=0,room:GetGridSize()-1 do
                local view = gridView(room, index)
                if view.present then layout[#layout+1] = view end
            end
            log("gridfree_settled", {index=gridFree.index, grid=gridView(room, gridFree.index), layout=layout,
                coverage="LUA_OBSERVATION_ONLY_NATIVE_FREE_AND_NEW_USE_EVIDENCE_STILL_REQUIRED"})
            gridFree.observation_at = nil
        end)
        if not ok then failFixture("gridfree-wait", message); return end
    end
    if morph ~= nil and frame >= morph.at then
        if not morph.changed then
            local changed = morph.npc:Morph(EntityType.ENTITY_ATTACKFLY, 0, 0, -1)
            log("npc_morph_return", {result=changed, type=morph.npc.Type, variant=morph.npc.Variant, init_seed=morph.npc.InitSeed})
            morph.pickup:Morph(EntityType.ENTITY_PICKUP, PickupVariant.PICKUP_HEART, 1, false, false, true)
            log("pickup_morph_return", {type=morph.pickup.Type, variant=morph.pickup.Variant, init_seed=morph.pickup.InitSeed})
            morph.changed, morph.at = true, frame+10
        else morph.npc:Remove(); morph.pickup:Remove(); morph=nil end
    end
    if reuse ~= nil and game:GetFrameCount() >= reuse.start+4 then
        if reuse.entity ~= nil then reuse.entity:Remove(); reuse.entity=nil end
        if reuse.count < 256 then
            reuse.count = reuse.count+1
            local bomb = spawn("pool_reuse_" .. reuse.count, EntityType.ENTITY_BOMB, 0, 0, 9740000+reuse.count, nil)
            bomb:Remove() -- one per natural frame; no explosion, forced pool overwrite, or recursive Room.Update
        else log("reuse_finished", {spawns=reuse.count}); reuse=nil end
    end
    if alignAt ~= nil and game:GetFrameCount() == alignAt then
        log("aligned_frame", {target=alignAt})
    end
end)

mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED, function()
    enabled, queue, done, serial = false, nil, {}, 0
    nesting, nested, reuse, grid, alignAt = false, false, nil, nil, nil
    morph, inputFrame = nil, nil
    nativeUse, gridFree, failed = nil, nil, false
    esauSettled = 0
    log("run_start", {startup_witness=true})
end)
