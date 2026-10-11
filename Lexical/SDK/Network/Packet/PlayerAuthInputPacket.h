#pragma once
#include "Packet.h"
#include "../../../Utils/Maths.h"
#include <bitset>

class PackedItemuseLegacyInventoryTransaction;
struct ItemStackRequestData;

class PlayerAuthInputPacket : public Packet {
public:
	Vec2<float> rotation;
	Vec3<float> position;
	float headYaw;
	Vec3<float> velocity;
	Vec2<float> mAnalogMoveVector;
	Vec2<float> mVehicleRotation;
	Vec2<float> mMove;
	Vec3<float> mGazeDir;
	std::bitset<39> mInputData;
	int mInputMode;
	int mClientPlayMode;
	int mNewInteractionModel;
	uint64_t mClientTick;
	std::unique_ptr<PackedItemuseLegacyInventoryTransaction> mItemUseTransaction;
	std::unique_ptr<ItemStackRequestData> mItemStackRequest;
	int playerBlockActions;
	uint64_t mPredictedVehicle;
};