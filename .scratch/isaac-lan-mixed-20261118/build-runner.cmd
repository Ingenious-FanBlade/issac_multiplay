@echo off
call "D:\VisualStudio2022\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /O2 /EHsc /std:c++20 /DWIN32_LEAN_AND_MEAN /DNOMINMAX /Fo:"F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\lab_runner.obj" /Fe:"F:\projects_isaac\issac_multiplay\.scratch\isaac-lan-mixed-20261118\isaac_lan_lab.exe" "F:\projects_isaac\isaac-lan\tools\lab_runner.cpp"
