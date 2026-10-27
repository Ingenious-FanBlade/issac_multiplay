-- tx-04-08 专用世界夹具。持有的对象仅用于本房间动作，不充当 DLL 身份或跨进程 key。
-- 已知失败方式：参数未核定、对象被游戏退休、同址复用、切房后旧引用、API 返回但状态未到达、
-- 格子被占用、Morph 不返回 bool、种子命令重启 run、动作重入/超时。任一错误停止队列，不重试。
local M = { names = { seed = true, spawn = true, spawn_short_lived = true, remove = true, morph_npc = true,
    morph_pickup = true, grid_spawn = true, grid_remove = true, grid_replace = true,
    world_observe = true } }
local emit, finish, configuration
local objects, grids, roomGeneration = {}, {}, 0

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

function M.install(fixture, config, output, terminal)
    emit, finish, configuration = output, terminal, config
    -- 加载与退 run 一并废弃句柄，不把旧 Lua userdata 当成物理对象连续性。
    local function invalidate(reason)
        objects, grids = {}, {}
        roomGeneration = roomGeneration + 1
        emit("world-fixture-invalidate", { reason = reason, room_generation = roomGeneration })
    end
    fixture:AddCallback(ModCallbacks.MC_POST_NEW_ROOM, function() invalidate("new_room") end)
    fixture:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT, function() invalidate("run_exit") end)
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
        if current.name == "grid_remove" or current.name == "grid_replace" then
            assert(current.data.before.present, "grid_missing")
            assert(s.expected_type == current.data.before.type and s.expected_variant == current.data.before.variant,
                "grid_before_mismatch")
            assert(s.id and grids[s.id] and grids[s.id].room_generation == roomGeneration, "grid_not_fixture_owned")
            assert(grids[s.id].seed == current.data.before.seed, "grid_seed_changed_since_fixture_owner")
            if current.name == "grid_remove" then
                assert(type(s.keep_decoration) == "boolean", "keep_decoration_required")
                assert(s.path_trail == grids[s.id].original_path and s.path_trail == 0, "grid_path_not_original_clear_floor")
                room:RemoveGridEntityImmediate(index, integer(s.path_trail, -2147483648, 2147483647, "path_trail"), s.keep_decoration)
                assert(not room:GetGridEntity(index), "grid_not_removed")
            end
        else
            assert(not current.data.before.present, "grid_already_occupied")
            local position = room:GetGridPosition(index)
            assert(room:IsPositionInRoom(position, 0) and room:GetGridCollisionAtPos(position) == 0, "grid_not_clear_floor")
            assert(room:GetGridPath(index) == 0, "grid_path_not_clear_before_fixture")
        end
        if current.name ~= "grid_remove" then
            -- replace 直接走 SpawnGridEntity 的真实覆盖 owner，不先 Remove 冒充原生替换。
            local result = room:SpawnGridEntity(index, integer(s.type, 0, 65535, "grid_type"),
                integer(s.variant, 0, 65535, "grid_variant"), integer(s.seed, 1, 4294967295, "grid_seed"),
                integer(s.var_data, -2147483648, 2147483647, "grid_var_data"))
            assert(result == true, "grid_spawn_false")
        end
        current.phase = "world_grid"
        if s.id then
            grids[s.id] = current.name ~= "grid_remove" and { index = index,
                room_generation = roomGeneration, seed = s.seed, original_path = 0 } or nil
        end
    end
    current.data.return_frame = Game():GetFrameCount()
    emit("world-fixture", { action = current.name, step = current.step, action_id = current.action_id,
        phase = "call_return", before = current.data.before, created = current.data.created, return_frame = current.data.return_frame,
        id = s.id, callback_phase = "MC_POST_RENDER" })
end

-- 动作至少跨过一次实际游戏帧；Lua 完成只证明视图到达，DLL 原生链由统一归约单独判定。
function M.poll(current)
    local s, data = current.spec, current.data
    if Game():GetFrameCount() == data.return_frame then return end
    if current.phase == "world_seed" then
        if Game():GetSeeds():GetStartSeedString() == s.seed and Game():GetNumPlayers() == 1 then
            ready(current, { seed = s.seed }); return
        end
    elseif current.phase == "world_remove" then
        if not data.entity:Exists() then
            objects[s.id] = nil
            ready(current, { before = data.before, created = data.created, after = { exists = false } })
        end
    elseif current.phase == "world_spawn" or current.phase == "world_morph" then
        local after = entityView(owned(s))
        if current.name == "morph_pickup" and s.keep_seed then
            assert(after.seed == data.before.seed, "pickup_keep_seed_changed")
        end
        if after.type == s.type and after.variant == s.variant and after.subtype == s.subtype then
            ready(current, { before = data.before, after = after })
        end
    elseif current.phase == "world_grid" then
        local after = gridView(s.index)
        if (current.name == "grid_remove" and not after.present)
            or (after.present and after.type == s.type and after.variant == s.variant
                and after.seed == s.seed and after.var_data == s.var_data) then
            ready(current, { before = data.before, after = after })
        end
    end
end
return M
