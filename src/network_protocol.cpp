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
#include <algorithm>

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
        // Note: EXT_OPCODE_MARKET_DATA removed -- Market opcodes (0xF6-0xF9, 0xDF)
        // are handled directly in HookGetNextOpcode() as server->client packets.
    }

    return false;
}


// Opcode Dispatch Hook
typedef int(__cdecl* t_GetByte)();
static t_GetByte o_GetByte = nullptr;

static inline int ReadNetworkByte() {
    return o_GetByte ? o_GetByte() : -1;
}

static inline uint16_t ReadNetworkU16() {
    int b1 = ReadNetworkByte();
    int b2 = ReadNetworkByte();
    if (b1 == -1 || b2 == -1) return 0;
    return static_cast<uint16_t>((uint8_t)b1 | ((uint8_t)b2 << 8));
}

static inline uint32_t ReadNetworkU32() {
    uint32_t w1 = ReadNetworkU16();
    uint32_t w2 = ReadNetworkU16();
    return w1 | (w2 << 16);
}

// -----------------------------------------------------------------------
// Helper: skip a market offer block (used to consume unknown market data)
// -----------------------------------------------------------------------
static void SkipMarketOfferEntry(bool hasPlayerName) {
    ReadNetworkU32(); // timestamp
    ReadNetworkU16(); // counter
    ReadNetworkU16(); // itemId
    ReadNetworkU16(); // amount
    ReadNetworkU32(); // price
    if (hasPlayerName) {
        uint16_t nameLen = ReadNetworkU16();
        for (uint16_t i = 0; i < nameLen; ++i)
            ReadNetworkByte();
    }
}

// -----------------------------------------------------------------------
// Market opcode handlers (server → client)
// -----------------------------------------------------------------------

// 0xF6: sendMarketEnter — depot items list + balance
static void Handle_MarketEnter() {
    uint64_t bankBalance = ReadNetworkU32() | ((uint64_t)ReadNetworkU32() << 32); // uint64
    uint64_t playerMoney = ReadNetworkU32() | ((uint64_t)ReadNetworkU32() << 32); // uint64
    uint8_t offerCount = static_cast<uint8_t>(ReadNetworkByte());
    uint16_t itemCount = ReadNetworkU16();

    MarketSystem& mkt = MarketSystem::get();
    mkt.clearOffers();
    mkt.setAccountBalance(static_cast<uint32_t>(bankBalance));

    std::map<uint16_t, uint32_t> depotItems;
    for (uint16_t i = 0; i < itemCount; ++i) {
        uint16_t itemId   = ReadNetworkU16();
        uint16_t count    = ReadNetworkU16();
        depotItems[itemId] = count;
    }
    mkt.updateDepotItems(depotItems);
    mkt.open();
    (void)offerCount; (void)playerMoney;
}

// 0xF7: sendMarketLeave — close market
static void Handle_MarketLeave() {
    MarketSystem::get().close();
}

// 0xF8: sendMarketDetail — item detail strings
static void Handle_MarketDetail() {
    uint16_t itemId = ReadNetworkU16();
    // 16 optional strings (empty = u16 0x0000, present = u16 len + bytes)
    static const int NUM_STRINGS = 16;
    std::string strings[NUM_STRINGS];
    for (int s = 0; s < NUM_STRINGS; ++s) {
        uint16_t len = ReadNetworkU16();
        for (uint16_t c = 0; c < len; ++c)
            strings[s] += static_cast<char>(ReadNetworkByte());
    }
    // Buy stats
    bool hasBuyStats  = (ReadNetworkByte() != 0);
    uint32_t buyTx = 0, buyHigh = 0, buyTotal = 0, buyLow = 0;
    if (hasBuyStats) {
        buyTx    = ReadNetworkU32();
        buyHigh  = ReadNetworkU32();
        buyTotal = ReadNetworkU32();
        buyLow   = ReadNetworkU32();
    }
    // Sell stats
    bool hasSellStats = (ReadNetworkByte() != 0);
    uint32_t sellTx = 0, sellHigh = 0, sellTotal = 0, sellLow = 0;
    if (hasSellStats) {
        sellTx    = ReadNetworkU32();
        sellHigh  = ReadNetworkU32();
        sellTotal = ReadNetworkU32();
        sellLow   = ReadNetworkU32();
    }
    MarketSystem::get().updateDetail(itemId, strings, NUM_STRINGS,
        hasBuyStats, buyTx, buyHigh, buyTotal, buyLow,
        hasSellStats, sellTx, sellHigh, sellTotal, sellLow);
}

