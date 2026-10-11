#pragma once
#include "../../../Utils/MemoryUtil.h"
#include "../../Network/PacketSender.h"
#include "../Physical/HitResult.h"
#include <vector>

class Actor;

class Level {
public:
	std::vector<Actor*> getRuntimeActorList() {
		std::vector<Actor*> listOut;
		MemoryUtil::CallVFunc<278, decltype(&listOut)>(this, &listOut);
		return listOut;
	}

	PacketSender* getPacketSender() {
		return MemoryUtil::CallVFunc<281, PacketSender*>(this);
	}

	HitResult* getHitResult() {
		return MemoryUtil::CallVFunc<288, HitResult*>(this);
	}
};
