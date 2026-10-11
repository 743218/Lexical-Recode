#pragma once
#include "../FuncHook.h"

// old dont use
class RakNetHook : public FuncHook {
private:
	using func_t = void(__thiscall*)(RaknetConnector*);
	static inline func_t oFunc;

	static void callback(RaknetConnector* _this) {
		oFunc(_this);
	}
public:
	RakNetHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&callback;
	}
};
