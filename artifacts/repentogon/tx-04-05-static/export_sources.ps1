param([string]$SdkRoot='F:/projects_isaac/REPENTOGON-1.1.3')
# 仅复制本轮静态审阅使用的源码片段；不构建、不运行游戏或测试，也不计算哈希。
$ErrorActionPreference='Stop'
$repoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$lines=[Collections.Generic.List[string]]::new()
# 保留来源和原始行号，让审阅者能逐句对照官方源码与原型语义。
function Add-Excerpt([string]$Path,[int]$First,[int]$Last) {
    $lines.Add("`nSOURCE: $Path [$First..$Last]")
    $source=Get-Content -LiteralPath $Path
    for($row=$First;$row -le $Last -and $row -le $source.Count;$row++) {
        $lines.Add(('{0}: {1}' -f $row,$source[$row-1]))
    }
}
Add-Excerpt (Join-Path $SdkRoot 'repentogon/LuaInterfaces/Menus/LuaMenuManager.cpp') 191 216
Add-Excerpt (Join-Path $SdkRoot 'repentogon/Patches/ModsMenuStuff.cpp') 130 136
Add-Excerpt (Join-Path $SdkRoot 'repentogon/ImGuiFeatures/ImGui.cpp') 306 346
Add-Excerpt (Join-Path $SdkRoot 'repentogon/LuaInterfaces/CustomCallbacks.cpp') 492 534
Add-Excerpt (Join-Path $SdkRoot 'repentogon/Patches/ASMPatches/ASMCallbacks.cpp') 737 770
Add-Excerpt (Join-Path $SdkRoot 'libzhl/HookSystem.cpp') 397 425
Add-Excerpt (Join-Path $SdkRoot 'libzhl/HookSystem.cpp') 576 590
Add-Excerpt (Join-Path $SdkRoot 'libzhl/HookSystem.cpp') 649 675
Add-Excerpt (Join-Path $SdkRoot 'libzhl/functions/EntityPlayer.zhl') 773 783
Add-Excerpt (Join-Path $SdkRoot 'libzhl/functions/EntityPlayer.zhl') 853 859
Add-Excerpt (Join-Path $SdkRoot 'libzhl/functions/Entity.zhl') 232 239
Add-Excerpt (Join-Path $SdkRoot 'libzhl/functions/Manager.zhl') 89 96
Add-Excerpt (Join-Path $SdkRoot 'libzhl/functions/Manager.zhl') 109 114
Add-Excerpt (Join-Path $SdkRoot 'libzhl/functions/OptionsConfig.zhl') 40 46
Add-Excerpt (Join-Path $SdkRoot 'libzhl/functions/Game.zhl') 285 290
Add-Excerpt (Join-Path $SdkRoot 'libzhl/functions/RoomTransition.zhl') 13 30
Add-Excerpt (Join-Path $repoRoot 'cs-t0/native/src/coop_input.cpp') 131 154
Add-Excerpt (Join-Path $repoRoot 'cs-t0/native/src/coop_input.cpp') 430 463
Add-Excerpt (Join-Path $repoRoot 'cs-t0/native/src/coop_active_runtime.cpp') 729 754
[IO.File]::WriteAllLines((Join-Path $PSScriptRoot 'source-evidence.txt'),$lines,[Text.UTF8Encoding]::new($false))
Write-Output '静态源码片段已保存到 source-evidence.txt。'
