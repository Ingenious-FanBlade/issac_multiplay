[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$GameExe,
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$Controller = 'cs-t0/target/release/IsaacCsT0.exe',
    [string]$TargetEquivalence = 'out/cs-t0/target-equivalence-inspect/unpacked-target-equivalence-v1-release-final.json',
    [string]$LayoutEvidence = 'cs-t0/coop-c0-layout-evidence-v1.json',
    [string]$Catalog = 'cs-t0/coop-catalog-v2.json',
    [string]$Graph = 'cs-t0/coop-static-graph-v1.json',
    [string]$Plan = 'cs-t0/coop-plan-v3.json',
    [string]$VerificationOutput,
    [string]$OfflineSingle,
    [string]$LocalCoopDual,
    [string]$OriginalOnlineDual,
    [string]$ComparisonOutput,
    [string]$DecisionOutput,
    [string]$MatrixOutput,
    [string]$LedgerOutput,
    [string]$FinalPlanOutput,
    [string]$LoopGate = 'artifacts/cs-t0/20260821-t0-03r-v5-loop-gate-7of7.json',
    [string]$ActiveDecision = 'out/cs-t0/formal-active-gate-20260827-r1/active-gate-decision.json',
    [switch]$SkipFullRegression
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-RepoPath {
    param([string]$Path)
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $RepositoryRoot $Path))
}

function Assert-RepositoryRelativeArtifactPath {
    param(
        [AllowEmptyString()]
        [string]$Path,
        [string]$Label,
        [switch]$AllowEmpty
    )
    if ([string]::IsNullOrWhiteSpace($Path)) {
        if ($AllowEmpty) {
            return $Path
        }
        throw "$Label must not be empty."
    }
    if ([System.IO.Path]::IsPathRooted($Path)) {
        throw "$Label must be repository-relative; absolute paths are forbidden."
    }

    $normalized = $Path.Replace('\', '/')
    if ($normalized.StartsWith('//') -or $normalized -match '^[A-Za-z][A-Za-z0-9+.-]*:') {
        throw "$Label must not be a UNC path or URI."
    }
    $segments = @($normalized.Split('/'))
    if (@($segments | Where-Object { [string]::IsNullOrEmpty($_) -or $_ -eq '.' -or $_ -eq '..' }).Count -ne 0) {
        throw "$Label must be a normalized repository-relative path without empty, '.' or '..' segments."
    }

    $rootFull = [System.IO.Path]::GetFullPath($RepositoryRoot).TrimEnd([char[]]@('\', '/'))
    $full = [System.IO.Path]::GetFullPath((Join-Path $rootFull $normalized))
    $rootPrefix = $rootFull + [System.IO.Path]::DirectorySeparatorChar
    if (-not $full.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "$Label resolves outside the repository root."
    }
    return $normalized
}

function Get-Sha256 {
    param([string]$Path)
    return (Get-FileHash -LiteralPath (Resolve-RepoPath $Path) -Algorithm SHA256).Hash
}

function Assert-Equal {
    param([object]$Actual, [object]$Expected, [string]$Label)
    if ($Actual -cne $Expected) {
        throw "$Label mismatch: expected '$Expected', got '$Actual'"
    }
}

function Invoke-Checked {
    param([string]$Label, [scriptblock]$Command)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed with exit code $LASTEXITCODE"
    }
}

function Invoke-CheckedPowerShellScript {
    param([string]$Label, [scriptblock]$Command)
    try {
        & $Command
    }
    catch {
        throw "$Label failed: $($_.Exception.Message)"
    }
    # Some existing verifier scripts deliberately finish by checking a CLI's
    # create-new refusal (exit 2) and then emit their own PASS object. Their
    # stale LASTEXITCODE is not the verifier process status.
    $global:LASTEXITCODE = 0
}

function Write-JsonCreateNew {
    param([string]$Path, [object]$Value)
    $full = Resolve-RepoPath $Path
    if (Test-Path -LiteralPath $full) {
        throw "Refusing to overwrite $full"
    }
    $parent = Split-Path -Parent $full
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        throw "Output parent does not exist: $parent"
    }
    $json = $Value | ConvertTo-Json -Depth 100
    $utf8 = [System.Text.UTF8Encoding]::new($false)
    [System.IO.File]::WriteAllText($full, $json, $utf8)
}

function Assert-CreateNewTargetAvailable {
    param([string]$Path, [string]$Label)
    $full = Resolve-RepoPath $Path
    if (Test-Path -LiteralPath $full) {
        throw "$Label refuses to overwrite $full"
    }
    $parent = Split-Path -Parent $full
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        throw "$Label output parent does not exist: $parent"
    }
}

