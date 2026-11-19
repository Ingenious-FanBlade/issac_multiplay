@echo off
call "D:\VisualStudio2022\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /O2 /EHsc /Fo:"F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\input-check.obj" /Fe:"F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\input-check.exe" "F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\input-check.cpp" dinput8.lib dxguid.lib ole32.lib
