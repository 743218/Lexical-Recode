#pragma once
#include "Packet.h"
#include "../../World/ActorRuntimeID.h"
#include "../../World/Item/NetworkItemStackDescriptor.h"
#include "../../World/Inventory/ContainerID.h"

class MobEquipmentPacket : public Packet {
public:
    ActorRuntimeID             mRuntimeId;    // this+0x30
    NetworkItemStackDescriptor mItem;         // this+0x38
    int32_t                    mSlot;         // this+0x98
    int32_t                    mSelectedSlot; // this+0x9C
    ContainerID                mContainerId;  // this+0xA0

    uint8_t mSlotByte;         // this+0xA1
    uint8_t mSelectedSlotByte; // this+0xA2
    uint8_t mContainerIdByte;  // this+0xA3
public:
    MobEquipmentPacket(ActorRuntimeID runtimeId, ItemStack* itemStack, int32_t slot, int32_t selectedSlot, ContainerID containerId = ContainerID::Inventory) {
        memset(this, 0x0, sizeof(MobEquipmentPacket));
        using MobEquimentPacketConstructor_t = void(__thiscall*)(MobEquipmentPacket*, ActorRuntimeID, ItemStack*, int32_t, int32_t, ContainerID);
        static MobEquimentPacketConstructor_t MobEquimentPacketConstructor = reinterpret_cast<MobEquimentPacketConstructor_t>(Addresses::MobEquimentPacket_Constructor);
        MobEquimentPacketConstructor(this, runtimeId, itemStack, slot, selectedSlot, containerId);
    }
};