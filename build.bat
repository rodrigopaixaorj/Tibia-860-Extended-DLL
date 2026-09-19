@echo off
setlocal enabledelayedexpansion

:: ===================================================================
:: Tibia 860 - Extended Client DLL Build Script
:: Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)
:: ===================================================================

echo ===================================================================
echo Building ExtendedDLL (ddraw.dll) for Tibia 8.60 (32-bit x86)
echo ===================================================================

:: 1. Check if cmake is in PATH
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] CMake not found in PATH. Searching for Visual Studio bundled CMake...
    
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;!PATH!"
        echo [INFO] Found CMake in Visual Studio 2026 Community.
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;!PATH!"
        echo [INFO] Found CMake in Visual Studio 2022 Community.
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;!PATH!"
        echo [INFO] Found CMake in Visual Studio 2022 Professional.
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;!PATH!"
        echo [INFO] Found CMake in Visual Studio 2022 Enterprise.
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;!PATH!"
        echo [INFO] Found CMake in Visual Studio 2019 Community.
    ) else (
        echo [ERROR] CMake could not be located. Please install CMake or Visual Studio C++ CMake tools.
        pause
        exit /b 1
    )
)

:: 2. Create build directory
if not exist build (
    mkdir build
)

cd build

:: 3. Configure and build
cmake .. -A Win32 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to configure project with CMake.
    cd ..
    pause
    exit /b %ERRORLEVEL%
)

cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed.
    cd ..
    pause
    exit /b %ERRORLEVEL%
)

:: 4. Copy output dll to root
if exist "Release\ddraw.dll" (
    copy /y "Release\ddraw.dll" "..\ddraw.dll" >nul
) else if exist "ddraw.dll" (
    copy /y "ddraw.dll" "..\ddraw.dll" >nul
)

echo.
echo ===================================================================
echo [SUCCESS] ddraw.dll generated successfully in ExtendedDLL folder!
echo Copy ddraw.dll and config.ini to your Tibia 8.60 folder.
echo ===================================================================
cd ..
pause
