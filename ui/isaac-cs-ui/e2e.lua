-- E2E专用界面：只交换编排纯值，不发游戏输入，不修改世界，不注册玩法回调。
-- 界面文字为中文；提交给协调器的动作名与回答种类（ready/done/observed 等）是协议值，保持原样。
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
    if #text > 16384 then feedback="配置或回答超过 16 KiB。"; return false end
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
    feedback = result.ok == "1" and "已提交，正在等待协调器。" or (result.error or "DLL 不可用")
    return result.ok == "1"
end
local function answer(kind)
    if not pending or submitted == pending.id then return end
    local text = note:match("^%s*(.-)%s*$")
    if kind == "observed" or kind == "reviewed" or kind == "retry" or kind == "fail" then
        -- 这里只防止误点；字符数和实际合同继续由原runner独立判定。
        if text == "" then feedback="请先填写具体的观察、审阅或修复说明。"; return end
        local count=0;for _ in text:gmatch("[^\128-\191]") do count=count+1 end
        local minimum=kind == "retry" and 6 or (kind == "fail" and 1 or 12)
        if count < minimum then feedback="请补充具体描述（至少 "..minimum.." 个字符）。";return end
        text = kind .. " " .. text
    elseif kind ~= "text" then text = kind end
    if send("reply", {id=pending.id, answer=text}) then
        submitted=pending.id
    end
end
ImGui.CreateMenu("isaac-cs-e2e-menu", "以撒联机测试")
ImGui.AddElement("isaac-cs-e2e-menu", "isaac-cs-acceptance-open", ImGuiElement.MenuItem, "tx-04-21 验收（E2E）")
ImGui.CreateWindow(panel, "以撒联机 - tx-04-21 验收")
ImGui.SetWindowSize(panel, 780, 680)
ImGui.LinkWindowToElement(panel, "isaac-cs-acceptance-open")
ImGui.AddElement(panel, panel.."-summary", ImGuiElement.TextWrapped, "P2 完整执行 E01—E26；拓扑 phase=p2 在 P2 后停止，p2-p4 继续执行 P4。每次启动都等待明确的就绪确认。")
ImGui.AddElement(panel, panel.."-status", ImGuiElement.TextWrapped, "正在连接本地协调器邮箱……")
ImGui.AddInputText(panel, panel.."-topology", "实际拓扑 JSON", function(v) profile.topology=v end, "", "phase=p2 为实际房主＋客机；p2-p4 为四个节点")
ImGui.AddInputText(panel, panel.."-run_directory", "运行目录（留空＝新建）", function(v) profile.run_directory=v end)
ImGui.AddCheckbox(panel, panel.."-resume", "继续本 tx", function(v) profile.resume=v end, false)
ImGui.AddInputText(panel, panel.."-rerun", "受影响用例（逗号分隔）", function(v) profile.rerun=v end, "", "p2/E01...,p4/E01...；或 p2,p4")
ImGui.AddInputTextMultiline(panel, panel.."-reuse_reason", "此前通过项不受影响的原因", function(v) profile.reuse_reason=v end, "", 3)
ImGui.AddButton(panel, panel.."-save", "保存配置", function() send("save", profile) end)
ImGui.AddCheckbox(panel, panel.."-consent", "已准备好 I39 构建／部署；点击开始后我会正常退出这个空闲游戏", function(v) acceptedStart=v end, false)
ImGui.AddButton(panel, panel.."-start", "开始／继续统一验收", function()
    if not acceptedStart then feedback="请先勾选上方的就绪确认，并在开始前回到空闲的主菜单。"; return end
    if profile.topology == "" then feedback="请先填写实际拓扑 JSON 的路径。"; return end
    if profile.resume and (profile.run_directory == "" or profile.reuse_reason == "") then feedback="继续验收需要填写上次的运行目录和沿用原因。"; return end
    if send("start", profile) then
        acceptedStart=false;ImGui.UpdateData(panel.."-consent", ImGuiData.Value, false)
    end
end)
ImGui.AddElement(panel, panel.."-prompt", ImGuiElement.TextWrapped, "当前没有待完成的手动操作。")
ImGui.AddInputTextMultiline(panel, panel.."-note", "具体观察／审阅／修复说明", function(v) note=v end, "", 5)
for _, entry in ipairs({{"ready","本次启动／操作已准备好"},{"done","操作完成，已停止手动操作"},{"observed","提交观察"},{"reviewed","提交最终审阅"},{"retry","提交修复并重试"},{"text","提交回答"},{"fail","缺失／不一致"},{"stop","中断并保留恢复数据"}}) do
    local kind, label = entry[1], entry[2]
    ImGui.AddButton(panel, panel.."-"..kind, label, function() answer(kind) end)
    ImGui.SetVisible(panel.."-"..kind, false)
end
ImGui.AddElement(panel, panel.."-feedback", ImGuiElement.TextWrapped, "")
ImGui.AddElement(panel, panel.."-help", ImGuiElement.TextWrapped, "点击开始后：请从菜单正常退出这个引导用的游戏，协调器会在重启后继续运行。\n产品版 E24 期间或游戏关闭期间，请使用独立的确认窗口。\n本面板只在本地房主上显示。进行实际输入前，请先把指定的游戏窗口切回前台；界面不会替你抢焦点。")
-- 仅可见窗口轮询；关闭界面后备用窗口接手，不增加Client玩法Lua白名单。
ImGui.AddCallback(panel, ImGuiCallback.Render, function()
    command("cs e2e_ui rendered")
    -- 固定窗口在ImGui关闭后也会收到Render；此时不做JSON/hex/控件轮询。
    if not ImGui.IsVisible() then return end
    frames=frames+1
    if frames ~= 1 and frames % 20 ~= 0 then return end
    local result=command("cs e2e_ui poll")
    if result.ok ~= "1" or result.ready ~= "1" or (result.error and result.error ~= "") then
        ImGui.UpdateText(panel.."-status", result.error or "正在准备本地邮箱……"); return
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
    local details={life.message or "开始前请先配置实际拓扑。", progress.context or "", life.run_directory or ""}
    ImGui.UpdateText(panel.."-status", table.concat(details, "\n"))
    ImGui.UpdateText(panel.."-feedback", (state.error and state.error ~= "") and state.error or feedback)
    for _, key in ipairs(fields) do ImGui.SetVisible(panel.."-"..key, not state.busy) end
    for _, key in ipairs({"resume","save","consent","start"}) do ImGui.SetVisible(panel.."-"..key, not state.busy) end
    if progress.pending and progress.id then
        if not pending or pending.id ~= progress.id then
            note="";submitted=nil;feedback="";ImGui.UpdateData(panel.."-note", ImGuiData.Value, "")
        end
        pending=progress
        ImGui.UpdateText(panel.."-prompt", (progress.instruction or "") .. (submitted == progress.id and "\n回答已排队。" or ""))
    else pending=nil;submitted=nil;ImGui.UpdateText(panel.."-prompt", "当前没有待完成的手动操作。") end
    for _, kind in ipairs({"ready","done","observed","reviewed","retry","text","fail","stop"}) do
        ImGui.SetVisible(panel.."-"..kind, pending ~= nil and submitted ~= pending.id and (kind == pending.kind or kind == "stop" or (kind == "fail" and pending.kind ~= "ready" and pending.kind ~= "done")))
    end
    ImGui.SetVisible(panel.."-note", pending ~= nil and pending.kind ~= "ready" and pending.kind ~= "done")
end)
ImGui.SetVisible(panel, false)
return true
