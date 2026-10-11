#include "Timer.h"
#include "../../../ModuleManager.h"

Timer::Timer() : Module("Timer", "Modify TPS (ticks per second) of game", Category::MISC) {
	registerSetting(new SliderSetting<int>("TPS", "ticks per second", &timerValue, 40, 0, 200));
	registerSetting(new BoolSetting("Eat Stop", "Set timer to 20 while eating.", &eatStop, false));
	registerSetting(new BoolSetting("Break Stop", "Set timer to 20 while PacketMine is breaking.", &breakStop, false));
}

void Timer::onDisable() {
	Minecraft* mc = Game::clientInstance->minecraft;
	if (mc != nullptr) {
		*mc->minecraftTimer = 20.0f;
		*mc->minecraftRenderTimer = 20.0f;
	}
}

void Timer::onClientTick() {
	Minecraft* mc = Game::clientInstance->minecraft;
	if (mc != nullptr) {
		float value = (float)timerValue;

		if (eatStop || breakStop) {
			bool shouldStop = false;

			if (eatStop) {
				LocalPlayer* localPlayer = Game::getLocalPlayer();
				if (localPlayer && PlayerUtil::isEating(localPlayer))
					shouldStop = true;
			}

			if (!shouldStop && breakStop) {
				if (auto packetMine = ModuleManager::getModule<PacketMine>()) {
					if (packetMine->isEnabled() && packetMine->getBreakPos() != BlockPos(0, 0, 0))
						shouldStop = true;
				}
			}

			if (shouldStop)
				value = 20.f;
		}

		*mc->minecraftTimer = value;
		*mc->minecraftRenderTimer = value;
	}
}
