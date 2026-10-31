-- 仅E2E回调见证，不创建本地run、不写Mod持久档；Client不得分派这些通用回调。
local mod = RegisterMod("Isaac CS Presentation E2E", 1)
local counts = {}
IsaacCS_PresentationE2E = counts
-- 提前保存native别名，确认正文常驻Hook不会被Lua别名绕过；只涉及本方临时夹具命名空间。
local saveAlias, removeAlias = Isaac.SaveModData, Isaac.RemoveModData
IsaacCS_PresentationTryPersistence = function()
    mod:SaveData("tx17-fixture-must-not-be-written")
    mod:RemoveData()
    saveAlias(mod, "tx17-alias-must-not-be-written")
    removeAlias(mod)
    -- 没有参数时原API会先做类型检查而不能写文件；隔离wrapper必须在original前返回。
    local essm = _CBindings and _CBindings.ESSM
    if essm then
        if essm.SaveData then assert(pcall(essm.SaveData), "ESSM SaveData original entered") end
        if essm.DeleteData then assert(pcall(essm.DeleteData), "ESSM DeleteData original entered") end
    end
    return true
end
-- 退出callback仍可能写Mod存档；证明保护延续到资源退休和Shutdown，不能提前Off放开。
mod:AddCallback(ModCallbacks.MC_PRE_MOD_UNLOAD, function(_, unloaded, shuttingDown)
    if shuttingDown and unloaded == mod and IsaacCS_PresentationTestInternal then
        IsaacCS_PresentationTryPersistence()
        assert(IsaacCS_PresentationTestInternal(), "shutdown ESSM persistence was not protected")
        Isaac.DebugString("[tx17-exit-persist] guarded")
    end
end)
for _, name in ipairs({"MC_PRE_RENDER", "MC_MAIN_MENU_RENDER", "MC_POST_UPDATE", "MC_POST_RENDER", "MC_POST_HUD_RENDER"}) do
    local id = ModCallbacks[name]
    if id then mod:AddCallback(id, function()
        counts[name] = (counts[name] or 0) + 1
        -- 低频可读计数记录；存储在日志而非任何存档或Mod SaveData中。
        if counts[name] == 1 or counts[name] % 60 == 0 then
            Isaac.DebugString("[tx17-callback] " .. name .. "=" .. counts[name])
        end
    end) end
end
