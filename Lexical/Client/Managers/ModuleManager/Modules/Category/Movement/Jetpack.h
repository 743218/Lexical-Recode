#pragma once
#include "../../ModuleBase/Module.h"

class Jetpack : public Module {
private:
	bool fastStop = false;
	float speed = 1.f;
public:
	Jetpack();
	void onNormalTick(LocalPlayer* localPlayer) override;
	void onDisable() override;
};
