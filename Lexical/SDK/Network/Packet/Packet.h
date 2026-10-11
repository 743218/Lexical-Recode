#pragma once
#include "../../../Utils/MemoryUtil.h"
#include "../PacketHandlerDispatcherInstance.h"
#include "PacketIDs.h"
#include <string>

class Packet {
public:
	CLASS_MEMBER(PacketHandlerDispatcherInstance*, packetHandler, 0x20);
private:
	char pad_0x0[0x30];
public:
	PacketID getId() {
		return MemoryUtil::CallVFunc<1, PacketID>(this);
	}

	std::string getName() {
		return *MemoryUtil::CallVFunc<2, std::string*>(this, std::string());
	}
};
