#include "pch.h"
#include "ReplayMoviesConfig.h"

#include <Windows.h>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C"
{
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesAccessMode =
        static_cast<unsigned long>(ReplayMoviesConfig::AccessMode::User);

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesConfigRefreshes = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesConfigInvalidModeValues = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesConfigIniFound = 0;
}

namespace
{
    constexpr const char* kReplayMoviesIniName = "ReplayMovies.ini";
    constexpr const char* kReplayMoviesIniSection = "ReplayMovies";
    constexpr const char* kReplayMoviesIniModeKey = "Mode";
    constexpr const char* kReplayMoviesDefaultMode = "User";

    HMODULE g_ModuleHandle = nullptr;
    ReplayMoviesConfig::AccessMode g_AccessMode =
        ReplayMoviesConfig::AccessMode::User;

    bool BuildReplayMoviesIniPath(char* output, std::size_t capacity) noexcept
    {
        if (output == nullptr || capacity == 0 || g_ModuleHandle == nullptr)
            return false;

        char modulePath[MAX_PATH]{};
        const DWORD length = GetModuleFileNameA(
            g_ModuleHandle,
            modulePath,
            static_cast<DWORD>(sizeof(modulePath)));

        if (length == 0 || length >= sizeof(modulePath))
            return false;

        char* lastSlash = std::strrchr(modulePath, '\\');
        if (lastSlash == nullptr)
            lastSlash = std::strrchr(modulePath, '/');

        if (lastSlash == nullptr)
            return false;

        const std::size_t directoryLength =
            static_cast<std::size_t>(lastSlash - modulePath) + 1u;
        const std::size_t iniNameLength = std::strlen(kReplayMoviesIniName);

        if (directoryLength + iniNameLength + 1u > capacity)
            return false;

        std::memcpy(output, modulePath, directoryLength);
        std::memcpy(
            output + directoryLength,
            kReplayMoviesIniName,
            iniNameLength + 1u);

        return true;
    }

    void TrimIniValue(char* value) noexcept
    {
        if (value == nullptr)
            return;

        // Accept convenient inline comments in the shipped INI.
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

    void PublishAccessMode(ReplayMoviesConfig::AccessMode mode) noexcept
    {
        g_AccessMode = mode;
        g_ReplayMoviesAccessMode = static_cast<unsigned long>(mode);
    }
}

namespace ReplayMoviesConfig
{
    void Initialize(HMODULE moduleHandle) noexcept
    {
        g_ModuleHandle = moduleHandle;
        PublishAccessMode(AccessMode::User);
        g_ReplayMoviesConfigRefreshes = 0;
        g_ReplayMoviesConfigInvalidModeValues = 0;
        g_ReplayMoviesConfigIniFound = 0;
    }

    bool Refresh() noexcept
    {
        ++g_ReplayMoviesConfigRefreshes;

        char iniPath[MAX_PATH]{};
        if (!BuildReplayMoviesIniPath(iniPath, sizeof(iniPath)))
        {
            g_ReplayMoviesConfigIniFound = 0;
            PublishAccessMode(AccessMode::User);
            return false;
        }

        const DWORD attributes = GetFileAttributesA(iniPath);
        g_ReplayMoviesConfigIniFound =
            attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0
            ? 1ul
            : 0ul;

        char modeValue[64]{};
        GetPrivateProfileStringA(
            kReplayMoviesIniSection,
            kReplayMoviesIniModeKey,
            kReplayMoviesDefaultMode,
            modeValue,
            static_cast<DWORD>(sizeof(modeValue)),
            iniPath);

        TrimIniValue(modeValue);

        if (EqualsIgnoreCase(modeValue, "Developer"))
        {
            PublishAccessMode(AccessMode::Developer);
            return true;
        }

        if (EqualsIgnoreCase(modeValue, "User"))
        {
            PublishAccessMode(AccessMode::User);
            return true;
        }

        ++g_ReplayMoviesConfigInvalidModeValues;
        PublishAccessMode(AccessMode::User);
        return true;
    }

    AccessMode GetAccessMode() noexcept
    {
        return g_AccessMode;
    }

    bool IsUserMode() noexcept
    {
        return g_AccessMode == AccessMode::User;
    }

    bool IsDeveloperMode() noexcept
    {
        return g_AccessMode == AccessMode::Developer;
    }
}