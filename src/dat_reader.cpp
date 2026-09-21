/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "dat_reader.h"
#include "main.h"
#include "config.h"
#include "hook.h"
#include "ui_market.h"
#include <cstdio>
#include <vector>
#include <string>

typedef void* (__cdecl *_client_malloc)(size_t size);
typedef void  (__cdecl *_client_free)(void* ptr);
typedef void  (__cdecl *_ClearDatData)();

static inline uint32_t readLE32(FILE* f) {
    uint32_t val = 0;
    fread(&val, 4, 1, f);
    return val;
}

static inline uint16_t readLE16(FILE* f) {
    uint16_t val = 0;
    fread(&val, 2, 1, f);
    return val;
}

bool LoadDatFileCustom(const char* datPath) {
    if (!datPath || !datPath[0]) {
        datPath = GetFileName ? GetFileName(3, false) : "Tibia.dat";
    }

    FILE* f = nullptr;
    fopen_s(&f, datPath, "rb");
    if (!f) {
        char errMsg[512];
        sprintf_s(errMsg, sizeof(errMsg), "[ExtendedDLL] Failed to open DAT file '%s'.", datPath);
        OutputDebugStringA(errMsg);
        return false;
    }

    _client_malloc client_malloc = (_client_malloc)(g_clientBaseAddr + 0x1719E2);
    _client_free   client_free   = (_client_free)(g_clientBaseAddr + 0x171A0D);
    _ClearDatData  ClearDatData  = (_ClearDatData)(g_clientBaseAddr + 0x106D20);

    if (ClearDatData) {
        ClearDatData();
    }

    uint32_t signature = readLE32(f);
    uint16_t itemsCount = readLE16(f);
    uint16_t outfitsCount = readLE16(f);
    uint16_t effectsCount = readLE16(f);
    uint16_t missilesCount = readLE16(f);

    *(uint32_t*)(g_clientBaseAddr + 0x3998E0) = signature;

    // Detect DAT file format features
    bool isModernDat = (signature == 0x4A10 || itemsCount > 20000 || g_config.extendedSprites);
    bool hasFrameGroups = isModernDat;
    bool hasAnimators = isModernDat;
    bool isExtendedSprites = (g_config.extendedSprites || isModernDat);

    // 1. Allocate Items Wrapper (ID 100 to itemsCount)
    ThingWrapper* itemsWrap = (ThingWrapper*)client_malloc(sizeof(ThingWrapper));
    itemsWrap->minId = 100;
    itemsWrap->count = (itemsCount >= 100) ? (itemsCount - 100 + 1) : 0;
    itemsWrap->data = (TibiaThingType*)client_malloc(itemsWrap->count * sizeof(TibiaThingType));
    memset(itemsWrap->data, 0, itemsWrap->count * sizeof(TibiaThingType));
    *(ThingWrapper**)(g_clientBaseAddr + 0x3998DC) = itemsWrap;

    // 2. Allocate Outfits Wrapper (ID 1 to outfitsCount)
    ThingWrapper* outfitsWrap = (ThingWrapper*)client_malloc(sizeof(ThingWrapper));
    outfitsWrap->minId = 1;
    outfitsWrap->count = outfitsCount;
    outfitsWrap->data = (TibiaThingType*)client_malloc(outfitsWrap->count * sizeof(TibiaThingType));
    memset(outfitsWrap->data, 0, outfitsWrap->count * sizeof(TibiaThingType));
    *(ThingWrapper**)(g_clientBaseAddr + 0x3998D8) = outfitsWrap;

    // 3. Allocate Effects Wrapper (ID 1 to effectsCount)
    ThingWrapper* effectsWrap = (ThingWrapper*)client_malloc(sizeof(ThingWrapper));
    effectsWrap->minId = 1;
    effectsWrap->count = effectsCount;
    effectsWrap->data = (TibiaThingType*)client_malloc(effectsWrap->count * sizeof(TibiaThingType));
    memset(effectsWrap->data, 0, effectsWrap->count * sizeof(TibiaThingType));
    *(ThingWrapper**)(g_clientBaseAddr + 0x3998D4) = effectsWrap;

    // 4. Allocate Missiles Wrapper (ID 1 to missilesCount)
    ThingWrapper* missilesWrap = (ThingWrapper*)client_malloc(sizeof(ThingWrapper));
    missilesWrap->minId = 1;
    missilesWrap->count = missilesCount;
    missilesWrap->data = (TibiaThingType*)client_malloc(missilesWrap->count * sizeof(TibiaThingType));
    memset(missilesWrap->data, 0, missilesWrap->count * sizeof(TibiaThingType));
    *(ThingWrapper**)(g_clientBaseAddr + 0x3998D0) = missilesWrap;

    auto parseThing = [&](DatThingCategory category, uint16_t thingId, TibiaThingType* thing) -> bool {
        while (true) {
            int c = fgetc(f);
            if (c == EOF) return false;
            uint8_t attr = static_cast<uint8_t>(c);
            if (attr == 0xFF) break;

            if (isModernDat) {
                if (attr == 16) attr = 252;
                else if (attr > 16) attr -= 1;
            }

            switch (attr) {
                case 0: // Ground
                    thing->flags[0] |= (1 << 0);
                    thing->groundSpeed = readLE16(f);
                    break;
                case 1: thing->flags[0] |= (1 << 1); break; // GroundBorder (Top1)
                case 2: thing->flags[0] |= (1 << 2); break; // OnBottom (Top2)
                case 3: thing->flags[0] |= (1 << 3); break; // OnTop (Top3)
                case 4: thing->flags[0] |= (1 << 4); break; // Container
                case 5: thing->flags[0] |= (1 << 5); break; // Stackable
                case 6: thing->flags[0] |= (1 << 6); break; // ForceUse
                case 7: thing->flags[0] |= (1 << 7); break; // MultiUse
                case 8: // Writable
                    thing->flags[1] |= (1 << 0);
                    thing->maxTextLen = readLE16(f);
                    break;
                case 9: // WritableOnce
                    thing->flags[1] |= (1 << 1);
                    thing->maxTextLen = readLE16(f);
                    break;
                case 10: thing->flags[1] |= (1 << 2); break; // FluidContainer
                case 11: thing->flags[1] |= (1 << 3); break; // Splash
                case 12: thing->flags[1] |= (1 << 4); break; // NotWalkable
                case 13: thing->flags[1] |= (1 << 5); break; // NotMoveable
                case 14: thing->flags[1] |= (1 << 6); break; // BlockProjectile
                case 15: thing->flags[1] |= (1 << 7); break; // NotPathable
                case 16: thing->flags[2] |= (1 << 0); break; // Pickupable
                case 17: thing->flags[2] |= (1 << 1); break; // Hangable
                case 18: thing->flags[2] |= (1 << 2); break; // HookSouth
                case 19: thing->flags[2] |= (1 << 3); break; // HookEast
                case 20: thing->flags[2] |= (1 << 4); break; // Rotateable
                case 21: // Light
                    thing->flags[2] |= (1 << 5);
                    thing->lightColor = readLE16(f);
                    thing->lightIntensity = readLE16(f);
                    break;
                case 22: thing->flags[2] |= (1 << 6); break; // DontHide
                case 23: thing->flags[2] |= (1 << 7); break; // Translucent
                case 24: // Displacement
                    thing->flags[3] |= (1 << 0);
                    thing->dispX = readLE16(f);
                    thing->dispY = readLE16(f);
                    break;
                case 25: // Elevation
                    thing->flags[3] |= (1 << 1);
                    thing->elevation = readLE16(f);
                    break;
                case 26: thing->flags[3] |= (1 << 2); break; // LyingCorpse
                case 27: thing->flags[3] |= (1 << 3); break; // AnimateAlways
                case 28: // MinimapColor
                    thing->flags[3] |= (1 << 4);
                    thing->minimapColor = readLE16(f);
                    break;
                case 29: // LensHelp
                    thing->flags[3] |= (1 << 5);
                    thing->lensHelp = readLE16(f);
                    break;
                case 30: thing->flags[3] |= (1 << 6); break; // FullGround
                case 31: thing->flags[3] |= (1 << 7); break; // LookThrough
                case 32: { // Cloth
                    readLE16(f);
                    break;
                }
                case 33: { // Market Data
                    uint16_t category = readLE16(f);
                    uint16_t tradeAs = readLE16(f);
                    uint16_t showAs = readLE16(f);
                    uint16_t nameLen = readLE16(f);
                    std::string itemName;
                    if (nameLen > 0) {
                        std::vector<char> strBuf(nameLen + 1, 0);
                        fread(strBuf.data(), 1, nameLen, f);
                        itemName = strBuf.data();
                    }
                    uint16_t reqVoc = readLE16(f);
                    uint16_t reqLvl = readLE16(f);

                    MarketSystem::get().registerMarketItem(tradeAs, itemName, category, reqLvl, reqVoc);
                    break;
                }
                case 34: { // DefaultAction
                    readLE16(f);
                    break;
                }
                case 35: // Wrapable
                case 36: // Unwrapable
                case 37: // TopEffect
                case 252: // NoMoveAnimation
                case 253: // Usable
                case 254: // Chargeable
                    break;
                default:
                    // Skip unknown modern attributes
                    break;
            }
        }

        uint8_t numGroups = 1;
        if (category == DAT_THING_CREATURE && hasFrameGroups) {
            numGroups = static_cast<uint8_t>(fgetc(f));
        }

        bool needCopyToIdle = false;
        for (uint8_t g = 0; g < numGroups; ++g) {
            uint8_t groupType = 0;
            if (category == DAT_THING_CREATURE && hasFrameGroups) {
                groupType = static_cast<uint8_t>(fgetc(f));
                if (groupType == 1 && numGroups == 1) {
                    needCopyToIdle = true;
                }
            }

            uint8_t w = static_cast<uint8_t>(fgetc(f));
            uint8_t h = static_cast<uint8_t>(fgetc(f));
            uint8_t realSize = 32;
            if (w > 1 || h > 1) {
                realSize = static_cast<uint8_t>(fgetc(f));
            }
            uint8_t layers = static_cast<uint8_t>(fgetc(f));
            uint8_t px = static_cast<uint8_t>(fgetc(f));
            uint8_t py = static_cast<uint8_t>(fgetc(f));
            uint8_t pz = static_cast<uint8_t>(fgetc(f));
            uint8_t anim = static_cast<uint8_t>(fgetc(f));

            if (anim > 1 && hasAnimators) {
                fgetc(f); // asyncAnim
                readLE32(f); // loopCount
                fgetc(f); // startFrame
                for (int a = 0; a < anim; ++a) {
                    readLE32(f); // minDur
                    readLE32(f); // maxDur
                }
            }

            uint32_t totalSprites = (uint32_t)w * h * layers * px * py * pz * anim;
            uint32_t* sprites = (uint32_t*)client_malloc(totalSprites * sizeof(uint32_t));
            if (!sprites) return false;

            if (isExtendedSprites) {
                fread(sprites, 4, totalSprites, f);
            } else {
                for (uint32_t s = 0; s < totalSprites; ++s) {
                    sprites[s] = readLE16(f);
                }
            }

            bool useThisGroup = false;
            if (g == 0) {
                useThisGroup = true;
            } else if (category == DAT_THING_CREATURE) {
                // For creatures with multiple FrameGroups (Idle and Moving):
                // Prefer the Moving group (groupType == 1) or any group with walking animation frames (anim > thing->animCount)
                if (groupType == 1 || anim > thing->animCount) {
                    useThisGroup = true;
                    if (thing->sprites) {
                        client_free(thing->sprites);
                        thing->sprites = nullptr;
                    }
                }
            }

            if (useThisGroup) {
                thing->width = w;
                thing->height = h;
                thing->exactSize = realSize;
                thing->layers = layers;
                thing->patternX = px;
                thing->patternY = py;
                thing->patternZ = pz;
                thing->animCount = anim;
                thing->sprites = sprites;
            } else {
                client_free(sprites);
            }
        }
        return true;
    };

    // Parse all items
    for (uint16_t id = 100; id <= itemsCount; ++id) {
        uint32_t index = id - itemsWrap->minId;
        if (index < itemsWrap->count) {
            parseThing(DAT_THING_ITEM, id, &itemsWrap->data[index]);
        }
    }

    // Parse all creatures/outfits
    for (uint16_t id = 1; id <= outfitsCount; ++id) {
        uint32_t index = id - outfitsWrap->minId;
        if (index < outfitsWrap->count) {
            parseThing(DAT_THING_CREATURE, id, &outfitsWrap->data[index]);
        }
    }

    // Parse all effects
    for (uint16_t id = 1; id <= effectsCount; ++id) {
        uint32_t index = id - effectsWrap->minId;
        if (index < effectsWrap->count) {
            parseThing(DAT_THING_EFFECT, id, &effectsWrap->data[index]);
        }
    }

    // Parse all missiles
    for (uint16_t id = 1; id <= missilesCount; ++id) {
        uint32_t index = id - missilesWrap->minId;
        if (index < missilesWrap->count) {
            parseThing(DAT_THING_MISSILE, id, &missilesWrap->data[index]);
        }
    }

    fclose(f);

    char logMsg[256];
    sprintf_s(logMsg, sizeof(logMsg), "[ExtendedDLL] Successfully loaded DAT file: Items=%u, Outfits=%u, Effects=%u, Missiles=%u\n",
              itemsCount, outfitsCount, effectsCount, missilesCount);
    OutputDebugStringA(logMsg);

    return true;
}

static void __cdecl HookLoadDatFileProxy(const char* datPath) {
    LoadDatFileCustom(datPath);
}

void InitDatReaderHooks() {
    // Hook the call to LoadDatFile at 0x107558 in 8.60 (VA: g_clientBaseAddr + 0x107558)
    HookCall(g_clientBaseAddr + 0x107558, (DWORD)&HookLoadDatFileProxy);

    // Also HookJMP the entry point of 0x107000 as fallback
    HookJMP(g_clientBaseAddr + 0x107000, (DWORD)&HookLoadDatFileProxy);
}
