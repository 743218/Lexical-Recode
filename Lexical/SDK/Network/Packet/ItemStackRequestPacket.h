#pragma once
#include "Packet.h"

enum class ContainerEnumName : int8_t {
    AnvilInputContainer = 0,
    AnvilMaterialContainer = 1,
    AnvilResultPreviewContainer = 2,
    SmithingTableInputContainer = 3,
    SmithingTableMaterialContainer = 4,
    SmithingTableResultPreviewContainer = 5,
    ArmorContainer = 6,
    LevelEntityContainer = 7,
    BeaconPaymentContainer = 8,
    BrewingStandInputContainer = 9,
    BrewingStandResultContainer = 10,
    BrewingStandFuelContainer = 11,
    CombinedHotbarAndInventoryContainer = 12,
    CraftingInputContainer = 13,
    CraftingOutputPreviewContainer = 14,
    RecipeConstructionContainer = 15,
    RecipeNatureContainer = 16,
    RecipeItemsContainer = 17,
    RecipeSearchContainer = 18,
    RecipeSearchBarContainer = 19,
    RecipeEquipmentContainer = 20,
    RecipeBookContainer = 21,
    EnchantingInputContainer = 22,
    EnchantingMaterialContainer = 23,
    FurnaceFuelContainer = 24,
    FurnaceIngredientContainer = 25,
    FurnaceResultContainer = 26,
    HorseEquipContainer = 27,
    HotbarContainer = 28,
    InventoryContainer = 29,
    ShulkerBoxContainer = 30,
    TradeIngredient1Container = 31,
    TradeIngredient2Container = 32,
    TradeResultPreviewContainer = 33,
    OffhandContainer = 34,
    CompoundCreatorInput = 35,
    CompoundCreatorOutputPreview = 36,
    ElementConstructorOutputPreview = 37,
    MaterialReducerInput = 38,
    MaterialReducerOutput = 39,
    LabTableInput = 40,
    LoomInputContainer = 41,
    LoomDyeContainer = 42,
    LoomMaterialContainer = 43,
    LoomResultPreviewContainer = 44,
    BlastFurnaceIngredientContainer = 45,
    SmokerIngredientContainer = 46,
    Trade2Ingredient1Container = 47,
    Trade2Ingredient2Container = 48,
    Trade2ResultPreviewContainer = 49,
    GrindstoneInputContainer = 50,
    GrindstoneAdditionalContainer = 51,
    GrindstoneResultPreviewContainer = 52,
    StonecutterInputContainer = 53,
    StonecutterResultPreviewContainer = 54,
    CartographyInputContainer = 55,
    CartographyAdditionalContainer = 56,
    CartographyResultPreviewContainer = 57,
    BarrelContainer = 58,
    CursorContainer = 59,
    CreatedOutputContainer = 60
};
class ItemStackNetIdVariant {
public:
    int id1;
private:
    char pad1[4];
public:
    int id2;
private:
    char pad2[4];
public:
    uint8_t type;
private:
    char pad3[7];
public:
    __forceinline int getId() {
        if (type == 0) return id1;
        else if (type <= 2) return id2;
        return 0;
    }
    __forceinline void setId(int id) {
        if (type == 0) id1 = id;
        else if (type <= 2) id2 = id;
    }
};
class ItemStackRequestSlotInfo {
public:
    ContainerEnumName mContainerName;
    uint8_t slot;
private:
    char pad[6];
public:
    ItemStackNetIdVariant networkStackId;
};
enum ItemStackRequestActionType : uint8_t {
    Take = 0x0,
    Place = 0x1,
    Swap = 0x2,
    Drop = 0x3,
    Destroy = 0x4,
    Consume = 0x5,
    Create = 0x6,
    PlaceInItemContainer = 0x7,
    TakeFromItemContainer = 0x8,
    ScreenLabTableCombine = 0x9,
    ScreenBeaconPayment = 0xA,
    ScreenHUDMineBlock = 0xB,
    CraftRecipe = 0xC,
    CraftRecipeAuto = 0xD,
    CraftCreative = 0xE,
    CraftRecipeOptional = 0xF,
    CraftRepairAndDisenchant = 0x10,
    CraftLoom = 0x11,
    CraftNonImplemented_DEPRECATEDASKTYLAING = 0x12,
    CraftResults_DEPRECATEDASKTYLAING = 0x13,
    OnRender = 0x14,
};

class ItemStackRequestAction {
public:
	uintptr_t vTable;
	ItemStackRequestActionType mType;
};

class ItemStackRequestActionTransferBase : public ::ItemStackRequestAction {};
class ItemStackRequestActionSwap : public ItemStackRequestActionTransferBase {
private:
    uintptr_t one;
public:
    ItemStackRequestSlotInfo slot1;
    ItemStackRequestSlotInfo slot2;
public:
};

struct ItemStackRequestData {
public:
	uintptr_t vTable;
	uint32_t mCounter;
	uint32_t zero;
	uintptr_t zeroes[3];
	uintptr_t idk;
	std::vector<ItemStackRequestAction*> mActions;
};

class ItemStackRequestBatch {
public:
	std::vector<ItemStackRequestData*> requests;
};

class ItemStackRequestPacket : public Packet {
public:
	ItemStackRequestBatch* mRequestBatch;
};