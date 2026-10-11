#pragma once
#include "../../ModuleBase/Module.h"

class ReverseStep : public Module {
public:
	ReverseStep() : Module("ReverseStep", "Step but reverse", Category::MOVEMENT) {
	}

	virtual void onNormalTick(LocalPlayer* localPlayer) override;
};