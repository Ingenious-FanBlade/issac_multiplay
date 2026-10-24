-- 测试夹具只注册渲染回调，菜单与 run 各观察一次命令返回值。
local fixture = RegisterMod("Isaac CS E2E fixture", 1)
local menuDone, runDone = false, false

-- 加载阶段另作记录，不能代替实际菜单时的 Q14 观察。
local function observe(token, phase)
    local success, response = pcall(Isaac.ExecuteCommand, "cs echo " .. token)
    Isaac.DebugString("[isaac-cs-test] " .. token .. " phase=" .. phase
        .. " success=" .. tostring(success)
        .. " response=" .. tostring(response):gsub("\n", "|"))
end
observe("lua-menu", "load")

fixture:AddCallback(ModCallbacks.MC_MAIN_MENU_RENDER, function()
    if menuDone then return end
    menuDone = true
    observe("lua-menu", "menu")
end)
fixture:AddCallback(ModCallbacks.MC_POST_RENDER, function()
    if runDone then return end
    local ok, count = pcall(Isaac.GetNumPlayers)
    if not ok or count == 0 then return end
    runDone = true
    observe("lua-run", "run")
end)
