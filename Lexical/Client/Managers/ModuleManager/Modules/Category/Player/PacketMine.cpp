#include "PacketMine.h"

PacketMine::PacketMine() : Module("PacketMine", "Mines using packets", Category::PLAYER) {
	registerSetting(new EnumSetting("Packet", "Type of packet\nAuthInput: Uses player auth input packet to mine, good for ignpvp\nPlayerAction: Uses player action packet to mine, good for geyser", { "AuthInput", "PlayerAction" }, &packetMode, 0));
	registerSetting(new SliderSetting<float>("Speed", "Mining speed", &mineSpeed, 1.f, 0.1f, 2.f));
	registerSetting(new SliderSetting<float>("Mine Range", "Mining range", &mineRange, 6.f, 1.f, 12.f));
	registerSetting(new SliderSetting<float>("Continue Reset", "The break progress at the 2nd and more mine if using continue mode", &continueReset, 1.f, 0.f, 1.f));
	registerSetting(new EnumSetting("Mine Type", "Mining Type\nNormal: Mines normally\nContinue: Mines the same block over and over again\nInstant: Instamines all blocks", { "Normal", "Continue", "Instant" }, &mineType, 0));
	registerSetting(new BoolSetting("Double Mine", "Mines 2 blocks", &doubleMine, false, [&]() {
		return packetMode == 1;
		}));
	registerSetting(new KeybindSetting("Mine Bind", "Set this to none to automine", &bind, bind));
	registerSetting(new EnumSetting("MultiTask", "Multitasking mode\nNormal: When eating the client will continue mining but will only break when the player is no longer eating/doing something\nStrict: Breaking will reset when eating/bowing/using an item\nStrict2: Eating will reset when placing, breaking, attacking, using an item\nNone: Mines even if doing something", { "Normal", "Strict", "Strict2", "None" }, &multiTask, 0));
	registerSetting(new BoolSetting("Switch Back", "Switches back to old slot after mining, this may nullify the mine on some servers", &switchBack, false));
}

bool PacketMine::canBreakBlock(const BlockPos& blockPos) {
	if (blockPos == BlockPos(0, 0, 0)) return false;
	if (blockPos.CastTo<float>().dist(Game::getLocalPlayer()->getPos()) > mineRange) return false;
	if (WorldUtil::getBlock(blockPos)->blockLegacy->canBeBuiltOver(Game::clientInstance->getRegion(), blockPos)) return false;
	if (WorldUtil::getBlock(blockPos)->blockLegacy->blockId == 7) return false;
	return true;
}

void PacketMine::reset() {
	LocalPlayer* lp = Game::getLocalPlayer();
	Block* block = WorldUtil::getBlock(BlockPos(0, 0, 0));
	breakPair.first = BlockPos(0, 0, 0);
	breakPair.second = -1;
	lp->gameMode->destroyProgress = 0;
}

bool PacketMine::breakBlock(const BlockPos& blockPos, uint8_t face) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	GameMode* gm = localPlayer->gameMode;
	if (bind != 0x0 && !Game::isKeyDown(bind)) return false;
	if (packetMode == 0) gm->destroyBlock(blockPos, face);
	else {
		Block* block = WorldUtil::getBlock(blockPos);
		std::unique_ptr<ItemUseInventoryTransaction> destroyTransac = ItemUseInventoryTransaction::make_unique();
		destroyTransac->actionType = ItemUseInventoryTransaction::ActionType::Destroy;
		destroyTransac->blockPos = blockPos;
		destroyTransac->targetBlockRuntimeId = *WorldUtil::getBlock(blockPos)->getRuntimeId();
		destroyTransac->face = face;
		destroyTransac->slot = getMineTool(blockPos).second;
		destroyTransac->itemInHand = NetworkItemStackDescriptor(localPlayer->playerInventory->container->getItem(getMineTool(blockPos).second));
		destroyTransac->playerPos = localPlayer->getPos();
		destroyTransac->clickPos = Vec3<float>(0.5f, 0.5f, 0.5f);
		InventoryTransactionPacket itp(std::move(destroyTransac));
		localPlayer->level->getPacketSender()->send(&itp);
	}
	if (mineType != 1) {
		breakPair = std::make_pair(BlockPos(0, 0, 0), -1);
		Game::getLocalPlayer()->gameMode->lastBreakPos = BlockPos(0, 0, 0);
	}
	else Game::getLocalPlayer()->gameMode->destroyProgress = continueReset;
	return true;
}

