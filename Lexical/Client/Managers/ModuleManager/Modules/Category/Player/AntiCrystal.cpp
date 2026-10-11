#include "AntiCrystal.h"
#include "../../../ModuleManager.h"

AntiCrystal::AntiCrystal() : Module("AntiCrystal", "Uses packets to phase your bottom half", Category::PLAYER) {
	registerSetting(new SliderSetting<float>("Reduce", "Amount to reduce", &reduce, reduce, 0.1f, 1.f));
	registerSetting(new BoolSetting("Jetpack Stop", "Stop phasing while Jetpack is enabled.", &jetpackStop, false));
}

void AntiCrystal::onSendPacket(Packet* packet, bool& cancel) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!localPlayer) return;

	if (jetpackStop) {
		if (auto jetpack = ModuleManager::getModule<Jetpack>()) {
			if (jetpack->isEnabled()) return;
		}
	}

	const Vec3<float>& actorPos = localPlayer->getPos();
	if (packet->getId() == PacketID::PlayerAuthInput) {
		PlayerAuthInputPacket* authPkt = (PlayerAuthInputPacket*)packet;
		authPkt->position.y = (actorPos.y - reduce);
	}
	if (packet->getId() == PacketID::MovePlayer) {
		MovePlayerPacket* movePkt = (MovePlayerPacket*)packet;
		movePkt->mPosition.y = (actorPos.y - reduce);
	}
}
