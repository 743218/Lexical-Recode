#include "KillAura.h"
#include "../../../ModuleManager.h"

KillAura::KillAura() : Module("KillAura", "Attacks entities around you", Category::COMBAT) {
	registerSetting(new SliderSetting<float>("Range", "Range in which targets will be hit", &range, 5.f, 3.f, 20.f));
	registerSetting(new SliderSetting<float>("WallRange", "Range in which targets will be hit through walls", &wallRange, 0.f, 0.f, 20.f));
	registerSetting(new SliderSetting<int>("Interval", "Attack delay (Tick)", &interval, 1, 0, 20));
	registerSetting(new EnumSetting("Rotations", "Rotates to the targets", { "None", "Normal", "Strafe" }, &rotMode, 1));
	registerSetting(new BoolSetting("Randomize", "NULL", &randomize, false));
	registerSetting(new BoolSetting("Java", "Java hit cooldown", &java, false));
	registerSetting(new BoolSetting("CA-Compatibility", "Better compatibility with AutoCrystal", &caCompatibility, false));
	registerSetting(new EnumSetting("HitType", "NULL", { "Single", "Multi" }, &hitType, 0));
	registerSetting(new BoolSetting("Packet", "Uses packets to attack", &packetAttack, false));
	registerSetting(new SliderSetting<int>("HitChance", "NULL", &hitChance, 100, 0, 100));
	registerSetting(new SliderSetting<int>("Hit Attempts", "NULL", &hitAttempts, 1, 1, 50));
	registerSetting(new EnumSetting("Weapon", "Auto switch to best weapon", { "None", "Switch", "Spoof" }, &autoWeaponMode, 0));
	registerSetting(new BoolSetting("Mobs", "Attack Mobs", &includeMobs, false));
	registerSetting(new BoolSetting("EatStop", "StopOnEat", &eatStop, false));
	registerSetting(new BoolSetting("Render", "NULL", &render, false));
	registerSetting(new ColorSetting("Color", "Render Color", &renderColor, renderColor, true, true, [&]() -> bool {
		return render;
		})
	);
}

std::string KillAura::getModeText() {
	switch (hitType) {
	case 0: {
		return "Single";
		break;
	}
	case 1: {
		return "Multi";
		break;
	}
	}
	return "NULL";
}

void KillAura::onDisable() {
	targetList.clear();
	shouldRot = false;
}

void KillAura::onEnable() {
}

bool KillAura::sortByDist(Actor* a1, Actor* a2) {
	Vec3<float> lpPos = Game::getLocalPlayer()->getPos();
	return ((a1->getPos().dist(lpPos)) < (a2->getPos().dist(lpPos)));
}

int KillAura::getBestWeaponSlot(Actor* target) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	Container* inventory = localPlayer->playerInventory->container;
	float damage = 0.f;
	int slot = localPlayer->playerInventory->selectedSlot;
	int oldSlot = localPlayer->playerInventory->selectedSlot;
	for (int i = 0; i < 9; i++) {
		localPlayer->playerInventory->selectedSlot = i;
		float currentDamage = localPlayer->calculateAttackDamage(target);
		if (currentDamage > damage) {
			damage = currentDamage;
			slot = i;
		}
		localPlayer->playerInventory->selectedSlot = oldSlot;
	}

	return slot;
}

void KillAura::Attack(Actor* target) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	GameMode* gm = localPlayer->gameMode;
	const int randomNumber = (int)(rand() % 100);
	if (randomNumber < hitChance) {
		for (int i = 0; i < hitAttempts; i++) {
			if (packetAttack) {
				std::unique_ptr<ItemUseOnActorInventoryTransaction> attackInvTransac = ItemUseOnActorInventoryTransaction::make_unique();
				attackInvTransac->actorRuntimeId = target->getRuntimeIDComponent()->runtimeId.id;
				attackInvTransac->actionType = ItemUseOnActorInventoryTransaction::ActionType::Attack;
				attackInvTransac->slot = localPlayer->playerInventory->selectedSlot;
				attackInvTransac->itemInHand = NetworkItemStackDescriptor(localPlayer->playerInventory->container->getItem(attackInvTransac->slot));
				attackInvTransac->playerPos = localPlayer->getPos();
				attackInvTransac->clickPos = Vec3<float>(0.f, 0.f, 0.f);
				InventoryTransactionPacket itp(std::move(attackInvTransac));
				localPlayer->level->getPacketSender()->send(&itp);
			}
			else gm->attack(target);
		}
	}

	localPlayer->swing();
}

