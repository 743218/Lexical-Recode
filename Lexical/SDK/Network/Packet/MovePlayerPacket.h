#pragma once
#include "Packet.h"
#include "../../../Utils/Maths.h"

class MovePlayerPacket : public Packet {
public:
	int64_t          mRuntimeId;
	Vec3<float>      mPosition;
	float            headYaw;
	uint8_t          mode;
	bool             onGround;
	uint64_t         ridingRuntimeId;
	int              mTeleportCause;
	__int32          mTeleportItem;
	__int32          mTeleportTick;
};