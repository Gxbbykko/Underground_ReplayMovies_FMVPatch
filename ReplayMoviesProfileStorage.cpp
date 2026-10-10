#include "pch.h"
#include "ReplayMoviesProfileStorage.h"
#include "ReplayMoviesCatalog.h"
#include "Addresses.h"

#include <Windows.h>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C"
{
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesStorageStatus =
        static_cast<unsigned long>(
            ReplayMoviesProfileStorage::Status::Uninitialized);

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesProfileRefreshes = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesProfileRefreshSuccesses = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesProgressLoads = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesProgressWrites = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCurrentSeenMask = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCustomSaveDirectoryEnabled = 0;

    __declspec(dllexport)
        char g_ReplayMoviesCurrentProfileName[13] = {};

    __declspec(dllexport)
        char g_ReplayMoviesCurrentProfileKeyHex[25] = {};
}

namespace
{
    using ReplayMoviesProfileStorage::ProfileIdentity;
    using ReplayMoviesProfileStorage::Status;

    constexpr std::size_t kPathCapacity = MAX_PATH;
    constexpr std::size_t kProfileNameBytes =
        ReplayMoviesProfileStorage::kProfileNameBytes;
    constexpr std::size_t kLegacyProfileNameBytes = 8;
    constexpr std::uint32_t kAllCareerMovieBits =
        (1u << ReplayMoviesCatalog::kCareerMovieCount) - 1u;

    char g_ModuleDirectory[kPathCapacity]{};
    char g_GameDirectory[kPathCapacity]{};
    char g_ReplayMoviesProgressPath[kPathCapacity]{};
    char g_WidescreenFixIniPath[kPathCapacity]{};
    char g_CustomUserFilesDirectory[kPathCapacity]{};

    std::uint8_t g_CurrentProfileKey[kProfileNameBytes]{};
    bool g_PathsReady = false;

    void SetStatus(Status status) noexcept
    {
        g_ReplayMoviesStorageStatus =
            static_cast<unsigned long>(status);
    }

    bool FileExists(const char* path) noexcept
    {
        if (path == nullptr || path[0] == '\0')
            return false;

        const DWORD attributes = GetFileAttributesA(path);
        return attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    bool CopyString(
        char* destination,
        std::size_t capacity,
        const char* source) noexcept
    {
        if (destination == nullptr || capacity == 0 || source == nullptr)
            return false;

        const std::size_t length = std::strlen(source);
        if (length >= capacity)
            return false;

        std::memcpy(destination, source, length + 1u);
        return true;
    }

    bool GetDirectoryFromModule(
        HMODULE module,
        char* output,
        std::size_t capacity) noexcept
    {
        if (module == nullptr || output == nullptr || capacity == 0)
            return false;

        char path[kPathCapacity]{};
        const DWORD length = GetModuleFileNameA(
            module,
            path,
            static_cast<DWORD>(sizeof(path)));

        if (length == 0 || length >= sizeof(path))
            return false;

        char* lastSlash = std::strrchr(path, '\\');
        if (lastSlash == nullptr)
            lastSlash = std::strrchr(path, '/');

        if (lastSlash == nullptr)
            return false;

        *lastSlash = '\0';
        return CopyString(output, capacity, path);
    }

    bool GetThisModule(HMODULE& module) noexcept
    {
        module = nullptr;

        const BOOL ok = GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&GetThisModule),
            &module);

