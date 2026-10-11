#pragma once

#include "../../ModuleBase/Module.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct AnvilStackRequestDataView;
struct AnvilStackRequestBatchView;
class ComplexInventoryTransaction;
class InventoryTransactionPacket;
class InventoryAction;
class Item;

// Kirara's Dupe engine (anvil / dye / enchant duplication). Ported without the Aether,
// mapart and saved-NBT features: those members remain so the shared engine compiles,
// but mode 5 is not selectable and the NBT shims in DupeCompat.h always report "unavailable".
class Dupe : public Module {
public:
    Dupe();

    bool isHoldMode() override { return false; }
    void onEnable() override;
    void onDisable() override;
    void onNormalTick(LocalPlayer* localPlayer) override;
    void onSendPacket(Packet* packet, bool& cancel) override;
    void onRecievePacket(Packet* packet, bool& cancel) override;

protected:
    Dupe(
        const char* moduleName,
        const char* moduleDescription,
        int fixedMode,
        bool nestedOnly,
        bool publicOnly,
        bool anvilOnly = false
    );

    virtual bool rewriteDedicatedNestedTransaction(
        InventoryTransactionPacket* packet,
        int& addedDestinationCount
    );
    bool rewriteNestedAnvilTransaction(
        InventoryTransactionPacket* packet,
        int& addedDestinationCount
    );
    bool useNestedDyeDropCarrier() const {
        return activeNestedCarrierMode == 1;
    }

private:
    enum class DyeRewriteStrategy {
        OldInventory,
        NewInventory,
        DirectWorld,
        NestedBox,
        DirectInventory,
        VirtualInventory,
        VirtualWorld
    };

    enum class Stage {
        WaitForRenameTake,
        WaitForRenamedItem,
        FillInventory,
        Completed,
        DropInventory,
        DropResponse,
        ReadAnvilMaterial,
        ExtractAnvilOutput,
        WaitForNbtWindowAck,
        ExtractNbtWindow
    };

    enum class EnchantSequencePhase {
        Idle,
        Preload,
        Complete
    };

    // ---- settings
    int mode = 0;
    int activeMode = 0;
    int copyCount = 1;
    int dyeCopyCount = 27;
    int dupe3CopyCount = 27;
    int dupe3Destination = 0;
    int boxInBoxCopyCount = 1;
    bool nestedAllCrafting = false;
    int enchantCopyCount = 2;
    int enchantDonorSlot = 8;
    int anvilDupe2CopyCount = 27;
    bool anvilDupe2SingleItem = false;
    bool anvilDupe2NbtWindowMode = false;
    bool anvilDupe2SourceBoxMode = false;
    int anvilDupe2SingleNbtExperiment = 3;
    int anvilDupe2InnerItemLock = 0;
    bool anvilDupe2MaterialSlotMode = false;
    int anvilDupe2Destination = 1;
    int anvilSwapSourceSlot = 0;
    bool craftDropStack = false;
    bool stacking = false;
    bool itemStackRequestStacking = false;
    int stackCount = 64;
    bool dropInventory = false;
    bool autoRepeat = true;
    bool chatDisplay = true;
    int nestedSourceSlot = 0;
    int nestedCarrierMode = 0;
    int anvilDupe3CopyCount = 27;
    bool anvilDirectDrop = false;
    bool anvilDupe3Drop = false;
    bool nestedOnlyModule = false;
    bool publicOnlyModule = false;
    bool anvilOnlyModule = false;
    int anvilModeDisplay = 0;

    // ---- Aether / mapart (not ported; kept so the shared engine compiles, never enabled)
    int aetherCraftAmount = 1;
    int aetherCarrierMode = 0;
    bool aetherMapart = false;
    bool aetherBoxInBox = false;
    int aetherBoxMode = 0;
    int aetherBoxCount = 1;
    bool aetherDefaultNbt = false;
    bool aetherEnchantItems = false;
    int aetherEnchantEditMode = 0;
    int aetherInnerItemLock = 0;
    std::string aetherOuterName;
    std::vector<std::string> aetherBoxColorOptions;
    std::vector<bool> selectedAetherBoxColors;
    std::vector<bool> selectedAetherBoxSlots;
    std::vector<int> aetherBoxSourceSlots;
    std::vector<int> aetherBoxSlotColors;
    std::vector<int> aetherBoxSlotCounts;
    std::vector<std::string> aetherBoxSlotNames;
    std::vector<bool> selectedAetherEnchantments;
    std::vector<int> aetherEnchantIds;
    std::vector<int> aetherEnchantLevels;
    int mapartTilesX = 2;
    int mapartTilesY = 2;
    bool mapartReduceColors = false;
    bool mapartLoadPending = false;
    bool nestedGeneratePending = false;
    int mapartLoadTicks = 0;
    bool mapartNbtRequested = false;
    bool mapartNbtReady = false;
    std::string aetherNbtLoadName;
    bool aetherUsingDefaultNbt = false;
    bool aetherCraftResyncPending = false;
    bool aetherAnvilCarrierActive = false;

