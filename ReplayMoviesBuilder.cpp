#include "ReplayMoviesBuilder.h"
#include "pch.h"

#include "Addresses.h"

#include <Windows.h>
#include <cstdint>
#include <cstring>


extern "C"
{
    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesBuildAttempts = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesBuildSuccesses = 0;

    __declspec(dllexport)
        volatile unsigned long g_ReplayMoviesBuildStage = 0;

    __declspec(dllexport)
        volatile std::uintptr_t g_LastReplayMoviesObject = 0;
}


namespace
{
    constexpr std::uint32_t kSelectorExclusionBit =
        0x00400000;

    constexpr std::size_t kRowPayloadSize =
        0x44;

    constexpr std::uint32_t kGeometryDescriptorHeader =
        0x00010C04;

    constexpr std::uint32_t kGeometryChannel =
        0x006C1830;

    constexpr std::uint32_t kSelectedGeometryState =
        0x07000000;

    constexpr std::uint32_t kUnselectedGeometryState =
        0x07000014;

    constexpr std::uint32_t kStartupSubordinateState =
        0x00000578;

    constexpr std::uint32_t kSelectedSubordinateState =
        0x00000000;

    constexpr std::uint32_t kUnselectedSubordinateState =
        0x00000014;

    constexpr std::uint32_t kStartupSubordinateKey =
        0x001744B3;

    constexpr std::uint32_t kSelectedSubordinateKey =
        0x249DB7B7;

    constexpr std::uint32_t kUnselectedSubordinateKey =
        0x7AB5521A;

    constexpr std::uint32_t kHighlightSelectedStorageX =
        0x432F0000; // 175.0f

    constexpr std::uint32_t kHighlightSelectedStorageZ =
        0x42600000; // 56.0f

    constexpr std::uint32_t kHighlightUndergroundY =
        0xC2FC0000; // -126.0f

    // MU_MainMenuBar Organization keeps the native six-row outer span
    // (-141 .. +68) but redistributes seven rows uniformly inside it.
    // ReplayMovies therefore moves from -120 to -106.1666667, and the
    // shared hightlight01 selected presentation remains +15 above the row.
    constexpr std::uint32_t kHighlightReplayMoviesY =
        0xC2B65555; // -91.166664f

    constexpr std::size_t kMaxPackageObjects =
        256;

    constexpr std::uint32_t kGeometryX260 =
        0x43820000;

    constexpr std::uint32_t kGeometryZ50 =
        0x42480000;

    constexpr std::uint32_t kGeometryZ70 =
        0x428C0000;

    constexpr std::uintptr_t kDescriptorStride =
        0x38;

    constexpr std::size_t kMaxSubordinateNodes =
        64;

    constexpr std::size_t kPropertyScanSize =
        0x200;

    constexpr std::uint32_t kGeometryYMinus141 =
        0xC30D0000;

    // ---------------------------------------------------------
    // MU_MainMenuBar Organization
    // ---------------------------------------------------------
    // Native six-row visual order and current authored Y:
    //   UndergroundMode -141
    //   QuickRace        -99
    //   Statistics       -58
    //   Customize        -16
    //   Options           +26
    //   Online            +68
    //
    // Preserve the original outer endpoints exactly and divide the same
    // 209-unit span into six equal gaps for seven rows. This preserves the
    // Main Menu's existing top/bottom outer whitespace while making every
    // adjacent row interval mathematically identical.
    constexpr float kMenuBottomY = -141.0f;
    constexpr float kMenuTopY = 68.0f;
    constexpr float kMenuStep =
        (kMenuTopY - kMenuBottomY) / 6.0f;

    constexpr float kMenuYUndergroundMode =
        kMenuBottomY;
    constexpr float kMenuYReplayMovies =
        kMenuBottomY + kMenuStep;
    constexpr float kMenuYQuickRace =
        kMenuBottomY + (kMenuStep * 2.0f);
    constexpr float kMenuYStatistics =
        kMenuBottomY + (kMenuStep * 3.0f);
    constexpr float kMenuYCustomize =
        kMenuBottomY + (kMenuStep * 4.0f);
    constexpr float kMenuYOptions =
        kMenuBottomY + (kMenuStep * 5.0f);
    constexpr float kMenuYOnline =
        kMenuTopY;

    constexpr std::uint32_t kNativeYStatistics =
        0xC2680000; // -58.0f
    constexpr std::uint32_t kNativeYOnline =
        0x42880000; // +68.0f
    constexpr std::uint32_t kNativeYOptions =
        0x41D00000; // +26.0f
    constexpr std::uint32_t kNativeYCustomize =
        0xC1800000; // -16.0f
    constexpr std::uint32_t kNativeYQuickRace =
        0xC2C60000; // -99.0f
    constexpr std::uint32_t kNativeYUndergroundMode =
        0xC30D0000; // -141.0f
    constexpr std::uint32_t kBuilderYReplayMovies =
        0xC2F00000; // -120.0f before organization

    constexpr std::uint32_t kInheritedRestoreGeometryState =
        0x07000578;

    constexpr char kInternalName[] =
        "ReplayMovies";

    constexpr char kVisibleCaption[] =
        "REPLAY MOVIES";

    constexpr wchar_t kVisibleCaptionWide[] =
        L"REPLAY MOVIES";


    // Event-scoped return-path state. This is never a heap-address
    // constant or a generation cache. It is armed only by the exact
    // ReplayMovies -> UndergroundMode current-row commit and is cleared
    // by the next MU_Main row commit or by the matching +54 writer.
    static volatile std::uintptr_t g_PendingUndergroundReturnRuntime = 0;

    // Event-scoped ReplayMovies -> QuickRace transient suppression.
    // Armed only at the exact 004EFE87 row transition and consumed only
    // when 004FC374 targets the current generation's hightlight01 live Y
    // with the debugger-proven bad -126.0f intermediate value.
    static volatile std::uintptr_t g_PendingQuickRaceTransientRuntime = 0;


    static_assert(sizeof(wchar_t) == 2);


    using NativeCloneFEStringFn =
        void* (__thiscall*)(
            void* source,
            int mode);


    static DWORD g_NativeAttachFEObject =
        static_cast<DWORD>(
            Addr::NativeAttachFEObject);


    void* NativeCloneFEString(
        std::uintptr_t sourceAddress)
    {
        auto cloneFunction =
            reinterpret_cast<NativeCloneFEStringFn>(
                Addr::NativeCloneFEString);

        return cloneFunction(
            reinterpret_cast<void*>(
                sourceAddress),
            1);
    }


    __declspec(noinline)
        void NativeAttachFEObject(
            std::uintptr_t runtimeAddress,
            std::uintptr_t objectAddress)
    {
        __asm
        {
            // Proven native register contract:
            //
            // EAX = new FEObject*
            // EDX = package Runtime*

            mov eax, objectAddress
            mov edx, runtimeAddress

            call dword ptr[g_NativeAttachFEObject]
        }
    }


    struct GeometryDescriptorSet
    {
        std::uintptr_t selected = 0;
        std::uintptr_t unselected = 0;
        std::uintptr_t inheritedRestore = 0;
        std::uintptr_t startupNode = 0;
        std::uintptr_t unselectedNode = 0;
    };

    struct Highlight01State
    {
        std::uintptr_t object = 0;
        std::uintptr_t selectedNode = 0;
        std::uintptr_t unselectedNode = 0;
        std::uintptr_t selectedStorage = 0;
    };

    struct MenuRowRewritePlan
    {
        std::uintptr_t row = 0;
        std::uintptr_t payload = 0;
        float targetY = 0.0f;
        std::uintptr_t descriptors[32]{};
        std::size_t descriptorDistances[32]{};
        std::size_t descriptorCount = 0;
    };

    struct HighlightRewritePlan
    {
        std::uintptr_t object = 0;
        std::uintptr_t payload = 0;
        std::uintptr_t rootStorage = 0;
        std::uintptr_t selectedStorage = 0;
        std::uintptr_t tailStorage = 0;
        float deltaY = 0.0f;
    };


    bool IsReadableMemory(
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

        if (mbi.State != MEM_COMMIT)
            return false;

        if ((mbi.Protect & PAGE_GUARD) != 0)
            return false;

        if ((mbi.Protect & PAGE_NOACCESS) != 0)
            return false;

        const std::uintptr_t regionBegin =
            reinterpret_cast<std::uintptr_t>(
                mbi.BaseAddress);

        const std::uintptr_t regionEnd =
            regionBegin + mbi.RegionSize;

        if (address < regionBegin)
            return false;

        if (address > regionEnd)
            return false;

        if (size > regionEnd - address)
            return false;

        return true;
    }


    bool TryReadObjectHash(
        std::uintptr_t objectAddress,
        std::uint32_t& hash)
    {
        hash = 0;

        if (!IsReadableMemory(
            objectAddress + 0x10,
            sizeof(std::uint32_t)))
        {
            return false;
        }

        hash =
            *reinterpret_cast<const std::uint32_t*>(
                objectAddress + 0x10);

        return true;
    }


