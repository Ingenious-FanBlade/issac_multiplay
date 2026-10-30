-- tx-04-08 专用世界夹具。持有的对象仅用于本房间动作，不充当 DLL 身份或跨进程 key。
-- 已知失败方式：参数未核定、对象被游戏退休、同址复用、切房后旧引用、API 返回但状态未到达、
-- 格子被占用、Morph 不返回 bool、种子命令重启 run、动作重入/超时。任一错误停止队列，不重试。
local M = { names = { seed = true, spawn = true, spawn_short_lived = true, remove = true, morph_npc = true,
    morph_pickup = true, grid_spawn = true, grid_remove = true, grid_replace = true,
    world_observe = true, player_esau = true, pool_cycle = true } }
local emit, finish, configuration
local objects, grids, roomGeneration = {}, {}, 0
local playerInitSerial = 0
local esauPending = nil
local json = require("json")

-- AddCollectible完成不等于消费者已可使用；这里只读公开状态，等待原生自行恢复。
local function esauStatus(player)
    return { controls_enabled = player:AreControlsEnabled(),
        animation_finished = player:IsExtraAnimationFinished(),
        item_state = player:GetItemState(), item_cooldown = player:GetItemStateCooldown(),
        active_item = player:GetActiveItem(0), active_charge = player:GetActiveCharge(0),
        min_charge = player:GetActiveMinUsableCharge(0), max_charge = player:GetActiveMaxCharge(0) }
end

-- 只接受显式场景参数；按类型、范围和当前对象状态保护 API 调用，不设流程确认字段。
local function integer(v, lo, hi, name)
    assert(type(v) == "number" and v == math.floor(v) and v >= lo and v <= hi, name)
    return v
end
local function vector(v, name)
    assert(type(v) == "table" and #v == 2, name)
    for _, n in ipairs(v) do assert(type(n) == "number" and n == n and math.abs(n) <= 100000, name) end
    return Vector(v[1], v[2])
end
local function entityView(e)
    if not e or not e:Exists() then return { exists = false } end
    return { exists = true, type = e.Type, variant = e.Variant,
        subtype = e.SubType, seed = e.InitSeed, index = e.Index,
        position = { e.Position.X, e.Position.Y }, velocity = { e.Velocity.X, e.Velocity.Y } }
end
local function owned(spec)
    local item = objects[spec.id]
    assert(item and item.room_generation == roomGeneration, "unknown_or_previous_room_object")
    assert(item.entity:Exists()
        and item.entity.InitSeed == item.seed and item.entity.Index == item.index, "object_retired_or_reused")
    return item.entity
end
local function gridView(index)
    local grid = Game():GetRoom():GetGridEntity(index)
    if not grid then return { present = false, index = index } end
    return { present = true, index = index, type = grid:GetType(), variant = grid:GetVariant(),
        state = grid.State, var_data = grid.VarData, seed = grid:GetSaveState().SpawnSeed,
        path_trail = Game():GetRoom():GetGridPath(index) }
end
-- 固定格子的空地查询只作核对，不把返回的附近位置改成新的动作目标。
local function clearGridPosition(room, index)
    assert(not room:GetGridEntity(index), "grid_already_occupied")
    local position = room:GetGridPosition(index)
    assert(room:IsPositionInRoom(position, 0) and room:GetGridCollisionAtPos(position) == 0, "grid_not_clear_floor")
    assert(room:GetGridPath(index) == 0, "grid_path_not_clear_before_fixture")
    local free = Isaac.GetFreeNearPosition(position, 1)
    assert(free and room:GetGridIndex(free) == index, "grid_free_near_position_changed_cell")
    return { position = { position.X, position.Y }, free_near = { free.X, free.Y }, index = index }
end
-- 明确保存 bool 的实际类型和值；失败也先落日志，不能把 nil、false 或异常混为一个返回。
local function spawnGrid(current)
    local s, data = current.spec, current.data
    assert(data.grid_room_generation == roomGeneration, "grid_spawn_previous_room")
    local room = Game():GetRoom()
    data.clear = clearGridPosition(room, s.index)
    current.phase = "world_grid"
    emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
        phase = "grid_spawn_call", route = data.route, clear = data.clear })
    local target = data.grid_target
    local result = room:SpawnGridEntity(s.index, target.type, target.variant, target.seed, target.var_data)
    data.spawn_return = { type = type(result), value = tostring(result) }
    data.created = gridView(s.index)
    -- 用户要求 Grid variant 仅观察；同时保留请求值和实际值，不把楼层变种当失败。
    data.variant_observation.requested = target.variant
    data.variant_observation.after_spawn = data.created.variant
    emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
        phase = "grid_spawn_return", route = data.route, raw_return = data.spawn_return, after = data.created,
        variant_policy = "OBSERVE_ONLY", variant_observation = data.variant_observation })
    assert(result == true, "grid_spawn_not_true")
    assert(data.created.present and data.created.type == target.type
        and data.created.seed == target.seed and data.created.var_data == target.var_data, "grid_spawn_parameters_changed")
    -- ToPoop 仅取得已是便便的派生接口；类型变更由删除与重新生成完成。
    if target.type == GridEntityType.GRID_POOP then
        data.poop_accessor = room:GetGridEntity(s.index):ToPoop() ~= nil
        assert(data.poop_accessor, "spawned_grid_not_poop")
    end
    if s.id then
        grids[s.id] = { index = s.index, room_generation = roomGeneration,
            seed = target.seed, variant = data.created.variant, original_path = data.original_path }
    end
    data.return_frame = Game():GetFrameCount()
