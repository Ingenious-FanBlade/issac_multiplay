param([Parameter(Mandatory=$true)][string]$OutputDirectory)
# Pure compilation, used only by the unified tx-03-06 acceptance entry.
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskPublicationOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if(Test-Path -LiteralPath $taskPublicationOutput) {throw 'Use a new build directory.'}
New-Item -ItemType Directory -Path $taskPublicationOutput | Out-Null
& (Join-Path $PSScriptRoot 'build-t09-b.ps1') -OutputDirectory "$OutputDirectory/production" -Transaction 'tx-03-06'
$taskVsWhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$taskVsRoot=& $taskVsWhere -latest -version '[17.0,18.0)' -products * -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
if(-not $taskVsRoot) {throw 'VS 2022/v143 is required.'}
$taskMsBuild=Join-Path $taskVsRoot 'MSBuild/Current/Bin/amd64/MSBuild.exe'
$taskToolsVersion=(Get-Content -LiteralPath (Join-Path $taskVsRoot 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
$taskBin=(Join-Path $taskPublicationOutput 'Publication/bin')+'\'
$taskObj=(Join-Path $taskPublicationOutput 'Publication/obj')+'\'
& $taskMsBuild (Join-Path $PSScriptRoot 'native/IsaacCsT0.T09PublicationScenario.vcxproj') /m:1 /nr:false /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:PreferredToolArchitecture=x64 "/p:VCToolsVersion=$taskToolsVersion" "/p:OutDir=$taskBin" "/p:IntDir=$taskObj" 2>&1 |
    Tee-Object -FilePath (Join-Path $taskPublicationOutput 'Publication-build.log')
if($LASTEXITCODE -ne 0) {throw "Publication scenario compilation failed ($LASTEXITCODE); no scenario executed."}
