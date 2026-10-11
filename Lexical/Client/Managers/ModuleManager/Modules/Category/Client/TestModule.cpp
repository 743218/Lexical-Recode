#include "TestModule.h"

bool silentMode = false;
TestModule::TestModule() : Module("TestModule", "Test", Category::CLIENT) {
	registerSetting(new BoolSetting("Silent", "Dont give out sound!", &silentMode, false));
	registerSetting(new BoolSetting("Packet Logger", "Logs packets", &packetLogger, false));
	registerSetting(new BoolSetting("No PlayerAuth", "Logs packets", &noAuth, false, [&]() -> bool {
		return packetLogger;
		}));
	registerSetting(new BoolSetting("No MovePlayer", "Logs packets", &noMove, false, [&]() -> bool {
		return packetLogger;
		}));
}

void TestModule::onNormalTick(LocalPlayer* lp) {
	Game::DisplayClientMessage("Item ID: %i", InventoryUtil::getHeldItemId());
}

void TestModule::onSendPacket(Packet* packet, bool& cancel) {
	if (packetLogger) {
		if (noAuth && packet->getId() == PacketID::PlayerAuthInput) return;
		if (noMove && packet->getId() == PacketID::MovePlayer) return;
		Game::DisplayClientMessage("Packet Sent: %s", packet->getName().c_str());
	}
	if (silentMode) {
		if (packet->getId() == PacketID::LevelSoundEvent) cancel = true;
	}
}

void TestModule::onMCRender(MinecraftUIRenderContext* renderCtx) {
	
}

void TestModule::onLevelRender() {
}

void TestModule::onD2DRender() {
}

void TestModule::onRecievePacket(Packet* packet, bool& cancel) {
	if (packet->getId() == PacketID::ActorEvent) {
		ActorEventPacket* eventPacket = (ActorEventPacket*)packet;
		if (eventPacket->mEventId == ActorEventPacket::ActorEventID::TotemUse) Game::DisplayClientMessage("Totem is use");
	}
}

void TestModule::onEnable() {
}

void TestModule::onDisable() {
}