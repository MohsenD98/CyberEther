# USRP (Ettus B200 / B210) on Windows — CyberEther Setup Guide

This guide documents exactly what was done to get an Ettus **USRP B210** working inside
CyberEther on Windows from a fresh state. The same steps apply to the **B200 / B200mini /
B205mini** (they all use the `uhd` SoapySDR driver and the `b2xx` FPGA images).

It builds on the same SoapySDR external-module mechanism described in
[plutosdr-windows-setup.md](plutosdr-windows-setup.md) — read that first if you want the
background on *why* SoapySDR must be a shared library. This guide adds the two things that
are specific to USRP: **the UHD runtime + FPGA images**, and a **build fix** that was
required before CyberEther would compile at all on Windows.

---

## Prerequisites

| Tool | Purpose | Download |
|------|---------|----------|
| **RadioConda** | Provides pre-built `uhdSupport.dll` (SoapyUHD), `uhd.dll`, Boost, and `uhd_images_downloader` | https://github.com/ryanvolz/radioconda/releases |
| **Zadig** | Installs WinUSB driver on the USRP's USB interface | https://zadig.akeo.ie |
| Visual Studio Build Tools 2022 | Already needed to build CyberEther | — |

Install RadioConda at the default path (`C:\Users\<user>\radioconda`). No conda environment
activation is needed — we only copy files out of it.

> If you already had the USRP working in **GNU Radio / gqrx / uhd_find_devices**, then the
> WinUSB driver (Step 5) and the FPGA images (Step 3) are probably already in place. You can
> skip straight to Steps 1–2 and 4.

---

## Step 0 — Build fix (MSVC string-literal limit) — **required, already committed**

Before any USRP work, the Windows build itself failed with:

```
bridge.hh(554): error C2026: string too big, trailing characters truncated
```

### Why

The Python runtime bridge header (`src/runtime/python/bridge/prelude/bridge.hh`) is
**generated at build time** by [`meson.py`](../src/runtime/python/bridge/prelude/meson.py),
which embeds several `.py` files into one C++ raw string literal:

```cpp
inline constexpr const char* kPythonBridge = R"JST_PY_BRIDGE(... all python ...)JST_PY_BRIDGE";
```

MSVC (C2026) rejects any single string literal larger than **16380 bytes**. GCC and Clang
have no such limit, which is why the project compiled on Linux/macOS but not on Windows.

### The fix

`meson.py` was changed to split the content into ~8000-byte chunks emitted as **adjacent
raw string literals**, which the C++ compiler automatically concatenates back together:

```cpp
inline constexpr const char* kPythonBridge =
R"JST_PY_BRIDGE(chunk 1...)JST_PY_BRIDGE"
R"JST_PY_BRIDGE(chunk 2...)JST_PY_BRIDGE"
R"JST_PY_BRIDGE(chunk 3...)JST_PY_BRIDGE"
;
```

Chunks are split on **character boundaries with a byte budget**, so each chunk is valid
UTF-8 on its own and concatenation reproduces the original content byte-for-byte. This is a
source-level fix — once built, the generated header stays correct on every rebuild.

---

## Step 1 — Install the SoapyUHD module

Copy the pre-built SoapyUHD module from RadioConda into CyberEther's module directory:

```bat
copy C:\Users\%USERNAME%\radioconda\Library\lib\SoapySDR\modules0.8\uhdSupport.dll ^
     lib\SoapySDR\modules0.8\
```

> Only **SoapySDR module DLLs** belong in `lib\SoapySDR\modules0.8\`. Do **not** put
> `uhd.dll` or any Boost DLL there — putting non-module DLLs in that folder makes SoapySDR
> try to load them as modules and can crash enumeration.

---

## Step 2 — Copy UHD runtime dependencies to `build\`

When `uhdSupport.dll` is loaded, Windows resolves its dependencies starting from the
**executable's directory** (`build\`, where `cyberether.exe` lives). Copy these there:

```bat
set BIN=C:\Users\%USERNAME%\radioconda\Library\bin

copy %BIN%\uhd.dll                 build\
copy %BIN%\boost_filesystem.dll    build\
copy %BIN%\boost_serialization.dll build\
copy %BIN%\boost_thread.dll        build\
```

`uhd.dll` directly imports `boost_filesystem`, `boost_serialization`, `boost_thread`, and
`libusb-1.0.dll`. **`libusb-1.0.dll` is already in `build\`** from the PlutoSDR setup; if you
skipped that, copy it too. Copying the rest of the Boost DLLs
(`boost_system`, `boost_chrono`, `boost_atomic`, `boost_date_time`) is harmless and a safe
hedge against version differences.

### Dependency chain

```
uhdSupport.dll                          (lib\SoapySDR\modules0.8\)
├── SoapySDR.dll                         ← built by meson, in build\
└── uhd.dll                              (build\)
    ├── boost_filesystem.dll             (build\)
    ├── boost_serialization.dll          (build\)
    ├── boost_thread.dll                 (build\)
    └── libusb-1.0.dll                   (build\, shared with PlutoSDR)
