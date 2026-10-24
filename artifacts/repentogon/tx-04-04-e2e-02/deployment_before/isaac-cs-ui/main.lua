-- Pure UI: no ModCallbacks are registered. All mutations use the command queue.
local panel = "isaac-cs-panel"
local port, frames, lastState = 27090, 0, nil
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
    if lastState == "Hosting" then command("cs stop") end
end)
ImGui.AddElement(panel, "isaac-cs-trust", ImGuiElement.Text, "No authentication. Trusted networks only.\ntx-04-04: placeholder hosting; no network or input takeover.")
local function controls(state)
    local available = state == "Idle" or state == "Hosting"
    ImGui.SetVisible("isaac-cs-port", available)
    ImGui.SetVisible("isaac-cs-host", state == "Idle")
    ImGui.SetVisible("isaac-cs-stop", state == "Hosting")
end
controls("Disabled")
ImGui.AddCallback(panel, ImGuiCallback.Render, function()
    frames = frames + 1
    if frames == 1 then Isaac.DebugString("[isaac-cs-ui] window-render") end
    if frames ~= 1 and frames % 15 ~= 0 then return end
    -- Retry on render: Lua's native binding may return empty while g_Game is nil.
    local version = parse(command("cs version"))
    local status = parse(command("cs status"))
    if version.command_version ~= "1" or status.status_version ~= "1" then
        ImGui.UpdateText("isaac-cs-state", "DLL unavailable, disabled, or menu bridge not available.\nEnter a run to retry.")
        controls("Disabled")
        return
    end
    local state = status.state or "Disabled"
    ImGui.UpdateText("isaac-cs-state", "State: " .. state .. "\nDLL: " .. (version.dll_version or "?") .. "\nREPENTOGON: " .. (version.repentogon_version or "?"))
    local groups = {}
    for _, name in ipairs({"loop", "input", "player", "join", "control", "device"}) do
        local reason = status["reason." .. name] or ""
        groups[#groups + 1] = name .. "=" .. (status["capability." .. name] or "?") .. (reason ~= "" and (" (" .. reason .. ")") or "")
    end
    ImGui.UpdateText("isaac-cs-diagnostic", table.concat(groups, " ") .. "\nHooks: " .. (status.registered or "0") .. "  Calls: " .. (status.calls or "0") .. "\nQueue drops: " .. (status.queue_dropped or "0") .. "\nTrace: " .. (status.trace or "") .. "\nError: " .. (status.last_error or ""))
    controls(state)
    if lastState and lastState ~= state then ImGui.PushNotification("Isaac CS: " .. state, 0) end
    lastState = state
    Isaac.DebugString("[isaac-cs-ui] status state=" .. state)
end)
ImGui.SetVisible(panel, true)
