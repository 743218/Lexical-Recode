#include "InventoryUtil.h"

PlayerInventory* InventoryUtil::getPlayerInventory() {
	return Game::getLocalPlayer()->playerInventory;
}

Container* InventoryUtil::getInventory() {
	return getPlayerInventory()->container;
}
void InventoryUtil::switchSlot(int32_t slot) {
	Game::getLocalPlayer()->playerInventory->selectedSlot = slot;
}

uint8_t InventoryUtil::getSelectedSlot() {
	return Game::getLocalPlayer()->playerInventory->selectedSlot;
}

ItemStack* InventoryUtil::getItem(int32_t slot) {
	return Game::getLocalPlayer()->playerInventory->container->getItem(slot);
}

bool InventoryUtil::isValid(ItemStack* item) {
	if (!item) return false;
	if (!item->isValid()) return false;
	if (item->item.get() == nullptr) return false;
	if (item == ItemStack::getEmptyItem()) return false;
	return true;
}

bool InventoryUtil::hasItem(int itemId) {
	for (int i = 0; i < 9; i++) {
		ItemStack* stack = getItem(i);
		if (isValid(stack)) {
			if (stack->item.get()->itemId == itemId) return true;
		}
	}
	return false;
}

int32_t InventoryUtil::findItemSlotInHotbar(uint16_t itemId) {
	int32_t result = -1;
	for (int i = 0; i < 9; i++) {
		ItemStack* itemStack = Game::getLocalPlayer()->playerInventory->container->getItem(i);
		if (itemStack->isValid() && itemStack->item.get()->itemId == itemId) {
			result = i;
			break;
		}
	}
	return result;
}

int32_t InventoryUtil::findItemSlotInInventory(uint16_t itemId) {
	int32_t result = -1;
	for (int i = 0; i < 36; i++) {
		ItemStack* itemStack = Game::getLocalPlayer()->playerInventory->container->getItem(i);
		if (itemStack->isValid() && itemStack->item.get()->itemId == itemId) {
			result = i;
			break;
		}
	}
	return result;
}

int InventoryUtil::getHeldItemId() {
	return getItemId(getItem(getSelectedSlot()));
}

int InventoryUtil::getItemId(ItemStack* stack) {
	if (stack == nullptr) return 0;
	if (stack->item.get() == nullptr) return 0;
	return stack->item->itemId;
}

void InventoryUtil::sendMobEquipment(uint16_t slot) {
	if (getItem(slot) == nullptr) return; //  wow it crashes when itemstack is null(when slot is empty)
	MobEquipmentPacket mep(Game::getLocalPlayer()->getRuntimeIDComponent()->runtimeId, InventoryUtil::getItem(slot), slot, slot);
	Game::getLocalPlayer()->level->getPacketSender()->send(&mep);
}

bool InventoryUtil::isBlock(ItemStack* item) {
	if (!item) return false;
	return item->isBlock();
}

void InventoryUtil::swapItem(uint8_t from, uint8_t to, bool packet) {
	InventoryTransactionManager* invMgr = Game::getLocalPlayer()->getTransactionManager();
	ItemStack* fromStack = getItem(from);
	ItemStack* toStack = getItem(to);
	InventoryAction first(from, fromStack, ItemStack::getEmptyItem());
	InventoryAction second(to, toStack, fromStack);
	InventoryAction third(from, ItemStack::getEmptyItem(), toStack);
	if (!packet) {
		invMgr->addAction(first);
		invMgr->addAction(second);
		invMgr->addAction(third);
	}
	else {
		std::unique_ptr<ComplexInventoryTransaction> cit = std::make_unique<ComplexInventoryTransaction>();
		cit->type = ComplexInventoryTransaction::Type::NormalTransaction;
		cit->data.addAction(first);
		cit->data.addAction(second);
		cit->data.addAction(third);
		InventoryTransactionPacket pk(std::move(cit));
		Game::getLocalPlayer()->level->getPacketSender()->send(&pk);
	}
}

void InventoryUtil::moveItem(uint8_t from, uint8_t to) {
	getPlayerInventory()->container->setItem(to, *getItem(from));
	getPlayerInventory()->container->removeItem(from, getItem(from)->mCount);
}

int InventoryUtil::getItemCooldown(ItemStack* stack) {
	if (!stack) return 250;
	const std::string& name = stack->item.get()->texture_name;
	if (name.find("axe") != std::string::npos) {
		if (name.find("wooden") != std::string::npos || name.find("stone") != std::string::npos) return 1250;
		else if (name.find("diamond") != std::string::npos || name.find("netherite") != std::string::npos || name.find("gold") != std::string::npos) return 1000;
		else return 1100;
	}
	else if (name.find("sword") != std::string::npos) return 600;
	else if (name.find("pickaxe") != std::string::npos) return 850;
	else if (name.find("hoe") != std::string::npos) return 500;
	else if (name.find("shovel") != std::string::npos) return 1000;
	else if (stack->item.get()->itemId == 326) return 1500;
	return 250;
}