#pragma once
#include "../../FuncHook.h"

class DestroyBlockHook : public FuncHook {
private:
	using func_t = bool(__thiscall*)(GameMode*, BlockPos const&, uint8_t);
	static inline func_t oFunc;

	static bool callback(GameMode* _this, BlockPos const& blockPos, uint8_t face) {
		if (_this) {
		static PacketMine* packetMine = ModuleManager::getModule<PacketMine>();
		if (packetMine->isEnabled() && packetMine->getMultiTask() == 1 && Game::getLocalPlayer()->getItemUseDuration() > 0) packetMine->reset();
		}
		return oFunc(_this, blockPos, face);
	}
public:
	DestroyBlockHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};
