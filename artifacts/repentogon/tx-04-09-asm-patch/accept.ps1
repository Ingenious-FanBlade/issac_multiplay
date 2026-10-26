param([string]$RepentogonRoot = 'F:/projects_isaac/REPENTOGON-1.1.3')
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$buildRoot = Join-Path $repoRoot 'build/repentogon'
$dllPath = Join-Path $repoRoot 'out/repentogon/zhlIsaacCS.dll'
# 本入口只做一次统一的编译/链接与 PE 静态核对，不部署、不加载 DLL、不启动游戏。
$result = [ordered]@{
    transaction = 'tx-04-09'
    started_utc = [DateTimeOffset]::UtcNow.ToString('o')
    status = 'RUNNING'
    scope = 'Win32 Release build and PE imports only'
    runtime_e2e = 'NOT_RUN'
    dll = $dllPath
    required_imports = @('instance@ASMPatcher', 'FlatPatch@ASMPatcher',
        'AddRelativeJump@ASMPatch', 'AddInternalCall@ASMPatch', 'AddBytes@ASMPatch')
}
try {
    & cmake -S (Join-Path $repoRoot 'mod') -B $buildRoot `
        -G 'Visual Studio 17 2022' -A Win32 -T v143 "-DREPENTOGON_ROOT=$RepentogonRoot" `
        2>&1 | Tee-Object -FilePath (Join-Path $PSScriptRoot 'configure.log')
    if ($LASTEXITCODE) { throw "CMake configure failed: $LASTEXITCODE" }
    & cmake --build $buildRoot --config Release --target zhlIsaacCS -- /m:4 /v:minimal `
        2>&1 | Tee-Object -FilePath (Join-Path $PSScriptRoot 'build.log')
    if ($LASTEXITCODE) { throw "DLL build failed: $LASTEXITCODE" }

    # 从本次配置的 MSVC linker 路径定位 dumpbin；只读取 PE，不运行目标代码。
    $linkerLine = (Select-String -LiteralPath (Join-Path $buildRoot 'CMakeCache.txt') `
        -Pattern '^CMAKE_LINKER:FILEPATH=').Line
    $linkerPath = $linkerLine -replace '^[^=]*=', ''
    $dumpbinPath = Join-Path (Split-Path -Parent $linkerPath) 'dumpbin.exe'
    $importsPath = Join-Path $PSScriptRoot 'imports.txt'
    & $dumpbinPath /imports $dllPath 2>&1 | Tee-Object -FilePath $importsPath
    if ($LASTEXITCODE) { throw "PE imports read failed: $LASTEXITCODE" }
    $imports = Get-Content -LiteralPath $importsPath -Raw
    if ($imports -notmatch '(?im)^\s*libzhl\.dll\s*$') {
        throw 'Official libzhl.dll import is missing'
    }
    foreach ($symbol in $result.required_imports) {
        if (-not $imports.Contains($symbol)) { throw "Missing import: $symbol" }
    }
    $result.dll_bytes = (Get-Item -LiteralPath $dllPath).Length
    $result.status = 'PASS'
} catch {
    $result.status = 'FAIL'
    $result.error = $_.Exception.Message
} finally {
    $result.finished_utc = [DateTimeOffset]::UtcNow.ToString('o')
    $result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'result.json') -Encoding utf8
}
if ($result.status -ne 'PASS') { throw $result.error }
Write-Output 'tx-04-09: build and PE imports PASS; runtime E2E NOT_RUN.'