// 0xF9: sendMarketBrowse — offers list (buy/sell or my offers/history)
static void Handle_MarketBrowse() {
    uint16_t browseId = ReadNetworkU16();
    // MARKETREQUEST_OWN_OFFERS = 0xFFFF, MARKETREQUEST_OWN_HISTORY = 0xFFFE
    const uint16_t OWN_OFFERS  = 0xFFFF;
    const uint16_t OWN_HISTORY = 0xFFFE;

    if (browseId == OWN_OFFERS) {
        // My Buy offers
        uint32_t buyCount = ReadNetworkU32();
        std::vector<MarketOffer> buyOffers;
        for (uint32_t i = 0; i < buyCount; ++i) {
            MarketOffer o;
            o.timestamp  = ReadNetworkU32();
            o.counter    = ReadNetworkU16();
            o.itemId     = ReadNetworkU16();
            o.amount     = ReadNetworkU16();
            o.piecePrice = ReadNetworkU32();
            buyOffers.push_back(o);
        }
        uint32_t sellCount = ReadNetworkU32();
        std::vector<MarketOffer> sellOffers;
        for (uint32_t i = 0; i < sellCount; ++i) {
            MarketOffer o;
            o.timestamp  = ReadNetworkU32();
            o.counter    = ReadNetworkU16();
            o.itemId     = ReadNetworkU16();
            o.amount     = ReadNetworkU16();
            o.piecePrice = ReadNetworkU32();
            sellOffers.push_back(o);
        }
        MarketSystem::get().updateMyOffers(buyOffers, sellOffers);
    } else if (browseId == OWN_HISTORY) {
        // My history
        uint32_t buyCount = ReadNetworkU32();
        std::vector<MarketOffer> buyOffers;
        for (uint32_t i = 0; i < buyCount; ++i) {
            MarketOffer o;
            o.timestamp  = ReadNetworkU32();
            o.counter    = ReadNetworkU16();
            o.itemId     = ReadNetworkU16();
            o.amount     = ReadNetworkU16();
            o.piecePrice = ReadNetworkU32();
            o.state      = static_cast<uint8_t>(ReadNetworkByte());
            buyOffers.push_back(o);
        }
        uint32_t sellCount = ReadNetworkU32();
        std::vector<MarketOffer> sellOffers;
        for (uint32_t i = 0; i < sellCount; ++i) {
            MarketOffer o;
            o.timestamp  = ReadNetworkU32();
            o.counter    = ReadNetworkU16();
            o.itemId     = ReadNetworkU16();
            o.amount     = ReadNetworkU16();
            o.piecePrice = ReadNetworkU32();
            o.state      = static_cast<uint8_t>(ReadNetworkByte());
            sellOffers.push_back(o);
        }
        MarketSystem::get().updateHistory(buyOffers, sellOffers);
    } else {
        // Active offers for a specific item (browseId = itemId)
        uint32_t buyCount = ReadNetworkU32();
        std::vector<MarketOffer> buyOffers;
        for (uint32_t i = 0; i < buyCount; ++i) {
            MarketOffer o;
            o.timestamp  = ReadNetworkU32();
            o.counter    = ReadNetworkU16();
            o.amount     = ReadNetworkU16();
            o.piecePrice = ReadNetworkU32();
            // playerName
            uint16_t nameLen = ReadNetworkU16();
            for (uint16_t c = 0; c < nameLen; ++c)
                o.playerName += static_cast<char>(ReadNetworkByte());
            o.itemId = browseId;
            buyOffers.push_back(o);
        }
        uint32_t sellCount = ReadNetworkU32();
        std::vector<MarketOffer> sellOffers;
        for (uint32_t i = 0; i < sellCount; ++i) {
            MarketOffer o;
            o.timestamp  = ReadNetworkU32();
            o.counter    = ReadNetworkU16();
            o.amount     = ReadNetworkU16();
            o.piecePrice = ReadNetworkU32();
            uint16_t nameLen = ReadNetworkU16();
            for (uint16_t c = 0; c < nameLen; ++c)
                o.playerName += static_cast<char>(ReadNetworkByte());
            o.itemId = browseId;
            sellOffers.push_back(o);
        }
        MarketSystem::get().updateOffers(browseId, buyOffers, sellOffers);
    }
}

