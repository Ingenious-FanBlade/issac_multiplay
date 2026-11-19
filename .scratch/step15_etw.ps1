param([string]$Name,[int]$Seconds=20)
# 仅在性能计时结束后采集CPU事件；失败也关闭本脚本成功创建的会话。
$ErrorActionPreference='Stop'
$profiler='C:/Program Files (x86)/Windows Kits/10/Windows Performance Toolkit/xperf.exe'
$kernel=Join-Path $PSScriptRoot ($Name+'-kernel.etl')
$merged=Join-Path $PSScriptRoot ($Name+'.etl')
& $profiler -on PROC_THREAD+LOADER+PROFILE -stackwalk Profile -buffersize 1024 -minbuffers 16 -maxbuffers 64 -f $kernel
if($LASTEXITCODE){exit $LASTEXITCODE}
try {Start-Sleep -Seconds $Seconds}
finally {& $profiler -d $merged}
if($LASTEXITCODE){exit $LASTEXITCODE}
Write-Output $merged
