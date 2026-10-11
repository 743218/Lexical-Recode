#pragma once
#include "../../ModuleBase/Module.h"

class Timer : public Module {
private:
	int timerValue = 40;
	bool eatStop = false;
	bool breakStop = false;
public:
	Timer();

	void onDisable() override;
	void onClientTick() override;
};