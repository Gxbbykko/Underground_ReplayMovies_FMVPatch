#pragma once
#include <cstdint>

namespace Addr
{
    constexpr std::uintptr_t ImageBase = 0x00400000;

    // Hook begins at:
    //
    // 00501415  mov edi,[ebx+08]
    // 00501418  mov [ebp+48],edi
    //
    // Six bytes total are replayed by our hook.
    constexpr std::uintptr_t MuMainCapacityHook = 0x00501415;
    constexpr std::uintptr_t MuMainCapacityReturn = 0x0050141B;

    constexpr std::uint32_t MuMainHash = 0x5B543F80;
    constexpr std::uint32_t ButnMagic = 0x6E747542;

    constexpr std::uintptr_t MuMainCandidateWriteHook =
        0x005019D1;

    constexpr std::uintptr_t MuMainCandidateWriteReturn =
        0x005019D7;

    constexpr std::uint32_t UndergroundModeHash =
        0xD7BA6011;

    constexpr std::uint32_t QuickRaceHash =
        0xDE511657;

    constexpr std::uint32_t StatisticsHash =
        0xF20E404A;

    constexpr std::uint32_t OnlineHash =
        0x70FBB1E4;

    constexpr std::uint32_t OptionsHash =
        0x95AD096B;

    constexpr std::uint32_t CustomizeHash =
        0xA936C3A2;

    // Native MU_Main Type-5 presentation object used by the
    // UndergroundMode/ReplayMovies boundary.
    constexpr std::uint32_t Highlight01Hash =
        0x6DCB1D8C;

    constexpr std::uint32_t Highlight02Hash =
        0x6DCB1D8D;

    constexpr std::uint32_t Highlight03Hash =
        0x6DCB1D8E;

    constexpr std::uint32_t Highlight04Hash =
        0x6DCB1D8F;

    constexpr std::uint32_t Highlight05Hash =
        0x6DCB1D90;

    constexpr std::uint32_t Highlight06Hash =
        0x6DCB1D91;

    constexpr std::uintptr_t MuMainGenerationHook =
        0x004F1521;

    constexpr std::uintptr_t MuMainGenerationContinue =
        0x004F1527;

    constexpr std::uintptr_t MuMainGenerationNullPath =
        0x004F1545;


    // One-shot post-submenu Main Menu restore hooks.
    //
    // Both sites are immediately after native 004FA140 candidate
    // resolution and BEFORE the native row-transition command machinery.
    // v7.1 restored ReplayMovies too late at 004EFE87, after the native
    // incoming row had already been visually selected.
    //
    // Primary path:
    // 004EFCB5  mov [esp+1C],eax
    // 004EFCB9  mov eax,[esp+20]
    //
    // Eight whole bytes are replaced and replayed by the hook.
    constexpr std::uintptr_t MuMainRestoreCandidatePrimaryHook =
        0x004EFCB5;

    constexpr std::uintptr_t MuMainRestoreCandidatePrimaryReturn =
        0x004EFCBD;

    // Alternate/loop path:
    // 004EFDE2  mov [esp+1C],eax
    // 004EFDE6  jmp 004EFCCB
    //
    // Nine whole bytes are replaced. The hook replays the MOV and
    // returns directly to the original jump target 004EFCCB.
    constexpr std::uintptr_t MuMainRestoreCandidateLoopHook =
        0x004EFDE2;

    constexpr std::uintptr_t MuMainRestoreCandidateLoopReturn =
        0x004EFCCB;

    // Generic frontend current-row commit. The ReplayMovies visual
    // transition hook qualifies MU_Main_PC at runtime before touching
    // any generation-local data.
    //
    // 004EFE87  mov [ebx+80],esi
    constexpr std::uintptr_t MuMainCurrentRowCommitHook =
        0x004EFE87;

    constexpr std::uintptr_t MuMainCurrentRowCommitReturn =
        0x004EFE8D;


