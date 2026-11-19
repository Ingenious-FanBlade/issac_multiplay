-- 纯界面 mod 不注册 gameplay 回调，状态变更统一通过命令队列。
local panel = "isaac-cs-panel"
-- 安全查询命令桥，DLL 尚未就绪时保留空结果供下一次渲染重试。
local function command(text)
    local ok, result = pcall(Isaac.ExecuteCommand, text)
    return ok and result or ""
end
local function parse(text)
    local values = {}
    for key, value in text:gmatch("([^=\r\n]+)=([^\r\n]*)") do values[key] = value end
    return values
end
-- 只有E2E DLL显式公布能力才加载测试面板；产品包不包含e2e.lua。
local acceptanceLoaded = false
local e2eBuild = false
local function acceptance(version)
    if version.command_version == "1" then e2eBuild = version.e2e_ui_available ~= nil end
    if not acceptanceLoaded and version.e2e_ui_available == "1" then
        acceptanceLoaded = true
        include("e2e")
    end
end
if not ImGui then
    Isaac.DebugString("[isaac-cs-ui] unavailable: REPENTOGON ImGui API missing")
    return
end
-- 顶栏收到点击时立即打开窗口。菜单条目不用额外的子菜单操作。
ImGui.CreateMenu("isaac-cs-menu", "Isaac CS")
ImGui.CreateWindow(panel, "Isaac CS")
ImGui.SetWindowSize(panel, 430, 290)
ImGui.SetWindowFlags(panel, ImGuiWindowFlags.NoDocking)
local nextRefresh = 0
local reopenNextFrame = false
ImGui.AddCallback("isaac-cs-menu", ImGuiCallback.Render, function()
    if reopenNextFrame then
        reopenNextFrame = false
        ImGui.SetVisible(panel, true)
    end
end)
ImGui.AddCallback("isaac-cs-menu", ImGuiCallback.Clicked, function()
    nextRefresh = 0
    -- 已打开的窗口重新出现时会取得焦点，关闭顶栏菜单弹层，免得它挡住按钮。
    if ImGui.GetVisible(panel) then
        ImGui.SetVisible(panel, false)
        reopenNextFrame = true
    else
        ImGui.SetVisible(panel, true)
    end
end)
ImGui.AddText(panel, "Opening lobby...", true, panel .. "-status")
ImGui.AddText(panel, "", true, panel .. "-roster")

