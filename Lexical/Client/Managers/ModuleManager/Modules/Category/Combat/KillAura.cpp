#include "KillAura.h"
#include "../../../ModuleManager.h"
#include <algorithm>
#include <cmath>

KillAura::KillAura() : Module("KillAura", "Automatically attacks nearby entities.", Category::COMBAT) {
	registerSetting(new EnumSetting("Switch", "NULL", { "None", "Switch", "Silent" }, &switchMode, 0));

	registerSetting(new BoolSetting("Packet", "Attack using inventory packets instead of GameMode.", &packet, false));

	registerSetting(new EnumSetting("Rotate", "NULL", { "Always", "OnHit", "None" }, &rotateMode, 2));

	registerSetting(new BoolSetting("Silent Rotation", "NULL", &silent, false, [this]() -> bool {
		return rotateMode != 2;
		}));

	registerSetting(new SliderSetting<float>("Range", "NULL", &range, 4.f, 1.f, 8.f));

	registerSetting(new BoolSetting("Mobs", "NULL", &mobs, false));

	registerSetting(new EnumSetting("Target", "NULL", { "Single", "Multi" }, &targetMode, 0));

	registerSetting(new SliderSetting<int>("Attack Delay", "NULL", &attackDelay, 10, 0, 20));

	registerSetting(new BoolSetting("Ring", "Render a ring around the target.", &ring, false));

	registerSetting(new SliderSetting<float>("Ring Radius", "Ring radius in blocks.", &ringRadius, 0.6f, 0.1f, 3.f, [this]() -> bool {
		return ring;
		}));

	registerSetting(new SliderSetting<float>("Ring Thickness", "Ring thickness in blocks.", &ringThickness, 0.12f, 0.02f, 0.6f, [this]() -> bool {
		return ring;
		}));

	registerSetting(new SliderSetting<float>("Ring Height", "Vertical height of the ring band itself, in blocks.", &ringHeight, 0.12f, 0.01f, 2.f, [this]() -> bool {
		return ring;
		}));

	registerSetting(new EnumSetting("Ring Quality", "How many segments the ring is made of.", { "128", "256", "360" }, &ringQuality, 1, [this]() -> bool {
		return ring;
		}));

	registerSetting(new EnumSetting("Ring Animation", "Up/down animation type.", { "Robotic", "Smooth" }, &ringAnim, 1, [this]() -> bool {
		return ring;
		}));

	registerSetting(new SliderSetting<float>("Ring Speed", "Ring up/down speed.", &ringSpeed, 2.f, 0.2f, 10.f, [this]() -> bool {
		return ring;
		}));

	registerSetting(new ColorSetting("Ring Color", "NULL", &ringColor, ringColor, true, true, [this]() -> bool {
		return ring;
		}));

	registerSetting(new BoolSetting("Swing", "Swing hand when attacking.", &swing, true));
}

void KillAura::onEnable() {
	target = nullptr;
	targets.clear();
	attackTicks = attackDelay;
	hitTicks = 0;
	pauseTicks = 0;
	switchBackSlot = -1;
	ringLastTarget = nullptr;
	ringSmoothed = Vec3<float>(0.f, 0.f, 0.f);
	ringTime = 0.f;
}

bool KillAura::isTargetValid(Actor* actor) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!localPlayer || !actor) return false;
	if (actor == localPlayer || !actor->isAlive()) return false;

	auto* typeComp = actor->getActorTypeComponent();
	if (!typeComp) return false;

	if (!mobs && typeComp->id != 319)
		return false;

	if (localPlayer->getPos().dist(actor->getPos()) > range)
		return false;

	return TargetUtil::isTargetValid(actor, mobs, true, range);
}

bool KillAura::isSword(ItemStack* itemStack) {
	if (!itemStack || !itemStack->item.get()) return false;

	std::string name = itemStack->item->getfullName();
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	return name.find("_sword") != std::string::npos;
}

