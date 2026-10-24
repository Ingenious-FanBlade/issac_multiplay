#requires -Version 7.2
param(
    [Parameter(Mandatory=$true)][string]$GameExe,
    [Parameter(Mandatory=$true)][string]$Controller,
    [Parameter(Mandatory=$true)][string]$NativeDll,
    [Parameter(Mandatory=$true)][string]$Helper,
    [Parameter(Mandatory=$true)][string]$ArtifactDirectory,
    [Parameter(Mandatory=$true)][string]$Authorization,
    [string]$Plan = (Join-Path $PSScriptRoot 'testdata\tx-01-74-world-record-plan.json'),
    [string]$BCharacter = 'native',
    [string]$Python = 'python',
    [string]$ContinueFrom,
    [switch]$SkipFullReplay,
    [switch]$SupplementalCapture,
    [switch]$LiveRemaining,
    [switch]$InstrumentedRemaining,
    [string]$GameLog
)

$ErrorActionPreference = 'Stop'
# One entry, no build.ps1, build/test chain, game launch, gate or digest.
# Existing binaries are copied into the create-new acceptance directory.
# Python owns all human checkpoints, captures and reports in this invocation.
$arguments = @(
    (Join-Path $PSScriptRoot 'run-world-record-e2e.py'),
    '--game-exe', $GameExe, '--controller', $Controller,
    '--native-dll', $NativeDll, '--helper', $Helper,
    '--artifact-dir', $ArtifactDirectory, '--authorization', $Authorization,
    '--plan', $Plan, '--b-character', $BCharacter,
    '--powershell', (Join-Path $PSHOME 'pwsh.exe')
)
if ($ContinueFrom) { $arguments += @('--continue-from', $ContinueFrom) }
if ($SkipFullReplay) { $arguments += '--skip-full-replay' }
if ($SupplementalCapture) { $arguments += '--supplemental-capture' }
if ($LiveRemaining) { $arguments += '--live-remaining' }
if ($InstrumentedRemaining) { $arguments += '--instrumented-remaining' }
if ($GameLog) { $arguments += @('--game-log', $GameLog) }
& $Python @arguments
$acceptanceExit = $LASTEXITCODE
if ($acceptanceExit -eq 2) {
    Write-Output 'E2E invocation finished with uncovered obligations; inspect result.json.'
} elseif ($acceptanceExit -ne 0) {
    Write-Output 'E2E failed or was interrupted; artifacts retained, no automatic retry.'
}
exit $acceptanceExit
