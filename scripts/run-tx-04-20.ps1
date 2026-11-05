param(
    [Parameter(Mandatory=$true)][string]$GameRoot,
    [string]$RunDirectory,
    [switch]$Resume,
    [ValidateSet('All','Build','player-source','player-client','room-grid-source','room-grid-client','entity-source','entity-client')][string]$Stage='All',
    [switch]$LaunchReady,
    [switch]$SkipBuild,
    [string]$SourcePrefixDirectory,
    [int]$SourceReusePrefix=0,
    [string]$ClientPrefixDirectory,
    [int]$ClientReusePrefix=0,
    [ValidateSet('player-source','player-client','room-grid-source','room-grid-client','entity-source','entity-client')][string[]]$RerunStage=@()
)
$ErrorActionPreference='Stop'
$tx20Repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if(-not $RunDirectory){$RunDirectory=Join-Path $tx20Repo ('out/tx-04-20-e2e-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))}
$tx20Run=[IO.Path]::GetFullPath($RunDirectory)
if((Test-Path -LiteralPath $tx20Run) -and -not $Resume){throw 'Use a new temporary directory, or explicitly resume this tx'}
New-Item -ItemType Directory -Path $tx20Run -Force | Out-Null
# 纯构建也统一放在I39入口，构建脚本不运行测试；两种DLL分别输出。
if(-not $SkipBuild){
& (Join-Path $PSScriptRoot 'build.ps1')
& (Join-Path $PSScriptRoot 'build.ps1') -E2E
}
if($Stage -eq 'Build'){return}
$tx20Harness=Join-Path $tx20Repo 'out/repentogon/e2e/harness.exe'
# 一个整体验收入口包含20及全部18、19回归；失败保留必要进度，修复后同tx续测，不启动旧DLL。
$tx20Stages=@(
    @('player-source','tx-04-20','Source',$null),
    @('player-client','tx-04-20','Client','player-source'),
    @('room-grid-source','tx-04-18','Source',$null),
    @('room-grid-client','tx-04-18','Client','room-grid-source'),
    @('entity-source','tx-04-19','Source',$null),
    @('entity-client','tx-04-19','Client','entity-source')
)
$tx20Progress=@{}
$tx20ProgressPath=Join-Path $tx20Run 'progress.json'
if($Resume -and (Test-Path -LiteralPath $tx20ProgressPath)){
    $tx20Progress=Get-Content -LiteralPath $tx20ProgressPath -Raw | ConvertFrom-Json -AsHashtable
}
# 修复影响由执行者按实际改动声明，不比较哈希；重采Source时其Client也必须重跑。
foreach($name in $RerunStage){
    $tx20Progress.Remove($name)
    if($name -eq 'player-source'){$tx20Progress.Remove('player-client')}
    if($name -eq 'room-grid-source'){$tx20Progress.Remove('room-grid-client')}
    if($name -eq 'entity-source'){$tx20Progress.Remove('entity-client')}
}
foreach($stageEntry in $tx20Stages){
    $name,$tx,$role,$source=$stageEntry
    if($Stage -ne 'All' -and $Stage -ne $name){continue}
    if($source -and (-not $tx20Progress.ContainsKey($source) -or -not $tx20Progress[$source].automatic_complete)){throw "Completed Source required: $source"}
    if($tx20Progress.ContainsKey($name) -and $tx20Progress[$name].automatic_complete){continue}
    $attempt=1
    do{$stageOutput=Join-Path $tx20Run ($name+'-'+$attempt);$attempt++}while(Test-Path -LiteralPath $stageOutput)
    # 每次启动仍提醒；LaunchReady由当前会话的逐次或连续测试授权提供。
    Write-Host "$name：即将启动 $tx 的 $role。请只跳过开场动画；到标题或选档页立即停止操作，随后由编排器接管。"
    $ready=if($LaunchReady -and $Stage -ne 'All'){'ready'}else{Read-Host '准备好后输入 ready；其它输入保留本次进度并停止'}
    if($ready -cne 'ready'){Write-Host "尚未启动游戏，继续位置：$tx20Run";return}
    $arguments=@('e2e',$tx,'--game',$GameRoot,'--repo',$tx20Repo,'--out',$stageOutput,'--node-role',$role)
    if($source){$arguments+=@('--resume-from',$tx20Progress[$source].directory)}
    if($name -eq 'room-grid-client'){$arguments+='--player-environment-regression'}
    if($name -in @('player-client','entity-client') -and $ClientPrefixDirectory){
        if($ClientReusePrefix -le 0){throw 'Explicit unaffected Client prefix count required'}
        $arguments+=@('--resume-client-from',[IO.Path]::GetFullPath($ClientPrefixDirectory),'--resume-client-prefix',"$ClientReusePrefix")
    }
    if($name -eq 'player-source' -and $SourcePrefixDirectory){
        if($SourceReusePrefix -le 0){throw 'Explicit unaffected Source prefix count required'}
        $arguments+=@('--resume-from',[IO.Path]::GetFullPath($SourcePrefixDirectory),'--resume-source-prefix',"$SourceReusePrefix")
    }
    & $tx20Harness @arguments
    $stageExit=$LASTEXITCODE
    $resultPath=Join-Path $stageOutput 'result.json'
    $result=if(Test-Path -LiteralPath $resultPath){Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json}else{$null}
    $tx20Progress[$name]=@{directory=$stageOutput;automatic_complete=($stageExit -eq 0 -and $result.pass -eq $true);exit_code=$stageExit}
    $tx20Progress | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $tx20ProgressPath -Encoding utf8
    if(-not $tx20Progress[$name].automatic_complete){
        throw "同一tx整体验收尚未完成。保留必要诊断：$stageOutput。定位并修复后使用 -Resume 继续；不得忽略失败或改成PASS。"
    }
}
# 实际画面审阅和E01—E30逐项归约由I39执行者连续完成；自动完成不冒充最终验收通过。
Write-Host "自动场景已完成。继续审阅Source/Client同窗口画面与E01—E30矩阵：$tx20Run。正常完成后清理该次临时目录。"
