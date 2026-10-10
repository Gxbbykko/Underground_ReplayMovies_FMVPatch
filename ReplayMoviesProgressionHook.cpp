#include "pch.h"
#include "ReplayMoviesProgressionHook.h"

#include "Addresses.h"
#include "MemoryPatch.h"
#include "ReplayMoviesCatalog.h"
#include "ReplayMoviesProfileStorage.h"

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C"
{
    // Owned by GenerationHook.cpp. This is the debugger-proven discriminator:
    // ReplayMovies manual playback == 1, ordinary Career/native playback == 0.
    extern volatile unsigned long g_ReplayMoviesSubmenuActive;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesPendingCareerMovieIndex =
        0xFFFFFFFFul;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCareerStartObservations = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCareerMoviesRecognized = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCareerMoviesRejectedReplayMovies = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCareerProfileCaptureFailures = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCareerCommitAttempts = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCareerCommitSuccesses = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCareerCommitFailures = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesLastCommittedCareerMovieIndex =
        0xFFFFFFFFul;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesProfileCreateLifecycleHits = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesProfileLoadLifecycleHits = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesProfileLifecycleFailures = 0;

    // Retained for debugger/source compatibility. The 0x0041D8A0 native
    // writer detour was runtime-proven to disturb Career FMV smoothness and
    // is intentionally not installed anymore, so these remain zero.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesNativeSaveCheckpointHits = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesNativeSaveCheckpointFailures = 0;

    // 0 = not attempted, 1 = installed,
    // 2 = start-observer signature mismatch,
    // 3 = teardown signature mismatch,
    // 4 = teardown detour write failed,
    // 5 = start-observer detour write failed,
    // 6 = create-call signature mismatch,
    // 7 = load-call signature mismatch,
    // 8/9 = retired (former native-writer checkpoint hook),
    // 10 = create-call patch failed,
    // 11 = load-call patch failed,
    // 12 = skip-teardown signature mismatch,
    // 13 = skip-teardown detour write failed.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesProgressionHookStatus = 0;
}

namespace
{
    constexpr std::uint32_t kNoPendingMovie = 0xFFFFFFFFu;

    ReplayMoviesProfileStorage::ProfileIdentity g_PendingProfile{};
    bool g_PendingProfileValid = false;

    DWORD g_NativeMovieStartObserveReturn =
        static_cast<DWORD>(Addr::NativeMovieStartObserveReturn);

    DWORD g_NativeMovieTeardownReturn =
        static_cast<DWORD>(Addr::NativeMovieTeardownReturn);

    DWORD g_NativeMovieSkipTeardownReturn =
        static_cast<DWORD>(Addr::NativeMovieSkipTeardownReturn);

    DWORD g_NativeProfileCreatePublishOriginal =
        static_cast<DWORD>(Addr::NativeProfileCreatePublishOriginal);

    DWORD g_NativeProfileCreateComplete =
        static_cast<DWORD>(Addr::NativeProfileCreateComplete);

    DWORD g_NativeProfileLoadOriginal =
        static_cast<DWORD>(Addr::NativeProfileLoadOriginal);

    DWORD g_NativeProfileLoadComplete =
        static_cast<DWORD>(Addr::NativeProfileLoadComplete);

    bool WriteRelativeCall5(
        std::uintptr_t source,
        const void* destination) noexcept
    {
        DWORD oldProtect = 0;
        auto* patch = reinterpret_cast<std::uint8_t*>(source);

        if (!VirtualProtect(
            patch,
            5,
            PAGE_EXECUTE_READWRITE,
            &oldProtect))
        {
            return false;
        }

        const std::intptr_t relative =
            reinterpret_cast<std::intptr_t>(destination) -
            static_cast<std::intptr_t>(source + 5u);
        const std::int32_t relative32 =
            static_cast<std::int32_t>(relative);

        if (static_cast<std::intptr_t>(relative32) != relative)
        {
            DWORD ignored = 0;
            VirtualProtect(patch, 5, oldProtect, &ignored);
            return false;
        }

        patch[0] = 0xE8;
        std::memcpy(patch + 1, &relative32, sizeof(relative32));

        FlushInstructionCache(GetCurrentProcess(), patch, 5);

        DWORD ignored = 0;
        VirtualProtect(patch, 5, oldProtect, &ignored);
        return true;
    }

    void ClearPendingCareerMovie() noexcept
    {
        g_ReplayMoviesPendingCareerMovieIndex = kNoPendingMovie;
        std::memset(&g_PendingProfile, 0, sizeof(g_PendingProfile));
        g_PendingProfileValid = false;
    }

