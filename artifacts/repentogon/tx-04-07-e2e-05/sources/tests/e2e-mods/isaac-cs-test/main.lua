-- 测试专用夹具；tx-04-05/06 临时配置启用跳帧与切房，tx-04-07 启用 B 死亡/复活/换层与激光观测。
-- 本文件只用于 E2E，不是产品 gameplay mod；所有世界动作仅由 harness 通过 F8 显式触发。
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

local knownMode = config.tx == "tx-04-05" or config.tx == "tx-04-06" or config.tx == "tx-04-07"
if not knownMode then return end

-- tx-04-05/06：保留既有行为（F8 第一次跳帧、第二次切房），供旧场景使用。
if config.tx ~= "tx-04-07" then
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

    return
end

-- ============================================================================
-- tx-04-07：由 harness 生成的动作表驱动；动作顺序固定为
-- skip -> room -> kill_b -> revive_b -> stage -> room。
-- 每次 F8 只接受一个动作；完成前再次按下记为 rejected，不推进动作序号。
-- 本夹具不重试动作，也不把 Lua 返回值当作 DLL 绑定或 generation 证据。
-- ============================================================================
local logPrefix = "tx07"
-- 主线程渲染计数；用于超时、心跳与日志关联，不等同于 Authority tick。
local render, serial = 0, 0
local failed = false
local configError = nil
-- 当前唯一 pending 动作；结构见 startAction。
local pending = nil
-- 下一个待执行动作在 actions 中的下标（1 基）。
local nextIndex = 1
-- 每个 F8 请求都分配一个 action_id（同时作为 request_id），包括被拒绝的非法请求。
local requestSerial = 0
-- skip 动作的 MC_PRE_UPDATE 计数状态。
local skipRemaining, skipCount, skipEnding = 0, 0, false
-- 配置规范化后的动作表，每项 { name, timeout_ms }。
local actions = {}
-- 激光观测：每次换房/新 run 递增 epoch，避免拿旧房间地址作跨房去重。
local laserEpoch, laserSerial = 0, 0
local laserInitIds = {}
local laserBaselineDone = false
local laserSpawnEvents, laserOverflowReported = 0, false
-- 复活后的动画观察器（独立于动作终态，避免把 Appear 未完成当作重绑完成）。
local animationWatch = nil
-- 同一游戏帧内只查询一次 cs status，减少激光密集出生时的命令桥负载。
-- 主菜单自动 Host 只做一次；第 4 轮 Continue 仍由控制口 Host。
local hostRequested = false
local lastObservation = nil

local ACTION_TIMEOUTS = {
    skip = 10000,
    room = 30000,
    kill_b = 30000,
    revive_b = 40000,
    stage = 60000,
}
local ACTION_NAMES = {
    skip = true,
    room = true,
    kill_b = true,
    revive_b = true,
    stage = true,
}
-- 配置未提供动作表时使用与场景一致的默认序列；正式 harness 必须写出显式动作表。
local DEFAULT_ACTIONS = { "skip", "room", "kill_b", "revive_b", "stage", "room" }

-- 普通 Isaac 的 player type 值；harness 可在配置中为其他角色覆盖。
local expectedBPlayerType = tonumber(config.expected_b_player_type)
    or (PlayerType and PlayerType.PLAYER_ISAAC) or 0
local observationHeartbeat = tonumber(config.observation_heartbeat) or 30
local laserEventCap = tonumber(config.laser_event_cap) or 512
-- 心跳必须为正，避免 render % 0；事件上限为负时按 0 处理并显式溢出。
observationHeartbeat = math.max(1, math.floor(observationHeartbeat))
laserEventCap = math.max(0, math.floor(laserEventCap))
-- 动画观察上限；Appear 的实际帧数未知，因此只作为超时报告，不主动结束动画。
local animationTimeoutMs = tonumber(config.animation_timeout_ms) or 30000
local menuHostEnabled = config.menu_host ~= false

-- 把毫秒换算为游戏帧；Isaac 文档明确 Game:GetFrameCount() 为 30 帧/秒且不计暂停。
-- 超时以该计数为准，避免渲染停顿或暂停把夹具超时提前。
local function framesFor(milliseconds)
    return math.max(1, math.floor(milliseconds * 30 / 1000 + 0.5))
end

local function gameFrame()
    local ok, frame = pcall(function() return Game():GetFrameCount() end)
    if ok and type(frame) == "number" then return frame end
    return 0
end

-- 每次事件读取当前 Manager loop；暂停/跳帧时 Game frame 不变而 Manager 仍推进，
-- 因此不能用 Game frame 缓存 loop，更不能让旧值冒充动作结束时刻。
local function currentLoop()
    local ok, response = pcall(Isaac.ExecuteCommand, "cs status")
    if ok then return tonumber(tostring(response):match("\nframe=(%d+)")) end
    return nil
end

-- 所有 tx-04-07 结构化记录统一前缀，harness 按 token 过滤本次部署。
local function emit(event, data)
    data = data or {}
    serial = serial + 1
    data.event = event
    data.token = config.token
    data.serial = serial
    data.render = render
    data.game_frame = gameFrame()
    data.loop = currentLoop()
    Isaac.DebugString("[isaac-cs-test] " .. logPrefix .. " " .. json.encode(data))
