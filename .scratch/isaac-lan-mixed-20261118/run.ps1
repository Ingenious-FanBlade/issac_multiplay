param([Parameter(Mandatory)][int]$Attempt, [switch]$Prepare, [switch]$WslLaunch)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$run = Join-Path $root "run-$Attempt"
$release = Join-Path $root 'release/Isaac-LAN-v0.7.2-windows-x86'
$source = 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth'
$reference = 'F:\projects_isaac\isaac-lan'
$port = 31364
$roles = @('host','client')
$profile = 'profile/Documents/My Games/Binding of Isaac Repentance+'
if ($Prepare) {
    New-Item -ItemType Directory -Path $run | Out-Null
    $window = Join-Path $run 'window.txt'
    $paths = @()
    foreach ($role in $roles) {
        $lab = Join-Path $run $role
        $game = Join-Path $lab 'game'
        $paths += Join-Path $game 'isaac-ng.exe'
        $save = Join-Path $lab $profile
        New-Item -ItemType Directory -Path (Join-Path $game 'isaac-lan'),(Join-Path $game 'mods'),$save | Out-Null
        # 按准备工具复制原版 J460，只在新建隔离副本中部署 LAN。
        Get-ChildItem -LiteralPath $source -File | Where-Object { $_.Extension -eq '.dll' -or $_.Name -in 'isaac-ng.exe','curl-ca-bundle.crt' } | Copy-Item -Destination $game
        Copy-Item -LiteralPath (Join-Path $source 'resources') -Destination $game -Recurse
        foreach ($name in @('isaac_lan_probe.dll','winmm.dll')) { Copy-Item -LiteralPath (Join-Path $release $name) -Destination $game }
        foreach ($mod in Get-ChildItem -LiteralPath (Join-Path $source 'mods') -Directory) {
            $private = Join-Path (Join-Path $game 'mods') $mod.Name
            New-Item -ItemType Directory -Path $private | Out-Null
            New-Item -ItemType File -Path (Join-Path $private 'disable.it') | Out-Null
        }
        'Disposable isolated development instance.' | Set-Content -LiteralPath (Join-Path $lab '.isaac-lan-lab') -Encoding ascii
        'Owned isolated LAN automatic loader.' | Set-Content -LiteralPath (Join-Path $game '.isaac-lan-loader') -Encoding ascii
        'Isolated frontend test only.' | Set-Content -LiteralPath (Join-Path $lab 'frontend.test') -Encoding ascii
        'Internal input test only.' | Set-Content -LiteralPath (Join-Path $lab 'virtual-input.test') -Encoding ascii
        '250900' | Set-Content -LiteralPath (Join-Path $game 'steam_appid.txt') -Encoding ascii
        $x = if ($role -eq 'host') { 30 } else { 1010 }
        @"
[Options]
AcceptedPublicBeta_v1.9.7.17=1
AcceptedDataCollectionDisclaimer=1
SteamCloud=0
EnableMods=0
EnableDebugConsole=0
Fullscreen=0
WindowWidth=960
WindowHeight=540
WindowPosX=$x
WindowPosY=80
VSync=1
PauseOnFocusLost=0
ControllerHotplug=0
MusicVolume=0
SFXVolume=0
AnnouncerVoiceMode=0
PopUps=0
"@ | Set-Content -LiteralPath (Join-Path $save 'options.ini') -Encoding ascii
        @'
[logging]
level=INFO
[diagnostics]
performance=true
sample_interval_ms=1000
[network]
input_hz=60
'@ | Set-Content -LiteralPath (Join-Path $game 'isaac-lan/config.ini') -Encoding ascii
        $flag = ($role -eq 'host').ToString().ToLowerInvariant()
        $fixture = "_LanMixed = { host = $flag, port = '$port', window = [==[$window]==] }`n" + [IO.File]::ReadAllText((Join-Path $root 'mixed.lua'))
        [IO.File]::WriteAllText((Join-Path $lab 'frontend-scenario.lua'),$fixture)
    }
    # 复用上游窗口脚本，进程路径必须等于本次刚生成的两个可执行文件之一。
    $helper = [IO.File]::ReadAllText((Join-Path $reference 'tools/lab_window.ps1'))
    $originalGuard = "if (`$process.Path -notmatch '^D:\\isaac-lan-lab\\[^\\]+\\game\\isaac-ng\.exe$') {"
    if (!$helper.Contains($originalGuard)) { throw 'Unexpected upstream window guard' }
    $guard = "if (`$process.Path -notin @('$($paths[0])','$($paths[1])')) {"
    [IO.File]::WriteAllText((Join-Path $run 'lab_window.ps1'),$helper.Replace($originalGuard,$guard))
    Write-Output "Prepared LAN attempt $Attempt"
    return
}

