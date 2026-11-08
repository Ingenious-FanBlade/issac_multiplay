param(
 [Parameter(Mandatory=$true)][string]$ExpectedUserSid,
 [Parameter(Mandatory=$true)][guid]$VmId
)
$ErrorActionPreference='Stop'
if([Security.Principal.WindowsIdentity]::GetCurrent().User.Value -ne $ExpectedUserSid){throw '请使用发起本次诊断的同一个Windows用户'}
$inspectRoot=Join-Path ([IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))) 'out/tx-04-21-p2-20261106-01/vm-crash-diagnosis'
try{
 # 仅按已经核实的VM ID读取宿主机GPU-P配置与版本，不修改虚拟机或驱动。
 $vm=Get-VM -Id $VmId
 $os=Get-CimInstance Win32_OperatingSystem
 $version=Get-ItemProperty -LiteralPath 'HKLM:/SOFTWARE/Microsoft/Windows NT/CurrentVersion'
 $adapters=@(Get-VMGpuPartitionAdapter -VMName $vm.Name|Select-Object VMName,Id,InstancePath,PartitionId,MinPartitionVRAM,MaxPartitionVRAM,OptimalPartitionVRAM,MinPartitionCompute,MaxPartitionCompute,OptimalPartitionCompute)
 $partitionable=@(Get-VMHostPartitionableGpu|Select-Object Name,ValidPartitionCounts,PartitionCount,TotalVRAM,AvailableVRAM,TotalCompute,AvailableCompute)
 $drivers=@(Get-CimInstance Win32_PnPSignedDriver|Where-Object {$_.DeviceClass -eq 'DISPLAY'}|Select-Object DeviceName,DriverVersion,DriverProviderName,DriverDate,InfName,IsSigned)
 # 只读磁盘映射，空间不足时先核实具体VM和磁盘，不猜测路径或修改分区。
 $disks=@(Get-VMHardDiskDrive -VMName $vm.Name|ForEach-Object {$drive=$_;$vhd=Get-VHD -Path $drive.Path;[pscustomobject]@{path=$drive.Path;controller_type=[string]$drive.ControllerType;controller_number=$drive.ControllerNumber;controller_location=$drive.ControllerLocation;type=[string]$vhd.VhdType;size=$vhd.Size;file_size=$vhd.FileSize;minimum_size=$vhd.MinimumSize;parent_path=$vhd.ParentPath}})
 $data=@{host_os=@{caption=$os.Caption;build=$os.BuildNumber;ubr=$version.UBR;display_version=$version.DisplayVersion};host_dxgkrnl=(Get-Item -LiteralPath "$env:SystemRoot/System32/drivers/dxgkrnl.sys").VersionInfo.FileVersion;vm=$vm|Select-Object Name,Id,State,Version,Generation,GuestControlledCacheTypes,LowMemoryMappedIoSpace,HighMemoryMappedIoSpace;gpu_partition_adapters=$adapters;partitionable_gpus=$partitionable;display_drivers=$drivers;disks=$disks}
 $data|ConvertTo-Json -Depth 10|Set-Content -LiteralPath (Join-Path $inspectRoot 'host-hyperv-gpu.json') -Encoding utf8
}catch{@{error=$_.Exception.Message}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $inspectRoot 'host-hyperv-gpu.json') -Encoding utf8;throw}