    unsigned char AsciiLower(unsigned char value) noexcept
    {
        if (value >= 'A' && value <= 'Z')
            return static_cast<unsigned char>(value + ('a' - 'A'));

        return value;
    }

    bool EqualsAsciiIgnoreCase(const char* left, const char* right) noexcept
    {
        if (left == nullptr || right == nullptr)
            return false;

        while (*left != '\0' && *right != '\0')
        {
            if (AsciiLower(static_cast<unsigned char>(*left++)) !=
                AsciiLower(static_cast<unsigned char>(*right++)))
            {
                return false;
            }
        }

        return *left == '\0' && *right == '\0';
    }

    bool HasMoviesDirectoryPrefix(const char* path) noexcept
    {
        static constexpr char kPrefix[] = "MOVIES\\";

        if (path == nullptr)
            return false;

        for (std::size_t i = 0; i < sizeof(kPrefix) - 1u; ++i)
        {
            unsigned char pathCharacter =
                static_cast<unsigned char>(path[i]);
            unsigned char prefixCharacter =
                static_cast<unsigned char>(kPrefix[i]);

            if (pathCharacter == '/')
                pathCharacter = '\\';

            if (AsciiLower(pathCharacter) != AsciiLower(prefixCharacter))
                return false;
        }

        return true;
    }

    int FindCanonicalCareerMovieIndex(const char* resolvedMoviePath) noexcept
    {
        if (!HasMoviesDirectoryPrefix(resolvedMoviePath))
            return -1;

        const char* fileName = resolvedMoviePath + 7; // strlen("MOVIES\\")

        for (std::size_t index = 0;
            index < ReplayMoviesCatalog::kCareerMovieCount;
            ++index)
        {
            char expected[32]{};
            const char* basename =
                ReplayMoviesCatalog::kCareerMovies[index].madBasename;

            const std::size_t basenameLength = std::strlen(basename);
            if (basenameLength + 5u > sizeof(expected))
                continue;

            std::memcpy(expected, basename, basenameLength);
            std::memcpy(expected + basenameLength, ".mad", 5u);

            if (EqualsAsciiIgnoreCase(fileName, expected))
                return static_cast<int>(index);
        }

        return -1;
    }

    __declspec(noinline)
        void __cdecl ActivateProfileAfterCreateLocal()
    {
        ++g_ReplayMoviesProfileCreateLifecycleHits;
        if (!ReplayMoviesProfileStorage::ActivateCurrentProfile())
            ++g_ReplayMoviesProfileLifecycleFailures;
    }

    __declspec(noinline)
        void __cdecl ActivateProfileAfterLoadLocal()
    {
        ++g_ReplayMoviesProfileLoadLifecycleHits;
        if (!ReplayMoviesProfileStorage::ActivateCurrentProfile())
            ++g_ReplayMoviesProfileLifecycleFailures;
    }

    __declspec(naked)
        void NativeProfileCreatePostReturnHook()
    {
        __asm
        {
            pushfd
            pushad
            call ActivateProfileAfterCreateLocal
            popad
            popfd
            jmp dword ptr[g_NativeProfileCreateComplete]
        }
    }

    __declspec(naked)
        void NativeProfileCreatePublishCallHook()
    {
        __asm
        {
            // The patched native CALL already placed 0041CF75 on the stack.
            // Replace only that return address, then tail-jump to the original
            // 004AA4D0 function so its arguments and stack cleanup stay native.
            push eax
            mov eax, offset NativeProfileCreatePostReturnHook
            mov dword ptr[esp + 04h], eax
            pop eax
            jmp dword ptr[g_NativeProfileCreatePublishOriginal]
        }
    }

    __declspec(naked)
        void NativeProfileLoadPostReturnHook()
    {
        __asm
        {
            pushfd
            pushad
            call ActivateProfileAfterLoadLocal
            popad
            popfd
            jmp dword ptr[g_NativeProfileLoadComplete]
        }
    }

    __declspec(naked)
        void NativeProfileLoadCallHook()
    {
        __asm
        {
            // Same return-address substitution as creation.  004AB200 keeps
            // its proven ret-4 contract; ReplayMovies runs only after return.
            push eax
            mov eax, offset NativeProfileLoadPostReturnHook
            mov dword ptr[esp + 04h], eax
            pop eax
            jmp dword ptr[g_NativeProfileLoadOriginal]
        }
    }