$TargetEquivalence = Assert-RepositoryRelativeArtifactPath $TargetEquivalence 'TargetEquivalence'
$LayoutEvidence = Assert-RepositoryRelativeArtifactPath $LayoutEvidence 'LayoutEvidence'
$Catalog = Assert-RepositoryRelativeArtifactPath $Catalog 'Catalog'
$Graph = Assert-RepositoryRelativeArtifactPath $Graph 'Graph'
$Plan = Assert-RepositoryRelativeArtifactPath $Plan 'Plan'
$LoopGate = Assert-RepositoryRelativeArtifactPath $LoopGate 'LoopGate'
$ActiveDecision = Assert-RepositoryRelativeArtifactPath $ActiveDecision 'ActiveDecision'
$OfflineSingle = Assert-RepositoryRelativeArtifactPath $OfflineSingle 'OfflineSingle' -AllowEmpty
$LocalCoopDual = Assert-RepositoryRelativeArtifactPath $LocalCoopDual 'LocalCoopDual' -AllowEmpty
$OriginalOnlineDual = Assert-RepositoryRelativeArtifactPath $OriginalOnlineDual 'OriginalOnlineDual' -AllowEmpty
$ComparisonOutput = Assert-RepositoryRelativeArtifactPath $ComparisonOutput 'ComparisonOutput' -AllowEmpty
$DecisionOutput = Assert-RepositoryRelativeArtifactPath $DecisionOutput 'DecisionOutput' -AllowEmpty
$MatrixOutput = Assert-RepositoryRelativeArtifactPath $MatrixOutput 'MatrixOutput' -AllowEmpty
$LedgerOutput = Assert-RepositoryRelativeArtifactPath $LedgerOutput 'LedgerOutput' -AllowEmpty
$VerificationOutput = Assert-RepositoryRelativeArtifactPath $VerificationOutput 'VerificationOutput' -AllowEmpty
$FinalPlanOutput = Assert-RepositoryRelativeArtifactPath $FinalPlanOutput 'FinalPlanOutput' -AllowEmpty

$manifest = Resolve-RepoPath 'cs-t0/Cargo.toml'
$controllerPath = Resolve-RepoPath $Controller
$gameExePath = Resolve-RepoPath $GameExe

$frozen = [ordered]@{
    $TargetEquivalence = '8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138'
    $LayoutEvidence = '921EE4C4AA1FDCE86E21F1CFF1B38D5802E13E715242EC6006FA79B5F6316059'
    $Catalog = '1059711778DAD09870D37118571EE7D5E67761EC28FFAB84C7CF5AC5998A0224'
    $Graph = 'F87DA1DF383B7B1AE3724A4DB4EAAE791B9F42B68AF9210AD1727F71D9458934'
    'cs-t0/coop-static-decision-v1.json' = '2CA3A9956FEAED6E08BDAA2F89F2A244DE4CF6B6CB0E2AFA0EFAEE7A37C9E459'
    $LoopGate = 'CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA'
    $ActiveDecision = '927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C'
}
foreach ($entry in $frozen.GetEnumerator()) {
    Assert-Equal (Get-Sha256 $entry.Key) $entry.Value "Frozen artifact $($entry.Key)"
}