bool KillAura::isAxe(ItemStack* itemStack) {
	if (!itemStack || !itemStack->item.get()) return false;

	std::string name = itemStack->item->getfullName();
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	return name.find("_axe") != std::string::npos;
}

bool KillAura::isMace(ItemStack* itemStack) {
	if (!itemStack || !itemStack->item.get()) return false;

	std::string name = itemStack->item->getfullName();
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	return name.find("mace") != std::string::npos;
}

bool KillAura::isTrident(ItemStack* itemStack) {
	if (!itemStack || !itemStack->item.get()) return false;

	std::string name = itemStack->item->getfullName();
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);

	return name.find("trident") != std::string::npos;
}

int KillAura::getBestWeaponSlot() {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!localPlayer) return -1;

	int bestSlot = -1;
	float bestDamage = 0.f;

	for (int i = 0; i < 9; i++) {
		ItemStack* itemStack = InventoryUtil::getItem(i);
		if (!itemStack || !itemStack->item.get()) continue;

		float damage = static_cast<float>(itemStack->item->getAttackDamage());
		if (damage > bestDamage) {
			bestDamage = damage;
			bestSlot = i;
		}
	}

	return bestSlot;
}

void KillAura::rotateToTarget() {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!localPlayer || !target) return;

	Vec3<float> localPos = localPlayer->getEyePos();
	Vec3<float> targetPos = target->getEyePos();
	Vec3<float> delta = targetPos.sub(localPos);

	float yaw = atan2f(delta.z, delta.x) * 180.f / 3.14159265358979323846f - 90.f;
	float pitch = -atan2f(delta.y, sqrtf(delta.x * delta.x + delta.z * delta.z)) * 180.f / 3.14159265358979323846f;

	localPlayer->rotation->presentRot = Vec2<float>(pitch, yaw);
	localPlayer->getActorHeadRotationComponent()->headYaw = yaw;
}

void KillAura::attackActor(Actor* actor) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!localPlayer || !actor) return;

	if (actor->getStatusFlag(ActorFlags::Blocking))
		return;

	if (packet) {
		auto runtimeComp = actor->getRuntimeIDComponent();
		if (runtimeComp) {
			const int slot = (weaponSlot >= 0 && weaponSlot <= 8) ? weaponSlot : InventoryUtil::getSelectedSlot();

			auto attackTransac = ItemUseOnActorInventoryTransaction::make_unique();
			attackTransac->actorRuntimeId = runtimeComp->runtimeId.id;
			attackTransac->actionType = ItemUseOnActorInventoryTransaction::ActionType::Attack;
			attackTransac->slot = slot;
			attackTransac->itemInHand = NetworkItemStackDescriptor(InventoryUtil::getItem(slot));
			attackTransac->playerPos = localPlayer->getPos();
			attackTransac->clickPos = Vec3<float>(0.5f, 0.5f, 0.5f);

			InventoryTransactionPacket pkt(std::move(attackTransac));
			localPlayer->level->getPacketSender()->send(&pkt);
		}
	}
	else {
		localPlayer->gameMode->attack(actor);
	}

	attackTicks = 0;
	hitTicks = 2;
}

void KillAura::onUpdateRotation(LocalPlayer* localPlayer) {
	if (!localPlayer || !target || silent) return;

	if (rotateMode == 0)
		rotateToTarget();

	if (rotateMode == 1 && hitTicks > 0)
		rotateToTarget();
}

void KillAura::onSendPacket(Packet* packet, bool& cancel) {
	if (!packet || !target || !silent) return;

	if (packet->getId() == PacketID::PlayerAuthInput) {
		auto paip = static_cast<PlayerAuthInputPacket*>(packet);

		if (rotateMode == 0 || (rotateMode == 1 && hitTicks > 0)) {
			LocalPlayer* localPlayer = Game::getLocalPlayer();
			if (!localPlayer) return;

			Vec3<float> localPos = localPlayer->getEyePos();
			Vec3<float> targetPos = target->getEyePos();
			Vec3<float> delta = targetPos.sub(localPos);

			float yaw = atan2f(delta.z, delta.x) * 180.f / 3.14159265358979323846f - 90.f;
			float pitch = -atan2f(delta.y, sqrtf(delta.x * delta.x + delta.z * delta.z)) * 180.f / 3.14159265358979323846f;

			paip->rotation = Vec2<float>(pitch, yaw);
			paip->headYaw = yaw;
		}
	}
}

