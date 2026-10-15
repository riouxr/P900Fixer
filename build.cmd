@echo off
rem Builds P900Fixer (Release) without a Developer Prompt:
rem   build.cmd            configure if needed, build, test, copy to dist\
rem Overridable: P900_BUILD_DIR (C:\b\p900: short, no spaces, vcpkg's paths
rem get long), VCPKG_ROOT (C:\vcpkg), QT_DIR (C:\Qt\6.8.3\msvc2022_64), VCVARS.
setlocal
if not defined P900_BUILD_DIR set "P900_BUILD_DIR=C:\b\p900"
if not defined P900_VCPKG set "P900_VCPKG=C:\vcpkg"
if defined VCPKG_ROOT set "P900_VCPKG=%VCPKG_ROOT%"
if not defined QT_DIR set "QT_DIR=C:\Qt\6.8.3\msvc2022_64"
if not defined VCVARS (
    for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
)
call "%VCVARS%" >nul || exit /b 1
rem vcvars points VCPKG_ROOT at Visual Studio's own vcpkg: use the checkout.
set "VCPKG_ROOT=%P900_VCPKG%"
set "PATH=%QT_DIR%\bin;%PATH%"
cd /d "%~dp0"

if not exist "%P900_BUILD_DIR%\CMakeCache.txt" (
    cmake -S . -B "%P900_BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release ^
        "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
        "-DCMAKE_PREFIX_PATH=%QT_DIR%" || exit /b 1
)
cmake --build "%P900_BUILD_DIR%" || exit /b 1
ctest --test-dir "%P900_BUILD_DIR%" --output-on-failure || exit /b 1

rem dist\: the app and its DLLs, ready to run or zip.
if exist dist rmdir /s /q dist
robocopy "%P900_BUILD_DIR%\bin" dist /e /xf test_destripe.exe *.pdb *.ilk >nul
if %errorlevel% geq 8 exit /b 1
echo Built: %~dp0dist\P900Fixer.exe
exit /b 0
