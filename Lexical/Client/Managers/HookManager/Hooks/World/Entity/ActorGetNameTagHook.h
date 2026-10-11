#pragma once
#include "../../FuncHook.h"

class ActorGetNameTagHook : public FuncHook {
private:
	using func_t = std::string*(__thiscall*)(Actor*);
	static inline func_t oFunc;

	static std::string* callback(Actor* _this) {
		return oFunc(_this);
	}
public:
	ActorGetNameTagHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};