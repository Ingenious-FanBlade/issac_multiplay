param([ValidateSet('Build','Run','All')][string]$Stage='All',[string]$GameRoot,[string]$TemporaryDirectory)
$ErrorActionPreference='Stop'
$txRepoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# 全目标完成后调用；构建与必要修复续测均属于同一 tx 的整体验收。
if($Stage -ne 'Run') {
    & (Join-Path $PSScriptRoot 'build.ps1') -E2E
    if($LASTEXITCODE) { throw 'tx-04-10 acceptance build failed' }
}
if($Stage -ne 'Build') {
    if(-not $GameRoot) { throw 'Run requires actual game root' }
    if(-not $TemporaryDirectory) { $TemporaryDirectory=Join-Path $txRepoRoot ('out/tx-04-10-entity-temp-'+[guid]::NewGuid().ToString('N')) }
    $txTemporary=[IO.Path]::GetFullPath($TemporaryDirectory)
    $txOutput=[IO.Path]::GetFullPath((Join-Path $txRepoRoot 'out'))+[IO.Path]::DirectorySeparatorChar
    if(-not $txTemporary.StartsWith($txOutput,[StringComparison]::OrdinalIgnoreCase)) { throw 'Temporary directory must stay inside repository out/' }
    # 调用者每次启动前说明配合方式，并取得必要的用户准备确认。
    & (Join-Path $txRepoRoot 'out/repentogon/e2e/harness.exe') e2e tx-04-10 --game $GameRoot --artifact-dir $txTemporary --scenario (Join-Path $txRepoRoot 'tools/harness/scenarios/tx-04-10.json')
    if($LASTEXITCODE) { throw "tx-04-10 acceptance requires diagnosis/continuation: $txTemporary" }
}
