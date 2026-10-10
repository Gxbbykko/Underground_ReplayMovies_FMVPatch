#include <Windows.h>
#include "pch.h"

#include "CapacityHook.h"
#include "CandidateSafetyHook.h"
#include "GenerationHook.h"
#include "ReplayMoviesConfig.h"
#include "ReplayMoviesProgressionHook.h"

BOOL APIENTRY DllMain(
    HMODULE moduleHandle,
    DWORD reason,
    LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        // DllMain-safe only: remember this ASI module handle. ReplayMovies.ini
        // is read later from the normal ReplayMovies frontend action path.
        ReplayMoviesConfig::Initialize(moduleHandle);

        // Never expose count 7 unless the safety mechanism
        // is installed first.
        if (InstallCandidateSafetyHook())
        {
            if (InstallCapacityHook())
            {
                if (InstallGenerationHook())
                {
                    // Phase 2B only observes the already-proven native movie
                    // lifecycle and persists profile-local SeenMask bits.
                    // Hook installation performs no file I/O.
                    InstallReplayMoviesProgressionHooks();
                }
            }
        }
    }

    return TRUE;
}