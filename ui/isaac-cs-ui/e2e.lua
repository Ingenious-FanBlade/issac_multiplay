-- E2E专用界面：只交换编排纯值，不发游戏输入，不修改世界，不注册玩法回调。
local json = require("json")
local panel = "isaac-cs-acceptance"
local profile = {topology="", run_directory="", resume=false, rerun="", reuse_reason=""}
local pending, submitted, initialized, acceptedStart, lastBusy = nil, nil, false, false, false
local note, frames, feedback = "", 0, ""
local fields = {"topology", "run_directory", "rerun", "reuse_reason"}
local function command(text)
    local ok, result = pcall(Isaac.ExecuteCommand, text)
    local values = {}
    if ok and type(result) == "string" then
        for key, value in result:gmatch("([^=\r\n]+)=([^\r\n]*)") do values[key] = value end
    end
    return values
end
local function hex(text) return (text:gsub(".", function(c) return string.format("%02x", c:byte()) end)) end
local function unhex(text) return (text:gsub("%x%x", function(c) return string.char(tonumber(c, 16)) end)) end
-- 分块上传避免越过统一命令的512字节上限；失败时不提交半份配置/回答。
local function send(action, value)
    local text = json.encode(value)
    if #text > 16384 then feedback="Configuration / answer exceeds 16 KiB."; return false end
    local result = command("cs e2e_ui begin " .. action)
    if result.ok == "1" then
        for at = 1, #text, 192 do
            result = command("cs e2e_ui chunk " .. hex(text:sub(at, at + 191)))
            if result.ok ~= "1" then break end
        end
        if result.ok == "1" then
            -- 先关闭本方两个窗口及固定显示；Hide只改显示标志，实际解除屏蔽由后续Manager确认。
            if action == "reply" then
                ImGui.SetWindowPinned(panel, false);ImGui.SetVisible(panel, false)
                ImGui.SetWindowPinned("isaac-cs-panel", false);ImGui.SetVisible("isaac-cs-panel", false)
                ImGui.Hide()
            end
            result = command("cs e2e_ui commit")
        end
    end
    feedback = result.ok == "1" and "Submitted; waiting for coordinator." or (result.error or "DLL unavailable")
    return result.ok == "1"
end
local function answer(kind)
    if not pending or submitted == pending.id then return end
    local text = note:match("^%s*(.-)%s*$")
    if kind == "observed" or kind == "reviewed" or kind == "retry" or kind == "fail" then
        -- 这里只防止误点；字符数和实际合同继续由原runner独立判定。
        if text == "" then feedback="Enter specific observations / review / repair details first."; return end
        local count=0;for _ in text:gmatch("[^\128-\191]") do count=count+1 end
        local minimum=kind == "retry" and 6 or (kind == "fail" and 1 or 12)
        if count < minimum then feedback="Add a specific description (at least "..minimum.." characters).";return end
        text = kind .. " " .. text
    elseif kind ~= "text" then text = kind end
    if send("reply", {id=pending.id, answer=text}) then
        submitted=pending.id
    end
end
ImGui.CreateMenu("isaac-cs-e2e-menu", "Isaac CS E2E")
ImGui.AddElement("isaac-cs-e2e-menu", "isaac-cs-acceptance-open", ImGuiElement.MenuItem, "tx-04-21 acceptance (E2E)")
ImGui.CreateWindow(panel, "Isaac CS - tx-04-21 acceptance")
ImGui.SetWindowSize(panel, 780, 680)
ImGui.LinkWindowToElement(panel, "isaac-cs-acceptance-open")
ImGui.AddElement(panel, panel.."-summary", ImGuiElement.TextWrapped, "P2 FULL E01-E26; topology phase=p2 stops after P2, p2-p4 continues to P4. Every launch waits for explicit readiness.")
ImGui.AddElement(panel, panel.."-status", ImGuiElement.TextWrapped, "Connecting to the local coordinator mailbox...")
ImGui.AddInputText(panel, panel.."-topology", "Actual topology JSON", function(v) profile.topology=v end, "", "Actual Host + Client for phase=p2; four nodes for p2-p4")
ImGui.AddInputText(panel, panel.."-run_directory", "Run directory (blank = new)", function(v) profile.run_directory=v end)
ImGui.AddCheckbox(panel, panel.."-resume", "Resume this tx", function(v) profile.resume=v end, false)
ImGui.AddInputText(panel, panel.."-rerun", "Affected cases, comma separated", function(v) profile.rerun=v end, "", "p2/E01...,p4/E01...; or p2,p4")
ImGui.AddInputTextMultiline(panel, panel.."-reuse_reason", "Why previous passes are unaffected", function(v) profile.reuse_reason=v end, "", 3)
ImGui.AddButton(panel, panel.."-save", "Save configuration", function() send("save", profile) end)
ImGui.AddCheckbox(panel, panel.."-consent", "Ready for I39 builds/deployment; I will exit this idle game normally after Start", function(v) acceptedStart=v end, false)
ImGui.AddButton(panel, panel.."-start", "Start / resume unified acceptance", function()
    if not acceptedStart then feedback="Confirm readiness above; return to the idle main menu before Start."; return end
    if profile.topology == "" then feedback="Enter an actual topology JSON path first."; return end
    if profile.resume and (profile.run_directory == "" or profile.reuse_reason == "") then feedback="Resume needs the previous run directory and reuse reason."; return end
    if send("start", profile) then
        acceptedStart=false;ImGui.UpdateData(panel.."-consent", ImGuiData.Value, false)
    end
end)
ImGui.AddElement(panel, panel.."-prompt", ImGuiElement.TextWrapped, "No manual action pending.")
ImGui.AddInputTextMultiline(panel, panel.."-note", "Specific observation / review / repair", function(v) note=v end, "", 5)
for _, entry in ipairs({{"ready","Ready for this launch / operation"},{"done","Operation done; hands off"},{"observed","Submit observation"},{"reviewed","Submit final review"},{"retry","Submit repair and retry"},{"text","Submit answer"},{"fail","Missing / inconsistent"},{"stop","Interrupt; retain recovery data"}}) do
    local kind, label = entry[1], entry[2]
    ImGui.AddButton(panel, panel.."-"..kind, label, function() answer(kind) end)
    ImGui.SetVisible(panel.."-"..kind, false)
