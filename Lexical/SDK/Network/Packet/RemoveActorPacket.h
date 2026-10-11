#pragma once
#include "Packet.h"

class RemoveActorPacket : public Packet {
public:
	uint64_t mRuntimeId;
};