end
local function ready(current, detail)
    emit("world-fixture", { action = current.name, step = current.step,
        action_id = current.action_id, phase = "observed", detail = detail })
    finish("arrived", { detail = detail, native_lifecycle = "UNVERIFIED_BY_LUA" })
end

-- 独立 Lua 只读视图仅供限定字段交叉证据；其 witness 不能冒充帧尾完整采样。
local function observe(current)
    local entities = {}
    for _, e in ipairs(Isaac.GetRoomEntities()) do
        assert(#entities < 4096, "lua_observation_entity_budget")
        entities[#entities + 1] = entityView(e)
    end
    local grids, doors = {}, {}
    local room = Game():GetRoom()
    for index = 0, room:GetGridSize() - 1 do
        local grid = gridView(index)
        if grid.present then grids[#grids + 1] = grid end
    end
    for slot = 0, 7 do
        local door = room:GetDoor(slot)
        if door then doors[#doors + 1] = { slot = slot, index = room:GetGridIndex(door.Position),
            target = door.TargetRoomIndex } end
    end
    -- 独立视图分批写日志，避免一个大型 JSON 被游戏日志缓冲截断；action_id/offset 负责对账。
    for kind, values in pairs({ entities = entities, grids = grids, doors = doors }) do
        for offset = 1, #values, 16 do
            local chunk = {}
            for i = offset, math.min(offset + 15, #values) do chunk[#chunk + 1] = values[i] end
            emit("world-lua-view", { action_id = current.action_id, step = current.step,
                kind = kind, offset = offset, values = chunk, total = #values, phase = "MC_POST_RENDER" })
        end
    end
    ready(current, { entity_count = #entities, grid_count = #grids, door_count = #doors,
        seed = Game():GetSeeds():GetStartSeedString(), phase = "MC_POST_RENDER" })
end

-- 每轮只创建一个普通 Familiar，正常更新后 Remove，再等原生清理；不占满池或指定复用槽。
local function cycleSpawn(current)
    local s, data = current.spec, current.data
    local seed = s.seed + data.round
    local a = Isaac.GetPlayer(0)
    assert(a and a:Exists() and not a:IsDead(), "cycle_spawner_unavailable")
    local position = vector(s.position, "cycle_position")
    assert(Game():GetRoom():GetGridCollisionAtPos(position) == 0, "cycle_position_blocked")
    local e = Game():Spawn(s.type, s.variant, position, vector(s.velocity, "cycle_velocity"), a, s.subtype, seed)
    assert(e and e:Exists() and e.Type == s.type and e.Variant == s.variant and e.InitSeed == seed,
        "cycle_spawn_parameters_changed")
    data.entity, data.spawn_frame = e, Game():GetFrameCount()
    data.remove_frame, data.absent_frame = nil, nil
    current.phase = "world_pool_cycle"
    emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
        phase = "cycle_created", round = data.round + 1, entity = entityView(e) })
end

function M.install(fixture, config, output, terminal)
    emit, finish, configuration = output, terminal, config
    -- 加载与退 run 一并废弃句柄，不把旧 Lua userdata 当成物理对象连续性。
    local function invalidate(reason)
        esauPending = nil
        objects, grids = {}, {}
        roomGeneration = roomGeneration + 1
        emit("world-fixture-invalidate", { reason = reason, room_generation = roomGeneration })
    end
    fixture:AddCallback(ModCallbacks.MC_POST_NEW_ROOM, function() invalidate("new_room") end)
    fixture:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT, function() invalidate("run_exit") end)
    -- 仅观察真实 Player Init 回调；不把 backup 初始化等同于 active roster 已替换。
    fixture:AddCallback(ModCallbacks.MC_POST_PLAYER_INIT, function(_, player)
        playerInitSerial = playerInitSerial + 1
        emit("world-player-init", { serial = playerInitSerial, player = entityView(player),
            player_type = player:GetPlayerType(), callback_phase = "MC_POST_PLAYER_INIT" })
    end)
    -- 无返回值的只读观察：区分没有进入UseActiveItem与进入后未Replace，不修改回调结果。
    fixture:AddCallback(ModCallbacks.MC_POST_USE_ITEM, function(_, item, _, player, flags, slot, varData, discharged, removed)
        local current = esauPending
        if not current or item ~= 703 then return end
        current.data.use_callbacks = (current.data.use_callbacks or 0) + 1
        emit("world-fixture", { action = "player_esau", step = current.step, action_id = current.action_id,
            phase = "native_use_observed", item = item, slot = slot, flags = flags, var_data = varData,
            discharged = discharged, removed = removed, status = esauStatus(player),
            callback_phase = "MC_POST_USE_ITEM" })
    end, 703)
end

-- 所有世界写入在游戏线程的 F8 动作中执行一次；不提供任意代码/地址/原生 ABI 调用入口。
function M.request(current)
    local s = current.spec
    assert(configuration.tx == "tx-04-08" and type(configuration.token) == "string"
        and #configuration.token > 0, "missing_tx08_token")
    if current.name == "world_observe" then observe(current); return end
    emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
        phase = "call", parameters = s, room_generation = roomGeneration, callback_phase = "MC_POST_RENDER" })
    if current.name == "seed" then
        s.seed = s.seed or configuration.seed or Game():GetSeeds():GetStartSeedString()
        assert(type(s.seed) == "string" and s.seed:match("^[A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9] [A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9]$"), "seed_format")
        assert(Game():GetNumPlayers() == 1, "seed_requires_single_A_before_host")
        emit("world-fixture", { action = current.name, action_id = current.action_id,
            phase = "seed_resolved", seed = s.seed })
        Isaac.ExecuteCommand("seed " .. s.seed)
        current.phase = "world_seed"
    elseif current.name == "player_esau" then
        -- 只准备普通 A 的真实 primary 物品；使用由物理SPACE经统一输入进入原生consumer。
        -- 不直接调UseActiveItem、不写_itemToUse，也不创建Player或改变B的Join路线。
        assert(s.item == 703 and s.ordinal == 0 and s.slot == 0 and s.charge == 1
            and s.input_key == "SPACE" and s.input_count == 1, "esau_concrete_route")
        local a = Isaac.GetPlayer(0)
        assert(a and a:Exists() and a.Type == 1 and a.Variant == 0 and not a:IsDead() and not a:IsCoopGhost()
            and a:GetPlayerType() == 0 and Game():GetNumPlayers() == 2, "esau_requires_active_Isaac_A_and_B")
        local item = Isaac.GetItemConfig():GetCollectible(703)
        assert(item, "esau_item_config_missing")
        local charge = integer(s.charge, 1, 1, "esau_charge")
        assert(item.MaxCharges == 1, "esau_target_config_changed")
        current.data.before = entityView(a)
        current.data.before.player_type = a:GetPlayerType()
        current.data.before.active_item = a:GetActiveItem(0)
        current.data.before.active_charge = a:GetActiveCharge(0)
        current.data.before.item_state = a:GetItemState()
        current.data.before.has_item = a:HasCollectible(703, true)
        emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
            phase = "preparation_before", before = current.data.before, status = esauStatus(a) })
        assert(current.data.before.item_state == 0, "esau_requires_idle_item_state")
        current.data.old_player = a
        current.data.old_ptr = GetPtrHash(a)
        if s.expect_new_player == false then
            -- 原生切换后的槽显示与库存可能尚不一致；只用公开道具接口准备当前主动槽与一次充能。
            if a:GetActiveItem(0) ~= 703 then
                a:AddCollectible(703, charge, false, 0, 0)
            end
            a:SetActiveCharge(charge, 0)
        else
            a:AddCollectible(703, charge, false, 0, 0)
        end
        emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
            phase = "preparation_after", status = esauStatus(a), has_item = a:HasCollectible(703, true) })
        -- Esau Jr 备用角色的原生活动槽可为703而库存计数为0，返回切换不能用库存冒充槽可用性。
        assert((s.expect_new_player == false or a:HasCollectible(703, true)) and a:GetActiveItem(0) == 703
            and a:GetActiveCharge(0) >= charge, "esau_primary_not_prepared")
        -- 已静态核对的公开只读 getter；不调用 Set/ResetItemState 强造资格。
        current.data.prepared_item_state = a:GetItemState()
        assert(current.data.prepared_item_state == 0, "esau_prepared_item_state_not_idle")
        current.data.player_init_serial = playerInitSerial
        current.data.preparation_room = roomGeneration
        current.phase = "world_player_esau_prepare"
        esauPending = current
    elseif current.name == "pool_cycle" then
        assert(s.type == 3 and s.variant == 1 and s.subtype == 0, "cycle_requires_ordinary_familiar")
        integer(s.count, 65, 80, "cycle_count")
        integer(s.seed, 1, 4294967200, "cycle_seed")
        current.data.round = 0
        cycleSpawn(current)
    elseif current.name == "spawn" or current.name == "spawn_short_lived" then
        assert(type(s.id) == "string" and not objects[s.id], "duplicate_object_label")
        local kind = integer(s.type, 2, 9999, "spawn_type")
        assert(kind ~= 7 and kind ~= 8, "stable_fixture_disallows_laser_bomb")
        local variant = integer(s.variant, 0, 65535, "spawn_variant")
        local subtype = integer(s.subtype, 0, 2147483647, "spawn_subtype")
        local seed = integer(s.seed, 1, 4294967295, "spawn_seed")
        local position = vector(s.position, "position")
        local room = Game():GetRoom()
        assert(room:IsPositionInRoom(position, 0) and room:GetGridCollisionAtPos(position) == 0, "spawn_position_not_clear_floor")
        -- Game:Spawn 的顺序已静态核定，避免 Isaac.Spawn 的自动 seed helper。
        local spawner = nil
        if s.spawner_ordinal ~= nil then
            spawner = Isaac.GetPlayer(integer(s.spawner_ordinal, 0, 1, "spawner_ordinal"))
            assert(spawner and spawner:Exists() and not spawner:IsDead() and not spawner:IsCoopGhost(), "spawner_unavailable")
        end
        local e = Game():Spawn(kind, variant, position,
            vector(s.velocity, "velocity"), spawner, subtype, seed)
        assert(e and e:Exists(), "spawn_return_missing")
        local view = entityView(e)
        assert(view.type == kind and view.variant == variant and view.subtype == subtype and view.seed == seed,
            "spawn_concrete_parameters_changed")
        objects[s.id] = { entity = e, room_generation = roomGeneration,
            seed = view.seed, index = view.index }
        current.data.before = { exists = false }
        current.data.created = view
        current.phase = "world_spawn"
        if current.name == "spawn_short_lived" then
            -- 同一动作内按公开 owner 顺序创建再 Remove，生命周期仍由 DLL 真实事件链判断。
            current.data.entity = e
            e:Remove()
            current.phase = "world_remove"
        end
    elseif current.name == "remove" then
        local e = owned(s)
        current.data.before = entityView(e)
        current.data.entity = e
        e:Remove()
        current.phase = "world_remove"
    elseif current.name == "morph_npc" or current.name == "morph_pickup" then
        local e = owned(s)
        local kind = integer(s.type, 2, 9999, "morph_type")
        local variant = integer(s.variant, 0, 65535, "morph_variant")
        local subtype = integer(s.subtype, 0, 2147483647, "morph_subtype")
        current.data.before = entityView(e)
        if current.name == "morph_npc" then
            local npc = e:ToNPC(); assert(npc, "not_npc")
            npc:Morph(kind, variant, subtype, integer(s.champion, -1, 65535, "champion"))
        else
            assert(kind == 5, "pickup_target_type")
            local pickup = e:ToPickup(); assert(pickup, "not_pickup")
            assert(type(s.keep_price) == "boolean" and type(s.keep_seed) == "boolean"
                and type(s.ignore_modifiers) == "boolean", "pickup_flags_required")
            pickup:Morph(kind, variant, subtype, s.keep_price, s.keep_seed, s.ignore_modifiers)
        end
        -- 两个公开 Morph 均无 bool 返回值；下一轮观察实际类型，不声称返回成功。
        current.phase = "world_morph"
        -- Morph 可以改变 InitSeed；更新夹具句柄的防复用指纹，不把它当成 DLL use/birth 证据。
        objects[s.id].seed = e.InitSeed
    else
        local room = Game():GetRoom()
        -- 固定显式格子；不为了通过而寻找/删除其他占用对象。
        local index = integer(s.index, 0, math.min(447, room:GetGridSize() - 1), "grid_index")
        for slot = 0, 7 do
            local door = room:GetDoor(slot)
            assert(not door or room:GetGridIndex(door.Position) ~= index, "grid_is_door_alias")
        end
        if s.id and grids[s.id] then
            assert(grids[s.id].room_generation == roomGeneration and grids[s.id].index == index, "grid_label_previous_room")
        end
        current.data.before = gridView(index)
        current.data.variant_observation = { actual_before = current.data.before.variant,
            fixture_before = s.id and grids[s.id] and grids[s.id].variant or nil }
        current.data.grid_room_generation = roomGeneration
        current.data.route = current.name == "grid_replace" and "remove_then_spawn" or current.name
        -- 所有创建参数在任何删除之前核定；删除后失败保留空格，不回滚或自动重试。
        if current.name ~= "grid_remove" then
            current.data.grid_target = { type = integer(s.type, 0, 65535, "grid_type"),
                variant = integer(s.variant, 0, 65535, "grid_variant"),
                seed = integer(s.seed, 1, 4294967295, "grid_seed"),
                var_data = integer(s.var_data, -2147483648, 2147483647, "grid_var_data") }
        end
        if current.name == "grid_remove" or current.name == "grid_replace" then
            assert(current.data.before.present, "grid_missing")
            -- variant 不承担身份判断；位置/房间代次/自有标签/类型/seed 仍须匹配。
            assert(s.expected_type == current.data.before.type, "grid_before_mismatch")
            assert(s.id and grids[s.id] and grids[s.id].room_generation == roomGeneration, "grid_not_fixture_owned")
            assert(grids[s.id].seed == current.data.before.seed, "grid_seed_changed_since_fixture_owner")
            assert(s.keep_decoration == false, "grid_requires_explicit_remove_decoration")
            assert(s.path_trail == grids[s.id].original_path and s.path_trail == 0, "grid_path_not_original_clear_floor")
            if current.name == "grid_replace" then
                assert(s.replacement_mode == "remove_then_spawn", "grid_replacement_mode_required")
            end
            current.data.original_path = grids[s.id].original_path
            emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
                phase = "grid_remove_call", route = current.data.route, before = current.data.before,
                path_trail = s.path_trail, keep_decoration = s.keep_decoration })
            room:RemoveGridEntityImmediate(index, integer(s.path_trail, -2147483648, 2147483647, "path_trail"), s.keep_decoration)
            current.data.removed = gridView(index)
            emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
                phase = "grid_remove_return", route = current.data.route, after = current.data.removed })
            assert(not current.data.removed.present, "grid_not_removed")
            grids[s.id] = nil
            -- 跨过一次真实游戏帧再确认空地，不调用 Room:Update 额外推进原生模拟。
            current.phase = current.name == "grid_replace" and "world_grid_replace_cleared" or "world_grid"
        else
            current.data.original_path = 0
            spawnGrid(current)
        end
    end
    current.data.return_frame = Game():GetFrameCount()
    emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
        phase = "call_return", before = current.data.before, created = current.data.created, return_frame = current.data.return_frame,
        id = s.id, route = current.data.route, removed = current.data.removed,
        raw_return = current.data.spawn_return, variant_observation = current.data.variant_observation,
        callback_phase = "MC_POST_RENDER" })
