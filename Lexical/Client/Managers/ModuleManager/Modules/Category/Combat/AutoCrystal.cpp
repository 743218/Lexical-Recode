#include "AutoCrystal.h"

AutoCrystal::AutoCrystal() : Module("AutoCrystal", "Place and break crystals", Category::COMBAT) {
	registerSetting(new BoolSetting("Place", "Place end crystals", &place, true));
	registerSetting(new BoolSetting("Packet Place", "Place using packets", &packetPlace, false));
	registerSetting(new BoolSetting("Break", "Break end crystals", &explode, true));

	registerSetting(new SliderSetting<int>("Place Delay", "Delay in ticks", &placeDelay, 0, 0, 20));
	registerSetting(new SliderSetting<int>("Break Delay", "Delay in ticks", &breakDelay, 0, 0, 20));

	registerSetting(new SliderSetting<float>("Target Range", "If the distance between you and the target exceeds this then we wont add them to target list", &targetRange, 12.f, 1.f, 20.f));
	registerSetting(new SliderSetting<float>("Place Range", "Maximum range for placing an end crystal, if the distance between the you and the place pos exceeds this then we wont place", &placeRange, 6.f, 1.f, 12.f));
	registerSetting(new SliderSetting<float>("Break Range", "Maximum range for breaking an end crystal, if the distance between you and the end crystal exceeds this then we wont break it", &breakRange, 6.f, 1.f, 12.f));
	registerSetting(new SliderSetting<float>("Proximity", "If the distance between the target and an end crystal exceeds this then we will use another placement if available", &placeProximity, 6.f, 1.f, 12.f));

	registerSetting(new SliderSetting<float>("Enemy Damage", "If the place position wont deal more than this then we wont add it to place list", &enemyDamage, 4.f, 0.f, 20.f));
	registerSetting(new SliderSetting<float>("Self Damage", "If the damage to u deals more than this then we wont add it to place list", &selfDamage, 4.f, 0.f, 20.f));

	registerSetting(new EnumSetting("Priority", "Target priority", { "Distance", "Damage" }, &priority, 0));
	registerSetting(new BoolSetting("Phase Place", "In IGNPVP, when an entity is phased vertically or phased far enough horizontally, explosions can go through blocks to deal full damage to him\nthis will allow the autocrystal to place on places that would normally deal zero damage", &phasePlace, false));
	registerSetting(new BoolSetting("Multitask", "If this is on then the client will continue autocrystalling the enemy even if you are doing something(like eating, using a bow)", &multiTask, false));
	registerSetting(new BoolSetting("Java", "Uses 1.13+ java placements", &java, false));
	registerSetting(new BoolSetting("Rotate", "Rotates to end crystal", &rotate, false));
	registerSetting(new BoolSetting("Terrain", "If place list is empty then it will place obsidian in where it will deal the most damage", &obsidian, false));
	registerSetting(new BoolSetting("Packet Terrain", "Uses packets to place the obsidian", &packetTerrain, false, [&]() -> bool {
		return obsidian;
	}));
	registerSetting(new BoolSetting("AirPlace", "Places the obsidian on air", &airplace, false, [&]() -> bool {
		return obsidian;
	}));
	registerSetting(new SliderSetting<float>("Min Damage", "NULL", &minTerrainDamage, 6.f, 0.f, 20.f, [&]() -> bool {
		return obsidian;
	}));
	registerSetting(new SliderSetting<int>("Blocks Per Tick", "NULL", &blocksPerTick, 4, 1, 10, [&]() -> bool {
		return obsidian;
	}));
	registerSetting(new SliderSetting<int>("Proximity", "NULL", &terrainProximity, 4, 1, 10, [&]() -> bool {
		return obsidian;
	}));
	registerSetting(new SliderSetting<int>("Delay", "NULL", &terrainDelay, 0, 0, 20, [&]() -> bool {
		return obsidian;
	}));
	registerSetting(new EnumSetting("Switch Mode", "Switch Type\nNone: Dont switch\nNormal: Switch to end crystal\nSilent: Switches silently\nPacket: Uses MobEquipmentPackets to switch to end crystal, 'real silent'", { "None", "Normal", "Silent", "Packet" }, &switchMode, 0));
	registerSetting(new BoolSetting("Alternate", "Alternates between crystal and old slot smartly instead of alternating everytime", &alternate, false, [&]() -> bool {
		return switchMode == 3;
	}));
	registerSetting(new SliderSetting<int>("Waste Amount", "Amount of crystals we will place in 1 go, it basically multiplace", &wasteAmount, 1, 1, 5));
	registerSetting(new BoolSetting("Self", "NULL", &self, false));
	registerSetting(new BoolSetting("IDPredict", "Predicts the runtime id of the next end crystal that will be spawning and attacking that before it even spawns so the moment it spawns it dies instantly :exploding_head:", &idPredict, false));
	registerSetting(new SliderSetting<int>("Packets", "IDPredict packets, the more you put the faster id predict will be", &idPackets, 4, 1, 10, [&]() -> bool {
		return idPredict;
	}));
	registerSetting(new SliderSetting<int>("ID Delay", "Delay in ticks", &idDelay, 0, 0, 20, [&]() -> bool {
		return idPredict;
	}));
	registerSetting(new SliderSetting<float>("ID Damage", "If damage is less than this then we wont idpredict", &idDamage, 4.f, 0.f, 20.f, [&]() -> bool {
		return idPredict;
	}));
	registerSetting(new BoolSetting("Render", "Render the placement", &render, true));
	registerSetting(new BoolSetting("Render Damage", "NULL", &dmgText, false, [&]() -> bool {
		return render;
	}));
	registerSetting(new BoolSetting("Fade", "Fade render!!", &fade, true, [&]() -> bool {
		return render;
	}));
	registerSetting(new SliderSetting<float>("Fade Duration", "Fading duration in seconds", &fadeDuration, 1.f, 0.1f, 3.f, [&]() -> bool {
		return (render && fade);
	}));
	registerSetting(new BoolSetting("Lerp", "Slide renders yeah", &lerp, true, [&]() -> bool {
		return (render);
	}));
	registerSetting(new SliderSetting<float>("Speed", "Sliding speed", &lerpSpeed, 10.f, 1.f, 20.f, [&]() -> bool {
		return (render && lerp);
	}));
	registerSetting(new BoolSetting("Flat", "2D Render!", &flat, false, [&]() -> bool {
		return render;
	}));
	registerSetting(new ColorSetting("Fill Color", "Color of the render", &color, UIColor(255, 255, 255), true, true, [&]() -> bool {
		return render;
	}));
	registerSetting(new ColorSetting("Line Color", "Color of the render", &lineColor, UIColor(255, 255, 255), true, true, [&]() -> bool {
		return render;
	}));
	registerSetting(new ColorSetting("Text Color", "Color of the render", &damageColor, UIColor(255, 255, 255), false, true, [&]() -> bool {
		return render && dmgText;
		}));
}

