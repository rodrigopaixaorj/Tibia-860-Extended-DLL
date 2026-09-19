/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "network_protocol.h"
#include "creature_manager.h"
#include "ui_market.h"
#include "main.h"
#include "config.h"
#include "hook.h"

NetworkReader::NetworkReader(const uint8_t* buffer, size_t size)
    : m_buffer(buffer), m_size(size), m_pos(0) {}

bool NetworkReader::canRead(size_t bytes) const {
    return (m_pos + bytes <= m_size);
}

uint8_t NetworkReader::readByte() {
    if (!canRead(1)) return 0;
    return m_buffer[m_pos++];
}

uint16_t NetworkReader::readU16() {
    if (!canRead(2)) return 0;
    uint16_t val = *(uint16_t*)&m_buffer[m_pos];
    m_pos += 2;
    return val;
}

uint32_t NetworkReader::readU32() {
    if (!canRead(4)) return 0;
    uint32_t val = *(uint32_t*)&m_buffer[m_pos];
    m_pos += 4;
    return val;
}

std::string NetworkReader::readString() {
    uint16_t len = readU16();
    if (!canRead(len)) return "";
    std::string str((const char*)&m_buffer[m_pos], len);
    m_pos += len;
    return str;
}

bool ProcessExtendedOpcode(const uint8_t* buffer, size_t size) {
    if (!buffer || size < 2) return false;

    NetworkReader reader(buffer, size);
    uint8_t opcode = reader.readByte();

    // 0x32 = 50 decimal (Extended Opcode)
    if (opcode != 0x32) return false;

    uint8_t subOpcode = reader.readByte();
    switch (subOpcode) {
        case EXT_OPCODE_SET_MOUNT: {
            uint32_t creatureId = reader.readU32();
            uint16_t mountId = reader.readU16();
            uint8_t head = reader.readByte();
            uint8_t body = reader.readByte();
            uint8_t legs = reader.readByte();
            uint8_t feet = reader.readByte();
            CreatureManager::get().setMount(creatureId, mountId, head, body, legs, feet);
            return true;
        }

        case EXT_OPCODE_REMOVE_MOUNT: {
            uint32_t creatureId = reader.readU32();
            CreatureManager::get().removeMount(creatureId);
            return true;
        }

        case EXT_OPCODE_MARKET_DATA: {
            MarketSystem::get().open();
            MarketSystem::get().parseMarketData(buffer + reader.getReadPos(), size - reader.getReadPos());
            return true;
        }
    }

    return false;
}

// Network Decrypt Hook
typedef void(__cdecl* t_DecryptCall)(void* packetBuffer, int packetLength);
static t_DecryptCall o_DecryptCall = nullptr;

static void SafeDecryptCall(void* packetBuffer, int packetLength) {
    if (g_config.extendedOpcode && packetBuffer && packetLength > 2) {
        uint8_t* rawData = static_cast<uint8_t*>(packetBuffer);
        ProcessExtendedOpcode(rawData, static_cast<size_t>(packetLength));
    }
}

static void __cdecl Hooked_DecryptCall(void* packetBuffer, int packetLength) {
    if (o_DecryptCall) {
        o_DecryptCall(packetBuffer, packetLength);
    }

    try {
        SafeDecryptCall(packetBuffer, packetLength);
    } catch (...) {
        // Protection against network parsing faults
    }
}

void InitNetworkHooks() {
    if (g_config.extendedOpcode && g_clientBaseAddr) {
        uint32_t decryptCallAddr = g_clientBaseAddr + 0x5C3A0;
        o_DecryptCall = (t_DecryptCall)decryptCallAddr;
        HookCall(decryptCallAddr, (uintptr_t)&Hooked_DecryptCall);
    }
}