    // Native FE_String active-subordinate writer used by the property/
    // transition machinery:
    //
    // 004F1116  mov [eax+54],esi
    // 004F1119  call 004FF030
    //
    // The hook replays both instructions and resumes at 004F111E.
    constexpr std::uintptr_t MuMainActiveNodeWriterHook =
        0x004F1116;

    constexpr std::uintptr_t MuMainActiveNodeWriterReturn =
        0x004F111E;

    constexpr std::uintptr_t NativePostActiveNodeWrite =
        0x004FF030;

    // Native live geometry output writer. During the exact
    // ReplayMovies -> QuickRace transition, hightlight01's tail
    // evaluation briefly attempts to write its authored UndergroundMode
    // Y (-126) into the live presentation payload before QuickRace wins.
    //
    // 004FC374  mov [ebx+04],eax
    // 004FC377  mov [ebx+08],ecx
    //
    // The six-byte hook replays both writes and resumes at 004FC37A.
    constexpr std::uintptr_t LiveGeometryWriterHook =
        0x004FC374;

    constexpr std::uintptr_t LiveGeometryWriterReturn =
        0x004FC37A;

    // Main Menu action dispatcher decision used by ReplayMovies.
    //
    // 004B33FA  cmp eax,95AD096B    ; Options
    // 004B33FF  jne 004B3769       ; unknown-hash fallback
    // 004B3405  ...                 ; native Options route
    //
    // ReplayMovies already reaches this boundary through the native
    // MU_Main_PC notification/Enter action. The hook adds only the
    // missing ReplayMovies hash branch.
    constexpr std::uintptr_t MuMainDispatchHook =
        0x004B33FF;

    constexpr std::uintptr_t MuMainDispatchFallback =
        0x004B3769;

    constexpr std::uintptr_t MuMainOptionsRoute =
        0x004B3405;


    // Native Main Menu "select/restore row by NameHash" worker.
    //
    // Captures(20260830-203614) proved this function scans Runtime+84/+88
    // for a candidate whose +10 NameHash matches ECX, runs the native
    // deselect/select presentation transaction, commits Runtime+80, and
    // returns with `ret 4`.
    //
    // Entry bytes:
    //   004F5ED0  57                 push edi
    //   004F5ED1  68 04 61 74 00     push 00746104
    //
    // The six-byte detour replays both and resumes at 004F5ED6.
    constexpr std::uintptr_t MuMainNativeSelectionHook =
        0x004F5ED0;

    constexpr std::uintptr_t MuMainNativeSelectionReturn =
        0x004F5ED6;

    // Late MU_Main completed-generation fallback boundary inside the same
    // native select/restore-by-NameHash worker. Runtime A/B on 2026-10-04
    // proved that Race -> Quit Game reconstructs a fresh MU_Main, reaches
    // this point with the fully populated seven-entry safety vector, but
    // bypasses the primary 004F1521 generation observer entirely.
    //
    // 004F5EF2  mov eax,[esi+88]   ; candidate count
    //
    // At this point ESI is the resolved runtime. ReplayMovies observes only
    // the exact unbuilt safety state, replays this MOV, and resumes normally.
    constexpr std::uintptr_t MuMainSelectionPreScanHook =
        0x004F5EF2;

    constexpr std::uintptr_t MuMainSelectionPreScanReturn =
        0x004F5EF8;

    // Native Trailers/Cinematics runtime observed at 004D0B55.
    // It owns the generic ten-slot OptionName_10..OptionName_1 vector.
    constexpr std::uint32_t TrailersRuntimeHash =
        0xB73E8C87;

    constexpr std::uintptr_t OptionControlVtable =
        0x006C17D8;

    // Native frontend package switch used by Main Menu and Options.
    constexpr std::uintptr_t FEngSwitchPackages =
        0x004F6360;

    // Native Options selector state. GameTrailers/Cinematics is ID A.
    constexpr std::uintptr_t OptionsSelectorId =
        0x00735E20;

    constexpr std::uint32_t GameTrailersSelectorId =
        0x0000000A;