bool AutoCrystal::sortByDamage(CrystalData d1, CrystalData d2) {
	return d1.targetDamage > d2.targetDamage;
}

bool AutoCrystal::isPlaceValid(const BlockPos& blockPos, Actor* actor) {
	if (blockPos.CastTo<float>().dist(Game::getLocalPlayer()->getPos()) > placeRange) return false;
	const Vec3<float>& actorPos = actor->getPos().floor().sub(0.f, 1.f, 0.f);
	if ((blockPos.CastTo<float>() == actorPos || blockPos.CastTo<float>() == actorPos.add(0, 1, 0)) && phasePlace) return false;
	BlockLegacy* base = WorldUtil::getBlock(blockPos)->blockLegacy;
	if (!(base->blockId == 7 || base->blockId == 49)) return false;
	BlockLegacy* upper = WorldUtil::getBlock(blockPos.add(0, 1, 0))->blockLegacy;
	if (upper->blockId != 0) return false;
	if (!java) {
		BlockLegacy* upper2 = WorldUtil::getBlock(blockPos.add(0, 2, 0))->blockLegacy;
		if (upper2->blockId != 0) return false;
	}
	const AABB blockAABB(blockPos.add(0, 1, 0).CastTo<float>(), blockPos.add(0, 1, 0).CastTo<float>().add(1.f));
	for (Actor* entity : entityList) {
		if (entity->getActorTypeComponent()->id != 71) {
			AABB entityAABB = entity->aabbShape->aabb;
			if (entity->getActorTypeComponent()->id == 319) {
				entityAABB = TargetUtil::makeAABB(0.6f, entity->aabbShape->height, entity->getEyePos());
				if (!java) {
					entityAABB.lower = entityAABB.lower.sub(0.1f, 0.f, 0.1f);
					entityAABB.upper = entityAABB.upper.add(0.1f, 0.f, 0.1f);
				}
			}
			if (entityAABB.intersects(blockAABB)) return false;
		}
	}
	return true;
}