// 0xDF: sendCoinBalance — coins
static void Handle_CoinBalance() {
    ReadNetworkByte();  // update flag
    uint32_t coins = ReadNetworkU32(); // coins
    ReadNetworkU32();   // transferable coins
    MarketSystem::get().setCoins(coins);
}

static int __cdecl HookGetNextOpcode() {
    while (true) {
        if (!o_GetByte) return -1;
        int opcode = o_GetByte();
        if (opcode == -1) {
            return -1;
        }

        // Opcode 0xEF (239): sendTibiaTime [uint8 hour] [uint8 minute] (introduced in 8.70)
        if (opcode == 0xEF) {
            ReadNetworkByte(); // hour
            ReadNetworkByte(); // minute
            continue; // Skip 0xEF safely and fetch next valid opcode for 8.60!
        }

        // Opcode 0x9F (159): sendBasicData [uint8 premium] [uint8 vocation] [uint16 spellCount] [spells...]
        if (opcode == 0x9F) {
            ReadNetworkByte(); // premium
            ReadNetworkByte(); // vocation
            uint16_t spellCount = ReadNetworkU16();
            for (uint16_t i = 0; i < spellCount; ++i) {
                ReadNetworkByte(); // spellId
            }
            continue; // Skip 0x9F safely and fetch next valid opcode!
        }

        // Opcode 0x1D (29): sendPing from TFS (GAME_FEATURE_PING >= 860)
        // The vanilla 8.60 client only knows opcode 0x1E for ping (which carries 5 extra bytes).
        // TFS sendPing() sends ONLY the 0x1D byte (no payload). Calling 0x40bee0 would read
        // 5 bytes that don't exist → Utils.cpp Position<=Size-1 crash.
        // Fix: call 0x459f30(0) directly — the internal "send pong response" function — which
        // writes the reply to the outgoing buffer and flushes it WITHOUT reading anything from net.
        if (opcode == 0x1D) {
            if (g_clientBaseAddr) {
                // 0x459f30 = sendPingResponse(int pingId) — sends pong back to server
                ((void(__cdecl*)(int))(g_clientBaseAddr + 0x59F30))(0);
            }
            continue; // Handled: fetch next valid opcode!
        }

        // Opcode 0x32 (50): Extended Opcode (OTClient / Extended Protocol)
        if (opcode == 0x32) {
            int subOpcode = ReadNetworkByte();
            if (subOpcode == EXT_OPCODE_SET_MOUNT) {
                uint32_t creatureId = ReadNetworkU32();
                uint16_t mountId = ReadNetworkU16();
                uint8_t head = static_cast<uint8_t>(ReadNetworkByte());
                uint8_t body = static_cast<uint8_t>(ReadNetworkByte());
                uint8_t legs = static_cast<uint8_t>(ReadNetworkByte());
                uint8_t feet = static_cast<uint8_t>(ReadNetworkByte());
                CreatureManager::get().setMount(creatureId, mountId, head, body, legs, feet);
            } else if (subOpcode == EXT_OPCODE_REMOVE_MOUNT) {
                uint32_t creatureId = ReadNetworkU32();
                CreatureManager::get().removeMount(creatureId);
            }
            continue; // Skip 0x32 and fetch next game opcode!
        }

        // ----------------------------------------------------------------
        // Market opcodes (server → client) — not known by Tibia 8.60 native
        // ----------------------------------------------------------------

        // 0xF6: sendMarketEnter (balance + depot items)
        if (opcode == 0xF6) {
            Handle_MarketEnter();
            continue;
        }

        // 0xF7: sendMarketLeave
        if (opcode == 0xF7) {
            Handle_MarketLeave();
            continue;
        }

        // 0xF8: sendMarketDetail
        if (opcode == 0xF8) {
            Handle_MarketDetail();
            continue;
        }

        // 0xF9: sendMarketBrowse (active offers, own offers, history)
        if (opcode == 0xF9) {
            Handle_MarketBrowse();
            continue;
        }

        // 0xDF: sendCoinBalance
        if (opcode == 0xDF) {
            Handle_CoinBalance();
            continue;
        }

        return opcode;
    }
}

#include <cmath>

