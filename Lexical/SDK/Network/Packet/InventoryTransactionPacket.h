#pragma once
#include "Packet.h"
#include "../../World/Inventory/Transaction/ComplexInventoryTransaction.h"
#include <memory>
#include <cstring>

class InventoryTransactionPacket : public Packet {
private:
    char pad_0x30[0x48];   // 0x30 .. 0x78
    char pad_0x78[0x10];   // 0x78 .. 0x88   (native tail, keeps sizeof == 0x88)
public:
    InventoryTransactionPacket(
        std::unique_ptr<ComplexInventoryTransaction> transaction,
        bool isClientSide = false
    ) {
        memset(this, 0x0, sizeof(InventoryTransactionPacket));
        using InvTransactionPacket_ctt = void(__thiscall*)(
            InventoryTransactionPacket*,
            std::unique_ptr<ComplexInventoryTransaction>,
            bool
            );
        static InvTransactionPacket_ctt constructor =
            reinterpret_cast<InvTransactionPacket_ctt>(
                Addresses::InventoryTransactionPacket_Constructor
                );
        constructor(this, std::move(transaction), isClientSide);
    }

    // Reads the raw pointer the game installed at 0x78.
    // (This must match kTransactionPointerOffsets in Dupe.cpp.)
    ComplexInventoryTransaction* getTransaction() {
        return *reinterpret_cast<ComplexInventoryTransaction**>(
            reinterpret_cast<uintptr_t>(this) + 0x78
            );
    }

    bool isClientSide() const {
        return *reinterpret_cast<const bool*>(
            reinterpret_cast<uintptr_t>(this) + 0x80
            );
    }
};