#pragma once
#include "../../ModuleBase/Module.h"

class KillAura : public Module {
private:
	int switchMode = 0;    // 0 = None, 1 = Switch, 2 = Silent
	int targetMode = 0;    // 0 = Single, 1 = Multi
	int rotateMode = 2;

	float range = 4.f;
	int attackDelay = 10;

	bool packet = false;
	bool mobs = false;
	bool silent = false;

	// Swing
	bool swing = true;

	Actor* target = nullptr;
	std::vector<Actor*> targets;
	int weaponSlot = -1;
	int attackTicks = 0;
	int hitTicks = 0;
	int pauseTicks = 0;
	int switchBackSlot = -1;

	// Ring render
	bool ring = false;
	float ringRadius = 0.6f;
	float ringThickness = 0.12f;
	float ringHeight = 0.12f;
	float ringSpeed = 2.f;
	int ringAnim = 1;      // 0 = Robotic, 1 = Smooth
	int ringQuality = 1;   // 0 = 128, 1 = 256, 2 = 360
	UIColor ringColor = UIColor(255, 255, 255, 255);

	Actor* ringLastTarget = nullptr;
	Vec3<float> ringSmoothed;
	float ringTime = 0.f;

	void renderRing(const Vec3<float>& center, float radius, float thickness, float height, int segments, const UIColor& color);

	int getBestWeaponSlot();
	bool isAxe(ItemStack* itemStack);
	bool isSword(ItemStack* itemStack);
	bool isMace(ItemStack* itemStack);
	bool isTrident(ItemStack* itemStack);
	bool isTargetValid(Actor* actor);
	void rotateToTarget();
	void attackActor(Actor* actor);

public:
	KillAura();

	int getWeaponSlot() const { return weaponSlot; }
	std::string getModeText() override;

	void onEnable() override;
	void onUpdateRotation(LocalPlayer* localPlayer) override;
	void onSendPacket(Packet* packet, bool& cancel) override;
	void onNormalTick(LocalPlayer* localPlayer) override;
	void onLevelRender() override;
};