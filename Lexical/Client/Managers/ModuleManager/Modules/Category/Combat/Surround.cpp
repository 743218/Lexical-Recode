#include "Surround.h"

Surround::Surround() : Module("Surround", "Surrounds you in blocks to prevent massive crystal damage", Category::COMBAT) {
	registerSetting(new SliderSetting<int>("Place Delay", "Delay, in ticks, between block placements", &placeDelay, 1, 0, 20));
	registerSetting(new SliderSetting<int>("Blocks Per Tick", "How many blocks to place each tick", &placePerTick, 1, 1, 10));
	registerSetting(new BoolSetting("Air Place", "Place on air\n(Not recommanded to enable this!)", &airPlace, false));
	registerSetting(new BoolSetting("Packet Place", "Send packet to server to place\n(Placements can be slower but can prevents ghost blocks)", &packetPlace, false));
	registerSetting(new BoolSetting("Rotate", "Rotates to placement", &rotate, true));
	registerSetting(new BoolSetting("Center", "Teleports you to the center of the block", &center, true));
	registerSetting(new BoolSetting("Dynamic", "Normally surround only place 1x1 around you\nThis setting will make surround auto place fit your hitbox", &dynamic, true));
	registerSetting(new BoolSetting("Nukkit AABB", "Nukkit aabb is fatter so this will make the dynamic mode adjust for it", &nukkitAABB, true));
	registerSetting(new BoolSetting("Auto Disable", "Auto disable module after finish surround", &disableOnComplete, false));
	registerSetting(new BoolSetting("Jump Disable", "Auto disable module when jump", &disableOnJump, true));
	registerSetting(new BoolSetting("No MultiTask", "Stop when eating", &eatStop, false));
	registerSetting(new BoolSetting("Crystal Check", "Attacks end crystals", &crystalCheck, true));
	registerSetting(new BoolSetting("Crystal Blocker", "NULL", &crystalBlocker, false));
	registerSetting(new BoolSetting("Button", "Places button below", &button, true));
	registerSetting(new BoolSetting("E-Chest", "Places echest below", &echest, true));
	registerSetting(new EnumSetting("Switch", "Switch to obsidian", { "None", "Normal", "SwitchBack", "Spoof", "Packet" }, &switchMode, 2));
	registerSetting(new BoolSetting("Render", "Renders a block overlay where the obsidian will be placed", &render, true));
	registerSetting(new ColorSetting("Color", "NULL", &color, UIColor(125, 0, 255), false));
	registerSetting(new SliderSetting<int>("Alpha", "NULL", &alpha, 40, 0, 255));
	registerSetting(new SliderSetting<int>("LineAlpha", "NULL", &lineAlpha, 90, 0, 255));
	registerSetting(new BoolSetting("Fade", "Fade animation for render", &fade, false));
}

void Surround::onDisable() {
	placeList.clear();
	fadeBlockList.clear();
	shouldSurround = false;
	if (oldSlot != -1) {
		LocalPlayer* localPlayer = Game::getLocalPlayer();
		if (localPlayer != nullptr)
			localPlayer->playerInventory->selectedSlot = oldSlot;
		oldSlot = -1;
	}
	wasSurrounding = false;
}

void Surround::onEnable() {
	placeList.clear();
	shouldSurround = false;
	oldSlot = -1;
	wasSurrounding = false;

	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr)
		return;

	if (center) {
		Vec3<float> playerPos = localPlayer->getPos();
		Vec3<float> centerPos = Vec3<float>(floor(playerPos.x), playerPos.y, floor(playerPos.z));
		centerPos.x += 0.5f;
		centerPos.z += 0.5f;

		localPlayer->setPos(centerPos);
	}
	shouldSurround = true;
}

