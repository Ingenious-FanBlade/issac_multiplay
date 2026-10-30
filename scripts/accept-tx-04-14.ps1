param([ValidateSet('Build','Run','All','Reduce')][string]$Stage='All',[string]$GameRoot='F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth',[string]$TemporaryDirectory,[string]$ResumeFrom,[string]$RetestIds,[switch]$GlobalRandom)
$ErrorActionPreference='Stop'
$txRepoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if($Stage -eq 'Reduce') {
    if(-not $TemporaryDirectory) { throw 'Reduce requires completed scene directory' }
    & (Join-Path $txRepoRoot 'out/repentogon/harness.exe') 'tx14-reduce' $TemporaryDirectory
    if($LASTEXITCODE) { throw 'tx-04-14 formal-record reduction failed' }
    return
}
# 全目标完成后统一验收；真实框架作为临时依赖，框架/Mod存档及部署由harness备份恢复。
if(-not $GlobalRandom -and -not (Test-Path -LiteralPath (Join-Path $txRepoRoot 'out/tx14-source/BOIStageAPI15-master/main.lua'))) {
    $txSource=Join-Path $txRepoRoot 'out/tx14-source'
    New-Item -ItemType Directory -Path $txSource -Force | Out-Null
    Invoke-WebRequest -Uri 'https://codeload.github.com/Meowlala/BOIStageAPI15/zip/refs/heads/master' -OutFile (Join-Path $txSource 'stageapi.zip')
    Expand-Archive -LiteralPath (Join-Path $txSource 'stageapi.zip') -DestinationPath $txSource -Force
}
if($Stage -ne 'Run') { & (Join-Path $PSScriptRoot 'build.ps1'); if($LASTEXITCODE) { throw 'tx-04-14 acceptance build failed' } }
if($Stage -ne 'Build') {
    if(-not $TemporaryDirectory) { $TemporaryDirectory=Join-Path $txRepoRoot ('out/tx-04-14-temp-'+[guid]::NewGuid().ToString('N')) }
    $txTemporary=[IO.Path]::GetFullPath($TemporaryDirectory)
    $txOutput=[IO.Path]::GetFullPath((Join-Path $txRepoRoot 'out'))+[IO.Path]::DirectorySeparatorChar
    if(-not $txTemporary.StartsWith($txOutput,[StringComparison]::OrdinalIgnoreCase)) { throw 'Temporary directory must stay inside repository out/' }
    $txScenario=if($GlobalRandom){'tools/harness/scenarios/tx-04-14-global-random.json'}else{'tools/harness/scenarios/tx-04-14.json'}
    $txArguments=@('e2e','tx-04-14','--game',$GameRoot,'--artifact-dir',$txTemporary,'--scenario',(Join-Path $txRepoRoot $txScenario))
    if($ResumeFrom) { $txArguments+=@('--resume-from',$ResumeFrom) }
    if($RetestIds) { $txArguments+=@('--retest-ids',$RetestIds) }
    & (Join-Path $txRepoRoot 'out/repentogon/harness.exe') @txArguments
    if($LASTEXITCODE) { throw "tx-04-14 acceptance requires diagnosis/continuation: $txTemporary" }
}
