#include "Speed.h"
#include "../../../ModuleManager.h"

Speed::Speed() : Module("Speed", "Increase Speed", Category::MOVEMENT) {
	std::string modeStr[] = {
		"Type of speed",
		"Normal: speeds you up by increasing velocity",
		"Timer: speeds you up by increasing the ticks per second of the world",
		"LowHop: Hops very very low like simulating head hitting to be fast asf",
		"IGNStrict: IGN speed bypass, works on all ign servers",
		"IGN: less strict version of IGNStrict",
		"TickShift: Switches between Min TPS and Max TPS randomly"
	};
	std::string modeDesc =
		modeStr[0] + '\n'
		+ modeStr[1] + '\n'
		+ modeStr[2] + '\n'
		+ modeStr[3] + '\n'
		+ modeStr[4] + '\n'
		+ modeStr[5] + '\n'
		+ modeStr[6];
	registerSetting(new EnumSetting("Mode", modeDesc, {"Motion", "Timer", "LowHop", "IGNStrict", "IGN", "TickShift" }, &mode, 0));
	registerSetting(new BoolSetting("AutoJump", "like the name says", &jump, false));
	registerSetting(new BoolSetting("EatSlowdown", "Slows you down when eating", &eatSlow, false));
	registerSetting(new EnumSetting("Sprint", "Type of sprint", { "Always", "Smart" }, &sprintType, 0));
	// Motion
	registerSetting(new SliderSetting<float>("Value", "Speed value", &speed, 1.f, 0.f, 2.f, [&]() -> bool { return mode == 0; }));
	// Timer
	registerSetting(new SliderSetting<int>("TPS", "Ticks per second value, 20 is normal", &tps, 20, 1, 100, [&]() -> bool { return mode == 1; }));
	// LowHop
	registerSetting(new SliderSetting<float>("Speed", "Speed value", &groundSpeed, 1.f, 0.f, 2.f, [&]() -> bool { return mode == 2; }));
	registerSetting(new SliderSetting<float>("Air Speed", "Speed value on air", &airSpeed, 1.f, 0.f, 2.f, [&]() -> bool { return mode == 2; }));
	// IGNStrict
	registerSetting(new SliderSetting<float>("Strict Speed", "Speed value", &ignSpeed2, 1.f, 0.f, 2.f, [&]() -> bool { return mode == 3; }));
	// IGN
	registerSetting(new SliderSetting<float>("IGN Speed", "Speed value", &ignSpeed, 1.f, 0.f, 2.f, [&]() -> bool { return mode == 4; }));
	// TickShift
	registerSetting(new SliderSetting<int>("Min TPS", "NULL", &minTps, minTps, 1, 100, [&]() -> bool { return mode == 5; }));
	registerSetting(new SliderSetting<int>("Max TPS", "NULL", &maxTps, maxTps, 1, 100, [&]() -> bool { return mode == 5; }));
}