    bool FindReplayMoviesStateNodes(
        std::uintptr_t replayAddress,
        std::uintptr_t& startupNode,
        std::uintptr_t& unselectedNode)
    {
        startupNode = 0;
        unselectedNode = 0;

        // Runtime selection must not depend on geometry-descriptor headers.
        // Those descriptors can legitimately become inactive after the
        // Main Menu intro (descriptor byte +03 |= 0x80), while the
        // subordinate-node topology itself remains stable.
        //
        // The debugger-proven ReplayMovies chain is:
        //   +4C -> root/startup  state 0x578, key 001744B3
        //          +04 -> selected   state 0, key 249DB7B7
        //                  +04 -> unselected state 0x14
        //
        // This is exactly the structure used by the successful manual
        // first-selection A/B, so reacquire those nodes directly.

        if (!IsReadableMemory(replayAddress, 0x58))
            return false;

        std::uint32_t replayHash = 0;

        if (!TryReadObjectHash(replayAddress, replayHash) ||
            replayHash != Addr::ReplayMoviesHash)
        {
            return false;
        }

        const std::uintptr_t root =
            *reinterpret_cast<const std::uintptr_t*>(
                replayAddress + 0x4C);

        if (!IsReadableMemory(root, 0x34))
            return false;

        const std::uintptr_t selected =
            *reinterpret_cast<const std::uintptr_t*>(
                root + 0x04);

        if (!IsReadableMemory(selected, 0x34))
            return false;

        const std::uintptr_t unselected =
            *reinterpret_cast<const std::uintptr_t*>(
                selected + 0x04);

        if (!IsReadableMemory(unselected, 0x34))
            return false;

        if (*reinterpret_cast<const std::uintptr_t*>(
            selected + 0x08) != root ||
            *reinterpret_cast<const std::uintptr_t*>(
                unselected + 0x08) != selected)
        {
            return false;
        }

        if (*reinterpret_cast<const std::uint32_t*>(
            root + 0x0C) != kStartupSubordinateState ||
            *reinterpret_cast<const std::uint32_t*>(
                root + 0x30) != kStartupSubordinateKey ||
            *reinterpret_cast<const std::uint32_t*>(
                selected + 0x0C) != 0 ||
            *reinterpret_cast<const std::uint32_t*>(
                selected + 0x30) != kSelectedSubordinateKey ||
            *reinterpret_cast<const std::uint32_t*>(
                unselected + 0x0C) != kUnselectedSubordinateState)
        {
            return false;
        }

        startupNode = root;
        unselectedNode = unselected;
        return true;
    }


    bool FindHighlight01State(
        std::uintptr_t runtimeAddress,
        Highlight01State& result)
    {
        result = {};

        if (!IsReadableMemory(
            runtimeAddress,
            0x48))
        {
            return false;
        }

        if (*reinterpret_cast<const std::uint32_t*>(
            runtimeAddress + 0x10) != Addr::MuMainHash)
        {
            return false;
        }

        const auto objectCount =
            *reinterpret_cast<const std::uint32_t*>(
                runtimeAddress + 0x3C);

        if (objectCount == 0 ||
            objectCount > kMaxPackageObjects)
        {
            return false;
        }

        std::uintptr_t objectAddress =
            *reinterpret_cast<const std::uintptr_t*>(
                runtimeAddress + 0x44);

        for (std::uint32_t index = 0;
            index < objectCount && objectAddress != 0;
            ++index)
        {
            if (!IsReadableMemory(
                objectAddress,
                0x58))
            {
                return false;
            }

            const auto objectHash =
                *reinterpret_cast<const std::uint32_t*>(
                    objectAddress + 0x10);

            if (objectHash == Addr::Highlight01Hash)
            {
                const std::uintptr_t rootNode =
                    *reinterpret_cast<const std::uintptr_t*>(
                        objectAddress + 0x4C);

                if (!IsReadableMemory(rootNode, 0x34))
                    return false;

                const std::uintptr_t selectedNode =
                    *reinterpret_cast<const std::uintptr_t*>(
                        rootNode + 0x04);

                if (!IsReadableMemory(selectedNode, 0x34))
                    return false;

                const std::uintptr_t unselectedNode =
                    *reinterpret_cast<const std::uintptr_t*>(
                        selectedNode + 0x04);

                if (!IsReadableMemory(unselectedNode, 0x34))
                    return false;

                // hightlight01 uses a three-record Type-5 family:
                // root <-> selected/middle <-> tail. The selected record
                // is identified by the independently proven 249DB7B7 key.
                if (*reinterpret_cast<const std::uintptr_t*>(
                    selectedNode + 0x08) != rootNode ||
                    *reinterpret_cast<const std::uintptr_t*>(
                        unselectedNode + 0x08) != selectedNode)
                {
                    return false;
                }

                if (*reinterpret_cast<const std::uint32_t*>(
                    selectedNode + 0x30) !=
                    kSelectedSubordinateKey)
                {
                    return false;
                }

                const std::uintptr_t selectedStorage =
                    *reinterpret_cast<const std::uintptr_t*>(
                        selectedNode + 0x20);

                if (!IsReadableMemory(
                    selectedStorage,
                    0x5C))
                {
                    return false;
                }

                // hightlight01's selected Type-5 storage was
                // debugger-proven to use {175, Y, 56} at +50/+54/+58.
                // Keep these invariant components as a fail-closed guard;
                // Y is intentionally allowed to be either native or Replay.
                if (*reinterpret_cast<const std::uint32_t*>(
                    selectedStorage + 0x50) !=
                    kHighlightSelectedStorageX)
                {
                    return false;
                }

                if (*reinterpret_cast<const std::uint32_t*>(
                    selectedStorage + 0x58) !=
                    kHighlightSelectedStorageZ)
                {
                    return false;
                }

                result.object = objectAddress;
                result.selectedNode = selectedNode;
                result.unselectedNode = unselectedNode;
                result.selectedStorage = selectedStorage;
                return true;
            }

            objectAddress =
                *reinterpret_cast<const std::uintptr_t*>(
                    objectAddress + 0x08);
        }

        return false;
    }


    bool IsGeometryDescriptor(
        std::uintptr_t descriptorAddress,
        std::uint32_t expectedState,
        std::uint32_t expectedZ)
    {
        if (!IsReadableMemory(
            descriptorAddress,
            0x24))
        {
            return false;
        }

        const auto header =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x00);

