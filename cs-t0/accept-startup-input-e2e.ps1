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
if([string]::IsNullOrWhiteSpace($Authorization)) {throw 'Record the existing isolated-run authorization.'}
$games=@(Get-Process -Name 'isaac-ng' -ErrorAction SilentlyContinue)
if($games.Count -ne 1) {throw 'Exactly one user-started game is required.'}
$game=$games[0]
$creation=$game.StartTime.ToUniversalTime().ToFileTimeUtc().ToString()
if(@($game.Modules | Where-Object ModuleName -eq 'IsaacCsT0.Native.dll').Count) {
    throw 'A resident Native DLL is already loaded. Fully restart the game; do not reinject.'
}
$entryFailure=$null
$controllerExit=$null
try {
    & (Join-Path $PSScriptRoot 'accept-virtual-device-e2e.ps1') -GameExe $GameExe -ArtifactDirectory $casePath -Authorization $Authorization -BCharacter $BCharacter -Transaction 'tx-01-51' -Script (Join-Path $PSScriptRoot 'testdata\tx-01-51-startup-script.json')
    $controllerExit=$LASTEXITCODE
}
catch {$entryFailure=$_.Exception.Message;$controllerExit=$LASTEXITCODE}
if(-not (Test-Path -LiteralPath $casePath)) {throw $entryFailure}
@{tx='tx-01-51';pid=$game.Id;creationTime100ns=$creation;controllerExit=$controllerExit;entryFailure=$entryFailure;automaticRetry=$false} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'input-window-invocation.json') -Encoding utf8
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'testdata\tx-01-51-startup-operations.json') -Destination (Join-Path $casePath 'operation-plan.json')
$remaining=Get-Process -Id $game.Id -ErrorAction SilentlyContinue
$sameProcess=$null -ne $remaining -and $remaining.StartTime.ToUniversalTime().ToFileTimeUtc().ToString() -eq $creation
@{running=$sameProcess;pid=$game.Id;creationTime100ns=$creation} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'process-after.json') -Encoding utf8
if(-not (Test-Path -LiteralPath (Join-Path $casePath 'capture\restoration.json'))) {
    throw "Acceptance did not reach confirmed restoration: $entryFailure"
}
# The full-feature report retains all missing cases; this report covers only the
# startup-input scenario from this same execution.
Move-Item -LiteralPath (Join-Path $casePath 'result.json') -Destination (Join-Path $casePath 'full-feature-result.json')
& $Python (Join-Path $PSScriptRoot 'report-startup-input-e2e.py') --artifact-dir $casePath --controller-exit $controllerExit
if($LASTEXITCODE -ne 0) {throw 'Input repair acceptance incomplete or failed; inspect result.json; no retry.'}
