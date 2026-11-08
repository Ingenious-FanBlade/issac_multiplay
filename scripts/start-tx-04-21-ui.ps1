param([int]$BootstrapPid=0,[switch]$Worker)
$ErrorActionPreference='Stop'
$uiRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$uiRoot=Join-Path $uiRepo 'out/tx-04-21-ui'
$uiUtf8=[Text.UTF8Encoding]::new($false)
if(-not (Test-Path -LiteralPath $uiRoot -PathType Container)){throw 'GUI mailbox must be created by the E2E panel'}
if((Get-Item -LiteralPath $uiRoot).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Refuse reparse GUI mailbox'}
foreach($path in @((Split-Path -Parent $uiRoot),(Join-Path $uiRoot 'responses'))){if((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Refuse reparse GUI parent/responses'}}

# 所有入口参数来自固定profile文件；用户文字不拼入命令或表达式。
function Write-UiJson([string]$Name,$Value){
 if($Name -eq 'lifecycle.json'){$Value.start_filetime=[string]((Get-Process -Id $PID).StartTime.ToUniversalTime().ToFileTimeUtc())}
 $path=Join-Path $uiRoot $Name
 $temporary=$path+'.new'
 [IO.File]::WriteAllText($temporary,($Value | ConvertTo-Json -Depth 16 -Compress),$uiUtf8)
 [IO.File]::Move($temporary,$path,$true)
}
function Read-UiJson([string]$Name){
 $path=Join-Path $uiRoot $Name
 if(Test-Path -LiteralPath $path -PathType Leaf){
  # 允许发布者替换/删除已读文件，读者只解析完整快照，不锁住Rust/原生worker。
  try{
   $stream=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
   try{
    $bytes=[byte[]]::new(131073);$count=0
    while($count -lt $bytes.Length){$read=$stream.Read($bytes,$count,$bytes.Length-$count);if($read -eq 0){break};$count+=$read}
    if($count -gt 131072){throw 'GUI mailbox size exceeded'}
    return $uiUtf8.GetString($bytes,0,$count) | ConvertFrom-Json -AsHashtable
   }finally{$stream.Dispose()}
  }catch{return $null}
 }return $null
}
if($Worker){
 try{
  $profile=Read-UiJson 'active-profile.json'
  if(-not $profile){throw 'Missing explicit GUI configuration'}
  Write-UiJson 'lifecycle.json' @{busy=$true;pid=$PID;message='请从游戏菜单正常退出当前引导游戏；后台将在实际退出后开始I39构建。';run_directory=$profile.run_directory}
  if($BootstrapPid -gt 0){
   $bootstrap=Get-Process -Id $BootstrapPid -ErrorAction SilentlyContinue
   if($bootstrap -and $bootstrap.ProcessName -ne 'isaac-ng'){throw 'Bootstrap PID is not the requesting game'}
   if($bootstrap){$bootstrap.WaitForExit()}
  }
  Write-UiJson 'lifecycle.json' @{busy=$true;pid=$PID;message='I39统一整体验收：构建、部署及连接真实节点；每次启动仍需准备确认。';run_directory=$profile.run_directory}
  $arguments=@{Topology=[string]$profile.topology;RunDirectory=[string]$profile.run_directory;UiDirectory=$uiRoot;UiSession=[string]$profile.session}
  if($profile.resume -eq $true){$arguments.Resume=$true;$arguments.ReuseReason=[string]$profile.reuse_reason;$arguments.Rerun=@($profile.rerun -split ',' | ForEach-Object {$_.Trim()} | Where-Object {$_})}
  & (Join-Path $PSScriptRoot 'run-tx-04-21.ps1') @arguments
  $finishedTopology=Get-Content -LiteralPath $profile.topology -Raw | ConvertFrom-Json -AsHashtable
  $finishedMessage=if($finishedTopology.phase -eq 'p2'){'P2完整验收及部署还原完成；P4未执行，保留必要续验进度。'}else{'P2/P4整体验收及部署还原完成。'}
  Write-UiJson 'lifecycle.json' @{busy=$false;pid=$PID;complete=$true;message=$finishedMessage;run_directory=$profile.run_directory}
 }catch{
  Write-UiJson 'lifecycle.json' @{busy=$false;pid=$PID;complete=$false;message=('必要中断：'+$_);run_directory=$profile.run_directory}
  exit 1
 }exit 0
}

# 窗口和编排分进程，构建/等待/游戏重启不会阻塞确认界面；互斥量阻止重复全量运行。
$uiMutex=[Threading.Mutex]::new($false,'Local\IsaacCS-tx04-21-UI')
$uiAcquired=$false
$child=$null
try{
 try{$uiAcquired=$uiMutex.WaitOne(0)}catch [Threading.AbandonedMutexException]{$uiAcquired=$true}
 if(-not $uiAcquired){throw 'A tx-04-21 GUI controller is already active'}
 $previous=Read-UiJson 'lifecycle.json'
 $previousProcess=if($previous.busy){Get-Process -Id ([int]$previous.pid) -ErrorAction SilentlyContinue}else{$null}
 if($previousProcess -and ([string]($previousProcess.StartTime.ToUniversalTime().ToFileTimeUtc())) -ceq $previous.start_filetime){throw 'Previous coordinator still running; do not start another attempt'}
 $profile=Read-UiJson 'profile.json'
 if(-not $profile -or [string]::IsNullOrWhiteSpace($profile.topology)){throw '请先填写实际拓扑文件路径'}
 if(-not (Test-Path -LiteralPath $profile.topology -PathType Leaf)){throw '实际拓扑文件不存在，请先修正面板中的路径'}
 $topology=Get-Content -LiteralPath $profile.topology -Raw | ConvertFrom-Json -AsHashtable
 $phase=if($topology.phase){[string]$topology.phase}else{'p2-p4'}
 if($topology.format -ne 'isaac.cs.tx21.topology' -or $phase -notin @('p2','p2-p4') -or $topology.nodes.Count -notin @(2,4) -or ($phase -eq 'p2-p4' -and $topology.nodes.Count -ne 4) -or $topology.nodes[0].id -ne 'host' -or $topology.nodes[0].computer -ne 'localhost'){throw 'GUI需要本机Host：p2为两个真实节点，p2-p4为四个真实节点'}
 $profile.session=[guid]::NewGuid().ToString('N')
 if([string]::IsNullOrWhiteSpace($profile.run_directory)){$profile.run_directory=Join-Path $uiRepo ('out/tx-04-21-'+[guid]::NewGuid().ToString('N'))}
 if($profile.resume -eq $true -and [string]::IsNullOrWhiteSpace($profile.reuse_reason)){throw '续跑必须填写未受影响结果复用原因'}
 Write-UiJson 'active-profile.json' $profile
 Write-UiJson 'profile.json' $profile
 Write-UiJson 'state.json' @{pending=$false;context='等待当前引导游戏正常退出'}
 Write-UiJson 'lifecycle.json' @{busy=$true;pid=$PID;message='正在启动隐藏编排进程';run_directory=$profile.run_directory}
 $pwsh=(Get-Process -Id $PID).Path
 $workerArguments='-NoProfile -WindowStyle Hidden -File "'+$PSCommandPath+'" -Worker -BootstrapPid '+$BootstrapPid
 Add-Type -AssemblyName System.Windows.Forms
 Add-Type -AssemblyName System.Drawing
 Add-Type -TypeDefinition @'
public class Tx21PassiveForm : System.Windows.Forms.Form {
    // 备用确认窗口不能自动抢走正在验证的物理输入焦点。
    protected override bool ShowWithoutActivation { get { return true; } }
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    public static extern System.IntPtr GetForegroundWindow();
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(System.IntPtr window, out uint process);
    [System.Runtime.InteropServices.DllImport("user32.dll")]
    public static extern bool ShowWindow(System.IntPtr window, int command);
    public void Reveal() { ShowWindow(Handle, 4); }
}
'@ -ReferencedAssemblies @([Windows.Forms.Form].Assembly.Location,[ComponentModel.Component].Assembly.Location,[Drawing.Size].Assembly.Location,'System.Runtime.dll')
 $form=[Tx21PassiveForm]::new();$form.Text='Isaac CS · tx-04-21';$form.Width=920;$form.Height=670
 $form.StartPosition='CenterScreen';$form.Font=[Drawing.Font]::new('Microsoft YaHei UI',10)
 $layout=[Windows.Forms.TableLayoutPanel]::new();$layout.Dock='Fill';$layout.ColumnCount=1;$layout.RowCount=5
 foreach($height in @(52,340,92,55,45)){$style=[Windows.Forms.RowStyle]::new();$style.SizeType='Absolute';$style.Height=$height;$layout.RowStyles.Add($style)|Out-Null}
 $status=[Windows.Forms.Label]::new();$status.Dock='Fill'
 $instruction=[Windows.Forms.TextBox]::new();$instruction.Multiline=$true;$instruction.ReadOnly=$true;$instruction.ScrollBars='Vertical';$instruction.Dock='Fill'
 $note=[Windows.Forms.TextBox]::new();$note.Multiline=$true;$note.Dock='Fill';$note.PlaceholderText='按提示填写实际观察、总审结论或修复说明；准备/完成确认无需说明。'
 $buttons=[Windows.Forms.FlowLayoutPanel]::new();$buttons.Dock='Fill'
 $accept=[Windows.Forms.Button]::new();$accept.Width=210;$accept.Height=38
 $fail=[Windows.Forms.Button]::new();$fail.Text='未观察到 / 不一致';$fail.Width=170;$fail.Height=38
 $stop=[Windows.Forms.Button]::new();$stop.Text='中断并保留恢复材料';$stop.Width=210;$stop.Height=38
 $buttons.Controls.AddRange(@($accept,$fail,$stop))
 $hint=[Windows.Forms.Label]::new();$hint.Dock='Fill';$hint.Text='房主游戏：Isaac CS → tx-04-21 acceptance。点击确认后若游戏仍在运行，请手动切回游戏；检测到游戏前台后才提交回答。'
 foreach($control in @($status,$instruction,$note,$buttons,$hint)){$layout.Controls.Add($control)}
 $form.Controls.Add($layout)
 $script:uiPending=$null;$script:uiSeen='';$script:uiFinished=$false;$script:uiArmed=$null
 function Send-UiAnswer([string]$Answer){
  $current=Read-UiJson 'state.json'
  if(-not $script:uiPending -or -not $current.pending -or $current.id -cne $script:uiPending.id){return}
  if($current.id -notmatch '^[0-9a-f]+-[0-9]+-[0-9]+$'){throw 'Invalid prompt identity'}
  $path=Join-Path $uiRoot ('responses/'+$current.id+'.json')
  $bytes=$uiUtf8.GetBytes((@{id=$current.id;answer=$Answer}|ConvertTo-Json -Compress))
  if($bytes.Length -gt 16384){throw '回答过长'}
  try{$stream=[IO.File]::Open($path,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,([IO.FileShare]::Read -bor [IO.FileShare]::Delete))}catch [IO.IOException]{return}
  try{$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}
  $accept.Enabled=$false;$fail.Enabled=$false;$stop.Enabled=$false
 }
 function Arm-UiAnswer([string]$Answer){
  if($Answer -eq 'stop' -or $Answer.StartsWith('fail ') -or -not (Get-Process -Name 'isaac-ng' -ErrorAction SilentlyContinue)){Send-UiAnswer $Answer;return}
  # 确认是明确的用户操作，随后只等待用户恢复前台；没有超时自动回答或抢焦点。
  $script:uiArmed=@{id=$script:uiPending.id;answer=$Answer}
  $status.Text='确认已暂存：请手动切回本机游戏，恢复前台后提交本条回答。'
  $form.WindowState='Minimized'
 }
 $accept.Add_Click({
  if(-not $script:uiPending){return}
  $kind=$script:uiPending.kind
  if($kind -in @('ready','done')){Arm-UiAnswer $kind;return}
  $min=if($kind -eq 'reviewed'){12}elseif($kind -eq 'retry'){6}else{12}
  if($note.Text.Trim().Length -lt $min){[Windows.Forms.MessageBox]::Show('请按当前提示填写具体说明，不能空白确认。')|Out-Null;return}
  Arm-UiAnswer $(if($kind -eq 'text'){$note.Text.Trim()}else{$kind+' '+$note.Text.Trim()})
 })
 $fail.Add_Click({if($note.Text.Trim()){Send-UiAnswer ('fail '+$note.Text.Trim())}})
 $stop.Add_Click({Send-UiAnswer 'stop'})
 $form.Add_FormClosing({param($sender,$event)if(-not $script:uiFinished){$event.Cancel=$true;$form.WindowState='Minimized'}})
 $timer=[Windows.Forms.Timer]::new();$timer.Interval=400
 $timer.Add_Tick({
  try{
   $life=Read-UiJson 'lifecycle.json';$state=Read-UiJson 'state.json'
   $script:uiFinished=$child.HasExited
   $status.Text=[string]$life.message+"`r`n"+[string]$state.context
   if($script:uiFinished){
    $accept.Enabled=$false;$fail.Enabled=$false;$stop.Enabled=$false
    if($life.busy){$status.Text='编排异常退出；请保留当前运行目录，修复后同tx续跑。'}
    $instruction.Text=$status.Text+"`r`n运行目录："+$life.run_directory+"`r`n详情："+(Join-Path $uiRoot 'controller-error.txt')
    if($form.WindowState -eq 'Minimized'){$form.Reveal()}
    if($life.complete){
     # 正常结束只保留界面配置/末态；中断才留输出、提示和本次恢复位置。
     foreach($name in @('controller-output.txt','controller-error.txt','active-profile.json','state.json','game-heartbeat')){
      $path=Join-Path $uiRoot $name;if(Test-Path -LiteralPath $path){Remove-Item -LiteralPath $path -Force}
     }
    }
    $timer.Stop();return
   }
   $script:uiPending=if($state.pending){$state}else{$null}
   if($script:uiArmed){
    if(-not $script:uiPending -or $state.id -cne $script:uiArmed.id){$script:uiArmed=$null}
    else{
     [uint32]$foregroundPid=0
     [Tx21PassiveForm]::GetWindowThreadProcessId([Tx21PassiveForm]::GetForegroundWindow(),[ref]$foregroundPid)|Out-Null
     $foreground=Get-Process -Id $foregroundPid -ErrorAction SilentlyContinue
     if(($foreground -and $foreground.ProcessName -eq 'isaac-ng') -or -not (Get-Process -Name 'isaac-ng' -ErrorAction SilentlyContinue)){
      Send-UiAnswer $script:uiArmed.answer;$script:uiArmed=$null;return
     }else{$status.Text='确认已暂存：请手动切回本机游戏，恢复前台后提交本条回答。';return}
    }
   }
   $pending=[bool]$script:uiPending
   $accept.Enabled=$pending;$fail.Enabled=$pending;$stop.Enabled=$pending
   if($pending){
    $instruction.Text=$state.instruction
    $accept.Text=switch($state.kind){ready{'已准备好：允许本次启动/操作'}done{'已完成操作并停手'}observed{'提交具体观察'}reviewed{'提交总审结论'}retry{'提交修复说明并续测'}default{'提交回答'}}
    if($script:uiSeen -cne $state.id){$script:uiSeen=$state.id;$note.Clear()}
    $heartbeat=Join-Path $uiRoot 'game-heartbeat'
    $inGame=(Test-Path -LiteralPath $heartbeat) -and (([DateTime]::UtcNow-(Get-Item -LiteralPath $heartbeat).LastWriteTimeUtc).TotalSeconds -lt 3)
    if(-not $inGame -and $form.WindowState -eq 'Minimized'){$form.Reveal()}
   }else{$instruction.Text=$status.Text}
  }catch{$status.Text='界面读取失败：'+$_}
 })
 # 备用窗口所需类型/控件完成后才启动编排，避免界面初始化失败留下无人确认的任务。
 $child=Start-Process -FilePath $pwsh -ArgumentList $workerArguments -WorkingDirectory $uiRepo -WindowStyle Hidden -RedirectStandardOutput (Join-Path $uiRoot 'controller-output.txt') -RedirectStandardError (Join-Path $uiRoot 'controller-error.txt') -PassThru
 $timer.Start();[Windows.Forms.Application]::Run($form)
 $timer.Dispose();$form.Dispose()
}catch{
 if($uiAcquired -and -not $child){Write-UiJson 'lifecycle.json' @{busy=$false;complete=$false;pid=$PID;message=('界面启动失败：'+$_)}}
 throw
}finally{if($uiAcquired){$uiMutex.ReleaseMutex()};$uiMutex.Dispose()}
