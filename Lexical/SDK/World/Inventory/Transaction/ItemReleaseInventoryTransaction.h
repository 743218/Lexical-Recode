#pragma once
#include "ComplexInventoryTransaction.h"
#include "../../Item/NetworkItemStackDescriptor.h"

class ItemReleaseInventoryTransaction : public ComplexInventoryTransaction {
public:
	enum class ActionType : int32_t {
		Release = 0,
		Use = 1
	};

	ActionType actionType;
	int32_t slot;
	NetworkItemStackDescriptor itemInHand;
	Vec3<float> mPos;
};