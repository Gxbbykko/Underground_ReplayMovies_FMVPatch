#pragma once

#include <cstdint>

bool BuildReplayMoviesForGeneration(
    std::uintptr_t runtimeAddress,
    std::uintptr_t vectorAddress,
    std::uintptr_t undergroundAddress);

// Called by the MU_Main current-row commit hook. These functions are
// deliberately generation-relative: they reacquire hightlight01 from the
// current MU_Main FEObject graph and never cache heap addresses.
void ApplyReplayMoviesVisualTransitionPreCommit(
    std::uintptr_t runtimeAddress,
    std::uintptr_t oldRowAddress,
    std::uintptr_t newRowAddress);

// Called only by the debugger-proven direct post-return selection path.
// 004F5ED0 bypasses the normal 004EFE87 pre-commit hook, so prepare only
// hightlight01's selected Y for ReplayMovies while deliberately leaving
// ReplayMovies +54 on its root/startup node. Moving +54 to the unselected
// node during MU_Main generation completion reintroduces the proven
// premature-render defect.
bool PrepareReplayMoviesPostReturnNativeSelection(
    std::uintptr_t runtimeAddress,
    std::uintptr_t replayAddress);

// Called by the native FE_String +54 writer hook at 004F1116.
// Returns the node pointer that native code should write. It normally
// returns incomingNodeAddress unchanged; only a debugger-proven
// ReplayMovies -> UndergroundMode hightlight01 tail write is substituted.
std::uintptr_t ResolveReplayMoviesActiveNodeWrite(
    std::uintptr_t parentObjectAddress,
    std::uintptr_t incomingNodeAddress);

// Called by the native live geometry writer hook at 004FC374.
// It normally returns incomingY unchanged. During only the exact armed
// ReplayMovies -> QuickRace transient, the debugger-proven H1 live-Y
// output -126 is substituted with the current organized ReplayMovies
// presentation Y so the highlight never visibly passes through
// UndergroundMode before QuickRace settles.
std::uint32_t ResolveReplayMoviesQuickRaceTransientYWrite(
    std::uintptr_t writerBaseAddress,
    std::uint32_t incomingY);

extern "C"
{
    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesBuildAttempts;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesBuildSuccesses;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesBuildStage;

    __declspec(dllexport)
        extern volatile std::uintptr_t g_LastReplayMoviesObject;
}