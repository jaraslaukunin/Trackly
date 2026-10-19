@echo off
REM ═══════════════════════════════════════════════════════════════════════════════
REM Trackly Desktop — сборка дистрибутива для Windows (NSIS-установщик)
REM ═══════════════════════════════════════════════════════════════════════════════
REM
REM Использование:
REM   scripts\package.bat
REM
REM Результат: dist\TracklyDesktop-0.1.0-win64.exe
REM
REM Требования:
REM   - Qt 6 (с windeployqt в PATH)
REM   - CMake 3.21+
REM   - NSIS (Nullsoft Scriptable Install System)
REM   - Visual Studio 2022 (или другой компилятор C++17)
REM ═══════════════════════════════════════════════════════════════════════════════

setlocal enabledelayedexpansion

set VERSION=0.1.0
set BUILD_DIR=build\release
set STAGE_DIR=dist\stage
set OUTPUT_DIR=dist

echo ════════════════════════════════════════════════════════════════════
echo   Trackly Desktop — Windows package builder
echo   Version:  %VERSION%
echo   Build:    %BUILD_DIR%
echo ════════════════════════════════════════════════════════════════════

REM ─── Шаг 1: Конфигурация ───
echo.
echo [1/4] Configuring CMake (Release, TRACKLY_DEPLOY=ON)...
cmake -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DTRACKLY_DEPLOY=ON

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configure failed
    exit /b 1
)

REM ─── Шаг 2: Сборка ───
echo.
echo [2/4] Building...
cmake --build "%BUILD_DIR%" --config Release

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    exit /b 1
)

REM ─── Шаг 3: Установка с windeployqt ───
echo.
echo [3/4] Installing + running windeployqt...
if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
cmake --install "%BUILD_DIR%" --prefix "%STAGE_DIR%" --config Release

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Install failed
    exit /b 1
)

REM Проверяем наличие DLL
if exist "%STAGE_DIR%\Qt6Core.dll" (
    echo        windeployqt: OK (Qt DLLs found)
) else (
    echo        WARNING: Qt DLLs not found in stage dir. windeployqt may have failed.
)

REM ─── Шаг 4: NSIS-установщик через CPack ───
echo.
echo [4/4] Creating NSIS installer...
cd "%STAGE_DIR%"
cpack -G NSIS --config "%CD%\..\..\%BUILD_DIR%\CPackConfig.cmake"

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CPack failed
    cd "%CD%\..\.."
    exit /b 1
)

REM Переносим установщик в dist/
cd "%CD%\..\.."
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
move /Y "%STAGE_DIR%\TracklyDesktop-*.exe" "%OUTPUT_DIR%\" 2>nul

echo.
echo ════════════════════════════════════════════════════════════════════
echo   Done: %OUTPUT_DIR%\TracklyDesktop-%VERSION%-win64.exe
echo ════════════════════════════════════════════════════════════════════

endlocal
