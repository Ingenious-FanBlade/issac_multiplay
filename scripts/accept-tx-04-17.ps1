param([ValidateSet('All','Build','Run')][string]$Stage='All',
    [string]$GameRoot='F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth',
    [string]$TemporaryDirectory,[string]$ResumeFrom)
$ErrorActionPreference='Stop'
$tx17Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if($Stage -in @('All','Build')) {
    & (Join-Path $PSScriptRoot 'build.ps1')
    if($LASTEXITCODE){throw 'tx-04-17 build failed'}
}
if($Stage -eq 'Build'){return}
if(-not $TemporaryDirectory){$TemporaryDirectory=Join-Path $tx17Root ('out/tx-04-17-temp-'+[guid]::NewGuid().ToString('N'))}
$tx17Temporary=[IO.Path]::GetFullPath($TemporaryDirectory)
$tx17Output=[IO.Path]::GetFullPath((Join-Path $tx17Root 'out'))+[IO.Path]::DirectorySeparatorChar
if(-not $tx17Temporary.StartsWith($tx17Output,[StringComparison]::OrdinalIgnoreCase)){throw 'Temporary directory must stay inside repository out/'}
Write-Output 'tx-04-17 is starting one real game. Stop manual game input until it exits; no manual cooperation or focus switching is required.'
$tx17Args=@('e2e','tx-04-17','--game',$GameRoot,'--artifact-dir',$tx17Temporary)
if($ResumeFrom){$tx17Args+=@('--resume-from',[IO.Path]::GetFullPath($ResumeFrom))}
& (Join-Path $tx17Root 'out/repentogon/harness.exe') @tx17Args
if($LASTEXITCODE){throw "tx-04-17 continuous diagnosis/continuation: $tx17Temporary"}
