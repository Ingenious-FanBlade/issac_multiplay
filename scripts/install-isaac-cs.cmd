@echo off
setlocal EnableExtensions DisableDelayedExpansion
rem ============================================================================
rem  Isaac CS 安装脚本
rem  注意：本文件必须保存为 GBK（ANSI 936）编码 + CRLF 换行。
rem        cmd 在 65001 代码页下读取 UTF-8 中文行会错位切行，故不用 chcp 65001。
rem
rem  功能：
rem    1. 在本机所有盘符中搜索 steamapps\common\The Binding of Isaac Rebirth；
rem    2. 把与本脚本同目录的 zhlIsaacCS.dll 复制到每个找到的 ...\Repentogon 目录；
rem    3. 在每个 ...\Repentogon\isaac-cs\config.json 启用联机：
rem       不存在时新建（enabled=true，监听 0.0.0.0，stream 使用代码默认值）；
rem       已存在且含旧版脚本写入的错误 stream 带宽时删除该段；
rem       已存在时只把 enabled 改为 true，其余设置不动；监听 127.0.0.1 时给出提醒。
rem       DLL 读不到配置或 enabled=false 时不注册任何 Hook，界面会提示“联机 DLL 不可用”。
rem    4. 把与本脚本同目录的 isaac-cs-ui 文件夹镜像到每个找到的 ...\mods 目录。
rem    找到多个游戏目录时全部安装；目标子目录（Repentogon / mods）不存在的游戏目录跳过。
rem    修改已有配置需要 Windows PowerShell（系统自带）。
rem
rem  用法：
rem    install-isaac-cs.cmd                 搜索全部盘符并安装，结束后暂停
rem    install-isaac-cs.cmd "D:\Games"      只在指定目录下搜索
rem    install-isaac-cs.cmd ... /nopause    结束后不暂停（供其他脚本调用）
rem
rem  返回码：0 = 全部成功；1 = 源文件缺失或未找到目标；2 = 有目标安装失败。
rem ============================================================================

rem ---- 源文件：与本脚本同目录 ----
set "SRC_DIR=%~dp0"
set "SRC_DLL=%SRC_DIR%zhlIsaacCS.dll"
set "SRC_UI=%SRC_DIR%isaac-cs-ui"

rem ---- 参数解析：可选搜索根目录与 /nopause ----
set "SEARCH_ROOT="
set "NO_PAUSE="
:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="/nopause" (
    set "NO_PAUSE=1"
) else (
    set "SEARCH_ROOT=%~f1"
)
shift
goto parse_args
:args_done

rem ---- 计数器：成功数与失败数 ----
set /a DLL_OK=0, DLL_FAIL=0, CFG_OK=0, CFG_FAIL=0, UI_OK=0, UI_FAIL=0, GAME_COUNT=0
set "EXIT_CODE=0"

rem ---- 检查源文件 ----
if not exist "%SRC_DLL%" (
    echo [错误] 未找到 "%SRC_DLL%"
    set "EXIT_CODE=1"
    goto finish
)
if not exist "%SRC_UI%\" (
    echo [错误] 未找到文件夹 "%SRC_UI%"
    set "EXIT_CODE=1"
    goto finish
)

rem ---- 游戏运行时 DLL 被占用会导致复制失败，先提醒 ----
tasklist /fi "imagename eq isaac-ng.exe" 2>nul | findstr /i /c:"isaac-ng.exe" >nul
if not errorlevel 1 (
    echo [警告] 检测到 isaac-ng.exe 正在运行，DLL 可能因被占用而安装失败，建议先关闭游戏。
    echo.
)

rem ---- 临时清单：两类目标目录各一份，每行一个路径 ----
set "LIST_REPO=%TEMP%\isaac-cs-install-repo-%RANDOM%%RANDOM%.txt"
set "LIST_MODS=%TEMP%\isaac-cs-install-mods-%RANDOM%%RANDOM%.txt"
type nul > "%LIST_REPO%"
type nul > "%LIST_MODS%"

rem ---- 搜索游戏目录 ----
if defined SEARCH_ROOT (
    echo 正在搜索 "%SEARCH_ROOT%" ……
    call :scan_root "%SEARCH_ROOT%"
) else (
    echo 正在搜索全部盘符，可能需要几分钟……
    for %%D in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
        if exist "%%D:\" call :scan_root "%%D:\"
    )
)
echo.

if %GAME_COUNT%==0 (
    echo [错误] 未找到任何 steamapps\common\The Binding of Isaac Rebirth 目录。
    set "EXIT_CODE=1"
    goto cleanup
)

rem ---- 安装 DLL ----
echo ==== 安装 zhlIsaacCS.dll ====
for /f "usebackq delims=" %%T in ("%LIST_REPO%") do call :install_dll "%%T"
if %DLL_OK%==0 if %DLL_FAIL%==0 echo [跳过] 找到的游戏目录中都没有 Repentogon 文件夹。
echo.

