#pragma once
#include "../FuncHook.h"

class SendPacketHook : public FuncHook {
private:
	using func_t = void(__thiscall*)(PacketSender*, Packet*);
	static inline func_t oFunc;

	static void SendPacketCallback(PacketSender* _this, Packet* packet) {
		static PacketMine* pMine = ModuleManager::getModule<PacketMine>();
		//static NoPacket* noPacketMod = ModuleManager::getModule<NoPacket>();
		bool cancel = false;
		//if (noPacketMod->isEnabled() && Game::getLocalPlayer() != nullptr) return;

		if (packet->getId() == PacketID::MobEquipment) {
			MobEquipmentPacket* mep = static_cast<MobEquipmentPacket*>(packet);
			if (mep->mContainerId == ContainerID::Inventory)
				PlayerUtil::selectedSlotServerSide = mep->mSelectedSlot;
		}
		if (pMine->isEnabled() && pMine->getMultiTask() == 1) {
			if (packet->getId() == PacketID::InventoryTransaction) pMine->reset();
		}
		ModuleManager::onSendPacket(packet, cancel);
		if (cancel) return;
		oFunc(_this, packet);
	}
public:
	SendPacketHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&SendPacketCallback;
	}
};