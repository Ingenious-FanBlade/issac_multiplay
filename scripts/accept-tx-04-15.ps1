param([ValidateSet('All','Build','Run','Reduce')][string]$Stage='All',[string]$GameRoot='F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth',[string]$TemporaryDirectory,[switch]$RetestContract)
$ErrorActionPreference='Stop'
$tx15Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if($Stage -in @('All','Build')) { & (Join-Path $PSScriptRoot 'build.ps1') -E2E; if($LASTEXITCODE) { throw 'tx-04-15 acceptance build failed' } }
if($Stage -eq 'Build'){return}
if($Stage -eq 'Reduce') {
    if(-not $TemporaryDirectory){throw 'Reduce requires interrupted/completed temporary directory'}
    $tx15Reduce=@('tx15-reduce',$TemporaryDirectory)
    if($RetestContract){$tx15Reduce+='--retest-contract'}
    & (Join-Path $tx15Root 'out/repentogon/e2e/harness.exe') @tx15Reduce
    if($LASTEXITCODE){throw 'tx-04-15 reduction failed'}
    return
}
if(-not $TemporaryDirectory){$TemporaryDirectory=Join-Path $tx15Root ('out/tx-04-15-temp-'+[guid]::NewGuid().ToString('N'))}
$tx15Temporary=[IO.Path]::GetFullPath($TemporaryDirectory)
$tx15Output=[IO.Path]::GetFullPath((Join-Path $tx15Root 'out'))+[IO.Path]::DirectorySeparatorChar
if(-not $tx15Temporary.StartsWith($tx15Output,[StringComparison]::OrdinalIgnoreCase)){throw 'Temporary directory must stay inside repository out/'}
& (Join-Path $tx15Root 'out/repentogon/e2e/harness.exe') 'e2e' 'tx-04-15' '--game' $GameRoot '--artifact-dir' $tx15Temporary
if($LASTEXITCODE){throw "tx-04-15 requires continuous diagnosis/continuation: $tx15Temporary"}
