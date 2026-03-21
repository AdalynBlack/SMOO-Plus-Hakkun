#pragma once

#include "../Packet.h"

struct PACKED ArchipelagoChatMessage : Packet {
    ArchipelagoChatMessage() : Packet() {
        this->mType = PacketType::APCHATMESSAGE;
        mPacketSize = sizeof(ArchipelagoChatMessage) - sizeof(Packet);
    };
    char message1[MESSAGESIZE] = {};
    char message2[MESSAGESIZE] = {};
    char message3[MESSAGESIZE] = {};
};