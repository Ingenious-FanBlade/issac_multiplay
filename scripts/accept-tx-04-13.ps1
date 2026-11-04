param([ValidateSet('Build','Run','All','Reduce')][string]$Stage='All',[string]$GameRoot,[string]$TemporaryDirectory,[string]$ResumeFrom,[string]$RetestIds)
$ErrorActionPreference='Stop'
$txRepoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# 全目标完成后调用；构建与必要修复续测均属于同一 tx 的整体验收。
if($Stage -eq 'Reduce') {
    if(-not $TemporaryDirectory) { throw 'Reduce requires a completed tx temporary directory' }
    # 仅重读已完成实机且恢复环境的正式记录；不启动游戏、不重发动作。
    & (Join-Path $txRepoRoot 'out/repentogon/e2e/harness.exe') 'tx13-reduce' $TemporaryDirectory
    if($LASTEXITCODE) { throw 'tx-04-13 formal-record reduction failed' }
    return
}
if($Stage -ne 'Run') {
    & (Join-Path $PSScriptRoot 'build.ps1') -E2E
    if($LASTEXITCODE) { throw 'tx-04-13 acceptance build failed' }
}
if($Stage -ne 'Build') {
    if(-not $GameRoot) { throw 'Run requires actual game root' }
    if(-not $TemporaryDirectory) { $TemporaryDirectory=Join-Path $txRepoRoot ('out/tx-04-13-continuity-temp-'+[guid]::NewGuid().ToString('N')) }
    $txTemporary=[IO.Path]::GetFullPath($TemporaryDirectory)
    $txOutput=[IO.Path]::GetFullPath((Join-Path $txRepoRoot 'out'))+[IO.Path]::DirectorySeparatorChar
    if(-not $txTemporary.StartsWith($txOutput,[StringComparison]::OrdinalIgnoreCase)) { throw 'Temporary directory must stay inside repository out/' }
    # 用户已授权测试直接启动；调用者每次启动前提示停止手动操作。
    $txArguments=@('e2e','tx-04-13','--game',$GameRoot,'--artifact-dir',$txTemporary,'--scenario',(Join-Path $txRepoRoot 'tools/harness/scenarios/tx-04-13.json'))
    # 续测复用同tx正式样本；按实际中断阶段重建必要前置，仅执行未完成项目。
    if($ResumeFrom) { $txArguments+=@('--resume-from',$ResumeFrom) }
    if($RetestIds) { $txArguments+=@('--retest-ids',$RetestIds) }
    & (Join-Path $txRepoRoot 'out/repentogon/e2e/harness.exe') @txArguments
    if($LASTEXITCODE) { throw "tx-04-13 acceptance requires diagnosis/continuation: $txTemporary" }
}