static void __cdecl Hooked_ParsePlayerStats() {
    uint32_t health = 0;
    uint32_t maxHealth = 0;
    uint32_t capacity = 0;
    uint32_t expLow = 0;
    uint16_t level = 0;
    uint8_t levelPercent = 0;
    uint32_t mana = 0;
    uint32_t maxMana = 0;
    uint8_t magicLevel = 0;
    uint8_t magicLevelPercent = 0;
    uint8_t soul = 0;
    uint16_t stamina = 0;

    // Check remaining bytes in current network packet buffer safely
    int remaining = 0;
    if (g_clientBaseAddr) {
        uint32_t readPos = *(uint32_t*)(g_clientBaseAddr + 0x3998B4);
        uint32_t endPos  = *(uint32_t*)(g_clientBaseAddr + 0x3998B0);
        if (endPos >= readPos) {
            remaining = static_cast<int>(endPos - readPos);
        }
    }

    // Extended OTServ format (e.g. TFS with GAME_FEATURE_DOUBLE_EXPERIENCE & GAME_FEATURE_DETAILED_EXPERIENCE_BONUS, 41 bytes)
    if (g_config.extendedPlayerStats || remaining >= 41) {
        health = ReadNetworkU16();
        maxHealth = ReadNetworkU16();
        capacity = ReadNetworkU32();
        expLow = ReadNetworkU32();
        ReadNetworkU32(); // high 32 bits of 64-bit experience
        level = ReadNetworkU16();
        levelPercent = static_cast<uint8_t>(ReadNetworkByte());

        // Detailed Experience Bonus (5x uint16 = 10 bytes)
        ReadNetworkU16(); // base xp gain rate
        ReadNetworkU16(); // xp voucher
        ReadNetworkU16(); // low level bonus
        ReadNetworkU16(); // xp boost percentage
        ReadNetworkU16(); // stamina multiplier

        mana = ReadNetworkU16();
        maxMana = ReadNetworkU16();
        magicLevel = static_cast<uint8_t>(ReadNetworkByte());
        magicLevelPercent = static_cast<uint8_t>(ReadNetworkByte());
        soul = static_cast<uint8_t>(ReadNetworkByte());
        stamina = ReadNetworkU16();

        // Detailed XP Bonus tail (3 bytes)
        ReadNetworkU16(); // xp boost time (seconds)
        ReadNetworkByte(); // enable exp boost in store
    } else {
        // Standard CipSoft 8.60 format (24 bytes)
        health = ReadNetworkU16();
        maxHealth = ReadNetworkU16();
        capacity = ReadNetworkU32();
        expLow = ReadNetworkU32();
        level = ReadNetworkU16();
        levelPercent = static_cast<uint8_t>(ReadNetworkByte());
        mana = ReadNetworkU16();
        maxMana = ReadNetworkU16();
        magicLevel = static_cast<uint8_t>(ReadNetworkByte());
        magicLevelPercent = static_cast<uint8_t>(ReadNetworkByte());
        soul = static_cast<uint8_t>(ReadNetworkByte());
        stamina = ReadNetworkU16();
    }

    // Update client internal player stats via native engine functions
    typedef void(__cdecl* t_SetHealth)(uint32_t health, uint32_t maxHealth);
    typedef void(__cdecl* t_SetCapacity)(uint32_t capacity);
    typedef void(__cdecl* t_SetMana)(uint32_t mana, uint32_t maxMana);
    typedef void(__cdecl* t_SetSoul)(uint32_t soul);
    typedef void(__cdecl* t_SetStamina)(uint32_t stamina);
    typedef void(__cdecl* t_SetLevelAndExp)(uint32_t exp, uint32_t level, uint32_t levelPercent);
    typedef void(__cdecl* t_SetMagicLevel)(uint32_t magicLevel, uint32_t magicLevelPercent);

    if (g_clientBaseAddr) {
        ((t_SetHealth)(g_clientBaseAddr + 0x5D540))(health, maxHealth);
        ((t_SetCapacity)(g_clientBaseAddr + 0x5D5A0))(capacity);
        ((t_SetMana)(g_clientBaseAddr + 0x5D560))(mana, maxMana);
        ((t_SetSoul)(g_clientBaseAddr + 0x5D580))(soul);
        ((t_SetStamina)(g_clientBaseAddr + 0x5D590))(stamina);
        ((t_SetLevelAndExp)(g_clientBaseAddr + 0x5EC30))(expLow, level, levelPercent);
        ((t_SetMagicLevel)(g_clientBaseAddr + 0x5EF00))(magicLevel, magicLevelPercent);
    }
}