```

---

## Step 3 — Download the FPGA images

The B2xx family has **no on-board flash for the FPGA** — UHD loads the FPGA image over USB
every time the device is opened. The images are not shipped with RadioConda by default, so
download just the B2xx set:

```bat
set UHD_IMAGES_DIR=%USERPROFILE%\radioconda\Library\share\uhd\images
%USERPROFILE%\radioconda\Scripts\uhd_images_downloader.exe -t b2xx
```

This fetches `usrp_b200_fpga.bin`, `usrp_b210_fpga.bin`, etc. from `files.ettus.com` into
`radioconda\Library\share\uhd\images\`.

> The downloader logs to **stderr**, so it may look like an error even on success. The final
> line `Images download complete.` is what matters.

---

## Step 4 — Point CyberEther at the images (`run.bat`)

Because `uhd.dll` lives in `build\`, UHD's default image search path
(`build\..\share\uhd\images`) does not exist. Set `UHD_IMAGES_DIR` so UHD can find the
images. This line was added to [`run.bat`](../run.bat):

```bat
set UHD_IMAGES_DIR=%USERPROFILE%\radioconda\Library\share\uhd\images
```

Without it, the device **enumerates** fine (the dropdown shows it) but **fails to open**,
because loading the FPGA needs the image.

---

## Step 5 — Install the WinUSB driver via Zadig

USRP B2xx uses a Cypress FX3 USB controller that needs the WinUSB driver:

1. Connect the USRP via USB.
2. Open Zadig → **Options → List All Devices**.
3. Select the USRP entry (typically **VID `2500`**, or **`04B4`** before firmware loads).
4. Select **WinUSB** as the driver → **Install Driver**.

Verify from the command line:

```bat
set UHD_IMAGES_DIR=%USERPROFILE%\radioconda\Library\share\uhd\images
%USERPROFILE%\radioconda\Library\bin\uhd_find_devices.exe
```

Expected:

```
Device Address:
    serial: 328AFE3
    name: MyB210
    product: B210
    type: b200
```

---

## Step 6 — Run CyberEther

```bat
run.bat
```

1. Add a **Soapy SDR** block (right-click canvas → **Add Block → IO → Soapy SDR**).
2. Open the **Device** dropdown → it should list **`B210 <serial>`**.
3. Set **Frequency** (70 MHz – 6 GHz) and **Sample Rate** (up to 61.44 MHz, e.g. 10 MHz).
4. Wire the `signal` output into a **Spectrogram** / **Waterfall** block.

The first time the device opens, expect a **few seconds' pause** while UHD loads the FPGA
image onto the B2xx.

---

## Troubleshooting

### Device dropdown is empty even though `uhd_find_devices` works

The dropdown is populated **once**, when the Soapy block is created (`define()` calls
`SoapySDR::Device::enumerate()`), and the result is cached. If the device was **busy**
(GNU Radio / gqrx / another SoapySDR process still holding the USB), or not yet ready at that
moment, the dropdown stays empty until the block is recreated.

Fix:
1. Close any other program using the USRP (GNU Radio, gqrx, `uhd_*` tools).
2. **Delete the Soapy block and add it again**, or restart CyberEther, so enumeration
   re-runs against the now-free device.

### Verifying the DLL stack independently of the GUI

To confirm CyberEther's exact SoapySDR stack can see the device without launching the GUI,
a tiny test program was compiled against `build\SoapySDR.lib` and run from `build\`. It
loads the modules and enumerates, printing the SoapySDR log:

```
=== Loading each module ===
  load ...\PlutoSDRSupport.dll -> OK
  load ...\uhdSupport.dll      -> OK
=== Registered find functions (drivers) ===
  driver: plutosdr
  driver: uhd
=== Enumerating devices ===
  Found 1 device(s)
    driver = uhd
    label  = B210 328AFE3
```

If this succeeds but the GUI dropdown is empty, the problem is a stale/cached enumeration
(see above), **not** the DLL setup. The `[ERROR] [X300] ... network discovery` lines during
enumeration are harmless — UHD is scanning the network for X-series devices you don't have;
the USB device is still found.

### `error C2026: string too big` when building

You are missing the Step 0 fix. Make sure
[`src/runtime/python/bridge/prelude/meson.py`](../src/runtime/python/bridge/prelude/meson.py)
emits chunked string literals, then rebuild.

---

## File layout after setup

```
CyberEther\
├── build\
│   ├── cyberether.exe
│   ├── jetstream.dll
│   ├── SoapySDR.dll               ← built by meson (shared)
│   ├── uhd.dll                    ← from RadioConda
│   ├── boost_filesystem.dll       ← from RadioConda
│   ├── boost_serialization.dll    ← from RadioConda
│   ├── boost_thread.dll           ← from RadioConda
│   ├── libusb-1.0.dll             ← shared with PlutoSDR
│   └── (Pluto DLLs: libiio, libad9361, libxml2, zstd, iconv, charset, zlib, zlib1)
├── lib\
│   └── SoapySDR\
│       └── modules0.8\
│           ├── PlutoSDRSupport.dll
│           └── uhdSupport.dll      ← SoapyUHD module (from RadioConda)
└── (RadioConda)\Library\share\uhd\images\
    ├── usrp_b200_fpga.bin          ← FPGA images (via uhd_images_downloader)
    └── usrp_b210_fpga.bin
```

---

## Summary of everything that made USRP work

1. **Build fix** — chunk the generated `bridge.hh` string literal so MSVC (C2026) can compile it.
2. **Module** — `uhdSupport.dll` → `lib\SoapySDR\modules0.8\`.
3. **Runtime** — `uhd.dll` + Boost DLLs → `build\`.
4. **FPGA images** — `uhd_images_downloader -t b2xx`.
5. **Image path** — `UHD_IMAGES_DIR` set in `run.bat`.
6. **Driver** — WinUSB via Zadig (already done if it worked in GNU Radio).
7. **Enumerate with a free device** — close other SDR apps; re-add the block if the dropdown was cached empty.
