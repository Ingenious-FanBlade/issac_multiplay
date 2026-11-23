-- 独立夹具走公开 SDK。只有配置选择负向握手样本时，才跳过登记或提交不同声明。
local config = include("fixture_config")
local CS = IsaacCS or include("isaaccs_sdk")
local fixture = RegisterMod("Isaac CS Mod Adaptation E2E", 1)
local json = require("json")
local serial = 0

-- 同一轮用 token 分组，用 node、角色、帧和本地递增序号对应两端观察。
local function emit(event, row)
    row = row or {}
    serial = serial + 1
    row.event, row.serial = event, serial
    if event ~= "loaded" then row.frame = Isaac.GetFrameCount() end
    row.node, row.token, row.client = config.node, config.token, CS.IsClient()
    Isaac.DebugString("[isaac-cs-mod-fixture] " .. json.encode(row))
end

emit("loaded", {manifest = config.manifest, native_sdk = CS._isaaccs.native})
if config.manifest == "undeclared" then return end
CS.Declare(fixture, {version = "1.0.0", api = 1})
if config.manifest == "different" then
    -- 保持目录、metadata 和自报版本相同，只改变声明文本。
    CS.State(fixture, "different_manifest", {type = "bool"})
end

local native = include("native_cases")(fixture, CS, config, emit)
local key = Keyboard.KEY_K
CS.Keys(fixture, {key})

local filter = {type = EntityType.ENTITY_PICKUP, variant = PickupVariant.PICKUP_COIN, subtype = CoinSubType.COIN_DOUBLEPACK}
local entityDataKey = "isaacCsFixtureData"
local public, private = {}, {}
local entity, retainedReference, entitySerial = nil, nil, 0
local run, running, clientScene = 0, false, false
local pendingAction, readFault = nil, nil
local releaseReferenceAfter
local reads = {world = 0, low = 0, mark = 0}
local inputSignatures, pendingIntents = {}, {}
local rateProbe, driftFrame = nil, nil

-- 用实际 GetData 和显式 setter 各承载一条同实体的记录。
local function entityData(ent)
    if not ent or ent.Type ~= filter.type or ent.Variant ~= filter.variant or ent.SubType ~= filter.subtype then return end
    return ent:GetData()[entityDataKey]
end

-- 客机默认写回应复用表和子表，保留双方各自的未声明字段。
local localWorld, localNested
local function emptyWorld()
    localNested = {local_only = "nested " .. config.node}
    localWorld = {local_only = "world " .. config.node, nested = localNested}
    fixture.world = localWorld
end
emptyWorld()

local boundedText = {string = {max = 32}}
local leaf = {struct = {value = "int"}}
local worldType = {struct = {
    epoch = "int", score = "int", enabled = "bool", weight = "float", precise = "number",
    text = boundedText, optional = boundedText, point = "vector", tint = "color", ink = "kcolor",
    target = "entity", owner = "player", nested = {struct = {
        rows = {list = {max = 3, of = {struct = {name = boundedText,
            counts = {map = {key = "int", max = 4, of = "int"}}}}}},
        byName = {map = {key = "string", keymax = 16, max = 4,
            of = {list = {max = 4, of = "number"}}}},
        one = leaf, two = leaf, cycle = {struct = {value = "int", next = leaf}},
    }},
}}
CS.State(fixture, "world", {type = worldType, rate = "world", get = function()
    if not CS.IsClient() then
        reads.world = reads.world + 1
        if readFault == "read-error" then error("fixture read failure") end
        if readFault == "wrong-type" then return function() end end
        if readFault == "slow-read" then
            readFault = nil
            -- 只拖慢一次读取，给计时留出可见差异，并限制循环次数。
            local started = Isaac.GetTime()
            for _ = 1, 20000 do
                if Isaac.GetTime() - started >= 12 then break end
            end
            emit("slow-read", {elapsed_ms = Isaac.GetTime() - started, world_read = reads.world})
        end
        if readFault == "too-many" or readFault == "long-string" then
            local sample = setmetatable({}, {__index = fixture.world})
            if readFault == "long-string" then
                sample.text = string.rep("x", 33)
            else
                sample.nested = {rows = {{}, {}, {}, {}}}
            end
            return sample
        end
    end
    return fixture.world
end, reset = function()
    emptyWorld()
    emit("reset", {key = "world", value_cleared = fixture.world.epoch == nil})
end})
CS.State(fixture, "low", {type = "int", rate = "low", get = function()
    if CS.IsClient() then return fixture.low end
    reads.low = reads.low + 1
    return Game():GetFrameCount()
end, reset = function() fixture.low = nil end})
CS.State(fixture, "marked", {type = "int", rate = "mark", get = function()
    if not CS.IsClient() then reads.mark = reads.mark + 1 end
    return fixture.marked
end, reset = function()
    fixture.marked = nil
    emit("reset", {key = "marked", value_cleared = fixture.marked == nil})
end})
CS.State(fixture, "large", {type = {list = {max = 257, of = "bool"}}, rate = "world"})
CS.State(fixture, "fragile", {type = "int", get = function() return fixture.fragile end, set = function(value)
    if value == -1 then error("fixture set failure") end
    fixture.fragile = value
end, reset = function() fixture.fragile = nil end})