    // ---- latched copies of settings for the running attempt
    int activeCopyCount = 1;
    bool activeStacking = false;
    int activeStackCount = 1;
    int activeNestedSourceSlot = 0;
    int activeNestedCarrierMode = 0;
    bool activeAnvilDirectDrop = false;
    bool activeAnvilDupe3Drop = false;
    bool activeAnvilDupe2MaterialSlotMode = false;
    bool activeAnvilDupe2NbtWindowMode = false;
    bool activeAnvilDupe2SingleItem = false;
    bool activeAnvilDupe2SourceBoxMode = false;
    int activeAnvilDupe2SingleNbtExperiment = 3;
    int activeAnvilDupe2InnerItemLock = 0;
    int activeAnvilDupe2Destination = 1;
    int activeAnvilSwapSourceSlot = 0;
    bool activeCraftDropStack = false;
    bool activeItemStackRequestStacking = false;
    bool activeNestedAllCrafting = false;
    int activeEnchantDonorSlot = 8;
    int activeDupe3Destination = 0;
    bool activeDupe3AnyCrafting = false;
    int activeDupe4Destination = 0;
    bool activeDupe4AnyCrafting = false;

    // dyedupe4 (internal mode 6, no UI entry in this build)
    int dupe4CopyCount = 27;
    int dupe4Destination = 0;
    bool dupe4AnyCrafting = false;

    // ---- helpers
    void refreshMaparts();
    std::string getSelectedMapart() const;
    void refreshDefaultNbtFiles();
    std::string getSelectedDefaultNbt() const;
    int getSelectedAetherBoxColor() const;
    bool generateNestedNbt(LocalPlayer* localPlayer);
    void displayMessage(const char* format, ...) const;
    void resetAttemptState();
    void rememberExpectedResult(const ItemStack& item);
    int refreshExpectedDestinations(LocalPlayer* localPlayer);
    bool matchesExpectedResult(const ItemStack* item, uint8_t expectedCount) const;
    bool captureNestedSourceBox(LocalPlayer* localPlayer);
    bool prepareNativeNestedCraftingResult(ItemStack& nativeResult);
    int prepareEarlyNestedAction(const InventoryAction& action, ItemStack& replacement);
    int prepareEarlySourceBoxAction(const InventoryAction& action, ItemStack& replacement);
    bool finalizeEarlyNestedDescriptors(InventoryTransactionPacket* packet);
    void traceSourceBoxStackRequests(Packet* packet);
    bool rewriteItemStackRequest(Packet* packet);
    void observeBatch(const AnvilStackRequestBatchView* batch);
    bool observeRequest(const AnvilStackRequestDataView* request);
    void armInventoryCapture(int inventorySlot);
    void snapshotEmptyInventoryForCursor();
    bool tryCaptureCursorPlacementFallback(LocalPlayer* localPlayer);
    std::vector<int> findEmptyInventorySlots(LocalPlayer* localPlayer) const;
    void observeEnchantSlots(ComplexInventoryTransaction* transaction);
    void observeDyeCraftIngredients(ComplexInventoryTransaction* transaction);
    bool rewriteOldDyeCraftTransaction(InventoryTransactionPacket* packet, int& addedDestinationCount);
    bool rewriteNewDyeCraftTransaction(InventoryTransactionPacket* packet, int& addedDestinationCount);
    bool rewriteDyeDupe3Transaction(InventoryTransactionPacket* packet, int& addedDestinationCount);
    bool rewriteDyeDupe4Transaction(InventoryTransactionPacket* packet, int& addedDestinationCount);
    bool rewriteBoxInBoxTransaction(InventoryTransactionPacket* packet, int& addedDestinationCount);
    bool rewriteAetherCraftTransaction(InventoryTransactionPacket* packet, int& addedDestinationCount);
    bool rewriteDyeCraftTransaction(InventoryTransactionPacket* packet, int& addedDestinationCount, DyeRewriteStrategy strategy);
    bool rewriteAnvilTransaction(InventoryTransactionPacket* packet, int& addedDestinationCount);
    bool sendEnchantDupeTransaction(LocalPlayer* localPlayer);
    bool sendNbtWindowTakeTransaction(LocalPlayer* localPlayer);
    bool sendAnvilMaterialProbe(LocalPlayer* localPlayer);
    bool sendInventoryResyncProbe(LocalPlayer* localPlayer);
    bool sendDuplicateTransaction(LocalPlayer* localPlayer, ItemStack* renamedItem, int destinationSlot);
    bool sendDropTransaction(LocalPlayer* localPlayer, ItemStack* item, int inventorySlot, bool splitInventorySource);

