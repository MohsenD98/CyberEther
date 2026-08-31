@echo off
set VULKAN_SDK=C:\VulkanSDK\1.4.350.0
@REM UHD (USRP B200/B210) FPGA images — required so uhd.dll can load the device firmware
set UHD_IMAGES_DIR=%USERPROFILE%\radioconda\Library\share\uhd\images
@REM Uncomment to disable colored log output (plain text, no ANSI codes):
@REM set NO_COLOR=1
@REM Run CPU
set PATH=%VULKAN_SDK%\Bin;%PATH%
@REM set VK_ICD_FILENAMES=C:\Windows\System32\DriverStore\FileRepository\iigd_dch.inf_amd64_b53c057d22ce6f37\igvk64.json
@REM Run GPU
set VK_ICD_FILENAMES=C:\Windows\System32\DriverStore\FileRepository\u0360470.inf_amd64_35c64671e7fac064\B360357\amd-vulkan64.json
cd /d "%~dp0"
.\build\cyberether.exe %*
