#pragma once
#include "../../ModuleBase/Module.h"

class ChatSuffix : public Module {
private:
	bool randomText = false;
	bool greenText = false;
	bool suffix = false;
	bool fancyText = false;
public:
	ChatSuffix() : Module("Chat", "Adds stuff next to ur message", Category::MISC) {
		registerSetting(new BoolSetting("Suffix", "Suffix", &suffix, false));
		registerSetting(new BoolSetting("Fancy Text", "fancy!", &fancyText, false));
		registerSetting(new BoolSetting("Green Text", "Adds > before to ur message", &greenText, false));
		registerSetting(new BoolSetting("Random Text", "NULL", &randomText, randomText));
	}
	virtual void onSendPacket(Packet* packet, bool& cancel) override;
};