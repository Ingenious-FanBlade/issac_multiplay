param([ValidateSet('Build','Run','All')][string]$Stage='All',[string]$GameRoot,[string]$TemporaryDirectory)
$ErrorActionPreference='Stop'
$txRepoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# 全目标完成后调用；构建与必要修复续测均属于同一 tx 的整体验收。
if($Stage -ne 'Run') {
    & (Join-Path $PSScriptRoot 'build.ps1')
    if($LASTEXITCODE) { throw 'tx-04-11 acceptance build failed' }
}
if($Stage -ne 'Build') {
    if(-not $GameRoot) { throw 'Run requires actual game root' }
    if(-not $TemporaryDirectory) { $TemporaryDirectory=Join-Path $txRepoRoot ('out/tx-04-11-grid-temp-'+[guid]::NewGuid().ToString('N')) }
    $txTemporary=[IO.Path]::GetFullPath($TemporaryDirectory)
    $txOutput=[IO.Path]::GetFullPath((Join-Path $txRepoRoot 'out'))+[IO.Path]::DirectorySeparatorChar
    if(-not $txTemporary.StartsWith($txOutput,[StringComparison]::OrdinalIgnoreCase)) { throw 'Temporary directory must stay inside repository out/' }
    # 用户已授权测试直接启动；调用者每次启动前提示停止手动操作。
    & (Join-Path $txRepoRoot 'out/repentogon/harness.exe') e2e tx-04-11 --game $GameRoot --artifact-dir $txTemporary --scenario (Join-Path $txRepoRoot 'tools/harness/scenarios/tx-04-11.json')
    if($LASTEXITCODE) { throw "tx-04-11 acceptance requires diagnosis/continuation: $txTemporary" }
}