float AutoCrystal::getExposure(const Vec3<float>& blockPos, Actor* actor) const {
	if (phasePlace) {
		const Vec3<float>& actorPos = actor->getPos().floor().sub(0.f, 1.f, 0.f);
		const AABB& actorAABB = actor->aabbShape->aabb;
		const AABB& blockAABB = AABB(actorPos.sub(0, 1, 0), actorPos.sub(0, 1, 0).add(1.f));
		if (actorAABB.intersects(blockAABB)) {
			Block* block = WorldUtil::getBlock(blockAABB.lower.CastTo<int>());
			if (block->blockLegacy->isSolid()) return 1.f;
		}
	}
	return WorldUtil::getSeenPercent(blockPos, actor);
}

bool AutoCrystal::isInList(const BlockPos& blockPos) {
	if (placeList.empty()) return false;
	std::vector<Place>::iterator it = std::find_if(placeList.begin(), placeList.end(), [blockPos](const Place& place) {
		return place.placePos == blockPos;
	});
	return it != placeList.end();
}

void AutoCrystal::generatePlacement(LocalPlayer* localPlayer, Actor* actor) {
	const Vec3<float>& actorPos = Vec3<float>(actor->getPos().x, ceil(actor->aabbShape->aabb.lower.y), actor->getPos().z);
	const int& radius = (int)placeProximity;
	for (int x = -radius; x <= radius; x++) {
		for (int y = -5; y <= 3; y++) {
			for (int z = -radius; z <= radius; z++) {
				const BlockPos& blockPos = BlockPos(actorPos.x + x, actorPos.y + y, actorPos.z + z);
				if (!isInList(blockPos)) {
					const bool& isValid = isPlaceValid(blockPos, actor);
					if (isValid) {
						const Place placement(localPlayer, blockPos, actor, getExposure(blockPos.CastTo<float>().add(0.5f, 1.f, 0.5f), actor));
						if (placement.targetDamage > enemyDamage && placement.localDamage <= selfDamage) placeList.emplace_back(placement);
					}
				}
			}
		}
	}
	std::sort(placeList.begin(), placeList.end(), AutoCrystal::sortByDamage);
}

