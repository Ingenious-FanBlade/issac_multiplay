-- 测试专用夹具；只有 tx-04-05 临时配置启用跳帧与切房，不属于产品 gameplay mod。
local fixture = RegisterMod("Isaac CS E2E fixture", 1)
local config = include("harness_config")
local json = require("json")
local menuDone, runDone, panelShown = false, false, false
-- 界面 mod 的窗口 id；只在 E2E 中由本夹具强制显示。
local PANEL = "isaac-cs-panel"

-- 加载阶段另作记录，不能代替实际菜单时的 Q14 观察。
local function observe(token, phase)
    local success, response = pcall(Isaac.ExecuteCommand, "cs echo " .. token)
    Isaac.DebugString("[isaac-cs-test] " .. token .. " phase=" .. phase
        .. " success=" .. tostring(success)
        .. " response=" .. tostring(response):gsub("\n", "|"))
end

-- 当前玩家数：GetNumPlayers 是 Game 对象的方法，Isaac 命名空间没有这个函数。
local function playerCount()
    local ok, count = pcall(function() return Game():GetNumPlayers() end)
    if ok and type(count) == "number" then return count end
    return 0
end

-- REPENTOGON 在第一帧 ImGui 时按 imgui.ini 恢复窗口可见性，会覆盖界面 mod 加载时的设置。
-- 夹具在每次主菜单渲染时重新显示窗口，让 E2E 不依赖上一次运行留下的 imgui.ini。
local function showPanel()
    local ok, err = pcall(ImGui.SetVisible, PANEL, true)
    if not panelShown then
        panelShown = true
        Isaac.DebugString("[isaac-cs-test] panel-visible phase=menu success=" .. tostring(ok)
            .. (ok and "" or (" error=" .. tostring(err))))
    end
end

observe("lua-menu", "load")

fixture:AddCallback(ModCallbacks.MC_MAIN_MENU_RENDER, function()
    if ImGui then showPanel() end
    if menuDone then return end
    menuDone = true
    observe("lua-menu", "menu")
end)
fixture:AddCallback(ModCallbacks.MC_POST_RENDER, function()
    if runDone then return end
    if playerCount() == 0 then return end
    runDone = true
    observe("lua-run", "run")
end)

if config.tx ~= "tx-04-05" and config.tx ~= "tx-04-06" then return end
local logPrefix = config.tx == "tx-04-06" and "tx06" or "tx05"

-- 每条结构化记录带本次部署标识和 Manager 快照轮次，拒绝沿用旧 log.txt 的记录。
local render, serial, skipRemaining, skipCount = 0, 0, 0, 0
local skipEnding, pendingRoom, previousObservation = false, nil, nil
local f8Presses = 0
local failed = false
local function emit(event, data)
    data = data or {}
    serial = serial + 1
    data.event, data.token, data.serial, data.render = event, config.token, serial, render
    local response = Isaac.ExecuteCommand("cs status")
    data.loop = tonumber(tostring(response):match("\nframe=(%d+)"))
    data.game_frame = Game():GetFrameCount()
    Isaac.DebugString("[isaac-cs-test] " .. logPrefix .. " " .. json.encode(data))
end

-- tx-04-06 首轮只从实际主菜单发起一次 Host，覆盖 Lua 到排队命令的完整路径。
local hostRequested = false
if config.tx == "tx-04-06" then
    fixture:AddCallback(ModCallbacks.MC_MAIN_MENU_RENDER, function()
        if hostRequested then return end
        local response = tostring(Isaac.ExecuteCommand("cs status"))
        if not response:find("\nstate=Idle\n", 1, true) then return end
        hostRequested = true
        local result = Isaac.ExecuteCommand("cs host " .. tostring(config.host_port))
        emit("menu-host", { response = tostring(result) })
    end)
end

-- 房间身份包含楼层和 seed；房间变化不是最终 Player 位置稳定的证明。
local function roomIdentity()
    local level = Game():GetLevel()
    local desc = level:GetCurrentRoomDesc()
    return { index = level:GetCurrentRoomIndex(), stage = level:GetStage(),
        stage_type = level:GetStageType(), seed = desc.SpawnSeed }
end
local function sameRoom(a, b)
    return a.index == b.index and a.stage == b.stage
        and a.stage_type == b.stage_type and a.seed == b.seed
end

