#include "CapacityHook.h"
#include "pch.h"

#include "Addresses.h"
#include "MemoryPatch.h"

#include <Windows.h>
#include <cstdint>
#include <cstring>

volatile unsigned long g_MuMainCapacityHits = 0;

static DWORD g_MuMainCapacityReturn =
static_cast<DWORD>(Addr::MuMainCapacityReturn);

__declspec(naked) static void MuMainCapacityHook()
{
    __asm
    {
        // Original instructions at 00501415/00501418
        // do not alter EFLAGS. Preserve incoming flags because
        // our qualification CMPs do.

        pushfd

        // ORIGINAL @ 00501415
        mov edi, dword ptr[ebx + 08h]

        // "Butn"
        cmp eax, 06E747542h
        jne NotMuMain

        // Native authored MU_Main count must be exactly six.
        cmp edi, 6
        jne NotMuMain

        // ESI was loaded by:
        // 00501412 mov esi,[ebp]
        //
        // Runtime+10 == MU_Main_PC hash.
        cmp dword ptr[esi + 10h], 05B543F80h
        jne NotMuMain

        // Proven semantic modification.
        mov edi, 7

        inc dword ptr[g_MuMainCapacityHits]

        NotMuMain:

        popfd

            // ORIGINAL @ 00501418
            mov dword ptr[ebp + 48h], edi

            // Continue at 0050141B.
            jmp dword ptr[g_MuMainCapacityReturn]
    }
}

bool InstallCapacityHook()
{
    static const std::uint8_t expected[6] =
    {
        0x8B, 0x7B, 0x08, // mov edi,[ebx+08]
        0x89, 0x7D, 0x48  // mov [ebp+48],edi
    };

    const auto* current =
        reinterpret_cast<const std::uint8_t*>(
            Addr::MuMainCapacityHook);

    if (std::memcmp(
        current,
        expected,
        sizeof(expected)) != 0)
    {
        return false;
    }

    return WriteAbsoluteJump6(
        Addr::MuMainCapacityHook,
        reinterpret_cast<const void*>(
            &MuMainCapacityHook));
}