local publicType = {struct = {
    ordinal = "int", score = "int", held = "bool", edges = "int",
    moving = "bool", move_edge = "bool", move_value = "float", move_edges = "int",
}}
CS.State(fixture, "public_player", {scope = "player", type = publicType,
    get = function(player) return public[CS.GetParticipant(player)] end,
    set = function(player, value) public[CS.GetParticipant(player)] = value end,
    reset = function() public = {} end})
CS.State(fixture, "private_player", {scope = "player", private = true, type = boundedText,
    get = function(player) return private[CS.GetParticipant(player)] end,
    set = function(player, value) private[CS.GetParticipant(player)] = value end,
    reset = function() private = {} end})
CS.EntityData(fixture, filter, {[entityDataKey] = {struct = {id = "int", label = boundedText}}})
CS.EntityState(fixture, "entity_value", {filter = filter, type = {struct = {value = "int", owner = "player"}},
    get = function(ent) return ent:GetData().isaacCsFixtureState end,
    set = function(ent, value) ent:GetData().isaacCsFixtureState = value end})

-- 成功时修改发起者的计分。失败用负数复现；较长参数留给线上 512 B 限制拒绝。
CS.Intent(fixture, "score", {args = {struct = {delta = "int", note = {string = {max = 600}}, target = "entity"}},
    handler = function(player, args)
        local ordinal = CS.GetParticipant(player)
        local sample = ordinal and public[ordinal]
        if not sample or not args or args.delta < 0 then
            emit("intent-handled", {ok = false, ordinal = ordinal})
            return false, "negative-or-missing-player"
        end
        local target = entityData(args.target)
        sample.score = sample.score + args.delta
        fixture.world.score = fixture.world.score + args.delta
        emit("intent-handled", {ok = true, ordinal = ordinal, score = sample.score, target = target and target.id})
        return true, {ordinal = ordinal, score = sample.score, target = target and target.id}
    end})
CS.AddCallback(fixture, CS.CLIENT_INTENT_RESULT, function(name, seq, ok, value)
    local sent = pendingIntents[seq]
    pendingIntents[seq] = nil
    emit("intent-result", {name = name, seq = seq, ok = ok, result = value,
        sent_frame = sent and sent.frame, probe = sent and sent.probe})
end)

-- 实体通知记录已安装内容和本地实例生灭。实体已离开时只使用移除前的通知参数。
local function entityEvent(event, ent)
    local data = entityData(ent)
    if data then
        local value = ent:GetData().isaacCsFixtureState
        emit(event, {id = data.id, label = data.label, value = value and value.value, seed = ent.InitSeed})
    end
end
CS.AddCallback(fixture, CS.CLIENT_ENTITY_INSTALLED, function(ent) entityEvent("entity-installed", ent) end)
CS.AddCallback(fixture, CS.CLIENT_ENTITY_REMOVED, function(ent) entityEvent("entity-removed", ent) end)
CS.AddCallback(fixture, CS.CLIENT_STATE_CHANGED, function(name, target)
    -- low 按 cadence 快照取证，避免每次定期更新都刷日志。
    if name ~= "low" and name ~= "public_player" then
        emit("state-changed", {key = name, target_type = target and target.Type})
    end
end)
CS.AddCallback(fixture, CS.CLIENT_SCENE_STARTED, function()
    clientScene = true
    emit("scene-started", {epoch = fixture.world.epoch, low = fixture.low, marked = fixture.marked})
end)
CS.AddCallback(fixture, CS.CLIENT_SCENE_ENDED, function()
    clientScene, rateProbe, driftFrame = false, nil, nil
    emit("scene-ended", {epoch = fixture.world.epoch, score = fixture.world.score, marked = fixture.marked})
end)

