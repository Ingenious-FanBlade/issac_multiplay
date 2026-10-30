-- 纯界面 mod 不注册 gameplay 回调，状态变更统一通过命令队列。
local panel = "isaac-cs-panel"
local port, frames, lastState = 27090, 0, nil
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
if not ImGui then
    Isaac.DebugString("[isaac-cs-ui] unavailable: REPENTOGON ImGui API missing")
    return
end
ImGui.CreateMenu("isaac-cs-menu", "Isaac CS")
ImGui.AddElement("isaac-cs-menu", "isaac-cs-open", ImGuiElement.MenuItem, "Session panel")
ImGui.CreateWindow(panel, "Isaac CS")
ImGui.LinkWindowToElement(panel, "isaac-cs-open")
ImGui.AddElement(panel, "isaac-cs-state", ImGuiElement.Text, "Checking DLL...")
ImGui.AddElement(panel, "isaac-cs-diagnostic", ImGuiElement.Text, "")
ImGui.AddInputInteger(panel, "isaac-cs-port", "Host port", function(value) port = value end, port)
ImGui.AddButton(panel, "isaac-cs-host", "Start host", function()
    if lastState == "Idle" then command("cs host " .. tostring(port)) end
end)
ImGui.AddButton(panel, "isaac-cs-stop", "Stop", function()
    if lastState == "Hosting" or lastState == "Warmup" or lastState == "Active" then command("cs stop") end
end)
ImGui.AddElement(panel, "isaac-cs-peer", ImGuiElement.Text, "Peer: none")
ImGui.AddElement(panel, "isaac-cs-trust", ImGuiElement.Text, "No authentication. Trusted networks only.")
-- 状态只影响界面可见性，host/stop 由 DLL 队列执行。
local function controls(state)
    ImGui.SetVisible("isaac-cs-port", state == "Idle")
    ImGui.SetVisible("isaac-cs-host", state == "Idle")
    ImGui.SetVisible("isaac-cs-stop", state == "Hosting" or state == "Warmup" or state == "Active")
end
controls("Disabled")
ImGui.AddCallback(panel, ImGuiCallback.Render, function()
    frames = frames + 1
    if frames == 1 then Isaac.DebugString("[isaac-cs-ui] window-render") end
    if frames ~= 1 and frames % 60 ~= 0 then return end
    -- 浮层可见时每 60 次渲染查询一次；g_Game 未就绪时保留重试。
    local version = parse(command("cs version"))
    local status = parse(command("cs status"))
    if version.command_version ~= "1" or status.status_version ~= "1" then
        ImGui.UpdateText("isaac-cs-state", "DLL unavailable, disabled, or menu bridge not available.\nEnter a run to retry.")
        controls("Disabled")
        return
    end
    local state = status.state or "Disabled"
    local peers = { "Participants: " .. (status.participants or "0") .. "/4   Bodies: " .. (status.active_bodies or "0") }
    for ordinal = 1, 3 do
        local prefix = "participant." .. ordinal .. "."
        peers[#peers + 1] = "Remote " .. ordinal .. ": " .. (status[prefix .. "peer"] or "Dormant")
            .. "   Join: " .. (status[prefix .. "join"] or "None")
            .. "   Bodies: " .. (status[prefix .. "active_bodies"] or "0")
    end
    ImGui.UpdateText("isaac-cs-peer", table.concat(peers, "\n"))
    ImGui.UpdateText("isaac-cs-state", "State: " .. state .. "\nDLL: " .. (version.dll_version or "?") .. "\nREPENTOGON: " .. (version.repentogon_version or "?"))
    local groups = {}
    for _, name in ipairs({"loop", "input", "player", "join", "control", "device"}) do
        local reason = status["reason." .. name] or ""
        groups[#groups + 1] = name .. "=" .. (status["capability." .. name] or "?") .. (reason ~= "" and (" (" .. reason .. ")") or "")
    end
    ImGui.UpdateText("isaac-cs-diagnostic", table.concat(groups, " ") .. "\nHooks: " .. (status.registered or "0") .. "  Calls: " .. (status.calls or "0") .. "\nQueue drops: " .. (status.queue_dropped or "0") .. "\nTrace: " .. (status.trace or "") .. "\nError: " .. (status.last_error or ""))
    controls(state)
    if lastState ~= state then
        if lastState then ImGui.PushNotification("Isaac CS: " .. state, 0) end
        Isaac.DebugString("[isaac-cs-ui] status state=" .. state)
    end
    lastState = state
end)
ImGui.SetVisible(panel, true)
