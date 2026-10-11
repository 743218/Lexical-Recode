#pragma once
#include "../../ModuleBase/Module.h"

class NoRender : public Module {
public:
	bool noFire = false;
	bool noWeather = false;
	bool noEntities = false;
	bool noBlockEntities = false;
	bool noParticles = false;
	bool noScoreboard = false;
public:
	bool* noWeatherBoolPtr = nullptr;
	bool* noEntitiesBoolPtr = nullptr;
	bool* noBlockEntitiesBoolPtr = nullptr;
	bool* noParticlesBoolPtr = nullptr;

	NoRender();

	void onDisable() override;
	void onEnable() override;
	void onClientTick() override;
	void onRecievePacket(Packet* packet, bool& cancel) override;
};