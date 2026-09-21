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

// Function signature of PrintCreature in Tibia 8.60 (0x4F0F30)
// Calling convention: __thiscall, arguments pushed right-to-left:
// [esp+16]: void* pCreature
// [esp+12]: int flag
// [esp+8]:  int screenY
// [esp+4]:  int screenX
// ecx:      void* thisPtr (Screen/Map renderer)
typedef void(__thiscall* t_PrintCreature)(void* thisPtr, int screenX, int screenY, int flag, void* pCreature);
static t_PrintCreature o_PrintCreature = nullptr;

static void __stdcall Hooked_PrintCreatureInternal(void* thisPtr, int screenX, int screenY, int flag, void* pCreature) {
    if (!pCreature || !o_PrintCreature) {
        if (o_PrintCreature) {
            o_PrintCreature(thisPtr, screenX, screenY, flag, pCreature);
        }
        return;
    }

    try {
        uint32_t creatureId = *(uint32_t*)((uintptr_t)pCreature + 0x00);
        ExtendedCreature extData;
        bool hasExt = CreatureManager::get().getExtendedData(creatureId, extData);

        if (hasExt && g_config.enableMounts && extData.mount.mountId > 0) {
            // Save original player outfit
            uint32_t origLookType = *(uint32_t*)((uintptr_t)pCreature + 0x60);
            uint32_t origHead     = *(uint32_t*)((uintptr_t)pCreature + 0x64);
            uint32_t origBody     = *(uint32_t*)((uintptr_t)pCreature + 0x68);
            uint32_t origLegs     = *(uint32_t*)((uintptr_t)pCreature + 0x6C);
            uint32_t origFeet     = *(uint32_t*)((uintptr_t)pCreature + 0x70);
            uint32_t origAddons   = *(uint32_t*)((uintptr_t)pCreature + 0x74);

            // 1. Draw Mount on the ground (screenX, screenY)
            *(uint32_t*)((uintptr_t)pCreature + 0x60) = extData.mount.mountId;
            *(uint32_t*)((uintptr_t)pCreature + 0x64) = extData.mount.head;
            *(uint32_t*)((uintptr_t)pCreature + 0x68) = extData.mount.body;
            *(uint32_t*)((uintptr_t)pCreature + 0x6C) = extData.mount.legs;
            *(uint32_t*)((uintptr_t)pCreature + 0x70) = extData.mount.feet;
            *(uint32_t*)((uintptr_t)pCreature + 0x74) = 0;

            o_PrintCreature(thisPtr, screenX, screenY, flag, pCreature);

            // 2. Restore character outfit and draw elevated by 3px (screenX, screenY - 3)
            *(uint32_t*)((uintptr_t)pCreature + 0x60) = origLookType;
            *(uint32_t*)((uintptr_t)pCreature + 0x64) = origHead;
            *(uint32_t*)((uintptr_t)pCreature + 0x68) = origBody;
            *(uint32_t*)((uintptr_t)pCreature + 0x6C) = origLegs;
            *(uint32_t*)((uintptr_t)pCreature + 0x70) = origFeet;
            *(uint32_t*)((uintptr_t)pCreature + 0x74) = origAddons;

            o_PrintCreature(thisPtr, screenX, screenY - 3, flag, pCreature);
        } else {
            o_PrintCreature(thisPtr, screenX, screenY, flag, pCreature);
        }
    } catch (...) {
        if (o_PrintCreature) {
            o_PrintCreature(thisPtr, screenX, screenY, flag, pCreature);
        }
    }
}

static __declspec(naked) void Hooked_PrintCreature() {
    __asm {
        // [esp+0]  = retAddr
        // [esp+4]  = screenX
        // [esp+8]  = screenY
        // [esp+12] = flag
        // [esp+16] = pCreature
        // ecx      = thisPtr
        push dword ptr [esp + 16] // pCreature
        push dword ptr [esp + 16] // flag (shifted by 4)
        push dword ptr [esp + 16] // screenY (shifted by 8)
        push dword ptr [esp + 16] // screenX (shifted by 12)
        push ecx                  // thisPtr
        call Hooked_PrintCreatureInternal
        ret 16                    // Cleanup 16 bytes pushed by caller
    }
}

void InitCreatureHooks() {
    if (g_config.enableMounts && g_clientBaseAddr) {
        o_PrintCreature = (t_PrintCreature)(g_clientBaseAddr + 0xF0F30);
        // Hook the call to PrintCreature at 0x4F27CA inside the map rendering loop
        HookCall(g_clientBaseAddr + 0xF27CA, (uintptr_t)&Hooked_PrintCreature);
    }
}
