param(
    [Parameter(Mandatory=$true)][string]$GameExe,
    [Parameter(Mandatory=$true)][string]$ArtifactDirectory,
    [Parameter(Mandatory=$true)][string]$Authorization,
    [string]$BCharacter = 'native',
    [string]$NativeDll = (Join-Path $PSScriptRoot '..\artifacts\cs-t0\tx-01-48-live-01\build\Native\bin\IsaacCsT0.Native.dll'),
    [string]$Helper = (Join-Path $PSScriptRoot '..\artifacts\cs-t0\tx-01-48-live-01\build\Helper\bin\IsaacCsT0.Helper.exe'),
    [string]$Controller = (Join-Path $PSScriptRoot 'target\release\IsaacCsT0.exe'),
    [string]$Python = 'python'
)

$ErrorActionPreference = 'Stop'
$casePath = [IO.Path]::GetFullPath($ArtifactDirectory)
if (Test-Path -LiteralPath $casePath) { throw 'Use a new artifact directory; no automatic retry.' }
if ([string]::IsNullOrWhiteSpace($Authorization)) { throw 'Record the existing isolated-run authorization.' }
New-Item -ItemType Directory -Path $casePath | Out-Null
$fixture = Join-Path $PSScriptRoot 'testdata\tx-01-49-b-death-respawn-script.json'
$capture = Join-Path $casePath 'capture'
$stage = 'input-files'
$failure = $null
$controllerExit = $null
try {
    $binaries = @($NativeDll,$Helper,$Controller) | ForEach-Object {
        $binary = Get-Item -LiteralPath $_ -ErrorAction Stop
        @{path=$binary.FullName; length=$binary.Length; lastWriteUtc=$binary.LastWriteTimeUtc.ToString('o')}
    }
    Copy-Item -LiteralPath $fixture -Destination (Join-Path $casePath 'scenario-input.json')
    $stage = 'fresh-process'
    $games = @(Get-Process -Name 'isaac-ng' -ErrorAction SilentlyContinue)
    if ($games.Count -ne 1) { throw 'Exactly one user-started game process is required; no injection performed.' }
    $game = $games[0]
    $creationTime = $game.StartTime.ToUniversalTime().ToFileTimeUtc().ToString()
    if ($game.Id -eq 35976 -and $creationTime -eq '134366351339428419') {
        throw 'tx-01-48 process still contains the pinned DLL; fully restart the game before this acceptance.'
    }
    @{
        tx='tx-01-49'; feature='tx-01-45'; scope='B death/respawn focused E2E only'
        authorization=$Authorization; gameExe=$GameExe; gamePid=$game.Id; creationTime100ns=$creationTime
        bCharacter=$BCharacter; coopIpcVersion=3
        reusedBinaries=$binaries; buildPerformed=$false; automaticRetry=$false
        trigger='User performs native B death, then respawn after observed death interval'
        fullFeatureResult='Unchanged; this execution does not complete the remaining full E2E cases'
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $casePath 'invocation.json') -Encoding utf8
    $stage = 'runtime'
    & $Controller coop-active --game-exe $GameExe --native-dll $NativeDll --helper $Helper --artifact-dir $capture --script $fixture --authorization $Authorization --b-character $BCharacter 2>&1 |
        Tee-Object -FilePath (Join-Path $casePath 'controller.log')
    $controllerExit = $LASTEXITCODE
    $remaining = Get-Process -Id $game.Id -ErrorAction SilentlyContinue
    $sameProcess = $null -ne $remaining -and $remaining.StartTime.ToUniversalTime().ToFileTimeUtc().ToString() -eq $creationTime
    @{running=$sameProcess;pid=$game.Id;creationTime100ns=$creationTime} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'process-after.json') -Encoding utf8
    $stage = 'capture-only-report'
    & $Python (Join-Path $PSScriptRoot 'report-b-death-respawn-e2e.py') --artifact-dir $casePath --controller-exit $controllerExit
    if ($LASTEXITCODE -ne 0) { throw 'Focused death/respawn acceptance incomplete or failed; inspect result.json; no retry.' }
}
catch {
    $failure = $_.Exception.Message
    if (-not (Test-Path -LiteralPath (Join-Path $casePath 'result.json'))) {
        @{tx='tx-01-49';scope='B death/respawn focused E2E only';status='FAIL';stage=$stage;error=$failure;controllerExit=$controllerExit;automaticRetry=$false} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'result.json') -Encoding utf8
    }
}
if ($failure) { throw $failure }
