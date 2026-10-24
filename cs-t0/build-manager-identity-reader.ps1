param(
    [ValidateSet('Debug', 'Release', 'All')]
    [string]$Configuration = 'All'
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
$output = Join-Path $workspace 'out\cs-t0'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'vswhere.exe was not found; Visual Studio Build Tools 2022 are required'
}
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) {
    throw 'MSBuild.exe was not found by vswhere'
}

$nativeRoot = Join-Path $PSScriptRoot 'native'
$configurations = if ($Configuration -eq 'All') { @('Debug', 'Release') } else { @($Configuration) }
foreach ($current in $configurations) {
    foreach ($project in @('IsaacCsT0.ManagerIdentityReader.vcxproj',
                           'IsaacCsT0.ManagerIdentityReaderTests.vcxproj')) {
        & $msbuild (Join-Path $nativeRoot $project) /m /nologo /verbosity:minimal "/p:Configuration=$current" /p:Platform=Win32
        if ($LASTEXITCODE -ne 0) {
            throw "$project $current build failed with exit code $LASTEXITCODE"
        }
    }

    $testBinary = Join-Path $output "obj\manager-identity-reader-tests\$current\IsaacCsT0.ManagerIdentityReaderTests.exe"
    $productionDll = Join-Path $output "obj\manager-identity-reader\$current\IsaacCsT0.ManagerIdentityReader.dll"
    $testOutput = & $testBinary $productionDll
    $testOutput | Write-Output
    if ($LASTEXITCODE -ne 0 -or
        -not ($testOutput | Select-String -SimpleMatch '"identity":"IsaacCsT0-ManagerIdentityReaderHarness-v1"') -or
        -not ($testOutput | Select-String -SimpleMatch '"status":"PASS"')) {
        throw "manager-this production reader $current suite failed or returned the wrong marker"
    }
}

Get-FileHash -Algorithm SHA256 -LiteralPath (
    $configurations | ForEach-Object {
        Join-Path $output "obj\manager-identity-reader\$_\IsaacCsT0.ManagerIdentityReader.dll"
    }
)
