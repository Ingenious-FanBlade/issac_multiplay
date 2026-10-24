param([string]$OutputDirectory='out/cs-t0/tx-02-00-build',[string]$Transaction='tx-02-00')
# Pure compilation only; never executes tests, the game, or digest tools.
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskOutput=[IO.Path]::GetFullPath((Join-Path $taskRoot $OutputDirectory))
if(Test-Path -LiteralPath $taskOutput) {throw 'Use a new build directory.'}
New-Item -ItemType Directory -Path $taskOutput | Out-Null
$taskOriginalPath=$env:Path
try {
    $taskVsWhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $taskVsRoot=& $taskVsWhere -latest -version '[17.0,18.0)' -products * -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
    if(-not $taskVsRoot) {throw 'VS 2022/v143 is required.'}
    $taskMsBuild=Join-Path $taskVsRoot 'MSBuild/Current/Bin/amd64/MSBuild.exe'
    $taskToolsVersion=(Get-Content -LiteralPath (Join-Path $taskVsRoot 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
    $taskToolBin=Join-Path $taskVsRoot "VC/Tools/MSVC/$taskToolsVersion/bin/Hostx64/x86"
    $env:Path="$taskToolBin;$taskOriginalPath"
    & cargo build --manifest-path (Join-Path $PSScriptRoot 'Cargo.toml') --locked --release 2>&1 | Tee-Object -FilePath (Join-Path $taskOutput 'Rust-build.log')
    if($LASTEXITCODE -ne 0) {throw 'Rust compilation failed; no programs executed.'}
    New-Item -ItemType Directory -Path (Join-Path $taskOutput 'Rust') | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'target/release/IsaacCsT0.exe') -Destination (Join-Path $taskOutput 'Rust/IsaacCsT0.exe')
    foreach($taskTarget in @('Native','Helper','ProtocolFullScenario')) {
        $taskBin=(Join-Path $taskOutput "$taskTarget/bin")+'\'
        $taskObj=(Join-Path $taskOutput "$taskTarget/obj")+'\'
        & $taskMsBuild (Join-Path $PSScriptRoot "native/IsaacCsT0.$taskTarget.vcxproj") /m:1 /nr:false /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:PreferredToolArchitecture=x64 "/p:VCToolsVersion=$taskToolsVersion" "/p:OutDir=$taskBin" "/p:IntDir=$taskObj" 2>&1 | Tee-Object -FilePath (Join-Path $taskOutput "$taskTarget-build.log")
        if($LASTEXITCODE -ne 0) {throw "$taskTarget compilation failed; no programs executed."}
    }
    @{tx=$Transaction;scope='pure compilation';visualStudio=$taskVsRoot;toolsVersion=$taskToolsVersion;host='x64';target='Win32';automaticRetry=$false} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskOutput 'build.json') -Encoding utf8
} finally {$env:Path=$taskOriginalPath}
