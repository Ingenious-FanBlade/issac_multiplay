[CmdletBinding()]
param(
    [string]$GameExe,
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [switch]$SkipInheritedVerifiers,
    [string]$VerificationOutput
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$manifest = Join-Path $PSScriptRoot 'Cargo.toml'
$nativeRoot = Join-Path $PSScriptRoot 'native'
$outputRoot = Join-Path $RepositoryRoot 'out\cs-t0'
$fullGate = -not $SkipInheritedVerifiers -and -not [string]::IsNullOrWhiteSpace($GameExe)

function Invoke-Checked {
    param([string]$Label, [scriptblock]$Command)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed with exit code $LASTEXITCODE"
    }
}

function Invoke-Verifier {
    param([string]$Label, [scriptblock]$Command)
    try {
        & $Command
    }
    catch {
        throw "$Label failed: $($_.Exception.Message)"
    }
    $global:LASTEXITCODE = 0
}

function Assert-Exact {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

if ($VerificationOutput) {
    Assert-Exact (-not (Test-Path -LiteralPath $VerificationOutput)) 'verification output already exists; create-new only'
    Assert-Exact $fullGate 'verification publication requires all inherited verifiers and GameExe'
}
$catalogPath = Join-Path $PSScriptRoot 'coop-catalog-v4.json'
$planPath = Join-Path $PSScriptRoot 'coop-plan-v5r1.json'
$backendScreenPath = Join-Path $PSScriptRoot 'coop-passive-backend-screen-v1.json'
$shutdownRepairPath = Join-Path $PSScriptRoot 'coop-device-rebind-shutdown-repair-v1.json'
$shutdownRepair = Get-Content -LiteralPath $shutdownRepairPath -Raw | ConvertFrom-Json
Assert-Exact ($shutdownRepair.repairIdentity -ceq 'IsaacCsT0-CoopDeviceRebindShutdownRepair-v1') 'shutdown repair identity mismatch'
Assert-Exact ($shutdownRepair.invariants.analyzerRelaxationAllowed -eq $false) 'shutdown repair must not relax analyzer'
Assert-Exact ($shutdownRepair.authorization.liveInjectionAuthorized -eq $false) 'shutdown repair must not authorize live injection'
foreach ($binding in $shutdownRepair.unchangedBindings) {
    Assert-Exact ($binding.sha256 -ceq (Get-FileHash -LiteralPath (Join-Path $RepositoryRoot $binding.path) -Algorithm SHA256).Hash) "shutdown repair changed frozen predecessor/analyzer: $($binding.path)"
}
$catalog = Get-Content -LiteralPath $catalogPath -Raw | ConvertFrom-Json
$plan = Get-Content -LiteralPath $planPath -Raw | ConvertFrom-Json
$backendScreen = Get-Content -LiteralPath $backendScreenPath -Raw | ConvertFrom-Json
Assert-Exact ($catalog.catalogIdentity -ceq 'IsaacCsT0-CoopCatalog-v4') 'catalog v4 identity mismatch'
Assert-Exact ($catalog.orderedHookTransaction.slotCount -eq 56) 'catalog must freeze exactly 56 slots'
Assert-Exact ($catalog.orderedHookTransaction.completeMask -ceq '0x00FFFFFFFFFFFFFF') 'catalog complete mask mismatch'
Assert-Exact (@($catalog.orderedHookTransaction.slots).Count -eq 56) 'catalog slot array count mismatch'
for ($ordinal = 0; $ordinal -lt 56; ++$ordinal) {
    Assert-Exact ($catalog.orderedHookTransaction.slots[$ordinal].ordinal -eq $ordinal) "catalog ordinal $ordinal is out of order"
}
Assert-Exact ($plan.planIdentity -ceq 'IsaacCsT0-CoopPlan-v5r1') 'plan v5r1 identity mismatch'
$catalogHash = (Get-FileHash -LiteralPath $catalogPath -Algorithm SHA256).Hash
$catalogBindings = @($plan.artifactBindings | Where-Object { $_.identity -ceq 'IsaacCsT0-CoopCatalog-v4' })
Assert-Exact ($catalogBindings.Count -eq 1) 'plan v5r1 must bind catalog v4 exactly once'
Assert-Exact ($catalogBindings[0].sha256 -ceq $catalogHash) 'plan v5r1 catalog v4 hash mismatch'
Assert-Exact ($plan.permissions.passiveHookTransactionAllowed -eq $true) 'plan v5r1 must grant the passive transaction'
foreach ($field in @('passiveObservationAllowed', 'activeInputAllowed', 'inputMutationAllowed', 'worldMutationAllowed', 'runtimeMutationAllowed', 'overrideAllowed', 'transportAllowed', 'schedulerAllowed', 'applyAllowed', 'modeLatchAllowed', 'originalOnlinePositiveAllowed', 't005Unlocked', 'csT00504Unlocked')) {
    Assert-Exact ($plan.permissions.$field -eq $false) "plan v5r1 widened forbidden capability $field"
}
Assert-Exact ($plan.passiveAuthorizationContract.liveAuthorizationCreated -eq $false) 'pre-live plan must not contain a live authorization'
Assert-Exact ($plan.passiveAuthorizationContract.requiresFreshAuthorizationPerScenario -eq $true) 'fresh per-scenario authorization is not frozen'
Assert-Exact ($plan.passiveAuthorizationContract.failedAuthorizationMayRetry -eq $false) 'failed passive authorization must not retry'

foreach ($binding in $plan.artifactBindings) {
    Assert-Exact ($binding.sha256 -ceq (Get-FileHash -LiteralPath (Join-Path $RepositoryRoot $binding.path) -Algorithm SHA256).Hash) "frozen plan binding changed: $($binding.identity)"
}
Assert-Exact ($catalog.contract.sha256 -ceq (Get-FileHash -LiteralPath (Join-Path $RepositoryRoot $catalog.contract.path) -Algorithm SHA256).Hash) 'device contract hash mismatch'
Assert-Exact ($catalog.staticEvidence.sha256 -ceq (Get-FileHash -LiteralPath (Join-Path $RepositoryRoot $catalog.staticEvidence.path) -Algorithm SHA256).Hash) 'device static evidence hash mismatch'
if ($fullGate) {
    $staticDevice = Get-Content -LiteralPath (Join-Path $RepositoryRoot $catalog.staticEvidence.path) -Raw | ConvertFrom-Json
    Assert-Exact ((Get-FileHash -LiteralPath $GameExe -Algorithm SHA256).Hash -ceq $staticDevice.target.observedApprovedUnpackedSha256) 'device static target hash mismatch'
    $targetImage = [System.IO.File]::ReadAllBytes($GameExe)
    $peOffset = [BitConverter]::ToInt32($targetImage, 0x3c)
    $sectionCount = [BitConverter]::ToUInt16($targetImage, $peOffset + 6)
    $sectionTable = $peOffset + 24 + [BitConverter]::ToUInt16($targetImage, $peOffset + 20)
    foreach ($slot in $staticDevice.newSlots) {
        foreach ($site in @(@($slot.rva, $slot.expectedBytes), @($slot.featureStartRva, $slot.featureBytes))) {
            $rva = [Convert]::ToUInt32($site[0].Substring(2), 16)
            $expectedBytes = [Convert]::FromHexString($site[1])
            $rawOffset = -1
            for ($section = 0; $section -lt $sectionCount; ++$section) {
                $header = $sectionTable + 40 * $section
                $virtualAddress = [BitConverter]::ToUInt32($targetImage, $header + 12)
                $rawSize = [BitConverter]::ToUInt32($targetImage, $header + 16)
                if ($rva -ge $virtualAddress -and $rva + $expectedBytes.Length -le $virtualAddress + $rawSize) {
                    $rawOffset = [BitConverter]::ToUInt32($targetImage, $header + 20) + $rva - $virtualAddress
                    break
                }
            }
            Assert-Exact ($rawOffset -ge 0) "device slot $($slot.ordinal) is outside target sections"
            $actualBytes = [byte[]]::new($expectedBytes.Length)
            [Array]::Copy($targetImage, $rawOffset, $actualBytes, 0, $actualBytes.Length)
            Assert-Exact ([Convert]::ToHexString($actualBytes) -ceq $site[1]) "device slot $($slot.ordinal) exact static bytes drifted"
        }
    }
}

Assert-Exact ($backendScreen.decisionIdentity -ceq 'IsaacCsT0-CoopPassiveBackendScreen-v1') 'backend screen identity mismatch'
Assert-Exact ($backendScreen.status -ceq 'PASS_SCOPED_REPAIR') 'backend screen status mismatch'
Assert-Exact ($backendScreen.createNewOnly -eq $true) 'backend screen must remain create-new only'
Assert-Exact ($backendScreen.scope -ceq 'COOP_PASSIVE_NONZERO_BACKEND_LOCAL_ROUTE') 'backend screen scope widened'
$predecessorLayoutPath = Join-Path $RepositoryRoot $backendScreen.predecessorLayout.path
Assert-Exact ($backendScreen.predecessorLayout.identity -ceq 'IsaacCsT0-CoopC0LayoutEvidence-v1') 'backend screen predecessor identity mismatch'
Assert-Exact ($backendScreen.predecessorLayout.unchanged -eq $true) 'backend screen changed the predecessor C0 contract'
Assert-Exact ($backendScreen.predecessorLayout.sha256 -ceq (Get-FileHash -LiteralPath $predecessorLayoutPath -Algorithm SHA256).Hash) 'backend screen predecessor hash mismatch'
$backendGraphPath = Join-Path $RepositoryRoot $backendScreen.staticEvidence.graphPath
Assert-Exact ($backendScreen.staticEvidence.graphIdentity -ceq 'IsaacCsT0-CoopStaticGraph-v1') 'backend screen graph identity mismatch'
Assert-Exact ($backendScreen.staticEvidence.graphSha256 -ceq (Get-FileHash -LiteralPath $backendGraphPath -Algorithm SHA256).Hash) 'backend screen graph hash mismatch'
Assert-Exact ($backendScreen.staticEvidence.localBodyGuardRva -ceq '0x005BAC6D') 'backend screen local-body guard mismatch'
Assert-Exact ($backendScreen.staticEvidence.nonemptyRosterTargetRva -ceq '0x005BB574') 'backend screen nonempty-roster target mismatch'
Assert-Exact ($backendScreen.staticEvidence.localBodyRange -ceq '0x005BAC85..0x005BB56E') 'backend screen local-body range mismatch'
Assert-Exact ((@($backendScreen.staticEvidence.ordinaryRouteSlots) -join ',') -ceq '46,49,51') 'backend screen ordinary route slots mismatch'
Assert-Exact ($backendScreen.staticEvidence.steamBackendVtableRva -ceq '0x007A2440') 'backend screen Steam vtable mismatch'
Assert-Exact ($backendScreen.scopedRule.nullBackendAllowedWithoutRouteWitness -eq $true) 'backend screen regressed the null-backend route'
Assert-Exact ($backendScreen.scopedRule.nonzeroBackendPresenceAloneIsPositiveOnlineEvidence -eq $false) 'backend presence became positive online evidence'
foreach ($field in @('nonzeroBackendRequiresAlignedReadablePointer', 'nonzeroBackendRequiresExactSteamVtable', 'onlineManagerRosterBeginMustBeZero', 'onlineManagerRosterEndMustBeZero', 'onlineManagerRosterCapacityEndMustBeZero', 'onlineFrameMustBeZero', 'ordinaryRouteMustBeInsideOneManagerTransaction', 'ordinaryRouteMustMatchCurrentGeneration', 'nextConsecutiveManagerPassMayConfirmSameCandidate', 'possessorOrExitCannotAuthorizeOrdinaryRoute', 'staleGenerationRouteRejected', 'unknownBackendOrAnyOnlineSignalKeepsUnbound')) {
    Assert-Exact ($backendScreen.scopedRule.$field -eq $true) "backend screen weakened required rule $field"
}
Assert-Exact ($backendScreen.scopedRule.newHookAllowed -eq $false) 'backend screen authorized a new hook'
Assert-Exact ($backendScreen.scopedRule.analyzerRelaxationAllowed -eq $false) 'backend screen authorized analyzer relaxation'
$expectedOrdinaryRoute = 'ORDINARY_SPAWN_BEGIN,ORDINARY_SPAWN_END,ORDINARY_ASSIGNMENT_BEGIN,ORDINARY_ASSIGNMENT_END,JOIN_HUD_BEGIN,JOIN_HUD_END'
Assert-Exact ((@($backendScreen.scopedRule.ordinaryRouteMustBeExact) -join ',') -ceq $expectedOrdinaryRoute) 'backend screen ordinary route sequence mismatch'
foreach ($field in @('ordinaryJoinInvalidatesExactlyOnce', 'ordinaryAssignmentInsideTheSameJoinRouteDoesNotInvalidateAgain', 'unrelatedAssignmentStillInvalidates')) {
    Assert-Exact ($backendScreen.generationRule.$field -eq $true) "backend screen weakened generation rule $field"
}
foreach ($field in @('passiveObservationAllowed', 'activeInputAllowed', 'inputMutationAllowed', 'worldMutationAllowed', 'runtimeMutationAllowed', 'overrideAllowed', 'transportAllowed', 'schedulerAllowed', 'applyAllowed', 'modeLatchAllowed', 'originalOnlinePositiveAllowed', 't005Unlocked', 'csT00504Unlocked')) {
    Assert-Exact ($backendScreen.permissions.$field -eq $false) "backend screen widened forbidden capability $field"
}
$live04Source = Join-Path $RepositoryRoot $backendScreen.liveEvidence.sourceDirectory
Assert-Exact ($backendScreen.liveEvidence.analysisResult -ceq 'BLOCKED_BY_EVIDENCE') 'backend screen rewrote the old live-04 result'
Assert-Exact ($backendScreen.liveEvidence.managerEndCount -eq 8177) 'backend screen live-04 manager count mismatch'
Assert-Exact ($backendScreen.liveEvidence.managerDiagnosticCode -eq 3) 'backend screen live-04 diagnostic mismatch'
Assert-Exact ($backendScreen.liveEvidence.ordinaryRouteObserved -eq $true) 'backend screen lost the live ordinary route'
Assert-Exact ($backendScreen.liveEvidence.oldSourceResultImmutable -eq $true) 'backend screen must preserve the old replay result'
Assert-Exact ($backendScreen.liveEvidence.manifestSha256 -ceq (Get-FileHash -LiteralPath (Join-Path $live04Source 'manifest.json') -Algorithm SHA256).Hash) 'backend screen live-04 manifest hash mismatch'
Assert-Exact ($backendScreen.liveEvidence.eventsSha256 -ceq (Get-FileHash -LiteralPath (Join-Path $live04Source 'events.jsonl') -Algorithm SHA256).Hash) 'backend screen live-04 events hash mismatch'
Assert-Exact ($backendScreen.liveEvidence.analysisSha256 -ceq (Get-FileHash -LiteralPath (Join-Path $live04Source 'analysis.json') -Algorithm SHA256).Hash) 'backend screen live-04 analysis hash mismatch'

Invoke-Checked 'cargo fmt --check' { cargo fmt --manifest-path $manifest -- --check }
Invoke-Checked 'cargo check --all-targets' { cargo check --manifest-path $manifest --all-targets --locked }
Invoke-Checked 'cargo clippy -D warnings' { cargo clippy --manifest-path $manifest --all-targets --locked -- -D warnings }
Invoke-Checked 'cargo test --all-targets' { cargo test --manifest-path $manifest --all-targets --locked }
Invoke-Checked 'cargo release build' { cargo build --manifest-path $manifest --release --locked }

# build.ps1 builds the production DLL/helper, the inherited 39-case harness, the
# inherited Active 262-case harness, and the passive 407-case harness in Debug and
# Release. It also runs the C++ publisher -> real Rust mapping test for both builds.
Invoke-Verifier 'production and all Native Debug/Release gates' {
    & (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release -SkipRustTests
}

# Require the new production-Shutdown path explicitly; a stale harness or an
# accidentally filtered zero-test Rust run must not satisfy these checks.
foreach ($configuration in @('Debug', 'Release')) {
    $shutdownHarness = Join-Path $outputRoot "obj\coop-device-rebind-tests\$configuration\IsaacCsT0.CoopDeviceRebindNativeTests.exe"
    $shutdownOutput = & $shutdownHarness --test-device-shutdown
    $shutdownExit = $LASTEXITCODE
    $shutdownOutput | Write-Output
    Assert-Exact ($shutdownExit -eq 0 -and @($shutdownOutput | Where-Object { $_ -ceq 'PASS coop-device-real-shutdown-v1' }).Count -eq 1) "real production shutdown $configuration Native regression failed or did not run"
    $previousShutdownHarness = $env:ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST
    try {
        $env:ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST = $shutdownHarness
        $shutdownBridgeOutput = & cargo test --manifest-path $manifest --locked --test coop_device_rebind_cross_language actual_native_shutdown_events_serialize_into_public_cli_contract -- --exact --nocapture
        $shutdownBridgeExit = $LASTEXITCODE
        $shutdownBridgeOutput | Write-Output
        Assert-Exact ($shutdownBridgeExit -eq 0 -and @($shutdownBridgeOutput | Select-String -SimpleMatch 'test actual_native_shutdown_events_serialize_into_public_cli_contract ... ok').Count -eq 1) "real shutdown $configuration public CLI bridge failed or did not run"
    }
    finally {
        $env:ISAAC_CS_T0_COOP_DEVICE_REBIND_NATIVE_CROSS_TEST = $previousShutdownHarness
    }
}

Invoke-Checked 'device v2 causal contract and mutations' {
    cargo test --manifest-path $manifest --locked --test coop_device_rebind_contract
}

Invoke-Checked 'passive six-scenario and mutation contract' {
    cargo test --manifest-path $manifest --locked --test coop_passive_contract
}
foreach ($configuration in @('Debug', 'Release')) {
    $harness = Join-Path $outputRoot "obj\coop-passive-tests\$configuration\IsaacCsT0.CoopPassiveNativeTests.exe"
    Assert-Exact (Test-Path -LiteralPath $harness -PathType Leaf) "missing passive $configuration harness"
    $previous = $env:ISAAC_CS_T0_COOP_PASSIVE_NATIVE_CROSS_TEST
    try {
        $env:ISAAC_CS_T0_COOP_PASSIVE_NATIVE_CROSS_TEST = $harness
        Invoke-Checked "passive real mapping $configuration" {
            cargo test --manifest-path $manifest --locked --test coop_passive_cross_language
        }
    }
    finally {
        $env:ISAAC_CS_T0_COOP_PASSIVE_NATIVE_CROSS_TEST = $previous
    }
}

if (-not $SkipInheritedVerifiers) {
    Invoke-Verifier 'CS-T0-05 bootstrap regression' {
        & (Join-Path $PSScriptRoot 'verify-coop-bootstrap.ps1') -RepositoryRoot $RepositoryRoot
    }
    if (-not [string]::IsNullOrWhiteSpace($GameExe)) {
        $resolvedGame = [System.IO.Path]::GetFullPath($GameExe)
        Invoke-Verifier 'CS-T0-05 static regression' {
            & (Join-Path $PSScriptRoot 'verify-coop-static.ps1') -GameExe $resolvedGame -RepositoryRoot $RepositoryRoot
        }
        Invoke-Verifier 'CS-T0-05 C0 regression' {
            & (Join-Path $PSScriptRoot 'verify-coop-c0.ps1') -GameExe $resolvedGame -RepositoryRoot $RepositoryRoot -SkipFullRegression
        }
    }
    else {
        Write-Warning 'GameExe not supplied: static/C0 target-image CLI replay is represented by the frozen Rust regression tests.'
    }
    Invoke-Verifier 'T0-04 Active regression' {
        & (Join-Path $PSScriptRoot 'verify-active.ps1')
    }
}

# Replaying old sources is read-only and must retain both verdict and stored bytes.
$historical = @(
    '20260828T072118Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-06',
    '20260830T182858Z-t0-05-03-death-respawn-passive-t0-05-03-death-respawn-passive-live-08',
    '20260830T185508Z-t0-05-03-room-rebind-passive-t0-05-03-room-rebind-passive-live-09'
)
$controller = Join-Path $outputRoot 'IsaacCsT0.exe'
$replayRoot = Join-Path $outputRoot ('device-v2-auto-replay-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $replayRoot | Out-Null
foreach ($case in $historical) {
    $source = Join-Path $RepositoryRoot "artifacts/cs-t0/$case"
    $sourceFiles = @('manifest.json', 'events.jsonl', 'analysis.json') | ForEach-Object { Join-Path $source $_ }
    $beforeHashes = @(Get-FileHash -LiteralPath $sourceFiles -Algorithm SHA256).Hash -join ','
    $replayOutput = Join-Path $replayRoot "$case.json"
    Invoke-Checked "immutable historical replay $case" { & $controller coop-analyze $source --output $replayOutput }
    Assert-Exact ((Get-FileHash -LiteralPath $replayOutput -Algorithm SHA256).Hash -ceq (Get-FileHash -LiteralPath (Join-Path $source 'analysis.json') -Algorithm SHA256).Hash) "historical analysis changed: $case"
    Assert-Exact ((@(Get-FileHash -LiteralPath $sourceFiles -Algorithm SHA256).Hash -join ',') -ceq $beforeHashes) "historical source was modified: $case"
}

# The observed failure is immutable. A production cleanup repair must not make
# the old generation-inconsistent stream pass or rewrite its original analysis.
$failedSource = Join-Path $RepositoryRoot $shutdownRepair.historicalFailure.source
$failedSourceFiles = @('manifest.json', 'events.jsonl', 'analysis.json') | ForEach-Object { Join-Path $failedSource $_ }
$failedExpectedHashes = @($shutdownRepair.historicalFailure.manifestSha256, $shutdownRepair.historicalFailure.eventsSha256, $shutdownRepair.historicalFailure.analysisSha256) -join ','
Assert-Exact ((@(Get-FileHash -LiteralPath $failedSourceFiles -Algorithm SHA256).Hash -join ',') -ceq $failedExpectedHashes) 'live-10 immutable failure source changed before replay'
$failedReplay = Join-Path $replayRoot 'device-live10-still-blocked.json'
& $controller coop-analyze $failedSource --output $failedReplay
Assert-Exact ($LASTEXITCODE -eq 1) 'live-10 generation-inconsistent source must remain rejected'
$failedAnalysis = Get-Content -LiteralPath $failedReplay -Raw | ConvertFrom-Json
Assert-Exact ($failedAnalysis.result -ceq 'BLOCKED_BY_EVIDENCE') 'live-10 verdict was relaxed'
Assert-Exact ((Get-FileHash -LiteralPath $failedReplay -Algorithm SHA256).Hash -ceq $shutdownRepair.historicalFailure.analysisSha256) 'live-10 analysis bytes changed'
Assert-Exact ((@(Get-FileHash -LiteralPath $failedSourceFiles -Algorithm SHA256).Hash -join ',') -ceq $failedExpectedHashes) 'live-10 source was modified by replay'
$global:LASTEXITCODE = 0
$report = [ordered]@{
    schemaVersion = 2
    verificationIdentity = 'IsaacCsT0-CoopPassiveAutomatedGate-v6'
    workPackage = 'CS-T0-05-03'
    status = $(if ($fullGate) { 'PASS' } else { 'PARTIAL_NOT_ELIGIBLE' })
    rustAllTargetsPassed = $true
    inheritedNativeCasesPerConfiguration = 39
    activeNativeCasesPerConfiguration = 262
    passiveNativeCasesPerConfiguration = 407
    deviceV2NativeConfigurations = 2
    deviceV2RealMappingConfigurations = 2
    deviceV2CausalMutationsPassed = $true
    realProductionShutdownNativeConfigurations = 2
    realProductionShutdownPublicCliConfigurations = 2
    legacyDeviceCounterexamplesRejected = $true
    historicalLiveSourceReplayCount = 3
    historicalFailedDeviceReplayUnchanged = $true
    offlinePassiveFixtureCount = 6
    completedLiveSourceCount = 3
    liveAuthorizationCreated = $false
    sixSourceGateEligible = $false
    nextRequiredAuthorization = 'device-rebind-passive'
}
$report | ConvertTo-Json -Depth 8

if ($VerificationOutput) {
    $bindingSpecs = @(
        @('out/cs-t0/target-equivalence-inspect/unpacked-target-equivalence-v1-release-final.json', 'IsaacCsT0-UnpackedTargetEquivalence-v1'),
        @('cs-t0/coop-c0-layout-evidence-v1.json', 'IsaacCsT0-CoopC0LayoutEvidence-v1'),
        @('cs-t0/coop-static-graph-v1.json', 'IsaacCsT0-CoopStaticGraph-v1'),
        @('cs-t0/coop-catalog-v4.json', 'IsaacCsT0-CoopCatalog-v4'),
        @('cs-t0/coop-plan-v5r1.json', 'IsaacCsT0-CoopPlan-v5r1'),
        @('cs-t0/coop-passive-backend-screen-v1.json', 'IsaacCsT0-CoopPassiveBackendScreen-v1'),
        @('cs-t0/verify-coop-passive.ps1', 'IsaacCsT0-CoopPassiveAutomatedGate-v6'),
        @('cs-t0/coop-device-rebind-shutdown-repair-v1.json', 'IsaacCsT0-CoopDeviceRebindShutdownRepair-v1')
    )
    $artifactBindings = @($bindingSpecs | ForEach-Object {
        [ordered]@{ path = $_[0]; identity = $_[1]; sha256 = (Get-FileHash -LiteralPath (Join-Path $RepositoryRoot $_[0]) -Algorithm SHA256).Hash }
    })
    $productionBindings = @('out/cs-t0/IsaacCsT0.exe', 'out/cs-t0/IsaacCsT0.Native.dll', 'out/cs-t0/IsaacCsT0.Helper.exe') | ForEach-Object {
        [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $RepositoryRoot $_) -Algorithm SHA256).Hash }
    }
    $checks = @(
        'frozen-artifact-bindings', 'rust-test-all-targets', 'rust-fmt-check',
        'rust-check-all-targets', 'rust-clippy-deny-warnings', 'rust-release-build',
        'legacy-native-debug-release-39', 'active-native-debug-release-262',
        'passive-native-debug-release-407', 'device-v2-native-debug-release',
        'passive-cross-language-debug-release', 'device-v2-cross-language-debug-release',
        'device-v2-causal-contract-and-mutations', 'legacy-device-counterexamples-rejected',
        'historical-live-replay-immutable', 'bootstrap-regression', 'static-c0-regression',
        't0-04-active-regression', 'six-offline-passive-fixtures-and-gate',
        'device-v2-real-shutdown-native-debug-release',
        'device-v2-real-shutdown-public-cli-debug-release',
        'device-live10-replay-remains-blocked'
    ) | ForEach-Object { [ordered]@{ id = $_; passed = $true } }
    $verification = [ordered]@{
        schemaVersion = 5
        verificationIdentity = 'IsaacCsT0-CoopVerification-v5r6'
        workPackage = 'CS-T0-05-03'
        createdUtc = [DateTime]::UtcNow.ToString('o')
        status = 'PASS'
        phase = 'PRE_LIVE_DEVICE_V2_SHUTDOWN_REPAIR_AUTOMATED_GATE'
        createNewOnly = $true
        supersedes = 'coop-verification-v5r5.json'
        supersedesSha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'coop-verification-v5r5.json') -Algorithm SHA256).Hash
        staticContractComplete = $true
        runtimeContractComplete = $true
        liveEvidenceComplete = $false
        completedLiveSourceCount = 3
        offlinePassiveFixtureCount = 6
        sixSourceGateEligible = $false
        liveAuthorizationCreated = $false
        nextRequiredAuthorization = 'device-rebind-passive'
        artifactBindings = $artifactBindings
        productionBindings = @($productionBindings)
        checks = @($checks)
        permissions = $plan.permissions
    }
    $json = ($verification | ConvertTo-Json -Depth 20) + [Environment]::NewLine
    $stream = [System.IO.File]::Open($VerificationOutput, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write)
    try {
        $bytes = [System.Text.UTF8Encoding]::new($false).GetBytes($json)
        $stream.Write($bytes, 0, $bytes.Length)
    }
    finally { $stream.Dispose() }
    Get-FileHash -LiteralPath $VerificationOutput -Algorithm SHA256
}
