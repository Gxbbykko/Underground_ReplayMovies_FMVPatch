#pragma once

// Installs the fail-closed native profile create/load lifecycle hooks plus the
// already-proven Career movie start/teardown recorder. Profile create/load
// establish the current ReplayMovies identity; only native Career FMV teardown
// unlocks movie bits. The native 0x0041D8A0 .ugd writer is deliberately left
// completely untouched because its ReplayMovies detour was runtime-proven to
// disturb Career FMV smoothness. No frontend/catalog projection or post-return
// mapping is implemented here.
bool InstallReplayMoviesProgressionHooks();

extern "C"
{
    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesProgressionHookStatus;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesProfileCreateLifecycleHits;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesProfileLoadLifecycleHits;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesProfileLifecycleFailures;

    // Retained debugger telemetry. With the native save hook removed these
    // counters intentionally remain zero.
    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesNativeSaveCheckpointHits;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesNativeSaveCheckpointFailures;
}