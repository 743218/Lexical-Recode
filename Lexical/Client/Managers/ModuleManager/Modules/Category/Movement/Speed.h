#pragma once
#include "../../ModuleBase/Module.h"

class Speed : public Module {
protected:
	float ignSpeed = 1.f;
	float ignSpeed2 = 1.f;
	float speed = 1.f;
	float groundSpeed = 0.4f;
	float airSpeed = 0.3f;
	bool jump = false;
	bool eatSlow = false;
	int sprintType = 0;
	int tps = 20;
	int mode = 0;
	int minTps = 20;
	int maxTps = 20;
public:
	Speed();
	virtual void onNormalTick(LocalPlayer* localPlayer);
	virtual void onDisable();
	virtual std::string getModeText() override;
};