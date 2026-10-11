#pragma once
#include "../../FuncHook.h"

class ActorShouldRenderHook : public FuncHook {
private:
	using func_t = bool(__thiscall*)(Actor*);
	static inline func_t oFunc;

	static bool callback(Actor* _this) {
		//static CrystalChams* crystalCham = ModuleManager::getModule<CrystalChams>();
		//if (crystalCham->isEnabled() && _this->getActorTypeComponent()->id == 71) return false;
		return oFunc(_this);
	}
public:
	ActorShouldRenderHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};