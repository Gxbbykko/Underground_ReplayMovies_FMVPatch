#pragma once

#include <cstddef>
#include <cstdint>

namespace ReplayMoviesProfileStorage
{
    // Debugger-proven native profile creation copies at most 0x0D bytes into
    // the active-name buffer: up to 12 visible characters plus the terminator.
    constexpr std::size_t kProfileNameBytes = 12;

    struct ProfileIdentity
    {
        // Zero-padded full native profile name bytes.  The key is deliberately
        // wider than the obsolete eight-byte implementation so profiles that
        // share their first eight characters remain independent.
        std::uint8_t key[kProfileNameBytes]{};
        char name[kProfileNameBytes + 1]{};
    };

    enum class Status : std::uint32_t
    {
        Uninitialized = 0,
        Ready = 1,
        ModulePathUnavailable = 2,
        WidescreenFixIniNotFound = 3,
        CustomUserFilesDirectoryDisabled = 4,
        ProfilesFileNotFound = 5,   // retained for telemetry compatibility
        ProfilesFileInvalid = 6,    // retained for telemetry compatibility
        ActiveProfileUnavailable = 7,
        ProgressReadFailed = 8,
        ProgressWriteFailed = 9,
    };

    // Called only from the debugger-proven native profile create/load
    // completion paths. Reads the already-published native active profile name,
    // creates the corresponding ReplayMoviesProgress.ini section when absent,
    // loads its SeenMask, and makes that identity the current ReplayMovies
    // profile. ReplayMovies menu entry must not call this.
    bool ActivateCurrentProfile() noexcept;

    // Captures the already-activated profile identity at native movie start.
    // No profile discovery, section creation, or refresh occurs here.  The
    // returned identity is carried through to teardown so a later profile
    // switch can never redirect an in-flight movie transaction.
    bool CaptureCurrentProfile(ProfileIdentity& output) noexcept;

    // Commits one canonical movie bit to the supplied start-time profile
    // snapshot. The profile identity is NOT re-resolved here.
    bool MarkMovieSeenForProfile(
        const ProfileIdentity& profile,
        std::size_t canonicalMovieIndex) noexcept;

    bool HasCurrentProfile() noexcept;
    const char* GetCurrentProfileName() noexcept;
    std::uint32_t GetCurrentSeenMask() noexcept;
    bool IsMovieSeen(std::size_t canonicalMovieIndex) noexcept;
}

extern "C"
{
    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesStorageStatus;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesProfileRefreshes;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesProfileRefreshSuccesses;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesProgressLoads;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesProgressWrites;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesCurrentSeenMask;

    __declspec(dllexport)
        extern volatile unsigned long g_ReplayMoviesCustomSaveDirectoryEnabled;

    __declspec(dllexport)
        extern char g_ReplayMoviesCurrentProfileName[13];

    __declspec(dllexport)
        extern char g_ReplayMoviesCurrentProfileKeyHex[25];
}