void KillAura::onNormalTick(LocalPlayer* localPlayer) {
	static std::vector<Actor*> crystalList;
	static AutoCrystal* crystalAura = ModuleManager::getModule<AutoCrystal>();
	static float timePassed = 0.f;
	Level* level = localPlayer->level;
	BlockSource* region = Game::clientInstance->getRegion();
	crystalList.clear();
	targetList.clear();
	if (eatStop && localPlayer->getItemUseDuration() > 0) {
		shouldRot = false;
		return;
	}
	for (auto& entity : level->getRuntimeActorList()) {
		if (entity == nullptr) continue;
		if (WorldUtil::distanceToEntity(localPlayer->getPos(), entity) > range) continue;
		if (entity->getActorTypeComponent()->id == 71) crystalList.push_back(entity);
		if (TargetUtil::isTargetValid(entity, includeMobs)) {
			float rangeCheck = range;
			if (region->getSeenPercent(localPlayer->getEyePos(), entity->aabbShape->aabb) == 0.0f)
				rangeCheck = wallRange;

			if (WorldUtil::distanceToEntity(localPlayer->getPos(), entity) <= rangeCheck)
				targetList.push_back(entity);
		}
	}
	
	if (targetList.empty()) {
		shouldRot = false;
		return;
	}

	std::sort(targetList.begin(), targetList.end(), KillAura::sortByDist);

	int oldSlot = localPlayer->playerInventory->selectedSlot;

	Vec3<float> aimPos = targetList[0]->getEyePos();
	aimPos.y = targetList[0]->aabbShape->aabb.getCenter().y;
	rotAngle = localPlayer->getEyePos().CalcAngle(aimPos);

	if (randomize)
		rotAngle = rotAngle.add(Vec2<float>(Math::randomFloat(-7.f, 7.f), Math::randomFloat(-7.f, 7.f)));

	shouldRot = true;
	const int weaponSlot = getBestWeaponSlot(targetList[0]);
	if (crystalAura->isEnabled() && !crystalAura->placeList.empty() && caCompatibility) return;
	if ((!java && TimerUtil::hasReached<ticks>(&timePassed, interval)) || (java && TimerUtil::hasReached<milliseconds>(&timePassed, InventoryUtil::getItemCooldown(InventoryUtil::getItem(weaponSlot))))) {
		if (autoWeaponMode != 0) {
			if (localPlayer->playerInventory->selectedSlot != weaponSlot) localPlayer->playerInventory->selectedSlot = weaponSlot;
			if (autoWeaponMode == 2 && PlayerUtil::selectedSlotServerSide != weaponSlot) {
				MobEquipmentPacket mep(localPlayer->getRuntimeIDComponent()->runtimeId, localPlayer->playerInventory->container->getItem(weaponSlot), weaponSlot, weaponSlot);
				level->getPacketSender()->send(&mep);
			}
		}
		for (auto& target : targetList) {
			Attack(target);
			if (hitType != 1) break;
		}
		if (autoWeaponMode == 2) {
			if (localPlayer->playerInventory->selectedSlot != oldSlot) localPlayer->playerInventory->selectedSlot = oldSlot;
		}
	}
}

void KillAura::onUpdateRotation(LocalPlayer* localPlayer) {

	if (!shouldRot)
		return;

	ActorRotationComponent* rotation = localPlayer->rotation;
	ActorHeadRotationComponent* headRot = localPlayer->getActorHeadRotationComponent();
	MobBodyRotationComponent* bodyRot = localPlayer->getMobBodyRotationComponent();

	switch (rotMode) {
	case 0: {
		return;
		break;
	}
	case 1: {
		rotation->presentRot.x = rotAngle.x;
		headRot->headYaw = rotAngle.y;
		bodyRot->bodyYaw = rotAngle.y;
		break;
	}
	case 2: {
		rotation->presentRot = rotAngle;
		headRot->headYaw = rotAngle.y;
		bodyRot->bodyYaw = rotAngle.y;
		break;
	}
	}
}

