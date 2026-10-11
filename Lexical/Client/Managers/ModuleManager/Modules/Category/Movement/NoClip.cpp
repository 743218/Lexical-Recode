#include "NoClip.h"
#include "../../../ModuleManager.h"

NoClip::NoClip() : Module("NoClip", "Phase and flight into 1", Category::MOVEMENT) {
	registerSetting(new SliderSetting<float>("HSpeed", "Speed horizontally", &hSpeed, 1.5f, 0.1f, 2.f));
	registerSetting(new SliderSetting<float>("VSpeed", "Speed vertically", &vSpeed, 1.5f, 0.1f, 2.f));
}

void NoClip::onDisable() {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr) return;
	AABB& aabb = localPlayer->aabbShape->aabb;
	aabb.upper.y = aabb.lower.y + 1.8f;
}

void NoClip::onNormalTick(LocalPlayer* localPlayer) {
	AABB& aabb = localPlayer->aabbShape->aabb;
	aabb.upper.y = aabb.lower.y - 1.8f;

	Vec3<float>& velocity = localPlayer->stateVector->velocity;
	velocity = Vec3<float>(0.f, 0.f, 0.f);

	if (Game::canUseMoveKeys()) {
		float yaw = localPlayer->rotation->presentRot.y;

		bool isForward = Game::isKeyDown('W');
		bool isLeft = Game::isKeyDown('A');
		bool isBackward = Game::isKeyDown('S');
		bool isRight = Game::isKeyDown('D');
		bool isUp = Game::isKeyDown(VK_SPACE);
		bool isDown = Game::isKeyDown(VK_SHIFT);

		Vec2<int> moveValue;

		if (isRight) moveValue.x += 1;
		if (isLeft) moveValue.x += -1;
		if (isForward) moveValue.y += 1;
		if (isBackward) moveValue.y += -1;

		if (isUp) velocity.y += vSpeed;

		if (isDown) velocity.y -= vSpeed;

		float angleRad = (float)std::atan2(moveValue.x, moveValue.y);
		float angleDeg = angleRad * (180.f / PI);
		yaw += angleDeg;

		if ((moveValue.x != 0) || (moveValue.y != 0)) {
			float calcYaw = (yaw + 90.f) * (PI / 180.f);
			Vec3<float> moveVec;
			moveVec.x = cos(calcYaw) * hSpeed;
			moveVec.y = velocity.y;
			moveVec.z = sin(calcYaw) * hSpeed;
			localPlayer->lerpMotion(moveVec);
		}
	}
}