bool AutoCrystal::isTerrainValid(const BlockPos& blockPos, Actor* actor) {
	if (actor->getPos().dist(Game::getLocalPlayer()->getPos()) > placeRange) return false;
	BlockLegacy* base = WorldUtil::getBlock(blockPos)->blockLegacy;
	if (base->blockId != 0) return false;
	BlockLegacy* upper = WorldUtil::getBlock(blockPos.add(0, 1, 0))->blockLegacy;
	if (upper->blockId != 0) return false;
	if (!java) {
		BlockLegacy* upper2 = WorldUtil::getBlock(blockPos.add(0, 2, 0))->blockLegacy;
		if (upper2->blockId != 0) return false;
	}
	const AABB blockAABB(blockPos.CastTo<float>(), blockPos.CastTo<float>().add(1.f));
	const AABB blockUpperAABB(blockPos.CastTo<float>().add(0, 1, 0), blockPos.CastTo<float>().add(0, 1, 0).add(1.f));
	for (Actor* entity : entityList) {
		if (entity->getActorTypeComponent()->id != 71) {
			AABB entityAABB = entity->aabbShape->aabb;
			if (entity->getActorTypeComponent()->id == 319 && !java) {
				entityAABB.lower = entityAABB.lower.sub(0.1f, 1.f, 0.1f);
				entityAABB.upper = entityAABB.upper.add(0.1f, 1.f, 0.1f);
			}
			if (entityAABB.intersects(blockUpperAABB)) return false;
			if (!TargetUtil::isTargetValid(entity, true, false) && entity != Game::getLocalPlayer()) continue;
			if (entityAABB.intersects(blockAABB)) return false;
		}
	}
	return true;
}

void AutoCrystal::getTerrainList(Actor* actor) {
	const int radius = terrainProximity;
	const Vec3<float>& actorPos = Vec3<float>(actor->getPos().x, ceil(actor->aabbShape->aabb.lower.y), actor->getPos().z);
	for (int x = -radius; x <= radius; x++) {
		for (int y = -5; y < 0; y++) {
			for (int z = -radius; z <= radius; z++) {
				const BlockPos& blockPos = actorPos.CastTo<int>().add(x, y, z);
				const bool& isValid = isTerrainValid(blockPos, actor);
				if (isValid) {
					const Terrain terrain(blockPos, actor, getExposure(blockPos.CastTo<float>().add(0.5f, 1.f, 0.5f), actor));
					if (terrain.targetDamage >= minTerrainDamage) terrainList.emplace_back(terrain);
				}
			}
		}
	}
	std::sort(terrainList.begin(), terrainList.end(), AutoCrystal::sortByDamage);
}

void AutoCrystal::placeObsidian(GameMode* gm) {
	if (terrainList.empty()) return;
	static float iDelay = 0;
	if (InventoryUtil::getHeldItemId() != 49 && switchMode != 3) return;
	if (TimerUtil::hasReached<ticks>(&iDelay, terrainDelay)) {
		if (!PlayerUtil::tryPlaceBlock(terrainList[0].placePos, airplace, packetTerrain) && WorldUtil::canBuildOn(terrainList[0].placePos)) PlayerUtil::tryPlaceBlockSupport(terrainList[0].placePos, terrainProximity, airplace, packetTerrain);
	}
}

void AutoCrystal::onTerrain(LocalPlayer* localPlayer, Actor* actor) {
	if (!placeList.empty()) {
		terrainList.clear();
		return;
	}
	getTerrainList(actor);
	uint8_t oldSlot = InventoryUtil::getSelectedSlot();
	if (switchMode != 0) {
		if (switchMode != 3 && localPlayer->playerInventory->selectedSlot != InventoryUtil::findItemSlotInHotbar(49)) localPlayer->playerInventory->selectedSlot = InventoryUtil::findItemSlotInHotbar(49);
		else if (switchMode == 3) {
			if (PlayerUtil::selectedSlotServerSide != InventoryUtil::findItemSlotInHotbar(49)) InventoryUtil::sendMobEquipment(InventoryUtil::findItemSlotInHotbar(49));
		}
	}
	placeObsidian(localPlayer->gameMode);
	if (switchMode != 0) {
		if (switchMode == 2 && localPlayer->playerInventory->selectedSlot != oldSlot && !packetPlace) localPlayer->playerInventory->selectedSlot = oldSlot;
		else if (switchMode == 3 && PlayerUtil::selectedSlotServerSide != oldSlot) InventoryUtil::sendMobEquipment(oldSlot);
	}
}

