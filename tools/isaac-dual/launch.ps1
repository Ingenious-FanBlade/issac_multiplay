param([switch]$BuildOnly, [switch]$P2)

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = Join-Path $repo 'build/isaac-dual'
$cargoBuild = Join-Path $repo 'build/harness'

# 两份副本在同一桌面共享启动器，避免并发调用交错创建进程和写选项。
$launcherMutex = [Threading.Mutex]::new($false, 'Local\IsaacCsDualLauncher')
$ownsMutex = $false
try {
    try { $ownsMutex = $launcherMutex.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $ownsMutex = $true }
    if (-not $ownsMutex) { throw 'Another dual launcher is active' }

    & cmake -S $PSScriptRoot -B $build -G 'Visual Studio 17 2022' -A Win32 -T v143
    if ($LASTEXITCODE) { throw 'Dual bootstrap configure failed' }
    & cmake --build $build --config Release --target isaac-dual-bootstrap
    if ($LASTEXITCODE) { throw 'Dual bootstrap build failed' }
    & cargo build --manifest-path (Join-Path $repo 'tools/harness/Cargo.toml') --bin isaac-dual --release --locked --target i686-pc-windows-msvc --target-dir $cargoBuild
    if ($LASTEXITCODE) { throw 'Dual launcher build failed' }
    if ($BuildOnly) { return }

    $diagnostic = Join-Path $build ('launch-' + [Guid]::NewGuid().ToString('N'))
    $arguments = @((Join-Path $build 'Release/isaac-dual-bootstrap.dll'), $diagnostic)
    if ($P2) {
        # 打包当前二进制 E2E 构建，供启动器分别部署到两个隔离副本。
        & (Join-Path $repo 'scripts/build.ps1') -E2E
        $package = Join-Path $diagnostic 'package'
        & (Join-Path $repo 'scripts/package.ps1') -E2E -OutputDirectory $package | Out-Null
        $arguments += $package
    }
    Write-Host 'Launching Host, then Client1. Wait for the launcher to finish before using the keyboard.'
    & (Join-Path $cargoBuild 'i686-pc-windows-msvc/release/isaac-dual.exe') @arguments
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
