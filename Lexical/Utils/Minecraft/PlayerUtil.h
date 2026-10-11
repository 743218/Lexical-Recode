#pragma once
#include "../../SDK/Game.h"

namespace PlayerUtil {
	extern uint32_t selectedSlotServerSide;
	bool canPlaceBlock(const BlockPos& blockPos, bool airPlace = false);
	bool tryPlaceBlock(const BlockPos& blockPos, bool airPlace = false, bool packetPlace = false);
	bool tryPlaceBlockSupport(const BlockPos& pos, int proximity = 5, bool airplace = false, bool packetPlace = false);
	inline void SendTextMessage(const std::string& message) {
		std::shared_ptr<Packet> sharedPkt = MinecraftPackets::createPacket(PacketID::Text);
		TextPacket* textPacket = (TextPacket*)sharedPkt.get();
		textPacket->mType = TextPacketType::Chat;
		textPacket->mAuthor = *Game::getLocalPlayer()->getNameTag();
		textPacket->mMessage = message;
		textPacket->mLocalize = false;
		textPacket->mPlatformId = textPacket->mPlatformId;
		textPacket->mXuid = textPacket->mXuid;
		Game::getLocalPlayer()->level->getPacketSender()->send(textPacket);
	}
}

namespace MoveUtil {
	bool isMoving();
	Vec2<float> getMotion(const float& speed);
	float getAdjustedYaw();
	void setSpeed(const float& speed);
	inline void stopMotion() {
		Game::getLocalPlayer()->stateVector->velocity.x = 0.f;
		Game::getLocalPlayer()->stateVector->velocity.z = 0.f;
	}

	inline Vec3<float>& getVelocity() {
		return Game::getLocalPlayer()->stateVector->velocity;
	}
}