void AutoCrystal::getCrystalList(LocalPlayer* localPlayer, Actor* actor) {
	for (Actor* entity : entityList) {
		if (entity->getActorTypeComponent()->id != 71) continue;
		if (entity->getPos().dist(Game::getLocalPlayer()->getPos()) > breakRange) continue;
		highestID = entity->getRuntimeIDComponent()->runtimeId.id;
		const Break breakment(localPlayer, entity, actor, getExposure(entity->getPos(), actor));
		if (breakment.targetDamage > enemyDamage && breakment.localDamage <= selfDamage) {
			highestID = entity->getRuntimeIDComponent()->runtimeId.id;
			breakList.emplace_back(breakment);
		}
	}
	std::sort(breakList.begin(), breakList.end(), AutoCrystal::sortByDamage);
}

void AutoCrystal::attackCrystal(GameMode* gm, LocalPlayer* localPlayer) {
	static float iBreakDelay = 0;
	static float iPredictDelay = 0;
	if (!breakList.empty() && !idPredict) {
		if ((TimerUtil::hasReached<ticks>(&iBreakDelay, breakDelay)) && ((check && alternate && switchMode == 3) || (!alternate))) {
			if (breakList[0].endCrystal != nullptr) {
				gm->attack(breakList[0].endCrystal);
				check = false;
			}
		}
	}
	if (placeList.empty()) return;
	if (placeList[0].targetDamage < idDamage) {
		if (!breakList.empty()) gm->attack(breakList[0].endCrystal);
		return;
	}
	if (idPredict) {
		if (TimerUtil::hasReached<ticks>(&iPredictDelay, idDelay) && ((check && alternate && switchMode == 3) || (!alternate))) {
			for (int i = 0; i < idPackets; i++) {
				std::unique_ptr<ItemUseOnActorInventoryTransaction> attackInvTransac = ItemUseOnActorInventoryTransaction::make_unique();
				attackInvTransac->actorRuntimeId = highestID;
				attackInvTransac->actionType = ItemUseOnActorInventoryTransaction::ActionType::Attack;
				attackInvTransac->slot = localPlayer->playerInventory->selectedSlot;
				attackInvTransac->itemInHand = NetworkItemStackDescriptor(localPlayer->playerInventory->container->getItem(attackInvTransac->slot));
				attackInvTransac->playerPos = localPlayer->getPos();
				attackInvTransac->clickPos = Vec3<float>(0.f, 0.f, 0.f);
				InventoryTransactionPacket itp(std::move(attackInvTransac));
				localPlayer->level->getPacketSender()->send(&itp);
				highestID++;
			}
			highestID -= idPackets;
			check = false;
		}
	}
}

void AutoCrystal::placeCrystal(GameMode* gm, LocalPlayer* localPlayer) {
	if (placeList.empty()) return;
	static float iPlaceDelay = 0;
	int placed = 0;
	if ((TimerUtil::hasReached<ticks>(&iPlaceDelay, placeDelay)) && ((!check && alternate && switchMode == 3) || (!alternate))) {
		if (alternate && switchMode == 3 && PlayerUtil::selectedSlotServerSide != InventoryUtil::findItemSlotInHotbar(758)) InventoryUtil::sendMobEquipment(InventoryUtil::findItemSlotInHotbar(758));
		for (const Place& place : placeList) {
			if (!packetPlace) gm->buildBlock(place.placePos, 0, false);
			else {
				std::unique_ptr<ItemUseInventoryTransaction> itemUseInvTransac = ItemUseInventoryTransaction::make_unique();
				itemUseInvTransac->actionType = ItemUseInventoryTransaction::ActionType::Place;
				itemUseInvTransac->blockPos = place.placePos;
				itemUseInvTransac->targetBlockRuntimeId = *Game::clientInstance->getRegion()->getBlock(place.placePos)->getRuntimeId();
				itemUseInvTransac->face = 0;
				itemUseInvTransac->slot = InventoryUtil::getSelectedSlot();
				itemUseInvTransac->itemInHand = NetworkItemStackDescriptor(InventoryUtil::getItem(InventoryUtil::getSelectedSlot()));
				itemUseInvTransac->playerPos = localPlayer->getPos();
				itemUseInvTransac->clickPos = Vec3<float>(0.5f, 0.5f, 0.5f);
				InventoryTransactionPacket pk(std::move(itemUseInvTransac));
				localPlayer->level->getPacketSender()->send(&pk);
				localPlayer->swing();
			}
			check = true;
			if (++placed >= wasteAmount) break;
		}
	}
}