$layoutDoc = Get-Content -Raw -LiteralPath (Resolve-RepoPath $LayoutEvidence) | ConvertFrom-Json
Assert-Equal $layoutDoc.schemaVersion 1 'Layout schema'
Assert-Equal $layoutDoc.evidenceIdentity 'IsaacCsT0-CoopC0LayoutEvidence-v1' 'Layout identity'
Assert-Equal $layoutDoc.workPackage 'CS-T0-05-02' 'Layout work package'
Assert-Equal $layoutDoc.status 'STATIC_CONFIRMED' 'Layout status'
Assert-Equal $layoutDoc.tickContract.directPostCommitObservedInC0 $false 'Direct post-commit claim'
Assert-Equal $layoutDoc.tickContract.status 'PASS_INHERITED_CONTRACT' 'Inherited commit contract'

$planDoc = Get-Content -Raw -LiteralPath (Resolve-RepoPath $Plan) | ConvertFrom-Json
Assert-Equal $planDoc.schemaVersion 3 'Plan schema'
Assert-Equal $planDoc.planIdentity 'IsaacCsT0-CoopPlan-v3' 'Plan identity'
Assert-Equal $planDoc.status 'AUTHORIZED_C0_READ_ONLY' 'Plan status'
Assert-Equal $planDoc.c0Authorization.observerAccessBits '0x00101410' 'Observer access mask'
Assert-Equal $planDoc.c0Authorization.processReadAllowed $true 'C0 process read authorization'
foreach ($field in @('runtimeMutationAllowed', 'hookAllowed', 'ipcAllowed', 'mappingAllowed', 'helperAllowed', 'nativeLoadAllowed', 'schedulerAllowed', 'applyAllowed')) {
    Assert-Equal $planDoc.c0Authorization.$field $false "C0 capability $field"
}
Assert-Equal (($planDoc.c0Authorization.scenarios) -join '|') 'offline-single-c0|local-coop-dual-c0|original-online-dual-disarmed' 'C0 scenario set'

Invoke-Checked 'cargo test' { cargo test --manifest-path $manifest --all-targets --locked }
Invoke-Checked 'cargo fmt check' { cargo fmt --manifest-path $manifest -- --check }
Invoke-Checked 'cargo check' { cargo check --manifest-path $manifest --all-targets --locked }
Invoke-Checked 'cargo clippy' { cargo clippy --manifest-path $manifest --all-targets --locked -- -D warnings }

if (-not $SkipFullRegression) {
    Invoke-Checked 'Debug/Release Native build and harness' { & (Resolve-RepoPath 'cs-t0/build.ps1') -Configuration Release }
    Invoke-CheckedPowerShellScript 'CS-T0-05 static verifier' { & (Resolve-RepoPath 'cs-t0/verify-coop-static.ps1') -GameExe $gameExePath }
    Invoke-CheckedPowerShellScript 'T0-04 active regression verifier' { & (Resolve-RepoPath 'cs-t0/verify-active.ps1') }
}

