param([Parameter(Mandatory)][int]$Run, [int]$Previous = 0, [switch]$ReuseBuild)
$ErrorActionPreference = 'Stop'
$bin = (Resolve-Path -LiteralPath build/harness/i686-pc-windows-msvc/release).Path
$buildDir = (Resolve-Path -LiteralPath build/isaac-dual).Path
$previousRun = {
if ($Previous) {
    $old = Join-Path $buildDir "step15-ec84-r$Previous"
    @"
import sys
sys.path.insert(0,'.scratch')
from step15_sample import query
for port in (27092,27091):
    try: query(port,'cs stop')
    except OSError: pass
"@ | & 'D:/miniconda3/python.exe' -B -
    foreach ($role in @('client1','host')) {
        $oldLaunch = Join-Path $old "$role/launch.json"
        if (!(Test-Path -LiteralPath $oldLaunch)) { continue }
        $oldGame = Get-Content -LiteralPath $oldLaunch -Raw | ConvertFrom-Json
        $game = Get-Process -Id $oldGame.pid -ErrorAction SilentlyContinue
        $root = "F:\projects_isaac\Isaac-for-test\$role\The Binding of Isaac Rebirth\Repentogon\isaac-ng.exe"
        if ($game -and $game.Path -eq $root -and $oldGame.exe -eq $root) {
            if (!$game.CloseMainWindow()) { throw "Could not request normal $role exit" }
            if (!$game.WaitForExit(20000)) { throw "$role did not exit after close" }
        }
    }
    @"
import socket
with socket.create_connection(('127.0.0.1',27111),timeout=3) as peer:
    for _ in [0]:
        peer.sendall(b'reconnect\n')
        peer.recv(2048)
"@ | & 'D:/miniconda3/python.exe' -B -
}
}
$env:Path = 'D:/CMake/bin;C:/Users/114514/.cargo/bin;' + $env:Path
function cmake {
    if ($args[0] -eq '--build') {
        & 'D:/VisualStudio2022/MSBuild/Current/Bin/amd64/MSBuild.exe' (Join-Path $args[1] 'ALL_BUILD.vcxproj') /p:Configuration=Release /p:Platform=Win32 /m:1 /nr:false /p:TrackFileAccess=false /clp:ErrorsOnly /nologo
    } else {
        & 'D:/CMake/bin/cmake.exe' @args
    }
    $global:LASTEXITCODE = $LASTEXITCODE
}
if (!$ReuseBuild) { & ./scripts/build.ps1 -E2E }
& $previousRun
$runDir = Join-Path $buildDir "step15-ec84-r$Run"
$package = & ./scripts/package.ps1 -E2E -OutputDirectory (Join-Path $runDir 'package')
$env:ISAAC_CS_P2_E2E_TOKEN = 'step15-ec84-local'
$env:ISAAC_CS_P2_HOST_FIXTURE = 'step15'
$env:ISAAC_CS_P2_LOAD = 'tears-300'
$env:ISAAC_CS_P2_CLIENT_PORT = '27110'
& (Join-Path $bin 'isaac-dual.exe') (Join-Path $buildDir 'Release/isaac-dual-bootstrap.dll') $runDir $package
if ($LASTEXITCODE -ne 0) { throw "Isolated P2 launch failed: $LASTEXITCODE" }
$launch = Get-Content -LiteralPath (Join-Path $runDir 'host/launch.json') -Raw | ConvertFrom-Json
$game = Get-Process -Id $launch.pid -ErrorAction Stop
$expected = 'F:\projects_isaac\Isaac-for-test\host\The Binding of Isaac Rebirth\Repentogon\isaac-ng.exe'
if ($game.Path -ne $expected -or $launch.exe -ne $expected) { throw 'Unexpected Host F8 target' }
& (Join-Path $bin 'step15-host-f8.exe') $game.Id
if ($LASTEXITCODE -ne 0) { throw 'Host F8 failed' }
Start-Sleep -Seconds 3
foreach ($mode in @('clear','loss','delay')) {
    & 'D:/miniconda3/python.exe' -B .scratch/step15-ec-sample.py 30 $mode ".scratch/step15-ec84-r$Run-$mode.json"
    if ($LASTEXITCODE -ne 0) { throw "P2 sample failed in $mode" }
}
