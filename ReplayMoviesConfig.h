#pragma once

#include <Windows.h>
#include <cstdint>

namespace ReplayMoviesConfig
{
    enum class AccessMode : std::uint32_t
    {
        User = 0,
        Developer = 1,
    };

    // DllMain-safe setup: this only remembers the module handle and resets
    // configuration state. ReplayMovies.ini is not read until Refresh() is
    // called later from the normal frontend action path.
    void Initialize(HMODULE moduleHandle) noexcept;

    // Re-read ReplayMovies.ini from the directory containing ReplayMovies.asi.
    // Missing or invalid Mode values fail closed to User.
    bool Refresh() noexcept;

    AccessMode GetAccessMode() noexcept;
    bool IsUserMode() noexcept;
    bool IsDeveloperMode() noexcept;
}

extern "C"
{
    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesAccessMode;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesConfigRefreshes;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesConfigInvalidModeValues;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesConfigIniFound;
}