void KillAura::onNormalTick(LocalPlayer* localPlayer) {
	if (!localPlayer || !localPlayer->isAlive()) return;
	if (PlayerUtil::isEating(localPlayer)) return;

	if (auto packetMine = ModuleManager::getModule<PacketMine>()) {
		if (packetMine->isEnabled()) {
			if (packetMine->getBreakPos() != BlockPos(0, 0, 0)) pauseTicks = 2;
			if (pauseTicks > 0) { pauseTicks--; return; }
		}
	}

	target = nullptr;
	targets.clear();

	std::vector<std::pair<float, Actor*>> candidates;

	for (auto& entity : localPlayer->level->getRuntimeActorList()) {
		if (!isTargetValid(entity)) continue;

		const float distance = localPlayer->getPos().dist(entity->getPos());
		candidates.emplace_back(distance, entity);
	}

	std::sort(candidates.begin(), candidates.end(),
		[](const std::pair<float, Actor*>& a, const std::pair<float, Actor*>& b) {
			return a.first < b.first;
		});

	for (auto& candidate : candidates)
		targets.push_back(candidate.second);

	if (!targets.empty())
		target = targets.front();

	if (!target) {
		attackTicks = attackDelay;
		hitTicks = 0;
		weaponSlot = -1;
		return;
	}

	if (hitTicks > 0)
		hitTicks--;

	bool holdingBestWeapon = true;
	if (switchMode != 0) {
		const int bestSlot = getBestWeaponSlot();
		holdingBestWeapon = (bestSlot == -1 || bestSlot == InventoryUtil::getSelectedSlot());
	}

	if (!holdingBestWeapon) {
		weaponSlot = -1;

		const int bestSlot = getBestWeaponSlot();
		if (bestSlot != -1 && bestSlot != InventoryUtil::getSelectedSlot() &&
			(switchMode != 2 || attackTicks >= attackDelay)) {
			if (switchMode == 2)
				switchBackSlot = InventoryUtil::getSelectedSlot();

			InventoryUtil::switchSlot(bestSlot);
			InventoryUtil::sendMobEquipment(bestSlot);
			weaponSlot = bestSlot;
		}

		if (switchBackSlot == -1)
			return;
	}

	weaponSlot = InventoryUtil::getSelectedSlot();

	if (attackTicks < attackDelay) {
		attackTicks++;
		return;
	}

	if (targetMode == 1) {
		for (Actor* actor : targets) {
			if (actor)
				attackActor(actor);
		}
	}
	else {
		attackActor(target);
	}

	if (swing) {
		localPlayer->swing();
	}

	if (switchBackSlot != -1) {
		InventoryUtil::switchSlot(switchBackSlot);
		switchBackSlot = -1;
	}
}

std::string KillAura::getModeText() {
	return targetMode == 1 ? "Multi" : "Single";
}

