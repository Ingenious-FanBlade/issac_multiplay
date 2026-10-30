-- tx-04-09整体E2E夹具：只由F8推进显式动作；独立记录所有原生身体及公开关系。
local fixture = RegisterMod("Isaac CS four participant E2E", 1)
local config = include("harness_config")
local json = require("json")
local serial, action, render = 0, 0, 0
local runReady = false
local actions = {"seed", "resources", "room", "kill3", "revive3", "stage", "host_jacob", "resources", "remote1_jacob", "resources", "forms", "flip", "esau_jr", "esau_jr", "kill0", "revive0"}
local function status()
    local result = {}
    for k,v in tostring(Isaac.ExecuteCommand("cs status")):gmatch("([^=\r\n]+)=([^\r\n]*)") do result[k]=v end
    return result
end
local function emit(event, value)
    value = value or {}
    serial = serial + 1
    value.event, value.token, value.serial, value.action, value.render = event, config.token, serial, action, render
    value.game_frame = Game():GetFrameCount()
    Isaac.DebugString("[isaac-cs-test] tx09 " .. json.encode(value))
end
-- 夹具按DLL已公布的设备key定位根，同时记录原生列表/关系；不以列表index当参与者身份。
local function roots()
    local s, result = status(), {}
    for _,p in ipairs(PlayerManager.GetPlayers()) do
        for ordinal=0,3 do
            if tonumber(s["participant."..ordinal..".controller"]) == p.ControllerIndex
                and not result[ordinal] then result[ordinal]=p end
        end
    end
    return result
end
local function snapshot()
    local s, bodies = status(), {}
    -- Getter可为同一对象创建新userdata包装；只在同步观察窗口按原生实体索引/seed去重。
    local function same_entity(a,b)
        return a and b and a.Index==b.Index and a.InitSeed==b.InitSeed
    end
    local function add(p, relation)
        if not p then return end
        for _,body in ipairs(bodies) do if same_entity(body.player,p) then return end end
        if #bodies>=64 then error("fixture topology overflow") end
        bodies[#bodies+1]={player=p, relation=relation}
    end
    for _,p in ipairs(PlayerManager.GetPlayers()) do add(p,"roster") end
    local cursor=1
    while cursor<=#bodies do
        local p=bodies[cursor].player
        add(p:GetOtherTwin(),"twin")
        add(p:GetSubPlayer(),"sub_player")
        cursor=cursor+1
    end
    local output={}
    for _,body in ipairs(bodies) do
        local p=body.player
        local ordinal=-1
        for i=0,3 do if tonumber(s["participant."..i..".controller"])==p.ControllerIndex then ordinal=i end end
        local shots=0
        for _,entity in ipairs(Isaac.GetRoomEntities()) do
            if same_entity(entity.SpawnerEntity,p) and (entity.Type==EntityType.ENTITY_TEAR or entity.Type==EntityType.ENTITY_LASER or entity.Type==EntityType.ENTITY_KNIFE) then shots=shots+1 end
        end
        output[#output+1]={shots=shots, ordinal=ordinal, relation=body.relation, index=p:GetPlayerIndex(), controller=p.ControllerIndex,
            player_type=p:GetPlayerType(), fire_direction=p:GetFireDirection(), hologram=p:IsHologram(), dead=p:IsDead(), ghost=p:IsCoopGhost(), exists=p:Exists(),
            hearts=p:GetHearts(), soul_hearts=p:GetSoulHearts(), bone_hearts=p:GetBoneHearts(),
            position={p.Position.X,p.Position.Y}, velocity={p.Velocity.X,p.Velocity.Y},
            shooting={p:GetShootingInput().X,p:GetShootingInput().Y}, bombs=p:GetNumBombs(),
            active=p:GetActiveItem(), charge=p:GetActiveCharge(), card=p:GetCard(0),
            twin=p:GetOtherTwin()~=nil, sub_player=p:GetSubPlayer()~=nil}
    end
    local backups={}
    for i=0,3 do
        local p=PlayerManager.GetEsauJrState(i)
        backups[#backups+1]={slot=i, present=p~=nil, player_type=p and p:GetPlayerType() or -1,
            controller=p and p.ControllerIndex or -1}
    end
    -- 游戏DebugString单条输出有长度上限；完整DLL状态由控制口保存，不塞进夹具JSON。
    return {players=output, esau_jr=backups, status={participants=s.participants,
        active_bodies=s.active_bodies, completed_tick=s.completed_tick, session_state=s.session_state}}
end
local function resources()
    local center=Game():GetRoom():GetCenterPos()
    for i,p in ipairs(PlayerManager.GetPlayers()) do
        -- seed命令沿用入口位置，先把夹具身体放回房中，避免初始就贴住出口。
        p.Position=center+Vector(((i-1)%4-1.5)*24,math.floor((i-1)/4)*24)
        p.Velocity=Vector.Zero
        p:AddBombs(30-p:GetNumBombs())
        p:AddCollectible(CollectibleType.COLLECTIBLE_BOOK_OF_SHADOWS, 6, false)
        p:SetCard(0,Card.CARD_STRENGTH)
    end
    -- 清理当前测试房威胁；这是显式夹具玩法动作，不属于产品观测模块。
    for _,entity in ipairs(Isaac.GetRoomEntities()) do
        if entity:ToNPC() and entity:IsVulnerableEnemy() then entity:Remove() end
    end
    Game():GetRoom():SetClear(true)
end
-- 不同方向的持续输入不能意外跨门并换代；切房由后续显式Game:ChangeRoom场景覆盖。
fixture:AddCallback(ModCallbacks.MC_POST_UPDATE,function()
    if runReady then
        local room=Game():GetRoom()
        room:KeepDoorsClosed()
        -- KeepDoorsClosed只延缓开门，已经打开的门仍需显式关闭。
        for slot=0,7 do
            local door=room:GetDoor(slot)
            if door and door:IsOpen() then door:Close(true) end
        end
    end
end)
fixture:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function() runReady=true end)
fixture:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function() runReady=false end)
local function perform(name)
    local r=roots()
    if name=="seed" then Isaac.ExecuteCommand("seed "..config.seed)
    elseif name=="resources" then resources()
    elseif name=="room" then
        local level=Game():GetLevel()
        local current=level:GetCurrentRoomIndex()
        local target=nil
        for _,direction in ipairs({0,1,2,3}) do
            local door=Game():GetRoom():GetDoor(direction)
            if door and door.TargetRoomIndex>=0 then target=door.TargetRoomIndex; break end
        end
        if not target or target==current then error("fixture no native adjacent room") end
        Game():ChangeRoom(target)
    elseif name=="kill0" then assert(r[0],"host missing"):Kill()
    elseif name=="revive0" then assert(assert(r[0],"host missing"):ReviveCoopGhost(),"host revive returned false")
    elseif name=="kill3" then assert(r[3],"remote3 missing"):Kill()
    elseif name=="revive3" then assert(assert(r[3],"remote3 missing"):ReviveCoopGhost(),"remote3 revive returned false")
    elseif name=="stage" then Isaac.ExecuteCommand("stage 2")
    elseif name=="host_jacob" then Isaac.GetPlayer(0):ChangePlayerType(PlayerType.PLAYER_JACOB)
    elseif name=="remote1_jacob" then assert(r[1],"remote1 missing"):ChangePlayerType(PlayerType.PLAYER_JACOB)
    elseif name=="forms" then
        assert(r[0]):ChangePlayerType(PlayerType.PLAYER_THEFORGOTTEN)
        -- ChangePlayerType不会保证另一形态有健康；原生Ctrl交换会拒绝全空的Soul备份。
        local soul=assert(r[0]:GetSubPlayer(),"Forgotten soul state missing")
        soul:AddSoulHearts(4)
        assert(soul:GetSoulHearts()>0,"Forgotten soul health preparation failed")
        assert(r[1]):ChangePlayerType(PlayerType.PLAYER_THEFORGOTTEN_B)
        assert(r[2]):ChangePlayerType(PlayerType.PLAYER_LAZARUS_B)
        r[2]:AddCollectible(CollectibleType.COLLECTIBLE_BIRTHRIGHT)
        r[3]:ChangePlayerType(PlayerType.PLAYER_ISAAC)
        r[3]:AddCollectible(CollectibleType.COLLECTIBLE_ESAU_JR,1,false)
    elseif name=="flip" then assert(r[2]):UseActiveItem(CollectibleType.COLLECTIBLE_FLIP)
    elseif name=="esau_jr" then
        local player=assert(r[3],"remote3 missing")
        assert(player:GetActiveItem(0)==CollectibleType.COLLECTIBLE_ESAU_JR,"real owned Esau Jr slot0 required")
        player:SetActiveCharge(1,0) -- 夹具只补真实槽充能，使用动作由远端canonical输入触发。
    else error("unknown fixture action "..tostring(name)) end
