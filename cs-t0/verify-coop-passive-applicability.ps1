[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameExe,
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$VerificationOutput,
    [switch]$PublishFinalArtifacts,
    [switch]$TestOnlyExportInvocation
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$contractPath = Join-Path $RepositoryRoot 'cs-t0/coop-native-exit-applicability-v1.json'
$contract = Get-Content -LiteralPath $contractPath -Raw | ConvertFrom-Json
$outputRoot = Join-Path $RepositoryRoot 'out/cs-t0'
$controller = Join-Path $outputRoot 'IsaacCsT0.exe'
if (-not $VerificationOutput) { $VerificationOutput = Join-Path $RepositoryRoot 'cs-t0/coop-verification-v5r7.json' }
$finalPaths = @(
    'cs-t0/coop-passive-decision-v3.json', 'cs-t0/coop-matrix-v3.json',
    'cs-t0/ledger/isaac-cs-t0-coop-ledger-v4.json', 'cs-t0/coop-hypothesis-index-v3.json',
    'cs-t0/coop-verification-v6.json', 'cs-t0/coop-plan-v6.json',
    'cs-t0/coop-passive-live-progress-v8.json'
)

function Assert-Exact {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Hash-File {
    param([string]$Path)
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

function Get-Binding {
    param([string]$RelativePath)
    [ordered]@{ path = $RelativePath; sha256 = Hash-File (Join-Path $RepositoryRoot $RelativePath) }
}

function Write-NewJson {
    param([string]$Path, [object]$Value)
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Depth 50) + [Environment]::NewLine)
    $stream = [IO.File]::Open($Path, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write)
    try { $stream.Write($bytes, 0, $bytes.Length); $stream.Flush($true) }
    finally { $stream.Dispose() }
}

function Assert-FrozenInputs {
    foreach ($binding in @($contract.staticArtifacts) + @($contract.baseline)) {
        Assert-Exact ((Hash-File (Join-Path $RepositoryRoot $binding.path)) -ceq $binding.sha256) "frozen input changed: $($binding.path)"
    }
    $baseline = Get-Content -LiteralPath (Join-Path $RepositoryRoot $contract.baseline.path) -Raw | ConvertFrom-Json
    foreach ($binding in $baseline.bindings) {
        Assert-Exact ((Hash-File (Join-Path $RepositoryRoot $binding.path)) -ceq $binding.sha256) "immutable baseline changed: $($binding.path)"
    }
}

function Get-GateInvocation {
    $gateArguments = [Collections.Generic.List[string]]::new()
    $legacyArguments = [Collections.Generic.List[string]]::new()
    $exitSource = Join-Path $RepositoryRoot $contract.historicalExit.sourcePath
    $gateArguments.AddRange([string[]]@('coop-passive-applicability-gate', '--repository-root', $RepositoryRoot,
        '--applicability', $contractPath, '--native-exit-source', $exitSource))
    $legacyArguments.Add('coop-passive-gate')
    foreach ($source in $contract.sources) {
        # Add each argv element separately: PowerShell '+' and ',' precedence
        # must never merge a scenario option with its source path.
        $flag = '--' + $source.scenario
        $sourcePath = Join-Path $RepositoryRoot $source.sourcePath
        $gateArguments.Add($flag)
        $gateArguments.Add($sourcePath)
        $legacyArguments.Add($flag)
        $legacyArguments.Add($sourcePath)
    }
    $legacyArguments.Add('--native-exit-passive')
    $legacyArguments.Add($exitSource)
    [pscustomobject]@{ gateArguments = $gateArguments.ToArray(); legacyArguments = $legacyArguments.ToArray() }
}

$invocation = Get-GateInvocation
if ($TestOnlyExportInvocation) {
    Assert-Exact (-not $PSBoundParameters.ContainsKey('PublishFinalArtifacts') -and -not $PSBoundParameters.ContainsKey('VerificationOutput')) 'test-only invocation export cannot publish verification or final artifacts'
    # A public, read-only test export, not a partial verifier or an authorization.
    # It never executes a controller, reads GameExe, creates a directory or file.
    [ordered]@{
        schemaVersion = 1
        identity = 'IsaacCsT0-CoopPassiveGateInvocation-TestOnly-v1'
        status = 'TEST_ONLY_INVOCATION_NOT_VERIFIED'
        verificationEligible = $false
        liveAuthorizationCreated = $false
        gateArguments = $invocation.gateArguments
        legacyArguments = $invocation.legacyArguments
    } | ConvertTo-Json -Depth 5
    $global:LASTEXITCODE = 0
    return
}
Assert-Exact ($invocation.gateArguments.Count -eq 17 -and $invocation.legacyArguments.Count -eq 13) 'gate invocation must contain distinct option/value elements'

Assert-Exact (-not (Test-Path -LiteralPath $VerificationOutput)) 'verification output already exists; create-new only'
$verificationParent = (Resolve-Path -LiteralPath (Split-Path -Parent ([IO.Path]::GetFullPath($VerificationOutput)))).Path
Assert-Exact ($verificationParent -ceq $RepositoryRoot -or $verificationParent.StartsWith($RepositoryRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) 'verification output must remain inside repository root'
if ($PublishFinalArtifacts) {
    foreach ($path in $finalPaths) { Assert-Exact (-not (Test-Path -LiteralPath (Join-Path $RepositoryRoot $path))) "final output already exists: $path" }
}
Assert-FrozenInputs

# Disk-only static verification. No process handle, game control, injection or live authorization.
$static = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'cs-t0/coop-native-exit-applicability-static-v1.json') -Raw | ConvertFrom-Json
Assert-Exact ((Hash-File $GameExe) -ceq $static.target.observedApprovedUnpackedSha256) 'static target hash mismatch'
$targetBytes = [IO.File]::ReadAllBytes($GameExe)
$peOffset = [BitConverter]::ToInt32($targetBytes, 0x3c)
$sectionCount = [BitConverter]::ToUInt16($targetBytes, $peOffset + 6)
$sectionTable = $peOffset + 24 + [BitConverter]::ToUInt16($targetBytes, $peOffset + 20)
$imageBase = [BitConverter]::ToUInt32($targetBytes, $peOffset + 24 + 28)
Assert-Exact ([BitConverter]::ToUInt16($targetBytes, $peOffset + 4) -eq 0x14c) 'static target is not x86'
Assert-Exact (@($static.anchors).Count -eq 29) 'exact 29 static anchors required'
foreach ($anchor in $static.anchors) {
    $rva = [Convert]::ToUInt32($anchor.rva.Substring(2), 16)
    $expected = [Convert]::FromHexString($anchor.expectedBytes)
    $raw = -1
    for ($section = 0; $section -lt $sectionCount; $section++) {
        $offset = $sectionTable + 40 * $section
        $va = [BitConverter]::ToUInt32($targetBytes, $offset + 12)
        $rawSize = [BitConverter]::ToUInt32($targetBytes, $offset + 16)
        if ($rva -ge $va -and [uint64]$rva + $expected.Length -le [uint64]$va + $rawSize) {
            $raw = [BitConverter]::ToUInt32($targetBytes, $offset + 20) + ($rva - $va)
            break
        }
    }
    Assert-Exact ($raw -ge 0 -and $raw + $expected.Length -le $targetBytes.Length) "static RVA mapping failed: $($anchor.id)"
    $actual = [Convert]::ToHexString($targetBytes, $raw, $expected.Length)
    Assert-Exact ($actual -ceq $anchor.expectedBytes) "static bytes drifted: $($anchor.id)"
    if ($anchor.kind -ceq 'CALL_REL32') {
        $target = [int64]$rva + 5 + [BitConverter]::ToInt32($targetBytes, $raw + 1)
        Assert-Exact ($target -eq [Convert]::ToUInt32($anchor.targetRva.Substring(2), 16)) "static call target drifted: $($anchor.id)"
    }
    elseif ($anchor.kind -ceq 'IMAGE_POINTER32') {
        $target = [uint64][BitConverter]::ToUInt32($targetBytes, $raw) - $imageBase
        Assert-Exact ($target -eq [Convert]::ToUInt32($anchor.targetRva.Substring(2), 16)) "static pointer target drifted: $($anchor.id)"
    }
}
Write-Output 'PASS native-exit-static-target-bytes (29 anchors)'

# Keep the predecessor verifier byte-for-byte intact, and run its full suite.
& (Join-Path $RepositoryRoot 'cs-t0/verify-coop-passive.ps1') -GameExe $GameExe -RepositoryRoot $RepositoryRoot | Tee-Object -Variable inheritedOutput
Assert-Exact ($LASTEXITCODE -eq 0) 'inherited full passive automatic gate failed'

# cargo test --all-targets above includes the independent suite. Require every
# named case's actual success line, so filtered/zero-test execution cannot pass.
$requiredApplicabilityTests = @(
    'applicability_five_live_sources_without_verification_are_evidence_only',
    'applicability_public_replays_preserve_five_passes_and_live07_blocked_bytes',
    'legacy_six_source_cli_still_rejects_only_five_and_applicability_switches',
    'applicability_gate_output_is_create_new_even_for_valid_evidence',
    'applicability_cli_rejects_unknown_skip_and_duplicate_source_flags',
    'applicability_cli_rejects_each_missing_applicable_source',
    'applicability_decisions_cannot_be_written_inside_any_source',
    'applicability_contract_rejects_identity_scope_hash_skip_and_capability_drift',
    'applicability_contract_rejects_unknown_duplicate_whitespace_and_generic_skip_documents',
    'applicability_rejects_each_source_file_rewrite_and_live07_pass_forgery',
    'applicability_never_waives_source_safety_zero_capabilities_or_privacy',
    'applicability_rejects_wrong_duplicate_missing_and_outside_root_source_paths',
    'applicability_checks_every_static_and_recursive_historical_baseline_binding',
    'applicability_formal_branch_accepts_only_complete_test_fixture_verification',
    'applicability_formal_branch_rejects_old_partial_forged_and_widened_verifications',
    'applicability_formal_verification_rejects_executable_substitution_and_duplicate_permissions',
    'applicability_powershell_invocation_preserves_distinct_sources_and_cannot_publish'
)
foreach ($testName in $requiredApplicabilityTests) {
    Assert-Exact (@($inheritedOutput | Where-Object { $_ -ceq "test $testName ... ok" }).Count -ge 1) "independent applicability test did not run and pass: $testName"
}
Write-Output 'PASS applicability-black-box-contract (17 exact test cases)'

$replayRoot = Join-Path $outputRoot ('passive-applicability-replay-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $replayRoot | Out-Null
$gateArgs = $invocation.gateArguments
$legacyArgs = $invocation.legacyArguments
foreach ($source in $contract.sources) {
    $path = Join-Path $RepositoryRoot $source.sourcePath
    $replay = Join-Path $replayRoot ($source.scenario + '.json')
    & $controller coop-analyze $path --output $replay
    Assert-Exact ($LASTEXITCODE -eq 0) "source replay failed: $($source.scenario)"
    Assert-Exact ((Hash-File $replay) -ceq $source.analysisSha256) 'source replay analysis changed'
}
$exitReplay = Join-Path $replayRoot 'native-exit-still-blocked.json'
& $controller coop-analyze (Join-Path $RepositoryRoot $contract.historicalExit.sourcePath) --output $exitReplay
Assert-Exact ($LASTEXITCODE -eq 1) 'historical exit must remain rejected'
Assert-Exact ((Hash-File $exitReplay) -ceq $contract.historicalExit.analysisSha256) 'historical exit analysis changed'
$legacyOutput = Join-Path $replayRoot 'legacy-six-source-still-blocked.json'
$legacyArgs += @('--output', $legacyOutput)
& $controller @legacyArgs
Assert-Exact ($LASTEXITCODE -eq 1) 'legacy six-source gate must remain rejected'
$legacyDecision = Get-Content -LiteralPath $legacyOutput -Raw | ConvertFrom-Json
Assert-Exact ($legacyDecision.status -ceq 'BLOCKED_BY_EVIDENCE' -and $legacyDecision.csT00504Unlocked -eq $false) 'legacy gate was relaxed'

$evidenceOutput = Join-Path $replayRoot 'applicability-evidence-only.json'
& $controller @gateArgs --output $evidenceOutput
Assert-Exact ($LASTEXITCODE -eq 0) 'five-source applicability pre-verification failed'
$evidence = Get-Content -LiteralPath $evidenceOutput -Raw | ConvertFrom-Json
Assert-Exact ($evidence.status -ceq 'PASS_EVIDENCE_ONLY' -and $evidence.csT00504Unlocked -eq $false) 'evidence-only gate granted permissions'
Assert-FrozenInputs

$verification = [ordered]@{
    schemaVersion = 5
    verificationIdentity = 'IsaacCsT0-CoopVerification-v5r7'
    workPackage = 'CS-T0-05-03'
    createdUtc = [DateTime]::UtcNow.ToString('o')
    status = 'PASS'
    phase = 'PASSIVE_APPLICABILITY_MIGRATION_AUTOMATED_GATE'
    createNewOnly = $true
    fullGate = $true
    predecessorVerification = $contract.predecessorVerification
    artifactBindings = @($contract.verificationArtifactPaths | ForEach-Object { Get-Binding $_ })
    productionBindings = @('out/cs-t0/IsaacCsT0.exe', 'out/cs-t0/IsaacCsT0.Native.dll', 'out/cs-t0/IsaacCsT0.Helper.exe' | ForEach-Object { Get-Binding $_ })
    checks = @($contract.requiredAutomaticChecks | ForEach-Object { [ordered]@{id = $_; passed = $true} })
    completedLiveSourceCount = 5
    notApplicableCount = 1
    historicalExitResult = 'BLOCKED_BY_EVIDENCE'
    sixSourceGateEligible = $false
    liveAuthorizationCreated = $false
    permissions = $contract.zeroCapabilities
}
Write-NewJson $VerificationOutput $verification
$decisionOutput = if ($PublishFinalArtifacts) { Join-Path $RepositoryRoot $finalPaths[0] } else { Join-Path $replayRoot 'formal-applicability-decision.json' }
& $controller @gateArgs --verification $VerificationOutput --output $decisionOutput
Assert-Exact ($LASTEXITCODE -eq 0) 'formal applicability gate failed'
$decision = Get-Content -LiteralPath $decisionOutput -Raw | ConvertFrom-Json
Assert-Exact ($decision.status -ceq 'PASS' -and $decision.csT00504Unlocked -eq $true) 'formal gate did not grant scoped work-package eligibility'
foreach ($permission in $decision.permissions.PSObject.Properties) {
    Assert-Exact ($permission.Value -eq ($permission.Name -ceq 'csT00504Unlocked')) "formal gate widened capability: $($permission.Name)"
}
Assert-FrozenInputs

if ($PublishFinalArtifacts) {
    # Acyclic create-new chain: decision -> matrix -> ledger -> hypothesis -> verification -> plan -> progress.
    $created = [DateTime]::UtcNow.ToString('o')
    $decisionBinding = Get-Binding $finalPaths[0]
    $template = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'cs-t0/coop-matrix-v1.template.json') -Raw | ConvertFrom-Json
    $oldMatrix = Get-Content -LiteralPath (Join-Path $RepositoryRoot 'cs-t0/coop-matrix-v2.json') -Raw | ConvertFrom-Json
    $rows = @($template.basePrimaryCases | ForEach-Object {
        $row = [ordered]@{ ordinal = $_.ordinal; caseName = $_.caseName; tier = $_.tier; requiredSource = $true; status = 'NOT_STARTED'; evidence = $null }
        if ($_.ordinal -le 4) { $previousRow = $oldMatrix.c0Rows[$_.ordinal - 1]; $row.status = $previousRow.result; $row.evidence = [ordered]@{path = $previousRow.evidencePath.Replace('\', '/'); sha256 = $previousRow.evidenceSha256} }
        elseif ($_.ordinal -eq 6) { $row.tier = 'APPLICABILITY'; $row.requiredSource = $false; $row.status = 'NOT_APPLICABLE_NATIVE_ROUTE'; $row.evidence = Get-Binding 'cs-t0/coop-native-exit-applicability-v1.json'; $row.historicalSourceResult = 'BLOCKED_BY_EVIDENCE' }
        elseif ($_.ordinal -le 10) { $row.status = 'PASS'; $row.evidence = @($contract.sources | Where-Object scenario -CEQ $row.caseName)[0] }
        $row
    })
    $matrix = [ordered]@{
        schemaVersion=3; matrixIdentity='IsaacCsT0-CoopMatrix-v3'; workPackage='CS-T0-05-03'; createdUtc=$created; status='PASSIVE_COMPLETE_T005_INCOMPLETE'; createNewOnly=$true
        supersedes=(Get-Binding 'cs-t0/coop-matrix-v2.json'); decision=$decisionBinding
        basePrimaryCases=$rows; baseEntryCount=20; requiredApplicableBasePrimaryCount=19; completedApplicableBasePrimaryCount=9; notApplicableCount=1
        passiveApplicablePassCount=5; virtualDeviceDecision='UNKNOWN'; conditionalPrimaryCases=$template.conditionalPrimaryCases; repeatabilityCases=$template.repeatabilityCases
        comparisonPath=$null; remainingRequiredBasePrimaryOrdinals=@(11..20); t005Unlocked=$false
    }
    Write-NewJson (Join-Path $RepositoryRoot $finalPaths[1]) $matrix
    $ledger = [ordered]@{
        schemaVersion=4; ledgerIdentity='IsaacCsT0-CoopLedger-v4'; workPackage='CS-T0-05-03'; createdUtc=$created; status='PASS_SCOPED_PASSIVE_APPLICABILITY'; createNewOnly=$true
        supersedes=(Get-Binding 'cs-t0/ledger/isaac-cs-t0-coop-ledger-v3.json'); decision=$decisionBinding; matrix=(Get-Binding $finalPaths[1]); sourceBindings=$contract.sources
        nativeExitApplicability=(Get-Binding 'cs-t0/coop-native-exit-applicability-v1.json'); historicalExit=$contract.historicalExit; capabilityLimitations=$contract.capabilityLimitations; permissions=$decision.permissions
    }
    Write-NewJson (Join-Path $RepositoryRoot $finalPaths[2]) $ledger
    $hypothesis = [ordered]@{
        schemaVersion=3; indexIdentity='IsaacCsT0-CoopHypothesisIndex-v3'; workPackage='CS-T0-05-03'; createdUtc=$created; status='PASSIVE_SCOPED_EVIDENCE_COMPLETE'; createNewOnly=$true
        supersedes=(Get-Binding 'cs-t0/coop-hypothesis-index-v2.json'); decision=$decisionBinding; ledger=(Get-Binding $finalPaths[2])
        ordinaryNativeExitHypothesis='FALSIFIED_FOR_FROZEN_VARIANT0_CONSTRUCTION'; nativeExitApplicability='NOT_APPLICABLE_NATIVE_ROUTE'
        resolvedQuestions=@('stable-and-ephemeral-mapping-with-frozen-lifecycle-provenance')
        openQuestions=@('native-join-active-safety', 'virtual-device-necessity', 'dual-endpoint-isolation', 'custom-logical-exit', 'custom-physical-player-removal', 'other-exit-callers-or-variant-changing-states')
        capabilityLimitations=$contract.capabilityLimitations; permissions=$decision.permissions
    }
    Write-NewJson (Join-Path $RepositoryRoot $finalPaths[3]) $hypothesis
    $finalVerification = [ordered]@{
        schemaVersion=6; verificationIdentity='IsaacCsT0-CoopVerification-v6'; workPackage='CS-T0-05-03'; createdUtc=$created; status='PASS'; phase='POST_PASSIVE_APPLICABILITY_GATE'; createNewOnly=$true
        supersedes=[ordered]@{path=$decision.verification.path; sha256=$decision.verification.sha256}
        decision=$decisionBinding; artifactBindings=@($finalPaths[1..3] | ForEach-Object { Get-Binding $_ }); productionBindings=$verification.productionBindings
        requiredApplicableSourceCount=5; completedApplicableSourceCount=5; notApplicableCount=1; sixSourceGateEligible=$false
        nextWorkPackage='CS-T0-05-04'; activeRuntimeVerified=$false; liveAuthorizationCreated=$false; permissions=$decision.permissions
    }
    Write-NewJson (Join-Path $RepositoryRoot $finalPaths[4]) $finalVerification
    $plan = [ordered]@{
        schemaVersion=6; planIdentity='IsaacCsT0-CoopPlan-v6'; workPackage='CS-T0-05-03'; createdUtc=$created; status='PASSIVE_CLOSED_NEXT_WORK_PACKAGE_ELIGIBLE'; createNewOnly=$true
        supersedes=(Get-Binding 'cs-t0/coop-plan-v5r1.json'); artifactBindings=@($finalPaths[0..4] | ForEach-Object { Get-Binding $_ })
        nextWorkPackage='CS-T0-05-04'; permissions=$decision.permissions
        nextStageRequirements=@('Implement and independently verify native-join-active before live authorization', 'Fresh single-run active authorization', 'No implicit authority for synthetic input, custom exit or T0-06')
        passiveCaptureClosed=$true; liveAuthorizationCreated=$false; historicalAuthorizationReusable=$false
    }
    Write-NewJson (Join-Path $RepositoryRoot $finalPaths[5]) $plan
    $progress = [ordered]@{
        schemaVersion=8; progressIdentity='IsaacCsT0-CoopPassiveLiveProgress-v8'; workPackage='CS-T0-05-03'; createdUtc=$created; status='PASS_SCOPED_APPLICABILITY'; createNewOnly=$true
        supersedes=$contract.predecessorProgress; decision=$decisionBinding; plan=(Get-Binding $finalPaths[5]); verification=(Get-Binding $finalPaths[4])
        formalPassCount=5; requiredApplicablePassCount=5; notApplicableCount=1; historicalSixSourcePassCount=5; historicalSixSourceRequiredCount=6
        historicalExitResultChanged=$false; sources=$contract.sources; historicalExit=$contract.historicalExit; capabilityLimitations=$contract.capabilityLimitations
        nextWorkPackage='CS-T0-05-04'; nextScenario=$null; automaticRetryAllowed=$false; liveAuthorizationCreated=$false; permissions=$decision.permissions
    }
    Write-NewJson (Join-Path $RepositoryRoot $finalPaths[6]) $progress
    Assert-FrozenInputs
    foreach ($path in $finalPaths) { Get-Binding $path | ConvertTo-Json -Compress }
}
$global:LASTEXITCODE = 0
[ordered]@{ status='PASS'; automaticCheckCount=28; passivePassSources=5; scopedNotApplicable=1; csT00504Unlocked=$true; activeInputAllowed=$false; t005Unlocked=$false; liveAuthorizationCreated=$false; finalArtifactsPublished=[bool]$PublishFinalArtifacts } | ConvertTo-Json