$liveArguments = @($OfflineSingle, $LocalCoopDual, $OriginalOnlineDual, $ComparisonOutput, $DecisionOutput)
$liveArgumentCount = @($liveArguments | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
if ($liveArgumentCount -ne 0 -and $liveArgumentCount -ne $liveArguments.Count) {
    throw 'OfflineSingle, LocalCoopDual, OriginalOnlineDual, ComparisonOutput and DecisionOutput must be supplied together.'
}

$finalOutputs = @($MatrixOutput, $LedgerOutput, $FinalPlanOutput)
$finalOutputCount = @($finalOutputs | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count
if ($finalOutputCount -ne 0 -and $finalOutputCount -ne $finalOutputs.Count) {
    throw 'MatrixOutput, LedgerOutput and FinalPlanOutput must be supplied together.'
}
if ($finalOutputCount -ne 0 -and ($liveArgumentCount -ne $liveArguments.Count -or [string]::IsNullOrWhiteSpace($VerificationOutput))) {
    throw 'Final C0 outputs require all live replay inputs and VerificationOutput.'
}

$createNewTargets = @()
if ($liveArgumentCount -eq $liveArguments.Count) {
    $createNewTargets += @(
        [ordered]@{ path = $ComparisonOutput; label = 'ComparisonOutput' },
        [ordered]@{ path = $DecisionOutput; label = 'DecisionOutput' }
    )
}
if ($finalOutputCount -eq $finalOutputs.Count) {
    $createNewTargets += @(
        [ordered]@{ path = $MatrixOutput; label = 'MatrixOutput' },
        [ordered]@{ path = $LedgerOutput; label = 'LedgerOutput' },
        [ordered]@{ path = $VerificationOutput; label = 'VerificationOutput' },
        [ordered]@{ path = $FinalPlanOutput; label = 'FinalPlanOutput' }
    )
}
elseif (-not [string]::IsNullOrWhiteSpace($VerificationOutput)) {
    $createNewTargets += [ordered]@{ path = $VerificationOutput; label = 'VerificationOutput' }
}
$seenCreateNewTargets = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($target in $createNewTargets) {
    $resolvedTarget = Resolve-RepoPath $target.path
    if (-not $seenCreateNewTargets.Add($resolvedTarget)) {
        throw "Create-new outputs must be distinct; duplicate target: $resolvedTarget"
    }
    Assert-CreateNewTargetAvailable $target.path $target.label
}

if ($liveArgumentCount -eq $liveArguments.Count) {
    foreach ($source in @($OfflineSingle, $LocalCoopDual, $OriginalOnlineDual)) {
        $sourcePath = Resolve-RepoPath $source
        $analysisPath = Join-Path $sourcePath 'analysis.json'
        if (-not (Test-Path -LiteralPath $analysisPath)) {
            Invoke-Checked "coop-analyze $source" {
                & $controllerPath coop-analyze $sourcePath --output $analysisPath
            }
        }
    }
    Invoke-Checked 'coop-compare' {
        & $controllerPath coop-compare `
            --offline-single (Resolve-RepoPath $OfflineSingle) `
            --local-coop-dual (Resolve-RepoPath $LocalCoopDual) `
            --original-online-dual (Resolve-RepoPath $OriginalOnlineDual) `
            --output (Resolve-RepoPath $ComparisonOutput)
    }
    Invoke-Checked 'coop-c0-gate' {
        & $controllerPath coop-c0-gate `
            --offline-single (Resolve-RepoPath $OfflineSingle) `
            --local-coop-dual (Resolve-RepoPath $LocalCoopDual) `
            --original-online-dual (Resolve-RepoPath $OriginalOnlineDual) `
            --comparison (Resolve-RepoPath $ComparisonOutput) `
            --loop-gate (Resolve-RepoPath $LoopGate) `
            --active-decision (Resolve-RepoPath $ActiveDecision) `
            --output (Resolve-RepoPath $DecisionOutput)
    }
}

if (-not [string]::IsNullOrWhiteSpace($VerificationOutput) -and $finalOutputCount -eq 0) {
    $bindings = @(
        [ordered]@{ path = $TargetEquivalence; identity = 'IsaacCsT0-UnpackedTargetEquivalence-v1'; sha256 = Get-Sha256 $TargetEquivalence },
        [ordered]@{ path = $LayoutEvidence; identity = 'IsaacCsT0-CoopC0LayoutEvidence-v1'; sha256 = Get-Sha256 $LayoutEvidence },
        [ordered]@{ path = $Catalog; identity = 'IsaacCsT0-CoopCatalog-v2'; sha256 = Get-Sha256 $Catalog },
        [ordered]@{ path = $Graph; identity = 'IsaacCsT0-CoopStaticGraph-v1'; sha256 = Get-Sha256 $Graph },
        [ordered]@{ path = $Plan; identity = 'IsaacCsT0-CoopPlan-v3'; sha256 = Get-Sha256 $Plan },
        [ordered]@{ path = $LoopGate; identity = 'IsaacCsT0-LoopGateDecision-v2'; sha256 = Get-Sha256 $LoopGate },
        [ordered]@{ path = $ActiveDecision; identity = 'IsaacCsT0-ActiveGateDecision-v4'; sha256 = Get-Sha256 $ActiveDecision }
    )
    $checks = @(
        'frozen-artifact-bindings',
        'strict-layout-contract',
        'exact-c0-read-authorization',
        'rust-test-all-targets',
        'rust-fmt-check',
        'rust-check-all-targets',
        'rust-clippy-deny-warnings'
    )
    if (-not $SkipFullRegression) {
        $checks += @('native-debug-release-harness', 'coop-static-regression', 't0-04-active-regression')
    }
    $verification = [ordered]@{
        schemaVersion = 3
        verificationIdentity = 'IsaacCsT0-CoopVerification-v3'
        workPackage = 'CS-T0-05-02'
        createdUtc = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
        status = if ($liveArgumentCount -eq $liveArguments.Count) { 'PASS_C0_COMPLETE' } else { 'PASS_AUTOMATED_PREFLIGHT' }
        staticContractComplete = $true
        runtimeContractComplete = ($liveArgumentCount -eq $liveArguments.Count)
        processReadAllowed = $true
        observerAccessBits = '0x00101410'
        directPostCommitObserved = $false
        authorityCommitStatus = if ($liveArgumentCount -eq $liveArguments.Count) { 'PASS_INHERITED_CONTRACT' } else { 'PENDING_INHERITED_GATE' }
        artifactBindings = $bindings
        checks = @($checks | ForEach-Object { [ordered]@{ id = $_; passed = $true } })
        permissions = [ordered]@{
            activeInputAllowed = $false
            passiveProbeAllowed = $false
            runtimeMutationAllowed = $false
            t005Unlocked = $false
            csT00503Unlocked = ($liveArgumentCount -eq $liveArguments.Count)
        }
    }
    Write-JsonCreateNew -Path $VerificationOutput -Value $verification
}

if ($finalOutputCount -eq $finalOutputs.Count) {
    $comparisonDoc = Get-Content -Raw -LiteralPath (Resolve-RepoPath $ComparisonOutput) | ConvertFrom-Json
    $decisionDoc = Get-Content -Raw -LiteralPath (Resolve-RepoPath $DecisionOutput) | ConvertFrom-Json
    Assert-Equal $comparisonDoc.result 'PASS' 'Final comparison result'
    Assert-Equal $comparisonDoc.compositeNoMutationPassed $true 'Composite no-mutation result'
    Assert-Equal $comparisonDoc.compositeNoMutationReport.noFourthLiveCapture $true 'No fourth live capture'
    Assert-Equal $decisionDoc.status 'PASS' 'Final C0 decision status'
    Assert-Equal $decisionDoc.loopGateReplayed $true 'Loop gate replay'
    Assert-Equal $decisionDoc.activeSourcesReplayed $true 'Active source replay'
    Assert-Equal $decisionDoc.authorityCommitStatus 'PASS_INHERITED_CONTRACT' 'Inherited authority status'
    Assert-Equal $decisionDoc.directPostCommitObserved $false 'Direct post-commit observation'
    Assert-Equal $decisionDoc.csT00503Unlocked $true 'CS-T0-05-03 unlock'
    Assert-Equal $decisionDoc.t005Unlocked $false 'T0-05 total unlock'
    Assert-Equal $decisionDoc.activeInputAllowed $false 'Active input permission'
    Assert-Equal $decisionDoc.passiveProbeAllowed $false 'Passive permission'
    Assert-Equal $decisionDoc.runtimeMutationAllowed $false 'Mutation permission'

    $createdUtc = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
    $sourceRoles = @(
        [ordered]@{ role = 'OFFLINE_SINGLE'; scenario = 'offline-single-c0'; sourcePath = $OfflineSingle },
        [ordered]@{ role = 'LOCAL_COOP_DUAL'; scenario = 'local-coop-dual-c0'; sourcePath = $LocalCoopDual },
        [ordered]@{ role = 'ORIGINAL_ONLINE_DUAL'; scenario = 'original-online-dual-disarmed'; sourcePath = $OriginalOnlineDual }
    )
    $sourceBindings = @($sourceRoles | ForEach-Object {
        $analysisPath = Join-Path $_.sourcePath 'analysis.json'
        $manifestPath = Join-Path $_.sourcePath 'manifest.json'
        $samplesPath = Join-Path $_.sourcePath 'samples.jsonl'
        [ordered]@{
            role = $_.role
            scenario = $_.scenario
            sourcePath = $_.sourcePath
            analysisSha256 = Get-Sha256 $analysisPath
            manifestSha256 = Get-Sha256 $manifestPath
            samplesSha256 = Get-Sha256 $samplesPath
        }
    })

    $matrix = [ordered]@{
        schemaVersion = 2
        matrixIdentity = 'IsaacCsT0-CoopMatrix-v2'
        workPackage = 'CS-T0-05-02'
        createdUtc = $createdUtc
        status = 'PASS_C0_4_OF_4'
        supersedes = 'coop-matrix-v1.template.json'
        supersedesSha256 = Get-Sha256 'cs-t0/coop-matrix-v1.template.json'
        c0Rows = @(
            [ordered]@{ ordinal = 1; caseName = 'control-no-native'; result = 'PASS_COMPOSITE'; evidencePath = $ComparisonOutput; evidenceSha256 = Get-Sha256 $ComparisonOutput },
            [ordered]@{ ordinal = 2; caseName = 'offline-single-c0'; result = 'PASS'; evidencePath = (Join-Path $OfflineSingle 'analysis.json'); evidenceSha256 = Get-Sha256 (Join-Path $OfflineSingle 'analysis.json') },
            [ordered]@{ ordinal = 3; caseName = 'local-coop-dual-c0'; result = 'PASS'; evidencePath = (Join-Path $LocalCoopDual 'analysis.json'); evidenceSha256 = Get-Sha256 (Join-Path $LocalCoopDual 'analysis.json') },
            [ordered]@{ ordinal = 4; caseName = 'original-online-dual-disarmed'; result = 'PASS'; evidencePath = (Join-Path $OriginalOnlineDual 'analysis.json'); evidenceSha256 = Get-Sha256 (Join-Path $OriginalOnlineDual 'analysis.json') }
        )
        sourceBindings = $sourceBindings
        comparisonPath = $ComparisonOutput
        comparisonSha256 = Get-Sha256 $ComparisonOutput
        decisionPath = $DecisionOutput
        decisionSha256 = Get-Sha256 $DecisionOutput
        compositeNoMutationPassed = $true
        noFourthLiveCapture = $true
        deferredBasePrimaryOrdinals = @(5..20)
        virtualDeviceDecision = 'UNKNOWN'
        permissions = [ordered]@{ activeInputAllowed = $false; passiveProbeAllowed = $false; runtimeMutationAllowed = $false; t005Unlocked = $false; csT00503Unlocked = $true }
    }
    Write-JsonCreateNew -Path $MatrixOutput -Value $matrix

    $ledger = [ordered]@{
        schemaVersion = 3
        ledgerIdentity = 'IsaacCsT0-CoopLedger-v3'
        workPackage = 'CS-T0-05-02'
        createdUtc = $createdUtc
        status = 'PASS_C0_COMPLETE'
        supersedes = 'isaac-cs-t0-coop-ledger-v2.json'
        supersedesSha256 = Get-Sha256 'cs-t0/ledger/isaac-cs-t0-coop-ledger-v2.json'
        evidenceDomain = 'T0-05_THREE_MODE_C0'
        sourceBindings = $sourceBindings
        comparison = [ordered]@{ path = $ComparisonOutput; sha256 = Get-Sha256 $ComparisonOutput; result = 'PASS' }
        decision = [ordered]@{ path = $DecisionOutput; sha256 = Get-Sha256 $DecisionOutput; status = 'PASS' }
        matrix = [ordered]@{ path = $MatrixOutput; sha256 = Get-Sha256 $MatrixOutput; status = 'PASS_C0_4_OF_4' }
        authority = [ordered]@{ tickProxyStatus = 'PASS_STEADY_STANDARD_PRE_ATTEMPT_MONOTONIC'; directPostCommitObserved = $false; commitStatus = 'PASS_INHERITED_CONTRACT'; loopGateSha256 = $decisionDoc.loopGateSha256; activeDecisionSha256 = $decisionDoc.activeDecisionSha256 }
        privacy = [ordered]@{ recursiveScanPassed = $true; rawRuntimeIdentityPersisted = $false; absoluteUserPathPersisted = $false }
        permissions = [ordered]@{ activeInputAllowed = $false; passiveProbeAllowed = $false; runtimeMutationAllowed = $false; t005Unlocked = $false; csT00503Unlocked = $true }
        exitGate = [ordered]@{ threeModeAnalysisPassed = $true; comparisonPassed = $true; compositeNoMutationPassed = $true; inheritedTickGatePassed = $true; result = 'PASS' }
    }
    Write-JsonCreateNew -Path $LedgerOutput -Value $ledger

    $bindings = @(
        [ordered]@{ path = $TargetEquivalence; identity = 'IsaacCsT0-UnpackedTargetEquivalence-v1'; sha256 = Get-Sha256 $TargetEquivalence },
        [ordered]@{ path = $LayoutEvidence; identity = 'IsaacCsT0-CoopC0LayoutEvidence-v1'; sha256 = Get-Sha256 $LayoutEvidence },
        [ordered]@{ path = $Catalog; identity = 'IsaacCsT0-CoopCatalog-v2'; sha256 = Get-Sha256 $Catalog },
        [ordered]@{ path = $Graph; identity = 'IsaacCsT0-CoopStaticGraph-v1'; sha256 = Get-Sha256 $Graph },
        [ordered]@{ path = $Plan; identity = 'IsaacCsT0-CoopPlan-v3'; sha256 = Get-Sha256 $Plan },
        [ordered]@{ path = $LoopGate; identity = 'IsaacCsT0-LoopGateDecision-v2'; sha256 = Get-Sha256 $LoopGate },
        [ordered]@{ path = $ActiveDecision; identity = 'IsaacCsT0-ActiveGateDecision-v4'; sha256 = Get-Sha256 $ActiveDecision },
        [ordered]@{ path = $ComparisonOutput; identity = 'IsaacCsT0-CoopC0Comparison-v1'; sha256 = Get-Sha256 $ComparisonOutput },
        [ordered]@{ path = $DecisionOutput; identity = 'IsaacCsT0-CoopC0Decision-v1'; sha256 = Get-Sha256 $DecisionOutput },
        [ordered]@{ path = $MatrixOutput; identity = 'IsaacCsT0-CoopMatrix-v2'; sha256 = Get-Sha256 $MatrixOutput },
        [ordered]@{ path = $LedgerOutput; identity = 'IsaacCsT0-CoopLedger-v3'; sha256 = Get-Sha256 $LedgerOutput }
    )
    $checks = @(
        'frozen-artifact-bindings', 'strict-layout-contract', 'three-source-analysis-replay',
        'three-mode-comparison-replay', 'composite-no-mutation', 'target-53-slot-stability',
        'recursive-artifact-privacy', 't0-03r-loop-gate-replay', 't0-04-active-source-replay',
        'tick-path-and-pre-attempt-proxy', 'rust-test-all-targets', 'rust-fmt-check',
        'rust-check-all-targets', 'rust-clippy-deny-warnings'
    )
    if (-not $SkipFullRegression) {
        $checks += @('native-debug-release-harness', 'coop-static-regression', 't0-04-active-regression')
    }
    $verification = [ordered]@{
        schemaVersion = 4
        verificationIdentity = 'IsaacCsT0-CoopVerification-v4'
        workPackage = 'CS-T0-05-02'
        createdUtc = $createdUtc
        status = 'PASS_C0_COMPLETE'
        supersedes = 'coop-verification-v3.json'
        supersedesSha256 = Get-Sha256 'cs-t0/coop-verification-v3.json'
        staticContractComplete = $true
        runtimeContractComplete = $true
        completedLiveSourceCount = 3
        finalProcessReadAllowed = $false
        finalObserverAccessBits = '0x00000000'
        directPostCommitObserved = $false
        authorityCommitStatus = 'PASS_INHERITED_CONTRACT'
        artifactBindings = $bindings
        sourceBindings = $sourceBindings
        checks = @($checks | ForEach-Object { [ordered]@{ id = $_; passed = $true } })
        permissions = [ordered]@{ activeInputAllowed = $false; passiveProbeAllowed = $false; runtimeMutationAllowed = $false; t005Unlocked = $false; csT00503Unlocked = $true }
    }
    Write-JsonCreateNew -Path $VerificationOutput -Value $verification

    $finalPlan = [ordered]@{
        schemaVersion = 4
        planIdentity = 'IsaacCsT0-CoopPlan-v4'
        workPackage = 'CS-T0-05-02'
        createdUtc = $createdUtc
        status = 'C0_COMPLETE_READ_CLOSED'
        supersedes = 'coop-plan-v3.json'
        supersedesSha256 = Get-Sha256 'cs-t0/coop-plan-v3.json'
        createNewOnly = $true
        artifactBindings = @(
            [ordered]@{ path = $DecisionOutput; identity = 'IsaacCsT0-CoopC0Decision-v1'; sha256 = Get-Sha256 $DecisionOutput },
            [ordered]@{ path = $VerificationOutput; identity = 'IsaacCsT0-CoopVerification-v4'; sha256 = Get-Sha256 $VerificationOutput },
            [ordered]@{ path = $MatrixOutput; identity = 'IsaacCsT0-CoopMatrix-v2'; sha256 = Get-Sha256 $MatrixOutput },
            [ordered]@{ path = $LedgerOutput; identity = 'IsaacCsT0-CoopLedger-v3'; sha256 = Get-Sha256 $LedgerOutput }
        )
        c0Authorization = [ordered]@{
            probe = 'control-no-native'
            scenarios = @('offline-single-c0', 'local-coop-dual-c0', 'original-online-dual-disarmed')
            previousObserverAccessBits = '0x00101410'
            observerAccessBits = '0x00000000'
            processReadAllowed = $false
            runtimeMutationAllowed = $false
            hookAllowed = $false
            ipcAllowed = $false
            mappingAllowed = $false
            helperAllowed = $false
            nativeLoadAllowed = $false
            schedulerAllowed = $false
            applyAllowed = $false
            completedSourceCount = 3
            furtherC0CaptureAllowed = $false
        }
        permissions = [ordered]@{ activeInputAllowed = $false; passiveProbeAllowed = $false; runtimeMutationAllowed = $false; t005Unlocked = $false; csT00503Unlocked = $true }
        nextWorkPackage = 'CS-T0-05-03'
    }
    Write-JsonCreateNew -Path $FinalPlanOutput -Value $finalPlan
}

[PSCustomObject]@{
    WorkPackage = 'CS-T0-05-02'
    Status = if ($liveArgumentCount -eq $liveArguments.Count) { 'PASS_C0_COMPLETE' } else { 'PASS_AUTOMATED_PREFLIGHT' }
    ObserverAccessBits = if ($finalOutputCount -eq $finalOutputs.Count) { '0x00000000' } else { '0x00101410' }
    ProcessReadAllowed = ($finalOutputCount -ne $finalOutputs.Count)
    LiveSourcesComplete = ($liveArgumentCount -eq $liveArguments.Count)
    DirectPostCommitObserved = $false
    AuthorityCommitStatus = if ($liveArgumentCount -eq $liveArguments.Count) { 'PASS_INHERITED_CONTRACT' } else { 'PENDING_INHERITED_GATE' }
    ActiveInputAllowed = $false
    RuntimeMutationAllowed = $false
} | Format-List
