param([switch]$BuildOnly, [switch]$P2, [switch]$SkipBuild, [switch]$LaunchOnly)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = Join-Path $repo 'build/isaac-dual'
$cargoBuild = Join-Path $repo 'build/harness'
$bootstrap = Join-Path $build 'Release/isaac-dual-bootstrap.dll'
$launcher = Join-Path $cargoBuild 'i686-pc-windows-msvc/release/isaac-dual.exe'

if ($BuildOnly -and $SkipBuild) { throw 'Use either -BuildOnly or -SkipBuild.' }
if ($BuildOnly -and $LaunchOnly) { throw 'Use either -BuildOnly or -LaunchOnly.' }

# 两份副本在同一桌面共享启动器，避免并发调用交错创建进程和写选项。
$launcherMutex = [Threading.Mutex]::new($false, 'Local\IsaacCsDualLauncher')
$ownsMutex = $false
try {
    try { $ownsMutex = $launcherMutex.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $ownsMutex = $true }
    if (-not $ownsMutex) { throw 'Another dual launcher is active' }

    if ($SkipBuild) {
        # 跳过构建时复用现有启动器产物，缺失则在打包和启动前报错。
        foreach ($artifact in @($bootstrap, $launcher)) {
            if (-not (Test-Path -LiteralPath $artifact -PathType Leaf)) {
                throw "Missing existing build output: $artifact. Run launch.ps1 -BuildOnly first."
            }
        }
    } else {
        & cmake -S $PSScriptRoot -B $build -G 'Visual Studio 17 2022' -A Win32 -T v143
        if ($LASTEXITCODE) { throw 'Dual bootstrap configure failed' }
        & cmake --build $build --config Release --target isaac-dual-bootstrap
        if ($LASTEXITCODE) { throw 'Dual bootstrap build failed' }
        & cargo build --manifest-path (Join-Path $repo 'tools/harness/Cargo.toml') --bin isaac-dual --release --locked --target i686-pc-windows-msvc --target-dir $cargoBuild
        if ($LASTEXITCODE) { throw 'Dual launcher build failed' }
    }
    if ($BuildOnly) { return }

    $diagnostic = Join-Path $build ('launch-' + [Guid]::NewGuid().ToString('N'))
    $arguments = @($bootstrap, $diagnostic)
    if ($P2 -or $LaunchOnly) {
        # 无论是否重编译，都将现有 E2E 产物打包并部署到两个隔离副本。
        if (-not $SkipBuild) { & (Join-Path $repo 'scripts/build.ps1') -E2E }
        $package = Join-Path $diagnostic 'package'
        & (Join-Path $repo 'scripts/package.ps1') -E2E -OutputDirectory $package | Out-Null
        $arguments += $package
    }
    # 手动联机模式沿用 P2 部署，只跳过启动后的会话编排。
    if ($LaunchOnly) { $arguments += '--launch-only' }
    Write-Host 'Launching Host, then Client1. Wait for the launcher to finish before using the keyboard.'
    & $launcher @arguments
    if ($LASTEXITCODE) { throw "Dual launch failed. Startup diagnostics, if written: $diagnostic" }

    # 成功交接后只删除本轮生成的启动诊断；游戏配置和存档留在测试副本。
    $resolved = (Resolve-Path -LiteralPath $diagnostic).ProviderPath
    $buildRoot = (Resolve-Path -LiteralPath $build).ProviderPath + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Startup diagnostics escaped the build directory: $resolved"
    }
    Remove-Item -LiteralPath $resolved -Recurse -Force
} finally {
    if ($ownsMutex) { $launcherMutex.ReleaseMutex() }
    $launcherMutex.Dispose()
}
