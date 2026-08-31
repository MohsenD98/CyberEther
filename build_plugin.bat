@echo off
setlocal
set VULKAN_SDK=C:\VulkanSDK\1.4.350.0
set PATH=%VULKAN_SDK%\Bin;%PATH%
cd /d "%~dp0"

set TARGETS=cyberether_hadi_plugin_cep cyberether_mohsen_plugin_cep cyberether_blueprint_plugin_cep
if not "%~1"=="" set TARGETS=cyberether_%~1_plugin_cep

findstr /C:"cyberether_hadi_plugin_cep" build\build.ninja >nul 2>&1
if %errorlevel% neq 0 (
    echo [setup] Wiring plugins into the existing build...
    meson setup --reconfigure build -Dexamples=true
    if %errorlevel% neq 0 (
        echo [error] Configuration failed!
        pause
        exit /b %errorlevel%
    )
)

echo Building: %TARGETS%
meson compile -C build %TARGETS%
if %errorlevel% neq 0 (
    echo [error] Plugin build failed!
    pause
    exit /b %errorlevel%
)

echo Done! .cep plugin bundles are in build\examples\plugins\
pause
