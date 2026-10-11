#include "ReverseStep.h"
#include "../../../ModuleManager.h"

void ReverseStep::onNormalTick(LocalPlayer* localPlayer) {
	static Fly* flyMod = ModuleManager::getModule<Fly>();
	static NoClip* clipMod = ModuleManager::getModule<NoClip>();
	static bool wasOnGround = false;
	if (localPlayer->onGround()) wasOnGround = true;
	if (wasOnGround && !localPlayer->onGround()) wasOnGround = false;
	if (flyMod->isEnabled() || clipMod->isEnabled()) return;
	if (!Game::isKeyDown(VK_SPACE) && wasOnGround) localPlayer->stateVector->velocity.y = -1;
}