        return ok != FALSE && module != nullptr;
    }

    bool JoinPath(
        char* output,
        std::size_t capacity,
        const char* base,
        const char* child) noexcept
    {
        if (output == nullptr || capacity == 0 ||
            base == nullptr || child == nullptr)
        {
            return false;
        }

        const std::size_t baseLength = std::strlen(base);
        const std::size_t childLength = std::strlen(child);
        const bool needsSlash =
            baseLength != 0 &&
            base[baseLength - 1] != '\\' &&
            base[baseLength - 1] != '/';

        const std::size_t required =
            baseLength + (needsSlash ? 1u : 0u) + childLength + 1u;

        if (required > capacity)
            return false;

        std::memcpy(output, base, baseLength);
        std::size_t offset = baseLength;

        if (needsSlash)
            output[offset++] = '\\';

        std::memcpy(output + offset, child, childLength);
        output[offset + childLength] = '\0';
        return true;
    }

    void TrimIniValue(char* value) noexcept
    {
        if (value == nullptr)
            return;

        if (char* comment = std::strchr(value, ';'))
            *comment = '\0';
        if (char* comment = std::strchr(value, '#'))
            *comment = '\0';

        char* begin = value;
        while (*begin != '\0' &&
            std::isspace(static_cast<unsigned char>(*begin)) != 0)
        {
            ++begin;
        }

        char* end = begin + std::strlen(begin);
        while (end > begin &&
            std::isspace(static_cast<unsigned char>(end[-1])) != 0)
        {
            --end;
        }

        *end = '\0';

        if (begin != value)
        {
            std::memmove(
                value,
                begin,
                static_cast<std::size_t>(end - begin) + 1u);
        }
    }

    bool EqualsIgnoreCase(const char* left, const char* right) noexcept
    {
        if (left == nullptr || right == nullptr)
            return false;

        while (*left != '\0' && *right != '\0')
        {
            const unsigned char l = static_cast<unsigned char>(*left++);
            const unsigned char r = static_cast<unsigned char>(*right++);

            if (std::tolower(l) != std::tolower(r))
                return false;
        }

        return *left == '\0' && *right == '\0';
    }

    std::size_t GetEncodedProfileKeyBytes(
        const ProfileIdentity& profile) noexcept
    {
        // Preserve the existing section key for every native name that fits the
        // old eight-byte implementation. Only 9-12 character names expand to
        // the full 12-byte key, which fixes collisions without resetting masks
        // for established short-name profiles.
        return std::strlen(profile.name) <= kLegacyProfileNameBytes
            ? kLegacyProfileNameBytes
            : kProfileNameBytes;
    }

    void EncodeProfileKeyHex(
        const std::uint8_t key[kProfileNameBytes],
        std::size_t keyBytes,
        char* output) noexcept
    {
        static constexpr char kHex[] = "0123456789ABCDEF";

        for (std::size_t i = 0; i < keyBytes; ++i)
        {
            output[i * 2u] = kHex[(key[i] >> 4u) & 0x0Fu];
            output[(i * 2u) + 1u] = kHex[key[i] & 0x0Fu];
        }

        output[keyBytes * 2u] = '\0';
    }

    bool BuildProfileSectionName(
        const ProfileIdentity& profile,
        char* output,
        std::size_t capacity) noexcept
    {
        char keyHex[(kProfileNameBytes * 2u) + 1u]{};
        EncodeProfileKeyHex(
            profile.key,
            GetEncodedProfileKeyBytes(profile),
            keyHex);

        const int written = std::snprintf(
            output,
            capacity,
            "Profile_%s",
            keyHex);

        return written > 0 &&
            static_cast<std::size_t>(written) < capacity;
    }

    bool ResolveStoragePaths() noexcept
    {
        // Re-evaluate after a previous failure. This keeps the storage layer
        // fail-closed without making one early/missing INI permanently sticky.
        if (g_PathsReady)
            return true;

        g_ReplayMoviesCustomSaveDirectoryEnabled = 0;

        HMODULE thisModule = nullptr;
        HMODULE executable = GetModuleHandleA(nullptr);
        if (!GetThisModule(thisModule) || executable == nullptr ||
            !GetDirectoryFromModule(
                thisModule,
                g_ModuleDirectory,
                sizeof(g_ModuleDirectory)) ||
            !GetDirectoryFromModule(
                executable,
                g_GameDirectory,
                sizeof(g_GameDirectory)) ||
            !JoinPath(
                g_ReplayMoviesProgressPath,
                sizeof(g_ReplayMoviesProgressPath),
                g_ModuleDirectory,
                "ReplayMoviesProgress.ini"))
        {
            SetStatus(Status::ModulePathUnavailable);
            return false;
        }

        char candidate[kPathCapacity]{};
        if (JoinPath(
            candidate,
            sizeof(candidate),
            g_ModuleDirectory,
            "NFSUnderground.WidescreenFix.ini") &&
            FileExists(candidate))
        {
            CopyString(
                g_WidescreenFixIniPath,
                sizeof(g_WidescreenFixIniPath),
                candidate);
        }
        else
        {
            char scriptsDirectory[kPathCapacity]{};
            if (!JoinPath(
                scriptsDirectory,
                sizeof(scriptsDirectory),
                g_GameDirectory,
                "scripts") ||
                !JoinPath(
                    candidate,
                    sizeof(candidate),
                    scriptsDirectory,
                    "NFSUnderground.WidescreenFix.ini") ||
                !FileExists(candidate) ||
                !CopyString(
                    g_WidescreenFixIniPath,
                    sizeof(g_WidescreenFixIniPath),
                    candidate))
            {
                SetStatus(Status::WidescreenFixIniNotFound);
                return false;
            }
        }

        GetPrivateProfileStringA(
            "MISC",
            "CustomUserFilesDirectoryInGameDir",
            "0",
            g_CustomUserFilesDirectory,
            static_cast<DWORD>(sizeof(g_CustomUserFilesDirectory)),
            g_WidescreenFixIniPath);
        TrimIniValue(g_CustomUserFilesDirectory);

        if (g_CustomUserFilesDirectory[0] == '\0' ||
            EqualsIgnoreCase(g_CustomUserFilesDirectory, "0"))
        {
            SetStatus(Status::CustomUserFilesDirectoryDisabled);
            return false;
        }

        // Keep ThirteenAG's custom-user-files setting as the User-mode
        // installation contract, but profile identity itself now comes from
        // Underground's debugger-proven runtime active-name buffer.
        char customRoot[kPathCapacity]{};
        if (!JoinPath(
            customRoot,
            sizeof(customRoot),
            g_GameDirectory,
            g_CustomUserFilesDirectory))
        {
            SetStatus(Status::ModulePathUnavailable);
            return false;
        }

        g_ReplayMoviesCustomSaveDirectoryEnabled = 1;
        g_PathsReady = true;
        SetStatus(Status::Ready);
        return true;
    }

    bool ReadActiveProfile(ProfileIdentity& profile) noexcept
    {
        std::memset(&profile, 0, sizeof(profile));

        // Runtime proof: native profile creation publishes the active name to
        // 007588C4 with a 0x0D-byte bounded copy, and both profile-create and
        // profile-load completion preserve full names such as LONGNAMETEST.
        const auto* rawName = reinterpret_cast<const unsigned char*>(
            Addr::NativeActiveProfileName);

        std::size_t nameLength = 0;
        while (nameLength < kProfileNameBytes && rawName[nameLength] != 0)
            ++nameLength;

        if (nameLength == 0)
        {
            SetStatus(Status::ActiveProfileUnavailable);
            return false;
        }

        std::memcpy(profile.key, rawName, nameLength);
        std::memcpy(profile.name, rawName, nameLength);
        profile.name[nameLength] = '\0';
        return true;
    }

    bool GetPublishedProfileIdentity(ProfileIdentity& profile) noexcept
    {
        std::memset(&profile, 0, sizeof(profile));

        if (g_ReplayMoviesCurrentProfileName[0] == '\0')
            return false;

        std::memcpy(profile.key, g_CurrentProfileKey, sizeof(profile.key));
        std::memcpy(
            profile.name,
            g_ReplayMoviesCurrentProfileName,
            sizeof(profile.name));
        profile.name[kProfileNameBytes] = '\0';
        return true;
    }

    bool LoadMaskForProfile(
        const ProfileIdentity& profile,
        std::uint32_t& mask,
        bool ensureRecord) noexcept
    {
        mask = 0;

        if (!ResolveStoragePaths())
            return false;

        char section[40]{};
        if (!BuildProfileSectionName(profile, section, sizeof(section)))
        {
            SetStatus(Status::ProgressReadFailed);
            return false;
        }

        char maskText[32]{};
        GetPrivateProfileStringA(
            section,
            "SeenMask",
            "",
            maskText,
            static_cast<DWORD>(sizeof(maskText)),
            g_ReplayMoviesProgressPath);
        TrimIniValue(maskText);

        if (maskText[0] == '\0')
        {
            mask = 0;

            if (ensureRecord)
            {
                if (!WritePrivateProfileStringA(
                    section,
                    "Name",
                    profile.name,
                    g_ReplayMoviesProgressPath) ||
                    !WritePrivateProfileStringA(
                        section,
                        "SeenMask",
                        "00000000",
                        g_ReplayMoviesProgressPath))
                {
                    SetStatus(Status::ProgressWriteFailed);
                    return false;
                }

                ++g_ReplayMoviesProgressWrites;
            }

            ++g_ReplayMoviesProgressLoads;
            return true;
        }

        char* end = nullptr;
        const unsigned long parsed = std::strtoul(maskText, &end, 16);
        if (end == maskText)
        {
            SetStatus(Status::ProgressReadFailed);
            return false;
        }

        while (*end != '\0' &&
            std::isspace(static_cast<unsigned char>(*end)) != 0)
        {
            ++end;
        }

        if (*end != '\0')
        {
            SetStatus(Status::ProgressReadFailed);
            return false;
        }

        mask = static_cast<std::uint32_t>(parsed) & kAllCareerMovieBits;
        ++g_ReplayMoviesProgressLoads;
        return true;
    }

    bool PersistMaskForProfile(
        const ProfileIdentity& profile,
        std::uint32_t mask) noexcept
    {
        if (!ResolveStoragePaths())
            return false;

        char section[40]{};
        if (!BuildProfileSectionName(profile, section, sizeof(section)))
            return false;

        char maskText[16]{};
        const int written = std::snprintf(
            maskText,
            sizeof(maskText),
            "%08X",
            static_cast<unsigned int>(mask & kAllCareerMovieBits));

        if (written <= 0 ||
            static_cast<std::size_t>(written) >= sizeof(maskText))
        {
            return false;
        }

        if (!WritePrivateProfileStringA(
            section,
            "Name",
            profile.name,
            g_ReplayMoviesProgressPath) ||
            !WritePrivateProfileStringA(
                section,
                "SeenMask",
                maskText,
                g_ReplayMoviesProgressPath))
        {
            return false;
        }

        ++g_ReplayMoviesProgressWrites;
        return true;
    }

    void PublishCurrentProfile(
        const ProfileIdentity& profile,
        std::uint32_t mask) noexcept
    {
        std::memcpy(
            g_CurrentProfileKey,
            profile.key,
            sizeof(g_CurrentProfileKey));

        std::memset(
            g_ReplayMoviesCurrentProfileName,
            0,
            sizeof(g_ReplayMoviesCurrentProfileName));
        std::memcpy(
            g_ReplayMoviesCurrentProfileName,
            profile.name,
            std::strlen(profile.name));

        std::memset(
            g_ReplayMoviesCurrentProfileKeyHex,
            0,
            sizeof(g_ReplayMoviesCurrentProfileKeyHex));
        EncodeProfileKeyHex(
            g_CurrentProfileKey,
            GetEncodedProfileKeyBytes(profile),
            g_ReplayMoviesCurrentProfileKeyHex);

        g_ReplayMoviesCurrentSeenMask =
            static_cast<unsigned long>(mask & kAllCareerMovieBits);
    }

    bool ActivateCurrentProfileInternal(bool ensureRecord) noexcept
    {
        ++g_ReplayMoviesProfileRefreshes;

        ProfileIdentity profile{};
        if (!ReadActiveProfile(profile))
            return false;

        std::uint32_t mask = 0;
        if (!LoadMaskForProfile(profile, mask, ensureRecord))
            return false;

        PublishCurrentProfile(profile, mask);
        SetStatus(Status::Ready);
        ++g_ReplayMoviesProfileRefreshSuccesses;
        return true;
    }

}

