#include "MemoryPatch.h"
#include "pch.h"

#include <Windows.h>
#include <cstdint>
#include <cstring>

bool WriteAbsoluteJump6(
    std::uintptr_t source,
    const void* destination)
{
    DWORD oldProtect = 0;

    auto* patch = reinterpret_cast<std::uint8_t*>(source);

    if (!VirtualProtect(
        patch,
        6,
        PAGE_EXECUTE_READWRITE,
        &oldProtect))
    {
        return false;
    }

    const std::uint32_t target =
        static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(destination));

    patch[0] = 0x68; // push imm32

    std::memcpy(
        patch + 1,
        &target,
        sizeof(target));

    patch[5] = 0xC3; // ret

    FlushInstructionCache(
        GetCurrentProcess(),
        patch,
        6);

    DWORD ignored = 0;

    VirtualProtect(
        patch,
        6,
        oldProtect,
        &ignored);

    return true;
}

bool WriteAbsoluteJumpPadded(
    std::uintptr_t source,
    const void* destination,
    std::size_t patchLength)
{
    if (patchLength < 6)
        return false;

    DWORD oldProtect = 0;
    auto* patch = reinterpret_cast<std::uint8_t*>(source);

    if (!VirtualProtect(
        patch,
        patchLength,
        PAGE_EXECUTE_READWRITE,
        &oldProtect))
    {
        return false;
    }

    const std::uint32_t target =
        static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(destination));

    patch[0] = 0x68; // push imm32
    std::memcpy(
        patch + 1,
        &target,
        sizeof(target));
    patch[5] = 0xC3; // ret

    for (std::size_t i = 6; i < patchLength; ++i)
        patch[i] = 0x90;

    FlushInstructionCache(
        GetCurrentProcess(),
        patch,
        patchLength);

    DWORD ignored = 0;
    VirtualProtect(
        patch,
        patchLength,
        oldProtect,
        &ignored);

    return true;
}