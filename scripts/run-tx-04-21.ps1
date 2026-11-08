param(
 [Parameter(Mandatory=$true)][string]$Topology,
 [string]$RunDirectory,
 [switch]$Resume,
 [string[]]$Rerun=@(),
 [string]$ReuseReason,
 [string]$UiDirectory,
 [string]$UiSession
)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$tx21Repo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$tx21Spec=Get-Content -LiteralPath $Topology -Raw | ConvertFrom-Json -AsHashtable
# 两人设备可先独立完成P2；未明确选择p2时仍执行原有P2→P4完整流程。
$tx21Phase=if($tx21Spec.phase){[string]$tx21Spec.phase}else{'p2-p4'}
if($tx21Phase -notin @('p2','p2-p4')){throw 'Topology phase must be p2 or p2-p4'}
if($tx21Spec.format -ne 'isaac.cs.tx21.topology' -or $tx21Spec.nodes.Count -notin @(2,4) -or ($tx21Phase -eq 'p2-p4' -and $tx21Spec.nodes.Count -ne 4)){throw 'P2 requires Host and one real Client; P2/P4 requires Host and three real Clients'}
$tx21ExecutionNodes=if($tx21Phase -eq 'p2'){@($tx21Spec.nodes | Select-Object -First 2)}else{@($tx21Spec.nodes)}
for($i=0;$i -lt $tx21ExecutionNodes.Count;$i++){
 $expectedId=@('host','client1','client2','client3')[$i]
 $expectedRole=if($i -eq 0){'Host'}else{'Client'}
 if($tx21ExecutionNodes[$i].id -ne $expectedId -or $tx21ExecutionNodes[$i].role -ne $expectedRole){throw 'Explicit ordered Host/Client node identities required'}
}
if($UiDirectory -and ($tx21Spec.nodes[0].id -ne 'host' -or $tx21Spec.nodes[0].computer -ne 'localhost')){throw 'In-game GUI requires the coordinator to be the local Host'}
if(-not $RunDirectory){$RunDirectory=Join-Path $tx21Repo ('out/tx-04-21-'+[guid]::NewGuid().ToString('N'))}
$tx21Run=[IO.Path]::GetFullPath($RunDirectory)
if($Resume -and [string]::IsNullOrWhiteSpace($ReuseReason)){throw 'Resume requires explicit unaffected-result reuse reason and affected p2/E.. or p4/E.. reruns; no hashes'}
if($UiDirectory){
 $uiExpected=[IO.Path]::GetFullPath((Join-Path $tx21Repo 'out/tx-04-21-ui'))
 if([IO.Path]::GetFullPath($UiDirectory) -ine $uiExpected -or $UiSession -cnotmatch '^[0-9a-f]{32}$'){throw 'Explicit local GUI mailbox/session required'}
 if($tx21Run -ieq $uiExpected -or $uiExpected.StartsWith($tx21Run+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or $tx21Run.StartsWith($uiExpected+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Run temporary directory must be separate from GUI settings'}
}
if($tx21Run -eq $tx21Repo -or $tx21Run -eq [IO.Path]::GetPathRoot($tx21Run)){throw 'Dedicated temporary leaf directory required'}
$tx21Owner=Join-Path $tx21Run 'tx21-run.json'
if(Test-Path -LiteralPath $tx21Run){
 if(-not $Resume -or -not (Test-Path -LiteralPath $tx21Owner -PathType Leaf)){throw 'New directory or explicit same-tx resume required'}
 $tx21Saved=Get-Content -LiteralPath $tx21Owner -Raw | ConvertFrom-Json -AsHashtable
 if($tx21Saved.path -cne $tx21Run -or $tx21Saved.tx -ne 'tx-04-21'){throw 'Temporary owner record differs'}
}else{
 New-Item -ItemType Directory -Path $tx21Run | Out-Null
 $tx21Saved=@{tx='tx-04-21';path=$tx21Run;token=[guid]::NewGuid().ToString('N');attempt=0;nodes=@();history_nodes=@()}
}
if((Get-Item -LiteralPath $tx21Run).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Refuse reparse temporary root'}
$tx21Sessions=@{}
$tx21Complete=$false

# 所有远端操作只指向显式节点；节点进程运行于已登录用户的交互桌面，WinRM会话不冒充游戏窗口。
function In-Node($node,[scriptblock]$operation,[object[]]$arguments){
 if($node.computer -eq 'localhost'){& $operation @arguments}
 else{
  # VM重启会退休旧WinRM会话；下一次收尾先换新连接，不复用Broken状态，也不盲目重放已执行操作。
  if($tx21Sessions.ContainsKey($node.id) -and $tx21Sessions[$node.id].State -ne 'Opened'){
   Remove-PSSession -Session $tx21Sessions[$node.id] -ErrorAction SilentlyContinue
   $tx21Sessions.Remove($node.id)
  }
  if(-not $tx21Sessions.ContainsKey($node.id)){
   $connection=@{ComputerName=$node.computer;ErrorAction='Stop'}
   # 工作组VM使用用户在宿主机保存的DPAPI凭据；不把口令放入拓扑、日志或远端节点配置。
   if($node.credential_path){
    $credential=Import-Clixml -LiteralPath $node.credential_path
    if($credential -isnot [Management.Automation.PSCredential]){throw 'Explicit local PSCredential file required'}
    $connection.Credential=$credential;$connection.Authentication='Negotiate'
   }
   $tx21Sessions[$node.id]=New-PSSession @connection
  }
  Invoke-Command -Session $tx21Sessions[$node.id] -ScriptBlock $operation -ArgumentList $arguments
 }
}
function Stop-Node($node){
 # 必须先走实际原生退出和设置还原；不使用taskkill或Stop-Process替代退休。
 $address=$node.agent_endpoint.Split(':')
 $client=[Net.Sockets.TcpClient]::new()
 try{
  $client.Connect($address[0],[int]$address[1]);$client.ReceiveTimeout=180000;$client.SendTimeout=10000
  $stream=$client.GetStream();$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false),4096,$true);$writer.AutoFlush=$true
  $reader=[IO.StreamReader]::new($stream,[Text.Encoding]::UTF8,$false,4096,$true)
  $writer.WriteLine((@{id=1;token=$node.agent_token;op='shutdown';payload=@{}} | ConvertTo-Json -Compress))
  $reply=$reader.ReadLine() | ConvertFrom-Json
  if($reply.ok -ne $true -or $reply.data.game_exited -ne $true){throw "Node retirement/restore failed: $($node.id) $reply"}
  # 编排器中途修复时也保留节点真实退出/存档比较，不把断开的协调进程当作完整用例收尾。
  $reply.data | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath (Join-Path $tx21Run ('resume-shutdown-'+$tx21Saved.attempt+'-'+$node.id+'.json')) -Encoding utf8
 }finally{$client.Dispose()}
}
function Restore-Node($node){
 In-Node $node {
  param($game,$backup,$script,$task)
  if(Test-Path -LiteralPath (Join-Path $backup 'deployment-state.json')){& ([scriptblock]::Create((Get-Content -LiteralPath $script -Raw -Encoding utf8))) -Action Restore -GameRoot $game -BackupDirectory $backup}
  if($task -and (Get-ScheduledTask -TaskName $task -ErrorAction SilentlyContinue)){Unregister-ScheduledTask -TaskName $task -Confirm:$false}
 } @($node.game_root,$node.backup_directory,$node.deploy_script,$node.scheduled_task) | Out-Null
}
try{
 # 恢复旧尝试后才重构建/覆盖工具；缺席设备保留必要恢复记录，不猜测它已退出。
 if($Resume){foreach($node in $tx21Saved.nodes){if($node.restored){continue};if(-not ($node.ContainsKey('launch_requested') -and -not $node.launch_requested)){Stop-Node $node};Restore-Node $node;$node.restored=$true}}
 if(-not $tx21Saved.ContainsKey('history_nodes')){$tx21Saved.history_nodes=@()}
 $tx21Saved.history_nodes+=@($tx21Saved.nodes)
 $tx21Saved.attempt=[int]$tx21Saved.attempt+1
 $tx21Attempt=Join-Path $tx21Run ('attempt-'+$tx21Saved.attempt)
 New-Item -ItemType Directory -Path $tx21Attempt | Out-Null
 Write-Host "进入tx-04-21 I39统一整体验收（$tx21Phase）：现在才执行产品/E2E构建。P2完整E01—E26及全部子用例；仅p2-p4模式继续P4，每次启动另行等待人工准备。"
 & (Join-Path $PSScriptRoot 'build.ps1')
 & (Join-Path $PSScriptRoot 'build.ps1') -E2E
 $tx21Product=Join-Path $tx21Attempt 'product'
 $tx21E2E=Join-Path $tx21Attempt 'e2e'
 & (Join-Path $PSScriptRoot 'package.ps1') -OutputDirectory $tx21Product
 & (Join-Path $PSScriptRoot 'package.ps1') -E2E -OutputDirectory $tx21E2E
 $tx21Runtime=@{phase=$tx21Phase;e2e_token=$tx21Saved.token;proxy_seed=$tx21Spec.proxy_seed;host_endpoint=$tx21Spec.host_endpoint;reuse_passes=[bool]$Resume;reuse_reason=$ReuseReason;rerun=$Rerun;nodes=@()}
 if($UiDirectory){$tx21Runtime.ui_directory=$UiDirectory;$tx21Runtime.ui_session=$UiSession}
 $tx21Saved.nodes=@()
 foreach($entry in $tx21ExecutionNodes){
  $node=@{};foreach($key in $entry.Keys){$node[$key]=$entry[$key]}
  if($node.id -notin @('host','client1','client2','client3')){throw 'Fixed readable node identity required'}
  $node.e2e_token=$tx21Saved.token;$node.agent_token=[guid]::NewGuid().ToString('N')+[guid]::NewGuid().ToString('N')
  if($UiDirectory -and $node.id -eq 'host'){$node.ui_directory=$UiDirectory}
  $leaf='tx21-'+$tx21Saved.token+'-'+$tx21Saved.attempt+'-'+$node.id
  $node.work_directory=Join-Path $node.remote_work_root $leaf
  $node.run_directory=Join-Path $node.work_directory 'runtime'
  $node.product_package=Join-Path $node.work_directory 'product'
  $node.e2e_package=Join-Path $node.work_directory 'e2e'
  # 工具使用稳定路径，避免每次尝试产生新的防火墙程序身份；游戏与临时数据仍按尝试隔离。
  $node.agent_executable=if($node.computer -eq 'localhost'){Join-Path $tx21Repo 'out/repentogon/e2e/harness.exe'}else{Join-Path $node.remote_work_root 'node-tools/harness.exe'}
  $node.backup_directory=Join-Path $node.work_directory 'original'
  $node.deploy_script=Join-Path $node.work_directory 'deploy.ps1'
  $node.scheduled_task=if($node.computer -eq 'localhost'){$null}else{'IsaacCS-'+$leaf}
  $node.restored=$false
  $node.launch_requested=$false
  # 日志/恢复记录先写，远端拷贝或部署中断仍可按实际文件续收尾。
  $tx21Saved.nodes+=,$node
  $tx21Saved | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $tx21Owner -Encoding utf8
  In-Node $node {param($path)if(Test-Path -LiteralPath $path){throw 'Use new per-attempt node leaf'};New-Item -ItemType Directory -Path $path | Out-Null} @($node.work_directory) | Out-Null
  $nodeConfig=Join-Path $tx21Attempt ($node.id+'-node.json')
  $node | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $nodeConfig -Encoding utf8
  if($node.computer -eq 'localhost'){
   Copy-Item -LiteralPath $tx21Product -Destination $node.product_package -Recurse
   Copy-Item -LiteralPath $tx21E2E -Destination $node.e2e_package -Recurse
   Copy-Item -LiteralPath $nodeConfig -Destination (Join-Path $node.work_directory 'node.json')
   Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'deploy-tx-04-21.ps1') -Destination $node.deploy_script
  }else{
   $session=$tx21Sessions[$node.id]
   Copy-Item -LiteralPath $tx21Product -Destination $node.product_package -Recurse -ToSession $session
   Copy-Item -LiteralPath $tx21E2E -Destination $node.e2e_package -Recurse -ToSession $session
   Copy-Item -LiteralPath $nodeConfig -Destination (Join-Path $node.work_directory 'node.json') -ToSession $session
   Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'deploy-tx-04-21.ps1') -Destination $node.deploy_script -ToSession $session
  }
  In-Node $node {
   param($game,$backup,$package,$script)
   # WinRM直接执行本次已复制的UTF-8部署代码；PS5.1不能按系统ANSI读取中文注释，不修改磁盘脚本执行策略。
   & ([scriptblock]::Create((Get-Content -LiteralPath $script -Raw -Encoding utf8))) -Action Install -GameRoot $game -BackupDirectory $backup -PackageDirectory $package
  } @($node.game_root,$node.backup_directory,$node.e2e_package,$node.deploy_script) | Out-Null
  # 发出启动请求前记账；未到这里的部署失败可以直接还原，不等待不存在的helper。
  $node.launch_requested=$true
  $tx21Saved | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $tx21Owner -Encoding utf8
  In-Node $node {
   param($package,$work,$task,$interactiveUser,$agentExecutable,$remoteRoot)
   $exe=[IO.Path]::GetFullPath($agentExecutable);$configuration=Join-Path $work 'node.json'
   if($task){
    $expected=[IO.Path]::GetFullPath((Join-Path $remoteRoot 'node-tools/harness.exe'))
    if($exe -ine $expected){throw 'Stable helper path escaped explicit remote root'}
    $tools=Split-Path -Parent $exe
    if((Test-Path -LiteralPath $tools) -and ((Get-Item -LiteralPath $tools).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Stable helper directory cannot be a reparse point'}
    New-Item -ItemType Directory -Path $tools -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $package 'e2e-tools/harness.exe') -Destination $exe -Force
   }
   $arguments='tx21 node "'+$configuration+'"'
   if($task){
    if([string]::IsNullOrWhiteSpace($interactiveUser)){throw 'Remote logged-on interactive user required; service desktop is not a real test window'}
    $launcher=@'
param($nodeRoot,$nodeExe)
$ErrorActionPreference='Stop'
$nodeConfig=Join-Path $nodeRoot 'node.json'
Start-Process -FilePath $nodeExe -ArgumentList ('tx21 node "'+$nodeConfig+'"') -WorkingDirectory $nodeRoot -WindowStyle Hidden -Wait
'@
    # 固定代码与显式路径分开，路径按PowerShell单引号字面量转义，不拼入口令或任意命令。
    $launchCommand='& {'+[Environment]::NewLine+$launcher+[Environment]::NewLine+"} '"+$work.Replace("'","''")+"' '"+$exe.Replace("'","''")+"'"
    $encoded=[Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($launchCommand))
    $action=New-ScheduledTaskAction -Execute 'powershell.exe' -Argument ('-NoProfile -WindowStyle Hidden -EncodedCommand '+$encoded) -WorkingDirectory $work
    $principal=New-ScheduledTaskPrincipal -UserId $interactiveUser -LogonType Interactive -RunLevel Limited
    $settings=New-ScheduledTaskSettingsSet -ExecutionTimeLimit ([TimeSpan]::Zero) -Hidden
    Register-ScheduledTask -TaskName $task -Action $action -Principal $principal -Settings $settings | Out-Null
    Start-ScheduledTask -TaskName $task
   }else{Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $work -WindowStyle Hidden | Out-Null}
  } @($node.e2e_package,$node.work_directory,$node.scheduled_task,$node.interactive_user,$node.agent_executable,$node.remote_work_root) | Out-Null
  $tx21Runtime.nodes+=,$node
 }
 $tx21RuntimePath=Join-Path $tx21Attempt 'runtime.json'
 $tx21Runtime | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $tx21RuntimePath -Encoding utf8
 # Rust等待真实节点RPC；后台helper启动不等于远端游戏已启动/准备完成。
 & (Join-Path $tx21Repo 'out/repentogon/e2e/harness.exe') tx21 run $tx21RuntimePath $tx21Run
 if($LASTEXITCODE -ne 0){throw 'Same tx incomplete: fix and resume affected cases; no failed case is skipped'}
 foreach($node in $tx21Saved.nodes){Restore-Node $node;$node.restored=$true}
 $tx21Complete=$true
}finally{
 # 中断仍尽力正常退休及还原；恢复失败保留具体节点路径，绝不删除可能仍被使用的材料。
 foreach($node in $tx21Saved.nodes){if(-not $node.restored){try{if(-not ($node.ContainsKey('launch_requested') -and -not $node.launch_requested)){Stop-Node $node};Restore-Node $node;$node.restored=$true}catch{Write-Warning "必要中断恢复尚未完成 $($node.id)：$_"}}}
 $tx21Saved | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $tx21Owner -Encoding utf8
 if($tx21Complete -and ($tx21Saved.nodes | Where-Object {-not $_.restored}).Count -eq 0){
  foreach($node in @($tx21Saved.history_nodes)+@($tx21Saved.nodes)){
   In-Node $node {
    param($root,$path,$expected)
    $resolved=[IO.Path]::GetFullPath($path);$parent=[IO.Path]::GetFullPath($root)+[IO.Path]::DirectorySeparatorChar
    if(-not $resolved.StartsWith($parent,[StringComparison]::OrdinalIgnoreCase) -or (Split-Path -Leaf $resolved) -cne $expected){throw 'Refuse remote cleanup outside this exact temporary leaf'}
    if((Get-Item -LiteralPath $resolved).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Refuse reparse remote leaf'}
    Remove-Item -LiteralPath $resolved -Recurse -Force
   } @($node.remote_work_root,$node.work_directory,(Split-Path -Leaf $node.work_directory)) | Out-Null
  }
  # 固定路径只用于本次VM helper；全部节点正常退出后清理，宿主机标准构建输出继续保留。
  foreach($node in $tx21Saved.nodes){if($node.computer -ne 'localhost' -and $node.agent_executable){
   In-Node $node {
    param($root,$path)
    $expected=[IO.Path]::GetFullPath((Join-Path $root 'node-tools/harness.exe'))
    if([IO.Path]::GetFullPath($path) -ine $expected){throw 'Refuse stable helper cleanup outside explicit root'}
    $parent=Split-Path -Parent $expected
    if((Test-Path -LiteralPath $parent) -and ((Get-Item -LiteralPath $parent).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Refuse reparse stable helper cleanup'}
    if(Test-Path -LiteralPath $expected -PathType Leaf){Remove-Item -LiteralPath $expected -Force}
    if((Test-Path -LiteralPath $parent -PathType Container) -and @(Get-ChildItem -LiteralPath $parent -Force).Count -eq 0){Remove-Item -LiteralPath $parent -Force}
   } @($node.remote_work_root,$node.agent_executable) | Out-Null
  }}
  $record=Get-Content -LiteralPath $tx21Owner -Raw | ConvertFrom-Json
  if($record.path -cne $tx21Run -or $record.tx -ne 'tx-04-21' -or $tx21Run -eq $tx21Repo){throw 'Refuse local cleanup without exact temporary owner'}
  if($tx21Phase -eq 'p2'){
   # P4尚待后续设备；只保留续验所需的成功/失败事实与owner，清理已还原的包、采样和备份。
   foreach($item in Get-ChildItem -LiteralPath $tx21Run -Force){
    if($item.Name -in @('tx21-run.json','progress.json')){continue}
    $resolved=[IO.Path]::GetFullPath($item.FullName)
    if(-not $resolved.StartsWith($tx21Run+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Refuse continuation cleanup outside ordinary temporary leaf'}
    Remove-Item -LiteralPath $resolved -Recurse -Force
   }
   $tx21Saved.nodes=@();$tx21Saved.history_nodes=@();$tx21Saved.phase='p2-complete-p4-pending'
   $tx21Saved | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $tx21Owner -Encoding utf8
   Write-Host "P2完整E01—E26已完成，部署/设置已还原，临时包和采样已清理。P4未执行；仅保留同tx续验进度：$tx21Run"
  }else{
   Remove-Item -LiteralPath $tx21Run -Recurse -Force
   Write-Host 'tx-04-21 P2/P4整体验收完成，部署/选项/焦点相关设置已还原，本次临时材料已清理。'
  }
 }else{Write-Host "同tx必要恢复材料：$tx21Run；保留实际失败、恢复节点与续测位置。"}
 foreach($session in $tx21Sessions.Values){Remove-PSSession -Session $session}
}
