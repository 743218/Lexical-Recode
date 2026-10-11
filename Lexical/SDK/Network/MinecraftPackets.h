#pragma once
#include "Packet/InventoryTransactionPacket.h"
#include "Packet/MobEquipmentPacket.h"
#include "Packet/PlayerAuthInputPacket.h"
#include "Packet/TextPacket.h"
#include "Packet/MovePlayerPacket.h"
#include "Packet/CommandRequestPacket.h"
#include "Packet/LevelEventPacket.h"
#include "Packet/ActorEventPacket.h"
#include "Packet/RemoveActorPacket.h"
#include "Packet/SetScorePacket.h"
#include "Packet/EmotePacket.h"
#include "Packet/AddPlayerPacket.h"
#include "Packet/PlayerActionPacket.h"
#include "Packet/LevelSoundEventPacket.h"

class MinecraftPackets {
public:
	static std::shared_ptr<Packet> createPacket(PacketID id) {
		using func_t = std::shared_ptr<Packet>(__cdecl*)(PacketID);
		static func_t Func = reinterpret_cast<func_t>(Addresses::MinecraftPackets_createPacket);
		return Func(id);
	}
};