    // ---- runtime state
    Stage stage = Stage::WaitForRenameTake;
    int renamedInventorySlot = -1;
    int settleTicks = 0;
    int captureWaitTicks = 0;
    int packetCooldownTicks = 0;
    int sentTransactionCount = 0;
    bool renameNoticePending = false;
    bool rewriteNoticePending = false;
    bool nestedAbortNoticePending = false;
    bool nestedRewriteMatchedAndRejected = false;
    int rewrittenDestinationCount = 0;
    int legacyPacketCount = 0;
    int enchantOutgoingDiagnosticCount = 0;
    int sourceBoxRequestDiagnosticCount = 0;
    int responseWaitTicks = 0;
    int incomingDiagnosticCount = 0;
    bool anvilResyncProbeSent = false;
    bool anvilResyncAuthoritativePacketReceived = false;
    int anvilPreTransactionOwnedCount = 0;
    int nextDropSlot = 9;
    int dropTransactionCount = 0;
    int dropFailureCount = 0;
    bool sawDyeShulkerIngredient = false;
    bool sawDyeOtherIngredient = false;
    bool sawNamedDyeIngredient = false;
    std::unique_ptr<ItemStack> cachedDyeShulkerIngredient;
    std::unique_ptr<ItemStack> cachedNestedSourceBox;
    std::unique_ptr<ItemStack> cachedEarlyNestedResult;
    bool nestedEarlyResultInjected = false;
    std::unique_ptr<ItemStack> cachedEnchantInput;
    std::unique_ptr<ItemStack> cachedEnchantMaterial;
    std::unique_ptr<ItemStack> cachedEnchantDonor;
    std::unique_ptr<ItemStack> expectedResultStack;
    std::unique_ptr<ItemStack> stagedAnvilOutputStack;
    std::unique_ptr<ItemStack> nestedLocalShellStack;
    int stagedAnvilDestinationSlot = -1;
    bool anvilOutputStagePending = false;
    bool nbtWindowStagePending = false;
    bool nbtWindowFirstStageAccepted = false;
    bool nbtWindowSecondStageSent = false;
    bool anvilMaterialProbeSent = false;
    bool anvilMaterialInventoryPacketObserved = false;
    int anvilMaterialInventoryPacketCount = 0;
    int anvilMaterialReadbackCheckedPacketCount = 0;
    int anvilMaterialReadbackState = 0;
    bool anvilMaterialAuthoritativePacketReceived = false;
    bool anvilMaterialNativeTakeRequested = false;
    bool anvilMaterialNativeTakePacketObserved = false;
    int anvilMaterialNativeTakePacketId = 0;
    bool anvilMaterialReadbackGraceUsed = false;
    bool anvilMaterialPreTakeSnapshotValid = false;
    std::array<uint8_t, 36> anvilMaterialPreTakeCounts{};
    std::array<bool, 36> anvilMaterialPreTakeExpectedMatches{};
    std::array<bool, 36> anvilMaterialPreTakeEmptySlots{};
    bool anvilResyncInventoryPacketObserved = false;
    int anvilResyncInventoryPacketCount = 0;
    int anvilResyncReadbackState = 0;
    int anvilResyncClearedLocalSlots = 0;
    int enchantSettleTicks = 0;
    bool enchantTransactionPending = false;
    bool enchantPacketInFlight = false;
    EnchantSequencePhase enchantSequencePhase = EnchantSequencePhase::Idle;
    std::vector<int> enchantSequenceDestinationSlots;
    std::vector<int> enchantSequenceCompletedSlots;
    std::size_t enchantSequenceIndex = 0;
    int enchantSequenceInitialMaterialCount = 0;
    std::array<uint8_t, 36> expectedDestinationCounts{};
    Item* expectedResultItem = nullptr;
    short expectedResultAux = 0;
    uint8_t expectedResultCount = 0;
    bool waitingForCursorPlacement = false;
    bool cursorSnapshotValid = false;
    std::array<bool, 36> emptyBeforeCursor{};
    std::vector<int> pendingDestinationSlots;
    std::vector<int> rewrittenDestinationSlots;
    std::size_t nextDestinationIndex = 0;
};