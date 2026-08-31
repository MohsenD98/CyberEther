@echo off
set VULKAN_SDK=C:\VulkanSDK\1.4.350.0
set PATH=%VULKAN_SDK%\Bin;%PATH%
@REM Run CPU
set PATH=%VULKAN_SDK%\Bin;%PATH%
@REM set VK_ICD_FILENAMES=C:\Windows\System32\DriverStore\FileRepository\iigd_dch.inf_amd64_b53c057d22ce6f37\igvk64.json
@REM Run GPU
set VK_ICD_FILENAMES=C:\Windows\System32\DriverStore\FileRepository\u0360470.inf_amd64_35c64671e7fac064\B360357\amd-vulkan64.json
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