end

-- Lua 夹具自身错误：只记录一次并停止后续动作，不伪造动作成功。
local function failFixture(reason, extra)
    if failed then return end
    failed = true
    local current = pending
    pending = nil
    skipRemaining, skipEnding = 0, false
    local data = { reason = reason }
    if current ~= nil then
        data.action = current.name
        data.action_id = current.action_id
    end
    if extra then
        for key, value in pairs(extra) do data[key] = value end
    end
    emit("fixture-error", data)
end

-- 关卡/房间身份；加载窗口内读取失败时返回 error 字段，调用方必须等待而不是伪造变化。
local function roomIdentity()
    local ok, identity = pcall(function()
        local level = Game():GetLevel()
        local desc = level:GetCurrentRoomDesc()
        return { index = level:GetCurrentRoomIndex(), stage = level:GetStage(),
            stage_type = level:GetStageType(), seed = desc and desc.SpawnSeed or nil }
    end)
    if ok then return identity end
    return { error = tostring(identity) }
end

-- 只读 Player 视图；单个字段读取失败不会改变世界，也不会把对象当成健康 Player。
local function playerView(player, ordinal)
    if player == nil then return { ordinal = ordinal, present = false } end
    local ok, view = pcall(function()
        -- GetPlayer 缺席时可能回退；用 PlayerManager 中的 player index 复核 ordinal。
        -- Entity.Index 是实体编号，多轮新建 B 可为 17 等值，不能用于玩家槽判断。
        local playerIndex = player:GetPlayerIndex()
        if playerIndex ~= ordinal then
            return { ordinal = ordinal, present = false, resolved_ordinal = playerIndex,
                entity_index = player.Index }
        end
        -- harness 只从当前实际可通行的方向选择移动；逐点检查路径，不修改格子或玩家。
        local movementOptions = {}
        for _, direction in ipairs({ { 32767, 0 }, { -32767, 0 }, { 0, 32767 }, { 0, -32767 } }) do
            local clear = true
            for distance = 10, 80, 10 do
                local target = player.Position + Vector(direction[1] / 32767 * distance, direction[2] / 32767 * distance)
                if not Game():GetRoom():IsPositionInRoom(target, 20)
                    or Game():GetRoom():GetGridCollisionAtPos(target) ~= GridCollisionClass.COLLISION_NONE then
                    clear = false
                    break
                end
            end
            if clear then movementOptions[#movementOptions + 1] = direction end
        end
        return {
            ordinal = ordinal, present = true, exists = player:Exists(),
            type = player.Type, variant = player.Variant, index = player.Index,
            entity_index = player.Index, player_index = playerIndex,
            init_seed = player.InitSeed, ptr_hash = GetPtrHash(player),
            controller = player.ControllerIndex, player_type = player:GetPlayerType(),
            dead = player:IsDead(), ghost = player:IsCoopGhost(),
            animation_finished = player:IsExtraAnimationFinished(),
            hearts = player:GetHearts(), soul_hearts = player:GetSoulHearts(),
            -- GetBlackHearts 返回的是位掩码，不是黑心数量；这里按原值记录，避免误当数量。
            black_heart_mask = player:GetBlackHearts(),
            bone_hearts = player:GetBoneHearts(), max_hearts = player:GetMaxHearts(),
            bombs = player:GetNumBombs(), x = player.Position.X, y = player.Position.Y,
            movement_options = movementOptions,
        }
    end)
    if ok then return view end
    return { ordinal = ordinal, present = true, read_error = tostring(view) }
end

-- 当前全部 Player 的只读快照；玩家数来自 Game:GetNumPlayers。
local function allPlayers()
    local count = playerCount()
    local list = {}
    for i = 0, count - 1 do
        list[#list + 1] = playerView(Isaac.GetPlayer(i), i)
    end
    return count, list
end

local function snapshot()
    local count, players = allPlayers()
    return { player_count = count, players = players, room = roomIdentity() }
end

-- 仅记录 A/B 生命身份，供激光事件关联；不把未绑定角色写成已绑定。
local function lifeView(ordinal)
    local player = Isaac.GetPlayer(ordinal)
    if player == nil then return { ordinal = ordinal, present = false } end
    local ok, view = pcall(function()
        -- 玩家槽与实体编号分开；回退到 A 时拒绝，不把新 B 的 Entity.Index 当缺席。
        local playerIndex = player:GetPlayerIndex()
        if playerIndex ~= ordinal then
            return { ordinal = ordinal, present = false, resolved_ordinal = playerIndex,
                entity_index = player.Index }
        end
        return { ordinal = ordinal, present = true, exists = player:Exists(),
            player_index = playerIndex, entity_index = player.Index,
            dead = player:IsDead(), ghost = player:IsCoopGhost(),
            player_type = player:GetPlayerType(), controller = player.ControllerIndex,
            animation_finished = player:IsExtraAnimationFinished() }
    end)
    if ok then return view end
    return { ordinal = ordinal, present = true, read_error = tostring(view) }
end

-- 动作表规范化：允许字符串项，也允许 { name = "...", timeout_ms = N }；无效项在首次渲染时报错。
local function normalizeActions()
    local raw = config.actions
    if raw == nil then raw = DEFAULT_ACTIONS end
    if type(raw) ~= "table" then return nil, "actions must be a table" end
    if #raw == 0 and next(raw) ~= nil then
        return nil, "actions must be an array, not a string-keyed map"
    end
    local result = {}
    for index = 1, #raw do
        local item = raw[index]
        local name, timeout
        if type(item) == "string" then
            name = item
        elseif type(item) == "table" then
            name, timeout = item.name, item.timeout_ms
        else
            return nil, "action item " .. index .. " must be string or table"
        end
        if type(name) ~= "string" or not ACTION_NAMES[name] then
            return nil, "unknown action at " .. index .. ": " .. tostring(name)
        end
        if timeout ~= nil and (type(timeout) ~= "number" or timeout <= 0) then
            return nil, "invalid timeout at " .. index
        end
        result[#result + 1] = { name = name, timeout_ms = timeout or ACTION_TIMEOUTS[name] }
    end
    return result
end

local configuredActions, actionsError = normalizeActions()
if actionsError then
    configError = actionsError
else
    actions = configuredActions
end
if menuHostEnabled and tonumber(config.host_port) == nil then
    configError = configError or "host_port is required for the first-round menu host"
end

-- 动作终态只写一次；rejected/timeout 立即停止后续动作，不自动重试。
local function finishAction(phase, detail)
    local current = pending
    if current == nil then return end
    pending = nil
    local data = {
        phase = phase,
        action = current.name,
        action_id = current.action_id,
        request_id = current.request_id,
        step = current.step,
        fixture_error = phase ~= "arrived",
        elapsed_frames = gameFrame() - current.started_frame,
        elapsed_render = render - current.started_render,
    }
    if detail then
        for key, value in pairs(detail) do data[key] = value end
    end
    emit("action", data)
    if phase ~= "arrived" then
        failed = true
        skipRemaining, skipEnding = 0, false
    end
end

-- 普通 A：完整 Player、未死亡、不是 co-op ghost。
local function readHealthyA()
    local player = Isaac.GetPlayer(0)
    if player == nil then return nil, "missing" end
    local ok, reason = pcall(function()
        if not player:Exists() then error("not_exists") end
        if player.Type ~= EntityType.ENTITY_PLAYER or player.Variant ~= 0 then error("not_full_player") end
        if player:GetPlayerIndex() ~= 0 then error("wrong_player_index") end
        if player:IsDead() then error("dead") end
        if player:IsCoopGhost() then error("ghost") end
    end)
    if ok then return player end
    return nil, tostring(reason)
end

-- 完整普通 B（未死亡、未 ghost）；用于 kill_b 前置确认。
local function readAliveB()
    local player = Isaac.GetPlayer(1)
    if player == nil then return nil, "missing" end
    local ok, reason = pcall(function()
        if player:GetPlayerIndex() ~= 1 then error("wrong_player_index") end
        if not player:Exists() then error("not_exists") end
        if player.Type ~= EntityType.ENTITY_PLAYER or player.Variant ~= 0 then error("not_full_player") end
        if player:IsDead() then error("dead") end
        if player:IsCoopGhost() then error("ghost") end
        if player:GetPlayerType() ~= expectedBPlayerType then error("wrong_player_type") end
    end)
    if ok then return player end
    return nil, tostring(reason)
end

-- 已确认的 B ghost；revive_b 只对这种对象调用一次 ReviveCoopGhost。
local function readGhostB()
    local player = Isaac.GetPlayer(1)
    if player == nil then return nil, "missing" end
    local ok, reason = pcall(function()
        if player:GetPlayerIndex() ~= 1 then error("wrong_player_index") end
        -- ghost 可能已被原生标记为死亡；Exists 不作为 ghost 资格的唯一依据。
        if player.Type ~= EntityType.ENTITY_PLAYER or player.Variant ~= 0 then error("not_full_player") end
        if not player:IsCoopGhost() then error("not_ghost") end
        if player:GetPlayerType() ~= expectedBPlayerType then error("wrong_player_type") end
    end)
    if ok then return player end
    return nil, tostring(reason)
end

-- 复活后的实际健康与身份重读；true 只表示原生调用返回，不代表已稳定重绑。
local function readHealthyB()
    local player = Isaac.GetPlayer(1)
    if player == nil then return nil, "missing" end
    local ok, reason = pcall(function()
        if player:GetPlayerIndex() ~= 1 then error("wrong_player_index") end
        -- 复活后的短暂帧可能仍带 dead/exists 过渡；以生命字段与身份判断，不只看 Exists。
        if player.Type ~= EntityType.ENTITY_PLAYER or player.Variant ~= 0 then error("not_full_player") end
        if player:IsDead() then error("dead") end
        if player:IsCoopGhost() then error("ghost") end
        if player:GetPlayerType() ~= expectedBPlayerType then error("wrong_player_type") end
    end)
    if ok then return player end
    return nil, tostring(reason)
end

-- ===== 动作处理器 =====

local function requestSkip(current)
    current.phase = "waiting_skip"
    skipRemaining, skipCount, skipEnding = 30, 0, false
end

-- 选择已开启、未上锁的普通相邻门；记录来源/目标，目标层/房变化由轮询确认。
local function requestRoom(current)
    local source = roomIdentity()
    if source.error then
        finishAction("rejected", { reason = "room_unreadable", error = source.error })
        return
    end
    local game = Game()
    local room, level = game:GetRoom(), game:GetLevel()
    for slot = 0, 7 do
        local door = room:GetDoor(slot)
        if door and door:IsOpen() and not door:IsLocked() and door.TargetRoomIndex >= 0
            and door.TargetRoomIndex ~= source.index then
            local target = level:GetRoomByIdx(door.TargetRoomIndex, -1)
            if target and target.Data and target.Data.Type == RoomType.ROOM_DEFAULT then
                current.data = { source = source, target = door.TargetRoomIndex,
                    direction = door.Direction, slot = slot }
                current.phase = "waiting_room"
                -- 全参数调用；不把调用返回当作过渡成功，也不写门或 Player 状态。
                local ok, err = pcall(function()
                    game:StartRoomTransition(current.data.target, current.data.direction, 0, nil, -1)
                end)
                if not ok then
                    finishAction("rejected", { reason = "room_call_error", error = tostring(err),
                        source = source, target = current.data.target,
                        direction = current.data.direction, slot = current.data.slot })
                end
                return
            end
        end
    end
    finishAction("rejected", { reason = "no_open_adjacent_normal_room", source = source })
end

-- kill_b：只对已确认的完整普通 B 调用一次 Kill()，等待原生 dead/ghost。
local function requestKillB(current)
    local count = playerCount()
    if count ~= 2 then
        finishAction("rejected", { reason = "player_count", player_count = count })
        return
    end
    local a, aReason = readHealthyA()
    if not a then
        finishAction("rejected", { reason = "a_not_healthy", detail = aReason })
        return
    end
    local b, bReason = readAliveB()
    if not b then
        local reason = "b_not_alive"
        if bReason == "wrong_player_type" then
            reason = "b_wrong_role"
        elseif bReason == "missing" or bReason == "not_exists" or bReason == "not_full_player" then
            reason = "b_missing"
        elseif bReason == "wrong_index" then
            reason = "b_identity_changed"
        end
        finishAction("rejected", { reason = reason, detail = bReason })
        return
    end
    local aHash, bHash = GetPtrHash(a), GetPtrHash(b)
    if aHash == bHash then
        finishAction("rejected", { reason = "a_b_same_object" })
        return
    end
    current.data = { dead_seen = false, dead_frame = nil, ghost_frame = nil, call_return = nil }
    current.phase = "waiting_ghost"
    -- 先记录 Kill 请求前的 B 视图，再调用原生 Kill；不把调用后的状态标成 before。
    emit("b-kill", { phase = "request", action_id = current.action_id,
        before = playerView(b, 1), player_count = count })
    local ok, result = pcall(function() return b:Kill() end)
    if not ok then
        finishAction("rejected", { reason = "kill_call_error", error = tostring(result) })
        return
    end
    current.data.call_return = tostring(result)
end

-- revive_b：只对已确认的 B ghost 调用一次 ReviveCoopGhost；健康、动画与 DLL 重绑分开观察。
local function requestReviveB(current)
    local count = playerCount()
    if count ~= 2 then
        finishAction("rejected", { reason = "player_count", player_count = count })
        return
    end
    local a, aReason = readHealthyA()
    if not a then
        finishAction("rejected", { reason = "a_not_healthy", detail = aReason })
        return
    end
    local b, bReason = readGhostB()
    if not b then
        local reason = "b_not_ghost"
        if bReason == "wrong_player_type" then
            reason = "b_wrong_role"
        elseif bReason == "missing" or bReason == "not_exists" or bReason == "not_full_player" then
            reason = "b_missing"
        elseif bReason == "wrong_index" then
            reason = "b_identity_changed"
        end
        finishAction("rejected", { reason = reason, detail = bReason })
        return
    end
    current.data = { revive_return = nil, revive_true = false }
    current.phase = "waiting_health"
    local ok, result = pcall(function() return b:ReviveCoopGhost() end)
    if not ok then
        finishAction("rejected", { reason = "revive_call_error", error = tostring(result) })
        return
    end
    current.data.revive_return = tostring(result)
    current.data.revive_true = (result == true)
    emit("b-revive", { phase = "return", action_id = current.action_id,
        revive_return = current.data.revive_return, revive_true = current.data.revive_true,
        before = playerView(b, 1), player_count = count })
end

-- stage：通过官方 Lua 方法调用；不写 StageTransition 结构或原生 FSM 字段。
local function requestStage(current)
    local before = current.before
    if before == nil or before.player_count < 1 then
        finishAction("rejected", { reason = "no_run" })
        return
    end
    if before.room and before.room.error then
        finishAction("rejected", { reason = "room_unreadable", error = before.room.error })
        return
    end
    local paused = false
    pcall(function() paused = Game():IsPaused() end)
    if paused then
        finishAction("rejected", { reason = "paused" })
        return
    end
    current.data = { before = before, new_level_callbacks = 0, new_room_callbacks = 0 }
    current.phase = "waiting_stage"
    local ok, result = pcall(function() Game():StartStageTransition(false, 0, nil) end)
    if not ok then
        finishAction("rejected", { reason = "stage_call_error", error = tostring(result) })
        return
    end
    emit("stage", { phase = "call_return", action_id = current.action_id,
        before = before, call_return = tostring(result),
        -- Lua 只能证明调用正常返回；是否真正进入 FSM 由 DLL stage_transition 窗口判定。
        entered = "unverified" })
end

local ACTION_HANDLERS = {
    skip = requestSkip,
    room = requestRoom,
    kill_b = requestKillB,
    revive_b = requestReviveB,
    stage = requestStage,
}

-- 一个新动作：先记录 request，再调用对应处理器；处理器失败即夹具错误，不重试。
local function startAction(requestId)
    local spec = actions[nextIndex]
    if spec == nil then
        emit("action", { phase = "rejected", action_id = requestId, request_id = requestId,
            reason = "sequence_exhausted", total = #actions })
        return
    end
    local startedFrame = gameFrame()
    local before = snapshot()
    pending = {
        name = spec.name,
        step = nextIndex,
        action_id = requestId,
        request_id = requestId,
        timeout_ms = spec.timeout_ms,
        started_frame = startedFrame,
        started_render = render,
        deadline_frame = startedFrame + framesFor(spec.timeout_ms),
        phase = "requested",
        before = before,
        data = {},
    }
    nextIndex = nextIndex + 1
    emit("action", { phase = "request", action = spec.name, action_id = pending.action_id,
        request_id = requestId, step = pending.step, timeout_ms = spec.timeout_ms,
        before = before })
    local ok, err = pcall(ACTION_HANDLERS[spec.name], pending)
    if not ok then
        finishAction("rejected", { reason = "handler_error", error = tostring(err) })
    end
end

-- F8 请求：完成前重复按下只记录 rejected，不推进动作序号，也不建立第二个 pending。
local function onF8()
    requestSerial = requestSerial + 1
    local requestId = requestSerial
    if failed then
        emit("action", { phase = "rejected", action_id = requestId, request_id = requestId,
            reason = "fixture_failed" })
        return
    end
    if pending ~= nil then
        emit("action", { phase = "rejected", action_id = requestId, request_id = requestId,
            reason = "pending", pending_action = pending.name,
            pending_action_id = pending.action_id })
        return
    end
    if playerCount() < 1 then
        emit("action", { phase = "rejected", action_id = requestId, request_id = requestId,
            reason = "no_run" })
        return
    end
    startAction(requestId)
end

-- ===== 动作进行中的只读轮询 =====

-- 房间是否已经从来源房间加载到目标；不等待 DLL 重绑。
local function pollRoom(current)
    local identity = roomIdentity()
    current.data.last = identity
    if identity.error then return end
    local source = current.data.source
    if identity.index ~= source.index or identity.stage ~= source.stage
        or identity.stage_type ~= source.stage_type then
        finishAction("arrived", {
            source = source,
            target = current.data.target,
            actual = identity,
            target_matched = identity.index == current.data.target,
            after = snapshot(),
            dll_rebind = "pending",
        })
    end
end

-- 死亡窗口：先记录 dead（若可观察），再记录 ghost；不补造未观察到的 dead。
local function pollKill(current)
    local count = playerCount()
    if count < 2 then
        finishAction("rejected", { reason = "b_missing", player_count = count, after = snapshot() })
        return
    end
    local b = Isaac.GetPlayer(1)
    if b == nil then
        finishAction("rejected", { reason = "b_missing", player_count = count })
        return
    end
    local view = playerView(b, 1)
    current.data.last = view
    if view.read_error then return end
    -- 死亡窗口内不用 Exists 单次结果判定移除：先看人数/对象身份；移除会在人数或读取上体现。
    if view.type ~= EntityType.ENTITY_PLAYER or view.variant ~= 0 or view.player_index ~= 1 then
        finishAction("rejected", { reason = "b_identity_changed", after = view })
        return
    end
    if view.player_type ~= expectedBPlayerType then
        finishAction("rejected", { reason = "b_wrong_role", after = view })
        return
    end
    local frame = gameFrame()
    if not current.data.dead_seen and view.dead then
        current.data.dead_seen = true
        current.data.dead_frame = frame
        emit("b-kill", { phase = "dead", action_id = current.action_id,
            view = view, player_count = count,
            frames_since_call = frame - current.started_frame })
    end
    if view.ghost then
        current.data.ghost_frame = frame
        emit("b-kill", { phase = "ghost", action_id = current.action_id,
            view = view, player_count = count,
            frames_since_call = frame - current.started_frame })
        finishAction("arrived", {
            dead_seen = current.data.dead_seen,
            dead_frame_delta = current.data.dead_frame and (current.data.dead_frame - current.started_frame) or nil,
            ghost_frame_delta = frame - current.started_frame,
            kill_return = current.data.call_return,
            kill_calls = 1,
            after = snapshot(),
        })
        return
    end
    -- 死亡后没有进入 ghost 却恢复健康：属于意外自动复活，不当作本轮 ghost 用例成功。
    if current.data.dead_seen and not view.dead and not view.ghost then
        finishAction("rejected", { reason = "auto_revived", after = view })
        return
    end
    -- Kill 后长时间既无 dead 也无 ghost：单列原因，不把“什么都没发生”拖成普通超时。
    if not current.data.dead_seen and not view.dead and not view.ghost
        and frame - current.started_frame > 150 then
        finishAction("rejected", { reason = "no_native_death", after = view })
    end
end

-- 复活窗口：只接受实际健康与身份重读成功；ghost/dead 仍属于等待状态。
local function pollRevive(current)
    local count = playerCount()
    if count < 2 then
        finishAction("rejected", { reason = "b_missing", player_count = count })
        return
    end
    local b, reason = readHealthyB()
    if b ~= nil then
        local view = playerView(b, 1)
        emit("b-revive", { phase = "healthy", action_id = current.action_id,
            view = view, player_count = count,
            frames_since_call = gameFrame() - current.started_frame })
        animationWatch = {
            action_id = current.action_id,
            started_frame = gameFrame(),
            deadline_frame = gameFrame() + framesFor(animationTimeoutMs),
        }
        finishAction("arrived", {
            revive_return = current.data.revive_return,
            revive_true = current.data.revive_true,
            healthy = true,
            animation_finished = view.animation_finished,
            dll_rebind = "pending",
            after = snapshot(),
        })
        return
    end
    current.data.last = { reason = reason, view = playerView(Isaac.GetPlayer(1), 1) }
    if reason == "missing" or reason == "not_exists" or reason == "not_full_player" then
        finishAction("rejected", { reason = "b_missing", detail = reason, after = current.data.last })
    elseif reason == "wrong_index" then
        finishAction("rejected", { reason = "b_identity_changed", detail = reason,
            after = current.data.last })
    elseif reason == "wrong_player_type" then
        finishAction("rejected", { reason = "b_wrong_role", detail = reason,
            after = current.data.last })
    end
end

-- 换层窗口：等 actual stage/room 身份变化；稳定完成与重绑仍由 DLL 证据判断。
local function pollStage(current)
    local identity = roomIdentity()
    current.data.last = identity
    if identity.error then return end
    local before = current.data.before.room
    if identity.index ~= before.index or identity.stage ~= before.stage
        or identity.stage_type ~= before.stage_type then
        finishAction("arrived", {
            before = current.data.before,
            actual = identity,
            after = snapshot(),
            changed = true,
            new_level_callbacks = current.data.new_level_callbacks,
            new_room_callbacks = current.data.new_room_callbacks,
            dll_stable = "pending",
        })
    end
end

local function pollPending()
    local current = pending
    if current == nil or current.name == "skip" then return end
    if current.phase == "waiting_room" then
        pollRoom(current)
    elseif current.phase == "waiting_ghost" then
        pollKill(current)
    elseif current.phase == "waiting_health" then
        pollRevive(current)
    elseif current.phase == "waiting_stage" then
        pollStage(current)
    end
end

-- 超时只产生一个 action timeout 终态；失败后不继续后续动作。
local function checkTimeout()
    local current = pending
    if current == nil then return end
    if current.name == "skip" then
        -- skip 由 MC_PRE_UPDATE 次数推进；渲染层只做兜底，不把暂停当成超时。
        if render - current.started_render > framesFor(current.timeout_ms) * 2 then
            finishAction("timeout", { reason = "skip_render_deadline" })
        end
        return
    end
    -- 主判据是游戏帧；渲染帧兜底覆盖 Game:GetFrameCount() 因暂停/加载不前进的情况。
    local renderDeadline = framesFor(current.timeout_ms) * 2
    if gameFrame() >= current.deadline_frame or render - current.started_render > renderDeadline then
        local detail = {
            reason = gameFrame() >= current.deadline_frame and "deadline" or "render_deadline",
            action_phase = current.phase,
            timeout_ms = current.timeout_ms,
            last = current.data.last,
        }
        -- 超时也保留动作已经取得的目标/来源或生命观测，避免只剩一个空终态。
        if current.before ~= nil then detail.before = current.before end
        for _, key in ipairs({ "source", "target", "dead_seen", "revive_return" }) do
            if current.data[key] ~= nil then detail[key] = current.data[key] end
        end
        finishAction("timeout", detail)
    end
end

-- 复活动画观察：独立于动作终态，只报告实际 IsExtraAnimationFinished 结果。
local function pollAnimation()
    local watch = animationWatch
    if watch == nil then return end
    local b, reason = readHealthyB()
    if b == nil then
        emit("b-revive", { phase = "animation_lost", action_id = watch.action_id,
            reason = reason, frames_since_healthy = gameFrame() - watch.started_frame })
        animationWatch = nil
        return
    end
    local view = playerView(b, 1)
    if view.read_error then return end
    if view.ghost or view.dead then
        emit("b-revive", { phase = "reghost", action_id = watch.action_id,
            view = view, frames_since_healthy = gameFrame() - watch.started_frame })
        animationWatch = nil
        return
    end
    if view.animation_finished then
        emit("b-revive", { phase = "animation_finished", action_id = watch.action_id,
            view = view, frames_since_healthy = gameFrame() - watch.started_frame })
        animationWatch = nil
        return
    end
    if gameFrame() >= watch.deadline_frame then
        emit("b-revive", { phase = "animation_timeout", action_id = watch.action_id,
            view = view, frames_since_healthy = gameFrame() - watch.started_frame })
        animationWatch = nil
    end
end

-- ===== 激光出生与归属（只读） =====

-- 实体指针身份；不能用易复用地址单独去重，因此同时记录 InitSeed 与所在房间 epoch。
local function ptrHash(entity)
    if entity == nil then return nil end
    local ok, value = pcall(GetPtrHash, entity)
    if ok then return value end
    return nil
end

-- 把 spawner/parent 映射到当前 A/B；无法确认时返回 nil，调用方记 UNKNOWN。
local function roleFor(entity)
    local hash = ptrHash(entity)
    if hash == nil then return nil end
    if playerCount() == 0 then return nil end
    local a = Isaac.GetPlayer(0)
    if a ~= nil and a:GetPlayerIndex() ~= 0 then a = nil end
    local aHash = ptrHash(a)
    -- GetPlayer 的回退不能冒充 B；只认当前 PlayerManager ordinal，不认实体编号。
    local b = nil
    if playerCount() > 1 then
        b = Isaac.GetPlayer(1)
        if b ~= nil and b:GetPlayerIndex() ~= 1 then b = nil end
    end
    local bHash = ptrHash(b)
    if aHash ~= nil and hash == aHash then return "A" end
    if bHash ~= nil and hash == bHash then return "B" end
    return nil
end

-- 只读实体引用视图；不强行把未知 spawner 归给 B。
local function entityRefView(entity)
    if entity == nil then return nil end
    local ok, view = pcall(function()
        return { type = entity.Type, variant = entity.Variant, index = entity.Index,
            init_seed = entity.InitSeed, ptr_hash = GetPtrHash(entity),
            role = roleFor(entity) }
    end)
    if ok then return view end
    return { read_error = tostring(view) }
end

-- 激光生命周期视图；attribution 只来自可确认的 spawner/parent 链。
local function laserView(laser)
    local ok, view = pcall(function()
        -- 引用字段单独读取：spawner/parent 失效时仍保留激光自身的身份与类型。
        local spawner, parent = nil, nil
        pcall(function() spawner = laser.SpawnerEntity end)
        pcall(function() parent = laser.Parent end)
        local spawnerRole = roleFor(spawner)
        local parentRole = roleFor(parent)
        local attribution = "UNKNOWN"
        if spawnerRole == "B" or parentRole == "B" then
            attribution = "B"
        elseif spawnerRole == "A" or parentRole == "A" then
            attribution = "A"
        end
        local timeout = nil
        pcall(function() timeout = laser.Timeout end)
        return {
            type = laser.Type, variant = laser.Variant, sub_type = laser.SubType,
            index = laser.Index, init_seed = laser.InitSeed, ptr_hash = GetPtrHash(laser),
            timeout = timeout,
            -- spawner 类型/变种只作补充；身份仍只由 SpawnerEntity/Parent 的指针确认。
            spawner_type = laser.SpawnerType, spawner_variant = laser.SpawnerVariant,
            spawner = entityRefView(spawner),
            parent = entityRefView(parent),
            spawner_role = spawnerRole,
            parent_role = parentRole,
            attribution = attribution,
        }
    end)
    if ok then return view end
    return { read_error = tostring(view) }
end

-- 房间切换后，把已有激光单列为 baseline；不作为“出生”事件。
local function recordLaserBaseline()
    local ok, entities = pcall(Isaac.GetRoomEntities)
    if not ok or type(entities) ~= "table" then
        -- 读取失败要留显式记录，不能静默当成“本房间没有激光”。
        emit("laser-baseline", { epoch = laserEpoch, room = roomIdentity(),
            players = { lifeView(0), lifeView(1) },
            count = 0, lasers = {}, error = tostring(entities) })
        return true
    end
    local list = {}
    for index = 1, #entities do
        local entity = entities[index]
        local okType, entityType = pcall(function() return entity.Type end)
        if okType and entityType == EntityType.ENTITY_LASER then
            local view = laserView(entity)
            local key = tostring(view.ptr_hash) .. ":" .. tostring(view.init_seed)
            view.seen_init = laserInitIds[key] ~= nil
            list[#list + 1] = view
        end
    end
    emit("laser-baseline", { epoch = laserEpoch, room = roomIdentity(),
        players = { lifeView(0), lifeView(1) }, count = #list, lasers = list })
    return true
end

-- MC_POST_LASER_INIT：只在 run 内、未失败时记录；达到上限后明确报告一次溢出。
local function recordLaserSpawn(laser)
    if failed or configError ~= nil or playerCount() < 1 then return end
    if laserSpawnEvents >= laserEventCap then
        if not laserOverflowReported then
            laserOverflowReported = true
            emit("laser-overflow", { cap = laserEventCap, epoch = laserEpoch })
        end
        return
    end
    laserSerial = laserSerial + 1
    laserSpawnEvents = laserSpawnEvents + 1
    local view = laserView(laser)
    local key = tostring(view.ptr_hash) .. ":" .. tostring(view.init_seed)
    local laserId = tostring(laserEpoch) .. ":" .. tostring(laserSerial)
    laserInitIds[key] = laserId
    emit("laser-spawn", { laser_id = laserId, epoch = laserEpoch, serial = laserSerial,
        room = roomIdentity(), players = { lifeView(0), lifeView(1) }, laser = view })
end

-- 换房/新层/新 run 重新编号激光生命周期，清除旧房间的 init 去重表。
local function resetLaserRoom()
    laserEpoch = laserEpoch + 1
    laserInitIds = {}
    laserBaselineDone = false
end

-- ===== 回调注册 =====

-- 首轮主菜单自动 Host；hostRequested 在进程内只成功设置一次，第 4 轮仍由控制口 Host。
fixture:AddCallback(ModCallbacks.MC_MAIN_MENU_RENDER, function()
    if hostRequested or not menuHostEnabled or configError ~= nil then return end
    local response = tostring(Isaac.ExecuteCommand("cs status"))
    if not response:find("\nstate=Idle\n", 1, true) then return end
    hostRequested = true
    local result = Isaac.ExecuteCommand("cs host " .. tostring(config.host_port))
    emit("menu-host", { response = tostring(result) })
end)

-- skip 动作：30 次 MC_PRE_UPDATE 返回 true，然后在下一次允许更新前记录 arrived。
fixture:AddCallback(ModCallbacks.MC_PRE_UPDATE, function()
    if failed or pending == nil or pending.name ~= "skip" then return end
    if skipRemaining > 0 then
        skipRemaining = skipRemaining - 1
        skipCount = skipCount + 1
        if skipRemaining == 0 then skipEnding = true end
        return true
    end
    if skipEnding then
        skipEnding = false
        finishAction("arrived", { skipped = skipCount, skipped_updates = skipCount, after = snapshot() })
    end
end)

-- 主渲染循环：F8 -> 超时 -> 轮询 -> baseline -> 动画 -> 周期观测。
local function renderFixture()
    render = render + 1
    if configError ~= nil then
        local errorText = configError
        configError = nil
        failFixture(errorText)
        return
    end
    if failed then return end
    if Input.IsButtonTriggered(Keyboard.KEY_F8, 0) then onF8() end
    if failed then return end
    checkTimeout()
    if failed then return end
    if pending ~= nil then pollPending() end
    if failed then return end
    if playerCount() > 0 and not laserBaselineDone then
        if recordLaserBaseline() then laserBaselineDone = true end
    end
    pollAnimation()
    if playerCount() > 0 then
        local count, players = allPlayers()
        local observation = { player_count = count, players = players, room = roomIdentity() }
        local encoded = json.encode(observation)
        if encoded ~= lastObservation or render % observationHeartbeat == 0 then
            lastObservation = encoded
            emit("observation", observation)
        end
    end
end

fixture:AddCallback(ModCallbacks.MC_POST_RENDER, function()
    local ok, err = pcall(renderFixture)
    if not ok then failFixture("render: " .. tostring(err)) end
end)

-- 激光只在实体初始化回调登记新出生；baseline 另在每次建房的首次渲染扫描。
fixture:AddCallback(ModCallbacks.MC_POST_LASER_INIT, function(_, laser)
    if failed then return end
    local ok, err = pcall(recordLaserSpawn, laser)
    if not ok then failFixture("laser: " .. tostring(err)) end
end)

-- 新层/新房间回调：记录动作期间的加载边界，并让激光生命周期重新编号。
-- 加载边界：所有新层/新房间都记录；若正有 stage/room 动作，再累计到该动作的窗口计数。
local function levelCallback(phase, actionField)
    if failed or configError ~= nil then return end
    local actionName, actionId = nil, nil
    if pending ~= nil and (pending.name == "stage" or pending.name == "room") then
        pending.data[actionField] = (pending.data[actionField] or 0) + 1
        actionName, actionId = pending.name, pending.action_id
    end
    emit("level-callback", { phase = phase, action = actionName, action_id = actionId,
        room = roomIdentity() })
    resetLaserRoom()
end
fixture:AddCallback(ModCallbacks.MC_POST_NEW_LEVEL, function()
    levelCallback("new_level", "new_level_callbacks")
end)
fixture:AddCallback(ModCallbacks.MC_POST_NEW_ROOM, function()
    levelCallback("new_room", "new_room_callbacks")
end)

-- 新 run 也重新建立激光 baseline；动作序列跨四轮保留，不在这里重置。
fixture:AddCallback(ModCallbacks.MC_POST_GAME_STARTED, function(_, continued)
    resetLaserRoom()
    animationWatch = nil
    -- 实际恢复人数与保存请求人数分开记录；此回调不代表 DLL 稳定重绑已完成。
    emit("run-started", { continued = continued == true, snapshot = snapshot(), dll_stable = "pending" })
end)

-- 记录原生退出的保存请求，不把请求直接等同于磁盘保存成功。
fixture:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT, function(_, shouldSave)
    emit("run-exit", { should_save = shouldSave == true, snapshot = snapshot(), save_result = "UNKNOWN" })
end)
