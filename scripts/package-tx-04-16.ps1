param([string]$Cohort='tx16-pair-20261031-01', [string]$HarnessPath)
$ErrorActionPreference='Stop'
$tx16PackageRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if(-not $HarnessPath){$HarnessPath=Join-Path $tx16PackageRoot 'out/repentogon/e2e/harness.exe'}
$tx16Output=Join-Path $tx16PackageRoot 'out'
$tx16Leaf='tx-04-16-access-'+[guid]::NewGuid().ToString('N')
$tx16Staging=Join-Path $tx16Output $tx16Leaf
$tx16Zip=Join-Path $tx16Output ($tx16Leaf+'.zip')
# 接入包同时提供产品安装树和可搬到另一台Windows的E2E节点；Rust不是游戏Client。
foreach($tx16Dir in @('install/Repentogon/isaac-cs','install/mods/isaac-cs-ui','e2e/scripts','e2e/out/repentogon/e2e','e2e/ui/isaac-cs-ui','e2e/tools/harness/scenarios')) {
    New-Item -ItemType Directory -Path (Join-Path $tx16Staging $tx16Dir) -Force | Out-Null
}
foreach($tx16Pair in @(
    @('out/repentogon/zhlIsaacCS.dll','install/Repentogon/zhlIsaacCS.dll'),
    @('out/repentogon/e2e/zhlIsaacCS.dll','e2e/out/repentogon/e2e/zhlIsaacCS.dll'),
    @('out/repentogon/e2e/build-flavor.json','e2e/out/repentogon/e2e/build-flavor.json'),
    @('scripts/accept-tx-04-16.ps1','e2e/scripts/accept-tx-04-16.ps1'),
    @('tools/harness/scenarios/tx-04-16.json','e2e/tools/harness/scenarios/tx-04-16.json')
)) {
    Copy-Item -LiteralPath (Join-Path $tx16PackageRoot $tx16Pair[0]) -Destination (Join-Path $tx16Staging $tx16Pair[1])
}
Copy-Item -LiteralPath $HarnessPath -Destination (Join-Path $tx16Staging 'e2e/out/repentogon/e2e/harness.exe')
Copy-Item -Path (Join-Path $tx16PackageRoot 'ui/isaac-cs-ui/*') -Destination (Join-Path $tx16Staging 'install/mods/isaac-cs-ui') -Recurse
Copy-Item -Path (Join-Path $tx16PackageRoot 'ui/isaac-cs-ui/*') -Destination (Join-Path $tx16Staging 'e2e/ui/isaac-cs-ui') -Recurse
$tx16Config=Get-Content -LiteralPath (Join-Path $tx16PackageRoot 'mod/config.example.json') -Raw | ConvertFrom-Json
$tx16Config.enabled=$true
[IO.File]::WriteAllText((Join-Path $tx16Staging 'install/Repentogon/isaac-cs/config.json'),($tx16Config | ConvertTo-Json -Depth 8),(New-Object System.Text.UTF8Encoding($false)))
# 二人节点默认一位远端；正式四端能力没有据此改成两人上限。
$tx16ClientScript=@'
param(
    [Parameter(Mandatory=$true)][string]$GameRoot,
    [Parameter(Mandatory=$true)][string]$HostAddress,
    [Parameter(Mandatory=$true)][ValidateRange(1,65535)][int]$HostPort,
    [string]$Cohort='COHORT_PLACEHOLDER'
)
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'scripts/accept-tx-04-16.ps1') -Stage Run -NodeRole Client -PeerCount 1 -GameRoot $GameRoot -HostAddress $HostAddress -HostPort $HostPort -Cohort $Cohort
'@
if($Cohort -notmatch '^[A-Za-z0-9_.-]{1,64}$'){throw 'Cohort must be a short identifier'}
$tx16ClientScript.Replace('COHORT_PLACEHOLDER',$Cohort) | Set-Content -LiteralPath (Join-Path $tx16Staging 'e2e/Start-Client.ps1') -Encoding UTF8
@"
tx-04-16 two-device acceptance access package
Cohort: $Cohort
Environment: Repentance+ v1.9.7.12.J273 + official REPENTOGON 1.1.3, Windows.

Automatic Client acceptance (recommended):
1. Close the game. Start Steam. Disable third-party gameplay mods.
2. Enable REPENTOGON SkipIntro, use default keyboard bindings, and manually select
   English input mode. Chinese IME caused all-neutral samples in the user's VM.
3. From the e2e folder run in PowerShell:
   .\Start-Client.ps1 -GameRoot 'D:\your-game-root' -HostAddress 'Sakura-IPv4' -HostPort 12345
   GameRoot is the folder containing mods and Repentogon, not Repentogon itself.
4. Automatic window activation is disabled. Select the game window manually once
   when it appears, then stop keyboard/mouse operations until automatic exit.
   The Host temporarily disables PauseOnFocusLost and restores its original files.
   Select the Host again when its test requests returning to the native menu.
5. The script restores the old DLL/UI/configuration. Keep the printed temporary
   result directory until this two-device cohort has been combined.

Manual installation/connection (automatic acceptance does not require this step):
Back up existing zhlIsaacCS.dll, isaac-cs config and isaac-cs-ui folder first.
Copy the CONTENTS of install into the game root; enable the isaac-cs-ui mod.
At a stable native menu, open Isaac CS > Session panel, enter the public IPv4,
UDP port and local device key (keyboard default 0), then Connect as client.
Do not start New Run or Continue on Client. Use Stop to disconnect.

Host endpoint is one UDP port: local 127.0.0.1:27090 for SakuraFrp forwarding.
Do not forward TCP control port 27091.
Client currently waits for Authority world; this tx does not display a shared world.
This package is for ongoing acceptance; four-device acceptance is still pending.
"@ | Set-Content -LiteralPath (Join-Path $tx16Staging 'README.txt') -Encoding UTF8
# Windows PowerShell 5.1按ANSI解码无BOM脚本，中文可能被误读为引号；节点脚本统一带UTF-8 BOM。
Get-ChildItem -LiteralPath (Join-Path $tx16Staging 'e2e') -Recurse -Filter '*.ps1' -File | ForEach-Object {
    $tx16ScriptText=[IO.File]::ReadAllText($_.FullName,[Text.Encoding]::UTF8)
    [IO.File]::WriteAllText($_.FullName,$tx16ScriptText,(New-Object System.Text.UTF8Encoding($true)))
}
Compress-Archive -LiteralPath (Join-Path $tx16Staging 'install'),(Join-Path $tx16Staging 'e2e'),(Join-Path $tx16Staging 'README.txt') -DestinationPath $tx16Zip
# 只移除本次创建且核定在out之内的打包临时树，不删除最终接入包。
$tx16Resolved=[IO.Path]::GetFullPath($tx16Staging)
$tx16Expected=[IO.Path]::GetFullPath($tx16Output)+[IO.Path]::DirectorySeparatorChar
if(-not $tx16Resolved.StartsWith($tx16Expected,[StringComparison]::OrdinalIgnoreCase)){throw 'Package cleanup escaped out'}
Remove-Item -LiteralPath $tx16Resolved -Recurse -Force
Write-Output $tx16Zip
