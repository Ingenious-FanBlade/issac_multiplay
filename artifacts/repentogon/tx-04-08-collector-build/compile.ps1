param([string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path)
$ErrorActionPreference = 'Stop'
# 只做代码生成与纯编译；不链接 DLL、不部署、不执行任何测试或游戏。
$buildDir = Join-Path $RepoRoot 'build/repentogon'
& cmake -S (Join-Path $RepoRoot 'mod') -B $buildDir -G 'Visual Studio 17 2022' -A Win32 -T v143
if ($LASTEXITCODE) { throw 'configure failed' }
& cmake --build $buildDir --config Release --target isaac_cs_world -- /m:4 /v:minimal
if ($LASTEXITCODE) { throw 'world library compile failed' }
$cache = Get-Content -LiteralPath (Join-Path $buildDir 'CMakeCache.txt')
$pythonLine = $cache | Where-Object { $_ -match '^_Python3_EXECUTABLE:INTERNAL=' } | Select-Object -First 1
if (-not $pythonLine) { throw 'Python runtime absent from CMake cache' }
$pythonPath = ($pythonLine -split '=', 2)[1]
& $pythonPath (Join-Path $RepoRoot 'mod/tools/gen_binding.py') --out (Join-Path $buildDir 'generated')
if ($LASTEXITCODE) { throw 'binding generation failed' }

# 复制生成工程，只保留本轮 DLL 编译单元；保留原 CMake 的 include/宏/逐文件标准。
# 避开既有 odd/host 的 SDK friend-static 错误，仅编译，不消费其旧 obj 进行链接。
$projectPath = Join-Path $buildDir 'zhlIsaacCS.vcxproj'
$project = [xml](Get-Content -LiteralPath $projectPath -Raw)
$ns = New-Object System.Xml.XmlNamespaceManager($project.NameTable)
$ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
$selected = @('mod_init.cpp', 'definitions.cpp', 'world_layout.cpp', 'registry.cpp',
  'tick.cpp', 'consumers.cpp', 'lifecycle.cpp', 'remote_life.cpp', 'world_bridge.cpp',
  'config.cpp', 'command.cpp', 'hooks_generated.cpp')
foreach ($node in @($project.SelectNodes('//m:ClCompile[@Include]', $ns))) {
  if ([IO.Path]::GetFileName($node.Include) -notin $selected) { [void]$node.ParentNode.RemoveChild($node) }
}
foreach ($node in @($project.SelectNodes('//m:CustomBuild[@Include]', $ns))) {
  [void]$node.ParentNode.RemoveChild($node)
}
$filteredProject = Join-Path $buildDir 'world-bridge-compile.vcxproj'
$project.Save($filteredProject)
$instanceLine = $cache | Where-Object { $_ -match '^CMAKE_GENERATOR_INSTANCE:INTERNAL=' } | Select-Object -First 1
$instancePath = ($instanceLine -split '=', 2)[1]
$msbuildPath = Join-Path $instancePath 'MSBuild/Current/Bin/MSBuild.exe'
if (-not $msbuildPath) { throw 'MSBuild not found' }
& $msbuildPath $filteredProject /t:ClCompile /p:Configuration=Release /p:Platform=Win32 /p:BuildProjectReferences=false /v:minimal
if ($LASTEXITCODE) { throw 'bridge/wrapper compile failed' }
