#pragma once
#include "Packet.h"

class CommandRequestPacket : public Packet {
public:
	std::string mCommand;
	uint8_t origin = 0;
	bool isExternal = 0;
	char shit[0x256];
};