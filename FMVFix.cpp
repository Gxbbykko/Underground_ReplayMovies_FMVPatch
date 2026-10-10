#include "FMVFix.h"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <limits>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

static_assert(sizeof(void*) == 4, "FMVFix must be built as Win32/x86.");

// ============================================================================
// Frozen XboxPort -> PCPort bridge state
// ============================================================================

extern "C"
{
    void FMVFix_HookMovieQuad();
    void FMVFix_HookIntroFrameCommit();
    void FMVFix_HookPostUploadRuntime();

    DWORD g_FMVFix_Orig651A00 = 0;
    DWORD g_FMVFix_Orig654E20 = 0;
    DWORD g_FMVFix_Orig657690 = 0;
    DWORD g_FMVFix_ReturnStartMovieCapability = 0;
    DWORD g_FMVFix_ReturnMovieQuad = 0;
    DWORD g_FMVFix_ReturnIntroFrameCommit = 0;
    DWORD g_FMVFix_GpuMovieTextureSlot = 0;
    DWORD g_FMVFix_OrigMovieUpload = 0;
    volatile LONG g_FMVFix_MovieGeneration = 0;
}

namespace
{
    constexpr std::uintptr_t kPreferredImageBase = 0x00400000;

    // Stable StartMovie publication boundary.
    constexpr std::uintptr_t kStartMovieHookSite = 0x005A4ABD;
    constexpr std::uintptr_t kStartMovieContinue = 0x005A4AC5;

    // NA_BOOT-only presentation hook. This site is deliberately NOT patched
    // during FMVFix initialization. It is detoured lazily from StartMovie only
    // after ThirteenAG has already installed its Cropped geometry.
    constexpr std::uintptr_t kMovieQuadHookSite = 0x004F1FBE;
    constexpr std::uintptr_t kMovieQuadContinue = 0x004F1FC3;
    constexpr std::uintptr_t kMovieQuadBottomImmediate = 0x004F1FC4;
    constexpr std::uintptr_t kMovieQuadRightImmediate = 0x004F1FC9;
    constexpr std::uintptr_t kMovieQuadTopImmediate = 0x004F1FCE;
    constexpr std::uintptr_t kMovieQuadLeftImmediate = 0x004F1FD3;

    constexpr std::uintptr_t kActiveMovieObject = 0x00733F6C;

    // Proven decoded-frame / presentation boundaries. Intro-class movies use
    // 004132F6. Career-class movies use the proven post-00413270 wrapper at
    // 005A48FC. Final substitution remains the qualified 0040C2D0 movie draw.
    constexpr std::uintptr_t kIntroFrameCommitHookSite = 0x004132F6;
    constexpr std::uintptr_t kIntroFrameCommitContinue = 0x004132FB;
    constexpr std::uintptr_t kPostUploadCallSite = 0x005A48FC;
    constexpr std::uintptr_t kNativeMovieUpload = 0x00413270;
    constexpr std::uintptr_t kGpuMovieTextureGlobal = 0x0078A494;
    constexpr std::uintptr_t kD3DDeviceGlobal = 0x0073636C;
    constexpr std::uintptr_t kDrawPrimitiveUPCallSite = 0x0040C2D0;

    constexpr std::uint32_t kRuntimeMaxOutputDimension = 4096;
    constexpr std::uint32_t kRuntimeMaxOutputFrameBytes = 64u * 1024u * 1024u;
    constexpr std::uint32_t kRuntimeMaxRingSlots = 16;
    constexpr std::uint32_t kRuntimeMinRingSlots = 2;
    constexpr std::uint64_t kRuntimeRingBudgetBytes = 128ull * 1024ull * 1024ull;

    // Stage 2.5 startup runway: one next-movie cache is prepared ahead of the
    // controlled validation sequence. Six RGB32 frames provide about 200 ms at
    // 30 FPS, enough to hide the HEVC/Media Foundation startup cost observed in
    // Stage 2.2 without decoding on StartMovie or the render thread.
    constexpr std::uint32_t kMp4StartupPrewarmFrames = 6;

    // Production release: diagnostic counters remain available internally,
    // but no startup manifests, per-generation or canonical TXT reports are
    // created. Legacy report writers stay compiled for a diagnostic rollback.
    constexpr bool kRuntimeReportsEnabled = false;

    // Keep the v4 rule that fixed the 4K startup freeze/audio desync: large
    // outputs receive no synchronous frame preload. The background reader starts
    // at frame 0 and the draw path fails open to the exact native MAD frame if
    // a requested replacement frame has not arrived yet.
    constexpr std::uint32_t kRuntimeSyncPreloadMaxFrameBytes = 8u * 1024u * 1024u;

    constexpr std::size_t kDeviceCreateTextureIndex = 23;
    constexpr std::size_t kDeviceGetTextureIndex = 64;
    constexpr std::size_t kDeviceSetTextureIndex = 65;
    constexpr std::size_t kDeviceDrawPrimitiveUPIndex = 83;
    constexpr std::size_t kTextureLockRectIndex = 19;
    constexpr std::size_t kTextureUnlockRectIndex = 20;
    constexpr std::size_t kIUnknownReleaseIndex = 2;

    constexpr DWORD kD3DFormatA8R8G8B8 = 21;
    constexpr DWORD kD3DPoolManaged = 1;
    // Proven manual NA_BOOT presentation boundaries.
    constexpr std::uint32_t kNaBootEAEnd = 106;
    constexpr std::uint32_t kNaBootTHXStart = 181;
    constexpr std::uint32_t kNaBootTHXEnd = 346;

    // ThirteenAG FMVWidescreenMode=1 (Cropped).
    constexpr std::uint32_t kCroppedBottom = 0x3F2AAAAB;
    constexpr std::uint32_t kCroppedRight = 0x3F2AAAAB;
    constexpr std::uint32_t kCroppedTop = 0xBF2AAAAB;
    constexpr std::uint32_t kCroppedLeft = 0xBF2AAAAB;

    // Desired EA / THX presentation: keep ThirteenAG horizontal Cropped values,
    // change only the vertical pair to +/-0.5.
    constexpr std::uint32_t kStretchedBottom = 0x3F000000;
    constexpr std::uint32_t kStretchedTop = 0xBF000000;

    // Native PC capability table.
    // COUNT is BYTE; candidates are WORDs.
    constexpr std::uintptr_t kCapabilityCount = 0x00713DBC;
    constexpr std::uintptr_t kCapabilityC0 = 0x00713DBE;
    constexpr std::uintptr_t kCapabilityC1 = 0x00713DC0;

    // Proven downstream call sites.
    constexpr std::uintptr_t kHook651A00Call = 0x00655209;
    constexpr std::uintptr_t kHook654E20Call = 0x006514AD;
    constexpr std::uintptr_t kHook657690Call = 0x006518B9;

    // Proven class-gate immediates.
    constexpr std::uintptr_t kClassGateImmediate = 0x006517AB;
    constexpr std::uintptr_t kMatrixGateImmediate = 0x0065131E;
    constexpr std::uintptr_t kRuntimeGateImmediate = 0x0065141D;

    std::uintptr_t g_GameBase = 0;

    // NA_BOOT framing is strictly session-local. The render detour is installed
    // only for NA_BOOT and physically removed again at the proven THX-end
    // black frame (or at the next StartMovie if playback is skipped early).
    volatile LONG g_NaBootFramingActive = 0;
    LONG g_NaBootGeometryMode = 0; // 0 = Cropped baseline, 1 = special Stretched
    bool g_NaBootRendererHookInstalled = false;
    bool g_NaBootBaselineValid = false;
    std::uint32_t g_NaBootBaselineBottom = 0;
    std::uint32_t g_NaBootBaselineTop = 0;

#pragma pack(push, 1)
    struct NfsuSrPackHeader
    {
        char magic[8];
        std::uint32_t version;
        std::uint32_t sourceWidth;
        std::uint32_t sourceHeight;
        std::uint32_t outputWidth;
        std::uint32_t outputHeight;
        std::uint32_t framesTotal;
        std::uint32_t framesComplete;
        std::uint32_t frameBytes;
        std::uint32_t dataOffset;
        std::uint32_t d3dFormat;
        std::uint32_t presetCode;
        float aiStrength;
        BYTE reserved[8];
    };
#pragma pack(pop)

    static_assert(sizeof(NfsuSrPackHeader) == 64, "NFSUSR2A header must be 64 bytes.");

    struct D3DLockedRectCompat
    {
        INT Pitch;
        void* pBits;
    };

    using DeviceCreateTextureFn = HRESULT(__stdcall*)(void*, UINT, UINT, UINT, DWORD, DWORD, DWORD, void**, HANDLE*);
    using DeviceGetTextureFn = HRESULT(__stdcall*)(void*, DWORD, void**);
    using DeviceSetTextureFn = HRESULT(__stdcall*)(void*, DWORD, void*);
    using DeviceDrawPrimitiveUPFn = HRESULT(__stdcall*)(void*, DWORD, UINT, const void*, UINT);
    using TextureLockRectFn = HRESULT(__stdcall*)(void*, UINT, D3DLockedRectCompat*, const RECT*, DWORD);
    using TextureUnlockRectFn = HRESULT(__stdcall*)(void*, UINT);
    using UnknownReleaseFn = ULONG(__stdcall*)(void*);

    enum class RuntimeClockClass : LONG
    {
        None = 0,
        IntroCommit = 1,
        CareerPostUpload = 2,
    };

    enum class RuntimeMode : LONG
    {
        Xbox = 0,
        P1080 = 1,
        P4K = 2,
    };

    struct MovieSpec
    {
        const char* stem;
        std::uint32_t sourceWidth;
        std::uint32_t sourceHeight;
        std::uint32_t frames;
        RuntimeClockClass clock;
    };

    // Counts are the canonical contracts established by the audited capture run.
    constexpr MovieSpec kMovieCatalog[] =
    {
        { "NA_BOOT",  512, 256,  390, RuntimeClockClass::IntroCommit },
        { "PSA",      512, 256,  544, RuntimeClockClass::IntroCommit },
        { "E3_TITLE", 512, 512, 2426, RuntimeClockClass::IntroCommit },
        { "01_DAY",   512, 256,  536, RuntimeClockClass::CareerPostUpload },
        { "02_FAN_S", 512, 256,  425, RuntimeClockClass::CareerPostUpload },
        { "02_FAN_E", 512, 256,  888, RuntimeClockClass::CareerPostUpload },
        { "04_GUM_S", 512, 256,  318, RuntimeClockClass::CareerPostUpload },
        { "05_ENG",   512, 256,  546, RuntimeClockClass::CareerPostUpload },
        { "06_SAM",   512, 256,  653, RuntimeClockClass::CareerPostUpload },
        { "07_GOD",   512, 256,  365, RuntimeClockClass::CareerPostUpload },
        { "08_EDD",   512, 256,  234, RuntimeClockClass::CareerPostUpload },
        { "09_GBE",   512, 256,  334, RuntimeClockClass::CareerPostUpload },
        { "11_PNK",   512, 256,  446, RuntimeClockClass::CareerPostUpload },
        { "12_BUD",   512, 256,  310, RuntimeClockClass::CareerPostUpload },
        { "13_KIT",   512, 256,  270, RuntimeClockClass::CareerPostUpload },
        { "14_MUT",   512, 256,  183, RuntimeClockClass::CareerPostUpload },
        { "15_DIK",   512, 256,  151, RuntimeClockClass::CareerPostUpload },
        { "16_RES",   512, 256,  167, RuntimeClockClass::CareerPostUpload },
        { "17_MET",   512, 256,  214, RuntimeClockClass::CareerPostUpload },
        { "19_FIN",   512, 256,  531, RuntimeClockClass::CareerPostUpload },
    };

    // Coded HEVC frames may contain padding that is excluded by the display
    // aperture (for example 2176x1088 coded, 2160x1080 visible). Only the
    // validated visible picture enters the existing native-MAD-sized texture.
    struct Mp4FrameGeometry
    {
        UINT32 codedWidth = 0;
        UINT32 codedHeight = 0;
        UINT32 cropX = 0;
        UINT32 cropY = 0;
        UINT32 visibleWidth = 0;
        UINT32 visibleHeight = 0;
        UINT32 apertureKind = 0; // 0 exact, 1 minimum-display, 2 geometric
    };

    struct Mp4CopyLayout
    {
        UINT32 sourceWidth = 0;
        UINT32 sourceHeight = 0;
        UINT32 cropX = 0;
        UINT32 cropY = 0;
        LONG stride = 0;
    };

    // The native media type is authoritative for the displayed rectangle.
    // Never infer visible pixels by silently cropping an arbitrary coded size.
    HRESULT ResolveMp4NativeGeometry(
        IMFMediaType* nativeType, const UINT32 expectedWidth,
        const UINT32 expectedHeight, Mp4FrameGeometry* geometry)
    {
        if (nativeType == nullptr || geometry == nullptr ||
            expectedWidth == 0 || expectedHeight == 0)
            return E_INVALIDARG;

        *geometry = {};
        HRESULT hr = MFGetAttributeSize(nativeType, MF_MT_FRAME_SIZE,
            &geometry->codedWidth, &geometry->codedHeight);
        if (FAILED(hr))
            return hr;

        geometry->visibleWidth = expectedWidth;
        geometry->visibleHeight = expectedHeight;
        if (geometry->codedWidth == expectedWidth &&
            geometry->codedHeight == expectedHeight)
            return S_OK;

        if (geometry->codedWidth < expectedWidth ||
            geometry->codedHeight < expectedHeight)
            return MF_E_INVALIDMEDIATYPE;

        const GUID keys[] = {
            MF_MT_MINIMUM_DISPLAY_APERTURE, MF_MT_GEOMETRIC_APERTURE
        };
        for (UINT32 i = 0; i < 2; ++i)
        {
            MFVideoArea area = {};
            UINT32 size = 0;
            if (FAILED(nativeType->GetBlob(keys[i],
                reinterpret_cast<UINT8*>(&area), sizeof(area), &size)) ||
                size != sizeof(area))
                continue;

            if (area.OffsetX.fract != 0 || area.OffsetY.fract != 0 ||
                area.OffsetX.value < 0 || area.OffsetY.value < 0 ||
                area.Area.cx != static_cast<LONG>(expectedWidth) ||
                area.Area.cy != static_cast<LONG>(expectedHeight))
                continue;

            const UINT32 x = static_cast<UINT32>(area.OffsetX.value);
            const UINT32 y = static_cast<UINT32>(area.OffsetY.value);
            if (x > geometry->codedWidth - expectedWidth ||
                y > geometry->codedHeight - expectedHeight)
                continue;

            geometry->cropX = x;
            geometry->cropY = y;
            geometry->apertureKind = i + 1;
            return S_OK;
        }
        return MF_E_INVALIDMEDIATYPE;
    }

    HRESULT ResolveMp4RgbCopyLayout(
        IMFSourceReader* reader, const Mp4FrameGeometry& geometry,
        Mp4CopyLayout* layout, UINT32* subtypeData1Out)
    {
        if (reader == nullptr || layout == nullptr || subtypeData1Out == nullptr)
            return E_POINTER;
        IMFMediaType* currentType = nullptr;
        HRESULT hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType);
        if (FAILED(hr) || currentType == nullptr)
            return FAILED(hr) ? hr : E_FAIL;

