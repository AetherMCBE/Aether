#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <thread>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        std::thread([=]() {
            spdlog::default_logger()->sinks().assign({
                std::make_shared<spdlog::sinks::msvc_sink_mt>()
            });

            spdlog::info("Hello from Aether!");

            // With std::thread, FreeLibrary should be used instead of FreeLibraryAndExitThread
            FreeLibrary(hinstDLL);
        }).detach();
    }

    return TRUE;
}
