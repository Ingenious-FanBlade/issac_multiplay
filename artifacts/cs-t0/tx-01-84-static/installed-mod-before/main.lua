local mod = RegisterMod("Isaac CS World E2E Fixture", 1)
local game = Game()
local json = require("json")
local enabled, queue, done, serial = false, nil, {}, 0
local nesting, nested, reuse, grid, alignAt = false, false, nil, nil, nil
local morph, inputFrame = nil, nil
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
        Isaac.GetPlayer(0):AddCollectible(CollectibleType.COLLECTIBLE_ESAU_JR, 6, true, ActiveSlot.SLOT_PRIMARY)
    elseif name:match("^esau%-use%-%d$") then
        local p = Isaac.GetPlayer(0)
        if p:GetActiveItem(ActiveSlot.SLOT_PRIMARY) ~= CollectibleType.COLLECTIBLE_ESAU_JR then error("Esau Jr. not equipped") end
        p:UseActiveItem(CollectibleType.COLLECTIBLE_ESAU_JR, UseFlag.USE_NOANIM | UseFlag.USE_OWNED, ActiveSlot.SLOT_PRIMARY)
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
        if enabled then log("refused", {reason="already enabled"}); return end
        if game:GetNumPlayers() < 1 or game:GetSeeds():GetStartSeed() == 0 then return end
        enabled = true
        log("enabled", {fixture="tx-01-80", input_override=false})
        return
    end
    if not enabled then Isaac.ConsoleOutput("Run cswe2e enable in the authorized seeded local run first.\n"); return end
    local frame = parameters:match("^align (%d+)$")
    if frame then
        frame = tonumber(frame)
        if alignAt ~= nil or frame < game:GetFrameCount()+120 then log("refused", {reason="alignment frame too early or repeated"}); return end
        alignAt = frame
        log("alignment_armed", {target=frame})
        return
    end
    local allowed = {scene=true, ["esau-equip"]=true,["esau-use-1"]=true,["esau-use-2"]=true,["esau-use-3"]=true,
        ["hourglass-equip"]=true,["hourglass-use"]=true,["grid-spawn"]=true,["grid-remove"]=true,["grid-respawn"]=true,witness=true}
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
        if not ok then log("fixture_error", {action=request, message=tostring(message)}); enabled=false; return end
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
    log("run_start", {startup_witness=true})
end)