    // Main Menu action state used by the native Options route. A normal
    // Main Menu -> Options -> GameTrailers path enters the movie list with
    // this value already set to 0x10. The direct ReplayMovies route mirrors
    // that native internal context without displaying the Options selector.
    constexpr std::uintptr_t MainMenuActionState =
        0x00777B4C;

    constexpr std::uint32_t OptionsActionState =
        0x00000010;

    // Static executable string: "MU_Options_PC.fng". Entering this
    // package with selector ID A reproduces the native Trailers movie-list
    // mode directly, bypassing MU_OptionsSelector_PC.fng.
    constexpr std::uintptr_t MuOptionsPackageName =
        0x006C2C10;

    // Native MU_Options_PC Back/Exit route.
    //
    // 004CF335  mov eax,[edi+0C]
    // 004CF338  push eax
    // 004CF339  mov edi,006C25E0    ; MU_OptionsSelector_PC.fng
    // 004CF33E  call 004F6360
    // 004CF343  add esp,4
    //
    // The six-byte hook replaces only `push eax ; mov edi,imm32` and
    // resumes at the untouched native call. ReplayMovies mode substitutes
    // MU_Main_PC.fng as the destination; all normal Options modes replay
    // the original selector destination unchanged.
    constexpr std::uintptr_t MuOptionsBackRouteHook =
        0x004CF338;

    constexpr std::uintptr_t MuOptionsBackRouteReturn =
        0x004CF33E;

    constexpr std::uintptr_t MuOptionsSelectorPackageName =
        0x006C25E0;

    constexpr std::uintptr_t MuMainPackageName =
        0x006C2B6C;

    // Native Trailers mode installs exactly one control callback at
    // MenuScreen+40. v8 retains that native callback unchanged as the
    // current Developer-catalog bootstrap route into TrailerMovie.fng.
    constexpr std::uintptr_t NativeTrailersCallback =
        0x004D0B20;

    // TrailerMovie.fng type-7 Video callback. The first seven bytes are:
    //   004F5860  mov eax,[esp+04]
    //   004F5864  mov ecx,[eax+18]
    //
    // ReplayMovies v8 uses a fail-closed hook here only while its submenu
    // mode is active. The hook substitutes Video+24 with the selected
    // Developer-catalog .mad basename, then replays the two native loads.
    constexpr std::uintptr_t TrailerMovieVideoCallbackHook =
        0x004F5860;

    constexpr std::uintptr_t TrailerMovieVideoCallbackReturn =
        0x004F5867;

    constexpr std::uint32_t TrailerMovieVideoHash =
        0x03E844D6;

    // Debugger-proven native profile lifecycle.  Underground publishes the
    // complete active profile name here; creation uses a 0x0D-byte bounded
    // copy (12 visible characters + terminator), and profile-load completion
    // preserved both PROVOKETEST and LONGNAMETEST without eight-byte truncation.
    constexpr std::uintptr_t NativeActiveProfileName =
        0x007588C4;

    // New-profile creation has two 004AA4D0 calls.  The earlier 0041CF70
    // call only publishes the provisional default name "Player 1".  The
    // debugger-proven real entered-name call is 0041CFDE; after its native
    // return, 0041CFE3 has the complete selected profile name.  Hook only
    // that five-byte CALL so the native callee and stack cleanup remain intact.
    constexpr std::uintptr_t NativeProfileCreatePublishCall =
        0x0041CFDE;

    constexpr std::uintptr_t NativeProfileCreatePublishOriginal =
        0x004AA4D0;

    constexpr std::uintptr_t NativeProfileCreateComplete =
        0x0041CFE3;

    // Existing-profile load calls the native 004AB200 loader and returns to
    // 0041DB34.  Runtime proof at 0041DB34 showed the full active name.
    constexpr std::uintptr_t NativeProfileLoadCall =
        0x0041DB2F;

    constexpr std::uintptr_t NativeProfileLoadOriginal =
        0x004AB200;

    constexpr std::uintptr_t NativeProfileLoadComplete =
        0x0041DB34;

