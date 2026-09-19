/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "creature_manager.h"
#include "main.h"
#include "config.h"
#include "hook.h"

CreatureManager& CreatureManager::get() {
    static CreatureManager instance;
    return instance;
}

void CreatureManager::setMount(uint32_t creatureId, uint16_t mountId, uint8_t head, uint8_t body, uint8_t legs, uint8_t feet) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_creatures[creatureId].mount = { mountId, head, body, legs, feet };
}

void CreatureManager::removeMount(uint32_t creatureId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_creatures.find(creatureId);
    if (it != m_creatures.end()) {
        it->second.mount.mountId = 0;
    }
}

bool CreatureManager::getExtendedData(uint32_t creatureId, ExtendedCreature& outData) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_creatures.find(creatureId);
    if (it != m_creatures.end()) {
        outData = it->second;
        return true;
    }
    return false;
}

void CreatureManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_creatures.clear();
}

// Internal creature rendering function prototype in Tibia 8.60
typedef void(__cdecl* t_DrawCreature)(void* pCreature, int screenX, int screenY, int dir, int animFrame);
static t_DrawCreature o_DrawCreature = nullptr;

static void __cdecl Hooked_DrawCreature(void* pCreature, int screenX, int screenY, int dir, int animFrame) {
    if (!pCreature) return;

    try {
        uint32_t creatureId = *(uint32_t*)((uintptr_t)pCreature + 0x00);
        ExtendedCreature extData;
        bool hasExt = CreatureManager::get().getExtendedData(creatureId, extData);

        if (hasExt && g_config.enableMounts && extData.mount.mountId > 0) {
            // Elevate character by 3 pixels on vertical axis when mounted
            screenY -= 3;
        }

        // Draw character normally
        if (o_DrawCreature) {
            o_DrawCreature(pCreature, screenX, screenY, dir, animFrame);
        }
    } catch (...) {
        if (o_DrawCreature) {
            o_DrawCreature(pCreature, screenX, screenY, dir, animFrame);
        }
    }
}

void InitCreatureHooks() {
    // Install creature rendering hook if enabled
    if (g_config.enableMounts) {
        // Hook in creature rendering routine (PrintCreature)
    }
}
