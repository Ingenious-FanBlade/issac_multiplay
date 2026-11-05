param(
 [Parameter(Mandatory=$true)][ValidateSet('Install','Restore')][string]$Action,
 [Parameter(Mandatory=$true)][string]$GameRoot,
 [Parameter(Mandatory=$true)][string]$BackupDirectory,
 [string]$PackageDirectory
)
$ErrorActionPreference='Stop'
$tx21Game=[IO.Path]::GetFullPath($GameRoot)
$tx21Backup=[IO.Path]::GetFullPath($BackupDirectory)
$tx21Record=Join-Path $tx21Backup 'deployment-state.json'
if($tx21Backup.StartsWith($tx21Game+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Backup must be outside installation'}
# 每个目标先核对绝对边界；还原仅处理此次显式部署过的文件，不递归删除安装目录。
function Target([string]$relative){
 $resolved=[IO.Path]::GetFullPath((Join-Path $tx21Game $relative))
 if(-not $resolved.StartsWith($tx21Game+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Deployment path escaped game root'}
 return $resolved
}
if($Action -eq 'Install'){
 if(-not (Test-Path -LiteralPath (Join-Path $tx21Game 'isaac-ng.exe') -PathType Leaf)){throw 'Explicit real game installation required'}
 if(Test-Path -LiteralPath $tx21Backup){throw 'Use a new backup directory'}
 $tx21Package=[IO.Path]::GetFullPath($PackageDirectory)
 $manifest=Get-Content -LiteralPath (Join-Path $tx21Package 'deployment.json') -Raw | ConvertFrom-Json
 if($manifest.format -ne 'isaac.cs.deployment' -or $manifest.version -ne 1){throw 'Deployment manifest required'}
 New-Item -ItemType Directory -Path $tx21Backup | Out-Null
 $state=@{game=$tx21Game;package=$tx21Package;build=$manifest.build;files=@();complete=$false}
 foreach($file in $manifest.files){
  if($file.relative -like 'e2e-tools*'){continue}
  $destination=Target $file.relative
  $source=[IO.Path]::GetFullPath((Join-Path $tx21Package $file.relative))
  if(-not $source.StartsWith($tx21Package+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Package path escaped root'}
  $backup=Join-Path $tx21Backup ('original/'+$file.relative)
  $existed=Test-Path -LiteralPath $destination -PathType Leaf
  if($existed){New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null;Copy-Item -LiteralPath $destination -Destination $backup}
  $state.files+=@{relative=$file.relative;existed=$existed;original_source=if($existed){$destination}else{$null}}
  $state | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $tx21Record -Encoding utf8
  New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
  Copy-Item -LiteralPath $source -Destination $destination -Force
 }
 $state.complete=$true
 $state | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $tx21Record -Encoding utf8
}else{
 $state=Get-Content -LiteralPath $tx21Record -Raw | ConvertFrom-Json
 if($state.game -cne $tx21Game){throw 'Restore installation differs from recorded destination'}
 foreach($file in $state.files){
  $destination=Target $file.relative
  if($file.existed){$backup=Join-Path $tx21Backup ('original/'+$file.relative);Copy-Item -LiteralPath $backup -Destination $destination -Force}
  elseif(Test-Path -LiteralPath $destination -PathType Leaf){Remove-Item -LiteralPath $destination -Force}
 }
}