-- 三种频率的主机来源、稀疏 map、共享和自引用 Lua 表都使用可预测的内容。
local function startRun()
    run, running = run + 1, true
    public, private, inputSignatures, pendingIntents = {}, {}, {}, {}
    entity, retainedReference, pendingAction, readFault, rateProbe = nil, nil, nil, nil, nil
    releaseReferenceAfter = nil
    reads = {world = 0, low = 0, mark = 0}
    emptyWorld()
    local w = fixture.world
    w.epoch, w.score, w.enabled = run, 0, true
    w.weight, w.precise = 2.25, 1 + 2^-40
    w.text, w.optional = "fixture-" .. run, "present"
    w.point, w.tint, w.ink = Vector(12.5, -4), Color(0.25, 0.5, 0.75, 1, 0.125, 0.25, 0.375),
        KColor(0.75, 0.5, 0.25, 1)
    local shared, cycle = {value = 42}, {value = 81}
    cycle.next = cycle
    w.nested.one, w.nested.two, w.nested.cycle = shared, shared, cycle
    w.nested.rows = {{name = "sparse", counts = {[12] = 13, [-7] = 23}}}
    w.nested.byName = {zulu = {5, 6}, alpha = {1.25, 2.5}}
    fixture.low, fixture.marked, fixture.fragile, fixture.large = nil, nil, 1, {true, false}
    emit("run-started", {epoch = run})
end
fixture:AddCallback(ModCallbacks.MC_POST_GAME_STARTED, startRun)
fixture:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT, function()
    native.reset()
    running, pendingAction = false, nil
    emit("run-ended", {epoch = run})
end)
fixture:AddCallback(ModCallbacks.MC_POST_NEW_ROOM, function()
    if not running then return end
    entity, retainedReference, releaseReferenceAfter = nil, nil, nil
    emit("room-entered", {index = Game():GetLevel():GetCurrentRoomIndex()})
end)
fixture:AddCallback(ModCallbacks.MC_POST_NEW_LEVEL, function()
    if not running then return end
    native.reset()
    emit("level-entered", {stage = Game():GetLevel():GetStage()})
end)

local actions = native.actions
function actions.spawn()
    if entity and entity:Exists() then error("请先 remove 现有夹具实体") end
    local room = Game():GetRoom()
    local position = room:FindFreePickupSpawnPosition(room:GetCenterPos(), 30, true)
    entity = Isaac.Spawn(filter.type, filter.variant, filter.subtype, position, Vector.Zero, nil):ToPickup()
    entity.Wait = 9000
    entitySerial = entitySerial + 1
    local data = entity:GetData()
    data[entityDataKey] = {id = entitySerial, label = "fixture-entity"}
    data.isaacCsFixtureState = {value = 100 + entitySerial, owner = CS.GetLocalPlayer()}
    retainedReference = entity
    releaseReferenceAfter = nil
    emit("entity-spawned", {id = entitySerial, seed = entity.InitSeed})
end
function actions.remove()
    if not entity or not entity:Exists() then error("需要当前房间的夹具实体") end
    emit("entity-remove-request", {id = entityData(entity).id, seed = entity.InitSeed})
    entity:Remove()
    entity = nil
    -- 保留到两次 World 读取，覆盖旧 userdata 已脱离成员表的路径。
    releaseReferenceAfter = reads.world + 2
end
function actions.bump()
    fixture.world.score = fixture.world.score + 10
    fixture.world.optional = nil
    -- 删掉键 12，交换字符串键的插入顺序；客户端应清理旧键。
    fixture.world.nested.rows[1].counts = {[-7] = 23}
    fixture.world.nested.byName = {alpha = {1.25, 2.5}, zulu = {5, 6}}
end
function actions.mark()
    fixture.marked = (fixture.marked or 0) + 1
    CS.MarkDirty(fixture, "marked")
end
function actions.unmarked() fixture.marked = (fixture.marked or 0) + 100 end
actions["read-error"] = function() readFault = "read-error" end
actions["wrong-type"] = function() readFault = "wrong-type" end
actions["too-many"] = function() readFault = "too-many" end
actions["long-string"] = function() readFault = "long-string" end
actions["slow-read"] = function()
    fixture.world.score = fixture.world.score + 1000
    readFault = "slow-read"
end
actions["set-error"] = function() fixture.fragile = -1 end
function actions.restore()
    readFault = nil
    fixture.fragile = 2
    fixture.world.score = fixture.world.score + 1
end
function actions.cadence()
    emit("cadence", {world_reads = reads.world, low_reads = reads.low, mark_reads = reads.mark,
        world = fixture.world.score, marked = fixture.marked, read_fault = readFault})
end

