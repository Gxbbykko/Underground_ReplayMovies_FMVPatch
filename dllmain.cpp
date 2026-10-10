#include "FMVFix.h"

namespace
{
    DWORD WINAPI FMVFixInitThread(LPVOID parameter)
    {
        FMVFix::Initialize(static_cast<HMODULE>(parameter));
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        HANDLE thread = CreateThread(
            nullptr,
            0,
            FMVFixInitThread,
            hModule,
            0,
            nullptr);

        if (thread != nullptr)
            CloseHandle(thread);
    }

    return TRUE;
}