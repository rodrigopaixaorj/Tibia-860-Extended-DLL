/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __CREATURE_MANAGER_H__
#define __CREATURE_MANAGER_H__

#include <cstdint>
#include <unordered_map>
#include <mutex>

struct MountData {
    uint16_t mountId = 0;
    uint8_t head = 0;
    uint8_t body = 0;
    uint8_t legs = 0;
    uint8_t feet = 0;
};

struct ExtendedCreature {
    MountData mount;
};

class CreatureManager {
public:
    static CreatureManager& get();

    void setMount(uint32_t creatureId, uint16_t mountId, uint8_t head, uint8_t body, uint8_t legs, uint8_t feet);
    void removeMount(uint32_t creatureId);
    bool getExtendedData(uint32_t creatureId, ExtendedCreature& outData);
    void clear();

private:
    CreatureManager() = default;
    ~CreatureManager() = default;

    std::unordered_map<uint32_t, ExtendedCreature> m_creatures;
    std::mutex m_mutex;
};

void InitCreatureHooks();

#endif // __CREATURE_MANAGER_H__