-- 执行控制台命令时只排队，玩法动作留到当前局真正的 Host 更新窗口。
fixture:AddCallback(ModCallbacks.MC_EXECUTE_CMD, function(_, command, parameters)
    if command ~= "cs_mod_fixture" or CS.IsClient() then return end
    local action = (parameters or ""):match("^%s*([%w-]+)%s*$")
    if not running or not action or not actions[action] or pendingAction then
        return "IsaacCS fixture: action unavailable"
    end
    pendingAction = action
    emit("action-queued", {name = action})
    return "IsaacCS fixture: " .. action
end)

-- 在 Player 原生更新窗口读取同一 Authority 输入，避免 Game 尾部回调已经收束输入。
fixture:AddCallback(ModCallbacks.MC_POST_PLAYER_UPDATE, function(_, player)
    if not running or CS.IsClient() or Game():IsPaused() then return end
    local ordinal = CS.GetParticipant(player)
    if ordinal == nil then return end
    local representative = CS.GetPlayerByParticipant(ordinal)
    if not representative or representative.Index ~= player.Index then return end
    local sample = public[ordinal] or {ordinal = ordinal, score = 0, edges = 0, move_edges = 0}
    local action = ButtonAction.ACTION_RIGHT
    local frame = Isaac.GetFrameCount()
    local edge = CS.IsKeyTriggered(player, key)
    sample.held = CS.IsKeyPressed(player, key)
    sample.moving = Input.IsActionPressed(action, player.ControllerIndex)
    sample.move_edge = Input.IsActionTriggered(action, player.ControllerIndex)
    sample.move_value = Input.GetActionValue(action, player.ControllerIndex)
    local newEdge = edge and sample.edge_frame ~= frame
    local newMove = sample.move_edge and sample.move_frame ~= frame
    if newEdge then sample.edges, sample.edge_frame = sample.edges + 1, frame end
    if newMove then sample.move_edges, sample.move_frame = sample.move_edges + 1, frame end
    public[ordinal], private[ordinal] = sample, "private-for-" .. ordinal
    local signature = tostring(sample.held) .. "/" .. tostring(sample.moving)
    if inputSignatures[ordinal] ~= signature or newEdge or newMove then
        inputSignatures[ordinal] = signature
        emit("authority-input", {ordinal = ordinal, controller = player.ControllerIndex,
            held = sample.held, edge = edge, moving = sample.moving, move_edge = sample.move_edge,
            move_value = sample.move_value, edges = sample.edges, move_edges = sample.move_edges})
    end
end)

fixture:AddCallback(ModCallbacks.MC_POST_UPDATE, function()
    if not running or CS.IsClient() or Game():IsPaused() then return end
    if pendingAction then
        local action = pendingAction
        pendingAction = nil
        local ok, err = pcall(actions[action])
        emit("action", {name = action, ok = ok, error = not ok and tostring(err) or nil})
    end
    if releaseReferenceAfter and reads.world >= releaseReferenceAfter then
        retainedReference, releaseReferenceAfter = nil, nil
    end
    fixture.world.owner, fixture.world.target = CS.GetLocalPlayer(), retainedReference
    native.update()
end)

-- 渲染输入用于本端界面。意图在 Host 被执行，单机分支的 handler 同次调用即可改变计分。
local function send(delta, note, probe)
    local before = fixture.world.score
    local target
    if not probe and #note <= 512 then
        target = CS.IsClient() and fixture.world.target or (entity and entity:Exists() and entity or nil)
    end
    local seq = CS.SendIntent(fixture, "score", {delta = delta, note = note, target = target})
    if seq then pendingIntents[seq] = {frame = Isaac.GetFrameCount(), probe = probe} end
    if not probe then
        emit("intent-send", {seq = seq, accepted = seq ~= nil, delta = delta, length = #note,
            before = before, after = fixture.world.score})
    end
    return seq ~= nil
end

-- 单批 12 条检查 8 条在途上限。速率用例等前批结果回收之后再发，区分两种限流。
local function burst()
    local accepted = 0
    for _ = 1, 12 do if send(0, "burst", "burst") then accepted = accepted + 1 end end
    emit("intent-burst", {accepted = accepted, rejected = 12 - accepted})
end
local function driveRate()
    if not rateProbe then return end
    local elapsed = Isaac.GetTime() - rateProbe.started
    if elapsed >= 900 or rateProbe.attempted >= 24 then
        emit("intent-rate", {elapsed_ms = elapsed, attempted = rateProbe.attempted,
            accepted = rateProbe.accepted, rejected = rateProbe.attempted - rateProbe.accepted})
        rateProbe = nil
        return
    end
    if next(pendingIntents) then return end
    for _ = 1, 8 do
        rateProbe.attempted = rateProbe.attempted + 1
        if send(0, "rate", "rate") then rateProbe.accepted = rateProbe.accepted + 1 end
    end