-- 只展示能够作为新游戏起始角色的编号，伴生和复活形态由游戏处理。
local characterNames = {
    [0] = "Isaac", "Magdalene", "Cain", "Judas", "???", "Eve", "Samson", "Azazel",
    "Lazarus", "Eden", "The Lost", [13] = "Lilith", [14] = "Keeper", [15] = "Apollyon",
    [16] = "The Forgotten", [18] = "Bethany", [19] = "Jacob & Esau",
    [21] = "Tainted Isaac", [22] = "Tainted Magdalene", [23] = "Tainted Cain",
    [24] = "Tainted Judas", [25] = "Tainted ???", [26] = "Tainted Eve", [27] = "Tainted Samson",
    [28] = "Tainted Azazel", [29] = "Tainted Lazarus", [30] = "Tainted Eden",
    [31] = "Tainted Lost", [32] = "Tainted Lilith", [33] = "Tainted Keeper",
    [34] = "Tainted Apollyon", [35] = "Tainted Forgotten", [36] = "Tainted Bethany",
    [37] = "Tainted Jacob",
}
local characterIds, characterLabels, characterIndexes = {}, {}, {}
for id = 0, 37 do
    if characterNames[id] then
        characterIndexes[id] = #characterLabels
        characterIds[#characterIds + 1] = id
        characterLabels[#characterLabels + 1] = characterNames[id]
    end
end

local controls, view, status = {}, nil, {}
local selectingClient, address, port = false, "127.0.0.1", nil
local feedback, pending = nil, nil
local errorText = {
    Bind = "Could not open the UDP port. It may already be in use.",
    arguments = "Enter an IPv4 address and a port between 1 and 65535.",
    lobby_requires_main_menu = "Return to the main menu before hosting.",
    lobby_requires_save_menu = "Select a save file and return to the main menu.",
    client_requires_stable_menu = "Return to the main menu before joining.",
    lobby_wait_for_ready = "Wait for at least one client and for all clients to be ready.",
    live_ipv4 = "Enter a valid IPv4 address.",
    live_peer_timeout = "The room did not respond or the connection timed out.",
    live_lobby_required = "The room and client must both support the Isaac CS lobby.",
    lobby_native_start_failed = "The game could not start the new run.",
    lobby_start_timeout = "The game did not finish starting the new run.",
    lobby_capture_prepare_timeout = "Host world capture did not become ready. The room was closed.",
    process_exiting = "The game is closing.",
    host_network_stop_failed = "Could not confirm that the network thread stopped.",
    host_native_cleanup_failed = "Could not safely remove a remote player or device.",
}
local function submit(text, message)
    if pending then return end
    local result = parse(command("cs " .. text))
    if result.ok == "1" then
        feedback = nil
        if message and result.queued then
            pending = { id = result.queued, text = message, deadline = Isaac.GetTime() + 3000 }
            ImGui.UpdateText(panel .. "-status", message)
        end
    else
        feedback = errorText[result.error] or result.error or "Isaac CS DLL is unavailable."
        ImGui.UpdateText(panel .. "-status", feedback)
    end
    nextRefresh = 0
end

local function control(suffix)
    local id = panel .. "-" .. suffix
    controls[#controls + 1] = id
    return id
end

-- REPENTOGON 的控件列表只在窗口开始绘制前重建，点击回调不会移除正在遍历的节点。
local function showControls(nextView)
    if nextView == view then return end
    for _, id in ipairs(controls) do ImGui.RemoveElement(id) end
    controls, view = {}, nextView
    if view == "choose" then
        ImGui.AddButton(panel, control("host"), "Host", function() submit("lobby_host", "Opening room...") end)
        ImGui.AddElement(panel, control("same-line"), ImGuiElement.SameLine)
        ImGui.AddButton(panel, control("client"), "Client", function()
            selectingClient = true
            feedback = nil
            nextRefresh = 0
        end)
    elseif view == "join" then
        ImGui.AddInputText(panel, control("address"), "Host IPv4", function(value) address = value end, address)
        ImGui.AddInputInteger(panel, control("port"), "Port", function(value) port = value end, port)
        ImGui.AddButton(panel, control("join"), "Join room", function()
            submit("lobby_join " .. address .. " " .. tostring(port), "Connecting...")
        end)
        ImGui.AddElement(panel, control("same-line"), ImGuiElement.SameLine)
        ImGui.AddButton(panel, control("back"), "Back", function()
            selectingClient = false
            feedback = nil
            nextRefresh = 0
        end)
    elseif view == "host" or view == "guest" then
        ImGui.AddCombobox(panel, control("character"), "Your character", function(index)
            local id = characterIds[index + 1]
            if id then submit("lobby_role " .. id) end
        end, characterLabels, characterIndexes[tonumber(status.desired_character)] or 0)
        if view == "host" then
            ImGui.AddButton(panel, control("start"), "Start game", function()
                submit("lobby_start", "Starting game...")
            end)
            ImGui.AddElement(panel, control("same-line"), ImGuiElement.SameLine)
        else
            ImGui.AddButton(panel, control("ready"), "Ready", function()
                submit(status.desired_ready == "1" and "lobby_ready 0" or "lobby_ready 1")
            end)
            ImGui.AddElement(panel, control("same-line"), ImGuiElement.SameLine)
        end
    end
    if view == "host" or view == "guest" or view == "session" then
        ImGui.AddButton(panel, control("stop"), view == "host" and "Close room" or "Leave session", function()
            submit("stop", "Leaving session...")
        end)
    end
end

-- 房主广播唯一名单。客机显示本地请求，直到房主回显选角和准备状态。
local function rosterText()
    if status.count == nil or status.count == "0" then return "" end
    local rows = { "Players: " .. status.count .. "/4" }
    for ordinal = 0, 3 do
        local prefix = "member." .. ordinal .. "."
        local character = tonumber(status[prefix .. "character"])
        if character then
            local label = ordinal == 0 and "Host" or ("Client " .. ordinal)
            local own = status.ordinal == tostring(ordinal)
            local ready = status[prefix .. "ready"] == "1"
            local suffix = ""
            if ordinal ~= 0 and status.phase == "Waiting" then
                local waiting = own and (status.desired_character ~= tostring(character) or
                    status.desired_ready ~= status[prefix .. "ready"])
                suffix = waiting and " (sending selection...)" or (ready and " (ready)" or " (not ready)")
            end
            rows[#rows + 1] = label .. (own and " (you)" or "") .. "  " ..
                (characterNames[character] or tostring(character)) .. suffix
        end
    end
    return table.concat(rows, "\n")
end

local previousPhase
ImGui.AddCallback(panel, ImGuiCallback.Render, function()
    if e2eBuild then command("cs e2e_ui rendered") end
    local now = Isaac.GetTime()
    if now < nextRefresh then return end
    nextRefresh = now + 120
    status = parse(command("cs lobby_status"))
    if status.lobby_version ~= "1" then
        ImGui.UpdateText(panel .. "-status", "Isaac CS DLL is unavailable. Install the matching DLL and enable C/S.")
        showControls("off")
        return
    end
    port = port or tonumber(status.default_port) or 27090
    local role, phase = status.role, status.phase
    if pending and status.lobby_completed == pending.id then
        local error = status.lobby_error
        if error and error ~= "" then feedback = errorText[error] or error end
        pending = nil
    elseif pending and now >= pending.deadline then
        feedback = "The DLL did not confirm the request. Return to the main menu and retry."
        pending = nil
    end
    if previousPhase == "Waiting" and (phase == "Starting" or phase == "Playing") then
        ImGui.SetVisible(panel, false)
        ImGui.Hide()
    end
    previousPhase = phase

    local message, nextView
    if status.close_error and status.close_error ~= "" then
        message, nextView = "Could not close session.\n" .. (errorText[status.close_error] or status.close_error), "off"
    elseif status.retiring == "1" then
        message, nextView = "Closing session...", "off"
    elseif phase == "Starting" or phase == "Playing" then
        message = phase == "Starting" and "Starting game..." or "Game in progress."
        nextView = "session"
    elseif phase == "Waiting" and role == "Host" then
        message = "Room opened successfully\nIPv4: " .. status.addresses .. "    UDP port: " .. status.port
        if status.can_start ~= "1" then message = message .. "\nWaiting for clients to ready up." end
        if status.start_menu_ready == "0" then message = message .. "\nSelect a save file before starting." end
        nextView = "host"
    elseif phase == "Waiting" and role == "Client" and status.ordinal ~= "0" then
        message = "Joined " .. status.addresses .. ":" .. status.port
        if status.desired_ready == "1" then message = message .. "\nWaiting for host to start..." end
        nextView = "guest"
    elseif role == "Client" then
        message, nextView = "Connecting to host...", "session"
    elseif status.state == "Disabled" then
        message, nextView = "Isaac CS is disabled. Enable it in the mod configuration.", "off"
    elseif status.state ~= "Idle" then
        message, nextView = "Session: " .. status.state, "session"
    else
        message, nextView = selectingClient and "Join a room" or "Choose how to connect.", selectingClient and "join" or "choose"
    end

    local error = status.command_error ~= "" and status.command_error or status.session_error
    local errorMessage = error and error ~= "" and (errorText[error] or error)
    if errorMessage and errorMessage ~= feedback then message = message .. "\n" .. errorMessage end
    if feedback then message = message .. "\n" .. feedback end
    if pending then message = pending.text end
    ImGui.UpdateText(panel .. "-status", message)
    ImGui.UpdateText(panel .. "-roster", rosterText())
    showControls(nextView)
    if view == "guest" then
        ImGui.UpdateText(panel .. "-ready", status.desired_ready == "1" and "Cancel ready" or "Ready")
    end
end)
acceptance(parse(command("cs version")))
if e2eBuild then ImGui.SetVisible(panel, true) end
-- 纯菜单绘制回调：使用原生已有字体，不创建Sprite/World或自有图形/Lua借用资源。
-- Manager Update在Client态冻结菜单导航；此提示保持可见，连接失败也不会显示本地游戏。
local ui = RegisterMod("Isaac CS UI", 1)

-- 每半秒读取一次纯值累计量。差分只在同一会话内计算，等待新安装时频率回落到零。
local timingSample, timingAt, timingLines = nil, nil, {}
local function timingLine(label, key, current, elapsed, showRate)
    local count, total = 0, 0
    if elapsed then
        local countKey, totalKey = key .. "_n", key .. "_us"
        local currentCount, previousCount = tonumber(current[countKey]), tonumber(timingSample[countKey])
        local currentTotal, previousTotal = tonumber(current[totalKey]), tonumber(timingSample[totalKey])
        if currentCount and previousCount and currentTotal and previousTotal then
            count, total = currentCount - previousCount, currentTotal - previousTotal
        else
            elapsed = nil
        end
    end
    local duration = count > 0 and total >= 0 and string.format("%.2f ms", total / count / 1000) or "-- ms"
    if not showRate then return label .. "  " .. duration end
    local frequency = elapsed and string.format("%.1f Hz", math.max(count, 0) * 1000 / elapsed) or "-- Hz"
    return label .. "  " .. duration .. "   " .. frequency
end
ui:AddCallback(ModCallbacks.MC_POST_RENDER, function()
    local now = Isaac.GetTime()
    if not timingAt or now < timingAt or now - timingAt >= 500 then
        local current = parse(command("cs ui_perf"))
        local elapsed = timingSample and timingSample.role == current.role and timingSample.epoch == current.epoch
            and now > timingAt and now - timingAt or nil
        if current.role == "Host" then
            timingLines = {timingLine("Capture", "capture", current, elapsed, false)}
        elseif current.role == "Client" then
            timingLines = {timingLine("World install", "world", current, elapsed, true),
                timingLine("Body install", "body", current, elapsed, true)}
        else
            timingLines = {}
        end
        timingSample, timingAt = current, now
    end
    if #timingLines == 0 then return end
    local center = Isaac.GetScreenWidth() / 2
    local top = Isaac.GetScreenHeight() - 8 - #timingLines * 12
    for i, text in ipairs(timingLines) do
        local x = center - Isaac.GetTextWidth(text) / 2
        local y = top + (i - 1) * 12
        Isaac.RenderText(text, x + 1, y + 1, 0, 0, 0, 0.85)
        Isaac.RenderText(text, x, y, 1, 1, 1, 1)
    end
end)

local menuFrames, menuStatus, menuLobby = 0, {}, {}
ui:AddCallback(ModCallbacks.MC_MAIN_MENU_RENDER, function()
    menuFrames = menuFrames + 1
    if menuFrames == 1 or menuFrames % 15 == 0 then
        menuStatus = parse(command("cs ui_status"))
        acceptance(parse(command("cs version")))
        if menuStatus.role == "Client" then menuLobby = parse(command("cs lobby_status")) end
    end
    local status = menuStatus
    if status.role ~= "Client" then return end
    Isaac.RenderText("Isaac CS - " .. (status.client_state or "Connecting"), 30, 25, 1, 1, 1, 1)
    local waiting = menuLobby.phase == "Waiting" and "Waiting for the host to start." or "Waiting for the host's game."
    Isaac.RenderText(waiting, 30, 42, 1, 1, 1, 1)
    Isaac.RenderText("Participant " .. (status["client.participant"] or "0") .. " / roster " .. (status["client.roster"] or "0"), 30, 59, 1, 1, 1, 1)
    Isaac.RenderText("Open Isaac CS panel to stop. " .. (status["client.error"] or ""), 30, 76, 1, 1, 1, 1)
end)
