#pragma once
#include "Packet.h"

class ActorEventPacket : public Packet {
public:
	enum class ActorEventID : uint32_t {
		TotemUse = 0x7F41,
		Feed = 0x39
	};
	int64_t mRuntimeId;
	ActorEventID mEventId;
	int mData;
};