static int __cdecl Hooked_ReadCreatureLightLevel() {
    uint32_t creaturePtr = 0;
    __asm {
        mov creaturePtr, esi
    }

    // Read mount data sent by TFS in AddCreature (after AddOutfit)
    uint16_t mountId = ReadNetworkU16();
    uint8_t head = 0, body = 0, legs = 0, feet = 0;
    if (mountId != 0) {
        head = static_cast<uint8_t>(ReadNetworkByte());
        body = static_cast<uint8_t>(ReadNetworkByte());
        legs = static_cast<uint8_t>(ReadNetworkByte());
        feet = static_cast<uint8_t>(ReadNetworkByte());
    }

    if (creaturePtr) {
        uint32_t creatureId = *(uint32_t*)creaturePtr;
        if (mountId != 0) {
            CreatureManager::get().setMount(creatureId, mountId, head, body, legs, feet);
        } else {
            CreatureManager::get().removeMount(creatureId);
        }
    }

    // Return the actual light level byte expected by Tibia.exe at this position
    return ReadNetworkByte();
}

static uint16_t __cdecl Hooked_ReadCreatureSpeed() {
    uint16_t rawSpeed = ReadNetworkU16();
    if (rawSpeed == 0) return 0;
    // Tibia New Speed Law formula:
    // speedA = 857.36, speedB = 261.29, speedC = -4795.01
    // Server sends speed / 2; calculate real formulated speed:
    double formulated = std::floor((857.36 * std::log(static_cast<double>(rawSpeed) + 261.29) - 4795.01) + 0.5);
    if (formulated < 1.0) {
        formulated = 1.0;
    }
    return static_cast<uint16_t>(formulated);
}

static void __cdecl Hooked_ParseCreatureOutfit() {
    uint8_t* pBuffer = *(uint8_t**)(g_clientBaseAddr + 0x3998AC);
    uint32_t readPos = *(uint32_t*)(g_clientBaseAddr + 0x3998B4);
    uint32_t endPos  = *(uint32_t*)(g_clientBaseAddr + 0x3998B0);

    uint32_t creatureId = 0;
    if (pBuffer && readPos + 4 <= endPos) {
        creatureId = *(uint32_t*)(pBuffer + readPos);
    }

    // Call original parse function at 0x410FF0
    if (g_clientBaseAddr) {
        ((void(__cdecl*)())(g_clientBaseAddr + 0x10FF0))();
    }

    // Consume mount data sent by TFS for opcode 0x8E (if present)
    readPos = *(uint32_t*)(g_clientBaseAddr + 0x3998B4);
    endPos  = *(uint32_t*)(g_clientBaseAddr + 0x3998B0);
    if (readPos + 2 <= endPos) {
        uint16_t mountId = ReadNetworkU16();
        uint8_t head = 0, body = 0, legs = 0, feet = 0;
        readPos = *(uint32_t*)(g_clientBaseAddr + 0x3998B4);
        if (mountId != 0 && readPos + 4 <= endPos) {
            head = static_cast<uint8_t>(ReadNetworkByte());
            body = static_cast<uint8_t>(ReadNetworkByte());
            legs = static_cast<uint8_t>(ReadNetworkByte());
            feet = static_cast<uint8_t>(ReadNetworkByte());
        }

        if (creatureId != 0) {
            if (mountId != 0) {
                CreatureManager::get().setMount(creatureId, mountId, head, body, legs, feet);
            } else {
                CreatureManager::get().removeMount(creatureId);
            }
        }
    }
}