void Surround::onNormalTick(LocalPlayer* localPlayer) {
	static std::vector<Actor*> entityList;
	entityList.clear();
	entityList.push_back(localPlayer);
	for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
		if (!TargetUtil::isTargetValid(actor, true, false, 6.f)) continue;
		if (actor->getActorTypeComponent()->id == 71) continue; // attack crystal, no need for making dynamic if entyt is crystal
		entityList.push_back(actor);
	}
	if (eatStop && localPlayer->getItemUseDuration() > 0) return;
	if (!shouldSurround)
		return;

	if (disableOnJump && Game::canUseMoveKeys() && Game::isKeyDown(VK_SPACE)) {
		this->setEnabled(false);
		return;
	}
	BlockSource* region = Game::clientInstance->getRegion();
	int obsidianSlot = InventoryUtil::findItemSlotInHotbar(49);
	if (obsidianSlot == -1) {
		this->setEnabled(false);
		return;
	}

	if (!placeList.empty()) {
		placeList.clear();
	}
	else {
		if (wasSurrounding) {
			wasSurrounding = false;
			if (disableOnComplete) {
				this->setEnabled(false);
				return;
			}
		}
	}
	if (entityList.empty()) return;
	if (crystalCheck) {
		for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
			if (actor == nullptr) continue;
			if (actor->getActorTypeComponent()->id != 71) continue;
			if (actor->getEyePos().dist(localPlayer->getEyePos()) > 4.f) continue;
			localPlayer->gameMode->attack(actor);
		}
	}
	AABB localAABB = localPlayer->aabbShape->aabb;
	Vec3<float> feetPos = localAABB.getCenter();
	feetPos.y = localAABB.lower.y;

	BlockPos currentPos = feetPos.add(Vec3<float>(0.f, 0.5f, 0.f)).floor().CastTo<int>();

	int xStart = -1;
	int zStart = -1;
	int xEnd = 1;
	int zEnd = 1;

	if (dynamic) {
		for (Actor* actor : entityList) {
			AABB actorAABB = actor->aabbShape->aabb;
			if (actor->getActorTypeComponent()->id == 319) {
				actorAABB = TargetUtil::makeAABB(0.6f, actor->aabbShape->height, actor->getEyePos());
				if (nukkitAABB) {
					actorAABB.lower = actorAABB.lower.sub(0.1f, 0.f, 0.1f);
					actorAABB.upper = actorAABB.upper.add(0.1f, 0.f, 0.1f);
				}
			}
			if (actorAABB.intersects(getBlockAABB(currentPos.add(BlockPos(xStart, 0, 0))))) xStart -= 1;
			if (actorAABB.intersects(getBlockAABB(currentPos.add(BlockPos(0, 0, zStart))))) zStart -= 1;
			if (actorAABB.intersects(getBlockAABB(currentPos.add(BlockPos(xEnd, 0, 0))))) xEnd += 1;
			if (actorAABB.intersects(getBlockAABB(currentPos.add(BlockPos(0, 0, zEnd))))) zEnd += 1;
		}
	}

	for (int x = xStart; x <= xEnd; x++) {
		for (int z = zStart; z <= zEnd; z++) {
			if (x == xStart && z == zStart) continue;
			if (x == xStart && z == zEnd) continue;
			if (x == xEnd && z == zEnd) continue;
			if (x == xEnd && z == zStart) continue;
			if (x > xStart && x < xEnd && z > zStart && z < zEnd) {
				const BlockPos& placePos = currentPos.add(BlockPos(x, -1, z));
				if (PlayerUtil::canPlaceBlock(placePos, airPlace)) placeList.push_back(placePos);
				continue;
			}

			const BlockPos& placePos = currentPos.add(BlockPos(x, 0, z));
			const BlockPos& blockBelow = BlockPos(placePos.x, placePos.y - 1, placePos.z);

			if (!region->getBlock(placePos)->blockLegacy->canBeBuiltOver(region, placePos)) continue;

			if (PlayerUtil::canPlaceBlock(placePos, airPlace)) placeList.push_back(placePos);
			else if (PlayerUtil::canPlaceBlock(blockBelow, airPlace)) placeList.push_back(blockBelow);
		}
	}
	oldSlot = InventoryUtil::getSelectedSlot();
	if (echest) {
		const BlockPos& chestPos = localPlayer->getPos().floor().sub(0.f, 2.f, 0.f).CastTo<int>();
		uint8_t chestSlot = InventoryUtil::findItemSlotInHotbar(143);
		if (switchMode != 0) {
			if (switchMode != 4) InventoryUtil::switchSlot(chestSlot);
			if (switchMode == 4 && PlayerUtil::selectedSlotServerSide != chestSlot) InventoryUtil::sendMobEquipment(chestSlot);
		}
		PlayerUtil::tryPlaceBlock(chestPos, airPlace, packetPlace);
	}
	if (!placeList.empty()) {
		std::sort(placeList.begin(), placeList.end(), [localPlayer](const BlockPos& a1, const BlockPos& a2) {
			AABB playerAABB = localPlayer->aabbShape->aabb;
			Vec3<float> feetPos = playerAABB.getCenter();
			feetPos.y = playerAABB.lower.y;
			return (WorldUtil::distanceToBlock(feetPos, a1) < WorldUtil::distanceToBlock(feetPos, a2));
			}
		);
		if (switchMode != 0) {
			if (switchMode != 4) InventoryUtil::switchSlot(obsidianSlot);
			if (switchMode == 4 && PlayerUtil::selectedSlotServerSide != obsidianSlot) InventoryUtil::sendMobEquipment(obsidianSlot);
		}

		if (InventoryUtil::getSelectedSlot() == obsidianSlot || switchMode == 4) {
			static int oTick = 0;
			if (oTick >= placeDelay) {
				int placed = 0;
				if (crystalBlocker) {
					for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
						if (actor == nullptr) continue;
						if (actor->getActorTypeComponent()->id != 71) continue;
						for (BlockPos& blockPos : placeList) {
							if (actor->getEyePos() == blockPos.CastTo<float>() || actor->getPos() == blockPos.CastTo<float>()) {
								placeList.push_back(actor->getEyePos().CastTo<int>());
								localPlayer->gameMode->attack(actor);
							}
						}
					}
				}
				for (BlockPos& blockPos : placeList) {
					if (PlayerUtil::tryPlaceBlock(blockPos, airPlace, packetPlace)) {
						if (fade) fadeBlockList[blockPos] = 1.f;
						placed++;
					}
					if (placed >= placePerTick) break;
				}
				oTick = 0;
			}
			else oTick++;
		}
		wasSurrounding = true;
	}
	if (button) {
		const BlockPos& buttonPos = localPlayer->getPos().floor().sub(0.f, 1.f, 0.f).CastTo<int>();
		uint8_t buttonSlot = InventoryUtil::findItemSlotInHotbar(143);
		if (switchMode != 0) {
			if (switchMode != 4) InventoryUtil::switchSlot(buttonSlot);
			if (switchMode == 4 && PlayerUtil::selectedSlotServerSide != buttonSlot) InventoryUtil::sendMobEquipment(buttonSlot);
		}
		PlayerUtil::tryPlaceBlock(buttonPos, airPlace, packetPlace);
	}
	
	if (switchMode == 3 || switchMode == 4) {
		if (switchMode == 3) InventoryUtil::switchSlot(oldSlot);
		else InventoryUtil::sendMobEquipment(oldSlot);
	}
}

