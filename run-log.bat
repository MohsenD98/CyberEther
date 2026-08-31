@echo off
@REM Same as run.bat, but also writes ALL logs to cyberether.log next to this script.
@REM The log is written live (flushed per line) and without ANSI color codes.
@REM Useful for debugging issues that build up over time (dropped samples, buffer health, ...).
@REM To watch it live in another window:  powershell Get-Content cyberether.log -Wait -Tail 50
set VULKAN_SDK=C:\VulkanSDK\1.4.350.0
set PATH=%VULKAN_SDK%\Bin;%PATH%
@REM Resolve the AMD Vulkan ICD without hard-coding the DriverStore folder name.
for /f "delims=" %%i in ('dir /b /s "C:\Windows\System32\DriverStore\FileRepository\amd-vulkan64.json" 2^>nul') do set VK_ICD_FILENAMES=%%i
@REM UHD (USRP B200/B210) FPGA images.
set UHD_IMAGES_DIR=%USERPROFILE%\radioconda\Library\share\uhd\images
@REM Send logs to a file (handled inside CyberEther via the logger sink).
set JST_LOG_FILE=%~dp0cyberether.log
@REM Optional: raise verbosity (0=error .. 3=debug .. 4=trace)
@REM set JST_DEBUG=4
cd /d "%~dp0"
echo Logging to "%JST_LOG_FILE%"
.\build\cyberether.exe %*