// Opcode 200 (0xC8): Open Outfit Window
static void __cdecl Hooked_ParseOutfitWindow() {
    uint16_t lookType = ReadNetworkU16();
    uint8_t head   = static_cast<uint8_t>(ReadNetworkByte());
    uint8_t body   = static_cast<uint8_t>(ReadNetworkByte());
    uint8_t legs   = static_cast<uint8_t>(ReadNetworkByte());
    uint8_t feet   = static_cast<uint8_t>(ReadNetworkByte());
    uint8_t addons = static_cast<uint8_t>(ReadNetworkByte());

    // Read current mount if sent by TFS (GAME_FEATURE_MOUNTS)
    uint16_t lookMount = ReadNetworkU16();
    uint8_t mountHead = 0, mountBody = 0, mountLegs = 0, mountFeet = 0;
    if (lookMount != 0) {
        mountHead = static_cast<uint8_t>(ReadNetworkByte());
        mountBody = static_cast<uint8_t>(ReadNetworkByte());
        mountLegs = static_cast<uint8_t>(ReadNetworkByte());
        mountFeet = static_cast<uint8_t>(ReadNetworkByte());
    }

    // Save local player mount
    if (g_clientBaseAddr) {
        uint32_t* pPlayerId = (uint32_t*)(g_clientBaseAddr + 0x23FE98);
        if (pPlayerId && *pPlayerId) {
            if (lookMount != 0) {
                CreatureManager::get().setMount(*pPlayerId, lookMount, mountHead, mountBody, mountLegs, mountFeet);
            } else {
                CreatureManager::get().removeMount(*pPlayerId);
            }
        }
    }

    // Read available outfits list
    uint8_t outfitCount = static_cast<uint8_t>(ReadNetworkByte());
    struct OutfitInfo {
        uint16_t lookType;
        std::string name;
        uint8_t addons;
    };
    std::vector<OutfitInfo> outfits;
    for (uint32_t i = 0; i < outfitCount; ++i) {
        uint16_t oLookType = ReadNetworkU16();
        uint16_t nameLen = ReadNetworkU16();
        std::string name;
        for (uint16_t n = 0; n < nameLen; ++n) {
            name += static_cast<char>(ReadNetworkByte());
        }
        uint8_t oAddons = static_cast<uint8_t>(ReadNetworkByte());
        outfits.push_back({ oLookType, name, oAddons });
    }

    // Read available mounts list if sent by TFS to cleanly consume packet
    uint32_t readPos = *(uint32_t*)(g_clientBaseAddr + 0x3998B4);
    uint32_t endPos  = *(uint32_t*)(g_clientBaseAddr + 0x3998B0);
    if (readPos < endPos) {
        uint8_t mountCount = static_cast<uint8_t>(ReadNetworkByte());
        for (uint32_t m = 0; m < mountCount; ++m) {
            ReadNetworkU16(); // clientId
            uint16_t mNameLen = ReadNetworkU16();
            for (uint16_t n = 0; n < mNameLen; ++n) {
                ReadNetworkByte();
            }
        }
    }

    // Native Tibia 8.60 QueueWindow supports up to 86 outfits (cmp esi, 0x56 at 0x51F67A)
    // Buffer layout: header = 0x1C bytes, each outfit = 8 bytes (lookType DWORD + addons DWORD)
    // Names buffer: each outfit gets 30 bytes
    const uint32_t MAX_NATIVE_OUTFITS = 86;
    uint32_t displayCount = static_cast<uint32_t>(outfits.size());
    if (displayCount > MAX_NATIVE_OUTFITS) {
        displayCount = MAX_NATIVE_OUTFITS;
    }
    if (displayCount == 0) {
        displayCount = 1;
    }

    // Use native client malloc (0x571A12) so client free (0x571A0D) works on the same CRT heap
    typedef void*(__cdecl* t_ClientMalloc)(size_t);
    t_ClientMalloc client_malloc = (t_ClientMalloc)(g_clientBaseAddr + 0x171A12);

    // Compute dynamic buffer sizes based on actual displayCount
    const size_t HEADER_SIZE = 0x1C;          // 7 DWORDs: lookType,head,body,legs,feet,addons,count
    const size_t OUTFIT_ENTRY_SIZE = 8;       // lookType DWORD + addons DWORD per outfit
    const size_t NAME_ENTRY_SIZE = 30;        // 30 bytes per outfit name (null-terminated)
    size_t outfitDataSize = HEADER_SIZE + displayCount * OUTFIT_ENTRY_SIZE;
    size_t namesDataSize  = displayCount * NAME_ENTRY_SIZE;

    void* outfitData = client_malloc ? client_malloc(outfitDataSize) : malloc(outfitDataSize);
    if (!outfitData) return;
    memset(outfitData, 0, outfitDataSize);

    *(uint32_t*)((uintptr_t)outfitData + 0x00) = lookType;
    *(uint32_t*)((uintptr_t)outfitData + 0x04) = head;
    *(uint32_t*)((uintptr_t)outfitData + 0x08) = body;
    *(uint32_t*)((uintptr_t)outfitData + 0x0C) = legs;
    *(uint32_t*)((uintptr_t)outfitData + 0x10) = feet;
    *(uint32_t*)((uintptr_t)outfitData + 0x14) = addons;
    *(uint32_t*)((uintptr_t)outfitData + 0x18) = displayCount;

    char* namesData = (char*)(client_malloc ? client_malloc(namesDataSize) : malloc(namesDataSize));
    if (!namesData) {
        if (client_malloc) { /* native heap – no MSVC free here */ } else { free(outfitData); }
        return;
    }
    memset(namesData, 0, namesDataSize);

    for (uint32_t i = 0; i < displayCount; ++i) {
        if (i < outfits.size()) {
            *(uint32_t*)((uintptr_t)outfitData + HEADER_SIZE + i * OUTFIT_ENTRY_SIZE + 0) = outfits[i].lookType;
            *(uint32_t*)((uintptr_t)outfitData + HEADER_SIZE + i * OUTFIT_ENTRY_SIZE + 4) = outfits[i].addons;
            strncpy_s(namesData + i * NAME_ENTRY_SIZE, NAME_ENTRY_SIZE, outfits[i].name.c_str(), NAME_ENTRY_SIZE - 1);
        }
    }

    // Call native QueueWindow(0x11, outfitData, namesData) at 0x51F650
    ((void(__cdecl*)(int, void*, void*))(g_clientBaseAddr + 0x11F650))(0x11, outfitData, namesData);
}

