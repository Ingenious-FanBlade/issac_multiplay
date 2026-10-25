# 只读保存本轮引用的官方源码片段；不构建、不运行目标或测试。
$sourceRoot='F:/projects_isaac/REPENTOGON-1.1.3'
$sourceRanges=@(
    @('libzhl/functions/PlayerManager.zhl',1,94),
    @('libzhl/functions/PersistentGameData.zhl',1,10),
    @('libzhl/functions/PersistentGameData.zhl',84,94),
    @('libzhl/functions/PersistentGameData.zhl',126,151),
    @('libzhl/functions/EntityPlayer.zhl',614,625),
    @('libzhl/functions/EntityPlayer.zhl',739,783),
    @('libzhl/functions/ASM.zhl',166,183),
    @('libzhl/functions/Input.zhl',1,45),
    @('repentogon/LuaInterfaces/LuaPlayerManager.cpp',332,360),
    @('repentogon/LuaInterfaces/CustomCallbacks.cpp',883,900),
    @('repentogon/LuaInterfaces/CustomCallbacks.cpp',1737,1740),
    @('repentogon/LuaInterfaces/CustomCallbacks.cpp',3808,3825),
    @('repentogon/Patches/EntityManager.cpp',148,183),
    @('repentogon/Patches/ItemSpoofSystem.cpp',330,369),
    @('repentogon/Patches/XMLPlayerExtras.cpp',1,61),
    @('repentogon/SaveStateManagement/EntitySaveStateManagement.cpp',2680,2739),
    @('repentogon/Patches/ASMPatches/ASMPlayerManager.cpp',1,218),
    @('repentogon/SaveSyncing/InputConfigs.cpp',13,56),
    @('repentogon/Patches/MiscFixes.cpp',260,280),
    @('repentogon/Patches/ModsMenuStuff.cpp',121,141)
)
$sourceOutput=[System.Collections.Generic.List[string]]::new()
$sourceOutput.Add('SOURCE：官方本地源码，仅为静态资料；不代表已加载的 Hook 顺序或运行时观察。')
foreach($sourceRange in $sourceRanges){
    $sourcePath=Join-Path $sourceRoot $sourceRange[0]
    $sourceLines=Get-Content -LiteralPath $sourcePath
    $sourceOutput.Add("`n$sourcePath")
    for($lineIndex=$sourceRange[1]-1;$lineIndex -lt [Math]::Min($sourceRange[2],$sourceLines.Count);$lineIndex++){
        $sourceOutput.Add(('{0}: {1}' -f ($lineIndex+1),$sourceLines[$lineIndex]))
    }
}
$sourceOutput|Set-Content -LiteralPath (Join-Path $PSScriptRoot 'st2-st9-primary-source-evidence.txt') -Encoding utf8
