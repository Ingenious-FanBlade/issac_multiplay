param([string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path)
$ErrorActionPreference = 'Stop'
# 只构建完整 DLL 及其依赖，不执行测试、启动游戏或部署到游戏目录。
# 使用独立构建树，不读取此前筛选工程生成的 obj/lib。
$buildDir = Join-Path $RepoRoot 'build/repentogon-tx-04-08-sdk-fix'
& cmake -S (Join-Path $RepoRoot 'mod') -B $buildDir -G 'Visual Studio 17 2022' -A Win32 -T v143
if ($LASTEXITCODE) { throw 'DLL configure failed' }
& cmake --build $buildDir --config Release --target zhlIsaacCS -- /m:4 /v:minimal
if ($LASTEXITCODE) { throw 'complete DLL build failed' }
