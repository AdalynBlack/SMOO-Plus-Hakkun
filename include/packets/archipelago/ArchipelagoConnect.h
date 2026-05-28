#pragma once

#include "../Packet.h"

struct PACKED ArchipelagoConnect : Packet {
    ArchipelagoConnect() : Packet() {
        this->mType = PacketType::APCONNECT;
        mPacketSize = sizeof(ArchipelagoConnect) - sizeof(Packet);
    };
    char hostName[APNAMESIZE] = {};
    ushort port = 38281;
    char slotName[APNAMESIZE] = {};
    char password[APNAMESIZE] = {};
};