#pragma once

#include <cstdint>

bool InstallGenerationHook();

extern "C"
{
    __declspec(dllexport)
        extern volatile unsigned long g_MuMainGenerationHits;

    __declspec(dllexport)
        extern volatile std::uintptr_t g_LastMuMainRuntime;

    __declspec(dllexport)
        extern volatile std::uintptr_t g_LastMuMainVector;

    __declspec(dllexport)
        extern volatile std::uintptr_t g_LastUndergroundMode;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesDispatchHits;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesSubmenuActive;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesExitHits;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesCatalogTitleUpdates;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesMovieIdentityOverrides;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesSelectedDeveloperMovieIndex;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesDeveloperCatalogPage;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesCatalogConfiguredSlots;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesCatalogSelectionUpdates;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesUserProjectionApplies;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesUserProjectionGuardFallbacks;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesUserVisibleCount;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesUserSeenMask;

    __declspec(dllexport)
        extern volatile std::uintptr_t g_ReplayMoviesUserProjectionRuntime;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesCatalogPageTransitions;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesBorrowedRouteStateSaves;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesBorrowedRouteStateRestores;

    __declspec(dllexport)
        extern volatile unsigned long g_PendingReplayMoviesMainMenuRestore;

    __declspec(dllexport)
        extern volatile std::uintptr_t g_ReplayMoviesNativeRestoreContext;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesNativeRestoreContextCaptures;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesNativeRestoreAttempts;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesNativeRestoreSuccess;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesNativeRestoreFailures;
}