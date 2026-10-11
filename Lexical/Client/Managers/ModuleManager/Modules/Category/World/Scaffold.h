#pragma once
#include "../../ModuleBase/Module.h"

class Scaffold : public Module {
public:
	int yPos = 0.f;
	Vec2<float> rotPos;
public:
	bool sameY = false;
	int extend = 1;
	int switchType = 0;
	int sprintType = 0;
	int tps = 20;
	bool clutch = false;
	float placeSpeed = 20.f;
	int iPlaceSpeed = 0;
public:
    bool buildBlock(BlockPos pos);
public:
	Scaffold();
	virtual void onNormalTick(LocalPlayer* localPlayer) override;
	virtual void onEnable();
	void onDisable();
};