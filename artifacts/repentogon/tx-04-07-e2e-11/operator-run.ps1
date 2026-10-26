# 本次一次性 E2E 编排：按既有 D20 临时禁用下载辅助 mod，结束时恢复原启用状态。
# 仅用于用户确认准备后的单次完整入口，不重试、不回滚存档、不修改功能源码。
$ErrorActionPreference = 'Stop'
$repoRoot = 'F:\projects_isaac\issac_multiplay'
$artifactRoot = Join-Path $repoRoot 'artifacts\repentogon\tx-04-07-e2e-11'
$preparationLog = Join-Path $repoRoot 'artifacts\repentogon\tx-04-07-e2e-11-preparation.jsonl'
$helperRoot = 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\mods\repentogonreleasesbackup_3643104060'
$markerPath = Join-Path $helperRoot 'disable.it'
if ((Test-Path -LiteralPath $artifactRoot) -or (Test-Path -LiteralPath $preparationLog)) {
    throw '本次产物已经存在，不自动重复执行。'
}
$resolvedHelper = (Resolve-Path -LiteralPath $helperRoot).Path
if ($resolvedHelper -ne $helperRoot) { throw '下载辅助 mod 路径不符。' }
$markerExisted = Test-Path -LiteralPath $markerPath
$createdMarker = $false
$runExit = 1
function Write-Preparation([string]$EventName, $Detail) {
    @{ event = $EventName; utc = [DateTime]::UtcNow.ToString('o'); detail = $Detail } |
        ConvertTo-Json -Depth 5 -Compress | Add-Content -LiteralPath $preparationLog -Encoding utf8
}
try {
    Write-Preparation 'before' @{ helper = $helperRoot; marker_existed = $markerExisted }
    if (-not $markerExisted) {
        $markerStream = [IO.File]::Open($markerPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write)
        $createdMarker = $true
        $markerStream.Dispose()
    }
    Write-Preparation 'helper_disabled' @{ marker = $markerPath; created_by_this_run = $createdMarker }
    & (Join-Path $repoRoot 'out\repentogon\harness.exe') e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out $artifactRoot
    $runExit = $LASTEXITCODE
    Write-Preparation 'harness_finished' @{ exit_code = $runExit }
}
finally {
    # 只移除本次创建的精确文件；原先已禁用时保留原文件，不作目录递归操作。
    if ($createdMarker) { Remove-Item -LiteralPath $markerPath }
    Write-Preparation 'helper_restored' @{ marker_existed_before = $markerExisted; marker_exists_after = (Test-Path -LiteralPath $markerPath) }
    if (Test-Path -LiteralPath $artifactRoot) {
        Copy-Item -LiteralPath $preparationLog -Destination (Join-Path $artifactRoot 'operator-preparation.jsonl')
        Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $artifactRoot 'operator-run.ps1')
    }
}
exit $runExit