end
fixture:AddCallback(ModCallbacks.MC_POST_RENDER,function()
    render=render+1
    if Game():GetNumPlayers()==0 then return end
    if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
        action=action+1
        local name=actions[action]
        emit("action_begin",{name=name})
        local ok,err=pcall(perform,name)
        emit(ok and "action_return" or "fixture_error",{name=name,error=not ok and tostring(err) or nil})
    end
    if render%15==0 then
        local ok,value=pcall(snapshot)
        if ok then emit("observation",value) else emit("fixture_error",{error=tostring(value)}) end
    end
end)

-- 使用真实Bomb首次更新确认创建结果；不用查询次数或尚未设置Spawner的Init回调代替。
local seenBombs={}
fixture:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function() seenBombs={} end)
fixture:AddCallback(ModCallbacks.MC_POST_BOMB_UPDATE,function(_,bomb)
    local key=tostring(bomb.Index)..":"..tostring(bomb.InitSeed)
    if seenBombs[key] then return end
    seenBombs[key]=true
    local spawner=bomb.SpawnerEntity and bomb.SpawnerEntity:ToPlayer()
    local ordinal=-1
    local s=status()
    if spawner then
        for i=0,3 do if tonumber(s["participant."..i..".controller"])==spawner.ControllerIndex then ordinal=i end end
    end
    emit("bomb_spawn",{ordinal=ordinal, player_type=spawner and spawner:GetPlayerType() or -1,
        controller=spawner and spawner.ControllerIndex or -1, index=bomb.Index, init_seed=bomb.InitSeed})
end)