    __declspec(noinline)
        void __cdecl ObserveNativeMovieStartLocal(const char* resolvedMoviePath)
    {
        ++g_ReplayMoviesCareerStartObservations;

        // Exactly match the debugger-proven transaction: every new movie
        // request first invalidates stale pending state.
        ClearPendingCareerMovie();

        // Manual ReplayMovies playback must never unlock User progression.
        if (g_ReplayMoviesSubmenuActive != 0)
        {
            ++g_ReplayMoviesCareerMoviesRejectedReplayMovies;
            return;
        }

        const int canonicalMovieIndex =
            FindCanonicalCareerMovieIndex(resolvedMoviePath);

        if (canonicalMovieIndex < 0)
            return;

        ++g_ReplayMoviesCareerMoviesRecognized;

        ReplayMoviesProfileStorage::ProfileIdentity profile{};
        if (!ReplayMoviesProfileStorage::CaptureCurrentProfile(profile))
        {
            ++g_ReplayMoviesCareerProfileCaptureFailures;
            return;
        }

        // Bind the transaction to the profile that was active when the native
        // game started the movie. Teardown never re-resolves another profile.
        g_PendingProfile = profile;
        g_PendingProfileValid = true;
        g_ReplayMoviesPendingCareerMovieIndex =
            static_cast<unsigned long>(canonicalMovieIndex);
    }

    __declspec(noinline)
        void __cdecl CommitPendingCareerMovieLocal()
    {
        const std::uint32_t pendingMovie =
            static_cast<std::uint32_t>(
                g_ReplayMoviesPendingCareerMovieIndex);

        // Explicit sentinel rejection is intentional. The debugger harness
        // proved that treating FFFFFFFF as a range-tested index is unsafe.
        if (pendingMovie == kNoPendingMovie ||
            pendingMovie >= ReplayMoviesCatalog::kCareerMovieCount ||
            !g_PendingProfileValid)
        {
            ClearPendingCareerMovie();
            return;
        }

        ++g_ReplayMoviesCareerCommitAttempts;

        const bool committed =
            ReplayMoviesProfileStorage::MarkMovieSeenForProfile(
                g_PendingProfile,
                static_cast<std::size_t>(pendingMovie));

        if (committed)
        {
            ++g_ReplayMoviesCareerCommitSuccesses;
            g_ReplayMoviesLastCommittedCareerMovieIndex = pendingMovie;
        }
        else
        {
            ++g_ReplayMoviesCareerCommitFailures;
        }

        ClearPendingCareerMovie();
    }

    __declspec(naked)
        void NativeMovieStartObserveHook()
    {
        __asm
        {
            // Cooperative entry is 005A4AC5. FMVFix/XboxPort owns the
            // preceding 005A4ABD-005A4AC4 range and returns here after
            // reproducing the native ECX/EDX argument loads. Before our
            // saves, the original StartMovie stack contract is unchanged:
            //   [ESP+04] = resolved movie path.
            pushfd
            pushad

            // pushfd + pushad consume 0x24 bytes, so original [ESP+04]
            // becomes [ESP+28].
            mov eax, dword ptr[esp + 28h]
            push eax
            call ObserveNativeMovieStartLocal
            add esp, 4

            popad
            popfd

            // Replay the exact seven native bytes overwritten at 005A4AC5.
            // The push shifts the original stack by four bytes, so the
            // native mov ecx,[esp+08] still reloads the movie path exactly
            // as it did before this observer was installed.
            push esi
            mov dword ptr[eax], ecx
            mov ecx, dword ptr[esp + 08h]

            jmp dword ptr[g_NativeMovieStartObserveReturn]
        }
    }

    __declspec(naked)
        void NativeMovieTeardownHook()
    {
        __asm
        {
            pushfd
            pushad
            call CommitPendingCareerMovieLocal

            // Replay the exact native teardown write while the original
            // registers are still saved. Using EAX as the absolute-address
            // scratch register is harmless because popad restores native EAX.
            mov eax, 00733F6Ch
            mov dword ptr[eax], 0

            popad
            popfd
            jmp dword ptr[g_NativeMovieTeardownReturn]
        }
    }

    __declspec(naked)
        void NativeMovieSkipTeardownHook()
    {
        __asm
        {
            // Debugger proof: skipped Career movies bypass 005A4C1F and clear
            // the active movie object at 005A4CAF instead. Commit the exact
            // same pending profile-bound transaction before replaying that
            // native clear.
            pushfd
            pushad
            call CommitPendingCareerMovieLocal

            mov eax, 00733F6Ch
            mov dword ptr[eax], 0

            popad
            popfd
            jmp dword ptr[g_NativeMovieSkipTeardownReturn]
        }
    }
}

