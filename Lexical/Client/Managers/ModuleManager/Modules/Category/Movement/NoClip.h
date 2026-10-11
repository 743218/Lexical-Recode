#pragma once
#include "../../ModuleBase/Module.h"

class NoClip : public Module {
private:
	float hSpeed = 1.5f;
	float vSpeed = 1.5f;
public:
	NoClip();
	virtual void onNormalTick(LocalPlayer* lp) override;
	virtual void onDisable() override;
};