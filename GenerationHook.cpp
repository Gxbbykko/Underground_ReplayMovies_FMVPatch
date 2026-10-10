// v11.4 DEBUGGER-PROVEN MU_MAIN BYPASS FALLBACK - 2026-10-04
#include "GenerationHook.h"
#include "pch.h"
#include "ReplayMoviesBuilder.h"
#include "ReplayMoviesCatalog.h"
#include "ReplayMoviesConfig.h"
#include "ReplayMoviesProfileStorage.h"

#include "Addresses.h"
#include "MemoryPatch.h"

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>


extern "C"
{
    __declspec(dllexport)
        volatile unsigned long g_MuMainGenerationHits = 0;

    __declspec(dllexport)
        volatile std::uintptr_t g_LastMuMainRuntime = 0;

    __declspec(dllexport)
        volatile std::uintptr_t g_LastMuMainVector = 0;

    __declspec(dllexport)
        volatile std::uintptr_t g_LastUndergroundMode = 0;

    // v7 prototype telemetry. Incremented only when the native MU_Main
    // action dispatcher delivers ReplayMovies to our added hash branch.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesDispatchHits = 0;

    // Set before switching directly into native Trailers/Cinematics mode.
    // Kept for the next milestone, where Back will be specialized to
    // return to MU_Main instead of MU_OptionsSelector.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesSubmenuActive = 0;

    // Incremented only when MU_Options_PC's native Back route is
    // specialized from ReplayMovies mode to MU_Main_PC.fng.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesExitHits = 0;

    // One-shot native restore request. Back from ReplayMovies remembers that
    // ReplayMovies was the parent Main Menu selection. During the rebuilt
    // MU_Main generation, the upstream native 004FA140 candidate pipeline
    // is redirected to that generation's ReplayMovies candidate BEFORE
    // native deselect/select/highlight commands run. The request is only
    // consumed later when 004EFE87 confirms ReplayMovies reached the
    // native current-row commit.
    __declspec(dllexport)
        volatile unsigned long g_PendingReplayMoviesMainMenuRestore = 0;

    // v7.2 restore telemetry. The substitution counter increments when
    // one of the native 004FA140 candidate-writer sites is redirected to
    // this generation's ReplayMovies object. CommitSuccess proves that
    // the later native 004EFE87 commit arrived with ReplayMovies already
    // selected by the native transition pipeline. CommitMiss is fail-safe
    // telemetry only; the request is expired rather than hijacking a
    // later user navigation.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesRestoreCandidateSubstitutions = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesRestoreCommitSuccess = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesRestoreCommitMiss = 0;

    // Historical v7.4 telemetry retained for ABI/debugger continuity.
    // v8 no longer neutralizes NASCAR into an empty list.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesNascarNeutralizations = 0;

    // v8 Developer-catalog bootstrap telemetry.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCatalogTitleUpdates = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesMovieIdentityOverrides = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesSelectedDeveloperMovieIndex = 0;

    // Multi-row Developer catalog telemetry/state. v11.1 uses one physical
    // 17-row page, so page remains zero and each movie owns one selector.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesDeveloperCatalogPage = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCatalogConfiguredSlots = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCatalogSelectionUpdates = 0;

    // Phase 2C User-catalog projection telemetry. The projection is built only
    // from the already-published per-profile SeenMask; no profile discovery or
    // filesystem work occurs in the frontend path.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesUserProjectionApplies = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesUserProjectionGuardFallbacks = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesUserVisibleCount = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesUserSeenMask = 0;

    __declspec(dllexport)
        volatile std::uintptr_t g_ReplayMoviesUserProjectionRuntime = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesCatalogPageTransitions = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesBorrowedRouteStateSaves = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesBorrowedRouteStateRestores = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesDescriptionUpdates = 0;

    // v7.4 post-return native restoration telemetry.
    __declspec(dllexport)
        volatile std::uintptr_t g_ReplayMoviesNativeRestoreContext = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesNativeRestoreContextCaptures = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesNativeRestoreAttempts = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesNativeRestoreSuccess = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesNativeRestoreFailures = 0;

    // v11.0 one-page prototype telemetry. These identify the three dedicated
    // row-11 clones created inside the ReplayMovies-owned MU_Options_PC
    // generation. "Success" is only a construction diagnostic; end-to-end
    // implementation promotion still requires runtime validation in game.
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesSynthetic11BuildAttempts = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesSynthetic11BuildSuccesses = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesSynthetic11BuildStage = 0;

    __declspec(dllexport)
        volatile std::uintptr_t g_ReplayMoviesSynthetic11OptionName = 0;

    __declspec(dllexport)
        volatile std::uintptr_t g_ReplayMoviesSynthetic11OptionText = 0;

    __declspec(dllexport)
        volatile std::uintptr_t g_ReplayMoviesSynthetic11Highlight = 0;
}


static DWORD g_MuMainNativeSelectionReturn =
static_cast<DWORD>(
    Addr::MuMainNativeSelectionReturn);

static DWORD g_MuMainSelectionPreScanReturn =
static_cast<DWORD>(
    Addr::MuMainSelectionPreScanReturn);

static DWORD g_NativeMuMainSelectionByHash =
static_cast<DWORD>(
    Addr::MuMainNativeSelectionHook);

static DWORD g_MuMainGenerationContinue =
static_cast<DWORD>(
    Addr::MuMainGenerationContinue);

static DWORD g_MuMainGenerationNullPath =
static_cast<DWORD>(
    Addr::MuMainGenerationNullPath);

static DWORD g_MuMainCurrentRowCommitReturn =
static_cast<DWORD>(
    Addr::MuMainCurrentRowCommitReturn);

static DWORD g_MuMainRestoreCandidatePrimaryReturn =
static_cast<DWORD>(
    Addr::MuMainRestoreCandidatePrimaryReturn);

static DWORD g_MuMainRestoreCandidateLoopReturn =
static_cast<DWORD>(
    Addr::MuMainRestoreCandidateLoopReturn);


static DWORD g_MuMainActiveNodeWriterReturn =
static_cast<DWORD>(
    Addr::MuMainActiveNodeWriterReturn);

static DWORD g_NativePostActiveNodeWrite =
static_cast<DWORD>(
    Addr::NativePostActiveNodeWrite);

static DWORD g_LiveGeometryWriterReturn =
static_cast<DWORD>(
    Addr::LiveGeometryWriterReturn);

static DWORD g_MuMainDispatchFallback =
static_cast<DWORD>(
    Addr::MuMainDispatchFallback);

static DWORD g_MuMainOptionsRoute =
static_cast<DWORD>(
    Addr::MuMainOptionsRoute);

static DWORD g_FEngSwitchPackages =
static_cast<DWORD>(
    Addr::FEngSwitchPackages);

static DWORD g_TrailerMovieVideoCallbackReturn =
static_cast<DWORD>(
    Addr::TrailerMovieVideoCallbackReturn);

static DWORD g_MuOptionsBackRouteReturn =
static_cast<DWORD>(
    Addr::MuOptionsBackRouteReturn);

static DWORD g_MuOptionsSelectorPackageName =
static_cast<DWORD>(
    Addr::MuOptionsSelectorPackageName);

static DWORD g_MuMainPackageName =
static_cast<DWORD>(
    Addr::MuMainPackageName);

static DWORD g_MuMainDescriptionFadeReturn =
static_cast<DWORD>(
    Addr::MuMainDescriptionFadeReturn);

static DWORD g_FEngPrintfObj =
static_cast<DWORD>(
    Addr::FEngPrintfObj);

static const char g_ReplayMoviesDescription[] =
"Replay your favourite movies";

// The current v8 bootstrap still borrows the retail MU_Options_PC Trailers
// shell as a construction template. Preserve the two native Options globals
// before entering that shell and restore them when ReplayMovies exits, so a
// later ordinary Main Menu -> Options visit does not inherit GameTrailers.
static volatile std::uint32_t g_SavedMainMenuActionState = 0;
static volatile std::uint32_t g_SavedOptionsSelectorId = 0;
static volatile unsigned long g_ReplayMoviesBorrowedRouteStateSaved = 0;

