#include "HotbarSorter.h"

using namespace InventoryUtil;

HotbarSorter::HotbarSorter() : Module("HotbarSorter", "Automatically sorts items", Category::PLAYER) {
	registerSetting(new SliderSetting<int>("End Crystal", "NULL", &endCrystalSlot, 0, 0, 9));
	registerSetting(new SliderSetting<int>("Respawn Anchor", "NULL", &anchorSlot, 0, 0, 9));
	registerSetting(new SliderSetting<int>("Golden Apple", "NULL", &gappleSlot, 0, 0, 9));
	registerSetting(new SliderSetting<int>("Stacked Bed", "NULL", &bedSlot, 0, 0, 9));
	registerSetting(new SliderSetting<int>("Normal Bed", "NULL", &nBedSlot, 0, 0, 9));
	registerSetting(new SliderSetting<int>("Obsidian", "NULL", &obsidianSlot, 0, 0, 9));
	registerSetting(new SliderSetting<int>("Glowstone", "NULL", &glowstoneSlot, 0, 0, 9));
	registerSetting(new SliderSetting<int>("Ender Chest", "NULL", &echest, 0, 0, 9));
	registerSetting(new SliderSetting<int>("Strength", "NULL", &strength, 0, 0, 9));
}

void HotbarSorter::onNormalTick(LocalPlayer* localPlayer) {
	static int sentPackets = 0;
	static int delay = 0;
	std::unordered_map<uint8_t, int> itemMap = {
		{endCrystalSlot, 758},
		{bedSlot, 26},
		{gappleSlot, 263},
		{obsidianSlot, 49},
		{glowstoneSlot, 89},
		{anchorSlot, 65264},
		{nBedSlot, 255},
		{echest, 130},
		{strength, 435}
	};

	for (int i = 9; i < 36; i++) {
		for (int j = 0; j < 9; j++) {
			ItemStack* invStack = getItem(i);
			ItemStack* hotbarStack = getItem(j);
			if (isValid(invStack)) {
				for (std::pair itemPair : itemMap) {
					if ((j == (itemPair.first - 1) && getItemId(hotbarStack) == 0 && getItemId(invStack) == itemPair.second) && itemPair.first != 0) {
						moveItem(i, j);
						break;
					}
				}
			}
		}
	}
}