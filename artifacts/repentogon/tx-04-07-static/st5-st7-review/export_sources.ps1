# 只读摘录本次主审引用的源码，保留原行号；不执行构建、测试或目标程序。
$projectRoot = 'F:\projects_isaac\issac_multiplay'
$officialRoot = 'F:\projects_isaac\REPENTOGON-1.1.3'
$excerptSources = @(
    @{ Path = "$projectRoot\mod\src\game\consumers.cpp"; Ranges = @('15-76') },
    @{ Path = "$projectRoot\mod\src\game\context.cpp"; Ranges = @('24-40') },
    @{ Path = "$projectRoot\mod\src\game\persistence.cpp"; Ranges = @('1-54') },
    @{ Path = "$projectRoot\mod\tools\gen_binding.py"; Ranges = @('51-81') },
    @{ Path = "$projectRoot\tools\harness\src\tx06\evidence.rs"; Ranges = @('611-654') },
    @{ Path = "$officialRoot\include\IsaacRepentance.h"; Ranges = @('854-864', '1700-1705', '5983-5995', '8375-8385') },
    @{ Path = "$officialRoot\libzhl\functions\Game.zhl"; Ranges = @('1-17') },
    @{ Path = "$officialRoot\libzhl\functions\EntityPlayer.zhl"; Ranges = @('88-89', '296-297', '747-747', '815-815') },
    @{ Path = "$officialRoot\repentogon\LuaInterfaces\CustomCallbacks.cpp"; Ranges = @('521-536') },
    @{ Path = "$officialRoot\repentogon\Patches\ASMPatches\ASMPlayerManager.cpp"; Ranges = @('20-40') },
    @{ Path = "$officialRoot\repentogon\Patches\AchievementsStuff.cpp"; Ranges = @('63-78', '105-136', '248-257') },
    @{ Path = "$officialRoot\repentogon\Patches\GameStateManagement.cpp"; Ranges = @('24-48', '181-220', '261-319', '390-423') },
    @{ Path = "$officialRoot\repentogon\SaveStateManagement\EntitySaveStateManagement.cpp"; Ranges = @('1942-1945', '2193-2242', '2307-2327') },
    @{ Path = "$officialRoot\repentogon\SaveStateManagement\EntitySaveStateEx.cpp"; Ranges = @('200-255', '353-364') },
    @{ Path = "$officialRoot\repentogon\Patches\ItemPoolManager.cpp"; Ranges = @('886-889', '1476-1500', '1520-1535') },
    @{ Path = "$officialRoot\repentogon\VirtualRoomConfig\VirtualRoomSetManager.cpp"; Ranges = @('948-1000', '1167-1184', '1220-1251') }
)
$excerptLines = [System.Collections.Generic.List[string]]::new()
$excerptLines.Add('# ST5–ST7 主审源码摘录')
$excerptLines.Add('2026-10-25；只读摘录，不代表运行时 Hook 排列或实际写入。')
foreach ($source in $excerptSources) {
    $sourceLines = Get-Content -LiteralPath $source.Path
    $excerptLines.Add('')
    $excerptLines.Add('## ' + $source.Path)
    foreach ($range in $source.Ranges) {
        $rangeBounds = $range.Split('-')
        $firstLine = [int]$rangeBounds[0]
        $lastLine = [Math]::Min([int]$rangeBounds[1], [int]$sourceLines.Count)
        $excerptLines.Add('')
        $excerptLines.Add('```text')
        for ($line = $firstLine; $line -le $lastLine; $line++) {
            $excerptLines.Add(('{0}: {1}' -f $line, $sourceLines[$line - 1]))
        }
        $excerptLines.Add('```')
    }
}
$excerptLines | Set-Content -LiteralPath "$projectRoot\artifacts\repentogon\tx-04-07-static\st5-st7-review\source-excerpts.md" -Encoding utf8
