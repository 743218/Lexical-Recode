#pragma once
#include "../../ModuleBase/Module.h"

class Fly : public Module {
private:
	float hSpeed = 1.f;
	float vSpeed = 0.5f;
	float glide = 0.0f;
public:
	Fly();

	void onNormalTick(LocalPlayer* localPlayer) override;
};