# PlutoSDR (ADALM-PLUTO) on Windows — CyberEther Setup Guide

This guide documents exactly what was done to get PlutoSDR working inside CyberEther on Windows from a fresh state.

---

## Prerequisites

| Tool | Purpose | Download |
|------|---------|----------|
| **RadioConda** | Provides pre-built `PlutoSDRSupport.dll`, `libiio.dll`, and all dependencies | https://github.com/ryanvolz/radioconda/releases |
| **Zadig** | Installs WinUSB driver on PlutoSDR's IIO interface | https://zadig.akeo.ie |
| Visual Studio Build Tools 2022 | Already needed to build CyberEther | — |

Install RadioConda at the default path (`C:\Users\<user>\radioconda`). No conda environment activation needed — we only copy DLL files from it.

---

## Step 1 — Code changes (already committed)

Three files in the repo were modified to enable SoapySDR external module loading on Windows.

### `subprojects/packagefiles/soapysdr/meson.build`

SoapySDR must be a **shared library** (`.dll`) so that external modules (`PlutoSDRSupport.dll`) register in the same in-process factory registry as CyberEther.

Key changes:
- `shared_library()` instead of `library()` — forces shared build
- `configure_file()` for `Modules.in.cpp` and `Version.in.cpp` — substitutes CMake template variables (`@CMAKE_INSTALL_LIBDIR@` → `lib`, `@MODULE_EXT@` → `.dll`) that meson would otherwise leave as literal strings
- `interface_cpp_args = []` — removes `-DSOAPY_SDR_DLL_EXPORTS` from consumers (it belongs only in the DLL build)

### `meson/loaders/soapy/meson.build`

```meson
default_options: ['default_library=shared']
```

Forces the subproject to build as a shared library regardless of the top-level default.

### `src/domains/io/soapy/block_impl.cc`

CyberEther never called `SoapySDR::loadModules()`, so external modules were never discovered. Added:

```cpp
static void LoadExternalModules() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        SoapySDR::loadModules();
    });
}
```

Called at the top of `ListAvailableDevices()` before every enumeration.

---

## Step 2 — Build CyberEther

```bat
build_and_run.bat
```

This produces `build\SoapySDR.dll` (731 KB). Verify with:

```bat
dumpbin /dependents build\jetstream.dll | findstr SoapySDR
```

Expected output: `SoapySDR.dll` — confirming jetstream.dll imports the shared SoapySDR.

---

## Step 3 — Copy DLL dependencies to `build\`

SoapySDR auto-discovers modules from `<root>\lib\SoapySDR\modules0.8\` where root = two directories up from `SoapySDR.dll`. Since `SoapySDR.dll` lives in `build\`, root = project root, and the module path is `lib\SoapySDR\modules0.8\`.

When `PlutoSDRSupport.dll` is loaded, Windows searches for its dependencies starting from the **executable's directory** (`build\`). All of these must be present there:

```bat
set CONDA=C:\Users\%USERNAME%\radioconda\Library\bin

copy %CONDA%\libiio.dll       build\
copy %CONDA%\libad9361.dll    build\
copy %CONDA%\libusb-1.0.dll   build\
copy %CONDA%\libxml2.dll      build\
copy %CONDA%\zstd.dll         build\
copy %CONDA%\iconv.dll        build\
copy %CONDA%\charset.dll      build\    ← critical: iconv.dll depends on this
copy %CONDA%\zlib.dll         build\
copy %CONDA%\zlib1.dll        build\
```

> **`charset.dll` is easy to miss** — it is an indirect dependency (`iconv.dll → charset.dll`) and its absence causes `LoadLibrary` to silently fail with "The specified module could not be found", leaving the device list empty with no visible error.

---

## Step 4 — Install the PlutoSDR SoapySDR module

Create the module directory and copy the pre-built module from RadioConda:

```bat
mkdir lib\SoapySDR\modules0.8
copy C:\Users\%USERNAME%\radioconda\Library\lib\SoapySDR\modules0.8\PlutoSDRSupport.dll ^
     lib\SoapySDR\modules0.8\
```

Do **not** put `libiio.dll` or any other DLL in this directory — only SoapySDR module DLLs belong here. Putting non-module DLLs there causes SoapySDR to attempt loading them as modules, creating duplicate DLL instances that crash during USB enumeration.

---

## Step 5 — Install WinUSB driver via Zadig

PlutoSDR presents multiple USB interfaces. Only **IIO Interface 5** needs WinUSB:

1. Connect PlutoSDR via USB
2. Open Zadig → Options → List All Devices
3. Select **"IIO (Interface 5)"** — `USB\VID_0456&PID_B673&MI_05`
4. Select **WinUSB** as the driver
5. Click **Install Driver**

The other interfaces (DFU, CDC ACM, etc.) do not need WinUSB.

Verify the driver works:

```bat
C:\Users\%USERNAME%\radioconda\Library\bin\iio_info.exe -u usb:
```

Expected: `IIO context with 9 devices` and `usb:1.x.5`.

---

## Step 6 — Run CyberEther

```bat
run.bat
```

Open or create a flowgraph, add a **Soapy SDR** block. The Device dropdown should show:

```
PlutoSDR #0 usb:1.1.5
```

---

## Why SoapySDR must be a shared library

CyberEther's original build compiled SoapySDR as a static library linked into `jetstream.dll`. External modules like `PlutoSDRSupport.dll` (compiled against RadioConda's `SoapySDR.dll`) register their factories by calling `SoapySDR::Registry::Registry()`. With two separate SoapySDR instances (one static inside `jetstream.dll`, one dynamic in `PlutoSDRSupport.dll`), they write to different factory tables — enumerate sees nothing.

Making SoapySDR a shared `SoapySDR.dll` means there is exactly one factory table in the process, shared by `jetstream.dll` and all loaded modules.

---

## File layout after setup

```
CyberEther\
├── build\
│   ├── cyberether.exe
│   ├── jetstream.dll
│   ├── SoapySDR.dll          ← built by meson (shared)
│   ├── libiio.dll            ← from RadioConda
│   ├── libad9361.dll         ← from RadioConda
│   ├── libusb-1.0.dll        ← from RadioConda
│   ├── libxml2.dll           ← from RadioConda
│   ├── zstd.dll              ← from RadioConda
│   ├── iconv.dll             ← from RadioConda
│   ├── charset.dll           ← from RadioConda (iconv dependency)
│   ├── zlib.dll              ← from RadioConda
│   └── zlib1.dll             ← from RadioConda
└── lib\
    └── SoapySDR\
        └── modules0.8\
            └── PlutoSDRSupport.dll   ← from RadioConda
```