    // Common native .ugd writer shared by manual save and Autosave.
    // Entry bytes proved in x32dbg:
    //   0041D8A0  mov edx,[esp+04]
    //   0041D8A4  sub esp,00000134
    // Both success exits return EAX=1 with `ret 8`; the lifecycle hook runs
    // its ReplayMovies checkpoint only after that successful native return.
    constexpr std::uintptr_t NativeProfileWriterHook =
        0x0041D8A0;

    constexpr std::uintptr_t NativeProfileWriterContinue =
        0x0041D8AA;

    // Native StartMovie observer point inside 005A4A50.
    //
    // FMVFix/XboxPort owns 005A4ABD-005A4AC4 and returns to 005A4AC5
    // after reproducing the native ECX/EDX argument loads. Runtime proof
    // at 005A4AC5 confirmed the original StartMovie stack contract is still
    // directly addressable and the native register state is intact:
    //   [ESP+04] = resolved MOVIES\\<name>.mad path
    //   [ESP+08] = activation mode
    //   [ESP+0C] = movie metadata/id
    //   EAX      = newly allocated native movie object
    //   ECX      = movie metadata/id
    //   EDX      = activation mode
    //
    // Exact overwritten instructions at the cooperative boundary:
    //   005A4AC5  push esi
    //   005A4AC6  mov [eax],ecx
    //   005A4AC8  mov ecx,[esp+08]
    //
    // ReplayMovies observes the path before replaying those seven bytes,
    // then resumes at the untouched native 005A4ACC push edx.
    constexpr std::uintptr_t NativeMovieStartObserveHook =
        0x005A4AC5;

    constexpr std::uintptr_t NativeMovieStartObserveReturn =
        0x005A4ACC;

    // Proven native movie teardown/ownership clear:
    //   005A4C1F  mov dword ptr [00733F6C],0
    //   005A4C29  call 00671102
    //
    // The progression recorder commits its pending profile-bound movie bit
    // at this boundary, then replays the exact native clear.
    constexpr std::uintptr_t NativeMovieTeardownHook =
        0x005A4C1F;

    constexpr std::uintptr_t NativeMovieTeardownReturn =
        0x005A4C29;

    // Debugger-proven alternate movie ownership clear used when the player
    // skips a Career FMV:
    //   005A4CAF  mov dword ptr [00733F6C],0
    //   005A4CB9  ... skip-path continuation
    //
    // ReplayMovies must commit the same pending profile-bound movie bit here
    // because the skip path does not execute the normal 005A4C1F teardown.
    constexpr std::uintptr_t NativeMovieSkipTeardownHook =
        0x005A4CAF;

    constexpr std::uintptr_t NativeMovieSkipTeardownReturn =
        0x005A4CB9;

    constexpr std::uintptr_t NativeActiveMovieObject =
        0x00733F6C;

    // Main Menu row-description presentation tail.
    //
    // 004B3758  mov edi,[ebp+40]       ; Description FE_String
    // 004B375B  push ebx
    // 004B375C  push 006CBF28          ; "FadeIn"
    // 004B3761  call 004F6B70
    //
    // ReplayMovies inserts its custom description after the native
    // row-dependent binding has been chosen and immediately before FadeIn.
    constexpr std::uintptr_t MuMainDescriptionFadeHook =
        0x004B3758;

    constexpr std::uintptr_t MuMainDescriptionFadeReturn =
        0x004B3761;

    // Object-targeted native printf/text worker. EAX = FE_String*, first
    // explicit stack argument = format/text.
    constexpr std::uintptr_t FEngPrintfObj =
        0x004F68A0;

    constexpr std::uintptr_t NativeCloneFEString =
        0x005009A0;

    constexpr std::uintptr_t NativeAttachFEObject =
        0x00500820;

    constexpr std::uint32_t ReplayMoviesHash =
        0xA1E9FD7F;

    constexpr std::uint32_t ReplayMoviesCaptionHash =
        0x520B331F;
}