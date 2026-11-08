param(
 [Parameter(Mandatory=$true)][ValidateSet('Prepare','Inspect','Restore')][string]$Action,
 [Parameter(Mandatory=$true)][string]$ComputerName,
 [Parameter(Mandatory=$true)][string]$UserName,
 [Parameter(Mandatory=$true)][string]$GameRoot,
 [Parameter(Mandatory=$true)][string]$ExpectedUserSid,
 [switch]$UseSavedCredential
)
$ErrorActionPreference='Stop'
$connectionRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$connectionRoot=Join-Path $connectionRepo 'out/tx-04-21-connection'
$statePath=Join-Path $connectionRoot 'state.json'
$credentialPath=Join-Path $connectionRoot 'credential.clixml'
$inspectionPath=Join-Path $connectionRoot 'inspection.json'
$statusPath=Join-Path $connectionRoot 'status.json'
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
if($identity.User.Value -ne $ExpectedUserSid){throw '请使用发起本次验收的同一个Windows用户确认管理员权限；凭据仅可由该用户解密。'}
if([Uri]::CheckHostName($ComputerName) -ne [UriHostNameType]::IPv4){throw '本次临时信任只接受明确的VM IPv4地址'}
if((Test-Path -LiteralPath $connectionRoot) -and ((Get-Item -LiteralPath $connectionRoot).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw '连接材料目录不能是重解析点'}
New-Item -ItemType Directory -Path $connectionRoot -Force | Out-Null
function Write-ConnectionJson([string]$Path,$Value){$Value | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $Path -Encoding utf8}
function Restore-Connection($state){
 # 宿主机规则也按本次精确名称、端口和地址还原，不通过界面放行整个程序或网络。
 foreach($record in @($state.host_firewall_rules)){
  if(-not $record){continue}
  if($record.name -cnotmatch '^IsaacCS-tx21-[0-9a-f]{32}-host-(agent|proxy|game|harness)$'){throw '宿主机临时规则名称不属于本次测试'}
  $rule=Get-NetFirewallRule -Name $record.name -ErrorAction SilentlyContinue
  if($rule){
   $ports=$rule|Get-NetFirewallPortFilter;$addresses=$rule|Get-NetFirewallAddressFilter
   if([string]$rule.Direction -ne 'Inbound' -or [string]$rule.Action -ne 'Allow' -or [string]$ports.LocalPort -ne [string]$record.port -or ((@($addresses.RemoteAddress)|Sort-Object) -join ',') -ne ((@($record.remote_addresses)|Sort-Object) -join ',')){throw '宿主机临时规则已被其它操作修改，请核对'}
   if($record.program -and [string](($rule|Get-NetFirewallApplicationFilter).Program) -ine [string]$record.program){throw '宿主机测试工具规则的程序路径已被修改'}
   Remove-NetFirewallRule -Name $record.name
  }
 }
 # 用户单独授权临时禁用的本次helper阻止规则，按原启用状态恢复，不扩大到其它程序。
 if($state.disabled_firewall_rules){
  $restoreCredential=Import-Clixml -LiteralPath $credentialPath
  $restoreSession=New-PSSession -ComputerName $state.computer -Authentication Negotiate -Credential $restoreCredential
  try{
   Invoke-Command -Session $restoreSession -ScriptBlock {
    param($records)
    foreach($record in $records){
     $rule=Get-NetFirewallRule -Name $record.name -ErrorAction Stop
     $app=$rule|Get-NetFirewallApplicationFilter
     if([string]$app.Program -ine $record.program -or [string]$rule.Action -ne 'Block' -or [string]$rule.Direction -ne 'Inbound'){throw '原helper阻止规则已被其它操作修改，请人工核对'}
     Set-NetFirewallRule -Name $record.name -Enabled $record.enabled
    }
   } -ArgumentList (,$state.disabled_firewall_rules)
  }finally{Remove-PSSession -Session $restoreSession}
 }
 # 先经仍有效的本次连接撤销VM临时规则，再还原宿主机信任与服务；规则缺席时不触碰其它规则。
 if($state.firewall_rule){
  if($state.firewall_rule.name -cnotmatch '^IsaacCS-tx21-[0-9a-f]{32}-agent$'){throw '临时防火墙记录的名称不属于本次节点'}
  $restoreCredential=Import-Clixml -LiteralPath $credentialPath
  $restoreSession=New-PSSession -ComputerName $state.computer -Authentication Negotiate -Credential $restoreCredential
  try{
   Invoke-Command -Session $restoreSession -ScriptBlock {
    param($name,$remoteAddress,$port)
    $rule=Get-NetFirewallRule -Name $name -ErrorAction SilentlyContinue
    if($rule){
     $ports=$rule|Get-NetFirewallPortFilter;$addresses=$rule|Get-NetFirewallAddressFilter
     if([string]$rule.Direction -ne 'Inbound' -or [string]$rule.Action -ne 'Allow' -or [string]$ports.LocalPort -ne [string]$port -or @($addresses.RemoteAddress).Count -ne 1 -or [string]$addresses.RemoteAddress -ne $remoteAddress){throw '临时规则已被其它操作修改，请人工核对'}
     Remove-NetFirewallRule -Name $name
    }
   } -ArgumentList $state.firewall_rule.name,$state.firewall_rule.remote_address,$state.firewall_rule.port
  }finally{Remove-PSSession -Session $restoreSession}
 }
 # 仅还原本次实际设置的精确值；用户中途另行修改时保留现场，不覆盖其它配置。
 if($state.trust_captured){
  $current=[string](Get-Item -LiteralPath 'WSMan:\localhost\Client\TrustedHosts').Value
  if($current -cne $state.applied_trust -and $current -cne $state.original_trust){throw 'TrustedHosts已被其它操作修改，请人工核对后还原'}
  if($current -cne $state.original_trust){Set-Item -LiteralPath 'WSMan:\localhost\Client\TrustedHosts' -Value ([string]$state.original_trust) -Force}
 }
 if($state.original_service -eq 'Stopped' -and (Get-Service -Name WinRM).Status -ne 'Stopped'){Stop-Service -Name WinRM}
 $state.restored=$true
 Write-ConnectionJson $statePath $state
 if(Test-Path -LiteralPath $credentialPath -PathType Leaf){Remove-Item -LiteralPath $credentialPath -Force}
}
try{
 Import-Module Microsoft.WSMan.Management
 if($Action -eq 'Restore'){
  $state=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json -AsHashtable
  if($state.computer -ne $ComputerName -or $state.user_sid -ne $ExpectedUserSid){throw '还原目标与本次连接记录不符'}
  Restore-Connection $state
  Write-ConnectionJson $statusPath @{state='restored';computer=$ComputerName;credentials_removed=$true}
  exit 0
 }
 if($Action -eq 'Prepare'){
  # 无人值守续验仅复用本次用户在Windows凭据窗口新保存的DPAPI凭据，不把密码送入命令或日志。
  if($UseSavedCredential){
   $credential=Import-Clixml -LiteralPath $credentialPath
   if($credential -isnot [Management.Automation.PSCredential]){throw '本次本机凭据文件不是PSCredential'}
  }
  $priorAttempts=@()
  if(Test-Path -LiteralPath $statePath){
   $previous=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json -AsHashtable
   if(-not $previous.restored -or $previous.computer -ne $ComputerName -or $previous.user_sid -ne $ExpectedUserSid){throw '已有未还原或不同目标的连接记录；请先Restore，不覆盖原配置备份'}
   # 凭据失败后允许同tx再次输入，保留实际首因，不重新放宽配置范围。
   $priorAttempts=@($previous.prior_attempts)
   if(Test-Path -LiteralPath $statusPath){$priorAttempts+=,(Get-Content -LiteralPath $statusPath -Raw | ConvertFrom-Json -AsHashtable)}
  }
  $state=@{computer=$ComputerName;user_sid=$ExpectedUserSid;original_service=[string](Get-Service -Name WinRM).Status;trust_captured=$false;restored=$false;prior_attempts=$priorAttempts}
  Write-ConnectionJson $statePath $state
  # 用户明确授权后才临时启动服务，并在原列表中加入唯一指定地址。
  Start-Service -Name WinRM
  $original=[string](Get-Item -LiteralPath 'WSMan:\localhost\Client\TrustedHosts').Value
  $hosts=@($original -split ',' | ForEach-Object {$_.Trim()} | Where-Object {$_})
  $state.original_trust=$original
  $state.applied_trust=if($ComputerName -in $hosts){$original}else{(@($hosts)+$ComputerName) -join ','}
  $state.trust_captured=$true
  Write-ConnectionJson $statePath $state
  Set-Item -LiteralPath 'WSMan:\localhost\Client\TrustedHosts' -Value $state.applied_trust -Force
  Write-ConnectionJson $statusPath @{state='awaiting-local-credential';computer=$ComputerName}
  # Windows上的Export-Clixml由DPAPI保护口令；明文不进入拓扑、命令参数或输出。
  if(-not $UseSavedCredential){$credential=Get-Credential -UserName $UserName -Message 'tx-04-21：请输入VM的Windows账户密码，仅用于本次远程验收。'}
  if(-not $credential){throw '用户未提供VM凭据'}
  $credential | Export-Clixml -LiteralPath $credentialPath
 }else{$credential=Import-Clixml -LiteralPath $credentialPath}
 $session=New-PSSession -ComputerName $ComputerName -Authentication Negotiate -Credential $credential
 try{
  # 这里只读设备与安装事实；不部署、不启动游戏、不运行玩法程序。
  $inspection=Invoke-Command -Session $session -ScriptBlock {
   param($game)
   $machine=Get-CimInstance Win32_ComputerSystem
   $save=Join-Path $env:USERPROFILE 'Documents/My Games/Binding of Isaac Repentance+'
   [pscustomobject]@{
    computer=$env:COMPUTERNAME;user=$env:USERNAME;interactive_user=$machine.UserName
    processors=$machine.NumberOfLogicalProcessors;memory_bytes=$machine.TotalPhysicalMemory
    game_root=$game;game_exists=Test-Path -LiteralPath (Join-Path $game 'isaac-ng.exe')
    repentogon_exists=Test-Path -LiteralPath (Join-Path $game 'Repentogon/isaac-ng.exe')
    loader_exists=Test-Path -LiteralPath (Join-Path $game 'Repentogon/zhlLoader.dll')
    save_directory=$save;save_exists=Test-Path -LiteralPath $save
    savedata_declaration=@(Get-Content -LiteralPath (Join-Path $game 'Repentogon/savedatapath.txt') -ErrorAction SilentlyContinue)
    version_lines=@(Get-Content -LiteralPath (Join-Path $save 'log.txt') -TotalCount 35 -ErrorAction SilentlyContinue | Select-String -Pattern 'Game Version|Command Line' | ForEach-Object {$_.Line})
    game_processes=@(Get-Process -Name isaac-ng -ErrorAction SilentlyContinue | Select-Object Id,Path,MainWindowTitle)
    ipv4=@([Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces() | Where-Object {$_.OperationalStatus -eq 'Up'} | ForEach-Object {$_.GetIPProperties().UnicastAddresses} | Where-Object {$_.Address.AddressFamily -eq 'InterNetwork'} | ForEach-Object {$_.Address.ToString()})
   }
  } -ArgumentList $GameRoot
  $inspection | Select-Object * -ExcludeProperty PSComputerName,RunspaceId,PSShowComputerName | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $inspectionPath -Encoding utf8
 }finally{Remove-PSSession -Session $session}
 Write-ConnectionJson $statusPath @{state='inspected';computer=$ComputerName;inspection=$inspectionPath;credential_path=$credentialPath}
 Write-Host 'VM只读检查完成。请返回Codex；游戏尚未启动。'
}catch{
 # 失败时保留精确恢复记录，凭据/服务/信任按本次原值还原。
 $failure=$_.Exception.Message
 if($Action -eq 'Prepare' -and $state){try{Restore-Connection $state}catch{$failure+="；自动还原尚未完成：$($_.Exception.Message)"}}
 Write-ConnectionJson $statusPath @{state='failed';computer=$ComputerName;error=$failure}
 Write-Error $failure
 exit 1
}