function Send-LabKey($id, $key) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $run 'lab_window.ps1') -GameProcessId $id -Action PostKeys -Value $key
    if ($LASTEXITCODE -ne 0) { throw "Lab menu input failed: $id $key" }
}

$owned = @{}
try {
    $active = @(Get-Process isaac-ng -ErrorAction SilentlyContinue)
    if ($active) { throw 'There is already a running Isaac game' }
    foreach ($role in $roles) {
        $lab = Join-Path $run $role
        $exe = Join-Path $lab 'game/isaac-ng.exe'
        $log = Join-Path $lab "$profile/log.txt"
        if (!(Test-Path -LiteralPath (Join-Path $lab '.isaac-lan-lab')) -or (Test-Path -LiteralPath $log)) { throw "Unexpected lab state: $lab" }
        $launcher = Join-Path $root 'isaac_lan_lab.exe'
        if ($WslLaunch) {
            # 沿用之前静止场景的 WSL 互操作进程路径。
            $wslExe = '/mnt/f' + $launcher.Substring(2).Replace('\','/')
            $launch = & wsl.exe --exec $wslExe $lab --automatic --no-luadebug
        } else {
            $launch = & $launcher $lab --automatic --no-luadebug
        }
        if ($LASTEXITCODE -ne 0 -or "$launch" -notmatch 'autoload_started=1 pid=(\d+)') { throw "LAN launch failed: $launch" }
        $game = Get-Process -Id ([int]$Matches[1])
        if ($game.Path -ne $exe) { throw 'Unexpected game executable' }
        $null = $game.Handle
        $owned[$role] = $game
        Write-Output "Started $role PID $($game.Id)"
        $deadline = [DateTime]::UtcNow.AddSeconds(35)
        while (!(Test-Path -LiteralPath $log) -or !(Select-String -LiteralPath $log -SimpleMatch 'playing cutscene 1' -Quiet)) {
            if ($game.HasExited -or [DateTime]::UtcNow -ge $deadline) { throw "$role did not reach the intro" }
            Start-Sleep -Milliseconds 200
        }
        Send-LabKey $game.Id '{SPACE}'
        Start-Sleep -Seconds 1
        Send-LabKey $game.Id '{ENTER}'
        Start-Sleep -Seconds 1
        Send-LabKey $game.Id '{ENTER}'
    }
    $deadline = [DateTime]::UtcNow.AddSeconds(95)
    $announced = $false
    $results = @($roles | ForEach-Object { Join-Path $run "$_/game/lan-mixed-result.json" })
    while (@($results | Where-Object { !(Test-Path -LiteralPath $_) }).Count) {
        if (!$announced -and (Test-Path -LiteralPath (Join-Path $run 'window.txt'))) {
            Write-Output 'LAN fixture active; completing warm-up and 30-second window.'
            $announced = $true
        }
        foreach ($role in $roles) { if ($owned[$role].HasExited) { throw "$role exited early: $($owned[$role].ExitCode)" } }
        if ([DateTime]::UtcNow -ge $deadline) { throw 'LAN benchmark did not complete on both peers' }
        Start-Sleep -Milliseconds 350
    }
    # 给最后一个覆盖测量窗的诊断桶留出写盘时间。
    Start-Sleep -Seconds 2
} finally {
    foreach ($role in @('client','host')) {
        $game = $owned[$role]
        if (!$game) { continue }
        if (!$game.HasExited) {
            if (!$game.CloseMainWindow() -or !$game.WaitForExit(15000)) { Write-Warning "$role did not close normally"; continue }
        }
        Write-Output "$role exit=$($game.ExitCode)"
    }
}