-- 固定按 DoorSlot 顺序选取实际存在、已开启且不带锁的普通相邻门；找不到就记录失败。
local function requestRoom()
    if pendingRoom then emit("room-transition", { phase = "rejected", reason = "pending" }); return end
    local game, source = Game(), roomIdentity()
    local room, level = game:GetRoom(), game:GetLevel()
    for slot = 0, 7 do
        local door = room:GetDoor(slot)
        if door and door:IsOpen() and not door:IsLocked() and door.TargetRoomIndex >= 0
            and door.TargetRoomIndex ~= source.index then
            local target = level:GetRoomByIdx(door.TargetRoomIndex, -1)
            if target and target.Data and target.Data.Type == RoomType.ROOM_DEFAULT then
                pendingRoom = { source = source, target = door.TargetRoomIndex,
                    direction = door.Direction, slot = slot, started = render }
                emit("room-transition", { phase = "request", source = source,
                    target = pendingRoom.target, direction = pendingRoom.direction,
                    slot = slot, animation = 0, dimension = -1 })
                -- 全参数；不把调用返回当作过渡成功，也不写入 Player 或门状态。
                game:StartRoomTransition(pendingRoom.target, pendingRoom.direction, 0, nil, -1)
                return
            end
        end
    end
    emit("room-transition", { phase = "rejected", reason = "no_open_adjacent_normal_room", source = source })
end

-- 30 次 true 之后，在下一次允许更新前写 end；区间边界可直接与 Manager loop 对齐。
fixture:AddCallback(ModCallbacks.MC_PRE_UPDATE, function()
    if failed then return end
    if skipRemaining > 0 then
        skipRemaining, skipCount = skipRemaining - 1, skipCount + 1
        if skipRemaining == 0 then skipEnding = true end
        return true
    end
    if skipEnding then
        skipEnding = false
        emit("skip-update", { phase = "end", count = skipCount })
    end
end)

-- 所有观测均为只读，变化时记录位置、资源和实体计数；静止时每 30 render 留心跳。
local function renderFixture()
    render = render + 1
    local count = playerCount()
    if count < 1 then return end
    local player = Isaac.GetPlayer(0)
    -- F8 按次序复用（D21）：第一次触发跳帧，第二次触发切房，之后的按键记为夹具错误。
    -- F9 是原版手动截图热键，会在用户 Documents 中留下截图，不再使用。
    if Input.IsButtonTriggered(Keyboard.KEY_F8, 0) then
        f8Presses = f8Presses + 1
        if f8Presses == 1 then
            skipRemaining, skipCount = 30, 0
            emit("skip-update", { phase = "start", requested = 30 })
        elseif f8Presses == 2 and (skipRemaining > 0 or skipEnding) then
            emit("room-transition", { phase = "rejected", reason = "skip_pending" })
        elseif f8Presses == 2 then
            requestRoom()
        else
            emit("fixture-error", { reason = "unexpected_f8", presses = f8Presses })
        end
    end
    local identity = roomIdentity()
    if pendingRoom then
        if not sameRoom(identity, pendingRoom.source) then
            emit("room-transition", { phase = "arrived", source = pendingRoom.source,
                target = pendingRoom.target, actual = identity,
                target_matched = identity.index == pendingRoom.target })
            pendingRoom = nil
        elseif render - pendingRoom.started > 600 then
            emit("room-transition", { phase = "timeout", target = pendingRoom.target, actual = identity })
            pendingRoom = nil
        end
    end
    local observation = { room = identity, x = player.Position.X, y = player.Position.Y,
        bombs = player:GetNumBombs(), tears = Isaac.CountEntities(nil, EntityType.ENTITY_TEAR, -1, -1),
        bomb_entities = Isaac.CountEntities(nil, EntityType.ENTITY_BOMB, -1, -1),
        controller = player.ControllerIndex, player_type = player:GetPlayerType() }
    -- A 的旧字段保持供 tx-04-05 使用；tx-04-06 每次同时复制所有 Player 的只读状态。
    observation.players, observation.player_count = {}, count
    for i = 0, count - 1 do
        local p = Isaac.GetPlayer(i)
        observation.players[#observation.players + 1] = { ordinal = i, x = p.Position.X, y = p.Position.Y,
            controller = p.ControllerIndex, player_type = p:GetPlayerType(), type = p.Type, variant = p.Variant,
            ghost = p:IsCoopGhost(), dead = p:IsDead(), bombs = p:GetNumBombs() }
    end
    local encoded = json.encode(observation)
    if encoded ~= previousObservation or render % 30 == 0 then
        previousObservation = encoded
        emit("observation", observation)
    end
end
fixture:AddCallback(ModCallbacks.MC_POST_RENDER, function()
    if failed then return end
    local ok, err = pcall(renderFixture)
    if not ok then
        failed = true
        Isaac.DebugString("[isaac-cs-test] " .. logPrefix .. " " .. json.encode({ event = "fixture-error",
            token = config.token, error = tostring(err), render = render }))
    end
end)
