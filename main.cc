#include "jetstream/run.hh"
#include "updater.hh"

#include <cstdlib>
#include <fstream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <iostream>
#endif

using namespace Jetstream;

#if defined(JST_OS_BROWSER)
extern "C" {
EMSCRIPTEN_KEEPALIVE
void cyberether_shutdown() {
    RequestShutdown();
}
}
#endif

namespace {

// Kept alive for the whole program so the log sink stays valid.
std::ofstream g_logFile;

void CyberEtherSetupLogging() {
    // Color control. Logs carry ANSI color codes by default; disable them if the user opts
    // out via the standard NO_COLOR convention or JST_LOG_COLOR=0 (useful for consoles that
    // render the escape codes as raw text like "←[1m" instead of colors).
    const char* colorEnv = std::getenv("JST_LOG_COLOR");
    const bool noColor = (std::getenv("NO_COLOR") != nullptr) ||
                         (colorEnv != nullptr && colorEnv[0] == '0');
    if (noColor) {
        JST_LOG_COLOR(false);
    }

    // If JST_LOG_FILE is set, mirror all logs into that file. This routes the logger sink
    // straight to an ofstream (flushed per line via std::endl), which is reliable regardless
    // of the GUI-subsystem stdio quirks. Color is always stripped for a clean, greppable log.
    const char* path = std::getenv("JST_LOG_FILE");
    if (path && *path) {
        g_logFile.open(path, std::ios::out | std::ios::trunc);
        if (g_logFile.is_open()) {
            JST_LOG_COLOR(false);
            JST_LOG_SET_SINK(&g_logFile);
        }
    }
}

int CyberEtherMain(int argc, char* argv[]) {
    CyberEtherSetupLogging();
    try {
        Updater::Initialize(argc, argv);
#if defined(JST_OS_BROWSER)
        (void)argc;
        (void)argv;
        return Run();
#endif
#if defined(JST_OS_LINUX) || defined(JST_OS_WINDOWS) || defined(JST_OS_MAC)
        return Run(argc, argv);
#endif
    } catch (const Result& status) {
        JST_ERROR("[CYBERETHER] Exception: {}", status);
        return -1;
    } catch (const std::exception& e) {
        JST_ERROR("[CYBERETHER] Exception: {}", e.what());
        return -1;
    } catch (...) {
        JST_ERROR("[CYBERETHER] Unknown exception.");
        return -1;
    }

    return 0;
}

}

#if defined(JST_OS_WINDOWS)
// CyberEther is built as a GUI-subsystem app, so its standard streams are detached when it
// is launched from a terminal — logs (which go to std::cout) would be invisible. Reattach to
// the parent console, if there is one, so `run.bat` / cmd / PowerShell show live logs. Any
// explicit redirection (e.g. `cyberether.exe > cyberether.log`) is detected and left intact.
static void CyberEtherAttachParentConsole() {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        return;  // No parent console (e.g. launched by double-click) — nothing to attach to.
    }

    // Only rebind a stream to the console when it is NOT already pointed at a real file or
    // pipe — that way an explicit `> cyberether.log` or `| tee` redirection is preserved,
    // while the plain "run from a terminal" case gets live console output.
    const auto redirected = [](DWORD handle) {
        const DWORD type = GetFileType(GetStdHandle(handle));
        return type == FILE_TYPE_DISK || type == FILE_TYPE_PIPE;
    };

    FILE* stream = nullptr;
    if (!redirected(STD_OUTPUT_HANDLE)) {
        freopen_s(&stream, "CONOUT$", "w", stdout);
    }
    if (!redirected(STD_ERROR_HANDLE)) {
        freopen_s(&stream, "CONOUT$", "w", stderr);
    }

    std::ios::sync_with_stdio(true);
    std::cout.clear();
    std::cerr.clear();

    // Enable ANSI/VT processing on the console so the logger's color codes render as colors
    // instead of showing up as raw escape sequences (e.g. "←[1m"). Requires Windows 10+.
    const HANDLE console = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                       OPEN_EXISTING, 0, nullptr);
    if (console != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(console, &mode)) {
            SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
        CloseHandle(console);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    CyberEtherAttachParentConsole();
    // Disable stdio buffering so logs appear live — otherwise, when redirected to a file or
    // pipe (block-buffered by default), they only surface in ~4 KB bursts or on exit.
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    return CyberEtherMain(__argc, __argv);
}
#else
int main(int argc, char* argv[]) {
    return CyberEtherMain(argc, argv);
}
#endif
