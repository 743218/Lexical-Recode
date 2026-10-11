#pragma once
#include "../../ModuleBase/Module.h"

class HotbarSorter : public Module {
public:
	bool packet = true;
	int maxPackets = 1;

	int endCrystalSlot = 0;
	int bedSlot = 0;
	int nBedSlot = 0;
	int anchorSlot = 0;
	int gappleSlot = 0;
	int glowstoneSlot = 0;
	int obsidianSlot = 0;
	int echest = 0;
	int strength = 0;
public:
	HotbarSorter();
	virtual void onNormalTick(LocalPlayer* localPlayer) override;
};