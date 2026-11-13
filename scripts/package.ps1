param([string]$OutputDirectory,[switch]$E2E)
$ErrorActionPreference='Stop'
$packageRepo=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if(-not $OutputDirectory){$OutputDirectory=Join-Path $packageRepo $(if($E2E){'out/e2e-package'}else{'out/product'})}
elseif(-not [IO.Path]::IsPathRooted($OutputDirectory)){$OutputDirectory=Join-Path $packageRepo $OutputDirectory}
$packageTarget=[IO.Path]::GetFullPath($OutputDirectory)
# 产品只采用显式清单；不从包含Rust/fixture程序的out目录整体复制，不覆盖已有安装树。
if(Test-Path -LiteralPath $packageTarget){throw 'Product destination must be a new directory'}
$inputRoot=if($E2E){'out/repentogon/e2e'}else{'out/repentogon'}
$expectedFlavor=if($E2E){'e2e'}else{'product'}
$flavor=Get-Content -LiteralPath (Join-Path $packageRepo "$inputRoot/build-flavor.json") -Raw | ConvertFrom-Json
if($flavor.format -ne 'isaac.cs.build' -or $flavor.version -ne 2 -or $flavor.flavor -ne $expectedFlavor -or $flavor.e2e_tools -ne [bool]$E2E -or $flavor.framing -ne 2 -or $flavor.stream_schema -ne 1){throw 'Matching binary state build flavor required'}
$receipt=Get-Content -LiteralPath (Join-Path $packageRepo "$inputRoot/build-complete.json") -Raw | ConvertFrom-Json
if($receipt.format -ne 'isaac.cs.build.complete' -or $receipt.flavor -ne $expectedFlavor){throw 'Completed matching build required'}
$packageFiles=@(
    @('out/repentogon/zhlIsaacCS.dll','Repentogon/zhlIsaacCS.dll'),
    @('out/repentogon/build-flavor.json','Repentogon/isaac-cs/build-flavor.json')
)
$packageFiles=@($packageFiles | ForEach-Object { ,@($_[0].Replace('out/repentogon',$inputRoot),$_[1]) })
$packageFiles+=,@("$inputRoot/zstd-LICENSE.txt",'Repentogon/isaac-cs/licenses/zstd-LICENSE.txt')
$packageFiles+=,@("$inputRoot/build-complete.json",'Repentogon/isaac-cs/build-complete.json')
foreach($entry in $packageFiles){if(-not (Test-Path -LiteralPath (Join-Path $packageRepo $entry[0]) -PathType Leaf)){throw "Missing product build input: $($entry[0])"}}
foreach($entry in $packageFiles){
    $destination=Join-Path $packageTarget $entry[1]
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $packageRepo $entry[0]) -Destination $destination
}
$luaPackages=@(@('ui/isaac-cs-ui','mods/isaac-cs-ui'))
if($E2E){
    $luaPackages+=,@("$inputRoot/isaac-cs-observation",'mods/isaac-cs-observation')
    $luaPackages+=,@("$inputRoot/isaac-cs-test",'mods/isaac-cs-test')}
foreach($entry in $luaPackages){
    $source=Join-Path $packageRepo $entry[0]
    if(-not (Test-Path -LiteralPath (Join-Path $source 'main.lua') -PathType Leaf)){throw "Missing product Lua: $source"}
    $destination=Join-Path $packageTarget $entry[1]
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Copy-Item -Path (Join-Path $source '*') -Destination $destination -Recurse
    if(-not $E2E -and $entry[0] -eq 'ui/isaac-cs-ui'){
        Remove-Item -LiteralPath (Join-Path $destination 'e2e.lua')
    }
}
# 默认Off、关闭录制/详细诊断/控制口，且没有E2E token；启用沿正常产品配置完成。
Copy-Item -LiteralPath (Join-Path $packageRepo 'mod/config.example.json') -Destination (Join-Path $packageTarget 'Repentogon/isaac-cs/config.json')
if($E2E){
    # 引导安装仍默认Off；显式启用C/S后才显示本机测试面板。节点启动会覆盖此专用设置。
    $configuration=Get-Content -LiteralPath (Join-Path $packageTarget 'Repentogon/isaac-cs/config.json') -Raw | ConvertFrom-Json -AsHashtable
    $configuration.e2e_ui=@{enabled=$true}
    $configuration | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $packageTarget 'Repentogon/isaac-cs/config.json') -Encoding utf8
}
# 工具放在独立目录，不属于游戏加载树；产品包没有测试程序。
if($E2E){
    New-Item -ItemType Directory -Path (Join-Path $packageTarget 'e2e-tools') -Force | Out-Null
    foreach($name in @('harness.exe','client-fixture.exe','replica-contract.exe')){Copy-Item -LiteralPath (Join-Path $packageRepo "$inputRoot/$name") -Destination (Join-Path $packageTarget "e2e-tools/$name")}
}
$manifest=@{format='isaac.cs.deployment';version=1;build=$flavor;runtime=@('official REPENTOGON 1.1.3','Repentance+ v1.9.7.12.J273','Microsoft Visual C++ v143 x86 /MD runtime');zstd='statically linked; licenses/zstd-LICENSE.txt';files=@()}
foreach($file in Get-ChildItem -LiteralPath $packageTarget -Recurse -File){$manifest.files+=@{relative=[IO.Path]::GetRelativePath($packageTarget,$file.FullName);bytes=$file.Length}}
$manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $packageTarget 'deployment.json') -Encoding utf8
Write-Output $packageTarget