void AutoCrystal::onNormalTick(LocalPlayer* localPlayer) {
	terrainList.clear();
	placeList.clear();
	breakList.clear();
	entityList.clear();
	targetList.clear();
	if (Game::clientInstance->getRegion() == nullptr) return;
	if (localPlayer == nullptr) return;
	if (localPlayer->level == nullptr) return;
	if (!multiTask && localPlayer->getItemUseDuration() > 0) {
		if (alternate && switchMode == 3 && PlayerUtil::selectedSlotServerSide != oldSlot && oldSlot != -1) {
			InventoryUtil::sendMobEquipment(oldSlot);
			oldSlot = -1;
		}
		check = false;
		return;
	}
	for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
		if (!actor) continue;
		entityList.push_back(actor);
		if (!TargetUtil::isTargetValid(actor, false, true, targetRange)) continue;
		targetList.push_back(actor);
	}
	if (self) targetList.push_back((Actor*)localPlayer);
	if (entityList.empty()) return;
	if (targetList.empty()) {
		if (alternate && switchMode == 3 && PlayerUtil::selectedSlotServerSide != oldSlot && oldSlot != -1) {
			InventoryUtil::sendMobEquipment(oldSlot);
			oldSlot = -1;
		}
		check = false;
		return;
	}
	std::sort(targetList.begin(), targetList.end(), TargetUtil::sortByDist);
	for (Actor* actor : targetList) {
		if (place) generatePlacement(localPlayer, actor);
		if (explode) getCrystalList(localPlayer, actor);
		if (obsidian) onTerrain(localPlayer, actor);
		if (priority != 1) break; // if priority is 1(damage) this will iterate through all the actors in targetlist and get the best damage out of all of them, this will decrease performance tho :<
	}
	oldSlot = localPlayer->playerInventory->selectedSlot;
	if (switchMode != 0 && !placeList.empty()) {
		uint8_t endCrystal = InventoryUtil::findItemSlotInHotbar(758);
		if (switchMode != 3 && InventoryUtil::getSelectedSlot() != endCrystal) InventoryUtil::switchSlot(endCrystal);
		else if (switchMode == 3 && PlayerUtil::selectedSlotServerSide != endCrystal && !alternate) InventoryUtil::sendMobEquipment(endCrystal);
	}
	if (place) placeCrystal(localPlayer->gameMode, localPlayer);
	if (explode) attackCrystal(localPlayer->gameMode, localPlayer);
	if (switchMode == 2 && InventoryUtil::getSelectedSlot() != oldSlot) InventoryUtil::switchSlot(oldSlot);
	if (!alternate && switchMode == 3 && PlayerUtil::selectedSlotServerSide != oldSlot) InventoryUtil::sendMobEquipment(oldSlot);
}

void AutoCrystal::onDisable() {
	if (alternate && switchMode == 3 && PlayerUtil::selectedSlotServerSide != oldSlot && oldSlot != -1) {
		InventoryUtil::sendMobEquipment(oldSlot);
		oldSlot = -1;
	}
	check = false;
}