void PacketMine::mineBlock(const BlockPos& blockPos, uint8_t face) {
	if (!canBreakBlock(blockPos)) return;
	LocalPlayer* lp = Game::getLocalPlayer();
	if (mineType == 2) lp->gameMode->destroyProgress = 1.f;
	else lp->gameMode->destroyProgress = 0.f;
	breakPair = std::make_pair(blockPos, face);
}

std::pair<float, uint8_t> PacketMine::getMineTool(const BlockPos& blockPos) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	GameMode* gm = localPlayer->gameMode;
	Block* block = WorldUtil::getBlock(blockPos);
	uint8_t oldSlot = InventoryUtil::getSelectedSlot();
	uint8_t bestSlot = 0;
	if (block != nullptr) { // this shouldnt be nullptr but....
		float destroyRate = gm->getDestroyRate(block);
		for (uint8_t i = 0; i < 9; i++) {
			InventoryUtil::switchSlot(i);
			const float value = gm->getDestroyRate(block);
			if (value > destroyRate) {
				destroyRate = value;
				bestSlot = i;
			}
		}
		InventoryUtil::switchSlot(oldSlot);
		return std::make_pair(destroyRate, bestSlot);
	}
	return std::make_pair(0.f, 0);
}

void PacketMine::onNormalTick(LocalPlayer* localPlayer) {
	static bool shouldSwitch = false;
	static uint8_t lastSlot = -1;
	GameMode* gm = localPlayer->gameMode;
	if (localPlayer->getItemUseDuration() > 0 && multiTask == 1) {
		gm->destroyProgress = 0.f;
		breakPair = std::make_pair(BlockPos(0, 0, 0), -1);
		return;
	}
	if (localPlayer->getItemUseDuration() > 0 && multiTask == 0) {
		if (PlayerUtil::selectedSlotServerSide != InventoryUtil::getSelectedSlot()) InventoryUtil::sendMobEquipment(InventoryUtil::getSelectedSlot());
	}
	if (PlayerUtil::selectedSlotServerSide != InventoryUtil::getSelectedSlot() && packetMode == 1) InventoryUtil::sendMobEquipment(InventoryUtil::getSelectedSlot());
	if (!canBreakBlock(breakPair.first)) return;
	if (switchBack && shouldSwitch && lastSlot != -1) {
		InventoryUtil::sendMobEquipment(lastSlot);
		lastSlot = -1;
	}
	std::pair<float, uint8_t> bestSlot = getMineTool(breakPair.first);
	if (gm->destroyProgress < 1.f) {
		gm->destroyProgress += bestSlot.first * mineSpeed;
		if (mineType == 2) gm->destroyProgress = 1.f;
		if (gm->destroyProgress > 1.f) gm->destroyProgress = 1.f;
	}
	else {
		if (localPlayer->getItemUseDuration() > 0 && multiTask == 0) return;
		InventoryUtil::sendMobEquipment(bestSlot.second);
		uint8_t oldSlot = InventoryUtil::getSelectedSlot();
		if (PlayerUtil::selectedSlotServerSide == bestSlot.second) breakBlock(breakPair.first, breakPair.second);
		if (switchBack) {
			shouldSwitch = true;
			lastSlot = oldSlot;
		}
	}
}

void PacketMine::onSendPacket(Packet* packet, bool& cancel) {
	if (packetMode != 1) return;
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!localPlayer) return;
	if (packet->getId() == PacketID::PlayerAction) {
		PlayerActionPacket* actionPkt = (PlayerActionPacket*)packet;
		if (actionPkt->mAction == PlayerActionPacket::PlayerAction::StartDestroyBlock) {
			mineBlock(actionPkt->mPos, actionPkt->mFace);
		}
	}
}