param([string]$OutputDirectory)
$ErrorActionPreference='Stop'
$packageRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if(-not $OutputDirectory){$OutputDirectory=Join-Path $packageRepo 'out/product'}
elseif(-not [IO.Path]::IsPathRooted($OutputDirectory)){$OutputDirectory=Join-Path $packageRepo $OutputDirectory}
$packageTarget=[IO.Path]::GetFullPath($OutputDirectory)
# 产品只采用显式清单；不从包含Rust/fixture程序的out目录整体复制，不覆盖已有安装树。
if(Test-Path -LiteralPath $packageTarget){throw 'Product destination must be a new directory'}
$packageFiles=@(
    @('out/repentogon/zhlIsaacCS.dll','Repentogon/zhlIsaacCS.dll'),
    @('out/repentogon/replication-catalog.json','Repentogon/isaac-cs/replication-catalog.json'),
    @('out/repentogon/room-grid-application.json','Repentogon/isaac-cs/room-grid-application.json')
)
foreach($entry in $packageFiles){if(-not (Test-Path -LiteralPath (Join-Path $packageRepo $entry[0]) -PathType Leaf)){throw "Missing product build input: $($entry[0])"}}
foreach($entry in $packageFiles){
    $destination=Join-Path $packageTarget $entry[1]
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $packageRepo $entry[0]) -Destination $destination
}
foreach($entry in @(@('ui/isaac-cs-ui','mods/isaac-cs-ui'),@('out/repentogon/isaac-cs-observation','mods/isaac-cs-observation'))){
    $source=Join-Path $packageRepo $entry[0]
    if(-not (Test-Path -LiteralPath (Join-Path $source 'main.lua') -PathType Leaf)){throw "Missing product Lua: $source"}
    $destination=Join-Path $packageTarget $entry[1]
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Copy-Item -Path (Join-Path $source '*') -Destination $destination -Recurse
}
# 默认Off、关闭录制/详细诊断/控制口，且没有E2E token；启用沿正常产品配置完成。
Copy-Item -LiteralPath (Join-Path $packageRepo 'mod/config.example.json') -Destination (Join-Path $packageTarget 'Repentogon/isaac-cs/config.json')
Write-Output $packageTarget
