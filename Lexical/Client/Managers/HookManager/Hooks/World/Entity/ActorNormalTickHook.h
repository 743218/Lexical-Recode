#pragma once
#include "../../Utils/NetworkUtil.h"
#include "../../FuncHook.h"

class ActorNormalTickHook : public FuncHook {
protected:
	static inline bool once = false;
private:
	using func_t = void(__thiscall*)(Actor*);
	static inline func_t oFunc;

	static void ActorNormalTickCallback(Actor* _this) {
		LocalPlayer* localPlayer = Game::getLocalPlayer();
		if (localPlayer == _this) {
			ModuleManager::onNormalTick(localPlayer);
		}

		oFunc(_this);
	}
public:
	ActorNormalTickHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&ActorNormalTickCallback;
	}
	~ActorNormalTickHook() {
		once = false;
	}
};