void Surround::onLevelRender() {
	if (!render)
		return;

	for (BlockPos& blockPos : placeList) {
		MCR::drawBox3dFilled(getBlockAABB(blockPos), UIColor(color.r, color.g, color.b, alpha), UIColor(color.r, color.g, color.b, lineAlpha));
	}

	for (auto it = fadeBlockList.begin(); it != fadeBlockList.end(); ) {
		if (it->second > 0.f) {
			MCR::drawBox3dFilled(getBlockAABB(it->first), UIColor(color.r, color.g, color.b, (int)(alpha * it->second)), UIColor(color.r, color.g, color.b, (int)(lineAlpha * it->second)));
			it->second -= MCR::deltaTime * 1.5f;
			it++;
		}
		else {
			it = fadeBlockList.erase(it);
		}
	}
}

void Surround::onSendPacket(Packet* packet, bool& cancel) {
	if (!rotate) return;
	LocalPlayer* lp = Game::getLocalPlayer();
	if (packet->getId() == PacketID::PlayerAuthInput) {
		PlayerAuthInputPacket* authPkt = (PlayerAuthInputPacket*)packet;
		for (const BlockPos& blockPos : placeList) {
			if (WorldUtil::canBuildOn(blockPos)) {
				const Vec2<float>& angle = lp->getEyePos().CalcAngle(blockPos.CastTo<float>());
				authPkt->rotation = angle;
				authPkt->headYaw = angle.y;
			}
		}
	}
}