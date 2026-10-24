# Static source/document export only. Never reads a live process or computes hashes.
param([switch]$SkipReferenceSnapshot)
$ErrorActionPreference = 'Stop'
$taskRoot = 'D:\projects\isaac_hook_reserach'
$referenceRoot = 'D:\projects\REPENTOGON'
$outputRoot = Join-Path $taskRoot 'artifacts\cs-t0\tx-01-71-static'
[System.IO.Directory]::CreateDirectory($outputRoot) | Out-Null
function Write-NewText([string]$Path, [string]$Text) {
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write)
    try {
        $bytes = [System.Text.UTF8Encoding]::new($false).GetBytes($Text)
        $stream.Write($bytes, 0, $bytes.Length)
    } finally { $stream.Dispose() }
}
$reference = Join-Path $referenceRoot 'docs\research\repentogon-t0-06-source-static-reference-2026-10-18.md'
if (!$SkipReferenceSnapshot) {
    Write-NewText (Join-Path $outputRoot 'reference-source-static.md') ([System.IO.File]::ReadAllText($reference))
}
$specs = @(
    @{ Root=$taskRoot; File='cs-t0\native\src\loop_runtime.cpp'; Ranges=@(@(30,115),@(174,240),@(253,294),@(382,423),@(623,679),@(790,910),@(1002,1071),@(1108,1207),@(1221,1339),@(1589,1617),@(1642,1705)) },
    @{ Root=$taskRoot; File='cs-t0\native\include\isaac_cs_t0\loop_runtime.hpp'; Ranges=@(@(18,93),@(194,236)) },
    @{ Root=$taskRoot; File='cs-t0\native\src\coop_active_runtime.cpp'; Ranges=@(@(19,105),@(749,791),@(1106,1241),@(1333,1369)) },
    @{ Root=$taskRoot; File='cs-t0\native\include\isaac_cs_t0\hooks.hpp'; Ranges=@(@(10,39),@(97,112)) },
    @{ Root=$taskRoot; File='cs-t0\native\src\hooks.cpp'; Ranges=@(@(174,267),@(520,569),@(578,609),@(628,713)) },
    @{ Root=$referenceRoot; File='repentogon\Patches\EntityManager.cpp'; Ranges=@(@(63,184),@(191,288)) },
    @{ Root=$referenceRoot; File='repentogon\Patches\EntityPlus.cpp'; Ranges=@(@(22,128)) },
    @{ Root=$referenceRoot; File='repentogon\Patches\ASMPatches\ASMGridEntitySpawn.cpp'; Ranges=@(@(83,134)) },
    @{ Root=$referenceRoot; File='repentogon\LuaInterfaces\Entities\LuaEntity.cpp'; Ranges=@(@(852,878)) },
    @{ Root=$referenceRoot; File='repentogon\SaveSyncing\SaveSyncing.cpp'; Ranges=@(@(342,392)) },
    @{ Root=$referenceRoot; File='repentogon\LuaInterfaces\CustomCallbacks.cpp'; Ranges=@(@(1192,1337),@(5417,5464)) },
    @{ Root=$referenceRoot; File='repentogon\SaveStateManagement\EntitySaveStateManagement.cpp'; Ranges=@(@(492,541),@(1496,1539),@(3395,3415)) }
)
$records = foreach ($spec in $specs) {
    $path = Join-Path $spec.Root $spec.File
    $lines = [System.IO.File]::ReadAllLines($path)
    $flatRanges = @($spec.Ranges | ForEach-Object { $_ })
    $excerpts = for ($rangeIndex=0; $rangeIndex -lt $flatRanges.Count; $rangeIndex+=2) {
        $start = [int]$flatRanges[$rangeIndex]
        $end = [Math]::Min([int]$flatRanges[$rangeIndex+1], $lines.Length)
        $body = for ($line=$start; $line -le $end; $line++) { '{0}: {1}' -f $line, $lines[$line-1] }
        [ordered]@{start=$start; end=$end; text=($body -join "`n")}
    }
    [ordered]@{path=$path; evidence_type=$(if($spec.Root -eq $taskRoot){'CURRENT_REPOSITORY_SOURCE'}else{'REFERENCE_VERSION_SOURCE'}); excerpts=@($excerpts)}
}
$document = [ordered]@{tx='tx-01-71'; kind='STATIC_SOURCE_SNAPSHOT'; reference_document=$reference; files=@($records)}
Write-NewText (Join-Path $outputRoot 'source-evidence.json') ($document | ConvertTo-Json -Depth 8)
Write-Output "Created reference-source-static.md and source-evidence.json; $($records.Count) source files."
