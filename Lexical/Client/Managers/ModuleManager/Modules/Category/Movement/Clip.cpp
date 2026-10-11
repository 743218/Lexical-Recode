#include "Clip.h"
#include "../../../ModuleManager.h"

Clip::Clip() : Module("Clip", "Clip through walls", Category::MOVEMENT) {
	registerSetting(new SliderSetting<float>("Value", "The more the deeper", &value, 0.25f, 0.f, 1.f));
	registerSetting(new BoolSetting("Jetpack Stop", "Reset AABB while Jetpack is enabled.", &jetpackStop, false));
}

void Clip::onNormalTick(LocalPlayer* localPlayer) {
	if (jetpackStop) {
		if (auto jetpack = ModuleManager::getModule<Jetpack>()) {
			if (jetpack->isEnabled()) {
				// Reset to the default player AABB while Jetpack is on.
				Vec3<float> feetPos = localPlayer->getPos();
				Vec3<float>& lower = localPlayer->aabbShape->aabb.lower;
				Vec3<float>& upper = localPlayer->aabbShape->aabb.upper;
				lower.x = feetPos.x - 0.3f;
				lower.y = feetPos.y;
				lower.z = feetPos.z - 0.3f;
				upper.x = feetPos.x + 0.3f;
				upper.y = feetPos.y + 1.8f;
				upper.z = feetPos.z + 0.3f;
				return;
			}
		}
	}

	const float normalized = 0.35f * value;
	Vec3<float>& lower = localPlayer->aabbShape->aabb.lower;
	Vec3<float>& upper = localPlayer->aabbShape->aabb.upper;
	if (upper.x - lower.x > 0.5f) {
		lower.x -= -normalized;
		upper.x += -normalized;
	}
	if (upper.z - lower.z > 0.5f) {
		lower.z -= -normalized;
		upper.z += -normalized;
	}
	if (WorldUtil::getBlock(localPlayer->getPos().floor().sub(0, 1, 0).CastTo<int>())->blockLegacy->blockId == 26) {
		if (MoveUtil::isMoving()) upper.y = lower.y;
	}
	else if (upper.y == lower.y) upper.y += 1.8f;
}

void Clip::onDisable() {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!localPlayer) return;
	const float normalized = 0.35f * value;
	Vec3<float>& lower = localPlayer->aabbShape->aabb.lower;
	Vec3<float>& upper = localPlayer->aabbShape->aabb.upper;
	if (upper.x - lower.x < 0.5f) {
		lower.x += -normalized;
		upper.x -= -normalized;
	}
	if (upper.z - lower.z < 0.5f) {
		lower.z += -normalized;
		upper.z -= -normalized;
	}
}
