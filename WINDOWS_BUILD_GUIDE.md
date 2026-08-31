# راهنمای Build کردن CyberEther روی ویندوز

این راهنما تمام مراحل و مشکلاتی که هنگام build روی Windows 10 با کامپایلر MSVC پیش می‌آید را پوشش می‌دهد.

---

## پیش‌نیازهای سیستم

- **ویندوز 10/11** (64-bit)
- **GPU** با پشتیبانی از Vulkan 1.1+ (اکثر GPU‌های مدرن)
- **اینترنت** برای دانلود dependencies

---

## مرحله ۱ — Git

اگر نصب نیست:
```powershell
winget install Git.Git
```

---

## مرحله ۲ — Python

اگر نصب نیست:
```powershell
winget install Python.Python.3.12
```

---

## مرحله ۳ — Visual Studio Build Tools 2022

این بزرگ‌ترین دانلود است (~4-5 گیگ). در PowerShell **ادمین** اجرا کنید:

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --override "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive"
```

بعد از نصب سیستم را **restart** کنید.

---

## مرحله ۴ — Meson و Ninja

```powershell
pip install meson ninja
```

---

## مرحله ۵ — Vulkan SDK

```powershell
winget install KhronosGroup.VulkanSDK --accept-package-agreements --accept-source-agreements
```

بعد از نصب، **VULKAN_SDK** را در محیط ست کنید (یا در ادامه در دستور build می‌آید):

```
C:\VulkanSDK\<version>\   <- مسیر نصب معمول
```

---

## مرحله ۶ — Python packages

```powershell
pip install pyyaml numpy mapbox_earcut
```

---

## مرحله ۷ — تنظیم Git (مهم!)

برای جلوگیری از خراب شدن patch files روی ویندوز:
```powershell
git config --global core.autocrlf false
```

---

## مرحله ۸ — رفع باگ diff file (باگ پروژه)

فایل `subprojects/packagefiles/libmodes/mode-s-timeh.diff` دو مشکل دارد که باید قبل از build رفع شوند:

**مشکل اول: CRLF line endings**

در PowerShell:
```powershell
$file = "subprojects\packagefiles\libmodes\mode-s-timeh.diff"
$content = [System.IO.File]::ReadAllBytes($file)
$fixed = [System.Collections.Generic.List[byte]]::new()
for ($i = 0; $i -lt $content.Length; $i++) {
    if ($content[$i] -eq 13 -and $i+1 -lt $content.Length -and $content[$i+1] -eq 10) {
        # skip CR
    } else {
        $fixed.Add($content[$i])
    }
}
[System.IO.File]::WriteAllBytes($file, $fixed.ToArray())
```

**مشکل دوم: یک context line گمشده در انتهای diff**

فایل را باز کنید و بعد از آخرین خط (` #define MODE_S_UNIT_METERS 1`) یک خط خالی با یک space اضافه کنید:
```
 #define MODE_S_UNIT_METERS 1
 
```
(خط آخر باید یک space داشته باشد، نه کاملاً خالی)

---

## مرحله ۹ — Clone پروژه

```powershell
git clone --recursive https://github.com/luigifcruz/CyberEther.git
cd CyberEther
```

---

## مرحله ۱۰ — Build

```powershell
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.350.0"   # نسخه نصب شده را جایگزین کنید
$env:PATH = "$env:VULKAN_SDK\Bin;$env:PATH"

meson setup build --vsenv -Dbuildtype=debugoptimized -Dtests=false -Dexamples=false
meson compile -C build cyberether
```

در صورت موفقیت، فایل `build\cyberether.exe` ساخته می‌شود.

---

## مرحله ۱۱ — رفع مشکل Vulkan (اگر app اجرا نشد)

### مشکل ۱: VULKAN_SDK محیطی ست نیست

بعد از نصب Vulkan SDK باید محیط را ست کنید (ترمینال قبلاً باز بوده). راه‌حل: مسیر را دستی ست کنید:
```
C:\VulkanSDK\1.4.350.0\Bin  را به PATH اضافه کنید
```

### مشکل ۲: درایور قدیمی Vulkan در Registry

اگر Intel GPU دارید و درایور جدید نصب کردید، Registry هنوز ممکن است به درایور قدیمی اشاره کند.

در PowerShell **ادمین**:
```powershell
# مسیر JSON درایور جدید را پیدا کنید
Get-ChildItem "C:\Windows\System32\DriverStore\FileRepository" -Recurse -Filter "igvk64.json" |
    Select-Object FullName, LastWriteTime

# Registry را آپدیت کنید (مسیرهای زیر را با مسیر واقعی جایگزین کنید)
$regKey = "HKLM:\SOFTWARE\Khronos\Vulkan\Drivers"
$newJson = "C:\Windows\System32\DriverStore\FileRepository\<NEW_DRIVER_FOLDER>\igvk64.json"
$oldJson = "C:\Windows\System32\DriverStore\FileRepository\<OLD_DRIVER_FOLDER>\igvk64.json"

New-ItemProperty -Path $regKey -Name $newJson -Value 0 -PropertyType DWORD -Force
Remove-ItemProperty -Path $regKey -Name $oldJson -Force
```

### مشکل ۳: تداخل چند Vulkan driver (AMD + Intel)

اگر سیستم هم AMD و هم Intel GPU دارد و AMD driver کرش می‌کند، فقط از Intel استفاده کنید:

```powershell
$env:VK_ICD_FILENAMES = "C:\Windows\System32\DriverStore\FileRepository\<INTEL_FOLDER>\igvk64.json"
```

---

## اجرای برنامه

از فایل `run.bat` در ریشه پروژه استفاده کنید (دوبار کلیک یا از ترمینال):

```
.\run.bat
```

یا دستی:

```powershell
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.350.0"
$env:PATH = "$env:VULKAN_SDK\Bin;$env:PATH"
$env:VK_ICD_FILENAMES = "C:\Windows\System32\DriverStore\FileRepository\iigd_dch.inf_amd64_b53c057d22ce6f37\igvk64.json"
.\build\cyberether.exe
```

---

## Build مجدد (بار دوم به بعد)

فقط این دستور کافیه (بدون نیاز به setup دوباره):

```powershell
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.350.0"
$env:PATH = "$env:VULKAN_SDK\Bin;$env:PATH"
meson compile -C build cyberether
```

---

## خلاصه ابزارهای نصب شده

| ابزار | نسخه تست شده | نحوه نصب |
|---|---|---|
| Visual Studio Build Tools 2022 | 17.14 | winget |
| MSVC Compiler | 14.44 | با VS Build Tools |
| Meson | 1.11.1 | pip |
| Ninja | 1.13.0 | pip |
| Vulkan SDK | 1.4.350.0 | winget |
| Python | 3.12 | winget |
| PyYAML | 6.0.3 | pip |
| numpy | 2.4.6 | pip |
| mapbox_earcut | 2.0.0 | pip |

---

## مشکلات شناخته شده

- **AMD R7 M360**: درایور Vulkan این GPU با AMDVLK کرش می‌کند. باید `VK_ICD_FILENAMES` را روی Intel ICD ست کرد.
- **درایور Intel قدیمی**: نسخه‌های قبل از `31.x` از Vulkan 1.1+ پشتیبانی نمی‌کنند. درایور را آپدیت کنید.
- **`glslangValidator` پیدا نمی‌شود**: `VULKAN_SDK\Bin` باید در `PATH` باشد.
