param(
 [Parameter(Mandatory=$true)][ValidateSet('Attach','Detach')][string]$Action,
 [Parameter(Mandatory=$true)][string]$ExpectedUserSid
)
$ErrorActionPreference='Stop'
if([Security.Principal.WindowsIdentity]::GetCurrent().User.Value -ne $ExpectedUserSid){throw '必须使用发起诊断的同一Windows用户'}
$diagnosticRoot=Join-Path ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))) 'out/tx-04-21-p2-20261106-01/vm-crash-diagnosis'
$statePath=Join-Path $diagnosticRoot 'vm-update-scratch-disk.json'
$vm=Get-VM -Id 'ED0FB69F-F538-487D-8A1D-4DA6AC5F8227'
$diskPath='F:\VM\VM\Virtual Hard Disks\IsaacCS-tx21-update-temp.vhdx'
$capacity=32GB
try{
 if($Action -eq 'Attach'){
  # 只创建独立的临时更新盘；既有系统盘、恢复分区与GPU配置不作修改。
  if((Test-Path -LiteralPath $statePath) -or (Test-Path -LiteralPath $diskPath)){throw '临时盘或恢复记录已经存在，请先检查，不覆盖'}
  $system=@(Get-VMHardDiskDrive -VMName $vm.Name)
  if($system.Count -ne 1 -or $system[0].Path -ine 'F:\VM\VM\Virtual Hard Disks\VM.vhdx'){throw 'VM系统盘映射已变化'}
  $occupied=@($system)+@(Get-VMDvdDrive -VMName $vm.Name)
  $slot=@(0..63|Where-Object {$candidate=$_;@($occupied|Where-Object {$_.ControllerType -eq 'SCSI' -and $_.ControllerNumber -eq 0 -and $_.ControllerLocation -eq $candidate}).Count -eq 0})[0]
  if($null -eq $slot){throw '没有空闲SCSI位置'}
  $state=@{vm_id=$vm.Id.ToString();vm_name=$vm.Name;path=$diskPath;capacity=$capacity;controller=0;location=$slot;stage='creating';guest_prepared=$false}
  $state|ConvertTo-Json|Set-Content -LiteralPath $statePath -Encoding utf8
  $vhd=New-VHD -Path $diskPath -SizeBytes $capacity -Dynamic
  $state.disk_identifier=$vhd.DiskIdentifier.ToString();$state.stage='created'
  $state|ConvertTo-Json|Set-Content -LiteralPath $statePath -Encoding utf8
  Add-VMHardDiskDrive -VMName $vm.Name -ControllerType SCSI -ControllerNumber 0 -ControllerLocation $slot -Path $diskPath
  $state.stage='attached'
 }else{
  # 客体已确认完成文件清理并将临时盘离线后才允许卸载删除，防止切断在用安装目录。
  $state=Get-Content -LiteralPath $statePath -Raw|ConvertFrom-Json -AsHashtable
  if($state.path -ine $diskPath -or $state.vm_id -ine $vm.Id.ToString() -or $state.guest_offline_confirmed -ne $true){throw '临时盘owner或客体离线确认不符'}
  $drive=@(Get-VMHardDiskDrive -VMName $vm.Name|Where-Object {$_.Path -ieq $diskPath})
  if($drive.Count -ne 1){throw '临时盘连接映射不唯一'}
  $vhd=Get-VHD -Path $diskPath
  if($vhd.Size -ne $capacity -or $vhd.DiskIdentifier.ToString() -ine $state.disk_identifier){throw '临时盘身份已变化'}
  Remove-VMHardDiskDrive -VMHardDiskDrive $drive[0]
  $state.stage='detached';$state|ConvertTo-Json|Set-Content -LiteralPath $statePath -Encoding utf8
  # 非递归删除已核实的本次独立VHDX，不对计算出的目录做删除。
  Remove-Item -LiteralPath $diskPath -Force
  $state.stage='removed'
 }
 $state|ConvertTo-Json|Set-Content -LiteralPath $statePath -Encoding utf8
}catch{
 @{action=$Action;error=$_.Exception.Message}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $diagnosticRoot 'vm-update-scratch-disk-error.json') -Encoding utf8
 throw
}
