param(
    [ValidateSet('Build','Run','All')][string]$Stage='All',
    [ValidateSet('Diagnostic','Production')][string]$Profile='Diagnostic',
    [string]$GameRoot,
    [string]$TemporaryDirectory
)
$ErrorActionPreference='Stop'
$txRepoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# 本入口只在tx-04-09完整实现后使用；Build与Run属于同一整体验收，可在修复后续测。
if($Stage -ne 'Run') {
    & (Join-Path $PSScriptRoot 'build.ps1')
    if($LASTEXITCODE) { throw 'tx-04-09 acceptance build failed' }
}
if($Stage -ne 'Build') {
    if(-not $GameRoot) { throw 'Run requires the actual game root' }
    if(-not $TemporaryDirectory) {
        $TemporaryDirectory=Join-Path $txRepoRoot ('out/tx-04-09-four-participant-temp-'+[guid]::NewGuid().ToString('N'))
    }
    $txTemporary=[IO.Path]::GetFullPath($TemporaryDirectory)
    $txOutput=[IO.Path]::GetFullPath((Join-Path $txRepoRoot 'out'))+[IO.Path]::DirectorySeparatorChar
    if(-not $txTemporary.StartsWith($txOutput,[StringComparison]::OrdinalIgnoreCase)) { throw 'Temporary directory must be inside repository out/' }
    # 启动前的用户操作提示和必要的准备确认由调用者完成，不从历史授权推导本次确认。
    & (Join-Path $txRepoRoot 'out/repentogon/harness.exe') e2e tx-04-09 --game $GameRoot --artifact-dir $txTemporary --scenario (Join-Path $txRepoRoot 'tools/harness/scenarios/tx-04-09.json') --profile ($Profile.ToLowerInvariant())
    if($LASTEXITCODE) { throw "tx-04-09 acceptance requires diagnosis/continuation: $txTemporary" }
}
