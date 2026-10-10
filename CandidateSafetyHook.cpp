#include "CandidateSafetyHook.h"
#include "pch.h"

#include "Addresses.h"
#include "MemoryPatch.h"

#include <Windows.h>
#include <cstdint>
#include <cstring>

volatile unsigned long g_MuMainSafetyHits = 0;

static DWORD g_MuMainCandidateWriteReturn =
static_cast<DWORD>(
    Addr::MuMainCandidateWriteReturn);

__declspec(naked) static void MuMainCandidateWriteHook()
{
    __asm
    {
        // -------------------------------------------------
        // Replay the two overwritten native instructions.
        // -------------------------------------------------

        mov dword ptr[edx + ecx * 4], eax
        inc dword ptr[ebp + 4Ch]

        // INC changed EFLAGS in the original game too.
        // Preserve that exact post-INC state while our
        // qualification logic runs.
        pushfd

        // We need ECX temporarily. Preserve its original
        // candidate index value.
        push ecx

        // -------------------------------------------------
        // We only care about the completed sixth native
        // candidate insertion.
        //
        // After UndergroundMode:
        // [EBP+4C] == 6
        // -------------------------------------------------

        cmp dword ptr[ebp + 4Ch], 6
        jne SkipSafety

        // The candidate that was just inserted must really
        // be UndergroundMode.
        cmp dword ptr[eax + 10h], 0D7BA6011h
        jne SkipSafety

        // EDX is the current candidate-array base.
        //
        // Verify slot5 is exactly the EAX we just wrote.
        cmp dword ptr[edx + 14h], eax
        jne SkipSafety

        // -------------------------------------------------
        // Qualify the owning runtime.
        // EBP+00 = current SelectorRuntime*
        // -------------------------------------------------

        mov ecx, dword ptr[ebp]

        test ecx, ecx
        je SkipSafety

        // MU_Main_PC
        cmp dword ptr[ecx + 10h], 05B543F80h
        jne SkipSafety

        // Our capacity hook must already have produced
        // a seven-entry native allocation.
        cmp dword ptr[ecx + 88h], 7
        jne SkipSafety

        // -------------------------------------------------
        // SAFETY PUBLICATION
        //
        // candidate[6] = candidate[5]
        //
        // slot5 offset = 5*4 = 0x14
        // slot6 offset = 6*4 = 0x18
        // -------------------------------------------------

        mov dword ptr[edx + 18h], eax

        inc dword ptr[g_MuMainSafetyHits]

        SkipSafety:

        pop ecx
            popfd

            // Original execution continues at:
            // 005019D7 jmp 00501A56
            jmp dword ptr[g_MuMainCandidateWriteReturn]
    }
}

bool InstallCandidateSafetyHook()
{
    static const std::uint8_t expected[6] =
    {
        0x89, 0x04, 0x8A,   // mov [edx+ecx*4],eax
        0xFF, 0x45, 0x4C    // inc dword ptr [ebp+4C]
    };

    const auto* current =
        reinterpret_cast<const std::uint8_t*>(
            Addr::MuMainCandidateWriteHook);

    if (std::memcmp(
        current,
        expected,
        sizeof(expected)) != 0)
    {
        return false;
    }

    return WriteAbsoluteJump6(
        Addr::MuMainCandidateWriteHook,
        reinterpret_cast<const void*>(
            &MuMainCandidateWriteHook));
}