        GUID majorType = {};
        GUID subtype = {};
        UINT32 width = 0;
        UINT32 height = 0;
        if (SUCCEEDED(hr)) hr = currentType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
        if (SUCCEEDED(hr)) hr = currentType->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (SUCCEEDED(hr)) hr = MFGetAttributeSize(currentType, MF_MT_FRAME_SIZE, &width, &height);
        if (SUCCEEDED(hr))
        {
            *subtypeData1Out = subtype.Data1;
            Mp4CopyLayout candidate = {};
            candidate.sourceWidth = width;
            candidate.sourceHeight = height;
            if (width == geometry.codedWidth && height == geometry.codedHeight)
            {
                candidate.cropX = geometry.cropX;
                candidate.cropY = geometry.cropY;
            }
            else if (width == geometry.visibleWidth && height == geometry.visibleHeight)
            {
                // Some MF transforms already apply the aperture internally.
                candidate.cropX = 0;
                candidate.cropY = 0;
            }
            else
            {
                hr = MF_E_INVALIDMEDIATYPE;
            }

            UINT32 strideBits = 0;
            if (SUCCEEDED(hr))
            {
                candidate.stride = static_cast<LONG>(width * 4u);
                if (SUCCEEDED(currentType->GetUINT32(MF_MT_DEFAULT_STRIDE, &strideBits)))
                    candidate.stride = static_cast<LONG>(strideBits);
                const std::int64_t absStride = candidate.stride < 0
                    ? -static_cast<std::int64_t>(candidate.stride)
                    : static_cast<std::int64_t>(candidate.stride);
                if (!IsEqualGUID(majorType, MFMediaType_Video) ||
                    !IsEqualGUID(subtype, MFVideoFormat_RGB32) ||
                    absStride < static_cast<std::int64_t>(width) * 4 ||
                    candidate.cropX > width - geometry.visibleWidth ||
                    candidate.cropY > height - geometry.visibleHeight)
                {
                    hr = MF_E_INVALIDMEDIATYPE;
                }
                else
                {
                    *layout = candidate;
                }
            }
        }
        currentType->Release();
        return hr;
    }

    RuntimeMode g_RuntimeMode = RuntimeMode::Xbox;
    RuntimeClockClass g_RuntimeClock = RuntimeClockClass::None;
    const MovieSpec* g_RuntimeMovie = nullptr;

    // All shared LONG state is accessed directly through Win32 Interlocked calls
    // at the actual call sites. This keeps MSVC /analyze C28112 satisfied.
    volatile LONG g_RuntimeActive = 0;
    volatile LONG g_RuntimeCurrentSerial = -1;
    volatile LONG g_RuntimeFrameClockHits = 0;
    volatile LONG g_RuntimeExcessFrameEvents = 0;
    volatile LONG g_RuntimeFramesRead = 0;
    volatile LONG g_RuntimeSidecarReadFailures = 0;
    volatile LONG g_RuntimeMatchingDraws = 0;
    volatile LONG g_RuntimeExactFrameHits = 0;
    volatile LONG g_RuntimeFallbackDraws = 0;
    volatile LONG g_RuntimeDistinctMisses = 0;
    volatile LONG g_RuntimeTextureUploadFailures = 0;
    volatile LONG g_RuntimeSetTextureFailures = 0;
    volatile LONG g_RuntimeRestoreTextureFailures = 0;
    volatile LONG g_RuntimeVertexAdjustedDraws = 0;
    volatile LONG g_RuntimeLastHitSerial = -1;
    volatile LONG g_RuntimeLastMissSerial = -1;
    volatile LONG g_RuntimeUploadedSerial = -1;
    volatile LONG g_RuntimeNextReadSerial = 0;

    HANDLE g_RuntimeSrFile = INVALID_HANDLE_VALUE;
    HANDLE g_RuntimeReaderThread = nullptr;
    HANDLE g_RuntimeReaderStopEvent = nullptr;
    BYTE* g_RuntimeRing = nullptr;
    volatile LONG g_RuntimeRingState[kRuntimeMaxRingSlots] = {}; // 0 empty, 1 ready
    LONG g_RuntimeRingSerial[kRuntimeMaxRingSlots] = {};
    std::uint32_t g_RuntimeOutputWidth = 0;
    std::uint32_t g_RuntimeOutputHeight = 0;
    std::uint32_t g_RuntimeOutputFrameBytes = 0;
    std::uint32_t g_RuntimeRingSlots = 0;
    std::uint32_t g_RuntimeInitialSyncFrames = 0;
    void* g_RuntimeReplacementTexture = nullptr;
    char g_RuntimeSrPath[MAX_PATH] = {};
    char g_RuntimeLogPath[MAX_PATH] = {};          // generation-specific report
    char g_RuntimeCanonicalLogPath[MAX_PATH] = {}; // last complete/proven session only
    LONG g_RuntimeGeneration = 0;
    char g_RuntimeIniPath[MAX_PATH] = {};
    char g_RuntimeModuleDir[MAX_PATH] = {};
    bool g_RuntimeHeaderValid = false;
    bool g_RuntimeCreateTexturePass = false;
    bool g_RuntimeIntroHookInstalled = false;
    bool g_RuntimeCareerHookInstalled = false;
    bool g_RuntimeDrawHookInstalled = false;

    // MP4 Stage 2 decoder state. The native MAD remains the authoritative
    // audio/timing/lifecycle clock. Media Foundation only produces replacement
    // video frames on a background thread.
    char g_RuntimeMp4Path[MAX_PATH] = {};
    volatile LONG g_Mp4DecoderHr = S_OK;
    volatile LONG g_Mp4DecoderEos = 0;
    volatile LONG g_Mp4DecodedDiscarded = 0;
    volatile LONG g_Mp4FirstReadySerial = -1;
    volatile LONG g_Mp4LastReadySerial = -1;
    UINT32 g_Mp4NativeWidth = 0;
    UINT32 g_Mp4NativeHeight = 0;
    Mp4FrameGeometry g_Mp4NativeGeometry = {};
    Mp4CopyLayout g_Mp4CopyLayout = {};
    UINT32 g_Mp4FrameRateNumerator = 0;
    UINT32 g_Mp4FrameRateDenominator = 0;
    ULONGLONG g_Mp4PresentationDuration100ns = 0;
    LONG g_Mp4OutputStride = 0;
    volatile LONG g_Mp4MediaTypeChanges = 0;
    volatile LONG g_Mp4MediaTypeRenegotiations = 0;
    UINT32 g_Mp4CurrentSubtypeData1 = 0;


    // Stage 2.5 single next-movie prewarm cache. When complete, ownership of
    // the buffer is transferred to the normal runtime ring at StartMovie. No
    // D3D object is touched by this worker.
    HANDLE g_Mp4PrewarmThread = nullptr;
    BYTE* g_Mp4PrewarmRing = nullptr;
    volatile LONG g_Mp4PrewarmState = 0; // 0 idle, 1 running, 2 ready, 3 failed, 4 adopted
    volatile LONG g_Mp4PrewarmReadyFrames = 0;
    volatile LONG g_Mp4PrewarmHr = S_OK;
    RuntimeMode g_Mp4PrewarmMode = RuntimeMode::Xbox;
    std::uint32_t g_Mp4PrewarmWidth = 0;
    std::uint32_t g_Mp4PrewarmHeight = 0;
    std::uint32_t g_Mp4PrewarmFrameBytes = 0;
    std::uint32_t g_Mp4PrewarmRingSlots = 0;
    char g_Mp4PrewarmPath[MAX_PATH] = {};

    volatile LONG g_Mp4StartupPrewarmUsed = 0;
    volatile LONG g_Mp4StartupPrewarmFrames = 0;
    volatile LONG g_Mp4PrewarmStateAtStart = 0;
    volatile LONG g_Mp4PrewarmHrAtStart = S_OK;
    volatile LONG g_Mp4DecoderSkipPrefix = 0;

    // MP4 framework Stage 2.5: full-catalog async substitution + catalog-next prewarm.
    // The original MAD remains authoritative for audio/timing/skip/end/lifecycle.
    constexpr char kMp4FrameworkBuild[] =
        "FMVFix MP4 Production Full20 QuietReports ErrorPopups 2026-10-09";

    struct Mp4ProbeJob
    {
        const MovieSpec* movie;
        RuntimeMode mode;
        LONG generation;
        char mp4Path[MAX_PATH];
        char logPath[MAX_PATH];
    };

    [[maybe_unused]] constexpr char kBuildMarker[] =
        "FMVFix MP4 Production Full20 QuietReports ErrorPopups + XboxPort-PCPort + NA_BOOT Framing v3 2026-10-09";

    const char* RuntimeModeName();

    enum RuntimeErrorClass : LONG
    {
        kErrorMissingMp4 = 0x01,
        kErrorPaths = 0x02,
        kErrorAllocation = 0x04,
        kErrorDecoder = 0x08,
        kErrorGraphics = 0x10,
        kErrorInstall = 0x20
    };

    volatile LONG g_RuntimeErrorDialogClasses = 0;

    struct RuntimeErrorDialog
    {
        char message[512];
    };

    DWORD WINAPI RuntimeErrorDialogThread(LPVOID parameter)
    {
        RuntimeErrorDialog* const dialog = static_cast<RuntimeErrorDialog*>(parameter);
        if (dialog != nullptr)
        {
            MessageBoxA(nullptr, dialog->message, "FMVFix - MP4 playback error",
                MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
            HeapFree(GetProcessHeap(), 0, dialog);
        }
        return 0;
    }

    void NotifyRuntimeError(const LONG errorClass, const char* movie,
        const char* problem, const HRESULT hr)
    {
        char message[512] = {};
        const int length = std::snprintf(message, sizeof(message),
            "FMVFix encountered a playback or setup error.\r\n\r\n"
            "Mode: %s\r\nMovie: %s\r\nProblem: %s\r\n"
            "HRESULT: 0x%08lX\r\n\r\n"
            "The native MAD video remains the fallback.\r\n"
            "This warning appears at most once per error category per game launch.",
            RuntimeModeName(), movie != nullptr ? movie : "unknown",
            problem != nullptr ? problem : "unknown", static_cast<unsigned long>(hr));
        if (length <= 0 || length >= static_cast<int>(sizeof(message)))
            return;

        OutputDebugStringA(message);
        OutputDebugStringA("\r\n");

        // InterlockedOr returns the previous mask; reserve one notification per
        // error class for the entire process, preventing 20 sequential dialogs.
        if ((InterlockedOr(&g_RuntimeErrorDialogClasses, errorClass) & errorClass) != 0)
            return;

        RuntimeErrorDialog* const dialog = static_cast<RuntimeErrorDialog*>(
            HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RuntimeErrorDialog)));
        if (dialog == nullptr)
            return;
        std::memcpy(dialog->message, message, static_cast<std::size_t>(length) + 1u);
        HANDLE const thread = CreateThread(nullptr, 0, RuntimeErrorDialogThread, dialog, 0, nullptr);
        if (thread == nullptr)
        {
            HeapFree(GetProcessHeap(), 0, dialog);
            return;
        }
        CloseHandle(thread);
    }

    std::uintptr_t VA(const std::uintptr_t originalVA)
    {
        return g_GameBase + (originalVA - kPreferredImageBase);
    }

    bool MatchBytes(
        const std::uintptr_t address,
        const void* expected,
        const std::size_t size)
    {
        return std::memcmp(
            reinterpret_cast<const void*>(address),
            expected,
            size) == 0;
    }

    bool WriteBytes(
        const std::uintptr_t address,
        const void* data,
        const std::size_t size)
    {
        DWORD oldProtection = 0;

        if (!VirtualProtect(
            reinterpret_cast<void*>(address),
            size,
            PAGE_EXECUTE_READWRITE,
            &oldProtection))
        {
            return false;
        }

        std::memcpy(
            reinterpret_cast<void*>(address),
            data,
            size);

        FlushInstructionCache(
            GetCurrentProcess(),
            reinterpret_cast<void*>(address),
            size);

        DWORD ignored = 0;
        VirtualProtect(
            reinterpret_cast<void*>(address),
            size,
            oldProtection,
            &ignored);

        return true;
    }

    bool WriteByte(
        const std::uintptr_t address,
        const BYTE value)
    {
        return WriteBytes(address, &value, sizeof(value));
    }

    bool MakeRel32(
        const std::uintptr_t site,
        const std::uintptr_t target,
        std::int32_t& out)
    {
        const std::int64_t delta =
            static_cast<std::int64_t>(target) -
            static_cast<std::int64_t>(site + 5);

        if (delta < static_cast<std::int64_t>(
            (std::numeric_limits<std::int32_t>::min)()) ||
            delta > static_cast<std::int64_t>(
                (std::numeric_limits<std::int32_t>::max)()))
        {
            return false;
        }

        out = static_cast<std::int32_t>(delta);
        return true;
    }

    bool WriteRelativeCall(
        const std::uintptr_t site,
        const std::uintptr_t target)
    {
        std::int32_t rel = 0;
        if (!MakeRel32(site, target, rel))
            return false;

        BYTE patch[5] = { 0xE8, 0, 0, 0, 0 };
        std::memcpy(patch + 1, &rel, sizeof(rel));

        return WriteBytes(site, patch, sizeof(patch));
    }

    bool WriteRelativeJump5(
        const std::uintptr_t site,
        const std::uintptr_t target)
    {
        std::int32_t rel = 0;
        if (!MakeRel32(site, target, rel))
            return false;

        BYTE patch[5] = { 0xE9, 0, 0, 0, 0 };
        std::memcpy(patch + 1, &rel, sizeof(rel));

        return WriteBytes(site, patch, sizeof(patch));
    }

    bool WriteMovieVerticalGeometry(
        const std::uint32_t bottom,
        const std::uint32_t top)
    {
        // Intentionally touch only ThirteenAG's Bottom and Top immediates.
        // Right and Left remain owned by WidescreenFix.
        const std::uintptr_t bottomAddress = VA(kMovieQuadBottomImmediate);
        const std::uintptr_t topAddress = VA(kMovieQuadTopImmediate);
        const std::size_t span =
            (topAddress + sizeof(std::uint32_t)) - bottomAddress;

        DWORD oldProtection = 0;
        if (!VirtualProtect(
            reinterpret_cast<void*>(bottomAddress),
            span,
            PAGE_EXECUTE_READWRITE,
            &oldProtection))
        {
            return false;
        }

        *reinterpret_cast<volatile std::uint32_t*>(bottomAddress) = bottom;
        *reinterpret_cast<volatile std::uint32_t*>(topAddress) = top;

        FlushInstructionCache(
            GetCurrentProcess(),
            reinterpret_cast<void*>(bottomAddress),
            span);

        DWORD ignored = 0;
        VirtualProtect(
            reinterpret_cast<void*>(bottomAddress),
            span,
            oldProtection,
            &ignored);

        return true;
    }

    char AsciiLower(const char c)
    {
        if (c >= 'A' && c <= 'Z')
            return static_cast<char>(c + ('a' - 'A'));

        return c;
    }

    bool EqualsAsciiInsensitive(const char* a, const char* b)
    {
        if (a == nullptr || b == nullptr)
            return false;

        while (*a != '\0' && *b != '\0')
        {
            if (AsciiLower(*a) != AsciiLower(*b))
                return false;

            ++a;
            ++b;
        }

        return *a == '\0' && *b == '\0';
    }

    bool IsNaBootMoviePath(const char* moviePath)
    {
        if (moviePath == nullptr)
            return false;

        const char* basename = moviePath;
        for (const char* p = moviePath; *p != '\0'; ++p)
        {
            if (*p == '\\' || *p == '/')
                basename = p + 1;
        }

        return EqualsAsciiInsensitive(basename, "na_boot.mad");
    }

    bool ExtractMovieStem(const char* moviePath, char* outStem, const std::size_t outSize)
    {
        if (moviePath == nullptr || outStem == nullptr || outSize < 2)
            return false;
        const char* base = moviePath;
        for (const char* p = moviePath; *p != '\0'; ++p)
        {
            if (*p == '\\' || *p == '/')
                base = p + 1;
        }
        std::size_t length = std::strlen(base);
        if (length >= 4 && AsciiLower(base[length - 4]) == '.' &&
            AsciiLower(base[length - 3]) == 'm' &&
            AsciiLower(base[length - 2]) == 'a' &&
            AsciiLower(base[length - 1]) == 'd')
        {
            length -= 4;
        }
        if (length == 0 || length >= outSize)
            return false;
        std::memcpy(outStem, base, length);
        outStem[length] = '\0';
        return true;
    }

    const MovieSpec* FindMovieSpec(const char* moviePath)
    {
        char stem[64] = {};
        if (!ExtractMovieStem(moviePath, stem, sizeof(stem)))
            return nullptr;
        for (const auto& spec : kMovieCatalog)
        {
            if (EqualsAsciiInsensitive(stem, spec.stem))
                return &spec;
        }
        return nullptr;
    }

    const MovieSpec* FindMovieSpecByStem(const char* stem)
    {
        if (stem == nullptr || *stem == '\0')
            return nullptr;
        for (const auto& spec : kMovieCatalog)
        {
            if (EqualsAsciiInsensitive(stem, spec.stem))
                return &spec;
        }
        return nullptr;
    }

    bool IsStage25RuntimeTarget(const MovieSpec& movie)
    {
        // FindMovieSpec() only returns entries from kMovieCatalog, so every
        // catalog movie is an MP4 runtime target in Stage 2.5. Missing or
        // invalid selected-tier MP4s still fail open to the native MAD picture.
        (void)movie;
        return true;
    }

    bool IsStage25ReportTarget(const MovieSpec* movie)
    {
        if (movie == nullptr || g_RuntimeMode == RuntimeMode::Xbox)
            return false;

        // 1080p has not yet been validated as a tier, so EVERY catalog movie
        // must report there, including the five movies already proven at 4K.
        // The old Stage 2.5 suppression was mode-blind and made a failed 1080p
        // NA_BOOT/PSA/E3/01_DAY/02_FAN_S session completely silent.
        if (g_RuntimeMode == RuntimeMode::P1080)
            return true;

        // Preserve the existing 4K validation policy: the five Stage 2.4-proven
        // controls stay active but only the 15 newly enabled movies emit reports.
        return
            !EqualsAsciiInsensitive(movie->stem, "NA_BOOT") &&
            !EqualsAsciiInsensitive(movie->stem, "PSA") &&
            !EqualsAsciiInsensitive(movie->stem, "E3_TITLE") &&
            !EqualsAsciiInsensitive(movie->stem, "01_DAY") &&
            !EqualsAsciiInsensitive(movie->stem, "02_FAN_S");
    }

    const MovieSpec* Stage25NextPrewarmTarget(const MovieSpec& movie)
    {
        // Prewarm exactly one catalog-next movie at a time. This gives zero-miss
        // startup for the natural intro sequence and for sequential catalog /
        // ReplayMovies validation without retaining decoded startup frames for
        // all 20 movies in the 32-bit process. Arbitrary out-of-order selection
        // still remains safe: BeginRuntime falls back to the normal async decoder
        // and never blocks StartMovie or the render thread.
        constexpr std::size_t count = sizeof(kMovieCatalog) / sizeof(kMovieCatalog[0]);
        for (std::size_t i = 0; i < count; ++i)
        {
            if (EqualsAsciiInsensitive(movie.stem, kMovieCatalog[i].stem))
                return (i + 1u < count) ? &kMovieCatalog[i + 1u] : nullptr;
        }
        return nullptr;
    }

    const char* RuntimeModeName()
    {
        switch (g_RuntimeMode)
        {
        case RuntimeMode::P1080: return "1080p";
        case RuntimeMode::P4K: return "4K";
        default: return "Xbox";
        }
    }



    template <typename T>
    T D3DMethod(void* object, const std::size_t index)
    {
        if (object == nullptr)
            return nullptr;
        void** const table = *reinterpret_cast<void***>(object);
        return reinterpret_cast<T>(table[index]);
    }

    bool DirectoryExistsOrCreate(const char* path)
    {
        if (CreateDirectoryA(path, nullptr))
            return true;
        return GetLastError() == ERROR_ALREADY_EXISTS;
    }

    bool GetGameDirectory(char* out, const std::size_t outSize)
    {
        if (out == nullptr || outSize == 0)
            return false;
        const DWORD written = GetModuleFileNameA(nullptr, out, static_cast<DWORD>(outSize));
        if (written == 0 || written >= outSize)
            return false;
        for (std::size_t i = written; i > 0; --i)
        {
            if (out[i - 1] == '\\' || out[i - 1] == '/')
            {
                out[i - 1] = '\0';
                return true;
            }
        }
        return false;
    }

    bool GetModuleDirectory(HMODULE module, char* out, const std::size_t outSize)
    {
        if (module == nullptr || out == nullptr || outSize == 0)
            return false;
        const DWORD written = GetModuleFileNameA(module, out, static_cast<DWORD>(outSize));
        if (written == 0 || written >= outSize)
            return false;
        for (std::size_t i = written; i > 0; --i)
        {
            if (out[i - 1] == '\\' || out[i - 1] == '/')
            {
                out[i - 1] = '\0';
                return true;
            }
        }
        return false;
    }

    bool BuildRuntimeIniPath(HMODULE selfModule)
    {
        if (!GetModuleDirectory(selfModule, g_RuntimeModuleDir, sizeof(g_RuntimeModuleDir)))
            return false;
        return std::snprintf(g_RuntimeIniPath, sizeof(g_RuntimeIniPath), "%s\\FMVFix.ini", g_RuntimeModuleDir) > 0;
    }

    void TrimAsciiWhitespace(char* value)
    {
        if (value == nullptr)
            return;

        char* begin = value;
        while (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n')
            ++begin;

        if (begin != value)
            std::memmove(value, begin, std::strlen(begin) + 1u);

        std::size_t length = std::strlen(value);
        while (length > 0)
        {
            const char c = value[length - 1u];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
                break;
            value[--length] = '\0';
        }
    }

    void LoadRuntimeMode()
    {
        g_RuntimeMode = RuntimeMode::Xbox;
        if (g_RuntimeIniPath[0] == '\0')
            return;
        char value[32] = {};
        GetPrivateProfileStringA("FMVFix", "Mode", "Xbox", value, static_cast<DWORD>(sizeof(value)), g_RuntimeIniPath);
        TrimAsciiWhitespace(value);
        if (EqualsAsciiInsensitive(value, "1080p") ||
            EqualsAsciiInsensitive(value, "1080") ||
            EqualsAsciiInsensitive(value, "FHD"))
        {
            g_RuntimeMode = RuntimeMode::P1080;
        }
        else if (EqualsAsciiInsensitive(value, "4K") ||
            EqualsAsciiInsensitive(value, "2160p"))
        {
            g_RuntimeMode = RuntimeMode::P4K;
        }
        else
        {
            g_RuntimeMode = RuntimeMode::Xbox;
        }
    }

    void WriteRuntimeStartupManifest()
    {
        if (!kRuntimeReportsEnabled || g_RuntimeMode == RuntimeMode::Xbox)
            return;

        char root[MAX_PATH] = {};
        char movies[MAX_PATH] = {};
        char movieHd[MAX_PATH] = {};
        char reports[MAX_PATH] = {};
        char manifest[MAX_PATH] = {};
        if (!GetGameDirectory(root, sizeof(root)))
            return;

        const char* const tier = g_RuntimeMode == RuntimeMode::P4K ? "4K" : "1080p";
        if (std::snprintf(movies, sizeof(movies), "%s\\MOVIES", root) <= 0 ||
            std::snprintf(movieHd, sizeof(movieHd), "%s\\MOVIES\\MovieHD", root) <= 0 ||
            std::snprintf(reports, sizeof(reports), "%s\\MOVIES\\MovieHD\\Reports", root) <= 0 ||
            std::snprintf(manifest, sizeof(manifest), "%s\\MOVIES\\MovieHD\\Reports\\FMVFix_MP4Startup_%s.txt", root, tier) <= 0)
        {
            return;
        }

        if (!DirectoryExistsOrCreate(movies) ||
            !DirectoryExistsOrCreate(movieHd) ||
            !DirectoryExistsOrCreate(reports))
        {
            return;
        }

        FILE* const file = std::fopen(manifest, "wb");
        if (file == nullptr)
            return;

        std::fprintf(file,
            "FMVFix MP4 Framework Stage 2.5.2 - Startup/Tier Manifest\r\n"
            "=========================================================\r\n"
            "Build=%s\r\n"
            "RuntimeIniPath=%s\r\n"
            "ParsedMode=%s\r\n"
            "SelectedTier=%s\r\n"
            "NormalExpectedGeometry=%ux%u\r\n"
            "E3ExpectedGeometry=%ux%u\r\n"
            "1080pReportPolicy=ALL-20\r\n"
            "4KReportPolicy=NEW-15\r\n",
            kMp4FrameworkBuild,
            g_RuntimeIniPath[0] != '\0' ? g_RuntimeIniPath : "<none>",
            RuntimeModeName(),
            tier,
            g_RuntimeMode == RuntimeMode::P4K ? 3840u : 2160u,
            g_RuntimeMode == RuntimeMode::P4K ? 1920u : 1080u,
            g_RuntimeMode == RuntimeMode::P4K ? 3840u : 2160u,
            g_RuntimeMode == RuntimeMode::P4K ? 3840u : 2160u);

        for (const auto& movie : kMovieCatalog)
        {
            char path[MAX_PATH] = {};
            const int n = std::snprintf(path, sizeof(path),
                "%s\\MOVIES\\MovieHD\\%s\\%s.mp4", root, tier, movie.stem);
            const DWORD attrs = n > 0 ? GetFileAttributesA(path) : INVALID_FILE_ATTRIBUTES;
            std::fprintf(file, "%s=%s | %s\r\n",
                movie.stem,
                attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0 ? "FOUND" : "MISSING",
                n > 0 ? path : "<path-build-failed>");
        }

        std::fclose(file);
    }

    bool BuildRuntimePaths(const MovieSpec& movie)
    {
        char root[MAX_PATH] = {};
        if (!GetGameDirectory(root, sizeof(root)))
            return false;
        char movies[MAX_PATH] = {};
        char movieHd[MAX_PATH] = {};
        char reports[MAX_PATH] = {};
        const char* tier = g_RuntimeMode == RuntimeMode::P4K ? "4K" : "1080p";
        if (std::snprintf(movies, sizeof(movies), "%s\\MOVIES", root) <= 0 ||
            std::snprintf(movieHd, sizeof(movieHd), "%s\\MOVIES\\MovieHD", root) <= 0 ||
            std::snprintf(reports, sizeof(reports), "%s\\MOVIES\\MovieHD\\Reports", root) <= 0 ||
            std::snprintf(g_RuntimeMp4Path, sizeof(g_RuntimeMp4Path), "%s\\MOVIES\\MovieHD\\%s\\%s.mp4", root, tier, movie.stem) <= 0 ||
            std::snprintf(g_RuntimeLogPath, sizeof(g_RuntimeLogPath), "%s\\MOVIES\\MovieHD\\Reports\\FMVFix_MP4Runtime_%s_%s_G%ld.txt", root, tier, movie.stem, g_RuntimeGeneration) <= 0 ||
            std::snprintf(g_RuntimeCanonicalLogPath, sizeof(g_RuntimeCanonicalLogPath), "%s\\MOVIES\\MovieHD\\Reports\\FMVFix_MP4Runtime_%s_%s.txt", root, tier, movie.stem) <= 0)
        {
            return false;
        }
        return DirectoryExistsOrCreate(movies) &&
            DirectoryExistsOrCreate(movieHd) &&
            (!kRuntimeReportsEnabled || DirectoryExistsOrCreate(reports));
    }

    const char* Mp4ModeTier(const RuntimeMode mode)
    {
        return mode == RuntimeMode::P4K ? "4K" : "1080p";
    }

    bool BuildMp4ProbePaths(const MovieSpec& movie, const RuntimeMode mode, const LONG generation,
        char* mp4Path, const std::size_t mp4PathSize,
        char* logPath, const std::size_t logPathSize)
    {
        if (mp4Path == nullptr || logPath == nullptr || mp4PathSize == 0 || logPathSize == 0)
            return false;

        char root[MAX_PATH] = {};
        char movies[MAX_PATH] = {};
        char movieHd[MAX_PATH] = {};
        char probeDir[MAX_PATH] = {};
        if (!GetGameDirectory(root, sizeof(root)))
            return false;

        const char* const tier = Mp4ModeTier(mode);
        if (std::snprintf(movies, sizeof(movies), "%s\\MOVIES", root) <= 0 ||
            std::snprintf(movieHd, sizeof(movieHd), "%s\\MOVIES\\MovieHD", root) <= 0 ||
            std::snprintf(probeDir, sizeof(probeDir), "%s\\MOVIES\\MovieHD\\Probe", root) <= 0 ||
            std::snprintf(mp4Path, mp4PathSize, "%s\\MOVIES\\MovieHD\\%s\\%s.mp4", root, tier, movie.stem) <= 0 ||
            std::snprintf(logPath, logPathSize, "%s\\MOVIES\\MovieHD\\Probe\\FMVFix_MP4Probe_%s_G%ld.txt",
                root, movie.stem, generation) <= 0)
        {
            return false;
        }

        return DirectoryExistsOrCreate(movies) &&
            DirectoryExistsOrCreate(movieHd) &&
            DirectoryExistsOrCreate(probeDir);
    }

    bool AnsiToWide(const char* input, wchar_t* output, const std::size_t outputCount)
    {
        if (input == nullptr || output == nullptr || outputCount == 0)
            return false;
        const int written = MultiByteToWideChar(CP_ACP, 0, input, -1, output, static_cast<int>(outputCount));
        return written > 0;
    }

    const char* Mp4SubtypeName(const GUID& subtype)
    {
        if (IsEqualGUID(subtype, MFVideoFormat_H264))
            return "H.264/AVC";
#ifdef MFVideoFormat_HEVC
        if (IsEqualGUID(subtype, MFVideoFormat_HEVC))
            return "H.265/HEVC";
#endif
        if (IsEqualGUID(subtype, MFVideoFormat_WMV3))
            return "WMV3";
        return "other/unknown";
    }

    bool WriteMp4ProbeLog(const Mp4ProbeJob& job,
        const char* status,
        const HRESULT hr,
        const UINT32 width,
        const UINT32 height,
        const UINT32 fpsNumerator,
        const UINT32 fpsDenominator,
        const GUID& subtype,
        const ULONGLONG presentationDuration,
        const ULONGLONG sampleCount,
        const LONGLONG firstPts,
        const LONGLONG lastPts,
        const LONGLONG lastDuration,
        const LONGLONG timelineEnd,
        const bool ptsMonotonic,
        const bool geometryMatch,
        const bool sampleCountMatch)
    {
        HANDLE file = CreateFileA(job.logPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;

        const UINT32 expectedWidth = job.mode == RuntimeMode::P4K ? 3840u : 2160u;
        const UINT32 expectedHeight = (job.movie != nullptr && job.movie->sourceHeight == job.movie->sourceWidth)
            ? expectedWidth
            : (job.mode == RuntimeMode::P4K ? 1920u : 1080u);

        char text[4096] = {};
        const int n = std::snprintf(text, sizeof(text),
            "FMVFix MP4 Framework Stage 1 Probe\r\n"
            "===================================\r\n"
            "Build=%s\r\n"
            "Status=%s\r\n"
            "HRESULT=0x%08lX\r\n"
            "Generation=%ld\r\n"
            "Mode=%s\r\n"
            "Movie=%s\r\n"
            "MP4Path=%s\r\n"
            "ExpectedMADFrames=%u\r\n"
            "ExpectedWidth=%u\r\n"
            "ExpectedHeight=%u\r\n"
            "MP4Codec=%s\r\n"
            "MP4Width=%u\r\n"
            "MP4Height=%u\r\n"
            "MP4FrameRateNumerator=%u\r\n"
            "MP4FrameRateDenominator=%u\r\n"
            "MP4PresentationDuration100ns=%llu\r\n"
            "MP4VideoSamples=%llu\r\n"
            "FirstPTS100ns=%lld\r\n"
            "LastPTS100ns=%lld\r\n"
            "LastSampleDuration100ns=%lld\r\n"
            "TimelineEnd100ns=%lld\r\n"
            "PTSMonotonic=%s\r\n"
            "GeometryMatch=%s\r\n"
            "SampleCountMatch=%s\r\n"
            "RendererSubstitution=DISABLED-STAGE1\r\n"
            "MADAudioTimingLifecycle=NATIVE\r\n",
            kMp4FrameworkBuild,
            status != nullptr ? status : "unknown",
            static_cast<unsigned long>(hr),
            job.generation,
            job.mode == RuntimeMode::P4K ? "4K" : "1080p",
            job.movie != nullptr ? job.movie->stem : "none",
            job.mp4Path,
            job.movie != nullptr ? job.movie->frames : 0u,
            expectedWidth,
            expectedHeight,
            Mp4SubtypeName(subtype),
            width,
            height,
            fpsNumerator,
            fpsDenominator,
            static_cast<unsigned long long>(presentationDuration),
            static_cast<unsigned long long>(sampleCount),
            static_cast<long long>(firstPts),
            static_cast<long long>(lastPts),
            static_cast<long long>(lastDuration),
            static_cast<long long>(timelineEnd),
            ptsMonotonic ? "yes" : "no",
            geometryMatch ? "PASS" : "FAIL",
            sampleCountMatch ? "PASS" : "FAIL");

        bool ok = false;
        if (n > 0)
        {
            DWORD written = 0;
            ok = WriteFile(file, text, static_cast<DWORD>(n), &written, nullptr) && written == static_cast<DWORD>(n);
        }
        CloseHandle(file);
        return ok;
    }

    DWORD WINAPI Mp4ProbeThread(LPVOID parameter)
    {
        Mp4ProbeJob* const job = static_cast<Mp4ProbeJob*>(parameter);
        if (job == nullptr)
            return 0;

        HRESULT finalHr = S_OK;
        UINT32 width = 0;
        UINT32 height = 0;
        UINT32 fpsNumerator = 0;
        UINT32 fpsDenominator = 0;
        GUID subtype = GUID_NULL;
        ULONGLONG presentationDuration = 0;
        ULONGLONG sampleCount = 0;
        LONGLONG firstPts = -1;
        LONGLONG lastPts = -1;
        LONGLONG lastDuration = 0;
        LONGLONG timelineEnd = -1;
        bool ptsMonotonic = true;
        bool geometryMatch = false;
        bool sampleCountMatch = false;
        const char* status = "FAIL";

        const HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool coInitialized = SUCCEEDED(coHr);
        if (FAILED(coHr) && coHr != RPC_E_CHANGED_MODE)
        {
            finalHr = coHr;
            status = "COM-INIT-FAILED";
            WriteMp4ProbeLog(*job, status, finalHr, width, height, fpsNumerator, fpsDenominator,
                subtype, presentationDuration, sampleCount, firstPts, lastPts, lastDuration,
                timelineEnd, ptsMonotonic, geometryMatch, sampleCountMatch);
            HeapFree(GetProcessHeap(), 0, job);
            return 0;
        }

        const HRESULT mfHr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
        if (FAILED(mfHr))
        {
            finalHr = mfHr;
            status = "MFSTARTUP-FAILED";
        }
        else
        {
            IMFSourceReader* reader = nullptr;
            IMFMediaType* nativeType = nullptr;
            wchar_t widePath[MAX_PATH] = {};

            if (!AnsiToWide(job->mp4Path, widePath, _countof(widePath)))
            {
                finalHr = E_INVALIDARG;
                status = "PATH-CONVERSION-FAILED";
            }
            else if (GetFileAttributesA(job->mp4Path) == INVALID_FILE_ATTRIBUTES)
            {
                finalHr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                status = "MP4-MISSING";
            }
            else
            {
                finalHr = MFCreateSourceReaderFromURL(widePath, nullptr, &reader);
                if (FAILED(finalHr) || reader == nullptr)
                {
                    status = "SOURCE-READER-OPEN-FAILED";
                }
                else
                {
                    HRESULT typeHr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &nativeType);
                    if (SUCCEEDED(typeHr) && nativeType != nullptr)
                    {
                        MFGetAttributeSize(nativeType, MF_MT_FRAME_SIZE, &width, &height);
                        MFGetAttributeRatio(nativeType, MF_MT_FRAME_RATE, &fpsNumerator, &fpsDenominator);
                        nativeType->GetGUID(MF_MT_SUBTYPE, &subtype);
                    }

                    PROPVARIANT durationVar;
                    PropVariantInit(&durationVar);
                    if (SUCCEEDED(reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &durationVar)))
                    {
                        if (durationVar.vt == VT_UI8)
                            presentationDuration = durationVar.uhVal.QuadPart;
                        else if (durationVar.vt == VT_I8 && durationVar.hVal.QuadPart > 0)
                            presentationDuration = static_cast<ULONGLONG>(durationVar.hVal.QuadPart);
                    }
                    PropVariantClear(&durationVar);

                    LONGLONG previousPts = 0;
                    bool havePreviousPts = false;
                    bool readFailed = false;
                    for (;;)
                    {
                        DWORD actualStream = 0;
                        DWORD flags = 0;
                        LONGLONG timestamp = 0;
                        IMFSample* sample = nullptr;
                        const HRESULT readHr = reader->ReadSample(
                            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                            0,
                            &actualStream,
                            &flags,
                            &timestamp,
                            &sample);

                        if (FAILED(readHr))
                        {
                            finalHr = readHr;
                            status = "VIDEO-SAMPLE-SCAN-FAILED";
                            readFailed = true;
                            if (sample != nullptr)
                                sample->Release();
                            break;
                        }

                        if (sample != nullptr)
                        {
                            LONGLONG duration = 0;
                            if (FAILED(sample->GetSampleDuration(&duration)))
                                duration = 0;

                            if (sampleCount == 0)
                                firstPts = timestamp;
                            if (havePreviousPts && timestamp < previousPts)
                                ptsMonotonic = false;

                            previousPts = timestamp;
                            havePreviousPts = true;
                            lastPts = timestamp;
                            lastDuration = duration;
                            const LONGLONG candidateEnd = timestamp + duration;
                            if (candidateEnd > timelineEnd)
                                timelineEnd = candidateEnd;
                            ++sampleCount;
                            sample->Release();
                        }

                        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
                            break;
                    }

                    if (!readFailed)
                    {
                        finalHr = S_OK;
                        const UINT32 expectedWidth = job->mode == RuntimeMode::P4K ? 3840u : 2160u;
                        const UINT32 expectedHeight = (job->movie != nullptr && job->movie->sourceHeight == job->movie->sourceWidth)
                            ? expectedWidth
                            : (job->mode == RuntimeMode::P4K ? 1920u : 1080u);
                        geometryMatch = width == expectedWidth && height == expectedHeight;
                        sampleCountMatch = job->movie != nullptr && sampleCount == job->movie->frames;
                        status = (geometryMatch && sampleCountMatch && ptsMonotonic)
                            ? "PASS"
                            : "VALIDATION-FAILED";
                    }
                }
            }

            if (nativeType != nullptr)
                nativeType->Release();
            if (reader != nullptr)
                reader->Release();
            MFShutdown();
        }

        WriteMp4ProbeLog(*job, status, finalHr, width, height, fpsNumerator, fpsDenominator,
            subtype, presentationDuration, sampleCount, firstPts, lastPts, lastDuration,
            timelineEnd, ptsMonotonic, geometryMatch, sampleCountMatch);

        if (coInitialized)
            CoUninitialize();
        HeapFree(GetProcessHeap(), 0, job);
        return 0;
    }

    bool BeginMp4Probe(const MovieSpec& movie)
    {
        if (g_RuntimeMode == RuntimeMode::Xbox)
            return false;

        Mp4ProbeJob* const job = static_cast<Mp4ProbeJob*>(
            HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Mp4ProbeJob)));
        if (job == nullptr)
            return false;

        job->movie = &movie;
        job->mode = g_RuntimeMode;
        job->generation = InterlockedCompareExchange(&g_FMVFix_MovieGeneration, 0, 0);
        if (!BuildMp4ProbePaths(movie, job->mode, job->generation,
            job->mp4Path, sizeof(job->mp4Path), job->logPath, sizeof(job->logPath)))
        {
            HeapFree(GetProcessHeap(), 0, job);
            return false;
        }

        HANDLE thread = CreateThread(nullptr, 0, Mp4ProbeThread, job, 0, nullptr);
        if (thread == nullptr)
        {
            HeapFree(GetProcessHeap(), 0, job);
            return false;
        }
        CloseHandle(thread);
        return true;
    }

    bool ReadAll(HANDLE file, void* data, const DWORD bytes)
    {
        if (file == INVALID_HANDLE_VALUE || data == nullptr)
            return false;
        DWORD read = 0;
        return ReadFile(file, data, bytes, &read, nullptr) && read == bytes;
    }

    bool WriteAll(HANDLE file, const void* data, const DWORD bytes)
    {
        if (file == INVALID_HANDLE_VALUE || data == nullptr)
            return false;
        DWORD written = 0;
        return WriteFile(file, data, bytes, &written, nullptr) && written == bytes;
    }

    bool ValidateRuntimeSrHeader(const NfsuSrPackHeader& h, const MovieSpec& movie)
    {
        if (std::memcmp(h.magic, "NFSUSR2A", 8) != 0 || h.version != 1 ||
            h.sourceWidth != movie.sourceWidth || h.sourceHeight != movie.sourceHeight ||
            h.framesTotal != movie.frames || h.framesComplete != movie.frames ||
            h.dataOffset != sizeof(NfsuSrPackHeader) || h.d3dFormat != kD3DFormatA8R8G8B8 ||
            h.presetCode != 1 || h.aiStrength < 0.9999f || h.aiStrength > 1.0001f)
        {
            return false;
        }
        if (h.outputWidth < movie.sourceWidth || h.outputHeight < movie.sourceHeight ||
            h.outputWidth > kRuntimeMaxOutputDimension || h.outputHeight > kRuntimeMaxOutputDimension)
        {
            return false;
        }

        // Tier geometry is intentionally strict. This prevents an old/wrong pack
        // (especially the earlier stretched E3_TITLE export) from being accepted.
        const std::uint32_t expectedWidth = g_RuntimeMode == RuntimeMode::P4K ? 3840u : 2160u;
        const std::uint32_t expectedHeight =
            movie.sourceWidth == movie.sourceHeight
            ? expectedWidth
            : (g_RuntimeMode == RuntimeMode::P4K ? 1920u : 1080u);
        if (h.outputWidth != expectedWidth || h.outputHeight != expectedHeight)
            return false;
        const std::uint64_t expectedFrameBytes =
            static_cast<std::uint64_t>(h.outputWidth) * static_cast<std::uint64_t>(h.outputHeight) * 4ull;
        return expectedFrameBytes != 0 && expectedFrameBytes <= kRuntimeMaxOutputFrameBytes &&
            h.frameBytes == static_cast<std::uint32_t>(expectedFrameBytes);
    }

    bool ConfigureRuntimeOutput(const NfsuSrPackHeader& h, const MovieSpec& movie)
    {
        if (!ValidateRuntimeSrHeader(h, movie))
            return false;
        g_RuntimeOutputWidth = h.outputWidth;
        g_RuntimeOutputHeight = h.outputHeight;
        g_RuntimeOutputFrameBytes = h.frameBytes;
        std::uint64_t slots = kRuntimeRingBudgetBytes / g_RuntimeOutputFrameBytes;
        if (slots < kRuntimeMinRingSlots) slots = kRuntimeMinRingSlots;
        if (slots > kRuntimeMaxRingSlots) slots = kRuntimeMaxRingSlots;
        g_RuntimeRingSlots = static_cast<std::uint32_t>(slots);
        g_RuntimeInitialSyncFrames =
            (g_RuntimeOutputFrameBytes <= kRuntimeSyncPreloadMaxFrameBytes)
            ? (g_RuntimeRingSlots < 8 ? g_RuntimeRingSlots : 8)
            : 0;
        return true;
    }

    bool ValidateRuntimeSrFileSize(HANDLE file, const MovieSpec& movie)
    {
        if (g_RuntimeOutputFrameBytes == 0)
            return false;
        LARGE_INTEGER size = {};
        if (!GetFileSizeEx(file, &size))
            return false;
        const LONGLONG expected = static_cast<LONGLONG>(sizeof(NfsuSrPackHeader)) +
            static_cast<LONGLONG>(movie.frames) * g_RuntimeOutputFrameBytes;
        return size.QuadPart == expected;
    }

    bool LoadRuntimeFrameSequential(const LONG serial)
    {
        const MovieSpec* const movie = g_RuntimeMovie;
        if (movie == nullptr || serial < 0 || serial >= static_cast<LONG>(movie->frames) ||
            g_RuntimeSrFile == INVALID_HANDLE_VALUE || g_RuntimeRing == nullptr ||
            g_RuntimeRingSlots == 0 || g_RuntimeOutputFrameBytes == 0)
        {
            return false;
        }
        const std::uint32_t slot = static_cast<std::uint32_t>(serial) % g_RuntimeRingSlots;
        BYTE* const dst = g_RuntimeRing + static_cast<std::size_t>(slot) * g_RuntimeOutputFrameBytes;
        if (!ReadAll(g_RuntimeSrFile, dst, g_RuntimeOutputFrameBytes))
        {
            InterlockedIncrement(&g_RuntimeSidecarReadFailures);
            return false;
        }
        g_RuntimeRingSerial[slot] = serial;
        MemoryBarrier();
        InterlockedExchange(&g_RuntimeRingState[slot], 1);
        InterlockedIncrement(&g_RuntimeFramesRead);
        return true;
    }


    bool CopyRgb32SampleToRawFrame(
        IMFSample* sample, BYTE* dstFrame,
        const UINT32 visibleWidth, const UINT32 visibleHeight,
        const Mp4CopyLayout& layout)
    {
        if (sample == nullptr || dstFrame == nullptr ||
            visibleWidth == 0 || visibleHeight == 0 ||
            layout.sourceWidth < visibleWidth || layout.sourceHeight < visibleHeight ||
            layout.cropX > layout.sourceWidth - visibleWidth ||
            layout.cropY > layout.sourceHeight - visibleHeight || layout.stride == 0)
            return false;

        const std::size_t rowBytes = static_cast<std::size_t>(visibleWidth) * 4u;
        const std::size_t sourceRowBytes = static_cast<std::size_t>(layout.sourceWidth) * 4u;
        if (rowBytes > static_cast<std::size_t>((std::numeric_limits<LONG>::max)()) ||
            sourceRowBytes > static_cast<std::size_t>((std::numeric_limits<LONG>::max)()))
            return false;

        IMFMediaBuffer* buffer = nullptr;
        HRESULT hr = sample->ConvertToContiguousBuffer(&buffer);
        if (FAILED(hr) || buffer == nullptr)
            return false;

        bool copied = false;
        IMF2DBuffer* buffer2d = nullptr;
        if (SUCCEEDED(buffer->QueryInterface(__uuidof(IMF2DBuffer),
            reinterpret_cast<void**>(&buffer2d))) && buffer2d != nullptr)
        {
            BYTE* scanline0 = nullptr;
            LONG pitch = 0;
            if (SUCCEEDED(buffer2d->Lock2D(&scanline0, &pitch)))
            {
                const std::int64_t absPitch = pitch < 0
                    ? -static_cast<std::int64_t>(pitch) : static_cast<std::int64_t>(pitch);
                if (scanline0 != nullptr && pitch != 0 &&
                    absPitch >= static_cast<std::int64_t>(sourceRowBytes))
                {
                    const std::ptrdiff_t startOffset =
                        static_cast<std::ptrdiff_t>(layout.cropY) * pitch +
                        static_cast<std::ptrdiff_t>(layout.cropX) * 4;
                    const BYTE* visibleStart = scanline0 + startOffset;
                    copied = SUCCEEDED(MFCopyImage(dstFrame, static_cast<LONG>(rowBytes),
                        visibleStart, pitch, static_cast<DWORD>(rowBytes), visibleHeight));
                }
                buffer2d->Unlock2D();
            }
            buffer2d->Release();
        }

        if (!copied)
        {
            BYTE* data = nullptr;
            DWORD maxLength = 0;
            DWORD currentLength = 0;
            if (SUCCEEDED(buffer->Lock(&data, &maxLength, &currentLength)))
            {
                const std::int64_t absStride64 = layout.stride < 0
                    ? -static_cast<std::int64_t>(layout.stride)
                    : static_cast<std::int64_t>(layout.stride);
                const std::size_t absStride = static_cast<std::size_t>(absStride64);
                const std::size_t fullFootprint =
                    (static_cast<std::size_t>(layout.sourceHeight) - 1u) * absStride +
                    sourceRowBytes;
                if (data != nullptr && absStride >= sourceRowBytes &&
                    fullFootprint <= static_cast<std::size_t>(maxLength) &&
                    fullFootprint <= static_cast<std::size_t>(currentLength))
                {
                    const std::size_t firstRowIndex = layout.stride < 0
                        ? static_cast<std::size_t>(layout.sourceHeight - 1u - layout.cropY)
                        : static_cast<std::size_t>(layout.cropY);
                    const std::size_t offset = firstRowIndex * absStride +
                        static_cast<std::size_t>(layout.cropX) * 4u;
                    const BYTE* visibleStart = data + offset;
                    copied = SUCCEEDED(MFCopyImage(dstFrame, static_cast<LONG>(rowBytes),
                        visibleStart, layout.stride,
                        static_cast<DWORD>(rowBytes), visibleHeight));
                }
                buffer->Unlock();
            }
        }
        buffer->Release();
        return copied;
    }

    bool CopyRgb32SampleToRing(IMFSample* sample, const LONG serial)
    {
        if (sample == nullptr || serial < 0 || g_RuntimeRing == nullptr ||
            g_RuntimeRingSlots == 0 || g_RuntimeOutputWidth == 0 || g_RuntimeOutputHeight == 0)
            return false;
        const std::uint32_t slot = static_cast<std::uint32_t>(serial) % g_RuntimeRingSlots;
        const LONG current = InterlockedCompareExchange(&g_RuntimeCurrentSerial, 0, 0);
        if (current > serial)
        {
            InterlockedIncrement(&g_Mp4DecodedDiscarded);
            return true;
        }
        while (InterlockedCompareExchange(&g_RuntimeRingState[slot], 0, 0) != 0)
        {
            if (g_RuntimeReaderStopEvent != nullptr &&
                WaitForSingleObject(g_RuntimeReaderStopEvent, 0) == WAIT_OBJECT_0)
                return false;
            const LONG resident = g_RuntimeRingSerial[slot];
            const LONG now = InterlockedCompareExchange(&g_RuntimeCurrentSerial, 0, 0);
            if (now > resident)
            {
                InterlockedExchange(&g_RuntimeRingState[slot], 0);
                continue;
            }
            Sleep(1);
        }

        BYTE* const dstFrame = g_RuntimeRing +
            static_cast<std::size_t>(slot) * g_RuntimeOutputFrameBytes;
        if (!CopyRgb32SampleToRawFrame(sample, dstFrame,
            g_RuntimeOutputWidth, g_RuntimeOutputHeight, g_Mp4CopyLayout))
            return false;

        g_RuntimeRingSerial[slot] = serial;
        MemoryBarrier();
        InterlockedExchange(&g_RuntimeRingState[slot], 1);
        if (InterlockedCompareExchange(&g_Mp4FirstReadySerial, 0, -1) == -1)
            InterlockedExchange(&g_Mp4FirstReadySerial, serial);
        InterlockedExchange(&g_Mp4LastReadySerial, serial);
        return true;
    }

    HRESULT RefreshMp4OutputType(IMFSourceReader* reader)
    {
        Mp4CopyLayout layout = {};
        UINT32 subtypeData1 = 0;
        const HRESULT hr = ResolveMp4RgbCopyLayout(
            reader, g_Mp4NativeGeometry, &layout, &subtypeData1);
        g_Mp4CurrentSubtypeData1 = subtypeData1;
        if (SUCCEEDED(hr))
        {
            g_Mp4CopyLayout = layout;
            g_Mp4OutputStride = layout.stride;
        }
        return hr;
    }

    HRESULT RefreshMp4OutputTypeLocal(
        IMFSourceReader* reader, const Mp4FrameGeometry& geometry,
        Mp4CopyLayout* layout, UINT32* subtypeData1Out)
    {
        return ResolveMp4RgbCopyLayout(reader, geometry, layout, subtypeData1Out);
    }

    DWORD WINAPI Mp4StartupPrewarmThread(LPVOID)
    {
        HRESULT finalHr = S_OK;
        const HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool coInitialized = SUCCEEDED(coHr);
        if (FAILED(coHr) && coHr != RPC_E_CHANGED_MODE)
        {
            InterlockedExchange(&g_Mp4PrewarmHr, coHr);
            InterlockedExchange(&g_Mp4PrewarmState, 3);
            return 0;
        }

        const HRESULT mfHr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
        if (FAILED(mfHr))
        {
            InterlockedExchange(&g_Mp4PrewarmHr, mfHr);
            InterlockedExchange(&g_Mp4PrewarmState, 3);
            if (coInitialized)
                CoUninitialize();
            return 0;
        }

        IMFAttributes* attributes = nullptr;
        IMFSourceReader* reader = nullptr;
        IMFMediaType* nativeType = nullptr;
        IMFMediaType* outputType = nullptr;
        wchar_t widePath[MAX_PATH] = {};
        Mp4FrameGeometry prewarmGeometry = {};
        Mp4CopyLayout prewarmLayout = {};
        UINT32 subtypeData1 = 0;

        do
        {
            if (!AnsiToWide(g_Mp4PrewarmPath, widePath, _countof(widePath)))
            {
                finalHr = E_INVALIDARG;
                break;
            }

            finalHr = MFCreateAttributes(&attributes, 4);
            if (FAILED(finalHr) || attributes == nullptr)
                break;
            attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
            attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

            finalHr = MFCreateSourceReaderFromURL(widePath, attributes, &reader);
            if (FAILED(finalHr) || reader == nullptr)
                break;
            reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
            finalHr = reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
            if (FAILED(finalHr))
                break;

            finalHr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &nativeType);
            if (FAILED(finalHr) || nativeType == nullptr)
                break;

            UINT32 width = 0;
            UINT32 height = 0;
            finalHr = ResolveMp4NativeGeometry(
                nativeType, g_Mp4PrewarmWidth, g_Mp4PrewarmHeight, &prewarmGeometry);
            if (FAILED(finalHr))
                break;
            width = prewarmGeometry.codedWidth;
            height = prewarmGeometry.codedHeight;

            finalHr = MFCreateMediaType(&outputType);
            if (FAILED(finalHr) || outputType == nullptr)
                break;
            outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
            MFSetAttributeSize(outputType, MF_MT_FRAME_SIZE, width, height);
            outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

            finalHr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);
            if (FAILED(finalHr))
                break;

            finalHr = RefreshMp4OutputTypeLocal(
                reader, prewarmGeometry, &prewarmLayout, &subtypeData1);
            if (FAILED(finalHr))
                break;

            LONG serial = 0;
            while (serial < static_cast<LONG>(g_Mp4PrewarmRingSlots))
            {
                DWORD actualStream = 0;
                DWORD flags = 0;
                LONGLONG timestamp = 0;
                IMFSample* sample = nullptr;
                HRESULT readHr = reader->ReadSample(
                    MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
                    &actualStream, &flags, &timestamp, &sample);

                if (readHr == MF_E_TRANSFORM_STREAM_CHANGE)
                {
                    HRESULT typeHr = RefreshMp4OutputTypeLocal(
                        reader, prewarmGeometry, &prewarmLayout, &subtypeData1);
                    if (FAILED(typeHr))
                    {
                        typeHr = reader->SetCurrentMediaType(
                            MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);
                        if (SUCCEEDED(typeHr))
                            typeHr = RefreshMp4OutputTypeLocal(
                                reader, prewarmGeometry, &prewarmLayout, &subtypeData1);
                    }
                    if (sample != nullptr)
                        sample->Release();
                    if (FAILED(typeHr))
                    {
                        finalHr = typeHr;
                        break;
                    }
                    continue;
                }

                if (FAILED(readHr))
                {
                    if (sample != nullptr)
                        sample->Release();
                    finalHr = readHr;
                    break;
                }

                if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0)
                {
                    HRESULT typeHr = RefreshMp4OutputTypeLocal(
                        reader, prewarmGeometry, &prewarmLayout, &subtypeData1);
                    if (FAILED(typeHr) && sample == nullptr)
                    {
                        typeHr = reader->SetCurrentMediaType(
                            MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);
                        if (SUCCEEDED(typeHr))
                            typeHr = RefreshMp4OutputTypeLocal(
                                reader, prewarmGeometry, &prewarmLayout, &subtypeData1);
                    }
                    if (FAILED(typeHr))
                    {
                        if (sample != nullptr)
                            sample->Release();
                        finalHr = typeHr;
                        break;
                    }
                }

                if (sample != nullptr)
                {
                    BYTE* const dst = g_Mp4PrewarmRing +
                        static_cast<std::size_t>(serial) * g_Mp4PrewarmFrameBytes;
                    if (!CopyRgb32SampleToRawFrame(
                        sample, dst, g_Mp4PrewarmWidth, g_Mp4PrewarmHeight, prewarmLayout))
                    {
                        sample->Release();
                        finalHr = E_FAIL;
                        break;
                    }
                    sample->Release();
                    ++serial;
                    InterlockedExchange(&g_Mp4PrewarmReadyFrames, serial);
                }

                if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0 &&
                    serial < static_cast<LONG>(g_Mp4PrewarmRingSlots))
                {
                    finalHr = E_FAIL;
                    break;
                }
            }
        } while (false);

        if (outputType != nullptr)
            outputType->Release();
        if (nativeType != nullptr)
            nativeType->Release();
        if (reader != nullptr)
            reader->Release();
        if (attributes != nullptr)
            attributes->Release();
        MFShutdown();
        if (coInitialized)
            CoUninitialize();

        InterlockedExchange(&g_Mp4PrewarmHr, finalHr);
        if (SUCCEEDED(finalHr) &&
            InterlockedCompareExchange(&g_Mp4PrewarmReadyFrames, 0, 0) ==
            static_cast<LONG>(g_Mp4PrewarmRingSlots))
        {
            InterlockedExchange(&g_Mp4PrewarmState, 2);
        }
        else
        {
            InterlockedExchange(&g_Mp4PrewarmState, 3);
        }
        return 0;
    }

    void ResetUnusedMp4PrewarmCache()
    {
        const LONG state = InterlockedCompareExchange(&g_Mp4PrewarmState, 0, 0);
        if (state == 1)
            return; // never block StartMovie waiting for an in-flight prewarm

        if (g_Mp4PrewarmThread != nullptr)
        {
            if (WaitForSingleObject(g_Mp4PrewarmThread, 0) == WAIT_OBJECT_0)
            {
                CloseHandle(g_Mp4PrewarmThread);
                g_Mp4PrewarmThread = nullptr;
            }
            else
            {
                return;
            }
        }

        // If state==4 the buffer was adopted by g_RuntimeRing and this pointer is
        // already null. If state==2/3 but it was never adopted, release it now.
        if (g_Mp4PrewarmRing != nullptr)
        {
            VirtualFree(g_Mp4PrewarmRing, 0, MEM_RELEASE);
            g_Mp4PrewarmRing = nullptr;
        }

        g_Mp4PrewarmMode = RuntimeMode::Xbox;
        g_Mp4PrewarmWidth = 0;
        g_Mp4PrewarmHeight = 0;
        g_Mp4PrewarmFrameBytes = 0;
        g_Mp4PrewarmRingSlots = 0;
        g_Mp4PrewarmPath[0] = '\0';
        InterlockedExchange(&g_Mp4PrewarmReadyFrames, 0);
        InterlockedExchange(&g_Mp4PrewarmHr, S_OK);
        InterlockedExchange(&g_Mp4PrewarmState, 0);
    }

    void StartMp4PrewarmForMovie(const MovieSpec& movie)
    {
        if (g_RuntimeMode == RuntimeMode::Xbox)
            return;

        ResetUnusedMp4PrewarmCache();
        if (InterlockedCompareExchange(&g_Mp4PrewarmState, 0, 0) != 0)
            return;

        char root[MAX_PATH] = {};
        if (!GetGameDirectory(root, sizeof(root)))
            return;

        const UINT32 width = g_RuntimeMode == RuntimeMode::P4K ? 3840u : 2160u;
        const UINT32 height = movie.sourceWidth == movie.sourceHeight
            ? width
            : (g_RuntimeMode == RuntimeMode::P4K ? 1920u : 1080u);
        const std::uint64_t frameBytes64 = static_cast<std::uint64_t>(width) * height * 4ull;
        const std::uint64_t ringBytes64 = frameBytes64 * kMp4StartupPrewarmFrames;
        if (frameBytes64 == 0 || frameBytes64 > kRuntimeMaxOutputFrameBytes ||
            ringBytes64 > static_cast<std::uint64_t>((std::numeric_limits<SIZE_T>::max)()))
        {
            InterlockedExchange(&g_Mp4PrewarmHr, E_OUTOFMEMORY);
            InterlockedExchange(&g_Mp4PrewarmState, 3);
            return;
        }

        const char* const tier = g_RuntimeMode == RuntimeMode::P4K ? "4K" : "1080p";
        if (std::snprintf(g_Mp4PrewarmPath, sizeof(g_Mp4PrewarmPath),
            "%s\\MOVIES\\MovieHD\\%s\\%s.mp4", root, tier, movie.stem) <= 0 ||
            GetFileAttributesA(g_Mp4PrewarmPath) == INVALID_FILE_ATTRIBUTES)
        {
            InterlockedExchange(&g_Mp4PrewarmHr, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
            InterlockedExchange(&g_Mp4PrewarmState, 3);
            return;
        }

        g_Mp4PrewarmMode = g_RuntimeMode;
        g_Mp4PrewarmWidth = width;
        g_Mp4PrewarmHeight = height;
        g_Mp4PrewarmFrameBytes = static_cast<std::uint32_t>(frameBytes64);
        g_Mp4PrewarmRingSlots = kMp4StartupPrewarmFrames;
        g_Mp4PrewarmRing = static_cast<BYTE*>(VirtualAlloc(
            nullptr, static_cast<SIZE_T>(ringBytes64), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (g_Mp4PrewarmRing == nullptr)
        {
            InterlockedExchange(&g_Mp4PrewarmHr, E_OUTOFMEMORY);
            InterlockedExchange(&g_Mp4PrewarmState, 3);
            return;
        }

        InterlockedExchange(&g_Mp4PrewarmReadyFrames, 0);
        InterlockedExchange(&g_Mp4PrewarmHr, S_OK);
        InterlockedExchange(&g_Mp4PrewarmState, 1);
        g_Mp4PrewarmThread = CreateThread(nullptr, 0, Mp4StartupPrewarmThread, nullptr, 0, nullptr);
        if (g_Mp4PrewarmThread == nullptr)
        {
            VirtualFree(g_Mp4PrewarmRing, 0, MEM_RELEASE);
            g_Mp4PrewarmRing = nullptr;
            InterlockedExchange(&g_Mp4PrewarmHr, HRESULT_FROM_WIN32(GetLastError()));
            InterlockedExchange(&g_Mp4PrewarmState, 3);
        }
    }

    void StartStage25NextPrewarm(const MovieSpec& currentMovie)
    {
        const MovieSpec* const next = Stage25NextPrewarmTarget(currentMovie);
        if (next != nullptr)
            StartMp4PrewarmForMovie(*next);
    }

    DWORD WINAPI RuntimeReaderThread(LPVOID)
    {
        HRESULT finalHr = S_OK;
        const HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool coInitialized = SUCCEEDED(coHr);
        if (FAILED(coHr) && coHr != RPC_E_CHANGED_MODE)
        {
            NotifyRuntimeError(kErrorDecoder,
                g_RuntimeMovie != nullptr ? g_RuntimeMovie->stem : "unknown",
                "Media Foundation COM initialization failed", coHr);
            InterlockedExchange(&g_Mp4DecoderHr, coHr);
            InterlockedIncrement(&g_RuntimeSidecarReadFailures);
            return 0;
        }

        const HRESULT mfHr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
        if (FAILED(mfHr))
        {
            NotifyRuntimeError(kErrorDecoder,
                g_RuntimeMovie != nullptr ? g_RuntimeMovie->stem : "unknown",
                "Media Foundation startup failed", mfHr);
            InterlockedExchange(&g_Mp4DecoderHr, mfHr);
            InterlockedIncrement(&g_RuntimeSidecarReadFailures);
            if (coInitialized)
                CoUninitialize();
            return 0;
        }

        IMFAttributes* attributes = nullptr;
        IMFSourceReader* reader = nullptr;
        IMFMediaType* nativeType = nullptr;
        IMFMediaType* outputType = nullptr;
        wchar_t widePath[MAX_PATH] = {};

        do
        {
            if (!AnsiToWide(g_RuntimeMp4Path, widePath, _countof(widePath)))
            {
                finalHr = E_INVALIDARG;
                break;
            }

            finalHr = MFCreateAttributes(&attributes, 4);
            if (FAILED(finalHr) || attributes == nullptr)
                break;

            attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
            attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

            finalHr = MFCreateSourceReaderFromURL(widePath, attributes, &reader);
            if (FAILED(finalHr) || reader == nullptr)
                break;

            reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
            finalHr = reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
            if (FAILED(finalHr))
                break;

            finalHr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &nativeType);
            if (FAILED(finalHr) || nativeType == nullptr)
                break;

            UINT32 width = 0;
            UINT32 height = 0;
            UINT32 fpsN = 0;
            UINT32 fpsD = 0;
            MFGetAttributeSize(nativeType, MF_MT_FRAME_SIZE, &width, &height);
            MFGetAttributeRatio(nativeType, MF_MT_FRAME_RATE, &fpsN, &fpsD);
            g_Mp4NativeWidth = width;
            g_Mp4NativeHeight = height;
            g_Mp4FrameRateNumerator = fpsN;
            g_Mp4FrameRateDenominator = fpsD;

            PROPVARIANT durationVar;
            PropVariantInit(&durationVar);
            if (SUCCEEDED(reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &durationVar)))
            {
                if (durationVar.vt == VT_UI8)
                    g_Mp4PresentationDuration100ns = durationVar.uhVal.QuadPart;
                else if (durationVar.vt == VT_I8 && durationVar.hVal.QuadPart > 0)
                    g_Mp4PresentationDuration100ns = static_cast<ULONGLONG>(durationVar.hVal.QuadPart);
            }
            PropVariantClear(&durationVar);

            finalHr = ResolveMp4NativeGeometry(
                nativeType, g_RuntimeOutputWidth, g_RuntimeOutputHeight, &g_Mp4NativeGeometry);
            if (FAILED(finalHr))
                break;

            finalHr = MFCreateMediaType(&outputType);
            if (FAILED(finalHr) || outputType == nullptr)
                break;
            outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
            MFSetAttributeSize(outputType, MF_MT_FRAME_SIZE, width, height);
            if (fpsN != 0 && fpsD != 0)
                MFSetAttributeRatio(outputType, MF_MT_FRAME_RATE, fpsN, fpsD);
            outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

            finalHr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);
            if (FAILED(finalHr))
                break;

            finalHr = RefreshMp4OutputType(reader);
            if (FAILED(finalHr))
                break;

            g_RuntimeHeaderValid = true;

            LONG serial = 0;
            while (g_RuntimeReaderStopEvent == nullptr ||
                WaitForSingleObject(g_RuntimeReaderStopEvent, 0) != WAIT_OBJECT_0)
            {
                DWORD actualStream = 0;
                DWORD flags = 0;
                LONGLONG timestamp = 0;
                IMFSample* sample = nullptr;
                const HRESULT readHr = reader->ReadSample(
                    MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                    0,
                    &actualStream,
                    &flags,
                    &timestamp,
                    &sample);

                if (readHr == MF_E_TRANSFORM_STREAM_CHANGE)
                {
                    InterlockedIncrement(&g_Mp4MediaTypeChanges);
                    HRESULT typeHr = RefreshMp4OutputType(reader);
                    if (FAILED(typeHr))
                    {
                        typeHr = reader->SetCurrentMediaType(
                            MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);
                        if (SUCCEEDED(typeHr))
                        {
                            InterlockedIncrement(&g_Mp4MediaTypeRenegotiations);
                            typeHr = RefreshMp4OutputType(reader);
                        }
                    }
                    if (sample != nullptr)
                        sample->Release();
                    if (FAILED(typeHr))
                    {
                        finalHr = typeHr;
                        break;
                    }
                    continue;
                }

                if (FAILED(readHr))
                {
                    finalHr = readHr;
                    if (sample != nullptr)
                        sample->Release();
                    break;
                }

                if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0)
                {
                    InterlockedIncrement(&g_Mp4MediaTypeChanges);
                    HRESULT typeHr = RefreshMp4OutputType(reader);

                    // A decoder may report a format change before producing the
                    // first RGB32 sample. If it moved away from the requested
                    // output type and no sample was returned on this call,
                    // re-assert RGB32 and continue without advancing the MAD/MP4
                    // serial mapping. Never reinterpret a returned non-RGB sample.
                    if (FAILED(typeHr) && sample == nullptr)
                    {
                        typeHr = reader->SetCurrentMediaType(
                            MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);
                        if (SUCCEEDED(typeHr))
                        {
                            InterlockedIncrement(&g_Mp4MediaTypeRenegotiations);
                            typeHr = RefreshMp4OutputType(reader);
                        }
                    }

                    if (FAILED(typeHr))
                    {
                        finalHr = typeHr;
                        if (sample != nullptr)
                            sample->Release();
                        break;
                    }
                }

                if (sample != nullptr)
                {
                    if (g_RuntimeMovie == nullptr || serial >= static_cast<LONG>(g_RuntimeMovie->frames))
                    {
                        sample->Release();
                        finalHr = E_FAIL;
                        break;
                    }

                    const LONG skipPrefix = InterlockedCompareExchange(&g_Mp4DecoderSkipPrefix, 0, 0);
                    if (serial < skipPrefix)
                    {
                        // Stage 2.3 already has this exact serial in the startup
                        // prewarm ring. Decode/release it only to advance the HEVC
                        // stream, then begin normal publishing at skipPrefix.
                        sample->Release();
                        InterlockedIncrement(&g_RuntimeFramesRead);
                        InterlockedExchange(&g_RuntimeNextReadSerial, serial + 1);
                        ++serial;
                        continue;
                    }

                    if (!CopyRgb32SampleToRing(sample, serial))
                    {
                        sample->Release();
                        if (g_RuntimeReaderStopEvent != nullptr &&
                            WaitForSingleObject(g_RuntimeReaderStopEvent, 0) == WAIT_OBJECT_0)
                        {
                            finalHr = S_OK;
                            break;
                        }
                        finalHr = E_FAIL;
                        break;
                    }

                    sample->Release();
                    InterlockedIncrement(&g_RuntimeFramesRead);
                    InterlockedExchange(&g_RuntimeNextReadSerial, serial + 1);
                    ++serial;
                }

                if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
                {
                    if (g_RuntimeMovie != nullptr && serial == static_cast<LONG>(g_RuntimeMovie->frames))
                    {
                        InterlockedExchange(&g_Mp4DecoderEos, 1);
                        finalHr = S_OK;
                    }
                    else
                    {
                        finalHr = E_FAIL;
                    }
                    break;
                }
            }
        } while (false);

        if (FAILED(finalHr))
        {
            NotifyRuntimeError(kErrorDecoder,
                g_RuntimeMovie != nullptr ? g_RuntimeMovie->stem : "unknown",
                "HEVC decode, display aperture or RGB output validation failed", finalHr);
            g_RuntimeHeaderValid = false;
            InterlockedIncrement(&g_RuntimeSidecarReadFailures);
        }
        InterlockedExchange(&g_Mp4DecoderHr, finalHr);

        if (outputType != nullptr)
            outputType->Release();
        if (nativeType != nullptr)
            nativeType->Release();
        if (reader != nullptr)
            reader->Release();
        if (attributes != nullptr)
            attributes->Release();
        MFShutdown();
        if (coInitialized)
            CoUninitialize();
        return 0;
    }

    void ResetRuntimeCounters()
    {
        InterlockedExchange(&g_RuntimeCurrentSerial, -1);
        InterlockedExchange(&g_RuntimeFrameClockHits, 0);
        InterlockedExchange(&g_RuntimeExcessFrameEvents, 0);
        InterlockedExchange(&g_RuntimeFramesRead, 0);
        InterlockedExchange(&g_RuntimeSidecarReadFailures, 0);
        InterlockedExchange(&g_RuntimeMatchingDraws, 0);
        InterlockedExchange(&g_RuntimeExactFrameHits, 0);
        InterlockedExchange(&g_RuntimeFallbackDraws, 0);
        InterlockedExchange(&g_RuntimeDistinctMisses, 0);
        InterlockedExchange(&g_RuntimeTextureUploadFailures, 0);
        InterlockedExchange(&g_RuntimeSetTextureFailures, 0);
        InterlockedExchange(&g_RuntimeRestoreTextureFailures, 0);
        InterlockedExchange(&g_RuntimeVertexAdjustedDraws, 0);
        InterlockedExchange(&g_RuntimeLastHitSerial, -1);
        InterlockedExchange(&g_RuntimeLastMissSerial, -1);
        InterlockedExchange(&g_RuntimeUploadedSerial, -1);
        InterlockedExchange(&g_RuntimeNextReadSerial, 0);
        InterlockedExchange(&g_Mp4DecoderHr, S_OK);
        InterlockedExchange(&g_Mp4DecoderEos, 0);
        InterlockedExchange(&g_Mp4DecodedDiscarded, 0);
        InterlockedExchange(&g_Mp4FirstReadySerial, -1);
        InterlockedExchange(&g_Mp4LastReadySerial, -1);
        InterlockedExchange(&g_Mp4MediaTypeChanges, 0);
        InterlockedExchange(&g_Mp4MediaTypeRenegotiations, 0);
        InterlockedExchange(&g_Mp4StartupPrewarmUsed, 0);
        InterlockedExchange(&g_Mp4StartupPrewarmFrames, 0);
        InterlockedExchange(&g_Mp4PrewarmStateAtStart, 0);
        InterlockedExchange(&g_Mp4PrewarmHrAtStart, S_OK);
        InterlockedExchange(&g_Mp4DecoderSkipPrefix, 0);
        g_RuntimeOutputWidth = 0;
        g_RuntimeOutputHeight = 0;
        g_RuntimeOutputFrameBytes = 0;
        g_RuntimeRingSlots = 0;
        g_RuntimeInitialSyncFrames = 0;
        g_RuntimeMp4Path[0] = '\0';
        g_Mp4NativeWidth = 0;
        g_Mp4NativeHeight = 0;
        g_Mp4NativeGeometry = {};
        g_Mp4CopyLayout = {};
        g_Mp4FrameRateNumerator = 0;
        g_Mp4FrameRateDenominator = 0;
        g_Mp4PresentationDuration100ns = 0;
        g_Mp4OutputStride = 0;
        g_Mp4CurrentSubtypeData1 = 0;
        for (std::uint32_t i = 0; i < kRuntimeMaxRingSlots; ++i)
        {
            InterlockedExchange(&g_RuntimeRingState[i], 0);
            g_RuntimeRingSerial[i] = -1;
        }
    }

    void ReleaseComObject(void*& object)
    {
        if (object == nullptr)
            return;
        const auto release = D3DMethod<UnknownReleaseFn>(object, kIUnknownReleaseIndex);
        if (release != nullptr)
            release(object);
        object = nullptr;
    }

    void CloseRuntimeObjects()
    {
        if (g_RuntimeReaderStopEvent != nullptr)
            SetEvent(g_RuntimeReaderStopEvent);
        if (g_RuntimeReaderThread != nullptr)
        {
            WaitForSingleObject(g_RuntimeReaderThread, INFINITE);
            CloseHandle(g_RuntimeReaderThread);
            g_RuntimeReaderThread = nullptr;
        }
        if (g_RuntimeReaderStopEvent != nullptr)
        {
            CloseHandle(g_RuntimeReaderStopEvent);
            g_RuntimeReaderStopEvent = nullptr;
        }
        ReleaseComObject(g_RuntimeReplacementTexture);
        if (g_RuntimeRing != nullptr)
        {
            VirtualFree(g_RuntimeRing, 0, MEM_RELEASE);
            g_RuntimeRing = nullptr;
        }
        if (g_RuntimeSrFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(g_RuntimeSrFile);
            g_RuntimeSrFile = INVALID_HANDLE_VALUE;
        }
    }

    bool RuntimeSessionIsCanonicalPass(const char* finalState)
    {
        const MovieSpec* const movie = g_RuntimeMovie;
        if (movie == nullptr || finalState == nullptr || std::strcmp(finalState, "next-StartMovie") != 0)
            return false;

        const LONG currentSerial = InterlockedCompareExchange(&g_RuntimeCurrentSerial, 0, 0);
        const LONG clockHits = InterlockedCompareExchange(&g_RuntimeFrameClockHits, 0, 0);
        const LONG excess = InterlockedCompareExchange(&g_RuntimeExcessFrameEvents, 0, 0);
        const LONG framesDecoded = InterlockedCompareExchange(&g_RuntimeFramesRead, 0, 0);
        const LONG decodeFailures = InterlockedCompareExchange(&g_RuntimeSidecarReadFailures, 0, 0);
        const LONG exactHits = InterlockedCompareExchange(&g_RuntimeExactFrameHits, 0, 0);
        const LONG misses = InterlockedCompareExchange(&g_RuntimeDistinctMisses, 0, 0);
        const LONG uploadFailures = InterlockedCompareExchange(&g_RuntimeTextureUploadFailures, 0, 0);
        const LONG setFailures = InterlockedCompareExchange(&g_RuntimeSetTextureFailures, 0, 0);
        const LONG restoreFailures = InterlockedCompareExchange(&g_RuntimeRestoreTextureFailures, 0, 0);
        const LONG decoderHr = InterlockedCompareExchange(&g_Mp4DecoderHr, 0, 0);
        const LONG decoderEos = InterlockedCompareExchange(&g_Mp4DecoderEos, 0, 0);

        return
            g_RuntimeHeaderValid &&
            g_RuntimeCreateTexturePass &&
            clockHits == static_cast<LONG>(movie->frames) &&
            currentSerial == static_cast<LONG>(movie->frames) - 1 &&
            excess == 0 &&
            framesDecoded == static_cast<LONG>(movie->frames) &&
            decodeFailures == 0 &&
            SUCCEEDED(static_cast<HRESULT>(decoderHr)) &&
            decoderEos == 1 &&
            exactHits == static_cast<LONG>(movie->frames) &&
            misses == 0 &&
            uploadFailures == 0 &&
            setFailures == 0 &&
            restoreFailures == 0;
    }

    bool WriteRuntimeLogToPath(
        const char* path,
        const char* finalState,
        const char* canonicalPromotion)
    {
        if (path == nullptr || path[0] == '\0')
            return false;
        HANDLE log = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (log == INVALID_HANDLE_VALUE)
            return false;

        const MovieSpec* const movie = g_RuntimeMovie;
        const LONG active = InterlockedCompareExchange(&g_RuntimeActive, 0, 0);
        const LONG currentSerial = InterlockedCompareExchange(&g_RuntimeCurrentSerial, 0, 0);
        const LONG clockHits = InterlockedCompareExchange(&g_RuntimeFrameClockHits, 0, 0);
        const LONG excess = InterlockedCompareExchange(&g_RuntimeExcessFrameEvents, 0, 0);
        const LONG decoded = InterlockedCompareExchange(&g_RuntimeFramesRead, 0, 0);
        const LONG decodeFailures = InterlockedCompareExchange(&g_RuntimeSidecarReadFailures, 0, 0);
        const LONG matchingDraws = InterlockedCompareExchange(&g_RuntimeMatchingDraws, 0, 0);
        const LONG exactHits = InterlockedCompareExchange(&g_RuntimeExactFrameHits, 0, 0);
        const LONG fallbacks = InterlockedCompareExchange(&g_RuntimeFallbackDraws, 0, 0);
        const LONG distinctMisses = InterlockedCompareExchange(&g_RuntimeDistinctMisses, 0, 0);
        const LONG uploadFailures = InterlockedCompareExchange(&g_RuntimeTextureUploadFailures, 0, 0);
        const LONG setFailures = InterlockedCompareExchange(&g_RuntimeSetTextureFailures, 0, 0);
        const LONG restoreFailures = InterlockedCompareExchange(&g_RuntimeRestoreTextureFailures, 0, 0);
        const LONG vertexDraws = InterlockedCompareExchange(&g_RuntimeVertexAdjustedDraws, 0, 0);
        const LONG decoderHr = InterlockedCompareExchange(&g_Mp4DecoderHr, 0, 0);
        const LONG decoderEos = InterlockedCompareExchange(&g_Mp4DecoderEos, 0, 0);
        const LONG discarded = InterlockedCompareExchange(&g_Mp4DecodedDiscarded, 0, 0);
        const LONG firstReady = InterlockedCompareExchange(&g_Mp4FirstReadySerial, 0, 0);
        const LONG lastReady = InterlockedCompareExchange(&g_Mp4LastReadySerial, 0, 0);
        const LONG mediaTypeChanges = InterlockedCompareExchange(&g_Mp4MediaTypeChanges, 0, 0);
        const LONG mediaTypeRenegotiations = InterlockedCompareExchange(&g_Mp4MediaTypeRenegotiations, 0, 0);

        char report[6144] = {};
        const int n = std::snprintf(report, sizeof(report),
            "FMVFix MP4 Framework Stage 2.5.3 - Full Catalog + 1080p Display Aperture\r\n"
            "===========================================================\r\n"
            "Build=%s\r\n"
            "RuntimeIniPath=%s\r\n"
            "Mode=%s\r\nMovie=%s\r\nGeneration=%ld\r\n"
            "MP4Path=%s\r\n"
            "MADClock=%s\r\nMADAudioTimingLifecycle=NATIVE\r\n"
            "Mapping=EXACT-SERIAL-N-to-MP4-SAMPLE-N\r\n"
            "ExpectedFrames=%u\r\n"
            "ExpectedOutputWidth=%u\r\nExpectedOutputHeight=%u\r\n"
            "MP4NativeWidth=%u\r\nMP4NativeHeight=%u\r\n"
            "MP4TargetVisibleWidth=%u\r\nMP4TargetVisibleHeight=%u\r\n"
            "MP4DisplayApertureKind=%u\r\nMP4CropOffsetX=%u\r\nMP4CropOffsetY=%u\r\n"
            "MP4RGBSourceWidth=%u\r\nMP4RGBSourceHeight=%u\r\n"
            "MP4FrameRate=%u/%u\r\nMP4PresentationDuration100ns=%llu\r\n"
            "MP4FormatValidation=%s\r\n"
            "CreateTexture=%s\r\n"
            "IntroClockHook=%s\r\nCareerClockHook=%s\r\nDrawHook=%s\r\n"
            "OutputFrameBytes=%u\r\nStreamingRingSlots=%u\r\nInitialSyncFrames=%u\r\nAsyncOnly=yes\r\n"
            "StartupPrewarmStateAtStart=%ld\r\nStartupPrewarmUsed=%s\r\nStartupPrewarmFrames=%ld\r\n"
            "StartupPrewarmHRESULT=0x%08lX\r\nDecoderSkipPrefix=%ld\r\n"
            "RuntimeActive=%s\r\nCurrentMADSerial=%ld\r\nMADFrameClockHits=%ld\r\nExcessFrameEvents=%ld\r\n"
            "MP4DecodedSamples=%ld\r\nMP4DecodedDiscardedAsLate=%ld\r\nMP4DecodeFailures=%ld\r\n"
            "MP4DecoderHRESULT=0x%08lX\r\nMP4EndOfStream=%s\r\n"
            "MP4MediaTypeChanges=%ld\r\nMP4MediaTypeRenegotiations=%ld\r\n"
            "MP4CurrentSubtypeData1=0x%08X\r\nMP4OutputStride=%ld\r\n"
            "FirstMP4ReadySerial=%ld\r\nLastMP4ReadySerial=%ld\r\n"
            "MatchingNativeDraws=%ld\r\nExactMP4FrameHits=%ld\r\n"
            "ExactFrameMissesFallbackToMAD=%ld\r\nFallbackDraws=%ld\r\n"
            "TextureUploadFailures=%ld\r\nSetTextureFailures=%ld\r\nRestoreTextureFailures=%ld\r\n"
            "VertexAdjustedDraws=%ld\r\nRendererSubstitution=%s\r\n"
            "CanonicalPromotion=%s\r\nFinalState=%s\r\n",
            kMp4FrameworkBuild,
            g_RuntimeIniPath[0] != '\0' ? g_RuntimeIniPath : "<none>",
            RuntimeModeName(), movie != nullptr ? movie->stem : "none", g_RuntimeGeneration,
            g_RuntimeMp4Path,
            g_RuntimeClock == RuntimeClockClass::IntroCommit ? "004132F6-intro" :
            (g_RuntimeClock == RuntimeClockClass::CareerPostUpload ? "005A48FC-post-upload" : "none"),
            movie != nullptr ? movie->frames : 0,
            g_RuntimeOutputWidth, g_RuntimeOutputHeight,
            g_Mp4NativeWidth, g_Mp4NativeHeight,
            g_Mp4NativeGeometry.visibleWidth, g_Mp4NativeGeometry.visibleHeight,
            g_Mp4NativeGeometry.apertureKind,
            g_Mp4NativeGeometry.cropX, g_Mp4NativeGeometry.cropY,
            g_Mp4CopyLayout.sourceWidth, g_Mp4CopyLayout.sourceHeight,
            g_Mp4FrameRateNumerator, g_Mp4FrameRateDenominator,
            static_cast<unsigned long long>(g_Mp4PresentationDuration100ns),
            g_RuntimeHeaderValid ? "PASS" : "FAIL",
            g_RuntimeCreateTexturePass ? "PASS" : "FAIL",
            g_RuntimeIntroHookInstalled ? "PASS" : "FAIL",
            g_RuntimeCareerHookInstalled ? "PASS" : "FAIL",
            g_RuntimeDrawHookInstalled ? "PASS" : "FAIL",
            g_RuntimeOutputFrameBytes, g_RuntimeRingSlots, g_RuntimeInitialSyncFrames,
            InterlockedCompareExchange(&g_Mp4PrewarmStateAtStart, 0, 0),
            InterlockedCompareExchange(&g_Mp4StartupPrewarmUsed, 0, 0) != 0 ? "yes" : "no",
            InterlockedCompareExchange(&g_Mp4StartupPrewarmFrames, 0, 0),
            static_cast<unsigned long>(InterlockedCompareExchange(&g_Mp4PrewarmHrAtStart, 0, 0)),
            InterlockedCompareExchange(&g_Mp4DecoderSkipPrefix, 0, 0),
            active != 0 ? "yes" : "no", currentSerial, clockHits, excess,
            decoded, discarded, decodeFailures,
            static_cast<unsigned long>(decoderHr), decoderEos != 0 ? "yes" : "no",
            mediaTypeChanges, mediaTypeRenegotiations,
            g_Mp4CurrentSubtypeData1, g_Mp4OutputStride,
            firstReady, lastReady,
            matchingDraws, exactHits, distinctMisses, fallbacks,
            uploadFailures, setFailures, restoreFailures, vertexDraws,
            (exactHits > 0 && uploadFailures == 0 && setFailures == 0) ? "PASS" : "not-yet/FAIL",
            canonicalPromotion != nullptr ? canonicalPromotion : "NO",
            finalState != nullptr ? finalState : "unknown");

        bool ok = false;
        if (n > 0)
            ok = WriteAll(log, report, static_cast<DWORD>(n));
        CloseHandle(log);
        return ok;
    }

    void WriteRuntimeLog(const char* finalState, const bool promoteCanonical)
    {
        if (!kRuntimeReportsEnabled)
            return;

        // 1080p is a fresh tier validation and reports all 20 movies. 4K keeps
        // the Stage 2.5 policy of reporting only the 15 newly enabled movies.
        // Canonical promotion still requires a full exact frame-clock session.
        if (!IsStage25ReportTarget(g_RuntimeMovie))
            return;

        const char* promotion = promoteCanonical ? "PASS" : "NO";
        WriteRuntimeLogToPath(g_RuntimeLogPath, finalState, promotion);
        if (promoteCanonical && g_RuntimeCanonicalLogPath[0] != '\0')
        {
            // Write the canonical report independently; a later partial/restarted
            // ReplayMovies generation can never overwrite it because only a full
            // exact frame-clock session reaches this branch.
            WriteRuntimeLogToPath(g_RuntimeCanonicalLogPath, finalState, "PASS");
        }
    }

    void FinalizeRuntime(const char* finalState)
    {
        if (InterlockedCompareExchange(&g_RuntimeActive, 0, 0) == 0 &&
            g_RuntimeReaderThread == nullptr && g_RuntimeReplacementTexture == nullptr)
        {
            g_RuntimeMovie = nullptr;
            g_RuntimeClock = RuntimeClockClass::None;
            return;
        }
        InterlockedExchange(&g_RuntimeActive, 0);
        if (g_RuntimeReaderStopEvent != nullptr)
            SetEvent(g_RuntimeReaderStopEvent);
        if (g_RuntimeReaderThread != nullptr)
            WaitForSingleObject(g_RuntimeReaderThread, INFINITE);

        // Frame-specific fallback (late/not-yet-ready) is normal and must not
        // trigger a dialog. Persistent texture-path failures are not normal.
        if (InterlockedCompareExchange(&g_RuntimeTextureUploadFailures, 0, 0) > 0 ||
            InterlockedCompareExchange(&g_RuntimeSetTextureFailures, 0, 0) > 0 ||
            InterlockedCompareExchange(&g_RuntimeRestoreTextureFailures, 0, 0) > 0)
        {
            NotifyRuntimeError(kErrorGraphics,
                g_RuntimeMovie != nullptr ? g_RuntimeMovie->stem : "unknown",
                "D3D9 MP4 texture upload, binding or restoration failed", E_FAIL);
        }
        if (kRuntimeReportsEnabled)
            WriteRuntimeLog(finalState, RuntimeSessionIsCanonicalPass(finalState));
        CloseRuntimeObjects();
        g_RuntimeMovie = nullptr;
        g_RuntimeClock = RuntimeClockClass::None;
    }

    bool CreateRuntimeReplacementTexture()
    {
        void* const device = *reinterpret_cast<void* volatile*>(VA(kD3DDeviceGlobal));
        if (device == nullptr)
            return false;
        const auto createTexture = D3DMethod<DeviceCreateTextureFn>(device, kDeviceCreateTextureIndex);
        if (createTexture == nullptr || g_RuntimeOutputWidth == 0 || g_RuntimeOutputHeight == 0)
            return false;
        void* texture = nullptr;
        const HRESULT hr = createTexture(device, g_RuntimeOutputWidth, g_RuntimeOutputHeight, 1, 0,
            kD3DFormatA8R8G8B8, kD3DPoolManaged, &texture, nullptr);
        if (FAILED(hr) || texture == nullptr)
            return false;
        g_RuntimeReplacementTexture = texture;
        g_RuntimeCreateTexturePass = true;
        return true;
    }

    bool BeginRuntime(const MovieSpec& movie)
    {
        FinalizeRuntime("restarted");
        ResetRuntimeCounters();
        g_RuntimeMovie = &movie;
        g_RuntimeClock = movie.clock;
        g_RuntimeGeneration = g_FMVFix_MovieGeneration;
        g_RuntimeHeaderValid = false;
        g_RuntimeCreateTexturePass = false;
        g_RuntimeLogPath[0] = '\0';
        g_RuntimeCanonicalLogPath[0] = '\0';

        // Stage 2.5 enables the complete audited 20-movie catalog. A missing
        // or invalid selected-tier MP4 fails open to the native MAD picture.
        if (g_RuntimeMode == RuntimeMode::Xbox || !IsStage25RuntimeTarget(movie))
        {
            g_RuntimeMovie = nullptr;
            g_RuntimeClock = RuntimeClockClass::None;
            return false;
        }

        if (!BuildRuntimePaths(movie))
        {
            NotifyRuntimeError(kErrorPaths, movie.stem, "Cannot build MP4 input path", E_INVALIDARG);
            g_RuntimeMovie = nullptr;
            g_RuntimeClock = RuntimeClockClass::None;
            return false;
        }

        if (GetFileAttributesA(g_RuntimeMp4Path) == INVALID_FILE_ATTRIBUTES)
        {
            const HRESULT fileHr = HRESULT_FROM_WIN32(GetLastError());
            NotifyRuntimeError(kErrorMissingMp4, movie.stem, "Selected-tier MP4 file missing or inaccessible", fileHr);
            WriteRuntimeLog("mp4-open-failed/native-fallback", false);
            g_RuntimeMovie = nullptr;
            g_RuntimeClock = RuntimeClockClass::None;
            return false;
        }

        g_RuntimeOutputWidth = g_RuntimeMode == RuntimeMode::P4K ? 3840u : 2160u;
        g_RuntimeOutputHeight = movie.sourceWidth == movie.sourceHeight
            ? g_RuntimeOutputWidth
            : (g_RuntimeMode == RuntimeMode::P4K ? 1920u : 1080u);
        const std::uint64_t frameBytes = static_cast<std::uint64_t>(g_RuntimeOutputWidth) *
            static_cast<std::uint64_t>(g_RuntimeOutputHeight) * 4ull;
        if (frameBytes == 0 || frameBytes > kRuntimeMaxOutputFrameBytes)
        {
            NotifyRuntimeError(kErrorPaths, movie.stem, "Replacement video dimensions are invalid", E_INVALIDARG);
            WriteRuntimeLog("mp4-output-geometry-invalid/native-fallback", false);
            g_RuntimeMovie = nullptr;
            g_RuntimeClock = RuntimeClockClass::None;
            return false;
        }
        g_RuntimeOutputFrameBytes = static_cast<std::uint32_t>(frameBytes);

        g_RuntimeInitialSyncFrames = 0; // no StartMovie/render-thread decoding

        const LONG prewarmState = InterlockedCompareExchange(&g_Mp4PrewarmState, 0, 0);
        const LONG prewarmReady = InterlockedCompareExchange(&g_Mp4PrewarmReadyFrames, 0, 0);
        const LONG prewarmHr = InterlockedCompareExchange(&g_Mp4PrewarmHr, 0, 0);
        InterlockedExchange(&g_Mp4PrewarmStateAtStart, prewarmState);
        InterlockedExchange(&g_Mp4PrewarmHrAtStart, prewarmHr);

        const bool canAdoptPrewarm =
            prewarmState == 2 &&
            prewarmReady == static_cast<LONG>(kMp4StartupPrewarmFrames) &&
            g_Mp4PrewarmRing != nullptr &&
            g_Mp4PrewarmMode == g_RuntimeMode &&
            g_Mp4PrewarmWidth == g_RuntimeOutputWidth &&
            g_Mp4PrewarmHeight == g_RuntimeOutputHeight &&
            g_Mp4PrewarmFrameBytes == g_RuntimeOutputFrameBytes &&
            EqualsAsciiInsensitive(g_Mp4PrewarmPath, g_RuntimeMp4Path);

        if (canAdoptPrewarm)
        {
            g_RuntimeRing = g_Mp4PrewarmRing;
            g_Mp4PrewarmRing = nullptr;
            g_RuntimeRingSlots = g_Mp4PrewarmRingSlots;
            for (LONG i = 0; i < prewarmReady; ++i)
            {
                g_RuntimeRingSerial[i] = i;
                InterlockedExchange(&g_RuntimeRingState[i], 1);
            }
            InterlockedExchange(&g_Mp4StartupPrewarmUsed, 1);
            InterlockedExchange(&g_Mp4StartupPrewarmFrames, prewarmReady);
            InterlockedExchange(&g_Mp4DecoderSkipPrefix, prewarmReady);
            InterlockedExchange(&g_Mp4FirstReadySerial, 0);
            InterlockedExchange(&g_Mp4LastReadySerial, prewarmReady - 1);
            InterlockedExchange(&g_Mp4PrewarmState, 4);

            if (g_Mp4PrewarmThread != nullptr &&
                WaitForSingleObject(g_Mp4PrewarmThread, 0) == WAIT_OBJECT_0)
            {
                CloseHandle(g_Mp4PrewarmThread);
                g_Mp4PrewarmThread = nullptr;
            }
        }
        else
        {
            std::uint64_t slots = kRuntimeRingBudgetBytes / g_RuntimeOutputFrameBytes;
            if (slots < kRuntimeMinRingSlots) slots = kRuntimeMinRingSlots;
            if (slots > kRuntimeMaxRingSlots) slots = kRuntimeMaxRingSlots;
            g_RuntimeRingSlots = static_cast<std::uint32_t>(slots);

            const std::uint64_t ringBytes = static_cast<std::uint64_t>(g_RuntimeRingSlots) * g_RuntimeOutputFrameBytes;
            if (ringBytes == 0 || ringBytes > static_cast<std::uint64_t>((std::numeric_limits<SIZE_T>::max)()))
            {
                NotifyRuntimeError(kErrorAllocation, movie.stem, "Decoder ring size cannot be represented", E_OUTOFMEMORY);
                WriteRuntimeLog("ring-size-invalid/native-fallback", false);
                g_RuntimeMovie = nullptr;
                g_RuntimeClock = RuntimeClockClass::None;
                return false;
            }

            g_RuntimeRing = static_cast<BYTE*>(VirtualAlloc(nullptr, static_cast<SIZE_T>(ringBytes),
                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        }

        g_RuntimeReaderStopEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        if (g_RuntimeRing == nullptr || g_RuntimeReaderStopEvent == nullptr)
        {
            NotifyRuntimeError(kErrorAllocation, movie.stem, "Cannot allocate decoder ring or stop event", E_OUTOFMEMORY);
            WriteRuntimeLog("runtime-allocation-failed/native-fallback", false);
            CloseRuntimeObjects();
            g_RuntimeMovie = nullptr;
            g_RuntimeClock = RuntimeClockClass::None;
            return false;
        }

        if (!CreateRuntimeReplacementTexture())
        {
            NotifyRuntimeError(kErrorGraphics, movie.stem, "D3D9 replacement texture creation failed", E_FAIL);
            WriteRuntimeLog("replacement-texture-create-failed/native-fallback", false);
            CloseRuntimeObjects();
            g_RuntimeMovie = nullptr;
            g_RuntimeClock = RuntimeClockClass::None;
            return false;
        }

        // The decoder thread performs the Media Foundation/native media-type
        // validation before publishing its first replacement frame.
        g_RuntimeHeaderValid = false;

        g_RuntimeReaderThread = CreateThread(nullptr, 0, RuntimeReaderThread, nullptr, 0, nullptr);
        if (g_RuntimeReaderThread == nullptr)
        {
            NotifyRuntimeError(kErrorDecoder, movie.stem, "Cannot start MP4 decoder thread", HRESULT_FROM_WIN32(GetLastError()));
            WriteRuntimeLog("mp4-decoder-thread-failed/native-fallback", false);
            CloseRuntimeObjects();
            g_RuntimeMovie = nullptr;
            g_RuntimeClock = RuntimeClockClass::None;
            return false;
        }

        InterlockedExchange(&g_RuntimeActive, 1);
        WriteRuntimeLog("runtime-armed", false);
        return true;
    }

    BYTE* GetReadyRuntimeFrame(const LONG serial)
    {
        const MovieSpec* const movie = g_RuntimeMovie;
        if (movie == nullptr || serial < 0 || serial >= static_cast<LONG>(movie->frames) ||
            g_RuntimeRing == nullptr || g_RuntimeRingSlots == 0 || g_RuntimeOutputFrameBytes == 0)
        {
            return nullptr;
        }
        const std::uint32_t slot = static_cast<std::uint32_t>(serial) % g_RuntimeRingSlots;
        MemoryBarrier();
        if (InterlockedCompareExchange(&g_RuntimeRingState[slot], 0, 0) == 1 &&
            g_RuntimeRingSerial[slot] == serial)
        {
            return g_RuntimeRing + static_cast<std::size_t>(slot) * g_RuntimeOutputFrameBytes;
        }
        return nullptr;
    }

    bool UploadRuntimeFrame(void* texture, const BYTE* frame)
    {
        if (texture == nullptr || frame == nullptr)
            return false;
        const auto lockRect = D3DMethod<TextureLockRectFn>(texture, kTextureLockRectIndex);
        const auto unlockRect = D3DMethod<TextureUnlockRectFn>(texture, kTextureUnlockRectIndex);
        if (lockRect == nullptr || unlockRect == nullptr || g_RuntimeOutputWidth == 0 || g_RuntimeOutputHeight == 0)
            return false;
        D3DLockedRectCompat locked = {};
        if (FAILED(lockRect(texture, 0, &locked, nullptr, 0)) || locked.pBits == nullptr ||
            locked.Pitch < static_cast<INT>(g_RuntimeOutputWidth * 4))
        {
            return false;
        }
        const std::size_t rowBytes = static_cast<std::size_t>(g_RuntimeOutputWidth) * 4;
        for (std::uint32_t y = 0; y < g_RuntimeOutputHeight; ++y)
        {
            std::memcpy(static_cast<BYTE*>(locked.pBits) + static_cast<std::size_t>(y) * locked.Pitch,
                frame + static_cast<std::size_t>(y) * rowBytes, rowBytes);
        }
        return SUCCEEDED(unlockRect(texture, 0));
    }

    void AdjustRuntimeUVs(BYTE* vertices, const UINT stride)
    {
        if (vertices == nullptr || stride != 0x20 || g_RuntimeOutputWidth == 0 || g_RuntimeOutputHeight == 0)
            return;
        const float u0 = 0.5f / static_cast<float>(g_RuntimeOutputWidth);
        const float u1 = (static_cast<float>(g_RuntimeOutputWidth) - 0.5f) / static_cast<float>(g_RuntimeOutputWidth);
        const float v0 = 0.5f / static_cast<float>(g_RuntimeOutputHeight);
        const float v1 = (static_cast<float>(g_RuntimeOutputHeight) - 0.5f) / static_cast<float>(g_RuntimeOutputHeight);
        *reinterpret_cast<float*>(vertices + 0 * stride + 0x10) = u0;
        *reinterpret_cast<float*>(vertices + 0 * stride + 0x14) = v0;
        *reinterpret_cast<float*>(vertices + 1 * stride + 0x10) = u1;
        *reinterpret_cast<float*>(vertices + 1 * stride + 0x14) = v0;
        *reinterpret_cast<float*>(vertices + 2 * stride + 0x10) = u1;
        *reinterpret_cast<float*>(vertices + 2 * stride + 0x14) = v1;
        *reinterpret_cast<float*>(vertices + 3 * stride + 0x10) = u0;
        *reinterpret_cast<float*>(vertices + 3 * stride + 0x14) = v1;
    }

    bool VerifyRuntimeSites()
    {
        const std::uintptr_t introSite = VA(kIntroFrameCommitHookSite);
        static constexpr BYTE postCall[5] = { 0xE8, 0x6F, 0xE9, 0xE6, 0xFF };
        static constexpr BYTE drawCall[6] = { 0xFF, 0x91, 0x4C, 0x01, 0x00, 0x00 };
        const bool introOk = *reinterpret_cast<const BYTE*>(introSite) == 0xA1 &&
            *reinterpret_cast<const std::uint32_t*>(introSite + 1) == static_cast<std::uint32_t>(VA(kGpuMovieTextureGlobal));
        return introOk && MatchBytes(VA(kPostUploadCallSite), postCall, sizeof(postCall)) &&
            MatchBytes(VA(kDrawPrimitiveUPCallSite), drawCall, sizeof(drawCall));
    }

    bool WriteRelativeCall6(const std::uintptr_t site, const std::uintptr_t target)
    {
        std::int32_t rel = 0;
        if (!MakeRel32(site, target, rel))
            return false;
        BYTE patch[6] = { 0xE8, 0, 0, 0, 0, 0x90 };
        std::memcpy(patch + 1, &rel, sizeof(rel));
        return WriteBytes(site, patch, sizeof(patch));
    }

    bool VerifyThirteenAGCroppedGeometry()
    {
        // This is deliberately strict. If WidescreenFix has not yet published
        // its Cropped geometry, FMVFix does nothing rather than converting the
        // renderer back to native 4:3.
        return
            *reinterpret_cast<volatile std::uint32_t*>(
                VA(kMovieQuadBottomImmediate)) == kCroppedBottom &&
            *reinterpret_cast<volatile std::uint32_t*>(
                VA(kMovieQuadRightImmediate)) == kCroppedRight &&
            *reinterpret_cast<volatile std::uint32_t*>(
                VA(kMovieQuadTopImmediate)) == kCroppedTop &&
            *reinterpret_cast<volatile std::uint32_t*>(
                VA(kMovieQuadLeftImmediate)) == kCroppedLeft;
    }

    bool VerifyMovieQuadSiteUnhooked()
    {
        static constexpr BYTE firstPush[5] =
        {
            0x68, 0x00, 0x00, 0x80, 0x3F
        };

        return
            MatchBytes(
                VA(kMovieQuadHookSite),
                firstPush,
                sizeof(firstPush)) &&
            *reinterpret_cast<const BYTE*>(VA(0x004F1FC3)) == 0x68 &&
            *reinterpret_cast<const BYTE*>(VA(0x004F1FC8)) == 0x68 &&
            *reinterpret_cast<const BYTE*>(VA(0x004F1FCD)) == 0x68 &&
            *reinterpret_cast<const BYTE*>(VA(0x004F1FD2)) == 0x68 &&
            *reinterpret_cast<const BYTE*>(VA(0x004F1FD7)) == 0x50 &&
            *reinterpret_cast<const BYTE*>(VA(0x004F1FD8)) == 0xE8;
    }

    bool RestoreMovieQuadSite()
    {
        static constexpr BYTE firstPush[5] =
        {
            0x68, 0x00, 0x00, 0x80, 0x3F
        };

        if (!g_NaBootRendererHookInstalled)
            return true;

        if (!WriteBytes(
            VA(kMovieQuadHookSite),
            firstPush,
            sizeof(firstPush)))
        {
            return false;
        }

        g_NaBootRendererHookInstalled = false;
        return true;
    }

    void StopNaBootFraming()
    {
        // Restore the exact Cropped B/T pair captured after ThirteenAG had
        // initialized, then remove our renderer detour entirely.
        if (g_NaBootBaselineValid)
        {
            WriteMovieVerticalGeometry(
                g_NaBootBaselineBottom,
                g_NaBootBaselineTop);
        }

        InterlockedExchange(&g_NaBootFramingActive, 0);
        g_NaBootGeometryMode = 0;
        g_NaBootBaselineValid = false;

        RestoreMovieQuadSite();
    }

    bool WriteRelativeJump8(
        const std::uintptr_t site,
        const std::uintptr_t target)
    {
        std::int32_t rel = 0;
        if (!MakeRel32(site, target, rel))
            return false;

        BYTE patch[8] =
        {
            0xE9, 0, 0, 0, 0,
            0x90, 0x90, 0x90
        };

        std::memcpy(patch + 1, &rel, sizeof(rel));

        return WriteBytes(site, patch, sizeof(patch));
    }

    void PublishXboxCapability()
    {
        // Frozen publication contract:
        //   00713DBC = BYTE COUNT
        //   00713DBE = WORD C0
        //   00713DC0 = WORD C1
        //
        // Publish candidate 1 completely first, then publish COUNT=2 last.
        auto* count = reinterpret_cast<volatile std::uint8_t*>(
            VA(kCapabilityCount));
        auto* c0 = reinterpret_cast<volatile std::uint16_t*>(
            VA(kCapabilityC0));
        auto* c1 = reinterpret_cast<volatile std::uint16_t*>(
            VA(kCapabilityC1));

        if (*count == 1 && *c0 == 0x0024)
        {
            *c1 = 0x0420;
            MemoryBarrier();
            *count = 2;
        }
    }

    bool ArmNaBootFraming()
    {
        // StartMovie occurs after ASI initialization. Delay touching the movie
        // quad until this exact NA_BOOT transaction so ThirteenAG's initial
        // signature scan/patch is already complete.
        if (!VerifyMovieQuadSiteUnhooked())
            return false;

        if (!VerifyThirteenAGCroppedGeometry())
            return false;

        g_NaBootBaselineBottom =
            *reinterpret_cast<volatile std::uint32_t*>(
                VA(kMovieQuadBottomImmediate));
        g_NaBootBaselineTop =
            *reinterpret_cast<volatile std::uint32_t*>(
                VA(kMovieQuadTopImmediate));

        g_NaBootBaselineValid = true;

        // EA begins too early for a P44=16 transition to be visually hidden.
        // We are already inside the exact NA_BOOT StartMovie transaction and
        // have just verified ThirteenAG Cropped is live, so enter the special
        // vertical Stretched geometry immediately while the movie is still in
        // its initial black startup. The render hook then has only three
        // visible-state transitions left to perform: 106, 181 and 346.
        if (!WriteMovieVerticalGeometry(
            kStretchedBottom,
            kStretchedTop))
        {
            g_NaBootBaselineValid = false;
            return false;
        }

        g_NaBootGeometryMode = 1;

        g_FMVFix_ReturnMovieQuad =
            static_cast<DWORD>(VA(kMovieQuadContinue));

        if (!WriteRelativeJump5(
            VA(kMovieQuadHookSite),
            reinterpret_cast<std::uintptr_t>(&FMVFix_HookMovieQuad)))
        {
            WriteMovieVerticalGeometry(
                g_NaBootBaselineBottom,
                g_NaBootBaselineTop);
            g_NaBootGeometryMode = 0;
            g_NaBootBaselineValid = false;
            return false;
        }

        g_NaBootRendererHookInstalled = true;
        InterlockedExchange(&g_NaBootFramingActive, 1);
        return true;
    }

    bool VerifyGameCode()
    {
        // Verify only the native downstream sites that must still contain the
        // expected PC instructions. The StartMovie site is installed separately
        // because another observer may already occupy 005A4ABD.

        static constexpr BYTE call651A00[5] =
        {
            0xE8, 0xF2, 0xC7, 0xFF, 0xFF
        };

        static constexpr BYTE call654E20[5] =
        {
            0xE8, 0x6E, 0x39, 0x00, 0x00
        };

        // Live/frozen layout:
        // 006518B9 E8 D2 5D 00 00  -> call 00657690
        static constexpr BYTE call657690[5] =
        {
            0xE8, 0xD2, 0x5D, 0x00, 0x00
        };

        static constexpr BYTE classGate[3] =
        {
            0xF6, 0xC1, 0x04
        };

        static constexpr BYTE matrixGate[3] =
        {
            0xF6, 0xC1, 0x04
        };

        static constexpr BYTE runtimeGate[4] =
        {
            0xF6, 0x47, 0x24, 0x44
        };

        return
            MatchBytes(
                VA(kHook651A00Call),
                call651A00,
                sizeof(call651A00)) &&
            MatchBytes(
                VA(kHook654E20Call),
                call654E20,
                sizeof(call654E20)) &&
            MatchBytes(
                VA(kHook657690Call),
                call657690,
                sizeof(call657690)) &&
            MatchBytes(
                VA(0x006517A9),
                classGate,
                sizeof(classGate)) &&
            MatchBytes(
                VA(0x0065131C),
                matrixGate,
                sizeof(matrixGate)) &&
            MatchBytes(
                VA(0x0065141A),
                runtimeGate,
                sizeof(runtimeGate));
    }
}

// ============================================================================
// StartMovie boundary
// ============================================================================

extern "C" void __cdecl FMVFix_OnStartMoviePath(const char* moviePath)
{
    PublishXboxCapability();

    // Next StartMovie is the hard close/report boundary for the preceding SR
    // session. This does not alter native MAD teardown/timing.
    FinalizeRuntime("next-StartMovie");

    if (g_NaBootRendererHookInstalled ||
        InterlockedCompareExchange(&g_NaBootFramingActive, 0, 0) != 0 ||
        g_NaBootBaselineValid)
    {
        StopNaBootFraming();
    }

    if (IsNaBootMoviePath(moviePath))
        ArmNaBootFraming();

    if (g_RuntimeMode != RuntimeMode::Xbox)
    {
        const MovieSpec* const movie = FindMovieSpec(moviePath);
        if (movie != nullptr)
        {
            BeginRuntime(*movie);
            if (IsStage25RuntimeTarget(*movie))
                StartStage25NextPrewarm(*movie);
        }
    }
}

extern "C" __declspec(naked) void FMVFix_HookStartMovieCapability()
{
    __asm
    {
        // Proven StartMovie stack contract at 005A4ABD:
        //   original [ESP+04] = resolved movie pathname
        //   original [ESP+08] = activation mode
        //   original [ESP+0C] = movie metadata/id
        //
        // Preserve exactly EFLAGS + EAX. After these pushes, [ESP+0C]
        // maps back to the original pathname argument.

        pushfd
        push eax

        // Retained frozen movie-generation ownership point.
        inc dword ptr[g_FMVFix_MovieGeneration]

        mov eax, dword ptr[esp + 0Ch]
        push eax
        call FMVFix_OnStartMoviePath
        add esp, 4

        pop eax
        popfd

        // Replay the two stolen native instructions exactly.
        mov ecx, dword ptr[esp + 0Ch]
        mov edx, dword ptr[esp + 08h]

        push dword ptr[g_FMVFix_ReturnStartMovieCapability]
        ret
    }
}

// ============================================================================
// NA_BOOT EA / THX framing
// ============================================================================

extern "C" void __cdecl FMVFix_OnMovieQuad()
{
    if (InterlockedCompareExchange(&g_NaBootFramingActive, 0, 0) == 0 || !g_NaBootBaselineValid)
        return;

    const auto movieObject =
        *reinterpret_cast<volatile std::uintptr_t*>(VA(kActiveMovieObject));

    if (movieObject == 0)
        return;

    const auto r28 =
        *reinterpret_cast<volatile std::uintptr_t*>(movieObject + 0x28);

    if (r28 == 0)
        return;

    const std::uint32_t p44 =
        *reinterpret_cast<volatile std::uint32_t*>(r28 + 0x44);

    // Proven manual schedule, with the EA entry moved to StartMovie so the
    // user never sees the Cropped -> Stretched transition on the EA logo:
    //   StartMovie(NA_BOOT) -> special Stretched during initial black startup
    //   106                 -> ThirteenAG Cropped
    //   181                 -> special Stretched
    //   346                 -> ThirteenAG Cropped, then terminate/unhook
    if (p44 >= kNaBootTHXEnd)
    {
        StopNaBootFraming();
        return;
    }

    const bool stretched =
        (p44 < kNaBootEAEnd) ||
        (p44 >= kNaBootTHXStart && p44 < kNaBootTHXEnd);

    const LONG desiredMode = stretched ? 1 : 0;
    if (desiredMode == g_NaBootGeometryMode)
        return;

    const std::uint32_t bottom =
        stretched ? kStretchedBottom : g_NaBootBaselineBottom;
    const std::uint32_t top =
        stretched ? kStretchedTop : g_NaBootBaselineTop;

    if (WriteMovieVerticalGeometry(bottom, top))
        g_NaBootGeometryMode = desiredMode;
}

extern "C" __declspec(naked) void FMVFix_HookMovieQuad()
{
    __asm
    {
        pushfd
        pushad
        call FMVFix_OnMovieQuad
        popad
        popfd

        // Replay the one stolen native instruction.
        push 3F800000h

        push dword ptr[g_FMVFix_ReturnMovieQuad]
        ret
    }
}

// ============================================================================
// Exact decoded-frame clocks + final-draw substitution
// ============================================================================

extern "C" void __cdecl FMVFix_OnIntroFrameCommit()
{
    if (InterlockedCompareExchange(&g_RuntimeActive, 0, 0) == 0 ||
        g_RuntimeClock != RuntimeClockClass::IntroCommit || g_RuntimeMovie == nullptr)
    {
        return;
    }
    const LONG serial = InterlockedIncrement(&g_RuntimeFrameClockHits) - 1;
    if (serial >= static_cast<LONG>(g_RuntimeMovie->frames))
    {
        InterlockedIncrement(&g_RuntimeExcessFrameEvents);
        return;
    }
    InterlockedExchange(&g_RuntimeCurrentSerial, serial);
}

extern "C" __declspec(naked) void FMVFix_HookIntroFrameCommit()
{
    __asm
    {
        pushfd
        pushad
        call FMVFix_OnIntroFrameCommit
        popad
        popfd

        // Replay 004132F6: mov eax,[0078A494]
        mov eax, dword ptr[g_FMVFix_GpuMovieTextureSlot]
        mov eax, dword ptr[eax]

        push dword ptr[g_FMVFix_ReturnIntroFrameCommit]
        ret
    }
}

extern "C" void __cdecl FMVFix_OnCareerPostUpload()
{
    if (InterlockedCompareExchange(&g_RuntimeActive, 0, 0) == 0 ||
        g_RuntimeClock != RuntimeClockClass::CareerPostUpload || g_RuntimeMovie == nullptr)
    {
        return;
    }
    const LONG serial = InterlockedIncrement(&g_RuntimeFrameClockHits) - 1;
    if (serial >= static_cast<LONG>(g_RuntimeMovie->frames))
    {
        InterlockedIncrement(&g_RuntimeExcessFrameEvents);
        return;
    }
    InterlockedExchange(&g_RuntimeCurrentSerial, serial);
}

extern "C" __declspec(naked) void FMVFix_HookPostUploadRuntime()
{
    __asm
    {
        // Exact proven 005A48FC wrapper shape: copy the original three cdecl
        // arguments, call untouched native 00413270, preserve complete returned
        // GPR/EFLAGS state around our post-return serial update, RET to 005A4901
        // with the ORIGINAL caller arguments still present for caller cleanup.
        push dword ptr[esp + 0Ch]
        push dword ptr[esp + 0Ch]
        push dword ptr[esp + 0Ch]
        call dword ptr[g_FMVFix_OrigMovieUpload]
        add esp, 0Ch

        pushfd
        pushad
        call FMVFix_OnCareerPostUpload
        popad
        popfd
        ret
    }
}

extern "C" HRESULT __stdcall FMVFix_DrawPrimitiveUPProxy(
    void* device,
    DWORD primitiveType,
    UINT primitiveCount,
    const void* vertexData,
    UINT stride)
{
    const auto draw = D3DMethod<DeviceDrawPrimitiveUPFn>(device, kDeviceDrawPrimitiveUPIndex);
    if (draw == nullptr)
        return E_FAIL;

    if (InterlockedCompareExchange(&g_RuntimeActive, 0, 0) == 0 ||
        primitiveType != 6 || primitiveCount != 2 || stride != 0x20 ||
        vertexData == nullptr || g_RuntimeReplacementTexture == nullptr || g_RuntimeMovie == nullptr)
    {
        return draw(device, primitiveType, primitiveCount, vertexData, stride);
    }

    const LONG serial = InterlockedCompareExchange(&g_RuntimeCurrentSerial, 0, 0);
    if (serial < 0 || serial >= static_cast<LONG>(g_RuntimeMovie->frames))
        return draw(device, primitiveType, primitiveCount, vertexData, stride);

    const auto getTexture = D3DMethod<DeviceGetTextureFn>(device, kDeviceGetTextureIndex);
    const auto setTexture = D3DMethod<DeviceSetTextureFn>(device, kDeviceSetTextureIndex);
    if (getTexture == nullptr || setTexture == nullptr)
        return draw(device, primitiveType, primitiveCount, vertexData, stride);

    void* originalTexture = nullptr;
    if (FAILED(getTexture(device, 0, &originalTexture)) || originalTexture == nullptr)
        return draw(device, primitiveType, primitiveCount, vertexData, stride);

    void* const nativeMovieTexture = *reinterpret_cast<void* volatile*>(VA(kGpuMovieTextureGlobal));
    if (originalTexture != nativeMovieTexture)
    {
        void* releaseTarget = originalTexture;
        ReleaseComObject(releaseTarget);
        return draw(device, primitiveType, primitiveCount, vertexData, stride);
    }

    InterlockedIncrement(&g_RuntimeMatchingDraws);
    BYTE* const frame = GetReadyRuntimeFrame(serial);
    if (frame == nullptr)
    {
        InterlockedIncrement(&g_RuntimeFallbackDraws);
        const LONG previousMiss = InterlockedExchange(&g_RuntimeLastMissSerial, serial);
        if (previousMiss != serial)
            InterlockedIncrement(&g_RuntimeDistinctMisses);
        const HRESULT hr = draw(device, primitiveType, primitiveCount, vertexData, stride);
        void* releaseTarget = originalTexture;
        ReleaseComObject(releaseTarget);
        return hr;
    }

    if (InterlockedCompareExchange(&g_RuntimeUploadedSerial, 0, 0) != serial)
    {
        if (!UploadRuntimeFrame(g_RuntimeReplacementTexture, frame))
        {
            InterlockedIncrement(&g_RuntimeTextureUploadFailures);
            InterlockedIncrement(&g_RuntimeFallbackDraws);
            const HRESULT hr = draw(device, primitiveType, primitiveCount, vertexData, stride);
            void* releaseTarget = originalTexture;
            ReleaseComObject(releaseTarget);
            return hr;
        }
        InterlockedExchange(&g_RuntimeUploadedSerial, serial);
    }

    if (FAILED(setTexture(device, 0, g_RuntimeReplacementTexture)))
    {
        InterlockedIncrement(&g_RuntimeSetTextureFailures);
        const HRESULT hr = draw(device, primitiveType, primitiveCount, vertexData, stride);
        void* releaseTarget = originalTexture;
        ReleaseComObject(releaseTarget);
        return hr;
    }

    BYTE adjusted[4 * 0x20] = {};
    std::memcpy(adjusted, vertexData, sizeof(adjusted));
    AdjustRuntimeUVs(adjusted, stride);
    InterlockedIncrement(&g_RuntimeVertexAdjustedDraws);

    const HRESULT drawHr = draw(device, primitiveType, primitiveCount, adjusted, stride);

    if (FAILED(setTexture(device, 0, originalTexture)))
        InterlockedIncrement(&g_RuntimeRestoreTextureFailures);

    const LONG previousHit = InterlockedExchange(&g_RuntimeLastHitSerial, serial);
    if (previousHit != serial)
        InterlockedIncrement(&g_RuntimeExactFrameHits);

    void* releaseTarget = originalTexture;
    ReleaseComObject(releaseTarget);
    return drawHr;
}

namespace
{
    bool InstallRuntimeHooks()
    {
        if (!VerifyRuntimeSites())
            return false;

        g_FMVFix_ReturnIntroFrameCommit = static_cast<DWORD>(VA(kIntroFrameCommitContinue));
        g_FMVFix_GpuMovieTextureSlot = static_cast<DWORD>(VA(kGpuMovieTextureGlobal));
        g_FMVFix_OrigMovieUpload = static_cast<DWORD>(VA(kNativeMovieUpload));

        if (!WriteRelativeJump5(VA(kIntroFrameCommitHookSite),
            reinterpret_cast<std::uintptr_t>(&FMVFix_HookIntroFrameCommit)))
        {
            return false;
        }
        g_RuntimeIntroHookInstalled = true;

        if (!WriteRelativeCall(VA(kPostUploadCallSite),
            reinterpret_cast<std::uintptr_t>(&FMVFix_HookPostUploadRuntime)))
        {
            return false;
        }
        g_RuntimeCareerHookInstalled = true;

        if (!WriteRelativeCall6(VA(kDrawPrimitiveUPCallSite),
            reinterpret_cast<std::uintptr_t>(&FMVFix_DrawPrimitiveUPProxy)))
        {
            return false;
        }
        g_RuntimeDrawHookInstalled = true;
        return true;
    }
}

// ============================================================================
// Proven downstream adapters
// ============================================================================

extern "C" __declspec(naked) void FMVFix_Hook651A00()
{
    __asm
    {
        // Private argument translation only: 0x420 -> 0x24.
        // The actual movie record remains 0x420.
        pushfd

        cmp dword ptr[esp + 08h], 0420h
        jne Hook651A00_Pass

        mov dword ptr[esp + 08h], 0024h

        Hook651A00_Pass :
        popfd

            push dword ptr[g_FMVFix_Orig651A00]
            ret
    }
}

extern "C" __declspec(naked) void FMVFix_Hook654E20()
{
    __asm
    {
        // Private descriptor lookup key only: 0x420 -> 0x24.
        pushfd

        cmp dword ptr[esp + 0Ch], 0420h
        jne Hook654E20_Pass

        mov dword ptr[esp + 0Ch], 0024h

        Hook654E20_Pass :
        popfd

            push dword ptr[g_FMVFix_Orig654E20]
            ret
    }
}

extern "C" __declspec(naked) void FMVFix_Hook657690()
{
    __asm
    {
        // Exact six-lane Xbox FMV backend adaptation only.
        //
        // CFG+00 WORD = 0xBB80 (48000 Hz)
        // CFG+04      = 6
        // CFG+05      = 0x0A -> 0x08
        // CFG+06      = 3
        // CFG+10 DWORD= 0

        pushfd
        push eax

        // PUSHFD + PUSH EAX shifts original CFG [ESP+08] to [ESP+10].
        mov eax, dword ptr[esp + 10h]

        test eax, eax
        je Hook657690_Done

        // Important: CFG+00 is a WORD. CFG+02 is a separate selector.
        cmp word ptr[eax + 00h], 0BB80h
        jne Hook657690_Done

        cmp byte ptr[eax + 04h], 06h
        jne Hook657690_Done

        cmp byte ptr[eax + 05h], 0Ah
        jne Hook657690_Done

        cmp byte ptr[eax + 06h], 03h
        jne Hook657690_Done

        cmp dword ptr[eax + 10h], 00000000h
        jne Hook657690_Done

        mov byte ptr[eax + 05h], 08h

        Hook657690_Done :
        pop eax
            popfd

            push dword ptr[g_FMVFix_Orig657690]
            ret
    }
}

namespace
{
    bool InstallBridge()
    {
        if (!VerifyGameCode())
            return false;

        g_FMVFix_Orig651A00 =
            static_cast<DWORD>(VA(0x00651A00));

        g_FMVFix_Orig654E20 =
            static_cast<DWORD>(VA(0x00654E20));

        g_FMVFix_Orig657690 =
            static_cast<DWORD>(VA(0x00657690));

        g_FMVFix_ReturnStartMovieCapability =
            static_cast<DWORD>(VA(kStartMovieContinue));

        // Exact proven call-site adapters.
        if (!WriteRelativeCall(
            VA(kHook651A00Call),
            reinterpret_cast<std::uintptr_t>(&FMVFix_Hook651A00)))
        {
            return false;
        }

        if (!WriteRelativeCall(
            VA(kHook654E20Call),
            reinterpret_cast<std::uintptr_t>(&FMVFix_Hook654E20)))
        {
            return false;
        }

        if (!WriteRelativeCall(
            VA(kHook657690Call),
            reinterpret_cast<std::uintptr_t>(&FMVFix_Hook657690)))
        {
            return false;
        }

        // Exact proven widened gates.
        if (!WriteByte(VA(kClassGateImmediate), 0x24))
            return false;

        if (!WriteByte(VA(kMatrixGateImmediate), 0x24))
            return false;

        if (!WriteByte(VA(kRuntimeGateImmediate), 0x64))
            return false;

        // Stable capability-publication boundary.
        if (!WriteRelativeJump8(
            VA(kStartMovieHookSite),
            reinterpret_cast<std::uintptr_t>(
                &FMVFix_HookStartMovieCapability)))
        {
            return false;
        }

        return true;
    }
}

// ============================================================================
// Public initialization
// ============================================================================

bool FMVFix::Initialize(HMODULE selfModule)
{
    g_GameBase = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    if (g_GameBase == 0)
        return false;

    BuildRuntimeIniPath(selfModule);
    LoadRuntimeMode();
    WriteRuntimeStartupManifest();

    if (!InstallBridge())
    {
        NotifyRuntimeError(kErrorInstall, "initialization", "Xbox-PC compatibility hook installation failed", E_FAIL);
        return false;
    }

    // Install both proven frame-clock classes eagerly. In Xbox mode they remain
    // transparent observers because no MP4 runtime session is armed.
    if (!InstallRuntimeHooks())
    {
        NotifyRuntimeError(kErrorInstall, "initialization", "Movie frame clock or renderer hook installation failed", E_FAIL);
        return false;
    }

    // Stage 2.5 starts with the proven NA_BOOT runway. Each catalog StartMovie
    // then prepares the next catalog movie on a worker thread. If a movie is
    // selected out of order or its prewarm is not ready, BeginRuntime fails open
    // to the normal asynchronous decoder and never blocks StartMovie or the
    // render thread.
    if (g_RuntimeMode != RuntimeMode::Xbox)
    {
        const MovieSpec* const naBoot = FindMovieSpecByStem("NA_BOOT");
        if (naBoot != nullptr)
            StartMp4PrewarmForMovie(*naBoot);
    }
    return true;
}