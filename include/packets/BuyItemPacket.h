#pragma once

#include "Packet.h"

struct PACKED BuyItemPacket : Packet {
    BuyItemPacket() : Packet() {
        this->mType = PacketType::BUYITEM;
        mPacketSize = sizeof(BuyItemPacket) - sizeof(Packet);
    };
    int itemType = -1;
    char name[0x80] = {};
};