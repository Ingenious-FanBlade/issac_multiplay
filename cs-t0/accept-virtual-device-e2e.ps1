param(
    [Parameter(Mandatory=$true)][string]$GameExe,
    [Parameter(Mandatory=$true)][string]$ArtifactDirectory,
    [Parameter(Mandatory=$true)][string]$Authorization,
    [string]$BCharacter = 'native',
    [string]$Script = (Join-Path $PSScriptRoot 'testdata\tx-01-45-coop-script.json'),
    [string]$Transaction = 'tx-01-48'
)

$ErrorActionPreference = 'Stop'
$casePath = [IO.Path]::GetFullPath($ArtifactDirectory)
if (Test-Path -LiteralPath $casePath) { throw 'ArtifactDirectory must be new; an acceptance is never automatically repeated.' }
if ([string]::IsNullOrWhiteSpace($Authorization)) { throw 'Record the actual authorization for the isolated run and operations.' }
New-Item -ItemType Directory -Path $casePath | Out-Null
$buildLog = Join-Path $casePath 'build.log'
$capture = Join-Path $casePath 'capture'
$failure = $null
$entryStage = 'toolchain-selection'
$originalPath = $env:Path
@{
    tx=$Transaction; feature='tx-01-45'; authorization=$Authorization
    gameExe=$GameExe; script=[IO.Path]::GetFullPath($Script); artifactDirectory=$casePath
    bCharacter=$BCharacter; coopIpcVersion=3
    scope='Build and complete virtual-device real-game acceptance with fresh native artifacts.'
    automaticRetry=$false
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'invocation.json') -Encoding utf8
Copy-Item -LiteralPath $Script -Destination (Join-Path $casePath 'scenario-input.json')
try {
    # v143 is installed with VS 2022. Do not select a newer MSBuild installation
    # and let it discover an older compiler/PDB toolchain across installations.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsRoot = & $vswhere -latest -version '[17.0,18.0)' -products * -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
    if (-not $vsRoot) { throw 'VS 2022 with the v143 x86/x64 tools is required.' }
    $msbuild = Join-Path $vsRoot 'MSBuild\Current\Bin\amd64\MSBuild.exe'
    $toolsVersion = (Get-Content -LiteralPath (Join-Path $vsRoot 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
    $toolsRoot = Join-Path $vsRoot "VC\Tools\MSVC\$toolsVersion"
    $toolBin = Join-Path $toolsRoot 'bin\Hostx64\x86'
    $components = @($msbuild) + @('cl.exe','link.exe','mspdb140.dll','mspdbcore.dll','mspdbsrv.exe','c2.dll' | ForEach-Object { Join-Path $toolBin $_ })
    $toolchain = @($components | ForEach-Object {
        $component = Get-Item -LiteralPath $_ -ErrorAction Stop
        @{path=$component.FullName; version=$component.VersionInfo.FileVersion}
    })
    @{visualStudio=$vsRoot; toolsVersion=$toolsVersion; host='x64'; target='Win32'; components=$toolchain} |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $casePath 'toolchain.json') -Encoding utf8
    # Only this entry's children inherit the selected tool directory. Keep system
    # installations and other build processes unchanged.
    $env:Path = "$toolBin;$(Join-Path $toolsRoot 'bin\Hostx64\x64');$originalPath"
    "VS 2022: $vsRoot; v143: $toolsVersion; Hostx64 -> Win32; fresh per-invocation objects/PDBs." |
        Tee-Object -FilePath $buildLog -Append
    $entryStage = 'rust-build'
    # One unified acceptance invocation; no old build.ps1, tests, smoke or digests.
    & cargo build --manifest-path (Join-Path $PSScriptRoot 'Cargo.toml') --locked --release 2>&1 | Tee-Object -FilePath $buildLog -Append
    if ($LASTEXITCODE -ne 0) { throw "Rust build failed ($LASTEXITCODE); no retry." }
    foreach ($target in @('Native','Helper')) {
        $project = "IsaacCsT0.$target.vcxproj"
        $entryStage = "$target-build"
        $targetRoot = Join-Path $casePath "build\$target"
        $binaryDirectory = (Join-Path $targetRoot 'bin') + '\'
        $objectDirectory = (Join-Path $targetRoot 'obj') + '\'
        & $msbuild (Join-Path $PSScriptRoot "native\$project") /m:1 /nr:false /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:PreferredToolArchitecture=x64 "/p:VCToolsVersion=$toolsVersion" "/p:OutDir=$binaryDirectory" "/p:IntDir=$objectDirectory" 2>&1 | Tee-Object -FilePath $buildLog -Append
        if ($LASTEXITCODE -ne 0) { throw "$project build failed ($LASTEXITCODE); no retry." }
    }
    $native = Join-Path $casePath 'build\Native\bin\IsaacCsT0.Native.dll'
    $helper = Join-Path $casePath 'build\Helper\bin\IsaacCsT0.Helper.exe'
    $controller = Join-Path $PSScriptRoot 'target\release\IsaacCsT0.exe'
    $entryStage = 'native-disassembly'
    & (Join-Path $toolBin 'dumpbin.exe') /nologo /disasm:bytes $native |
        Set-Content -LiteralPath (Join-Path $casePath 'native-disassembly.txt') -Encoding utf8
    if ($LASTEXITCODE -ne 0) { throw "Native disassembly failed ($LASTEXITCODE); no retry." }
    $entryStage = 'runtime'
    & $controller coop-active --game-exe $GameExe --native-dll $native --helper $helper --artifact-dir $capture --script $Script --authorization $Authorization --b-character $BCharacter
    if ($LASTEXITCODE -ne 0) { throw "E2E incomplete or failed ($LASTEXITCODE); inspect capture; no retry." }
}
catch {
    $failure = $_.Exception.Message
}
finally {
    $env:Path = $originalPath
    if (Test-Path -LiteralPath (Join-Path $capture 'result.json')) {
        Copy-Item -LiteralPath (Join-Path $capture 'result.json') -Destination (Join-Path $casePath 'result.json')
    }
    else {
        @{tx=$Transaction;feature='tx-01-45';status='FAIL';stage=$entryStage;error=$failure;runtime='NOT_RUN';automaticRetry=$false} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'result.json') -Encoding utf8
    }
}
if ($failure) { throw $failure }