end

-- 快照只输出可独立比对的普通数据，保留 nil、键缺席和字节编码后的分量。
local function snapshot(player, event)
    local w, n = fixture.world, fixture.world.nested
    local row = n.rows and n.rows[1]
    local target = CS.IsClient() and w.target or (entity and entity:Exists() and entity or nil)
    local data = entityData(target)
    local state = data and target:GetData().isaacCsFixtureState
    local visiblePrivate, visiblePublic = {}, {}
    for ordinal = 0, 3 do
        if private[ordinal] then visiblePrivate[#visiblePrivate + 1] = {ordinal = ordinal, value = private[ordinal]} end
        if public[ordinal] then visiblePublic[#visiblePublic + 1] = public[ordinal] end
    end
    local current = native.observe(player)
    local level = Game():GetLevel()
    emit(event, {run = w.epoch, stage = level:GetStage(), room = level:GetCurrentRoomIndex(),
        score = w.score, enabled = w.enabled, weight = w.weight, precise = w.precise,
        text = w.text, optional = w.optional, has_optional = w.optional ~= nil,
        point = w.point and {w.point.X, w.point.Y}, tint = w.tint and {w.tint.R, w.tint.G, w.tint.B, w.tint.A, w.tint.RO, w.tint.GO, w.tint.BO},
        ink = w.ink and {w.ink.Red, w.ink.Green, w.ink.Blue, w.ink.Alpha},
        sparse = row and {name = row.name, negative = row.counts[-7], positive = row.counts[12]},
        names = n.byName, shared_equal = n.one and n.two and n.one.value == n.two.value,
        shared_alias = n.one ~= nil and n.one == n.two,
        cycle_value = n.cycle and n.cycle.next and n.cycle.next.value,
        cycle_alias = n.cycle ~= nil and n.cycle == n.cycle.next,
        preserves_world = rawequal(w, localWorld), preserves_nested = rawequal(n, localNested),
        local_world = w.local_only, local_nested = n.local_only,
        has_target = target ~= nil, target_id = data and data.id, entity_value = state and state.value,
        owner = w.owner and CS.GetParticipant(w.owner), low = fixture.low, marked = fixture.marked,
        fragile = fixture.fragile, public = visiblePublic, private = visiblePrivate, native = current})
end

local function render()
    if not running and not clientScene then return end
    local player = CS.GetLocalPlayer()
    local displayPlayer = CS.GetPlayerByParticipant(0)
    if not player or not displayPlayer then return end
    local pressed = function(button) return Input.IsButtonTriggered(button, 0) end
    if pressed(Keyboard.KEY_F6) then send(3, "normal") end
    if pressed(Keyboard.KEY_F7) then send(-1, "expected failure") end
    if pressed(Keyboard.KEY_F8) then burst() end
    if pressed(Keyboard.KEY_F9) then send(0, string.rep("x", 513)) end
    if pressed(Keyboard.KEY_F10) and CS.IsClient() then native.playLocal() end
    if pressed(Keyboard.KEY_F11) and CS.IsClient() and fixture.world.score ~= nil then
        local before = fixture.world.score
        fixture.world.score = -9001
        driftFrame = Isaac.GetFrameCount()
        emit("local-drift", {before = before, after = fixture.world.score})
    end
    if pressed(Keyboard.KEY_F5) and not rateProbe and not next(pendingIntents) then
        rateProbe = {started = Isaac.GetTime(), attempted = 0, accepted = 0}
    end
    driveRate()
    if pressed(Keyboard.KEY_F4) then snapshot(displayPlayer, "snapshot") end
    if driftFrame and Isaac.GetFrameCount() ~= driftFrame and fixture.world.score ~= -9001 then
        emit("drift-restored", {since = driftFrame, score = fixture.world.score})
        driftFrame = nil
    end
    if fixture.world.score == nil then return end
    local pgd = Isaac.GetPersistentGameData()
    local unlocked = pgd:Unlocked(config.achievement)
    local color = fixture.world.ink or KColor.White
    Isaac.RenderText(("%s  score=%d  cache=%.2f  camo=%s"):format(unlocked and "UNLOCKED" or "[LOCKED]",
        fixture.world.score, displayPlayer:GetCustomCacheValue("isaaccs_e2e_stat"), tostring(displayPlayer:HasForcedCamoEffect())),
        30, 56, color.Red, color.Green, color.Blue, color.Alpha)
    Isaac.RenderText(Game():GetLevel():GetName(), 30, 68, 1, 1, 1, 1)
end
fixture:AddCallback(ModCallbacks.MC_POST_RENDER, render)
