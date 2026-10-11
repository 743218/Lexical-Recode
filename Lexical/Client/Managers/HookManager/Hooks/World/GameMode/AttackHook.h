#pragma once
#include "../../FuncHook.h"

class AttackHook : public FuncHook {
private:
	using func_t = bool(__thiscall*)(GameMode*, Actor*);
	static inline func_t oFunc;

	static bool callback(GameMode* _this, Actor* _actor) {
		static PacketMine* packetMine = ModuleManager::getModule<PacketMine>();
		//static AntiDesync* antiDesync = ModuleManager::getModule<AntiDesync>();
		if (packetMine->isEnabled() && packetMine->getMultiTask() == 1) packetMine->reset();
		//if (antiDesync->isEnabled() && Game::getLocalPlayer()->getItemUseDuration() > 0) return false;
		bool cancel = false;
		ModuleManager::onAttack(_actor, cancel);
		if (cancel) return false;
		return oFunc(_this, _actor);
	}
public:
	AttackHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};