void KillAura::onSendPacket(Packet* packet, bool& cancel) {
	if (rotMode == 0)
		return;

	if (!shouldRot)
		return;

	if (packet->getId() == PacketID::PlayerAuthInput) {
		PlayerAuthInputPacket* paip = static_cast<PlayerAuthInputPacket*>(packet);
		paip->rotation = rotAngle;
		paip->headYaw = rotAngle.y;
	}
}

void KillAura::onLevelRender() {
	if (!render) return;
	if (targetList.empty())
		return;

	Vec3<float> origin = Game::clientInstance->getLevelRenderer()->levelRendererPlayer->realOrigin;

	Actor* target = targetList[0];
	AABBShapeComponent* targetAABBShape = target->aabbShape;
	Vec3<float> targetPos = target->getEyePos();

	if (target->getActorTypeComponent()->id != 319)
		targetPos.y += targetAABBShape->height;

	static float anim = 0.f;
	anim += MCR::deltaTime * 45.f;
	const float coolAnim = 1.f + 1.f * sin((anim / 60.f) * PI);

	targetPos.y -= 1.6f;
	targetPos.y += coolAnim;

	Vec3<float> posList[360];
	for (int i = 0; i < 360; i++) {
		float calcYaw = (i + 90.f) * (PI / 180.f);
		Vec3<float> perm = Vec3<float>(cos(calcYaw) * 0.58f, 0.f, sin(calcYaw) * 0.58f);
		posList[i] = targetPos.add(perm).sub(origin);
	}

	MCR::setColor(UIColor(renderColor.r, renderColor.g, renderColor.b, 5));
	MCR::tessellator->begin(VertextFormat::QUAD);

	static float oldCoolAnim = 1.f;
	oldCoolAnim = Math::lerp(oldCoolAnim, coolAnim, MCR::deltaTime * 5.f);
	float oldY = targetPos.y - coolAnim + oldCoolAnim;

	int quatily = 15;
	int times = 0;
	for (float yAdd = targetPos.y; yAdd != oldY;) {
		for (int i = 0; i < 360; i++) {
			Vec3<float> p1 = posList[i];
			Vec3<float> p2 = posList[(i + 1) % 360];  // loop back to first point when reaching end of list
			Vec3<float> p3 = Vec3<float>(p2.x, yAdd - origin.y, p2.z);      // bottom point of cylinder
			Vec3<float> p4 = Vec3<float>(p1.x, yAdd - origin.y, p1.z);      // bottom point of cylinder
			
			// Front side
			MCR::tessellator->vertex(p1.x, p1.y, p1.z);
			MCR::tessellator->vertex(p2.x, p2.y, p2.z);
			MCR::tessellator->vertex(p3.x, p3.y, p3.z);
			MCR::tessellator->vertex(p4.x, p4.y, p4.z);

			// Back side
			MCR::tessellator->vertex(p4.x, p4.y, p4.z);
			MCR::tessellator->vertex(p3.x, p3.y, p3.z);
			MCR::tessellator->vertex(p2.x, p2.y, p2.z);
			MCR::tessellator->vertex(p1.x, p1.y, p1.z);

		}

		yAdd = Math::lerp(yAdd, oldY, 0.08f);
		if (times >= quatily)
			break;

		times++;
	}

	MeshHelpers::renderMeshImmediately(MCR::screenCtx, MCR::tessellator, MCR::blendMaterial);

	
	MCR::setColor(renderColor);
	MCR::tessellator->begin(VertextFormat::LINE_LIST);

	for (int i = 0; i < 360; i++) {
		Vec3<float> p1 = posList[i];
		Vec3<float> p2 = posList[(i + 1) % 360];  // loop back to first point when reaching end of list

		MCR::tessellator->vertex(p1.x, p1.y, p1.z);
		MCR::tessellator->vertex(p2.x, p2.y, p2.z);
	}

	MeshHelpers::renderMeshImmediately(MCR::screenCtx, MCR::tessellator, MCR::blendMaterial);
}