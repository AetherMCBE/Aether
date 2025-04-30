#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <thread>

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        std::thread([=]() {
            OutputDebugStringA("Hello from Aether!");

            // With std::thread, FreeLibrary should be used instead of FreeLibraryAndExitThread
            FreeLibrary(hinstDLL);
        }).detach();
    }

    return TRUE;
}