end

-- 动作至少跨过一次实际游戏帧；Lua 完成只证明视图到达，DLL 原生链由统一归约单独判定。
function M.poll(current)
    local s, data = current.spec, current.data
    if Game():GetFrameCount() == data.return_frame then return end
    if current.phase == "world_seed" then
        if Game():GetSeeds():GetStartSeedString() == s.seed and Game():GetNumPlayers() == 1 then
            ready(current, { seed = s.seed }); return
        end
    elseif current.phase == "world_player_esau_prepare" then
        local a = Isaac.GetPlayer(0)
        assert(a and a:Exists() and GetPtrHash(a) == data.old_ptr and roomGeneration == data.preparation_room
            and a:GetPlayerType() == 0 and a.Type == 1 and a.Variant == 0
            and not a:IsDead() and not a:IsCoopGhost() and Game():GetNumPlayers() == 2,
            "esau_preparation_identity_changed")
        local status = esauStatus(a)
        local encoded = json.encode(status)
        if encoded ~= data.last_preparation then
            data.last_preparation = encoded
            emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
                phase = "waiting_native_readiness", status = status, callback_phase = "MC_POST_RENDER" })
        end
        local usable = status.controls_enabled and status.animation_finished and status.item_state == 0
            and status.item_cooldown <= 0 and status.active_item == 703 and status.min_charge == 1
            and status.max_charge == 1 and status.active_charge >= status.min_charge
        if not usable then data.ready_frame = nil; return end
        -- 两个不同实际游戏帧均可用后才发布唯一一次发键请求，不采用墙钟延时重试。
        if not data.ready_frame then data.ready_frame = Game():GetFrameCount(); return end
        if data.ready_frame == Game():GetFrameCount() then return end
        current.phase = "world_player_esau"
        emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
            phase = "input_prepared", slot = 0, item = 703, charge = s.charge, item_state = status.item_state,
            native_readiness = status, input_key = "SPACE", input_count = 1,
            before = data.before, callback_phase = "MC_POST_RENDER" })
    elseif current.phase == "world_player_esau" then
        local a = Isaac.GetPlayer(0)
        -- 首次输入后的实际公开状态也保留，未发生Replace时不再仅剩一个笼统超时。
        if a and a:Exists() then
            local status = esauStatus(a)
            local encoded = json.encode(status)
            if encoded ~= data.last_input_status then
                data.last_input_status = encoded
                emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
                    phase = "after_input_status", status = status,
                    new_player_init_callbacks = playerInitSerial - data.player_init_serial,
                    use_callbacks = data.use_callbacks or 0, callback_phase = "MC_POST_RENDER" })
            end
        end
        -- 只等候该动作后真实新 Player Init；类型可能被保留，不猜 Esau 的 PlayerType。
        -- 真正交换及两侧地址/成员由 DLL episode 独立判断，Lua 终态不代表 Replace 成功。
        local initialized = playerInitSerial > data.player_init_serial
        local switched = a and GetPtrHash(a) ~= data.old_ptr
        if (s.expect_new_player == false and switched or s.expect_new_player ~= false and initialized) and a and a:Exists()
            and not a:IsDead() and not a:IsCoopGhost() and Game():GetNumPlayers() == 2 then
            local after = entityView(a)
            after.player_type = a:GetPlayerType()
            after.active_item = a:GetActiveItem(0)
            ready(current, { before = data.before, after = after, old_player_still_exists = data.old_player:Exists(),
                new_player_init_callbacks = playerInitSerial - data.player_init_serial,
                roster_address_changed = switched,
                route = "physical_SPACE_canonical_scheduler_native_item_consumer", native_replace = "REQUIRES_DLL_EPISODE" })
            esauPending = nil
        end
    elseif current.phase == "world_remove" then
        if data.absent_frame then
            if Game():GetFrameCount() - data.absent_frame < 2 then return end
            ready(current, { before = data.before, created = data.created, after = { exists = false },
                physical_destruction = "REQUIRES_NATIVE_EVENT_NOT_LUA_ABSENCE" })
        elseif not data.entity:Exists() then
            objects[s.id] = nil
            data.entity = nil
            data.absent_frame = Game():GetFrameCount()
        end
    elseif current.phase == "world_pool_cycle" then
        local frame = Game():GetFrameCount()
        if data.absent_frame then
            if frame - data.absent_frame < 2 then return end
            data.round = data.round + 1
            if data.round == s.count then
                ready(current, { rounds = data.round, route = "bounded_public_spawn_remove", forced_overwrite = false })
            else cycleSpawn(current) end
        elseif data.remove_frame then
            if not data.entity:Exists() then data.entity = nil; data.absent_frame = frame end
        elseif frame > data.spawn_frame then
            data.entity:Remove()
            data.remove_frame = frame
        end
    elseif current.phase == "world_spawn" or current.phase == "world_morph" then
        local after = entityView(owned(s))
        if current.name == "morph_pickup" and s.keep_seed then
            assert(after.seed == data.before.seed, "pickup_keep_seed_changed")
        end
        if after.type == s.type and after.variant == s.variant and after.subtype == s.subtype then
            ready(current, { before = data.before, after = after })
        end
    elseif current.phase == "world_grid_replace_cleared" then
        -- 删除后的旧 userdata 不再解引用；只用当前房间的同格查询与新 owner 返回。
        spawnGrid(current)
    elseif current.phase == "world_grid" then
        assert(data.grid_room_generation == roomGeneration, "grid_observe_previous_room")
        local after = gridView(s.index)
        if (current.name == "grid_remove" and not after.present)
            or (after.present and after.type == s.type
                and after.seed == s.seed and after.var_data == s.var_data) then
            data.variant_observation.after_observation = after.variant
            if current.name ~= "grid_remove" and s.id then grids[s.id].variant = after.variant end
            ready(current, { before = data.before, removed = data.removed, after = after,
                route = data.route, raw_return = data.spawn_return, clear = data.clear, poop_accessor = data.poop_accessor,
                variant_policy = "OBSERVE_ONLY", variant_observation = data.variant_observation })
        end
    end
end
return M
