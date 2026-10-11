#include "Fly.h"
#include "../../../ModuleManager.h"

Fly::Fly() : Module("Fly", "Fly like superman", Category::MOVEMENT) {
	registerSetting(new SliderSetting<float>("HSpeed", "Horizontal speed", &hSpeed, 1.f, 0.2f, 3.f));
	registerSetting(new SliderSetting<float>("VSpeed", "Vertical speed", &vSpeed, 0.5f, 0.2f, 3.f));
	registerSetting(new SliderSetting<float>("Glide", "Glide down slowly", &glide, 0.f, -0.15f, 0.f));
}

void Fly::onNormalTick(LocalPlayer* localPlayer) {
	Vec3<float>& velocity = localPlayer->stateVector->velocity;
	velocity = Vec3<float>(0.f, 0.f, 0.f);
	velocity.y -= -glide;
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