void AutoCrystal::onSendPacket(Packet* packet, bool& cancel) {
	if (placeList.empty()) return;
	if (!rotate) return;
	const Vec2<float>& angle = Game::getLocalPlayer()->getPos().CalcAngle(placeList[0].placePos.CastTo<float>());
	if (packet->getId() == PacketID::PlayerAuthInput) {
		PlayerAuthInputPacket* authPkt = (PlayerAuthInputPacket*)packet;
		authPkt->rotation = angle;
		authPkt->headYaw = angle.y;
	}
}

void AutoCrystal::onLevelRender() {
	if (!render) return;
	static std::vector<Fade> fadeList;
	static Vec3<float> lastPlaceLoc;
	static float decreasingAlpha = 0.f;
	int placed = 0;
	for (auto it = fadeList.begin(); it != fadeList.end(); ) {
		it->deltaTime += D2D::deltaTime;
		const float fadeAlpha = 1.f - (it->deltaTime / it->duration);
		if (fadeAlpha <= 0.0f) it = fadeList.erase(it);
		else {
			const AABB blockAABB(it->fadePos.add(0.f, flat ? 1.f : 0.f, 0.f), it->fadePos.add(1.f));
			MCR::drawBox3dFilled(blockAABB, UIColor(color.r, color.g, color.b, (uint8_t)((float)color.a * fadeAlpha)), UIColor(lineColor.r, lineColor.g, lineColor.b, (uint8_t)((float)lineColor.a * fadeAlpha)));
			it++;
		}
	}
	if (placeList.empty()) {
		decreasingAlpha = Math::lerp(decreasingAlpha, 0.f, D2D::deltaTime * 4.f);
		const AABB blockAABB(lastPlaceLoc.add(0.f, flat ? 1.f : 0.f, 0.f), lastPlaceLoc.add(1.f));
		MCR::drawBox3dFilled(blockAABB, UIColor(color.r, color.g, color.b, (uint8_t)((float)color.a * decreasingAlpha)), UIColor(lineColor.r, lineColor.g, lineColor.b, (uint8_t)((float)lineColor.a * decreasingAlpha)));
	}
	else {
		decreasingAlpha = Math::lerp(decreasingAlpha, 1.f, D2D::deltaTime * 8.f);
		for (int i = 0; i < placeList.size(); i++) {
			Vec3<float> placeLoc = placeList[i].placePos.CastTo<float>();
			if (fade && placeLoc != lastPlaceLoc) fadeList.emplace_back(Fade(lastPlaceLoc, fadeDuration));
			const float animSpeed = D2D::deltaTime * lerpSpeed;
			if (lerp) placeLoc = lastPlaceLoc.lerp(placeLoc, animSpeed, animSpeed, animSpeed);
			AABB blockAABB(placeLoc.add(0.f, flat ? 1.f : 0.f, 0.f), placeLoc.add(1.f));
			MCR::drawBox3dFilled(blockAABB, UIColor(color.r, color.g, color.b, (uint8_t)((float)color.a * decreasingAlpha)), UIColor(lineColor.r, lineColor.g, lineColor.b, (uint8_t)((float)lineColor.a * decreasingAlpha)));
			lastPlaceLoc = placeLoc;
			if (++placed >= wasteAmount) break;
		}
	}
}

void AutoCrystal::onMCRender(MinecraftUIRenderContext* renderCtx) { // this is so retarded
	if (!dmgText || !render) return;
	if (placeList.empty()) return;
	int placed = 0;
	for (int i = 0; i < placeList.size(); i++) {
		Vec3<float> placeLoc = placeList[i].placePos.CastTo<float>();
		const int& damage = placeList[i].targetDamage;
		MCR::drawTextInWorld(placeLoc.sub(0.f, 0.3f, 0.f), std::to_string(damage), damageColor, 0.7f);
		if (++placed >= wasteAmount) break;
	}
}