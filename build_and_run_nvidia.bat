@echo off
set VULKAN_SDK=C:\VulkanSDK\1.4.350.0
set PATH=%VULKAN_SDK%\Bin;%PATH%

cd /d "%~dp0"

echo Building CyberEther...
meson compile -C build cyberether
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo Build successful! Starting CyberEther...
.\build\cyberether.exe %*
