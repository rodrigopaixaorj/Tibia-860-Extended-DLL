/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __DAT_READER_H__
#define __DAT_READER_H__

#include <cstdint>
#include <windows.h>

#pragma pack(push, 1)
struct TibiaThingType {
    uint32_t width;          // 0x00
    uint32_t height;         // 0x04
    uint32_t exactSize;      // 0x08 (32 or realSize)
    uint32_t layers;         // 0x0C
    uint32_t patternX;       // 0x10
    uint32_t patternY;       // 0x14
    uint32_t patternZ;       // 0x18
    uint32_t animCount;      // 0x1C
    uint32_t* sprites;       // 0x20
    uint8_t  flags[4];       // 0x24 (bitmask of attributes 0..31)
    uint32_t groundSpeed;    // 0x28 (attr 0)
    uint32_t maxTextLen;     // 0x2C (attr 8/9)
    uint32_t lightColor;     // 0x30 (attr 21 light[0])
    uint32_t lightIntensity; // 0x34 (attr 21 light[1])
    uint32_t dispX;          // 0x38 (attr 24 disp[0])
    uint32_t dispY;          // 0x3C (attr 24 disp[1])
    uint32_t elevation;      // 0x40 (attr 25 elevation)
    uint32_t minimapColor;   // 0x44 (attr 28 minimapColor)
    uint32_t lensHelp;       // 0x48 (attr 29 lensHelp)
};
#pragma pack(pop)

static_assert(sizeof(TibiaThingType) == 0x4C, "TibiaThingType must be exactly 0x4C (76 bytes)");

struct ThingWrapper {
    uint32_t minId;          // offset 0x00
    uint32_t count;          // offset 0x04
    TibiaThingType* data;    // offset 0x08
};
static_assert(sizeof(ThingWrapper) == 12, "ThingWrapper must be exactly 12 bytes");

enum DatThingCategory {
    DAT_THING_ITEM = 0,
    DAT_THING_CREATURE = 1,
    DAT_THING_EFFECT = 2,
    DAT_THING_MISSILE = 3
};

struct CreatureFrameGroupInfo {
    uint8_t idleAnim = 1;
    uint8_t movingAnim = 0;
    uint32_t idleDurationMs = 150;
};

CreatureFrameGroupInfo GetCreatureFrameGroupInfo(uint16_t lookType);

bool LoadDatFileCustom(const char* datPath);
void InitDatReaderHooks();

#endif // __DAT_READER_H__
