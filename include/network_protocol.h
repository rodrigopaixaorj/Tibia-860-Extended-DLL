/*
  Tibia World RPG Client - Extended DLL
  Copyright (C) 2020-2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __NETWORK_PROTOCOL_H__
#define __NETWORK_PROTOCOL_H__

#include <cstdint>
#include <string>

// Sub-Opcodes for extended channel (0x32 / 50)
enum ExtendedSubOpcode : uint8_t {
    EXT_OPCODE_SET_MOUNT        = 0x01,
    EXT_OPCODE_REMOVE_MOUNT     = 0x02,
    EXT_OPCODE_MARKET_DATA      = 0x10,
    EXT_OPCODE_MARKET_RESPONSE  = 0x11
};

class NetworkReader {
public:
    NetworkReader(const uint8_t* buffer, size_t size);

    bool canRead(size_t bytes) const;
    uint8_t readByte();
    uint16_t readU16();
    uint32_t readU32();
    std::string readString();

    size_t getReadPos() const { return m_pos; }
    size_t getSize() const { return m_size; }

private:
    const uint8_t* m_buffer;
    size_t m_size;
    size_t m_pos;
};

void InitNetworkHooks();
bool ProcessExtendedOpcode(const uint8_t* buffer, size_t size);

#endif // __NETWORK_PROTOCOL_H__
