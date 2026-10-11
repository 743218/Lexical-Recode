#pragma once
#include "../../FuncHook.h"

class StartDestroyBlockHook : public FuncHook {
private:
	using func_t = bool(__thiscall*)(GameMode*, BlockPos*, uint8_t, bool*);
	static inline func_t oFunc;

	static bool StartDestroyBlockCallback(GameMode* _this, BlockPos* pos, uint8_t face, bool* hasDestroyedBlock) {
		static PacketMine* packetMine = ModuleManager::getModule<PacketMine>();
		//static AntiDesync* antiDesync = ModuleManager::getModule<AntiDesync>();
		//if (antiDesync->isEnabled() && Game::getLocalPlayer()->getItemUseDuration() > 0) return false;
		if (packetMine->isEnabled() && packetMine->getPacket() == 0) packetMine->mineBlock(*pos, face);
		if (packetMine->isEnabled() && packetMine->getPacket() == 1) {
			if (!packetMine->getDoubleMine()) _this->destroyProgress = 0.f;
			else {
				if (packetMine->getBreakPos() == BlockPos(0, 0, 0)) _this->destroyProgress = 0.f;
			}
		}
		return oFunc(_this, pos, face, hasDestroyedBlock);
	}
public:
	StartDestroyBlockHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&StartDestroyBlockCallback;
	}
};