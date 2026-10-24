param(
    [Parameter(Mandatory=$true)][string]$GameExe,
    [Parameter(Mandatory=$true)][string]$ArtifactDirectory,
    [Parameter(Mandatory=$true)][string]$Authorization,
    [string]$BCharacter='native',
    [string]$Python='python'
)
$ErrorActionPreference='Stop'
$casePath=[IO.Path]::GetFullPath($ArtifactDirectory)
if(Test-Path -LiteralPath $casePath) {throw 'Use a new artifact directory; no automatic retry.'}
if([string]::IsNullOrWhiteSpace($Authorization)) {throw 'Record the actual isolated-run authorization.'}
$games=@(Get-Process -Name 'isaac-ng' -ErrorAction SilentlyContinue)
if($games.Count -ne 1) {throw 'Exactly one user-started game is required.'}
$game=$games[0]
$creation=$game.StartTime.ToUniversalTime().ToFileTimeUtc().ToString()
if(@($game.Modules | Where-Object ModuleName -eq 'IsaacCsT0.Native.dll').Count) {
    throw 'A resident Native DLL is loaded. Fully restart; do not reinject.'
}
$entryFailure=$null
$controllerExit=0
try {
    & (Join-Path $PSScriptRoot 'accept-virtual-device-e2e.ps1') -GameExe $GameExe -ArtifactDirectory $casePath -Authorization $Authorization -BCharacter $BCharacter -Transaction 'tx-01-55' -Script (Join-Path $PSScriptRoot 'testdata\tx-01-55-loading-query-script.json')
    $controllerExit=$LASTEXITCODE
}
catch {$entryFailure=$_.Exception.Message;$controllerExit=$LASTEXITCODE}
if(-not (Test-Path -LiteralPath $casePath)) {throw $entryFailure}
@{tx='tx-01-55';pid=$game.Id;creationTime100ns=$creation;controllerExit=$controllerExit;entryFailure=$entryFailure;automaticRetry=$false} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'loading-query-invocation.json') -Encoding utf8
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'testdata\tx-01-55-loading-query-operations.json') -Destination (Join-Path $casePath 'operation-plan.json')
New-Item -ItemType Directory -Path (Join-Path $casePath 'sources') | Out-Null
foreach($source in @('native\src\coop_active_runtime.cpp','native\include\isaac_cs_t0\coop_active_ipc.hpp','src\coop_active.rs','report-loading-query-e2e.py','accept-loading-query-e2e.ps1')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot $source) -Destination (Join-Path $casePath 'sources')
}
$remaining=Get-Process -Id $game.Id -ErrorAction SilentlyContinue
$sameProcess=$null -ne $remaining -and $remaining.StartTime.ToUniversalTime().ToFileTimeUtc().ToString() -eq $creation
@{running=$sameProcess;pid=$game.Id;creationTime100ns=$creation} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'process-after.json') -Encoding utf8
if(-not (Test-Path -LiteralPath (Join-Path $casePath 'capture\restoration.json'))) {
    throw "Acceptance did not reach confirmed restoration: $entryFailure"
}
Move-Item -LiteralPath (Join-Path $casePath 'result.json') -Destination (Join-Path $casePath 'full-feature-result.json')
& $Python (Join-Path $PSScriptRoot 'report-loading-query-e2e.py') --artifact-dir $casePath --controller-exit $controllerExit
if($LASTEXITCODE -ne 0) {throw 'Loading/query acceptance incomplete or failed; inspect result.json; no retry.'}
