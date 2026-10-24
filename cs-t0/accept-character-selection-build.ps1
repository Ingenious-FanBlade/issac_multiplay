param(
    [Parameter(Mandatory=$true)][string]$ArtifactDirectory,
    [string]$BuildArtifactDirectory,
    [string]$Transaction='tx-01-58'
)

# One build/CLI acceptance entry. This never enumerates, opens or injects a game.
$ErrorActionPreference='Stop'
$casePath=[IO.Path]::GetFullPath($ArtifactDirectory)
if(Test-Path -LiteralPath $casePath) {throw 'Use a new artifact directory; no automatic retry.'}
New-Item -ItemType Directory -Path $casePath | Out-Null
$originalPath=$env:Path
$checks=[Collections.Generic.List[object]]::new()
$failure=$null
$stage='toolchain-selection'
$buildLog=Join-Path $casePath 'build.log'
@{tx=$Transaction;scope='Native/helper/controller artifacts and executable CLI acceptance';realGame='NOT_RUN';automaticRetry=$false;buildPerformed=[string]::IsNullOrWhiteSpace($BuildArtifactDirectory);buildArtifactDirectory=$BuildArtifactDirectory} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'invocation.json') -Encoding utf8
function Record-Check([string]$Name,[bool]$Passed,[string[]]$Evidence) {
    # ErrorRecord carries a recursive graph. Its displayed text is the evidence;
    # never retain the PowerShell object in JSON or let it expand in the summary.
    $record=@{name=$Name;status=$(if($Passed){'PASS'}else{'FAIL'});evidence=$Evidence}
    $record | ConvertTo-Json -Depth 4 -Compress | Add-Content -LiteralPath (Join-Path $casePath 'checks.jsonl') -Encoding utf8
    $checks.Add($record)
    if(-not $Passed) {throw "Acceptance failed: $Name; no retry."}
}
try {
    $controller=Join-Path $casePath 'IsaacCsT0.exe'
    if([string]::IsNullOrWhiteSpace($BuildArtifactDirectory)) {
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vsRoot=& $vswhere -latest -version '[17.0,18.0)' -products * -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
    if(-not $vsRoot) {throw 'VS 2022/v143 is required.'}
    $msbuild=Join-Path $vsRoot 'MSBuild\Current\Bin\amd64\MSBuild.exe'
    $toolsVersion=(Get-Content -LiteralPath (Join-Path $vsRoot 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
    $toolBin=Join-Path $vsRoot "VC\Tools\MSVC\$toolsVersion\bin\Hostx64\x86"
    $env:Path="$toolBin;$originalPath"
    @{visualStudio=$vsRoot;toolsVersion=$toolsVersion;host='x64';target='Win32'} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $casePath 'toolchain.json') -Encoding utf8
    $stage='rust-build'
    & cargo build --manifest-path (Join-Path $PSScriptRoot 'Cargo.toml') --locked --release 2>&1 | Tee-Object -FilePath $buildLog -Append
    Record-Check 'controller-build' ($LASTEXITCODE -eq 0) @('build.log',"exitCode=$LASTEXITCODE")
    foreach($target in @('Native','Helper')) {
        $stage="$target-build"
        $targetRoot=Join-Path $casePath "build\$target"
        $binaryDirectory=(Join-Path $targetRoot 'bin')+'\'
        $objectDirectory=(Join-Path $targetRoot 'obj')+'\'
        & $msbuild (Join-Path $PSScriptRoot "native\IsaacCsT0.$target.vcxproj") /m:1 /nr:false /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:PreferredToolArchitecture=x64 "/p:VCToolsVersion=$toolsVersion" "/p:OutDir=$binaryDirectory" "/p:IntDir=$objectDirectory" 2>&1 | Tee-Object -FilePath $buildLog -Append
        Record-Check "$target-build" ($LASTEXITCODE -eq 0) @('build.log',"exitCode=$LASTEXITCODE")
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'target\release\IsaacCsT0.exe') -Destination $controller
    } else {
        $stage='existing-artifacts'
        $buildPath=[IO.Path]::GetFullPath($BuildArtifactDirectory)
        $binaries=@('IsaacCsT0.exe','build\Native\bin\IsaacCsT0.Native.dll','build\Helper\bin\IsaacCsT0.Helper.exe')
        $existing=@($binaries | ForEach-Object {
            $binary=Get-Item -LiteralPath (Join-Path $buildPath $_) -ErrorAction Stop
            @{path=$binary.FullName;length=$binary.Length;lastWriteUtc=$binary.LastWriteTimeUtc.ToString('o')}
        })
        $existing | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $casePath 'existing-artifacts.json') -Encoding utf8
        foreach($binary in $existing) {Record-Check 'existing-binary' ($binary.length -gt 0) @($binary.path,"length=$($binary.length)")}
        Copy-Item -LiteralPath (Join-Path $buildPath 'IsaacCsT0.exe') -Destination $controller
    }
    $stage='cli'
    $listing=@(& $controller coop-active --list-characters 2>&1)
    Record-Check 'character-directory' ($LASTEXITCODE -eq 0 -and @($listing | Where-Object {"$_" -match '^\d+\s'}).Count -eq 41) $listing
    $helpText=@(& $controller coop-active --help 2>&1)
    Record-Check 'character-help' ($LASTEXITCODE -eq 0 -and ($helpText -join "`n").Contains('--b-character')) $helpText
    foreach($choice in @(
        @{text='native';id=$null},@{text='isaac';id=0},@{text='azazel';id=7},@{text='7';id=7},
        @{text='PLAYER_AZAZEL';id=7},@{text='tainted-azazel';id=28},@{text='azazel_b';id=28},
        @{text='lost';id=10},@{text='tainted-lost';id=31},@{text='1000';id=1000}
    )) {
        $output=@(& $controller coop-active --describe-character $choice.text 2>&1)
        $exitCode=$LASTEXITCODE
        $description=($output -join "`n") | ConvertFrom-Json
        Record-Check "describe-$($choice.text)" ($exitCode -eq 0 -and $description.id -eq $choice.id) $output
    }
    foreach($choice in @('unknown-character','-1','2147483648','7.5','')) {
        $output=@(& $controller coop-active --describe-character $choice 2>&1)
        Record-Check "reject-character-$choice" ($LASTEXITCODE -ne 0 -and ($output -join "`n").Contains('invalid B character')) $output
    }
    # Exercise the actual live-command argument path with an invalid script as
    # a deliberate stop before artifact creation and all process operations.
    $invalidScript=Join-Path $casePath 'invalid-script.json'
    '{"steps":"stop-before-game"}' | Set-Content -LiteralPath $invalidScript -Encoding utf8NoBOM
    $capture=Join-Path $casePath 'capture-must-not-exist'
    $baseArgs=@('coop-active','--game-exe','unused','--native-dll','unused','--helper','unused','--artifact-dir',$capture,'--script',$invalidScript,'--authorization','CLI only; no game operations')
    foreach($choice in @('native','azazel','7')) {
        $output=@(& $controller @baseArgs --b-character $choice 2>&1)
        Record-Check "live-parse-$choice" ($LASTEXITCODE -ne 0 -and ($output -join "`n").Contains('script:') -and -not (Test-Path -LiteralPath $capture)) $output
    }
    $output=@(& $controller @baseArgs 2>&1)
    Record-Check 'live-default-native' ($LASTEXITCODE -ne 0 -and ($output -join "`n").Contains('script:') -and -not (Test-Path -LiteralPath $capture)) $output
    $output=@(& $controller @baseArgs --b-character unknown-character 2>&1)
    Record-Check 'live-reject-before-script' ($LASTEXITCODE -ne 0 -and ($output -join "`n").Contains('invalid B character') -and -not (Test-Path -LiteralPath $capture)) $output
    $output=@(& $controller @baseArgs --b-character isaac --b-character azazel 2>&1)
    Record-Check 'live-duplicate-rejected' ($LASTEXITCODE -ne 0 -and ($output -join "`n").Contains('unique --option') -and -not (Test-Path -LiteralPath $capture)) $output
    $stage='entry-syntax'
    foreach($entry in Get-ChildItem -LiteralPath $PSScriptRoot -Filter 'accept-*-e2e.ps1') {
        $parseTokens=$null;$parseErrors=$null
        $syntax=[Management.Automation.Language.Parser]::ParseFile($entry.FullName,[ref]$parseTokens,[ref]$parseErrors)
        $parameters=@($syntax.ParamBlock.Parameters | ForEach-Object {$_.Name.VariablePath.UserPath})
        Record-Check "entry-$($entry.Name)" ($parseErrors.Count -eq 0 -and $parameters -contains 'BCharacter') @($parseErrors | ForEach-Object {$_.Message})
    }
    $stage='source-snapshot'
    New-Item -ItemType Directory -Path (Join-Path $casePath 'sources') | Out-Null
    foreach($source in @('src\coop_active.rs','native\src\coop_active_runtime.cpp','native\include\isaac_cs_t0\coop_active_ipc.hpp','accept-character-selection-build.ps1','accept-virtual-device-e2e.ps1')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $source) -Destination (Join-Path $casePath 'sources')
    }
    Get-ChildItem -LiteralPath $PSScriptRoot -Filter 'accept-*-e2e.ps1' |
        Where-Object Name -ne 'accept-virtual-device-e2e.ps1' |
        Copy-Item -Destination (Join-Path $casePath 'sources')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\docs\refactor\cs-t0\t0-05\07-virtual-device\tx-01-58-character-selection.md') -Destination (Join-Path $casePath 'sources')
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\docs\refactor\cs-t0\t0-05\07-virtual-device\tx-01-59-reporting-repair.md') -Destination (Join-Path $casePath 'sources')
}
catch {$failure=$_.Exception.Message}
finally {
    $env:Path=$originalPath
    @{tx=$Transaction;status=$(if($failure){'FAIL'}elseif([string]::IsNullOrWhiteSpace($BuildArtifactDirectory)){'PASS_BUILD_AND_CLI'}else{'PASS_CLI_AND_REPORTING'});stage=$stage;error=$failure;checks=@($checks.ToArray());realGame='NOT_RUN';automaticRetry=$false;buildPerformed=[string]::IsNullOrWhiteSpace($BuildArtifactDirectory);buildArtifactDirectory=$BuildArtifactDirectory} |
        ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $casePath 'result.json') -Encoding utf8
}
if($failure) {throw $failure}
$saved=Get-Content -LiteralPath (Join-Path $casePath 'result.json') -Raw | ConvertFrom-Json
$savedChecks=@(Get-Content -LiteralPath (Join-Path $casePath 'checks.jsonl') | ForEach-Object {$_ | ConvertFrom-Json})
if($saved.checks.Count -ne $checks.Count -or $savedChecks.Count -ne $checks.Count) {throw 'Persisted report count differs; no retry.'}
Write-Output "$Transaction acceptance complete: $($checks.Count) checks; reports readable; real game NOT_RUN; $casePath"
