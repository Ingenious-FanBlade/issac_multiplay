local config = assert(_LanMixed)
local native, frame, gate = assert(_IsaacLan), assert(_IsaacLanFrame), assert(_IsaacLan.net_gate)
local phases, json = native.net_phases(), require("json")
local renders, captures, applies = 0, 0, 0
local linked, chosen, started = false, false, false
local spawned, seed, windowStart, first, lastSample = false, 212121, nil, nil, 0
local samples = {}
local duration = 30000

local function room(slot, fn)
    return native.rooms_with_player(slot, fn)
end

local function write(name, data)
    local file = assert(io.open(name, "w"))
    assert(file:write(data))
    file:close()
end

local function readWindow()
    local file = io.open(config.window, "r")
    if not file then return nil end
    local value = tonumber(file:read("*a"))
    file:close()
    return value
end

local function activeEnemy(entity)
    return entity:Exists() and entity:IsActiveEnemy(false) and not entity:IsDead()
end

-- 在权威更新边界补发真实移动泪弹。消亡中但仍在名册里的泪弹占用名额。
local function supply()
    if not native.rooms_ready() or not started then return end
    local now = native.api_info().nowMs
    if windowStart and now > windowStart + duration + 800 then return end
    room(0, function()
        local game, enemies, tears = Game(), 0, 0
        if game:GetNumPlayers() ~= 2 then return end
        local current = game:GetRoom()
        if not spawned then
            assert(game:AchievementUnlocksDisallowed(), "fixture requires a seeded, achievement-disabled run")
            for _, entity in ipairs(Isaac.GetRoomEntities()) do
                if activeEnemy(entity) or entity.Type == 2 or entity.Type == 9 then entity:Remove() end
            end
            spawned = true
            return
        end
        for slot = 0, 7 do
            local door = current:GetDoor(slot)
            if door and door:IsOpen() then door:Close(true) end
        end
        for index = 0, 1 do
            local player = Isaac.GetPlayer(index)
            if player:GetSoulHearts() < 6 then player:AddSoulHearts(12 - player:GetSoulHearts()) end
        end
        for _, entity in ipairs(Isaac.GetRoomEntities()) do
            if activeEnemy(entity) then
                enemies = enemies + 1
                if enemies > 8 then entity:Remove() end
            elseif entity.Type == EntityType.ENTITY_TEAR then
                tears = tears + 1
                if tears > 64 then entity:Remove() end
            end
        end
        local center = current:GetCenterPos()
        for index = enemies + 1, 8 do
            seed = seed + 1
            local pos = current:FindFreePickupSpawnPosition(center + Vector((index % 4 - 2) * 55, -70), 0, true)
            assert(game:Spawn(10, 0, pos, Vector(1, 0), nil, 0, seed):ToNPC())
        end
        for index = tears + 1, 64 do
            local player = Isaac.GetPlayer((index - 1) % 2)
            player:FireTear(player.Position, Vector.FromAngle((index * 23) % 360) * 5, false, true, false, player, 1)
        end
    end)
    if spawned and not windowStart then
        windowStart = now + 5000
        write(config.window, tostring(windowStart))
    end
end

-- 以现有回调计数实际采集与成功应用，耗时仍由内置诊断记录。
native.net_gate = function(capture, before, collect, restore, present, beginFloor)
    return gate(capture, function(...)
        before(...)
        if config.host then supply() end
    end, function(...)
        local payload, stable = collect(...)
        captures = captures + 1
        return payload, stable
    end, function(...)
        local result = restore(...)
        if result ~= false then applies = applies + 1 end
        return result
    end, present, beginFloor)
end

local function observation(slot, now)
    local view = { now = now, tears = 0, moving_tears = 0, npc = 0, fx = 0, projectiles = 0, bodies = 0 }
    if not room(slot, function()
        view.bodies = Game():GetNumPlayers()
        for _, entity in ipairs(Isaac.GetRoomEntities()) do
            if entity.Type == 2 then
                view.tears = view.tears + 1
                if entity.Velocity:LengthSquared() > 0.01 then view.moving_tears = view.moving_tears + 1 end
            elseif entity.Type == 1000 then
                view.fx = view.fx + 1
            elseif entity.Type == 9 then
                view.projectiles = view.projectiles + 1
            elseif activeEnemy(entity) or (not config.host and entity:Exists() and not entity:IsDead() and entity:ToNPC()) then
                view.npc = view.npc + 1
            end
        end
    end) then return end
    samples[#samples + 1] = view
end

function _IsaacLanFrame()
    local status = frame()
    renders = renders + 1
    assert(status.phase ~= phases.failed and status.phase ~= phases.waiting, status.error)
    if not linked and renders >= (config.host and 280 or 460) then
        linked = true
        _IsaacLanCommand(config.host and "host" or "join", config.host and config.port or "127.0.0.1:" .. config.port)
        Isaac.DebugString("LAN_MIXED MENU_READY")
    end
    if status.phase == phases.lobby and not chosen then
        chosen = true
        _IsaacLanCommand("choose", "0:1")
    end
    if config.host and not started and status.phase == phases.lobby and status.players == 2 and status.ready0 == 1 and status.ready1 == 1 then
        started = true
        _IsaacLanCommand("start", "YV039KQF:0:0:0:0:0")
    end
    local now = native.api_info().nowMs
    if not config.host and not windowStart and now - lastSample >= 200 then
        lastSample, windowStart = now, readWindow()
    end
    if not windowStart or now < windowStart or status.phase ~= phases.running or not status.prepared or not native.rooms_ready() then return status end
    if not first then
        first = { now = now, renders = renders, captures = captures, applies = applies, tick = status.verified }
        lastSample = 0
    end
    if now >= windowStart + duration then
        write("./lan-mixed-result.json", json.encode({
            first = first, last = { now = now, renders = renders, captures = captures, applies = applies, tick = status.verified },
            start_ms = windowStart, end_ms = windowStart + duration, samples = samples,
        }))
        Isaac.DebugString("LAN_MIXED COMPLETE")
        _IsaacLanFrame = frame
    elseif now - lastSample >= 1000 then
        lastSample = now
        observation(status.slot, now)
    end
    return status
end
