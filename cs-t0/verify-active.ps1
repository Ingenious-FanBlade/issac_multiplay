param(
    [string]$VerificationV2Output
)

$ErrorActionPreference = 'Stop'
$manifest = Join-Path $PSScriptRoot 'Cargo.toml'
$mcpManifest = Join-Path (Split-Path -Parent $PSScriptRoot) 'isaac-test-mcp\Cargo.toml'
$verificationPath = Join-Path $PSScriptRoot 'input-active-verification-v1.json'

& cargo test --manifest-path $manifest --all-targets --locked
if ($LASTEXITCODE -ne 0) { throw 'Rust tests failed' }

& cargo fmt --manifest-path $manifest -- --check
if ($LASTEXITCODE -ne 0) { throw 'rustfmt failed' }

& cargo check --manifest-path $manifest --all-targets --locked
if ($LASTEXITCODE -ne 0) { throw 'cargo check failed' }

& cargo clippy --manifest-path $manifest --all-targets --locked -- -D warnings
if ($LASTEXITCODE -ne 0) { throw 'cargo clippy failed' }

& (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release
if ($LASTEXITCODE -ne 0) { throw 'Native Debug/Release harness failed' }

$activeCrossLanguageHarness = Join-Path (Split-Path -Parent $PSScriptRoot) 'out\cs-t0\obj\active-tests\Debug\IsaacCsT0.ActiveNativeTests.exe'
$previousCrossLanguageHarness = $env:ISAAC_CS_T0_ACTIVE_NATIVE_CROSS_TEST
try {
    $env:ISAAC_CS_T0_ACTIVE_NATIVE_CROSS_TEST = $activeCrossLanguageHarness
    & cargo test --manifest-path $manifest --test active_cross_language --locked -- --exact native_publisher_real_mapping_round_trip_is_fail_closed
    if ($LASTEXITCODE -ne 0) { throw 'Native-to-Rust real mapping cross-language test failed' }
}
finally {
    if ($null -eq $previousCrossLanguageHarness) {
        Remove-Item Env:ISAAC_CS_T0_ACTIVE_NATIVE_CROSS_TEST -ErrorAction SilentlyContinue
    }
    else {
        $env:ISAAC_CS_T0_ACTIVE_NATIVE_CROSS_TEST = $previousCrossLanguageHarness
    }
}

& cargo test --manifest-path $mcpManifest --all-targets --locked
if ($LASTEXITCODE -ne 0) { throw 'IsaacTest MCP tests failed' }

$verification = Get-Content -LiteralPath $verificationPath -Raw | ConvertFrom-Json
if ($verification.verificationIdentity -ne 'IsaacCsT0-ActiveVerification-v1' -or
    $verification.status -ne 'BLOCKED_BY_DEFERRED_SCHEMA4_NATIVE' -or
    -not $verification.semanticContractComplete -or
    $verification.runtimeContractComplete -or
    $verification.restoredSlotCount -ne 45 -or
    $verification.activeOwnedSlotCount -ne 13 -or
    $verification.requiredScenarios.Count -ne 18) {
    throw 'active verification document is not the frozen deferred-runtime identity'
}

$runtime = @($verification.checks | Where-Object { $_.id -eq 'schema4-native-runtime' })
$completed = @($verification.checks | Where-Object { $_.id -ne 'schema4-native-runtime' })
$failedCompleted = @($completed | Where-Object { -not $_.passed })
if ($runtime.Count -ne 1 -or $runtime.passed -or
    $failedCompleted.Count -ne 0) {
    throw 'active verification subgate status is inconsistent'
}

if (-not [string]::IsNullOrWhiteSpace($VerificationV2Output)) {
    $suiteEvidence = @()
    $outputRoot = Join-Path (Split-Path -Parent $PSScriptRoot) 'out\cs-t0'
    foreach ($suite in @(
        [ordered]@{
            id = 'schema4-native-debug'
            configuration = 'Debug'
            path = Join-Path $outputRoot 'obj\active-tests\Debug\IsaacCsT0.ActiveNativeTests.exe'
        },
        [ordered]@{
            id = 'schema4-native-release-ltcg'
            configuration = 'Release-LTCG'
            path = Join-Path $outputRoot 'obj\active-tests\Release\IsaacCsT0.ActiveNativeTests.exe'
        }
    )) {
        if (-not (Test-Path -LiteralPath $suite.path -PathType Leaf)) {
            throw "active Native suite binary is missing: $($suite.path)"
        }
        $suiteOutput = & $suite.path
        $suiteExitCode = $LASTEXITCODE
        $summaryLine = @($suiteOutput | Where-Object {
            $_ -like '*"identity":"IsaacCsT0-ActiveNativeHarness-v1"*'
        }) | Select-Object -Last 1
        if ($suiteExitCode -ne 0 -or -not $summaryLine) {
            throw "$($suite.id) did not return a machine-readable PASS summary"
        }
        $summary = $summaryLine | ConvertFrom-Json
        if ($summary.identity -ne 'IsaacCsT0-ActiveNativeHarness-v1' -or
            $summary.totalCases -ne 262 -or $summary.status -ne 'PASS') {
            throw "$($suite.id) returned the wrong identity, case count, or status"
        }
        $suiteEvidence += [ordered]@{
            id = $suite.id
            configuration = $suite.configuration
            exitCode = $suiteExitCode
            caseCount = [int]$summary.totalCases
            binarySha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $suite.path).Hash
        }
    }
    if ($suiteEvidence[0].binarySha256 -eq $suiteEvidence[1].binarySha256) {
        throw 'Debug and Release/LTCG active harness binaries unexpectedly have the same SHA-256'
    }
    $checkIds = @(
        'rust-test',
        'rust-fmt',
        'rust-check-all-targets',
        'rust-clippy-deny-warnings',
        'isaac-test-mcp',
        'schema4-manifest-companion-binding',
        'schema4-loop-context-replay',
        'schema4-disarmed-companion-forbidden',
        'schema4-ordered-eighteen-scenario-semantics',
        'schema4-repeatability-normalization',
        'schema4-mutation-fail-closed',
        'schema4-active-event-288-byte-static-assert',
        'schema4-active-session-id-16-byte-static-assert',
        'schema4-active-shared-block-720-offset-static-assert',
        'schema4-active-numeric-codebook',
        'schema4-active-process-creation-binding',
        'schema4-active-unpacked-equivalence-binding',
        'schema4-native-debug-harness',
        'schema4-native-release-harness',
        'schema4-45-slot-install-transaction',
        'schema4-relative-mid-install-rollback',
        'schema4-pointer-mid-install-rollback',
        'schema4-bool-x87-original-once-abi',
        'schema4-staged-shutdown-recovery',
        'schema4-active-helper-cli',
        'schema4-rust-live-orchestration',
        'schema4-cross-language-real-mapping'
    )
    $document = [ordered]@{
        schemaVersion = 2
        verificationIdentity = 'IsaacCsT0-ActiveVerification-v2'
        status = 'AUTOMATED_RUNTIME_COMPLETE_AWAITING_ACTIVE_EVIDENCE'
        semanticContractComplete = $true
        runtimeContractComplete = $true
        activeInputAllowed = $false
        activeArtifactSchemaVersion = 4
        activeIpcSchemaVersion = 4
        activeMatrixSchemaVersion = 2
        activeMatrixIdentity = 'IsaacCsT0-ActiveMatrix-v2'
        requiredScenarios = @(
            'control-no-native', 'keyboard', 'controller', 'simultaneous', 'rapid-tap',
            'device-switch', 'pause', 'loading', 'room', 'menu', 'queue-full', 'expired',
            'too-far', 'stale', 'conflict', 'duplicate', 'original-online-disarmed', 'loopback'
        )
        companionLoopRequiredForPipeline = $true
        companionLoopForbiddenForDisarmed = $true
        activeEventSize = 288
        activeSessionIdSize = 16
        activeEventsOffset = 720
        activeSharedBlockSize = 18875088
        restoredSlotCount = 45
        v5OwnedSlotCount = 32
        activeOwnedSlotCount = 13
        suiteEvidence = $suiteEvidence
        checks = @($checkIds | ForEach-Object { [ordered]@{ id = $_; passed = $true } })
    }
    $fullOutput = [System.IO.Path]::GetFullPath($VerificationV2Output)
    $json = $document | ConvertTo-Json -Depth 8
    $encoding = [System.Text.UTF8Encoding]::new($false)
    $stream = [System.IO.File]::Open(
        $fullOutput,
        [System.IO.FileMode]::CreateNew,
        [System.IO.FileAccess]::Write,
        [System.IO.FileShare]::None
    )
    try {
        $writer = [System.IO.StreamWriter]::new($stream, $encoding)
        try {
            $writer.Write($json)
            $writer.Write("`n")
        }
        finally {
            $writer.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }
    Get-Content -LiteralPath $fullOutput -Raw
}
else {
    Get-Content -LiteralPath $verificationPath -Raw
}
