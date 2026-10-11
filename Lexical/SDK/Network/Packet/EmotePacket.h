#pragma once
#include "Packet.h"

class EmotePacket : public Packet {
public:
	uint64_t mRuntimeId;
	std::string mPieceId;
	std::string mXuid;
	std::string mPlatformId;
	uint8_t mFlags;
};