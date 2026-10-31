param([ValidateSet('All','Build','Run')][string]$Stage='All',
    [string]$GameRoot='F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth',
    [string]$TemporaryDirectory,[string]$ResumeFrom,[string]$Retest)
$ErrorActionPreference='Stop'
$tx18Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if($Stage -in @('All','Build')) {
    & (Join-Path $PSScriptRoot 'build.ps1')
    if($LASTEXITCODE){throw 'tx-04-18 build failed'}
}
if($Stage -eq 'Build'){return}
if(-not $TemporaryDirectory){$TemporaryDirectory=Join-Path $tx18Root ('out/tx-04-18-temp-'+[guid]::NewGuid().ToString('N'))}
$tx18Temporary=[IO.Path]::GetFullPath($TemporaryDirectory)
$tx18Output=[IO.Path]::GetFullPath((Join-Path $tx18Root 'out'))+[IO.Path]::DirectorySeparatorChar
if(-not $tx18Temporary.StartsWith($tx18Output,[StringComparison]::OrdinalIgnoreCase)){throw 'Temporary directory must stay inside repository out/'}
Write-Output 'tx-04-18 is starting one real game. Stop manual game input until it exits; no manual cooperation or focus switching is required.'
$tx18Args=@('e2e','tx-04-18','--game',$GameRoot,'--artifact-dir',$tx18Temporary)
if($ResumeFrom){$tx18Args+=@('--resume-from',[IO.Path]::GetFullPath($ResumeFrom))}
if($Retest){$tx18Args+=@('--retest',$Retest)}
& (Join-Path $tx18Root 'out/repentogon/harness.exe') @tx18Args
if($LASTEXITCODE){throw "tx-04-18 continuous diagnosis/continuation: $tx18Temporary"}