end
ImGui.AddElement(panel, panel.."-feedback", ImGuiElement.TextWrapped, "")
ImGui.AddElement(panel, panel.."-help", ImGuiElement.TextWrapped, "After Start: exit this bootstrap game from its menu. The controller survives restarts.\nDuring product E24 / closed-game periods, use the separate confirmation window.\nThe panel is local Host only. Refocus the named game before physical input; UI never takes focus for you.")
-- 仅可见窗口轮询；关闭界面后备用窗口接手，不增加Client玩法Lua白名单。
ImGui.AddCallback(panel, ImGuiCallback.Render, function()
    command("cs e2e_ui rendered")
    -- 固定窗口在ImGui关闭后也会收到Render；此时不做JSON/hex/控件轮询。
    if not ImGui.IsVisible() then return end
    frames=frames+1
    if frames ~= 1 and frames % 20 ~= 0 then return end
    local result=command("cs e2e_ui poll")
    if result.ok ~= "1" or result.ready ~= "1" or (result.error and result.error ~= "") then
        ImGui.UpdateText(panel.."-status", result.error or "Preparing local mailbox..."); return
    end
    local ok, state=pcall(json.decode, unhex(result.data or ""))
    if not ok or type(state) ~= "table" then return end
    if submitted and state.error and state.error ~= "" then submitted=nil end
    if (not initialized or state.busy or lastBusy) and type(state.profile) == "table" then
        initialized=true
        for _, key in ipairs(fields) do profile[key]=state.profile[key] or ""; ImGui.UpdateData(panel.."-"..key, ImGuiData.Value, profile[key]) end
        profile.resume=state.profile.resume == true; ImGui.UpdateData(panel.."-resume", ImGuiData.Value, profile.resume)
    end
    lastBusy=state.busy
    local life, progress=state.lifecycle or {}, state.state or {}
    local details={life.message or "Configure the actual topology before starting.", progress.context or "", life.run_directory or ""}
    ImGui.UpdateText(panel.."-status", table.concat(details, "\n"))
    ImGui.UpdateText(panel.."-feedback", (state.error and state.error ~= "") and state.error or feedback)
    for _, key in ipairs(fields) do ImGui.SetVisible(panel.."-"..key, not state.busy) end
    for _, key in ipairs({"resume","save","consent","start"}) do ImGui.SetVisible(panel.."-"..key, not state.busy) end
    if progress.pending and progress.id then
        if not pending or pending.id ~= progress.id then
            note="";submitted=nil;feedback="";ImGui.UpdateData(panel.."-note", ImGuiData.Value, "")
        end
        pending=progress
        ImGui.UpdateText(panel.."-prompt", (progress.instruction or "") .. (submitted == progress.id and "\nAnswer queued." or ""))
    else pending=nil;submitted=nil;ImGui.UpdateText(panel.."-prompt", "No manual action pending.") end
    for _, kind in ipairs({"ready","done","observed","reviewed","retry","text","fail","stop"}) do
        ImGui.SetVisible(panel.."-"..kind, pending ~= nil and submitted ~= pending.id and (kind == pending.kind or kind == "stop" or (kind == "fail" and pending.kind ~= "ready" and pending.kind ~= "done")))
    end
    ImGui.SetVisible(panel.."-note", pending ~= nil and pending.kind ~= "ready" and pending.kind ~= "done")
end)
ImGui.SetVisible(panel, false)
return true