std::string Speed::getModeText() {
	Minecraft* mc = Game::clientInstance->minecraft;
	float curTps = 0.f;
	if (!mc) curTps = 0.f;
	else curTps = *mc->minecraftTimer;
	return mode == 0 ? "Motion" : mode == 1 ? "Timer" : mode == 2 ? "LowHop" : mode == 3 ? "IGNStrict" : mode == 4 ? "IGN" : "TickShift [" + std::to_string(curTps) + "]";
}
void Speed::onNormalTick(LocalPlayer* localPlayer) {
	Minecraft* mc = Game::clientInstance->minecraft;
	if (!mc) return;
	static Jetpack* jetPack = ModuleManager::getModule<Jetpack>();
	if (jetPack->isEnabled()) return;
	static bool wasOnGround = false;
	const float& yaw = MoveUtil::getAdjustedYaw();
	if (localPlayer->onGround()) wasOnGround = true;
	if (wasOnGround && !localPlayer->onGround()) wasOnGround = false;
	if (mode != 2) {
		if (sprintType == 0) localPlayer->setSprinting(true);
		if (sprintType == 1 && localPlayer->getItemUseDuration() <= 0) localPlayer->setSprinting(true);
		else if (sprintType == 1 && localPlayer->getItemUseDuration() > 0) localPlayer->setSprinting(false);
	}
	if (jump && localPlayer->onGround()) localPlayer->jumpFromGround();
	if (mode == 0) MoveUtil::setSpeed(speed);
	else if (mode == 1) {
		if (MoveUtil::isMoving()) {
			*mc->minecraftTimer = (float)tps;
			*mc->minecraftRenderTimer = (float)tps;
		}
		else {
			*mc->minecraftTimer = (float)20;
			*mc->minecraftRenderTimer = (float)20;
		}
	}
	else if (mode == 2) {
		float realSpeed = 0.2f * groundSpeed; // normalized
		float realJumpSpeed = 0.3f * airSpeed; // normalized
		float calcYaw = (yaw + 90) * (PI / 180);
		static constexpr float currSpeed = 0.15f;
		Vec3<float> moveVec2 = Vec3<float>(cos(calcYaw) * currSpeed, localPlayer->stateVector->velocity.y, sin(calcYaw) * currSpeed);
		localPlayer->setSprinting(localPlayer->getItemUseDuration() <= 0);
		if (!localPlayer->isInLiquid()) {
			if ((localPlayer->getItemUseDuration() <= 0 && eatSlow) || !eatSlow) {
				if (!localPlayer->hasHorizontalCollision()) {
					if (Game::isKeyDown('W')) {
						if (!localPlayer->onGround()) MoveUtil::setSpeed(realJumpSpeed);
						if (wasOnGround) MoveUtil::setSpeed(realSpeed);
						if (localPlayer->onGround()) localPlayer->stateVector->velocity.y = 0.4f;
					}
					else if (Game::isKeyDown('S') || Game::isKeyDown('A') || Game::isKeyDown('D')) MoveUtil::setSpeed(0.3f);
					if (!Game::isKeyDown(VK_SPACE) && wasOnGround) localPlayer->stateVector->velocity.y = -1.f;
				}
			}
			if (localPlayer->getItemUseDuration() > 0 && Game::isKeyDown('W') && eatSlow) localPlayer->lerpMotion(moveVec2);
		}
	}
	else if (mode == 3 || mode == 4) {
		if (MoveUtil::isMoving()) {
			if (localPlayer->onGround()) localPlayer->stateVector->velocity.y = 0.4f;
			if (!Game::isKeyDown(VK_SPACE) && wasOnGround) {
				localPlayer->stateVector->velocity.y = 0.0000001f;
				localPlayer->stateVector->velocity.y -= 0.0000001f / 3;
			}
		}
		if (mode == 3) {
			if (localPlayer->getItemUseDuration() > 0) localPlayer->setSprinting(false);
			else localPlayer->setSprinting(true);
		}
		const float multiplier = mode == 3 ? ignSpeed2 : ignSpeed;
		float currSpeed = (localPlayer->onGround() ? 0.33f : 0.36f) * multiplier;
		bool bound = false;
		if (!localPlayer->isInLiquid()) {
			if ((localPlayer->getItemUseDuration() <= 0 && mode == 3) || mode == 4) {
				if (!Game::isKeyDown(VK_SHIFT) && localPlayer->onGround()) currSpeed = 0.42f * multiplier;
				else if (!Game::isKeyDown(VK_SHIFT) && !localPlayer->onGround() && !bound) {
					currSpeed = 0.42f * multiplier;
					bound = true;
				}
			}
			if (localPlayer->getItemUseDuration() > 0 && !Game::isKeyDown(VK_SHIFT) && mode == 3) currSpeed = 0.36f * multiplier;
		}
		else {
			if (Game::isKeyDown(VK_SHIFT)) {
				if (mode == 3) {
					if (localPlayer->getItemUseDuration() <= 0) currSpeed = 0.39f * multiplier;
					else currSpeed = 0.35f * multiplier;
				}
				else currSpeed = 0.39f * multiplier;
			}
		}
		if (bound) {
			currSpeed *= (0.94f * multiplier);
			if (localPlayer->onGround()) bound = false;
		}
		float calcYaw = (yaw + 90) * (PI / 180);
		Vec3<float> moveVec2 = Vec3<float>(cos(calcYaw) * currSpeed, localPlayer->stateVector->velocity.y, sin(calcYaw) * currSpeed);
		if (MoveUtil::isMoving()) localPlayer->lerpMotion(moveVec2);
	}
	else if (mode == 5) {
		float curTps = Math::randomFloat(minTps, maxTps);
		if (MoveUtil::isMoving()) {
			*mc->minecraftTimer = (float)curTps;
			*mc->minecraftRenderTimer = (float)curTps;
		}
		else {
			*mc->minecraftTimer = (float)20.f;
			*mc->minecraftRenderTimer = (float)20.f;
		}
	}
}

void Speed::onDisable() {
	if (!Game::getLocalPlayer()) return;
	Minecraft* mc = Game::clientInstance->minecraft;
	if (!mc) return;
	*mc->minecraftTimer = 20;
}