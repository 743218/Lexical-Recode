#pragma once
// Shims so Kirara's Dupe.cpp builds here without its NBT-file / mapart stack.
// Everything reports "unavailable", so every path that needs it bails out through
// the checks Dupe.cpp already has (isExecutableAddress, hasLoadedTag, ...).

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../../../../../SDK/Addresses.h"
#include "../../../../../../SDK/NBT/ShulkerNBT.h"

// No signatures for these in this client. 0 fails isExecutableAddress(), which gates every use.
namespace Addresses {
    inline uintptr_t CompoundTag_putByte = 0;
    inline uintptr_t ItemStack_loadItem = 0;
    inline uintptr_t ItemStackBase_save = 0;
}

// Kirara's ItemStack::loadItem(CompoundTag) wraps Addresses::ItemStack_loadItem, which this client
// lacks. Every caller is gated on isExecutableAddress(ItemStack_loadItem); if one ever slips past,
// the stack stays empty and the caller's isValid() check aborts.
template <typename Stack, typename Nbt>
inline void dupeLoadItem(Stack&, const Nbt&) {}

// Stand-in for Kirara's .nbt command: nothing is ever loaded or saved.
namespace NbtCommand {
    struct LegacyItemEntry {
        CompoundTag* itemEntry = nullptr;
        uint8_t slot = 0;
        std::string modernName;
        int16_t legacyId = 0;
        int16_t legacyDamage = 0;
        bool preserveIdentity = false;
        uint8_t count = 1;
        std::string customName;
    };
    struct EnchantmentEdit {
        int16_t id = 0;
        int16_t level = 0;
    };

    inline bool* statusMessagesSetting() { static bool value = false; return &value; }
    inline bool hasLoadedTag() { return false; }
    inline std::unique_ptr<CompoundTag> copyLoadedTag() { return nullptr; }
    inline const std::string& loadedName() { static const std::string empty; return empty; }
    inline std::vector<std::string> listSavedNbtFiles() { return {}; }
    inline bool saveLegacyItemEntriesJson(const std::string&, const std::vector<LegacyItemEntry>&) { return false; }
    inline bool rewriteSavedRootCustomName(const std::string&, const std::string&, const std::string&) { return false; }
    inline bool rewriteSavedEnchantments(const std::string&, const std::string&, const std::vector<EnchantmentEdit>&, int) { return false; }
    inline bool saveTagJson(const std::string&, const CompoundTag&) { return false; }
    inline bool replaceLoadedTag(std::unique_ptr<CompoundTag>, const std::string&) { return false; }
}

// Mapart generation is not ported.
namespace NbtMap {
    inline std::vector<std::string> listMaparts() { return {}; }
    inline bool generateMapart(const std::string&, int, int, bool) { return false; }
}