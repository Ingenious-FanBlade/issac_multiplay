-- 测试夹具只注册渲染回调，菜单与 run 各观察一次命令返回值。
local fixture = RegisterMod("Isaac CS E2E fixture", 1)
local menuDone, runDone, panelShown = false, false, false
-- 界面 mod 的窗口 id；只在 E2E 中由本夹具强制显示。
local PANEL = "isaac-cs-panel"

-- 加载阶段另作记录，不能代替实际菜单时的 Q14 观察。
local function observe(token, phase)
    local success, response = pcall(Isaac.ExecuteCommand, "cs echo " .. token)
    Isaac.DebugString("[isaac-cs-test] " .. token .. " phase=" .. phase
        .. " success=" .. tostring(success)
        .. " response=" .. tostring(response):gsub("\n", "|"))
end

-- 当前玩家数：GetNumPlayers 是 Game 对象的方法，Isaac 命名空间没有这个函数。
local function playerCount()
    local ok, count = pcall(function() return Game():GetNumPlayers() end)
    if ok and type(count) == "number" then return count end
    return 0
end

-- REPENTOGON 在第一帧 ImGui 时按 imgui.ini 恢复窗口可见性，会覆盖界面 mod 加载时的设置。
-- 夹具在每次主菜单渲染时重新显示窗口，让 E2E 不依赖上一次运行留下的 imgui.ini。
local function showPanel()
    local ok, err = pcall(ImGui.SetVisible, PANEL, true)
    if not panelShown then
        panelShown = true
        Isaac.DebugString("[isaac-cs-test] panel-visible phase=menu success=" .. tostring(ok)
            .. (ok and "" or (" error=" .. tostring(err))))
    end
end

observe("lua-menu", "load")

fixture:AddCallback(ModCallbacks.MC_MAIN_MENU_RENDER, function()
    if ImGui then showPanel() end
    if menuDone then return end
    menuDone = true
    observe("lua-menu", "menu")
end)
fixture:AddCallback(ModCallbacks.MC_POST_RENDER, function()
    if runDone then return end
    if playerCount() == 0 then return end
    runDone = true
    observe("lua-run", "run")
end)
