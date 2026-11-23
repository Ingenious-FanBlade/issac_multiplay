-- 纯界面 mod 不注册 gameplay 回调，状态变更统一通过命令队列。
-- 界面文字全部为中文；命令、字段名、控件 id 和 RegisterMod 名称是内部标识，保持原样。
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
-- ImGui 使用 REPENTOGON 自带的 Unifont，可直接显示中文。
ImGui.CreateMenu("isaac-cs-menu", "以撒联机")
ImGui.CreateWindow(panel, "以撒联机")
ImGui.SetWindowSize(panel, 430, 360)
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
ImGui.AddText(panel, "正在打开大厅……", true, panel .. "-status")
ImGui.AddText(panel, "", true, panel .. "-roster")

-- 只展示能够作为新游戏起始角色的编号，伴生和复活形态由游戏处理。
-- 名称采用游戏中文版译名，堕化角色统一加“堕化”前缀。
local characterNames = {
    [0] = "以撒", "抹大拉", "该隐", "犹大", "???", "夏娃", "参孙", "阿撒泻勒",
    "拉撒路", "伊甸", "游魂", [13] = "莉莉丝", [14] = "店主", [15] = "亚玻伦",
    [16] = "遗骸", [18] = "伯大尼", [19] = "雅各和以扫",
    [21] = "堕化以撒", [22] = "堕化抹大拉", [23] = "堕化该隐",
    [24] = "堕化犹大", [25] = "堕化???", [26] = "堕化夏娃", [27] = "堕化参孙",
    [28] = "堕化阿撒泻勒", [29] = "堕化拉撒路", [30] = "堕化伊甸",
    [31] = "堕化游魂", [32] = "堕化莉莉丝", [33] = "堕化店主",
    [34] = "堕化亚玻伦", [35] = "堕化遗骸", [36] = "堕化伯大尼",
    [37] = "堕化雅各",
}
-- DLL 公布的房主存档解锁位图（第 n 位＝角色 n）；无效或缺失时只当作以撒可选。
local function unlockedMask(values)
    local mask = math.tointeger(tonumber(values.unlocked or "0") or 0) or 0
    return mask | 1
end
local function characterUnlocked(mask, id)
    return characterNames[id] ~= nil and ((mask >> id) & 1) == 1
