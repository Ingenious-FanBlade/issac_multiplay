# 只读摘录 ST3/ST4 主审使用的源码与原行号，不执行目标程序、构建或测试。
$projectRoot = 'F:\projects_isaac\issac_multiplay'
$officialRoot = 'F:\projects_isaac\REPENTOGON-1.1.3'
$sources = @(
    @{ Path = "$projectRoot\mod\tools\gen_binding.py"; Ranges = @('55-89') },
    @{ Path = "$projectRoot\mod\src\game\lifecycle.cpp"; Ranges = @('37-166', '192-218') },
    @{ Path = "$projectRoot\mod\src\game\tick.cpp"; Ranges = @('29-75', '145-154', '223-273', '290-368', '370-438') },
    @{ Path = "$projectRoot\mod\src\game\peer.cpp"; Ranges = @('38-134') },
    @{ Path = "$projectRoot\mod\src\game\join.cpp"; Ranges = @('53-125', '174-204') },
    @{ Path = "$projectRoot\mod\src\game\topology.cpp"; Ranges = @('47-101') },
    @{ Path = "$projectRoot\mod\src\game\context.cpp"; Ranges = @('24-40', '75-115') },
    @{ Path = "$projectRoot\mod\src\hooks\context.cpp"; Ranges = @('10-45') },
    @{ Path = "$projectRoot\mod\src\ops\command.cpp"; Ranges = @('285-299') },
    @{ Path = "$projectRoot\mod\src\game\device_k.cpp"; Ranges = @('83-107', '168-176', '209-271') },
    @{ Path = "$projectRoot\mod\src\game\query.cpp"; Ranges = @('8-39') },
    @{ Path = "$projectRoot\mod\src\hooks\host_patch.cpp"; Ranges = @('12-20', '125-211') },
    @{ Path = "$projectRoot\mod\src\core\session.cpp"; Ranges = @('94-152') },
    @{ Path = "$officialRoot\repentogon\LuaInterfaces\LuaGame.cpp"; Ranges = @('104-115', '418-426') },
    @{ Path = "$officialRoot\libzhl\functions\Game.zhl"; Ranges = @('1-17', '68-69', '284-291') },
    @{ Path = "$officialRoot\libzhl\functions\StageTransition.zhl"; Ranges = @('1-3') },
    @{ Path = "$officialRoot\libzhl\functions\PlayerManager.zhl"; Ranges = @('7-8') },
    @{ Path = "$officialRoot\include\IsaacRepentance.h"; Ranges = @('1631-1635') },
    @{ Path = "$officialRoot\repentogon\Patches\ASMPatches\ASMPlayerManager.cpp"; Ranges = @('42-64', '69-134', '137-194') },
    @{ Path = "$officialRoot\repentogon\Patches\GameStateManagement.cpp"; Ranges = @('219-246', '316-355') },
    @{ Path = "$officialRoot\repentogon\SaveStateManagement\EntitySaveStateManagement.cpp"; Ranges = @('2410-2423', '2631-2680') }
)
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('# tx-04-07 ST3/ST4 主审源码摘录')
$lines.Add('2026-10-25。仅为源码事实；实际 Hook 安装、顺序、条件和结果仍非运行观察。')
foreach ($source in $sources) {
    $sourceLines = Get-Content -LiteralPath $source.Path
    $lines.Add('')
    $lines.Add('## ' + $source.Path)
    foreach ($range in $source.Ranges) {
        $bounds = $range.Split('-')
        $first = [int]$bounds[0]
        $last = [Math]::Min([int]$bounds[1], [int]$sourceLines.Count)
        $lines.Add('')
        $lines.Add('```text')
        for ($line = $first; $line -le $last; $line++) {
            $lines.Add(('{0}: {1}' -f $line, $sourceLines[$line - 1]))
        }
        $lines.Add('```')
    }
}
$lines | Set-Content -LiteralPath "$projectRoot\artifacts\repentogon\tx-04-07-static\st3-st4-review\source-excerpts.md" -Encoding utf8
