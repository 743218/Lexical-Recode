#pragma once
#include "../../ModuleBase/Module.h"

class AntiCrystal : public Module {
private:
	float reduce = 0.6f;
	bool jetpackStop = false;
public:
	AntiCrystal();

	virtual void onSendPacket(Packet* packet, bool& cancel) override;
};