void KillAura::renderRing(const Vec3<float>& center, float radius, float thickness, float height, int segments, const UIColor& color) {
	if (MCR::tessellator == nullptr || MCR::screenCtx == nullptr || segments < 3) return;

	const float inner = std::max(0.f, radius - thickness * 0.5f);
	const float outer = radius + thickness * 0.5f;
	const float half = std::max(0.001f, height) * 0.5f;

	const Vec3<float> c = center.sub(MCR::origin);
	const mce::Color mc = color.toMCColor();

	MCR::setColor(mce::Color(1.f, 1.f, 1.f, 1.f));
	MCR::tessellator->begin(VertextFormat::QUAD);

	auto quad = [&](const Vec3<float>& a, const Vec3<float>& b, const Vec3<float>& d, const Vec3<float>& e) {
		MCR::tessellator->color(mc.r, mc.g, mc.b, mc.a);
		MCR::tessellator->vertex(a.x, a.y, a.z);
		MCR::tessellator->vertex(b.x, b.y, b.z);
		MCR::tessellator->vertex(d.x, d.y, d.z);
		MCR::tessellator->vertex(e.x, e.y, e.z);

		// Double sided so every face is visible from either side.
		MCR::tessellator->vertex(e.x, e.y, e.z);
		MCR::tessellator->vertex(d.x, d.y, d.z);
		MCR::tessellator->vertex(b.x, b.y, b.z);
		MCR::tessellator->vertex(a.x, a.y, a.z);
		};

	for (int i = 0; i < segments; i++) {
		const float a0 = ((float)i / (float)segments) * 2.f * PI;
		const float a1 = ((float)(i + 1) / (float)segments) * 2.f * PI;

		const float c0 = cosf(a0), s0 = sinf(a0);
		const float c1 = cosf(a1), s1 = sinf(a1);

		const float yt = c.y + half;
		const float yb = c.y - half;

		const Vec3<float> ob(c.x + c0 * outer, yb, c.z + s0 * outer);
		const Vec3<float> ot(c.x + c0 * outer, yt, c.z + s0 * outer);
		const Vec3<float> ib(c.x + c0 * inner, yb, c.z + s0 * inner);
		const Vec3<float> it(c.x + c0 * inner, yt, c.z + s0 * inner);

		const Vec3<float> ob1(c.x + c1 * outer, yb, c.z + s1 * outer);
		const Vec3<float> ot1(c.x + c1 * outer, yt, c.z + s1 * outer);
		const Vec3<float> ib1(c.x + c1 * inner, yb, c.z + s1 * inner);
		const Vec3<float> it1(c.x + c1 * inner, yt, c.z + s1 * inner);

		quad(ob, ot, ot1, ob1);
		quad(it, ib, ib1, it1);
		quad(it, ot, ot1, it1);
		quad(ib, ob, ob1, ib1);
	}

	MeshHelpers::renderMeshImmediately(MCR::screenCtx, MCR::tessellator, MCR::blendMaterial);
}

void KillAura::onLevelRender() {
	if (!ring || target == nullptr) {
		ringLastTarget = nullptr;
		return;
	}

	AABBShapeComponent* aabbShape = target->aabbShape;
	if (aabbShape == nullptr) return;

	const AABB& aabb = aabbShape->aabb;

	const Vec3<float> desired((aabb.lower.x + aabb.upper.x) * 0.5f, 0.f, (aabb.lower.z + aabb.upper.z) * 0.5f);

	if (ringLastTarget != target) {
		ringSmoothed = desired;
		ringLastTarget = target;
	}
	else {
		const float t = std::clamp(MCR::deltaTime * 12.f, 0.f, 1.f);
		ringSmoothed.x += (desired.x - ringSmoothed.x) * t;
		ringSmoothed.z += (desired.z - ringSmoothed.z) * t;
	}

	ringTime += MCR::deltaTime;

	float bob;
	if (ringAnim == 0) {
		float phase = std::fmod(ringTime * ringSpeed, 1.f);
		if (phase < 0.f) phase += 1.f;
		bob = phase < 0.5f ? (phase * 2.f) : ((1.f - phase) * 2.f);
	}
	else {
		bob = std::sin(ringTime * ringSpeed * 2.f * PI) * 0.5f + 0.5f;
	}

	const float y = aabb.lower.y + bob * (aabb.upper.y - aabb.lower.y);

	static const int qualitySegments[3] = { 128, 256, 360 };
	const int segments = qualitySegments[std::clamp(ringQuality, 0, 2)];

	renderRing(Vec3<float>(ringSmoothed.x, y, ringSmoothed.z), ringRadius, ringThickness, ringHeight, segments, ringColor);
}