end
-- 按解锁位图生成下拉框内容：只列出房主已解锁的角色。
-- 返回 下标→角色编号、显示名称、角色编号→下拉框下标（0 起）。
local characterIds, characterLabels, characterIndexes = {}, {}, {}
local function rebuildCharacterList(mask)
    characterIds, characterLabels, characterIndexes = {}, {}, {}
    for id = 0, 37 do
        if characterUnlocked(mask, id) then
            characterIndexes[id] = #characterLabels
            characterIds[#characterIds + 1] = id
            characterLabels[#characterLabels + 1] = characterNames[id]
        end
    end
end

local controls, view, status = {}, nil, {}
local selectingClient, address, port = false, "127.0.0.1", nil
local feedback, pending = nil, nil
-- DLL 返回的错误代码到中文说明；未登记的代码以“错误代码：xxx”原样显示，不隐藏原始值。
local errorText = {
    Bind = "无法打开 UDP 端口，端口可能已被占用。",
    arguments = "请输入 IPv4 地址，以及 1 到 65535 之间的端口。",
    lobby_requires_main_menu = "请先回到主菜单再开房。",
    lobby_requires_save_menu = "请先选择存档，再回到主菜单。",
    client_requires_stable_menu = "请先回到主菜单再加入房间。",
    lobby_wait_for_ready = "需要至少一名客机加入，且所有客机都已准备。",
    live_ipv4 = "请输入有效的 IPv4 地址。",
    live_peer_timeout = "房间没有响应，或连接已超时。",
    live_lobby_required = "房间和客机都必须支持以撒联机大厅。",
    lobby_native_start_failed = "游戏无法开始新的一局。",
    lobby_start_timeout = "游戏未能完成新一局的启动。",
    lobby_capture_prepare_timeout = "房主的世界采集未能就绪，房间已关闭。",
    process_exiting = "游戏正在关闭。",
    host_network_stop_failed = "无法确认网络线程已经停止。",
    host_native_cleanup_failed = "无法安全移除远程玩家或设备。",
    client_ui_unavailable = "联机界面尚未就绪，请稍后重试。",
    client_binary_display_hooks_unavailable = "客机显示钩子未安装，无法加入房间。",
    capabilities_unavailable = "联机功能不可用。",
    lobby_character_locked = "该角色在房主存档中尚未解锁。",
}
local function describeError(code)
    if type(code) == "string" and code:sub(1, 14) == "mod_manifest: " then
        return code:sub(15)
    end
    return errorText[code] or ("错误代码：" .. tostring(code))
end
-- 会话与客机状态名到中文；未登记的状态名原样附在后面。
local stateText = {
    Dormant = "休眠", Negotiating = "协商中", Warmup = "预热中", Active = "运行中",
    Retiring = "正在关闭", Retired = "已关闭", Failed = "失败", Off = "未连接",
    Connecting = "正在连接", AwaitingWorld = "等待房主世界", PreparingWorld = "正在准备世界",
}
local function describeState(name)
    return stateText[name] or tostring(name)
end
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
        feedback = result.error and describeError(result.error) or "联机 DLL 不可用。"
        ImGui.UpdateText(panel .. "-status", feedback)
    end
    nextRefresh = 0
end

-- 本机显示设置：自身角色描边颜色、其他玩家透明度。值由 DLL 保存在 isaac-cs/display.json，
-- 只影响本机绘制。下标与 DLL 的颜色表一致（0 为关闭）；透明度档位 0—10 对应 10%—100%，与原版
-- Online 的“玩家不透明度”同一刻度。
local outlineLabels = { "关闭", "白色", "红色", "橙色", "黄色", "绿色", "青色", "蓝色", "紫色", "粉色" }
local opacityLabels = {}
for level = 0, 10 do opacityLabels[#opacityLabels + 1] = tostring(10 + level * 9) .. "%" end
local display = nil
local function loadDisplay()
    local values = parse(command("cs display_settings"))
    if values.display_version ~= "1" then return nil end
    return { outline = tonumber(values.outline) or 4, opacity = tonumber(values.opacity) or 10 }
end
local function setDisplay(verb, value)
    local values = parse(command("cs " .. verb .. " " .. tostring(value)))
    if values.ok == "1" and values.saved ~= "1" then
        feedback = "显示设置已生效，但未能写入 isaac-cs/display.json。"
    end
    display = loadDisplay() or display
end

local function control(suffix)
    local id = panel .. "-" .. suffix
    controls[#controls + 1] = id
    return id
end

-- REPENTOGON 的控件列表只在窗口开始绘制前重建，点击回调不会移除正在遍历的节点。
-- 选角视图另以解锁位图区分：房主换存档或名单带来新位图时重建下拉框。
local viewMask = nil
local function showControls(nextView)
    local mask = (nextView == "host" or nextView == "guest") and unlockedMask(status) or nil
    if nextView == view and mask == viewMask then return end
    for _, id in ipairs(controls) do ImGui.RemoveElement(id) end
    controls, view, viewMask = {}, nextView, mask
    if mask then rebuildCharacterList(mask) end
    if view == "choose" then
        ImGui.AddButton(panel, control("host"), "创建房间", function() submit("lobby_host", "正在开房……") end)
        ImGui.AddElement(panel, control("same-line"), ImGuiElement.SameLine)
        ImGui.AddButton(panel, control("client"), "加入房间", function()
            selectingClient = true
            feedback = nil
            nextRefresh = 0
        end)
    elseif view == "join" then
        ImGui.AddInputText(panel, control("address"), "房主 IPv4 地址", function(value) address = value end, address)
        ImGui.AddInputInteger(panel, control("port"), "端口", function(value) port = value end, port)
        ImGui.AddButton(panel, control("join"), "加入", function()
            submit("lobby_join " .. address .. " " .. tostring(port), "正在连接……")
        end)
        ImGui.AddElement(panel, control("same-line"), ImGuiElement.SameLine)
        ImGui.AddButton(panel, control("back"), "返回", function()
            selectingClient = false
            feedback = nil
            nextRefresh = 0
        end)
    elseif view == "host" or view == "guest" then
        ImGui.AddCombobox(panel, control("character"), "你的角色", function(index)
            local id = characterIds[index + 1]
            if id then submit("lobby_role " .. id) end
        end, characterLabels, characterIndexes[tonumber(status.desired_character)] or 0)
        if view == "host" then
            ImGui.AddButton(panel, control("start"), "开始游戏", function()
                submit("lobby_start", "正在开始游戏……")
            end)
            ImGui.AddElement(panel, control("same-line"), ImGuiElement.SameLine)
        else
            ImGui.AddButton(panel, control("ready"), "准备", function()
                submit(status.desired_ready == "1" and "lobby_ready 0" or "lobby_ready 1")
            end)
            ImGui.AddElement(panel, control("same-line"), ImGuiElement.SameLine)
        end
    end
    if view == "host" or view == "guest" or view == "session" then
        ImGui.AddButton(panel, control("stop"), view == "host" and "关闭房间" or "退出联机", function()
            submit("stop", "正在退出联机……")
        end)
    end
    -- 显示设置放在操作按钮之后，随视图一起重建，保证总在窗口底部；DLL 不可用时不显示。
    if view ~= "off" then
        display = display or loadDisplay()
        if display then
            ImGui.AddElement(panel, control("display-title"), ImGuiElement.SeparatorText, "显示设置")
            ImGui.AddCombobox(panel, control("outline"), "自身描边颜色", function(index)
                setDisplay("display_outline", index)
            end, outlineLabels, display.outline)
            ImGui.AddCombobox(panel, control("opacity"), "其他玩家不透明度", function(index)
                setDisplay("display_opacity", index)
            end, opacityLabels, display.opacity, true)
        end
    end
end

-- 房主广播唯一名单。客机显示本地请求，直到房主回显选角和准备状态。
local function rosterText()
    if status.count == nil or status.count == "0" then return "" end
    local rows = { "玩家：" .. status.count .. "/4" }
    for ordinal = 0, 3 do
        local prefix = "member." .. ordinal .. "."
        local character = tonumber(status[prefix .. "character"])
        if character then
            local label = ordinal == 0 and "房主" or ("客机 " .. ordinal)
            local own = status.ordinal == tostring(ordinal)
            local ready = status[prefix .. "ready"] == "1"
            local suffix = ""
            if ordinal ~= 0 and status.phase == "Waiting" then
                local waiting = own and (status.desired_character ~= tostring(character) or
                    status.desired_ready ~= status[prefix .. "ready"])
                suffix = waiting and "（正在发送选择……）" or (ready and "（已准备）" or "（未准备）")
            end
            rows[#rows + 1] = label .. (own and "（你）" or "") .. "  " ..
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
        ImGui.UpdateText(panel .. "-status", "联机 DLL 不可用。请安装版本匹配的 DLL 并启用联机功能。")
        showControls("off")
        return
    end
    port = port or tonumber(status.default_port) or 27090
    local role, phase = status.role, status.phase
    if pending and status.lobby_completed == pending.id then
        local error = status.lobby_error
        if error and error ~= "" then feedback = describeError(error) end
        pending = nil
    elseif pending and now >= pending.deadline then
        feedback = "DLL 未确认该请求，请回到主菜单后重试。"
        pending = nil
    end
    if previousPhase == "Waiting" and (phase == "Starting" or phase == "Playing") then
        ImGui.SetVisible(panel, false)
        ImGui.Hide()
    end
    previousPhase = phase

    local message, nextView
    if status.close_error and status.close_error ~= "" then
        message, nextView = "无法关闭联机会话。\n" .. describeError(status.close_error), "off"
    elseif status.retiring == "1" then
        message, nextView = "正在关闭联机会话……", "off"
    elseif phase == "Starting" or phase == "Playing" then
        message = phase == "Starting" and "正在开始游戏……" or "游戏进行中。"
        nextView = "session"
    elseif phase == "Waiting" and role == "Host" then
        message = "房间已打开\nIPv4 地址：" .. status.addresses .. "    UDP 端口：" .. status.port
        if status.can_start ~= "1" then message = message .. "\n正在等待客机准备。" end
        if status.start_menu_ready == "0" then message = message .. "\n开始前请先选择存档。" end
        nextView = "host"
    elseif phase == "Waiting" and role == "Client" and status.ordinal ~= "0" then
        message = "已加入 " .. status.addresses .. ":" .. status.port
        if status.desired_ready == "1" then message = message .. "\n正在等待房主开始……" end
        nextView = "guest"
    elseif role == "Client" then
        message, nextView = "正在连接房主……", "session"
    elseif status.state == "Disabled" then
        message, nextView = "以撒联机已禁用，请在模组配置中启用。", "off"
    elseif status.state ~= "Idle" then
        message, nextView = "会话状态：" .. describeState(status.state), "session"
    else
        message, nextView = selectingClient and "加入房间" or "请选择联机方式。", selectingClient and "join" or "choose"
    end

    local error = status.command_error ~= "" and status.command_error or status.session_error
    local errorMessage = error and error ~= "" and describeError(error)
    if errorMessage and errorMessage ~= feedback then message = message .. "\n" .. errorMessage end
    if feedback then message = message .. "\n" .. feedback end
    if pending then message = pending.text end
    ImGui.UpdateText(panel .. "-status", message)
    ImGui.UpdateText(panel .. "-roster", rosterText())
    showControls(nextView)
    -- 客机当前角色不在房主解锁位图中（例如房主换了存档）时，自动改回以撒并取消准备。
    -- unlocked=0 表示尚未收到房主名单，此时不处理。
    if view == "guest" and not pending and status.unlocked and status.unlocked ~= "0" and
        not characterUnlocked(unlockedMask(status), tonumber(status.desired_character) or 0) then
        submit("lobby_role 0")
    end
    if view == "guest" then
        ImGui.UpdateText(panel .. "-ready", status.desired_ready == "1" and "取消准备" or "准备")
    end
end)
acceptance(parse(command("cs version")))
if e2eBuild then ImGui.SetVisible(panel, true) end
-- 屏幕文字回调：Isaac.RenderText 的 terminus 字体只有 ASCII，无法显示中文，
-- 因此改用游戏自带的中日韩字体 lanapixel（原生资源文件，不是本 mod 自带图形）。
-- 本 mod 只持有这一个 Font 对象，首次绘制时加载一次，之后复用；不创建Sprite/World。
-- Manager Update在Client态冻结菜单导航；此提示保持可见，连接失败也不会显示本地游戏。
local ui = RegisterMod("Isaac CS UI", 1)

-- 中文屏幕字体：只尝试加载一次；失败时记录一次日志，并退回 Isaac.RenderText（中文会显示不全）。
local CJK_FONT_PATH = "font/cjk/lanapixel.fnt"
local cjkFont, cjkFontTried = nil, false
local function screenFont()
    if not cjkFontTried then
        cjkFontTried = true
        local ok, font = pcall(function()
            local loaded = Font()
            loaded:Load(CJK_FONT_PATH)
            return loaded:IsLoaded() and loaded or nil
        end)
        cjkFont = ok and font or nil
        if not cjkFont then
            Isaac.DebugString("[isaac-cs-ui] cjk font unavailable: " .. CJK_FONT_PATH
                .. (ok and "" or (" error=" .. tostring(font))))
        end
    end
    return cjkFont
end
-- 文字宽度：中文字体按 UTF-8 计算，退回路径沿用原 terminus 宽度。
local function textWidth(text)
    local font = screenFont()
    if font then return font:GetStringWidthUTF8(text) end
    return Isaac.GetTextWidth(text)
end
-- 绘制一行屏幕文字（颜色分量 0–1）。
local function drawText(text, x, y, r, g, b, a)
    local font = screenFont()
    if font then
        font:DrawStringUTF8(text, x, y, KColor(r, g, b, a), 0, false)
    else
        Isaac.RenderText(text, x, y, r, g, b, a)
    end
end

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
    local duration = count > 0 and total >= 0 and string.format("%.2f 毫秒", total / count / 1000) or "-- 毫秒"
    if not showRate then return label .. "  " .. duration end
    local frequency = elapsed and string.format("%.1f 次/秒", math.max(count, 0) * 1000 / elapsed) or "-- 次/秒"
    return label .. "  " .. duration .. "   " .. frequency
end
ui:AddCallback(ModCallbacks.MC_POST_RENDER, function()
    local now = Isaac.GetTime()
    if not timingAt or now < timingAt or now - timingAt >= 500 then
        local current = parse(command("cs ui_perf"))
        local elapsed = timingSample and timingSample.role == current.role and timingSample.epoch == current.epoch
            and now > timingAt and now - timingAt or nil
        if current.role == "Host" then
            timingLines = {timingLine("采集", "capture", current, elapsed, false)}
        elseif current.role == "Client" then
            timingLines = {timingLine("世界安装", "world", current, elapsed, true),
                timingLine("身体安装", "body", current, elapsed, true)}
        else
            timingLines = {}
        end
        timingSample, timingAt = current, now
    end
    if #timingLines == 0 then return end
    local center = Isaac.GetScreenWidth() / 2
    local top = Isaac.GetScreenHeight() - 8 - #timingLines * 12
    for i, text in ipairs(timingLines) do
        local x = center - textWidth(text) / 2
        local y = top + (i - 1) * 12
        drawText(text, x + 1, y + 1, 0, 0, 0, 0.85)
        drawText(text, x, y, 1, 1, 1, 1)
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
    drawText("以撒联机 - " .. describeState(status.client_state or "Connecting"), 30, 25, 1, 1, 1, 1)
    local waiting = menuLobby.phase == "Waiting" and "正在等待房主开始。" or "正在等待房主的游戏。"
    drawText(waiting, 30, 42, 1, 1, 1, 1)
    drawText("参与者 " .. (status["client.participant"] or "0") .. " / 名单 " .. (status["client.roster"] or "0"), 30, 59, 1, 1, 1, 1)
    local clientError = status["client.error"]
    drawText("如需停止，请打开以撒联机面板。" .. ((clientError and clientError ~= "") and describeError(clientError) or ""), 30, 76, 1, 1, 1, 1)
end)
