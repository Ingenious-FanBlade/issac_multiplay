param(
    [Parameter(Mandatory=$true)][string]$GameRoot,
    [string]$RunDirectory,
    [switch]$Resume,
    [ValidateSet('room-grid-source','room-grid-client','entity-source','entity-client')][string[]]$RerunStage=@()
)
$ErrorActionPreference='Stop'
$tx19Repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if(-not $RunDirectory){$RunDirectory=Join-Path $tx19Repo ('out/tx-04-19-e2e-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))}
$tx19Run=[IO.Path]::GetFullPath($RunDirectory)
if((Test-Path -LiteralPath $tx19Run) -and -not $Resume){throw 'Use a new temporary directory, or explicitly resume this tx'}
New-Item -ItemType Directory -Path $tx19Run -Force | Out-Null
# 纯构建也统一放在I34入口，构建脚本不运行测试；两种DLL分别输出。
& (Join-Path $PSScriptRoot 'build.ps1')
& (Join-Path $PSScriptRoot 'build.ps1') -E2E
$tx19Harness=Join-Path $tx19Repo 'out/repentogon/harness.exe'
# 一个整体验收入口包含全部18回归与19；失败保留必要进度，修复后同tx续测，不启动旧DLL。
$tx19Stages=@(
    @('room-grid-source','tx-04-18','Source',$null),
    @('room-grid-client','tx-04-18','Client','room-grid-source'),
    @('entity-source','tx-04-19','Source',$null),
    @('entity-client','tx-04-19','Client','entity-source')
)
$tx19Progress=@{}
$tx19ProgressPath=Join-Path $tx19Run 'progress.json'
if($Resume -and (Test-Path -LiteralPath $tx19ProgressPath)){
    $tx19Progress=Get-Content -LiteralPath $tx19ProgressPath -Raw | ConvertFrom-Json -AsHashtable
}
# 修复影响由执行者按实际改动声明，不比较哈希；重采Source时其Client也必须重跑。
foreach($name in $RerunStage){
    $tx19Progress.Remove($name)
    if($name -eq 'room-grid-source'){$tx19Progress.Remove('room-grid-client')}
    if($name -eq 'entity-source'){$tx19Progress.Remove('entity-client')}
}
foreach($stage in $tx19Stages){
    $name,$tx,$role,$source=$stage
    if($tx19Progress.ContainsKey($name) -and $tx19Progress[$name].automatic_complete){continue}
    $attempt=1
    do{$stageOutput=Join-Path $tx19Run ($name+'-'+$attempt);$attempt++}while(Test-Path -LiteralPath $stageOutput)
    # 每一次游戏启动单独说明实际配合和停止时机；不把过去的准备确认沿用到下一次启动。
    Write-Host "$name：即将启动 $tx 的 $role。请只跳过开场动画；到标题或选档页立即停止操作，随后由编排器接管。"
    $ready=Read-Host '准备好后输入 ready；其它输入保留本次进度并停止'
    if($ready -cne 'ready'){Write-Host "尚未启动游戏，继续位置：$tx19Run";return}
    $arguments=@('e2e',$tx,'--game',$GameRoot,'--repo',$tx19Repo,'--out',$stageOutput,'--node-role',$role)
    if($source){$arguments+=@('--resume-from',$tx19Progress[$source].directory)}
    & $tx19Harness @arguments
    $stageExit=$LASTEXITCODE
    $resultPath=Join-Path $stageOutput 'result.json'
    $result=if(Test-Path -LiteralPath $resultPath){Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json}else{$null}
    $tx19Progress[$name]=@{directory=$stageOutput;automatic_complete=($stageExit -eq 0 -and $result.pass -eq $true);exit_code=$stageExit}
    $tx19Progress | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $tx19ProgressPath -Encoding utf8
    if(-not $tx19Progress[$name].automatic_complete){
        throw "同一tx整体验收尚未完成。保留必要诊断：$stageOutput。定位并修复后使用 -Resume 继续；不得忽略失败或改成PASS。"
    }
}
# 实际画面审阅和E01—E26逐项归约由I34执行者连续完成；自动完成不冒充最终验收通过。
Write-Host "自动场景已完成。继续审阅Source/Client同窗口画面与E01—E26矩阵：$tx19Run。正常完成后清理该次临时目录。"
