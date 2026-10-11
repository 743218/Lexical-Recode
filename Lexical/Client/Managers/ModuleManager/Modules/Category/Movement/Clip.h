#pragma once
#include "../../ModuleBase/Module.h"

class Clip : public Module {
private:
	float value = 0.25f;
	bool jetpackStop = false;
public:
	Clip();

	void onNormalTick(LocalPlayer* localPlayer) override;
	void onDisable() override;
};
