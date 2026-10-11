#pragma once
#include "../../FuncHook.h"

class BuildBlockHook : public FuncHook {
private:
	using func_t = bool(__thiscall*)(GameMode*, BlockPos const&, uint8_t, bool);
	static inline func_t oFunc;

	static bool callback(GameMode* _this, BlockPos const& blockPos, uint8_t face, bool a4) {
		static PacketMine* packetMine = ModuleManager::getModule<PacketMine>();
		//static AntiDesync* antiDesync = ModuleManager::getModule<AntiDesync>();
		if (packetMine->isEnabled() && packetMine->getMultiTask() == 1) packetMine->reset();
		//if (antiDesync->isEnabled() && Game::getLocalPlayer()->getItemUseDuration() > 0) return false;
		bool cancel = false;
		ModuleManager::onBuild(blockPos, face, cancel);
		if (cancel) return false;
		return oFunc(_this, blockPos, face, a4);
	}
public:
	BuildBlockHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};