#pragma once
#include "../../Utils/Maths.h"
#include "Packet.h"

class AddPlayerPacket : public Packet {
public:
	std::vector<void*> mLink;
	std::string name;
	char mUUID[16];
	int64_t mUniqueId;
	uint64_t mRuntimeId;
	std::string mPlatformId;
	Vec3<float> mPos;
	Vec3<float> mVelocity;
	Vec2<float> mRot;
};

class AddActorPacket : public Packet {
public:
	std::vector<void*> mLink;
	Vec3<float> mPos;
	Vec3<float> mVelocity;
	Vec2<float> mRot;
	float mYHeadRot;
	float mYBodyRot;
	int64_t mUniqueId;
	uint64_t mRuntimeId;
	std::string mNamespace; // like minecraft:pig
};