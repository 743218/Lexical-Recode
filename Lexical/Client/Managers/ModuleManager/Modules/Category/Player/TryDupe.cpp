// Tiast's Dupe
#include "TryDupe.h"
#include "Dupe.h"
#include "DupeCompat.h"

#include "../../../../CommandManager/CommandManager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <string>

#include <Windows.h>

#include "../../../../../../SDK/Game.h"
#include "../../../../../../SDK/Network/Packet/InventoryTransactionPacket.h"
#include "../../../../../../SDK/Network/Packet/LevelEventPacket.h"
#include "../../../../../../SDK/World/Inventory/Transaction/InventoryAction.h"
#include "../../../../../../SDK/World/Inventory/Transaction/InventoryTransaction.h"
#include "../../../../../../SDK/World/Inventory/Transaction/ComplexInventoryTransaction.h"
#include "../../../../../../SDK/NBT/ShulkerNBT.h"
#include "../../../../../../Utils/FileUtil.h"

struct RawPointerVectorView {
    const uintptr_t* begin;
    const uintptr_t* end;
    const uintptr_t* capacity;
};
//DUpe1
struct RawObjectVectorView {
    const uint8_t* begin;
    const uint8_t* end;
    const uint8_t* capacity;
};

struct AnvilStackRequestDataView {
    uint64_t requestId;
    RawObjectVectorView stringsToFilter;
    RawPointerVectorView actions;
};

struct AnvilStackRequestBatchView {
    RawPointerVectorView requests;
};

namespace {
    static_assert(
        sizeof(Packet) == 0x30,
        "InventoryTransactionPacket ABI requires Packet size 0x30"
        );
    static_assert(
        sizeof(InventoryTransactionPacket) == 0x88,
        "InventoryTransactionPacket wrapper no longer matches the 0x78 ABI"
        );

    constexpr uintptr_t kItemStackRequestBatchOffset = 0x30;
    constexpr std::array<uintptr_t, 1> kTransactionPointerOffsets = {
        0x78
    };
    constexpr std::array<uintptr_t, 3> kAuthRequestOffsets = {
        0xA0,
        0xA8,
        0xB8
    };

    constexpr uintptr_t kActionTypeOffset = 0x08;
    constexpr uintptr_t kTransferCountOffset = 0x10;
    constexpr uintptr_t kTransferSourceOffset = 0x18;
    constexpr uintptr_t kLegacyTransferDestinationOffset = 0x38;
    constexpr uintptr_t kFullNameTransferDestinationOffset = 0x40;
    constexpr uintptr_t kLegacySlotOffset = 0x01;
    constexpr uintptr_t kFullNameSlotOffset = 0x0C;

    constexpr uint8_t kTakeAction = 0x00;
    constexpr uint8_t kPlaceAction = 0x01;
    constexpr uint8_t kSwapAction = 0x02;
    constexpr uint8_t kCraftRecipeOptionalAction = 0x0F;

    constexpr int8_t kAnvilResultPreviewContainer = 2;
    constexpr int8_t kCombinedInventoryContainer = 12;
    constexpr int8_t kHotbarContainer = 28;
    constexpr int8_t kInventoryContainer = 29;
    constexpr int8_t kCursorContainer = 59;
    constexpr int8_t kCreatedOutputContainer = 60;

    constexpr int8_t kAnvilInputWindowId = -10;
    constexpr int8_t kAnvilMaterialWindowId = -11;
    constexpr int8_t kAnvilResultWindowId = -12;
    constexpr int8_t kAnvilOutputWindowId = -13;
    constexpr uint32_t kAnvilMaterialInventorySlot = 1;
    constexpr int8_t kEnchantInputWindowId = -15;
    constexpr int8_t kEnchantMaterialWindowId = -16;
    constexpr int8_t kEnchantOutputWindowId = -17;
    constexpr int8_t kCraftingAddIngredientWindowId = -2;
    constexpr int8_t kCraftingRemoveIngredientWindowId = -3;
    constexpr int8_t kCraftingResultWindowId = -4;
    constexpr int8_t kCraftingUseIngredientWindowId = -5;
    constexpr int32_t kCraftSlotSourceType = 100;
    constexpr uint32_t kEnchantInputUiSlot = 14;
    constexpr uint32_t kEnchantMaterialUiSlot = 15;

    constexpr int kInventorySlotCount = 36;
    constexpr int kMainInventoryFirstSlot = 9;
    constexpr int kInventorySettleTicks = 2;
    constexpr int kCaptureTimeoutTicks = 80;
    constexpr int kLegacyPacketTimeoutTicks = 1200;
    constexpr int kServerResponseCaptureTicks = 12;
    constexpr int kAnvilResyncProbeTicks = 12;
    constexpr int kNbtWindowAckTimeoutTicks = 80;
    constexpr int kNbtWindowExtractDelayTicks = 2;
    constexpr int kNbtWindowPostTakeSettleTicks = 4;
    constexpr int kEnchantResponseCaptureTicks = 8;
    constexpr int kEnchantStageGapTicks = 1;
    constexpr int kDropResponseCaptureTicks = 20;
    constexpr int kTicksBetweenTransactions = 1;
    constexpr unsigned int kMaxTransactionItemCount = 255;
    constexpr int kAnvilMaterialReadbackNone = 0;
    constexpr int kAnvilMaterialReadbackUnavailable = 1;
    constexpr int kAnvilMaterialReadbackAir = 2;
    constexpr int kAnvilMaterialReadbackDifferent = 3;
    constexpr int kAnvilMaterialReadbackExact = 4;

    const char* getDupeModeName(int selectedMode) {
        switch (selectedMode) {
        case 1:
            return "newdyedupe";
        case 2:
            return "olddyedupe";
        case 3:
            return "dyedupe3";
        case 4:
            return "Nested";
        case 5:
            return "AnvilSwap";
        case 6:
            return "dyedupe4";
        case 7:
            return "enchantdupe";
        case 8:
            return "AnvilSwap";
        case 9:
            return "Ether";
        default:
            return "Anvil";
        }
    }

    const char* getSingleNbtExperimentName(int selectedMode) {
        switch (selectedMode) {
        case 0:
            return "Control";
        case 1:
            return "Items[0].Count";
        case 2:
            return "Items[0].Name-or-ID";
        default:
            return "Entire Items";
        }
    }

    const char* getInnerItemLockName(int selectedMode) {
        switch (selectedMode) {
        case 1:
            return "Can't Move";
        case 2:
            return "Can't Drop";
        default:
            return "Unchanged";
        }
    }

    const char* getAnvilMaterialReadbackName(int state) {
        switch (state) {
        case kAnvilMaterialReadbackUnavailable:
            return "unavailable";
        case kAnvilMaterialReadbackAir:
            return "air";
        case kAnvilMaterialReadbackDifferent:
            return "different";
        case kAnvilMaterialReadbackExact:
            return "exact";
        default:
            return "not-observed";
        }
    }

    std::vector<uint8_t> planDestinationCounts(
        std::size_t destinationCount,
        uint8_t baseCount,
        bool stackingEnabled,
        int requestedStackCount
    ) {
        std::vector<uint8_t> counts(destinationCount, baseCount);
        if (!stackingEnabled ||
            requestedStackCount <= static_cast<int>(baseCount) ||
            destinationCount == 0) {
            return counts;
        }

        const unsigned int requestedCount = static_cast<unsigned int>(
            std::clamp(requestedStackCount, 1, 255));
        const unsigned int targetCount =
            (requestedCount / static_cast<unsigned int>(baseCount)) *
            static_cast<unsigned int>(baseCount);
        unsigned int transactionItemCount =
            static_cast<unsigned int>(baseCount) +
            static_cast<unsigned int>(destinationCount) *
            static_cast<unsigned int>(baseCount);
        const unsigned int extraPerStackedSlot =
            targetCount - static_cast<unsigned int>(baseCount);

        for (uint8_t& count : counts) {
            if (transactionItemCount + extraPerStackedSlot >
                kMaxTransactionItemCount) {
                break;
            }
            count = static_cast<uint8_t>(targetCount);
            transactionItemCount += extraPerStackedSlot;
        }

        return counts;
    }

    struct DecodedDestination {
        int8_t container = -1;
        uint8_t slot = 0;
        bool valid = false;
    };

    bool isReadableRange(const void* address, std::size_t byteCount) {
        if (!address || byteCount == 0) {
            return false;
        }

        uintptr_t cursor = reinterpret_cast<uintptr_t>(address);
        if (cursor > std::numeric_limits<uintptr_t>::max() - byteCount) {
            return false;
        }

        const uintptr_t end = cursor + byteCount;
        while (cursor < end) {
            MEMORY_BASIC_INFORMATION info{};
            if (VirtualQuery(
                reinterpret_cast<const void*>(cursor),
                &info,
                sizeof(info)
            ) == 0) {
                return false;
            }

            if (info.State != MEM_COMMIT ||
                (info.Protect & PAGE_GUARD) != 0 ||
                (info.Protect & PAGE_NOACCESS) != 0) {
                return false;
            }

            const uintptr_t regionStart =
                reinterpret_cast<uintptr_t>(info.BaseAddress);
            const uintptr_t regionEnd = regionStart + info.RegionSize;
            if (regionEnd <= cursor) {
                return false;
            }
            cursor = regionEnd;
        }

        return true;
    }

    bool isWritableRange(void* address, std::size_t byteCount) {
        if (!isReadableRange(address, byteCount)) {
            return false;
        }

        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(address, &info, sizeof(info)) == 0) {
            return false;
        }
        const DWORD protection = info.Protect & 0xFF;
        return protection == PAGE_READWRITE ||
            protection == PAGE_WRITECOPY ||
            protection == PAGE_EXECUTE_READWRITE ||
            protection == PAGE_EXECUTE_WRITECOPY;
    }

    bool isExecutableAddress(uintptr_t address) {
        if (address == 0) {
            return false;
        }

        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(
            reinterpret_cast<const void*>(address),
            &info,
            sizeof(info)
        ) == 0 ||
            info.State != MEM_COMMIT ||
            (info.Protect & PAGE_GUARD) != 0) {
            return false;
        }

        const DWORD protection = info.Protect & 0xFF;
        return protection == PAGE_EXECUTE ||
            protection == PAGE_EXECUTE_READ ||
            protection == PAGE_EXECUTE_READWRITE ||
            protection == PAGE_EXECUTE_WRITECOPY;
    }

    bool canCloneNativeItemStack() {
        const auto* itemStackVtable = reinterpret_cast<const uintptr_t*>(
            Addresses::ItemStackVTable
            );
        return
            isExecutableAddress(Addresses::ItemStackBase_CopyConstructor) &&
            isReadableRange(
                itemStackVtable,
                sizeof(uintptr_t) * 5
            ) &&
            isExecutableAddress(itemStackVtable[4]);
    }

    bool canConstructNativeInventoryTransaction() {
        const auto* transactionVtable = reinterpret_cast<const uintptr_t*>(
            Addresses::ComplexInventoryTransactionVTable
            );
        const auto* itemStackVtable = reinterpret_cast<const uintptr_t*>(
            Addresses::ItemStackVTable
            );
        const auto* emptyItem = reinterpret_cast<const ItemStack*>(
            Addresses::ItemStack_EMPTY_ITEM
            );
        return
            Addresses::ComplexInventoryTransactionVTable != 0 &&
            isReadableRange(
                transactionVtable,
                sizeof(uintptr_t)
            ) &&
            isExecutableAddress(transactionVtable[0]) &&
            isExecutableAddress(Addresses::ItemStackBase_CopyConstructor) &&
            isExecutableAddress(Addresses::InventoryAction_Constructor) &&
            isExecutableAddress(Addresses::InventoryTransaction_addAction) &&
            isExecutableAddress(
                Addresses::InventoryTransactionPacket_Constructor
            ) &&
            isReadableRange(
                emptyItem,
                sizeof(ItemStack)
            ) &&
            emptyItem->Vtable != nullptr &&
            isReadableRange(
                emptyItem->Vtable,
                sizeof(uintptr_t) * 5
            ) &&
            isExecutableAddress(
                reinterpret_cast<const uintptr_t*>(
                    emptyItem->Vtable
                    )[4]
            ) &&
            isReadableRange(
                itemStackVtable,
                sizeof(uintptr_t) * 5
            ) &&
            isExecutableAddress(
                itemStackVtable[4]
            );
    }

    class NativeItemStackResetGuard {
    public:
        explicit NativeItemStackResetGuard(ItemStack* stack)
            : mStack(stack) {
        }

        ~NativeItemStackResetGuard() {
            if (mStack) {
                mStack->setNull(std::nullopt);
            }
        }

        NativeItemStackResetGuard(
            const NativeItemStackResetGuard&
        ) = delete;
        NativeItemStackResetGuard& operator=(
            const NativeItemStackResetGuard&
            ) = delete;

    private:
        ItemStack* mStack;
    };

    void resetNativeItemStack(
        std::unique_ptr<ItemStack>& stack
    ) {
        if (stack) {
            stack->setNull(std::nullopt);
            stack.reset();
        }
    }

    class NativeItemStackOwnerResetGuard {
    public:
        explicit NativeItemStackOwnerResetGuard(
            std::unique_ptr<ItemStack>& stack
        ) : mStack(stack) {
        }

        ~NativeItemStackOwnerResetGuard() {
            resetNativeItemStack(mStack);
        }

        NativeItemStackOwnerResetGuard(
            const NativeItemStackOwnerResetGuard&
        ) = delete;
        NativeItemStackOwnerResetGuard& operator=(
            const NativeItemStackOwnerResetGuard&
            ) = delete;

    private:
        std::unique_ptr<ItemStack>& mStack;
    };

    class NativeInventoryActionResetGuard {
    public:
        explicit NativeInventoryActionResetGuard(
            InventoryAction* action
        ) : mAction(action) {
        }

        ~NativeInventoryActionResetGuard() {
            if (mAction) {
                mAction->fromItem.setNull(std::nullopt);
                mAction->toItem.setNull(std::nullopt);
            }
        }

        NativeInventoryActionResetGuard(
            const NativeInventoryActionResetGuard&
        ) = delete;
        NativeInventoryActionResetGuard& operator=(
            const NativeInventoryActionResetGuard&
            ) = delete;

    private:
        InventoryAction* mAction;
    };

    void resetNativeTransactionItemStacks(
        ComplexInventoryTransaction* transaction
    ) {
        if (!transaction) {
            return;
        }
        for (auto& [source, actions] : transaction->data.actions) {
            (void)source;
            for (InventoryAction& action : actions) {
                action.fromItem.setNull(std::nullopt);
                action.toItem.setNull(std::nullopt);
            }
        }
    }

    void resetNativeActionItemStacks(
        std::vector<InventoryAction>* actions
    ) {
        if (!actions) {
            return;
        }
        for (InventoryAction& action : *actions) {
            action.fromItem.setNull(std::nullopt);
            action.toItem.setNull(std::nullopt);
        }
    }

    ComplexInventoryTransaction* getValidatedTransaction(
        const InventoryTransactionPacket* packet
    ) {
        if (!packet ||
            Addresses::ComplexInventoryTransactionVTable == 0) {
            return nullptr;
        }

        for (const uintptr_t offset : kTransactionPointerOffsets) {
            const auto* field =
                reinterpret_cast<ComplexInventoryTransaction* const*>(
                    reinterpret_cast<uintptr_t>(packet) + offset
                    );
            if (!isReadableRange(field, sizeof(*field))) {
                continue;
            }

            ComplexInventoryTransaction* candidate = *field;
            if (!candidate ||
                !isReadableRange(candidate, sizeof(uintptr_t) + sizeof(int))) {
                continue;
            }

            const uintptr_t vtable =
                *reinterpret_cast<const uintptr_t*>(candidate);
            if (vtable == Addresses::ComplexInventoryTransactionVTable) {
                return candidate;
            }
        }

        return nullptr;
    }

    std::string getDiagnosticPath() {
        return FileUtil::getClientPath() + "Dupe-Debug.log";
    }

    void resetDiagnosticLog() {
        std::ofstream stream(getDiagnosticPath(), std::ios::trunc);
        if (stream.is_open()) {
            stream << "Dupe InventoryTransaction diagnostic\n";
        }
    }

    void writeDiagnostic(const char* format, ...) {
        char buffer[1024]{};
        va_list arguments;
        va_start(arguments, format);
        vsnprintf_s(
            buffer,
            sizeof(buffer),
            _TRUNCATE,
            format,
            arguments
        );
        va_end(arguments);

        std::ofstream stream(getDiagnosticPath(), std::ios::app);
        if (stream.is_open()) {
            stream << buffer << '\n';
        }
    }

    uint64_t hashBytes(const void* data, std::size_t size) {
        constexpr uint64_t offsetBasis = 1469598103934665603ULL;
        constexpr uint64_t prime = 1099511628211ULL;
        uint64_t hash = offsetBasis;
        const auto* bytes = static_cast<const uint8_t*>(data);
        for (std::size_t index = 0; index < size; ++index) {
            hash ^= bytes[index];
            hash *= prime;
        }
        return hash;
    }

    struct DescriptorPayloadSummary {
        bool readable = false;
        bool containsItems = false;
        bool containsLegacyItems = false;
        bool containsCustomName = false;
        uint8_t hasNetId = 0;
        int32_t blockRuntimeId = 0;
        std::size_t size = 0;
        std::size_t capacity = 0;
        uint64_t hash = 0;
        uint64_t prefix = 0;
    };

    bool descriptorPayloadContains(
        const uint8_t* payload,
        std::size_t payloadSize,
        const char* token
    ) {
        if (!payload || !token) {
            return false;
        }

        const std::size_t tokenSize = std::strlen(token);
        if (tokenSize == 0 || tokenSize > payloadSize) {
            return false;
        }

        return std::search(
            payload,
            payload + payloadSize,
            reinterpret_cast<const uint8_t*>(token),
            reinterpret_cast<const uint8_t*>(token) + tokenSize
        ) != payload + payloadSize;
    }

    DescriptorPayloadSummary inspectDescriptorPayload(
        const NetworkItemStackDescriptor& descriptor
    ) {
        DescriptorPayloadSummary summary{};
        const auto* raw = reinterpret_cast<const uint8_t*>(&descriptor);

        summary.hasNetId = raw[0x18];
        std::memcpy(
            &summary.blockRuntimeId,
            raw + 0x38,
            sizeof(summary.blockRuntimeId)
        );
        std::memcpy(&summary.size, raw + 0x50, sizeof(summary.size));
        std::memcpy(
            &summary.capacity,
            raw + 0x58,
            sizeof(summary.capacity)
        );

        constexpr std::size_t kMaximumDiagnosticPayload = 16 * 1024 * 1024;
        if (summary.size > kMaximumDiagnosticPayload ||
            summary.capacity < summary.size) {
            return summary;
        }

        const uint8_t* payload = raw + 0x40;
        if (summary.capacity >= 0x10) {
            std::memcpy(&payload, raw + 0x40, sizeof(payload));
        }

        if (summary.size != 0 &&
            !isReadableRange(payload, summary.size)) {
            return summary;
        }

        summary.readable = true;
        summary.hash = hashBytes(payload, summary.size);
        if (summary.size != 0) {
            std::memcpy(
                &summary.prefix,
                payload,
                std::min(summary.size, sizeof(summary.prefix))
            );
        }
        summary.containsItems = descriptorPayloadContains(
            payload,
            summary.size,
            "Items"
        );
        summary.containsLegacyItems = descriptorPayloadContains(
            payload,
            summary.size,
            "__Legacy__Items"
        );
        summary.containsCustomName = descriptorPayloadContains(
            payload,
            summary.size,
            "CustomName"
        );
        return summary;
    }

    bool descriptorPayloadsEqual(
        const DescriptorPayloadSummary& expected,
        const DescriptorPayloadSummary& actual
    ) {
        return expected.readable &&
            actual.readable &&
            expected.size == actual.size &&
            expected.hash == actual.hash &&
            expected.blockRuntimeId == actual.blockRuntimeId;
    }

    struct ComparableItemSummary {
        bool readable = false;
        std::size_t rootTagCount = 0;
        uint64_t nbtHash = 0;
        uintptr_t blockIdentity = 0;
        uint64_t canPlaceOnHash = 0;
        uint64_t canDestroyHash = 0;
    };

    bool inspectComparableItem(
        const ItemStack* stack,
        ComparableItemSummary& summary
    ) {
        summary = {};
        if (!stack || !stack->isValid()) {
            return false;
        }

        summary.blockIdentity = static_cast<uintptr_t>(stack->mBlock);
        summary.canPlaceOnHash = stack->mCanPlaceOnHash;
        summary.canDestroyHash = stack->mCanDestroyHash;
        if (stack->mUserData == 0) {
            summary.readable = true;
            return true;
        }

        const auto* tag = reinterpret_cast<const Tag*>(stack->mUserData);
        if (!isReadableRange(tag, sizeof(uintptr_t))) {
            return false;
        }
        summary.nbtHash = static_cast<uint64_t>(tag->hash());
        if (tag->getId() == Tag::Type::Compound) {
            const auto* compound = reinterpret_cast<const CompoundTag*>(tag);
            summary.rootTagCount = compound->mTags.size();
        }
        summary.readable = true;
        return true;
    }

    bool comparableItemPayloadsEqual(
        const ItemStack* expected,
        const ItemStack* actual,
        ComparableItemSummary& expectedSummary,
        ComparableItemSummary& actualSummary
    ) {
        if (!inspectComparableItem(expected, expectedSummary) ||
            !inspectComparableItem(actual, actualSummary) ||
            expected->item.get() != actual->item.get() ||
            expected->mAuxValue != actual->mAuxValue ||
            expectedSummary.blockIdentity != actualSummary.blockIdentity ||
            expectedSummary.canPlaceOnHash != actualSummary.canPlaceOnHash ||
            expectedSummary.canDestroyHash != actualSummary.canDestroyHash ||
            expected->mCanPlaceOn != actual->mCanPlaceOn ||
            expected->mCanDestroy != actual->mCanDestroy) {
            return false;
        }

        if (expected->mUserData == 0 || actual->mUserData == 0) {
            return expected->mUserData == actual->mUserData;
        }
        if (expectedSummary.nbtHash != actualSummary.nbtHash ||
            expectedSummary.rootTagCount != actualSummary.rootTagCount) {
            return false;
        }

        const auto* expectedTag =
            reinterpret_cast<const Tag*>(expected->mUserData);
        const auto* actualTag =
            reinterpret_cast<const Tag*>(actual->mUserData);
        return expectedTag->equals(*actualTag);
    }

    struct MaterialTakeDestinationDelta {
        int slot = -1;
        uint8_t finalCount = 0;
        int candidateCount = 0;
    };

    MaterialTakeDestinationDelta findMaterialTakeDestinationDelta(
        PlayerInventory* inventory,
        const ItemStack* expected,
        uint8_t transferCount,
        const std::array<uint8_t, kInventorySlotCount>& beforeCounts,
        const std::array<bool, kInventorySlotCount>& beforeExpectedMatches,
        const std::array<bool, kInventorySlotCount>& beforeEmptySlots
    ) {
        MaterialTakeDestinationDelta result{};
        if (!inventory || !inventory->container ||
            !expected || !expected->isValid() || transferCount == 0) {
            return result;
        }

        for (int slot = 0; slot < kInventorySlotCount; ++slot) {
            ItemStack* item = inventory->container->getItem(slot);
            if (!item || !item->isValid() || item->mCount == 0 ||
                item->item.get() != expected->item.get() ||
                item->mAuxValue != expected->mAuxValue) {
                continue;
            }

            ComparableItemSummary expectedWire{};
            ComparableItemSummary actualWire{};
            if (!comparableItemPayloadsEqual(
                expected,
                item,
                expectedWire,
                actualWire
            )) {
                continue;
            }

            const bool wasMatchingStack = beforeExpectedMatches[slot];
            const bool wasEmpty = beforeEmptySlots[slot];
            if (!wasMatchingStack && !wasEmpty) {
                continue;
            }
            const uint8_t beforeCount = wasMatchingStack
                ? beforeCounts[slot]
                : 0;
            if (item->mCount <= beforeCount ||
                static_cast<unsigned int>(item->mCount - beforeCount) !=
                static_cast<unsigned int>(transferCount)) {
                continue;
            }

            ++result.candidateCount;
            result.slot = slot;
            result.finalCount = item->mCount;
        }

        if (result.candidateCount != 1) {
            result.slot = -1;
            result.finalCount = 0;
        }
        return result;
    }

    bool appendNativeActionClone(
        ComplexInventoryTransaction* transaction,
        const InventorySource& ownerSource,
        std::size_t actionIndex,
        const InventorySource& cloneSource,
        uint32_t cloneSlot
    ) {
        if (!transaction) {
            return false;
        }

        auto ownerGroup = transaction->data.actions.find(ownerSource);
        if (ownerGroup == transaction->data.actions.end() ||
            actionIndex >= ownerGroup->second.size()) {
            return false;
        }

        ownerGroup->second.reserve(ownerGroup->second.size() + 1);

        InventoryAction& originalAction =
            ownerGroup->second[actionIndex];
        const InventorySource originalSource = originalAction.source;
        const uint32_t originalSlot = originalAction.slot;

        originalAction.source = cloneSource;
        originalAction.slot = cloneSlot;
        transaction->data.addAction(originalAction);

        ownerGroup = transaction->data.actions.find(ownerSource);
        if (ownerGroup == transaction->data.actions.end() ||
            actionIndex >= ownerGroup->second.size()) {
            return false;
        }

        ownerGroup->second[actionIndex].source = originalSource;
        ownerGroup->second[actionIndex].slot = originalSlot;
        return true;
    }

    bool normalizeAndAppendRichDirectWorldDrops(
        ComplexInventoryTransaction* transaction,
        std::vector<InventoryAction>* resultActions,
        std::vector<InventoryAction>* destinationActions,
        const InventorySource& resultSource,
        uint32_t resultSlot,
        const InventorySource& destinationSource,
        uint32_t destinationSlot,
        ItemStack& richResult,
        ItemStack* empty,
        int requestedCopies,
        bool directWorldOnly = false,
        bool stackingEnabled = false,
        int requestedStackCount = 1
    ) {
        if (!transaction ||
            !resultActions ||
            !destinationActions ||
            resultActions == destinationActions ||
            resultActions->size() != 1 ||
            destinationActions->size() != 1 ||
            !richResult.isValid() ||
            richResult.mCount == 0 ||
            !empty ||
            requestedCopies <= 0) {
            return false;
        }

        resultActions->clear();
        resultActions->emplace_back(
            resultSlot,
            &richResult,
            empty,
            resultSource.containerId,
            resultSource.type,
            resultSource.flags
        );
        destinationActions->clear();
        if (directWorldOnly) {
            InventoryAction firstDirectDrop(
                0,
                empty,
                &richResult,
                ContainerID::None,
                InventorySourceType::WorldInteraction,
                InventorySourceFlags::NoFlag
            );
            transaction->data.addAction(firstDirectDrop);
        }
        else {
            destinationActions->emplace_back(
                destinationSlot,
                empty,
                &richResult,
                destinationSource.containerId,
                destinationSource.type,
                destinationSource.flags
            );
        }

        const int appendedCopies = directWorldOnly
            ? requestedCopies - 1
            : requestedCopies;
        const int resultRepeatsPerDrop =
            stackingEnabled &&
            requestedStackCount > static_cast<int>(richResult.mCount)
            ? std::clamp(requestedStackCount, 1, 255) /
            static_cast<int>(richResult.mCount)
            : 1;
        for (int copyIndex = 0;
            copyIndex < appendedCopies;
            ++copyIndex) {
            for (int repeatIndex = 0;
                repeatIndex < resultRepeatsPerDrop;
                ++repeatIndex) {
                InventoryAction repeatedResult(
                    resultSlot,
                    &richResult,
                    empty,
                    resultSource.containerId,
                    resultSource.type,
                    resultSource.flags
                );
                transaction->data.addAction(repeatedResult);
            }

            ItemStack stackedWorldResult(richResult);
            NativeItemStackResetGuard stackedWorldResultGuard(
                &stackedWorldResult
            );
            stackedWorldResult.mCount = stackingEnabled
                ? static_cast<uint8_t>(resultRepeatsPerDrop *
                    static_cast<int>(richResult.mCount))
                : richResult.mCount;

            InventoryAction directDrop(
                0,
                empty,
                &stackedWorldResult,
                ContainerID::None,
                InventorySourceType::WorldInteraction,
                InventorySourceFlags::NoFlag
            );
            transaction->data.addAction(directDrop);
        }
        return true;
    }

    bool normalizeAndAppendRichInventoryPairs(
        std::vector<InventoryAction>* resultActions,
        std::vector<InventoryAction>* destinationActions,
        const InventorySource& resultSource,
        uint32_t resultSlot,
        const InventorySource& destinationSource,
        uint32_t destinationSlot,
        ItemStack& richResult,
        ItemStack* empty,
        const std::vector<int>& additionalDestinationSlots,
        bool stackingEnabled = false,
        int requestedStackCount = 1
    ) {
        if (!resultActions ||
            !destinationActions ||
            resultActions == destinationActions ||
            resultActions->size() != 1 ||
            destinationActions->size() != 1 ||
            !richResult.isValid() ||
            richResult.mCount == 0 ||
            !empty) {
            return false;
        }

        const int actionRepeatsPerDestination =
            stackingEnabled &&
            requestedStackCount > static_cast<int>(richResult.mCount)
            ? std::clamp(requestedStackCount, 1, 255) /
            static_cast<int>(richResult.mCount)
            : 1;

        resultActions->clear();
        destinationActions->clear();
        resultActions->reserve(
            1 + additionalDestinationSlots.size() *
            static_cast<std::size_t>(actionRepeatsPerDestination)
        );
        destinationActions->reserve(
            1 + additionalDestinationSlots.size() *
            static_cast<std::size_t>(actionRepeatsPerDestination)
        );

        resultActions->emplace_back(
            resultSlot,
            &richResult,
            empty,
            resultSource.containerId,
            resultSource.type,
            resultSource.flags
        );
        destinationActions->emplace_back(
            destinationSlot,
            empty,
            &richResult,
            destinationSource.containerId,
            destinationSource.type,
            destinationSource.flags
        );

        for (const int additionalSlot : additionalDestinationSlots) {
            for (int repeatIndex = 0;
                repeatIndex < actionRepeatsPerDestination;
                ++repeatIndex) {
                resultActions->emplace_back(
                    resultSlot,
                    &richResult,
                    empty,
                    resultSource.containerId,
                    resultSource.type,
                    resultSource.flags
                );
                destinationActions->emplace_back(
                    static_cast<uint32_t>(additionalSlot),
                    empty,
                    &richResult,
                    destinationSource.containerId,
                    destinationSource.type,
                    destinationSource.flags
                );
            }
        }
        return true;
    }

    int getItemId(const ItemStack& stack) {
        Item* item = stack.item.get();
        return item ? item->itemId : 0;
    }

    std::string getItemIdentifier(const ItemStack& stack) {
        Item* item = stack.item.get();
        if (!item) {
            return {};
        }

        std::string identifier = item->rawName;
        if (identifier.empty()) {
            identifier = item->texture_name;
        }
        if (identifier.empty()) {
            identifier = item->rawName;
        }

        std::transform(
            identifier.begin(),
            identifier.end(),
            identifier.begin(),
            [](unsigned char value) {
                return static_cast<char>(std::tolower(value));
            }
        );
        return identifier;
    }

    bool isShulkerBox(const ItemStack& stack) {
        return stack.isValid() &&
            getItemIdentifier(stack).find("shulker_box") !=
            std::string::npos;
    }

    const ListTag* findShulkerItemsList(const CompoundTag* root) {
        if (!root) {
            return nullptr;
        }

        auto findDirect = [](const CompoundTag* compound) -> const ListTag* {
            if (!compound) {
                return nullptr;
            }
            auto itemList = compound->mTags.find("Items");
            if (itemList == compound->mTags.end()) {
                itemList = compound->mTags.find("items");
            }
            if (itemList == compound->mTags.end()) {
                itemList = compound->mTags.find("__Legacy__Items");
            }
            return itemList == compound->mTags.end()
                ? nullptr
                : itemList->second.get<ListTag>();
            };

        if (const ListTag* direct = findDirect(root)) {
            return direct;
        }

        for (const char* key : { "tag", "BlockEntityTag" }) {
            auto nestedEntry = root->mTags.find(key);
            if (nestedEntry == root->mTags.end()) {
                continue;
            }
            const CompoundTag* nested =
                nestedEntry->second.get<CompoundTag>();
            if (const ListTag* nestedItems = findDirect(nested)) {
                return nestedItems;
            }
            if (!nested) {
                continue;
            }
            auto blockEntity = nested->mTags.find("BlockEntityTag");
            if (blockEntity != nested->mTags.end()) {
                if (const ListTag* blockItems = findDirect(
                    blockEntity->second.get<CompoundTag>())) {
                    return blockItems;
                }
            }
        }

        return nullptr;
    }

    int getShulkerContentEntryCount(const ItemStack& stack) {
        if (!stack.isValid() || stack.mUserData == 0) {
            return 0;
        }
        const auto* root = reinterpret_cast<const CompoundTag*>(
            stack.mUserData
            );
        const ListTag* items = findShulkerItemsList(root);
        return items ? static_cast<int>(items->mList.size()) : 0;
    }

    std::size_t getFirstMapColorsSize(const ItemStack& stack) {
        if (!stack.isValid() || stack.mUserData == 0) return 0;
        const auto* root = reinterpret_cast<const CompoundTag*>(stack.mUserData);
        const ListTag* items = findShulkerItemsList(root);
        if (!items || items->mList.empty() || !items->mList.front() ||
            items->mList.front()->getId() != Tag::Compound) return 0;
        const auto* item = static_cast<const CompoundTag*>(items->mList.front().get());
        auto tagIt = item->mTags.find("tag");
        if (tagIt == item->mTags.end()) return 0;
        const CompoundTag* mapTag = tagIt->second.get<CompoundTag>();
        if (!mapTag) return 0;
        auto colorsIt = mapTag->mTags.find("Colors");
        if (colorsIt == mapTag->mTags.end()) return 0;
        const ByteArrayTag* colors = colorsIt->second.get<ByteArrayTag>();
        return colors ? colors->data.size() : 0;
    }

    ListTag* findMutableShulkerItemsList(CompoundTag* root) {
        return const_cast<ListTag*>(findShulkerItemsList(root));
    }

    struct MutableShulkerItemsEntry {
        CompoundTag* parent = nullptr;
        TagMap::iterator iterator{};
    };

    bool findMutableShulkerItemsEntry(
        CompoundTag* root,
        MutableShulkerItemsEntry& result
    ) {
        if (!root) {
            return false;
        }

        auto findDirect = [&result](CompoundTag* compound) -> bool {
            if (!compound) {
                return false;
            }
            auto iterator = compound->mTags.find("Items");
            if (iterator == compound->mTags.end()) {
                iterator = compound->mTags.find("items");
            }
            if (iterator == compound->mTags.end()) {
                iterator = compound->mTags.find("__Legacy__Items");
            }
            if (iterator == compound->mTags.end() ||
                !iterator->second.get<ListTag>()) {
                return false;
            }
            result.parent = compound;
            result.iterator = iterator;
            return true;
            };

        if (findDirect(root)) {
            return true;
        }
        for (const char* key : { "tag", "BlockEntityTag" }) {
            auto nestedEntry = root->mTags.find(key);
            if (nestedEntry == root->mTags.end()) {
                continue;
            }
            CompoundTag* nested = nestedEntry->second.get<CompoundTag>();
            if (findDirect(nested)) {
                return true;
            }
            if (!nested) {
                continue;
            }
            auto blockEntry = nested->mTags.find("BlockEntityTag");
            if (blockEntry != nested->mTags.end() &&
                findDirect(blockEntry->second.get<CompoundTag>())) {
                return true;
            }
        }
        return false;
    }

    bool stripExportOnlyLegacyItems(
        CompoundTag* root,
        std::size_t& modernCount,
        std::size_t& legacyCount
    ) {
        modernCount = 0;
        legacyCount = 0;
        if (!root) return false;

        const auto modern = root->mTags.find("Items");
        const auto legacy = root->mTags.find("__Legacy__Items");
        if (modern == root->mTags.end() || legacy == root->mTags.end())
            return false;

        const ListTag* modernList = modern->second.get<ListTag>();
        const ListTag* legacyList = legacy->second.get<ListTag>();
        if (!modernList || !legacyList) return false;

        modernCount = modernList->mList.size();
        legacyCount = legacyList->mList.size();
        root->mTags.erase(legacy);
        return true;
    }

    bool graftNativeShulkerContents(
        CompoundTag* targetRoot,
        CompoundTag* donorRoot,
        std::size_t& donorItemCount,
        std::size_t& copiedListCount
    ) {
        donorItemCount = 0;
        copiedListCount = 0;

        MutableShulkerItemsEntry targetEntry;
        MutableShulkerItemsEntry donorEntry;
        if (!findMutableShulkerItemsEntry(targetRoot, targetEntry) ||
            !findMutableShulkerItemsEntry(donorRoot, donorEntry) ||
            !targetEntry.parent ||
            !donorEntry.parent) {
            return false;
        }

        auto listHasLegacySlotSchema = [](const ListTag* list) {
            if (!list || list->mType != Tag::Compound ||
                list->mList.empty() || !list->mList.front() ||
                list->mList.front()->getId() != Tag::Compound) {
                return false;
            }
            const auto* first = static_cast<const CompoundTag*>(
                list->mList.front().get()
                );
            return first->mTags.find("Slot") != first->mTags.end() &&
                first->mTags.find("id") != first->mTags.end();
            };

        const auto donorLegacyIterator =
            donorEntry.parent->mTags.find("__Legacy__Items");
        const bool donorHasLegacyItems =
            donorLegacyIterator != donorEntry.parent->mTags.end() &&
            donorLegacyIterator->second.get<ListTag>();

        const ListTag* canonicalDonorItems = donorHasLegacyItems
            ? donorLegacyIterator->second.get<ListTag>()
            : donorEntry.iterator->second.get<ListTag>();
        if (!canonicalDonorItems ||
            canonicalDonorItems->mType != Tag::Compound) {
            return false;
        }

        auto convertItems = [](
            const ListTag* source,
            bool legacyFormat,
            std::vector<std::unique_ptr<Tag>>& result,
            std::size_t& failedIndex,
            const char*& failedReason
            ) -> bool {
                result.clear();
                failedIndex = 0;
                failedReason = "none";
                if (!source || source->mType != Tag::Compound) {
                    failedReason = "source-list-invalid";
                    return false;
                }
                result.reserve(source->mList.size());
                for (std::size_t index = 0; index < source->mList.size(); ++index) {
                    failedIndex = index;
                    const auto& sourceItem = source->mList[index];
                    if (!sourceItem || sourceItem->getId() != Tag::Compound) {
                        failedReason = "entry-not-compound";
                        return false;
                    }
                    std::unique_ptr<Tag> copiedTag = sourceItem->copy();
                    if (!copiedTag || copiedTag->getId() != Tag::Compound) {
                        failedReason = "entry-copy-failed";
                        return false;
                    }
                    auto* copied = static_cast<CompoundTag*>(copiedTag.get());
                    if (legacyFormat) {
                        copied->mTags.erase("Name");
                        copied->mTags.erase("name");
                        if (copied->mTags.find("id") == copied->mTags.end() &&
                            copied->mTags.find("Id") == copied->mTags.end()) {
                            failedReason = "legacy-id-missing";
                            return false;
                        }
                    }
                    else {
                        copied->mTags.erase("id");
                        copied->mTags.erase("Id");
                        copied->mTags.erase("tag");
                        if (copied->mTags.find("Slot") == copied->mTags.end() &&
                            copied->mTags.find("slot") == copied->mTags.end()) {
                            if (!copied->putNative(
                                "Slot",
                                Tag::createNative<ByteTag>(
                                    static_cast<uint8_t>(index)
                                ))) {
                                failedReason = "network-slot-create-failed";
                                return false;
                            }
                        }
                        if (copied->mTags.find("Name") == copied->mTags.end() &&
                            copied->mTags.find("name") == copied->mTags.end()) {
                            failedReason = "network-name-missing";
                            return false;
                        }
                    }
                    result.emplace_back(std::move(copiedTag));
                }
                return true;
            };

        bool counted = false;
        for (const char* key : { "Items", "items", "__Legacy__Items" }) {
            auto targetIterator = targetEntry.parent->mTags.find(key);
            if (targetIterator == targetEntry.parent->mTags.end() ||
                !targetIterator->second.get<ListTag>()) {
                continue;
            }

            const bool legacyFormat =
                std::strcmp(key, "__Legacy__Items") == 0 ||
                listHasLegacySlotSchema(targetIterator->second.get<ListTag>());

            const ListTag* sourceItems = canonicalDonorItems;
            if (legacyFormat) {
                if (donorHasLegacyItems)
                    sourceItems = donorLegacyIterator->second.get<ListTag>();
            }
            else {
                auto donorNetworkIterator = donorEntry.parent->mTags.find("Items");
                if (donorNetworkIterator == donorEntry.parent->mTags.end())
                    donorNetworkIterator = donorEntry.parent->mTags.find("items");
                if (donorNetworkIterator != donorEntry.parent->mTags.end() &&
                    donorNetworkIterator->second.get<ListTag>()) {
                    sourceItems = donorNetworkIterator->second.get<ListTag>();
                }
            }
            if (!counted) {
                donorItemCount = canonicalDonorItems->mList.size();
                counted = true;
            }

            std::vector<std::unique_ptr<Tag>> copiedItems;
            std::size_t failedIndex = 0;
            const char* failedReason = "none";
            const bool converted = convertItems(
                sourceItems,
                legacyFormat,
                copiedItems,
                failedIndex,
                failedReason
            );
            ListTag* targetItems = targetIterator->second.get<ListTag>();
            if (!converted || !targetItems) {
                writeDiagnostic(
                    "[ANVILDUPE2_DUAL_FORMAT_ABORT] targetList=%s format=%s donorEntries=%llu failedIndex=%llu reason=%s targetListValid=%d",
                    key,
                    legacyFormat ? "legacy" : "network",
                    static_cast<unsigned long long>(
                        sourceItems ? sourceItems->mList.size() : 0
                        ),
                    static_cast<unsigned long long>(failedIndex),
                    failedReason,
                    targetItems ? 1 : 0
                );
                return false;
            }
            targetItems->mType = Tag::Compound;
            targetItems->mList.clear();
            if (targetItems->mList.capacity() < copiedItems.size()) {
                targetItems->mList.reserve(copiedItems.size());
            }
            for (auto& copiedItem : copiedItems) {
                targetItems->mList.emplace_back(std::move(copiedItem));
            }
            ++copiedListCount;
        }

        return counted && copiedListCount > 0;
    }

    struct FirstShulkerItemFieldCopy {
        std::size_t targetItemCount = 0;
        std::size_t donorItemCount = 0;
        std::size_t changedListCount = 0;
        std::string key;
        std::string beforeValue;
        std::string afterValue;
    };

    std::string describeScalarNativeTag(const Tag* tag) {
        if (!tag) {
            return "missing";
        }

        switch (tag->getId()) {
        case Tag::Byte:
            return std::to_string(
                static_cast<unsigned int>(
                    static_cast<const ByteTag*>(tag)->data
                    )
            );
        case Tag::Short:
            return std::to_string(
                static_cast<const ShortTag*>(tag)->data
            );
        case Tag::Int:
            return std::to_string(
                static_cast<const IntTag*>(tag)->data
            );
        case Tag::String: {
            std::string value = static_cast<const StringTag*>(tag)->data;
            if (value.size() > 96) {
                value.resize(96);
                value += "...";
            }
            return value;
        }
        default:
            return "type-" + std::to_string(
                static_cast<unsigned int>(tag->getId())
            );
        }
    }

    template <std::size_t FieldKeyCount>
    bool copyFirstShulkerItemField(
        CompoundTag* targetRoot,
        CompoundTag* donorRoot,
        const std::array<const char*, FieldKeyCount>& fieldKeys,
        FirstShulkerItemFieldCopy& result
    ) {
        result = {};

        MutableShulkerItemsEntry targetEntry;
        MutableShulkerItemsEntry donorEntry;
        if (!findMutableShulkerItemsEntry(targetRoot, targetEntry) ||
            !findMutableShulkerItemsEntry(donorRoot, donorEntry) ||
            !targetEntry.parent ||
            !donorEntry.parent) {
            return false;
        }

        struct PendingFieldCopy {
            CompoundTag* targetItem = nullptr;
            std::string key;
            std::string beforeValue;
            std::string afterValue;
            std::unique_ptr<Tag> copiedField;
        };
        std::vector<PendingFieldCopy> pendingCopies;

        bool counted = false;
        for (const char* listKey : {
                 "Items",
                 "items",
                 "__Legacy__Items"
            }) {
            auto targetListEntry = targetEntry.parent->mTags.find(listKey);
            if (targetListEntry == targetEntry.parent->mTags.end()) {
                continue;
            }

            ListTag* targetItems = targetListEntry->second.get<ListTag>();
            auto donorListEntry = donorEntry.parent->mTags.find(listKey);
            if (donorListEntry == donorEntry.parent->mTags.end() ||
                !donorListEntry->second.get<ListTag>()) {
                return false;
            }
            const ListTag* donorItems =
                donorListEntry->second.get<ListTag>();
            if (!targetItems ||
                !donorItems ||
                targetItems->mType != Tag::Compound ||
                donorItems->mType != Tag::Compound ||
                targetItems->mList.empty() ||
                donorItems->mList.empty() ||
                targetItems->mList.size() > 27 ||
                donorItems->mList.size() > 27 ||
                !targetItems->mList.front() ||
                !donorItems->mList.front() ||
                targetItems->mList.front()->getId() != Tag::Compound ||
                donorItems->mList.front()->getId() != Tag::Compound) {
                return false;
            }

            if (!counted) {
                result.targetItemCount = targetItems->mList.size();
                result.donorItemCount = donorItems->mList.size();
                counted = true;
            }

            auto* targetItem = static_cast<CompoundTag*>(
                targetItems->mList.front().get()
                );
            const auto* donorItem = static_cast<const CompoundTag*>(
                donorItems->mList.front().get()
                );

            TagMap::iterator targetField{};
            TagMap::const_iterator donorField{};
            bool foundField = false;
            for (const char* fieldKey : fieldKeys) {
                auto targetCandidate = targetItem->mTags.find(fieldKey);
                auto donorCandidate = donorItem->mTags.find(fieldKey);
                if (targetCandidate == targetItem->mTags.end() ||
                    donorCandidate == donorItem->mTags.end() ||
                    !targetCandidate->second.get() ||
                    !donorCandidate->second.get() ||
                    targetCandidate->second->getId() !=
                    donorCandidate->second->getId()) {
                    continue;
                }
                targetField = targetCandidate;
                donorField = donorCandidate;
                foundField = true;
                break;
            }
            if (!foundField) {
                return false;
            }

            const std::string selectedKey = targetField->first;
            const std::string beforeValue =
                describeScalarNativeTag(targetField->second.get());
            const std::string afterValue =
                describeScalarNativeTag(donorField->second.get());
            std::unique_ptr<Tag> copiedField = donorField->second->copy();
            if (!copiedField ||
                copiedField->getId() != donorField->second->getId()) {
                return false;
            }

            PendingFieldCopy pending;
            pending.targetItem = targetItem;
            pending.key = selectedKey;
            pending.beforeValue = beforeValue;
            pending.afterValue = afterValue;
            pending.copiedField = std::move(copiedField);
            pendingCopies.emplace_back(std::move(pending));
        }

        if (!counted || pendingCopies.empty()) {
            return false;
        }

        for (PendingFieldCopy& pending : pendingCopies) {
            if (!pending.targetItem ||
                !pending.copiedField ||
                !pending.targetItem->putNative(
                    pending.key,
                    std::move(pending.copiedField)
                )) {
                return false;
            }
            if (result.changedListCount == 0) {
                result.key = pending.key;
                result.beforeValue = pending.beforeValue;
                result.afterValue = pending.afterValue;
            }
            else if (result.key != pending.key) {
                result.key = "mixed";
            }
            ++result.changedListCount;
        }

        return result.changedListCount == pendingCopies.size();
    }

    bool copyFirstShulkerItemIdentity(
        CompoundTag* targetRoot,
        CompoundTag* donorRoot,
        FirstShulkerItemFieldCopy& result
    ) {
        result = {};
        MutableShulkerItemsEntry targetEntry;
        MutableShulkerItemsEntry donorEntry;
        if (!findMutableShulkerItemsEntry(targetRoot, targetEntry) ||
            !findMutableShulkerItemsEntry(donorRoot, donorEntry) ||
            !targetEntry.parent || !donorEntry.parent) return false;

        auto donorLegacy = donorEntry.parent->mTags.find("__Legacy__Items");
        const ListTag* donorItems = donorLegacy != donorEntry.parent->mTags.end()
            ? donorLegacy->second.get<ListTag>()
            : donorEntry.iterator->second.get<ListTag>();
        if (!donorItems || donorItems->mType != Tag::Compound ||
            donorItems->mList.empty() || !donorItems->mList.front() ||
            donorItems->mList.front()->getId() != Tag::Compound) return false;
        const auto* donorItem = static_cast<const CompoundTag*>(
            donorItems->mList.front().get()
            );

        result.donorItemCount = donorItems->mList.size();
        for (const char* listKey : { "Items", "items", "__Legacy__Items" }) {
            auto targetListEntry = targetEntry.parent->mTags.find(listKey);
            if (targetListEntry == targetEntry.parent->mTags.end()) continue;
            ListTag* targetItems = targetListEntry->second.get<ListTag>();
            if (!targetItems || targetItems->mType != Tag::Compound ||
                targetItems->mList.empty() || !targetItems->mList.front() ||
                targetItems->mList.front()->getId() != Tag::Compound) return false;
            if (result.targetItemCount == 0)
                result.targetItemCount = targetItems->mList.size();

            auto* targetItem = static_cast<CompoundTag*>(targetItems->mList.front().get());
            const bool legacy = std::strcmp(listKey, "__Legacy__Items") == 0 ||
                targetItem->mTags.find("Slot") != targetItem->mTags.end();
            const std::array<const char*, 2> keys = legacy
                ? std::array<const char*, 2>{"id", "Id"}
            : std::array<const char*, 2>{ "Name", "name" };
            TagMap::iterator targetField{};
            TagMap::const_iterator donorField{};
            bool found = false;
            for (const char* key : keys) {
                auto targetCandidate = targetItem->mTags.find(key);
                auto donorCandidate = donorItem->mTags.find(key);
                if (targetCandidate != targetItem->mTags.end() &&
                    donorCandidate != donorItem->mTags.end() &&
                    targetCandidate->second->getId() == donorCandidate->second->getId()) {
                    targetField = targetCandidate;
                    donorField = donorCandidate;
                    found = true;
                    break;
                }
            }
            if (!found) return false;
            std::unique_ptr<Tag> copied = donorField->second->copy();
            if (!copied) return false;
            const std::string before = describeScalarNativeTag(targetField->second.get());
            const std::string after = describeScalarNativeTag(donorField->second.get());
            const std::string key = targetField->first;
            if (!targetItem->putNative(key, std::move(copied))) return false;
            if (result.changedListCount == 0) {
                result.key = key;
                result.beforeValue = before;
                result.afterValue = after;
            }
            else if (result.key != key) result.key = "mixed";
            ++result.changedListCount;
        }
        return result.changedListCount > 0;
    }

    bool keepFirstShulkerItemInEveryNativeList(
        CompoundTag* root,
        std::size_t& originalItemCount,
        std::size_t& changedListCount
    ) {
        originalItemCount = 0;
        changedListCount = 0;

        MutableShulkerItemsEntry entry;
        if (!findMutableShulkerItemsEntry(root, entry) || !entry.parent) {
            return false;
        }

        bool foundList = false;
        for (const char* key : {
                 "Items",
                 "items",
                 "__Legacy__Items"
            }) {
            auto iterator = entry.parent->mTags.find(key);
            if (iterator == entry.parent->mTags.end()) {
                continue;
            }

            ListTag* items = iterator->second.get<ListTag>();
            if (!items) {
                return false;
            }
            if (!foundList) {
                originalItemCount = items->mList.size();
            }
            foundList = true;

            if (items->mList.size() > 1) {
                items->mList.erase(
                    items->mList.begin() + 1,
                    items->mList.end()
                );
                ++changedListCount;
            }
        }
        return foundList;
    }

    struct MutableEnchantEntry {
        CompoundTag* parent = nullptr;
        TagMap::iterator iterator{};
        std::size_t enchantmentCount = 0;
    };

    bool findMutableNativeEnchantEntry(
        CompoundTag* root,
        MutableEnchantEntry& result,
        int depth = 0
    ) {
        if (!root || depth > 4) {
            return false;
        }

        for (const char* key : {
                 "ench",
                 "Enchantments",
                 "enchantments"
            }) {
            auto entry = root->mTags.find(key);
            if (entry == root->mTags.end()) {
                continue;
            }
            ListTag* list = entry->second.get<ListTag>();
            if (!list || list->mList.empty()) {
                continue;
            }
            result.parent = root;
            result.iterator = entry;
            result.enchantmentCount = list->mList.size();
            return true;
        }

        for (const char* key : {
                 "tag",
                 "components",
                 "minecraft:enchantments"
            }) {
            auto entry = root->mTags.find(key);
            if (entry == root->mTags.end()) {
                continue;
            }
            CompoundTag* nested = entry->second.get<CompoundTag>();
            if (nested && findMutableNativeEnchantEntry(
                nested,
                result,
                depth + 1
            )) {
                return true;
            }
        }
        return false;
    }

    std::size_t getNativeEnchantEntryCount(const ItemStack& stack) {
        if (!stack.isValid() || stack.mUserData == 0) {
            return 0;
        }
        MutableEnchantEntry entry;
        auto* root = reinterpret_cast<CompoundTag*>(stack.mUserData);
        return findMutableNativeEnchantEntry(root, entry)
            ? entry.enchantmentCount
            : 0;
    }

    bool graftNativeEnchantEntry(
        ItemStack& output,
        const ItemStack& donor,
        std::size_t& enchantmentCount,
        bool preserveOutputNbt
    ) {
        enchantmentCount = 0;
        if (!output.isValid() || output.mUserData == 0 ||
            !donor.isValid() || donor.mUserData == 0 ||
            !canCloneNativeItemStack()) {
            return false;
        }

        ItemStack donorCopy(donor);
        NativeItemStackResetGuard donorGuard(&donorCopy);
        auto* donorRoot = reinterpret_cast<CompoundTag*>(donorCopy.mUserData);
        auto* outputRoot = reinterpret_cast<CompoundTag*>(output.mUserData);
        if (!donorRoot || !outputRoot) {
            return false;
        }

        MutableEnchantEntry donorEntry;
        if (!findMutableNativeEnchantEntry(donorRoot, donorEntry)) {
            return false;
        }
        enchantmentCount = donorEntry.enchantmentCount;

        auto enchantNode = donorEntry.parent->mTags.extract(
            donorEntry.iterator
        );
        if (enchantNode.empty()) {
            enchantmentCount = 0;
            return false;
        }

        if (preserveOutputNbt) {
            outputRoot->mTags.erase("ench");
            outputRoot->mTags.erase("Enchantments");
            outputRoot->mTags.erase("enchantments");
        }
        else {
            outputRoot->mTags.clear();
        }
        enchantNode.key() = "ench";
        const auto inserted = outputRoot->mTags.insert(
            std::move(enchantNode)
        );
        if (!inserted.inserted ||
            getNativeEnchantEntryCount(output) != enchantmentCount) {
            enchantmentCount = 0;
            return false;
        }
        return true;
    }

    bool readContainerEntrySlot(
        const CompoundTag* compound,
        int& slot
    ) {
        if (!compound) {
            return false;
        }

        auto slotEntry = compound->mTags.find("Slot");
        if (slotEntry == compound->mTags.end()) {
            slotEntry = compound->mTags.find("slot");
        }
        if (slotEntry == compound->mTags.end()) {
            return false;
        }

        if (const auto* value = slotEntry->second.get<ByteTag>()) {
            slot = static_cast<int>(value->data);
            return true;
        }
        if (const auto* value = slotEntry->second.get<ShortTag>()) {
            slot = static_cast<int>(value->data);
            return true;
        }
        if (const auto* value = slotEntry->second.get<IntTag>()) {
            slot = value->data;
            return true;
        }
        if (const auto* value = slotEntry->second.get<Int64Tag>()) {
            slot = static_cast<int>(value->data);
            return true;
        }
        return false;
    }

    bool collectContainerEntrySlots(
        const ListTag* items,
        std::vector<int>& slots
    ) {
        slots.clear();
        if (!items ||
            items->mList.empty() ||
            items->mList.size() > 27) {
            return false;
        }

        std::array<bool, 27> seen{};
        slots.reserve(items->mList.size());
        for (const std::unique_ptr<Tag>& entry : items->mList) {
            if (!entry || entry->getId() != Tag::Type::Compound) {
                slots.clear();
                return false;
            }

            const auto* compound =
                reinterpret_cast<const CompoundTag*>(entry.get());
            int slot = -1;
            if (!readContainerEntrySlot(compound, slot) ||
                slot < 0 || slot >= 27 || seen[slot]) {
                slots.clear();
                return false;
            }

            seen[slot] = true;
            slots.push_back(slot);
        }
        return true;
    }

    bool collectShulkerContainerEntrySlots(
        const ItemStack& stack,
        std::vector<int>& slots
    ) {
        slots.clear();
        if (!stack.isValid() || stack.mUserData == 0) {
            return false;
        }
        const auto* root = reinterpret_cast<const CompoundTag*>(
            stack.mUserData
            );
        return collectContainerEntrySlots(
            findShulkerItemsList(root),
            slots
        );
    }

    struct NbtComplexity {
        uint64_t bytes = 0;
        uint64_t nodes = 0;
        int maxDepth = 0;
    };

    constexpr uint64_t kNbtInspectionByteLimit =
        64ULL * 1024ULL * 1024ULL;
    constexpr uint64_t kNbtInspectionNodeLimit = 1000000ULL;
    constexpr int kNbtInspectionDepthLimit = 512;

    bool addNbtBytes(NbtComplexity& complexity, uint64_t bytes) {
        if (bytes >
            kNbtInspectionByteLimit - complexity.bytes) {
            return false;
        }
        complexity.bytes += bytes;
        return true;
    }

    bool inspectNbtTag(
        const Tag* tag,
        int depth,
        NbtComplexity& complexity
    ) {
        if (!tag ||
            depth > kNbtInspectionDepthLimit ||
            complexity.nodes >= kNbtInspectionNodeLimit) {
            return false;
        }

        ++complexity.nodes;
        complexity.maxDepth = std::max(
            complexity.maxDepth,
            depth
        );
        if (!addNbtBytes(complexity, 8)) {
            return false;
        }

        switch (tag->getId()) {
        case Tag::Type::End:
        case Tag::Type::Byte:
            return addNbtBytes(complexity, 1);
        case Tag::Type::Short:
            return addNbtBytes(complexity, 2);
        case Tag::Type::Int:
        case Tag::Type::Float:
            return addNbtBytes(complexity, 4);
        case Tag::Type::Int64:
        case Tag::Type::Double:
            return addNbtBytes(complexity, 8);
        case Tag::Type::ByteArray: {
            const auto* value =
                reinterpret_cast<const ByteArrayTag*>(tag);
            return addNbtBytes(
                complexity,
                static_cast<uint64_t>(value->data.size())
            );
        }
        case Tag::Type::String: {
            const auto* value =
                reinterpret_cast<const StringTag*>(tag);
            return addNbtBytes(
                complexity,
                static_cast<uint64_t>(value->data.size())
            );
        }
        case Tag::Type::IntArray: {
            const auto* value =
                reinterpret_cast<const IntArrayTag*>(tag);
            const uint64_t count =
                static_cast<uint64_t>(value->data.size());
            if (count > kNbtInspectionByteLimit / sizeof(int)) {
                return false;
            }
            return addNbtBytes(
                complexity,
                count * sizeof(int)
            );
        }
        case Tag::Type::List: {
            const auto* list =
                reinterpret_cast<const ListTag*>(tag);
            for (const std::unique_ptr<Tag>& child : list->mList) {
                if (!inspectNbtTag(
                    child.get(),
                    depth + 1,
                    complexity
                )) {
                    return false;
                }
            }
            return true;
        }
        case Tag::Type::Compound: {
            const auto* compound =
                reinterpret_cast<const CompoundTag*>(tag);
            for (const auto& [key, value] : compound->mTags) {
                if (!addNbtBytes(
                    complexity,
                    static_cast<uint64_t>(key.size()) + 4
                ) ||
                    !inspectNbtTag(
                        value.get(),
                        depth + 1,
                        complexity
                    )) {
                    return false;
                }
            }
            return true;
        }
        default:
            return false;
        }
    }

    bool inspectItemNbt(
        const ItemStack& stack,
        NbtComplexity& complexity
    ) {
        if (!stack.isValid()) {
            return false;
        }
        if (stack.mUserData == 0) {
            return true;
        }
        return inspectNbtTag(
            reinterpret_cast<const Tag*>(stack.mUserData),
            0,
            complexity
        );
    }

    std::unique_ptr<CompoundTag> saveNativeItemEntry(
        const ItemStack& stack
    ) {
        if (!stack.isValid() ||
            !isExecutableAddress(Addresses::ItemStackBase_save)) {
            return nullptr;
        }

        struct SaveContextView {
            uint8_t saveUseCase = 2;
        } context;

        using SaveFunction = std::unique_ptr<CompoundTag>* (__fastcall*)(
            const ItemStackBase*,
            std::unique_ptr<CompoundTag>*,
            const SaveContextView*
            );
        const auto saveFunction = reinterpret_cast<SaveFunction>(
            Addresses::ItemStackBase_save
            );

        std::unique_ptr<CompoundTag> entry;
        saveFunction(&stack, &entry, &context);
        return entry;
    }

    bool rebuildNativeItemStack(
        const ItemStack& source,
        ItemStack& rebuilt
    ) {
        if (!source.isValid() ||
            !isExecutableAddress(Addresses::ItemStack_loadItem)) {
            return false;
        }

        std::unique_ptr<CompoundTag> persisted =
            saveNativeItemEntry(source);
        if (!persisted) {
            return false;
        }

        dupeLoadItem(rebuilt, *persisted);
        if (!rebuilt.isValid() ||
            rebuilt.item.get() != source.item.get() ||
            rebuilt.mAuxValue != source.mAuxValue ||
            (source.mUserData != 0 && rebuilt.mUserData == 0)) {
            return false;
        }

        rebuilt.item = source.item;
        rebuilt.mBlock = source.mBlock;
        rebuilt.mAuxValue = source.mAuxValue;
        rebuilt.mCount = source.mCount;
        rebuilt.mValid = source.mValid;
        rebuilt.mNetIdVariant = source.mNetIdVariant;
        return true;
    }

    bool putNativeByte(
        CompoundTag* compound,
        const char* key,
        uint8_t value
    ) {
        if (!compound || !key ||
            !isExecutableAddress(Addresses::CompoundTag_putByte)) {
            return false;
        }

        using PutByteFunction = void* (__fastcall*)(
            CompoundTag*,
            std::string,
            uint8_t
            );
        const auto putByteFunction = reinterpret_cast<PutByteFunction>(
            Addresses::CompoundTag_putByte
            );
        return putByteFunction(compound, std::string(key), value) != nullptr;
    }

    bool convertContainerItemEntryToLegacy(
        CompoundTag* entry,
        const ItemStack& source,
        uint8_t slot
    ) {
        if (!entry || !source.isValid()) return false;

        entry->mTags.erase("Block");
        entry->mTags.erase("Name");
        entry->mTags.erase("WasPickedUp");
        entry->mTags.erase("id");

        auto numericId = Tag::createNative<ShortTag>(
            static_cast<int16_t>(getItemId(source))
        );
        if (!numericId ||
            !entry->putNative("id", std::move(numericId)) ||
            !putNativeByte(entry, "Slot", slot)) {
            return false;
        }
        return true;
    }

    struct ShulkerItemLockWrite {
        std::size_t itemCount = 0;
        std::size_t changedListCount = 0;
        std::size_t createdTagCount = 0;
        int previousMode = -1;
        bool mixedPreviousModes = false;
    };

    bool writeShulkerItemLock(
        CompoundTag* root,
        uint8_t lockMode,
        ShulkerItemLockWrite& result
    ) {
        result = {};
        if (!root || lockMode < 1 || lockMode > 2) {
            return false;
        }

        MutableShulkerItemsEntry entry;
        if (!findMutableShulkerItemsEntry(root, entry) || !entry.parent) {
            return false;
        }

        struct PendingItemLockWrite {
            CompoundTag* itemEntry = nullptr;
            bool createTag = false;
        };
        std::vector<PendingItemLockWrite> pendingWrites;

        const auto nativeLegacyEntry =
            entry.parent->mTags.find("__Legacy__Items");
        const bool hasNativeLegacyList =
            nativeLegacyEntry != entry.parent->mTags.end() &&
            nativeLegacyEntry->second.get<ListTag>();

        for (const char* listKey : {
                 "Items",
                 "items",
                 "__Legacy__Items"
            }) {
            if (hasNativeLegacyList &&
                std::strcmp(listKey, "__Legacy__Items") != 0) {
                continue;
            }
            auto listEntry = entry.parent->mTags.find(listKey);
            if (listEntry == entry.parent->mTags.end()) {
                continue;
            }

            ListTag* items = listEntry->second.get<ListTag>();
            if (!items || items->mType != Tag::Compound ||
                items->mList.empty() || items->mList.size() > 27) {
                return false;
            }

            result.itemCount += items->mList.size();
            for (const auto& listItem : items->mList) {
                if (!listItem || listItem->getId() != Tag::Compound) {
                    return false;
                }
                auto* itemEntry = static_cast<CompoundTag*>(listItem.get());
                auto itemTagEntry = itemEntry->mTags.find("tag");
                CompoundTag* itemTag = nullptr;
                bool createTag = false;
                if (itemTagEntry == itemEntry->mTags.end()) {
                    createTag = true;
                }
                else {
                    itemTag = itemTagEntry->second.get<CompoundTag>();
                    if (!itemTag) {
                        return false;
                    }
                }

                int previousMode = -1;
                if (itemTag) {
                    auto lockEntry = itemTag->mTags.find("minecraft:item_lock");
                    if (lockEntry != itemTag->mTags.end()) {
                        const ByteTag* previous = lockEntry->second.get<ByteTag>();
                        if (!previous) return false;
                        previousMode = static_cast<int>(previous->data);
                    }
                }

                if (pendingWrites.empty()) result.previousMode = previousMode;
                else if (result.previousMode != previousMode) result.mixedPreviousModes = true;
                pendingWrites.push_back({ itemEntry, createTag });
            }
        }

        if (pendingWrites.empty()) {
            return false;
        }

        for (const PendingItemLockWrite& pending : pendingWrites) {
            if (!pending.itemEntry) {
                return false;
            }

            if (pending.createTag) {
                std::unique_ptr<CompoundTag> nativeTag =
                    Tag::createNative<CompoundTag>();
                if (!nativeTag ||
                    !pending.itemEntry->putNative(
                        "tag",
                        std::move(nativeTag)
                    )) {
                    return false;
                }
                ++result.createdTagCount;
            }

            auto itemTagEntry = pending.itemEntry->mTags.find("tag");
            if (itemTagEntry == pending.itemEntry->mTags.end()) {
                return false;
            }
            CompoundTag* itemTag =
                itemTagEntry->second.get<CompoundTag>();
            if (!itemTag ||
                !putNativeByte(
                    itemTag,
                    "minecraft:item_lock",
                    lockMode
                )) {
                return false;
            }

            auto writtenEntry = itemTag->mTags.find(
                "minecraft:item_lock"
            );
            const ByteTag* written =
                writtenEntry == itemTag->mTags.end()
                ? nullptr
                : writtenEntry->second.get<ByteTag>();
            if (!written || written->data != lockMode) {
                return false;
            }
            ++result.changedListCount;
        }

        return result.changedListCount == pendingWrites.size();
    }

    bool writeShulkerItemEnchantments(
        CompoundTag* root,
        const ItemStack& donor,
        std::size_t& changedItemCount,
        std::size_t& enchantmentCount
    ) {
        changedItemCount = 0;
        enchantmentCount = 0;
        if (!root || !donor.isValid() || donor.mUserData == 0 ||
            !canCloneNativeItemStack()) {
            return false;
        }

        MutableShulkerItemsEntry entry;
        if (!findMutableShulkerItemsEntry(root, entry) || !entry.parent) {
            return false;
        }

        const auto nativeLegacyEntry =
            entry.parent->mTags.find("__Legacy__Items");
        const bool hasNativeLegacyList =
            nativeLegacyEntry != entry.parent->mTags.end() &&
            nativeLegacyEntry->second.get<ListTag>();

        for (const char* listKey : { "Items", "items", "__Legacy__Items" }) {
            if (hasNativeLegacyList &&
                std::strcmp(listKey, "__Legacy__Items") != 0) {
                continue;
            }
            auto listEntry = entry.parent->mTags.find(listKey);
            if (listEntry == entry.parent->mTags.end()) continue;
            ListTag* items = listEntry->second.get<ListTag>();
            if (!items || items->mType != Tag::Compound ||
                items->mList.empty() || items->mList.size() > 27) {
                return false;
            }

            for (const auto& listItem : items->mList) {
                if (!listItem || listItem->getId() != Tag::Compound) {
                    return false;
                }
                auto* itemEntry = static_cast<CompoundTag*>(listItem.get());
                auto itemTagEntry = itemEntry->mTags.find("tag");
                if (itemTagEntry == itemEntry->mTags.end()) {
                    auto nativeTag = Tag::createNative<CompoundTag>();
                    if (!nativeTag ||
                        !itemEntry->putNative("tag", std::move(nativeTag))) {
                        return false;
                    }
                    itemTagEntry = itemEntry->mTags.find("tag");
                }
                CompoundTag* itemTag =
                    itemTagEntry == itemEntry->mTags.end()
                    ? nullptr
                    : itemTagEntry->second.get<CompoundTag>();
                if (!itemTag) return false;

                ItemStack donorCopy(donor);
                NativeItemStackResetGuard donorGuard(&donorCopy);
                auto* donorRoot = reinterpret_cast<CompoundTag*>(
                    donorCopy.mUserData
                    );
                MutableEnchantEntry donorEntry;
                if (!donorRoot ||
                    !findMutableNativeEnchantEntry(donorRoot, donorEntry)) {
                    return false;
                }
                if (enchantmentCount == 0) {
                    enchantmentCount = donorEntry.enchantmentCount;
                }
                auto enchantNode = donorEntry.parent->mTags.extract(
                    donorEntry.iterator
                );
                if (enchantNode.empty()) return false;

                itemTag->mTags.erase("ench");
                itemTag->mTags.erase("Enchantments");
                itemTag->mTags.erase("enchantments");
                enchantNode.key() = "ench";
                if (!itemTag->mTags.insert(std::move(enchantNode)).inserted) {
                    return false;
                }
                ++changedItemCount;
            }
        }
        return changedItemCount != 0 && enchantmentCount != 0;
    }

    struct SelectedAetherEnchant {
        int16_t id = 0;
        int16_t level = 1;
    };

    bool editShulkerItemEnchantments(
        CompoundTag* root,
        const std::vector<SelectedAetherEnchant>& selected,
        int editMode,
        std::size_t& changedItemCount
    ) {
        changedItemCount = 0;
        if (!root || editMode < 0 || editMode > 2) return false;
        MutableShulkerItemsEntry entry;
        if (!findMutableShulkerItemsEntry(root, entry) || !entry.parent) return false;
        const auto legacy = entry.parent->mTags.find("__Legacy__Items");
        const bool hasLegacy = legacy != entry.parent->mTags.end() && legacy->second.get<ListTag>();
        auto readEnchantId = [](const std::unique_ptr<Tag>& node, int& id) {
            if (!node || node->getId() != Tag::Compound) return false;
            auto* compound = static_cast<CompoundTag*>(node.get());
            auto value = compound->mTags.find("id");
            if (value == compound->mTags.end()) return false;
            if (auto* shortValue = value->second.get<ShortTag>()) { id = shortValue->data; return true; }
            if (auto* intValue = value->second.get<IntTag>()) { id = intValue->data; return true; }
            return false;
            };
        auto appendEnchant = [](ListTag* list, const SelectedAetherEnchant& enchantment) {
            auto compound = Tag::createNative<CompoundTag>();
            if (!compound ||
                !compound->putNative("id", Tag::createNative<ShortTag>(enchantment.id)) ||
                !compound->putNative("lvl", Tag::createNative<ShortTag>(enchantment.level))) return false;
            list->mType = Tag::Compound;
            list->mList.push_back(std::unique_ptr<Tag>(compound.release()));
            return true;
            };

        for (const char* listKey : { "Items", "items", "__Legacy__Items" }) {
            if (hasLegacy && std::strcmp(listKey, "__Legacy__Items") != 0) continue;
            auto listEntry = entry.parent->mTags.find(listKey);
            if (listEntry == entry.parent->mTags.end()) continue;
            ListTag* items = listEntry->second.get<ListTag>();
            if (!items || items->mType != Tag::Compound || items->mList.size() > 27) return false;
            for (const auto& node : items->mList) {
                if (!node || node->getId() != Tag::Compound) return false;
                auto* item = static_cast<CompoundTag*>(node.get());
                auto tagEntry = item->mTags.find("tag");
                CompoundTag* itemTag = tagEntry == item->mTags.end() ? nullptr : tagEntry->second.get<CompoundTag>();
                if (!itemTag && editMode != 2) {
                    auto created = Tag::createNative<CompoundTag>();
                    if (!created || !item->putNative("tag", std::move(created))) return false;
                    tagEntry = item->mTags.find("tag");
                    itemTag = tagEntry == item->mTags.end() ? nullptr : tagEntry->second.get<CompoundTag>();
                }
                if (!itemTag) continue;
                auto enchantEntry = itemTag->mTags.find("ench");
                if (enchantEntry == itemTag->mTags.end()) enchantEntry = itemTag->mTags.find("Enchantments");
                if (enchantEntry == itemTag->mTags.end()) enchantEntry = itemTag->mTags.find("enchantments");
                ListTag* enchantList = enchantEntry == itemTag->mTags.end() ? nullptr : enchantEntry->second.get<ListTag>();

                if (editMode == 2) {
                    if (!enchantList) continue;
                    const auto oldSize = enchantList->mList.size();
                    enchantList->mList.erase(std::remove_if(enchantList->mList.begin(), enchantList->mList.end(), [&](const auto& enchantment) {
                        int id = -1;
                        if (!readEnchantId(enchantment, id)) return false;
                        return std::any_of(selected.begin(), selected.end(), [&](const SelectedAetherEnchant& wanted) { return wanted.id == id; });
                        }), enchantList->mList.end());
                    if (enchantList->mList.size() != oldSize) ++changedItemCount;
                    continue;
                }

                if (editMode == 0) {
                    itemTag->mTags.erase("ench");
                    itemTag->mTags.erase("Enchantments");
                    itemTag->mTags.erase("enchantments");
                    auto created = Tag::createNative<ListTag>();
                    if (!created) return false;
                    for (const auto& enchantment : selected) if (!appendEnchant(created.get(), enchantment)) return false;
                    if (!selected.empty() && !itemTag->putNative("ench", std::move(created))) return false;
                    ++changedItemCount;
                    continue;
                }

                if (!enchantList) {
                    auto created = Tag::createNative<ListTag>();
                    if (!created || !itemTag->putNative("ench", std::move(created))) return false;
                    enchantEntry = itemTag->mTags.find("ench");
                    enchantList = enchantEntry == itemTag->mTags.end() ? nullptr : enchantEntry->second.get<ListTag>();
                }
                if (!enchantList) return false;
                bool changed = false;
                for (const auto& wanted : selected) {
                    const bool exists = std::any_of(enchantList->mList.begin(), enchantList->mList.end(), [&](const auto& enchantment) {
                        int id = -1; return readEnchantId(enchantment, id) && id == wanted.id;
                        });
                    if (!exists) { if (!appendEnchant(enchantList, wanted)) return false; changed = true; }
                }
                if (changed) ++changedItemCount;
            }
        }
        return true;
    }

    bool isLikelyDye(const ItemStack& stack) {
        return stack.isValid() &&
            getItemIdentifier(stack).find("dye") != std::string::npos;
    }

    bool isLapis(const ItemStack& stack) {
        if (!stack.isValid()) {
            return false;
        }

        std::string identifier = getItemIdentifier(stack);
        const std::size_t namespaceSeparator = identifier.rfind(':');
        if (namespaceSeparator != std::string::npos) {
            identifier.erase(0, namespaceSeparator + 1);
        }

        return identifier == "lapis_lazuli" ||
            identifier == "lapis" ||
            ((identifier == "dye" || identifier == "dye_powder") &&
                stack.mAuxValue == 4);
    }

    bool isCraftingPseudoSource(const InventorySource& source) {
        const int sourceType = static_cast<int>(source.type);
        return sourceType ==
            static_cast<int>(
                InventorySourceType::NonImplementedFeatureTODO
                ) ||
            sourceType == kCraftSlotSourceType;
    }

    bool isSameStackKind(
        const ItemStack& first,
        const ItemStack& second
    ) {
        return first.isValid() &&
            second.isValid() &&
            first.item.get() == second.item.get() &&
            first.mAuxValue == second.mAuxValue &&
            first.mCount == second.mCount;
    }

    void logLegacyTransaction(
        const char* phase,
        const ComplexInventoryTransaction* transaction
    ) {
        if (!transaction) {
            writeDiagnostic("[%s] transaction=null", phase);
            return;
        }

        std::size_t totalActions = 0;
        for (const auto& [source, actions] : transaction->data.actions) {
            (void)source;
            totalActions += actions.size();
        }

        writeDiagnostic(
            "[%s] tx=%p type=%d groups=%llu actions=%llu",
            phase,
            transaction,
            static_cast<int>(transaction->type),
            static_cast<unsigned long long>(transaction->data.actions.size()),
            static_cast<unsigned long long>(totalActions)
        );

        std::size_t actionIndex = 0;
        for (const auto& [source, actions] : transaction->data.actions) {
            for (const InventoryAction& action : actions) {
                writeDiagnostic(
                    "[%s][%llu] srcType=%d container=%d flags=%u slot=%u "
                    "from={valid:%d,id:%d,count:%u,aux:%d,userData:%p,items:%d,net:%016llX,desc:%016llX} "
                    "to={valid:%d,id:%d,count:%u,aux:%d,userData:%p,items:%d,net:%016llX,desc:%016llX}",
                    phase,
                    static_cast<unsigned long long>(actionIndex++),
                    static_cast<int>(source.type),
                    static_cast<int>(source.containerId),
                    static_cast<unsigned int>(source.flags),
                    action.slot,
                    action.fromItem.isValid() ? 1 : 0,
                    getItemId(action.fromItem),
                    static_cast<unsigned int>(action.fromItem.mCount),
                    static_cast<int>(action.fromItem.mAuxValue),
                    reinterpret_cast<void*>(action.fromItem.mUserData),
                    getShulkerContentEntryCount(action.fromItem),
                    static_cast<unsigned long long>(hashBytes(
                        &action.fromItem.mNetIdVariant,
                        sizeof(action.fromItem.mNetIdVariant)
                    )),
                    static_cast<unsigned long long>(hashBytes(
                        &action.fromItemDescriptor,
                        sizeof(action.fromItemDescriptor)
                    )),
                    action.toItem.isValid() ? 1 : 0,
                    getItemId(action.toItem),
                    static_cast<unsigned int>(action.toItem.mCount),
                    static_cast<int>(action.toItem.mAuxValue),
                    reinterpret_cast<void*>(action.toItem.mUserData),
                    getShulkerContentEntryCount(action.toItem),
                    static_cast<unsigned long long>(hashBytes(
                        &action.toItem.mNetIdVariant,
                        sizeof(action.toItem.mNetIdVariant)
                    )),
                    static_cast<unsigned long long>(hashBytes(
                        &action.toItemDescriptor,
                        sizeof(action.toItemDescriptor)
                    ))
                );
            }
        }
    }

    void logInventorySnapshot(const char* phase) {
        LocalPlayer* localPlayer = Game::getLocalPlayer();
        if (!localPlayer ||
            !localPlayer->playerInventory ||
            !localPlayer->playerInventory->container) {
            writeDiagnostic("[%s] inventory unavailable", phase);
            return;
        }

        writeDiagnostic("[%s] local inventory snapshot begin", phase);
        for (int slot = 0; slot < kInventorySlotCount; ++slot) {
            ItemStack* stack =
                localPlayer->playerInventory->container->getItem(slot);
            if (!stack || !stack->isValid()) {
                writeDiagnostic("[%s] slot=%d empty", phase, slot);
                continue;
            }

            const int outerItems = getShulkerContentEntryCount(*stack);
            int firstChildValid = 0;
            int firstChildId = 0;
            unsigned int firstChildCount = 0;
            int firstChildItems = 0;
            if (outerItems > 0 &&
                stack->mUserData != 0 &&
                isExecutableAddress(Addresses::ItemStack_loadItem)) {
                const auto* root = reinterpret_cast<const CompoundTag*>(
                    stack->mUserData
                    );
                const ListTag* items = findShulkerItemsList(root);
                if (items &&
                    !items->mList.empty() &&
                    items->mList[0] &&
                    items->mList[0]->getId() == Tag::Type::Compound) {
                    ItemStack firstChild;
                    dupeLoadItem(firstChild,
                        *reinterpret_cast<const CompoundTag*>(
                            items->mList[0].get()
                            )
                    );
                    firstChildValid = firstChild.isValid() ? 1 : 0;
                    firstChildId = getItemId(firstChild);
                    firstChildCount =
                        static_cast<unsigned int>(firstChild.mCount);
                    firstChildItems =
                        getShulkerContentEntryCount(firstChild);
                    firstChild.setNull(std::nullopt);
                }
            }

            writeDiagnostic(
                "[%s] slot=%d id=%d count=%u aux=%d net=%016llX userData=%p outerItems=%d firstChild={valid:%d,id:%d,count:%u,items:%d}",
                phase,
                slot,
                getItemId(*stack),
                static_cast<unsigned int>(stack->mCount),
                static_cast<int>(stack->mAuxValue),
                static_cast<unsigned long long>(hashBytes(
                    &stack->mNetIdVariant,
                    sizeof(stack->mNetIdVariant)
                )),
                reinterpret_cast<void*>(stack->mUserData),
                outerItems,
                firstChildValid,
                firstChildId,
                firstChildCount,
                firstChildItems
            );
        }
        writeDiagnostic("[%s] local inventory snapshot end", phase);
    }

    bool isPointerVectorSane(
        const RawPointerVectorView& vector,
        std::size_t maxElements
    ) {
        const uintptr_t begin = reinterpret_cast<uintptr_t>(vector.begin);
        const uintptr_t end = reinterpret_cast<uintptr_t>(vector.end);
        const uintptr_t capacity = reinterpret_cast<uintptr_t>(vector.capacity);

        if (begin == 0 || end == 0 || capacity == 0) {
            return begin == 0 && end == 0 && capacity == 0;
        }

        if (begin > end || end > capacity ||
            (end - begin) % sizeof(uintptr_t) != 0 ||
            (capacity - begin) % sizeof(uintptr_t) != 0) {
            return false;
        }

        const std::size_t count =
            (end - begin) / sizeof(uintptr_t);
        const std::size_t reserved =
            (capacity - begin) / sizeof(uintptr_t);
        if (count > maxElements || reserved > maxElements * 4) {
            return false;
        }

        return count == 0 ||
            isReadableRange(vector.begin, count * sizeof(uintptr_t));
    }

    bool isObjectVectorSane(
        const RawObjectVectorView& vector,
        std::size_t elementSize,
        std::size_t maxElements
    ) {
        const uintptr_t begin = reinterpret_cast<uintptr_t>(vector.begin);
        const uintptr_t end = reinterpret_cast<uintptr_t>(vector.end);
        const uintptr_t capacity = reinterpret_cast<uintptr_t>(vector.capacity);

        if (begin == 0 || end == 0 || capacity == 0) {
            return begin == 0 && end == 0 && capacity == 0;
        }

        if (elementSize == 0 || begin > end || end > capacity ||
            (end - begin) % elementSize != 0 ||
            (capacity - begin) % elementSize != 0) {
            return false;
        }

        const std::size_t count = (end - begin) / elementSize;
        const std::size_t reserved = (capacity - begin) / elementSize;
        if (count > maxElements || reserved > maxElements * 4) {
            return false;
        }

        return count == 0 || isReadableRange(vector.begin, end - begin);
    }

    bool isPlayerInventoryContainer(int8_t container) {
        return container == kCombinedInventoryContainer ||
            container == kHotbarContainer ||
            container == kInventoryContainer;
    }

    bool isUsefulDestination(int8_t container, uint8_t slot) {
        if (container == kCursorContainer) {
            return true;
        }

        return isPlayerInventoryContainer(container) &&
            slot < kInventorySlotCount;
    }

    DecodedDestination decodeDestination(const void* action) {
        if (!action) {
            return {};
        }

        const uintptr_t base = reinterpret_cast<uintptr_t>(action);

        const auto* legacyContainer = reinterpret_cast<const int8_t*>(
            base + kLegacyTransferDestinationOffset
            );
        const auto* legacySlot = reinterpret_cast<const uint8_t*>(
            base + kLegacyTransferDestinationOffset + kLegacySlotOffset
            );
        if (isReadableRange(legacyContainer, 2) &&
            isUsefulDestination(*legacyContainer, *legacySlot)) {
            return { *legacyContainer, *legacySlot, true };
        }

        const auto* fullNameContainer = reinterpret_cast<const int8_t*>(
            base + kFullNameTransferDestinationOffset
            );
        const auto* fullNameSlot = reinterpret_cast<const uint8_t*>(
            base + kFullNameTransferDestinationOffset + kFullNameSlotOffset
            );
        if (isReadableRange(fullNameContainer, kFullNameSlotOffset + 1) &&
            isUsefulDestination(*fullNameContainer, *fullNameSlot)) {
            return { *fullNameContainer, *fullNameSlot, true };
        }

        return {};
    }

    const RawPointerVectorView* getActionVector(
        const AnvilStackRequestDataView* request,
        bool& hasFilterStrings
    ) {
        hasFilterStrings = false;
        if (!request || !isReadableRange(request, sizeof(*request))) {
            return nullptr;
        }

        if (isObjectVectorSane(
            request->stringsToFilter,
            sizeof(std::string),
            16
        ) &&
            isPointerVectorSane(request->actions, 64)) {
            hasFilterStrings =
                request->stringsToFilter.begin !=
                request->stringsToFilter.end;
            return &request->actions;
        }

        constexpr uintptr_t kLegacyActionsOffset = 0x30;
        const auto* legacyActions =
            reinterpret_cast<const RawPointerVectorView*>(
                reinterpret_cast<uintptr_t>(request) + kLegacyActionsOffset
                );
        if (isReadableRange(legacyActions, sizeof(*legacyActions)) &&
            isPointerVectorSane(*legacyActions, 64)) {
            return legacyActions;
        }

        return nullptr;
    }

    const AnvilStackRequestDataView* getAuthRequest(
        const Packet* packet,
        uintptr_t offset
    ) {
        const auto* field =
            reinterpret_cast<const AnvilStackRequestDataView* const*>(
                reinterpret_cast<uintptr_t>(packet) + offset
                );
        if (!isReadableRange(field, sizeof(*field))) {
            return nullptr;
        }

        const AnvilStackRequestDataView* request = *field;
        bool ignored = false;
        return getActionVector(request, ignored) ? request : nullptr;
    }
}

Dupe::Dupe()
    : Dupe(
        "Dupe",
        "Anvil and dye duplication",
        -1,
        false,
        false
    ) {
}

Dupe::Dupe(
    const char* moduleName,
    const char* moduleDescription,
    int fixedMode,
    bool nestedOnly,
    bool publicOnly,
    bool anvilOnly
)
    : Module(
        moduleName,
        moduleDescription,
        Category::PLAYER
    ) {
    nestedOnlyModule = nestedOnly;
    publicOnlyModule = publicOnly;
    anvilOnlyModule = anvilOnly;
    if (fixedMode >= 0) {
        mode = fixedMode;
    }

    if (anvilOnlyModule) {
        registerSetting(new EnumSetting(
            "Mode",
            "Duplication method",
            { "WorkingDupe" },
            &anvilModeDisplay,
            0
        ));
        registerSetting(new SliderSetting<int>(
            "Copies",
            "Inventory/world copies balanced by the ignored -10 virtual source",
            &anvilDupe2CopyCount,
            27,
            1,
            27,
            [this]() { return !anvilDupe2SingleItem; }
        ));
        registerSetting(new EnumSetting(
            "Destination",
            "Send the balanced copies to the world or empty inventory slots",
            { "World", "Inventory" },
            &anvilDupe2Destination,
            1,
            [this]() { return !anvilDupe2SingleItem; }
        ));
        registerSetting(new BoolSetting(
            "Stacking",
            "Overstack duplicated items per destination; only works for items whose max stack size is > 1",
            &stacking,
            false,
            [this]() { return !anvilDupe2SingleItem; }
        ));
        registerSetting(new SliderSetting<int>(
            "Stack Count",
            "Items per destination stack; the server still caps each stack at the item's max stack size",
            &stackCount,
            64,
            1,
            255,
            [this]() { return stacking && !anvilDupe2SingleItem; }
        ));
        registerSetting(new BoolSetting(
            "Stack Request",
            "Change the count in the genuine anvil ItemStackRequest instead of rewriting its legacy transaction",
            &itemStackRequestStacking,
            false,
            [this]() { return stacking && !anvilDupe2SingleItem; }
        ));
        registerSetting(new BoolSetting(
            "Single Box Item",
            "Experimental one-copy shulker NBT path (NBT graft features are unavailable in this build)",
            &anvilDupe2SingleItem,
            false
        ));
        registerSetting(new BoolSetting(
            "Auto Repeat",
            "Keep Dupe1 enabled and wait for the next genuine result transaction",
            &autoRepeat,
            true
        ));
        return;
    }

    if (nestedOnlyModule) {
        registerSetting(new EnumSetting(
            "Carrier",
            "Rewrite the genuine anvil result and its inventory target",
            { "Anvil Result" },
            &nestedCarrierMode,
            0
        ));
        return;
    }

    if (publicOnlyModule) {
        registerSetting(new SliderSetting<int>(
            "Copies",
            "Additional dyedupe3 results; Inventory is limited by empty slots",
            &dupe3CopyCount,
            27,
            1,
            256
        ));
        registerSetting(new EnumSetting(
            "Destination",
            "Send rich-NBT dyedupe3 copies to the world or empty inventory slots",
            { "World", "Inventory" },
            &dupe3Destination,
            0
        ));
        registerSetting(new BoolSetting(
            "Drop Inventory",
            "After the response window, drop main-inventory slots 9-35; hotbar is excluded",
            &dropInventory,
            false
        ));
        registerSetting(new BoolSetting(
            "Auto Repeat",
            "Keep PublicDupe enabled and wait for the next genuine result transaction",
            &autoRepeat,
            true
        ));
        return;
    }

    registerSetting(new EnumSetting(
        "Mode",
        "Duplication method",
        { "Mystic Dupe", "WorkingDupe", "enchantdupe", "AnvilSwap", "New Mystic Dupe" },
        &mode,
        0
    ));
    registerSetting(new SliderSetting<int>(
        "Dupe Copies",
        "Additional dyedupe3 results; Inventory is limited by empty slots",
        &dupe3CopyCount,
        27,
        1,
        256,
        [this]() { return mode == 0; }
    ));
    registerSetting(new EnumSetting(
        "Dupe Destination",
        "Send rich-NBT dyedupe3 copies to the world or empty inventory slots",
        { "World", "Inventory" },
        &dupe3Destination,
        0,
        [this]() { return mode == 0; }
    ));
    registerSetting(new SliderSetting<int>(
        "Nested Copies",
        "Additional Inventory results for the new mystery duplication mode",
        &boxInBoxCopyCount,
        1,
        1,
        27,
        [this]() { return mode == 4; }
    ));
    registerSetting(new BoolSetting(
        "ALL",
        "Experimental: replace any crafting result with the box from hotbar slot 0",
        &nestedAllCrafting,
        false,
        [this]() { return mode == 4; }
    ));
    registerSetting(new SliderSetting<int>(
        "Enchant Copies",
        "Additional rich-NBT results; each result consumes one lapis and one XP level",
        &enchantCopyCount,
        2,
        1,
        27,
        [this]() { return mode == 2; }
    ));
    registerSetting(new SliderSetting<int>(
        "Enchant Donor Slot",
        "Hotbar slot containing a genuinely enchanted item whose native enchant list is copied to the enchanting-table target",
        &enchantDonorSlot,
        8,
        0,
        8,
        [this]() { return mode == 2; }
    ));
    registerSetting(new SliderSetting<int>(
        "Dupe Copies",
        "World drops or inventory destinations balanced by the ignored -10 virtual source",
        &anvilDupe2CopyCount,
        27,
        1,
        27,
        [this]() {
            return mode == 1 &&
                !anvilDupe2SingleItem &&
                !anvilDupe2MaterialSlotMode;
        }
    ));
    registerSetting(new BoolSetting(
        "Single Box Item",
        "Experimental one-copy shulker NBT path; choose the exact graft scope below",
        &anvilDupe2SingleItem,
        false,
        [this]() { return mode == 1; }
    ));
    registerSetting(new BoolSetting(
        "Source Box Mode",
        "Experimental source-side route: replace Items on the genuine non-empty anvil source action and keep its -10 mirror synchronized",
        &anvilDupe2SourceBoxMode,
        false,
        [this]() { return mode == 1 && anvilDupe2SingleItem; }
    ));
    registerSetting(new EnumSetting(
        "Single NBT Test",
        "Choose exactly which loaded box node is copied; Control keeps the generated result unchanged",
        {
            "Control",
            "Items[0].Count",
            "Items[0].Name/ID",
            "Entire Items"
        },
        &anvilDupe2SingleNbtExperiment,
        3,
        [this]() { return mode == 1 && anvilDupe2SingleItem; }
    ));
    registerSetting(new EnumSetting(
        "Item Lock",
        "Write the Bedrock item-lock byte into every contained item without changing IDs, counts or slots",
        {
            "Unchanged",
            "Can't Move",
            "Can't Drop"
        },
        &anvilDupe2InnerItemLock,
        0,
        [this]() { return mode == 1 && anvilDupe2SingleItem; }
    ));
    registerSetting(new EnumSetting(
        "Dupe Destination",
        "Send -10-balanced copies directly to the world or empty inventory slots",
        { "World", "Inventory" },
        &anvilDupe2Destination,
        1,
        [this]() {
            return mode == 1 &&
                !anvilDupe2SingleItem &&
                !anvilDupe2MaterialSlotMode;
        }
    ));
    registerSetting(new SliderSetting<int>(
        "A Hotbar Slot",
        "Source A used only by the independent AnvilSwap -10 experiment",
        &anvilSwapSourceSlot,
        0,
        0,
        8,
        [this]() { return mode == 3; }
    ));
    registerSetting(new BoolSetting(
        "Craft Drop Stack",
        "Experimental: turn one genuine crafting result into one server-side world drop carrying Stack Count items",
        &craftDropStack,
        false,
        [this]() { return mode == 0; }
    ));
    registerSetting(new BoolSetting(
        "Stacking",
        "Place multiple duplicated items in each destination stack",
        &stacking,
        false,
        [this]() {
            return mode == 0 ||
                (mode == 1 &&
                    !anvilDupe2SingleItem &&
                    !anvilDupe2MaterialSlotMode);
        }
    ));
    registerSetting(new BoolSetting(
        "Stack Request",
        "Experimental: change the count in the genuine anvil ItemStackRequest instead of rewriting its legacy transaction",
        &itemStackRequestStacking,
        false,
        [this]() {
            return (mode == 0 || mode == 1) && stacking;
        }
    ));
    registerSetting(new SliderSetting<int>(
        "Stack Count",
        "Number of duplicated items placed in each destination stack",
        &stackCount,
        64,
        1,
        255,
        [this]() {
            return (stacking || craftDropStack) &&
                (mode == 0 ||
                    (mode == 1 &&
                        !anvilDupe2SingleItem &&
                        !anvilDupe2MaterialSlotMode));
        }
    ));
    registerSetting(new BoolSetting(
        "Drop Inventory",
        "After the response window, drop main-inventory slots 9-35; hotbar is excluded",
        &dropInventory,
        false,
        [this]() { return mode == 0 || mode == 2; }
    ));
    registerSetting(new BoolSetting(
        "Auto Repeat",
        "Keep Dupe enabled and wait for the next genuine result transaction",
        &autoRepeat,
        true
    ));
}

void Dupe::refreshMaparts() {}
std::string Dupe::getSelectedMapart() const { return {}; }
void Dupe::refreshDefaultNbtFiles() {}
std::string Dupe::getSelectedDefaultNbt() const { return {}; }
int Dupe::getSelectedAetherBoxColor() const { return 9; }

bool Dupe::generateNestedNbt(LocalPlayer* localPlayer) {
    if (!localPlayer) return false;
    const int limit = aetherBoxMode == 1 ? 27 : std::clamp(aetherBoxCount, 1, 27);
    std::vector<std::unique_ptr<CompoundTag>> ownedEntries;
    std::vector<NbtCommand::LegacyItemEntry> outputEntries;
    if (aetherBoxMode == 1 && localPlayer->playerInventory && localPlayer->playerInventory->container) {
        for (int target = 0; target < limit; ++target) {
            if (target >= static_cast<int>(selectedAetherBoxSlots.size()) ||
                !selectedAetherBoxSlots[static_cast<size_t>(target)]) continue;
            const int sourceSlot = aetherBoxSourceSlots[static_cast<size_t>(target)];
            if (sourceSlot < 0 || sourceSlot > 35) continue;
            ItemStack* source = localPlayer->playerInventory->container->getItem(sourceSlot);
            if (!source || !source->isValid() || source->mCount == 0) continue;
            std::unique_ptr<CompoundTag> saved = saveNativeItemEntry(*source);
            if (!saved) continue;
            const int color = std::clamp(aetherBoxSlotColors[static_cast<size_t>(target)], 0, 16);
            const bool recolor = isShulkerBox(*source) && color < 16;
            NbtCommand::LegacyItemEntry output;
            output.itemEntry = saved.get();
            output.slot = static_cast<uint8_t>(target);
            if (recolor) {
                output.modernName = "minecraft:" + aetherBoxColorOptions[static_cast<size_t>(color)];
                output.legacyId = 218;
                output.legacyDamage = static_cast<int16_t>(color);
            }
            else {
                std::string sourceName = source->item.get()->rawName;
                if (!sourceName.empty() && sourceName.find(':') == std::string::npos)
                    sourceName = "minecraft:" + sourceName;
                output.modernName = std::move(sourceName);
                output.legacyId = static_cast<int16_t>(InventoryUtil::getItemId(source));
                output.legacyDamage = source->mAuxValue;
            }
            output.preserveIdentity = !recolor;
            output.count = static_cast<uint8_t>(std::clamp(aetherBoxSlotCounts[static_cast<size_t>(target)], 1, 64));
            output.customName = aetherBoxSlotNames[static_cast<size_t>(target)];
            outputEntries.push_back(std::move(output));
            ownedEntries.push_back(std::move(saved));
        }
    }
    if (!outputEntries.empty()) {
        const bool saved = NbtCommand::saveLegacyItemEntriesJson("nested", outputEntries);
        for (auto& entry : ownedEntries) (void)entry.release();
        if (!saved || !NbtCommand::rewriteSavedRootCustomName("nested", "nested", aetherOuterName)) {
            writeDiagnostic("[AETHER_NESTED_ABORT] failed to write selected boxes");
            return false;
        }
        writeDiagnostic("[AETHER_NESTED_READY] wrote nested.json selected=%d", static_cast<int>(outputEntries.size()));
        return true;
    }
    if (aetherBoxMode == 1) {
        writeDiagnostic("[AETHER_NESTED_ABORT] custom mode has no valid selected boxes");
        return false;
    }
    ItemStack* held = localPlayer->getCarriedItem();
    if (!held || !held->isValid() || held->mCount == 0) {
        writeDiagnostic("[AETHER_NESTED_ABORT] held item is invalid");
        return false;
    }

    const int count = limit;
    std::unique_ptr<CompoundTag> entry = saveNativeItemEntry(*held);
    if (!entry) {
        writeDiagnostic("[AETHER_NESTED_ABORT] failed to serialize held box");
        return false;
    }
    std::vector<NbtCommand::LegacyItemEntry> repeated;
    repeated.reserve(static_cast<size_t>(count));
    const int selectedColor = std::clamp(getSelectedAetherBoxColor(), 0, 16);
    const bool recolor = isShulkerBox(*held) && selectedColor < 16;
    for (int slot = 0; slot < count; ++slot) {
        NbtCommand::LegacyItemEntry output;
        output.itemEntry = entry.get();
        output.slot = static_cast<uint8_t>(slot);
        if (recolor) {
            output.modernName = "minecraft:" + aetherBoxColorOptions[static_cast<size_t>(selectedColor)];
            output.legacyId = 218;
            output.legacyDamage = static_cast<int16_t>(selectedColor);
        }
        else {
            std::string heldName = held->item.get()->rawName;
            if (!heldName.empty() && heldName.find(':') == std::string::npos)
                heldName = "minecraft:" + heldName;
            output.modernName = std::move(heldName);
            output.legacyId = static_cast<int16_t>(InventoryUtil::getItemId(held));
            output.legacyDamage = held->mAuxValue;
        }
        output.preserveIdentity = !recolor;
        repeated.push_back(std::move(output));
    }
    const bool saved = NbtCommand::saveLegacyItemEntriesJson("nested", repeated) &&
        NbtCommand::rewriteSavedRootCustomName("nested", "nested", aetherOuterName);
    (void)entry.release();
    if (!saved) {
        writeDiagnostic("[AETHER_NESTED_ABORT] failed to write NBT\\nested.json");
        return false;
    }
    writeDiagnostic("[AETHER_NESTED_READY] wrote nested.json count=%d", count);
    return true;
}

void Dupe::displayMessage(const char* format, ...) const {
    (void)format;
}

void Dupe::resetAttemptState() {
    stage = Stage::WaitForRenameTake;
    renamedInventorySlot = -1;
    settleTicks = 0;
    captureWaitTicks = 0;
    packetCooldownTicks = 0;
    sentTransactionCount = 0;
    renameNoticePending = false;
    rewriteNoticePending = false;
    nestedAbortNoticePending = false;
    nestedRewriteMatchedAndRejected = false;
    rewrittenDestinationCount = 0;
    legacyPacketCount = 0;
    enchantOutgoingDiagnosticCount = 0;
    responseWaitTicks = 0;
    incomingDiagnosticCount = 0;
    anvilResyncProbeSent = false;
    aetherCraftResyncPending = false;
    anvilResyncAuthoritativePacketReceived = false;
    anvilPreTransactionOwnedCount = 0;
    nextDropSlot = kMainInventoryFirstSlot;
    dropTransactionCount = 0;
    dropFailureCount = 0;
    sawDyeShulkerIngredient = false;
    sawDyeOtherIngredient = false;
    sawNamedDyeIngredient = false;
    resetNativeItemStack(cachedDyeShulkerIngredient);
    resetNativeItemStack(cachedNestedSourceBox);
    resetNativeItemStack(cachedEarlyNestedResult);
    nestedEarlyResultInjected = false;
    resetNativeItemStack(cachedEnchantInput);
    resetNativeItemStack(cachedEnchantMaterial);
    resetNativeItemStack(cachedEnchantDonor);
    resetNativeItemStack(expectedResultStack);
    resetNativeItemStack(stagedAnvilOutputStack);
    stagedAnvilDestinationSlot = -1;
    anvilOutputStagePending = false;
    nbtWindowStagePending = false;
    nbtWindowFirstStageAccepted = false;
    nbtWindowSecondStageSent = false;
    anvilMaterialProbeSent = false;
    anvilMaterialInventoryPacketObserved = false;
    anvilMaterialInventoryPacketCount = 0;
    anvilMaterialReadbackCheckedPacketCount = 0;
    anvilMaterialReadbackState = kAnvilMaterialReadbackNone;
    anvilMaterialAuthoritativePacketReceived = false;
    anvilMaterialNativeTakeRequested = false;
    anvilMaterialNativeTakePacketObserved = false;
    anvilMaterialNativeTakePacketId = 0;
    anvilMaterialReadbackGraceUsed = false;
    anvilMaterialPreTakeSnapshotValid = false;
    anvilMaterialPreTakeCounts.fill(0);
    anvilMaterialPreTakeExpectedMatches.fill(false);
    anvilMaterialPreTakeEmptySlots.fill(false);
    anvilResyncInventoryPacketObserved = false;
    anvilResyncInventoryPacketCount = 0;
    anvilResyncReadbackState = kAnvilMaterialReadbackNone;
    anvilResyncClearedLocalSlots = 0;
    resetNativeItemStack(nestedLocalShellStack);
    enchantSettleTicks = 0;
    enchantTransactionPending = false;
    enchantPacketInFlight = false;
    enchantSequencePhase = EnchantSequencePhase::Idle;
    enchantSequenceDestinationSlots.clear();
    enchantSequenceCompletedSlots.clear();
    enchantSequenceIndex = 0;
    enchantSequenceInitialMaterialCount = 0;
    expectedDestinationCounts.fill(0);
    expectedResultItem = nullptr;
    expectedResultAux = 0;
    expectedResultCount = 0;
    waitingForCursorPlacement = false;
    cursorSnapshotValid = false;
    emptyBeforeCursor.fill(false);
    pendingDestinationSlots.clear();
    rewrittenDestinationSlots.clear();
    nextDestinationIndex = 0;
}

void Dupe::rememberExpectedResult(const ItemStack& item) {
    resetNativeItemStack(expectedResultStack);
    expectedResultStack = item.isValid()
        ? std::make_unique<ItemStack>(item)
        : nullptr;
    expectedResultItem = item.isValid() ? item.item.get() : nullptr;
    expectedResultAux = item.mAuxValue;
    expectedResultCount = item.mCount;
}

int Dupe::refreshExpectedDestinations(LocalPlayer* localPlayer) {
    const ItemStack* predictionStack =
        activeMode == 4 &&
        !nestedOnlyModule &&
        nestedLocalShellStack &&
        nestedLocalShellStack->isValid()
        ? nestedLocalShellStack.get()
        : expectedResultStack.get();
    if (!localPlayer ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container ||
        !predictionStack ||
        !predictionStack->isValid()) {
        return 0;
    }

    Container* container = localPlayer->playerInventory->container;
    int refreshedCount = 0;
    for (const int slot : rewrittenDestinationSlots) {
        if (slot < 0 || slot >= kInventorySlotCount) {
            continue;
        }

        const uint8_t expectedCount = expectedDestinationCounts[slot];
        if (expectedCount == 0) {
            continue;
        }

        ItemStack* current = container->getItem(slot);
        bool replaceVanillaCraftShell = false;
        if (((nestedOnlyModule &&
            activeMode == 4 &&
            useNestedDyeDropCarrier()) ||
            activeMode == 9) &&
            current &&
            current->isValid() &&
            expectedResultStack &&
            current->item.get() == expectedResultStack->item.get() &&
            current->mAuxValue == expectedResultStack->mAuxValue &&
            current->mCount == expectedCount) {
            replaceVanillaCraftShell =
                getShulkerContentEntryCount(*current) !=
                getShulkerContentEntryCount(*expectedResultStack);
        }
        if (current && current->isValid() &&
            !replaceVanillaCraftShell) {
            continue;
        }

        const int replacedShellItemCount = replaceVanillaCraftShell
            ? getShulkerContentEntryCount(*current)
            : 0;
        ItemStack predicted(*predictionStack);
        predicted.mCount = expectedCount;
        container->setItem(slot, predicted);
        if (replaceVanillaCraftShell) {
            writeDiagnostic(
                "[CRAFT_LOCAL_SHELL_REPLACED] mode=%s slot=%d oldItems=%d newItems=%d",
                getDupeModeName(activeMode),
                slot,
                replacedShellItemCount,
                getShulkerContentEntryCount(predicted)
            );
        }
        ++refreshedCount;
    }

    if (refreshedCount > 0) {
        writeDiagnostic(
            "[LOCAL_REFRESH] mode=%s refreshed=%d destinations=%llu",
            getDupeModeName(activeMode),
            refreshedCount,
            static_cast<unsigned long long>(
                rewrittenDestinationSlots.size()
                )
        );
    }
    return refreshedCount;
}

bool Dupe::matchesExpectedResult(
    const ItemStack* item,
    uint8_t expectedCount
) const {
    return item &&
        item->isValid() &&
        expectedResultItem &&
        item->item.get() == expectedResultItem &&
        item->mAuxValue == expectedResultAux &&
        item->mCount == expectedCount;
}

bool Dupe::captureNestedSourceBox(LocalPlayer* localPlayer) {
    if (activeMode != 4 ||
        !localPlayer ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container) {
        return false;
    }

    const int sourceSlot = std::clamp(activeNestedSourceSlot, 0, 8);
    writeDiagnostic(
        "[BOXINBOX_SOURCE_CAPTURE_BEGIN] hotbarSlot=%d player=%p container=%p",
        sourceSlot,
        reinterpret_cast<void*>(localPlayer),
        reinterpret_cast<void*>(localPlayer->playerInventory->container)
    );
    ItemStack* source =
        localPlayer->playerInventory->container->getItem(sourceSlot);
    writeDiagnostic(
        "[BOXINBOX_SOURCE_GETITEM] hotbarSlot=%d source=%p",
        sourceSlot,
        reinterpret_cast<void*>(source)
    );
    if (!source || !source->isValid()) {
        writeDiagnostic(
            "[BOXINBOX_SOURCE_REJECT] hotbarSlot=%d reason=missing-or-invalid",
            sourceSlot
        );
        return false;
    }
    if (!isShulkerBox(*source)) {
        writeDiagnostic(
            "[BOXINBOX_SOURCE_REJECT] hotbarSlot=%d reason=not-shulker id=%d",
            sourceSlot,
            getItemId(*source)
        );
        return false;
    }

    writeDiagnostic(
        "[BOXINBOX_SOURCE_COPY_BEGIN] hotbarSlot=%d source=%p userData=%p",
        sourceSlot,
        reinterpret_cast<void*>(source),
        reinterpret_cast<void*>(source->mUserData)
    );
    resetNativeItemStack(cachedNestedSourceBox);
    cachedNestedSourceBox = std::make_unique<ItemStack>(*source);
    cachedNestedSourceBox->mCount = 1;
    writeDiagnostic(
        "[BOXINBOX_SOURCE] hotbarSlot=%d id=%d aux=%d userData=%p",
        sourceSlot,
        getItemId(*cachedNestedSourceBox),
        static_cast<int>(cachedNestedSourceBox->mAuxValue),
        reinterpret_cast<void*>(cachedNestedSourceBox->mUserData)
    );
    return true;
}

bool Dupe::prepareNativeNestedCraftingResult(ItemStack& nativeResult) {
    const bool aetherCraftCarrier = isEnabled() && !nestedOnlyModule &&
        activeMode == 9 && std::clamp(aetherCarrierMode, 0, 2) != 1;
    if (aetherCraftCarrier &&
        std::clamp(aetherCarrierMode, 0, 2) == 0 &&
        sawDyeShulkerIngredient && sawNamedDyeIngredient &&
        nativeResult.isValid() &&
        isShulkerBox(nativeResult) && NbtCommand::hasLoadedTag()) {
        ItemStack carrier(nativeResult);
        NativeItemStackResetGuard carrierGuard(&carrier);
        std::unique_ptr<CompoundTag> loaded = NbtCommand::copyLoadedTag();
        if (!loaded) return false;
        std::size_t modernCount = 0;
        std::size_t legacyCount = 0;
        if (stripExportOnlyLegacyItems(
            loaded.get(), modernCount, legacyCount)) {
            writeDiagnostic(
                "[AETHER_WIRE_CANONICALIZE] path=early-copy removedLegacy=1 modernItems=%zu legacyItems=%zu",
                modernCount,
                legacyCount
            );
        }
        carrier.mUserData = reinterpret_cast<__int64>(loaded.get());

        ItemStack rebuilt;
        NativeItemStackResetGuard rebuiltGuard(&rebuilt);
        const bool rebuiltSuccessfully =
            rebuildNativeItemStack(carrier, rebuilt);
        carrier.mUserData = 0;
        if (!rebuiltSuccessfully) {
            writeDiagnostic("[AETHER_DYE_EARLY_ABORT] native rebuild failed");
            return false;
        }
        rebuilt.item = nativeResult.item;
        rebuilt.mBlock = nativeResult.mBlock;
        rebuilt.mAuxValue = nativeResult.mAuxValue;
        rebuilt.mCount = nativeResult.mCount;
        rebuilt.mValid = nativeResult.mValid;
        rebuilt.mNetIdVariant = nativeResult.mNetIdVariant;
        if (getShulkerContentEntryCount(rebuilt) <= 0) return false;

        resetNativeItemStack(cachedEarlyNestedResult);
        cachedEarlyNestedResult = std::make_unique<ItemStack>(rebuilt);
        nestedEarlyResultInjected = false;
        writeDiagnostic(
            "[AETHER_DYE_EARLY_CACHE] id=%d items=%d userData=%p",
            getItemId(rebuilt), getShulkerContentEntryCount(rebuilt),
            reinterpret_cast<void*>(rebuilt.mUserData)
        );
        return true;
    }

    if (!isEnabled() ||
        !nestedOnlyModule ||
        activeMode != 4 ||
        activeNestedCarrierMode < 1 ||
        !nativeResult.isValid() ||
        !isShulkerBox(nativeResult)) {
        return false;
    }

    LocalPlayer* localPlayer = Game::getLocalPlayer();
    if (!localPlayer ||
        !captureNestedSourceBox(localPlayer) ||
        !cachedNestedSourceBox ||
        !cachedNestedSourceBox->isValid()) {
        writeDiagnostic("[NESTED_NATIVE_ABORT] source A unavailable");
        return false;
    }

    ItemStack sourceA(*cachedNestedSourceBox);
    NativeItemStackResetGuard sourceAGuard(&sourceA);
    sourceA.mCount = 1;

    ItemStack rebuiltB(sourceA);
    NativeItemStackResetGuard rebuiltBGuard(&rebuiltB);
    rebuiltB.item = nativeResult.item;
    rebuiltB.mBlock = nativeResult.mBlock;
    rebuiltB.mAuxValue = nativeResult.mAuxValue;
    rebuiltB.mCount = nativeResult.mCount;
    rebuiltB.mValid = nativeResult.mValid;
    rebuiltB.mNetIdVariant = nativeResult.mNetIdVariant;
    auto* bRoot = reinterpret_cast<CompoundTag*>(rebuiltB.mUserData);
    MutableShulkerItemsEntry bItemsEntry;
    if (!findMutableShulkerItemsEntry(bRoot, bItemsEntry)) {
        writeDiagnostic(
            "[NESTED_NATIVE_ABORT] source A has no native Items list"
        );
        return false;
    }

    ListTag* bItems = bItemsEntry.iterator->second.get<ListTag>();
    if (!bItems) {
        return false;
    }
    bItems->mList.clear();
    bItems->mType = Tag::Type::Compound;
    std::unique_ptr<CompoundTag> nestedA = saveNativeItemEntry(sourceA);
    if (!nestedA || !putNativeByte(nestedA.get(), "Slot", 0)) {
        writeDiagnostic("[NESTED_NATIVE_ABORT] could not serialize inner A");
        return false;
    }
    bItems->mList.emplace_back(std::move(nestedA));

    ItemStack displayDonor(sourceA);
    NativeItemStackResetGuard displayDonorGuard(&displayDonor);
    auto* displayRoot = reinterpret_cast<CompoundTag*>(displayDonor.mUserData);
    TagMap::node_type displayNode;
    if (displayRoot) {
        for (const char* key : { "display", "Display" }) {
            auto iterator = displayRoot->mTags.find(key);
            if (iterator == displayRoot->mTags.end()) {
                continue;
            }
            displayNode = displayRoot->mTags.extract(iterator);
            displayNode.key() = "display";
            break;
        }
    }
    if (!displayNode.empty()) {
        bRoot->mTags.erase("display");
        bRoot->mTags.erase("Display");
        bRoot->mTags.insert(std::move(displayNode));
    }

    std::unique_ptr<CompoundTag> persisted = saveNativeItemEntry(rebuiltB);
    if (!persisted || !isExecutableAddress(Addresses::ItemStack_loadItem)) {
        writeDiagnostic("[NESTED_NATIVE_ABORT] rebuilt B could not be persisted");
        return false;
    }

    ItemStack stagedResult;
    NativeItemStackResetGuard stagedResultGuard(&stagedResult);
    dupeLoadItem(stagedResult, *persisted);
    stagedResult.mCount = nativeResult.mCount;
    stagedResult.mNetIdVariant = nativeResult.mNetIdVariant;
    if (!stagedResult.isValid() ||
        !isShulkerBox(stagedResult) ||
        getShulkerContentEntryCount(stagedResult) != 1) {
        writeDiagnostic("[NESTED_NATIVE_ABORT] cached result failed validation");
        return false;
    }

    resetNativeItemStack(cachedEarlyNestedResult);
    cachedEarlyNestedResult = std::make_unique<ItemStack>(stagedResult);
    nestedEarlyResultInjected = false;
    writeDiagnostic(
        "[NESTED_NATIVE_CACHE] result=%p items=1 userData=%p",
        reinterpret_cast<void*>(&nativeResult),
        reinterpret_cast<void*>(cachedEarlyNestedResult->mUserData)
    );
    return true;
}

int Dupe::prepareEarlyNestedAction(
    const InventoryAction& action,
    ItemStack& replacement
) {
    const bool aetherCraftCarrier = isEnabled() && !nestedOnlyModule &&
        activeMode == 9 && std::clamp(aetherCarrierMode, 0, 2) != 1;
    const bool dedicatedNested = isEnabled() && nestedOnlyModule &&
        activeMode == 4 && activeNestedCarrierMode >= 1;
    if (!aetherCraftCarrier && !dedicatedNested) {
        return 0;
    }
    int expectedEarlyItems = aetherCraftCarrier && cachedEarlyNestedResult
        ? getShulkerContentEntryCount(*cachedEarlyNestedResult)
        : 1;

    const int containerId = static_cast<int>(action.source.containerId);
    if (aetherCraftCarrier &&
        action.source.type ==
        InventorySourceType::NonImplementedFeatureTODO &&
        containerId == kCraftingResultWindowId &&
        action.fromItem.isValid() &&
        (!cachedEarlyNestedResult ||
            !cachedEarlyNestedResult->isValid()) &&
        NbtCommand::hasLoadedTag()) {
        ItemStack carrier(action.fromItem);
        NativeItemStackResetGuard carrierGuard(&carrier);
        std::unique_ptr<CompoundTag> loaded = NbtCommand::copyLoadedTag();
        if (loaded) {
            std::size_t modernCount = 0;
            std::size_t legacyCount = 0;
            if (stripExportOnlyLegacyItems(
                loaded.get(), modernCount, legacyCount)) {
                writeDiagnostic(
                    "[AETHER_WIRE_CANONICALIZE] path=early-minus4 removedLegacy=1 modernItems=%zu legacyItems=%zu",
                    modernCount,
                    legacyCount
                );
            }
            carrier.mUserData = reinterpret_cast<__int64>(loaded.get());
            ItemStack rebuilt;
            NativeItemStackResetGuard rebuiltGuard(&rebuilt);
            const bool rebuiltSuccessfully =
                rebuildNativeItemStack(carrier, rebuilt);
            carrier.mUserData = 0;
            if (rebuiltSuccessfully) {
                rebuilt.item = action.fromItem.item;
                rebuilt.mBlock = action.fromItem.mBlock;
                rebuilt.mAuxValue = action.fromItem.mAuxValue;
                rebuilt.mCount = action.fromItem.mCount;
                rebuilt.mValid = action.fromItem.mValid;
                rebuilt.mNetIdVariant = action.fromItem.mNetIdVariant;
                if (rebuilt.isValid() &&
                    getShulkerContentEntryCount(rebuilt) > 0) {
                    resetNativeItemStack(cachedEarlyNestedResult);
                    cachedEarlyNestedResult =
                        std::make_unique<ItemStack>(rebuilt);
                    expectedEarlyItems =
                        getShulkerContentEntryCount(rebuilt);
                    writeDiagnostic(
                        "[AETHER_DIRECT_OUTPUT_CACHE] id=%d aux=%d items=%d userData=%p",
                        getItemId(rebuilt),
                        static_cast<int>(rebuilt.mAuxValue),
                        getShulkerContentEntryCount(rebuilt),
                        reinterpret_cast<void*>(rebuilt.mUserData)
                    );
                }
            }
        }
    }

    if (action.source.type ==
        InventorySourceType::NonImplementedFeatureTODO &&
        containerId == kCraftingResultWindowId &&
        action.fromItem.isValid() &&
        cachedEarlyNestedResult &&
        cachedEarlyNestedResult->isValid()) {
        std::unique_ptr<CompoundTag> persisted =
            saveNativeItemEntry(*cachedEarlyNestedResult);
        if (!persisted) {
            writeDiagnostic("[NESTED_EARLY_-4_ABORT] cache save failed");
            return 0;
        }

        dupeLoadItem(replacement, *persisted);
        replacement.item = action.fromItem.item;
        replacement.mBlock = action.fromItem.mBlock;
        replacement.mAuxValue = action.fromItem.mAuxValue;
        replacement.mCount = action.fromItem.mCount;
        replacement.mValid = action.fromItem.mValid;
        replacement.mNetIdVariant = action.fromItem.mNetIdVariant;
        if (!replacement.isValid() ||
            getShulkerContentEntryCount(replacement) != expectedEarlyItems) {
            replacement.setNull(std::nullopt);
            writeDiagnostic("[NESTED_EARLY_-4_ABORT] replacement invalid");
            return 0;
        }

        nestedEarlyResultInjected = true;
        writeDiagnostic(
            "[NESTED_EARLY_-4] items=%d fromNet=%llX userData=%p",
            expectedEarlyItems,
            static_cast<unsigned long long>(
                hashBytes(
                    &action.fromItem.mNetIdVariant,
                    sizeof(action.fromItem.mNetIdVariant)
                )
                ),
            reinterpret_cast<void*>(replacement.mUserData)
        );
        return 1;
    }

    if (aetherCraftCarrier &&
        action.source.type == InventorySourceType::ContainerInventory &&
        cachedEarlyNestedResult &&
        cachedEarlyNestedResult->isValid() &&
        action.toItem.isValid() &&
        action.toItem.item.get() == cachedEarlyNestedResult->item.get() &&
        action.toItem.mAuxValue == cachedEarlyNestedResult->mAuxValue) {
        std::unique_ptr<CompoundTag> persisted =
            saveNativeItemEntry(*cachedEarlyNestedResult);
        if (!persisted) return 0;
        dupeLoadItem(replacement, *persisted);
        replacement.item = action.toItem.item;
        replacement.mBlock = action.toItem.mBlock;
        replacement.mAuxValue = action.toItem.mAuxValue;
        replacement.mCount = action.toItem.mCount;
        replacement.mValid = action.toItem.mValid;
        replacement.mNetIdVariant = action.toItem.mNetIdVariant;
        if (!replacement.isValid() ||
            getShulkerContentEntryCount(replacement) != expectedEarlyItems) {
            replacement.setNull(std::nullopt);
            return 0;
        }
        nestedEarlyResultInjected = true;
        writeDiagnostic(
            "[AETHER_EARLY_DESTINATION] slot=%u items=%d userData=%p",
            action.slot,
            expectedEarlyItems,
            reinterpret_cast<void*>(replacement.mUserData)
        );
        return 2;
    }

    if (aetherCraftCarrier) return 0;

    if (action.source.type ==
        InventorySourceType::NonImplementedFeatureTODO &&
        containerId == kCraftingResultWindowId &&
        action.fromItem.isValid() &&
        isShulkerBox(action.fromItem)) {
        LocalPlayer* localPlayer = Game::getLocalPlayer();
        if (!localPlayer ||
            !captureNestedSourceBox(localPlayer) ||
            !cachedNestedSourceBox ||
            !cachedNestedSourceBox->isValid()) {
            return 0;
        }

        ItemStack sourceA(*cachedNestedSourceBox);
        NativeItemStackResetGuard sourceAGuard(&sourceA);
        sourceA.mCount = 1;
        ItemStack nativeB(action.fromItem);
        NativeItemStackResetGuard nativeBGuard(&nativeB);
        auto* bRoot = reinterpret_cast<CompoundTag*>(nativeB.mUserData);
        MutableShulkerItemsEntry bItemsEntry;
        if (!findMutableShulkerItemsEntry(bRoot, bItemsEntry)) {
            return 0;
        }
        ListTag* bItems = bItemsEntry.iterator->second.get<ListTag>();
        if (!bItems) {
            return 0;
        }
        bItems->mList.clear();
        bItems->mType = Tag::Type::Compound;
        std::unique_ptr<CompoundTag> nestedA =
            saveNativeItemEntry(sourceA);
        if (!nestedA || !putNativeByte(nestedA.get(), "Slot", 0)) {
            return 0;
        }
        bItems->mList.emplace_back(std::move(nestedA));

        ItemStack displayDonor(sourceA);
        NativeItemStackResetGuard displayDonorGuard(&displayDonor);
        auto* displayRoot = reinterpret_cast<CompoundTag*>(
            displayDonor.mUserData
            );
        TagMap::node_type displayNode;
        if (displayRoot) {
            for (const char* key : { "display", "Display" }) {
                auto iterator = displayRoot->mTags.find(key);
                if (iterator == displayRoot->mTags.end()) {
                    continue;
                }
                displayNode = displayRoot->mTags.extract(iterator);
                displayNode.key() = "display";
                break;
            }
        }
        if (displayNode.empty()) {
            return 0;
        }
        bRoot->mTags.erase("display");
        bRoot->mTags.erase("Display");
        if (!bRoot->mTags.insert(std::move(displayNode)).inserted) {
            return 0;
        }

        nativeB.item = action.fromItem.item;
        nativeB.mBlock = action.fromItem.mBlock;
        nativeB.mAuxValue = action.fromItem.mAuxValue;
        nativeB.mCount = action.fromItem.mCount;
        nativeB.mValid = action.fromItem.mValid;
        nativeB.mNetIdVariant = action.fromItem.mNetIdVariant;
        std::unique_ptr<CompoundTag> persisted =
            saveNativeItemEntry(nativeB);
        if (!persisted ||
            !isExecutableAddress(Addresses::ItemStack_loadItem)) {
            return 0;
        }

        dupeLoadItem(replacement, *persisted);
        replacement.mCount = action.fromItem.mCount;
        replacement.mNetIdVariant = action.fromItem.mNetIdVariant;
        if (!replacement.isValid() ||
            replacement.item.get() != action.fromItem.item.get() ||
            replacement.mAuxValue != action.fromItem.mAuxValue ||
            getShulkerContentEntryCount(replacement) != expectedEarlyItems) {
            replacement.setNull(std::nullopt);
            return 0;
        }

        resetNativeItemStack(cachedEarlyNestedResult);
        cachedEarlyNestedResult =
            std::make_unique<ItemStack>(replacement);
        nestedEarlyResultInjected = false;
        return 1;
    }

    if (action.source.type == InventorySourceType::ContainerInventory &&
        cachedEarlyNestedResult &&
        cachedEarlyNestedResult->isValid() &&
        action.toItem.isValid() &&
        action.toItem.item.get() ==
        cachedEarlyNestedResult->item.get() &&
        action.toItem.mAuxValue ==
        cachedEarlyNestedResult->mAuxValue) {
        std::unique_ptr<CompoundTag> persisted =
            saveNativeItemEntry(*cachedEarlyNestedResult);
        if (!persisted) {
            return 0;
        }
        dupeLoadItem(replacement, *persisted);
        replacement.mCount = action.toItem.mCount;
        replacement.mNetIdVariant = action.toItem.mNetIdVariant;
        if (!replacement.isValid() ||
            getShulkerContentEntryCount(replacement) != 1) {
            replacement.setNull(std::nullopt);
            return 0;
        }
        nestedEarlyResultInjected = true;
        writeDiagnostic(
            "[NESTED_EARLY_DESTINATION] slot=%u items=1 userData=%p",
            action.slot,
            reinterpret_cast<void*>(replacement.mUserData)
        );
        return 2;
    }

    return 0;
}

int Dupe::prepareEarlySourceBoxAction(
    const InventoryAction& action,
    ItemStack& replacement
) {
    (void)replacement;
    if (!isEnabled() ||
        !activeAnvilDupe2SourceBoxMode ||
        stage != Stage::WaitForRenameTake) {
        return 0;
    }

    const bool fromBox = action.fromItem.isValid() &&
        isShulkerBox(action.fromItem);
    const bool toBox = action.toItem.isValid() &&
        isShulkerBox(action.toItem);
    if (!fromBox && !toBox) {
        return 0;
    }

    const DescriptorPayloadSummary fromWire =
        inspectDescriptorPayload(action.fromItemDescriptor);
    const DescriptorPayloadSummary toWire =
        inspectDescriptorPayload(action.toItemDescriptor);
    writeDiagnostic(
        "[ANVIL_SOURCE_BOX_EARLY_ACTION] srcType=%d container=%d flags=%d slot=%u from={valid=%d,count=%u,items=%d,size=%zu,hash=%016llX} to={valid=%d,count=%u,items=%d,size=%zu,hash=%016llX}",
        static_cast<int>(action.source.type),
        static_cast<int>(static_cast<int8_t>(action.source.containerId)),
        static_cast<int>(action.source.flags),
        action.slot,
        fromBox ? 1 : 0,
        static_cast<unsigned int>(action.fromItem.mCount),
        fromBox ? getShulkerContentEntryCount(action.fromItem) : 0,
        fromWire.size,
        static_cast<unsigned long long>(fromWire.hash),
        toBox ? 1 : 0,
        static_cast<unsigned int>(action.toItem.mCount),
        toBox ? getShulkerContentEntryCount(action.toItem) : 0,
        toWire.size,
        static_cast<unsigned long long>(toWire.hash)
    );
    return 0;
}

bool Dupe::finalizeEarlyNestedDescriptors(
    InventoryTransactionPacket* packet
) {
    const bool aetherPrimaryOutputFlow =
        isEnabled() && !nestedOnlyModule && activeMode == 9 &&
        std::clamp(aetherCarrierMode, 0, 2) != 1;
    if (!packet ||
        !cachedEarlyNestedResult ||
        !cachedEarlyNestedResult->isValid()) {
        writeDiagnostic("[NESTED_FINAL_ABORT] cached result unavailable");
        return false;
    }

    ComplexInventoryTransaction* transaction =
        getValidatedTransaction(packet);
    if (!transaction) {
        writeDiagnostic("[NESTED_FINAL_ABORT] transaction unavailable");
        return false;
    }

    InventoryAction* resultAction = nullptr;
    InventoryAction* destinationAction = nullptr;
    InventorySource destinationMapSource;
    bool destinationMapSourceValid = false;
    for (auto& [source, actions] : transaction->data.actions) {
        const int containerId = static_cast<int>(source.containerId);
        for (InventoryAction& action : actions) {
            if (!resultAction &&
                source.type ==
                InventorySourceType::NonImplementedFeatureTODO &&
                containerId == kCraftingResultWindowId &&
                action.fromItem.isValid()) {
                resultAction = &action;
                continue;
            }

            if (!destinationAction &&
                source.type == InventorySourceType::ContainerInventory &&
                action.toItem.isValid() &&
                action.toItem.item.get() ==
                cachedEarlyNestedResult->item.get() &&
                action.toItem.mAuxValue ==
                cachedEarlyNestedResult->mAuxValue) {
                destinationAction = &action;
                destinationMapSource = source;
                destinationMapSourceValid = true;
            }
        }
    }

    if (!resultAction || !destinationAction) {
        writeDiagnostic(
            "[NESTED_FINAL_ABORT] result=%p destination=%p",
            reinterpret_cast<void*>(resultAction),
            reinterpret_cast<void*>(destinationAction)
        );
        return false;
    }

    const bool useGridCarrier = !aetherPrimaryOutputFlow &&
        (activeNestedCarrierMode == 2 || activeNestedCarrierMode == 3);
    if (useGridCarrier) {
        if (!destinationMapSourceValid) {
            writeDiagnostic("[NESTED_GRID_ABORT] destination map source missing");
            return false;
        }

        auto destinationGroupIterator =
            transaction->data.actions.find(destinationMapSource);
        if (destinationGroupIterator == transaction->data.actions.end() ||
            destinationGroupIterator->second.size() != 1 ||
            &destinationGroupIterator->second.front() != destinationAction) {
            writeDiagnostic(
                "[NESTED_GRID_ABORT] destination group shape is not singular"
            );
            return false;
        }

        std::vector<InventoryAction> destinationGroup =
            std::move(destinationGroupIterator->second);
        transaction->data.actions.erase(destinationGroupIterator);

        InventorySource gridSource;
        gridSource.type =
            InventorySourceType::NonImplementedFeatureTODO;
        gridSource.containerId = static_cast<ContainerID>(
            activeNestedCarrierMode == 2
            ? kCraftingAddIngredientWindowId
            : kCraftingRemoveIngredientWindowId
            );
        gridSource.flags = InventorySourceFlags::NoFlag;

        destinationGroup.front().source = gridSource;
        destinationGroup.front().slot = 0;
        auto insertion = transaction->data.actions.emplace(
            gridSource,
            std::move(destinationGroup)
        );
        if (!insertion.second || insertion.first->second.empty()) {
            writeDiagnostic("[NESTED_GRID_ABORT] could not re-key destination");
            return false;
        }
        destinationAction = &insertion.first->second.front();
        writeDiagnostic(
            "[NESTED_GRID_CARRIER] source=%d slot=%u fromItems=%d toItems=%d",
            static_cast<int>(gridSource.containerId),
            destinationAction->slot,
            getShulkerContentEntryCount(destinationAction->fromItem),
            getShulkerContentEntryCount(destinationAction->toItem)
        );
    }

    std::unique_ptr<CompoundTag> persisted =
        saveNativeItemEntry(*cachedEarlyNestedResult);
    if (!persisted || !isExecutableAddress(Addresses::ItemStack_loadItem)) {
        writeDiagnostic("[NESTED_FINAL_ABORT] result save failed");
        return false;
    }

    ItemStack richResult;
    NativeItemStackResetGuard richResultGuard(&richResult);
    dupeLoadItem(richResult, *persisted);
    richResult.item = resultAction->fromItem.item;
    richResult.mBlock = resultAction->fromItem.mBlock;
    richResult.mAuxValue = resultAction->fromItem.mAuxValue;
    richResult.mCount = resultAction->fromItem.mCount;
    richResult.mValid = resultAction->fromItem.mValid;
    richResult.mNetIdVariant = resultAction->fromItem.mNetIdVariant;

    ItemStack richDestination;
    NativeItemStackResetGuard richDestinationGuard(&richDestination);
    dupeLoadItem(richDestination, *persisted);
    richDestination.item = destinationAction->toItem.item;
    richDestination.mBlock = destinationAction->toItem.mBlock;
    richDestination.mAuxValue = destinationAction->toItem.mAuxValue;
    richDestination.mCount = destinationAction->toItem.mCount;
    richDestination.mValid = destinationAction->toItem.mValid;
    richDestination.mNetIdVariant = destinationAction->toItem.mNetIdVariant;

    const int expectedRichItems =
        getShulkerContentEntryCount(*cachedEarlyNestedResult);
    const int resultRichItems =
        getShulkerContentEntryCount(richResult);
    const int destinationRichItems =
        getShulkerContentEntryCount(richDestination);
    if (!richResult.isValid() ||
        !richDestination.isValid() ||
        expectedRichItems <= 0 ||
        resultRichItems != expectedRichItems ||
        destinationRichItems != expectedRichItems) {
        writeDiagnostic(
            "[NESTED_FINAL_ABORT] rebuilt stacks invalid expected=%d result=%d destination=%d",
            expectedRichItems,
            resultRichItems,
            destinationRichItems
        );
        return false;
    }

    const DescriptorPayloadSummary resultPayloadBefore =
        inspectDescriptorPayload(resultAction->fromItemDescriptor);
    const DescriptorPayloadSummary destinationPayloadBefore =
        inspectDescriptorPayload(destinationAction->toItemDescriptor);

    resultAction->fromItem.setNull(std::nullopt);
    new (&resultAction->fromItem) ItemStack(richResult);
    destinationAction->toItem.setNull(std::nullopt);
    new (&destinationAction->toItem) ItemStack(richDestination);

    resultAction->fromItemDescriptor =
        NetworkItemStackDescriptor(&resultAction->fromItem);
    destinationAction->toItemDescriptor =
        NetworkItemStackDescriptor(&destinationAction->toItem);

    const uintptr_t preservedDestinationUserData =
        static_cast<uintptr_t>(destinationAction->toItem.mUserData);

    const uint64_t resultDescriptorHash = hashBytes(
        &resultAction->fromItemDescriptor,
        sizeof(resultAction->fromItemDescriptor)
    );
    const uint64_t destinationDescriptorHash = hashBytes(
        &destinationAction->toItemDescriptor,
        sizeof(destinationAction->toItemDescriptor)
    );
    const DescriptorPayloadSummary resultPayloadAfter =
        inspectDescriptorPayload(resultAction->fromItemDescriptor);
    const DescriptorPayloadSummary destinationPayloadAfter =
        inspectDescriptorPayload(destinationAction->toItemDescriptor);
    if (aetherPrimaryOutputFlow) {
        writeDiagnostic(
            "[AETHER_DIRECT_OUTPUT_FORGE] baseId=%d baseAux=%d resultContainer=-4 resultSlot=%u destinationContainer=%d destinationSlot=%u inputsUntouched=1 resultPayload=%016llX destinationPayload=%016llX payloadMatch=%d items=%d",
            getItemId(resultAction->fromItem),
            static_cast<int>(resultAction->fromItem.mAuxValue),
            resultAction->slot,
            static_cast<int>(destinationMapSource.containerId),
            destinationAction->slot,
            static_cast<unsigned long long>(resultPayloadAfter.hash),
            static_cast<unsigned long long>(destinationPayloadAfter.hash),
            resultPayloadAfter.readable &&
            destinationPayloadAfter.readable &&
            resultPayloadAfter.hash == destinationPayloadAfter.hash &&
            resultPayloadAfter.size == destinationPayloadAfter.size
            ? 1 : 0,
            expectedRichItems
        );
    }
    writeDiagnostic(
        "[NESTED_FINAL_DESCRIPTORS] resultSlot=%u destinationSlot=%u resultItems=%d destinationItems=%d preservedDestinationUserData=%p resultDesc=%016llX destinationDesc=%016llX resultNet=%016llX destinationNet=%016llX",
        resultAction->slot,
        destinationAction->slot,
        getShulkerContentEntryCount(resultAction->fromItem),
        getShulkerContentEntryCount(destinationAction->toItem),
        reinterpret_cast<void*>(preservedDestinationUserData),
        static_cast<unsigned long long>(resultDescriptorHash),
        static_cast<unsigned long long>(destinationDescriptorHash),
        static_cast<unsigned long long>(hashBytes(
            &resultAction->fromItem.mNetIdVariant,
            sizeof(resultAction->fromItem.mNetIdVariant)
        )),
        static_cast<unsigned long long>(hashBytes(
            &destinationAction->toItem.mNetIdVariant,
            sizeof(destinationAction->toItem.mNetIdVariant)
        ))
    );
    writeDiagnostic(
        "[NESTED_WIRE_PAYLOAD] resultBefore={ok=%d size=%zu cap=%zu hash=%016llX items=%d legacy=%d name=%d} destinationBefore={ok=%d size=%zu cap=%zu hash=%016llX items=%d legacy=%d name=%d} resultAfter={ok=%d size=%zu cap=%zu hash=%016llX prefix=%016llX items=%d legacy=%d name=%d net=%u block=%d} destinationAfter={ok=%d size=%zu cap=%zu hash=%016llX prefix=%016llX items=%d legacy=%d name=%d net=%u block=%d}",
        resultPayloadBefore.readable ? 1 : 0,
        resultPayloadBefore.size,
        resultPayloadBefore.capacity,
        static_cast<unsigned long long>(resultPayloadBefore.hash),
        resultPayloadBefore.containsItems ? 1 : 0,
        resultPayloadBefore.containsLegacyItems ? 1 : 0,
        resultPayloadBefore.containsCustomName ? 1 : 0,
        destinationPayloadBefore.readable ? 1 : 0,
        destinationPayloadBefore.size,
        destinationPayloadBefore.capacity,
        static_cast<unsigned long long>(destinationPayloadBefore.hash),
        destinationPayloadBefore.containsItems ? 1 : 0,
        destinationPayloadBefore.containsLegacyItems ? 1 : 0,
        destinationPayloadBefore.containsCustomName ? 1 : 0,
        resultPayloadAfter.readable ? 1 : 0,
        resultPayloadAfter.size,
        resultPayloadAfter.capacity,
        static_cast<unsigned long long>(resultPayloadAfter.hash),
        static_cast<unsigned long long>(resultPayloadAfter.prefix),
        resultPayloadAfter.containsItems ? 1 : 0,
        resultPayloadAfter.containsLegacyItems ? 1 : 0,
        resultPayloadAfter.containsCustomName ? 1 : 0,
        static_cast<unsigned int>(resultPayloadAfter.hasNetId),
        resultPayloadAfter.blockRuntimeId,
        destinationPayloadAfter.readable ? 1 : 0,
        destinationPayloadAfter.size,
        destinationPayloadAfter.capacity,
        static_cast<unsigned long long>(destinationPayloadAfter.hash),
        static_cast<unsigned long long>(destinationPayloadAfter.prefix),
        destinationPayloadAfter.containsItems ? 1 : 0,
        destinationPayloadAfter.containsLegacyItems ? 1 : 0,
        destinationPayloadAfter.containsCustomName ? 1 : 0,
        static_cast<unsigned int>(destinationPayloadAfter.hasNetId),
        destinationPayloadAfter.blockRuntimeId
    );

    rewrittenDestinationSlots.clear();
    expectedDestinationCounts.fill(0);
    if (!useGridCarrier &&
        destinationAction->slot < kInventorySlotCount) {
        rewrittenDestinationSlots.emplace_back(
            static_cast<int>(destinationAction->slot)
        );
        expectedDestinationCounts[destinationAction->slot] =
            destinationAction->toItem.mCount;
    }
    if (useGridCarrier) {
        writeDiagnostic(
            "[NESTED_GRID_RESULT_ARMED] source=%d gridSlot=%u count=%u items=%d",
            activeNestedCarrierMode == 2
            ? kCraftingAddIngredientWindowId
            : kCraftingRemoveIngredientWindowId,
            destinationAction->slot,
            static_cast<unsigned int>(destinationAction->toItem.mCount),
            getShulkerContentEntryCount(destinationAction->toItem)
        );
    }
    else {
        writeDiagnostic(
            "[NESTED_LOCAL_PREDICTION_ARMED] slot=%u count=%u items=%d",
            destinationAction->slot,
            static_cast<unsigned int>(destinationAction->toItem.mCount),
            getShulkerContentEntryCount(destinationAction->toItem)
        );
    }
    return true;
}

void Dupe::onEnable() {
    if (nestedOnlyModule) {
        activeMode = 4;
    }
    else if (publicOnlyModule) {
        activeMode = 3;
    }
    else {
        mode = std::clamp(mode, 0, 4);
        activeMode = mode == 0
            ? 3
            : mode == 1
            ? 5
            : mode == 2
            ? 7
            : mode == 3
            ? 8
            : mode == 4
            ? 4
            : 9;
    }
    mapartLoadPending = false;
    nestedGeneratePending = false;
    mapartLoadTicks = 0;
    mapartNbtRequested = false;
    mapartNbtReady = false;
    aetherNbtLoadName.clear();
    aetherUsingDefaultNbt = false;
    const bool useNestedNbt = activeMode == 9 && aetherBoxInBox;
    const bool useMapartNbt = activeMode == 9 && aetherMapart && !useNestedNbt;
    const bool useDefaultNbt = activeMode == 9 && aetherDefaultNbt && !useNestedNbt && !useMapartNbt;
    if (activeMode == 9 && !useMapartNbt && !useNestedNbt && !useDefaultNbt && NbtCommand::hasLoadedTag() &&
        (NbtCommand::loadedName() == "map1" ||
            NbtCommand::loadedName() == "map1.json" ||
            NbtCommand::loadedName() == "nested" ||
            NbtCommand::loadedName() == "nested.json" ||
            NbtCommand::loadedName() == "default" ||
            NbtCommand::loadedName() == "default.json")) {
        CommandManager::execute(".nbt clear");
    }
    if (useNestedNbt && !nestedOnlyModule && !publicOnlyModule) {
        mapartNbtRequested = true;
        nestedGeneratePending = true;
    }
    else if (useMapartNbt && !nestedOnlyModule && !publicOnlyModule) {
        refreshMaparts();
        const std::string selectedMapart = getSelectedMapart();
        mapartNbtRequested = !selectedMapart.empty();
        if (!selectedMapart.empty() && NbtMap::generateMapart(
            selectedMapart, mapartTilesX, mapartTilesY,
            mapartReduceColors)) {
            aetherNbtLoadName = "map1";
            mapartLoadPending = true;
            displayMessage("%s generated; loading map1 in 2 seconds",
                selectedMapart.c_str());
        }
        else if (selectedMapart.empty()) {
            displayMessage("No supported image found in Kirara\\Maparts");
        }
        else {
            displayMessage("Failed to generate map NBT from %s",
                selectedMapart.c_str());
        }
    }
    else if (useDefaultNbt && !nestedOnlyModule && !publicOnlyModule) {
        refreshDefaultNbtFiles();
        const std::string selectedNbt = getSelectedDefaultNbt();
        mapartNbtRequested = !selectedNbt.empty();
        if (!selectedNbt.empty()) {
            aetherNbtLoadName = selectedNbt;
            aetherUsingDefaultNbt = true;
            mapartLoadPending = true;
        }
        else {
            displayMessage("No NBT file found in Kirara\\NBT");
        }
    }
    activeNestedCarrierMode = 0;
    activeNestedAllCrafting =
        activeMode == 4 &&
        !nestedOnlyModule &&
        nestedAllCrafting;
    activeCraftDropStack =
        activeMode == 3 && craftDropStack;
    activeDupe3Destination = activeCraftDropStack
        ? 0
        : std::clamp(dupe3Destination, 0, 1);
    activeDupe3AnyCrafting = activeMode == 3;
    activeDupe4Destination = std::clamp(dupe4Destination, 0, 1);
    activeDupe4AnyCrafting = activeMode == 6 && dupe4AnyCrafting;
    activeAnvilDupe2Destination = std::clamp(
        anvilDupe2Destination,
        0,
        1
    );
    activeAnvilDupe2SingleItem =
        activeMode == 5 && anvilDupe2SingleItem;
    activeAnvilDupe2SourceBoxMode =
        activeAnvilDupe2SingleItem && anvilDupe2SourceBoxMode;
    activeAnvilDupe2NbtWindowMode =
        activeAnvilDupe2SingleItem &&
        anvilDupe2NbtWindowMode &&
        !activeAnvilDupe2SourceBoxMode;
    activeAnvilDupe2SingleNbtExperiment = std::clamp(
        anvilDupe2SingleNbtExperiment,
        0,
        3
    );
    activeAnvilDupe2InnerItemLock =
        activeAnvilDupe2SingleItem
        ? std::clamp(anvilDupe2InnerItemLock, 0, 2)
        : 0;
    activeAnvilDupe2MaterialSlotMode =
        activeMode == 5 &&
        anvilDupe2MaterialSlotMode &&
        !activeAnvilDupe2SourceBoxMode &&
        !activeAnvilDupe2NbtWindowMode;

    activeAnvilSwapSourceSlot = std::clamp(
        anvilSwapSourceSlot,
        0,
        8
    );
    activeCopyCount = activeMode == 0
        ? anvilDupe3Drop
        ? std::clamp(anvilDupe3CopyCount, 1, 256)
        : std::clamp(copyCount, 1, 27)
        : activeMode == 5
        ? std::clamp(anvilDupe2CopyCount, 1, 27)
        : activeMode == 9
        ? std::clamp(aetherCraftAmount, 1, 64)
        : activeMode == 8
        ? 1
        : activeMode == 6
        ? std::clamp(dupe4CopyCount, 1, 256)
        : activeMode == 7
        ? std::clamp(enchantCopyCount, 1, 27)
        : activeMode == 3
        ? activeCraftDropStack
        ? 1
        : std::clamp(dupe3CopyCount, 1, 256)
        : activeMode == 4
        ? nestedOnlyModule
        ? 1
        : std::clamp(boxInBoxCopyCount, 1, 27)
        : std::clamp(dyeCopyCount, 1, 27);
    activeNestedSourceSlot = std::clamp(nestedSourceSlot, 0, 8);
    activeEnchantDonorSlot = std::clamp(enchantDonorSlot, 0, 8);
    activeAnvilDupe3Drop = activeMode == 0 && anvilDupe3Drop;
    activeAnvilDirectDrop =
        activeMode == 0 && anvilDirectDrop && !activeAnvilDupe3Drop;
    activeStacking =
        (stacking || activeCraftDropStack) &&
        activeMode != 4 &&
        activeMode != 7 &&
        !activeAnvilDupe3Drop &&
        !(activeMode == 5 &&
            (activeAnvilDupe2SingleItem ||
                activeAnvilDupe2MaterialSlotMode));
    activeItemStackRequestStacking =
        activeStacking &&
        itemStackRequestStacking &&
        (activeMode == 0 || activeMode == 5);
    activeStackCount = std::clamp(stackCount, 1, 255);
    sourceBoxRequestDiagnosticCount = 0;
    resetDiagnosticLog();
    resetAttemptState();

    if (activeMode == 5) {
        if (activeAnvilDupe2NbtWindowMode) {
            displayMessage(
                "Dupe [anvildupe2 NBT Window -13]: preserve the native anvil result, balance one complete loaded ItemStack through -10 into the empty -13 slot, and extract it only after the server acknowledgement."
            );
        }
        else if (activeAnvilDupe2SourceBoxMode) {
            displayMessage(
                "Dupe [anvildupe2 Source Box]: replace Items on the genuine non-empty anvil source, synchronize its -10 mirror, and preserve the native -12 rename output."
            );
        }
        else if (activeAnvilDupe2MaterialSlotMode) {
            displayMessage(
                "Dupe [anvildupe2 Material Slot]: rename and take one real result; one staged item of any type will be read from the server anvil material slot and taken natively."
            );
        }
        else if (activeStacking) {
            displayMessage(
                "Dupe [anvildupe2 %s]: rename and take one real result; requesting %d destination(s) with %d item(s) each (255 total maximum).",
                activeAnvilDupe2Destination == 1
                ? "Inventory"
                : "World",
                activeCopyCount,
                activeStackCount
            );
        }
        else {
            displayMessage(
                "Dupe [anvildupe2 %s]: rename and take one real result; requesting %d destination(s) balanced by the ignored -10 source.",
                activeAnvilDupe2Destination == 1
                ? "Inventory"
                : "World",
                activeCopyCount
            );
        }
    }
    else if (activeMode == 0) {
        if (activeAnvilDupe3Drop) {
            displayMessage(
                "Dupe [Anvil Dupe3 Drop]: rename and take one real result; requesting %d native direct-world pair(s).",
                activeCopyCount
            );
        }
        else if (activeAnvilDirectDrop) {
            displayMessage(
                "Dupe [Anvil]: rename and take one real result; direct-drop mode is enabled."
            );
        }
        else {
            displayMessage(
                "Dupe [Anvil]: rename and take one real result; requesting %d destination(s), stacking %s at up to %d per slot.",
                activeCopyCount,
                activeStacking ? "enabled" : "disabled",
                activeStackCount
            );
        }
    }
    else if (activeMode == 7) {
        displayMessage(
            "Dupe [enchantdupe]: keep a genuinely enchanted donor in hotbar slot %d, then place one count-1 shulker and at least Copies+1 lapis into empty enchanting slots. Each requested result consumes one lapis and one XP level (up to %d).",
            activeEnchantDonorSlot,
            activeCopyCount
        );
    }
    else if (activeMode == 3 || activeMode == 6) {
        const int activeDyeDestination = activeMode == 6
            ? activeDupe4Destination
            : activeDupe3Destination;
        const bool activeAnyCraft = activeMode == 6
            ? activeDupe4AnyCrafting
            : activeDupe3AnyCrafting;
        if (activeDyeDestination == 1) {
            displayMessage(
                "Dupe [%s Inventory]: %s; requesting up to %d destination(s), stacking %s at up to %d per slot.",
                getDupeModeName(activeMode),
                activeAnyCraft
                ? "take any genuine crafting result"
                : "dye and take a shulker result",
                activeCopyCount,
                activeStacking ? "enabled" : "disabled",
                activeStackCount
            );
        }
        else {
            displayMessage(
                "Dupe [%s World]: %s; requesting %d world drop(s), stacking %s at up to %d per drop.",
                getDupeModeName(activeMode),
                activeAnyCraft
                ? "take any genuine crafting result"
                : "dye and take a shulker result",
                activeCopyCount,
                activeStacking ? "enabled" : "disabled",
                activeStackCount
            );
        }
    }
    else if (activeMode == 4) {
        displayMessage(
            "Nested [Anvil -10 Swap]: keep A in hotbar slot %d, rename and take B; -12 stays vanilla while -10.old balances one nested B into the genuine result destination.",
            activeNestedSourceSlot
        );
    }
    else {
        displayMessage(
            "Dupe [%s]: dye and take one real result; requesting %d destination(s), stacking %s at up to %d per slot.",
            getDupeModeName(activeMode),
            activeCopyCount,
            activeStacking ? "enabled" : "disabled",
            activeStackCount
        );
    }
}

void Dupe::onDisable() {
    mapartLoadPending = false;
    nestedGeneratePending = false;
    mapartLoadTicks = 0;
    mapartNbtRequested = false;
    mapartNbtReady = false;
    aetherNbtLoadName.clear();
    aetherUsingDefaultNbt = false;
    settleTicks = 0;
    captureWaitTicks = 0;
    packetCooldownTicks = 0;
    renameNoticePending = false;
    rewriteNoticePending = false;
    nestedAbortNoticePending = false;
    nestedRewriteMatchedAndRejected = false;
    rewrittenDestinationCount = 0;
    legacyPacketCount = 0;
    enchantOutgoingDiagnosticCount = 0;
    responseWaitTicks = 0;
    incomingDiagnosticCount = 0;
    anvilResyncProbeSent = false;
    aetherCraftResyncPending = false;
    anvilResyncAuthoritativePacketReceived = false;
    anvilPreTransactionOwnedCount = 0;
    nextDropSlot = kMainInventoryFirstSlot;
    dropTransactionCount = 0;
    dropFailureCount = 0;
    sawDyeShulkerIngredient = false;
    sawDyeOtherIngredient = false;
    sawNamedDyeIngredient = false;
    resetNativeItemStack(cachedDyeShulkerIngredient);
    resetNativeItemStack(cachedNestedSourceBox);
    resetNativeItemStack(cachedEnchantInput);
    resetNativeItemStack(cachedEnchantMaterial);
    resetNativeItemStack(cachedEnchantDonor);
    resetNativeItemStack(expectedResultStack);
    resetNativeItemStack(stagedAnvilOutputStack);
    stagedAnvilDestinationSlot = -1;
    anvilOutputStagePending = false;
    nbtWindowStagePending = false;
    nbtWindowFirstStageAccepted = false;
    nbtWindowSecondStageSent = false;
    anvilMaterialProbeSent = false;
    anvilMaterialInventoryPacketObserved = false;
    anvilMaterialInventoryPacketCount = 0;
    anvilMaterialReadbackCheckedPacketCount = 0;
    anvilMaterialReadbackState = kAnvilMaterialReadbackNone;
    anvilMaterialAuthoritativePacketReceived = false;
    anvilMaterialNativeTakeRequested = false;
    anvilMaterialNativeTakePacketObserved = false;
    anvilMaterialNativeTakePacketId = 0;
    anvilMaterialReadbackGraceUsed = false;
    anvilMaterialPreTakeSnapshotValid = false;
    anvilMaterialPreTakeCounts.fill(0);
    anvilMaterialPreTakeExpectedMatches.fill(false);
    anvilMaterialPreTakeEmptySlots.fill(false);
    anvilResyncInventoryPacketObserved = false;
    anvilResyncInventoryPacketCount = 0;
    anvilResyncReadbackState = kAnvilMaterialReadbackNone;
    anvilResyncClearedLocalSlots = 0;
    resetNativeItemStack(nestedLocalShellStack);
    enchantSettleTicks = 0;
    enchantTransactionPending = false;
    enchantPacketInFlight = false;
    enchantSequencePhase = EnchantSequencePhase::Idle;
    enchantSequenceDestinationSlots.clear();
    enchantSequenceCompletedSlots.clear();
    enchantSequenceIndex = 0;
    enchantSequenceInitialMaterialCount = 0;
    expectedDestinationCounts.fill(0);
    expectedResultItem = nullptr;
    expectedResultAux = 0;
    expectedResultCount = 0;
    waitingForCursorPlacement = false;
    cursorSnapshotValid = false;
    emptyBeforeCursor.fill(false);
    pendingDestinationSlots.clear();
    rewrittenDestinationSlots.clear();
    nextDestinationIndex = 0;
}

void Dupe::onNormalTick(LocalPlayer* localPlayer) {
    if (nestedGeneratePending) {
        nestedGeneratePending = false;
        if (generateNestedNbt(localPlayer)) {
            aetherNbtLoadName = "nested";
            mapartLoadTicks = 0;
            mapartLoadPending = true;
        }
        else {
            mapartNbtRequested = false;
            mapartNbtReady = false;
        }
    }
    if (mapartLoadPending && ++mapartLoadTicks >= 40) {
        mapartLoadPending = false;
        mapartLoadTicks = 0;
        const std::string sourceNbtName = aetherNbtLoadName;
        const std::string transformedNbtName = aetherUsingDefaultNbt
            ? "default"
            : sourceNbtName;
        if (aetherEnchantItems) {
            std::vector<NbtCommand::EnchantmentEdit> selected;
            for (size_t index = 0; index < selectedAetherEnchantments.size() &&
                index < aetherEnchantIds.size() && index < aetherEnchantLevels.size(); ++index) {
                if (!selectedAetherEnchantments[index]) continue;
                const int16_t id = static_cast<int16_t>(aetherEnchantIds[index]);
                NbtCommand::EnchantmentEdit value{ id, static_cast<int16_t>(aetherEnchantLevels[index]) };
                auto existing = std::find_if(selected.begin(), selected.end(),
                    [id](const NbtCommand::EnchantmentEdit& entry) { return entry.id == id; });
                if (existing == selected.end()) selected.push_back(value);
                else *existing = value;
            }
            const int editMode = std::clamp(aetherEnchantEditMode, 0, 2);
            if (!NbtCommand::rewriteSavedEnchantments(
                sourceNbtName, transformedNbtName, selected, editMode)) {
                mapartNbtRequested = false;
                mapartNbtReady = false;
                writeDiagnostic("[AETHER_ENCHANT_ABORT] json mode=%d selected=%d",
                    editMode, static_cast<int>(selected.size()));
                return;
            }
            aetherNbtLoadName = transformedNbtName;
            writeDiagnostic("[AETHER_ENCHANT] json mode=%d selected=%d",
                editMode, static_cast<int>(selected.size()));
        }
        CommandManager::execute(".nbt load " + aetherNbtLoadName);
        mapartNbtReady = NbtCommand::hasLoadedTag() &&
            (NbtCommand::loadedName() == aetherNbtLoadName ||
                NbtCommand::loadedName() == aetherNbtLoadName + ".json");
        const int requestedItemLock = std::clamp(aetherInnerItemLock, 0, 2);
        if (mapartNbtReady && requestedItemLock != 0) {
            std::unique_ptr<CompoundTag> transformed =
                NbtCommand::copyLoadedTag();
            ShulkerItemLockWrite lockWrite;
            if (!transformed ||
                !writeShulkerItemLock(
                    transformed.get(),
                    static_cast<uint8_t>(requestedItemLock),
                    lockWrite
                ) ||
                !NbtCommand::saveTagJson(
                    transformedNbtName,
                    *transformed
                ) ||
                !NbtCommand::replaceLoadedTag(
                    std::move(transformed),
                    transformedNbtName + ".json"
                )) {
                mapartNbtReady = false;
                writeDiagnostic(
                    "[AETHER_ITEM_LOCK_ABORT] mode=%d",
                    requestedItemLock
                );
            }
            else {
                writeDiagnostic(
                    "[AETHER_ITEM_LOCK] mode=%d items=%llu changed=%llu createdTags=%llu",
                    requestedItemLock,
                    static_cast<unsigned long long>(lockWrite.itemCount),
                    static_cast<unsigned long long>(lockWrite.changedListCount),
                    static_cast<unsigned long long>(lockWrite.createdTagCount)
                );
            }
        }
        if (mapartNbtReady)
            displayMessage("Loaded generated map NBT: map1");
        else
            displayMessage("Failed to load generated map NBT; crafting rewrite is blocked");
    }
    if (!localPlayer) {
        return;
    }

    if (activeMode == 9 && aetherCraftResyncPending) {
        aetherCraftResyncPending = false;
        if (localPlayer->level &&
            localPlayer->level->getPacketSender() &&
            canConstructNativeInventoryTransaction()) {
            auto mismatch =
                std::make_unique<ComplexInventoryTransaction>();
            mismatch->type =
                ComplexInventoryTransaction::Type::InventoryMismatch;
            InventoryTransactionPacket resync(std::move(mismatch));
            localPlayer->level->getPacketSender()->send(&resync);
            writeDiagnostic("[AETHER_RESYNC] InventoryMismatch sent");
        }
    }

    if (activeMode == 7 && stage == Stage::WaitForRenameTake) {
        if (!enchantTransactionPending) {
            return;
        }
        if (enchantSettleTicks > 0) {
            --enchantSettleTicks;
            return;
        }

        if (!sendEnchantDupeTransaction(localPlayer)) {
            displayMessage(
                "Dupe [enchantdupe]: failed to construct the transaction; verify one shulker, at least two lapis and empty inventory slots."
            );
            setEnabled(false);
            return;
        }

        if (enchantSequencePhase != EnchantSequencePhase::Idle) {
            return;
        }

        rewriteNoticePending = true;
        responseWaitTicks = kEnchantResponseCaptureTicks;
        anvilResyncProbeSent = false;
        anvilResyncAuthoritativePacketReceived = false;
        stage = Stage::Completed;
        return;
    }

    if (nestedAbortNoticePending) {
        nestedAbortNoticePending = false;
        if (useNestedDyeDropCarrier()) {
            displayMessage(
                "Nested [Dye Drop]: blocked the dye-result take because A/B NBT, Count=1 placeholders, transaction shape, or payload validation failed. Reinsert B and dye before retrying."
            );
        }
        else {
            displayMessage(
                "Nested [Anvil -10 Swap]: blocked the anvil take because A/B NBT, the -10.old carrier, native reload, the genuine destination, or descriptor validation failed. Reinsert B before retrying."
            );
        }
    }

    if (rewriteNoticePending) {
        rewriteNoticePending = false;

        if ((activeMode == 4 && useNestedDyeDropCarrier()) ||
            activeMode == 9) {
            const int refreshedCount =
                refreshExpectedDestinations(localPlayer);
            writeDiagnostic(
                "[CRAFT_LOCAL_PREDICTION_APPLY] mode=%s refreshed=%d destinations=%llu",
                getDupeModeName(activeMode),
                refreshedCount,
                static_cast<unsigned long long>(
                    rewrittenDestinationSlots.size()
                    )
            );
        }

        if (rewrittenDestinationCount > 0) {
            if (activeMode == 5 && activeAnvilDupe2NbtWindowMode) {
                displayMessage(
                    "Dupe [anvildupe2 NBT Window -13]: staged one complete ItemStack in the server anvil slot; waiting for acknowledgement before extraction."
                );
            }
            else if (activeMode == 5) {
                displayMessage(
                    "Dupe [anvildupe2 %s]: kept the genuine result at its vanilla count and injected %d balanced -10-source destination(s).",
                    activeAnvilDupe2Destination == 1
                    ? "Inventory"
                    : "World",
                    rewrittenDestinationCount
                );
            }
            else if (activeMode == 0) {
                if (activeAnvilDupe3Drop) {
                    displayMessage(
                        "Dupe [Anvil Dupe3 Drop]: appended %d native result/world-drop pair(s).",
                        rewrittenDestinationCount
                    );
                }
                else if (activeAnvilDirectDrop) {
                    displayMessage(
                        "Dupe [Anvil]: appended a direct world drop containing %d duplicated item(s).",
                        rewrittenDestinationCount
                    );
                }
                else {
                    displayMessage(
                        "Dupe [Anvil]: modified the real transaction and added %d inventory destination(s).",
                        rewrittenDestinationCount
                    );
                }
            }
            else if (activeMode == 3 || activeMode == 6) {
                const int dyeDestination = activeMode == 6
                    ? activeDupe4Destination
                    : activeDupe3Destination;
                if (dyeDestination == 1) {
                    displayMessage(
                        "Dupe [%s Inventory]: appended %d rich-NBT inventory destination(s) to the real crafting transaction.",
                        getDupeModeName(activeMode),
                        rewrittenDestinationCount
                    );
                }
                else {
                    displayMessage(
                        "Dupe [%s World]: appended %d direct-world destination(s) to the real crafting transaction.",
                        getDupeModeName(activeMode),
                        rewrittenDestinationCount
                    );
                }
            }
            else if (activeMode == 7) {
                displayMessage(
                    "Dupe [enchantdupe]: kept the genuine input as the server witness, consumed one lapis per request and sent %d rich-NBT result request(s).",
                    rewrittenDestinationCount
                );
            }
            else if (activeMode == 4) {
                if (useNestedDyeDropCarrier()) {
                    displayMessage(
                        "Nested [Dye Drop]: replaced every occupied B slot with A from hotbar %d and requested %d direct world drop(s).",
                        activeNestedSourceSlot,
                        rewrittenDestinationCount
                    );
                }
                else {
                    displayMessage(
                        "Nested [Anvil -10 Swap]: kept -12 vanilla and paired A-inside-B from hotbar %d through -10.old with %d genuine result destination(s); the server still has to authorize it.",
                        activeNestedSourceSlot,
                        rewrittenDestinationCount
                    );
                }
            }
            else {
                displayMessage(
                    "Dupe [%s]: modified the real crafting transaction and added %d inventory destination(s).",
                    getDupeModeName(activeMode),
                    rewrittenDestinationCount
                );
            }
        }
        else if (activeMode == 3 || activeMode == 6) {
            const int dyeDestination = activeMode == 6
                ? activeDupe4Destination
                : activeDupe3Destination;
            displayMessage(
                "Dupe [%s %s]: the real crafting result matched, but no additional destination was available.",
                getDupeModeName(activeMode),
                dyeDestination == 1 ? "Inventory" : "World"
            );
        }
        else if (activeMode == 4) {
            if (useNestedDyeDropCarrier()) {
                displayMessage(
                    "Nested [Dye Drop]: the genuine dye result matched, but no direct drop was produced."
                );
            }
            else {
                displayMessage(
                    "Nested [Anvil -10 Swap]: no genuine result destination was rewritten, so no nested B was sent."
                );
            }
        }
        else {
            displayMessage(
                "Dupe: the real result transaction matched, but there were no eligible empty inventory slots."
            );
        }
        logInventorySnapshot("LOCAL_FIRST_TICK_AFTER_SEND");
        return;
    }

    if (stage == Stage::WaitForNbtWindowAck) {
        if (!nbtWindowStagePending ||
            !stagedAnvilOutputStack ||
            !stagedAnvilOutputStack->isValid()) {
            writeDiagnostic(
                "[ANVIL_NBT_WINDOW_ACK_ABORT] staged item state unavailable"
            );
            setEnabled(false);
            return;
        }

        if (responseWaitTicks > 0) {
            --responseWaitTicks;
            return;
        }

        writeDiagnostic(
            "[ANVIL_NBT_WINDOW_ACK_TIMEOUT] ticks=%d firstStageAccepted=%d secondStageSent=%d; probing -13 because the server may suppress the anvil sound",
            kNbtWindowAckTimeoutTicks,
            nbtWindowFirstStageAccepted ? 1 : 0,
            nbtWindowSecondStageSent ? 1 : 0
        );
        anvilMaterialProbeSent = false;
        anvilMaterialInventoryPacketObserved = false;
        anvilMaterialInventoryPacketCount = 0;
        anvilMaterialReadbackCheckedPacketCount = 0;
        anvilMaterialReadbackState = kAnvilMaterialReadbackNone;
        anvilMaterialAuthoritativePacketReceived = false;
        anvilMaterialReadbackGraceUsed = false;
        responseWaitTicks = kServerResponseCaptureTicks;
        settleTicks = 0;
        stage = Stage::ReadAnvilMaterial;
        if (!sendAnvilMaterialProbe(localPlayer)) {
            writeDiagnostic(
                "[ANVIL_NBT_WINDOW_SILENT_PROBE_ABORT] failed to construct or send -13 probe"
            );
            nbtWindowStagePending = false;
            setEnabled(false);
            return;
        }
        writeDiagnostic(
            "[ANVIL_NBT_WINDOW_SILENT_PROBE_WAIT] ticks=%d materialSlot=%u",
            responseWaitTicks,
            kAnvilMaterialInventorySlot
        );
        return;
    }

    if (stage == Stage::ExtractNbtWindow) {
        if (!nbtWindowStagePending ||
            !nbtWindowFirstStageAccepted ||
            nbtWindowSecondStageSent ||
            !stagedAnvilOutputStack ||
            !stagedAnvilOutputStack->isValid()) {
            writeDiagnostic(
                "[ANVIL_NBT_WINDOW_TAKE_ABORT] invalid two-stage state pending=%d accepted=%d secondSent=%d",
                nbtWindowStagePending ? 1 : 0,
                nbtWindowFirstStageAccepted ? 1 : 0,
                nbtWindowSecondStageSent ? 1 : 0
            );
            setEnabled(false);
            return;
        }

        if (settleTicks > 0) {
            --settleTicks;
            return;
        }

        if (!sendNbtWindowTakeTransaction(localPlayer)) {
            writeDiagnostic(
                "[ANVIL_NBT_WINDOW_TAKE_ABORT] failed to construct or send the server-slot extraction transaction"
            );
            nbtWindowStagePending = false;
            setEnabled(false);
            return;
        }

        nbtWindowSecondStageSent = true;
        nbtWindowStagePending = false;
        responseWaitTicks = kNbtWindowPostTakeSettleTicks;
        settleTicks = 0;
        stage = Stage::Completed;
        writeDiagnostic(
            "[ANVIL_NBT_WINDOW_TAKE_WAIT] ticks=%d destinationSlot=%d localPrediction=0",
            responseWaitTicks,
            stagedAnvilDestinationSlot
        );
        return;
    }

    if (stage == Stage::ExtractAnvilOutput) {
        const bool hasPendingMaterialStage = anvilOutputStagePending ||
            (activeAnvilDupe2NbtWindowMode && nbtWindowStagePending);
        if (!hasPendingMaterialStage ||
            !stagedAnvilOutputStack ||
            !stagedAnvilOutputStack->isValid()) {
            writeDiagnostic(
                "[ANVIL_-13_EXTRACT_ABORT] staged item state unavailable"
            );
            setEnabled(false);
            return;
        }

        if (settleTicks > 0) {
            --settleTicks;
            return;
        }

        responseWaitTicks = kServerResponseCaptureTicks;
        settleTicks = 0;
        stage = Stage::ReadAnvilMaterial;
        if (!sendAnvilMaterialProbe(localPlayer)) {
            writeDiagnostic(
                "[ANVIL_-13_MATERIAL_PROBE_ABORT] failed to construct or send material-slot probe"
            );
            setEnabled(false);
            return;
        }

        writeDiagnostic(
            "[ANVIL_-13_MATERIAL_WAIT] ticks=%d materialSlot=%u",
            responseWaitTicks,
            kAnvilMaterialInventorySlot
        );
        logInventorySnapshot("LOCAL_AFTER_-13_MATERIAL_PROBE");
        return;
    }

    if (stage == Stage::ReadAnvilMaterial) {
        if (responseWaitTicks > 0) {
            --responseWaitTicks;
        }

        if (anvilMaterialNativeTakeRequested &&
            anvilMaterialNativeTakePacketObserved &&
            anvilMaterialPreTakeSnapshotValid &&
            stagedAnvilOutputStack &&
            stagedAnvilOutputStack->isValid()) {
            const MaterialTakeDestinationDelta delta =
                findMaterialTakeDestinationDelta(
                    localPlayer->playerInventory,
                    stagedAnvilOutputStack.get(),
                    stagedAnvilOutputStack->mCount,
                    anvilMaterialPreTakeCounts,
                    anvilMaterialPreTakeExpectedMatches,
                    anvilMaterialPreTakeEmptySlots
                );
            if (delta.candidateCount == 1) {
                stagedAnvilDestinationSlot = delta.slot;
                rewrittenDestinationSlots = { delta.slot };
                expectedDestinationCounts.fill(0);
                expectedDestinationCounts[delta.slot] = delta.finalCount;
                anvilOutputStagePending = false;
                responseWaitTicks = kServerResponseCaptureTicks;
                stage = Stage::Completed;
                writeDiagnostic(
                    "[ANVIL_-13_NATIVE_TAKE_DEFERRED_DESTINATION] destinationSlot=%d finalCount=%u addedCount=%u packetId=0x%X source=snapshot-delta-tick",
                    delta.slot,
                    static_cast<unsigned int>(delta.finalCount),
                    static_cast<unsigned int>(
                        stagedAnvilOutputStack->mCount
                        ),
                    anvilMaterialNativeTakePacketId
                );
                return;
            }
        }

        if (settleTicks > 0) {
            --settleTicks;
            return;
        }

        if (responseWaitTicks > 0) {
            return;
        }

        if (!anvilMaterialReadbackGraceUsed &&
            anvilMaterialReadbackCheckedPacketCount <
            anvilMaterialInventoryPacketCount) {
            anvilMaterialReadbackGraceUsed = true;
            writeDiagnostic(
                "[ANVIL_-13_MATERIAL_READBACK_GRACE] pendingPackets=%d graceTicks=1",
                anvilMaterialInventoryPacketCount -
                anvilMaterialReadbackCheckedPacketCount
            );
            return;
        }

        writeDiagnostic(
            "[ANVIL_-13_NATIVE_TAKE_TIMEOUT] responsePackets=%d readback=%s exactResponse=%d takeRequested=%d takePacketObserved=%d takePacketId=0x%X; first stage or native extraction was not confirmed",
            anvilMaterialInventoryPacketCount,
            getAnvilMaterialReadbackName(
                anvilMaterialReadbackState
            ),
            anvilMaterialAuthoritativePacketReceived ? 1 : 0,
            anvilMaterialNativeTakeRequested ? 1 : 0,
            anvilMaterialNativeTakePacketObserved ? 1 : 0,
            anvilMaterialNativeTakePacketId
        );
        anvilOutputStagePending = false;
        anvilResyncProbeSent = true;
        stage = Stage::Completed;
        return;
    }

    if (stage == Stage::Completed) {
        if (responseWaitTicks > 0) {
            --responseWaitTicks;
            return;
        }

        if ((activeMode == 7 ||
            activeMode == 8 ||
            (activeMode == 5 &&
                (activeAnvilDupe2SingleItem ||
                    activeAnvilDupe2MaterialSlotMode))) &&
            !anvilResyncProbeSent) {
            anvilResyncProbeSent = true;
            anvilResyncInventoryPacketObserved = false;
            anvilResyncInventoryPacketCount = 0;
            anvilResyncReadbackState = kAnvilMaterialReadbackNone;
            anvilResyncAuthoritativePacketReceived = false;
            responseWaitTicks = kAnvilResyncProbeTicks;
            if (sendInventoryResyncProbe(localPlayer)) {
                writeDiagnostic(
                    activeMode == 7
                    ? "[ENCHANT_RESYNC_WAIT] ticks=%d destinations=%llu"
                    : "[ANVILDUPE2_RESYNC_WAIT] ticks=%d destinations=%llu",
                    kAnvilResyncProbeTicks,
                    static_cast<unsigned long long>(
                        rewrittenDestinationSlots.size()
                        )
                );
                return;
            }

            responseWaitTicks = 0;

            writeDiagnostic(
                "[ANVILDUPE2_RESYNC_PROBE] send failed; inspecting local slots without prediction"
            );
        }

        if ((activeMode == 0 &&
            !activeAnvilDirectDrop &&
            !activeAnvilDupe3Drop) ||
            (activeMode == 3 && activeDupe3Destination == 1) ||
            (activeMode == 6 && activeDupe4Destination == 1) ||
            activeMode == 1 ||
            activeMode == 2) {
            const int refreshedCount =
                refreshExpectedDestinations(localPlayer);
            if (refreshedCount > 0) {
                displayMessage(
                    "Dupe [%s]: refreshed %d hidden server-side inventory slot(s).",
                    getDupeModeName(activeMode),
                    refreshedCount
                );
            }
        }

        if (activeMode == 5 || activeMode == 8) {
            int matchingDestinationSlots = 0;
            int exactNbtDestinationSlots = 0;
            int finalMatchingInventoryCount = 0;
            PlayerInventory* inventory = localPlayer->playerInventory;
            if (inventory && inventory->container) {
                for (int slot = 0; slot < kInventorySlotCount; ++slot) {
                    ItemStack* item = inventory->container->getItem(slot);
                    if (item &&
                        item->isValid() &&
                        expectedResultItem &&
                        item->item.get() == expectedResultItem &&
                        item->mAuxValue == expectedResultAux) {
                        finalMatchingInventoryCount += item->mCount;
                    }
                }

                for (const int slot : rewrittenDestinationSlots) {
                    if (slot < 0 || slot >= kInventorySlotCount) {
                        continue;
                    }

                    const uint8_t expectedCount =
                        expectedDestinationCounts[slot];
                    ItemStack* item = inventory->container->getItem(slot);
                    if (expectedCount > 0 &&
                        matchesExpectedResult(item, expectedCount)) {
                        ++matchingDestinationSlots;
                    }
                    if (expectedCount > 0 &&
                        expectedResultStack &&
                        expectedResultStack->isValid() &&
                        matchesExpectedResult(item, expectedCount)) {
                        ComparableItemSummary expectedNbtWire{};
                        ComparableItemSummary actualNbtWire{};
                        const bool exactDescriptor =
                            comparableItemPayloadsEqual(
                                expectedResultStack.get(),
                                item,
                                expectedNbtWire,
                                actualNbtWire
                            );
                        if (exactDescriptor) {
                            ++exactNbtDestinationSlots;
                        }
                        writeDiagnostic(
                            activeAnvilDupe2MaterialSlotMode
                            ? "[ANVIL_-13_DESTINATION_NBT] slot=%d expected={rootTags=%zu nbtHash=%016llX block=%p} actual={rootTags=%zu nbtHash=%016llX block=%p} exact=%d"
                            : activeAnvilDupe2NbtWindowMode
                            ? "[ANVIL_NBT_WINDOW_DESTINATION_NBT] slot=%d expected={rootTags=%zu nbtHash=%016llX block=%p} actual={rootTags=%zu nbtHash=%016llX block=%p} exact=%d"
                            : "[ANVILDUPE2_SINGLE_DESTINATION_NBT] slot=%d expected={rootTags=%zu nbtHash=%016llX block=%p} actual={rootTags=%zu nbtHash=%016llX block=%p} exact=%d",
                            slot,
                            expectedNbtWire.rootTagCount,
                            static_cast<unsigned long long>(
                                expectedNbtWire.nbtHash
                                ),
                            reinterpret_cast<void*>(
                                expectedNbtWire.blockIdentity
                                ),
                            actualNbtWire.rootTagCount,
                            static_cast<unsigned long long>(
                                actualNbtWire.nbtHash
                                ),
                            reinterpret_cast<void*>(
                                actualNbtWire.blockIdentity
                                ),
                            exactDescriptor ? 1 : 0
                        );
                    }
                }
            }

            const int netGain =
                finalMatchingInventoryCount -
                anvilPreTransactionOwnedCount;

            writeDiagnostic(
                activeMode == 8
                ? "[ANVIL_SWAP_NET_RESULT] baselineOwned=%d finalInventory=%d netGain=%d destinationMatches=%d requested=%d authoritativePacket=%d localGhostRefresh=0"
                : "[ANVILDUPE2_NET_RESULT] baselineOwned=%d finalInventory=%d netGain=%d destinationMatches=%d requested=%d authoritativePacket=%d localGhostRefresh=0",
                anvilPreTransactionOwnedCount,
                finalMatchingInventoryCount,
                netGain,
                matchingDestinationSlots,
                rewrittenDestinationCount,
                anvilResyncAuthoritativePacketReceived ? 1 : 0
            );
            if (activeMode == 5 &&
                activeAnvilDupe2NbtWindowMode) {
                const char* correctionResult =
                    !anvilResyncAuthoritativePacketReceived
                    ? "not-observed"
                    : exactNbtDestinationSlots > 0
                    ? "exact"
                    : matchingDestinationSlots > 0
                    ? "normalized"
                    : "empty-or-rejected";
                writeDiagnostic(
                    "[ANVIL_NBT_WINDOW_AUTHORITATIVE_RESULT] firstStageAccepted=%d secondStageSent=%d correction=%s packetSeen=%d exactNbtDestinations=%d requested=%d destinationReadback=%s localSlotsClearedBeforeProbe=%d",
                    nbtWindowFirstStageAccepted ? 1 : 0,
                    nbtWindowSecondStageSent ? 1 : 0,
                    correctionResult,
                    anvilResyncAuthoritativePacketReceived ? 1 : 0,
                    exactNbtDestinationSlots,
                    rewrittenDestinationCount,
                    getAnvilMaterialReadbackName(
                        anvilResyncReadbackState
                    ),
                    anvilResyncClearedLocalSlots
                );
                if (anvilResyncAuthoritativePacketReceived &&
                    exactNbtDestinationSlots > 0) {
                    displayMessage(
                        "Dupe [anvildupe2 NBT Window -13]: the server-confirmed destination contains the exact staged ItemStack."
                    );
                }
                else {
                    displayMessage(
                        "Dupe [anvildupe2 NBT Window -13]: no exact server-confirmed destination was observed."
                    );
                }
            }
            else if (activeMode == 5 &&
                activeAnvilDupe2MaterialSlotMode) {
                writeDiagnostic(
                    "[ANVIL_-13_AUTHORITATIVE_RESULT] exactNbtDestinations=%d requested=%d authoritativePacket=%d responsePackets=%d destinationReadback=%s localSlotsClearedBeforeProbe=%d",
                    exactNbtDestinationSlots,
                    rewrittenDestinationCount,
                    anvilResyncAuthoritativePacketReceived ? 1 : 0,
                    anvilResyncInventoryPacketCount,
                    getAnvilMaterialReadbackName(
                        anvilResyncReadbackState
                    ),
                    anvilResyncClearedLocalSlots
                );
                if (anvilResyncAuthoritativePacketReceived &&
                    exactNbtDestinationSlots > 0) {
                    displayMessage(
                        "Dupe [anvildupe2 Material Slot]: the server-confirmed destination contains the exact staged item data."
                    );
                }
                else {
                    displayMessage(
                        "Dupe [anvildupe2 Material Slot]: no exact server-confirmed destination was observed."
                    );
                }
            }
            else if (activeMode == 5 &&
                activeAnvilDupe2SingleItem) {
                const char* correctionResult =
                    !anvilResyncAuthoritativePacketReceived
                    ? "not-observed"
                    : exactNbtDestinationSlots > 0
                    ? "exact"
                    : matchingDestinationSlots > 0
                    ? "normalized"
                    : "empty-or-rejected";
                writeDiagnostic(
                    "[ANVILDUPE2_SINGLE_AUTHORITATIVE_RESULT] test=%s itemLock=%s correction=%s packetSeen=%d localExactNbtDestinations=%d requested=%d localSlotsClearedBeforeProbe=%d",
                    getSingleNbtExperimentName(
                        activeAnvilDupe2SingleNbtExperiment
                    ),
                    getInnerItemLockName(
                        activeAnvilDupe2InnerItemLock
                    ),
                    correctionResult,
                    anvilResyncAuthoritativePacketReceived ? 1 : 0,
                    exactNbtDestinationSlots,
                    rewrittenDestinationCount,
                    anvilResyncClearedLocalSlots
                );
                if (anvilResyncAuthoritativePacketReceived &&
                    exactNbtDestinationSlots > 0) {
                    displayMessage(
                        "Dupe [anvildupe2 Single]: the server-confirmed destination contains the exact requested box data."
                    );
                }
                else {
                    displayMessage(
                        "Dupe [anvildupe2 Single]: no exact server-confirmed destination was observed."
                    );
                }
            }
            else if (netGain > 0) {
                displayMessage(
                    "Dupe [anvildupe2]: real net gain %d item(s), inventory %d -> %d.",
                    netGain,
                    anvilPreTransactionOwnedCount,
                    finalMatchingInventoryCount
                );
            }
            else if (netGain == 0) {
                displayMessage(
                    "Dupe [anvildupe2]: no net gain (%d -> %d); any destination match was only the original item moving slots.",
                    anvilPreTransactionOwnedCount,
                    finalMatchingInventoryCount
                );
            }
            else {
                displayMessage(
                    "Dupe [anvildupe2]: net loss/hidden state %d (%d -> %d); no duplication was confirmed.",
                    netGain,
                    anvilPreTransactionOwnedCount,
                    finalMatchingInventoryCount
                );
            }
        }

        if (activeMode == 7) {
            int matchingDestinationSlots = 0;
            PlayerInventory* inventory = localPlayer->playerInventory;
            if (inventory && inventory->container) {
                for (const int slot : rewrittenDestinationSlots) {
                    if (slot < 0 || slot >= kInventorySlotCount) {
                        continue;
                    }
                    const uint8_t expectedCount =
                        expectedDestinationCounts[slot];
                    ItemStack* item = inventory->container->getItem(slot);
                    if (expectedCount > 0 &&
                        matchesExpectedResult(item, expectedCount)) {
                        ++matchingDestinationSlots;
                    }
                }
            }

            writeDiagnostic(
                "[ENCHANT_NET_RESULT] destinationMatches=%d requested=%d authoritativePacket=%d localPrediction=0",
                matchingDestinationSlots,
                rewrittenDestinationCount,
                anvilResyncAuthoritativePacketReceived ? 1 : 0
            );
            if (matchingDestinationSlots > 0) {
                displayMessage(
                    "Dupe [enchantdupe]: server-authoritative success, %d/%d destination slot(s) contain the copied box.",
                    matchingDestinationSlots,
                    rewrittenDestinationCount
                );
            }
            else {
                displayMessage(
                    "Dupe [enchantdupe]: the server did not confirm a copied box; no local ghost was retained."
                );
            }
        }

        logInventorySnapshot("LOCAL_AFTER_RESPONSE_WINDOW");

        if (activeMode == 5 &&
            !activeAnvilDupe2SingleItem &&
            !activeAnvilDupe2MaterialSlotMode &&
            activeAnvilDupe2Destination == 0 &&
            rewrittenDestinationCount > 0) {
            nextDropSlot = kMainInventoryFirstSlot;
            dropTransactionCount = 0;
            dropFailureCount = 0;
            stage = Stage::DropInventory;
            writeDiagnostic(
                "[ANVILDUPE2_WORLD_DROP_BEGIN] destinations=%d scanSlots=%d-%d",
                rewrittenDestinationCount,
                kMainInventoryFirstSlot,
                kInventorySlotCount - 1
            );
            displayMessage(
                "Dupe [anvildupe2 World]: copies were materialised server-side; now dropping only the duplicated main-inventory slots without closing the anvil."
            );
            return;
        }

        if (dropInventory &&
            activeMode != 4 &&
            activeMode != 5 &&
            !activeAnvilDirectDrop &&
            !activeAnvilDupe3Drop &&
            rewrittenDestinationCount > 0) {
            int confirmedDestinationCount = 0;
            PlayerInventory* inventory = localPlayer->playerInventory;
            if (inventory && inventory->container) {
                for (const int slot : rewrittenDestinationSlots) {
                    ItemStack* item =
                        slot >= 0 && slot < kInventorySlotCount
                        ? inventory->container->getItem(slot)
                        : nullptr;
                    const uint8_t expectedCount =
                        slot >= 0 && slot < kInventorySlotCount
                        ? expectedDestinationCounts[slot]
                        : 0;
                    if (expectedCount > 0 &&
                        matchesExpectedResult(item, expectedCount)) {
                        ++confirmedDestinationCount;
                    }
                }
            }

            if (activeMode == 7 && confirmedDestinationCount == 0) {
                writeDiagnostic(
                    "[DROP_SKIP] enchantdupe has no server-confirmed destination"
                );
                displayMessage(
                    "Dupe [enchantdupe]: Drop Inventory was skipped because no copied slot was confirmed by the server."
                );
            }
            else if (expectedResultStack &&
                expectedResultStack->isValid() &&
                expectedResultStack->mCount > 0) {
                const bool activeDyeDirectWorld =
                    (activeMode == 3 &&
                        activeDupe3Destination == 0) ||
                    (activeMode == 6 &&
                        activeDupe4Destination == 0);
                nextDropSlot = kMainInventoryFirstSlot;
                dropTransactionCount = 0;
                dropFailureCount = 0;
                stage = Stage::DropInventory;
                if (activeDyeDirectWorld) {
                    writeDiagnostic(
                        "[DROP_BEGIN] directWorldCopies=%d scanSlots=%d-%d",
                        rewrittenDestinationCount,
                        kMainInventoryFirstSlot,
                        kInventorySlotCount - 1
                    );
                    displayMessage(
                        "Dupe [%s]: the direct-drop transaction was sent; now dropping the 27 main-inventory slots (hotbar excluded).",
                        getDupeModeName(activeMode)
                    );
                }
                else {
                    writeDiagnostic(
                        "[DROP_BEGIN] locallyVisibleDestinations=%d savedResultFallback=%d scanSlots=%d-%d",
                        confirmedDestinationCount,
                        confirmedDestinationCount == 0 ? 1 : 0,
                        kMainInventoryFirstSlot,
                        kInventorySlotCount - 1
                    );
                }
                if (!activeDyeDirectWorld &&
                    confirmedDestinationCount > 0) {
                    displayMessage(
                        "Dupe: duplication visible; dropping the 27 main-inventory slots (hotbar excluded)."
                    );
                }
                else if (!activeDyeDirectWorld) {
                    displayMessage(
                        "Dupe: copied slots are still hidden by the open GUI; dropping them from the saved server transaction without closing it."
                    );
                }
                return;
            }

            if (!(activeMode == 7 && confirmedDestinationCount == 0)) {
                writeDiagnostic(
                    "[DROP_SKIP] full expected result stack was unavailable"
                );
                displayMessage(
                    "Dupe: the full result stack was unavailable, so Drop Inventory was skipped."
                );
            }
        }
        else if (dropInventory &&
            activeMode != 4 &&
            activeMode != 5 &&
            !activeAnvilDirectDrop &&
            !activeAnvilDupe3Drop) {
            writeDiagnostic(
                "[DROP_SKIP] the result transaction added no duplicate destinations"
            );
            displayMessage(
                "Dupe: no duplicate destination was added, so Drop Inventory was skipped."
            );
        }

        writeDiagnostic("[DONE] response capture window complete");
        if (autoRepeat) {
            writeDiagnostic(
                "[AUTO_REPEAT_REARM] mode=%s previousDestinations=%d",
                getDupeModeName(activeMode),
                rewrittenDestinationCount
            );
            resetAttemptState();
            displayMessage(
                "Dupe [%s]: still enabled; ready for the next result.",
                getDupeModeName(activeMode)
            );
            return;
        }
        setEnabled(false);
        return;
    }

    if (stage == Stage::DropInventory) {
        PlayerInventory* inventory = localPlayer->playerInventory;
        if (!inventory || !inventory->container || !localPlayer->level) {
            writeDiagnostic("[DROP_ABORT] player inventory or level unavailable");
            setEnabled(false);
            return;
        }

        const auto isRewrittenDestination = [this](int inventorySlot) {
            return expectedResultStack &&
                expectedResultStack->isValid() &&
                expectedResultStack->mCount > 0 &&
                inventorySlot >= 0 &&
                inventorySlot < kInventorySlotCount &&
                expectedDestinationCounts[inventorySlot] > 0;
            };
        const bool rewrittenDestinationsOnly =
            activeMode == 5 &&
            activeAnvilDupe2Destination == 0;

        while (nextDropSlot < kInventorySlotCount) {
            ItemStack* item = inventory->container->getItem(nextDropSlot);
            const bool hasLocalItem =
                item && item->isValid() && item->mCount > 0;
            const bool rewrittenDestination =
                isRewrittenDestination(nextDropSlot);
            if ((rewrittenDestinationsOnly && rewrittenDestination) ||
                (!rewrittenDestinationsOnly &&
                    (hasLocalItem || rewrittenDestination))) {
                break;
            }
            ++nextDropSlot;
        }

        if (nextDropSlot >= kInventorySlotCount) {
            logInventorySnapshot("LOCAL_AFTER_DROP_REQUESTS");
            writeDiagnostic(
                "[DROP_REQUESTS_DONE] sent=%d failed=%d waitTicks=%d",
                dropTransactionCount,
                dropFailureCount,
                kDropResponseCaptureTicks
            );
            if (rewrittenDestinationsOnly) {
                displayMessage(
                    "Dupe [anvildupe2 World]: sent %d duplicated-slot drop transaction(s); %d slot(s) failed locally.",
                    dropTransactionCount,
                    dropFailureCount
                );
            }
            else {
                displayMessage(
                    "Dupe: sent %d main-inventory drop transaction(s); %d slot(s) failed locally.",
                    dropTransactionCount,
                    dropFailureCount
                );
            }
            responseWaitTicks = kDropResponseCaptureTicks;
            stage = Stage::DropResponse;
            return;
        }

        const int inventorySlot = nextDropSlot++;
        ItemStack* localItem = inventory->container->getItem(inventorySlot);
        const bool hasLocalItem =
            localItem && localItem->isValid() && localItem->mCount > 0;
        const uint8_t authoritativeCount =
            isRewrittenDestination(inventorySlot)
            ? expectedDestinationCounts[inventorySlot]
            : 0;
        const bool localIsExpectedKind =
            hasLocalItem &&
            expectedResultItem &&
            localItem->item.get() == expectedResultItem &&
            localItem->mAuxValue == expectedResultAux;

        std::unique_ptr<ItemStack> authoritativeItem;
        NativeItemStackOwnerResetGuard authoritativeItemGuard(
            authoritativeItem
        );
        ItemStack* dropItem = hasLocalItem ? localItem : nullptr;
        if (authoritativeCount > 0 &&
            (!hasLocalItem ||
                (localIsExpectedKind &&
                    localItem->mCount != authoritativeCount))) {
            authoritativeItem =
                std::make_unique<ItemStack>(*expectedResultStack);
            authoritativeItem->mCount = authoritativeCount;
            dropItem = authoritativeItem.get();
        }

        const bool splitInventorySource =
            authoritativeCount > 1 &&
            (!hasLocalItem || localIsExpectedKind);

        const bool dropSent = sendDropTransaction(
            localPlayer,
            dropItem,
            inventorySlot,
            splitInventorySource
        );
        if (!dropSent) {
            ++dropFailureCount;
            writeDiagnostic("[DROP_FAIL] slot=%d", inventorySlot);
            return;
        }

        ++dropTransactionCount;
        return;
    }

    if (stage == Stage::DropResponse) {
        if (responseWaitTicks > 0) {
            --responseWaitTicks;
            return;
        }

        logInventorySnapshot("LOCAL_AFTER_DROP_RESPONSE_WINDOW");
        writeDiagnostic(
            "[DROP_DONE] sent=%d failed=%d",
            dropTransactionCount,
            dropFailureCount
        );
        if (activeMode == 5 &&
            activeAnvilDupe2Destination == 0) {
            displayMessage(
                "Dupe [anvildupe2 World]: duplicated-slot drops finished; hotbar slots 0-8 were not touched."
            );
        }
        else {
            displayMessage(
                "Dupe: Drop Inventory finished; hotbar slots 0-8 were not touched."
            );
        }
        if (autoRepeat) {
            writeDiagnostic(
                "[AUTO_REPEAT_REARM] mode=%s afterDrop=1",
                getDupeModeName(activeMode)
            );
            resetAttemptState();
            displayMessage(
                "Dupe [%s]: still enabled; ready for the next result.",
                getDupeModeName(activeMode)
            );
            return;
        }
        setEnabled(false);
        return;
    }

    if (stage == Stage::WaitForRenameTake) {
        if (++captureWaitTicks > kLegacyPacketTimeoutTicks) {
            if (autoRepeat) {
                writeDiagnostic(
                    "[AUTO_REPEAT_IDLE] mode=%s packets=%d; continuing to wait",
                    getDupeModeName(activeMode),
                    legacyPacketCount
                );
                captureWaitTicks = 0;
                legacyPacketCount = 0;
                return;
            }
            if (legacyPacketCount == 0) {
                displayMessage(
                    "Dupe: no InventoryTransactionPacket was sent for the selected mode."
                );
            }
            else if (activeMode == 0 || activeMode == 5) {
                displayMessage(
                    "Dupe: saw %d InventoryTransactionPacket(s), but no genuine anvil -12/-13 result transaction matched.",
                    legacyPacketCount
                );
            }
            else if (activeMode == 4) {
                if (useNestedDyeDropCarrier()) {
                    displayMessage(
                        "Nested [Dye Drop]: saw %d InventoryTransactionPacket(s), but no genuine shulker-dye -4 result matched (ingredients: shulker=%d, other=%d, named-dye=%d).",
                        legacyPacketCount,
                        sawDyeShulkerIngredient ? 1 : 0,
                        sawDyeOtherIngredient ? 1 : 0,
                        sawNamedDyeIngredient ? 1 : 0
                    );
                }
                else {
                    displayMessage(
                        "Nested [Anvil -10 Swap]: saw %d InventoryTransactionPacket(s), but no singular genuine shulker -12 result, inventory destination, and usable -10.old carrier matched.",
                        legacyPacketCount
                    );
                }
            }
            else {
                displayMessage(
                    "Dupe [%s]: saw %d InventoryTransactionPacket(s), but no genuine shulker-dye -4 crafting result matched (ingredients: shulker=%d, other=%d, named-dye=%d).",
                    getDupeModeName(activeMode),
                    legacyPacketCount,
                    sawDyeShulkerIngredient ? 1 : 0,
                    sawDyeOtherIngredient ? 1 : 0,
                    sawNamedDyeIngredient ? 1 : 0
                );
            }
            setEnabled(false);
        }
        return;
    }

    if (stage == Stage::WaitForRenameTake &&
        waitingForCursorPlacement) {
        if (tryCaptureCursorPlacementFallback(localPlayer)) {
        }
        else if (++captureWaitTicks > kCaptureTimeoutTicks) {
            displayMessage(
                "Dupe: the renamed result was not placed in the inventory; disabling."
            );
            setEnabled(false);
            return;
        }
    }

    if (renameNoticePending) {
        renameNoticePending = false;
        displayMessage(
            "Dupe: renamed anvil result detected in inventory slot %d.",
            renamedInventorySlot
        );
    }

    if (stage != Stage::WaitForRenamedItem &&
        stage != Stage::FillInventory) {
        return;
    }

    if (stage == Stage::WaitForRenamedItem && settleTicks > 0) {
        --settleTicks;
        return;
    }

    PlayerInventory* inventory = localPlayer->playerInventory;
    if (!inventory || !inventory->container) {
        setEnabled(false);
        return;
    }

    ItemStack* renamedItem =
        renamedInventorySlot >= 0 &&
        renamedInventorySlot < kInventorySlotCount
        ? inventory->container->getItem(renamedInventorySlot)
        : nullptr;

    if (!renamedItem || !renamedItem->isValid()) {
        if (stage == Stage::WaitForRenamedItem &&
            ++captureWaitTicks <= kCaptureTimeoutTicks) {
            return;
        }

        displayMessage(
            "Dupe: the tracked renamed item is no longer in its inventory slot."
        );
        setEnabled(false);
        return;
    }

    if (stage == Stage::WaitForRenamedItem) {
        pendingDestinationSlots = findEmptyInventorySlots(localPlayer);
        const std::size_t requestedCopies =
            static_cast<std::size_t>(activeCopyCount);
        if (pendingDestinationSlots.size() > requestedCopies) {
            pendingDestinationSlots.resize(requestedCopies);
        }
        nextDestinationIndex = 0;
        sentTransactionCount = 0;

        if (pendingDestinationSlots.empty()) {
            displayMessage(
                "Dupe: no empty inventory slots; disabling."
            );
            setEnabled(false);
            return;
        }

        stage = Stage::FillInventory;
        displayMessage(
            "Dupe: refreshing %d empty inventory slot(s).",
            static_cast<int>(pendingDestinationSlots.size())
        );
    }

    if (packetCooldownTicks > 0) {
        --packetCooldownTicks;
        return;
    }

    while (nextDestinationIndex < pendingDestinationSlots.size()) {
        const int slot = pendingDestinationSlots[nextDestinationIndex];
        ItemStack* current = inventory->container->getItem(slot);
        if (!current || !current->isValid()) {
            break;
        }
        ++nextDestinationIndex;
    }

    if (nextDestinationIndex >= pendingDestinationSlots.size()) {
        stage = Stage::Completed;
        displayMessage(
            "Dupe: sent %d transaction(s); wait for server inventory updates.",
            sentTransactionCount
        );
        setEnabled(false);
        return;
    }

    const int destinationSlot =
        pendingDestinationSlots[nextDestinationIndex++];

    if (!sendDuplicateTransaction(
        localPlayer,
        renamedItem,
        destinationSlot
    )) {
        displayMessage(
            "Dupe: failed to build the inventory transaction."
        );
        setEnabled(false);
        return;
    }

    ++sentTransactionCount;
    packetCooldownTicks = kTicksBetweenTransactions;
}

void Dupe::onSendPacket(Packet* packet, bool& cancel) {
    if (!packet) {
        return;
    }

    if (activeMode == 9 &&
        stage == Stage::Completed &&
        packet->getId() == PacketID::InventoryTransaction) {
        writeDiagnostic(
            "[AETHER_AUTO_REPEAT_REARM] trigger=next-inventory-transaction loaded=%d",
            NbtCommand::hasLoadedTag() ? 1 : 0
        );
        resetAttemptState();
    }

    if (activeAnvilDupe2SourceBoxMode &&
        stage == Stage::WaitForRenameTake &&
        (packet->getId() == PacketID::ItemStackRequest ||
            packet->getId() == PacketID::PlayerAuthInput)) {
        traceSourceBoxStackRequests(packet);
    }

    if (activeItemStackRequestStacking &&
        stage == Stage::WaitForRenameTake &&
        (packet->getId() == PacketID::ItemStackRequest ||
            packet->getId() == PacketID::PlayerAuthInput)) {
        if (rewriteItemStackRequest(packet)) {
            responseWaitTicks = kServerResponseCaptureTicks;
            stage = Stage::Completed;
            rewriteNoticePending = true;
            renameNoticePending = false;
            writeDiagnostic(
                "[STACK_REQUEST_SEND] packetId=0x%X requestedCount=%d responseTicks=%d",
                static_cast<unsigned int>(packet->getId()),
                activeStackCount,
                responseWaitTicks
            );
            return;
        }
    }

    if (activeMode == 7 &&
        enchantOutgoingDiagnosticCount < 128) {
        const PacketID diagnosticId = packet->getId();
        if (diagnosticId == PacketID::InventoryTransaction ||
            diagnosticId == PacketID::ItemStackRequest ||
            diagnosticId == PacketID::PlayerAuthInput ||
            diagnosticId == PacketID::ContainerClose) {
            writeDiagnostic(
                "[ENCHANT_OUT_PACKET] id=0x%X stage=%d inFlight=%d",
                static_cast<unsigned int>(diagnosticId),
                static_cast<int>(stage),
                enchantPacketInFlight ? 1 : 0
            );
            ++enchantOutgoingDiagnosticCount;
        }
    }

    if (activeMode == 7 &&
        stage == Stage::WaitForRenameTake &&
        packet->getId() == PacketID::ContainerClose) {
        resetNativeItemStack(cachedEnchantInput);
        resetNativeItemStack(cachedEnchantMaterial);
        resetNativeItemStack(cachedEnchantDonor);
        enchantTransactionPending = false;
        enchantSettleTicks = 0;
        enchantSequencePhase = EnchantSequencePhase::Idle;
        enchantSequenceDestinationSlots.clear();
        enchantSequenceCompletedSlots.clear();
        enchantSequenceIndex = 0;
        enchantSequenceInitialMaterialCount = 0;
        return;
    }

    if (activeMode == 7 &&
        (enchantSequencePhase != EnchantSequencePhase::Idle ||
            stage == Stage::Completed) &&
        !enchantPacketInFlight &&
        (packet->getId() == PacketID::InventoryTransaction ||
            packet->getId() == PacketID::ItemStackRequest)) {
        writeDiagnostic(
            "[ENCHANT_SEQUENCE_BLOCK_OUT] packetId=0x%X phase=%d unit=%llu",
            static_cast<unsigned int>(packet->getId()),
            static_cast<int>(enchantSequencePhase),
            static_cast<unsigned long long>(enchantSequenceIndex + 1)
        );
        cancel = true;
        return;
    }

    if (activeMode == 7 &&
        packet->getId() == PacketID::InventoryTransaction) {
        if (!enchantPacketInFlight) {
            ++legacyPacketCount;
            ComplexInventoryTransaction* observedTransaction =
                getValidatedTransaction(
                    static_cast<InventoryTransactionPacket*>(packet)
                );
            if (observedTransaction) {
                if (stage == Stage::WaitForRenameTake) {
                    logLegacyTransaction(
                        "ENCHANT_PLACEMENT",
                        observedTransaction
                    );
                    observeEnchantSlots(observedTransaction);
                }
                else {
                    logLegacyTransaction(
                        "ENCHANT_NON_SYNTHETIC_OUTGOING",
                        observedTransaction
                    );
                }
            }
        }
        return;
    }

    if (activeMode == 5 &&
        activeAnvilDupe2MaterialSlotMode &&
        stage == Stage::ReadAnvilMaterial &&
        anvilMaterialNativeTakeRequested &&
        (packet->getId() == PacketID::InventoryTransaction ||
            packet->getId() == PacketID::ItemStackRequest)) {
        int nativeDestinationSlot = -1;
        uint8_t nativeDestinationCount = 0;
        const uint8_t stagedTransferCount =
            stagedAnvilOutputStack && stagedAnvilOutputStack->isValid()
            ? stagedAnvilOutputStack->mCount
            : 0;
        anvilMaterialNativeTakePacketObserved = true;
        anvilMaterialNativeTakePacketId =
            static_cast<unsigned int>(packet->getId());

        auto inferSnapshotDestination = [&]() {
            LocalPlayer* localPlayer = Game::getLocalPlayer();
            PlayerInventory* inventory = localPlayer
                ? localPlayer->playerInventory
                : nullptr;
            MaterialTakeDestinationDelta delta{};
            if (anvilMaterialPreTakeSnapshotValid) {
                delta = findMaterialTakeDestinationDelta(
                    inventory,
                    stagedAnvilOutputStack.get(),
                    stagedTransferCount,
                    anvilMaterialPreTakeCounts,
                    anvilMaterialPreTakeExpectedMatches,
                    anvilMaterialPreTakeEmptySlots
                );
            }
            if (delta.candidateCount == 1) {
                nativeDestinationSlot = delta.slot;
                nativeDestinationCount = delta.finalCount;
            }
            writeDiagnostic(
                "[ANVIL_-13_NATIVE_TAKE_SNAPSHOT_DELTA] candidates=%d destinationSlot=%d finalCount=%u transferCount=%u",
                delta.candidateCount,
                delta.slot,
                delta.candidateCount == 1
                ? static_cast<unsigned int>(delta.finalCount)
                : 0U,
                static_cast<unsigned int>(stagedTransferCount)
            );
            };

        if (packet->getId() == PacketID::InventoryTransaction) {
            ComplexInventoryTransaction* nativeTake =
                getValidatedTransaction(
                    static_cast<InventoryTransactionPacket*>(packet)
                );
            logLegacyTransaction("ANVIL_-13_NATIVE_TAKE_OUT", nativeTake);
            if (nativeTake &&
                nativeTake->type ==
                ComplexInventoryTransaction::Type::NormalTransaction) {
                int actionCandidateCount = 0;
                int actionCandidateSlot = -1;
                uint8_t actionCandidateFinalCount = 0;
                for (const auto& [source, actions] :
                    nativeTake->data.actions) {
                    if (source.type !=
                        InventorySourceType::ContainerInventory ||
                        source.containerId != ContainerID::Inventory) {
                        continue;
                    }
                    for (const InventoryAction& action : actions) {
                        if (action.slot >= kInventorySlotCount ||
                            !action.toItem.isValid() ||
                            action.toItem.mCount == 0 ||
                            stagedTransferCount == 0) {
                            continue;
                        }
                        const uint8_t fromCount =
                            action.fromItem.isValid()
                            ? action.fromItem.mCount
                            : 0;
                        if (action.toItem.mCount <= fromCount ||
                            static_cast<unsigned int>(
                                action.toItem.mCount - fromCount
                                ) != static_cast<unsigned int>(
                                    stagedTransferCount
                                    ) ||
                            !stagedAnvilOutputStack ||
                            !stagedAnvilOutputStack->isValid() ||
                            action.toItem.item.get() !=
                            stagedAnvilOutputStack->item.get() ||
                            action.toItem.mAuxValue !=
                            stagedAnvilOutputStack->mAuxValue) {
                            continue;
                        }

                        ComparableItemSummary expectedToWire{};
                        ComparableItemSummary actualToWire{};
                        if (!comparableItemPayloadsEqual(
                            stagedAnvilOutputStack.get(),
                            &action.toItem,
                            expectedToWire,
                            actualToWire
                        )) {
                            continue;
                        }
                        if (action.fromItem.isValid()) {
                            ComparableItemSummary expectedFromWire{};
                            ComparableItemSummary actualFromWire{};
                            if (action.fromItem.item.get() !=
                                stagedAnvilOutputStack->item.get() ||
                                action.fromItem.mAuxValue !=
                                stagedAnvilOutputStack->mAuxValue ||
                                !comparableItemPayloadsEqual(
                                    stagedAnvilOutputStack.get(),
                                    &action.fromItem,
                                    expectedFromWire,
                                    actualFromWire
                                )) {
                                continue;
                            }
                        }

                        ++actionCandidateCount;
                        actionCandidateSlot =
                            static_cast<int>(action.slot);
                        actionCandidateFinalCount =
                            action.toItem.mCount;
                    }
                }
                if (actionCandidateCount == 1) {
                    nativeDestinationSlot = actionCandidateSlot;
                    nativeDestinationCount = actionCandidateFinalCount;
                }
                writeDiagnostic(
                    "[ANVIL_-13_NATIVE_TAKE_ACTION_DELTA] candidates=%d destinationSlot=%d finalCount=%u transferCount=%u",
                    actionCandidateCount,
                    actionCandidateCount == 1
                    ? actionCandidateSlot
                    : -1,
                    actionCandidateCount == 1
                    ? static_cast<unsigned int>(
                        actionCandidateFinalCount
                        )
                    : 0U,
                    static_cast<unsigned int>(stagedTransferCount)
                );
            }
            if (nativeDestinationSlot < 0) {
                inferSnapshotDestination();
            }
        }
        else {
            inferSnapshotDestination();
            writeDiagnostic(
                "[ANVIL_-13_NATIVE_TAKE_STACK_REQUEST] packetId=0x%X inferredDestination=%d finalCount=%u",
                static_cast<unsigned int>(packet->getId()),
                nativeDestinationSlot,
                static_cast<unsigned int>(nativeDestinationCount)
            );
        }

        if (nativeDestinationSlot >= 0 &&
            nativeDestinationSlot < kInventorySlotCount &&
            nativeDestinationCount > 0) {
            stagedAnvilDestinationSlot = nativeDestinationSlot;
            rewrittenDestinationSlots = { nativeDestinationSlot };
            expectedDestinationCounts.fill(0);
            expectedDestinationCounts[nativeDestinationSlot] =
                nativeDestinationCount;
            anvilOutputStagePending = false;
            responseWaitTicks = kServerResponseCaptureTicks;
            stage = Stage::Completed;
            writeDiagnostic(
                "[ANVIL_-13_NATIVE_TAKE_SEND] destinationSlot=%d finalCount=%u addedCount=%u packetId=0x%X source=server-material-slot localPrediction=vanilla",
                nativeDestinationSlot,
                static_cast<unsigned int>(nativeDestinationCount),
                static_cast<unsigned int>(stagedTransferCount),
                static_cast<unsigned int>(packet->getId())
            );
        }
        else {
            writeDiagnostic(
                "[ANVIL_-13_NATIVE_TAKE_SHAPE_WAIT] outgoing packet had no unique exact player-inventory increase"
            );
        }
        return;
    }

    if (activeMode == 5 &&
        autoRepeat &&
        stage == Stage::Completed &&
        packet->getId() == PacketID::InventoryTransaction) {
        ComplexInventoryTransaction* repeatTransaction =
            getValidatedTransaction(
                static_cast<InventoryTransactionPacket*>(packet)
            );
        if (repeatTransaction &&
            repeatTransaction->type ==
            ComplexInventoryTransaction::Type::InventoryMismatch) {
            writeDiagnostic(
                activeAnvilDupe2MaterialSlotMode
                ? "[ANVIL_-13_CLIENT_MISMATCH] retained current attempt for authoritative probe"
                : activeAnvilDupe2SingleItem
                ? "[ANVILDUPE2_SINGLE_CLIENT_MISMATCH] retained current attempt for authoritative probe"
                : "[ANVILDUPE2_CLIENT_MISMATCH] retained current attempt for response window"
            );
        }
        else if (!anvilResyncProbeSent &&
            !activeAnvilDupe2SingleItem &&
            !activeAnvilDupe2MaterialSlotMode) {
            writeDiagnostic(
                "[ANVILDUPE2_AUTO_REPEAT_REARM] trigger=next-inventory-transaction"
            );
            resetAttemptState();
        }
    }

    if (packet->getId() == PacketID::InventoryTransaction &&
        stage == Stage::WaitForRenameTake) {
        if (activeItemStackRequestStacking) {
            writeDiagnostic(
                "[STACK_REQUEST_LEGACY_PASS] genuine legacy transaction left unchanged while waiting for an ItemStackRequest"
            );
            return;
        }
        ++legacyPacketCount;
        ComplexInventoryTransaction* observedTransaction =
            getValidatedTransaction(
                static_cast<InventoryTransactionPacket*>(packet)
            );
        if (observedTransaction) {
            logLegacyTransaction("OUT_BEFORE", observedTransaction);
            if ((activeMode >= 1 && activeMode <= 3) ||
                activeMode == 6 || activeMode == 9) {
                observeDyeCraftIngredients(observedTransaction);
            }
        }
        else {
            writeDiagnostic(
                "[OUT_BEFORE] packet=%p transaction pointer/layout validation failed",
                packet
            );
        }
        int addedDestinationCount = 0;
        bool rewritten = false;
        if (activeMode == 0 || activeMode == 5 || activeMode == 8) {
            rewritten = rewriteAnvilTransaction(
                static_cast<InventoryTransactionPacket*>(packet),
                addedDestinationCount
            );
        }
        else if (activeMode == 1) {
            rewritten = rewriteNewDyeCraftTransaction(
                static_cast<InventoryTransactionPacket*>(packet),
                addedDestinationCount
            );
        }
        else if (activeMode == 2) {
            rewritten = rewriteOldDyeCraftTransaction(
                static_cast<InventoryTransactionPacket*>(packet),
                addedDestinationCount
            );
        }
        else if (activeMode == 3) {
            rewritten = rewriteDyeDupe3Transaction(
                static_cast<InventoryTransactionPacket*>(packet),
                addedDestinationCount
            );
        }
        else if (activeMode == 6) {
            rewritten = rewriteDyeDupe4Transaction(
                static_cast<InventoryTransactionPacket*>(packet),
                addedDestinationCount
            );
        }
        else if (activeMode == 9) {
            if (std::clamp(aetherCarrierMode, 0, 2) != 1 &&
                nestedEarlyResultInjected) {
                rewritten = finalizeEarlyNestedDescriptors(
                    static_cast<InventoryTransactionPacket*>(packet)
                );
                addedDestinationCount = 0;
                nestedEarlyResultInjected = false;
                if (rewritten && cachedEarlyNestedResult)
                    rememberExpectedResult(*cachedEarlyNestedResult);
                resetNativeItemStack(cachedEarlyNestedResult);
            }
            else {
                if (std::clamp(aetherCarrierMode, 0, 2) != 1) {
                    rewritten = rewriteAetherCraftTransaction(
                        static_cast<InventoryTransactionPacket*>(packet),
                        addedDestinationCount
                    );
                }
                else {
                    const int savedMode = activeMode;
                    const bool savedSingle = activeAnvilDupe2SingleItem;
                    const bool savedSource = activeAnvilDupe2SourceBoxMode;
                    const bool savedWindow = activeAnvilDupe2NbtWindowMode;
                    const bool savedMaterial = activeAnvilDupe2MaterialSlotMode;
                    const bool savedAetherAnvil = aetherAnvilCarrierActive;
                    activeMode = 5;
                    aetherAnvilCarrierActive = true;
                    activeAnvilDupe2SingleItem = true;
                    activeAnvilDupe2SourceBoxMode = false;
                    activeAnvilDupe2NbtWindowMode = false;
                    activeAnvilDupe2MaterialSlotMode = false;
                    rewritten = rewriteAnvilTransaction(
                        static_cast<InventoryTransactionPacket*>(packet),
                        addedDestinationCount
                    );
                    activeMode = savedMode;
                    activeAnvilDupe2SingleItem = savedSingle;
                    activeAnvilDupe2SourceBoxMode = savedSource;
                    activeAnvilDupe2NbtWindowMode = savedWindow;
                    activeAnvilDupe2MaterialSlotMode = savedMaterial;
                    aetherAnvilCarrierActive = savedAetherAnvil;
                }
            }
        }
        else if (activeMode == 4) {
            if (nestedEarlyResultInjected) {
                rewritten = finalizeEarlyNestedDescriptors(
                    static_cast<InventoryTransactionPacket*>(packet)
                );
                addedDestinationCount = 0;
                nestedEarlyResultInjected = false;
                if (rewritten &&
                    cachedEarlyNestedResult &&
                    cachedEarlyNestedResult->isValid()) {
                    rememberExpectedResult(*cachedEarlyNestedResult);
                }
                else if (!rewritten) {
                    writeDiagnostic(
                        "[NESTED_EARLY_SEND_WARN] final descriptor rebuild failed"
                    );
                }
                resetNativeItemStack(cachedEarlyNestedResult);
            }
            else {
                rewritten = rewriteDedicatedNestedTransaction(
                    static_cast<InventoryTransactionPacket*>(packet),
                    addedDestinationCount
                );
            }
        }
        if (rewritten) {
            rewrittenDestinationCount = addedDestinationCount;
            rewriteNoticePending = true;
            renameNoticePending = false;
            waitingForCursorPlacement = false;
            cursorSnapshotValid = false;
            pendingDestinationSlots.clear();
            nextDestinationIndex = 0;
            if (activeMode == 5 &&
                activeAnvilDupe2NbtWindowMode &&
                nbtWindowStagePending) {
                settleTicks = 0;
                responseWaitTicks = kNbtWindowAckTimeoutTicks;
                stage = Stage::WaitForNbtWindowAck;
                writeDiagnostic(
                    "[ANVIL_NBT_WINDOW_ACK_WAIT] ticks=%d window=-13 materialSlot=%u destinationSlot=%d",
                    responseWaitTicks,
                    kAnvilMaterialInventorySlot,
                    stagedAnvilDestinationSlot
                );
            }
            else if (activeMode == 5 &&
                activeAnvilDupe2MaterialSlotMode &&
                anvilOutputStagePending) {
                settleTicks = kInventorySettleTicks;
                responseWaitTicks = 0;
                stage = Stage::ExtractAnvilOutput;
                writeDiagnostic(
                    "[ANVIL_-13_STAGE_WAIT] ticks=%d destinationSlot=%d",
                    settleTicks,
                    stagedAnvilDestinationSlot
                );
            }
            else {
                responseWaitTicks = kServerResponseCaptureTicks;
                stage = Stage::Completed;
            }
            logLegacyTransaction(
                "OUT_AFTER",
                getValidatedTransaction(
                    static_cast<InventoryTransactionPacket*>(packet)
                )
            );
            logInventorySnapshot("LOCAL_IMMEDIATELY_AFTER_REWRITE");
        }
        else {
            if (activeMode == 4 &&
                nestedRewriteMatchedAndRejected) {
                cancel = true;
                nestedAbortNoticePending = true;
                nestedRewriteMatchedAndRejected = false;
                sawDyeShulkerIngredient = false;
                sawDyeOtherIngredient = false;
                sawNamedDyeIngredient = false;
                resetNativeItemStack(cachedDyeShulkerIngredient);
                writeDiagnostic(
                    useNestedDyeDropCarrier()
                    ? "[NESTED_DYE_DROP_CANCEL] blocked genuine dye-result take after failed preflight"
                    : "[NESTED_ANVIL_CANCEL] blocked genuine anvil-result take after failed preflight"
                );
            }
            writeDiagnostic(
                "[OUT_RESULT] not a matching %s result transaction",
                getDupeModeName(activeMode)
            );
        }
        return;
    }
}

void Dupe::traceSourceBoxStackRequests(Packet* packet) {
    if (!packet || sourceBoxRequestDiagnosticCount >= 256) {
        return;
    }

    std::array<const AnvilStackRequestDataView*, 16> requests{};
    std::size_t requestCount = 0;
    auto appendRequest = [&](const AnvilStackRequestDataView* request) {
        if (!request || requestCount >= requests.size()) return;
        bool ignored = false;
        if (getActionVector(request, ignored)) requests[requestCount++] = request;
        };

    if (packet->getId() == PacketID::ItemStackRequest) {
        const auto* batchField =
            reinterpret_cast<AnvilStackRequestBatchView* const*>(
                reinterpret_cast<uintptr_t>(packet) + kItemStackRequestBatchOffset
                );
        if (isReadableRange(batchField, sizeof(*batchField)) && *batchField &&
            isReadableRange(*batchField, sizeof(**batchField)) &&
            isPointerVectorSane((*batchField)->requests, requests.size())) {
            for (const uintptr_t* it = (*batchField)->requests.begin;
                it != (*batchField)->requests.end; ++it) {
                appendRequest(reinterpret_cast<const AnvilStackRequestDataView*>(*it));
            }
        }
    }
    else if (packet->getId() == PacketID::PlayerAuthInput) {
        for (const uintptr_t offset : kAuthRequestOffsets) {
            const AnvilStackRequestDataView* request = getAuthRequest(packet, offset);
            if (request) {
                appendRequest(request);
                break;
            }
        }
    }

    if (requestCount == 0) {
        return;
    }

    writeDiagnostic(
        "[ANVIL_SOURCE_BOX_REQUEST_PACKET] packetId=0x%X requests=%zu stage=%d",
        static_cast<unsigned int>(packet->getId()), requestCount,
        static_cast<int>(stage)
    );
    ++sourceBoxRequestDiagnosticCount;

    for (std::size_t requestIndex = 0;
        requestIndex < requestCount && sourceBoxRequestDiagnosticCount < 256;
        ++requestIndex) {
        const AnvilStackRequestDataView* request = requests[requestIndex];
        bool hasFilterStrings = false;
        const RawPointerVectorView* actions = getActionVector(request, hasFilterStrings);
        if (!actions) continue;

        const std::size_t actionCount = static_cast<std::size_t>(actions->end - actions->begin);
        writeDiagnostic(
            "[ANVIL_SOURCE_BOX_REQUEST] requestId=%llu actions=%zu filterStrings=%d",
            static_cast<unsigned long long>(request->requestId), actionCount,
            hasFilterStrings ? 1 : 0
        );
        ++sourceBoxRequestDiagnosticCount;

        for (std::size_t actionIndex = 0;
            actionIndex < actionCount && sourceBoxRequestDiagnosticCount < 256;
            ++actionIndex) {
            const uintptr_t action = actions->begin[actionIndex];
            constexpr uintptr_t requiredBytes =
                kFullNameTransferDestinationOffset + kFullNameSlotOffset + 1;
            if (!isReadableRange(reinterpret_cast<const void*>(action), requiredBytes)) {
                writeDiagnostic(
                    "[ANVIL_SOURCE_BOX_REQUEST_ACTION] requestId=%llu index=%zu unreadable=1",
                    static_cast<unsigned long long>(request->requestId), actionIndex
                );
                ++sourceBoxRequestDiagnosticCount;
                continue;
            }

            const uint8_t type = *reinterpret_cast<const uint8_t*>(action + kActionTypeOffset);
            const uint8_t count = *reinterpret_cast<const uint8_t*>(action + kTransferCountOffset);
            const int8_t sourceContainer = *reinterpret_cast<const int8_t*>(action + kTransferSourceOffset);
            const uint8_t sourceSlot = *reinterpret_cast<const uint8_t*>(action + kTransferSourceOffset + kLegacySlotOffset);
            const int8_t legacyDestination = *reinterpret_cast<const int8_t*>(action + kLegacyTransferDestinationOffset);
            const uint8_t legacyDestinationSlot = *reinterpret_cast<const uint8_t*>(action + kLegacyTransferDestinationOffset + kLegacySlotOffset);
            const int8_t fullDestination = *reinterpret_cast<const int8_t*>(action + kFullNameTransferDestinationOffset);
            const uint8_t fullDestinationSlot = *reinterpret_cast<const uint8_t*>(action + kFullNameTransferDestinationOffset + kFullNameSlotOffset);

            writeDiagnostic(
                "[ANVIL_SOURCE_BOX_REQUEST_ACTION] requestId=%llu index=%zu type=%u count=%u source={container=%d,slot=%u} legacyDestination={container=%d,slot=%u} fullDestination={container=%d,slot=%u}",
                static_cast<unsigned long long>(request->requestId), actionIndex,
                static_cast<unsigned int>(type), static_cast<unsigned int>(count),
                static_cast<int>(sourceContainer), static_cast<unsigned int>(sourceSlot),
                static_cast<int>(legacyDestination), static_cast<unsigned int>(legacyDestinationSlot),
                static_cast<int>(fullDestination), static_cast<unsigned int>(fullDestinationSlot)
            );
            ++sourceBoxRequestDiagnosticCount;
        }
    }
}

bool Dupe::rewriteItemStackRequest(Packet* packet) {
    if (!packet || activeStackCount <= 1) {
        return false;
    }

    std::array<AnvilStackRequestDataView*, 16> requests{};
    std::size_t requestCount = 0;

    auto appendRequest = [&](const AnvilStackRequestDataView* request) {
        if (!request || requestCount >= requests.size()) {
            return;
        }
        bool ignored = false;
        if (getActionVector(request, ignored)) {
            requests[requestCount++] =
                const_cast<AnvilStackRequestDataView*>(request);
        }
        };

    if (packet->getId() == PacketID::ItemStackRequest) {
        const auto* batchField =
            reinterpret_cast<AnvilStackRequestBatchView* const*>(
                reinterpret_cast<uintptr_t>(packet) +
                kItemStackRequestBatchOffset
                );
        if (isReadableRange(batchField, sizeof(*batchField)) &&
            *batchField &&
            isReadableRange(*batchField, sizeof(**batchField)) &&
            isPointerVectorSane((*batchField)->requests, requests.size())) {
            for (const uintptr_t* it = (*batchField)->requests.begin;
                it != (*batchField)->requests.end;
                ++it) {
                appendRequest(
                    reinterpret_cast<const AnvilStackRequestDataView*>(*it)
                );
            }
        }
    }
    else if (packet->getId() == PacketID::PlayerAuthInput) {
        for (const uintptr_t offset : kAuthRequestOffsets) {
            const AnvilStackRequestDataView* request =
                getAuthRequest(packet, offset);
            if (request) {
                appendRequest(request);
                break;
            }
        }
    }

    int rewrittenActions = 0;
    for (std::size_t requestIndex = 0;
        requestIndex < requestCount;
        ++requestIndex) {
        AnvilStackRequestDataView* request = requests[requestIndex];
        bool hasFilterStrings = false;
        const RawPointerVectorView* actions =
            getActionVector(request, hasFilterStrings);
        if (!actions) {
            continue;
        }

        bool hasOptionalRecipeAction = false;
        for (const uintptr_t* it = actions->begin;
            it != actions->end;
            ++it) {
            if (isReadableRange(
                reinterpret_cast<const void*>(*it),
                kActionTypeOffset + 1
            ) &&
                *reinterpret_cast<const uint8_t*>(
                    *it + kActionTypeOffset
                    ) == kCraftRecipeOptionalAction) {
                hasOptionalRecipeAction = true;
                break;
            }
        }

        const bool hasRenameMarker =
            hasFilterStrings || hasOptionalRecipeAction;
        for (const uintptr_t* it = actions->begin;
            it != actions->end;
            ++it) {
            const uintptr_t action = *it;
            if (!isReadableRange(
                reinterpret_cast<const void*>(action),
                kTransferSourceOffset + 1
            )) {
                continue;
            }

            const uint8_t type = *reinterpret_cast<const uint8_t*>(
                action + kActionTypeOffset
                );
            const int8_t sourceContainer =
                *reinterpret_cast<const int8_t*>(
                    action + kTransferSourceOffset
                    );
            const bool takesAnvilResult =
                type == kTakeAction &&
                (sourceContainer == kAnvilResultPreviewContainer ||
                    (sourceContainer == kCreatedOutputContainer &&
                        hasRenameMarker));
            if (!takesAnvilResult) {
                continue;
            }

            auto* count = reinterpret_cast<uint8_t*>(
                action + kTransferCountOffset
                );
            if (!isWritableRange(count, sizeof(*count))) {
                writeDiagnostic(
                    "[STACK_REQUEST_SKIP] requestId=%llu action=%p reason=count-not-writable",
                    static_cast<unsigned long long>(request->requestId),
                    reinterpret_cast<void*>(action)
                );
                continue;
            }

            const uint8_t originalCount = *count;
            if (originalCount == 0) {
                writeDiagnostic(
                    "[STACK_REQUEST_SKIP] requestId=%llu action=%p reason=zero-count",
                    static_cast<unsigned long long>(request->requestId),
                    reinterpret_cast<void*>(action)
                );
                continue;
            }

            *count = static_cast<uint8_t>(activeStackCount);
            const DecodedDestination destination = decodeDestination(
                reinterpret_cast<const void*>(action)
            );
            writeDiagnostic(
                "[STACK_REQUEST_REWRITE] requestId=%llu action=%p sourceContainer=%d originalCount=%u requestedCount=%u destinationContainer=%d destinationSlot=%u destinationValid=%d",
                static_cast<unsigned long long>(request->requestId),
                reinterpret_cast<void*>(action),
                static_cast<int>(sourceContainer),
                static_cast<unsigned int>(originalCount),
                static_cast<unsigned int>(*count),
                destination.valid
                ? static_cast<int>(destination.container)
                : -1,
                destination.valid
                ? static_cast<unsigned int>(destination.slot)
                : 0U,
                destination.valid ? 1 : 0
            );
            ++rewrittenActions;
        }
    }

    if (requestCount == 0 || rewrittenActions == 0) {
        writeDiagnostic(
            "[STACK_REQUEST_NO_MATCH] packetId=0x%X requests=%zu rewrittenActions=%d",
            static_cast<unsigned int>(packet->getId()),
            requestCount,
            rewrittenActions
        );
    }
    return rewrittenActions > 0;
}

void Dupe::onRecievePacket(Packet* packet, bool& cancel) {
    (void)cancel;

    if (!packet) {
        return;
    }

    const PacketID id = packet->getId();
    if (activeMode == 5 &&
        activeAnvilDupe2NbtWindowMode &&
        stage == Stage::WaitForNbtWindowAck &&
        nbtWindowStagePending &&
        id == PacketID::LevelEvent) {
        auto* levelEvent = static_cast<LevelEventPacket*>(packet);
        const bool accepted =
            levelEvent->mEventId ==
            LevelEventPacket::EventID::SoundAnvilUsed ||
            levelEvent->mEventId ==
            LevelEventPacket::EventID::SoundAnvilBroken;
        writeDiagnostic(
            "[ANVIL_NBT_WINDOW_LEVEL_EVENT] event=0x%X accepted=%d pos={%.3f,%.3f,%.3f} data=%d",
            static_cast<unsigned int>(levelEvent->mEventId),
            accepted ? 1 : 0,
            levelEvent->mPos.x,
            levelEvent->mPos.y,
            levelEvent->mPos.z,
            levelEvent->data
        );
        if (accepted) {
            nbtWindowFirstStageAccepted = false;
            responseWaitTicks = 0;
            settleTicks = kNbtWindowExtractDelayTicks;
            stage = Stage::ExtractAnvilOutput;
            writeDiagnostic(
                "[ANVIL_NBT_WINDOW_ACK] event=0x%X delayTicks=%d next=-13-authoritative-probe",
                static_cast<unsigned int>(levelEvent->mEventId),
                settleTicks
            );
        }
        return;
    }

    if (activeMode == 7 &&
        enchantSequencePhase != EnchantSequencePhase::Idle &&
        incomingDiagnosticCount < 256) {
        writeDiagnostic(
            "[ENCHANT_SEQUENCE_IN] packetId=0x%X phase=%d unit=%llu",
            static_cast<unsigned int>(id),
            static_cast<int>(enchantSequencePhase),
            static_cast<unsigned long long>(enchantSequenceIndex + 1)
        );
        ++incomingDiagnosticCount;
    }
    if (activeMode == 7 &&
        stage == Stage::WaitForRenameTake &&
        id == PacketID::ContainerClose) {
        resetNativeItemStack(cachedEnchantInput);
        resetNativeItemStack(cachedEnchantMaterial);
        resetNativeItemStack(cachedEnchantDonor);
        enchantTransactionPending = false;
        enchantSettleTicks = 0;
        enchantSequencePhase = EnchantSequencePhase::Idle;
        enchantSequenceDestinationSlots.clear();
        enchantSequenceCompletedSlots.clear();
        enchantSequenceIndex = 0;
        enchantSequenceInitialMaterialCount = 0;
        return;
    }

    if (stage != Stage::ReadAnvilMaterial &&
        stage != Stage::Completed &&
        stage != Stage::DropInventory &&
        stage != Stage::DropResponse) {
        return;
    }

    if (incomingDiagnosticCount < 256) {
        writeDiagnostic(
            "[IN_ANY] packetId=0x%X packet=%p",
            static_cast<unsigned int>(id),
            packet
        );
        ++incomingDiagnosticCount;
    }

    if (id != PacketID::InventoryContent &&
        id != PacketID::InventorySlot &&
        id != PacketID::ItemStackResponse &&
        id != PacketID::ContainerClose &&
        id != PacketID::PacketViolationWarning) {
        return;
    }

    if (stage == Stage::ReadAnvilMaterial &&
        anvilMaterialProbeSent &&
        !anvilMaterialAuthoritativePacketReceived &&
        (id == PacketID::InventoryContent ||
            id == PacketID::InventorySlot)) {
        anvilMaterialInventoryPacketObserved = true;
        ++anvilMaterialInventoryPacketCount;
        writeDiagnostic(
            "[ANVIL_-13_MATERIAL_RESPONSE_CANDIDATE] packetId=0x%X packet=%d; waiting for exact post-apply controller readback",
            static_cast<unsigned int>(id),
            anvilMaterialInventoryPacketCount
        );
    }

    if (stage == Stage::Completed &&
        (activeMode == 5 || activeMode == 7 || activeMode == 8) &&
        anvilResyncProbeSent &&
        responseWaitTicks > 0 &&
        (id == PacketID::InventoryContent ||
            id == PacketID::InventorySlot ||
            id == PacketID::ItemStackResponse)) {
        anvilResyncInventoryPacketObserved = true;
        ++anvilResyncInventoryPacketCount;

        bool exactTrackedDestination = false;
        if (activeMode == 5 &&
            (activeAnvilDupe2MaterialSlotMode ||
                activeAnvilDupe2NbtWindowMode)) {
            if (id == PacketID::ItemStackResponse) {
                writeDiagnostic(
                    activeAnvilDupe2NbtWindowMode
                    ? "[ANVIL_NBT_WINDOW_DESTINATION_RESPONSE] packetId=0x%X packet=%d state=%s ignored=early-stack-response"
                    : "[ANVIL_-13_DESTINATION_RESPONSE] packetId=0x%X packet=%d state=%s ignored=early-stack-response",
                    static_cast<unsigned int>(id),
                    anvilResyncInventoryPacketCount,
                    getAnvilMaterialReadbackName(
                        anvilResyncReadbackState
                    )
                );
                return;
            }

            LocalPlayer* localPlayer = Game::getLocalPlayer();
            PlayerInventory* inventory = localPlayer
                ? localPlayer->playerInventory
                : nullptr;
            int destinationState = kAnvilMaterialReadbackUnavailable;
            if (inventory && inventory->container &&
                expectedResultStack &&
                expectedResultStack->isValid()) {
                for (const int slot : rewrittenDestinationSlots) {
                    if (slot < 0 || slot >= kInventorySlotCount) {
                        continue;
                    }
                    const uint8_t expectedCount =
                        expectedDestinationCounts[slot];
                    if (expectedCount == 0) {
                        continue;
                    }
                    ItemStack* item = inventory->container->getItem(slot);
                    if (!item) {
                        continue;
                    }
                    if (!item->isValid() || item->mCount == 0) {
                        destinationState = std::max(
                            destinationState,
                            kAnvilMaterialReadbackAir
                        );
                        continue;
                    }
                    destinationState = kAnvilMaterialReadbackDifferent;
                    if (!matchesExpectedResult(item, expectedCount)) {
                        continue;
                    }
                    ComparableItemSummary expectedWire{};
                    ComparableItemSummary actualWire{};
                    if (comparableItemPayloadsEqual(
                        expectedResultStack.get(),
                        item,
                        expectedWire,
                        actualWire
                    )) {
                        exactTrackedDestination = true;
                        destinationState = kAnvilMaterialReadbackExact;
                        break;
                    }
                }
            }
            anvilResyncReadbackState = destinationState;
            anvilResyncAuthoritativePacketReceived =
                exactTrackedDestination;
            writeDiagnostic(
                activeAnvilDupe2NbtWindowMode
                ? "[ANVIL_NBT_WINDOW_DESTINATION_RESPONSE] packetId=0x%X packet=%d state=%s exactTrackedDestination=%d"
                : "[ANVIL_-13_DESTINATION_RESPONSE] packetId=0x%X packet=%d state=%s exactTrackedDestination=%d",
                static_cast<unsigned int>(id),
                anvilResyncInventoryPacketCount,
                getAnvilMaterialReadbackName(destinationState),
                exactTrackedDestination ? 1 : 0
            );
        }
        else {
            anvilResyncAuthoritativePacketReceived = true;
        }
    }

    writeDiagnostic(
        "[IN] packetId=0x%X packet=%p",
        static_cast<unsigned int>(id),
        packet
    );
    logInventorySnapshot("LOCAL_ON_SERVER_INVENTORY_PACKET");
}

void Dupe::observeBatch(
    const AnvilStackRequestBatchView* batch
) {
    if (!batch ||
        !isReadableRange(batch, sizeof(*batch)) ||
        !isPointerVectorSane(batch->requests, 16)) {
        return;
    }

    for (const uintptr_t* it = batch->requests.begin;
        it != batch->requests.end;
        ++it) {
        const uintptr_t rawRequest = *it;
        const auto* request =
            reinterpret_cast<const AnvilStackRequestDataView*>(rawRequest);
        if (observeRequest(request)) {
            return;
        }
    }
}

bool Dupe::observeRequest(
    const AnvilStackRequestDataView* request
) {
    bool hasFilterStrings = false;
    const RawPointerVectorView* actions =
        getActionVector(request, hasFilterStrings);
    if (!actions || actions->begin == actions->end) {
        return false;
    }

    bool hasOptionalRecipeAction = false;
    for (const uintptr_t* it = actions->begin;
        it != actions->end;
        ++it) {
        const uintptr_t rawAction = *it;
        if (!isReadableRange(
            reinterpret_cast<const void*>(rawAction),
            kActionTypeOffset + 1
        )) {
            continue;
        }

        const uint8_t type = *reinterpret_cast<const uint8_t*>(
            rawAction + kActionTypeOffset
            );
        if (type == kCraftRecipeOptionalAction) {
            hasOptionalRecipeAction = true;
            break;
        }
    }

    const bool hasRenameMarker =
        hasFilterStrings || hasOptionalRecipeAction;

    bool observedRelevantAction = false;
    for (const uintptr_t* it = actions->begin;
        it != actions->end;
        ++it) {
        const uintptr_t rawAction = *it;
        if (!isReadableRange(
            reinterpret_cast<const void*>(rawAction),
            kTransferSourceOffset + 1
        )) {
            continue;
        }

        const uint8_t type = *reinterpret_cast<const uint8_t*>(
            rawAction + kActionTypeOffset
            );
        const int8_t sourceContainer =
            *reinterpret_cast<const int8_t*>(
                rawAction + kTransferSourceOffset
                );

        const bool takesAnvilResult =
            type == kTakeAction &&
            (sourceContainer == kAnvilResultPreviewContainer ||
                (sourceContainer == kCreatedOutputContainer &&
                    hasRenameMarker));

        const bool placesTrackedCursorResult =
            waitingForCursorPlacement &&
            (type == kPlaceAction || type == kSwapAction) &&
            sourceContainer == kCursorContainer;

        if (!takesAnvilResult && !placesTrackedCursorResult) {
            continue;
        }

        const DecodedDestination destination =
            decodeDestination(reinterpret_cast<const void*>(rawAction));
        if (!destination.valid) {
            continue;
        }

        observedRelevantAction = true;
        if (destination.container == kCursorContainer) {
            waitingForCursorPlacement = true;
            captureWaitTicks = 0;
            snapshotEmptyInventoryForCursor();
            continue;
        }

        if (isPlayerInventoryContainer(destination.container)) {
            armInventoryCapture(static_cast<int>(destination.slot));
            return true;
        }
    }

    return observedRelevantAction;
}

void Dupe::armInventoryCapture(int inventorySlot) {
    if (inventorySlot < 0 ||
        inventorySlot >= kInventorySlotCount ||
        stage != Stage::WaitForRenameTake) {
        return;
    }

    renamedInventorySlot = inventorySlot;
    stage = Stage::WaitForRenamedItem;
    settleTicks = kInventorySettleTicks;
    captureWaitTicks = 0;
    waitingForCursorPlacement = false;
    cursorSnapshotValid = false;
    renameNoticePending = true;
}

void Dupe::snapshotEmptyInventoryForCursor() {
    LocalPlayer* localPlayer = Game::getLocalPlayer();
    if (!localPlayer ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container) {
        cursorSnapshotValid = false;
        return;
    }

    for (int slot = 0; slot < kInventorySlotCount; ++slot) {
        ItemStack* stack =
            localPlayer->playerInventory->container->getItem(slot);
        emptyBeforeCursor[slot] = !stack || !stack->isValid();
    }
    cursorSnapshotValid = true;
}

bool Dupe::tryCaptureCursorPlacementFallback(
    LocalPlayer* localPlayer
) {
    if (!cursorSnapshotValid ||
        !localPlayer ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container) {
        return false;
    }

    for (int slot = 0; slot < kInventorySlotCount; ++slot) {
        if (!emptyBeforeCursor[slot]) {
            continue;
        }

        ItemStack* stack =
            localPlayer->playerInventory->container->getItem(slot);
        if (stack && stack->isValid()) {
            armInventoryCapture(slot);
            return true;
        }
    }

    return false;
}

std::vector<int> Dupe::findEmptyInventorySlots(
    LocalPlayer* localPlayer
) const {
    std::vector<int> slots;

    if (!localPlayer ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container) {
        return slots;
    }

    for (int slot = kMainInventoryFirstSlot;
        slot < kInventorySlotCount;
        ++slot) {
        ItemStack* stack =
            localPlayer->playerInventory->container->getItem(slot);
        if (!stack || !stack->isValid()) {
            slots.push_back(slot);
        }
    }

    for (int slot = 0; slot < kMainInventoryFirstSlot; ++slot) {
        ItemStack* stack =
            localPlayer->playerInventory->container->getItem(slot);
        if (!stack || !stack->isValid()) {
            slots.push_back(slot);
        }
    }

    return slots;
}

void Dupe::observeEnchantSlots(
    ComplexInventoryTransaction* transaction
) {
    if (activeMode != 7 ||
        enchantPacketInFlight ||
        !transaction ||
        transaction->type !=
        ComplexInventoryTransaction::Type::NormalTransaction) {
        return;
    }

    if (!canCloneNativeItemStack()) {
        writeDiagnostic(
            "[ENCHANT_CAPTURE_ABORT] native ItemStack copy preflight failed itemCopy=%p itemVtable=%p",
            reinterpret_cast<void*>(
                Addresses::ItemStackBase_CopyConstructor
                ),
            reinterpret_cast<void*>(Addresses::ItemStackVTable)
        );
        return;
    }

    const ItemStack* observedInput = nullptr;
    const ItemStack* observedMaterial = nullptr;
    bool inputTouched = false;
    bool materialTouched = false;

    for (const auto& [source, actions] : transaction->data.actions) {
        if (source.type != InventorySourceType::ContainerInventory ||
            source.containerId != ContainerID::PlayerUIOnly) {
            continue;
        }

        for (const InventoryAction& action : actions) {
            if (action.slot == kEnchantInputUiSlot) {
                inputTouched = true;
                if (action.toItem.isValid() &&
                    action.toItem.mCount == 1) {
                    observedInput = &action.toItem;
                }
                else {
                    observedInput = nullptr;
                }
            }
            else if (action.slot == kEnchantMaterialUiSlot) {
                materialTouched = true;
                if (action.toItem.isValid() &&
                    action.toItem.mCount > 0) {
                    observedMaterial = &action.toItem;
                }
                else {
                    observedMaterial = nullptr;
                }
            }
        }
    }

    if (inputTouched && !observedInput) {
        resetNativeItemStack(cachedEnchantInput);
    }
    if (materialTouched && !observedMaterial) {
        resetNativeItemStack(cachedEnchantMaterial);
    }

    if (observedInput) {
        const ItemStack* richestInput = observedInput;
        bool richestHasUserData = observedInput->mUserData != 0;
        const uint64_t observedNetworkHash = hashBytes(
            &observedInput->mNetIdVariant,
            sizeof(observedInput->mNetIdVariant)
        );
        for (const auto& [source, actions] : transaction->data.actions) {
            (void)source;
            for (const InventoryAction& action : actions) {
                const ItemStack* candidates[] = {
                    &action.fromItem,
                    &action.toItem
                };
                for (const ItemStack* candidate : candidates) {
                    if (!candidate ||
                        !candidate->isValid() ||
                        !isSameStackKind(*candidate, *observedInput) ||
                        hashBytes(
                            &candidate->mNetIdVariant,
                            sizeof(candidate->mNetIdVariant)
                        ) != observedNetworkHash) {
                        continue;
                    }

                    const bool candidateHasUserData =
                        candidate->mUserData != 0;
                    if (candidateHasUserData && !richestHasUserData) {
                        richestInput = candidate;
                        richestHasUserData = true;
                    }
                }
            }
        }

        resetNativeItemStack(cachedEnchantInput);
        cachedEnchantInput = std::make_unique<ItemStack>(*richestInput);
        cachedEnchantInput->mCount = 1;
        writeDiagnostic(
            "[ENCHANT_CAPTURE_INPUT] id=%d aux=%d userData=%p existingEnchantments=%llu netHash=%016llX",
            getItemId(*cachedEnchantInput),
            static_cast<int>(cachedEnchantInput->mAuxValue),
            reinterpret_cast<void*>(cachedEnchantInput->mUserData),
            static_cast<unsigned long long>(
                getNativeEnchantEntryCount(*cachedEnchantInput)
                ),
            static_cast<unsigned long long>(hashBytes(
                &cachedEnchantInput->mNetIdVariant,
                sizeof(cachedEnchantInput->mNetIdVariant)
            ))
        );
    }

    if (observedMaterial) {
        resetNativeItemStack(cachedEnchantMaterial);
        cachedEnchantMaterial =
            std::make_unique<ItemStack>(*observedMaterial);
        writeDiagnostic(
            "[ENCHANT_CAPTURE_MATERIAL] count=%u id=%s recognizedLapis=%d source=ui124/15",
            static_cast<unsigned int>(cachedEnchantMaterial->mCount),
            getItemIdentifier(*cachedEnchantMaterial).c_str(),
            isLapis(*cachedEnchantMaterial) ? 1 : 0
        );
    }

    const bool ready =
        cachedEnchantInput &&
        cachedEnchantInput->isValid() &&
        cachedEnchantInput->mCount == 1 &&
        cachedEnchantMaterial &&
        cachedEnchantMaterial->isValid() &&
        cachedEnchantMaterial->mCount >= 2;
    if (ready) {
        enchantTransactionPending = true;
        enchantSettleTicks = kInventorySettleTicks;
        captureWaitTicks = 0;
        writeDiagnostic(
            "[ENCHANT_ARM] copies=%d settleTicks=%d",
            activeCopyCount,
            enchantSettleTicks
        );
    }
    else {
        enchantTransactionPending = false;
    }
}

void Dupe::observeDyeCraftIngredients(
    ComplexInventoryTransaction* transaction
) {
    if (!transaction ||
        transaction->type !=
        ComplexInventoryTransaction::Type::NormalTransaction) {
        return;
    }

    const bool previouslySawShulker = sawDyeShulkerIngredient;
    const bool previouslySawOther = sawDyeOtherIngredient;
    const bool previouslySawNamedDye = sawNamedDyeIngredient;

    for (const auto& [source, actions] : transaction->data.actions) {
        if (!isCraftingPseudoSource(source) ||
            static_cast<int>(source.containerId) !=
            kCraftingUseIngredientWindowId) {
            continue;
        }

        for (const InventoryAction& action : actions) {
            const bool fromHasItem =
                action.fromItem.isValid() &&
                action.fromItem.mCount > 0;
            const bool toHasItem =
                action.toItem.isValid() &&
                action.toItem.mCount > 0;
            if (fromHasItem || !toHasItem) {
                continue;
            }

            if (isShulkerBox(action.toItem)) {
                sawDyeShulkerIngredient = true;
                const int observedItemCount =
                    getShulkerContentEntryCount(action.toItem);
                const int cachedItemCount =
                    cachedDyeShulkerIngredient
                    ? getShulkerContentEntryCount(
                        *cachedDyeShulkerIngredient
                    )
                    : -1;
                const bool observedHasNbt =
                    action.toItem.mUserData != 0;
                const bool cachedHasNbt =
                    cachedDyeShulkerIngredient &&
                    cachedDyeShulkerIngredient->mUserData != 0;
                const bool sameCachedNetworkStack =
                    cachedDyeShulkerIngredient &&
                    isSameStackKind(
                        *cachedDyeShulkerIngredient,
                        action.toItem
                    ) &&
                    hashBytes(
                        &cachedDyeShulkerIngredient->mNetIdVariant,
                        sizeof(
                            cachedDyeShulkerIngredient->mNetIdVariant
                            )
                    ) == hashBytes(
                        &action.toItem.mNetIdVariant,
                        sizeof(action.toItem.mNetIdVariant)
                    );
                std::vector<int> observedSlots;
                const bool structurallyValidNestedTarget =
                    activeMode == 4 &&
                    observedHasNbt &&
                    collectShulkerContainerEntrySlots(
                        action.toItem,
                        observedSlots
                    );
                const bool hasUsableNestedTargetPayload =
                    activeMode == 4;

                if (activeMode == 4) {
                    if (hasUsableNestedTargetPayload) {
                        resetNativeItemStack(
                            cachedDyeShulkerIngredient
                        );
                        cachedDyeShulkerIngredient =
                            std::make_unique<ItemStack>(action.toItem);
                        writeDiagnostic(
                            "[NESTED_EMPTY_B_CACHE] userData=%p entries=%d id=%d aux=%d",
                            reinterpret_cast<void*>(
                                cachedDyeShulkerIngredient->mUserData
                                ),
                            observedItemCount,
                            getItemId(*cachedDyeShulkerIngredient),
                            static_cast<int>(
                                cachedDyeShulkerIngredient->mAuxValue
                                )
                        );
                    }
                    else if (sameCachedNetworkStack) {
                        writeDiagnostic(
                            "[BOXINBOX_B_CACHE_KEEP] reduced preview matched cached network stack entries=%d userData=%p",
                            observedItemCount,
                            reinterpret_cast<void*>(
                                action.toItem.mUserData
                                )
                        );
                    }
                    else {
                        resetNativeItemStack(
                            cachedDyeShulkerIngredient
                        );
                        writeDiagnostic(
                            "[NESTED_EMPTY_B_REJECT] B is not empty entries=%d userData=%p",
                            observedItemCount,
                            reinterpret_cast<void*>(
                                action.toItem.mUserData
                                )
                        );
                    }
                }
                else if (!cachedDyeShulkerIngredient ||
                    !sameCachedNetworkStack ||
                    observedItemCount > cachedItemCount ||
                    (observedHasNbt && !cachedHasNbt)) {
                    const char* cacheReason =
                        !cachedDyeShulkerIngredient
                        ? "first"
                        : !sameCachedNetworkStack
                        ? "new-network-stack"
                        : "richer-payload";
                    resetNativeItemStack(cachedDyeShulkerIngredient);
                    cachedDyeShulkerIngredient =
                        std::make_unique<ItemStack>(action.toItem);
                    writeDiagnostic(
                        "[DYE_NBT_CACHE] reason=%s userData=%p items=%d id=%d aux=%d",
                        cacheReason,
                        reinterpret_cast<void*>(
                            cachedDyeShulkerIngredient->mUserData
                            ),
                        observedItemCount,
                        getItemId(*cachedDyeShulkerIngredient),
                        static_cast<int>(
                            cachedDyeShulkerIngredient->mAuxValue
                            )
                    );
                }
                else {
                    writeDiagnostic(
                        "[DYE_NBT_CACHE_KEEP] same network stack retained richer payload cachedItems=%d observedItems=%d",
                        cachedItemCount,
                        observedItemCount
                    );
                }
            }
            else {
                sawDyeOtherIngredient = true;
                if (isLikelyDye(action.toItem)) {
                    sawNamedDyeIngredient = true;
                }
            }
        }
    }

    if (previouslySawShulker != sawDyeShulkerIngredient ||
        previouslySawOther != sawDyeOtherIngredient ||
        previouslySawNamedDye != sawNamedDyeIngredient) {
        writeDiagnostic(
            "[DYE_MARKERS] shulker=%d other=%d namedDye=%d",
            sawDyeShulkerIngredient ? 1 : 0,
            sawDyeOtherIngredient ? 1 : 0,
            sawNamedDyeIngredient ? 1 : 0
        );
    }
}

bool Dupe::rewriteOldDyeCraftTransaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    return rewriteDyeCraftTransaction(
        packet,
        addedDestinationCount,
        DyeRewriteStrategy::OldInventory
    );
}

bool Dupe::rewriteNewDyeCraftTransaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    return rewriteDyeCraftTransaction(
        packet,
        addedDestinationCount,
        DyeRewriteStrategy::NewInventory
    );
}

bool Dupe::rewriteDyeDupe3Transaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    return rewriteDyeCraftTransaction(
        packet,
        addedDestinationCount,
        activeDupe3Destination == 1
        ? DyeRewriteStrategy::DirectInventory
        : DyeRewriteStrategy::DirectWorld
    );
}

bool Dupe::rewriteDyeDupe4Transaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    return rewriteDyeCraftTransaction(
        packet,
        addedDestinationCount,
        activeDupe4Destination == 1
        ? DyeRewriteStrategy::VirtualInventory
        : DyeRewriteStrategy::VirtualWorld
    );
}

bool Dupe::rewriteDedicatedNestedTransaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    return rewriteNestedAnvilTransaction(packet, addedDestinationCount);
}

bool Dupe::rewriteBoxInBoxTransaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    return rewriteDyeCraftTransaction(
        packet,
        addedDestinationCount,
        DyeRewriteStrategy::NestedBox
    );
}

bool Dupe::rewriteAetherCraftTransaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    addedDestinationCount = 0;

    const bool dyeCarrier = std::clamp(aetherCarrierMode, 0, 2) == 0;
    if (dyeCarrier &&
        (!sawDyeShulkerIngredient || !sawNamedDyeIngredient)) {
        return false;
    }

    if (mapartNbtRequested && !mapartNbtReady) {
        writeDiagnostic("[AETHER_ABORT] selected mapart NBT is not loaded");
        return false;
    }

    LocalPlayer* localPlayer = Game::getLocalPlayer();
    if (!packet || !localPlayer || !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container ||
        !canCloneNativeItemStack()) {
        return false;
    }

    ComplexInventoryTransaction* transaction =
        getValidatedTransaction(packet);
    if (!transaction ||
        transaction->type !=
        ComplexInventoryTransaction::Type::NormalTransaction) {
        return false;
    }

    const InventorySource craftSource{
        InventorySourceType::NonImplementedFeatureTODO,
        static_cast<ContainerID>(kCraftingResultWindowId)
    };
    const InventorySource inventorySource{
        InventorySourceType::ContainerInventory,
        ContainerID::Inventory
    };
    auto craftIt = transaction->data.actions.find(craftSource);
    auto inventoryIt = transaction->data.actions.find(inventorySource);
    if (craftIt == transaction->data.actions.end() ||
        inventoryIt == transaction->data.actions.end()) {
        return false;
    }

    InventoryAction* generated = nullptr;
    for (InventoryAction& action : craftIt->second) {
        if (action.fromItem.isValid() && action.fromItem.mCount > 0 &&
            (!action.toItem.isValid() || action.toItem.mCount == 0)) {
            generated = &action;
            break;
        }
    }

    InventoryAction* destination = nullptr;
    std::array<bool, kInventorySlotCount> reservedSlots{};
    for (InventoryAction& action : inventoryIt->second) {
        if (action.slot < kInventorySlotCount)
            reservedSlots[action.slot] = true;
        if (!destination &&
            (!action.fromItem.isValid() || action.fromItem.mCount == 0) &&
            action.toItem.isValid() && action.toItem.mCount > 0) {
            destination = &action;
        }
    }

    if (!generated || !destination ||
        !isShulkerBox(generated->fromItem) ||
        !isShulkerBox(destination->toItem)) {
        return false;
    }

    void* donorRoot = nullptr;
    if (NbtCommand::hasLoadedTag()) {
        std::unique_ptr<CompoundTag> loaded =
            NbtCommand::copyLoadedTag();
        if (loaded) {
            std::size_t modernCount = 0;
            std::size_t legacyCount = 0;
            if (stripExportOnlyLegacyItems(
                loaded.get(), modernCount, legacyCount)) {
                writeDiagnostic(
                    "[AETHER_WIRE_CANONICALIZE] path=late-rewrite removedLegacy=1 modernItems=%zu legacyItems=%zu",
                    modernCount,
                    legacyCount
                );
            }
        }
        donorRoot = loaded.release();
    }
    if (!donorRoot) {
        for (const auto& [source, actions] : transaction->data.actions) {
            (void)source;
            for (const InventoryAction& action : actions) {
                const ItemStack* donor = nullptr;
                if (isShulkerBox(action.fromItem) && action.fromItem.mUserData)
                    donor = &action.fromItem;
                else if (isShulkerBox(action.toItem) && action.toItem.mUserData)
                    donor = &action.toItem;
                if (donor) {
                    donorRoot = reinterpret_cast<void*>(donor->mUserData);
                    break;
                }
            }
            if (donorRoot) break;
        }
    }
    if (!donorRoot && cachedDyeShulkerIngredient)
        donorRoot = reinterpret_cast<void*>(cachedDyeShulkerIngredient->mUserData);
    if (!donorRoot && aetherBoxInBox) {
        auto emptyRoot = Tag::createNative<CompoundTag>();
        auto emptyItems = Tag::createNative<ListTag>();
        if (emptyRoot && emptyItems) {
            emptyItems->mType = Tag::Compound;
            if (emptyRoot->putNative("Items", std::move(emptyItems)))
                donorRoot = emptyRoot.release();
        }
    }
    if (!donorRoot) {
        writeDiagnostic(
            "[AETHER_ABORT] no loaded, transaction, or cached native NBT root"
        );
        return false;
    }

    ItemStack completed(generated->fromItem);
    completed.mUserData = reinterpret_cast<__int64>(donorRoot);
    ItemStack empty(generated->toItem);

    ItemStack nativeCompleted;
    NativeItemStackResetGuard nativeCompletedGuard(&nativeCompleted);
    if (!rebuildNativeItemStack(completed, nativeCompleted)) {
        writeDiagnostic(
            "[AETHER_ABORT] native ItemStack rebuild failed"
        );
        return false;
    }
    nativeCompleted.item = generated->fromItem.item;
    nativeCompleted.mBlock = generated->fromItem.mBlock;
    nativeCompleted.mAuxValue = generated->fromItem.mAuxValue;
    nativeCompleted.mCount = generated->fromItem.mCount;
    nativeCompleted.mValid = generated->fromItem.mValid;
    nativeCompleted.mNetIdVariant = generated->fromItem.mNetIdVariant;
    completed.setNull(std::nullopt);
    new (&completed) ItemStack(nativeCompleted);

    if (!completed.isValid() || completed.mCount == 0) {
        return false;
    }

    const uint32_t resultSlot = generated->slot;
    const uint32_t destinationSlot = destination->slot;
    const InventorySource resultSource = generated->source;
    const InventorySource destinationSource = destination->source;

    std::vector<int> extraSlots;
    const int requestedTotal = std::clamp(activeCopyCount, 1, 64);
    for (const int slot : findEmptyInventorySlots(localPlayer)) {
        if (extraSlots.size() >=
            static_cast<std::size_t>(requestedTotal - 1)) {
            break;
        }
        if (slot < 0 || slot >= kInventorySlotCount ||
            reservedSlots[slot]) {
            continue;
        }
        extraSlots.push_back(slot);
    }

    const ItemStackNetIdVariant generatedNetId =
        generated->fromItem.mNetIdVariant;
    const ItemStackNetIdVariant destinationNetId =
        destination->toItem.mNetIdVariant;
    InventoryAction patchedGenerated(
        resultSlot,
        &completed,
        &empty,
        resultSource.containerId,
        resultSource.type,
        resultSource.flags
    );
    InventoryAction patchedDestination(
        destinationSlot,
        &empty,
        &completed,
        destinationSource.containerId,
        destinationSource.type,
        destinationSource.flags
    );
    patchedGenerated.fromItem.mNetIdVariant = generatedNetId;
    patchedGenerated.fromItemDescriptor =
        NetworkItemStackDescriptor(&patchedGenerated.fromItem);
    patchedDestination.toItem.mNetIdVariant = destinationNetId;
    patchedDestination.toItemDescriptor =
        NetworkItemStackDescriptor(&patchedDestination.toItem);
    *generated = std::move(patchedGenerated);
    *destination = std::move(patchedDestination);

    for (const int slot : extraSlots) {
        InventoryAction sourceAction(
            resultSlot,
            &completed,
            &empty,
            resultSource.containerId,
            resultSource.type,
            resultSource.flags
        );
        InventoryAction destinationAction(
            static_cast<uint32_t>(slot),
            &empty,
            &completed,
            destinationSource.containerId,
            destinationSource.type,
            destinationSource.flags
        );
        transaction->data.addAction(sourceAction);
        transaction->data.addAction(destinationAction);
    }

    rewrittenDestinationSlots.clear();
    expectedDestinationCounts.fill(0);
    rewrittenDestinationSlots.push_back(
        static_cast<int>(destinationSlot)
    );
    if (destinationSlot < kInventorySlotCount) {
        expectedDestinationCounts[destinationSlot] = completed.mCount;
    }
    for (const int slot : extraSlots) {
        rewrittenDestinationSlots.push_back(slot);
        expectedDestinationCounts[slot] = completed.mCount;
    }
    rememberExpectedResult(completed);
    addedDestinationCount = static_cast<int>(extraSlots.size());
    aetherCraftResyncPending = false;
    resetNativeItemStack(cachedDyeShulkerIngredient);

    writeDiagnostic(
        "[AETHER_SUCCESS] loaded=%d total=%llu requested=%d extra=%d resultSlot=%u destinationSlot=%u items=%d colors=%llu",
        NbtCommand::hasLoadedTag() ? 1 : 0,
        static_cast<unsigned long long>(1 + extraSlots.size()),
        requestedTotal,
        addedDestinationCount,
        resultSlot,
        destinationSlot,
        getShulkerContentEntryCount(completed),
        static_cast<unsigned long long>(getFirstMapColorsSize(completed))
    );
    return true;
}

bool Dupe::rewriteDyeCraftTransaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount,
    DyeRewriteStrategy strategy
) {
    addedDestinationCount = 0;

    if (strategy == DyeRewriteStrategy::NestedBox) {
        nestedRewriteMatchedAndRejected = false;
    }

    const bool preserveOriginalDescriptors =
        strategy != DyeRewriteStrategy::OldInventory;
    const bool dupe4VirtualSource =
        strategy == DyeRewriteStrategy::VirtualWorld ||
        strategy == DyeRewriteStrategy::VirtualInventory;
    const bool dupe3RichResult =
        strategy == DyeRewriteStrategy::DirectWorld ||
        strategy == DyeRewriteStrategy::DirectInventory;
    const bool directWorldDrop =
        strategy == DyeRewriteStrategy::DirectWorld ||
        strategy == DyeRewriteStrategy::VirtualWorld;
    const bool directInventory =
        strategy == DyeRewriteStrategy::DirectInventory ||
        strategy == DyeRewriteStrategy::VirtualInventory;
    const bool richCraftResult =
        dupe3RichResult || dupe4VirtualSource;
    const bool generalDupe3Crafting =
        richCraftResult &&
        (dupe4VirtualSource
            ? activeDupe4AnyCrafting
            : activeDupe3AnyCrafting);
    const bool nestedBox =
        strategy == DyeRewriteStrategy::NestedBox;
    const bool nestedAllCraft =
        nestedBox && activeNestedAllCrafting;
    const char* strategyName = nestedBox
        ? "boxinbox"
        : dupe4VirtualSource && directInventory
        ? "dyedupe4-inventory"
        : dupe4VirtualSource
        ? "dyedupe4-world"
        : directInventory
        ? "dyedupe3-inventory"
        : directWorldDrop
        ? "dyedupe3-world"
        : preserveOriginalDescriptors
        ? "newdyedupe"
        : "olddyedupe";

    LocalPlayer* localPlayer = Game::getLocalPlayer();
    if (!packet ||
        !localPlayer ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container) {
        return false;
    }

    ComplexInventoryTransaction* transaction =
        getValidatedTransaction(packet);
    if (!transaction ||
        !isReadableRange(transaction, sizeof(*transaction)) ||
        transaction->type !=
        ComplexInventoryTransaction::Type::NormalTransaction) {
        return false;
    }

    observeDyeCraftIngredients(transaction);
    if (!generalDupe3Crafting &&
        !nestedAllCraft &&
        (!sawDyeShulkerIngredient || !sawDyeOtherIngredient)) {
        return false;
    }

    struct CraftResultCandidate {
        InventoryAction* action;
        std::vector<InventoryAction>* actionGroup;
        std::size_t actionIndex;
        InventorySource source;
    };
    struct ResultDestinationCandidate {
        InventoryAction* action;
        std::vector<InventoryAction>* actionGroup;
        InventorySource source;
        std::size_t actionIndex;
    };

    std::vector<CraftResultCandidate> candidates;
    std::vector<ResultDestinationCandidate> resultDestinations;
    std::array<bool, kInventorySlotCount> reservedInventorySlots{};
    std::unique_ptr<ItemStack> shulkerIngredient;
    NativeItemStackOwnerResetGuard shulkerIngredientGuard(
        shulkerIngredient
    );
    bool shulkerIngredientFromCache = false;

    for (auto& [source, actions] : transaction->data.actions) {
        for (std::size_t actionIndex = 0;
            actionIndex < actions.size();
            ++actionIndex) {
            InventoryAction& action = actions[actionIndex];
            const bool fromHasItem =
                action.fromItem.isValid() &&
                action.fromItem.mCount > 0;
            const bool toHasItem =
                action.toItem.isValid() &&
                action.toItem.mCount > 0;

            if (!shulkerIngredient &&
                isCraftingPseudoSource(source) &&
                static_cast<int>(source.containerId) ==
                kCraftingUseIngredientWindowId &&
                !fromHasItem &&
                toHasItem &&
                isShulkerBox(action.toItem)) {
                shulkerIngredient =
                    std::make_unique<ItemStack>(action.toItem);
            }

            if (source.type ==
                InventorySourceType::ContainerInventory &&
                source.containerId == ContainerID::Inventory &&
                action.slot < kInventorySlotCount) {
                reservedInventorySlots[action.slot] = true;

                if (!fromHasItem && toHasItem) {
                    resultDestinations.push_back({
                        &action,
                        &actions,
                        source,
                        actionIndex
                        });
                }
            }

            if (!isCraftingPseudoSource(source) ||
                static_cast<int>(source.containerId) !=
                kCraftingResultWindowId ||
                !fromHasItem ||
                toHasItem ||
                (!generalDupe3Crafting &&
                    !nestedAllCraft &&
                    !isShulkerBox(action.fromItem))) {
                continue;
            }

            candidates.push_back({
                &action,
                &actions,
                actionIndex,
                source
                });
        }
    }

    if (cachedDyeShulkerIngredient &&
        cachedDyeShulkerIngredient->isValid() &&
        (nestedBox || cachedDyeShulkerIngredient->mUserData != 0) &&
        (nestedBox || !shulkerIngredient ||
            shulkerIngredient->mUserData == 0 ||
            getShulkerContentEntryCount(*cachedDyeShulkerIngredient) >
            getShulkerContentEntryCount(*shulkerIngredient))) {
        resetNativeItemStack(shulkerIngredient);
        shulkerIngredient = std::make_unique<ItemStack>(
            *cachedDyeShulkerIngredient
        );
        shulkerIngredientFromCache = true;
    }

    const CraftResultCandidate* selectedCandidate = nullptr;
    const ResultDestinationCandidate* selectedDestination = nullptr;
    for (const CraftResultCandidate& candidate : candidates) {
        for (const ResultDestinationCandidate& destination :
            resultDestinations) {
            if (destination.action &&
                isSameStackKind(
                    candidate.action->fromItem,
                    destination.action->toItem
                )) {
                selectedCandidate = &candidate;
                selectedDestination = &destination;
                break;
            }
        }
        if (selectedCandidate) {
            break;
        }
    }

    if (!selectedCandidate ||
        !selectedDestination ||
        !selectedDestination->action ||
        !selectedCandidate->actionGroup ||
        selectedCandidate->actionGroup->size() != 1 ||
        selectedCandidate->actionIndex != 0) {
        return false;
    }

    const uint32_t resultSlot = selectedCandidate->action->slot;
    const InventorySource resultSource =
        selectedCandidate->action->source;
    const std::size_t resultActionIndex =
        selectedCandidate->actionIndex;
    const uint64_t resultFromDescriptorHash = hashBytes(
        &selectedCandidate->action->fromItemDescriptor,
        sizeof(selectedCandidate->action->fromItemDescriptor)
    );
    const uintptr_t resultUserData = static_cast<uintptr_t>(
        selectedCandidate->action->fromItem.mUserData
        );

    const InventorySource destinationSource =
        selectedDestination->action->source;
    const uint32_t destinationSlot =
        selectedDestination->action->slot;
    const std::size_t destinationActionIndex =
        selectedDestination->actionIndex;
    const uint64_t destinationToDescriptorHash = hashBytes(
        &selectedDestination->action->toItemDescriptor,
        sizeof(selectedDestination->action->toItemDescriptor)
    );

    ItemStack craftedResult(selectedDestination->action->toItem);
    NativeItemStackResetGuard craftedResultGuard(&craftedResult);
    if (!craftedResult.isValid() || craftedResult.mCount == 0) {
        return false;
    }

    if (generalDupe3Crafting &&
        !isShulkerBox(craftedResult) &&
        shulkerIngredient) {
        resetNativeItemStack(shulkerIngredient);
        shulkerIngredientFromCache = false;
        writeDiagnostic(
            "[DYE_ANY_CRAFT_CACHE_BYPASS] non-shulker result did not consume cached shulker NBT"
        );
    }

    ItemStack* empty = ItemStack::getEmptyItem();
    if (!empty) {
        return false;
    }

    writeDiagnostic(
        "[DYE_RICH_RESULT] strategy=%s inputSource=%s inputUserData=%p inputItems=%d pseudoUserData=%p pseudoItems=%d destinationUserData=%p destinationItems=%d resultFromDesc=%016llX destinationToDesc=%016llX count=%u",
        strategyName,
        shulkerIngredientFromCache ? "cache" : "packet",
        shulkerIngredient
        ? reinterpret_cast<void*>(shulkerIngredient->mUserData)
        : nullptr,
        shulkerIngredient
        ? getShulkerContentEntryCount(*shulkerIngredient)
        : 0,
        reinterpret_cast<void*>(resultUserData),
        getShulkerContentEntryCount(selectedCandidate->action->fromItem),
        reinterpret_cast<void*>(craftedResult.mUserData),
        getShulkerContentEntryCount(craftedResult),
        static_cast<unsigned long long>(resultFromDescriptorHash),
        static_cast<unsigned long long>(destinationToDescriptorHash),
        static_cast<unsigned int>(craftedResult.mCount)
    );

    const int requestedCopies = activeCopyCount;
    const bool dupe3Stacking =
        richCraftResult &&
        activeStacking &&
        activeStackCount > static_cast<int>(craftedResult.mCount);
    const int dupe3ResultCount = dupe3Stacking
        ? (std::clamp(activeStackCount, 1, 255) /
            static_cast<int>(craftedResult.mCount)) *
        static_cast<int>(craftedResult.mCount)
        : static_cast<int>(craftedResult.mCount);
    const int dupe3ActionRepeats = dupe3Stacking
        ? (dupe3ResultCount + static_cast<int>(craftedResult.mCount) - 1) /
        static_cast<int>(craftedResult.mCount)
        : 1;
    int dupe3EffectiveCopies = requestedCopies;
    if (activeCraftDropStack && dupe3RichResult) {
        writeDiagnostic(
            "[CRAFT_DROP_STACK_ARM] genuineCount=%u requestedCount=%d sourceRepeats=%d worldDrops=1 resultId=%d resultItems=%d",
            static_cast<unsigned int>(craftedResult.mCount),
            dupe3ResultCount,
            dupe3ActionRepeats,
            getItemId(craftedResult),
            getShulkerContentEntryCount(craftedResult)
        );
    }
    if (dupe3RichResult && directWorldDrop && dupe3Stacking) {
        constexpr int kDupe3WorldActionBudget = 3800;
        const int maxStackedWorldDrops = std::max(
            1,
            kDupe3WorldActionBudget / (dupe3ResultCount + 1)
        );
        dupe3EffectiveCopies = std::min(
            requestedCopies,
            maxStackedWorldDrops
        );
        if (dupe3EffectiveCopies != requestedCopies) {
            writeDiagnostic(
                "[DYE3_STACK_CAP] destination=world requested=%d effective=%d stackCount=%d actionBudget=%d",
                requestedCopies,
                dupe3EffectiveCopies,
                dupe3ResultCount,
                kDupe3WorldActionBudget
            );
        }
    }
    if (nestedBox) {
        {
            nestedRewriteMatchedAndRejected = true;
            if ((!nestedAllCraft && !sawNamedDyeIngredient) ||
                !isExecutableAddress(Addresses::ItemStackBase_save) ||
                !isExecutableAddress(Addresses::CompoundTag_putByte) ||
                !isExecutableAddress(Addresses::ItemStack_loadItem)) {
                writeDiagnostic(
                    "[NESTED_V1_ABORT] preflight namedDye=%d save=%p putByte=%p load=%p",
                    sawNamedDyeIngredient ? 1 : 0,
                    reinterpret_cast<void*>(Addresses::ItemStackBase_save),
                    reinterpret_cast<void*>(Addresses::CompoundTag_putByte),
                    reinterpret_cast<void*>(Addresses::ItemStack_loadItem)
                );
                return false;
            }

            if (!captureNestedSourceBox(localPlayer) ||
                !cachedNestedSourceBox ||
                !cachedNestedSourceBox->isValid() ||
                !isShulkerBox(*cachedNestedSourceBox)) {
                writeDiagnostic(
                    "[NESTED_V1_ABORT] A hotbar slot %d is not a shulker",
                    activeNestedSourceSlot
                );
                return false;
            }
            ItemStack sourceA(*cachedNestedSourceBox);
            NativeItemStackResetGuard sourceAGuard(&sourceA);
            sourceA.mCount = 1;
            ItemStack nestedResult(sourceA);
            NativeItemStackResetGuard nestedResultGuard(&nestedResult);
            auto* nestedRoot = reinterpret_cast<CompoundTag*>(
                nestedResult.mUserData
                );
            MutableShulkerItemsEntry outerItemsEntry;
            if (!findMutableShulkerItemsEntry(
                nestedRoot,
                outerItemsEntry
            )) {
                writeDiagnostic(
                    "[NESTED_B_OUTER_ABORT] A has no native Items list aRoot=%p",
                    nestedRoot
                );
                return false;
            }

            ListTag* outerItems =
                outerItemsEntry.iterator->second.get<ListTag>();
            if (!outerItems) {
                writeDiagnostic(
                    "[NESTED_B_OUTER_ABORT] A native Items node changed type"
                );
                return false;
            }
            outerItems->mList.clear();
            outerItems->mType = Tag::Type::Compound;
            outerItems->mList.reserve(27);
            for (uint8_t slot = 0; slot < 27; ++slot) {
                std::unique_ptr<CompoundTag> nestedAEntry =
                    saveNativeItemEntry(sourceA);
                if (!nestedAEntry ||
                    !putNativeByte(nestedAEntry.get(), "Slot", slot)) {
                    writeDiagnostic(
                        "[NESTED_V1_ABORT] native serialization of A failed at B slot=%u",
                        static_cast<unsigned int>(slot)
                    );
                    return false;
                }
                outerItems->mList.emplace_back(std::move(nestedAEntry));
            }

            if (!nestedAllCraft) {
                nestedResult.item = craftedResult.item;
                nestedResult.mBlock = craftedResult.mBlock;
                nestedResult.mAuxValue = craftedResult.mAuxValue;
                nestedResult.mValid = craftedResult.mValid;
            }
            nestedResult.mCount = craftedResult.mCount;
            nestedResult.mNetIdVariant = craftedResult.mNetIdVariant;

            Item* expectedOuterItem = nestedAllCraft
                ? sourceA.item.get()
                : craftedResult.item.get();
            const int16_t expectedOuterAux = nestedAllCraft
                ? sourceA.mAuxValue
                : craftedResult.mAuxValue;
            if (nestedResult.item.get() != expectedOuterItem ||
                nestedResult.mAuxValue != expectedOuterAux ||
                getShulkerContentEntryCount(nestedResult) != 27) {
                writeDiagnostic(
                    "[NESTED_B_OUTER_ABORT] rebuilt result lost output identity all=%d outerId=%d expectedId=%d outerAux=%d expectedAux=%d entries=%d",
                    nestedAllCraft ? 1 : 0,
                    getItemId(nestedResult),
                    expectedOuterItem ? expectedOuterItem->itemId : 0,
                    static_cast<int>(nestedResult.mAuxValue),
                    static_cast<int>(expectedOuterAux),
                    getShulkerContentEntryCount(nestedResult)
                );
                return false;
            }

            std::unique_ptr<CompoundTag> persistedNestedResult =
                saveNativeItemEntry(nestedResult);
            if (!persistedNestedResult ||
                !isExecutableAddress(Addresses::ItemStack_loadItem)) {
                writeDiagnostic(
                    "[NESTED_NATIVE_LOAD_ABORT] save/load preflight failed entry=%p loadItem=%p",
                    persistedNestedResult.get(),
                    reinterpret_cast<void*>(Addresses::ItemStack_loadItem)
                );
                return false;
            }

            ItemStack nativeLoadedResult;
            NativeItemStackResetGuard nativeLoadedResultGuard(
                &nativeLoadedResult
            );
            dupeLoadItem(nativeLoadedResult, *persistedNestedResult);
            nativeLoadedResult.mCount = craftedResult.mCount;
            nativeLoadedResult.mNetIdVariant =
                craftedResult.mNetIdVariant;
            if (!nativeLoadedResult.isValid() ||
                nativeLoadedResult.item.get() != expectedOuterItem ||
                nativeLoadedResult.mAuxValue != expectedOuterAux ||
                nativeLoadedResult.mCount != craftedResult.mCount ||
                getShulkerContentEntryCount(nativeLoadedResult) != 27) {
                writeDiagnostic(
                    "[NESTED_NATIVE_LOAD_ABORT] round-trip mismatch valid=%d id=%d expectedId=%d aux=%d expectedAux=%d count=%u expectedCount=%u entries=%d",
                    nativeLoadedResult.isValid() ? 1 : 0,
                    getItemId(nativeLoadedResult),
                    expectedOuterItem ? expectedOuterItem->itemId : 0,
                    static_cast<int>(nativeLoadedResult.mAuxValue),
                    static_cast<int>(expectedOuterAux),
                    static_cast<unsigned int>(nativeLoadedResult.mCount),
                    static_cast<unsigned int>(craftedResult.mCount),
                    getShulkerContentEntryCount(nativeLoadedResult)
                );
                return false;
            }

            rewrittenDestinationSlots.clear();
            expectedDestinationCounts.fill(0);
            if (!selectedCandidate->actionGroup ||
                selectedCandidate->actionGroup->size() != 1 ||
                selectedCandidate->actionIndex != 0 ||
                !selectedDestination->actionGroup ||
                selectedDestination->actionGroup->size() != 1 ||
                selectedDestination->actionIndex != 0) {
                writeDiagnostic(
                    "[NESTED_KIT_ABORT] genuine result/target groups are not singular"
                );
                return false;
            }

            const int requestedNestedCopies = nestedOnlyModule
                ? 1
                : std::clamp(activeCopyCount, 1, 27);
            std::vector<int> nestedAdditionalSlots;
            for (const int emptySlot : findEmptyInventorySlots(localPlayer)) {
                if (nestedAdditionalSlots.size() >=
                    static_cast<std::size_t>(requestedNestedCopies)) {
                    break;
                }
                if (emptySlot < 0 ||
                    emptySlot >= kInventorySlotCount ||
                    reservedInventorySlots[emptySlot]) {
                    continue;
                }
                nestedAdditionalSlots.push_back(emptySlot);
            }
            if (nestedAdditionalSlots.empty() ||
                !normalizeAndAppendRichInventoryPairs(
                    selectedCandidate->actionGroup,
                    selectedDestination->actionGroup,
                    resultSource,
                    resultSlot,
                    destinationSource,
                    destinationSlot,
                    nativeLoadedResult,
                    empty,
                    nestedAdditionalSlots
                )) {
                writeDiagnostic(
                    "[NESTED_KIT_ABORT] dyedupe inventory carrier unavailable"
                );
                return false;
            }

            const InventoryAction& finalResultAction =
                selectedCandidate->actionGroup->front();
            const InventoryAction& finalDestinationAction =
                selectedDestination->actionGroup->front();
            const DescriptorPayloadSummary resultWire =
                inspectDescriptorPayload(
                    finalResultAction.fromItemDescriptor
                );
            const DescriptorPayloadSummary destinationWire =
                inspectDescriptorPayload(
                    finalDestinationAction.toItemDescriptor
                );
            const bool wirePayloadsMatch =
                resultWire.readable &&
                destinationWire.readable &&
                resultWire.size == destinationWire.size &&
                resultWire.hash == destinationWire.hash &&
                resultWire.hasNetId == destinationWire.hasNetId &&
                resultWire.blockRuntimeId ==
                destinationWire.blockRuntimeId;
            writeDiagnostic(
                "[NESTED_WIRE_PAYLOAD] match=%d result={ok=%d size=%zu hash=%016llX items=%d legacy=%d name=%d net=%u block=%d} destination={ok=%d size=%zu hash=%016llX items=%d legacy=%d name=%d net=%u block=%d}",
                wirePayloadsMatch ? 1 : 0,
                resultWire.readable ? 1 : 0,
                resultWire.size,
                static_cast<unsigned long long>(resultWire.hash),
                resultWire.containsItems ? 1 : 0,
                resultWire.containsLegacyItems ? 1 : 0,
                resultWire.containsCustomName ? 1 : 0,
                static_cast<unsigned int>(resultWire.hasNetId),
                resultWire.blockRuntimeId,
                destinationWire.readable ? 1 : 0,
                destinationWire.size,
                static_cast<unsigned long long>(destinationWire.hash),
                destinationWire.containsItems ? 1 : 0,
                destinationWire.containsLegacyItems ? 1 : 0,
                destinationWire.containsCustomName ? 1 : 0,
                static_cast<unsigned int>(destinationWire.hasNetId),
                destinationWire.blockRuntimeId
            );
            const DescriptorPayloadSummary extraResultWire =
                inspectDescriptorPayload(
                    selectedCandidate->actionGroup->back()
                    .fromItemDescriptor
                );
            const DescriptorPayloadSummary extraDestinationWire =
                inspectDescriptorPayload(
                    selectedDestination->actionGroup->back()
                    .toItemDescriptor
                );
            writeDiagnostic(
                "[NESTED_WIRE_EXTRA] match=%d slot=%d result={size=%zu hash=%016llX items=%d} destination={size=%zu hash=%016llX items=%d}",
                extraResultWire.readable &&
                extraDestinationWire.readable &&
                extraResultWire.size ==
                extraDestinationWire.size &&
                extraResultWire.hash ==
                extraDestinationWire.hash
                ? 1
                : 0,
                nestedAdditionalSlots.front(),
                extraResultWire.size,
                static_cast<unsigned long long>(extraResultWire.hash),
                extraResultWire.containsItems ? 1 : 0,
                extraDestinationWire.size,
                static_cast<unsigned long long>(
                    extraDestinationWire.hash
                    ),
                extraDestinationWire.containsItems ? 1 : 0
            );
            rewrittenDestinationSlots.emplace_back(
                static_cast<int>(destinationSlot)
            );
            if (destinationSlot < kInventorySlotCount) {
                expectedDestinationCounts[destinationSlot] =
                    nativeLoadedResult.mCount;
            }
            for (const int nestedSlot : nestedAdditionalSlots) {
                rewrittenDestinationSlots.emplace_back(nestedSlot);
                expectedDestinationCounts[nestedSlot] =
                    nativeLoadedResult.mCount;
            }
            resetNativeItemStack(nestedLocalShellStack);
            if (!nestedOnlyModule) {
                nestedLocalShellStack =
                    std::make_unique<ItemStack>(
                        nestedAllCraft ? sourceA : craftedResult
                    );
                if (nestedAllCraft &&
                    nestedLocalShellStack->mUserData != 0) {
                    MutableShulkerItemsEntry localItemsEntry;
                    auto* localRoot = reinterpret_cast<CompoundTag*>(
                        nestedLocalShellStack->mUserData
                        );
                    if (findMutableShulkerItemsEntry(
                        localRoot,
                        localItemsEntry
                    )) {
                        ListTag* localItems =
                            localItemsEntry.iterator->second.get<ListTag>();
                        if (localItems) {
                            localItems->mList.clear();
                            localItems->mType = Tag::Type::Compound;
                        }
                    }
                }
                nestedLocalShellStack->mCount =
                    nativeLoadedResult.mCount;
                nestedLocalShellStack->mNetIdVariant =
                    nativeLoadedResult.mNetIdVariant;
            }
            rememberExpectedResult(nativeLoadedResult);

            addedDestinationCount = static_cast<int>(
                nestedAdditionalSlots.size()
                );
            writeDiagnostic(
                "[NESTED_KIT_SUCCESS] all=%d aHotbar=%d aItems=%d injectedABoxes=27 outerNbtFromA=1 inventoryCopies=%d requestedCopies=%d firstExtraSlot=%d localPrediction=1 localPredictionItems=%d outerId=%d outerAux=%d carrier=dyedupe3-inventory-pair",
                nestedAllCraft ? 1 : 0,
                activeNestedSourceSlot,
                getShulkerContentEntryCount(sourceA),
                addedDestinationCount,
                requestedNestedCopies,
                nestedAdditionalSlots.front(),
                nestedOnlyModule
                ? getShulkerContentEntryCount(nativeLoadedResult)
                : nestedLocalShellStack
                ? getShulkerContentEntryCount(
                    *nestedLocalShellStack
                )
                : 0,
                getItemId(nativeLoadedResult),
                static_cast<int>(nativeLoadedResult.mAuxValue)
            );
            resetNativeItemStack(cachedDyeShulkerIngredient);
            resetNativeItemStack(cachedNestedSourceBox);
            nestedRewriteMatchedAndRejected = false;
            return true;
        }

        nestedRewriteMatchedAndRejected = true;
        if (!sawNamedDyeIngredient) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] current ingredient sequence did not contain a named dye marker"
            );
            return false;
        }
        if (!isExecutableAddress(Addresses::ItemStackBase_save) ||
            !isExecutableAddress(Addresses::CompoundTag_putByte) ||
            !isExecutableAddress(Addresses::ItemStack_loadItem)) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] native NBT functions unavailable save=%p putByte=%p load=%p",
                reinterpret_cast<void*>(Addresses::ItemStackBase_save),
                reinterpret_cast<void*>(Addresses::CompoundTag_putByte),
                reinterpret_cast<void*>(Addresses::ItemStack_loadItem)
            );
            return false;
        }

        if (!captureNestedSourceBox(localPlayer) ||
            !cachedNestedSourceBox ||
            !cachedNestedSourceBox->isValid() ||
            !isShulkerBox(*cachedNestedSourceBox)) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] hotbar slot %d does not contain source shulker A",
                activeNestedSourceSlot
            );
            return false;
        }

        if (!shulkerIngredient ||
            !shulkerIngredient->isValid() ||
            !isShulkerBox(*shulkerIngredient) ||
            shulkerIngredient->mUserData == 0) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] dyed target B with native Items NBT was not captured"
            );
            return false;
        }

        ItemStack sourceA(*cachedNestedSourceBox);
        NativeItemStackResetGuard sourceAGuard(&sourceA);
        sourceA.mCount = 1;
        const int sourceAItemCount =
            getShulkerContentEntryCount(sourceA);

        ItemStack nestedResult(*shulkerIngredient);
        NativeItemStackResetGuard nestedResultGuard(&nestedResult);
        auto* nestedRoot = reinterpret_cast<CompoundTag*>(
            nestedResult.mUserData
            );
        ListTag* outerItems = findMutableShulkerItemsList(nestedRoot);
        if (!outerItems) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] copied outer box lost its native Items list"
            );
            return false;
        }

        std::vector<int> occupiedSlots;
        const bool nativeSlotLayoutReadable =
            collectContainerEntrySlots(outerItems, occupiedSlots);
        if (!nativeSlotLayoutReadable) {
            if (outerItems->mList.empty() ||
                outerItems->mList.size() > 27) {
                writeDiagnostic(
                    "[BOXINBOX_ABORT] B Items count is outside 1-27 entries=%llu",
                    static_cast<unsigned long long>(outerItems->mList.size())
                );
                return false;
            }

            occupiedSlots.resize(outerItems->mList.size());
            for (std::size_t entryIndex = 0;
                entryIndex < occupiedSlots.size();
                ++entryIndex) {
                occupiedSlots[entryIndex] =
                    static_cast<int>(entryIndex);
            }
            writeDiagnostic(
                "[BOXINBOX_SLOT_FALLBACK] native Slot tags unreadable; packed %llu entries into Slots 0-%llu",
                static_cast<unsigned long long>(occupiedSlots.size()),
                static_cast<unsigned long long>(
                    occupiedSlots.empty() ? 0 : occupiedSlots.size() - 1
                    )
            );
        }

        for (std::size_t entryIndex = 0;
            entryIndex < outerItems->mList.size();
            ++entryIndex) {
            const std::unique_ptr<Tag>& entry =
                outerItems->mList[entryIndex];
            if (!entry || entry->getId() != Tag::Type::Compound) {
                writeDiagnostic(
                    "[NESTED_DYE_DROP_ABORT] B entry %llu is not a compound",
                    static_cast<unsigned long long>(entryIndex)
                );
                return false;
            }

            ItemStack placeholder;
            dupeLoadItem(placeholder,
                *reinterpret_cast<CompoundTag*>(entry.get())
            );
            const bool validPlaceholder =
                placeholder.isValid() &&
                placeholder.mCount == 1;
            const unsigned int placeholderCount =
                static_cast<unsigned int>(placeholder.mCount);
            placeholder.setNull(std::nullopt);
            if (!validPlaceholder) {
                writeDiagnostic(
                    "[NESTED_DYE_DROP_ABORT] B Slot=%d placeholder Count=%u; every occupied slot must contain exactly 1 item",
                    occupiedSlots[entryIndex],
                    placeholderCount
                );
                return false;
            }
        }

        const int replacedEntryCount =
            static_cast<int>(occupiedSlots.size());
        NbtComplexity sourceComplexity;
        const uint64_t descriptorCopies =
            2ULL * static_cast<uint64_t>(requestedCopies + 1);
        const bool sourceComplexityReadable =
            inspectItemNbt(sourceA, sourceComplexity);
        const uint64_t minimumProjectedBytes =
            static_cast<uint64_t>(replacedEntryCount) *
            (sourceComplexity.bytes + 256ULL) *
            descriptorCopies;
        const uint64_t minimumProjectedNodes =
            static_cast<uint64_t>(replacedEntryCount) *
            (sourceComplexity.nodes + 8ULL) *
            descriptorCopies;
        const int minimumProjectedDepth =
            sourceComplexity.maxDepth + 4;
        constexpr uint64_t kMaxProjectedDescriptorBytes =
            16ULL * 1024ULL * 1024ULL;
        constexpr uint64_t kMaxProjectedDescriptorNodes = 250000ULL;

        writeDiagnostic(
            "[BOXINBOX_PRE_BUDGET] readable=%d entries=%d aBytes=%llu aNodes=%llu aDepth=%d minimumBytes=%llu minimumNodes=%llu minimumDepth=%d descriptorCopies=%llu",
            sourceComplexityReadable ? 1 : 0,
            replacedEntryCount,
            static_cast<unsigned long long>(sourceComplexity.bytes),
            static_cast<unsigned long long>(sourceComplexity.nodes),
            sourceComplexity.maxDepth,
            static_cast<unsigned long long>(minimumProjectedBytes),
            static_cast<unsigned long long>(minimumProjectedNodes),
            minimumProjectedDepth,
            static_cast<unsigned long long>(descriptorCopies)
        );
        if (!sourceComplexityReadable ||
            minimumProjectedBytes >
            kMaxProjectedDescriptorBytes ||
            minimumProjectedNodes >
            kMaxProjectedDescriptorNodes) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] minimum nested payload exceeded safety budget bytes=%llu/%llu nodes=%llu/%llu depth=%d",
                static_cast<unsigned long long>(
                    minimumProjectedBytes
                    ),
                static_cast<unsigned long long>(
                    kMaxProjectedDescriptorBytes
                    ),
                static_cast<unsigned long long>(
                    minimumProjectedNodes
                    ),
                static_cast<unsigned long long>(
                    kMaxProjectedDescriptorNodes
                    ),
                minimumProjectedDepth
            );
            return false;
        }

        std::vector<std::unique_ptr<CompoundTag>> replacementEntries(
            occupiedSlots.size()
        );
        bool canonicalSourceValidated = false;
        for (std::size_t entryIndex = 0;
            entryIndex < occupiedSlots.size();
            ++entryIndex) {
            const int slot = occupiedSlots[entryIndex];
            replacementEntries[entryIndex] =
                saveNativeItemEntry(sourceA);
            if (!replacementEntries[entryIndex] ||
                !putNativeByte(
                    replacementEntries[entryIndex].get(),
                    "Slot",
                    static_cast<uint8_t>(slot)
                )) {
                writeDiagnostic(
                    "[BOXINBOX_ABORT] failed to serialize independent A entry for Slot=%d",
                    slot
                );
                return false;
            }

            if (nativeSlotLayoutReadable) {
                int serializedSlot = -1;
                if (!readContainerEntrySlot(
                    replacementEntries[entryIndex].get(),
                    serializedSlot
                ) ||
                    serializedSlot != slot) {
                    writeDiagnostic(
                        "[BOXINBOX_ABORT] serialized A entry has wrong Slot=%d expected=%d",
                        serializedSlot,
                        slot
                    );
                    return false;
                }
            }

            if (!canonicalSourceValidated) {
                ItemStack entryProbe;
                dupeLoadItem(entryProbe,
                    *replacementEntries[entryIndex]
                );
                const int probeItemCount =
                    getShulkerContentEntryCount(entryProbe);
                const bool probeIsShulker = isShulkerBox(entryProbe);
                const bool probeMatches =
                    isSameStackKind(entryProbe, sourceA) &&
                    probeIsShulker &&
                    probeItemCount == sourceAItemCount;
                const int probeId = getItemId(entryProbe);
                const int probeAux =
                    static_cast<int>(entryProbe.mAuxValue);
                const unsigned int probeCount =
                    static_cast<unsigned int>(entryProbe.mCount);
                entryProbe.setNull(std::nullopt);

                if (!probeMatches) {
                    writeDiagnostic(
                        "[BOXINBOX_ABORT] A canonical round-trip failed Slot=%d shulker=%d id=%d aux=%d count=%u items=%d expectedId=%d expectedAux=%d expectedItems=%d",
                        slot,
                        probeIsShulker ? 1 : 0,
                        probeId,
                        probeAux,
                        probeCount,
                        probeItemCount,
                        getItemId(sourceA),
                        static_cast<int>(sourceA.mAuxValue),
                        sourceAItemCount
                    );
                    return false;
                }
                canonicalSourceValidated = true;
            }
        }

        const int outerItemCountBefore =
            static_cast<int>(outerItems->mList.size());
        for (std::size_t entryIndex = 0;
            entryIndex < replacementEntries.size();
            ++entryIndex) {
            outerItems->mList[entryIndex] =
                std::move(replacementEntries[entryIndex]);
        }

        nestedResult.item = craftedResult.item;
        nestedResult.mBlock = craftedResult.mBlock;
        nestedResult.mAuxValue = craftedResult.mAuxValue;
        nestedResult.mCount = craftedResult.mCount;
        nestedResult.mValid = craftedResult.mValid;
        nestedResult.mNetIdVariant = craftedResult.mNetIdVariant;

        const int outerItemCount =
            getShulkerContentEntryCount(nestedResult);
        bool replacementsValid =
            outerItemCount == outerItemCountBefore;
        for (std::size_t entryIndex = 0;
            replacementsValid &&
            entryIndex < occupiedSlots.size();
            ++entryIndex) {
            const std::unique_ptr<Tag>& entry =
                outerItems->mList[entryIndex];
            if (!entry || entry->getId() != Tag::Type::Compound) {
                replacementsValid = false;
                break;
            }

            if (nativeSlotLayoutReadable) {
                const auto* itemCompound =
                    reinterpret_cast<const CompoundTag*>(entry.get());
                int storedSlot = -1;
                replacementsValid =
                    readContainerEntrySlot(itemCompound, storedSlot) &&
                    storedSlot == occupiedSlots[entryIndex];
            }
        }

        if (!nativeSlotLayoutReadable) {
            writeDiagnostic(
                "[BOXINBOX_SLOT_WRITE] trusted native putByte for %d ordinal Slot entries",
                replacedEntryCount
            );
        }

        NbtComplexity rebuiltComplexity;
        const bool rebuiltComplexityReadable =
            inspectItemNbt(nestedResult, rebuiltComplexity);
        const uint64_t projectedDescriptorBytes =
            rebuiltComplexity.bytes * descriptorCopies;
        const uint64_t projectedDescriptorNodes =
            rebuiltComplexity.nodes * descriptorCopies;
        writeDiagnostic(
            "[BOXINBOX_BUDGET] readable=%d rebuiltBytes=%llu rebuiltNodes=%llu rebuiltDepth=%d projectedBytes=%llu projectedNodes=%llu descriptorCopies=%llu",
            rebuiltComplexityReadable ? 1 : 0,
            static_cast<unsigned long long>(rebuiltComplexity.bytes),
            static_cast<unsigned long long>(rebuiltComplexity.nodes),
            rebuiltComplexity.maxDepth,
            static_cast<unsigned long long>(projectedDescriptorBytes),
            static_cast<unsigned long long>(projectedDescriptorNodes),
            static_cast<unsigned long long>(descriptorCopies)
        );
        if (!rebuiltComplexityReadable ||
            projectedDescriptorBytes >
            kMaxProjectedDescriptorBytes ||
            projectedDescriptorNodes >
            kMaxProjectedDescriptorNodes) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] rebuilt nested payload exceeded safety budget bytes=%llu/%llu nodes=%llu/%llu depth=%d",
                static_cast<unsigned long long>(
                    projectedDescriptorBytes
                    ),
                static_cast<unsigned long long>(
                    kMaxProjectedDescriptorBytes
                    ),
                static_cast<unsigned long long>(
                    projectedDescriptorNodes
                    ),
                static_cast<unsigned long long>(
                    kMaxProjectedDescriptorNodes
                    ),
                rebuiltComplexity.maxDepth
            );
            return false;
        }

        const bool canNormalizeGenuinePair =
            replacementsValid &&
            selectedCandidate->actionGroup &&
            selectedCandidate->actionGroup->size() == 1 &&
            selectedCandidate->actionIndex == 0 &&
            selectedDestination->actionGroup &&
            selectedDestination->actionGroup->size() == 1 &&
            selectedDestination->actionIndex == 0;
        if (!canNormalizeGenuinePair) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] output validation/group shape failed replacementsValid=%d outerItems=%d expectedOuterItems=%d resultSize=%llu resultIndex=%llu destinationSize=%llu destinationIndex=%llu",
                replacementsValid ? 1 : 0,
                outerItemCount,
                outerItemCountBefore,
                static_cast<unsigned long long>(
                    selectedCandidate->actionGroup
                    ? selectedCandidate->actionGroup->size()
                    : 0
                    ),
                static_cast<unsigned long long>(
                    selectedCandidate->actionIndex
                    ),
                static_cast<unsigned long long>(
                    selectedDestination->actionGroup
                    ? selectedDestination->actionGroup->size()
                    : 0
                    ),
                static_cast<unsigned long long>(
                    selectedDestination->actionIndex
                    )
            );
            return false;
        }

        rewrittenDestinationSlots.clear();
        expectedDestinationCounts.fill(0);
        if (!normalizeAndAppendRichDirectWorldDrops(
            transaction,
            selectedCandidate->actionGroup,
            selectedDestination->actionGroup,
            resultSource,
            resultSlot,
            destinationSource,
            destinationSlot,
            nestedResult,
            empty,
            requestedCopies
        )) {
            writeDiagnostic(
                "[BOXINBOX_ABORT] shared dyedupe3 direct-world rewrite failed"
            );
            return false;
        }

        addedDestinationCount = requestedCopies;
        writeDiagnostic(
            "[BOXINBOX_INJECT] aHotbar=%d replaceAllBItems=1 replaced=%d outerItems=%d aItems=%d aUserData=%p bUserData=%p",
            activeNestedSourceSlot,
            replacedEntryCount,
            outerItemCount,
            sourceAItemCount,
            reinterpret_cast<void*>(sourceA.mUserData),
            reinterpret_cast<void*>(nestedResult.mUserData)
        );
        writeDiagnostic(
            "[BOXINBOX_DIRECT_WORLD] destinationSlot=%u extraPairs=%d outerItems=%d inputSource=%s nativeDescriptors=1 directOnly=0 genuineDestination=kept",
            destinationSlot,
            requestedCopies,
            outerItemCount,
            shulkerIngredientFromCache ? "cache" : "packet"
        );
        writeDiagnostic(
            "[NESTED_DYE_DROP_SUCCESS] aHotbar=%d bEntries=%d directDrops=%d placeholderCount=1",
            activeNestedSourceSlot,
            outerItemCount,
            requestedCopies
        );
        resetNativeItemStack(cachedDyeShulkerIngredient);
        resetNativeItemStack(cachedNestedSourceBox);
        nestedRewriteMatchedAndRejected = false;
        return true;
    }

    if (dupe4VirtualSource) {
        const bool hasRichShulkerIngredient =
            shulkerIngredient &&
            shulkerIngredient->isValid() &&
            shulkerIngredient->mUserData != 0 &&
            getShulkerContentEntryCount(*shulkerIngredient) > 0;
        ItemStack dupe4Result(
            hasRichShulkerIngredient
            ? *shulkerIngredient
            : craftedResult
        );
        NativeItemStackResetGuard dupe4ResultGuard(&dupe4Result);
        if (hasRichShulkerIngredient) {
            dupe4Result.item = craftedResult.item;
            dupe4Result.mBlock = craftedResult.mBlock;
            dupe4Result.mAuxValue = craftedResult.mAuxValue;
            dupe4Result.mCount = craftedResult.mCount;
            dupe4Result.mValid = craftedResult.mValid;
            dupe4Result.mNetIdVariant = craftedResult.mNetIdVariant;
        }

        const bool canNormalizeGenuinePair =
            selectedCandidate->actionGroup &&
            selectedCandidate->actionGroup->size() == 1 &&
            selectedCandidate->actionIndex == 0 &&
            selectedDestination->actionGroup &&
            selectedDestination->actionGroup->size() == 1 &&
            selectedDestination->actionIndex == 0 &&
            selectedCandidate->actionGroup !=
            selectedDestination->actionGroup;
        if (!canNormalizeGenuinePair ||
            !dupe4Result.isValid() ||
            dupe4Result.mCount == 0) {
            writeDiagnostic(
                "[DYE4_VIRTUAL_ABORT] invalid genuine pair/result resultSize=%llu destinationSize=%llu",
                static_cast<unsigned long long>(
                    selectedCandidate->actionGroup
                    ? selectedCandidate->actionGroup->size()
                    : 0
                    ),
                static_cast<unsigned long long>(
                    selectedDestination->actionGroup
                    ? selectedDestination->actionGroup->size()
                    : 0
                    )
            );
            return false;
        }

        const unsigned int baseResultCount =
            static_cast<unsigned int>(dupe4Result.mCount);
        const bool stackingApplied =
            activeStacking &&
            activeStackCount > static_cast<int>(baseResultCount);
        const unsigned int perDestinationCount = stackingApplied
            ? (static_cast<unsigned int>(std::clamp(activeStackCount, 1, 255)) /
                baseResultCount) * baseResultCount
            : baseResultCount;
        const unsigned int destinationActionRepeats = stackingApplied
            ? (perDestinationCount + baseResultCount - 1) / baseResultCount
            : 1;
        const unsigned int maxCopiesByVirtualCount =
            kMaxTransactionItemCount / perDestinationCount;

        std::vector<int> dupe4InventorySlots;
        unsigned int actualDestinationCount = 0;
        if (directInventory) {
            const std::vector<int> emptySlots =
                findEmptyInventorySlots(localPlayer);
            for (const int emptySlot : emptySlots) {
                if (dupe4InventorySlots.size() >=
                    static_cast<std::size_t>(requestedCopies) ||
                    dupe4InventorySlots.size() >=
                    static_cast<std::size_t>(
                        maxCopiesByVirtualCount
                        )) {
                    break;
                }
                if (emptySlot < 0 ||
                    emptySlot >= kInventorySlotCount ||
                    reservedInventorySlots[emptySlot]) {
                    continue;
                }
                dupe4InventorySlots.push_back(emptySlot);
            }
            actualDestinationCount = static_cast<unsigned int>(
                dupe4InventorySlots.size()
                );
        }
        else {
            actualDestinationCount = std::min(
                static_cast<unsigned int>(requestedCopies),
                maxCopiesByVirtualCount
            );
        }

        if (actualDestinationCount == 0) {
            writeDiagnostic(
                "[DYE4_VIRTUAL_ABORT] no destination requested=%d maxByCount=%u destination=%s",
                requestedCopies,
                maxCopiesByVirtualCount,
                directInventory ? "inventory" : "world"
            );
            return true;
        }

        const unsigned int virtualSourceCount =
            perDestinationCount * actualDestinationCount;
        ItemStack genuineResult(dupe4Result);
        NativeItemStackResetGuard genuineResultGuard(&genuineResult);
        genuineResult.mCount = static_cast<uint8_t>(baseResultCount);
        ItemStack virtualSource(dupe4Result);
        NativeItemStackResetGuard virtualSourceGuard(&virtualSource);
        virtualSource.mCount = static_cast<uint8_t>(virtualSourceCount);

        std::vector<InventoryAction>* resultActions =
            selectedCandidate->actionGroup;
        std::vector<InventoryAction>* destinationActions =
            selectedDestination->actionGroup;
        resetNativeActionItemStacks(resultActions);
        resultActions->clear();
        resultActions->reserve(2);
        resultActions->emplace_back(
            resultSlot,
            &genuineResult,
            empty,
            resultSource.containerId,
            resultSource.type,
            resultSource.flags
        );
        resultActions->emplace_back(
            resultSlot,
            &virtualSource,
            empty,
            resultSource.containerId,
            resultSource.type,
            resultSource.flags
        );

        resetNativeActionItemStacks(destinationActions);
        destinationActions->clear();
        destinationActions->reserve(
            directInventory
            ? 1 +
            dupe4InventorySlots.size() *
            static_cast<std::size_t>(
                destinationActionRepeats
                )
            : 1
        );
        destinationActions->emplace_back(
            destinationSlot,
            empty,
            &genuineResult,
            destinationSource.containerId,
            destinationSource.type,
            destinationSource.flags
        );

        rewrittenDestinationSlots.clear();
        expectedDestinationCounts.fill(0);
        rememberExpectedResult(genuineResult);

        if (directInventory) {
            for (const int inventorySlot : dupe4InventorySlots) {
                for (unsigned int repeatIndex = 0;
                    repeatIndex < destinationActionRepeats;
                    ++repeatIndex) {
                    destinationActions->emplace_back(
                        static_cast<uint32_t>(inventorySlot),
                        empty,
                        &genuineResult,
                        destinationSource.containerId,
                        destinationSource.type,
                        destinationSource.flags
                    );
                }
                expectedDestinationCounts[inventorySlot] =
                    static_cast<uint8_t>(perDestinationCount);
            }
            rewrittenDestinationSlots = dupe4InventorySlots;
        }
        else {
            ItemStack worldResult(dupe4Result);
            NativeItemStackResetGuard worldResultGuard(&worldResult);
            worldResult.mCount = static_cast<uint8_t>(
                perDestinationCount
                );
            for (unsigned int copyIndex = 0;
                copyIndex < actualDestinationCount;
                ++copyIndex) {
                InventoryAction directDrop(
                    0,
                    empty,
                    &worldResult,
                    ContainerID::None,
                    InventorySourceType::WorldInteraction,
                    InventorySourceFlags::NoFlag
                );
                NativeInventoryActionResetGuard directDropGuard(
                    &directDrop
                );
                transaction->data.addAction(directDrop);
            }
        }

        addedDestinationCount = static_cast<int>(
            actualDestinationCount
            );
        writeDiagnostic(
            "[DYE4_VIRTUAL_SOURCE] source=-4.old genuineCount=%u virtualCount=%u destinations=%u requested=%d destination=%s stacking=%d perDestination=%u actionRepeats=%u maxByCount=%u userData=%p items=%d",
            baseResultCount,
            virtualSourceCount,
            actualDestinationCount,
            requestedCopies,
            directInventory ? "inventory" : "world",
            stackingApplied ? 1 : 0,
            perDestinationCount,
            destinationActionRepeats,
            maxCopiesByVirtualCount,
            reinterpret_cast<void*>(dupe4Result.mUserData),
            getShulkerContentEntryCount(dupe4Result)
        );
        resetNativeItemStack(cachedDyeShulkerIngredient);
        return true;
    }

    if (dupe3RichResult) {
        rewrittenDestinationSlots.clear();
        expectedDestinationCounts.fill(0);

        std::vector<int> dupe3InventorySlots;
        if (directInventory) {
            const std::vector<int> emptySlots =
                findEmptyInventorySlots(localPlayer);
            for (const int emptySlot : emptySlots) {
                if (dupe3InventorySlots.size() >=
                    static_cast<std::size_t>(dupe3EffectiveCopies)) {
                    break;
                }
                if (emptySlot < 0 ||
                    emptySlot >= kInventorySlotCount ||
                    reservedInventorySlots[emptySlot]) {
                    continue;
                }
                dupe3InventorySlots.push_back(emptySlot);
            }
        }
        const int cachedIngredientItemCount =
            shulkerIngredient && shulkerIngredient->isValid()
            ? getShulkerContentEntryCount(*shulkerIngredient)
            : 0;
        if (shulkerIngredient &&
            shulkerIngredient->isValid() &&
            shulkerIngredient->mUserData != 0 &&
            cachedIngredientItemCount > 0) {
            ItemStack richResult(*shulkerIngredient);
            richResult.item = craftedResult.item;
            richResult.mBlock = craftedResult.mBlock;
            richResult.mAuxValue = craftedResult.mAuxValue;
            richResult.mCount = craftedResult.mCount;
            richResult.mValid = craftedResult.mValid;
            richResult.mNetIdVariant = craftedResult.mNetIdVariant;

            const int richItemCount =
                getShulkerContentEntryCount(richResult);
            writeDiagnostic(
                "[DYE3_NBT_REBUILD] inputId=%d inputAux=%d outputId=%d outputAux=%d userData=%p items=%d",
                getItemId(*shulkerIngredient),
                static_cast<int>(shulkerIngredient->mAuxValue),
                getItemId(richResult),
                static_cast<int>(richResult.mAuxValue),
                reinterpret_cast<void*>(richResult.mUserData),
                richItemCount
            );

            const bool canNormalizeGenuinePair =
                selectedCandidate->actionGroup &&
                selectedCandidate->actionGroup->size() == 1 &&
                selectedCandidate->actionIndex == 0 &&
                selectedDestination->actionGroup &&
                selectedDestination->actionGroup->size() == 1 &&
                selectedDestination->actionIndex == 0;
            if (richItemCount > 0 && canNormalizeGenuinePair) {
                const bool sharedRewriteSucceeded = directInventory
                    ? normalizeAndAppendRichInventoryPairs(
                        selectedCandidate->actionGroup,
                        selectedDestination->actionGroup,
                        resultSource,
                        resultSlot,
                        destinationSource,
                        destinationSlot,
                        richResult,
                        empty,
                        dupe3InventorySlots,
                        dupe3Stacking,
                        dupe3ResultCount
                    )
                    : normalizeAndAppendRichDirectWorldDrops(
                        transaction,
                        selectedCandidate->actionGroup,
                        selectedDestination->actionGroup,
                        resultSource,
                        resultSlot,
                        destinationSource,
                        destinationSlot,
                        richResult,
                        empty,
                        dupe3EffectiveCopies,
                        false,
                        dupe3Stacking,
                        dupe3ResultCount
                    );
                if (!sharedRewriteSucceeded) {
                    writeDiagnostic(
                        "[DYE3_NBT_FALLBACK] shared rich %s rewrite failed",
                        directInventory ? "inventory" : "direct-world"
                    );
                }
                else {
                    writeDiagnostic(
                        "[DYE3_NBT_NORMALIZE] resultSlot=%u destinationSlot=%u items=%d",
                        resultSlot,
                        destinationSlot,
                        richItemCount
                    );

                    rememberExpectedResult(richResult);
                    if (directInventory) {
                        rewrittenDestinationSlots =
                            dupe3InventorySlots;
                        for (const int inventorySlot :
                        dupe3InventorySlots) {
                            expectedDestinationCounts[inventorySlot] =
                                static_cast<uint8_t>(dupe3ResultCount);
                        }
                        addedDestinationCount = static_cast<int>(
                            dupe3InventorySlots.size()
                            );
                        writeDiagnostic(
                            "[DYE3_NBT_INVENTORY] pairs=%d requested=%d stackCount=%d stacked=%d userData=%p items=%d ingredientSource=%s worldActions=0 nativeDescriptors=1",
                            addedDestinationCount,
                            requestedCopies,
                            dupe3ResultCount,
                            dupe3Stacking ? 1 : 0,
                            reinterpret_cast<void*>(richResult.mUserData),
                            richItemCount,
                            shulkerIngredientFromCache
                            ? "cache"
                            : "packet"
                        );
                    }
                    else {
                        addedDestinationCount = dupe3EffectiveCopies;
                        writeDiagnostic(
                            "[DYE3_NBT_DIRECT_WORLD] pairs=%d requested=%d stackCount=%d stacked=%d userData=%p items=%d ingredientSource=%s nativeDescriptors=1",
                            dupe3EffectiveCopies,
                            requestedCopies,
                            dupe3ResultCount,
                            dupe3Stacking ? 1 : 0,
                            reinterpret_cast<void*>(richResult.mUserData),
                            richItemCount,
                            shulkerIngredientFromCache
                            ? "cache"
                            : "packet"
                        );
                        if (activeCraftDropStack) {
                            writeDiagnostic(
                                "[CRAFT_DROP_STACK_SEND] resultSources=%d worldTargetCount=%d worldActions=1 resultId=%d richNBT=1",
                                dupe3ActionRepeats,
                                dupe3ResultCount,
                                getItemId(richResult)
                            );
                        }
                    }
                    resetNativeItemStack(cachedDyeShulkerIngredient);
                    return true;
                }
            }
            if (richItemCount <= 0) {
                writeDiagnostic(
                    "[DYE3_NBT_FALLBACK] rebuilt result lost Items; using original native-clone path"
                );
            }
            else if (!canNormalizeGenuinePair) {
                writeDiagnostic(
                    "[DYE3_NBT_FALLBACK] genuine action groups were ambiguous resultSize=%llu resultIndex=%llu destinationSize=%llu destinationIndex=%llu",
                    static_cast<unsigned long long>(
                        selectedCandidate->actionGroup
                        ? selectedCandidate->actionGroup->size()
                        : 0
                        ),
                    static_cast<unsigned long long>(
                        selectedCandidate->actionIndex
                        ),
                    static_cast<unsigned long long>(
                        selectedDestination->actionGroup
                        ? selectedDestination->actionGroup->size()
                        : 0
                        ),
                    static_cast<unsigned long long>(
                        selectedDestination->actionIndex
                        )
                );
            }
        }
        else {
            writeDiagnostic(
                "[DYE3_NBT_FALLBACK] no filled ingredient NBT was captured; using original native-clone path inputItems=%d",
                cachedIngredientItemCount
            );
        }

        rememberExpectedResult(craftedResult);

        if (directInventory) {
            rewrittenDestinationSlots = dupe3InventorySlots;
            for (const int inventorySlot : dupe3InventorySlots) {
                expectedDestinationCounts[inventorySlot] =
                    static_cast<uint8_t>(dupe3ResultCount);
            }

            for (std::size_t copyIndex = 0;
                copyIndex < dupe3InventorySlots.size();
                ++copyIndex) {
                for (int repeatIndex = 0;
                    repeatIndex < dupe3ActionRepeats;
                    ++repeatIndex) {
                    if (!appendNativeActionClone(
                        transaction,
                        resultSource,
                        resultActionIndex,
                        resultSource,
                        resultSlot
                    ) ||
                        !appendNativeActionClone(
                            transaction,
                            destinationSource,
                            destinationActionIndex,
                            destinationSource,
                            static_cast<uint32_t>(
                                dupe3InventorySlots[copyIndex]
                                )
                        )) {
                        writeDiagnostic(
                            "[DYE3_NATIVE_INVENTORY_ABORT] copyIndex=%llu repeat=%d native addAction failed",
                            static_cast<unsigned long long>(copyIndex),
                            repeatIndex
                        );
                        return false;
                    }
                }
            }

            addedDestinationCount = static_cast<int>(
                dupe3InventorySlots.size()
                );
            writeDiagnostic(
                "[DYE3_NATIVE_INVENTORY] pairs=%d requested=%d stackCount=%d stacked=%d richNBT=0 worldActions=0 resultDesc=%016llX destinationDesc=%016llX",
                addedDestinationCount,
                requestedCopies,
                dupe3ResultCount,
                dupe3Stacking ? 1 : 0,
                static_cast<unsigned long long>(
                    resultFromDescriptorHash
                    ),
                static_cast<unsigned long long>(
                    destinationToDescriptorHash
                    )
            );
            resetNativeItemStack(cachedDyeShulkerIngredient);
            return true;
        }

        InventorySource worldSource;
        worldSource.type = InventorySourceType::WorldInteraction;
        worldSource.containerId = ContainerID::None;
        worldSource.flags = InventorySourceFlags::NoFlag;

        for (int copyIndex = 0;
            copyIndex < dupe3EffectiveCopies;
            ++copyIndex) {
            for (int repeatIndex = 0;
                repeatIndex < dupe3ActionRepeats;
                ++repeatIndex) {
                if (!appendNativeActionClone(
                    transaction,
                    resultSource,
                    resultActionIndex,
                    resultSource,
                    resultSlot
                )) {
                    writeDiagnostic(
                        "[DYE_DIRECT_WORLD_ABORT] copyIndex=%d repeat=%d native result addAction failed",
                        copyIndex,
                        repeatIndex
                    );
                    return false;
                }
            }

            if (dupe3Stacking) {
                ItemStack stackedWorldResult(craftedResult);
                NativeItemStackResetGuard stackedWorldResultGuard(
                    &stackedWorldResult
                );
                stackedWorldResult.mCount = static_cast<uint8_t>(
                    dupe3ResultCount
                    );
                InventoryAction stackedDirectDrop(
                    0,
                    empty,
                    &stackedWorldResult,
                    ContainerID::None,
                    InventorySourceType::WorldInteraction,
                    InventorySourceFlags::NoFlag
                );
                transaction->data.addAction(stackedDirectDrop);
            }
            else if (!appendNativeActionClone(
                transaction,
                destinationSource,
                destinationActionIndex,
                worldSource,
                0
            )) {
                writeDiagnostic(
                    "[DYE_DIRECT_WORLD_ABORT] copyIndex=%d native destination addAction failed",
                    copyIndex
                );
                return false;
            }
        }

        addedDestinationCount = dupe3EffectiveCopies;
        writeDiagnostic(
            "[DYE_DIRECT_WORLD] pairs=%d requested=%d stackCount=%d stacked=%d worldSlot=0 resultDesc=%016llX destinationDesc=%016llX sourceUserData=%p nativeAddAction=1 namedDyeMarker=%d",
            dupe3EffectiveCopies,
            requestedCopies,
            dupe3ResultCount,
            dupe3Stacking ? 1 : 0,
            static_cast<unsigned long long>(resultFromDescriptorHash),
            static_cast<unsigned long long>(
                destinationToDescriptorHash
                ),
            reinterpret_cast<void*>(resultUserData),
            sawNamedDyeIngredient ? 1 : 0
        );
        if (activeCraftDropStack) {
            writeDiagnostic(
                "[CRAFT_DROP_STACK_SEND] resultSources=%d worldTargetCount=%d worldActions=1 resultId=%d",
                dupe3ActionRepeats,
                dupe3ResultCount,
                getItemId(craftedResult)
            );
        }
        resetNativeItemStack(cachedDyeShulkerIngredient);
        return true;
    }

    std::vector<int> destinationSlots;
    for (int slot = kMainInventoryFirstSlot;
        slot < kInventorySlotCount &&
        destinationSlots.size() <
        static_cast<std::size_t>(requestedCopies);
        ++slot) {
        ItemStack* current =
            localPlayer->playerInventory->container->getItem(slot);
        if ((current && current->isValid()) ||
            reservedInventorySlots[slot]) {
            continue;
        }
        destinationSlots.push_back(slot);
    }

    const std::vector<uint8_t> destinationCounts =
        planDestinationCounts(
            destinationSlots.size(),
            craftedResult.mCount,
            activeStacking,
            activeStackCount
        );
    const bool stackingApplied =
        activeStacking &&
        activeStackCount > static_cast<int>(craftedResult.mCount);

    rewrittenDestinationSlots = destinationSlots;
    expectedDestinationCounts.fill(0);
    for (std::size_t index = 0;
        index < destinationSlots.size();
        ++index) {
        expectedDestinationCounts[destinationSlots[index]] =
            destinationCounts[index];
    }
    rememberExpectedResult(craftedResult);
    if (destinationSlots.empty()) {
        return true;
    }

    std::size_t totalActionRepeats = 0;
    for (const uint8_t destinationCount : destinationCounts) {
        totalActionRepeats += stackingApplied
            ? static_cast<std::size_t>(destinationCount / craftedResult.mCount)
            : 1;
    }

    std::vector<InventoryAction>* resultActions =
        selectedCandidate->actionGroup;
    if (!preserveOriginalDescriptors) {
        resultActions->reserve(
            resultActions->size() + totalActionRepeats
        );
    }

    unsigned int repeatedCraftResults = 0;
    unsigned int duplicatedItemCount = 0;
    unsigned int stackedDestinationCount = 0;
    for (std::size_t index = 0;
        index < destinationSlots.size();
        ++index) {
        const unsigned int finalCount = destinationCounts[index];
        const unsigned int actionRepeats =
            stackingApplied ? finalCount / craftedResult.mCount : 1;
        duplicatedItemCount += finalCount;
        if (finalCount > craftedResult.mCount) {
            ++stackedDestinationCount;
        }

        for (unsigned int repeat = 0;
            repeat < actionRepeats;
            ++repeat) {
            if (preserveOriginalDescriptors) {
                if (!appendNativeActionClone(
                    transaction,
                    resultSource,
                    resultActionIndex,
                    resultSource,
                    resultSlot
                )) {
                    writeDiagnostic(
                        "[DYE_NATIVE_RESULT_ABORT] repeat=%u native addAction failed",
                        repeatedCraftResults
                    );
                    return false;
                }
            }
            else {
                resultActions->emplace_back(
                    resultSlot,
                    &craftedResult,
                    empty,
                    resultSource.containerId,
                    resultSource.type,
                    resultSource.flags
                );
            }
            ++repeatedCraftResults;
        }
    }

    writeDiagnostic(
        "[DYE_BALANCE] strategy=%s repeatedCraftResults=%u destinations=%llu items=%u stackedDestinations=%u requested=%d stackCount=%d namedDyeMarker=%d",
        strategyName,
        repeatedCraftResults,
        static_cast<unsigned long long>(destinationSlots.size()),
        duplicatedItemCount,
        stackedDestinationCount,
        requestedCopies,
        activeStackCount,
        sawNamedDyeIngredient ? 1 : 0
    );

    auto inventoryGroup =
        transaction->data.actions.find(destinationSource);
    if (inventoryGroup == transaction->data.actions.end()) {
        return false;
    }
    auto& inventoryActions = inventoryGroup->second;
    if (!preserveOriginalDescriptors) {
        inventoryActions.reserve(
            inventoryActions.size() + totalActionRepeats
        );
    }
    for (std::size_t index = 0;
        index < destinationSlots.size();
        ++index) {
        const unsigned int actionRepeats = stackingApplied
            ? static_cast<unsigned int>(destinationCounts[index])
            : 1;
        for (unsigned int repeat = 0;
            repeat < actionRepeats;
            ++repeat) {
            if (preserveOriginalDescriptors) {
                if (!appendNativeActionClone(
                    transaction,
                    destinationSource,
                    destinationActionIndex,
                    destinationSource,
                    destinationSlots[index]
                )) {
                    writeDiagnostic(
                        "[DYE_NATIVE_DESTINATION_ABORT] slot=%d repeat=%u native addAction failed",
                        destinationSlots[index],
                        repeat
                    );
                    return false;
                }
            }
            else {
                inventoryActions.emplace_back(
                    destinationSlots[index],
                    empty,
                    &craftedResult,
                    destinationSource.containerId,
                    destinationSource.type,
                    destinationSource.flags
                );
            }
        }
        ++addedDestinationCount;
    }

    return true;
}

bool Dupe::rewriteNestedAnvilTransaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    addedDestinationCount = 0;
    nestedRewriteMatchedAndRejected = false;

    LocalPlayer* localPlayer = Game::getLocalPlayer();
    if (!packet ||
        !localPlayer ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container) {
        return false;
    }

    ComplexInventoryTransaction* transaction =
        getValidatedTransaction(packet);
    if (!transaction ||
        !isReadableRange(transaction, sizeof(*transaction)) ||
        transaction->type !=
        ComplexInventoryTransaction::Type::NormalTransaction) {
        return false;
    }

    struct AnvilTakeCandidate {
        InventoryAction* action;
        std::vector<InventoryAction>* actionGroup;
        std::size_t actionIndex;
        InventorySource source;
    };
    struct InventoryDestinationCandidate {
        InventoryAction* action;
        std::vector<InventoryAction>* actionGroup;
        std::size_t actionIndex;
        InventorySource source;
    };

    std::vector<AnvilTakeCandidate> anvilCandidates;
    std::vector<InventoryDestinationCandidate> inventoryDestinations;
    InventoryAction* anvilInputBalanceAction = nullptr;
    bool ambiguousAnvilInputBalanceAction = false;
    std::array<bool, kInventorySlotCount> reservedInventorySlots{};

    for (auto& [source, actions] : transaction->data.actions) {
        const int sourceContainer =
            static_cast<int>(source.containerId);
        for (std::size_t actionIndex = 0;
            actionIndex < actions.size();
            ++actionIndex) {
            InventoryAction& action = actions[actionIndex];
            const bool fromHasItem =
                action.fromItem.isValid() &&
                action.fromItem.mCount > 0;
            const bool toHasItem =
                action.toItem.isValid() &&
                action.toItem.mCount > 0;

            if (source.type ==
                InventorySourceType::NonImplementedFeatureTODO &&
                sourceContainer == kAnvilInputWindowId &&
                toHasItem) {
                if (anvilInputBalanceAction) {
                    ambiguousAnvilInputBalanceAction = true;
                }
                else {
                    anvilInputBalanceAction = &action;
                }
            }

            if (source.type ==
                InventorySourceType::ContainerInventory &&
                source.containerId == ContainerID::Inventory &&
                action.slot < kInventorySlotCount) {
                if (!fromHasItem &&
                    toHasItem &&
                    isShulkerBox(action.toItem)) {
                    inventoryDestinations.push_back({
                        &action,
                        &actions,
                        actionIndex,
                        source
                        });
                }
            }

            if (source.type !=
                InventorySourceType::NonImplementedFeatureTODO ||
                sourceContainer != kAnvilResultWindowId ||
                !fromHasItem ||
                toHasItem ||
                !isShulkerBox(action.fromItem)) {
                continue;
            }

            anvilCandidates.push_back({
                &action,
                &actions,
                actionIndex,
                source
                });
        }
    }

    const AnvilTakeCandidate* selectedCandidate = nullptr;
    const InventoryDestinationCandidate* selectedDestination = nullptr;
    int matchingPairCount = 0;
    for (const AnvilTakeCandidate& candidate : anvilCandidates) {
        for (const InventoryDestinationCandidate& destination :
            inventoryDestinations) {
            if (destination.action &&
                destination.action->toItem.isValid() &&
                destination.action->toItem.mCount ==
                candidate.action->fromItem.mCount &&
                destination.action->toItem.mAuxValue ==
                candidate.action->fromItem.mAuxValue &&
                destination.action->toItem.item.get() ==
                candidate.action->fromItem.item.get()) {
                ++matchingPairCount;
                if (matchingPairCount == 1) {
                    selectedCandidate = &candidate;
                    selectedDestination = &destination;
                }
            }
        }
    }

    if (!selectedCandidate ||
        !selectedDestination ||
        !selectedDestination->action) {
        return false;
    }

    nestedRewriteMatchedAndRejected = true;

    if (matchingPairCount != 1) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] ambiguous genuine result/destination pairs=%d",
            matchingPairCount
        );
        return false;
    }

    if (!anvilInputBalanceAction ||
        ambiguousAnvilInputBalanceAction ||
        anvilInputBalanceAction->fromItem.isValid() ||
        !anvilInputBalanceAction->toItem.isValid()) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] invalid -10 carrier inputAction=%p ambiguous=%d fromValid=%d toValid=%d",
            reinterpret_cast<void*>(anvilInputBalanceAction),
            ambiguousAnvilInputBalanceAction ? 1 : 0,
            anvilInputBalanceAction &&
            anvilInputBalanceAction->fromItem.isValid()
            ? 1
            : 0,
            anvilInputBalanceAction &&
            anvilInputBalanceAction->toItem.isValid()
            ? 1
            : 0
        );
        return false;
    }

    if (!isExecutableAddress(Addresses::ItemStackBase_save) ||
        !isExecutableAddress(Addresses::CompoundTag_putByte) ||
        !isExecutableAddress(Addresses::ItemStack_loadItem)) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] native NBT functions unavailable save=%p putByte=%p load=%p",
            reinterpret_cast<void*>(Addresses::ItemStackBase_save),
            reinterpret_cast<void*>(Addresses::CompoundTag_putByte),
            reinterpret_cast<void*>(Addresses::ItemStack_loadItem)
        );
        return false;
    }

    if (!captureNestedSourceBox(localPlayer) ||
        !cachedNestedSourceBox ||
        !cachedNestedSourceBox->isValid() ||
        !isShulkerBox(*cachedNestedSourceBox) ||
        cachedNestedSourceBox->mUserData == 0) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] hotbar slot %d does not contain a complete source shulker A",
            activeNestedSourceSlot
        );
        return false;
    }

    ItemStack renamedIdentity(selectedDestination->action->toItem);
    NativeItemStackResetGuard renamedIdentityGuard(&renamedIdentity);
    const unsigned int baseCount =
        static_cast<unsigned int>(renamedIdentity.mCount);
    if (!renamedIdentity.isValid() ||
        baseCount == 0 ||
        baseCount > kMaxTransactionItemCount) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] invalid genuine renamed B count=%u",
            baseCount
        );
        return false;
    }

    ItemStack sourceA(*cachedNestedSourceBox);
    NativeItemStackResetGuard sourceAGuard(&sourceA);
    sourceA.mCount = 1;
    const int sourceAItemCount =
        getShulkerContentEntryCount(sourceA);

    ItemStack nestedResult(renamedIdentity);
    NativeItemStackResetGuard nestedResultGuard(&nestedResult);
    auto* bRoot = reinterpret_cast<CompoundTag*>(
        nestedResult.mUserData
        );
    if (!bRoot) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] genuine renamed B has no native root"
        );
        return false;
    }

    MutableShulkerItemsEntry bItemsEntry;
    ItemStack itemsDonor(sourceA);
    NativeItemStackResetGuard itemsDonorGuard(&itemsDonor);
    if (!findMutableShulkerItemsEntry(bRoot, bItemsEntry)) {
        auto* donorRoot = reinterpret_cast<CompoundTag*>(
            itemsDonor.mUserData
            );
        MutableShulkerItemsEntry donorItemsEntry;
        if (!findMutableShulkerItemsEntry(
            donorRoot,
            donorItemsEntry
        )) {
            writeDiagnostic(
                "[NESTED_ANVIL_ABORT] neither B nor A supplies a native Items list"
            );
            return false;
        }

        TagMap::node_type itemsNode =
            donorItemsEntry.parent->mTags.extract(
                donorItemsEntry.iterator
            );
        if (itemsNode.empty()) {
            writeDiagnostic(
                "[NESTED_ANVIL_ABORT] failed to detach native Items list"
            );
            return false;
        }
        bRoot->mTags.erase("Items");
        bRoot->mTags.erase("items");
        bRoot->mTags.erase("__Legacy__Items");
        if (!bRoot->mTags.insert(std::move(itemsNode)).inserted ||
            !findMutableShulkerItemsEntry(bRoot, bItemsEntry)) {
            writeDiagnostic(
                "[NESTED_ANVIL_ABORT] failed to attach native Items list to B"
            );
            return false;
        }
    }

    ListTag* outerItems =
        bItemsEntry.iterator->second.get<ListTag>();
    if (!outerItems) {
        return false;
    }

    NbtComplexity sourceComplexity;
    constexpr uint64_t kMaxDescriptorBytes =
        16ULL * 1024ULL * 1024ULL;
    constexpr uint64_t kMaxDescriptorNodes = 250000ULL;
    if (!inspectItemNbt(sourceA, sourceComplexity)) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] A NBT complexity could not be inspected"
        );
        return false;
    }

    if (sourceComplexity.bytes > kMaxDescriptorBytes ||
        sourceComplexity.nodes > kMaxDescriptorNodes) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] A exceeds safety budget bytes=%llu/%llu nodes=%llu/%llu depth=%d",
            static_cast<unsigned long long>(sourceComplexity.bytes),
            static_cast<unsigned long long>(kMaxDescriptorBytes),
            static_cast<unsigned long long>(sourceComplexity.nodes),
            static_cast<unsigned long long>(kMaxDescriptorNodes),
            sourceComplexity.maxDepth + 4
        );
        return false;
    }

    outerItems->mList.clear();
    outerItems->mType = Tag::Type::Compound;
    outerItems->mList.reserve(1);
    std::unique_ptr<CompoundTag> nestedA =
        saveNativeItemEntry(sourceA);
    if (!nestedA || !putNativeByte(nestedA.get(), "Slot", 0)) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] failed to serialize complete A into B Slot=0"
        );
        return false;
    }
    outerItems->mList.emplace_back(std::move(nestedA));

    const int rebuiltOuterItems =
        getShulkerContentEntryCount(nestedResult);
    if (rebuiltOuterItems != 1) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] rebuilt B Items mismatch actual=%d expected=1",
            rebuiltOuterItems
        );
        return false;
    }

    NbtComplexity rebuiltComplexity;
    if (!inspectItemNbt(nestedResult, rebuiltComplexity) ||
        rebuiltComplexity.bytes == 0 ||
        rebuiltComplexity.nodes == 0 ||
        rebuiltComplexity.bytes > kMaxDescriptorBytes ||
        rebuiltComplexity.nodes > kMaxDescriptorNodes) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] rebuilt B complexity invalid bytes=%llu nodes=%llu depth=%d",
            static_cast<unsigned long long>(rebuiltComplexity.bytes),
            static_cast<unsigned long long>(rebuiltComplexity.nodes),
            rebuiltComplexity.maxDepth
        );
        return false;
    }

    std::unique_ptr<CompoundTag> persistedNestedResult =
        saveNativeItemEntry(nestedResult);
    if (!persistedNestedResult) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] rebuilt B could not be persisted"
        );
        return false;
    }

    ItemStack nativeLoadedResult;
    NativeItemStackResetGuard nativeLoadedResultGuard(
        &nativeLoadedResult
    );
    dupeLoadItem(nativeLoadedResult, *persistedNestedResult);
    nativeLoadedResult.mCount = renamedIdentity.mCount;
    nativeLoadedResult.mNetIdVariant = renamedIdentity.mNetIdVariant;
    if (!nativeLoadedResult.isValid() ||
        nativeLoadedResult.item.get() != renamedIdentity.item.get() ||
        nativeLoadedResult.mAuxValue != renamedIdentity.mAuxValue ||
        nativeLoadedResult.mCount != renamedIdentity.mCount ||
        getShulkerContentEntryCount(nativeLoadedResult) != 1) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] native reload changed B identity/count/items id=%d aux=%d count=%u items=%d",
            getItemId(nativeLoadedResult),
            static_cast<int>(nativeLoadedResult.mAuxValue),
            static_cast<unsigned int>(nativeLoadedResult.mCount),
            getShulkerContentEntryCount(nativeLoadedResult)
        );
        return false;
    }

    ItemStack virtualSource(nativeLoadedResult);
    NativeItemStackResetGuard virtualSourceGuard(&virtualSource);
    virtualSource.mCount = 1;
    new (&anvilInputBalanceAction->fromItem) ItemStack(virtualSource);
    anvilInputBalanceAction->fromItemDescriptor =
        NetworkItemStackDescriptor(
            &anvilInputBalanceAction->fromItem
        );

    const unsigned int genuineResultCount =
        static_cast<unsigned int>(
            selectedCandidate->action->fromItem.mCount
            );
    const int genuineResultItems =
        getShulkerContentEntryCount(
            selectedCandidate->action->fromItem
        );
    const int nestedDestinationSlot =
        static_cast<int>(selectedDestination->action->slot);
    selectedDestination->action->toItem.setNull(std::nullopt);
    new (&selectedDestination->action->toItem)
        ItemStack(virtualSource);
    selectedDestination->action->toItemDescriptor =
        NetworkItemStackDescriptor(
            &selectedDestination->action->toItem
        );

    InventoryAction& nestedDestinationAction =
        *selectedDestination->action;
    const DescriptorPayloadSummary virtualWire =
        inspectDescriptorPayload(
            anvilInputBalanceAction->fromItemDescriptor
        );
    const DescriptorPayloadSummary destinationWire =
        inspectDescriptorPayload(
            nestedDestinationAction.toItemDescriptor
        );
    const bool virtualContainsItems =
        virtualWire.containsItems ||
        virtualWire.containsLegacyItems;
    const bool destinationContainsItems =
        destinationWire.containsItems ||
        destinationWire.containsLegacyItems;
    const bool descriptorPayloadsMatch =
        virtualWire.readable &&
        destinationWire.readable &&
        virtualContainsItems &&
        destinationContainsItems &&
        virtualWire.size == destinationWire.size &&
        virtualWire.hash == destinationWire.hash &&
        virtualWire.hasNetId == destinationWire.hasNetId &&
        virtualWire.blockRuntimeId ==
        destinationWire.blockRuntimeId;
    if (!descriptorPayloadsMatch ||
        getShulkerContentEntryCount(
            anvilInputBalanceAction->fromItem
        ) != 1 ||
        getShulkerContentEntryCount(
            nestedDestinationAction.toItem
        ) != 1) {
        writeDiagnostic(
            "[NESTED_ANVIL_ABORT] -10.old/destination descriptor mismatch virtualReadable=%d virtualSize=%llu virtualHash=%llX virtualItems=%d virtualLegacy=%d virtualHasNet=%u virtualRuntime=%d destinationReadable=%d destinationSize=%llu destinationHash=%llX destinationItems=%d destinationLegacy=%d destinationHasNet=%u destinationRuntime=%d",
            virtualWire.readable ? 1 : 0,
            static_cast<unsigned long long>(virtualWire.size),
            static_cast<unsigned long long>(virtualWire.hash),
            virtualWire.containsItems ? 1 : 0,
            virtualWire.containsLegacyItems ? 1 : 0,
            static_cast<unsigned int>(virtualWire.hasNetId),
            virtualWire.blockRuntimeId,
            destinationWire.readable ? 1 : 0,
            static_cast<unsigned long long>(destinationWire.size),
            static_cast<unsigned long long>(destinationWire.hash),
            destinationWire.containsItems ? 1 : 0,
            destinationWire.containsLegacyItems ? 1 : 0,
            static_cast<unsigned int>(destinationWire.hasNetId),
            destinationWire.blockRuntimeId
        );
        return false;
    }

    rewrittenDestinationSlots = { nestedDestinationSlot };
    expectedDestinationCounts.fill(0);
    expectedDestinationCounts[nestedDestinationSlot] = 1;
    rememberExpectedResult(nestedDestinationAction.toItem);

    addedDestinationCount = 1;
    writeDiagnostic(
        "[NESTED_ANVIL_-10_SWAP] aHotbar=%d aEntries=%d bEntries=1 genuineResult=-12 genuineCount=%u genuineItems=%d virtualSource=-10.old virtualCount=1 genuineDestinationSlot=%d actionsAdded=0 outerRoot=B innerRoot=A",
        activeNestedSourceSlot,
        sourceAItemCount,
        genuineResultCount,
        genuineResultItems,
        nestedDestinationSlot
    );
    writeDiagnostic(
        "[NESTED_ANVIL_DESCRIPTOR_MATCH] count=1 bytes=%llu hash=%llX nodes=%llu depth=%d worldActions=0 localPrediction=0",
        static_cast<unsigned long long>(virtualWire.size),
        static_cast<unsigned long long>(virtualWire.hash),
        static_cast<unsigned long long>(rebuiltComplexity.nodes),
        rebuiltComplexity.maxDepth
    );

    resetNativeItemStack(cachedDyeShulkerIngredient);
    resetNativeItemStack(cachedNestedSourceBox);
    nestedRewriteMatchedAndRejected = false;
    return true;
}

bool Dupe::rewriteAnvilTransaction(
    InventoryTransactionPacket* packet,
    int& addedDestinationCount
) {
    addedDestinationCount = 0;

    LocalPlayer* localPlayer = Game::getLocalPlayer();
    if (!packet ||
        !localPlayer ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container) {
        return false;
    }

    ComplexInventoryTransaction* transaction =
        getValidatedTransaction(packet);
    if (!transaction ||
        !isReadableRange(transaction, sizeof(*transaction)) ||
        transaction->type !=
        ComplexInventoryTransaction::Type::NormalTransaction) {
        return false;
    }

    struct AnvilTakeCandidate {
        InventoryAction* action;
        std::vector<InventoryAction>* actionGroup;
        std::size_t actionIndex;
        InventorySource source;
        int score;
    };
    struct InventoryDestinationCandidate {
        InventoryAction* action;
        InventorySource source;
        std::size_t actionIndex;
    };
    std::vector<AnvilTakeCandidate> anvilCandidates;
    std::vector<InventoryDestinationCandidate> inventoryDestinations;
    InventoryAction* anvilInputBalanceAction = nullptr;
    bool ambiguousAnvilInputBalanceAction = false;
    InventoryAction* genuineAnvilSourceAction = nullptr;
    int genuineAnvilSourceContainer = 0;
    bool ambiguousGenuineAnvilSourceAction = false;

    std::array<bool, kInventorySlotCount> reservedInventorySlots{};

    for (auto& [source, actions] :
        transaction->data.actions) {
        const int sourceContainer =
            static_cast<int>(source.containerId);

        for (std::size_t actionIndex = 0;
            actionIndex < actions.size();
            ++actionIndex) {
            InventoryAction& action = actions[actionIndex];
            const bool fromHasItem =
                action.fromItem.isValid() &&
                action.fromItem.mCount > 0;
            const bool toHasItem =
                action.toItem.isValid() &&
                action.toItem.mCount > 0;

            if (source.type ==
                InventorySourceType::NonImplementedFeatureTODO &&
                sourceContainer == kAnvilInputWindowId &&
                toHasItem) {
                if (anvilInputBalanceAction) {
                    ambiguousAnvilInputBalanceAction = true;
                }
                else {
                    anvilInputBalanceAction = &action;
                }
            }

            if (source.type ==
                InventorySourceType::ContainerInventory &&
                source.containerId != ContainerID::Inventory &&
                fromHasItem && !toHasItem &&
                isShulkerBox(action.fromItem) &&
                getShulkerContentEntryCount(action.fromItem) > 0) {
                if (genuineAnvilSourceAction) {
                    ambiguousGenuineAnvilSourceAction = true;
                }
                else {
                    genuineAnvilSourceAction = &action;
                    genuineAnvilSourceContainer = sourceContainer;
                }
            }

            if (source.type ==
                InventorySourceType::ContainerInventory &&
                source.containerId == ContainerID::Inventory &&
                action.slot < kInventorySlotCount &&
                !fromHasItem &&
                toHasItem) {
                reservedInventorySlots[action.slot] = true;
                inventoryDestinations.push_back({
                    &action,
                    source,
                    actionIndex
                    });
            }

            if (source.type !=
                InventorySourceType::NonImplementedFeatureTODO ||
                (sourceContainer != kAnvilResultWindowId &&
                    sourceContainer != kAnvilOutputWindowId) ||
                !fromHasItem ||
                toHasItem) {
                continue;
            }

            anvilCandidates.push_back({
                &action,
                &actions,
                actionIndex,
                source,
                sourceContainer == kAnvilResultWindowId ? 2 : 1
                });
        }
    }

    const AnvilTakeCandidate* selectedCandidate = nullptr;
    const InventoryDestinationCandidate* selectedDestination = nullptr;
    for (const AnvilTakeCandidate& candidate : anvilCandidates) {
        const ItemStack& result = candidate.action->fromItem;
        const InventoryDestinationCandidate* matchingDestination = nullptr;
        for (const InventoryDestinationCandidate& destination :
            inventoryDestinations) {
            if (destination.action &&
                destination.action->toItem.isValid() &&
                destination.action->toItem.mCount == result.mCount &&
                destination.action->toItem.mAuxValue == result.mAuxValue &&
                destination.action->toItem.item.get() ==
                result.item.get()) {
                matchingDestination = &destination;
                break;
            }
        }

        if (matchingDestination &&
            (!selectedCandidate ||
                candidate.score > selectedCandidate->score)) {
            selectedCandidate = &candidate;
            selectedDestination = matchingDestination;
        }
    }

    if (!selectedCandidate ||
        !selectedDestination ||
        !selectedDestination->action) {
        return false;
    }

    if ((activeMode == 5 || activeMode == 8) &&
        static_cast<int8_t>(selectedCandidate->source.containerId) !=
        kAnvilResultWindowId) {
        writeDiagnostic(
            "[ANVILDUPE2_ABORT] expected the genuine -12 result source, got container=%d",
            static_cast<int>(
                static_cast<int8_t>(
                    selectedCandidate->source.containerId
                    )
                )
        );
        return false;
    }

    std::unique_ptr<ItemStack> renamedResult =
        std::make_unique<ItemStack>(
            selectedDestination->action->toItem
        );
    if (!renamedResult->isValid()) {
        return false;
    }

    ItemStack* empty = ItemStack::getEmptyItem();
    if (!empty) {
        return false;
    }

    const int requestedCopies = aetherAnvilCarrierActive
        ? std::max(activeCopyCount - 1, 0)
        : activeCopyCount;
    const unsigned int baseCount =
        static_cast<unsigned int>(renamedResult->mCount);
    if (baseCount == 0) {
        return false;
    }

    if (activeMode == 5 || activeMode == 8) {
        if (!anvilInputBalanceAction ||
            ambiguousAnvilInputBalanceAction ||
            anvilInputBalanceAction->fromItem.isValid() ||
            !anvilInputBalanceAction->toItem.isValid()) {
            writeDiagnostic(
                "[ANVIL_SWAP_ABORT] inputAction=%p ambiguous=%d fromValid=%d toValid=%d",
                reinterpret_cast<void*>(anvilInputBalanceAction),
                ambiguousAnvilInputBalanceAction ? 1 : 0,
                anvilInputBalanceAction &&
                anvilInputBalanceAction->fromItem.isValid()
                ? 1
                : 0,
                anvilInputBalanceAction &&
                anvilInputBalanceAction->toItem.isValid()
                ? 1
                : 0
            );
            return false;
        }

        std::unique_ptr<ItemStack> swapItem;
        if (activeMode == 8) {
            ItemStack* sourceA =
                localPlayer->playerInventory->container->getItem(
                    activeAnvilSwapSourceSlot
                );
            if (!sourceA || !sourceA->isValid() || sourceA->mCount == 0) {
                writeDiagnostic(
                    "[ANVIL_SWAP_ABORT] A hotbar slot=%d is empty or invalid",
                    activeAnvilSwapSourceSlot
                );
                return false;
            }
            swapItem = std::make_unique<ItemStack>(*sourceA);
            swapItem->mCount = 1;
        }
        else {
            const ItemStack& copySource =
                activeAnvilDupe2SingleItem ||
                activeAnvilDupe2MaterialSlotMode
                ? selectedCandidate->action->fromItem
                : *renamedResult;
            swapItem = std::make_unique<ItemStack>(copySource);
        }

        bool usingLoadedNbt = false;
        if (activeMode == 5 && activeAnvilDupe2SingleItem &&
            isShulkerBox(*swapItem)) {
            const int experimentMode =
                activeAnvilDupe2SingleNbtExperiment;
            const bool loadedTagAvailable = NbtCommand::hasLoadedTag();
            const int targetBeforeItems =
                getShulkerContentEntryCount(*swapItem);
            std::size_t originalItems =
                static_cast<std::size_t>(
                    std::max(targetBeforeItems, 0)
                    );
            std::size_t donorItems = 0;
            std::size_t changedLists = 0;
            std::string changedField = "none";
            std::string beforeValue = "unchanged";
            std::string afterValue = "unchanged";
            auto* generatedRoot = reinterpret_cast<CompoundTag*>(
                swapItem->mUserData
                );

            if (experimentMode == 1 || experimentMode == 2) {
                if (!loadedTagAvailable) {
                    writeDiagnostic(
                        "[ANVILDUPE2_SINGLE_ITEM_ABORT] test=%s requires an armed .nbt file",
                        getSingleNbtExperimentName(experimentMode)
                    );
                    return false;
                }

                std::unique_ptr<CompoundTag> loadedNbt =
                    NbtCommand::copyLoadedTag();
                FirstShulkerItemFieldCopy fieldCopy;
                static constexpr std::array<const char*, 2>
                    kCountKeys = { "Count", "count" };
                const bool copied = loadedNbt && generatedRoot &&
                    (experimentMode == 1
                        ? copyFirstShulkerItemField(
                            generatedRoot,
                            loadedNbt.get(),
                            kCountKeys,
                            fieldCopy
                        )
                        : copyFirstShulkerItemIdentity(
                            generatedRoot,
                            loadedNbt.get(),
                            fieldCopy
                        ));
                if (!copied) {
                    writeDiagnostic(
                        "[ANVILDUPE2_SINGLE_ITEM_ABORT] test=%s could not translate the selected Items[0] field into every native target representation",
                        getSingleNbtExperimentName(experimentMode)
                    );
                    return false;
                }

                usingLoadedNbt = true;
                originalItems = fieldCopy.donorItemCount;
                donorItems = fieldCopy.donorItemCount;
                changedLists = fieldCopy.changedListCount;
                changedField = fieldCopy.key;
                beforeValue = fieldCopy.beforeValue;
                afterValue = fieldCopy.afterValue;
            }
            else if (experimentMode == 3) {
                usingLoadedNbt = loadedTagAvailable;
                if (usingLoadedNbt) {
                    std::unique_ptr<CompoundTag> loadedNbt =
                        NbtCommand::copyLoadedTag();
                    const ListTag* loadedItems = loadedNbt
                        ? findShulkerItemsList(loadedNbt.get())
                        : nullptr;
                    if (!loadedNbt || !loadedItems ||
                        loadedItems->mType != Tag::Compound) {
                        writeDiagnostic(
                            "[ANVILDUPE2_SINGLE_ITEM_ABORT] test=%s loaded native root has no valid Items list",
                            getSingleNbtExperimentName(experimentMode)
                        );
                        return false;
                    }

                    if (!generatedRoot || !graftNativeShulkerContents(
                        generatedRoot,
                        loadedNbt.get(),
                        donorItems,
                        changedLists)) {
                        if (!aetherAnvilCarrierActive || !loadedNbt) {
                            writeDiagnostic(
                                "[ANVILDUPE2_SINGLE_ITEM_ABORT] test=%s native contents graft failed",
                                getSingleNbtExperimentName(experimentMode)
                            );
                            return false;
                        }
                        swapItem->mUserData = reinterpret_cast<__int64>(
                            loadedNbt.release()
                            );
                        donorItems = static_cast<std::size_t>(std::max(
                            getShulkerContentEntryCount(*swapItem), 0
                        ));
                        changedLists = donorItems > 0 ? 1 : 0;
                        if (donorItems == 0) {
                            writeDiagnostic(
                                "[AETHER_ANVIL_ABORT] complete native root has no Items"
                            );
                            return false;
                        }
                    }
                    originalItems = donorItems;
                    writeDiagnostic(
                        "[ANVILDUPE2_NATIVE_CONTENTS_TRANSFER] loadedName=%s donorItems=%llu copiedLists=%llu targetBeforeItems=%d outerItemId=%d preserveOuterName=1",
                        NbtCommand::loadedName().c_str(),
                        static_cast<unsigned long long>(donorItems),
                        static_cast<unsigned long long>(changedLists),
                        targetBeforeItems,
                        getItemId(*swapItem)
                    );
                }
                else if (!keepFirstShulkerItemInEveryNativeList(
                    generatedRoot,
                    originalItems,
                    changedLists
                )) {
                    writeDiagnostic(
                        "[ANVILDUPE2_SINGLE_ITEM_ABORT] test=%s generated box has no readable native Items list",
                        getSingleNbtExperimentName(experimentMode)
                    );
                    return false;
                }
                changedField = "Items";
                beforeValue = "entries-" + std::to_string(
                    targetBeforeItems
                );
                afterValue = "entries-" + std::to_string(
                    getShulkerContentEntryCount(*swapItem)
                );
            }

            const int requestedInnerItemLock =
                activeAnvilDupe2InnerItemLock;
            if (requestedInnerItemLock != 0) {
                ShulkerItemLockWrite lockWrite;
                if (!generatedRoot ||
                    !writeShulkerItemLock(
                        generatedRoot,
                        static_cast<uint8_t>(requestedInnerItemLock),
                        lockWrite
                    )) {
                    writeDiagnostic(
                        "[ANVILDUPE2_INNER_ITEM_LOCK_ABORT] mode=%s byte=%d not every Items[*].tag could be written with a native ByteTag",
                        getInnerItemLockName(requestedInnerItemLock),
                        requestedInnerItemLock
                    );
                    return false;
                }

                const std::string previousLock =
                    lockWrite.mixedPreviousModes
                    ? "mixed"
                    : lockWrite.previousMode < 0
                    ? "missing"
                    : std::to_string(lockWrite.previousMode);
                writeDiagnostic(
                    "[ANVILDUPE2_INNER_ITEM_LOCK] mode=%s byte=%d previous=%s itemEntries=%llu changedEntries=%llu createdTags=%llu path=Items[*].tag.minecraft:item_lock",
                    getInnerItemLockName(requestedInnerItemLock),
                    requestedInnerItemLock,
                    previousLock.c_str(),
                    static_cast<unsigned long long>(lockWrite.itemCount),
                    static_cast<unsigned long long>(
                        lockWrite.changedListCount
                        ),
                    static_cast<unsigned long long>(
                        lockWrite.createdTagCount
                        )
                );
                changedLists += lockWrite.changedListCount;
            }

            const int targetAfterItems =
                getShulkerContentEntryCount(*swapItem);
            const bool nativeNbtRequired =
                experimentMode != 0 || requestedInnerItemLock != 0;
            if (!swapItem->isValid() ||
                swapItem->item.get() != renamedResult->item.get() ||
                swapItem->mAuxValue != renamedResult->mAuxValue ||
                (nativeNbtRequired && swapItem->mUserData == 0) ||
                (experimentMode == 3 &&
                    !usingLoadedNbt &&
                    targetAfterItems > 1)) {
                writeDiagnostic(
                    "[ANVILDUPE2_SINGLE_ITEM_ABORT] test=%s direct native NBT validation failed originalItems=%llu targetAfterItems=%d",
                    getSingleNbtExperimentName(experimentMode),
                    static_cast<unsigned long long>(originalItems),
                    targetAfterItems
                );
                return false;
            }

            if (!activeAnvilDupe2MaterialSlotMode) {
                ItemStack rebuiltStack;
                NativeItemStackResetGuard rebuiltStackGuard(
                    &rebuiltStack
                );
                if (!rebuildNativeItemStack(
                    *swapItem,
                    rebuiltStack
                ) ||
                    getShulkerContentEntryCount(rebuiltStack) !=
                    targetAfterItems) {
                    writeDiagnostic(
                        "[ANVILDUPE2_SINGLE_ITEM_ABORT] test=%s native ItemStack save/load failed items=%d expected=%d",
                        getSingleNbtExperimentName(experimentMode),
                        getShulkerContentEntryCount(rebuiltStack),
                        targetAfterItems
                    );
                    return false;
                }

                swapItem->setNull(std::nullopt);
                new (swapItem.get()) ItemStack(rebuiltStack);
            }

            writeDiagnostic(
                "[ANVILDUPE2_SINGLE_NBT_TEST] test=%s loadedAvailable=%d loadedUsed=%d targetBeforeItems=%d donorItems=%llu targetAfterItems=%d changedLists=%llu field=%s value={%s -> %s} loadedName=%s",
                getSingleNbtExperimentName(experimentMode),
                loadedTagAvailable ? 1 : 0,
                usingLoadedNbt ? 1 : 0,
                targetBeforeItems,
                static_cast<unsigned long long>(donorItems),
                targetAfterItems,
                static_cast<unsigned long long>(changedLists),
                changedField.c_str(),
                beforeValue.c_str(),
                afterValue.c_str(),
                loadedTagAvailable
                ? NbtCommand::loadedName().c_str()
                : "none"
            );
            writeDiagnostic(
                "[ANVILDUPE2_SINGLE_ITEM] source=%s test=%s itemLock=%s originalItems=%llu duplicatedItems=%d nativeListsChanged=%llu loadedName=%s identity=generated-result",
                usingLoadedNbt ? "loaded-file" : "current-box",
                getSingleNbtExperimentName(experimentMode),
                getInnerItemLockName(requestedInnerItemLock),
                static_cast<unsigned long long>(originalItems),
                targetAfterItems,
                static_cast<unsigned long long>(changedLists),
                usingLoadedNbt ? NbtCommand::loadedName().c_str() : "none"
            );
        }

        if (activeMode == 5 && activeAnvilDupe2NbtWindowMode) {
            bool conflictingAnvilStageAction = false;
            for (const auto& [source, actions] : transaction->data.actions) {
                const int sourceContainer = static_cast<int8_t>(
                    source.containerId
                    );
                if (source.type ==
                    InventorySourceType::NonImplementedFeatureTODO &&
                    !actions.empty() &&
                    (sourceContainer == kAnvilMaterialWindowId ||
                        sourceContainer == kAnvilOutputWindowId)) {
                    conflictingAnvilStageAction = true;
                    break;
                }
            }
            if (conflictingAnvilStageAction) {
                writeDiagnostic(
                    "[ANVIL_NBT_WINDOW_ABORT] native transaction already uses -11 or -13; staging slot must be empty"
                );
                return false;
            }

            int extractionDestinationSlot = -1;
            for (const int emptySlot : findEmptyInventorySlots(localPlayer)) {
                if (emptySlot >= 0 &&
                    emptySlot < kInventorySlotCount &&
                    !reservedInventorySlots[emptySlot]) {
                    extractionDestinationSlot = emptySlot;
                    break;
                }
            }
            if (extractionDestinationSlot < 0) {
                writeDiagnostic(
                    "[ANVIL_NBT_WINDOW_ABORT] no independent empty inventory slot for second-stage extraction"
                );
                return false;
            }

            ItemStack stagedItem(*swapItem);
            NativeItemStackResetGuard stagedItemGuard(&stagedItem);
            stagedItem.mCount = 1;
            if (!stagedItem.isValid()) {
                writeDiagnostic(
                    "[ANVIL_NBT_WINDOW_ABORT] complete staged ItemStack is invalid"
                );
                return false;
            }

            auto replaceResultSnapshot = [&stagedItem](
                ItemStack& target,
                NetworkItemStackDescriptor& descriptor
                ) {
                    const uint8_t nativeCount = target.mCount;
                    const auto nativeNetId = target.mNetIdVariant;
                    ItemStack replacement(stagedItem);
                    NativeItemStackResetGuard replacementGuard(&replacement);
                    replacement.mCount = nativeCount;
                    replacement.mNetIdVariant = nativeNetId;

                    target.setNull(std::nullopt);
                    new (&target) ItemStack(replacement);
                    descriptor = NetworkItemStackDescriptor(&target);
                };

            replaceResultSnapshot(
                selectedCandidate->action->fromItem,
                selectedCandidate->action->fromItemDescriptor
            );
            replaceResultSnapshot(
                selectedDestination->action->toItem,
                selectedDestination->action->toItemDescriptor
            );
            replaceResultSnapshot(
                anvilInputBalanceAction->toItem,
                anvilInputBalanceAction->toItemDescriptor
            );

            renamedResult->setNull(std::nullopt);
            new (renamedResult.get()) ItemStack(
                selectedDestination->action->toItem
            );

            writeDiagnostic(
                "[ANVIL_NBT_RESULT_SYNC] items=%d minus12=1 destination=1 minus10=1 runtimeInputUntouched=1",
                getShulkerContentEntryCount(stagedItem)
            );

            InventoryAction virtualMaterialSource(
                kAnvilMaterialInventorySlot,
                &stagedItem,
                empty,
                static_cast<ContainerID>(kAnvilMaterialWindowId),
                InventorySourceType::NonImplementedFeatureTODO,
                InventorySourceFlags::NoFlag
            );
            NativeInventoryActionResetGuard virtualMaterialSourceGuard(
                &virtualMaterialSource
            );

            InventoryAction stageIntoNbtWindow(
                kAnvilMaterialInventorySlot,
                empty,
                &stagedItem,
                static_cast<ContainerID>(kAnvilOutputWindowId),
                InventorySourceType::NonImplementedFeatureTODO,
                InventorySourceFlags::NoFlag
            );
            NativeInventoryActionResetGuard stageIntoNbtWindowGuard(
                &stageIntoNbtWindow
            );
            stageIntoNbtWindow.toItem.mNetIdVariant =
                virtualMaterialSource.fromItem.mNetIdVariant;
            stageIntoNbtWindow.toItemDescriptor =
                NetworkItemStackDescriptor(&stageIntoNbtWindow.toItem);

            int matchingItemsAlreadyInInventory = 0;
            PlayerInventory* playerInventory = localPlayer->playerInventory;
            if (playerInventory && playerInventory->container) {
                for (int slot = 0; slot < kInventorySlotCount; ++slot) {
                    ItemStack* item =
                        playerInventory->container->getItem(slot);
                    if (item &&
                        item->isValid() &&
                        item->item.get() == stagedItem.item.get() &&
                        item->mAuxValue == stagedItem.mAuxValue) {
                        matchingItemsAlreadyInInventory += item->mCount;
                    }
                }
            }
            anvilPreTransactionOwnedCount =
                matchingItemsAlreadyInInventory +
                static_cast<int>(baseCount);

            resetNativeItemStack(stagedAnvilOutputStack);
            stagedAnvilOutputStack =
                std::make_unique<ItemStack>(stagedItem);
            stagedAnvilDestinationSlot = extractionDestinationSlot;
            nbtWindowStagePending = true;
            nbtWindowFirstStageAccepted = false;
            nbtWindowSecondStageSent = false;
            anvilResyncInventoryPacketObserved = false;
            anvilResyncInventoryPacketCount = 0;
            anvilResyncReadbackState = kAnvilMaterialReadbackNone;
            anvilResyncClearedLocalSlots = 0;
            rewrittenDestinationSlots = { extractionDestinationSlot };
            expectedDestinationCounts.fill(0);
            expectedDestinationCounts[extractionDestinationSlot] = 1;
            rememberExpectedResult(stagedItem);

            const DescriptorPayloadSummary sourceWire =
                inspectDescriptorPayload(
                    virtualMaterialSource.fromItemDescriptor
                );
            const DescriptorPayloadSummary stageWire =
                inspectDescriptorPayload(
                    stageIntoNbtWindow.toItemDescriptor
                );

            transaction->data.addAction(virtualMaterialSource);
            transaction->data.addAction(stageIntoNbtWindow);
            addedDestinationCount = 1;

            writeDiagnostic(
                "[ANVIL_NBT_WINDOW_STAGE] sourceWindow=-11 targetWindow=-13 materialSlot=%u destinationSlot=%d loaded=%d contents=%d identifier=%s clientItemId=%d source={size=%zu hash=%016llX net=%u legacy=%d} target={size=%zu hash=%016llX net=%u legacy=%d} balancedByMinus11=1 resultSnapshotsSynchronized=1 runtimeInputUntouched=1 localPrediction=0",
                kAnvilMaterialInventorySlot,
                extractionDestinationSlot,
                usingLoadedNbt ? 1 : 0,
                getShulkerContentEntryCount(stagedItem),
                getItemIdentifier(stagedItem).c_str(),
                getItemId(stagedItem),
                sourceWire.size,
                static_cast<unsigned long long>(sourceWire.hash),
                static_cast<unsigned int>(sourceWire.hasNetId),
                sourceWire.containsLegacyItems ? 1 : 0,
                stageWire.size,
                static_cast<unsigned long long>(stageWire.hash),
                static_cast<unsigned int>(stageWire.hasNetId),
                stageWire.containsLegacyItems ? 1 : 0
            );
            return true;
        }

        if (activeMode == 5 &&
            activeAnvilDupe2SingleItem &&
            !activeAnvilDupe2SourceBoxMode &&
            !activeAnvilDupe2MaterialSlotMode &&
            !activeAnvilDupe2NbtWindowMode) {
            ItemStack canonicalResult(*swapItem);
            NativeItemStackResetGuard canonicalResultGuard(
                &canonicalResult
            );
            canonicalResult.mCount =
                selectedCandidate->action->fromItem.mCount;

            ItemStack canonicalDestination(*swapItem);
            NativeItemStackResetGuard canonicalDestinationGuard(
                &canonicalDestination
            );
            canonicalDestination.mCount =
                selectedDestination->action->toItem.mCount;
            canonicalDestination.mNetIdVariant =
                selectedDestination->action->toItem.mNetIdVariant;

            if (!canonicalResult.isValid() ||
                !canonicalDestination.isValid() ||
                canonicalResult.item.get() !=
                selectedCandidate->action->fromItem.item.get() ||
                canonicalDestination.item.get() !=
                selectedDestination->action->toItem.item.get() ||
                canonicalResult.mAuxValue !=
                selectedCandidate->action->fromItem.mAuxValue ||
                canonicalDestination.mAuxValue !=
                selectedDestination->action->toItem.mAuxValue) {
                writeDiagnostic(
                    "[ANVILDUPE2_SINGLE_ITEM_ABORT] canonical ItemStack identity does not match the genuine anvil output pair"
                );
                return false;
            }

            selectedCandidate->action->fromItem.setNull(std::nullopt);
            new (&selectedCandidate->action->fromItem)
                ItemStack(canonicalResult);
            selectedCandidate->action->fromItemDescriptor =
                NetworkItemStackDescriptor(
                    &selectedCandidate->action->fromItem
                );

            selectedDestination->action->toItem.setNull(std::nullopt);
            new (&selectedDestination->action->toItem)
                ItemStack(canonicalDestination);
            selectedDestination->action->toItemDescriptor =
                NetworkItemStackDescriptor(
                    &selectedDestination->action->toItem
                );

            renamedResult->setNull(std::nullopt);
            new (renamedResult.get()) ItemStack(canonicalDestination);

            const DescriptorPayloadSummary resultWire =
                inspectDescriptorPayload(
                    selectedCandidate->action->fromItemDescriptor
                );
            const DescriptorPayloadSummary destinationWire =
                inspectDescriptorPayload(
                    selectedDestination->action->toItemDescriptor
                );
            const bool matchingPayload =
                resultWire.readable &&
                destinationWire.readable &&
                resultWire.size == destinationWire.size &&
                resultWire.hash == destinationWire.hash &&
                resultWire.hasNetId == destinationWire.hasNetId &&
                resultWire.blockRuntimeId ==
                destinationWire.blockRuntimeId;
            writeDiagnostic(
                "[ANVILDUPE2_SINGLE_ITEMSTACK_PAIR] identifier=%s clientItemId=%d items=%d payloadMatch=%d result={size=%zu hash=%016llX net=%u legacy=%d} backpack={size=%zu hash=%016llX net=%u legacy=%d}",
                getItemIdentifier(canonicalResult).c_str(),
                getItemId(canonicalResult),
                getShulkerContentEntryCount(canonicalResult),
                matchingPayload ? 1 : 0,
                resultWire.size,
                static_cast<unsigned long long>(resultWire.hash),
                static_cast<unsigned int>(resultWire.hasNetId),
                resultWire.containsLegacyItems ? 1 : 0,
                destinationWire.size,
                static_cast<unsigned long long>(destinationWire.hash),
                static_cast<unsigned int>(destinationWire.hasNetId),
                destinationWire.containsLegacyItems ? 1 : 0
            );

            if (aetherAnvilCarrierActive && requestedCopies == 0) {
                rewrittenDestinationSlots = {
                    static_cast<int>(selectedDestination->action->slot)
                };
                expectedDestinationCounts.fill(0);
                if (selectedDestination->action->slot < kInventorySlotCount) {
                    expectedDestinationCounts[
                        selectedDestination->action->slot
                    ] = canonicalDestination.mCount;
                }
                rememberExpectedResult(canonicalDestination);
                addedDestinationCount = 0;
                writeDiagnostic(
                    "[AETHER_ANVIL_SUCCESS] total=1 extra=0 resultSlot=%u destinationSlot=%u items=%d",
                    selectedCandidate->action->slot,
                    selectedDestination->action->slot,
                    getShulkerContentEntryCount(canonicalDestination)
                );
                return true;
            }
        }

        if (activeMode == 5 && activeAnvilDupe2SourceBoxMode) {
            if (!anvilInputBalanceAction ||
                !genuineAnvilSourceAction ||
                ambiguousGenuineAnvilSourceAction ||
                !anvilInputBalanceAction->toItem.isValid() ||
                !genuineAnvilSourceAction->fromItem.isValid() ||
                !isShulkerBox(genuineAnvilSourceAction->fromItem) ||
                getShulkerContentEntryCount(
                    genuineAnvilSourceAction->fromItem
                ) <= 0 ||
                !NbtCommand::hasLoadedTag()) {
                writeDiagnostic(
                    "[ANVIL_SOURCE_BOX_TARGET_ABORT] requires one genuine non-empty anvil source action, its -10 mirror and a loaded NBT file"
                );
                return false;
            }

            ItemStack sourceCarrier(genuineAnvilSourceAction->fromItem);
            NativeItemStackResetGuard sourceCarrierGuard(&sourceCarrier);
            auto* sourceRoot = reinterpret_cast<CompoundTag*>(
                sourceCarrier.mUserData
                );
            std::unique_ptr<CompoundTag> loadedNbt =
                NbtCommand::copyLoadedTag();
            std::size_t donorItemCount = 0;
            std::size_t changedListCount = 0;
            const int beforeItems =
                getShulkerContentEntryCount(sourceCarrier);
            if (!sourceRoot || !loadedNbt ||
                !graftNativeShulkerContents(
                    sourceRoot,
                    loadedNbt.get(),
                    donorItemCount,
                    changedListCount
                )) {
                writeDiagnostic(
                    "[ANVIL_SOURCE_BOX_TARGET_ABORT] loaded Items could not be grafted into the genuine source carrier"
                );
                return false;
            }

            const int requestedInnerItemLock =
                activeAnvilDupe2InnerItemLock;
            if (requestedInnerItemLock != 0) {
                ShulkerItemLockWrite lockWrite;
                if (!writeShulkerItemLock(
                    sourceRoot,
                    static_cast<uint8_t>(requestedInnerItemLock),
                    lockWrite
                )) {
                    writeDiagnostic(
                        "[ANVIL_SOURCE_BOX_TARGET_ABORT] item lock could not be written to every source Items entry"
                    );
                    return false;
                }
            }

            ItemStack rebuiltSource;
            NativeItemStackResetGuard rebuiltSourceGuard(&rebuiltSource);
            if (!rebuildNativeItemStack(sourceCarrier, rebuiltSource) ||
                !rebuiltSource.isValid() ||
                !isShulkerBox(rebuiltSource) ||
                getShulkerContentEntryCount(rebuiltSource) <= 0) {
                writeDiagnostic(
                    "[ANVIL_SOURCE_BOX_TARGET_ABORT] rebuilt genuine source box is invalid"
                );
                return false;
            }

            rebuiltSource.item = genuineAnvilSourceAction->fromItem.item;
            rebuiltSource.mBlock = genuineAnvilSourceAction->fromItem.mBlock;
            rebuiltSource.mAuxValue =
                genuineAnvilSourceAction->fromItem.mAuxValue;
            rebuiltSource.mCount = genuineAnvilSourceAction->fromItem.mCount;
            rebuiltSource.mValid = genuineAnvilSourceAction->fromItem.mValid;
            rebuiltSource.mNetIdVariant =
                genuineAnvilSourceAction->fromItem.mNetIdVariant;

            const DescriptorPayloadSummary beforeWire =
                inspectDescriptorPayload(
                    genuineAnvilSourceAction->fromItemDescriptor
                );

            ItemStack mirrorSource(rebuiltSource);
            NativeItemStackResetGuard mirrorSourceGuard(&mirrorSource);
            mirrorSource.mCount = anvilInputBalanceAction->toItem.mCount;
            mirrorSource.mNetIdVariant =
                anvilInputBalanceAction->toItem.mNetIdVariant;

            genuineAnvilSourceAction->fromItem.setNull(std::nullopt);
            new (&genuineAnvilSourceAction->fromItem)
                ItemStack(rebuiltSource);
            genuineAnvilSourceAction->fromItemDescriptor =
                NetworkItemStackDescriptor(
                    &genuineAnvilSourceAction->fromItem
                );

            anvilInputBalanceAction->toItem.setNull(std::nullopt);
            new (&anvilInputBalanceAction->toItem)
                ItemStack(mirrorSource);
            anvilInputBalanceAction->toItemDescriptor =
                NetworkItemStackDescriptor(
                    &anvilInputBalanceAction->toItem
                );
            const DescriptorPayloadSummary afterWire =
                inspectDescriptorPayload(
                    genuineAnvilSourceAction->fromItemDescriptor
                );

            addedDestinationCount = 0;
            writeDiagnostic(
                "[ANVIL_SOURCE_BOX_TARGET] realContainer=%d realSide=from mirrorWindow=-10 mirrorSide=target beforeItems=%d donorItems=%zu afterItems=%d changedLists=%zu loadedName=%s outerIdentity=preserved before={size=%zu,hash=%016llX} after={size=%zu,hash=%016llX} nativeMinus12Preserved=1 minus13Used=0",
                genuineAnvilSourceContainer,
                beforeItems,
                donorItemCount,
                getShulkerContentEntryCount(
                    genuineAnvilSourceAction->fromItem
                ),
                changedListCount,
                NbtCommand::loadedName().c_str(),
                beforeWire.size,
                static_cast<unsigned long long>(beforeWire.hash),
                afterWire.size,
                static_cast<unsigned long long>(afterWire.hash)
            );
            return true;
        }

        if (activeMode == 5 && activeAnvilDupe2MaterialSlotMode) {
            bool conflictingAnvilStageAction = false;
            for (const auto& [source, actions] : transaction->data.actions) {
                const int sourceContainer = static_cast<int8_t>(
                    source.containerId
                    );
                if (source.type ==
                    InventorySourceType::NonImplementedFeatureTODO &&
                    !actions.empty() &&
                    (sourceContainer == -11 ||
                        sourceContainer == kAnvilOutputWindowId)) {
                    conflictingAnvilStageAction = true;
                    break;
                }
            }
            if (conflictingAnvilStageAction) {
                writeDiagnostic(
                    "[ANVIL_-13_ABORT] native transaction already uses -11 or -13; material slot must be empty"
                );
                return false;
            }

            int extractionDestinationSlot = -1;
            for (const int emptySlot : findEmptyInventorySlots(localPlayer)) {
                if (emptySlot >= 0 &&
                    emptySlot < kInventorySlotCount &&
                    !reservedInventorySlots[emptySlot]) {
                    extractionDestinationSlot = emptySlot;
                    break;
                }
            }
            if (extractionDestinationSlot < 0) {
                writeDiagnostic(
                    "[ANVIL_-13_ABORT] no independent empty inventory slot for second-stage extraction"
                );
                return false;
            }

            ItemStack stagedItem(*swapItem);
            NativeItemStackResetGuard stagedItemGuard(&stagedItem);
            stagedItem.mCount = 1;

            anvilInputBalanceAction->fromItem.setNull(std::nullopt);
            new (&anvilInputBalanceAction->fromItem)
                ItemStack(stagedItem);
            anvilInputBalanceAction->fromItemDescriptor =
                NetworkItemStackDescriptor(
                    &anvilInputBalanceAction->fromItem
                );

            InventoryAction stageIntoMaterial(
                kAnvilMaterialInventorySlot,
                empty,
                &stagedItem,
                static_cast<ContainerID>(kAnvilOutputWindowId),
                InventorySourceType::NonImplementedFeatureTODO,
                InventorySourceFlags::NoFlag
            );
            NativeInventoryActionResetGuard stageIntoMaterialGuard(
                &stageIntoMaterial
            );

            stageIntoMaterial.toItem.mNetIdVariant =
                anvilInputBalanceAction->fromItem.mNetIdVariant;
            stageIntoMaterial.toItemDescriptor =
                NetworkItemStackDescriptor(&stageIntoMaterial.toItem);

            const DescriptorPayloadSummary sourceWire =
                inspectDescriptorPayload(
                    anvilInputBalanceAction->fromItemDescriptor
                );
            const DescriptorPayloadSummary stageWire =
                inspectDescriptorPayload(
                    stageIntoMaterial.toItemDescriptor
                );

            int matchingItemsAlreadyInInventory = 0;
            PlayerInventory* playerInventory = localPlayer->playerInventory;
            if (playerInventory && playerInventory->container) {
                for (int slot = 0; slot < kInventorySlotCount; ++slot) {
                    ItemStack* item =
                        playerInventory->container->getItem(slot);
                    if (item &&
                        item->isValid() &&
                        item->item.get() == stagedItem.item.get() &&
                        item->mAuxValue == stagedItem.mAuxValue) {
                        matchingItemsAlreadyInInventory += item->mCount;
                    }
                }
            }
            anvilPreTransactionOwnedCount =
                matchingItemsAlreadyInInventory +
                static_cast<int>(baseCount);

            resetNativeItemStack(stagedAnvilOutputStack);
            stagedAnvilOutputStack =
                std::make_unique<ItemStack>(stagedItem);
            stagedAnvilDestinationSlot = extractionDestinationSlot;
            anvilOutputStagePending = true;
            anvilMaterialProbeSent = false;
            anvilMaterialInventoryPacketObserved = false;
            anvilMaterialInventoryPacketCount = 0;
            anvilMaterialReadbackCheckedPacketCount = 0;
            anvilMaterialReadbackState =
                kAnvilMaterialReadbackNone;
            anvilMaterialAuthoritativePacketReceived = false;
            anvilMaterialNativeTakeRequested = false;
            anvilMaterialNativeTakePacketObserved = false;
            anvilMaterialNativeTakePacketId = 0;
            anvilMaterialReadbackGraceUsed = false;
            anvilMaterialPreTakeSnapshotValid = false;
            anvilMaterialPreTakeCounts.fill(0);
            anvilMaterialPreTakeExpectedMatches.fill(false);
            anvilMaterialPreTakeEmptySlots.fill(false);
            anvilResyncInventoryPacketObserved = false;
            anvilResyncInventoryPacketCount = 0;
            anvilResyncReadbackState = kAnvilMaterialReadbackNone;
            anvilResyncClearedLocalSlots = 0;
            rewrittenDestinationSlots = { extractionDestinationSlot };
            expectedDestinationCounts.fill(0);
            expectedDestinationCounts[extractionDestinationSlot] = 1;
            rememberExpectedResult(stagedItem);

            transaction->data.addAction(stageIntoMaterial);
            addedDestinationCount = 1;

            writeDiagnostic(
                activeAnvilDupe2SourceBoxMode
                ? "[ANVIL_SOURCE_BOX_STAGE] sourceWindow=-10.old targetWindow=-13 materialSlot=%u destinationSlot=%d single=%d loaded=%d contents=%d source={size=%zu hash=%016llX net=%u legacy=%d} target={size=%zu hash=%016llX net=%u legacy=%d} nativeOutputPreserved=1"
                : "[ANVIL_-13_STAGE] materialSlot=%u destinationSlot=%d single=%d loaded=%d contents=%d source={size=%zu hash=%016llX net=%u legacy=%d} target={size=%zu hash=%016llX net=%u legacy=%d} nativeOutputPreserved=1",
                kAnvilMaterialInventorySlot,
                extractionDestinationSlot,
                activeAnvilDupe2SingleItem ? 1 : 0,
                usingLoadedNbt ? 1 : 0,
                getShulkerContentEntryCount(stagedItem),
                sourceWire.size,
                static_cast<unsigned long long>(sourceWire.hash),
                static_cast<unsigned int>(sourceWire.hasNetId),
                sourceWire.containsLegacyItems ? 1 : 0,
                stageWire.size,
                static_cast<unsigned long long>(stageWire.hash),
                static_cast<unsigned int>(stageWire.hasNetId),
                stageWire.containsLegacyItems ? 1 : 0
            );
            return true;
        }

        const bool keepInInventory = activeMode == 8
            ? true
            : activeAnvilDupe2Destination == 1;
        const bool stackingApplied = activeMode == 5 &&
            !activeAnvilDupe2SingleItem &&
            activeStacking &&
            activeStackCount > static_cast<int>(baseCount);
        const unsigned int perDestinationCount = stackingApplied
            ? (static_cast<unsigned int>(activeStackCount) / baseCount) * baseCount
            : activeMode == 8
            ? 1
            : baseCount;
        const unsigned int destinationActionRepeats = stackingApplied
            ? (perDestinationCount + baseCount - 1) / baseCount
            : 1;

        std::vector<int> destinationSlots;
        const unsigned int encodedDestinationCapacity =
            kMaxTransactionItemCount / perDestinationCount;
        const unsigned int maxCopiesByEncodedCount =
            activeMode == 5 && activeAnvilDupe2SingleItem
            ? 1
            : encodedDestinationCapacity;
        const std::vector<int> emptySlots =
            findEmptyInventorySlots(localPlayer);
        for (const int destinationSlot : emptySlots) {
            if (destinationSlots.size() >=
                static_cast<std::size_t>(requestedCopies) ||
                destinationSlots.size() >=
                static_cast<std::size_t>(
                    maxCopiesByEncodedCount
                    )) {
                break;
            }
            if (destinationSlot < 0 ||
                destinationSlot >= kInventorySlotCount ||
                reservedInventorySlots[destinationSlot] ||
                (!keepInInventory &&
                    destinationSlot < kMainInventoryFirstSlot)) {
                continue;
            }
            destinationSlots.push_back(destinationSlot);
        }
        const unsigned int actualDestinationCount =
            static_cast<unsigned int>(destinationSlots.size());

        if (actualDestinationCount == 0) {
            writeDiagnostic(
                "[ANVIL_SWAP_ABORT] no empty inventory destination requested=%d maxByCount=%u destination=%s",
                requestedCopies,
                maxCopiesByEncodedCount,
                keepInInventory ? "inventory" : "world-follow-up"
            );
            return true;
        }

        const unsigned int balancedDestinationCount =
            actualDestinationCount;
        const unsigned int virtualSourceCount =
            perDestinationCount * balancedDestinationCount;
        ItemStack virtualSource(*swapItem);
        virtualSource.mCount =
            static_cast<uint8_t>(virtualSourceCount);
        ItemStack destinationResult(*swapItem);
        destinationResult.mCount =
            static_cast<uint8_t>(perDestinationCount);

        anvilInputBalanceAction->fromItem.setNull(std::nullopt);
        new (&anvilInputBalanceAction->fromItem)
            ItemStack(virtualSource);
        anvilInputBalanceAction->fromItemDescriptor =
            NetworkItemStackDescriptor(
                &anvilInputBalanceAction->fromItem
            );

        PlayerInventory* playerInventory = localPlayer->playerInventory;
        int matchingItemsAlreadyInInventory = 0;
        if (playerInventory && playerInventory->container) {
            for (int slot = 0; slot < kInventorySlotCount; ++slot) {
                ItemStack* item =
                    playerInventory->container->getItem(slot);
                if (item &&
                    item->isValid() &&
                    item->item.get() == swapItem->item.get() &&
                    item->mAuxValue == swapItem->mAuxValue) {
                    matchingItemsAlreadyInInventory += item->mCount;
                }
            }
        }
        anvilPreTransactionOwnedCount =
            matchingItemsAlreadyInInventory +
            (activeMode == 5 ? static_cast<int>(baseCount) : 0);

        rewrittenDestinationSlots = destinationSlots;
        expectedDestinationCounts.fill(0);
        for (const int destinationSlot : destinationSlots) {
            expectedDestinationCounts[destinationSlot] =
                static_cast<uint8_t>(perDestinationCount);
        }
        rememberExpectedResult(*swapItem);

        const int resultItems =
            getShulkerContentEntryCount(*renamedResult);
        const int virtualItems = getShulkerContentEntryCount(
            anvilInputBalanceAction->fromItem
        );
        const uint64_t resultNetHash = hashBytes(
            &selectedCandidate->action->fromItem.mNetIdVariant,
            sizeof(selectedCandidate->action->fromItem.mNetIdVariant)
        );
        const uint64_t virtualNetHash = hashBytes(
            &anvilInputBalanceAction->fromItem.mNetIdVariant,
            sizeof(anvilInputBalanceAction->fromItem.mNetIdVariant)
        );

        const InventorySource inventorySource =
            selectedDestination->source;
        auto inventoryActionsIt =
            transaction->data.actions.find(inventorySource);
        if (inventoryActionsIt == transaction->data.actions.end()) {
            writeDiagnostic(
                "[ANVILDUPE2_ABORT] native inventory action group disappeared"
            );
            return false;
        }
        std::vector<InventoryAction>* directInventoryActions =
            &inventoryActionsIt->second;
        directInventoryActions->reserve(
            directInventoryActions->size() +
            destinationSlots.size() *
            static_cast<std::size_t>(
                destinationActionRepeats
                )
        );
        for (const int destinationSlot : destinationSlots) {
            for (unsigned int repeatIndex = 0;
                repeatIndex < destinationActionRepeats;
                ++repeatIndex) {
                if (activeAnvilDupe2SingleItem) {
                    directInventoryActions->emplace_back(
                        destinationSlot,
                        empty,
                        swapItem.get(),
                        inventorySource.containerId,
                        inventorySource.type,
                        inventorySource.flags
                    );

                    InventoryAction& destination =
                        directInventoryActions->back();
                    ItemStack canonicalDestination(*swapItem);
                    NativeItemStackResetGuard canonicalDestinationGuard(
                        &canonicalDestination
                    );
                    canonicalDestination.mCount =
                        static_cast<uint8_t>(perDestinationCount);
                    canonicalDestination.mNetIdVariant =
                        anvilInputBalanceAction->fromItem.mNetIdVariant;
                    destination.toItem.setNull(std::nullopt);
                    new (&destination.toItem)
                        ItemStack(canonicalDestination);
                    destination.toItemDescriptor =
                        NetworkItemStackDescriptor(&destination.toItem);

                    const DescriptorPayloadSummary sourceWire =
                        inspectDescriptorPayload(
                            anvilInputBalanceAction->fromItemDescriptor
                        );
                    const DescriptorPayloadSummary destinationWire =
                        inspectDescriptorPayload(
                            destination.toItemDescriptor
                        );
                    writeDiagnostic(
                        "[ANVILDUPE2_SINGLE_WIRE] slot=%d source={size=%zu hash=%016llX net=%u items=%d legacy=%d} destination={size=%zu hash=%016llX net=%u items=%d legacy=%d}",
                        destinationSlot,
                        sourceWire.size,
                        static_cast<unsigned long long>(sourceWire.hash),
                        static_cast<unsigned int>(sourceWire.hasNetId),
                        sourceWire.containsItems ? 1 : 0,
                        sourceWire.containsLegacyItems ? 1 : 0,
                        destinationWire.size,
                        static_cast<unsigned long long>(
                            destinationWire.hash
                            ),
                        static_cast<unsigned int>(
                            destinationWire.hasNetId
                            ),
                        destinationWire.containsItems ? 1 : 0,
                        destinationWire.containsLegacyItems ? 1 : 0
                    );
                }
                else {
                    directInventoryActions->emplace_back(
                        destinationSlot,
                        empty,
                        swapItem.get(),
                        inventorySource.containerId,
                        inventorySource.type,
                        inventorySource.flags
                    );
                }
            }

            if (!activeAnvilDupe2SingleItem &&
                playerInventory && playerInventory->container) {
                ItemStack predicted(destinationResult);
                playerInventory->container->setItem(
                    destinationSlot,
                    predicted
                );
            }
        }

        addedDestinationCount =
            static_cast<int>(actualDestinationCount);
        writeDiagnostic(
            activeMode == 8
            ? "[ANVIL_SWAP_-10] aHotbar=%d resultBCountKept=%u virtualSource=-10.old virtualACount=%u destinations=%d requested=%d destination=%s stacking=%d perDestination=%u destinationActionRepeats=%u maxDestinationsByCount=%u resultBItems=%d virtualAItems=%d resultBNet=%016llX virtualANet=%016llX baselineOwned=%d localPrediction=%d"
            : "[ANVILDUPE2_VIRTUAL_SOURCE] aHotbar=%d resultCountKept=%u virtualSource=-10.old virtualCount=%u destinations=%d requested=%d destination=%s stacking=%d perDestination=%u destinationActionRepeats=%u maxDestinationsByCount=%u resultItems=%d virtualItems=%d resultNet=%016llX virtualNet=%016llX baselineOwned=%d localPrediction=%d carrier=canonical-output-pair",
            activeAnvilSwapSourceSlot,
            baseCount,
            virtualSourceCount,
            addedDestinationCount,
            requestedCopies,
            keepInInventory ? "inventory" : "world-follow-up",
            stackingApplied ? 1 : 0,
            perDestinationCount,
            destinationActionRepeats,
            maxCopiesByEncodedCount,
            resultItems,
            virtualItems,
            static_cast<unsigned long long>(resultNetHash),
            static_cast<unsigned long long>(virtualNetHash),
            anvilPreTransactionOwnedCount,
            activeMode == 5 && activeAnvilDupe2SingleItem ? 0 : 1
        );
        return true;
    }

    if (activeAnvilDupe3Drop) {
        const uint32_t resultSlot = selectedCandidate->action->slot;
        const InventorySource resultSource = selectedCandidate->source;
        const std::size_t resultActionIndex =
            selectedCandidate->actionIndex;
        const InventorySource destinationSource =
            selectedDestination->source;
        const std::size_t destinationActionIndex =
            selectedDestination->actionIndex;
        InventorySource worldSource;
        worldSource.type = InventorySourceType::WorldInteraction;
        worldSource.containerId = ContainerID::None;
        worldSource.flags = InventorySourceFlags::NoFlag;

        rewrittenDestinationSlots.clear();
        expectedDestinationCounts.fill(0);
        rememberExpectedResult(*renamedResult);

        for (int copyIndex = 0;
            copyIndex < requestedCopies;
            ++copyIndex) {
            if (!appendNativeActionClone(
                transaction,
                resultSource,
                resultActionIndex,
                resultSource,
                resultSlot
            ) ||
                !appendNativeActionClone(
                    transaction,
                    destinationSource,
                    destinationActionIndex,
                    worldSource,
                    0
                )) {
                writeDiagnostic(
                    "[ANVIL_DUPE3_ABORT] copyIndex=%d native addAction failed",
                    copyIndex
                );
                return false;
            }
        }

        addedDestinationCount = requestedCopies;
        writeDiagnostic(
            "[ANVIL_DUPE3_WORLD] pairs=%d resultSlot=%u worldSlot=0 nativeAddAction=1",
            requestedCopies,
            resultSlot
        );
        return true;
    }

    if (activeAnvilDirectDrop) {
        if (!selectedCandidate->actionGroup ||
            selectedCandidate->actionGroup->size() != 1 ||
            selectedCandidate->actionIndex != 0) {
            writeDiagnostic(
                "[ANVIL_DIRECT_ABORT] ambiguous result group size=%llu index=%llu",
                static_cast<unsigned long long>(
                    selectedCandidate->actionGroup
                    ? selectedCandidate->actionGroup->size()
                    : 0
                    ),
                static_cast<unsigned long long>(
                    selectedCandidate->actionIndex
                    )
            );
            return false;
        }

        const unsigned long long requestedItemCount =
            activeStacking && activeStackCount > static_cast<int>(baseCount)
            ? static_cast<unsigned long long>(activeCopyCount) *
            static_cast<unsigned long long>(activeStackCount)
            : static_cast<unsigned long long>(activeCopyCount) *
            static_cast<unsigned long long>(baseCount);
        const unsigned int availableCount =
            kMaxTransactionItemCount - baseCount;
        const unsigned int directDropCount =
            static_cast<unsigned int>(std::min<unsigned long long>(
                requestedItemCount,
                availableCount
            ));
        if (directDropCount == 0) {
            return true;
        }

        ItemStack amplifiedResult(*renamedResult);
        amplifiedResult.mCount = static_cast<uint8_t>(
            baseCount + directDropCount
            );

        const uint32_t resultSlot = selectedCandidate->action->slot;
        const InventorySource resultSource = selectedCandidate->source;
        selectedCandidate->actionGroup->clear();
        selectedCandidate->actionGroup->emplace_back(
            resultSlot,
            &amplifiedResult,
            empty,
            resultSource.containerId,
            resultSource.type,
            resultSource.flags
        );

        ItemStack unitDrop(*renamedResult);
        unitDrop.mCount = 1;
        for (unsigned int index = 0;
            index < directDropCount;
            ++index) {
            InventoryAction directDrop(
                0,
                empty,
                &unitDrop,
                ContainerID::None,
                InventorySourceType::WorldInteraction,
                InventorySourceFlags::NoFlag
            );
            transaction->data.addAction(directDrop);
        }

        rewrittenDestinationSlots.clear();
        expectedDestinationCounts.fill(0);
        rememberExpectedResult(*renamedResult);
        addedDestinationCount = static_cast<int>(directDropCount);
        writeDiagnostic(
            "[ANVIL_DIRECT_WORLD] requestedItems=%llu droppedItems=%u amplifiedResult=%u clampedByByteLimit=%d",
            requestedItemCount,
            directDropCount,
            static_cast<unsigned int>(amplifiedResult.mCount),
            requestedItemCount > availableCount ? 1 : 0
        );
        return true;
    }

    const unsigned int maxEncodedCount = kMaxTransactionItemCount;
    const unsigned int maxCopiesByCount =
        maxEncodedCount / baseCount > 0
        ? maxEncodedCount / baseCount - 1
        : 0;

    std::vector<int> destinationSlots;
    const std::vector<int> emptySlots =
        findEmptyInventorySlots(localPlayer);
    for (const int destinationSlot : emptySlots) {
        if (destinationSlots.size() >=
            static_cast<std::size_t>(requestedCopies) ||
            destinationSlots.size() >=
            static_cast<std::size_t>(maxCopiesByCount)) {
            break;
        }

        if (destinationSlot < 0 ||
            destinationSlot >= kInventorySlotCount ||
            reservedInventorySlots[destinationSlot]) {
            continue;
        }

        destinationSlots.push_back(destinationSlot);
    }

    const bool stackingApplied =
        activeStacking &&
        activeStackCount > static_cast<int>(renamedResult->mCount);

    std::vector<uint8_t> destinationCounts;
    if (stackingApplied) {
        unsigned int remainingCount =
            kMaxTransactionItemCount - baseCount;
        const unsigned int requestedPerDestination =
            (static_cast<unsigned int>(activeStackCount) / baseCount) * baseCount;
        for (std::size_t index = 0;
            index < destinationSlots.size() && remainingCount >= baseCount;
            ++index) {
            const unsigned int availableMultiple =
                (remainingCount / baseCount) * baseCount;
            const unsigned int destinationCount = std::min(
                requestedPerDestination, availableMultiple);
            destinationCounts.push_back(
                static_cast<uint8_t>(destinationCount)
            );
            remainingCount -= destinationCount;
        }
        destinationSlots.resize(destinationCounts.size());
    }
    else {
        destinationCounts = planDestinationCounts(
            destinationSlots.size(),
            renamedResult->mCount,
            false,
            activeStackCount
        );
    }

    rewrittenDestinationSlots = destinationSlots;
    expectedDestinationCounts.fill(0);
    for (std::size_t index = 0;
        index < destinationSlots.size();
        ++index) {
        expectedDestinationCounts[destinationSlots[index]] =
            destinationCounts[index];
    }
    rememberExpectedResult(*renamedResult);

    if (activeMode == 5) {
        int matchingItemsAlreadyInInventory = 0;
        PlayerInventory* playerInventory = localPlayer->playerInventory;
        if (playerInventory && playerInventory->container) {
            for (int slot = 0; slot < kInventorySlotCount; ++slot) {
                ItemStack* item =
                    playerInventory->container->getItem(slot);
                if (item &&
                    item->isValid() &&
                    item->item.get() == renamedResult->item.get() &&
                    item->mAuxValue == renamedResult->mAuxValue) {
                    matchingItemsAlreadyInInventory += item->mCount;
                }
            }
        }

        anvilPreTransactionOwnedCount =
            matchingItemsAlreadyInInventory + baseCount;
        writeDiagnostic(
            "[ANVILDUPE2_BASELINE] inventory=%d genuineAnvilResult=%u owned=%d",
            matchingItemsAlreadyInInventory,
            baseCount,
            anvilPreTransactionOwnedCount
        );
    }

    if (destinationSlots.empty()) {
        return true;
    }

    if (!selectedCandidate->actionGroup ||
        selectedCandidate->actionGroup->size() != 1 ||
        selectedCandidate->actionIndex != 0) {
        writeDiagnostic(
            "[OUT_RESULT] refusing ambiguous anvil result group size=%llu index=%llu",
            static_cast<unsigned long long>(
                selectedCandidate->actionGroup
                ? selectedCandidate->actionGroup->size()
                : 0
                ),
            static_cast<unsigned long long>(
                selectedCandidate->actionIndex
                )
        );
        return false;
    }

    unsigned int amplifiedCount = baseCount;
    unsigned int stackedDestinationCount = 0;
    for (const uint8_t destinationCount : destinationCounts) {
        amplifiedCount += static_cast<unsigned int>(destinationCount);
        if (destinationCount > baseCount) {
            ++stackedDestinationCount;
        }
    }
    if (amplifiedCount > kMaxTransactionItemCount) {
        writeDiagnostic(
            "[OUT_RESULT] anvil amplified count overflow=%u",
            amplifiedCount
        );
        return false;
    }

    ItemStack amplifiedResult(*renamedResult);
    amplifiedResult.mNetIdVariant =
        selectedCandidate->action->fromItem.mNetIdVariant;
    amplifiedResult.mCount = static_cast<uint8_t>(amplifiedCount);

    const uint32_t resultSlot = selectedCandidate->action->slot;
    const InventorySource resultSource = selectedCandidate->source;
    selectedCandidate->actionGroup->clear();
    selectedCandidate->actionGroup->emplace_back(
        resultSlot,
        &amplifiedResult,
        empty,
        resultSource.containerId,
        resultSource.type,
        resultSource.flags
    );

    if (activeMode == 5) {
        writeDiagnostic(
            "[ANVILDUPE2_SINGLE_RESULT] resultActions=1 source=-12 resultCount=%u->%u inventoryDestinations=%llu requested=%d resultNet=%016llX destinationNet=%016llX userData=%p items=%d",
            baseCount,
            static_cast<unsigned int>(amplifiedResult.mCount),
            static_cast<unsigned long long>(destinationSlots.size()),
            requestedCopies,
            static_cast<unsigned long long>(hashBytes(
                &amplifiedResult.mNetIdVariant,
                sizeof(amplifiedResult.mNetIdVariant)
            )),
            static_cast<unsigned long long>(hashBytes(
                &renamedResult->mNetIdVariant,
                sizeof(renamedResult->mNetIdVariant)
            )),
            reinterpret_cast<void*>(renamedResult->mUserData),
            getShulkerContentEntryCount(*renamedResult)
        );
    }
    else {
        writeDiagnostic(
            "[BALANCE] anvilResultCount=%u->%u destinations=%llu stackedDestinations=%u requested=%d stackCount=%d byteLimit=%u",
            baseCount,
            static_cast<unsigned int>(amplifiedResult.mCount),
            static_cast<unsigned long long>(destinationSlots.size()),
            stackedDestinationCount,
            requestedCopies,
            activeStackCount,
            kMaxTransactionItemCount
        );
    }

    const InventorySource inventorySource = selectedDestination->source;
    auto& inventoryActions = transaction->data.actions[inventorySource];
    for (std::size_t index = 0;
        index < destinationSlots.size();
        ++index) {
        const unsigned int actionRepeats = stackingApplied
            ? static_cast<unsigned int>(destinationCounts[index]) / baseCount
            : 1;
        for (unsigned int repeat = 0;
            repeat < actionRepeats;
            ++repeat) {
            inventoryActions.emplace_back(
                destinationSlots[index],
                empty,
                renamedResult.get(),
                inventorySource.containerId,
                inventorySource.type,
                inventorySource.flags
            );
        }
        ++addedDestinationCount;
    }

    return true;
}

bool Dupe::sendEnchantDupeTransaction(LocalPlayer* localPlayer) {
    if (activeMode != 7 ||
        enchantPacketInFlight ||
        !localPlayer ||
        !localPlayer->level ||
        !localPlayer->level->getPacketSender() ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container ||
        !cachedEnchantInput ||
        !cachedEnchantInput->isValid() ||
        cachedEnchantInput->mCount != 1 ||
        !cachedEnchantMaterial ||
        !cachedEnchantMaterial->isValid() ||
        cachedEnchantMaterial->mCount < 2) {
        return false;
    }

    if (!canConstructNativeInventoryTransaction()) {
        writeDiagnostic(
            "[ENCHANT_ABORT] native preflight failed txVtable=%p itemCopy=%p actionCtor=%p addAction=%p packetCtor=%p empty=%p itemVtable=%p",
            reinterpret_cast<void*>(
                Addresses::ComplexInventoryTransactionVTable
                ),
            reinterpret_cast<void*>(
                Addresses::ItemStackBase_CopyConstructor
                ),
            reinterpret_cast<void*>(Addresses::InventoryAction_Constructor),
            reinterpret_cast<void*>(
                Addresses::InventoryTransaction_addAction
                ),
            reinterpret_cast<void*>(
                Addresses::InventoryTransactionPacket_Constructor
                ),
            reinterpret_cast<void*>(Addresses::ItemStack_EMPTY_ITEM),
            reinterpret_cast<void*>(Addresses::ItemStackVTable)
        );
        return false;
    }

    if (enchantSequencePhase == EnchantSequencePhase::Idle) {
        ItemStack* donor = localPlayer->playerInventory->container->getItem(
            activeEnchantDonorSlot
        );
        const std::size_t donorEnchantCount = donor
            ? getNativeEnchantEntryCount(*donor)
            : 0;
        if (!donor || !donor->isValid() || donor->mUserData == 0 ||
            donorEnchantCount == 0) {
            writeDiagnostic(
                "[ENCHANT_ABORT] hotbar donor slot=%d has no native enchant list valid=%d userData=%p enchantments=%llu",
                activeEnchantDonorSlot,
                donor && donor->isValid() ? 1 : 0,
                donor
                ? reinterpret_cast<void*>(donor->mUserData)
                : nullptr,
                static_cast<unsigned long long>(donorEnchantCount)
            );
            return false;
        }
        resetNativeItemStack(cachedEnchantDonor);
        cachedEnchantDonor = std::make_unique<ItemStack>(*donor);
        cachedEnchantDonor->mCount = 1;
        writeDiagnostic(
            "[ENCHANT_DONOR] hotbar=%d id=%d enchantments=%llu userData=%p",
            activeEnchantDonorSlot,
            getItemId(*cachedEnchantDonor),
            static_cast<unsigned long long>(donorEnchantCount),
            reinterpret_cast<void*>(cachedEnchantDonor->mUserData)
        );

        enchantSequenceDestinationSlots =
            findEmptyInventorySlots(localPlayer);
        if (enchantSequenceDestinationSlots.size() >
            static_cast<std::size_t>(activeCopyCount)) {
            enchantSequenceDestinationSlots.resize(
                static_cast<std::size_t>(activeCopyCount)
            );
        }

        const std::size_t materialCopyLimit =
            static_cast<std::size_t>(cachedEnchantMaterial->mCount - 1);
        if (enchantSequenceDestinationSlots.size() > materialCopyLimit) {
            enchantSequenceDestinationSlots.resize(materialCopyLimit);
        }
        if (enchantSequenceDestinationSlots.empty()) {
            writeDiagnostic(
                "[ENCHANT_ABORT] no empty inventory destination copies=%d",
                activeCopyCount
            );
            return false;
        }

        enchantSequenceCompletedSlots.clear();
        enchantSequenceCompletedSlots.reserve(
            enchantSequenceDestinationSlots.size()
        );
        enchantSequenceIndex = 0;
        enchantSequenceInitialMaterialCount =
            static_cast<unsigned int>(cachedEnchantMaterial->mCount);
        enchantSequencePhase = EnchantSequencePhase::Preload;
        writeDiagnostic(
            "[ENCHANT_SEQUENCE_ARM] units=%llu lapisInitial=%u stageGapTicks=%d",
            static_cast<unsigned long long>(
                enchantSequenceDestinationSlots.size()
                ),
            enchantSequenceInitialMaterialCount,
            kEnchantStageGapTicks
        );
    }

    if (enchantSequenceDestinationSlots.empty() ||
        enchantSequenceIndex >= enchantSequenceDestinationSlots.size() ||
        enchantSequenceInitialMaterialCount < 2) {
        writeDiagnostic(
            "[ENCHANT_ABORT] invalid sequence phase=%d index=%llu units=%llu lapisInitial=%u",
            static_cast<int>(enchantSequencePhase),
            static_cast<unsigned long long>(enchantSequenceIndex),
            static_cast<unsigned long long>(
                enchantSequenceDestinationSlots.size()
                ),
            enchantSequenceInitialMaterialCount
        );
        return false;
    }

    const unsigned int resultCount = static_cast<unsigned int>(
        enchantSequenceDestinationSlots.size()
        );
    if (resultCount == 0 || resultCount > kMaxTransactionItemCount) {
        return false;
    }

    ItemStack* empty = ItemStack::getEmptyItem();
    if (!empty) {
        return false;
    }

    ItemStack inputTarget(*cachedEnchantInput);
    NativeItemStackResetGuard inputTargetGuard(&inputTarget);
    inputTarget.mCount = 1;

    ItemStack inputVirtualSource(*cachedEnchantInput);
    NativeItemStackResetGuard inputVirtualSourceGuard(
        &inputVirtualSource
    );
    inputVirtualSource.mCount = 1;

    const bool targetHadUserData = cachedEnchantInput->mUserData != 0;
    ItemStack outputSource(*cachedEnchantInput);
    NativeItemStackResetGuard outputSourceGuard(&outputSource);
    outputSource.mCount = 1;
    if (!targetHadUserData) {
        ItemStack donorRootOwner(*cachedEnchantDonor);
        NativeItemStackResetGuard donorRootOwnerGuard(&donorRootOwner);
        if (donorRootOwner.mUserData == 0) {
            writeDiagnostic(
                "[ENCHANT_ABORT] donor root allocation missing donorSlot=%d",
                activeEnchantDonorSlot
            );
            return false;
        }
        outputSource.mUserData = donorRootOwner.mUserData;
        donorRootOwner.mUserData = 0;
    }

    std::size_t graftedEnchantCount = 0;
    if (!cachedEnchantDonor ||
        !graftNativeEnchantEntry(
            outputSource,
            *cachedEnchantDonor,
            graftedEnchantCount,
            targetHadUserData
        ) ||
        graftedEnchantCount == 0 ||
        outputSource.item.get() != cachedEnchantInput->item.get() ||
        outputSource.mAuxValue != cachedEnchantInput->mAuxValue ||
        (isShulkerBox(*cachedEnchantInput) &&
            getShulkerContentEntryCount(outputSource) !=
            getShulkerContentEntryCount(*cachedEnchantInput))) {
        writeDiagnostic(
            "[ENCHANT_ABORT] failed to graft donor enchant NBT donorSlot=%d donorValid=%d grafted=%llu targetId=%d outputId=%d targetAux=%d outputAux=%d targetUserData=%d inputItems=%d outputItems=%d",
            activeEnchantDonorSlot,
            cachedEnchantDonor && cachedEnchantDonor->isValid() ? 1 : 0,
            static_cast<unsigned long long>(graftedEnchantCount),
            getItemId(*cachedEnchantInput),
            getItemId(outputSource),
            static_cast<int>(cachedEnchantInput->mAuxValue),
            static_cast<int>(outputSource.mAuxValue),
            targetHadUserData ? 1 : 0,
            getShulkerContentEntryCount(*cachedEnchantInput),
            getShulkerContentEntryCount(outputSource)
        );
        return false;
    }

    ItemStack outputUnit(outputSource);
    NativeItemStackResetGuard outputUnitGuard(&outputUnit);
    outputUnit.mCount = 1;

    const unsigned int initialMaterialCount =
        enchantSequenceInitialMaterialCount;
    const int destinationSlot =
        enchantSequenceDestinationSlots[enchantSequenceIndex];

    ItemStack unitMaterialBefore(*cachedEnchantMaterial);
    NativeItemStackResetGuard unitMaterialBeforeGuard(&unitMaterialBefore);
    unitMaterialBefore.mCount = static_cast<uint8_t>(
        initialMaterialCount -
        static_cast<unsigned int>(enchantSequenceIndex)
        );
    ItemStack unitMaterialAfter(*cachedEnchantMaterial);
    NativeItemStackResetGuard unitMaterialAfterGuard(&unitMaterialAfter);
    unitMaterialAfter.mCount = static_cast<uint8_t>(
        unitMaterialBefore.mCount - 1
        );

    const auto addNativeAction = [](
        ComplexInventoryTransaction* transactionPointer,
        uint32_t slot,
        ItemStack* sourceItem,
        ItemStack* targetItem,
        ContainerID containerId,
        InventorySourceType sourceType
        ) {
            InventoryAction action(
                slot,
                sourceItem,
                targetItem,
                containerId,
                sourceType,
                InventorySourceFlags::NoFlag
            );
            NativeInventoryActionResetGuard actionGuard(&action);
            transactionPointer->data.addAction(action);
        };

    const auto sendTransactionStage = [this, localPlayer](
        std::unique_ptr<ComplexInventoryTransaction> transaction,
        const char* logTag,
        const char* stageName,
        std::size_t unitIndex
        ) {
            logLegacyTransaction(logTag, transaction.get());

            ComplexInventoryTransaction* expectedTransaction = transaction.get();
            InventoryTransactionPacket packet(std::move(transaction));
            ComplexInventoryTransaction* validatedTransaction =
                getValidatedTransaction(&packet);
            if (!packet.getTransaction() ||
                packet.getTransaction() != expectedTransaction ||
                validatedTransaction != packet.getTransaction()) {
                writeDiagnostic(
                    "[ENCHANT_ABORT] unit=%llu stage=%s packet ownership/layout expected=%p member=%p validated=%p offset=0x%llX size=0x%llX",
                    static_cast<unsigned long long>(unitIndex + 1),
                    stageName,
                    expectedTransaction,
                    packet.getTransaction(),
                    validatedTransaction,
                    static_cast<unsigned long long>(
                        kTransactionPointerOffsets[0]
                        ),
                    static_cast<unsigned long long>(
                        sizeof(InventoryTransactionPacket)
                        )
                );
                resetNativeTransactionItemStacks(
                    validatedTransaction
                    ? validatedTransaction
                    : packet.getTransaction()
                );
                return false;
            }

            enchantPacketInFlight = true;
            localPlayer->level->getPacketSender()->sendToServer(&packet);
            enchantPacketInFlight = false;

            resetNativeTransactionItemStacks(packet.getTransaction());
            writeDiagnostic(
                "[ENCHANT_STAGE_SENT] unit=%llu stage=%s",
                static_cast<unsigned long long>(unitIndex + 1),
                stageName
            );
            return true;
        };

    if (enchantSequencePhase == EnchantSequencePhase::Preload) {
        auto preloadTransaction =
            std::make_unique<ComplexInventoryTransaction>();
        preloadTransaction->type =
            ComplexInventoryTransaction::Type::NormalTransaction;
        addNativeAction(
            preloadTransaction.get(),
            0,
            &inputVirtualSource,
            &inputTarget,
            static_cast<ContainerID>(kEnchantInputWindowId),
            InventorySourceType::NonImplementedFeatureTODO
        );


        writeDiagnostic(
            "[ENCHANT_UNIT_BEGIN] unit=%llu/%llu destination=%d phase=preload targetId=%d targetAux=%d targetHadUserData=%d lapisBefore=%u lapisAfter=%u",
            static_cast<unsigned long long>(enchantSequenceIndex + 1),
            static_cast<unsigned long long>(
                enchantSequenceDestinationSlots.size()
                ),
            destinationSlot,
            getItemId(inputTarget),
            static_cast<int>(inputTarget.mAuxValue),
            targetHadUserData ? 1 : 0,
            static_cast<unsigned int>(unitMaterialBefore.mCount),
            static_cast<unsigned int>(unitMaterialAfter.mCount)
        );
        if (!sendTransactionStage(
            std::move(preloadTransaction),
            "ENCHANT_SYNTHETIC_PRELOAD_BEFORE_SEND",
            "preload",
            enchantSequenceIndex
        )) {
            return false;
        }

        enchantSequencePhase = EnchantSequencePhase::Complete;
        enchantSettleTicks = kEnchantStageGapTicks;
        writeDiagnostic(
            "[ENCHANT_STAGE_WAIT] unit=%llu next=complete ticks=%d",
            static_cast<unsigned long long>(enchantSequenceIndex + 1),
            enchantSettleTicks
        );
        return true;
    }

    if (enchantSequencePhase != EnchantSequencePhase::Complete) {
        return false;
    }

    auto completionTransaction =
        std::make_unique<ComplexInventoryTransaction>();
    completionTransaction->type =
        ComplexInventoryTransaction::Type::NormalTransaction;

    addNativeAction(
        completionTransaction.get(),
        0,
        &unitMaterialAfter,
        &unitMaterialBefore,
        static_cast<ContainerID>(kEnchantMaterialWindowId),
        InventorySourceType::NonImplementedFeatureTODO
    );

    addNativeAction(
        completionTransaction.get(),
        kEnchantMaterialUiSlot,
        &unitMaterialBefore,
        &unitMaterialAfter,
        ContainerID::PlayerUIOnly,
        InventorySourceType::ContainerInventory
    );

    addNativeAction(
        completionTransaction.get(),
        0,
        &outputSource,
        empty,
        static_cast<ContainerID>(kEnchantOutputWindowId),
        InventorySourceType::NonImplementedFeatureTODO
    );
    addNativeAction(
        completionTransaction.get(),
        static_cast<uint32_t>(destinationSlot),
        empty,
        &outputUnit,
        ContainerID::Inventory,
        InventorySourceType::ContainerInventory
    );
    if (!sendTransactionStage(
        std::move(completionTransaction),
        "ENCHANT_SYNTHETIC_COMPLETE_BEFORE_SEND",
        "complete",
        enchantSequenceIndex
    )) {
        return false;
    }

    enchantSequenceCompletedSlots.push_back(destinationSlot);
    ++enchantSequenceIndex;
    if (enchantSequenceIndex < enchantSequenceDestinationSlots.size()) {
        enchantSequencePhase = EnchantSequencePhase::Preload;
        enchantSettleTicks = kEnchantStageGapTicks;
        writeDiagnostic(
            "[ENCHANT_STAGE_WAIT] completedUnits=%llu next=preload ticks=%d",
            static_cast<unsigned long long>(enchantSequenceIndex),
            enchantSettleTicks
        );
        return true;
    }

    rewrittenDestinationSlots = enchantSequenceCompletedSlots;
    expectedDestinationCounts.fill(0);
    for (const int completedSlot : rewrittenDestinationSlots) {
        expectedDestinationCounts[completedSlot] = 1;
    }
    rememberExpectedResult(outputUnit);

    rewrittenDestinationCount =
        static_cast<int>(rewrittenDestinationSlots.size());
    enchantTransactionPending = false;
    enchantSettleTicks = 0;
    enchantSequencePhase = EnchantSequencePhase::Idle;
    resetNativeItemStack(cachedEnchantInput);
    resetNativeItemStack(cachedEnchantMaterial);
    resetNativeItemStack(cachedEnchantDonor);

    writeDiagnostic(
        "[ENCHANT_SEND] mode=two-stage-paced-real-ench-transfer requested=%u completedUnits=%d packets=%d inputCount=1 outputCount=1 destinations=%d lapisInitial=%u lapisFinal=%u localPrediction=0 targetId=%d targetAux=%d targetHadUserData=%d inputItems=%d outputItems=%d donorSlot=%d donorId=%d enchantments=%llu",
        resultCount,
        rewrittenDestinationCount,
        rewrittenDestinationCount * 2,
        rewrittenDestinationCount,
        initialMaterialCount,
        initialMaterialCount -
        static_cast<unsigned int>(rewrittenDestinationCount),
        getItemId(inputTarget),
        static_cast<int>(inputTarget.mAuxValue),
        targetHadUserData ? 1 : 0,
        getShulkerContentEntryCount(inputTarget),
        getShulkerContentEntryCount(outputUnit),
        activeEnchantDonorSlot,
        cachedEnchantDonor
        ? getItemId(*cachedEnchantDonor)
        : 0,
        static_cast<unsigned long long>(graftedEnchantCount)
    );

    enchantSequenceDestinationSlots.clear();
    enchantSequenceCompletedSlots.clear();
    enchantSequenceIndex = 0;
    enchantSequenceInitialMaterialCount = 0;
    return true;
}
bool Dupe::sendNbtWindowTakeTransaction(LocalPlayer* localPlayer) {
    if (!localPlayer ||
        !localPlayer->level ||
        !localPlayer->level->getPacketSender() ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container ||
        !canConstructNativeInventoryTransaction() ||
        !activeAnvilDupe2NbtWindowMode ||
        !nbtWindowStagePending ||
        !nbtWindowFirstStageAccepted ||
        nbtWindowSecondStageSent ||
        !stagedAnvilOutputStack ||
        !stagedAnvilOutputStack->isValid() ||
        stagedAnvilOutputStack->mCount == 0 ||
        stagedAnvilDestinationSlot < 0 ||
        stagedAnvilDestinationSlot >= kInventorySlotCount) {
        return false;
    }

    ItemStack* empty = ItemStack::getEmptyItem();
    if (!empty) {
        return false;
    }

    ItemStack* currentDestination =
        localPlayer->playerInventory->container->getItem(
            stagedAnvilDestinationSlot
        );
    if (currentDestination && currentDestination->isValid()) {
        writeDiagnostic(
            "[ANVIL_NBT_WINDOW_TAKE_ABORT] destinationSlot=%d is no longer empty id=%d count=%u",
            stagedAnvilDestinationSlot,
            getItemId(*currentDestination),
            static_cast<unsigned int>(currentDestination->mCount)
        );
        return false;
    }

    ItemStack windowSource(*stagedAnvilOutputStack);
    NativeItemStackResetGuard windowSourceGuard(&windowSource);
    windowSource.mCount = 1;
    ItemStack inventoryTarget(*stagedAnvilOutputStack);
    NativeItemStackResetGuard inventoryTargetGuard(&inventoryTarget);
    inventoryTarget.mCount = 1;
    inventoryTarget.mNetIdVariant = windowSource.mNetIdVariant;

    InventoryAction takeFromNbtWindow(
        kAnvilMaterialInventorySlot,
        &windowSource,
        empty,
        static_cast<ContainerID>(kAnvilOutputWindowId),
        InventorySourceType::NonImplementedFeatureTODO,
        InventorySourceFlags::NoFlag
    );
    NativeInventoryActionResetGuard takeFromNbtWindowGuard(
        &takeFromNbtWindow
    );
    InventoryAction placeInInventory(
        static_cast<uint32_t>(stagedAnvilDestinationSlot),
        empty,
        &inventoryTarget,
        ContainerID::Inventory,
        InventorySourceType::ContainerInventory,
        InventorySourceFlags::NoFlag
    );
    NativeInventoryActionResetGuard placeInInventoryGuard(
        &placeInInventory
    );

    placeInInventory.toItem.mNetIdVariant =
        takeFromNbtWindow.fromItem.mNetIdVariant;
    placeInInventory.toItemDescriptor =
        NetworkItemStackDescriptor(&placeInInventory.toItem);

    const DescriptorPayloadSummary sourceWire =
        inspectDescriptorPayload(
            takeFromNbtWindow.fromItemDescriptor
        );
    const DescriptorPayloadSummary destinationWire =
        inspectDescriptorPayload(
            placeInInventory.toItemDescriptor
        );

    auto transaction = std::make_unique<ComplexInventoryTransaction>();
    transaction->type =
        ComplexInventoryTransaction::Type::NormalTransaction;
    transaction->data.addAction(takeFromNbtWindow);
    transaction->data.addAction(placeInInventory);

    ComplexInventoryTransaction* expectedTransaction = transaction.get();
    InventoryTransactionPacket packet(std::move(transaction));
    ComplexInventoryTransaction* packetTransaction = packet.getTransaction();
    if (!packetTransaction ||
        packetTransaction != expectedTransaction ||
        getValidatedTransaction(&packet) != packetTransaction) {
        resetNativeTransactionItemStacks(packetTransaction);
        return false;
    }

    logLegacyTransaction(
        "ANVIL_NBT_WINDOW_TAKE_OUT",
        packetTransaction
    );
    writeDiagnostic(
        "[ANVIL_NBT_WINDOW_TAKE_SEND] window=-13 materialSlot=%u destinationSlot=%d item=%s clientItemId=%d contents=%d source={size=%zu hash=%016llX net=%u legacy=%d} destination={size=%zu hash=%016llX net=%u legacy=%d} transaction=normal-balanced localPrediction=0",
        kAnvilMaterialInventorySlot,
        stagedAnvilDestinationSlot,
        getItemIdentifier(windowSource).c_str(),
        getItemId(windowSource),
        getShulkerContentEntryCount(windowSource),
        sourceWire.size,
        static_cast<unsigned long long>(sourceWire.hash),
        static_cast<unsigned int>(sourceWire.hasNetId),
        sourceWire.containsLegacyItems ? 1 : 0,
        destinationWire.size,
        static_cast<unsigned long long>(destinationWire.hash),
        static_cast<unsigned int>(destinationWire.hasNetId),
        destinationWire.containsLegacyItems ? 1 : 0
    );

    nbtWindowSecondStageSent = true;
    localPlayer->level->getPacketSender()->sendToServer(&packet);
    resetNativeTransactionItemStacks(packetTransaction);
    return true;
}

bool Dupe::sendAnvilMaterialProbe(LocalPlayer* localPlayer) {
    if (!localPlayer ||
        !localPlayer->level ||
        !localPlayer->level->getPacketSender() ||
        !canConstructNativeInventoryTransaction() ||
        (!anvilOutputStagePending &&
            !(activeAnvilDupe2NbtWindowMode &&
                nbtWindowStagePending)) ||
        !stagedAnvilOutputStack ||
        !stagedAnvilOutputStack->isValid() ||
        stagedAnvilOutputStack->mCount == 0) {
        return false;
    }

    ItemStack* empty = ItemStack::getEmptyItem();
    if (!empty) {
        return false;
    }

    InventoryAction inspectMaterial(
        kAnvilMaterialInventorySlot,
        empty,
        empty,
        static_cast<ContainerID>(kAnvilOutputWindowId),
        InventorySourceType::NonImplementedFeatureTODO,
        InventorySourceFlags::NoFlag
    );
    NativeInventoryActionResetGuard inspectMaterialGuard(&inspectMaterial);

    auto transaction = std::make_unique<ComplexInventoryTransaction>();
    transaction->type =
        ComplexInventoryTransaction::Type::NormalTransaction;
    transaction->data.addAction(inspectMaterial);

    ComplexInventoryTransaction* expectedTransaction = transaction.get();
    InventoryTransactionPacket packet(std::move(transaction));
    ComplexInventoryTransaction* packetTransaction = packet.getTransaction();
    if (!packetTransaction ||
        packetTransaction != expectedTransaction ||
        getValidatedTransaction(&packet) != packetTransaction) {
        resetNativeTransactionItemStacks(packetTransaction);
        return false;
    }

    logLegacyTransaction(
        "ANVIL_-13_MATERIAL_PROBE_OUT",
        packetTransaction
    );
    anvilMaterialProbeSent = true;
    anvilMaterialInventoryPacketObserved = false;
    anvilMaterialInventoryPacketCount = 0;
    anvilMaterialReadbackCheckedPacketCount = 0;
    anvilMaterialReadbackState = kAnvilMaterialReadbackNone;
    anvilMaterialAuthoritativePacketReceived = false;
    anvilMaterialNativeTakeRequested = false;
    anvilMaterialNativeTakePacketObserved = false;
    anvilMaterialNativeTakePacketId = 0;
    anvilMaterialReadbackGraceUsed = false;
    anvilMaterialPreTakeSnapshotValid = false;
    anvilMaterialPreTakeCounts.fill(0);
    anvilMaterialPreTakeExpectedMatches.fill(false);
    anvilMaterialPreTakeEmptySlots.fill(false);
    writeDiagnostic(
        "[ANVIL_-13_MATERIAL_PROBE_SEND] materialSlot=%u shape=AIR-to-AIR purpose=read-server-A localPrediction=0",
        kAnvilMaterialInventorySlot
    );
    localPlayer->level->getPacketSender()->sendToServer(&packet);

    resetNativeTransactionItemStacks(packetTransaction);
    return true;
}

bool Dupe::sendInventoryResyncProbe(LocalPlayer* localPlayer) {
    if (!localPlayer ||
        !localPlayer->level ||
        !localPlayer->level->getPacketSender() ||
        !canConstructNativeInventoryTransaction()) {
        return false;
    }

    std::vector<int> probeSlots = rewrittenDestinationSlots;
    if (probeSlots.empty()) {
        return false;
    }

    ItemStack* empty = ItemStack::getEmptyItem();
    if (!empty) {
        return false;
    }

    int clearedLocalSlots = 0;
    if (activeMode != 7 &&
        localPlayer->playerInventory &&
        localPlayer->playerInventory->container) {
        Container* container =
            localPlayer->playerInventory->container;
        for (const int slot : rewrittenDestinationSlots) {
            if (slot < 0 || slot >= kInventorySlotCount) {
                continue;
            }
            const uint8_t expectedCount =
                expectedDestinationCounts[slot];
            ItemStack* current = container->getItem(slot);
            ComparableItemSummary expectedWire{};
            ComparableItemSummary actualWire{};
            if (expectedCount > 0 &&
                expectedResultStack &&
                expectedResultStack->isValid() &&
                matchesExpectedResult(current, expectedCount) &&
                comparableItemPayloadsEqual(
                    expectedResultStack.get(),
                    current,
                    expectedWire,
                    actualWire
                )) {
                container->setItem(slot, *empty);
                ++clearedLocalSlots;
            }
        }
    }

    auto transaction = std::make_unique<ComplexInventoryTransaction>();
    transaction->type =
        ComplexInventoryTransaction::Type::NormalTransaction;

    InventorySource inventorySource{};
    inventorySource.type = InventorySourceType::ContainerInventory;
    inventorySource.containerId = ContainerID::Inventory;
    inventorySource.flags = InventorySourceFlags::NoFlag;

    auto& probeActions = transaction->data.actions[inventorySource];
    probeActions.reserve(probeSlots.size());
    for (const int slot : probeSlots) {
        if (slot < 0 || slot >= kInventorySlotCount) {
            continue;
        }

        probeActions.emplace_back(
            slot,
            empty,
            empty,
            inventorySource.containerId,
            inventorySource.type,
            inventorySource.flags
        );
    }

    std::size_t stageProbeCount = 0;
    if (activeMode == 7) {
        InventorySource uiSource{};
        uiSource.type = InventorySourceType::ContainerInventory;
        uiSource.containerId = ContainerID::PlayerUIOnly;
        uiSource.flags = InventorySourceFlags::NoFlag;

        auto& uiActions = transaction->data.actions[uiSource];
        uiActions.reserve(2);
        uiActions.emplace_back(
            kEnchantInputUiSlot,
            empty,
            empty,
            uiSource.containerId,
            uiSource.type,
            uiSource.flags
        );
        uiActions.emplace_back(
            kEnchantMaterialUiSlot,
            empty,
            empty,
            uiSource.containerId,
            uiSource.type,
            uiSource.flags
        );
        stageProbeCount = uiActions.size();
    }
    else if (activeMode == 5 &&
        activeAnvilDupe2NbtWindowMode) {
        InventorySource anvilStageSource{};
        anvilStageSource.type =
            InventorySourceType::NonImplementedFeatureTODO;
        anvilStageSource.containerId =
            static_cast<ContainerID>(kAnvilOutputWindowId);
        anvilStageSource.flags = InventorySourceFlags::NoFlag;

        auto& stageActions =
            transaction->data.actions[anvilStageSource];
        stageActions.emplace_back(
            kAnvilMaterialInventorySlot,
            empty,
            empty,
            anvilStageSource.containerId,
            anvilStageSource.type,
            anvilStageSource.flags
        );
        stageProbeCount = stageActions.size();
    }

    if (probeActions.empty()) {
        return false;
    }

    const std::size_t actionCount =
        probeActions.size() + stageProbeCount;
    ComplexInventoryTransaction* expectedTransaction = transaction.get();
    InventoryTransactionPacket packet(std::move(transaction));
    if (!packet.getTransaction() ||
        packet.getTransaction() != expectedTransaction ||
        getValidatedTransaction(&packet) != packet.getTransaction()) {
        resetNativeTransactionItemStacks(packet.getTransaction());
        return false;
    }
    if (activeMode == 7) {
        enchantPacketInFlight = true;
    }
    anvilResyncClearedLocalSlots = clearedLocalSlots;
    writeDiagnostic(
        activeMode == 7
        ? "[ENCHANT_RESYNC_PROBE] sent=1 actions=%llu shape=inventory-and-ui-empty-to-empty localPrediction=0 localSlotsCleared=%d"
        : activeAnvilDupe2NbtWindowMode
        ? "[ANVIL_NBT_WINDOW_RESYNC_PROBE] sent=1 actions=%llu shape=inventory-and-minus13-empty-to-empty localPrediction=0 localSlotsCleared=%d"
        : "[ANVILDUPE2_RESYNC_PROBE] sent=1 actions=%llu shape=inventory-empty-to-empty localSlotsCleared=%d",
        static_cast<unsigned long long>(actionCount),
        clearedLocalSlots
    );
    localPlayer->level->getPacketSender()->sendToServer(&packet);
    if (activeMode == 7) {
        enchantPacketInFlight = false;
    }
    resetNativeTransactionItemStacks(packet.getTransaction());
    return true;
}

bool Dupe::sendDuplicateTransaction(
    LocalPlayer* localPlayer,
    ItemStack* renamedItem,
    int destinationSlot
) {
    if (!localPlayer ||
        !localPlayer->level ||
        !renamedItem ||
        !renamedItem->isValid() ||
        destinationSlot < 0 ||
        destinationSlot >= kInventorySlotCount) {
        return false;
    }

    ItemStack* empty = ItemStack::getEmptyItem();
    if (!empty) {
        return false;
    }

    InventoryAction repeatedAnvilResult(
        0,
        renamedItem,
        empty,
        static_cast<ContainerID>(kAnvilResultWindowId),
        InventorySourceType::NonImplementedFeatureTODO,
        InventorySourceFlags::NoFlag
    );

    InventoryAction insertIntoInventory(
        destinationSlot,
        empty,
        renamedItem,
        ContainerID::Inventory,
        InventorySourceType::ContainerInventory,
        InventorySourceFlags::NoFlag
    );

    auto transaction = std::make_unique<ComplexInventoryTransaction>();
    transaction->type =
        ComplexInventoryTransaction::Type::NormalTransaction;
    transaction->data.addAction(repeatedAnvilResult);
    transaction->data.addAction(insertIntoInventory);

    InventoryTransactionPacket packet(std::move(transaction));
    localPlayer->level->getPacketSender()->sendToServer(&packet);

    return true;
}

bool Dupe::sendDropTransaction(
    LocalPlayer* localPlayer,
    ItemStack* item,
    int inventorySlot,
    bool splitInventorySource
) {
    if (!localPlayer ||
        !localPlayer->level ||
        !localPlayer->playerInventory ||
        !localPlayer->playerInventory->container ||
        !item ||
        !item->isValid() ||
        item->mCount == 0 ||
        inventorySlot < kMainInventoryFirstSlot ||
        inventorySlot >= kInventorySlotCount) {
        return false;
    }

    ItemStack* empty = ItemStack::getEmptyItem();
    if (!empty) {
        return false;
    }

    ItemStack droppedItem(*item);
    NativeItemStackResetGuard droppedItemGuard(&droppedItem);
    if (!droppedItem.isValid() || droppedItem.mCount == 0) {
        return false;
    }

    InventoryAction dropIntoWorld(
        0,
        empty,
        &droppedItem,
        ContainerID::None,
        InventorySourceType::WorldInteraction,
        InventorySourceFlags::NoFlag
    );
    NativeInventoryActionResetGuard dropIntoWorldGuard(
        &dropIntoWorld
    );

    auto transaction = std::make_unique<ComplexInventoryTransaction>();
    transaction->type =
        ComplexInventoryTransaction::Type::NormalTransaction;
    transaction->data.addAction(dropIntoWorld);

    if (splitInventorySource && droppedItem.mCount > 1) {
        ItemStack unitItem(droppedItem);
        NativeItemStackResetGuard unitItemGuard(&unitItem);
        unitItem.mCount = 1;
        InventoryAction firstRemove(
            inventorySlot,
            &unitItem,
            empty,
            ContainerID::Inventory,
            InventorySourceType::ContainerInventory,
            InventorySourceFlags::NoFlag
        );
        NativeInventoryActionResetGuard firstRemoveGuard(
            &firstRemove
        );
        const InventorySource inventorySource = firstRemove.source;
        transaction->data.addAction(firstRemove);

        auto group = transaction->data.actions.find(inventorySource);
        if (group == transaction->data.actions.end()) {
            resetNativeTransactionItemStacks(transaction.get());
            return false;
        }
        for (unsigned int index = 1;
            index < static_cast<unsigned int>(droppedItem.mCount);
            ++index) {
            group->second.emplace_back(
                inventorySlot,
                &unitItem,
                empty,
                ContainerID::Inventory,
                InventorySourceType::ContainerInventory,
                InventorySourceFlags::NoFlag
            );
        }
    }
    else {
        InventoryAction removeFromInventory(
            inventorySlot,
            &droppedItem,
            empty,
            ContainerID::Inventory,
            InventorySourceType::ContainerInventory,
            InventorySourceFlags::NoFlag
        );
        NativeInventoryActionResetGuard removeFromInventoryGuard(
            &removeFromInventory
        );
        transaction->data.addAction(removeFromInventory);
    }

    ComplexInventoryTransaction* sentTransaction = transaction.get();
    InventoryTransactionPacket packet(std::move(transaction));
    localPlayer->level->getPacketSender()->sendToServer(&packet);
    resetNativeTransactionItemStacks(sentTransaction);

    ItemStack* localItem =
        localPlayer->playerInventory->container->getItem(inventorySlot);
    const bool predictedLocally =
        localItem &&
        localItem->isValid() &&
        localItem->mCount == droppedItem.mCount;
    if (predictedLocally) {
        localPlayer->playerInventory->container->removeItem(
            inventorySlot,
            static_cast<int>(droppedItem.mCount)
        );
    }

    writeDiagnostic(
        "[DROP_SEND] slot=%d count=%u splitInventorySource=%d localPrediction=%d",
        inventorySlot,
        static_cast<unsigned int>(droppedItem.mCount),
        splitInventorySource ? 1 : 0,
        predictedLocally ? 1 : 0
    );
    return true;
}


// Lock the shared Dupe engine to the anvildupe2 path (fixedMode 1 -> activeMode 5,
// i.e. Dupe's WorkingDupe method) and request the trimmed anvil-only UI.
TryDupe1::TryDupe1()
    : Dupe(
        "AnvilDupe",
        "Anvil duplication (anvildupe2 method)",
        1,      // fixedMode -> internal mode 1 -> activeMode 5
        false,  // nestedOnly
        false,  // publicOnly
        true    // anvilOnly -> trimmed UI
    ) {
}
