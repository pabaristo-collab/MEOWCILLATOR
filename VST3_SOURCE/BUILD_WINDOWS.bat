@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title MEOWCILLATOR VST3 - WINDOWS x64 BUILD

echo -----------------------------------------------------
echo   MEOWCILLATOR 0.9.6 - VST3 Windows 64-bit
echo   by pablo aristo
echo -----------------------------------------------------
echo.
where cmake >nul 2>&1
if errorlevel 1 (
 echo [ERROR] Falta CMake: https://cmake.org/download/
 goto failed
)
where git >nul 2>&1
if errorlevel 1 (
 echo [ERROR] Falta Git para Windows: https://git-scm.com/downloads/win
 goto failed
)
if not exist "JUCE\CMakeLists.txt" (
 echo Descargando JUCE 8.0.15 de GitHub...
 git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE.git JUCE
 if errorlevel 1 goto failed
)
echo.
echo Configurando CMake + Visual Studio 2022 x64...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR="%CD%\JUCE"
if errorlevel 1 (
 echo [ERROR] Instala Visual Studio 2022 con Desktop development with C++.
 goto failed
)
echo.
echo Compilando VST3 y Standalone [Release]...
cmake --build build --config Release --parallel 4
if errorlevel 1 goto failed
if not exist "build\Meowcillator_artefacts\Release\VST3\MEOWCILLATOR.vst3" (
 echo [ERROR] No se encontro el VST3 en la ruta esperada.
 goto failed
)
echo.
echo [OK] VST3 compilado en:
echo %CD%\build\Meowcillator_artefacts\Release\VST3\MEOWCILLATOR.vst3
echo.
echo Copia toda la carpeta MEOWCILLATOR.vst3 a:
echo C:\Program Files\Common Files\VST3\
echo Despues: REAPER ^> Options ^> Preferences ^> Plug-ins ^> VST ^> Re-scan
pause
exit /b 0
:failed
echo.
echo [ERROR] No se genero un VST3. Conserva la salida completa para diagnostico.
pause
exit /b 1