// v11.1 final one-page Developer catalog architecture. Underground authors ten
// physical Trailers controls. ReplayMovies keeps those ten and appends seven
// independently owned native clones, producing seventeen real selector rows in
// one list. The debugger proved native bidirectional traversal across all 17
// after the synthetic selector payloads inherited the complete row-11 0x54
// payload and retained only their row-specific compact Y/height.
static constexpr std::size_t kReplayMoviesAuthoredCatalogSlotCount = 10;
static constexpr std::size_t kReplayMoviesSynthetic11CatalogSlotCount = 11;
static constexpr std::size_t kReplayMoviesNativeCatalogSlotCount = 17;
static constexpr std::size_t kReplayMoviesCatalogPageCount = 1;
static constexpr std::size_t kReplayMoviesCatalogPageOffsets[
    kReplayMoviesCatalogPageCount] = { 0 };

    static constexpr float kReplayMoviesSyntheticRowPitch = 30.0f;
    static constexpr std::uint32_t kReplayMoviesOptionName11Hash = 0x787C1E5A;
    static constexpr std::uint32_t kReplayMoviesOptionText11Hash = 0x5E8FB1BF;
    static constexpr std::uint32_t kReplayMoviesHighlight10Hash = 0x203C4AF7;
    static constexpr std::uint32_t kReplayMoviesHighlight11Hash = 0x203C4AF8;

    static constexpr const char* kReplayMoviesOptionName10 = "OptionName_10";
    static constexpr const char* kReplayMoviesOptionName11 = "OptionName_11";
    static constexpr const char* kReplayMoviesOptionText10 = "OptionNametext_10";
    static constexpr const char* kReplayMoviesOptionText11 = "OptionNametext_11";
    static constexpr const char* kReplayMoviesHighlight10 = "Highlight_10";
    static constexpr const char* kReplayMoviesHighlight11 = "Highlight_11";

    static const char* const kReplayMoviesOptionControlNames[
        kReplayMoviesNativeCatalogSlotCount] =
        {
            "OptionName_1", "OptionName_2", "OptionName_3", "OptionName_4",
            "OptionName_5", "OptionName_6", "OptionName_7", "OptionName_8",
            "OptionName_9", "OptionName_10", "OptionName_11", "OptionName_12",
            "OptionName_13", "OptionName_14", "OptionName_15", "OptionName_16",
            "OptionName_17",
        };

        static constexpr std::uint32_t kReplayMoviesOptionControlHashes[
            kReplayMoviesNativeCatalogSlotCount] =
            {
                0x1AEC7D09, 0x1AEC7D0A, 0x1AEC7D0B, 0x1AEC7D0C,
                0x1AEC7D0D, 0x1AEC7D0E, 0x1AEC7D0F, 0x1AEC7D10,
                0x1AEC7D11, 0x787C1E59, 0x787C1E5A, 0x787C1E5B,
                0x787C1E5C, 0x787C1E5D, 0x787C1E5E, 0x787C1E5F,
                0x787C1E60,
            };

            static const char* const kReplayMoviesOptionTextNames[
                kReplayMoviesNativeCatalogSlotCount] =
                {
                    "OptionNametext_1", "OptionNametext_2", "OptionNametext_3",
                    "OptionNametext_4", "OptionNametext_5", "OptionNametext_6",
                    "OptionNametext_7", "OptionNametext_8", "OptionNametext_9",
                    "OptionNametext_10", "OptionNametext_11", "OptionNametext_12",
                    "OptionNametext_13", "OptionNametext_14", "OptionNametext_15",
                    "OptionNametext_16", "OptionNametext_17",
                };

                static constexpr std::uint32_t kReplayMoviesOptionTextHashes[
                    kReplayMoviesNativeCatalogSlotCount] =
                    {
                        0x6F78B7CE, 0x6F78B7CF, 0x6F78B7D0, 0x6F78B7D1,
                        0x6F78B7D2, 0x6F78B7D3, 0x6F78B7D4, 0x6F78B7D5,
                        0x6F78B7D6, 0x5E8FB1BE, 0x5E8FB1BF, 0x5E8FB1C0,
                        0x5E8FB1C1, 0x5E8FB1C2, 0x5E8FB1C3, 0x5E8FB1C4,
                        0x5E8FB1C5,
                    };

                    static const char* const kReplayMoviesHighlightNames[
                        kReplayMoviesNativeCatalogSlotCount] =
                        {
                            "Highlight_01", "Highlight_02", "Highlight_03",
                            "Highlight_04", "Highlight_05", "Highlight_06",
                            "Highlight_07", "Highlight_08", "Highlight_09",
                            "Highlight_10", "Highlight_11", "Highlight_12",
                            "Highlight_13", "Highlight_14", "Highlight_15",
                            "Highlight_16", "Highlight_17",
                        };

                        static constexpr std::uint32_t kReplayMoviesHighlightHashes[
                            kReplayMoviesNativeCatalogSlotCount] =
                            {
                                0x203C4AD7, 0x203C4AD8, 0x203C4AD9, 0x203C4ADA,
                                0x203C4ADB, 0x203C4ADC, 0x203C4ADD, 0x203C4ADE,
                                0x203C4ADF, 0x203C4AF7, 0x203C4AF8, 0x203C4AF9,
                                0x203C4AFA, 0x203C4AFB, 0x203C4AFC, 0x203C4AFD,
                                0x203C4AFE,
                            };

                            // Exact compact one-page geometry proven in x32dbg. Values are
                            // IEEE-754 bit patterns so the production writes are byte-for-byte
                            // identical to the successful debugger transaction.
                            static constexpr std::uint32_t kReplayMoviesOptionY[
                                kReplayMoviesNativeCatalogSlotCount] =
                                {
                                    0xC2F8C632, 0xC2D62D6B, 0xC2B394A5, 0xC290FBDF,
                                    0xC25CC632, 0xC21794A5, 0xC1A4C632, 0xC05318C6,
                                    0x41600000, 0x41FA6319, 0x42426319, 0x4283CA53,
                                    0x42A66319, 0x42C8FBDF, 0x42EB94A5, 0x430716B6,
                                    0x43186319,
                                };

                                static constexpr std::uint32_t kReplayMoviesTextY[
                                    kReplayMoviesNativeCatalogSlotCount] =
                                    {
                                        0xC30475AD, 0xC2E779CE, 0xC2C4E108, 0xC2A24842,
                                        0xC280D6B6, 0xC23C7BDF, 0xC1EE94A5, 0xC1519CE7,
                                        0x40865295, 0x41ABF7BE, 0x421B2D6B, 0x42605EF8,
                                        0x4292C842, 0x42B56108, 0x42D7F9CE, 0x42FA9295,
                                        0x430E95AD,
                                    };

                                    static constexpr std::uint32_t kReplayMoviesHighlightY[
                                        kReplayMoviesNativeCatalogSlotCount] =
                                        {
                                            0xC2F8C632, 0xC2D62D6B, 0xC2B4BBDF, 0xC2922319,
                                            0xC25F14A5, 0xC219E319, 0xC1AE0000, 0xC0A0E73A,
                                            0x41445295, 0x41E7EF7C, 0x4239294A, 0x427E5AD7,
                                            0x42A1C632, 0x42C45EF8, 0x42E6F7BE, 0x4304C842,
                                            0x431614A5,
                                        };

                                        static constexpr std::uint32_t kReplayMoviesOptionHeight = 0x41139CE7;
                                        static constexpr std::uint32_t kReplayMoviesTextScale = 0x3F04DA03;
                                        static constexpr std::uint32_t kReplayMoviesHighlightHeight = 0x4166A529;

                                        // Current ReplayMovies-owned Trailers generation. Never trusted without
                                        // revalidating hash/count/vector before use by a generic frontend hook.
                                        static volatile std::uintptr_t g_ReplayMoviesCatalogRuntimeAddress = 0;

                                        // Debugger-proven v11.5 navigation presentation cache.  The old post-hook
                                        // synchronizer rediscovered Text/Highlight objects and their node graphs on
                                        // every 004F1116 presentation write.  That repeated package traversal/
                                        // VirtualQuery workload was the causal source of the navigation stall and
                                        // burst-double audio.  Cache only generation-local addresses after the full
                                        // 17-row construction has completed; invalidate the cache at every generation
                                        // boundary and movie launch.
                                        struct ReplayMoviesPresentationCacheEntry
                                        {
                                            std::uintptr_t row = 0;
                                            std::uintptr_t text = 0;
                                            std::uintptr_t textSelected = 0;
                                            std::uintptr_t textDormant = 0;
                                            std::uintptr_t highlight = 0;
                                            std::uintptr_t highlightSelected = 0;
                                            std::uintptr_t highlightDormant = 0;
                                        };

                                        static ReplayMoviesPresentationCacheEntry
                                            g_ReplayMoviesPresentationCache[kReplayMoviesNativeCatalogSlotCount]{};
                                        static volatile std::uintptr_t
                                            g_ReplayMoviesPresentationCacheRuntime = 0;
                                        static volatile unsigned long
                                            g_ReplayMoviesPresentationCacheValid = 0;
                                        static volatile std::uintptr_t
                                            g_ReplayMoviesPresentationLastRuntime = 0;
                                        static volatile std::uintptr_t
                                            g_ReplayMoviesPresentationLastCurrentRow = 0;

                                        static volatile std::uint32_t g_ReplayMoviesCatalogActiveButtonFlags = 0;
                                        static volatile std::uint32_t g_ReplayMoviesCatalogActiveTextFlags = 0;

                                        // Debugger-proven User projection state. The full 17-row generation is
                                        // constructed and cached first. User mode then reuses physical rows 1..N as
                                        // compact display shells and maps each physical shell back to its canonical
                                        // movie identity. The reversed array below is staging/debug storage only:
                                        // Runtime+84 must point to a native-game allocation because the native
                                        // MU_Options_PC destructor owns and frees that field through 00671102.
                                        //
                                        // A one-visible catalog is debugger-proven and may use the exact same compact
                                        // native candidate-vector architecture. Zero-visible frontend semantics remain
                                        // intentionally guarded until COUNT=0 is separately proven in x32dbg.
                                        static constexpr std::size_t kReplayMoviesUserMinimumProvenVisibleCount = 1;
                                        static constexpr std::uint32_t kReplayMoviesFullSeenMask = 0x0001FFFFu;
                                        static constexpr unsigned long kReplayMoviesInvalidCanonicalIndex = 0xFFFFFFFFul;

                                        static std::uintptr_t
                                            g_ReplayMoviesUserCandidateVector[kReplayMoviesNativeCatalogSlotCount]{};
                                        static volatile std::uintptr_t
                                            g_ReplayMoviesUserPublishedCandidateVector = 0;
                                        static unsigned long
                                            g_ReplayMoviesUserCanonicalByPhysical[kReplayMoviesNativeCatalogSlotCount]{};
                                        static volatile unsigned long
                                            g_ReplayMoviesUserProjectionActive = 0;

                                        // A page transition is decided at the upstream 004FA140 candidate-writer
                                        // hooks, before native row presentation runs. The actual title/page rebind
                                        // is deferred until 004EFE87, after native old/new-row transition work has
                                        // already targeted an authored active control.
                                        static volatile std::uintptr_t g_ReplayMoviesCatalogPendingRuntime = 0;
                                        static volatile std::uintptr_t g_ReplayMoviesCatalogPendingIncomingRow = 0;
                                        static volatile unsigned long g_ReplayMoviesCatalogPendingPage = 0;
                                        static volatile unsigned long g_ReplayMoviesCatalogPendingPageValid = 0;

                                        // v11.3 post-FMV submenu restoration. A movie launch arms one generation-
                                        // crossing restore request containing the exact catalog movie that launched.
                                        // Authored rows 1..10 continue to restore directly by native row hash.
                                        // Synthetic rows 11..17 restore first through virtual native rows 1..7 while
                                        // the fresh Trailers runtime still owns only its ten authored candidates.
                                        // After the 17-row bootstrap completes, Runtime+80 is handed from that virtual
                                        // row to the fresh physical synthetic row and the virtual Text/Highlight are
                                        // demoted N1 -> N2 through the native 004FF030 presentation transaction.
                                        static volatile unsigned long g_PendingReplayMoviesSubmenuRestore = 0;
                                        static volatile unsigned long g_PendingReplayMoviesSubmenuRestoreMovieIndex = 0;
                                        static volatile unsigned long g_PendingReplayMoviesSubmenuRestorePage = 0;
                                        static volatile std::uint32_t g_PendingReplayMoviesSubmenuRestoreHash = 0;
                                        static volatile unsigned long g_PendingReplayMoviesSubmenuRestoreWasSubstituted = 0;

                                        // v11.4 debugger-proven post-return presentation latch.  After a synthetic
                                        // row11..17 return has completed its native virtual-row restore and physical
                                        // Runtime+80 handoff, native 004F1116 presentation writes can reassert that
                                        // virtual row's N1 state several times while the frontend settles.  Keep only
                                        // generation-local slot identity here; FEObject/node addresses are reacquired
                                        // from the live runtime on every post-hook transaction.
                                        static volatile std::uintptr_t
                                            g_ReplayMoviesSyntheticPostReturnRuntime = 0;
                                        static volatile unsigned long
                                            g_ReplayMoviesSyntheticPostReturnPhysicalSlot = 0;
                                        static volatile unsigned long g_ReplayMoviesSubmenuRestoreSubstitutions = 0;
                                        static volatile unsigned long g_ReplayMoviesSubmenuRestoreCommitSuccess = 0;
                                        static volatile unsigned long g_ReplayMoviesSubmenuRestoreCommitMiss = 0;

                                        static bool IsReadableMemoryLocal(
                                            std::uintptr_t address,
                                            std::size_t size)
                                        {
                                            if (address == 0 || size == 0)
                                                return false;

                                            MEMORY_BASIC_INFORMATION mbi{};

                                            if (VirtualQuery(
                                                reinterpret_cast<const void*>(address),
                                                &mbi,
                                                sizeof(mbi)) == 0)
                                            {
                                                return false;
                                            }

                                            if (mbi.State != MEM_COMMIT ||
                                                (mbi.Protect & PAGE_GUARD) != 0 ||
                                                mbi.Protect == PAGE_NOACCESS)
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t regionStart =
                                                reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);

                                            const std::uintptr_t regionEnd =
                                                regionStart + mbi.RegionSize;

                                            if (address < regionStart ||
                                                address > regionEnd ||
                                                size > regionEnd - address)
                                            {
                                                return false;
                                            }

                                            return true;
                                        }


                                        static bool AsciiStringEqualsLocal(
                                            const char* value,
                                            const char* expected)
                                        {
                                            if (value == nullptr || expected == nullptr)
                                                return false;

                                            for (std::size_t i = 0; i < 64; ++i)
                                            {
                                                if (!IsReadableMemoryLocal(
                                                    reinterpret_cast<std::uintptr_t>(value + i),
                                                    1))
                                                {
                                                    return false;
                                                }

                                                const char a = value[i];
                                                const char b = expected[i];

                                                if (a != b)
                                                    return false;

                                                if (b == '\0')
                                                    return true;
                                            }

                                            return false;
                                        }


                                        // -------------------------------------------------------------------------
                                        // v11.0 synthetic row-11 native object helpers
                                        // -------------------------------------------------------------------------
                                        namespace
                                        {
                                            constexpr std::uintptr_t kNativeGameAllocate = 0x0067121A;
                                            constexpr std::uintptr_t kNativeGameFree = 0x00671102;
                                            constexpr std::uint32_t kGeometryDescriptorHeaderLocal = 0x00010C04;
                                            constexpr std::uint32_t kGeometryChannelLocal = 0x006C1830;
                                            constexpr std::size_t kSyntheticPropertyScanSize = 0x140;
                                            constexpr std::size_t kSyntheticMaxSubordinateNodes = 16;

                                            using NativeGameAllocateFn = void* (__cdecl*)(std::size_t);
                                            using NativeGameFreeFn = void(__cdecl*)(void*);
                                            using NativeCloneObjectFn = void* (__thiscall*)(void*, int);

                                            static DWORD g_NativeAttachFEObjectLocal =
                                                static_cast<DWORD>(Addr::NativeAttachFEObject);

                                            static void* NativeGameAllocateLocal(std::size_t bytes)
                                            {
                                                auto function = reinterpret_cast<NativeGameAllocateFn>(
                                                    kNativeGameAllocate);
                                                return function(bytes);
                                            }

                                            static void NativeGameFreeLocal(void* memory)
                                            {
                                                if (memory == nullptr)
                                                    return;

                                                auto function = reinterpret_cast<NativeGameFreeFn>(
                                                    kNativeGameFree);
                                                function(memory);
                                            }

                                            static void* NativeCloneObjectLocal(std::uintptr_t sourceAddress)
                                            {
                                                if (!IsReadableMemoryLocal(sourceAddress, 0x58))
                                                    return nullptr;

                                                const std::uintptr_t vtable =
                                                    *reinterpret_cast<const std::uintptr_t*>(sourceAddress + 0x00);

                                                if (!IsReadableMemoryLocal(vtable, 0x08))
                                                    return nullptr;

                                                const std::uintptr_t cloneAddress =
                                                    *reinterpret_cast<const std::uintptr_t*>(vtable + 0x04);

                                                if (cloneAddress == 0)
                                                    return nullptr;

                                                auto cloneFunction =
                                                    reinterpret_cast<NativeCloneObjectFn>(cloneAddress);

                                                return cloneFunction(
                                                    reinterpret_cast<void*>(sourceAddress),
                                                    1);
                                            }

                                            __declspec(noinline)
                                                static void NativeAttachFEObjectLocal(
                                                    std::uintptr_t runtimeAddress,
                                                    std::uintptr_t objectAddress)
                                            {
                                                __asm
                                                {
                                                    mov eax, objectAddress
                                                    mov edx, runtimeAddress
                                                    call dword ptr[g_NativeAttachFEObjectLocal]
                                                }
                                            }

                                            static bool CopyClonePayloadLocal(
                                                std::uintptr_t sourceAddress,
                                                std::uintptr_t cloneAddress)
                                            {
                                                if (!IsReadableMemoryLocal(sourceAddress, 0x34) ||
                                                    !IsReadableMemoryLocal(cloneAddress, 0x34))
                                                {
                                                    return false;
                                                }

                                                const std::uintptr_t sourcePayload =
                                                    *reinterpret_cast<const std::uintptr_t*>(sourceAddress + 0x2C);
                                                const std::uintptr_t clonePayload =
                                                    *reinterpret_cast<const std::uintptr_t*>(cloneAddress + 0x2C);
                                                const std::uint32_t sourceSize =
                                                    *reinterpret_cast<const std::uint32_t*>(sourceAddress + 0x30);
                                                const std::uint32_t cloneSize =
                                                    *reinterpret_cast<const std::uint32_t*>(cloneAddress + 0x30);

                                                if (sourcePayload == 0 || clonePayload == 0 ||
                                                    sourceSize == 0 || sourceSize != cloneSize ||
                                                    sourceSize > 0x400 ||
                                                    !IsReadableMemoryLocal(sourcePayload, sourceSize) ||
                                                    !IsReadableMemoryLocal(clonePayload, cloneSize))
                                                {
                                                    return false;
                                                }

                                                std::memcpy(
                                                    reinterpret_cast<void*>(clonePayload),
                                                    reinterpret_cast<const void*>(sourcePayload),
                                                    sourceSize);

                                                return true;
                                            }

                                            static bool AssignSyntheticObjectIdentityLocal(
                                                std::uintptr_t sourceAddress,
                                                std::uintptr_t cloneAddress,
                                                const char* expectedSourceName,
                                                const char* targetName,
                                                std::uint32_t targetHash)
                                            {
                                                if (!IsReadableMemoryLocal(sourceAddress, 0x20) ||
                                                    !IsReadableMemoryLocal(cloneAddress, 0x20) ||
                                                    expectedSourceName == nullptr || targetName == nullptr)
                                                {
                                                    return false;
                                                }

                                                const std::uintptr_t sourceNameAddress =
                                                    *reinterpret_cast<const std::uintptr_t*>(sourceAddress + 0x14);
                                                const std::uintptr_t cloneNameAddress =
                                                    *reinterpret_cast<const std::uintptr_t*>(cloneAddress + 0x14);

                                                if (sourceNameAddress == 0 || cloneNameAddress == 0 ||
                                                    sourceNameAddress == cloneNameAddress ||
                                                    !AsciiStringEqualsLocal(
                                                        reinterpret_cast<const char*>(sourceNameAddress),
                                                        expectedSourceName))
                                                {
                                                    return false;
                                                }

                                                const std::size_t sourceLength = std::strlen(expectedSourceName);
                                                const std::size_t targetLength = std::strlen(targetName);

                                                // _10 -> _11 names deliberately have identical length, so the
                                                // clone-owned name allocation never needs to be resized.
                                                if (sourceLength != targetLength ||
                                                    !IsReadableMemoryLocal(cloneNameAddress, targetLength + 1))
                                                {
                                                    return false;
                                                }

                                                std::memcpy(
                                                    reinterpret_cast<void*>(cloneNameAddress),
                                                    targetName,
                                                    targetLength + 1);

                                                *reinterpret_cast<std::uint32_t*>(cloneAddress + 0x10) =
                                                    targetHash;

                                                // Preserve the exact current authored row state. This keeps row 11
                                                // dormant at bootstrap when row 1 is the native current selection.
                                                *reinterpret_cast<std::uint32_t*>(cloneAddress + 0x1C) =
                                                    *reinterpret_cast<const std::uint32_t*>(sourceAddress + 0x1C);

                                                return true;
                                            }

                                            static void ShiftSyntheticGeometryDescriptorsLocal(
                                                std::uintptr_t cloneAddress,
                                                float deltaY)
                                            {
                                                if (!IsReadableMemoryLocal(cloneAddress, 0x58))
                                                    return;

                                                const std::uintptr_t payload =
                                                    *reinterpret_cast<const std::uintptr_t*>(cloneAddress + 0x2C);

                                                if (IsReadableMemoryLocal(payload, 0x28))
                                                {
                                                    *reinterpret_cast<float*>(payload + 0x20) += deltaY;
                                                }

                                                std::uintptr_t node =
                                                    *reinterpret_cast<const std::uintptr_t*>(cloneAddress + 0x4C);

                                                for (std::size_t nodeIndex = 0;
                                                    nodeIndex < kSyntheticMaxSubordinateNodes && node != 0;
                                                    ++nodeIndex)
                                                {
                                                    if (!IsReadableMemoryLocal(node, 0x24))
                                                        return;

                                                    const std::uintptr_t storage =
                                                        *reinterpret_cast<const std::uintptr_t*>(node + 0x20);

                                                    if (storage != 0 &&
                                                        IsReadableMemoryLocal(storage, kSyntheticPropertyScanSize))
                                                    {
                                                        for (std::size_t offset = 0;
                                                            offset + 0x24 <= kSyntheticPropertyScanSize;
                                                            offset += sizeof(std::uint32_t))
                                                        {
                                                            const std::uintptr_t descriptor = storage + offset;
                                                            const std::uint32_t header =
                                                                *reinterpret_cast<const std::uint32_t*>(descriptor + 0x00);
                                                            const std::uint32_t channel =
                                                                *reinterpret_cast<const std::uint32_t*>(descriptor + 0x08);

                                                            if ((header & 0x7FFFFFFFu) ==
                                                                kGeometryDescriptorHeaderLocal &&
                                                                channel == kGeometryChannelLocal)
                                                            {
                                                                *reinterpret_cast<float*>(descriptor + 0x1C) +=
                                                                    deltaY;
                                                            }
                                                        }
                                                    }

                                                    node =
                                                        *reinterpret_cast<const std::uintptr_t*>(node + 0x04);
                                                }
                                            }

                                            static bool AttachSyntheticObjectLocal(
                                                std::uintptr_t runtimeAddress,
                                                std::uintptr_t objectAddress)
                                            {
                                                if (!IsReadableMemoryLocal(runtimeAddress, 0x48) ||
                                                    !IsReadableMemoryLocal(objectAddress, 0x58))
                                                {
                                                    return false;
                                                }

                                                const std::uint32_t countBefore =
                                                    *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x3C);

                                                NativeAttachFEObjectLocal(runtimeAddress, objectAddress);

                                                return
                                                    *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x3C) ==
                                                    countBefore + 1 &&
                                                    *reinterpret_cast<const std::uintptr_t*>(runtimeAddress + 0x44) ==
                                                    objectAddress;
                                            }

                                            static void SetSyntheticTextLocal(
                                                std::uintptr_t textObjectAddress,
                                                const char* title)
                                            {
                                                if (textObjectAddress == 0 || title == nullptr)
                                                    return;

                                                __asm
                                                {
                                                    mov eax, textObjectAddress
                                                    push title
                                                    call dword ptr[g_FEngPrintfObj]
                                                    add esp, 4
                                                }
                                            }
                                        }



                                        // -------------------------------------------------------------------------
                                        // v11.1 one-page 17-row Developer-catalog activation
                                        //
                                        // MU_Options_PC uses a static MenuScreen vtable whose notification virtual is
                                        // vtable+08 = 004CF240. Retail Trailers installs one active control callback:
                                        // screen+40 = 004D0B20 (OptionName_1 / NASCAR launcher). The remaining nine
                                        // authored OptionName_N controls are dormant and their callback slots are zero.
                                        //
                                        // The two-row proof established that copying the proven row-1 callback into
                                        // screen+44 makes OptionName_2 launch through the exact same native pipeline,
                                        // with playback identity determined independently by the selected catalog row.
                                        // Scale that proven rule mechanically across all ten authored callback slots:
                                        //
                                        //   +40 OptionName_1  = 004D0B20 (native)
                                        //   +44 OptionName_2  = 004D0B20
                                        //   ...
                                        //   +64 OptionName_10 = 004D0B20
                                        //
                                        // No per-row launcher is invented. The original 004CF240 handler still owns
                                        // notification/event routing. Outside ReplayMovies the wrapper is a transparent
                                        // tail-jump to native code.
                                        // -------------------------------------------------------------------------
                                        static constexpr std::uintptr_t kMuOptionsMenuScreenVtable = 0x006C3E3C;
                                        static constexpr std::uintptr_t kMuOptionsNotificationVtableSlot = 0x006C3E44;
                                        static constexpr std::uintptr_t kNativeMuOptionsNotification = 0x004CF240;
                                        static constexpr std::uintptr_t kNativeTrailersCallback = 0x004D0B20;

                                        // Global frontend dispatch site immediately before the active MenuScreen
                                        // virtual notification call.  The first two native instructions are:
                                        //
                                        //   004F7D10  mov eax,[esp+50]
                                        //   004F7D14  mov ecx,[esp+4C]
                                        //
                                        // Synthetic rows must borrow authored OptionName_1 BEFORE the
                                        // MU_Options_PC notification handler performs its routing decision.
                                        static constexpr std::uintptr_t kFrontendNotificationPreRouteHook = 0x004F7D10;
                                        static constexpr std::uintptr_t kFrontendNotificationPreRouteReturn = 0x004F7D18;
                                        static constexpr std::size_t kReplayMoviesTrailersCallbackSlotCount = 10;
                                        static constexpr std::size_t kReplayMoviesTrailersCallbackBaseOffset = 0x40;

                                        static DWORD g_NativeMuOptionsNotification =
                                            static_cast<DWORD>(kNativeMuOptionsNotification);

                                        static DWORD g_NativeTrailersCallback =
                                            static_cast<DWORD>(kNativeTrailersCallback);

                                        // v11.3 exact start-side seam proven in x32dbg.  Synthetic rows already
                                        // publish the real 0x0C407210 frontend activation transaction.  At the
                                        // 004F7C00 dispatcher entry, only its candidate argument ([ESP+08]) must
                                        // borrow this generation's authored OptionName_1. Runtime+80 stays on the
                                        // physical synthetic row for the entire native callback/movie-resolution path.
                                        static constexpr std::uint32_t kReplayMoviesActivationEvent = 0x0C407210;
                                        static constexpr std::uintptr_t kFrontendDispatchEntryHook = 0x004F7C00;
                                        static constexpr std::uintptr_t kFrontendDispatchEntryReturn = 0x004F7C08;
                                        static DWORD g_ReplayMoviesFrontendDispatchEntryReturn =
                                            static_cast<DWORD>(kFrontendDispatchEntryReturn);

                                        static volatile std::uintptr_t g_ReplayMoviesSyntheticActivationRuntime = 0;
                                        static volatile std::uintptr_t g_ReplayMoviesSyntheticActivationRow = 0;
                                        static volatile unsigned long g_ReplayMoviesSyntheticActivationActive = 0;
                                        static volatile unsigned long g_ReplayMoviesSyntheticActivationReturnArmed = 0;
                                        static volatile std::uintptr_t g_ReplayMoviesNotificationOriginalReturn = 0;

                                        static DWORD g_ReplayMoviesFrontendNotificationPreRouteReturn =
                                            static_cast<DWORD>(kFrontendNotificationPreRouteReturn);

                                        static void ReplayMoviesTrailersCallbackProxy();
                                        static void ReplayMoviesMuOptionsNotificationReturnThunk();
                                        static void ReplayMoviesFrontendNotificationPreRouteHook();

                                        // User projection helpers are defined after the complete 17-row acquisition
                                        // helpers, but synthetic activation hooks execute earlier in this translation
                                        // unit and need their contracts declared here.
                                        static bool IsReplayMoviesUserProjectionPublishedLocal(
                                            std::uintptr_t runtimeAddress);
                                        static std::size_t GetReplayMoviesCatalogSlotNumber(
                                            std::uintptr_t runtimeAddress,
                                            std::uintptr_t rowAddress);
                                        static std::uintptr_t GetReplayMoviesCatalogSlotAddress(
                                            std::uintptr_t runtimeAddress,
                                            std::size_t slotNumber);


                                        static bool __cdecl PrepareReplayMoviesSyntheticActivationRoutingLocal()
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                g_ReplayMoviesSyntheticActivationActive != 0)
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t runtimeAddress =
                                                g_ReplayMoviesCatalogRuntimeAddress;

                                            if (!IsReadableMemoryLocal(runtimeAddress, 0x8C) ||
                                                *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                (*reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x88) !=
                                                    kReplayMoviesNativeCatalogSlotCount &&
                                                    !IsReplayMoviesUserProjectionPublishedLocal(runtimeAddress)))
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t currentRow =
                                                *reinterpret_cast<const std::uintptr_t*>(runtimeAddress + 0x80);
                                            const std::size_t physicalSlot =
                                                GetReplayMoviesCatalogSlotNumber(runtimeAddress, currentRow);

                                            if (physicalSlot <= kReplayMoviesAuthoredCatalogSlotCount ||
                                                physicalSlot > kReplayMoviesNativeCatalogSlotCount ||
                                                !IsReadableMemoryLocal(currentRow, 0x14))
                                            {
                                                return false;
                                            }

                                            const std::uint32_t currentHash =
                                                *reinterpret_cast<const std::uint32_t*>(currentRow + 0x10);

                                            if (currentHash < kReplayMoviesOptionName11Hash ||
                                                currentHash > kReplayMoviesOptionControlHashes[
                                                    kReplayMoviesNativeCatalogSlotCount - 1])
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t row1 =
                                                GetReplayMoviesCatalogSlotAddress(runtimeAddress, 1);

                                            if (!IsReadableMemoryLocal(row1, 0x14) ||
                                                *reinterpret_cast<const std::uint32_t*>(row1 + 0x10) !=
                                                kReplayMoviesOptionControlHashes[0])
                                            {
                                                return false;
                                            }

                                            g_ReplayMoviesSyntheticActivationRuntime = runtimeAddress;
                                            g_ReplayMoviesSyntheticActivationRow = currentRow;
                                            *reinterpret_cast<std::uintptr_t*>(runtimeAddress + 0x80) = row1;
                                            g_ReplayMoviesSyntheticActivationActive = 1;
                                            return true;
                                        }


                                        static void __cdecl RestoreReplayMoviesSyntheticActivationRoutingLocal()
                                        {
                                            if (g_ReplayMoviesSyntheticActivationActive == 0)
                                                return;

                                            const std::uintptr_t runtimeAddress =
                                                g_ReplayMoviesSyntheticActivationRuntime;
                                            const std::uintptr_t syntheticRow =
                                                g_ReplayMoviesSyntheticActivationRow;

                                            if (IsReadableMemoryLocal(runtimeAddress, 0x8C) &&
                                                *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x10) ==
                                                Addr::TrailersRuntimeHash &&
                                                (*reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x88) ==
                                                    kReplayMoviesNativeCatalogSlotCount ||
                                                    IsReplayMoviesUserProjectionPublishedLocal(runtimeAddress)) &&
                                                IsReadableMemoryLocal(syntheticRow, 0x14))
                                            {
                                                const std::uintptr_t row1 =
                                                    GetReplayMoviesCatalogSlotAddress(runtimeAddress, 1);

                                                // Restore only while the temporary row-1 identity is still live.
                                                if (row1 != 0 &&
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        runtimeAddress + 0x80) == row1)
                                                {
                                                    *reinterpret_cast<std::uintptr_t*>(
                                                        runtimeAddress + 0x80) = syntheticRow;
                                                }
                                            }

                                            g_ReplayMoviesSyntheticActivationRuntime = 0;
                                            g_ReplayMoviesSyntheticActivationRow = 0;
                                            g_ReplayMoviesSyntheticActivationActive = 0;
                                        }


                                        static std::uintptr_t __cdecl ResolveReplayMoviesActivationDispatchCandidateLocal(
                                            std::uint32_t message,
                                            std::uintptr_t candidateAddress,
                                            std::uintptr_t ownerRuntimeAddress)
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                message != kReplayMoviesActivationEvent ||
                                                ownerRuntimeAddress == 0 ||
                                                ownerRuntimeAddress != g_ReplayMoviesCatalogRuntimeAddress ||
                                                candidateAddress == 0 ||
                                                !IsReadableMemoryLocal(ownerRuntimeAddress, 0x8C) ||
                                                *reinterpret_cast<const std::uint32_t*>(ownerRuntimeAddress + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                (*reinterpret_cast<const std::uint32_t*>(ownerRuntimeAddress + 0x88) !=
                                                    kReplayMoviesNativeCatalogSlotCount &&
                                                    !IsReplayMoviesUserProjectionPublishedLocal(ownerRuntimeAddress)) ||
                                                !IsReadableMemoryLocal(candidateAddress, 0x14))
                                            {
                                                return candidateAddress;
                                            }

                                            const std::uintptr_t currentRow =
                                                *reinterpret_cast<const std::uintptr_t*>(ownerRuntimeAddress + 0x80);

                                            if (currentRow != candidateAddress)
                                                return candidateAddress;

                                            const std::size_t physicalSlot =
                                                GetReplayMoviesCatalogSlotNumber(
                                                    ownerRuntimeAddress,
                                                    candidateAddress);

                                            if (physicalSlot <= kReplayMoviesAuthoredCatalogSlotCount ||
                                                physicalSlot > kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return candidateAddress;
                                            }

                                            const std::uint32_t candidateHash =
                                                *reinterpret_cast<const std::uint32_t*>(candidateAddress + 0x10);

                                            if (candidateHash < kReplayMoviesOptionName11Hash ||
                                                candidateHash >
                                                kReplayMoviesOptionControlHashes[
                                                    kReplayMoviesNativeCatalogSlotCount - 1])
                                            {
                                                return candidateAddress;
                                            }

                                            const std::uintptr_t row1 =
                                                GetReplayMoviesCatalogSlotAddress(ownerRuntimeAddress, 1);

                                            if (!IsReadableMemoryLocal(row1, 0x14) ||
                                                *reinterpret_cast<const std::uint32_t*>(row1 + 0x10) !=
                                                kReplayMoviesOptionControlHashes[0])
                                            {
                                                return candidateAddress;
                                            }

                                            return row1;
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesFrontendDispatchEntryHook()
                                        {
                                            __asm
                                            {
                                                // Native 004F7C00 entry stack, proven in x32dbg:
                                                //   [ESP+04] = frontend message
                                                //   [ESP+08] = candidate FEObject*
                                                //   [ESP+0C] = auxiliary argument
                                                //   [ESP+10] = owning selector/runtime
                                                //
                                                // pushfd + pushad move those original slots by +0x24.  The helper
                                                // returns either the unchanged candidate (fail closed) or this
                                                // generation's authored OptionName_1.  Only original [ESP+08] is
                                                // modified; popad/popfd restore every register and flag unchanged.
                                                pushfd
                                                pushad

                                                mov eax, dword ptr[esp + 34h]  // original owner/runtime
                                                mov ecx, dword ptr[esp + 2Ch]  // original candidate
                                                mov edx, dword ptr[esp + 28h]  // original message
                                                push eax
                                                push ecx
                                                push edx
                                                call ResolveReplayMoviesActivationDispatchCandidateLocal
                                                add esp, 0Ch
                                                mov dword ptr[esp + 2Ch], eax  // original [ESP+08]

                                                popad
                                                popfd

                                                // Replay exactly the eight native bytes overwritten at 004F7C00:
                                                //   sub esp,38
                                                //   push ebp
                                                //   mov ebp,[esp+40]
                                                sub esp, 38h
                                                push ebp
                                                mov ebp, dword ptr[esp + 40h]

                                                jmp dword ptr[g_ReplayMoviesFrontendDispatchEntryReturn]
                                            }
                                        }


                                        static void __cdecl EnsureReplayMoviesFullCatalogActivation(
                                            std::uintptr_t screenAddress)
                                        {
                                            constexpr std::size_t kRequiredScreenBytes =
                                                kReplayMoviesTrailersCallbackBaseOffset +
                                                (kReplayMoviesTrailersCallbackSlotCount * sizeof(std::uintptr_t));

                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                !IsReadableMemoryLocal(screenAddress, kRequiredScreenBytes))
                                            {
                                                return;
                                            }

                                            if (*reinterpret_cast<const std::uintptr_t*>(screenAddress + 0x00) !=
                                                kMuOptionsMenuScreenVtable)
                                            {
                                                return;
                                            }

                                            const std::uintptr_t packageNameAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(screenAddress + 0x0C);

                                            if (packageNameAddress == 0 ||
                                                !AsciiStringEqualsLocal(
                                                    reinterpret_cast<const char*>(packageNameAddress),
                                                    "MU_Options_PC.fng"))
                                            {
                                                return;
                                            }

                                            auto* callbacks =
                                                reinterpret_cast<std::uintptr_t*>(
                                                    screenAddress + kReplayMoviesTrailersCallbackBaseOffset);
                                            const std::uintptr_t legacyCallbackProxy =
                                                reinterpret_cast<std::uintptr_t>(
                                                    &ReplayMoviesTrailersCallbackProxy);

                                            // On first ownership slot 1 must still be the proven retail Trailers
                                            // callback. Accept the old transparent proxy only so a re-entrant screen
                                            // from an earlier in-process state can be normalized back to native.
                                            if (callbacks[0] != kNativeTrailersCallback &&
                                                callbacks[0] != legacyCallbackProxy)
                                            {
                                                return;
                                            }

                                            // Rows1..10 keep the existing authored callback-slot solution, but publish
                                            // the real native Trailers callback directly. Synthetic rows11..17 do not
                                            // invent callback slots; the 004F7C00 candidate-only route borrows row1.
                                            for (std::size_t callbackIndex = 0;
                                                callbackIndex < kReplayMoviesTrailersCallbackSlotCount;
                                                ++callbackIndex)
                                            {
                                                callbacks[callbackIndex] = kNativeTrailersCallback;
                                            }
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesTrailersCallbackProxy()
                                        {
                                            __asm
                                            {
                                                // Preserve the native __thiscall callback frame. If synthetic
                                                // activation routing is armed, restore Runtime+80 to the actual
                                                // OptionName_11..17 before 004D0B20 sees the current selection.
                                                pushfd
                                                pushad
                                                call RestoreReplayMoviesSyntheticActivationRoutingLocal
                                                popad
                                                popfd
                                                jmp dword ptr[g_NativeTrailersCallback]
                                            }
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesMuOptionsNotificationReturnThunk()
                                        {
                                            __asm
                                            {
                                                // Native 004CF240 completed without taking the movie callback.
                                                // Restore the exact generation-local synthetic row that was saved
                                                // by PrepareReplayMoviesSyntheticActivationRoutingLocal().
                                                pushfd
                                                pushad
                                                call RestoreReplayMoviesSyntheticActivationRoutingLocal
                                                mov dword ptr[g_ReplayMoviesSyntheticActivationReturnArmed], 0
                                                popad
                                                popfd
                                                jmp dword ptr[g_ReplayMoviesNotificationOriginalReturn]
                                            }
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesMuOptionsNotificationProxy()
                                        {
                                            __asm
                                            {
                                                // Preserve the native __thiscall frame exactly.  This wrapper now
                                                // does one job only: keep authored callback slots 1..10 bound to the
                                                // proven native Trailers callback while ReplayMovies owns this screen.
                                                // It never changes Runtime+80 and never participates in synthetic
                                                // rows11..17 activation routing.
                                                pushfd
                                                pushad

                                                // pushfd + pushad: saved native ECX is at [esp+18].
                                                mov eax, dword ptr[esp + 18h]
                                                push eax
                                                call EnsureReplayMoviesFullCatalogActivation
                                                add esp, 4

                                                popad
                                                popfd
                                                jmp dword ptr[g_NativeMuOptionsNotification]
                                            }
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesFrontendNotificationPreRouteHook()
                                        {
                                            __asm
                                            {
                                                // This hook is intentionally earlier than 004CF240.  ESI is the
                                                // active MenuScreen at the proven global frontend dispatch site.
                                                // Every helper below revalidates ReplayMovies ownership and the
                                                // 17-row Trailers runtime, so unrelated frontend packages are inert.
                                                pushfd
                                                pushad

                                                push esi
                                                call EnsureReplayMoviesFullCatalogActivation
                                                add esp, 4

                                                // For OptionName_11..17 this saves that exact current FEObject and
                                                // temporarily publishes the generation's authored OptionName_1.
                                                // No synthetic row shares another synthetic row's pointer/state.
                                                call PrepareReplayMoviesSyntheticActivationRoutingLocal

                                                popad
                                                popfd

                                                // Replay exactly the two overwritten native instructions.
                                                mov eax, dword ptr[esp + 50h]
                                                mov ecx, dword ptr[esp + 4Ch]
                                                jmp dword ptr[g_ReplayMoviesFrontendNotificationPreRouteReturn]
                                            }
                                        }


                                        static bool InstallReplayMoviesFullCatalogNotificationProxy()
                                        {
                                            auto* slot = reinterpret_cast<std::uintptr_t*>(
                                                kMuOptionsNotificationVtableSlot);

                                            if (!IsReadableMemoryLocal(
                                                kMuOptionsNotificationVtableSlot,
                                                sizeof(std::uintptr_t)))
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t proxy =
                                                reinterpret_cast<std::uintptr_t>(
                                                    &ReplayMoviesMuOptionsNotificationProxy);

                                            if (*slot == proxy)
                                                return true;

                                            // Refuse to patch an unknown executable/vtable build.
                                            if (*slot != kNativeMuOptionsNotification)
                                                return false;

                                            DWORD oldProtect = 0;
                                            if (!VirtualProtect(
                                                slot,
                                                sizeof(std::uintptr_t),
                                                PAGE_EXECUTE_READWRITE,
                                                &oldProtect))
                                            {
                                                return false;
                                            }

                                            *slot = proxy;

                                            FlushInstructionCache(
                                                GetCurrentProcess(),
                                                slot,
                                                sizeof(std::uintptr_t));

                                            DWORD ignored = 0;
                                            VirtualProtect(
                                                slot,
                                                sizeof(std::uintptr_t),
                                                oldProtect,
                                                &ignored);

                                            return true;
                                        }

                                        static bool InstallReplayMoviesSyntheticActivationPreRouteHook()
                                        {
                                            // Best-effort only.  Do not put this generic frontend site into the
                                            // all-or-nothing ReplayMovies preflight; an unknown executable build
                                            // must fail closed without suppressing Main Menu construction.
                                            static const std::uint8_t kExpectedBytes[8] =
                                            {
                                                0x8B, 0x44, 0x24, 0x50, // mov eax,[esp+50]
                                                0x8B, 0x4C, 0x24, 0x4C  // mov ecx,[esp+4C]
                                            };

                                            const auto* current = reinterpret_cast<const std::uint8_t*>(
                                                kFrontendNotificationPreRouteHook);

                                            if (!IsReadableMemoryLocal(
                                                kFrontendNotificationPreRouteHook,
                                                sizeof(kExpectedBytes)))
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                current,
                                                kExpectedBytes,
                                                sizeof(kExpectedBytes)) != 0)
                                            {
                                                return false;
                                            }

                                            return WriteAbsoluteJumpPadded(
                                                kFrontendNotificationPreRouteHook,
                                                reinterpret_cast<const void*>(
                                                    &ReplayMoviesFrontendNotificationPreRouteHook),
                                                sizeof(kExpectedBytes));
                                        }


                                        static void __cdecl SaveReplayMoviesBorrowedRouteState()
                                        {
                                            if (g_ReplayMoviesBorrowedRouteStateSaved != 0)
                                                return;

                                            g_SavedMainMenuActionState =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    Addr::MainMenuActionState);

                                            g_SavedOptionsSelectorId =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    Addr::OptionsSelectorId);

                                            g_ReplayMoviesBorrowedRouteStateSaved = 1;
                                            ++g_ReplayMoviesBorrowedRouteStateSaves;
                                        }


                                        static void __cdecl RestoreReplayMoviesBorrowedRouteState()
                                        {
                                            if (g_ReplayMoviesBorrowedRouteStateSaved == 0)
                                                return;

                                            *reinterpret_cast<std::uint32_t*>(
                                                Addr::MainMenuActionState) =
                                                g_SavedMainMenuActionState;

                                            *reinterpret_cast<std::uint32_t*>(
                                                Addr::OptionsSelectorId) =
                                                g_SavedOptionsSelectorId;

                                            g_ReplayMoviesBorrowedRouteStateSaved = 0;
                                            ++g_ReplayMoviesBorrowedRouteStateRestores;
                                        }


                                        static bool WriteAsciiTitleToWideBufferLocal(
                                            std::uintptr_t bufferAddress,
                                            const char* title)
                                        {
                                            if (bufferAddress == 0 || title == nullptr)
                                                return false;

                                            std::size_t length = 0;
                                            while (title[length] != '\0' && length < 63)
                                                ++length;

                                            if (title[length] != '\0')
                                                return false;

                                            const std::size_t bytes =
                                                (length + 1) * sizeof(wchar_t);

                                            if (!IsReadableMemoryLocal(bufferAddress, bytes))
                                                return false;

                                            auto* output =
                                                reinterpret_cast<wchar_t*>(bufferAddress);

                                            for (std::size_t i = 0; i < length; ++i)
                                                output[i] = static_cast<unsigned char>(title[i]);

                                            output[length] = L'\0';
                                            return true;
                                        }


                                        static std::uintptr_t FindPackageObjectByNameLocal(
                                            std::uintptr_t runtimeAddress,
                                            const char* expectedName)
                                        {
                                            if (!IsReadableMemoryLocal(runtimeAddress, 0x48))
                                                return 0;

                                            const std::uint32_t objectCount =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    runtimeAddress + 0x3C);

                                            if (objectCount == 0 || objectCount > 512)
                                                return 0;

                                            std::uintptr_t objectAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    runtimeAddress + 0x44);

                                            for (std::uint32_t index = 0;
                                                index < objectCount && objectAddress != 0;
                                                ++index)
                                            {
                                                if (!IsReadableMemoryLocal(objectAddress, 0x58))
                                                    return 0;

                                                const std::uintptr_t nameAddress =
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        objectAddress + 0x14);

                                                if (nameAddress != 0 &&
                                                    IsReadableMemoryLocal(nameAddress, 1) &&
                                                    AsciiStringEqualsLocal(
                                                        reinterpret_cast<const char*>(nameAddress),
                                                        expectedName))
                                                {
                                                    return objectAddress;
                                                }

                                                objectAddress =
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        objectAddress + 0x08);
                                            }

                                            return 0;
                                        }


                                        static std::uintptr_t FindEquivalentCurrentNodeLocal(
                                            std::uintptr_t targetObject,
                                            std::uintptr_t templateObject)
                                        {
                                            if (!IsReadableMemoryLocal(targetObject, 0x58) ||
                                                !IsReadableMemoryLocal(templateObject, 0x58))
                                            {
                                                return 0;
                                            }

                                            const std::uintptr_t templateCurrent =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    templateObject + 0x54);

                                            const std::uintptr_t targetRoot =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    targetObject + 0x4C);

                                            if (!IsReadableMemoryLocal(templateCurrent, 0x34) ||
                                                !IsReadableMemoryLocal(targetRoot, 0x34))
                                            {
                                                return 0;
                                            }

                                            const std::uint32_t wantedState =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    templateCurrent + 0x0C);

                                            const std::uint32_t wantedKey =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    templateCurrent + 0x30);

                                            std::uintptr_t node = targetRoot;

                                            for (std::uint32_t index = 0;
                                                index < 16 && node != 0;
                                                ++index)
                                            {
                                                if (!IsReadableMemoryLocal(node, 0x34))
                                                    return 0;

                                                if (*reinterpret_cast<const std::uint32_t*>(
                                                    node + 0x0C) == wantedState &&
                                                    *reinterpret_cast<const std::uint32_t*>(
                                                        node + 0x30) == wantedKey)
                                                {
                                                    return node;
                                                }

                                                node =
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        node + 0x04);
                                            }

                                            return 0;
                                        }


                                        static bool EnsureReplayMoviesSyntheticSlot11(
                                            std::uintptr_t runtimeAddress)
                                        {
                                            ++g_ReplayMoviesSynthetic11BuildAttempts;
                                            g_ReplayMoviesSynthetic11BuildStage = 1;
                                            g_ReplayMoviesSynthetic11OptionName = 0;
                                            g_ReplayMoviesSynthetic11OptionText = 0;
                                            g_ReplayMoviesSynthetic11Highlight = 0;

                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C))
                                            {
                                                return false;
                                            }

                                            auto* runtime = reinterpret_cast<std::uint8_t*>(runtimeAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(runtime + 0x10) !=
                                                Addr::TrailersRuntimeHash)
                                            {
                                                return false;
                                            }

                                            const std::uint32_t existingCount =
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88);

                                            if (existingCount == kReplayMoviesSynthetic11CatalogSlotCount)
                                            {
                                                const std::uintptr_t existingButton =
                                                    FindPackageObjectByNameLocal(runtimeAddress, kReplayMoviesOptionName11);
                                                const std::uintptr_t existingText =
                                                    FindPackageObjectByNameLocal(runtimeAddress, kReplayMoviesOptionText11);
                                                const std::uintptr_t existingHighlight =
                                                    FindPackageObjectByNameLocal(runtimeAddress, kReplayMoviesHighlight11);

                                                if (existingButton != 0 && existingText != 0 && existingHighlight != 0)
                                                {
                                                    g_ReplayMoviesSynthetic11OptionName = existingButton;
                                                    g_ReplayMoviesSynthetic11OptionText = existingText;
                                                    g_ReplayMoviesSynthetic11Highlight = existingHighlight;
                                                    g_ReplayMoviesSynthetic11BuildStage = 8;
                                                    return true;
                                                }

                                                return false;
                                            }

                                            if (existingCount != kReplayMoviesAuthoredCatalogSlotCount)
                                                return false;

                                            const std::uintptr_t oldVectorAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(runtime + 0x84);

                                            if (oldVectorAddress == 0 ||
                                                !IsReadableMemoryLocal(
                                                    oldVectorAddress,
                                                    kReplayMoviesAuthoredCatalogSlotCount * sizeof(std::uintptr_t)))
                                            {
                                                return false;
                                            }

                                            const auto* oldCandidates =
                                                reinterpret_cast<const std::uintptr_t*>(oldVectorAddress);

                                            const std::uintptr_t sourceButton = oldCandidates[0];
                                            const std::uintptr_t sourceText =
                                                FindPackageObjectByNameLocal(runtimeAddress, kReplayMoviesOptionText10);
                                            const std::uintptr_t sourceHighlight =
                                                FindPackageObjectByNameLocal(runtimeAddress, kReplayMoviesHighlight10);

                                            if (!IsReadableMemoryLocal(sourceButton, 0x58) ||
                                                !IsReadableMemoryLocal(sourceText, 0x6C) ||
                                                !IsReadableMemoryLocal(sourceHighlight, 0x58) ||
                                                *reinterpret_cast<const std::uintptr_t*>(sourceButton + 0x00) !=
                                                Addr::OptionControlVtable ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceButton + 0x10) !=
                                                0x787C1E59 ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceButton + 0x18) != 1 ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceText + 0x10) !=
                                                0x5E8FB1BE ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceText + 0x18) != 2 ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceHighlight + 0x10) !=
                                                kReplayMoviesHighlight10Hash ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceHighlight + 0x18) != 1)
                                            {
                                                return false;
                                            }

                                            g_ReplayMoviesSynthetic11BuildStage = 2;

                                            const std::uintptr_t button11 = reinterpret_cast<std::uintptr_t>(
                                                NativeCloneObjectLocal(sourceButton));
                                            const std::uintptr_t text11 = reinterpret_cast<std::uintptr_t>(
                                                NativeCloneObjectLocal(sourceText));
                                            const std::uintptr_t highlight11 = reinterpret_cast<std::uintptr_t>(
                                                NativeCloneObjectLocal(sourceHighlight));

                                            if (button11 == 0 || text11 == 0 || highlight11 == 0)
                                                return false;

                                            g_ReplayMoviesSynthetic11OptionName = button11;
                                            g_ReplayMoviesSynthetic11OptionText = text11;
                                            g_ReplayMoviesSynthetic11Highlight = highlight11;

                                            g_ReplayMoviesSynthetic11BuildStage = 3;

                                            if (!CopyClonePayloadLocal(sourceButton, button11) ||
                                                !CopyClonePayloadLocal(sourceText, text11) ||
                                                !CopyClonePayloadLocal(sourceHighlight, highlight11) ||
                                                !AssignSyntheticObjectIdentityLocal(
                                                    sourceButton, button11,
                                                    kReplayMoviesOptionName10, kReplayMoviesOptionName11,
                                                    kReplayMoviesOptionName11Hash) ||
                                                !AssignSyntheticObjectIdentityLocal(
                                                    sourceText, text11,
                                                    kReplayMoviesOptionText10, kReplayMoviesOptionText11,
                                                    kReplayMoviesOptionText11Hash) ||
                                                !AssignSyntheticObjectIdentityLocal(
                                                    sourceHighlight, highlight11,
                                                    kReplayMoviesHighlight10, kReplayMoviesHighlight11,
                                                    kReplayMoviesHighlight11Hash))
                                            {
                                                return false;
                                            }

                                            // Keep each clone on the exact equivalent authored current subordinate
                                            // state when such a state exists. This is generation-local and uses the
                                            // clone-owned chain; no source node pointer is shared.
                                            const std::uintptr_t buttonNode =
                                                FindEquivalentCurrentNodeLocal(button11, sourceButton);
                                            const std::uintptr_t textNode =
                                                FindEquivalentCurrentNodeLocal(text11, sourceText);
                                            const std::uintptr_t highlightNode =
                                                FindEquivalentCurrentNodeLocal(highlight11, sourceHighlight);

                                            if (buttonNode != 0)
                                                *reinterpret_cast<std::uintptr_t*>(button11 + 0x54) = buttonNode;
                                            if (textNode != 0)
                                                *reinterpret_cast<std::uintptr_t*>(text11 + 0x54) = textNode;
                                            if (highlightNode != 0)
                                                *reinterpret_cast<std::uintptr_t*>(highlight11 + 0x54) = highlightNode;

                                            // Row 11 is one native 30-unit pitch below authored row 10 for this
                                            // prototype. Shift the live payload and any clone-owned standard
                                            // geometry descriptors together so presentation state is not aliased
                                            // back onto row 10.
                                            ShiftSyntheticGeometryDescriptorsLocal(button11, kReplayMoviesSyntheticRowPitch);
                                            ShiftSyntheticGeometryDescriptorsLocal(text11, kReplayMoviesSyntheticRowPitch);
                                            ShiftSyntheticGeometryDescriptorsLocal(highlight11, kReplayMoviesSyntheticRowPitch);

                                            g_ReplayMoviesSynthetic11BuildStage = 4;

                                            auto* newVector = reinterpret_cast<std::uintptr_t*>(
                                                NativeGameAllocateLocal(
                                                    kReplayMoviesSynthetic11CatalogSlotCount * sizeof(std::uintptr_t)));

                                            if (newVector == nullptr)
                                                return false;

                                            // Native order is descending: [10..1]. Extend it to [11..1].
                                            newVector[0] = button11;
                                            std::memcpy(
                                                newVector + 1,
                                                reinterpret_cast<const void*>(oldVectorAddress),
                                                kReplayMoviesAuthoredCatalogSlotCount * sizeof(std::uintptr_t));

                                            g_ReplayMoviesSynthetic11BuildStage = 5;

                                            if (!AttachSyntheticObjectLocal(runtimeAddress, button11) ||
                                                !AttachSyntheticObjectLocal(runtimeAddress, text11) ||
                                                !AttachSyntheticObjectLocal(runtimeAddress, highlight11))
                                            {
                                                NativeGameFreeLocal(newVector);
                                                return false;
                                            }

                                            g_ReplayMoviesSynthetic11BuildStage = 6;

                                            *reinterpret_cast<std::uintptr_t*>(runtime + 0x84) =
                                                reinterpret_cast<std::uintptr_t>(newVector);
                                            *reinterpret_cast<std::uint32_t*>(runtime + 0x88) =
                                                static_cast<std::uint32_t>(kReplayMoviesSynthetic11CatalogSlotCount);

                                            // Runtime now owns the replacement vector using the same native heap
                                            // convention as the original Butn allocation. Deliberately leave the
                                            // superseded authored 10-pointer vector allocated in this prototype: the
                                            // debugger proved late vector replacement, but not that no completion
                                            // frame/local still retains the old pointer. The 40-byte generation-local
                                            // leak is preferable to introducing an unproven use-after-free. Final
                                            // 17-row production code can move the enlargement into native allocation
                                            // time or free only after the lifetime boundary is explicitly proven.

                                            g_ReplayMoviesSynthetic11BuildStage = 7;

                                            if (*reinterpret_cast<const std::uintptr_t*>(runtime + 0x84) !=
                                                reinterpret_cast<std::uintptr_t>(newVector) ||
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88) !=
                                                kReplayMoviesSynthetic11CatalogSlotCount ||
                                                newVector[0] != button11 ||
                                                newVector[1] != sourceButton)
                                            {
                                                return false;
                                            }

                                            ++g_ReplayMoviesSynthetic11BuildSuccesses;
                                            g_ReplayMoviesSynthetic11BuildStage = 8;
                                            return true;
                                        }


                                        static bool EnsureReplayMoviesSyntheticSlots12To17(
                                            std::uintptr_t runtimeAddress)
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C))
                                            {
                                                return false;
                                            }

                                            auto* runtime = reinterpret_cast<std::uint8_t*>(runtimeAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(runtime + 0x10) !=
                                                Addr::TrailersRuntimeHash)
                                            {
                                                return false;
                                            }

                                            const std::uint32_t existingCount =
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88);

                                            if (existingCount == kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                // A completed current generation must own every synthetic family.
                                                for (std::size_t slotIndex = kReplayMoviesAuthoredCatalogSlotCount;
                                                    slotIndex < kReplayMoviesNativeCatalogSlotCount;
                                                    ++slotIndex)
                                                {
                                                    if (FindPackageObjectByNameLocal(
                                                        runtimeAddress,
                                                        kReplayMoviesOptionControlNames[slotIndex]) == 0 ||
                                                        FindPackageObjectByNameLocal(
                                                            runtimeAddress,
                                                            kReplayMoviesOptionTextNames[slotIndex]) == 0 ||
                                                        FindPackageObjectByNameLocal(
                                                            runtimeAddress,
                                                            kReplayMoviesHighlightNames[slotIndex]) == 0)
                                                    {
                                                        return false;
                                                    }
                                                }

                                                return true;
                                            }

                                            if (existingCount != kReplayMoviesSynthetic11CatalogSlotCount)
                                                return false;

                                            const std::uintptr_t oldVectorAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(runtime + 0x84);

                                            if (oldVectorAddress == 0 ||
                                                !IsReadableMemoryLocal(
                                                    oldVectorAddress,
                                                    kReplayMoviesSynthetic11CatalogSlotCount *
                                                    sizeof(std::uintptr_t)))
                                            {
                                                return false;
                                            }

                                            const auto* oldCandidates =
                                                reinterpret_cast<const std::uintptr_t*>(oldVectorAddress);

                                            // After the row-11 stage, vector order is [11..1]. Use those
                                            // independently owned row-11 objects as the exact templates proven
                                            // in the debugger for rows 12..17.
                                            const std::uintptr_t sourceButton = oldCandidates[0];
                                            const std::uintptr_t sourceText =
                                                FindPackageObjectByNameLocal(runtimeAddress, kReplayMoviesOptionText11);
                                            const std::uintptr_t sourceHighlight =
                                                FindPackageObjectByNameLocal(runtimeAddress, kReplayMoviesHighlight11);

                                            if (!IsReadableMemoryLocal(sourceButton, 0x58) ||
                                                !IsReadableMemoryLocal(sourceText, 0x58) ||
                                                !IsReadableMemoryLocal(sourceHighlight, 0x58) ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceButton + 0x10) !=
                                                kReplayMoviesOptionName11Hash ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceButton + 0x18) != 1 ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceText + 0x10) !=
                                                kReplayMoviesOptionText11Hash ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceText + 0x18) != 2 ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceHighlight + 0x10) !=
                                                kReplayMoviesHighlight11Hash ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceHighlight + 0x18) != 1 ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceButton + 0x30) != 0x54 ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceText + 0x30) != 0x44 ||
                                                *reinterpret_cast<const std::uint32_t*>(sourceHighlight + 0x30) != 0x54)
                                            {
                                                return false;
                                            }

                                            constexpr std::size_t kAdditionalSyntheticRows =
                                                kReplayMoviesNativeCatalogSlotCount -
                                                kReplayMoviesSynthetic11CatalogSlotCount;

                                            std::uintptr_t buttons[kAdditionalSyntheticRows]{};
                                            std::uintptr_t texts[kAdditionalSyntheticRows]{};
                                            std::uintptr_t highlights[kAdditionalSyntheticRows]{};

                                            for (std::size_t syntheticIndex = 0;
                                                syntheticIndex < kAdditionalSyntheticRows;
                                                ++syntheticIndex)
                                            {
                                                const std::size_t slotIndex =
                                                    kReplayMoviesSynthetic11CatalogSlotCount + syntheticIndex;

                                                buttons[syntheticIndex] = reinterpret_cast<std::uintptr_t>(
                                                    NativeCloneObjectLocal(sourceButton));
                                                texts[syntheticIndex] = reinterpret_cast<std::uintptr_t>(
                                                    NativeCloneObjectLocal(sourceText));
                                                highlights[syntheticIndex] = reinterpret_cast<std::uintptr_t>(
                                                    NativeCloneObjectLocal(sourceHighlight));

                                                if (buttons[syntheticIndex] == 0 ||
                                                    texts[syntheticIndex] == 0 ||
                                                    highlights[syntheticIndex] == 0)
                                                {
                                                    return false;
                                                }

                                                if (!CopyClonePayloadLocal(sourceButton, buttons[syntheticIndex]) ||
                                                    !CopyClonePayloadLocal(sourceText, texts[syntheticIndex]) ||
                                                    !CopyClonePayloadLocal(sourceHighlight, highlights[syntheticIndex]) ||
                                                    !AssignSyntheticObjectIdentityLocal(
                                                        sourceButton,
                                                        buttons[syntheticIndex],
                                                        kReplayMoviesOptionName11,
                                                        kReplayMoviesOptionControlNames[slotIndex],
                                                        kReplayMoviesOptionControlHashes[slotIndex]) ||
                                                    !AssignSyntheticObjectIdentityLocal(
                                                        sourceText,
                                                        texts[syntheticIndex],
                                                        kReplayMoviesOptionText11,
                                                        kReplayMoviesOptionTextNames[slotIndex],
                                                        kReplayMoviesOptionTextHashes[slotIndex]) ||
                                                    !AssignSyntheticObjectIdentityLocal(
                                                        sourceHighlight,
                                                        highlights[syntheticIndex],
                                                        kReplayMoviesHighlight11,
                                                        kReplayMoviesHighlightNames[slotIndex],
                                                        kReplayMoviesHighlightHashes[slotIndex]))
                                                {
                                                    return false;
                                                }

                                                const std::uintptr_t buttonNode =
                                                    FindEquivalentCurrentNodeLocal(
                                                        buttons[syntheticIndex], sourceButton);
                                                const std::uintptr_t textNode =
                                                    FindEquivalentCurrentNodeLocal(
                                                        texts[syntheticIndex], sourceText);
                                                const std::uintptr_t highlightNode =
                                                    FindEquivalentCurrentNodeLocal(
                                                        highlights[syntheticIndex], sourceHighlight);

                                                if (buttonNode != 0)
                                                    *reinterpret_cast<std::uintptr_t*>(
                                                        buttons[syntheticIndex] + 0x54) = buttonNode;
                                                if (textNode != 0)
                                                    *reinterpret_cast<std::uintptr_t*>(
                                                        texts[syntheticIndex] + 0x54) = textNode;
                                                if (highlightNode != 0)
                                                    *reinterpret_cast<std::uintptr_t*>(
                                                        highlights[syntheticIndex] + 0x54) = highlightNode;

                                                // Give each clone a distinct generation-local geometry immediately.
                                                // The exact compact grid is applied below before native execution
                                                // resumes; this intermediate shift only prevents row-11 aliasing.
                                                const float deltaY =
                                                    kReplayMoviesSyntheticRowPitch *
                                                    static_cast<float>(syntheticIndex + 1);

                                                ShiftSyntheticGeometryDescriptorsLocal(
                                                    buttons[syntheticIndex], deltaY);
                                                ShiftSyntheticGeometryDescriptorsLocal(
                                                    texts[syntheticIndex], deltaY);
                                                ShiftSyntheticGeometryDescriptorsLocal(
                                                    highlights[syntheticIndex], deltaY);
                                            }

                                            auto* newVector = reinterpret_cast<std::uintptr_t*>(
                                                NativeGameAllocateLocal(
                                                    kReplayMoviesNativeCatalogSlotCount * sizeof(std::uintptr_t)));

                                            if (newVector == nullptr)
                                                return false;

                                            // Final debugger-proven order is descending:
                                            // [17,16,15,14,13,12,11,10,...,1].
                                            for (std::size_t syntheticIndex = 0;
                                                syntheticIndex < kAdditionalSyntheticRows;
                                                ++syntheticIndex)
                                            {
                                                newVector[(kAdditionalSyntheticRows - 1) - syntheticIndex] =
                                                    buttons[syntheticIndex];
                                            }

                                            std::memcpy(
                                                newVector + kAdditionalSyntheticRows,
                                                reinterpret_cast<const void*>(oldVectorAddress),
                                                kReplayMoviesSynthetic11CatalogSlotCount *
                                                sizeof(std::uintptr_t));

                                            // Match the successful debugger attachment transaction exactly:
                                            // B12 -> T12 -> H12 -> ... -> B17 -> T17 -> H17.
                                            for (std::size_t syntheticIndex = 0;
                                                syntheticIndex < kAdditionalSyntheticRows;
                                                ++syntheticIndex)
                                            {
                                                if (!AttachSyntheticObjectLocal(
                                                    runtimeAddress, buttons[syntheticIndex]) ||
                                                    !AttachSyntheticObjectLocal(
                                                        runtimeAddress, texts[syntheticIndex]) ||
                                                    !AttachSyntheticObjectLocal(
                                                        runtimeAddress, highlights[syntheticIndex]))
                                                {
                                                    NativeGameFreeLocal(newVector);
                                                    return false;
                                                }
                                            }

                                            *reinterpret_cast<std::uintptr_t*>(runtime + 0x84) =
                                                reinterpret_cast<std::uintptr_t>(newVector);
                                            *reinterpret_cast<std::uint32_t*>(runtime + 0x88) =
                                                static_cast<std::uint32_t>(kReplayMoviesNativeCatalogSlotCount);

                                            // As with the proven 10->11 stage, retain the superseded vector for
                                            // the generation. Its lifetime beyond the completed construction frame
                                            // was never debugger-proven safe to free.
                                            if (*reinterpret_cast<const std::uintptr_t*>(runtime + 0x84) !=
                                                reinterpret_cast<std::uintptr_t>(newVector) ||
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88) !=
                                                kReplayMoviesNativeCatalogSlotCount ||
                                                newVector[0] != buttons[kAdditionalSyntheticRows - 1] ||
                                                newVector[kAdditionalSyntheticRows - 1] != buttons[0] ||
                                                newVector[kAdditionalSyntheticRows] != sourceButton)
                                            {
                                                return false;
                                            }

                                            return true;
                                        }


                                        static bool ApplyReplayMoviesCompactOnePageGeometryLocal(
                                            std::uintptr_t runtimeAddress)
                                        {
                                            if (!IsReadableMemoryLocal(runtimeAddress, 0x8C) ||
                                                *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x88) !=
                                                kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return false;
                                            }

                                            for (std::size_t slotIndex = 0;
                                                slotIndex < kReplayMoviesNativeCatalogSlotCount;
                                                ++slotIndex)
                                            {
                                                const std::uintptr_t button =
                                                    FindPackageObjectByNameLocal(
                                                        runtimeAddress,
                                                        kReplayMoviesOptionControlNames[slotIndex]);
                                                const std::uintptr_t text =
                                                    FindPackageObjectByNameLocal(
                                                        runtimeAddress,
                                                        kReplayMoviesOptionTextNames[slotIndex]);
                                                const std::uintptr_t highlight =
                                                    FindPackageObjectByNameLocal(
                                                        runtimeAddress,
                                                        kReplayMoviesHighlightNames[slotIndex]);

                                                if (!IsReadableMemoryLocal(button, 0x34) ||
                                                    !IsReadableMemoryLocal(text, 0x34) ||
                                                    !IsReadableMemoryLocal(highlight, 0x34) ||
                                                    *reinterpret_cast<const std::uint32_t*>(button + 0x10) !=
                                                    kReplayMoviesOptionControlHashes[slotIndex] ||
                                                    *reinterpret_cast<const std::uint32_t*>(text + 0x10) !=
                                                    kReplayMoviesOptionTextHashes[slotIndex] ||
                                                    *reinterpret_cast<const std::uint32_t*>(highlight + 0x10) !=
                                                    kReplayMoviesHighlightHashes[slotIndex])
                                                {
                                                    return false;
                                                }

                                                const std::uintptr_t buttonPayload =
                                                    *reinterpret_cast<const std::uintptr_t*>(button + 0x2C);
                                                const std::uintptr_t textPayload =
                                                    *reinterpret_cast<const std::uintptr_t*>(text + 0x2C);
                                                const std::uintptr_t highlightPayload =
                                                    *reinterpret_cast<const std::uintptr_t*>(highlight + 0x2C);

                                                if (*reinterpret_cast<const std::uint32_t*>(button + 0x30) != 0x54 ||
                                                    *reinterpret_cast<const std::uint32_t*>(text + 0x30) != 0x44 ||
                                                    *reinterpret_cast<const std::uint32_t*>(highlight + 0x30) != 0x54 ||
                                                    !IsReadableMemoryLocal(buttonPayload, 0x54) ||
                                                    !IsReadableMemoryLocal(textPayload, 0x44) ||
                                                    !IsReadableMemoryLocal(highlightPayload, 0x54))
                                                {
                                                    return false;
                                                }

                                                *reinterpret_cast<std::uint32_t*>(buttonPayload + 0x20) =
                                                    kReplayMoviesOptionY[slotIndex];
                                                *reinterpret_cast<std::uint32_t*>(buttonPayload + 0x3C) =
                                                    kReplayMoviesOptionHeight;

                                                *reinterpret_cast<std::uint32_t*>(textPayload + 0x20) =
                                                    kReplayMoviesTextY[slotIndex];
                                                *reinterpret_cast<std::uint32_t*>(textPayload + 0x38) =
                                                    kReplayMoviesTextScale;
                                                *reinterpret_cast<std::uint32_t*>(textPayload + 0x3C) =
                                                    kReplayMoviesTextScale;

                                                *reinterpret_cast<std::uint32_t*>(highlightPayload + 0x20) =
                                                    kReplayMoviesHighlightY[slotIndex];
                                                *reinterpret_cast<std::uint32_t*>(highlightPayload + 0x3C) =
                                                    kReplayMoviesHighlightHeight;
                                            }

                                            return true;
                                        }


                                        static bool AcquireReplayMoviesCatalogObjects(
                                            std::uintptr_t runtimeAddress,
                                            std::uintptr_t* buttons,
                                            std::uintptr_t* texts,
                                            std::uintptr_t* textBuffers)
                                        {
                                            if (!IsReadableMemoryLocal(runtimeAddress, 0x8C) ||
                                                buttons == nullptr ||
                                                texts == nullptr ||
                                                textBuffers == nullptr)
                                            {
                                                return false;
                                            }

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(runtimeAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                runtime + 0x10) != Addr::TrailersRuntimeHash ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    runtime + 0x88) != kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t vectorAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    runtime + 0x84);

                                            if (vectorAddress == 0 ||
                                                !IsReadableMemoryLocal(
                                                    vectorAddress,
                                                    kReplayMoviesNativeCatalogSlotCount *
                                                    sizeof(std::uintptr_t)))
                                            {
                                                return false;
                                            }

                                            const auto* candidates =
                                                reinterpret_cast<const std::uintptr_t*>(
                                                    vectorAddress);

                                            for (std::size_t slotIndex = 0;
                                                slotIndex < kReplayMoviesNativeCatalogSlotCount;
                                                ++slotIndex)
                                            {
                                                const std::size_t candidateIndex =
                                                    (kReplayMoviesNativeCatalogSlotCount - 1) - slotIndex;

                                                const std::uintptr_t button =
                                                    candidates[candidateIndex];

                                                if (!IsReadableMemoryLocal(button, 0x58) ||
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        button + 0x00) != Addr::OptionControlVtable ||
                                                    *reinterpret_cast<const std::uint32_t*>(
                                                        button + 0x18) != 1)
                                                {
                                                    return false;
                                                }

                                                const std::uintptr_t nameAddress =
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        button + 0x14);

                                                if (nameAddress == 0 ||
                                                    !AsciiStringEqualsLocal(
                                                        reinterpret_cast<const char*>(nameAddress),
                                                        kReplayMoviesOptionControlNames[slotIndex]))
                                                {
                                                    return false;
                                                }

                                                const std::uintptr_t text =
                                                    FindPackageObjectByNameLocal(
                                                        runtimeAddress,
                                                        kReplayMoviesOptionTextNames[slotIndex]);

                                                if (!IsReadableMemoryLocal(text, 0x6C) ||
                                                    *reinterpret_cast<const std::uint32_t*>(
                                                        text + 0x18) != 2)
                                                {
                                                    return false;
                                                }

                                                const std::uintptr_t textBuffer =
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        text + 0x60);

                                                if (textBuffer == 0 ||
                                                    !IsReadableMemoryLocal(
                                                        textBuffer,
                                                        sizeof(wchar_t)))
                                                {
                                                    return false;
                                                }

                                                buttons[slotIndex] = button;
                                                texts[slotIndex] = text;
                                                textBuffers[slotIndex] = textBuffer;
                                            }

                                            return true;
                                        }


                                        static void ResetReplayMoviesUserProjectionLocal()
                                        {
                                            g_ReplayMoviesUserProjectionActive = 0;
                                            g_ReplayMoviesUserProjectionRuntime = 0;
                                            g_ReplayMoviesUserPublishedCandidateVector = 0;
                                            g_ReplayMoviesUserVisibleCount = 0;
                                            g_ReplayMoviesUserSeenMask = 0;

                                            for (std::size_t index = 0;
                                                index < kReplayMoviesNativeCatalogSlotCount;
                                                ++index)
                                            {
                                                g_ReplayMoviesUserCandidateVector[index] = 0;
                                                g_ReplayMoviesUserCanonicalByPhysical[index] =
                                                    kReplayMoviesInvalidCanonicalIndex;
                                            }
                                        }


                                        static bool IsReplayMoviesUserProjectionPublishedLocal(
                                            std::uintptr_t runtimeAddress)
                                        {
                                            if (g_ReplayMoviesUserProjectionActive == 0 ||
                                                runtimeAddress == 0 ||
                                                g_ReplayMoviesUserProjectionRuntime != runtimeAddress)
                                            {
                                                return false;
                                            }

                                            const std::size_t visibleCount =
                                                static_cast<std::size_t>(g_ReplayMoviesUserVisibleCount);

                                            if (visibleCount < kReplayMoviesUserMinimumProvenVisibleCount ||
                                                visibleCount > kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return false;
                                            }

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(runtimeAddress);

                                            return
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x10) ==
                                                Addr::TrailersRuntimeHash &&
                                                g_ReplayMoviesUserPublishedCandidateVector != 0 &&
                                                *reinterpret_cast<const std::uintptr_t*>(runtime + 0x84) ==
                                                g_ReplayMoviesUserPublishedCandidateVector &&
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88) ==
                                                visibleCount;
                                        }


                                        static std::size_t GetReplayMoviesUserPhysicalSlotForCanonicalLocal(
                                            std::size_t canonicalIndex)
                                        {
                                            if (canonicalIndex >= ReplayMoviesCatalog::kCareerMovieCount ||
                                                g_ReplayMoviesUserProjectionActive == 0)
                                            {
                                                return 0;
                                            }

                                            const std::size_t visibleCount =
                                                static_cast<std::size_t>(g_ReplayMoviesUserVisibleCount);

                                            for (std::size_t physicalIndex = 0;
                                                physicalIndex < visibleCount;
                                                ++physicalIndex)
                                            {
                                                if (g_ReplayMoviesUserCanonicalByPhysical[physicalIndex] ==
                                                    canonicalIndex)
                                                {
                                                    return physicalIndex + 1;
                                                }
                                            }

                                            return 0;
                                        }


                                        static std::size_t GetReplayMoviesCanonicalIndexForPhysicalSlotLocal(
                                            std::size_t physicalSlot)
                                        {
                                            if (physicalSlot == 0 ||
                                                physicalSlot > kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return ReplayMoviesCatalog::kCareerMovieCount;
                                            }

                                            if (ReplayMoviesConfig::GetAccessMode() ==
                                                ReplayMoviesConfig::AccessMode::User &&
                                                g_ReplayMoviesUserProjectionActive != 0)
                                            {
                                                const std::size_t visibleCount =
                                                    static_cast<std::size_t>(
                                                        g_ReplayMoviesUserVisibleCount);

                                                if (physicalSlot > visibleCount)
                                                    return ReplayMoviesCatalog::kCareerMovieCount;

                                                const unsigned long canonicalIndex =
                                                    g_ReplayMoviesUserCanonicalByPhysical[physicalSlot - 1];

                                                return canonicalIndex < ReplayMoviesCatalog::kCareerMovieCount
                                                    ? static_cast<std::size_t>(canonicalIndex)
                                                    : ReplayMoviesCatalog::kCareerMovieCount;
                                            }

                                            return physicalSlot - 1;
                                        }


                                        static std::size_t GetReplayMoviesPhysicalSlotForCanonicalLocal(
                                            std::size_t canonicalIndex)
                                        {
                                            if (canonicalIndex >= ReplayMoviesCatalog::kCareerMovieCount)
                                                return 0;

                                            if (ReplayMoviesConfig::GetAccessMode() ==
                                                ReplayMoviesConfig::AccessMode::User &&
                                                g_ReplayMoviesUserProjectionActive != 0)
                                            {
                                                return GetReplayMoviesUserPhysicalSlotForCanonicalLocal(
                                                    canonicalIndex);
                                            }

                                            return canonicalIndex + 1;
                                        }


                                        static std::size_t GetReplayMoviesCatalogSlotNumber(
                                            std::uintptr_t runtimeAddress,
                                            std::uintptr_t rowAddress)
                                        {
                                            if (rowAddress == 0 ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C))
                                            {
                                                return 0;
                                            }

                                            // The generation-local cache is built while the native full 17-row vector
                                            // still exists. Keep using it after User mode redirects Runtime+84/+88 to
                                            // the compact candidate vector; physical-shell identity must not depend on
                                            // the currently published candidate count.
                                            if (g_ReplayMoviesPresentationCacheValid != 0 &&
                                                g_ReplayMoviesPresentationCacheRuntime == runtimeAddress)
                                            {
                                                for (std::size_t slotIndex = 0;
                                                    slotIndex < kReplayMoviesNativeCatalogSlotCount;
                                                    ++slotIndex)
                                                {
                                                    if (g_ReplayMoviesPresentationCache[slotIndex].row == rowAddress)
                                                        return slotIndex + 1;
                                                }
                                            }

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(runtimeAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(runtime + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88) !=
                                                kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return 0;
                                            }

                                            const std::uintptr_t vectorAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(runtime + 0x84);

                                            if (vectorAddress == 0 ||
                                                !IsReadableMemoryLocal(
                                                    vectorAddress,
                                                    kReplayMoviesNativeCatalogSlotCount *
                                                    sizeof(std::uintptr_t)))
                                            {
                                                return 0;
                                            }

                                            const auto* candidates =
                                                reinterpret_cast<const std::uintptr_t*>(vectorAddress);

                                            for (std::size_t slotNumber = 1;
                                                slotNumber <= kReplayMoviesNativeCatalogSlotCount;
                                                ++slotNumber)
                                            {
                                                const std::size_t candidateIndex =
                                                    kReplayMoviesNativeCatalogSlotCount - slotNumber;

                                                if (candidates[candidateIndex] == rowAddress)
                                                    return slotNumber;
                                            }

                                            return 0;
                                        }


                                        static std::uintptr_t GetReplayMoviesCatalogSlotAddress(
                                            std::uintptr_t runtimeAddress,
                                            std::size_t slotNumber)
                                        {
                                            if (slotNumber == 0 ||
                                                slotNumber > kReplayMoviesNativeCatalogSlotCount ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C))
                                            {
                                                return 0;
                                            }

                                            if (g_ReplayMoviesPresentationCacheValid != 0 &&
                                                g_ReplayMoviesPresentationCacheRuntime == runtimeAddress)
                                            {
                                                return g_ReplayMoviesPresentationCache[slotNumber - 1].row;
                                            }

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(runtimeAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(runtime + 0x88) !=
                                                kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return 0;
                                            }

                                            const std::uintptr_t vectorAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(runtime + 0x84);

                                            if (vectorAddress == 0 ||
                                                !IsReadableMemoryLocal(
                                                    vectorAddress,
                                                    kReplayMoviesNativeCatalogSlotCount *
                                                    sizeof(std::uintptr_t)))
                                            {
                                                return 0;
                                            }

                                            const auto* candidates =
                                                reinterpret_cast<const std::uintptr_t*>(vectorAddress);

                                            return candidates[
                                                kReplayMoviesNativeCatalogSlotCount - slotNumber];
                                        }


                                        static std::uintptr_t GetReplayMoviesPresentationNodeLocal(
                                            std::uintptr_t objectAddress,
                                            bool selected)
                                        {
                                            if (!IsReadableMemoryLocal(objectAddress, 0x58))
                                                return 0;

                                            const std::uintptr_t root =
                                                *reinterpret_cast<const std::uintptr_t*>(objectAddress + 0x4C);

                                            if (!IsReadableMemoryLocal(root, 0x08))
                                                return 0;

                                            const std::uintptr_t n1 =
                                                *reinterpret_cast<const std::uintptr_t*>(root + 0x04);

                                            if (!IsReadableMemoryLocal(n1, 0x14))
                                                return 0;

                                            if (selected)
                                                return n1;

                                            const std::uintptr_t n2 =
                                                *reinterpret_cast<const std::uintptr_t*>(n1 + 0x04);

                                            if (!IsReadableMemoryLocal(n2, 0x14))
                                                return 0;

                                            return n2;
                                        }


                                        static void ResetReplayMoviesPresentationCacheLocal()
                                        {
                                            g_ReplayMoviesPresentationCacheValid = 0;
                                            g_ReplayMoviesPresentationCacheRuntime = 0;
                                            g_ReplayMoviesPresentationLastRuntime = 0;
                                            g_ReplayMoviesPresentationLastCurrentRow = 0;
                                        }


                                        static bool BuildReplayMoviesPresentationCacheLocal(
                                            std::uintptr_t runtimeAddress)
                                        {
                                            ResetReplayMoviesPresentationCacheLocal();

                                            if (!IsReadableMemoryLocal(runtimeAddress, 0x8C) ||
                                                *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x88) !=
                                                kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return false;
                                            }

                                            ReplayMoviesPresentationCacheEntry
                                                staged[kReplayMoviesNativeCatalogSlotCount]{};

                                            for (std::size_t slotNumber = 1;
                                                slotNumber <= kReplayMoviesNativeCatalogSlotCount;
                                                ++slotNumber)
                                            {
                                                const std::size_t slotIndex = slotNumber - 1;
                                                const std::uintptr_t row =
                                                    GetReplayMoviesCatalogSlotAddress(runtimeAddress, slotNumber);
                                                const std::uintptr_t text =
                                                    FindPackageObjectByNameLocal(
                                                        runtimeAddress,
                                                        kReplayMoviesOptionTextNames[slotIndex]);
                                                const std::uintptr_t highlight =
                                                    FindPackageObjectByNameLocal(
                                                        runtimeAddress,
                                                        kReplayMoviesHighlightNames[slotIndex]);

                                                if (!IsReadableMemoryLocal(row, 0x1C) ||
                                                    !IsReadableMemoryLocal(text, 0x58) ||
                                                    !IsReadableMemoryLocal(highlight, 0x58) ||
                                                    *reinterpret_cast<const std::uint32_t*>(row + 0x10) !=
                                                    kReplayMoviesOptionControlHashes[slotIndex] ||
                                                    *reinterpret_cast<const std::uint32_t*>(row + 0x18) != 1 ||
                                                    *reinterpret_cast<const std::uint32_t*>(text + 0x10) !=
                                                    kReplayMoviesOptionTextHashes[slotIndex] ||
                                                    *reinterpret_cast<const std::uint32_t*>(text + 0x18) != 2 ||
                                                    *reinterpret_cast<const std::uint32_t*>(highlight + 0x10) !=
                                                    kReplayMoviesHighlightHashes[slotIndex] ||
                                                    *reinterpret_cast<const std::uint32_t*>(highlight + 0x18) != 1)
                                                {
                                                    return false;
                                                }

                                                const std::uintptr_t textSelected =
                                                    GetReplayMoviesPresentationNodeLocal(text, true);
                                                const std::uintptr_t textDormant =
                                                    GetReplayMoviesPresentationNodeLocal(text, false);
                                                const std::uintptr_t highlightSelected =
                                                    GetReplayMoviesPresentationNodeLocal(highlight, true);
                                                const std::uintptr_t highlightDormant =
                                                    GetReplayMoviesPresentationNodeLocal(highlight, false);

                                                if (textSelected == 0 ||
                                                    textDormant == 0 ||
                                                    highlightSelected == 0 ||
                                                    highlightDormant == 0)
                                                {
                                                    return false;
                                                }

                                                staged[slotIndex].row = row;
                                                staged[slotIndex].text = text;
                                                staged[slotIndex].textSelected = textSelected;
                                                staged[slotIndex].textDormant = textDormant;
                                                staged[slotIndex].highlight = highlight;
                                                staged[slotIndex].highlightSelected = highlightSelected;
                                                staged[slotIndex].highlightDormant = highlightDormant;
                                            }

                                            for (std::size_t slotIndex = 0;
                                                slotIndex < kReplayMoviesNativeCatalogSlotCount;
                                                ++slotIndex)
                                            {
                                                g_ReplayMoviesPresentationCache[slotIndex] = staged[slotIndex];
                                            }

                                            g_ReplayMoviesPresentationCacheRuntime = runtimeAddress;
                                            g_ReplayMoviesPresentationCacheValid = 1;
                                            return true;
                                        }


                                        __declspec(noinline)
                                            static void ApplyReplayMoviesCachedPresentationNodeLocal(
                                                std::uintptr_t objectAddress,
                                                std::uintptr_t nodeAddress)
                                        {
                                            // Hot-path variant of the proven 004F1116 transaction.  Object/node
                                            // readability and graph identity were validated once when this generation's
                                            // presentation cache was built, so do not repeat VirtualQuery here.
                                            __asm
                                            {
                                                mov eax, objectAddress
                                                mov esi, nodeAddress
                                                push esi
                                                push eax
                                                mov dword ptr[eax + 54h], esi
                                                call dword ptr[g_NativePostActiveNodeWrite]
                                                pop esi
                                                mov dword ptr[esi + 10h], 0
                                            }
                                        }


                                        __declspec(noinline)
                                            static void ApplyReplayMoviesPresentationNodeLocal(
                                                std::uintptr_t objectAddress,
                                                std::uintptr_t nodeAddress)
                                        {
                                            if (!IsReadableMemoryLocal(objectAddress, 0x58) ||
                                                !IsReadableMemoryLocal(nodeAddress, 0x14))
                                            {
                                                return;
                                            }

                                            // Exact native presentation transaction around 004F1116:
                                            // parent is pushed, +54 receives the semantic node, 004FF030 applies
                                            // presentation, then node+10 is cleared. This is the transaction used
                                            // by the debugger-proven post-hook visual synchronizer.
                                            __asm
                                            {
                                                mov eax, objectAddress
                                                mov esi, nodeAddress
                                                push esi
                                                push eax
                                                mov dword ptr[eax + 54h], esi
                                                call dword ptr[g_NativePostActiveNodeWrite]
                                                pop esi
                                                mov dword ptr[esi + 10h], 0
                                            }
                                        }


                                        static void __cdecl SynchronizeReplayMoviesPostHookVisualsLocal(
                                            std::uintptr_t runtimeAddress,
                                            bool forceApply)
                                        {
                                            // v11.5 debugger-proven replacement for the old graph-scanning
                                            // synchronizer. Ordinary navigation must do only fixed-address work:
                                            // no package object-name searches and no node-graph/VirtualQuery walks.
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                runtimeAddress == 0 ||
                                                runtimeAddress != g_ReplayMoviesCatalogRuntimeAddress ||
                                                g_ReplayMoviesPresentationCacheValid == 0 ||
                                                g_ReplayMoviesPresentationCacheRuntime != runtimeAddress)
                                            {
                                                return;
                                            }

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(runtimeAddress);

                                            const std::uint32_t candidateCount =
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88);

                                            if (*reinterpret_cast<const std::uint32_t*>(runtime + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                (candidateCount != kReplayMoviesNativeCatalogSlotCount &&
                                                    !IsReplayMoviesUserProjectionPublishedLocal(runtimeAddress)))
                                            {
                                                ResetReplayMoviesPresentationCacheLocal();
                                                return;
                                            }

                                            const std::uintptr_t currentRow =
                                                *reinterpret_cast<const std::uintptr_t*>(runtime + 0x80);

                                            // Match the successful debugger dispatcher exactly: the full cached
                                            // synthetic-row synchronization runs once when Runtime+80 changes. Same-row
                                            // 004F1116 writes perform only the Row10 dormant repair below. forceApply
                                            // is reserved for construction/post-FMV one-shot synchronization.
                                            const bool currentChanged =
                                                forceApply ||
                                                g_ReplayMoviesPresentationLastRuntime != runtimeAddress ||
                                                g_ReplayMoviesPresentationLastCurrentRow != currentRow;

                                            if (currentChanged)
                                            {
                                                g_ReplayMoviesPresentationLastRuntime = runtimeAddress;
                                                g_ReplayMoviesPresentationLastCurrentRow = currentRow;

                                                // Rows11..17 are plugin-created physical rows. Their Text state can be
                                                // switched directly; their Highlight state requires the native
                                                // 004FF030 presentation transaction, but only when +54 changes. If the
                                                // current row is native 1..10, all seven synthetic companions become N2.
                                                for (std::size_t slotNumber =
                                                    kReplayMoviesAuthoredCatalogSlotCount + 1;
                                                    slotNumber <= kReplayMoviesNativeCatalogSlotCount;
                                                    ++slotNumber)
                                                {
                                                    const std::size_t slotIndex = slotNumber - 1;
                                                    const auto& cached =
                                                        g_ReplayMoviesPresentationCache[slotIndex];
                                                    const bool selected = currentRow == cached.row;
                                                    const std::uintptr_t textNode = selected
                                                        ? cached.textSelected
                                                        : cached.textDormant;
                                                    const std::uintptr_t highlightNode = selected
                                                        ? cached.highlightSelected
                                                        : cached.highlightDormant;

                                                    if (forceApply ||
                                                        *reinterpret_cast<const std::uintptr_t*>(cached.text + 0x54) !=
                                                        textNode)
                                                    {
                                                        if (forceApply)
                                                        {
                                                            // Preserve the established one-shot force semantics used
                                                            // by bootstrap/post-FMV restoration. Ordinary navigation
                                                            // never enters this native Text transaction.
                                                            ApplyReplayMoviesCachedPresentationNodeLocal(
                                                                cached.text,
                                                                textNode);
                                                        }
                                                        else
                                                        {
                                                            *reinterpret_cast<std::uintptr_t*>(cached.text + 0x54) =
                                                                textNode;
                                                            *reinterpret_cast<std::uint32_t*>(textNode + 0x10) = 0;
                                                        }
                                                    }

                                                    if (forceApply ||
                                                        *reinterpret_cast<const std::uintptr_t*>(
                                                            cached.highlight + 0x54) != highlightNode)
                                                    {
                                                        ApplyReplayMoviesCachedPresentationNodeLocal(
                                                            cached.highlight,
                                                            highlightNode);
                                                    }
                                                }
                                            }

                                            // Exact debugger-proven native/synthetic boundary repair. Row10 remains
                                            // native while selected. Whenever current is anything else, stale Row10
                                            // Text is demoted by direct +54/N2 state only, while stale Row10 Highlight
                                            // receives the native 004FF030 transaction. This is what closed 10<->11
                                            // and the full 1<->17 ownership boundary without reintroducing double audio.
                                            const auto& row10 =
                                                g_ReplayMoviesPresentationCache[
                                                    kReplayMoviesAuthoredCatalogSlotCount - 1];

                                            if (currentRow != row10.row)
                                            {
                                                if (forceApply ||
                                                    *reinterpret_cast<const std::uintptr_t*>(row10.text + 0x54) !=
                                                    row10.textDormant)
                                                {
                                                    *reinterpret_cast<std::uintptr_t*>(row10.text + 0x54) =
                                                        row10.textDormant;
                                                    *reinterpret_cast<std::uint32_t*>(
                                                        row10.textDormant + 0x10) = 0;
                                                }

                                                if (forceApply ||
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        row10.highlight + 0x54) != row10.highlightDormant)
                                                {
                                                    ApplyReplayMoviesCachedPresentationNodeLocal(
                                                        row10.highlight,
                                                        row10.highlightDormant);
                                                }
                                            }
                                        }


                                        static void __cdecl
                                            MaintainReplayMoviesSyntheticPostReturnVirtualDemotionLocal(
                                                std::uintptr_t runtimeAddress)
                                        {
                                            // Exact debugger-proven v11.4 timing rule:
                                            //   native 004F1116 -> native 004FF030
                                            //   -> existing rows10..17 synchronizer
                                            //   -> re-demote the mapped virtual native row.
                                            //
                                            // The native frontend can repeat the 004F1116 transaction several times
                                            // after the completed 17-row handoff.  The debugger tail shim proved that
                                            // applying this N2 transaction after EACH such post-hook write keeps the
                                            // physical synthetic row as the sole selection and preserves immediate
                                            // native Up/Down navigation.
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                runtimeAddress == 0 ||
                                                runtimeAddress != g_ReplayMoviesCatalogRuntimeAddress ||
                                                runtimeAddress != g_ReplayMoviesSyntheticPostReturnRuntime ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C) ||
                                                *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                (*reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x88) !=
                                                    kReplayMoviesNativeCatalogSlotCount &&
                                                    !IsReplayMoviesUserProjectionPublishedLocal(runtimeAddress)))
                                            {
                                                return;
                                            }

                                            const std::size_t physicalSlot =
                                                static_cast<std::size_t>(
                                                    g_ReplayMoviesSyntheticPostReturnPhysicalSlot);

                                            if (physicalSlot <= kReplayMoviesAuthoredCatalogSlotCount ||
                                                physicalSlot > kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return;
                                            }

                                            const std::uintptr_t currentRow =
                                                *reinterpret_cast<const std::uintptr_t*>(runtimeAddress + 0x80);

                                            // Match the debugger shim exactly: while the restored physical synthetic
                                            // row is current, keep its mapped virtual authored row demoted.  When the
                                            // user navigates away this helper is inert; navigating back is safe and was
                                            // debugger-validated across the Row10 <-> Row11 boundary.
                                            if (GetReplayMoviesCatalogSlotNumber(runtimeAddress, currentRow) !=
                                                physicalSlot)
                                            {
                                                return;
                                            }

                                            const std::size_t virtualSlot =
                                                physicalSlot - kReplayMoviesAuthoredCatalogSlotCount;

                                            if (virtualSlot == 0 ||
                                                virtualSlot >
                                                (kReplayMoviesNativeCatalogSlotCount -
                                                    kReplayMoviesAuthoredCatalogSlotCount))
                                            {
                                                return;
                                            }

                                            const std::size_t virtualIndex = virtualSlot - 1;
                                            const std::uintptr_t virtualText =
                                                FindPackageObjectByNameLocal(
                                                    runtimeAddress,
                                                    kReplayMoviesOptionTextNames[virtualIndex]);
                                            const std::uintptr_t virtualHighlight =
                                                FindPackageObjectByNameLocal(
                                                    runtimeAddress,
                                                    kReplayMoviesHighlightNames[virtualIndex]);

                                            if (!IsReadableMemoryLocal(virtualText, 0x58) ||
                                                !IsReadableMemoryLocal(virtualHighlight, 0x58))
                                            {
                                                return;
                                            }

                                            const std::uintptr_t virtualTextN2 =
                                                GetReplayMoviesPresentationNodeLocal(virtualText, false);
                                            const std::uintptr_t virtualHighlightN2 =
                                                GetReplayMoviesPresentationNodeLocal(virtualHighlight, false);

                                            if (virtualTextN2 == 0 || virtualHighlightN2 == 0)
                                                return;

                                            if (*reinterpret_cast<const std::uintptr_t*>(virtualText + 0x54) !=
                                                virtualTextN2)
                                            {
                                                ApplyReplayMoviesPresentationNodeLocal(
                                                    virtualText,
                                                    virtualTextN2);
                                            }

                                            if (*reinterpret_cast<const std::uintptr_t*>(virtualHighlight + 0x54) !=
                                                virtualHighlightN2)
                                            {
                                                ApplyReplayMoviesPresentationNodeLocal(
                                                    virtualHighlight,
                                                    virtualHighlightN2);
                                            }
                                        }


                                        static bool CompletePendingReplayMoviesSyntheticSubmenuRestoreLocal(
                                            std::uintptr_t runtimeAddress)
                                        {
                                            if (g_PendingReplayMoviesSubmenuRestore == 0 ||
                                                g_PendingReplayMoviesSubmenuRestoreWasSubstituted == 0 ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C) ||
                                                *reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                (*reinterpret_cast<const std::uint32_t*>(runtimeAddress + 0x88) !=
                                                    kReplayMoviesNativeCatalogSlotCount &&
                                                    !IsReplayMoviesUserProjectionPublishedLocal(runtimeAddress)))
                                            {
                                                return false;
                                            }

                                            const std::size_t movieIndex =
                                                static_cast<std::size_t>(
                                                    g_PendingReplayMoviesSubmenuRestoreMovieIndex);

                                            // User mode decouples canonical movie identity from physical shell.
                                            // Resolve the saved canonical movie back to the current compact physical
                                            // slot before applying the already-proven virtual-native 11..17 handoff.
                                            const std::size_t physicalSlot =
                                                GetReplayMoviesPhysicalSlotForCanonicalLocal(movieIndex);

                                            if (physicalSlot <= kReplayMoviesAuthoredCatalogSlotCount ||
                                                physicalSlot > kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return false;
                                            }

                                            const std::size_t virtualSlot =
                                                physicalSlot - kReplayMoviesAuthoredCatalogSlotCount;

                                            if (virtualSlot == 0 ||
                                                virtualSlot >
                                                (kReplayMoviesNativeCatalogSlotCount -
                                                    kReplayMoviesAuthoredCatalogSlotCount))
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t virtualRow =
                                                GetReplayMoviesCatalogSlotAddress(
                                                    runtimeAddress,
                                                    virtualSlot);
                                            const std::uintptr_t physicalRow =
                                                GetReplayMoviesCatalogSlotAddress(
                                                    runtimeAddress,
                                                    physicalSlot);

                                            if (!IsReadableMemoryLocal(virtualRow, 0x58) ||
                                                !IsReadableMemoryLocal(physicalRow, 0x58))
                                            {
                                                return false;
                                            }

                                            const std::uint32_t expectedVirtualHash =
                                                kReplayMoviesOptionControlHashes[virtualSlot - 1];
                                            const std::uint32_t expectedPhysicalHash =
                                                kReplayMoviesOptionControlHashes[physicalSlot - 1];

                                            if (*reinterpret_cast<const std::uint32_t*>(virtualRow + 0x10) !=
                                                expectedVirtualHash ||
                                                *reinterpret_cast<const std::uint32_t*>(physicalRow + 0x10) !=
                                                expectedPhysicalHash ||
                                                g_PendingReplayMoviesSubmenuRestoreHash != expectedVirtualHash)
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t currentRow =
                                                *reinterpret_cast<const std::uintptr_t*>(runtimeAddress + 0x80);

                                            // Debugger proof on rows 11, 12 and 13: after native 004F5ED0 runs
                                            // against the temporary ten-row runtime, the completed 17-row runtime
                                            // still owns the corresponding virtual native row (1, 2 or 3).  Require
                                            // that exact invariant before touching Runtime+80.
                                            if (currentRow != virtualRow)
                                                return false;

                                            const std::size_t virtualIndex = virtualSlot - 1;
                                            const std::uintptr_t virtualText =
                                                FindPackageObjectByNameLocal(
                                                    runtimeAddress,
                                                    kReplayMoviesOptionTextNames[virtualIndex]);
                                            const std::uintptr_t virtualHighlight =
                                                FindPackageObjectByNameLocal(
                                                    runtimeAddress,
                                                    kReplayMoviesHighlightNames[virtualIndex]);

                                            if (!IsReadableMemoryLocal(virtualText, 0x58) ||
                                                !IsReadableMemoryLocal(virtualHighlight, 0x58))
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t virtualTextN2 =
                                                GetReplayMoviesPresentationNodeLocal(
                                                    virtualText,
                                                    false);
                                            const std::uintptr_t virtualHighlightN2 =
                                                GetReplayMoviesPresentationNodeLocal(
                                                    virtualHighlight,
                                                    false);

                                            if (virtualTextN2 == 0 || virtualHighlightN2 == 0)
                                                return false;

                                            // Exact debugger-proven physical handoff:
                                            //   Row11 <- virtual Row1
                                            //   Row12 <- virtual Row2
                                            //   Row13 <- virtual Row3
                                            // generalized mechanically through Row17 <- virtual Row7.
                                            *reinterpret_cast<std::uintptr_t*>(runtimeAddress + 0x80) =
                                                physicalRow;

                                            // Now that Runtime+80 owns the physical synthetic row, apply the already
                                            // validated rows10..17 synchronizer so the target synthetic Text/Highlight
                                            // receives N1 and every synthetic companion receives N2.
                                            SynchronizeReplayMoviesPostHookVisualsLocal(
                                                runtimeAddress,
                                                true);

                                            // The native virtual row is still visually N1 after its successful native
                                            // restore.  Demote only its Text and Highlight using the exact native
                                            // 004FF030 transaction proven independently on virtual rows 1, 2 and 3.
                                            ApplyReplayMoviesPresentationNodeLocal(
                                                virtualText,
                                                virtualTextN2);
                                            ApplyReplayMoviesPresentationNodeLocal(
                                                virtualHighlight,
                                                virtualHighlightN2);

                                            if (*reinterpret_cast<const std::uintptr_t*>(runtimeAddress + 0x80) !=
                                                physicalRow ||
                                                *reinterpret_cast<const std::uintptr_t*>(virtualText + 0x54) !=
                                                virtualTextN2 ||
                                                *reinterpret_cast<const std::uintptr_t*>(virtualHighlight + 0x54) !=
                                                virtualHighlightN2)
                                            {
                                                return false;
                                            }

                                            // Arm the generation-local persistent demotion only after the exact
                                            // debugger-proven handoff and initial N2 transaction have both verified.
                                            // Later 004F1116 writes are allowed to run natively first; the active-node
                                            // hook tail will then reapply this same mapped virtual-row N2 transaction.
                                            g_ReplayMoviesSyntheticPostReturnRuntime = runtimeAddress;
                                            g_ReplayMoviesSyntheticPostReturnPhysicalSlot =
                                                static_cast<unsigned long>(physicalSlot);

                                            ++g_ReplayMoviesSubmenuRestoreCommitSuccess;
                                            g_PendingReplayMoviesSubmenuRestore = 0;
                                            g_PendingReplayMoviesSubmenuRestoreHash = 0;
                                            g_PendingReplayMoviesSubmenuRestoreWasSubstituted = 0;
                                            return true;
                                        }


                                        static bool ConfigureReplayMoviesDeveloperCatalogPage(
                                            std::uintptr_t runtimeAddress,
                                            std::size_t pageIndex,
                                            bool synchronizeCurrentSelection)
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                pageIndex >= kReplayMoviesCatalogPageCount)
                                            {
                                                return false;
                                            }

                                            std::uintptr_t buttons[kReplayMoviesNativeCatalogSlotCount]{};
                                            std::uintptr_t texts[kReplayMoviesNativeCatalogSlotCount]{};
                                            std::uintptr_t textBuffers[kReplayMoviesNativeCatalogSlotCount]{};

                                            if (!AcquireReplayMoviesCatalogObjects(
                                                runtimeAddress,
                                                buttons,
                                                texts,
                                                textBuffers))
                                            {
                                                return false;
                                            }

                                            if (g_ReplayMoviesCatalogActiveButtonFlags == 0 ||
                                                g_ReplayMoviesCatalogActiveTextFlags == 0)
                                            {
                                                return false;
                                            }

                                            // ReplayMovies owns only the visible title FE_String; the borrowed
                                            // GameTrailers selector identity/callback path remains untouched.
                                            // Debugger proof on both initial entry and fresh post-FMV reconstruction
                                            // identified MenuTitle (hash F3C04E76, Type 2) as the visible TRAILERS
                                            // caption and proved FEngPrintfObj can safely replace it with
                                            // "REPLAY MOVIES". Reacquire the object from this generation's
                                            // package graph instead of caching a heap address across reconstruction.
                                            const std::uintptr_t replayMoviesMenuTitle =
                                                FindPackageObjectByNameLocal(runtimeAddress, "MenuTitle");

                                            if (!IsReadableMemoryLocal(replayMoviesMenuTitle, 0x68) ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    replayMoviesMenuTitle + 0x10) != 0xF3C04E76u ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    replayMoviesMenuTitle + 0x18) != 2)
                                            {
                                                return false;
                                            }

                                            SetSyntheticTextLocal(
                                                replayMoviesMenuTitle,
                                                "REPLAY MOVIES");

                                            const std::size_t pageOffset =
                                                kReplayMoviesCatalogPageOffsets[pageIndex];

                                            if (pageOffset +
                                                kReplayMoviesNativeCatalogSlotCount >
                                                ReplayMoviesCatalog::kCareerMovieCount)
                                            {
                                                return false;
                                            }

                                            for (std::size_t slotIndex = 0;
                                                slotIndex < kReplayMoviesNativeCatalogSlotCount;
                                                ++slotIndex)
                                            {
                                                *reinterpret_cast<std::uint32_t*>(
                                                    buttons[slotIndex] + 0x1C) =
                                                    g_ReplayMoviesCatalogActiveButtonFlags;

                                                *reinterpret_cast<std::uint32_t*>(
                                                    texts[slotIndex] + 0x1C) =
                                                    g_ReplayMoviesCatalogActiveTextFlags;

                                                const auto& entry =
                                                    ReplayMoviesCatalog::kCareerMovies[
                                                        pageOffset + slotIndex];

                                                if (slotIndex >= kReplayMoviesAuthoredCatalogSlotCount)
                                                {
                                                    // The Type-2 native clone can initially share its rendered-text
                                                    // backing with row 10. Use Underground's own text worker for the
                                                    // synthetic row so it performs any required grow/detach/dirty
                                                    // bookkeeping rather than writing through a potentially shared
                                                    // pointer.
                                                    SetSyntheticTextLocal(texts[slotIndex], entry.displayTitle);
                                                }
                                                else if (!WriteAsciiTitleToWideBufferLocal(
                                                    textBuffers[slotIndex],
                                                    entry.displayTitle))
                                                {
                                                    return false;
                                                }
                                            }

                                            g_ReplayMoviesDeveloperCatalogPage =
                                                static_cast<unsigned long>(pageIndex);

                                            g_ReplayMoviesCatalogConfiguredSlots +=
                                                static_cast<unsigned long>(
                                                    kReplayMoviesNativeCatalogSlotCount);

                                            g_ReplayMoviesCatalogTitleUpdates +=
                                                static_cast<unsigned long>(
                                                    kReplayMoviesNativeCatalogSlotCount);

                                            if (synchronizeCurrentSelection)
                                            {
                                                const std::uintptr_t currentRow =
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        runtimeAddress + 0x80);

                                                const std::size_t slotNumber =
                                                    GetReplayMoviesCatalogSlotNumber(
                                                        runtimeAddress,
                                                        currentRow);

                                                if (slotNumber != 0)
                                                {
                                                    const std::size_t movieIndex =
                                                        pageOffset + (slotNumber - 1);

                                                    if (movieIndex <
                                                        ReplayMoviesCatalog::kCareerMovieCount)
                                                    {
                                                        g_ReplayMoviesSelectedDeveloperMovieIndex =
                                                            static_cast<unsigned long>(movieIndex);

                                                        ++g_ReplayMoviesCatalogSelectionUpdates;
                                                    }
                                                }
                                            }

                                            return true;
                                        }


                                        static bool ConfigureReplayMoviesUserCatalogProjection(
                                            std::uintptr_t runtimeAddress,
                                            bool synchronizeCurrentSelection)
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                !ReplayMoviesProfileStorage::HasCurrentProfile())
                                            {
                                                return false;
                                            }

                                            std::uintptr_t buttons[kReplayMoviesNativeCatalogSlotCount]{};
                                            std::uintptr_t texts[kReplayMoviesNativeCatalogSlotCount]{};
                                            std::uintptr_t textBuffers[kReplayMoviesNativeCatalogSlotCount]{};

                                            // Acquisition deliberately happens before Runtime+84/+88 are redirected.
                                            // This is the same full-generation census used by the debugger proof.
                                            if (!AcquireReplayMoviesCatalogObjects(
                                                runtimeAddress,
                                                buttons,
                                                texts,
                                                textBuffers))
                                            {
                                                return false;
                                            }

                                            if (g_ReplayMoviesCatalogActiveButtonFlags == 0 ||
                                                g_ReplayMoviesCatalogActiveTextFlags == 0)
                                            {
                                                return false;
                                            }

                                            const std::uint32_t seenMask =
                                                ReplayMoviesProfileStorage::GetCurrentSeenMask() &
                                                kReplayMoviesFullSeenMask;

                                            unsigned long stagedCanonicalByPhysical[
                                                kReplayMoviesNativeCatalogSlotCount]{};
                                                std::size_t visibleCount = 0;

                                                for (std::size_t physicalIndex = 0;
                                                    physicalIndex < kReplayMoviesNativeCatalogSlotCount;
                                                    ++physicalIndex)
                                                {
                                                    stagedCanonicalByPhysical[physicalIndex] =
                                                        kReplayMoviesInvalidCanonicalIndex;
                                                }

                                                for (std::size_t canonicalIndex = 0;
                                                    canonicalIndex < ReplayMoviesCatalog::kCareerMovieCount;
                                                    ++canonicalIndex)
                                                {
                                                    if ((seenMask & (1u << canonicalIndex)) == 0)
                                                        continue;

                                                    if (visibleCount >= kReplayMoviesNativeCatalogSlotCount)
                                                        return false;

                                                    stagedCanonicalByPhysical[visibleCount] =
                                                        static_cast<unsigned long>(canonicalIndex);
                                                    ++visibleCount;
                                                }

                                                g_ReplayMoviesUserSeenMask = seenMask;
                                                g_ReplayMoviesUserVisibleCount =
                                                    static_cast<unsigned long>(visibleCount);

                                                // COUNT=0, COUNT=1, and 2+ User projection states are all now
                                                // debugger-proven. COUNT=0 is handled below as a special native empty
                                                // selector: keep Runtime+84 on the game's original heap-owned vector,
                                                // publish CURRENT=null and COUNT=0, and blank every physical title.
                                                // Do not allocate a zero-length replacement vector and do not fall back
                                                // to the Developer catalogue.

                                                const std::uintptr_t replayMoviesMenuTitle =
                                                    FindPackageObjectByNameLocal(runtimeAddress, "MenuTitle");

                                                if (!IsReadableMemoryLocal(replayMoviesMenuTitle, 0x68) ||
                                                    *reinterpret_cast<const std::uint32_t*>(
                                                        replayMoviesMenuTitle + 0x10) != 0xF3C04E76u ||
                                                    *reinterpret_cast<const std::uint32_t*>(
                                                        replayMoviesMenuTitle + 0x18) != 2)
                                                {
                                                    return false;
                                                }

                                                SetSyntheticTextLocal(
                                                    replayMoviesMenuTitle,
                                                    "REPLAY MOVIES");

                                                // COUNT=0 visual proof: native construction leaves Highlight_01 on its
                                                // selected/N1 presentation node even after the selector itself is made
                                                // empty.  Resolve and validate its dormant/N2 node before mutating the
                                                // zero-candidate runtime so the later demotion is the exact debugger-
                                                // proven 004FF030 presentation transaction.
                                                std::uintptr_t zeroHighlight = 0;
                                                std::uintptr_t zeroHighlightDormant = 0;

                                                if (visibleCount == 0)
                                                {
                                                    zeroHighlight = FindPackageObjectByNameLocal(
                                                        runtimeAddress,
                                                        kReplayMoviesHighlightNames[0]);

                                                    if (!IsReadableMemoryLocal(zeroHighlight, 0x58) ||
                                                        *reinterpret_cast<const std::uint32_t*>(
                                                            zeroHighlight + 0x10) !=
                                                        kReplayMoviesHighlightHashes[0] ||
                                                        *reinterpret_cast<const std::uint32_t*>(
                                                            zeroHighlight + 0x18) != 1)
                                                    {
                                                        return false;
                                                    }

                                                    zeroHighlightDormant =
                                                        GetReplayMoviesPresentationNodeLocal(
                                                            zeroHighlight,
                                                            false);

                                                    if (zeroHighlightDormant == 0)
                                                        return false;
                                                }

                                                // Projection C1/C2: the first N physical rows become compact shells for
                                                // the set canonical bits, while every remaining physical title becomes
                                                // empty.  Use the same native FEngPrintfObj worker proven by the debugger
                                                // for both remapping and blanking.
                                                for (std::size_t physicalIndex = 0;
                                                    physicalIndex < kReplayMoviesNativeCatalogSlotCount;
                                                    ++physicalIndex)
                                                {
                                                    *reinterpret_cast<std::uint32_t*>(
                                                        buttons[physicalIndex] + 0x1C) =
                                                        g_ReplayMoviesCatalogActiveButtonFlags;

                                                    *reinterpret_cast<std::uint32_t*>(
                                                        texts[physicalIndex] + 0x1C) =
                                                        g_ReplayMoviesCatalogActiveTextFlags;

                                                    if (physicalIndex < visibleCount)
                                                    {
                                                        const std::size_t canonicalIndex =
                                                            static_cast<std::size_t>(
                                                                stagedCanonicalByPhysical[physicalIndex]);

                                                        if (canonicalIndex >=
                                                            ReplayMoviesCatalog::kCareerMovieCount)
                                                        {
                                                            return false;
                                                        }

                                                        SetSyntheticTextLocal(
                                                            texts[physicalIndex],
                                                            ReplayMoviesCatalog::kCareerMovies[
                                                                canonicalIndex].displayTitle);
                                                    }
                                                    else
                                                    {
                                                        SetSyntheticTextLocal(texts[physicalIndex], "");
                                                    }
                                                }

                                                // Projection B1: publish only physical rows 1..N, preserving the native
                                                // reverse vector order (vector[0] = physical Row N, last = Row 1).
                                                for (std::size_t vectorIndex = 0;
                                                    vectorIndex < kReplayMoviesNativeCatalogSlotCount;
                                                    ++vectorIndex)
                                                {
                                                    g_ReplayMoviesUserCandidateVector[vectorIndex] = 0;
                                                }

                                                for (std::size_t vectorIndex = 0;
                                                    vectorIndex < visibleCount;
                                                    ++vectorIndex)
                                                {
                                                    const std::size_t physicalIndex =
                                                        (visibleCount - 1) - vectorIndex;

                                                    g_ReplayMoviesUserCandidateVector[vectorIndex] =
                                                        buttons[physicalIndex];
                                                }

                                                for (std::size_t physicalIndex = 0;
                                                    physicalIndex < kReplayMoviesNativeCatalogSlotCount;
                                                    ++physicalIndex)
                                                {
                                                    g_ReplayMoviesUserCanonicalByPhysical[physicalIndex] =
                                                        stagedCanonicalByPhysical[physicalIndex];
                                                }

                                                auto* runtime =
                                                    reinterpret_cast<std::uint8_t*>(runtimeAddress);

                                                // Debugger-proven zero-candidate state:
                                                //   Runtime+80 = 0
                                                //   Runtime+84 = original native-owned vector (preserved)
                                                //   Runtime+88 = 0
                                                // Native Up/Down remains inert and Back reconstructs Main Menu normally.
                                                // Keeping +84 untouched also preserves the native destructor ownership
                                                // contract proven by the earlier invalid-free investigation.
                                                if (visibleCount == 0)
                                                {
                                                    *reinterpret_cast<std::uintptr_t*>(runtime + 0x80) = 0;
                                                    *reinterpret_cast<std::uint32_t*>(runtime + 0x88) = 0;

                                                    // Debugger A/B proof on a live zero-candidate runtime:
                                                    //   Highlight_01 +54 == N1  -> orphan selected bar remains
                                                    //   apply its N2 through 004FF030 -> bar disappears
                                                    // while CURRENT=0 / COUNT=0 remain unchanged.
                                                    ApplyReplayMoviesPresentationNodeLocal(
                                                        zeroHighlight,
                                                        zeroHighlightDormant);

                                                    g_ReplayMoviesUserPublishedCandidateVector = 0;
                                                    g_ReplayMoviesUserProjectionRuntime = runtimeAddress;
                                                    g_ReplayMoviesUserProjectionActive = 1;
                                                    g_ReplayMoviesDeveloperCatalogPage = 0;

                                                    ++g_ReplayMoviesUserProjectionApplies;
                                                    g_ReplayMoviesCatalogTitleUpdates +=
                                                        static_cast<unsigned long>(
                                                            kReplayMoviesNativeCatalogSlotCount);

                                                    return true;
                                                }

                                                // Runtime+84 is native-owned storage. The native destructor at
                                                // 004FF965 loads this pointer and releases it through 00671102.
                                                // Publishing the DLL-static staging array here caused the debugger-
                                                // proven RtlFreeHeap crash. Allocate the compact vector with the same
                                                // native allocator used by the game's Butn candidate-vector path so
                                                // the normal destructor owns and frees it safely.
                                                auto* nativeCandidateVector =
                                                    reinterpret_cast<std::uintptr_t*>(
                                                        NativeGameAllocateLocal(
                                                            visibleCount * sizeof(std::uintptr_t)));

                                                if (nativeCandidateVector == nullptr)
                                                    return false;

                                                std::memcpy(
                                                    nativeCandidateVector,
                                                    g_ReplayMoviesUserCandidateVector,
                                                    visibleCount * sizeof(std::uintptr_t));

                                                const std::uintptr_t supersededVector =
                                                    *reinterpret_cast<const std::uintptr_t*>(runtime + 0x84);
                                                const std::uint32_t supersededCount =
                                                    *reinterpret_cast<const std::uint32_t*>(runtime + 0x88);

                                                *reinterpret_cast<std::uintptr_t*>(runtime + 0x84) =
                                                    reinterpret_cast<std::uintptr_t>(nativeCandidateVector);
                                                *reinterpret_cast<std::uint32_t*>(runtime + 0x88) =
                                                    static_cast<std::uint32_t>(visibleCount);

                                                g_ReplayMoviesUserPublishedCandidateVector =
                                                    reinterpret_cast<std::uintptr_t>(nativeCandidateVector);
                                                g_ReplayMoviesUserProjectionRuntime = runtimeAddress;
                                                g_ReplayMoviesUserProjectionActive = 1;
                                                g_ReplayMoviesDeveloperCatalogPage = 0;

                                                if (!IsReplayMoviesUserProjectionPublishedLocal(runtimeAddress))
                                                {
                                                    // We are still inside the projection transaction and native
                                                    // execution has not resumed, so roll back the publication before
                                                    // releasing the failed replacement. The superseded full-catalog
                                                    // vector is intentionally retained on success, matching the
                                                    // debugger-proven 10->17 enlargement policy: its late-construction
                                                    // lifetime has not been proven safe to free here.
                                                    *reinterpret_cast<std::uintptr_t*>(runtime + 0x84) =
                                                        supersededVector;
                                                    *reinterpret_cast<std::uint32_t*>(runtime + 0x88) =
                                                        supersededCount;

                                                    g_ReplayMoviesUserPublishedCandidateVector = 0;
                                                    g_ReplayMoviesUserProjectionActive = 0;
                                                    g_ReplayMoviesUserProjectionRuntime = 0;

                                                    NativeGameFreeLocal(nativeCandidateVector);
                                                    ResetReplayMoviesUserProjectionLocal();
                                                    return false;
                                                }

                                                ++g_ReplayMoviesUserProjectionApplies;
                                                g_ReplayMoviesCatalogConfiguredSlots +=
                                                    static_cast<unsigned long>(visibleCount);
                                                g_ReplayMoviesCatalogTitleUpdates +=
                                                    static_cast<unsigned long>(
                                                        kReplayMoviesNativeCatalogSlotCount);

                                                if (synchronizeCurrentSelection)
                                                {
                                                    const std::uintptr_t currentRow =
                                                        *reinterpret_cast<const std::uintptr_t*>(
                                                            runtime + 0x80);
                                                    const std::size_t physicalSlot =
                                                        GetReplayMoviesCatalogSlotNumber(
                                                            runtimeAddress,
                                                            currentRow);
                                                    const std::size_t canonicalIndex =
                                                        GetReplayMoviesCanonicalIndexForPhysicalSlotLocal(
                                                            physicalSlot);

                                                    if (canonicalIndex <
                                                        ReplayMoviesCatalog::kCareerMovieCount)
                                                    {
                                                        g_ReplayMoviesSelectedDeveloperMovieIndex =
                                                            static_cast<unsigned long>(canonicalIndex);
                                                        ++g_ReplayMoviesCatalogSelectionUpdates;
                                                    }
                                                }

                                                return true;
                                        }


                                        static bool ConfigureReplayMoviesCatalogPageForAccessMode(
                                            std::uintptr_t runtimeAddress,
                                            std::size_t pageIndex,
                                            bool synchronizeCurrentSelection)
                                        {
                                            switch (ReplayMoviesConfig::GetAccessMode())
                                            {
                                            case ReplayMoviesConfig::AccessMode::Developer:
                                                ResetReplayMoviesUserProjectionLocal();
                                                return ConfigureReplayMoviesDeveloperCatalogPage(
                                                    runtimeAddress,
                                                    pageIndex,
                                                    synchronizeCurrentSelection);

                                            case ReplayMoviesConfig::AccessMode::User:
                                            default:
                                                return ConfigureReplayMoviesUserCatalogProjection(
                                                    runtimeAddress,
                                                    synchronizeCurrentSelection);
                                            }
                                        }


                                        static bool ConfigureReplayMoviesDeveloperCatalogBootstrap(
                                            std::uintptr_t runtimeAddress)
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C))
                                            {
                                                return false;
                                            }

                                            auto* runtime =
                                                reinterpret_cast<std::uint8_t*>(runtimeAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                runtime + 0x10) != Addr::TrailersRuntimeHash)
                                            {
                                                return false;
                                            }

                                            if (!EnsureReplayMoviesSyntheticSlot11(runtimeAddress) ||
                                                !EnsureReplayMoviesSyntheticSlots12To17(runtimeAddress) ||
                                                !ApplyReplayMoviesCompactOnePageGeometryLocal(runtimeAddress))
                                            {
                                                return false;
                                            }

                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                runtime + 0x88) != kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return false;
                                            }

                                            // Build the debugger-proven generation-local presentation cache once,
                                            // after all 17 rows and compact geometry exist but before publishing this
                                            // runtime to the generic frontend hooks.  A failed cache build leaves the
                                            // previous generation invalid and fails the bootstrap closed.
                                            if (!BuildReplayMoviesPresentationCacheLocal(runtimeAddress))
                                                return false;

                                            // Publish the generation-local runtime only after the complete 17-row
                                            // structure, compact geometry and cached presentation graph have passed
                                            // validation. Generic frontend hooks then perform fixed-address work only.
                                            g_ReplayMoviesCatalogRuntimeAddress = runtimeAddress;

                                            std::uintptr_t buttons[kReplayMoviesNativeCatalogSlotCount]{};
                                            std::uintptr_t texts[kReplayMoviesNativeCatalogSlotCount]{};
                                            std::uintptr_t textBuffers[kReplayMoviesNativeCatalogSlotCount]{};

                                            if (!AcquireReplayMoviesCatalogObjects(
                                                runtimeAddress,
                                                buttons,
                                                texts,
                                                textBuffers))
                                            {
                                                return false;
                                            }

                                            g_ReplayMoviesCatalogActiveButtonFlags =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    buttons[0] + 0x1C);

                                            g_ReplayMoviesCatalogActiveTextFlags =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    texts[0] + 0x1C);

                                            if (g_ReplayMoviesCatalogActiveButtonFlags == 0 ||
                                                g_ReplayMoviesCatalogActiveTextFlags == 0)
                                            {
                                                return false;
                                            }

                                            std::size_t pageIndex =
                                                static_cast<std::size_t>(
                                                    g_ReplayMoviesDeveloperCatalogPage);

                                            const bool pendingPostMovieRestore =
                                                g_PendingReplayMoviesSubmenuRestore != 0;

                                            if (pendingPostMovieRestore)
                                            {
                                                const std::size_t pendingPage =
                                                    static_cast<std::size_t>(
                                                        g_PendingReplayMoviesSubmenuRestorePage);

                                                if (pendingPage < kReplayMoviesCatalogPageCount)
                                                    pageIndex = pendingPage;
                                            }

                                            if (pageIndex >= kReplayMoviesCatalogPageCount)
                                                pageIndex = 0;

                                            // On a fresh post-FMV generation, Runtime+80 is natively initialized to
                                            // OptionName_1 before the later native restore transaction. Do not let
                                            // that temporary default overwrite the movie index we deliberately kept
                                            // across the FMV. Configure the titles/page only; 004F5ED0 will perform
                                            // the real selection commit below.
                                            if (!ConfigureReplayMoviesCatalogPageForAccessMode(
                                                runtimeAddress,
                                                pageIndex,
                                                !pendingPostMovieRestore))
                                            {
                                                return false;
                                            }

                                            const std::size_t movieIndex =
                                                pendingPostMovieRestore
                                                ? static_cast<std::size_t>(
                                                    g_PendingReplayMoviesSubmenuRestoreMovieIndex)
                                                : 0;

                                            // Native 004F5ED0 has already run earlier while only the ten authored
                                            // candidates existed. Developer maps canonical index directly to physical
                                            // slot; User maps the saved canonical identity back through the compact
                                            // projection. Only physical slots 11..17 require the proven virtual-native
                                            // handoff.
                                            const std::size_t restorePhysicalSlot =
                                                pendingPostMovieRestore
                                                ? GetReplayMoviesPhysicalSlotForCanonicalLocal(movieIndex)
                                                : 0;

                                            if (pendingPostMovieRestore &&
                                                restorePhysicalSlot > kReplayMoviesAuthoredCatalogSlotCount &&
                                                restorePhysicalSlot <= kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                if (CompletePendingReplayMoviesSyntheticSubmenuRestoreLocal(
                                                    runtimeAddress))
                                                {
                                                    return true;
                                                }

                                                // Fail closed: if any invariant is missing, retain the valid native
                                                // virtual selection and leave every synthetic row visually N2.  Do not
                                                // fabricate a physical restore or overwrite the pending virtual hash.
                                                SynchronizeReplayMoviesPostHookVisualsLocal(
                                                    runtimeAddress,
                                                    true);
                                                return true;
                                            }

                                            // Authored rows 1..10 retain the established direct native restoration
                                            // behavior. Push the compact synthetic companion state once before native
                                            // execution resumes; later navigation is corrected by the existing post-
                                            // 004F1116 synchronizer.
                                            SynchronizeReplayMoviesPostHookVisualsLocal(
                                                runtimeAddress,
                                                true);

                                            if (!pendingPostMovieRestore)
                                                return true;

                                            if (movieIndex >= ReplayMoviesCatalog::kCareerMovieCount ||
                                                restorePhysicalSlot == 0 ||
                                                restorePhysicalSlot > kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                g_PendingReplayMoviesSubmenuRestoreHash = 0;
                                                return true;
                                            }

                                            const std::uintptr_t targetRow =
                                                GetReplayMoviesCatalogSlotAddress(
                                                    runtimeAddress,
                                                    restorePhysicalSlot);

                                            if (!IsReadableMemoryLocal(targetRow, 0x14))
                                            {
                                                g_PendingReplayMoviesSubmenuRestoreHash = 0;
                                                return true;
                                            }

                                            g_ReplayMoviesSelectedDeveloperMovieIndex =
                                                static_cast<unsigned long>(movieIndex);

                                            g_PendingReplayMoviesSubmenuRestoreHash =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    targetRow + 0x10);

                                            return true;
                                        }


                                        static std::uintptr_t __cdecl ResolveReplayMoviesCatalogNavigationCandidate(
                                            std::uintptr_t runtimeAddress,
                                            std::uintptr_t nativeCandidateAddress)
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                runtimeAddress == 0 ||
                                                nativeCandidateAddress == 0)
                                            {
                                                return nativeCandidateAddress;
                                            }

                                            // v11.0 row-11 prototype is intentionally one-page. The old virtual
                                            // page remapping is disabled; native selector navigation owns all 11
                                            // candidates directly.
                                            if (kReplayMoviesCatalogPageCount <= 1)
                                                return nativeCandidateAddress;

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    runtimeAddress);

                                            if (!IsReadableMemoryLocal(runtimeAddress, 0x8C) ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    runtime + 0x10) != Addr::TrailersRuntimeHash ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    runtime + 0x88) != kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                return nativeCandidateAddress;
                                            }

                                            if (g_ReplayMoviesCatalogPendingPageValid != 0 &&
                                                g_ReplayMoviesCatalogPendingRuntime == runtimeAddress &&
                                                g_ReplayMoviesCatalogPendingIncomingRow != 0)
                                            {
                                                return g_ReplayMoviesCatalogPendingIncomingRow;
                                            }

                                            const std::uintptr_t oldRow =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    runtime + 0x80);

                                            const std::size_t oldSlot =
                                                GetReplayMoviesCatalogSlotNumber(
                                                    runtimeAddress,
                                                    oldRow);

                                            const std::size_t nativeSlot =
                                                GetReplayMoviesCatalogSlotNumber(
                                                    runtimeAddress,
                                                    nativeCandidateAddress);

                                            if (oldSlot == 0 || nativeSlot == 0)
                                                return nativeCandidateAddress;

                                            const std::size_t pageIndex =
                                                static_cast<std::size_t>(
                                                    g_ReplayMoviesDeveloperCatalogPage);

                                            std::size_t targetPage = pageIndex;
                                            std::size_t targetSlot = nativeSlot;
                                            bool pageTransition = false;

                                            if (pageIndex == 0)
                                            {
                                                if (oldSlot == 10 && nativeSlot == 1)
                                                {
                                                    targetPage = 1;
                                                    targetSlot = 4;
                                                    pageTransition = true;
                                                }
                                                else if (oldSlot == 1 && nativeSlot == 10)
                                                {
                                                    targetPage = 1;
                                                    targetSlot = 10;
                                                    pageTransition = true;
                                                }
                                            }
                                            else if (pageIndex == 1)
                                            {
                                                if (oldSlot == 4 && nativeSlot == 3)
                                                {
                                                    targetPage = 0;
                                                    targetSlot = 10;
                                                    pageTransition = true;
                                                }
                                                else if (oldSlot == 10 && nativeSlot == 1)
                                                {
                                                    targetPage = 0;
                                                    targetSlot = 1;
                                                    pageTransition = true;
                                                }
                                            }

                                            if (!pageTransition)
                                                return nativeCandidateAddress;

                                            const std::uintptr_t targetRow =
                                                GetReplayMoviesCatalogSlotAddress(
                                                    runtimeAddress,
                                                    targetSlot);

                                            if (targetRow == 0)
                                                return nativeCandidateAddress;

                                            g_ReplayMoviesCatalogPendingRuntime =
                                                runtimeAddress;

                                            g_ReplayMoviesCatalogPendingIncomingRow =
                                                targetRow;

                                            g_ReplayMoviesCatalogPendingPage =
                                                static_cast<unsigned long>(
                                                    targetPage);

                                            g_ReplayMoviesCatalogPendingPageValid = 1;

                                            return targetRow;
                                        }


                                        static void __cdecl ObserveReplayMoviesCatalogCurrentRowCommit(
                                            std::uintptr_t runtimeAddress,
                                            std::uintptr_t incomingRowAddress)
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                runtimeAddress == 0 ||
                                                incomingRowAddress == 0 ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C))
                                            {
                                                return;
                                            }

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    runtimeAddress);

                                            const std::uint32_t candidateCount =
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88);

                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                runtime + 0x10) != Addr::TrailersRuntimeHash ||
                                                (candidateCount != kReplayMoviesNativeCatalogSlotCount &&
                                                    !IsReplayMoviesUserProjectionPublishedLocal(runtimeAddress)))
                                            {
                                                return;
                                            }

                                            if (g_ReplayMoviesCatalogPendingPageValid != 0 &&
                                                g_ReplayMoviesCatalogPendingRuntime == runtimeAddress &&
                                                g_ReplayMoviesCatalogPendingIncomingRow ==
                                                incomingRowAddress)
                                            {
                                                const std::size_t targetPage =
                                                    static_cast<std::size_t>(
                                                        g_ReplayMoviesCatalogPendingPage);

                                                if (ConfigureReplayMoviesCatalogPageForAccessMode(
                                                    runtimeAddress,
                                                    targetPage,
                                                    false))
                                                {
                                                    ++g_ReplayMoviesCatalogPageTransitions;
                                                }

                                                g_ReplayMoviesCatalogPendingRuntime = 0;
                                                g_ReplayMoviesCatalogPendingIncomingRow = 0;
                                                g_ReplayMoviesCatalogPendingPage = 0;
                                                g_ReplayMoviesCatalogPendingPageValid = 0;
                                            }

                                            const std::uint32_t incomingHash =
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    incomingRowAddress + 0x10);

                                            if (g_PendingReplayMoviesSubmenuRestore != 0 &&
                                                g_PendingReplayMoviesSubmenuRestoreHash != 0)
                                            {
                                                if (incomingHash ==
                                                    g_PendingReplayMoviesSubmenuRestoreHash)
                                                {
                                                    ++g_ReplayMoviesSubmenuRestoreCommitSuccess;
                                                    g_PendingReplayMoviesSubmenuRestore = 0;
                                                    g_PendingReplayMoviesSubmenuRestoreHash = 0;
                                                    g_PendingReplayMoviesSubmenuRestoreWasSubstituted = 0;
                                                }
                                                else if (
                                                    g_PendingReplayMoviesSubmenuRestoreWasSubstituted != 0 &&
                                                    incomingHash == 0x1AEC7D09 &&
                                                    g_PendingReplayMoviesSubmenuRestoreHash !=
                                                    0x1AEC7D09)
                                                {
                                                    // The fresh generation committed its retail default instead of
                                                    // our requested row. Expire fail-closed so a later user action
                                                    // can never be hijacked by a stale post-movie restore.
                                                    ++g_ReplayMoviesSubmenuRestoreCommitMiss;
                                                    g_PendingReplayMoviesSubmenuRestore = 0;
                                                    g_PendingReplayMoviesSubmenuRestoreHash = 0;
                                                    g_PendingReplayMoviesSubmenuRestoreWasSubstituted = 0;
                                                }
                                            }

                                            const std::size_t slotNumber =
                                                GetReplayMoviesCatalogSlotNumber(
                                                    runtimeAddress,
                                                    incomingRowAddress);

                                            if (slotNumber == 0)
                                                return;

                                            const std::size_t movieIndex =
                                                GetReplayMoviesCanonicalIndexForPhysicalSlotLocal(
                                                    slotNumber);

                                            if (movieIndex >= ReplayMoviesCatalog::kCareerMovieCount)
                                                return;

                                            g_ReplayMoviesSelectedDeveloperMovieIndex =
                                                static_cast<unsigned long>(movieIndex);

                                            ++g_ReplayMoviesCatalogSelectionUpdates;
                                        }


                                        static void __cdecl ApplyReplayMoviesDeveloperMovieIdentity(
                                            std::uintptr_t videoObjectAddress,
                                            std::uintptr_t runtimeAddress)
                                        {
                                            if (g_ReplayMoviesSubmenuActive == 0 ||
                                                !IsReadableMemoryLocal(videoObjectAddress, 0x28))
                                            {
                                                return;
                                            }

                                            auto* video =
                                                reinterpret_cast<std::uint8_t*>(videoObjectAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                video + 0x10) != Addr::TrailerMovieVideoHash ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    video + 0x18) != 7)
                                            {
                                                return;
                                            }

                                            const std::uintptr_t originalBasename =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    video + 0x24);

                                            // Fail closed: this bootstrap owns only the retail TrailerMovie Video
                                            // node that was proven to carry the authored NASCAR basename.
                                            if (originalBasename == 0 ||
                                                !AsciiStringEqualsLocal(
                                                    reinterpret_cast<const char*>(originalBasename),
                                                    "NASCAR"))
                                            {
                                                return;
                                            }

                                            // Resolve the selected movie again at the actual Video activation
                                            // boundary. Native captures showed EBX remains the owning Trailers
                                            // runtime here, including Runtime+80/current OptionName_N. This makes
                                            // playback identity independent of whether an earlier generic
                                            // 004EFE87 selection-telemetry hook happened to run for this input.
                                            if (IsReadableMemoryLocal(runtimeAddress, 0x8C) &&
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    runtimeAddress + 0x10) == Addr::TrailersRuntimeHash &&
                                                (*reinterpret_cast<const std::uint32_t*>(
                                                    runtimeAddress + 0x88) == kReplayMoviesNativeCatalogSlotCount ||
                                                    IsReplayMoviesUserProjectionPublishedLocal(runtimeAddress)))
                                            {
                                                const std::uintptr_t currentRow =
                                                    *reinterpret_cast<const std::uintptr_t*>(
                                                        runtimeAddress + 0x80);

                                                const std::size_t slotNumber =
                                                    GetReplayMoviesCatalogSlotNumber(
                                                        runtimeAddress,
                                                        currentRow);

                                                if (slotNumber != 0)
                                                {
                                                    const std::size_t resolvedIndex =
                                                        GetReplayMoviesCanonicalIndexForPhysicalSlotLocal(
                                                            slotNumber);

                                                    if (resolvedIndex <
                                                        ReplayMoviesCatalog::kCareerMovieCount)
                                                    {
                                                        g_ReplayMoviesSelectedDeveloperMovieIndex =
                                                            static_cast<unsigned long>(resolvedIndex);

                                                        ++g_ReplayMoviesCatalogSelectionUpdates;
                                                    }
                                                }
                                            }

                                            // Full-catalog activation: every authored OptionName_N control launches
                                            // through the proven native row-1 Trailers callback. Playback identity is
                                            // still determined independently by the catalog row/page that native
                                            // navigation committed, so the same launcher services all 17 movies.
                                            const std::size_t selectedIndex =
                                                static_cast<std::size_t>(
                                                    g_ReplayMoviesSelectedDeveloperMovieIndex);

                                            if (selectedIndex >= ReplayMoviesCatalog::kCareerMovieCount)
                                                return;

                                            const char* basename =
                                                ReplayMoviesCatalog::kCareerMovies[selectedIndex].madBasename;

                                            *reinterpret_cast<const char**>(
                                                video + 0x24) = basename;

                                            // This is the definitive movie-launch boundary. Preserve exactly which
                                            // catalog entry/page launched so the next fresh Trailers generation can
                                            // restore the same authored row instead of accepting native's default
                                            // OptionName_1 selection.
                                            std::size_t restorePage =
                                                static_cast<std::size_t>(
                                                    g_ReplayMoviesDeveloperCatalogPage);

                                            if (restorePage >= kReplayMoviesCatalogPageCount)
                                                restorePage = 0;

                                            std::uint32_t restoreHash = 0;
                                            const std::size_t physicalRestoreSlot =
                                                GetReplayMoviesPhysicalSlotForCanonicalLocal(selectedIndex);

                                            if (physicalRestoreSlot >= 1 &&
                                                physicalRestoreSlot <= kReplayMoviesNativeCatalogSlotCount)
                                            {
                                                if (physicalRestoreSlot <=
                                                    kReplayMoviesAuthoredCatalogSlotCount)
                                                {
                                                    // Physical User/Developer rows1..10 already exist when native
                                                    // post-FMV restoration runs.
                                                    restoreHash =
                                                        kReplayMoviesOptionControlHashes[
                                                            physicalRestoreSlot - 1];
                                                }
                                                else
                                                {
                                                    // Physical rows11..17 restore first through virtual native rows1..7
                                                    // while the fresh runtime still contains only ten authored rows.
                                                    const std::size_t virtualRestoreSlot =
                                                        physicalRestoreSlot -
                                                        kReplayMoviesAuthoredCatalogSlotCount;

                                                    restoreHash =
                                                        kReplayMoviesOptionControlHashes[
                                                            virtualRestoreSlot - 1];
                                                }
                                            }

                                            // A new movie launch supersedes any post-return demotion latch from the
                                            // previous movie.  It also invalidates the generation-local presentation
                                            // cache before the fresh post-FMV Trailers reconstruction begins; a reused
                                            // runtime address must never make stale FEObject/node pointers look valid.
                                            g_ReplayMoviesSyntheticPostReturnRuntime = 0;
                                            g_ReplayMoviesSyntheticPostReturnPhysicalSlot = 0;
                                            ResetReplayMoviesPresentationCacheLocal();

                                            g_PendingReplayMoviesSubmenuRestore = 1;
                                            g_PendingReplayMoviesSubmenuRestoreMovieIndex =
                                                static_cast<unsigned long>(selectedIndex);
                                            g_PendingReplayMoviesSubmenuRestorePage =
                                                static_cast<unsigned long>(restorePage);
                                            g_PendingReplayMoviesSubmenuRestoreHash = restoreHash;
                                            g_PendingReplayMoviesSubmenuRestoreWasSubstituted = 0;

                                            ++g_ReplayMoviesMovieIdentityOverrides;
                                        }


                                        static std::uint32_t __cdecl ResolvePendingReplayMoviesSubmenuRestoreHash(
                                            std::uint32_t requestedHash,
                                            std::uintptr_t packageNameAddress)
                                        {
                                            // Retail/fresh Trailers restores OptionName_1 by default after an FMV.
                                            // Only rewrite that exact request while ReplayMovies owns MU_Options_PC
                                            // and a movie-launch-generated restore request is armed.  For authored
                                            // rows1..10 the pending hash is the physical native row. For synthetic
                                            // rows11..17 it is the debugger-proven virtual native row1..7; physical
                                            // ownership is completed later by the 17-row bootstrap handoff.
                                            if (g_PendingReplayMoviesSubmenuRestore == 0 ||
                                                g_ReplayMoviesSubmenuActive == 0 ||
                                                requestedHash != 0x1AEC7D09 ||
                                                g_PendingReplayMoviesSubmenuRestoreHash == 0 ||
                                                packageNameAddress == 0 ||
                                                !AsciiStringEqualsLocal(
                                                    reinterpret_cast<const char*>(packageNameAddress),
                                                    "MU_Options_PC.fng"))
                                            {
                                                return requestedHash;
                                            }

                                            g_PendingReplayMoviesSubmenuRestoreWasSubstituted = 1;
                                            ++g_ReplayMoviesSubmenuRestoreSubstitutions;
                                            return g_PendingReplayMoviesSubmenuRestoreHash;
                                        }


                                        __declspec(noinline)
                                            static void __cdecl CapturePendingReplayMoviesNativeRestoreContext(
                                                std::uint32_t requestedHash,
                                                std::uintptr_t contextAddress)
                                        {
                                            if (g_PendingReplayMoviesMainMenuRestore == 0 ||
                                                requestedHash != Addr::OptionsHash ||
                                                contextAddress == 0 ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    Addr::MainMenuActionState) != Addr::OptionsActionState)
                                            {
                                                return;
                                            }

                                            if (!IsReadableMemoryLocal(contextAddress, 0x0C))
                                                return;

                                            g_ReplayMoviesNativeRestoreContext =
                                                contextAddress;

                                            ++g_ReplayMoviesNativeRestoreContextCaptures;
                                        }


                                        static void InvokeNativeMainMenuSelectionByHash(
                                            std::uint32_t targetHash,
                                            std::uintptr_t contextAddress)
                                        {
                                            // Captures(2026-09-02) completed the 004F5ED0 entry contract.
                                            // Native MU_Main restoration enters with:
                                            //   EAX     = 006C2B6C -> "MU_Main_PC.fng"
                                            //   ECX     = requested FEObject NameHash
                                            //   [ESP+4] = selection/restore context
                                            //
                                            // The earlier __thiscall wrapper reproduced ECX + stack only. EAX was
                                            // therefore garbage/the function address, causing the internal
                                            // 004F3F90 package lookup to return NULL and 004F5ED0 to exit before
                                            // candidate enumeration. Reproduce all three inputs explicitly.
                                            const std::uintptr_t nativeSelectAddress =
                                                g_NativeMuMainSelectionByHash;

                                            const std::uintptr_t packageNameAddress =
                                                Addr::MuMainPackageName;

                                            __asm
                                            {
                                                mov eax, packageNameAddress
                                                mov ecx, targetHash
                                                push contextAddress
                                                call dword ptr[nativeSelectAddress]
                                            }
                                        }


                                        static bool CompletePendingReplayMoviesNativeRestore(
                                            std::uintptr_t runtimeAddress)
                                        {
                                            if (g_PendingReplayMoviesMainMenuRestore == 0 ||
                                                g_ReplayMoviesNativeRestoreContext == 0 ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C))
                                            {
                                                return false;
                                            }

                                            auto* runtime =
                                                reinterpret_cast<std::uint8_t*>(
                                                    runtimeAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                runtime + 0x10) != Addr::MuMainHash ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    runtime + 0x88) != 7)
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t vectorAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    runtime + 0x84);

                                            if (vectorAddress == 0 ||
                                                !IsReadableMemoryLocal(
                                                    vectorAddress,
                                                    7 * sizeof(std::uintptr_t)))
                                            {
                                                return false;
                                            }

                                            const auto* candidates =
                                                reinterpret_cast<const std::uintptr_t*>(
                                                    vectorAddress);

                                            const std::uintptr_t replayAddress =
                                                candidates[6];

                                            if (!IsReadableMemoryLocal(replayAddress, 0x58) ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    replayAddress + 0x10) != Addr::ReplayMoviesHash)
                                            {
                                                return false;
                                            }

                                            const std::uintptr_t contextAddress =
                                                g_ReplayMoviesNativeRestoreContext;

                                            if (!IsReadableMemoryLocal(contextAddress, 0x0C))
                                            {
                                                ++g_ReplayMoviesNativeRestoreFailures;
                                                return false;
                                            }

                                            // The direct 004F5ED0 restore bypasses the ordinary 004EFE87
                                            // ReplayMovies pre-commit path. Prepare only the debugger-proven shared
                                            // H1 selected Y here. ReplayMovies +54 must remain on its root/startup
                                            // node until the native Main Menu entrance lifecycle advances normally;
                                            // forcing +54 to the unselected node at this generation boundary
                                            // reintroduces the old premature-render defect.
                                            if (!PrepareReplayMoviesPostReturnNativeSelection(
                                                runtimeAddress,
                                                replayAddress))
                                            {
                                                ++g_ReplayMoviesNativeRestoreFailures;
                                                return false;
                                            }

                                            ++g_ReplayMoviesNativeRestoreAttempts;

                                            // Replay the exact native selection-by-hash transaction after the
                                            // ReplayMovies FE_String has been built and published, but still inside
                                            // the MU_Main generation completion hook before control returns to the
                                            // frontend renderer/input loop. The wrapper reproduces the now-complete
                                            // native EAX/ECX/stack entry contract.
                                            InvokeNativeMainMenuSelectionByHash(
                                                Addr::ReplayMoviesHash,
                                                contextAddress);

                                            const bool restored =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    runtime + 0x80) == replayAddress;

                                            if (restored)
                                            {
                                                ++g_ReplayMoviesNativeRestoreSuccess;

                                                // 004EFE87 normally consumes the request through the existing
                                                // commit observer. Clear defensively as well in case this native
                                                // path committed internally without visiting that detour.
                                                g_PendingReplayMoviesMainMenuRestore = 0;
                                                g_ReplayMoviesNativeRestoreContext = 0;
                                                return true;
                                            }

                                            ++g_ReplayMoviesNativeRestoreFailures;
                                            return false;
                                        }


                                        __declspec(naked)
                                            static void MuMainNativeSelectionByHashHook()
                                        {
                                            __asm
                                            {
                                                // Original six bytes:
                                                // 004F5ED0  push edi
                                                // 004F5ED1  push 00746104
                                                //
                                                // At native entry:
                                                //   ECX     = requested candidate NameHash
                                                //   [ESP+4] = selection/restore context pointer
                                                //
                                                // Capture only the native Options restoration transaction that
                                                // occurs while ReplayMovies Back has armed a one-shot restore.
                                                pushfd
                                                pushad

                                                // pushfd + pushad moved the original entry stack down by 0x24.
                                                // Saved ECX is [esp+18], saved EAX is [esp+1C], and the original
                                                // explicit context argument is [esp+28].
                                                mov eax, dword ptr[esp + 18h]  // requested NameHash
                                                mov ecx, dword ptr[esp + 1Ch]  // native package-name pointer

                                                push ecx
                                                push eax
                                                call ResolvePendingReplayMoviesSubmenuRestoreHash
                                                add esp, 8

                                                // popad will restore ECX from this slot, so replacing the saved value
                                                // reproduces the native call with only the requested FEObject hash
                                                // changed. EAX/package and the explicit context remain untouched.
                                                mov dword ptr[esp + 18h], eax

                                                mov edx, dword ptr[esp + 28h]
                                                push edx
                                                push eax
                                                call CapturePendingReplayMoviesNativeRestoreContext
                                                add esp, 8

                                                popad
                                                popfd

                                                // Replay the two overwritten native instructions.
                                                push edi
                                                push 00746104h

                                                jmp dword ptr[g_MuMainNativeSelectionReturn]
                                            }
                                        }


                                        __declspec(noinline)
                                            static void __cdecl ObserveCompletedRuntime(
                                                std::uintptr_t runtimeAddress)
                                        {
                                            if (runtimeAddress == 0)
                                                return;

                                            auto* runtime =
                                                reinterpret_cast<std::uint8_t*>(
                                                    runtimeAddress);

                                            ConfigureReplayMoviesDeveloperCatalogBootstrap(
                                                runtimeAddress);


                                            // ---------------------------------------------------------
                                            // Qualify MU_Main_PC.
                                            // Runtime + 0x10 = package hash.
                                            // ---------------------------------------------------------

                                            if (*reinterpret_cast<std::uint32_t*>(
                                                runtime + 0x10) != Addr::MuMainHash)
                                            {
                                                return;
                                            }

                                            // Contain the temporary borrowed Trailers shell. Back keeps the native
                                            // ID A/0x10 state alive long enough for the established Main Menu parent
                                            // restoration transaction; once the rebuilt MU_Main runtime is complete,
                                            // restore the values that existed before ReplayMovies was entered.
                                            RestoreReplayMoviesBorrowedRouteState();


                                            // ---------------------------------------------------------
                                            // Phase 1 contract:
                                            // native candidate capacity must already be seven.
                                            // ---------------------------------------------------------

                                            if (*reinterpret_cast<std::uint32_t*>(
                                                runtime + 0x88) != 7)
                                            {
                                                return;
                                            }


                                            // ---------------------------------------------------------
                                            // Acquire this generation's candidate vector.
                                            //
                                            // Never cache this address between MU_Main generations.
                                            // ---------------------------------------------------------

                                            const std::uintptr_t vectorAddress =
                                                *reinterpret_cast<std::uintptr_t*>(
                                                    runtime + 0x84);

                                            if (vectorAddress == 0)
                                                return;

                                            auto* candidates =
                                                reinterpret_cast<std::uintptr_t*>(
                                                    vectorAddress);


                                            // ---------------------------------------------------------
                                            // Native slot 5 must be UndergroundMode.
                                            // Safety slot 6 must currently duplicate slot 5.
                                            // ---------------------------------------------------------

                                            const std::uintptr_t underground =
                                                candidates[5];

                                            const std::uintptr_t safety =
                                                candidates[6];

                                            if (underground == 0)
                                                return;


                                            auto* undergroundObject =
                                                reinterpret_cast<std::uint8_t*>(
                                                    underground);


                                            // UndergroundMode NameHash.
                                            if (*reinterpret_cast<std::uint32_t*>(
                                                undergroundObject + 0x10)
                                                != Addr::UndergroundModeHash)
                                            {
                                                return;
                                            }


                                            // ---------------------------------------------------------
                                            // Phase 1B contract:
                                            //
                                            // candidate[6] must still be the safe duplicate of
                                            // UndergroundMode before ReplayMovies is constructed.
                                            // ---------------------------------------------------------

                                            if (safety != underground)
                                                return;


                                            // ---------------------------------------------------------
                                            // Completed MU_Main generation is fully qualified.
                                            // Record transient addresses for validation/telemetry.
                                            // ---------------------------------------------------------

                                            g_LastMuMainRuntime =
                                                runtimeAddress;

                                            g_LastMuMainVector =
                                                vectorAddress;

                                            g_LastUndergroundMode =
                                                underground;

                                            ++g_MuMainGenerationHits;

                                            // A completed MU_Main generation means we are no longer owned by the
                                            // ReplayMovies movie-list screen. This also prevents stale prototype
                                            // state from surviving a later frontend reconstruction.
                                            g_ReplayMoviesSubmenuActive = 0;
                                            g_ReplayMoviesCatalogRuntimeAddress = 0;
                                            ResetReplayMoviesPresentationCacheLocal();
                                            ResetReplayMoviesUserProjectionLocal();
                                            g_ReplayMoviesSyntheticActivationRuntime = 0;
                                            g_ReplayMoviesSyntheticActivationRow = 0;
                                            g_ReplayMoviesSyntheticActivationActive = 0;
                                            g_ReplayMoviesSyntheticActivationReturnArmed = 0;
                                            g_ReplayMoviesNotificationOriginalReturn = 0;
                                            g_PendingReplayMoviesSubmenuRestore = 0;
                                            g_PendingReplayMoviesSubmenuRestoreHash = 0;
                                            g_PendingReplayMoviesSubmenuRestoreWasSubstituted = 0;
                                            g_ReplayMoviesSyntheticPostReturnRuntime = 0;
                                            g_ReplayMoviesSyntheticPostReturnPhysicalSlot = 0;


                                            // Build the actual generation-local ReplayMovies object.
                                            // BuildReplayMoviesForGeneration is fail-closed:
                                            // candidate[6] remains UndergroundMode unless construction,
                                            // configuration and package registration all succeed.

                                            const bool replayBuilt =
                                                BuildReplayMoviesForGeneration(
                                                    runtimeAddress,
                                                    vectorAddress,
                                                    underground);

                                            // v7.4: Back from ReplayMovies is restored through the same native
                                            // 004F5ED0 selection-by-hash worker that restored the authored parent
                                            // row earlier in this reconstruction. ReplayMovies now exists in
                                            // candidate[6], so the native transaction can genuinely select it
                                            // before control returns to the frontend renderer/input loop.
                                            if (replayBuilt &&
                                                g_PendingReplayMoviesMainMenuRestore != 0)
                                            {
                                                CompletePendingReplayMoviesNativeRestore(
                                                    runtimeAddress);
                                            }
                                        }


                                        __declspec(noinline)
                                            static std::uintptr_t __cdecl ResolvePendingReplayMoviesRestoreCandidate(
                                                std::uintptr_t runtimeAddress,
                                                std::uintptr_t nativeCandidateAddress)
                                        {
                                            nativeCandidateAddress =
                                                ResolveReplayMoviesCatalogNavigationCandidate(
                                                    runtimeAddress,
                                                    nativeCandidateAddress);

                                            if (g_PendingReplayMoviesMainMenuRestore == 0 ||
                                                runtimeAddress == 0)
                                            {
                                                return nativeCandidateAddress;
                                            }

                                            auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    runtimeAddress);

                                            // The candidate-writer sites are generic frontend code. Redirect only
                                            // a completed seven-row MU_Main generation whose candidate[6] is the
                                            // current generation's real ReplayMovies FE_String.
                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                runtime + 0x10) != Addr::MuMainHash ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    runtime + 0x88) != 7)
                                            {
                                                return nativeCandidateAddress;
                                            }

                                            const std::uintptr_t vectorAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    runtime + 0x84);

                                            if (vectorAddress == 0)
                                                return nativeCandidateAddress;

                                            const auto* candidates =
                                                reinterpret_cast<const std::uintptr_t*>(
                                                    vectorAddress);

                                            const std::uintptr_t replayAddress =
                                                candidates[6];

                                            if (replayAddress == 0)
                                                return nativeCandidateAddress;

                                            const auto* replay =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    replayAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                replay + 0x10) != Addr::ReplayMoviesHash)
                                            {
                                                // Before final publication candidate[6] is still the safe
                                                // UndergroundMode duplicate. Keep the request armed until the
                                                // generation hook publishes the real ReplayMovies object.
                                                return nativeCandidateAddress;
                                            }

                                            if (nativeCandidateAddress != replayAddress)
                                                ++g_ReplayMoviesRestoreCandidateSubstitutions;

                                            // IMPORTANT: do not consume the one-shot here. The navigation routine
                                            // has two candidate-producing branches (004EFCB5 and 004EFDE2). Keep
                                            // both forced to the same ReplayMovies object until the later native
                                            // current-row commit proves the transition completed.
                                            return replayAddress;
                                        }


                                        static void __cdecl FinalizePendingReplayMoviesMainMenuRestore(
                                            std::uintptr_t runtimeAddress,
                                            std::uintptr_t incomingRowAddress)
                                        {
                                            if (g_PendingReplayMoviesMainMenuRestore == 0 ||
                                                runtimeAddress == 0)
                                            {
                                                return;
                                            }

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    runtimeAddress);

                                            // Ignore unrelated generic frontend commits; keep waiting for MU_Main.
                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                runtime + 0x10) != Addr::MuMainHash ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    runtime + 0x88) != 7)
                                            {
                                                return;
                                            }

                                            const std::uintptr_t vectorAddress =
                                                *reinterpret_cast<const std::uintptr_t*>(
                                                    runtime + 0x84);

                                            if (vectorAddress == 0)
                                                return;

                                            const auto* candidates =
                                                reinterpret_cast<const std::uintptr_t*>(
                                                    vectorAddress);

                                            const std::uintptr_t replayAddress =
                                                candidates[6];

                                            if (replayAddress == 0)
                                                return;

                                            const auto* replay =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    replayAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(
                                                replay + 0x10) != Addr::ReplayMoviesHash)
                                            {
                                                return;
                                            }

                                            // Expire at the first qualified post-publication MU_Main commit.
                                            // Success means ReplayMovies arrived here naturally through the native
                                            // transition machinery after our earlier candidate substitution.
                                            if (incomingRowAddress == replayAddress)
                                                ++g_ReplayMoviesRestoreCommitSuccess;
                                            else
                                                ++g_ReplayMoviesRestoreCommitMiss;

                                            g_PendingReplayMoviesMainMenuRestore = 0;
                                        }


                                        __declspec(naked)
                                            static void MuMainRestoreCandidatePrimaryHook()
                                        {
                                            __asm
                                            {
                                                // Original 8 bytes:
                                                // 004EFCB5  mov [esp+1C],eax
                                                // 004EFCB9  mov eax,[esp+20]
                                                //
                                                // EAX is the candidate returned by native 004FA140. Redirect it
                                                // here, before any of the native row-transition commands execute.
                                                pushfd
                                                pushad

                                                push eax
                                                push ebx
                                                call ResolvePendingReplayMoviesRestoreCandidate
                                                add esp, 8

                                                // pushad saved EAX = [esp+1C].
                                                mov dword ptr[esp + 1Ch], eax

                                                popad
                                                popfd

                                                // Replay both whole native instructions.
                                                mov dword ptr[esp + 1Ch], eax
                                                mov eax, dword ptr[esp + 20h]

                                                jmp dword ptr[g_MuMainRestoreCandidatePrimaryReturn]
                                            }
                                        }


                                        __declspec(naked)
                                            static void MuMainRestoreCandidateLoopHook()
                                        {
                                            __asm
                                            {
                                                // Original 9 bytes:
                                                // 004EFDE2  mov [esp+1C],eax
                                                // 004EFDE6  jmp 004EFCCB
                                                //
                                                // This alternate resolver branch writes the same candidate local.
                                                pushfd
                                                pushad

                                                push eax
                                                push ebx
                                                call ResolvePendingReplayMoviesRestoreCandidate
                                                add esp, 8

                                                mov dword ptr[esp + 1Ch], eax

                                                popad
                                                popfd

                                                mov dword ptr[esp + 1Ch], eax
                                                jmp dword ptr[g_MuMainRestoreCandidateLoopReturn]
                                            }
                                        }


                                        __declspec(naked)
                                            static void MuMainCurrentRowCommitHook()
                                        {
                                            __asm
                                            {
                                                // =====================================================
                                                // Original code:
                                                //
                                                // 004EFE87  mov dword ptr [ebx+80],esi
                                                //
                                                // EBX = owning frontend runtime/state
                                                // ESI = incoming FEObject
                                                // =====================================================

                                                pushfd
                                                pushad

                                                // v7.2 no longer substitutes the row at this late boundary.
                                                // It only verifies/consumes the one-shot restore request. By this
                                                // point the native transition machinery must already have selected
                                                // ReplayMovies via the upstream 004FA140 candidate-writer hooks.
                                                push esi
                                                push ebx
                                                call FinalizePendingReplayMoviesMainMenuRestore
                                                add esp, 8

                                                push esi
                                                push ebx
                                                call ObserveReplayMoviesCatalogCurrentRowCommit
                                                add esp, 8

                                                // PRE-COMMIT:
                                                // old row is still [EBX+80], incoming row is the native ESI.
                                                mov ecx, esi
                                                mov eax, dword ptr[ebx + 80h]

                                                push ecx
                                                push eax
                                                push ebx
                                                call ApplyReplayMoviesVisualTransitionPreCommit
                                                add esp, 0Ch

                                                popad
                                                popfd

                                                // Replay the overwritten native instruction.
                                                mov dword ptr[ebx + 80h], esi

                                                jmp dword ptr[g_MuMainCurrentRowCommitReturn]
                                            }
                                        }


                                        static std::uintptr_t __cdecl
                                            ResolveReplayMoviesActiveNodeWriteForHookLocal(
                                                std::uintptr_t runtimeAddress,
                                                std::uintptr_t parentObjectAddress,
                                                std::uintptr_t incomingNodeAddress)
                                        {
                                            // Preserve the already debugger-proven MU_Main transition resolver first.
                                            // If it substitutes a node for ReplayMovies -> UndergroundMode, that result
                                            // wins and the ReplayMovies-submenu zero-candidate rule below does not
                                            // interfere with it.
                                            const std::uintptr_t resolvedNode =
                                                ResolveReplayMoviesActiveNodeWrite(
                                                    parentObjectAddress,
                                                    incomingNodeAddress);

                                            if (resolvedNode != incomingNodeAddress)
                                                return resolvedNode;

                                            // COUNT=0 User-catalogue late-writer proof (2026-09-14):
                                            //
                                            //   004F1527 zero projection -> Highlight_01 +54 == dormant/N2
                                            //   later 004F1116 hook      -> same Highlight_01 receives selected/N1
                                            //   exact writer repeats twice while the submenu settles
                                            //   substituting N2 on BOTH writes leaves the horizontal bar gone
                                            //   CURRENT=0 / COUNT=0 and Back remain native-safe.
                                            //
                                            // Therefore this must be an event-by-event substitution at the active-node
                                            // writer, not a one-shot post-generation demotion. Fail closed unless this
                                            // is the exact live zero-row User projection and the write targets
                                            // ReplayMovies Highlight_01's selected/N1 node.
                                            if (ReplayMoviesConfig::GetAccessMode() !=
                                                ReplayMoviesConfig::AccessMode::User ||
                                                g_ReplayMoviesSubmenuActive == 0 ||
                                                g_ReplayMoviesUserProjectionActive == 0 ||
                                                g_ReplayMoviesUserProjectionRuntime != runtimeAddress ||
                                                g_ReplayMoviesUserVisibleCount != 0 ||
                                                g_ReplayMoviesUserSeenMask != 0 ||
                                                !IsReadableMemoryLocal(runtimeAddress, 0x8C))
                                            {
                                                return resolvedNode;
                                            }

                                            const auto* runtime =
                                                reinterpret_cast<const std::uint8_t*>(runtimeAddress);

                                            if (*reinterpret_cast<const std::uint32_t*>(runtime + 0x10) !=
                                                Addr::TrailersRuntimeHash ||
                                                *reinterpret_cast<const std::uintptr_t*>(runtime + 0x80) != 0 ||
                                                *reinterpret_cast<const std::uint32_t*>(runtime + 0x88) != 0 ||
                                                !IsReadableMemoryLocal(parentObjectAddress, 0x58) ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    parentObjectAddress + 0x10) !=
                                                kReplayMoviesHighlightHashes[0] ||
                                                *reinterpret_cast<const std::uint32_t*>(
                                                    parentObjectAddress + 0x18) != 1)
                                            {
                                                return resolvedNode;
                                            }

                                            const std::uintptr_t selectedNode =
                                                GetReplayMoviesPresentationNodeLocal(
                                                    parentObjectAddress,
                                                    true);
                                            const std::uintptr_t dormantNode =
                                                GetReplayMoviesPresentationNodeLocal(
                                                    parentObjectAddress,
                                                    false);

                                            if (selectedNode == 0 ||
                                                dormantNode == 0 ||
                                                incomingNodeAddress != selectedNode)
                                            {
                                                return resolvedNode;
                                            }

                                            // Exact production promotion of the successful two-hit x32dbg A/B:
                                            // native MOV + 004FF030 still execute normally, but ESI carries N2.
                                            return dormantNode;
                                        }


                                        __declspec(naked)
                                            static void MuMainActiveNodeWriterHook()
                                        {
                                            __asm
                                            {
                                                // =====================================================
                                                // Original sequence:
                                                //
                                                // 004F1116  mov [eax+54],esi
                                                // 004F1119  call 004FF030
                                                //
                                                // EAX = parent FEObject
                                                // ESI = subordinate node selected by native machinery
                                                //
                                                // Two debugger-proven substitutions are resolved BEFORE the
                                                // native MOV: the MU_Main ReplayMovies -> UndergroundMode H1
                                                // correction, and the ReplayMovies User COUNT=0 Highlight_01
                                                // selected/N1 -> dormant/N2 suppression that must apply on every
                                                // repeated settling write.
                                                // =====================================================

                                                pushfd
                                                pushad

                                                push esi
                                                push eax
                                                push ebx
                                                call ResolveReplayMoviesActiveNodeWriteForHookLocal
                                                add esp, 0Ch

                                                // pushad stack layout at this point:
                                                // [esp+00] EDI
                                                // [esp+04] ESI
                                                // ...
                                                // Store helper return value into saved ESI so popad
                                                // restores either the original node or the proven
                                                // selected-node substitute.
                                                mov dword ptr[esp + 04h], eax

                                                popad
                                                popfd

                                                // Replay both native instructions covered by the hook.
                                                mov dword ptr[eax + 54h], esi
                                                call dword ptr[g_NativePostActiveNodeWrite]

                                                // FINAL/POST-HOOK ReplayMovies submenu synchronizer. The debugger
                                                // proved that the earlier pre-commit correction ran too soon: the
                                                // existing hook could subsequently put H10 back on N1. Run after the
                                                // native 004FF030 transaction so rows 10..17 finish with exactly one
                                                // Text/Highlight N1 pair and every other companion on N2.
                                                pushfd
                                                pushad
                                                push 0
                                                push ebx
                                                call SynchronizeReplayMoviesPostHookVisualsLocal
                                                add esp, 8

                                                // v11.4 post-return persistence.  The debugger proved native/frontend
                                                // settling can reassert the virtual authored row's N1 state multiple
                                                // times at this exact 004F1116 transaction.  After the existing
                                                // synthetic synchronizer finishes, immediately re-demote only the
                                                // mapped virtual row for the verified synthetic post-return mapping.
                                                push ebx
                                                call MaintainReplayMoviesSyntheticPostReturnVirtualDemotionLocal
                                                add esp, 4

                                                popad
                                                popfd

                                                // Resume after the original CALL.
                                                jmp dword ptr[g_MuMainActiveNodeWriterReturn]
                                            }
                                        }


                                        __declspec(naked)
                                            static void LiveGeometryWriterHook()
                                        {
                                            __asm
                                            {
                                                // Original six bytes:
                                                // 004FC374  mov [ebx+04],eax
                                                // 004FC377  mov [ebx+08],ecx
                                                //
                                                // The helper normally returns EAX unchanged. It substitutes only
                                                // the exact debugger-proven ReplayMovies -> QuickRace H1 transient.
                                                pushfd
                                                pushad

                                                push eax
                                                push ebx
                                                call ResolveReplayMoviesQuickRaceTransientYWrite
                                                add esp, 8

                                                // pushad saved EAX is at [esp+1C].
                                                mov dword ptr[esp + 1Ch], eax

                                                popad
                                                popfd

                                                // Replay both overwritten native instructions.
                                                mov dword ptr[ebx + 04h], eax
                                                mov dword ptr[ebx + 08h], ecx

                                                jmp dword ptr[g_LiveGeometryWriterReturn]
                                            }
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesTrailerMovieVideoHook()
                                        {
                                            __asm
                                            {
                                                // Hook entry stack is the native 004F5860 entry:
                                                //   [ESP+04] = current type-7 Video FEObject*.
                                                // Preserve every register/flag while the helper applies only the
                                                // ReplayMovies-owned basename substitution.
                                                pushfd
                                                pushad

                                                // pushfd + pushad move the original [ESP+04] argument to [ESP+28].
                                                // Saved native EBX is [ESP+10]; native captures proved EBX remains
                                                // the owning Trailers runtime at this callback.
                                                mov eax, dword ptr[esp + 28h]
                                                mov edx, dword ptr[esp + 10h]
                                                push edx
                                                push eax
                                                call ApplyReplayMoviesDeveloperMovieIdentity
                                                add esp, 8

                                                popad
                                                popfd

                                                // Replay the exact first seven native bytes.
                                                mov eax, dword ptr[esp + 04h]
                                                mov ecx, dword ptr[eax + 18h]

                                                jmp dword ptr[g_TrailerMovieVideoCallbackReturn]
                                            }
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesDescriptionFadeHook()
                                        {
                                            __asm
                                            {
                                                mov edi, dword ptr[ebp + 40h]

                                                mov ecx, dword ptr[esp + 3Ch]
                                                test ecx, ecx
                                                je NativeFade

                                                cmp dword ptr[ecx + 10h], 0A1E9FD7Fh
                                                jne NativeFade

                                                pushfd
                                                pushad

                                                mov eax, edi
                                                push offset g_ReplayMoviesDescription
                                                call dword ptr[g_FEngPrintfObj]
                                                add esp, 4

                                                inc dword ptr[g_ReplayMoviesDescriptionUpdates]

                                                popad
                                                popfd

                                                NativeFade :
                                                push ebx
                                                    push 006CBF28h
                                                    jmp dword ptr[g_MuMainDescriptionFadeReturn]
                                            }
                                        }


                                        __declspec(noinline)
                                            static void __cdecl RefreshReplayMoviesConfigurationForEntryLocal()
                                        {
                                            // Re-read the INI on every fresh Main Menu -> ReplayMovies entry.
                                            // This makes Mode changes effective after leaving/re-entering the
                                            // submenu without any process-global startup dependency.
                                            ReplayMoviesConfig::Refresh();
                                            ResetReplayMoviesUserProjectionLocal();

                                            // Profile identity/progression storage is intentionally NOT
                                            // discovered or refreshed from ReplayMovies entry. Native profile
                                            // create/load/save lifecycle hooks own that responsibility; this
                                            // path only re-reads ReplayMovies.ini and consumes already-published
                                            // profile state. The debugger-proven catalogue entry/post-return
                                            // mapping remains untouched.
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesDispatchHook()
                                        {
                                            __asm
                                            {
                                                // =====================================================
                                                // Hooked native decision:
                                                //
                                                // 004B33FA  cmp eax,95AD096B      ; Options
                                                // 004B33FF  jne 004B3769
                                                // 004B3405  ...                   ; native Options route
                                                //
                                                // EAX already contains the selected Main Menu FEObject NameHash.
                                                // This site is reached only through the native MU_Main action /
                                                // Enter notification path. Selection/highlight messages that take
                                                // other 004B3240 branches never reach this decision.
                                                // =====================================================

                                                cmp eax, 0A1E9FD7Fh
                                                je ReplayMoviesAction

                                                // Preserve the overwritten native decision exactly for Options.
                                                cmp eax, 095AD096Bh
                                                je NativeOptions

                                                // Any unrelated/unrecognized hash retains the native fallback.
                                                jmp dword ptr[g_MuMainDispatchFallback]


                                                ReplayMoviesAction:

                                                // Refresh ReplayMovies.ini before this submenu generation is
                                                // created. pushad/pushfd keep the native dispatcher register/flag
                                                // contract intact while the ordinary C++ configuration reader runs.
                                                pushfd
                                                    pushad
                                                    call RefreshReplayMoviesConfigurationForEntryLocal
                                                    popad
                                                    popfd

                                                    // Save the user's/native Options route state before temporarily
                                                    // borrowing selector ID A to construct the retail Trailers shell.
                                                    pushfd
                                                    pushad
                                                    call SaveReplayMoviesBorrowedRouteState
                                                    popad
                                                    popfd

                                                    // A new Main Menu -> ReplayMovies entry begins at movie 0 in
                                                    // the single physical 17-row catalog. Native OptionName_N commits
                                                    // update the selected movie index afterward.
                                                    mov dword ptr[g_ReplayMoviesSelectedDeveloperMovieIndex], 0
                                                    mov dword ptr[g_ReplayMoviesDeveloperCatalogPage], 0
                                                    mov dword ptr[g_ReplayMoviesCatalogPendingRuntime], 0
                                                    mov dword ptr[g_ReplayMoviesCatalogPendingIncomingRow], 0
                                                    mov dword ptr[g_ReplayMoviesCatalogPendingPage], 0
                                                    mov dword ptr[g_ReplayMoviesCatalogPendingPageValid], 0
                                                    mov dword ptr[g_ReplayMoviesCatalogRuntimeAddress], 0
                                                    mov dword ptr[g_ReplayMoviesPresentationCacheRuntime], 0
                                                    mov dword ptr[g_ReplayMoviesPresentationCacheValid], 0
                                                    mov dword ptr[g_ReplayMoviesPresentationLastRuntime], 0
                                                    mov dword ptr[g_ReplayMoviesPresentationLastCurrentRow], 0
                                                    mov dword ptr[g_ReplayMoviesSyntheticActivationRuntime], 0
                                                    mov dword ptr[g_ReplayMoviesSyntheticActivationRow], 0
                                                    mov dword ptr[g_ReplayMoviesSyntheticActivationActive], 0
                                                    mov dword ptr[g_ReplayMoviesSyntheticActivationReturnArmed], 0
                                                    mov dword ptr[g_ReplayMoviesNotificationOriginalReturn], 0
                                                    mov dword ptr[g_ReplayMoviesSyntheticPostReturnRuntime], 0
                                                    mov dword ptr[g_ReplayMoviesSyntheticPostReturnPhysicalSlot], 0

                                                    // Telemetry proves the native Enter/action path reached us.
                                                    inc dword ptr[g_ReplayMoviesDispatchHits]
                                                    mov dword ptr[g_ReplayMoviesSubmenuActive], 1

                                                    // -----------------------------------------------------
                                                    // Direct native Cinematics/Trailers-mode prototype.
                                                    //
                                                    // GameTrailers is selector ID A. Native Options uses this saved
                                                    // selector ID to configure MU_Options_PC.fng as its Trailers movie
                                                    // list, whose sole active callback is +40 = 004D0B20.
                                                    //
                                                    // We deliberately bypass MU_OptionsSelector_PC.fng: there is no
                                                    // visible/logical GameTrailers row between ReplayMovies and the
                                                    // movie list.
                                                    // -----------------------------------------------------
                                                    // Mirror the internal state that exists after the native Main Menu
                                                    // Options route, but skip the visible Options selector itself.
                                                    mov dword ptr ds : [00777B4Ch] , 10h
                                                    mov dword ptr ds : [00735E20h] , 0Ah

                                                    // Mirror the proven native FEngSwitchPackages call shape used by
                                                    // Main Menu routes:
                                                    //   push [current MenuScreen +0C package filename]
                                                    //   EDI = destination package filename
                                                    //   call 004F6360
                                                    //
                                                    // At this MU_Main handler boundary EBP is the current MenuScreen,
                                                    // and [EBP+0C] is MU_Main_PC.fng.
                                                    mov ecx, dword ptr[ebp + 0Ch]
                                                    push ecx
                                                    mov edi, 006C2C10h              // "MU_Options_PC.fng"
                                                    call dword ptr[g_FEngSwitchPackages]
                                                    add esp, 4

                                                    // Native GameTrailers does the same caller cleanup immediately
                                                    // after 004F6360. v7 omitted this DWORD cleanup, shifting MU_Main's
                                                    // saved-register/return frame and causing the observed exception.

                                                    // The package switch is the ReplayMovies action. Finish through
                                                    // the native MU_Main handler epilogue, as the tested dispatcher
                                                    // cave did previously.
                                                    jmp dword ptr[g_MuMainDispatchFallback]


                                                    NativeOptions:
                                                jmp dword ptr[g_MuMainOptionsRoute]
                                            }
                                        }


                                        __declspec(naked)
                                            static void ReplayMoviesOptionsBackRouteHook()
                                        {
                                            __asm
                                            {
                                                // =====================================================
                                                // Original six bytes @ 004CF338:
                                                //
                                                //   push eax
                                                //   mov  edi,006C25E0   ; MU_OptionsSelector_PC.fng
                                                //
                                                // 004CF335 has already executed:
                                                //   mov eax,[edi+0C]     ; current MU_Options_PC.fng source
                                                //
                                                // Native execution resumes at 004CF33E, which calls 004F6360
                                                // and then performs its own `add esp,4`.
                                                // =====================================================

                                                cmp dword ptr[g_ReplayMoviesSubmenuActive], 0
                                                je NativeOptionsBack

                                                inc dword ptr[g_ReplayMoviesExitHits]

                                                // Keep the borrowed ID A/0x10 state alive through the native
                                                // MU_Main reconstruction. The v7.4 parent-restore capture at
                                                // 004F5ED0 still requires that exact native Options action state.
                                                // ObserveCompletedRuntime restores the saved globals as soon as the
                                                // rebuilt MU_Main runtime is qualified, before user input resumes.
                                                mov dword ptr[g_ReplayMoviesNativeRestoreContext], 0
                                                mov dword ptr[g_PendingReplayMoviesMainMenuRestore], 1

                                                // Preserve the native source-package argument, but specialize the
                                                // destination to a fresh MU_Main_PC.fng reconstruction.
                                                push eax
                                                mov edi, dword ptr[g_MuMainPackageName]
                                                jmp dword ptr[g_MuOptionsBackRouteReturn]

                                                NativeOptionsBack:
                                                push eax
                                                    mov edi, dword ptr[g_MuOptionsSelectorPackageName]
                                                    jmp dword ptr[g_MuOptionsBackRouteReturn]
                                            }
                                        }


                                        // -------------------------------------------------------------
                                        // v11.4 Race -> Quit Game completed-generation fallback.
                                        //
                                        // Runtime A/B on 2026-10-04 proved that this native MU_Main
                                        // reconstruction route reaches 004F5EF2 with a fully populated
                                        // seven-entry vector, but bypasses the primary 004F1521 completed-
                                        // generation observer entirely. At this boundary ESI is the resolved
                                        // runtime and the exact pre-build safety state is already present.
                                        //
                                        // Qualify that exact state before invoking the existing observer.
                                        // Normal generations are inert because candidate[6] has already been
                                        // replaced by ReplayMovies by the time native selection reaches this
                                        // scan.
                                        // -------------------------------------------------------------
                                        __declspec(noinline)
                                            static void __cdecl ObserveCompletedRuntimeFallback(
                                                std::uintptr_t runtimeAddress)
                                        {
                                            if (runtimeAddress == 0)
                                                return;

                                            auto* runtime =
                                                reinterpret_cast<std::uint8_t*>(
                                                    runtimeAddress);

                                            if (*reinterpret_cast<std::uint32_t*>(
                                                runtime + 0x10) != Addr::MuMainHash)
                                            {
                                                return;
                                            }

                                            if (*reinterpret_cast<std::uint32_t*>(
                                                runtime + 0x88) != 7)
                                            {
                                                return;
                                            }

                                            const std::uintptr_t vectorAddress =
                                                *reinterpret_cast<std::uintptr_t*>(
                                                    runtime + 0x84);

                                            if (vectorAddress == 0)
                                                return;

                                            auto* candidates =
                                                reinterpret_cast<std::uintptr_t*>(
                                                    vectorAddress);

                                            const std::uintptr_t underground =
                                                candidates[5];

                                            if (underground == 0 ||
                                                candidates[6] != underground)
                                            {
                                                return;
                                            }

                                            if (*reinterpret_cast<std::uint32_t*>(
                                                reinterpret_cast<std::uint8_t*>(underground) + 0x10)
                                                != Addr::UndergroundModeHash)
                                            {
                                                return;
                                            }

                                            ObserveCompletedRuntime(runtimeAddress);
                                        }


                                        __declspec(naked)
                                            static void MuMainSelectionPreScanHook()
                                        {
                                            __asm
                                            {
                                                // Original six bytes @ 004F5EF2:
                                                //   mov eax,[esi+88]
                                                //
                                                // ESI is a non-null runtime resolved by the native
                                                // select/restore-by-hash worker. Preserve the complete
                                                // native context while the fail-closed fallback checks
                                                // whether this is an unobserved completed MU_Main
                                                // generation.
                                                pushfd
                                                pushad

                                                push esi
                                                call ObserveCompletedRuntimeFallback
                                                add esp, 4

                                                popad
                                                popfd

                                                // Replay the overwritten instruction. MOV does not alter
                                                // flags, so the native flag state remains exact.
                                                mov eax, dword ptr[esi + 88h]

                                                jmp dword ptr[g_MuMainSelectionPreScanReturn]
                                            }
                                        }


                                        __declspec(naked)
                                            static void MuMainGenerationHook()
                                        {
                                            __asm
                                            {
                                                // =====================================================
                                                // Original code:
                                                //
                                                // 004F151F  test eax,eax
                                                // 004F1521  je   004F1545
                                                // 004F1523  test byte ptr [esi+14],1
                                                // 004F1527  jne  004F154B
                                                //
                                                // Our hook begins at 004F1521.
                                                //
                                                // Therefore 004F151F has already executed before
                                                // control reaches us.
                                                // =====================================================


                                                // Re-evaluate EAX so the original JE behavior is
                                                // reproduced explicitly inside the hook.
                                                test eax, eax
                                                je NullRuntime


                                                // -----------------------------------------------------
                                                // Preserve the entire game register/flag state while
                                                // the C++ observer examines the completed runtime.
                                                // -----------------------------------------------------

                                                pushfd
                                                pushad


                                                // EAX = completed Runtime*.
                                                push eax

                                                call ObserveCompletedRuntime

                                                // __cdecl caller cleanup.
                                                add esp, 4


                                                popad
                                                popfd


                                                // -----------------------------------------------------
                                                // Replay original instruction @ 004F1523:
                                                //
                                                // test byte ptr [esi+14],1
                                                //
                                                // This establishes the exact flags expected by the
                                                // untouched JNE at 004F1527.
                                                // -----------------------------------------------------

                                                test byte ptr[esi + 14h], 1


                                                // Resume at original:
                                                //
                                                // 004F1527 jne 004F154B
                                                jmp dword ptr[g_MuMainGenerationContinue]


                                                NullRuntime:

                                                // Original 004F1521 JE target.
                                                jmp dword ptr[g_MuMainGenerationNullPath]
                                            }
                                        }


                                        bool InstallGenerationHook()
                                        {
                                            // ---------------------------------------------------------
                                            // Fail closed on ALL fixed-code sites before patching any.
                                            // ---------------------------------------------------------

                                            // 004F7C00  83 EC 38           sub esp,38
                                            // 004F7C03  55                 push ebp
                                            // 004F7C04  8B 6C 24 40        mov ebp,[esp+40]
                                            //
                                            // This is the debugger-proven candidate-only synthetic Enter seam.
                                            static const std::uint8_t expectedFrontendDispatchEntry[8] =
                                            {
                                                0x83, 0xEC, 0x38,
                                                0x55,
                                                0x8B, 0x6C, 0x24, 0x40
                                            };

                                            // 004F5ED0  57                 push edi
                                            // 004F5ED1  68 04 61 74 00     push 00746104
                                            static const std::uint8_t expectedMuMainNativeSelection[6] =
                                            {
                                                0x57,
                                                0x68, 0x04, 0x61, 0x74, 0x00
                                            };

                                            // 004F5EF2  8B 86 88 00 00 00  mov eax,[esi+88]
                                            //
                                            // Debugger-proven fallback boundary for Race -> Quit Game
                                            // MU_Main reconstruction. The primary 004F1521 observer is
                                            // bypassed on that route, but the seven-entry safety vector is
                                            // already complete here.
                                            static const std::uint8_t expectedMuMainSelectionPreScan[6] =
                                            {
                                                0x8B, 0x86, 0x88, 0x00, 0x00, 0x00
                                            };

                                            // 004F1521  74 22              je 004F1545
                                            // 004F1523  F6 46 14 01        test byte ptr [esi+14],1
                                            static const std::uint8_t expectedGeneration[6] =
                                            {
                                                0x74, 0x22,
                                                0xF6, 0x46, 0x14, 0x01
                                            };

                                            // 004EFE87  89 B3 80 00 00 00  mov [ebx+80],esi
                                            static const std::uint8_t expectedCurrentRowCommit[6] =
                                            {
                                                0x89, 0xB3, 0x80, 0x00, 0x00, 0x00
                                            };

                                            // Upstream candidate writers used only by the one-shot
                                            // ReplayMovies post-submenu Main Menu restore.
                                            //
                                            // 004EFCB5  89 44 24 1C       mov [esp+1C],eax
                                            // 004EFCB9  8B 44 24 20       mov eax,[esp+20]
                                            static const std::uint8_t expectedRestoreCandidatePrimary[8] =
                                            {
                                                0x89, 0x44, 0x24, 0x1C,
                                                0x8B, 0x44, 0x24, 0x20
                                            };

                                            // 004EFDE2  89 44 24 1C       mov [esp+1C],eax
                                            // 004EFDE6  E9 E0 FE FF FF    jmp 004EFCCB
                                            static const std::uint8_t expectedRestoreCandidateLoop[9] =
                                            {
                                                0x89, 0x44, 0x24, 0x1C,
                                                0xE9, 0xE0, 0xFE, 0xFF, 0xFF
                                            };

                                            // 004F1116  89 70 54           mov [eax+54],esi
                                            // 004F1119  E8 12 DF 00 00     call 004FF030
                                            //
                                            // Verify the full two-instruction sequence even though the
                                            // six-byte push/ret detour overwrites only its first six bytes.
                                            // The hook replays both and resumes at 004F111E.
                                            static const std::uint8_t expectedActiveNodeWriter[8] =
                                            {
                                                0x89, 0x70, 0x54,
                                                0xE8, 0x12, 0xDF, 0x00, 0x00
                                            };

                                            // 004FC374  89 43 04  mov [ebx+04],eax
                                            // 004FC377  89 4B 08  mov [ebx+08],ecx
                                            static const std::uint8_t expectedLiveGeometryWriter[6] =
                                            {
                                                0x89, 0x43, 0x04,
                                                0x89, 0x4B, 0x08
                                            };

                                            // 004B33FF  0F 85 64 03 00 00  jne 004B3769
                                            static const std::uint8_t expectedMuMainDispatch[6] =
                                            {
                                                0x0F, 0x85, 0x64, 0x03, 0x00, 0x00
                                            };

                                            // 004F5860  8B 44 24 04  mov eax,[esp+04]
                                            // 004F5864  8B 48 18     mov ecx,[eax+18]
                                            static const std::uint8_t expectedTrailerMovieVideoCallback[7] =
                                            {
                                                0x8B, 0x44, 0x24, 0x04,
                                                0x8B, 0x48, 0x18
                                            };

                                            // 004B3758  8B 7D 40           mov edi,[ebp+40]
                                            // 004B375B  53                 push ebx
                                            // 004B375C  68 28 BF 6C 00     push 006CBF28 ; "FadeIn"
                                            static const std::uint8_t expectedMuMainDescriptionFade[9] =
                                            {
                                                0x8B, 0x7D, 0x40,
                                                0x53,
                                                0x68, 0x28, 0xBF, 0x6C, 0x00
                                            };

                                            // 004CF338  50                 push eax
                                            // 004CF339  BF E0 25 6C 00     mov edi,006C25E0
                                            static const std::uint8_t expectedMuOptionsBackRoute[6] =
                                            {
                                                0x50,
                                                0xBF, 0xE0, 0x25, 0x6C, 0x00
                                            };

                                            const auto* currentFrontendDispatchEntry =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    kFrontendDispatchEntryHook);

                                            const auto* currentMuMainNativeSelection =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainNativeSelectionHook);

                                            const auto* currentMuMainSelectionPreScan =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainSelectionPreScanHook);

                                            const auto* currentGeneration =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainGenerationHook);

                                            const auto* currentRowCommit =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainCurrentRowCommitHook);

                                            const auto* currentRestoreCandidatePrimary =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainRestoreCandidatePrimaryHook);

                                            const auto* currentRestoreCandidateLoop =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainRestoreCandidateLoopHook);

                                            const auto* currentActiveNodeWriter =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainActiveNodeWriterHook);

                                            const auto* currentLiveGeometryWriter =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::LiveGeometryWriterHook);

                                            const auto* currentMuMainDispatch =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainDispatchHook);

                                            const auto* currentTrailerMovieVideoCallback =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::TrailerMovieVideoCallbackHook);

                                            const auto* currentMuMainDescriptionFade =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuMainDescriptionFadeHook);

                                            const auto* currentMuOptionsBackRoute =
                                                reinterpret_cast<const std::uint8_t*>(
                                                    Addr::MuOptionsBackRouteHook);

                                            if (std::memcmp(
                                                currentFrontendDispatchEntry,
                                                expectedFrontendDispatchEntry,
                                                sizeof(expectedFrontendDispatchEntry)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentMuMainNativeSelection,
                                                expectedMuMainNativeSelection,
                                                sizeof(expectedMuMainNativeSelection)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentMuMainSelectionPreScan,
                                                expectedMuMainSelectionPreScan,
                                                sizeof(expectedMuMainSelectionPreScan)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentGeneration,
                                                expectedGeneration,
                                                sizeof(expectedGeneration)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentRowCommit,
                                                expectedCurrentRowCommit,
                                                sizeof(expectedCurrentRowCommit)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentRestoreCandidatePrimary,
                                                expectedRestoreCandidatePrimary,
                                                sizeof(expectedRestoreCandidatePrimary)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentRestoreCandidateLoop,
                                                expectedRestoreCandidateLoop,
                                                sizeof(expectedRestoreCandidateLoop)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentActiveNodeWriter,
                                                expectedActiveNodeWriter,
                                                sizeof(expectedActiveNodeWriter)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentLiveGeometryWriter,
                                                expectedLiveGeometryWriter,
                                                sizeof(expectedLiveGeometryWriter)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentMuMainDispatch,
                                                expectedMuMainDispatch,
                                                sizeof(expectedMuMainDispatch)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentTrailerMovieVideoCallback,
                                                expectedTrailerMovieVideoCallback,
                                                sizeof(expectedTrailerMovieVideoCallback)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentMuMainDescriptionFade,
                                                expectedMuMainDescriptionFade,
                                                sizeof(expectedMuMainDescriptionFade)) != 0)
                                            {
                                                return false;
                                            }

                                            if (std::memcmp(
                                                currentMuOptionsBackRoute,
                                                expectedMuOptionsBackRoute,
                                                sizeof(expectedMuOptionsBackRoute)) != 0)
                                            {
                                                return false;
                                            }

                                            // Capture the native authored-parent restore context before any
                                            // ReplayMovies generation logic. The detour is inert unless Back from
                                            // the ReplayMovies submenu armed the one-shot and ECX is OptionsHash.
                                            if (!WriteAbsoluteJump6(
                                                Addr::MuMainNativeSelectionHook,
                                                reinterpret_cast<const void*>(
                                                    &MuMainNativeSelectionByHashHook)))
                                            {
                                                return false;
                                            }

                                            // Late completed-generation fallback for native MU_Main routes that
                                            // bypass 004F1521 entirely. The hook is fail-closed on the exact
                                            // seven-entry UndergroundMode safety-duplicate state and otherwise
                                            // replays the native instruction without side effects.
                                            if (!WriteAbsoluteJump6(
                                                Addr::MuMainSelectionPreScanHook,
                                                reinterpret_cast<const void*>(
                                                    &MuMainSelectionPreScanHook)))
                                            {
                                                return false;
                                            }

                                            // Install writer hooks before the row-commit hook that can arm either
                                            // event. Both are inert until 004EFE87 qualifies an exact transition.
                                            if (!WriteAbsoluteJump6(
                                                Addr::LiveGeometryWriterHook,
                                                reinterpret_cast<const void*>(
                                                    &LiveGeometryWriterHook)))
                                            {
                                                return false;
                                            }

                                            // Install the +54 writer hook. It is completely inert unless
                                            // the exact ReplayMovies -> UndergroundMode row transition arms an
                                            // event-scoped correction in the current-row hook.
                                            if (!WriteAbsoluteJump6(
                                                Addr::MuMainActiveNodeWriterHook,
                                                reinterpret_cast<const void*>(
                                                    &MuMainActiveNodeWriterHook)))
                                            {
                                                return false;
                                            }

                                            // 004EFCB5 MUST REMAIN NATIVE. Runtime A/B proved the former global
                                            // detour at this generic 004FA140 candidate writer was sufficient to
                                            // trigger the fatal frontend crash family (Profile Select, Customize,
                                            // Controls, Career/Settings). Keep its native bytes permanently.
                                            //
                                            // Retain only the alternate 004EFDE2 candidate hook for the established
                                            // ReplayMovies-specific restore path while the direct 004F5ED0 native
                                            // restoration remains in service.
                                            if (!WriteAbsoluteJumpPadded(
                                                Addr::MuMainRestoreCandidateLoopHook,
                                                reinterpret_cast<const void*>(
                                                    &MuMainRestoreCandidateLoopHook),
                                                sizeof(expectedRestoreCandidateLoop)))
                                            {
                                                return false;
                                            }

                                            // The current-row hook is likewise inert when ReplayMovies is absent.
                                            if (!WriteAbsoluteJump6(
                                                Addr::MuMainCurrentRowCommitHook,
                                                reinterpret_cast<const void*>(
                                                    &MuMainCurrentRowCommitHook)))
                                            {
                                                return false;
                                            }

                                            if (!WriteAbsoluteJump6(
                                                Addr::MuMainGenerationHook,
                                                reinterpret_cast<const void*>(
                                                    &MuMainGenerationHook)))
                                            {
                                                return false;
                                            }

                                            // Install the TrailerMovie basename override before enabling the Enter
                                            // dispatcher. It is globally inert unless ReplayMovies submenu mode is
                                            // active and the exact type-7 Video/NASCAR node is encountered.
                                            if (!WriteAbsoluteJumpPadded(
                                                Addr::TrailerMovieVideoCallbackHook,
                                                reinterpret_cast<const void*>(
                                                    &ReplayMoviesTrailerMovieVideoHook),
                                                sizeof(expectedTrailerMovieVideoCallback)))
                                            {
                                                return false;
                                            }

                                            // Install the Back-route specialization before enabling the Enter
                                            // dispatcher. It is completely inert while ReplayMovies submenu mode
                                            // is inactive and replays the native Options-selector destination.
                                            if (!WriteAbsoluteJump6(
                                                Addr::MuOptionsBackRouteHook,
                                                reinterpret_cast<const void*>(
                                                    &ReplayMoviesOptionsBackRouteHook)))
                                            {
                                                return false;
                                            }

                                            if (!WriteAbsoluteJumpPadded(
                                                Addr::MuMainDescriptionFadeHook,
                                                reinterpret_cast<const void*>(
                                                    &ReplayMoviesDescriptionFadeHook),
                                                sizeof(expectedMuMainDescriptionFade)))
                                            {
                                                return false;
                                            }

                                            // v10.6 full-catalog activation: best-effort vtable proxy. This is deliberately
                                            // NOT part of the mandatory hook preflight: if the static MU_Options_PC
                                            // notification slot differs, ReplayMovies must still appear and retain
                                            // all previously validated Main Menu/catalog behavior. If this proxy cannot
                                            // be installed, the previously validated frontend still remains intact.
                                            InstallReplayMoviesFullCatalogNotificationProxy();

                                            // v11.3 debugger-proven synthetic Enter routing.  Synthetic rows
                                            // already publish the real 0x0C407210 transaction; alter only the
                                            // 004F7C00 candidate argument to authored OptionName_1. Runtime+80 is
                                            // deliberately untouched for the entire callback/movie identity path.
                                            if (!WriteAbsoluteJumpPadded(
                                                kFrontendDispatchEntryHook,
                                                reinterpret_cast<const void*>(
                                                    &ReplayMoviesFrontendDispatchEntryHook),
                                                sizeof(expectedFrontendDispatchEntry)))
                                            {
                                                return false;
                                            }

                                            // Install the action dispatcher last. If its byte signature ever
                                            // differs, the existing v6 frontend construction/visual hooks remain
                                            // the only active behavior and ReplayMovies simply keeps its native
                                            // unknown-hash fallback instead of risking a wrong package transition.
                                            return WriteAbsoluteJump6(
                                                Addr::MuMainDispatchHook,
                                                reinterpret_cast<const void*>(
                                                    &ReplayMoviesDispatchHook));
                                        }