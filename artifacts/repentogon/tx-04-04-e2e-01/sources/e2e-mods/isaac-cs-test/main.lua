-- Render-only fixture; never registers update, spawn or lifecycle callbacks.
local fixture = RegisterMod("Isaac CS E2E fixture", 1)
-- MC_POST_RENDER may only fire during a run. Capture the load/menu binding too,
-- so Q14 still has an observable return value without gameplay callbacks.
local okMenu, menuReply = pcall(Isaac.ExecuteCommand, "cs echo lua-menu")
Isaac.DebugString("[isaac-cs-test] lua-menu phase=load success=" .. tostring(okMenu) .. " response=" .. tostring(menuReply):gsub("\n", "|"))
local menuDone, runDone = true, false
fixture:AddCallback(ModCallbacks.MC_POST_RENDER, function()
    local ok, count = pcall(Isaac.GetNumPlayers)
    local inRun = ok and count > 0
    local token
    if inRun and not runDone then runDone = true; token = "lua-run"
    elseif not inRun and not menuDone then menuDone = true; token = "lua-menu" end
    if token then
        local success, response = pcall(Isaac.ExecuteCommand, "cs echo " .. token)
        Isaac.DebugString("[isaac-cs-test] " .. token .. " success=" .. tostring(success) .. " response=" .. tostring(response):gsub("\n", "|"))
    end
end)
