#pragma once
#include "../../FuncHook.h"

class DestroyBlockHook : public FuncHook {
private:
	using func_t = bool(__thiscall*)(GameMode*, BlockPos const&, uint8_t);
	static inline func_t oFunc;

	static bool callback(GameMode* _this, BlockPos const& blockPos, uint8_t face) {
		//static Disabler* disablerMod = ModuleManager::getModule<Disabler>();
		//static AntiDesync* antiDesync = ModuleManager::getModule<AntiDesync>();
		static PacketMine* packetMine = ModuleManager::getModule<PacketMine>();
		if (packetMine->isEnabled() && packetMine->getMultiTask() == 1 && Game::getLocalPlayer()->getItemUseDuration() > 0) packetMine->reset();
		//if (antiDesync->isEnabled() && Game::getLocalPlayer()->getItemUseDuration() > 0) return false;
		//if (disablerMod->isEnabled() && disablerMod->mode == 0) Game::getLocalPlayer()->level->getHitResult()->type = HitResultType::AIR;
		return oFunc(_this, blockPos, face);
	}
public:
	DestroyBlockHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};