        const auto state =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x04);

        const auto channel =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x08);

        const auto x =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x18);

        const auto z =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x20);

        return
            header == kGeometryDescriptorHeader &&
            state == expectedState &&
            channel == kGeometryChannel &&
            x == kGeometryX260 &&
            z == expectedZ;
    }

    bool IsInheritedRestoreGeometryDescriptor(
        std::uintptr_t descriptorAddress)
    {
        if (!IsReadableMemory(
            descriptorAddress,
            0x24))
        {
            return false;
        }

        const auto header =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x00);

        const auto state =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x04);

        const auto channel =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x08);

        const auto x =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x18);

        const auto y =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x1C);

        const auto z =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x20);

        return
            header == kGeometryDescriptorHeader &&
            state == kInheritedRestoreGeometryState &&
            channel == kGeometryChannel &&
            x == kGeometryX260 &&
            y == kGeometryYMinus141 &&
            z == kGeometryZ70;
    }

    bool FindReplayMoviesGeometryDescriptors(
        std::uintptr_t cloneAddress,
        GeometryDescriptorSet& result)
    {
        result = {};

        // Diagnostic:
        // Entered geometry-descriptor discovery.
        g_ReplayMoviesBuildStage = 50;

        if (!IsReadableMemory(
            cloneAddress + 0x4C,
            sizeof(std::uintptr_t)))
        {
            // clone+4C cannot be read.
            g_ReplayMoviesBuildStage = 51;
            return false;
        }

        // ---------------------------------------------------------
        // Collect the unique property-storage regions reached through
        // this clone's native subordinate chain.
        //
        // Issue 5 correction:
        // physical spacing between the selected and unselected
        // property allocations is NOT a semantic ownership invariant.
        // Runtime A/B proved successful clones can place them 0x118
        // apart while failed-but-valid clones can place them 0xC8
        // apart or in entirely separate allocations.
        //
        // The stable relationship is subordinate ownership:
        //
        //   startup node    state 0x578 / key 0x001744B3
        //   selected node   state 0     / key 0x249DB7B7
        //   unselected node state 0x14  / key 0x7AB5521A
        //
        // and each relevant geometry descriptor is at propertyStorage
        // +0x38.  Resolve by that semantic node ownership, never by
        // selectedDescriptor + a fixed heap distance.
        // ---------------------------------------------------------

        std::uintptr_t propertyStorages[
            kMaxSubordinateNodes]{};

            std::uintptr_t subordinateNodes[
                kMaxSubordinateNodes]{};

                std::uintptr_t subordinatePropertyStorages[
                    kMaxSubordinateNodes]{};

                    std::size_t propertyStorageCount = 0;
                    std::size_t subordinateNodeCount = 0;

                    std::uintptr_t node =
                        *reinterpret_cast<const std::uintptr_t*>(
                            cloneAddress + 0x4C);

                    for (std::size_t nodeIndex = 0;
                        nodeIndex < kMaxSubordinateNodes && node != 0;
                        ++nodeIndex)
                    {
                        // Native subordinate node evidence:
                        //
                        // node+04 = next node
                        // node+0C = semantic state
                        // node+20 = property storage
                        // node+30 = semantic key

                        if (!IsReadableMemory(node, 0x34))
                        {
                            // A subordinate node in the clone-owned chain
                            // is not safely readable.
                            g_ReplayMoviesBuildStage = 52;
                            return false;
                        }

                        const std::uintptr_t nextNode =
                            *reinterpret_cast<const std::uintptr_t*>(
                                node + 0x04);

                        const std::uintptr_t propertyStorage =
                            *reinterpret_cast<const std::uintptr_t*>(
                                node + 0x20);

                        subordinateNodes[subordinateNodeCount] =
                            node;

                        subordinatePropertyStorages[
                            subordinateNodeCount] =
                            propertyStorage;

                            ++subordinateNodeCount;

                            if (propertyStorage != 0 &&
                                IsReadableMemory(
                                    propertyStorage,
                                    kPropertyScanSize))
                            {
                                // Avoid scanning the same property-storage region
                                // twice if multiple subordinate nodes reference it.

                                bool alreadyRecorded = false;

                                for (std::size_t i = 0;
                                    i < propertyStorageCount;
                                    ++i)
                                {
                                    if (propertyStorages[i] ==
                                        propertyStorage)
                                    {
                                        alreadyRecorded = true;
                                        break;
                                    }
                                }

                                if (!alreadyRecorded)
                                {
                                    if (propertyStorageCount >=
                                        kMaxSubordinateNodes)
                                    {
                                        // More unique property-storage regions than
                                        // our fail-closed capacity allows.
                                        g_ReplayMoviesBuildStage = 53;
                                        return false;
                                    }

                                    propertyStorages[
                                        propertyStorageCount++] =
                                        propertyStorage;
                                }
                            }

                            // Reject a malformed self-referencing node.
                            if (nextNode == node)
                            {
                                g_ReplayMoviesBuildStage = 54;
                                return false;
                            }

                            node = nextNode;
                    }

                    if (propertyStorageCount == 0)
                    {
                        // The clone-owned subordinate chain yielded no readable
                        // property-storage regions.
                        g_ReplayMoviesBuildStage = 55;
                        return false;
                    }

                    // ---------------------------------------------------------
                    // Independently locate the inherited restore descriptor.
                    // It is allowed to occupy any clone-owned property storage.
                    // ---------------------------------------------------------

                    std::uintptr_t inheritedRestoreDescriptor = 0;

                    for (std::size_t storageIndex = 0;
                        storageIndex < propertyStorageCount;
                        ++storageIndex)
                    {
                        const std::uintptr_t propertyStorage =
                            propertyStorages[storageIndex];

                        for (std::size_t offset = 0;
                            offset + 0x24 <= kPropertyScanSize;
                            offset += sizeof(std::uint32_t))
                        {
                            const std::uintptr_t candidate =
                                propertyStorage + offset;

                            if (!IsInheritedRestoreGeometryDescriptor(
                                candidate))
                            {
                                continue;
                            }

                            if (inheritedRestoreDescriptor == 0)
                            {
                                inheritedRestoreDescriptor =
                                    candidate;
                            }
                            else if (
                                inheritedRestoreDescriptor != candidate)
                            {
                                // More than one distinct inherited-restore
                                // descriptor satisfies the structural signature.
                                // Ambiguous -> fail closed.
                                g_ReplayMoviesBuildStage = 56;
                                result = {};
                                return false;
                            }
                        }
                    }

                    // ---------------------------------------------------------
                    // Resolve selected/unselected geometry through the exact
                    // subordinate nodes that semantically own those states.
                    //
                    // This is the debugger-proven Issue 5 replacement for the old
                    // selected + 0x118 == unselected address assumption.
                    // ---------------------------------------------------------

                    std::uintptr_t selectedDescriptor = 0;
                    std::uintptr_t unselectedDescriptor = 0;
                    std::uintptr_t unselectedNode = 0;

                    for (std::size_t nodeIndex = 0;
                        nodeIndex < subordinateNodeCount;
                        ++nodeIndex)
                    {
                        const std::uintptr_t candidateNode =
                            subordinateNodes[nodeIndex];

                        const std::uintptr_t propertyStorage =
                            subordinatePropertyStorages[nodeIndex];

                        if (propertyStorage == 0)
                            continue;

                        // +0x38 is the stable node-owned geometry descriptor
                        // position proved in GOOD and BAD reconstruction layouts.
                        if (!IsReadableMemory(
                            propertyStorage,
                            kDescriptorStride + 0x24))
                        {
                            continue;
                        }

                        const auto subordinateState =
                            *reinterpret_cast<const std::uint32_t*>(
                                candidateNode + 0x0C);

                        const auto subordinateKey =
                            *reinterpret_cast<const std::uint32_t*>(
                                candidateNode + 0x30);

                        const std::uintptr_t descriptor =
                            propertyStorage + kDescriptorStride;

                        if (subordinateState ==
                            kSelectedSubordinateState &&
                            subordinateKey ==
                            kSelectedSubordinateKey &&
                            IsGeometryDescriptor(
                                descriptor,
                                kSelectedGeometryState,
                                kGeometryZ50))
                        {
                            if (selectedDescriptor == 0)
                            {
                                selectedDescriptor = descriptor;
                            }
                            else if (selectedDescriptor != descriptor)
                            {
                                // More than one distinct semantic selected node owns
                                // a valid selected geometry descriptor.
                                g_ReplayMoviesBuildStage = 57;
                                result = {};
                                return false;
                            }
                        }

                        if (subordinateState ==
                            kUnselectedSubordinateState &&
                            subordinateKey ==
                            kUnselectedSubordinateKey &&
                            IsGeometryDescriptor(
                                descriptor,
                                kUnselectedGeometryState,
                                kGeometryZ70))
                        {
                            if (unselectedDescriptor == 0)
                            {
                                unselectedDescriptor = descriptor;
                                unselectedNode = candidateNode;
                            }
                            else if (
                                unselectedDescriptor != descriptor ||
                                unselectedNode != candidateNode)
                            {
                                // More than one distinct semantic unselected node owns
                                // a valid unselected geometry descriptor.
                                g_ReplayMoviesBuildStage = 62;
                                result = {};
                                return false;
                            }
                        }
                    }

                    // ---------------------------------------------------------
                    // Final fail-closed validation.
                    // ---------------------------------------------------------

                    if (selectedDescriptor == 0)
                    {
                        // No valid descriptor was owned by the exact semantic
                        // selected subordinate node.
                        g_ReplayMoviesBuildStage = 58;
                        result = {};
                        return false;
                    }

                    if (unselectedDescriptor == 0)
                    {
                        // No valid descriptor was owned by the exact semantic
                        // unselected subordinate node.
                        g_ReplayMoviesBuildStage = 59;
                        result = {};
                        return false;
                    }

                    if (inheritedRestoreDescriptor == 0)
                    {
                        // The {260,-141,70} state-07000578 descriptor was not
                        // found anywhere in the clone-owned subordinate chain.
                        g_ReplayMoviesBuildStage = 60;
                        result = {};
                        return false;
                    }

                    // Selected and unselected are distinct semantic states and must
                    // never resolve to the same descriptor.  No physical spacing is
                    // otherwise required.
                    if (selectedDescriptor == unselectedDescriptor)
                    {
                        g_ReplayMoviesBuildStage = 61;
                        result = {};
                        return false;
                    }

                    if (unselectedNode == 0)
                    {
                        // Defensive: unselected descriptor/node are assigned together.
                        g_ReplayMoviesBuildStage = 63;
                        result = {};
                        return false;
                    }

                    // The current startup A/B proved ReplayMovies must begin
                    // on its OWN root/startup subordinate node, matching the
                    // authored Main Menu rows at 004F1527. Publishing it
                    // directly on the state-0x14 unselected node makes the
                    // clone render before the six native rows enter.
                    const std::uintptr_t startupNode =
                        *reinterpret_cast<const std::uintptr_t*>(
                            cloneAddress + 0x4C);

                    if (!IsReadableMemory(startupNode, 0x34))
                    {
                        g_ReplayMoviesBuildStage = 64;
                        result = {};
                        return false;
                    }

                    if (*reinterpret_cast<const std::uint32_t*>(
                        startupNode + 0x0C) !=
                        kStartupSubordinateState)
                    {
                        g_ReplayMoviesBuildStage = 65;
                        result = {};
                        return false;
                    }

                    if (*reinterpret_cast<const std::uint32_t*>(
                        startupNode + 0x30) !=
                        kStartupSubordinateKey)
                    {
                        g_ReplayMoviesBuildStage = 66;
                        result = {};
                        return false;
                    }

                    if (startupNode == unselectedNode)
                    {
                        g_ReplayMoviesBuildStage = 67;
                        result = {};
                        return false;
                    }

                    result.selected =
                        selectedDescriptor;

                    result.unselected =
                        unselectedDescriptor;

                    result.inheritedRestore =
                        inheritedRestoreDescriptor;

                    result.startupNode =
                        startupNode;

                    result.unselectedNode =
                        unselectedNode;

                    return true;
    }

    bool IsMenuRowGeometryDescriptor(
        std::uintptr_t descriptorAddress)
    {
        if (!IsReadableMemory(descriptorAddress, 0x24))
            return false;

        const auto header =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x00);

        const auto state =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x04);

        const auto channel =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x08);

        const auto x =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x18);

        const auto z =
            *reinterpret_cast<const std::uint32_t*>(
                descriptorAddress + 0x20);

        // Descriptor byte +03 can legitimately acquire 0x80 after
        // evaluation. Ignore only that proven dormancy bit while keeping
        // the rest of the native descriptor signature exact.
        if ((header & 0x7FFFFFFFu) != kGeometryDescriptorHeader)
            return false;

        if (state != kSelectedGeometryState &&
            state != kUnselectedGeometryState &&
            state != kInheritedRestoreGeometryState)
        {
            return false;
        }

        return
            channel == kGeometryChannel &&
            x == kGeometryX260 &&
            (z == kGeometryZ50 || z == kGeometryZ70);
    }


    bool PrepareMenuRowRewrite(
        std::uintptr_t rowAddress,
        std::uint32_t expectedHash,
        std::uint32_t expectedLiveY,
        float targetY,
        MenuRowRewritePlan& plan)
    {
        plan = {};

        if (!IsReadableMemory(rowAddress, 0x58))
            return false;

        std::uint32_t rowHash = 0;
        if (!TryReadObjectHash(rowAddress, rowHash) ||
            rowHash != expectedHash)
        {
            return false;
        }

        const auto payload =
            *reinterpret_cast<const std::uintptr_t*>(
                rowAddress + 0x2C);

        if (!IsReadableMemory(payload, kRowPayloadSize))
            return false;

        if (*reinterpret_cast<const std::uint32_t*>(
            rowAddress + 0x30) != kRowPayloadSize ||
            *reinterpret_cast<const std::uint32_t*>(
                payload + 0x1C) != kGeometryX260 ||
            *reinterpret_cast<const std::uint32_t*>(
                payload + 0x20) != expectedLiveY)
        {
            return false;
        }

        const auto payloadZ =
            *reinterpret_cast<const std::uint32_t*>(
                payload + 0x24);

        if (payloadZ != kGeometryZ50 &&
            payloadZ != kGeometryZ70)
        {
            return false;
        }

        std::uintptr_t propertyStorages[kMaxSubordinateNodes]{};
        std::size_t propertyStorageCount = 0;

        std::uintptr_t node =
            *reinterpret_cast<const std::uintptr_t*>(
                rowAddress + 0x4C);

        for (std::size_t nodeIndex = 0;
            nodeIndex < kMaxSubordinateNodes && node != 0;
            ++nodeIndex)
        {
            if (!IsReadableMemory(node, 0x24))
                return false;

            const auto propertyStorage =
                *reinterpret_cast<const std::uintptr_t*>(
                    node + 0x20);

            if (propertyStorage != 0 &&
                IsReadableMemory(propertyStorage, kPropertyScanSize))
            {
                bool duplicate = false;

                for (std::size_t index = 0;
                    index < propertyStorageCount;
                    ++index)
                {
                    if (propertyStorages[index] == propertyStorage)
                    {
                        duplicate = true;
                        break;
                    }
                }

                if (!duplicate)
                {
                    if (propertyStorageCount >= kMaxSubordinateNodes)
                        return false;

                    propertyStorages[propertyStorageCount++] =
                        propertyStorage;
                }
            }

            node =
                *reinterpret_cast<const std::uintptr_t*>(
                    node + 0x04);
        }

        if (propertyStorageCount == 0)
            return false;

        for (std::size_t storageIndex = 0;
            storageIndex < propertyStorageCount;
            ++storageIndex)
        {
            const auto propertyStorage =
                propertyStorages[storageIndex];

            for (std::size_t offset = 0;
                offset + 0x24 <= kPropertyScanSize;
                offset += sizeof(std::uint32_t))
            {
                const auto descriptor =
                    propertyStorage + offset;

                if (!IsMenuRowGeometryDescriptor(descriptor))
                    continue;

                bool duplicate = false;

                for (std::size_t index = 0;
                    index < plan.descriptorCount;
                    ++index)
                {
                    if (plan.descriptors[index] == descriptor)
                    {
                        duplicate = true;

                        // One row can expose the same descriptor through more
                        // than one subordinate-property storage. Preserve the
                        // nearest local storage relationship for the later
                        // cross-row ownership arbitration.
                        if (offset < plan.descriptorDistances[index])
                            plan.descriptorDistances[index] = offset;

                        break;
                    }
                }

                if (duplicate)
                    continue;

                if (plan.descriptorCount >=
                    (sizeof(plan.descriptors) /
                        sizeof(plan.descriptors[0])))
                {
                    return false;
                }

                plan.descriptors[plan.descriptorCount] = descriptor;
                plan.descriptorDistances[plan.descriptorCount] = offset;
                ++plan.descriptorCount;
            }
        }

        // Current-generation census found four unique geometry sources per
        // row. Require at least the native selected/unselected/startup set;
        // any additional matching row-owned source is rewritten as well.
        if (plan.descriptorCount < 3)
            return false;

        plan.row = rowAddress;
        plan.payload = payload;
        plan.targetY = targetY;
        return true;
    }


    bool ResolveMenuRowDescriptorOwnership(
        MenuRowRewritePlan* rows,
        std::size_t rowCount)
    {
        if (rows == nullptr || rowCount == 0)
            return false;

        // PrepareMenuRowRewrite intentionally uses a broad read-only scan so
        // every active/startup geometry source is discovered. Runtime proof
        // showed that two neighboring property-storage windows can overlap,
        // causing one real descriptor to be collected by two different rows.
        // Rewriting both plans then lets the later row overwrite the earlier
        // row's selected-state Y. Resolve those collisions before any mutation.
        //
        // Ownership is assigned to the row whose subordinate property storage
        // is nearest to the descriptor. This exactly matches the debugger proof
        // for the Go Underground descriptor: local +0x38 versus accidental
        // Quick Race +0x128. Equal-distance ownership is ambiguous and fails
        // closed rather than guessing.
        bool keep[7][32]{};

        if (rowCount > (sizeof(keep) / sizeof(keep[0])))
            return false;

        for (std::size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
        {
            if (rows[rowIndex].descriptorCount >
                (sizeof(keep[rowIndex]) / sizeof(keep[rowIndex][0])))
            {
                return false;
            }

            for (std::size_t descriptorIndex = 0;
                descriptorIndex < rows[rowIndex].descriptorCount;
                ++descriptorIndex)
            {
                keep[rowIndex][descriptorIndex] = true;
            }
        }

        for (std::size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
        {
            for (std::size_t descriptorIndex = 0;
                descriptorIndex < rows[rowIndex].descriptorCount;
                ++descriptorIndex)
            {
                const std::uintptr_t descriptor =
                    rows[rowIndex].descriptors[descriptorIndex];

                std::size_t ownerRow = rowIndex;
                std::size_t ownerDistance =
                    rows[rowIndex].descriptorDistances[descriptorIndex];
                bool ambiguous = false;

                for (std::size_t otherRow = 0;
                    otherRow < rowCount;
                    ++otherRow)
                {
                    if (otherRow == rowIndex)
                        continue;

                    for (std::size_t otherIndex = 0;
                        otherIndex < rows[otherRow].descriptorCount;
                        ++otherIndex)
                    {
                        if (rows[otherRow].descriptors[otherIndex] != descriptor)
                            continue;

                        const std::size_t otherDistance =
                            rows[otherRow].descriptorDistances[otherIndex];

                        if (otherDistance < ownerDistance)
                        {
                            ownerRow = otherRow;
                            ownerDistance = otherDistance;
                            ambiguous = false;
                        }
                        else if (otherDistance == ownerDistance &&
                            otherRow != ownerRow)
                        {
                            ambiguous = true;
                        }
                    }
                }

                if (ambiguous)
                    return false;

                if (ownerRow != rowIndex)
                    keep[rowIndex][descriptorIndex] = false;
            }
        }

        for (std::size_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
        {
            std::size_t writeIndex = 0;

            for (std::size_t readIndex = 0;
                readIndex < rows[rowIndex].descriptorCount;
                ++readIndex)
            {
                if (!keep[rowIndex][readIndex])
                    continue;

                rows[rowIndex].descriptors[writeIndex] =
                    rows[rowIndex].descriptors[readIndex];
                rows[rowIndex].descriptorDistances[writeIndex] =
                    rows[rowIndex].descriptorDistances[readIndex];
                ++writeIndex;
            }

            rows[rowIndex].descriptorCount = writeIndex;

            // Preserve the existing production preflight contract: each row
            // must still expose at least the selected/unselected/startup family
            // after collision ownership is resolved.
            if (rows[rowIndex].descriptorCount < 3)
                return false;
        }

        return true;
    }


    bool PrepareHighlightRewrite(
        std::uintptr_t runtimeAddress,
        std::uint32_t highlightHash,
        float deltaY,
        HighlightRewritePlan& plan)
    {
        plan = {};

        if (!IsReadableMemory(runtimeAddress, 0x48) ||
            *reinterpret_cast<const std::uint32_t*>(
                runtimeAddress + 0x10) != Addr::MuMainHash)
        {
            return false;
        }

        const auto objectCount =
            *reinterpret_cast<const std::uint32_t*>(
                runtimeAddress + 0x3C);

        if (objectCount == 0 || objectCount > kMaxPackageObjects)
            return false;

        std::uintptr_t objectAddress =
            *reinterpret_cast<const std::uintptr_t*>(
                runtimeAddress + 0x44);

        for (std::uint32_t index = 0;
            index < objectCount && objectAddress != 0;
            ++index)
        {
            if (!IsReadableMemory(objectAddress, 0x58))
                return false;

            const auto objectHash =
                *reinterpret_cast<const std::uint32_t*>(
                    objectAddress + 0x10);

            if (objectHash == highlightHash)
            {
                const auto payload =
                    *reinterpret_cast<const std::uintptr_t*>(
                        objectAddress + 0x2C);

                const auto root =
                    *reinterpret_cast<const std::uintptr_t*>(
                        objectAddress + 0x4C);

                if (!IsReadableMemory(payload, 0x28) ||
                    !IsReadableMemory(root, 0x34))
                {
                    return false;
                }

                const auto selected =
                    *reinterpret_cast<const std::uintptr_t*>(
                        root + 0x04);

                if (!IsReadableMemory(selected, 0x34))
                    return false;

                const auto tail =
                    *reinterpret_cast<const std::uintptr_t*>(
                        selected + 0x04);

                if (!IsReadableMemory(tail, 0x34))
                    return false;

                if (*reinterpret_cast<const std::uintptr_t*>(
                    selected + 0x08) != root ||
                    *reinterpret_cast<const std::uintptr_t*>(
                        tail + 0x08) != selected ||
                    *reinterpret_cast<const std::uint32_t*>(
                        selected + 0x30) != kSelectedSubordinateKey)
                {
                    return false;
                }

                const auto rootStorage =
                    *reinterpret_cast<const std::uintptr_t*>(
                        root + 0x20);

                const auto selectedStorage =
                    *reinterpret_cast<const std::uintptr_t*>(
                        selected + 0x20);

                const auto tailStorage =
                    *reinterpret_cast<const std::uintptr_t*>(
                        tail + 0x20);

                const std::uintptr_t stores[] =
                {
                    rootStorage,
                    selectedStorage,
                    tailStorage
                };

                for (const auto store : stores)
                {
                    if (!IsReadableMemory(store, 0x5C) ||
                        *reinterpret_cast<const std::uint32_t*>(
                            store + 0x50) !=
                        kHighlightSelectedStorageX ||
                        *reinterpret_cast<const std::uint32_t*>(
                            store + 0x58) !=
                        kHighlightSelectedStorageZ)
                    {
                        return false;
                    }
                }

                if (*reinterpret_cast<const std::uint32_t*>(
                    payload + 0x1C) !=
                    kHighlightSelectedStorageX ||
                    *reinterpret_cast<const std::uint32_t*>(
                        payload + 0x24) !=
                    kHighlightSelectedStorageZ)
                {
                    return false;
                }

                plan.object = objectAddress;
                plan.payload = payload;
                plan.rootStorage = rootStorage;
                plan.selectedStorage = selectedStorage;
                plan.tailStorage = tailStorage;
                plan.deltaY = deltaY;
                return true;
            }

            objectAddress =
                *reinterpret_cast<const std::uintptr_t*>(
                    objectAddress + 0x08);
        }

        return false;
    }


    void ApplyMenuRowRewrite(
        const MenuRowRewritePlan& plan)
    {
        for (std::size_t index = 0;
            index < plan.descriptorCount;
            ++index)
        {
            *reinterpret_cast<float*>(
                plan.descriptors[index] + 0x1C) =
                plan.targetY;
        }

        *reinterpret_cast<float*>(
            plan.payload + 0x20) =
            plan.targetY;
    }


    void ApplyHighlightRewrite(
        const HighlightRewritePlan& plan)
    {
        auto shiftY = [delta = plan.deltaY](
            std::uintptr_t address)
            {
                auto* value =
                    reinterpret_cast<float*>(address);

                *value += delta;
            };

        // Root/selected/tail all contain presentation Y at +54. The
        // debugger-proven H1/H2 transition logs show tail evaluation can
        // feed live presentation, so shift all three sources together.
        shiftY(plan.rootStorage + 0x54);
        shiftY(plan.selectedStorage + 0x54);
        shiftY(plan.tailStorage + 0x54);

        // Synchronize the live RightSide presentation coordinate so the
        // current generation cannot expose one pre-organization frame.
        shiftY(plan.payload + 0x20);
    }


    bool OrganizeMuMainMenuBarForGeneration(
        std::uintptr_t runtimeAddress,
        std::uintptr_t vectorAddress,
        std::uintptr_t replayAddress)
    {
        if (!IsReadableMemory(runtimeAddress, 0x88) ||
            !IsReadableMemory(vectorAddress,
                7 * sizeof(std::uintptr_t)) ||
            !IsReadableMemory(replayAddress, 0x58))
        {
            return false;
        }

        if (*reinterpret_cast<const std::uint32_t*>(
            runtimeAddress + 0x10) != Addr::MuMainHash ||
            *reinterpret_cast<const std::uint32_t*>(
                runtimeAddress + 0x88) != 7)
        {
            return false;
        }

        auto* candidates =
            reinterpret_cast<std::uintptr_t*>(
                vectorAddress);

        // candidate[6] is intentionally still the UndergroundMode safety
        // duplicate here. ReplayMovies is supplied explicitly so all seven
        // objects can be organized before final candidate publication.
        MenuRowRewritePlan rows[7]{};

        if (!PrepareMenuRowRewrite(
            candidates[5],
            Addr::UndergroundModeHash,
            kNativeYUndergroundMode,
            kMenuYUndergroundMode,
            rows[0]) ||
            !PrepareMenuRowRewrite(
                replayAddress,
                Addr::ReplayMoviesHash,
                kBuilderYReplayMovies,
                kMenuYReplayMovies,
                rows[1]) ||
            !PrepareMenuRowRewrite(
                candidates[4],
                Addr::QuickRaceHash,
                kNativeYQuickRace,
                kMenuYQuickRace,
                rows[2]) ||
            !PrepareMenuRowRewrite(
                candidates[0],
                Addr::StatisticsHash,
                kNativeYStatistics,
                kMenuYStatistics,
                rows[3]) ||
            !PrepareMenuRowRewrite(
                candidates[3],
                Addr::CustomizeHash,
                kNativeYCustomize,
                kMenuYCustomize,
                rows[4]) ||
            !PrepareMenuRowRewrite(
                candidates[2],
                Addr::OptionsHash,
                kNativeYOptions,
                kMenuYOptions,
                rows[5]) ||
            !PrepareMenuRowRewrite(
                candidates[1],
                Addr::OnlineHash,
                kNativeYOnline,
                kMenuYOnline,
                rows[6]))
        {
            return false;
        }

        // The broad per-storage discovery windows can overlap across adjacent
        // menu rows. Resolve cross-row descriptor ownership while the entire
        // seven-row set is available and before any geometry DWORD is changed.
        if (!ResolveMenuRowDescriptorOwnership(
            rows,
            sizeof(rows) / sizeof(rows[0])))
        {
            return false;
        }

        // Native highlight-object mapping recovered from the package graph:
        //   H1 UndergroundMode / ReplayMovies (special dual ownership)
        //   H2 QuickRace
        //   H3 Customize
        //   H4 Options
        //   H5 Online
        //   H6 Statistics
        //
        // H1's UndergroundMode row remains at -141, so its root/tail/native
        // presentation sources need no organization shift. Its ReplayMovies
        // selected target is handled by kHighlightReplayMoviesY above.
        HighlightRewritePlan highlights[5]{};

        if (!PrepareHighlightRewrite(
            runtimeAddress,
            Addr::Highlight02Hash,
            kMenuYQuickRace - (-99.0f),
            highlights[0]) ||
            !PrepareHighlightRewrite(
                runtimeAddress,
                Addr::Highlight03Hash,
                kMenuYCustomize - (-16.0f),
                highlights[1]) ||
            !PrepareHighlightRewrite(
                runtimeAddress,
                Addr::Highlight04Hash,
                kMenuYOptions - 26.0f,
                highlights[2]) ||
            !PrepareHighlightRewrite(
                runtimeAddress,
                Addr::Highlight05Hash,
                kMenuYOnline - 68.0f,
                highlights[3]) ||
            !PrepareHighlightRewrite(
                runtimeAddress,
                Addr::Highlight06Hash,
                kMenuYStatistics - (-58.0f),
                highlights[4]))
        {
            return false;
        }

        // All reads/structural validation complete. Only now mutate the
        // generation, keeping the operation fail-closed against partial
        // discovery failures.
        for (const auto& row : rows)
            ApplyMenuRowRewrite(row);

        for (const auto& highlight : highlights)
            ApplyHighlightRewrite(highlight);

        return true;
    }


    void WriteGeometry(
        std::uintptr_t descriptorAddress,
        float x,
        float y,
        float z)
    {
        *reinterpret_cast<float*>(
            descriptorAddress + 0x18) = x;

        *reinterpret_cast<float*>(
            descriptorAddress + 0x1C) = y;

        *reinterpret_cast<float*>(
            descriptorAddress + 0x20) = z;
    }


    void WritePayloadGeometry(
        std::uintptr_t payloadAddress,
        float x,
        float y,
        float z)
    {
        *reinterpret_cast<float*>(
            payloadAddress + 0x1C) = x;

        *reinterpret_cast<float*>(
            payloadAddress + 0x20) = y;

        *reinterpret_cast<float*>(
            payloadAddress + 0x24) = z;
    }
}


bool BuildReplayMoviesForGeneration(
    std::uintptr_t runtimeAddress,
    std::uintptr_t vectorAddress,
    std::uintptr_t undergroundAddress)
{
    ++g_ReplayMoviesBuildAttempts;

    g_ReplayMoviesBuildStage = 1;
    g_LastReplayMoviesObject = 0;


    if (runtimeAddress == 0 ||
        vectorAddress == 0 ||
        undergroundAddress == 0)
    {
        return false;
    }


    auto* runtime =
        reinterpret_cast<std::uint8_t*>(
            runtimeAddress);

    auto* candidates =
        reinterpret_cast<std::uintptr_t*>(
            vectorAddress);

    auto* source =
        reinterpret_cast<std::uint8_t*>(
            undergroundAddress);


    // ---------------------------------------------------------
    // Revalidate the fail-closed state.
    // ---------------------------------------------------------

    if (*reinterpret_cast<std::uint32_t*>(
        runtime + 0x10) != Addr::MuMainHash)
    {
        return false;
    }

    if (*reinterpret_cast<std::uint32_t*>(
        runtime + 0x88) != 7)
    {
        return false;
    }

    if (candidates[5] != undergroundAddress)
        return false;

    if (candidates[6] != undergroundAddress)
        return false;

    if (*reinterpret_cast<std::uint32_t*>(
        source + 0x10)
        != Addr::UndergroundModeHash)
    {
        return false;
    }


    // ---------------------------------------------------------
    // Clone native UndergroundMode FE_String.
    // ---------------------------------------------------------

    g_ReplayMoviesBuildStage = 2;

    void* clonePointer =
        NativeCloneFEString(
            undergroundAddress);

    if (clonePointer == nullptr)
        return false;


    const std::uintptr_t cloneAddress =
        reinterpret_cast<std::uintptr_t>(
            clonePointer);

    g_LastReplayMoviesObject =
        cloneAddress;


    auto* clone =
        reinterpret_cast<std::uint8_t*>(
            cloneAddress);


    // ---------------------------------------------------------
    // Acquire clone-owned storage.
    // ---------------------------------------------------------

    g_ReplayMoviesBuildStage = 3;


    const std::uintptr_t cloneName =
        *reinterpret_cast<std::uintptr_t*>(
            clone + 0x14);

    const std::uintptr_t sourcePayload =
        *reinterpret_cast<std::uintptr_t*>(
            source + 0x2C);

    const std::uintptr_t clonePayload =
        *reinterpret_cast<std::uintptr_t*>(
            clone + 0x2C);

    const std::uintptr_t authoredKey =
        *reinterpret_cast<std::uintptr_t*>(
            clone + 0x58);

    const std::uintptr_t wideCaption =
        *reinterpret_cast<std::uintptr_t*>(
            clone + 0x60);


    if (cloneName == 0 ||
        sourcePayload == 0 ||
        clonePayload == 0 ||
        authoredKey == 0 ||
        wideCaption == 0)
    {
        return false;
    }


    // ---------------------------------------------------------
    // Populate the clone's independent 0x44 row payload.
    //
    // 005009A0 gives us legitimate independent storage,
    // but the cloned row payload is not automatically a
    // ready-to-render duplicate.
    // ---------------------------------------------------------

    g_ReplayMoviesBuildStage = 4;


    std::memcpy(
        reinterpret_cast<void*>(
            clonePayload),
        reinterpret_cast<const void*>(
            sourcePayload),
        kRowPayloadSize);


    // ---------------------------------------------------------
    // Internal ReplayMovies identity.
    // ---------------------------------------------------------

    *reinterpret_cast<std::uint32_t*>(
        clone + 0x10) =
        Addr::ReplayMoviesHash;


    std::memcpy(
        reinterpret_cast<void*>(
            cloneName),
        kInternalName,
        sizeof(kInternalName));


    // ---------------------------------------------------------
    // Preserve every legitimate runtime flag inherited from
    // the current source and clear ONLY the native clone
    // exclusion/dirty bit.
    // ---------------------------------------------------------

    auto& cloneFlags =
        *reinterpret_cast<std::uint32_t*>(
            clone + 0x1C);

    cloneFlags &=
        ~kSelectorExclusionBit;


    // ---------------------------------------------------------
    // Visible native-style caption:
    //
    // +58 -> authored/raw key
    // +5C -> FEHashUpper(+58)
    // +60 -> independent UTF-16 text
    // ---------------------------------------------------------

    std::memcpy(
        reinterpret_cast<void*>(
            authoredKey),
        kVisibleCaption,
        sizeof(kVisibleCaption));


    *reinterpret_cast<std::uint32_t*>(
        clone + 0x5C) =
        Addr::ReplayMoviesCaptionHash;


    std::memcpy(
        reinterpret_cast<void*>(
            wideCaption),
        kVisibleCaptionWide,
        sizeof(kVisibleCaptionWide));


    // ---------------------------------------------------------
    // Recover the clone-owned geometry descriptor set and the
    // subordinate node that owns the unselected state.
    //
    // Discovery is structural through the clone-owned +4C chain
    // and each node's +20 property storage. No heap-adjacency or
    // authored-string-relative address assumptions are used.
    //
    // Selected/unselected are resolved through their exact semantic
    // subordinate-node ownership. Physical heap spacing between their
    // property allocations is intentionally ignored.
    // ---------------------------------------------------------

    g_ReplayMoviesBuildStage = 5;


    GeometryDescriptorSet geometry{};

    if (!FindReplayMoviesGeometryDescriptors(
        cloneAddress,
        geometry))
    {
        return false;
    }


    const std::uintptr_t selectedDescriptor =
        geometry.selected;

    const std::uintptr_t baseDescriptor =
        geometry.unselected;

    const std::uintptr_t inheritedRestoreDescriptor =
        geometry.inheritedRestore;

    const std::uintptr_t startupNode =
        geometry.startupNode;

    const std::uintptr_t unselectedNode =
        geometry.unselectedNode;


    // Pre-organization ReplayMovies clone baseline:
    //
    // unselected = {260,-120,70}
    // selected   = {260,-120,50}
    //
    // MU_MainMenuBar Organization later rewrites every clone-owned row-Y
    // source to the mathematically normalized seven-row target before final
    // candidate publication.

    WriteGeometry(
        baseDescriptor,
        260.0f,
        -120.0f,
        70.0f);

    WriteGeometry(
        selectedDescriptor,
        260.0f,
        -120.0f,
        50.0f);

    WriteGeometry(
        inheritedRestoreDescriptor,
        260.0f,
        -120.0f,
        70.0f);


    // Synchronize the live payload to the unselected state.

    WritePayloadGeometry(
        clonePayload,
        260.0f,
        -120.0f,
        70.0f);


    g_ReplayMoviesBuildStage = 6;


    // ---------------------------------------------------------
    // Register into MU_Main's native heterogeneous FEObject
    // lifetime/object list BEFORE publishing it as candidate 6.
    // ---------------------------------------------------------

    const std::uint32_t objectCountBefore =
        *reinterpret_cast<std::uint32_t*>(
            runtime + 0x3C);


    NativeAttachFEObject(
        runtimeAddress,
        cloneAddress);


    g_ReplayMoviesBuildStage = 7;


    // Native attach validation.
    if (*reinterpret_cast<std::uint32_t*>(
        runtime + 0x3C)
        != objectCountBefore + 1)
    {
        return false;
    }


    if (*reinterpret_cast<std::uintptr_t*>(
        runtime + 0x44)
        != cloneAddress)
    {
        return false;
    }


    // ---------------------------------------------------------
    // Initialize the native startup/intro subordinate state.
    //
    // Current debugger A/B proof at 004F1527:
    //
    //   authored UndergroundMode +54 -> own root, +0C = 0x578
    //   premature ReplayMovies +54  -> own node2, +0C = 0x14
    //
    // Forcing ReplayMovies directly to state 0x14 made it render
    // before the six authored rows. Pointing +54 to ReplayMovies'
    // OWN clone-relative root/startup node made all seven rows enter
    // together. No timer/delay is involved.
    //
    // Keep the discovered unselected node validated because it is
    // part of the normal later selected/unselected state machine,
    // but do NOT publish it as the initial +54 state.
    // ---------------------------------------------------------

    if (startupNode == 0 ||
        unselectedNode == 0 ||
        !IsReadableMemory(startupNode, 0x34) ||
        !IsReadableMemory(unselectedNode, 0x14))
    {
        return false;
    }

    if (*reinterpret_cast<const std::uint32_t*>(
        startupNode + 0x0C) !=
        kStartupSubordinateState ||
        *reinterpret_cast<const std::uint32_t*>(
            startupNode + 0x30) !=
        kStartupSubordinateKey ||
        *reinterpret_cast<const std::uint32_t*>(
            unselectedNode + 0x0C) !=
        kUnselectedSubordinateState)
    {
        return false;
    }

    *reinterpret_cast<std::uintptr_t*>(
        clone + 0x54) =
        startupNode;

    if (*reinterpret_cast<const std::uintptr_t*>(
        clone + 0x54) !=
        startupNode)
    {
        return false;
    }


    // ---------------------------------------------------------
    // MU_MainMenuBar Organization.
    //
    // This is deliberately generation-local and runs after the real
    // ReplayMovies FE_String exists/owns valid native state, but before
    // candidate[6] is finally published and before control returns to the
    // native frontend presentation path. Every MU_Main reconstruction runs
    // this transaction again using only current-generation pointers.
    // ---------------------------------------------------------

    g_ReplayMoviesBuildStage = 75;

    if (!OrganizeMuMainMenuBarForGeneration(
        runtimeAddress,
        vectorAddress,
        cloneAddress))
    {
        g_ReplayMoviesBuildStage = 76;
        return false;
    }


    // ---------------------------------------------------------
    // FINAL PUBLICATION.
    //
    // Until this exact write, candidate 6 remains the valid
    // UndergroundMode safety duplicate.
    // ---------------------------------------------------------

    candidates[6] =
        cloneAddress;


    g_ReplayMoviesBuildStage = 8;
    ++g_ReplayMoviesBuildSuccesses;

    return true;
}

bool PrepareReplayMoviesPostReturnNativeSelection(
    std::uintptr_t runtimeAddress,
    std::uintptr_t replayAddress)
{
    // Captures(2026-09-02) isolated the direct post-return restore contract.
    // 004F5ED0 can select ReplayMovies by hash only after H1's selected Y
    // is moved to the organized ReplayMovies position. Unlike ordinary
    // directional selection, this direct restore runs during MU_Main
    // generation completion and MUST NOT perform the normal +54
    // startup->unselected handoff here: doing so makes ReplayMovies render
    // roughly 1-2 seconds before the native Main Menu entrance completes.

    if (!IsReadableMemory(runtimeAddress, 0x8C) ||
        *reinterpret_cast<const std::uint32_t*>(
            runtimeAddress + 0x10) != Addr::MuMainHash ||
        *reinterpret_cast<const std::uint32_t*>(
            runtimeAddress + 0x88) != 7 ||
        !IsReadableMemory(replayAddress, 0x58))
    {
        return false;
    }

    std::uint32_t replayHash = 0;

    if (!TryReadObjectHash(replayAddress, replayHash) ||
        replayHash != Addr::ReplayMoviesHash)
    {
        return false;
    }

    // The successful H1-only A/B required ReplayMovies to remain on the
    // generation-local root/startup node (state 0x578) while 004F5ED0 ran.
    // Fail closed instead of moving +54 or accepting an unexpected state.
    std::uintptr_t startupNode = 0;
    std::uintptr_t unselectedNode = 0;

    if (!FindReplayMoviesStateNodes(
        replayAddress,
        startupNode,
        unselectedNode))
    {
        return false;
    }

    const std::uintptr_t currentNode =
        *reinterpret_cast<const std::uintptr_t*>(
            replayAddress + 0x54);

    if (currentNode != startupNode)
        return false;

    Highlight01State highlight{};

    if (!FindHighlight01State(
        runtimeAddress,
        highlight))
    {
        return false;
    }

    auto* selectedY =
        reinterpret_cast<std::uint32_t*>(
            highlight.selectedStorage + 0x54);

    const std::uint32_t currentY = *selectedY;

    if (currentY != kHighlightUndergroundY &&
        currentY != kHighlightReplayMoviesY)
    {
        return false;
    }

    *selectedY = kHighlightReplayMoviesY;

    return *selectedY == kHighlightReplayMoviesY;
}


void ApplyReplayMoviesVisualTransitionPreCommit(
    std::uintptr_t runtimeAddress,
    std::uintptr_t oldRowAddress,
    std::uintptr_t newRowAddress)
{
    // The 004EFE87 hook is generic frontend code. Qualify MU_Main before
    // touching any generation-local presentation state.
    if (!IsReadableMemory(runtimeAddress, 0x88) ||
        *reinterpret_cast<const std::uint32_t*>(
            runtimeAddress + 0x10) != Addr::MuMainHash)
    {
        return;
    }

    // Pending visual corrections are valid only for one exact MU_Main
    // row transition. Any later row commit expires them automatically.
    g_PendingUndergroundReturnRuntime = 0;
    g_PendingQuickRaceTransientRuntime = 0;

    std::uint32_t oldHash = 0;
    std::uint32_t newHash = 0;

    if (!TryReadObjectHash(oldRowAddress, oldHash) ||
        !TryReadObjectHash(newRowAddress, newHash))
    {
        return;
    }

    const bool selectingReplayMovies =
        newHash == Addr::ReplayMoviesHash;

    const bool selectingUnderground =
        newHash == Addr::UndergroundModeHash;

    const bool returningToUndergroundFromReplayMovies =
        oldHash == Addr::ReplayMoviesHash &&
        selectingUnderground;

    const bool selectingQuickRaceFromReplayMovies =
        oldHash == Addr::ReplayMoviesHash &&
        newHash == Addr::QuickRaceHash;

    if (!selectingReplayMovies &&
        !selectingUnderground &&
        !selectingQuickRaceFromReplayMovies)
    {
        return;
    }

    if (selectingReplayMovies)
    {
        // -----------------------------------------------------
        // Startup/intro -> normal unselected handoff.
        //
        // Build-time proof requires ReplayMovies +54 to begin on
        // its OWN root/startup node (state 0x578), otherwise it
        // renders before the six native rows during Main Menu entry.
        //
        // First-selection proof then requires +54 to be on its OWN
        // unselected node (state 0x14) before the native selection
        // machinery runs; otherwise ReplayMovies stays cyan and
        // receives neither bar presentation nor selected Z state.
        //
        // Only perform this handoff when +54 is still exactly the
        // clone-relative startup node. Later selected/unselected
        // transitions remain native.
        // -----------------------------------------------------

        std::uintptr_t startupNode = 0;
        std::uintptr_t unselectedNode = 0;

        if (FindReplayMoviesStateNodes(
            newRowAddress,
            startupNode,
            unselectedNode))
        {
            auto* currentNodeField =
                reinterpret_cast<std::uintptr_t*>(
                    newRowAddress + 0x54);

            const std::uintptr_t currentNode =
                *currentNodeField;

            // Exact successful debugger intervention:
            // at first ReplayMovies selection pre-commit, +54 was still
            // the startup/root node. Replacing only that pointer with the
            // clone's own unselected node restored native selected-node
            // processing, WHITE text, horizontal bars and PinPointer.
            if (currentNode == startupNode)
            {
                *currentNodeField =
                    unselectedNode;
            }
        }
    }

    Highlight01State highlight{};

    if (!FindHighlight01State(
        runtimeAddress,
        highlight))
    {
        return;
    }

    auto* selectedY =
        reinterpret_cast<std::uint32_t*>(
            highlight.selectedStorage + 0x54);

    const std::uint32_t currentY = *selectedY;

    // Do not overwrite an unknown/native-revision value. The only two
    // values ever written by this patch are the exact x32dbg-proven
    // UndergroundMode and ReplayMovies positions.
    if (currentY != kHighlightUndergroundY &&
        currentY != kHighlightReplayMoviesY)
    {
        return;
    }

    if (selectingQuickRaceFromReplayMovies)
    {
        // ReplayMovies -> QuickRace is already correct in its final
        // settled state. The defect is only the millisecond intermediate
        // presentation where H1 falls back to authored UndergroundMode Y.
        // Arm a one-transition correction while H1 is genuinely selected
        // at the ReplayMovies presentation position. The 004FC374 hook
        // independently proves the exact generation-local output target
        // before changing any value.
        if (currentY != kHighlightReplayMoviesY)
            return;

        if (!IsReadableMemory(
            highlight.object + 0x54,
            sizeof(std::uintptr_t)))
        {
            return;
        }

        const std::uintptr_t activeNode =
            *reinterpret_cast<const std::uintptr_t*>(
                highlight.object + 0x54);

        if (activeNode != highlight.selectedNode)
            return;

        g_PendingQuickRaceTransientRuntime =
            runtimeAddress;
        return;
    }

    if (selectingReplayMovies)
    {
        // QuickRace/native row -> ReplayMovies causal A/B:
        // hightlight01 selected-storage Y must be the organized
        // ReplayMovies highlight target BEFORE the selected property is
        // evaluated.
        *selectedY = kHighlightReplayMoviesY;
        return;
    }

    // Any -> UndergroundMode stale-H1 causal A/B (2026-09-12):
    // a full downward Main Menu lap can leave hightlight01's selected
    // storage Y at the organized ReplayMovies value even after ReplayMovies
    // is no longer current. At the final PlayOnline -> UndergroundMode
    // 004EFE87 pre-commit checkpoint, changing ONLY this DWORD from
    // kHighlightReplayMoviesY to kHighlightUndergroundY fixed the displaced
    // Go Underground highlight 100% under native continuation.
    //
    // Therefore every UndergroundMode selection may normalize this shared
    // selected-property source, regardless of which native row precedes it.
    // Do not broaden the +54 writer correction below: that second correction
    // remains specific to the separately proven direct
    // ReplayMovies -> UndergroundMode transition.
    *selectedY = kHighlightUndergroundY;

    if (returningToUndergroundFromReplayMovies)
    {
        g_PendingUndergroundReturnRuntime =
            runtimeAddress;
    }
}


std::uintptr_t ResolveReplayMoviesActiveNodeWrite(
    std::uintptr_t parentObjectAddress,
    std::uintptr_t incomingNodeAddress)
{
    const std::uintptr_t runtimeAddress =
        g_PendingUndergroundReturnRuntime;

    if (runtimeAddress == 0)
        return incomingNodeAddress;

    // The pending state is event-scoped. If the generation/runtime is
    // no longer valid, expire it rather than touching an unrelated write.
    if (!IsReadableMemory(runtimeAddress, 0x88) ||
        *reinterpret_cast<const std::uint32_t*>(
            runtimeAddress + 0x10) != Addr::MuMainHash)
    {
        g_PendingUndergroundReturnRuntime = 0;
        return incomingNodeAddress;
    }

    // The current-row commit has already executed by the time 004F1116
    // is reached. Confirm MU_Main still considers UndergroundMode current.
    const std::uintptr_t currentRow =
        *reinterpret_cast<const std::uintptr_t*>(
            runtimeAddress + 0x80);

    std::uint32_t currentRowHash = 0;

    if (!TryReadObjectHash(currentRow, currentRowHash) ||
        currentRowHash != Addr::UndergroundModeHash)
    {
        g_PendingUndergroundReturnRuntime = 0;
        return incomingNodeAddress;
    }

    // Most 004F1116 invocations belong to other FEObjects. A cheap hash
    // guard avoids scanning the MU_Main object graph unless the parent is
    // actually hightlight01.
    std::uint32_t parentHash = 0;

    if (!TryReadObjectHash(parentObjectAddress, parentHash))
        return incomingNodeAddress;

    if (parentHash != Addr::Highlight01Hash)
        return incomingNodeAddress;

    Highlight01State highlight{};

    if (!FindHighlight01State(
        runtimeAddress,
        highlight))
    {
        g_PendingUndergroundReturnRuntime = 0;
        return incomingNodeAddress;
    }

    if (highlight.object != parentObjectAddress)
    {
        g_PendingUndergroundReturnRuntime = 0;
        return incomingNodeAddress;
    }

    const std::uint32_t selectedY =
        *reinterpret_cast<const std::uint32_t*>(
            highlight.selectedStorage + 0x54);

    if (selectedY != kHighlightUndergroundY)
    {
        g_PendingUndergroundReturnRuntime = 0;
        return incomingNodeAddress;
    }

    // Native processing can write the selected node first and then later
    // issue the bad tail/unselected write during the SAME
    // ReplayMovies -> UndergroundMode transition.
    //
    // Do NOT consume the pending event on this correct intermediate write.
    // The latest x32dbg chronology proves the first post-commit 004F1116
    // for hightlight01 already has ESI == selectedNode, while the final
    // settled state still becomes tail/unselected.  Consuming here was the
    // production bug: it disarmed the correction before the later bad write.
    //
    // The event remains bounded because every subsequent 004EFE87 row commit
    // clears/re-arms g_PendingUndergroundReturnRuntime, and the actual tail
    // substitution below consumes it.
    if (incomingNodeAddress == highlight.selectedNode)
    {
        return incomingNodeAddress;
    }

    // This is the exact debugger-proven defect:
    //
    // ReplayMovies -> UndergroundMode
    // 004F1116 about to write hightlight01 +54 = tail/unselected
    //
    // Replacing ESI with hightlight01's OWN selected/middle node before
    // the native MOV restored UndergroundMode's horizontal bars and
    // RightSide PinPointer. The native MOV and 004FF030 call then execute
    // normally using the corrected ESI.
    if (incomingNodeAddress == highlight.unselectedNode)
    {
        g_PendingUndergroundReturnRuntime = 0;
        return highlight.selectedNode;
    }

    // Unknown hightlight01 node for this event: fail closed and expire.
    g_PendingUndergroundReturnRuntime = 0;
    return incomingNodeAddress;
}

std::uint32_t ResolveReplayMoviesQuickRaceTransientYWrite(
    std::uintptr_t writerBaseAddress,
    std::uint32_t incomingY)
{
    const std::uintptr_t runtimeAddress =
        g_PendingQuickRaceTransientRuntime;

    if (runtimeAddress == 0)
        return incomingY;

    if (!IsReadableMemory(runtimeAddress, 0x88) ||
        *reinterpret_cast<const std::uint32_t*>(
            runtimeAddress + 0x10) != Addr::MuMainHash)
    {
        g_PendingQuickRaceTransientRuntime = 0;
        return incomingY;
    }

    // Do NOT require runtime+80 / visible row presentation to have reached
    // QuickRace here. The defect exists inside the ReplayMovies -> QuickRace
    // transition itself: the bars can briefly visit UndergroundMode before
    // the QuickRace presentation is visibly established. The exact transition
    // was already qualified and armed at the 004EFE87 pre-commit boundary.
    // From this point onward, identify the one bad output structurally by the
    // current generation's H1 object/node/payload and the proven
    // ReplayMovies-presentation -> UndergroundMode live-Y write.

    Highlight01State highlight{};

    if (!FindHighlight01State(
        runtimeAddress,
        highlight))
    {
        g_PendingQuickRaceTransientRuntime = 0;
        return incomingY;
    }

    if (!IsReadableMemory(
        highlight.object + 0x54,
        sizeof(std::uintptr_t)))
    {
        g_PendingQuickRaceTransientRuntime = 0;
        return incomingY;
    }

    // The causal chronology showed H1 selected -> tail first, then the
    // tail evaluator generated the bad -126 live output.
    const std::uintptr_t activeNode =
        *reinterpret_cast<const std::uintptr_t*>(
            highlight.object + 0x54);

    if (activeNode != highlight.unselectedNode)
        return incomingY;

    // Corrected debugger reconstruction:
    //   H1 presentation payload = [H1+2C]
    //   H1 live Y              = payload+20
    // At 004FC374 the destination is [EBX+4], therefore the exact H1
    // writer has EBX == payload+1C. Never hardcode the captured heap
    // address from a single generation.
    if (!IsReadableMemory(
        highlight.object + 0x2C,
        sizeof(std::uintptr_t)))
    {
        g_PendingQuickRaceTransientRuntime = 0;
        return incomingY;
    }

    const std::uintptr_t payload =
        *reinterpret_cast<const std::uintptr_t*>(
            highlight.object + 0x2C);

    if (!IsReadableMemory(payload, 0x24))
    {
        g_PendingQuickRaceTransientRuntime = 0;
        return incomingY;
    }

    const std::uintptr_t expectedWriterBase =
        payload + 0x1C;

    if (writerBaseAddress != expectedWriterBase)
        return incomingY;

    const std::uint32_t liveY =
        *reinterpret_cast<const std::uint32_t*>(
            payload + 0x20);

    // Exact debugger-proven transient:
    //     current H1 live Y  = organized ReplayMovies presentation
    //     incoming EAX       = -126 UndergroundMode
    // Suppress only that intermediate output. QuickRace's own H2 remains
    // fully native except for the generation-local MenuBar organization
    // shift and settles to its normalized QuickRace position afterward.
    if (incomingY == kHighlightUndergroundY &&
        liveY == kHighlightReplayMoviesY)
    {
        g_PendingQuickRaceTransientRuntime = 0;
        return kHighlightReplayMoviesY;
    }

    // The exact H1 output target was reached with an unexpected value.
    // Expire this event rather than modifying unknown presentation state.
    g_PendingQuickRaceTransientRuntime = 0;
    return incomingY;
}