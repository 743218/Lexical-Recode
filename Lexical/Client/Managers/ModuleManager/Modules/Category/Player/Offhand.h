#pragma once
#include "../../ModuleBase/Module.h"

class Offhand : public Module {
private:
	int itemMode = 0;
	int delay = 0;
	bool packet = false;
	EnumSetting* itemModeEnum = nullptr;
public:
	Offhand();
	std::string getModeText() override;
	void onNormalTick(LocalPlayer* localPlayer) override;
};