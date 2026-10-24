param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$SkipRustTests
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
$manifest = Join-Path $PSScriptRoot 'Cargo.toml'
$output = Join-Path $workspace 'out\cs-t0'
$profile = if ($Configuration -eq 'Release') { 'release' } else { 'debug' }
$cargoArguments = @('build', '--manifest-path', $manifest, '--locked')
if ($Configuration -eq 'Release') {
    $cargoArguments += '--release'
}

if (-not $SkipRustTests) {
    & cargo test --manifest-path $manifest --locked
    if ($LASTEXITCODE -ne 0) {
        throw "cargo test failed with exit code $LASTEXITCODE"
    }
}

& cargo @cargoArguments
if ($LASTEXITCODE -ne 0) {
    throw "cargo build failed with exit code $LASTEXITCODE"
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'vswhere.exe was not found; Visual Studio Build Tools 2022 are required'
}
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) {
    throw 'MSBuild.exe was not found by vswhere'
}
$nativeRoot = Join-Path $PSScriptRoot 'native'
foreach ($project in @('IsaacCsT0.Native.vcxproj', 'IsaacCsT0.Helper.vcxproj')) {
    & $msbuild (Join-Path $nativeRoot $project) /m /nologo /verbosity:minimal "/p:Configuration=$Configuration" /p:Platform=Win32
    if ($LASTEXITCODE -ne 0) {
        throw "$project build failed with exit code $LASTEXITCODE"
    }
}
foreach ($testConfiguration in @('Debug', 'Release')) {
    $testProject = Join-Path $nativeRoot 'IsaacCsT0.NativeTests.vcxproj'
    & $msbuild $testProject /m /nologo /verbosity:minimal "/p:Configuration=$testConfiguration" /p:Platform=Win32
    if ($LASTEXITCODE -ne 0) {
        throw "IsaacCsT0.NativeTests.vcxproj $testConfiguration build failed with exit code $LASTEXITCODE"
    }
    $nativeTests = Join-Path $output "obj\tests\$testConfiguration\IsaacCsT0.NativeTests.exe"
    $nativeSuiteOutput = & $nativeTests
    $nativeSuiteOutput | Write-Output
    if ($LASTEXITCODE -ne 0 -or
        -not ($nativeSuiteOutput | Select-String -SimpleMatch '"identity":"IsaacCsT0-NativeHarness-v1","totalCases":39,"status":"PASS"')) {
        throw "independent T0 Native $testConfiguration suite failed or returned the wrong machine-readable case count"
    }

    $activeTestProject = Join-Path $nativeRoot 'IsaacCsT0.ActiveNativeTests.vcxproj'
    & $msbuild $activeTestProject /m /nologo /verbosity:minimal "/p:Configuration=$testConfiguration" /p:Platform=Win32
    if ($LASTEXITCODE -ne 0) {
        throw "IsaacCsT0.ActiveNativeTests.vcxproj $testConfiguration build failed with exit code $LASTEXITCODE"
    }
    $activeNativeTests = Join-Path $output "obj\active-tests\$testConfiguration\IsaacCsT0.ActiveNativeTests.exe"
    $activeSuiteOutput = & $activeNativeTests
    $activeSuiteOutput | Write-Output
    if ($LASTEXITCODE -ne 0 -or
        -not ($activeSuiteOutput | Select-String -SimpleMatch '"identity":"IsaacCsT0-ActiveNativeHarness-v1","totalCases":262,"status":"PASS"')) {
        throw "schema-4 Active Native $testConfiguration suite failed or returned the wrong machine-readable case count"
    }

    $passiveTestProject = Join-Path $nativeRoot 'IsaacCsT0.CoopPassiveNativeTests.vcxproj'
    & $msbuild $passiveTestProject /m /nologo /verbosity:minimal "/p:Configuration=$testConfiguration" /p:Platform=Win32
    if ($LASTEXITCODE -ne 0) {
        throw "IsaacCsT0.CoopPassiveNativeTests.vcxproj $testConfiguration build failed with exit code $LASTEXITCODE"
    }
    $passiveNativeTests = Join-Path $output "obj\coop-passive-tests\$testConfiguration\IsaacCsT0.CoopPassiveNativeTests.exe"
    $passiveSuiteOutput = & $passiveNativeTests
    $passiveSuiteOutput | Write-Output
    if ($LASTEXITCODE -ne 0 -or
        -not ($passiveSuiteOutput | Select-String -SimpleMatch 'PASS CS-T0-05-03 passive Native harness (407 cases)')) {
        throw "CS-T0-05-03 passive Native $testConfiguration suite failed or returned the wrong case count"
    }

    $previousCrossHarness = $env:ISAAC_CS_T0_COOP_PASSIVE_NATIVE_CROSS_TEST
    try {
        $env:ISAAC_CS_T0_COOP_PASSIVE_NATIVE_CROSS_TEST = $passiveNativeTests
        & cargo test --manifest-path $manifest --locked --test coop_passive_cross_language
        if ($LASTEXITCODE -ne 0) {
            throw "CS-T0-05-03 $testConfiguration C++ to Rust mapping test failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        $env:ISAAC_CS_T0_COOP_PASSIVE_NATIVE_CROSS_TEST = $previousCrossHarness
    }

    $deviceProject = Join-Path $nativeRoot 'IsaacCsT0.CoopDeviceRebindNativeTests.vcxproj'
    & $msbuild $deviceProject /m /nologo /verbosity:minimal "/p:Configuration=$testConfiguration" /p:Platform=Win32
    if ($LASTEXITCODE -ne 0) { throw "device v2 $testConfiguration Native build failed" }
    $deviceTests = Join-Path $output "obj\coop-device-rebind-tests\$testConfiguration\IsaacCsT0.CoopDeviceRebindNativeTests.exe"
    $deviceOutput = & $deviceTests
    $deviceOutput | Write-Output
    if ($LASTEXITCODE -ne 0 -or -not ($deviceOutput | Where-Object { $_ -ceq 'PASS coop-device-rebind-native-v2' })) {
        throw "device v2 $testConfiguration Native suite failed or returned no exact success marker"
    }
    $previousDeviceHarness = $env:ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST
    try {
        $env:ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST = $deviceTests
        & cargo test --manifest-path $manifest --locked --test coop_device_rebind_cross_language
        if ($LASTEXITCODE -ne 0) { throw "device v2 $testConfiguration real mapping suite failed" }
    }
    finally {
        $env:ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST = $previousDeviceHarness
    }
}

New-Item -ItemType Directory -Force -Path $output | Out-Null
$source = Join-Path $PSScriptRoot "target\$profile\IsaacCsT0.exe"
$destination = Join-Path $output 'IsaacCsT0.exe'
Copy-Item -LiteralPath $source -Destination $destination -Force
Copy-Item -LiteralPath (Join-Path $output "obj\native\$Configuration\IsaacCsT0.Native.dll") -Destination (Join-Path $output 'IsaacCsT0.Native.dll') -Force
Copy-Item -LiteralPath (Join-Path $output "obj\helper\$Configuration\IsaacCsT0.Helper.exe") -Destination (Join-Path $output 'IsaacCsT0.Helper.exe') -Force
Get-FileHash -Algorithm SHA256 -LiteralPath $destination,(Join-Path $output 'IsaacCsT0.Native.dll'),(Join-Path $output 'IsaacCsT0.Helper.exe')
