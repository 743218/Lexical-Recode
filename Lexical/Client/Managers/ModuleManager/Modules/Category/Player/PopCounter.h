#pragma once
#include "../../ModuleBase/Module.h"

class PopCounter : public Module {
public:
	std::string popMessage = "[$1!clientname!$r] $c!player!$r popped $a!pops!$r !totem!.";
	std::string deathMessage = "[$1!clientname!$r] $4!player!$c died after popping $a!pops!$4 !totem!.";

	std::string popMessage2 = "@!player! just popped !pops! !totem!";
	std::string deathMessage2 = "@!player! just died after popping !pops! !totem!";

	std::string friendMessage = "[$1!clientname!$r] $b!player!$r popped $a!pops!$r !totem!.";
	std::string selfMessage = "[$1!clientname!$r] $aYou$r popped $a!pops!$r !totem!.";
public:
	bool sendPops = false;
	bool sendPopsOnDeath = false;

	bool countFriends = false;
	bool countSelf = false;
	bool actorEventOnly = false;
private:
	std::unordered_map<uint64_t, bool> didHaveTotem;
	std::unordered_map<uint64_t, int> totemMap;
public:
	int getTotemPops(uint64_t runtimeId) {
		return totemMap[runtimeId];
	}
public:
	std::string sanitize(const std::string& message, const std::string name, const int pops);
	void onActorPop(Actor* actor);
public:
	PopCounter();
	virtual void onRecievePacket(Packet* packet, bool& cancel) override;
	virtual void onNormalTick(LocalPlayer* localPlayer) override;
	virtual void onLoadConfig(void* confVoid) override;
	virtual void onSaveConfig(void* confVoid) override;
	virtual bool runOnBackground() override {
		return true;
	}
};