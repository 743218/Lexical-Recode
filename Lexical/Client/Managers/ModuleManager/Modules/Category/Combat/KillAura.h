#pragma once
#include "../../ModuleBase/Module.h"

class KillAura : public Module {
private:
	float range = 5.f;
	float wallRange = 3.f;
	int interval = 1;
	int rotMode = 1;
	bool randomize = true;
	int hitType = 0;
	int hitAttempts = 1;
	int hitChance = 100;
	int autoWeaponMode = 0;
	bool eatStop = false;
	bool includeMobs = false;
	bool hurtTimeCheck = false;
	bool caCompatibility = true;
	bool packetAttack = false;
	bool java = false;
public:
	bool render = false;
	UIColor renderColor = UIColor(255, 255, 255, 255);
private:
	std::vector<Actor*> targetList;
	int oTick = INT_MAX;
	bool shouldRot = false;
	Vec2<float> rotAngle;

	static bool sortByDist(Actor* a1, Actor* a2);
	int getBestWeaponSlot(Actor* target);
	void Attack(Actor* target);
public:
	KillAura();
	
	std::string getModeText() override;
	void onDisable() override;
	void onEnable() override;
	void onNormalTick(LocalPlayer* localPlayer) override;
	void onUpdateRotation(LocalPlayer* localPlayer) override;
	void onSendPacket(Packet* packet, bool& cancel) override;
	void onLevelRender() override;
};