bool InstallReplayMoviesProgressionHooks()
{
    // Exact debugger-proven whole-instruction/call signatures.
    static const std::uint8_t expectedStartObserver[7] =
    {
        0x56,                         // push esi
        0x89, 0x08,                   // mov [eax],ecx
        0x8B, 0x4C, 0x24, 0x08        // mov ecx,[esp+08]
    };

    static const std::uint8_t expectedTeardown[10] =
    {
        0xC7, 0x05, 0x6C, 0x3F, 0x73, 0x00,
        0x00, 0x00, 0x00, 0x00        // mov dword ptr [00733F6C],0
    };

    static const std::uint8_t expectedSkipTeardown[10] =
    {
        0xC7, 0x05, 0x6C, 0x3F, 0x73, 0x00,
        0x00, 0x00, 0x00, 0x00        // 005A4CAF: same active-object clear
    };

    static const std::uint8_t expectedCreateCall[5] =
    {
        0xE8, 0xED, 0xD4, 0x08, 0x00  // 0041CFDE: call 004AA4D0
    };

    static const std::uint8_t expectedLoadCall[5] =
    {
        0xE8, 0xCC, 0xD6, 0x08, 0x00  // call 004AB200
    };

    const auto* currentStartObserver =
        reinterpret_cast<const std::uint8_t*>(
            Addr::NativeMovieStartObserveHook);
    const auto* currentTeardown =
        reinterpret_cast<const std::uint8_t*>(
            Addr::NativeMovieTeardownHook);
    const auto* currentSkipTeardown =
        reinterpret_cast<const std::uint8_t*>(
            Addr::NativeMovieSkipTeardownHook);
    const auto* currentCreateCall =
        reinterpret_cast<const std::uint8_t*>(
            Addr::NativeProfileCreatePublishCall);
    const auto* currentLoadCall =
        reinterpret_cast<const std::uint8_t*>(
            Addr::NativeProfileLoadCall);

    g_ReplayMoviesProgressionHookStatus = 0;

    // Fail closed before changing any code bytes.
    if (std::memcmp(
        currentStartObserver,
        expectedStartObserver,
        sizeof(expectedStartObserver)) != 0)
    {
        g_ReplayMoviesProgressionHookStatus = 2;
        return false;
    }

    if (std::memcmp(
        currentTeardown,
        expectedTeardown,
        sizeof(expectedTeardown)) != 0)
    {
        g_ReplayMoviesProgressionHookStatus = 3;
        return false;
    }

    if (std::memcmp(
        currentSkipTeardown,
        expectedSkipTeardown,
        sizeof(expectedSkipTeardown)) != 0)
    {
        g_ReplayMoviesProgressionHookStatus = 12;
        return false;
    }

    if (std::memcmp(
        currentCreateCall,
        expectedCreateCall,
        sizeof(expectedCreateCall)) != 0)
    {
        g_ReplayMoviesProgressionHookStatus = 6;
        return false;
    }

    if (std::memcmp(
        currentLoadCall,
        expectedLoadCall,
        sizeof(expectedLoadCall)) != 0)
    {
        g_ReplayMoviesProgressionHookStatus = 7;
        return false;
    }

    if (!WriteRelativeCall5(
        Addr::NativeProfileCreatePublishCall,
        reinterpret_cast<const void*>(&NativeProfileCreatePublishCallHook)))
    {
        g_ReplayMoviesProgressionHookStatus = 10;
        return false;
    }

    if (!WriteRelativeCall5(
        Addr::NativeProfileLoadCall,
        reinterpret_cast<const void*>(&NativeProfileLoadCallHook)))
    {
        g_ReplayMoviesProgressionHookStatus = 11;
        return false;
    }

    // Install both debugger-proven teardown exits before StartMovie. If the
    // start observer cannot be installed, both remain harmless no-ops because
    // pending starts NONE.
    if (!WriteAbsoluteJumpPadded(
        Addr::NativeMovieTeardownHook,
        reinterpret_cast<const void*>(&NativeMovieTeardownHook),
        sizeof(expectedTeardown)))
    {
        g_ReplayMoviesProgressionHookStatus = 4;
        return false;
    }

    if (!WriteAbsoluteJumpPadded(
        Addr::NativeMovieSkipTeardownHook,
        reinterpret_cast<const void*>(&NativeMovieSkipTeardownHook),
        sizeof(expectedSkipTeardown)))
    {
        g_ReplayMoviesProgressionHookStatus = 13;
        return false;
    }

    if (!WriteAbsoluteJumpPadded(
        Addr::NativeMovieStartObserveHook,
        reinterpret_cast<const void*>(&NativeMovieStartObserveHook),
        sizeof(expectedStartObserver)))
    {
        g_ReplayMoviesProgressionHookStatus = 5;
        return false;
    }

    g_ReplayMoviesProgressionHookStatus = 1;
    return true;
}