rem ---- 启用联机配置 ----
echo ==== 启用联机配置 isaac-cs\config.json ====
for /f "usebackq delims=" %%T in ("%LIST_REPO%") do call :install_config "%%T"
if %CFG_OK%==0 if %CFG_FAIL%==0 echo [跳过] 找到的游戏目录中都没有 Repentogon 文件夹。
echo.

rem ---- 安装 UI 模组 ----
echo ==== 安装 isaac-cs-ui ====
for /f "usebackq delims=" %%T in ("%LIST_MODS%") do call :install_ui "%%T"
if %UI_OK%==0 if %UI_FAIL%==0 echo [跳过] 找到的游戏目录中都没有 mods 文件夹。
echo.

echo 汇总：游戏目录 %GAME_COUNT% 个；DLL 成功 %DLL_OK% / 失败 %DLL_FAIL%；配置 成功 %CFG_OK% / 失败 %CFG_FAIL%；UI 成功 %UI_OK% / 失败 %UI_FAIL%
set /a TOTAL_FAIL=DLL_FAIL+CFG_FAIL+UI_FAIL
if not %TOTAL_FAIL%==0 set "EXIT_CODE=2"
set /a TOTAL_OK=DLL_OK+CFG_OK+UI_OK
if %TOTAL_OK%==0 if %TOTAL_FAIL%==0 set "EXIT_CODE=1"

:cleanup
del /q "%LIST_REPO%" "%LIST_MODS%" 2>nul

:finish
if not defined NO_PAUSE pause
endlocal & exit /b %EXIT_CODE%


rem ============================================================================
rem  :scan_root <根目录>
rem  在根目录下递归查找名为 "The Binding of Isaac Rebirth" 的文件夹，交给 :check_game 判定。
rem  访问被拒绝等错误直接忽略。
rem ============================================================================
:scan_root
set "SCAN_PATH=%~1"
if not "%SCAN_PATH:~-1%"=="\" set "SCAN_PATH=%SCAN_PATH%\"
for /f "delims=" %%P in ('dir /s /b /a:d "%SCAN_PATH%The Binding of Isaac Rebirth" 2^>nul') do call :check_game "%%P"
exit /b 0


rem ============================================================================
rem  :check_game <游戏目录>
rem  仅接受路径形如 ...\steamapps\common\The Binding of Isaac Rebirth 的目录；
rem  其下存在 Repentogon / mods 时分别记入对应清单。
rem ============================================================================
:check_game
for %%A in ("%~1\..") do if /i not "%%~nxA"=="common" exit /b 0
for %%A in ("%~1\..\..") do if /i not "%%~nxA"=="steamapps" exit /b 0
set /a GAME_COUNT+=1
echo [找到] "%~1"
rem 路径可能含 "(x86)"，写清单的语句不能放进括号块，否则 ")" 会提前结束块
if exist "%~1\Repentogon\" >>"%LIST_REPO%" echo %~1\Repentogon
if not exist "%~1\Repentogon\" echo        （无 Repentogon 文件夹，跳过 DLL）
if exist "%~1\mods\" >>"%LIST_MODS%" echo %~1\mods
if not exist "%~1\mods\" echo        （无 mods 文件夹，跳过 UI）
exit /b 0


rem ============================================================================
rem  :install_dll <Repentogon 目录>
rem  覆盖复制 zhlIsaacCS.dll；失败多半是游戏正在运行导致文件被占用。
rem ============================================================================
:install_dll
copy /y "%SRC_DLL%" "%~1\zhlIsaacCS.dll" >nul 2>&1
if errorlevel 1 (
    set /a DLL_FAIL+=1
    echo [失败] "%~1"（文件可能被占用，请关闭游戏后重试）
) else (
    set /a DLL_OK+=1
    echo [完成] "%~1"
)
exit /b 0


rem ============================================================================
rem  :install_config <Repentogon 目录>
rem  DLL 从 Repentogon\isaac-cs\config.json 读取配置（schema=1）。
rem  不存在时新建默认配置；已存在时用 PowerShell 只把 enabled 置为 true。
rem  已存在但无法解析的配置不覆盖，记为失败，交给用户检查。
rem ============================================================================
:install_config
set "CFG_FILE=%~1\isaac-cs\config.json"
if exist "%CFG_FILE%" goto install_config_update
if not exist "%~1\isaac-cs\" mkdir "%~1\isaac-cs" 2>nul
call :write_default_config "%CFG_FILE%"
if errorlevel 1 (
    set /a CFG_FAIL+=1
    echo [失败] 无法创建 "%CFG_FILE%"
) else (
    set /a CFG_OK+=1
    echo [新建] "%CFG_FILE%"
)
exit /b 0