namespace ReplayMoviesProfileStorage
{
    bool ActivateCurrentProfile() noexcept
    {
        // Only the native create/load completion hooks call this.  A missing
        // section is initialized immediately with SeenMask=00000000.
        return ActivateCurrentProfileInternal(true);
    }

    bool CaptureCurrentProfile(ProfileIdentity& output) noexcept
    {
        // Movie start must never discover/create/refresh profiles.  It snapshots
        // only the identity already established by native create/load lifecycle.
        return GetPublishedProfileIdentity(output);
    }

    bool MarkMovieSeenForProfile(
        const ProfileIdentity& profile,
        std::size_t canonicalMovieIndex) noexcept
    {
        if (canonicalMovieIndex >= ReplayMoviesCatalog::kCareerMovieCount ||
            profile.name[0] == '\0')
        {
            return false;
        }

        std::uint32_t currentMask = 0;
        if (!LoadMaskForProfile(profile, currentMask, false))
            return false;

        const std::uint32_t bit =
            1u << static_cast<unsigned int>(canonicalMovieIndex);
        const std::uint32_t updatedMask = currentMask | bit;

        if (updatedMask != currentMask &&
            !PersistMaskForProfile(profile, updatedMask))
        {
            SetStatus(Status::ProgressWriteFailed);
            return false;
        }

        // If the captured transaction belongs to the currently published
        // profile, keep debugger/UI telemetry coherent without re-reading disk.
        if (std::memcmp(
            profile.key,
            g_CurrentProfileKey,
            sizeof(g_CurrentProfileKey)) == 0)
        {
            g_ReplayMoviesCurrentSeenMask =
                static_cast<unsigned long>(updatedMask & kAllCareerMovieBits);
        }

        SetStatus(Status::Ready);
        return true;
    }

    bool HasCurrentProfile() noexcept
    {
        return g_ReplayMoviesCurrentProfileName[0] != '\0';
    }

    const char* GetCurrentProfileName() noexcept
    {
        return g_ReplayMoviesCurrentProfileName;
    }

    std::uint32_t GetCurrentSeenMask() noexcept
    {
        return static_cast<std::uint32_t>(
            g_ReplayMoviesCurrentSeenMask & kAllCareerMovieBits);
    }

    bool IsMovieSeen(std::size_t canonicalMovieIndex) noexcept
    {
        if (canonicalMovieIndex >= ReplayMoviesCatalog::kCareerMovieCount)
            return false;

        return (GetCurrentSeenMask() &
            (1u << static_cast<unsigned int>(canonicalMovieIndex))) != 0;
    }
}