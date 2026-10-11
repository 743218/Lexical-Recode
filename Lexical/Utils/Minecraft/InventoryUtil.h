#pragma once
#include "../../SDK/Game.h"

namespace InventoryUtil {
	PlayerInventory* getPlayerInventory();
	Container* getInventory();
	void switchSlot(int32_t slot);
	ItemStack* getItem(int32_t slot);
	int32_t findItemSlotInHotbar(uint16_t itemId);
	int32_t findItemSlotInInventory(uint16_t itemId);
	int getHeldItemId();
	int getItemId(ItemStack* stack);
	void sendMobEquipment(uint16_t slot);
	bool isValid(ItemStack* item);
	bool isBlock(ItemStack* item);
	bool hasItem(int itemId);
	void swapItem(uint8_t from, uint8_t to, bool packet = false);
	void moveItem(uint8_t from, uint8_t to);
	int getItemCooldown(ItemStack* item); // in ticks
	uint8_t getSelectedSlot();
}