// Opcode 211 (0xD3): Hook writing addons to also send lookMount to TFS
typedef void(__cdecl* t_WriteByte)(uint8_t val);
typedef void(__cdecl* t_WriteU16)(uint16_t val);

static void __cdecl Hooked_WriteOutfitAddonsAndMount(uint8_t addons) {
    // 1. Write addons as normal via native 0x4F8560
    ((t_WriteByte)(g_clientBaseAddr + 0xF8560))(addons);

    // 2. Also write lookMount (uint16_t) expected by TFS GAME_FEATURE_MOUNTS
    uint16_t currentMount = 0;
    if (g_clientBaseAddr) {
        uint32_t* pPlayerId = (uint32_t*)(g_clientBaseAddr + 0x23FE98);
        if (pPlayerId && *pPlayerId) {
            ExtendedCreature extData;
            if (CreatureManager::get().getExtendedData(*pPlayerId, extData)) {
                currentMount = extData.mount.mountId;
            }
        }
    }
    ((t_WriteU16)(g_clientBaseAddr + 0xF8700))(currentMount);
}

void InitNetworkHooks() {
    if (g_clientBaseAddr) {
        o_GetByte = (t_GetByte)(g_clientBaseAddr + 0xF98A0);
        // Hook the call to getByte at 0x45C3A5 inside the main packet dispatcher loop
        HookCall(g_clientBaseAddr + 0x5C3A5, (uintptr_t)&HookGetNextOpcode);

        // Hook the call to parsePlayerStats at 0x45C996 inside the opcode dispatcher
        HookCall(g_clientBaseAddr + 0x5C996, (uintptr_t)&Hooked_ParsePlayerStats);

        // Hook mount data in creature parsing at 0x40E0EE (known creature) and 0x40E3B6 (new creature)
        HookCall(g_clientBaseAddr + 0x0E0EE, (uintptr_t)&Hooked_ReadCreatureLightLevel);
        HookCall(g_clientBaseAddr + 0x0E3B6, (uintptr_t)&Hooked_ReadCreatureLightLevel);

        // Hook opcode 0x8E (sendCreatureOutfit) at 0x45C926
        HookCall(g_clientBaseAddr + 0x5C926, (uintptr_t)&Hooked_ParseCreatureOutfit);

        // Hook opcode 200 (0xC8 - sendOutfitWindow) at 0x45CAB6
        HookCall(g_clientBaseAddr + 0x5CAB6, (uintptr_t)&Hooked_ParseOutfitWindow);

        // Hook opcode 211 (0xD3 - sendSetOutfit) at 0x40A81F to also append lookMount
        HookCall(g_clientBaseAddr + 0x0A81F, (uintptr_t)&Hooked_WriteOutfitAddonsAndMount);

        // Hook creature step speed reading (New Speed Law)
        HookCall(g_clientBaseAddr + 0x0E104, (uintptr_t)&Hooked_ReadCreatureSpeed);
        HookCall(g_clientBaseAddr + 0x0E3CC, (uintptr_t)&Hooked_ReadCreatureSpeed);
        HookCall(g_clientBaseAddr + 0x11266, (uintptr_t)&Hooked_ReadCreatureSpeed);
    }
}



