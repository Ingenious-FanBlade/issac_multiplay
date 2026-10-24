param(
    [Parameter(Mandatory=$true)][string]$ArtifactDirectory,
    [Parameter(Mandatory=$true)][string]$Authorization,
    [string]$GameExe='F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe',
    [string]$BuildArtifactDirectory=(Join-Path $PSScriptRoot '..\artifacts\cs-t0\tx-01-58-build-01'),
    [int]$ExpectedProcessId=0,
    [string]$ExpectedCreationTime,
    [string]$Transaction='tx-01-60',
    [string]$Python='python'
)
$ErrorActionPreference='Stop'
$casePath=[IO.Path]::GetFullPath($ArtifactDirectory)
if(Test-Path -LiteralPath $casePath) {throw 'Use a new artifact directory; no automatic retry.'}
if([string]::IsNullOrWhiteSpace($Authorization)) {throw 'Record actual operation authorization.'}
New-Item -ItemType Directory -Path $casePath | Out-Null
$stage='process-and-artifacts';$failure=$null;$controllerExit=$null
try {
    $buildPath=[IO.Path]::GetFullPath($BuildArtifactDirectory)
    $controller=Join-Path $buildPath 'IsaacCsT0.exe'
    $native=Join-Path $buildPath 'build\Native\bin\IsaacCsT0.Native.dll'
    $helper=Join-Path $buildPath 'build\Helper\bin\IsaacCsT0.Helper.exe'
    $binaries=@($controller,$native,$helper) | ForEach-Object {
        $file=Get-Item -LiteralPath $_
        @{path=$file.FullName;length=$file.Length;lastWriteUtc=$file.LastWriteTimeUtc.ToString('o')}
    }
    $games=@(Get-Process -Name 'isaac-ng' -ErrorAction SilentlyContinue)
    if($games.Count -ne 1) {throw "This execution environment observes $($games.Count) isaac-ng processes; no injection. Check process visibility against Isaac MCP before drawing a game-state conclusion."}
    $game=$games[0];$creation=$game.StartTime.ToUniversalTime().ToFileTimeUtc().ToString()
    if($ExpectedProcessId -and ($game.Id -ne $ExpectedProcessId -or $creation -ne $ExpectedCreationTime)) {
        throw 'Observed process does not match the Isaac MCP PID/creation-time identity; no injection.'
    }
    $modules=@($game.Modules | ForEach-Object {$_.ModuleName})
    if($modules -contains 'IsaacCsT0.Native.dll') {throw 'Native DLL already resident; no reinjection.'}
    @{tx=$Transaction;scope='B=1 character, remaining Pill/Card consumer and held10, input/retirement/restoration';authorization=$Authorization;
      pid=$game.Id;creationTime100ns=$creation;gameExe=$GameExe;bCharacter=1;coopIpcVersion=3;
      expectedProcessId=$ExpectedProcessId;expectedCreationTime=$ExpectedCreationTime;
      reusedBinaries=@($binaries);buildPerformed=$false;automaticRetry=$false} |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $casePath 'invocation.json') -Encoding utf8
    @{pid=$game.Id;creationTime100ns=$creation;modules=$modules} |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $casePath 'process-before.json') -Encoding utf8
    New-Item -ItemType Directory -Path (Join-Path $casePath 'sources') | Out-Null
    foreach($source in @('accept-character-pill-e2e.ps1','report-character-pill-e2e.py','testdata\tx-01-60-character-pill-script.json')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $source) -Destination (Join-Path $casePath 'sources')
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'testdata\tx-01-60-character-pill-script.json') -Destination (Join-Path $casePath 'scenario-input.json')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\docs\refactor\cs-t0\t0-05\07-virtual-device\tx-01-60-character-pill-e2e.md') -Destination (Join-Path $casePath 'operation-plan.md')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\docs\refactor\cs-t0\t0-05\07-virtual-device\tx-01-61-live-process-visibility.md') -Destination (Join-Path $casePath 'execution-environment-fix.md')
    Copy-Item -Path (Join-Path $buildPath 'sources\coop_active*') -Destination (Join-Path $casePath 'sources')
    $stage='scene-check'
    Write-Output "$Transaction started, PID=$($game.Id), B=1. Same acceptance pauses for the desktop scene observation."
    $begin=Read-Host 'Enter begin after observing the prepared Offline/only-A scene'
    if($begin -ne 'begin') {throw 'Scene check did not continue; no injection.'}
    $stage='capture'
    & $controller coop-active --game-exe $GameExe --native-dll $native --helper $helper --artifact-dir (Join-Path $casePath 'capture') --script (Join-Path $casePath 'scenario-input.json') --authorization $Authorization --b-character 1 2>&1 |
        Tee-Object -FilePath (Join-Path $casePath 'controller.log')
    $controllerExit=$LASTEXITCODE
    $remaining=Get-Process -Id $game.Id -ErrorAction SilentlyContinue
    $same=$null -ne $remaining -and $remaining.StartTime.ToUniversalTime().ToFileTimeUtc().ToString() -eq $creation
    @{running=$same;pid=$game.Id;creationTime100ns=$creation;nativeResident=($same -and @($remaining.Modules | Where-Object ModuleName -eq 'IsaacCsT0.Native.dll').Count -eq 1)} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'process-after.json') -Encoding utf8
    @{controllerExit=$controllerExit;automaticRetry=$false} | ConvertTo-Json |
        Set-Content -LiteralPath (Join-Path $casePath 'controller-exit.json') -Encoding utf8
    $stage='report'
    if(Test-Path -LiteralPath (Join-Path $casePath 'capture\result.json')) {
        Copy-Item -LiteralPath (Join-Path $casePath 'capture\result.json') -Destination (Join-Path $casePath 'full-feature-result.json')
    }
    & $Python (Join-Path $PSScriptRoot 'report-character-pill-e2e.py') --artifact-dir $casePath --controller-exit $controllerExit --tx $Transaction
    if($LASTEXITCODE -ne 0) {throw 'E2E failed or not fully observed; inspect result.json; no retry.'}
}
catch {
    $failure=$_.Exception.Message
    if(-not (Test-Path -LiteralPath (Join-Path $casePath 'result.json'))) {
        @{tx=$Transaction;status='FAIL';stage=$stage;error=$failure;controllerExit=$controllerExit;automaticRetry=$false} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'result.json') -Encoding utf8
    }
}
if($failure) {throw $failure}