:install_config_update
rem 路径经环境变量传给 PowerShell，避免引号与括号转义问题。
rem 输出一行，以竖线分隔：状态 ok/fail、处理结果（失败时为原因）、bind_address。
rem 处理结果：enabled 被改为 true 记 enabled；旧版脚本写入的错误 stream 段
rem （total_bytes_per_second=937500，会被 DLL 判为 config_invalid）被删除记 repaired；
rem 两者用 + 连接，都没有则为 unchanged，此时不写文件。
rem 写回时使用无 BOM 的 UTF-8：DLL 的 JSON 解析器不接受 BOM。
set "CS_CONFIG_PATH=%CFG_FILE%"
set "CFG_STATUS="
set "CFG_STATE="
set "CFG_BIND="
for /f "usebackq tokens=1-3 delims=|" %%A in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "try { $p=$env:CS_CONFIG_PATH; $j=[IO.File]::ReadAllText($p) | ConvertFrom-Json; if($j.schema -ne 1){ throw 'schema' }; $s=@(); if($j.enabled -ne $true){ $j | Add-Member -NotePropertyName enabled -NotePropertyValue $true -Force; $s+='enabled' }; if($j.stream -and $j.stream.total_bytes_per_second -eq 937500){ $j.PSObject.Properties.Remove('stream'); $s+='repaired' }; if($s.Count){ [IO.File]::WriteAllText($p, ($j | ConvertTo-Json -Depth 16), (New-Object Text.UTF8Encoding $false)); $s=$s -join '+' } else { $s='unchanged' }; $b=''; if($j.host -and $j.host.bind_address){ $b=$j.host.bind_address }; 'ok|' + $s + '|' + $b } catch { 'fail|' + $_.Exception.Message }" 2^>nul`) do (
    set "CFG_STATUS=%%A"
    set "CFG_STATE=%%B"
    set "CFG_BIND=%%C"
)
if not "%CFG_STATUS%"=="ok" goto install_config_failed
set /a CFG_OK+=1
if "%CFG_STATE:enabled=%"=="%CFG_STATE%" echo [已启用] "%CFG_FILE%"
if not "%CFG_STATE:enabled=%"=="%CFG_STATE%" echo [启用] "%CFG_FILE%"
if not "%CFG_STATE:repaired=%"=="%CFG_STATE%" echo        [修复] 删除了旧版安装脚本写入的错误 stream 带宽设置（会导致 DLL 拒绝整份配置）。
if "%CFG_BIND%"=="127.0.0.1" echo        [提醒] host.bind_address 为 127.0.0.1，只能本机连接；局域网联机请改为 0.0.0.0。
exit /b 0

:install_config_failed
set /a CFG_FAIL+=1
echo [失败] "%CFG_FILE%" 无法解析或写入，未改动。原因："%CFG_STATE%"
exit /b 0


rem ============================================================================
rem  :write_default_config <文件路径>
rem  写入默认配置：取 mod\config.example.json 的内容，enabled 改为 true，并且不写 stream 段。
rem  不写 stream 的原因：acceptance_profile=true 时 DLL 要求带宽与编译进去的预算完全相等
rem  （mod\src\replica\stream_config.cpp），模板中的 937500/312500/120000 等与预算
rem  1125000/375000/150000 不符，会被判为 config_invalid 而整体关闭；省略后使用代码默认值，
rem  始终与预算一致。返回码非 0 表示写入失败。
rem ============================================================================
:write_default_config
> "%~1" (
    echo {
    echo   "schema": 1,
    echo   "enabled": true,
    echo   "diagnostics": { "enabled": false, "repeat_topology_reads": false },
    echo   "world_record": { "enabled": false },
    echo   "motion": { "interpolation": true, "extrapolation": true, "prediction": true, "projectile_lead": false, "tear_birth": true },
    echo   "log_level": "info",
    echo   "trace_dir": "isaac-cs/traces",
    echo   "control_port": { "enabled": false, "port": 27091 },
    echo   "host": { "default_port": 27090, "bind_address": "0.0.0.0", "peer_timeout_ms": 3000 }
    echo }
) || exit /b 1
exit /b 0


rem ============================================================================
rem  :install_ui <mods 目录>
rem  用 robocopy /MIR 把 isaac-cs-ui 镜像到 mods\isaac-cs-ui，清除旧版本残留文件；
rem  /XF disable.it 保留用户在游戏内禁用模组时生成的标记文件。
rem  robocopy 返回码 >= 8 表示失败。
rem ============================================================================
:install_ui
robocopy "%SRC_UI%" "%~1\isaac-cs-ui" /MIR /XF disable.it /R:1 /W:1 /NFL /NDL /NJH /NJS /NP >nul
if errorlevel 8 (
    set /a UI_FAIL+=1
    echo [失败] "%~1\isaac-cs-ui"
) else (
    set /a UI_OK+=1
    echo [完成] "%~1\isaac-cs-ui"
)
exit /b 0
