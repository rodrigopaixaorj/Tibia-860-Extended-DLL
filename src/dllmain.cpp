/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "main.h"
#include "ddraw_proxy.h"
#include "config.h"
#include "hook.h"
#include "sprites.h"
#include "ext_engines.h"
#include "creature_manager.h"
#include "network_protocol.h"
#include "ui_market.h"
#include "market_window.h"
#include "timer.h"
#include "dat_reader.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

uint32_t g_clientBaseAddr = 0;
uint32_t g_clientVersion = 0;
uint32_t g_clientPointerTransPixels = 0;

_GetEngineAddr GetEngineAddr = nullptr;
_DrawSkin DrawSkin = nullptr;
_PrintText PrintText = nullptr;
_GetFileName GetFileName = nullptr;

static uint32_t PlayerHealthAddr = 0;
static uint32_t PlayerHealthMaxAddr = 0;
static uint32_t PlayerManaAddr = 0;
static uint32_t PlayerManaMaxAddr = 0;
static uint32_t PlayerIDAddr = 0;

static uint32_t CreateGlContext = 0;
static uint32_t DeleteGlContext = 0;
static uint32_t CreateDX9Context = 0;
static uint32_t DeleteDX9Context = 0;
static uint32_t CreateDX7Context = 0;
static uint32_t DeleteDX7Context = 0;
static uint32_t SpriteContext = 0;
static uint8_t  SpriteContextPos = 0x57;
static uint8_t  SpriteContextNeg = 0x51;

struct Render_NEW {
    DWORD padds1[5];
    void (__stdcall *DrawRectangle)(DWORD nSurface, DWORD X, DWORD Y, DWORD W, DWORD H, DWORD nRed, DWORD nGreen, DWORD nBlue);
    DWORD padds2[4];
    void (__stdcall *LoadSprite)(int surface, int x, int y, int w, int h, void* data);
};

static Render_NEW* g_newRenderer = nullptr;

static void __stdcall MyLoadSprite(int surface, int x, int y, int w, int h, void* data) {
    if (g_transEngine) {
        g_transEngine->LoadSprite(surface, x, y, w, h, data);
    }
}

static DWORD HookExtendedEngine() {
    if (g_transEngine) {
        return (DWORD)&g_newRenderer;
    } else {
        return GetEngineAddr ? GetEngineAddr() : 0;
    }
}

static DWORD HookEnginePresent() {
    if (InGameMarket::IsOpen()) {
        HWND hTibia = FindWindowA("TibiaClient", NULL);
        if (hTibia) {
            RECT rc;
            if (GetClientRect(hTibia, &rc)) {
                int sW = rc.right - rc.left;
                int sH = rc.bottom - rc.top;
                InGameMarket::Render(0, sW, sH);
            }
        }
    }
    return GetEngineAddr ? GetEngineAddr() : 0;
}

static bool __stdcall HookCreateGLContext() {
    bool created = ((bool(__stdcall*)())CreateGlContext)();
    if (created) {
        OverWriteByte(SpriteContext, SpriteContextPos);
        if (!g_transEngine) {
            g_transEngine = new ExtendedEngineOGL();
        }
    }
    return created;
}

static void __stdcall HookDeleteGLContext() {
    OverWriteByte(SpriteContext, SpriteContextNeg);
    if (g_transEngine && g_transEngine->getRenderId() == OGL_RENDER) {
        delete g_transEngine;
        g_transEngine = nullptr;
    }
    ((void(__stdcall*)())DeleteGlContext)();
}

static void __stdcall HookGLEnableAlpha(GLenum cap) {
    if (g_transEngine) {
        g_transEngine->enableAlpha();
    }
    glEnable(cap);
}

static void __stdcall HookGLDisableAlpha(void) {
    glEnd();
    if (g_transEngine) {
        g_transEngine->disableAlpha();
    }
}

static void __fastcall HookCreateDX9Context(DWORD dx9engine, int) {
    ((void(__fastcall*)(DWORD, int))CreateDX9Context)(dx9engine, 0);
    OverWriteByte(SpriteContext, SpriteContextPos);
    if (!g_transEngine) {
        g_transEngine = new ExtendedEngineDX9();
    }
}

static void __fastcall HookDeleteDX9Context(DWORD dx9engine, int) {
    OverWriteByte(SpriteContext, SpriteContextNeg);
    if (g_transEngine && g_transEngine->getRenderId() == DX9_RENDER) {
        delete g_transEngine;
        g_transEngine = nullptr;
    }
    ((void(__fastcall*)(DWORD, int))DeleteDX9Context)(dx9engine, 0);
}

static void __fastcall HookCreateDX7Context(DWORD dx7engine, int) {
    ((void(__fastcall*)(DWORD, int))CreateDX7Context)(dx7engine, 0);
    OverWriteByte(SpriteContext, SpriteContextNeg);
}

static void __fastcall HookDeleteDX7Context(DWORD dx7engine, int) {
    OverWriteByte(SpriteContext, SpriteContextNeg);
    ((void(__fastcall*)(DWORD, int))DeleteDX7Context)(dx7engine, 0);
}

static void HookDrawHealthBar(int nSurface, int X, int Y, int W, int H, int SkinId, int dX, int dY) {
    double tmp_float = 0.f;
    if (PlayerHealthMaxAddr && *(DWORD*)PlayerHealthMaxAddr != 0) {
        tmp_float = ((double)(*(DWORD*)PlayerHealthAddr)) / (*(DWORD*)PlayerHealthMaxAddr);
    }
    char tmpBuff[8];
    sprintf_s(tmpBuff, sizeof(tmpBuff), "%d%%", static_cast<int32_t>(100 * tmp_float));
    if (DrawSkin) DrawSkin(nSurface, X, Y, W, H, SkinId, dX, dY);
    if (PrintText) PrintText(nSurface, X + 45, Y, 2, 180, 180, 180, tmpBuff, 1);
}

static void HookDrawManaBar(int nSurface, int X, int Y, int W, int H, int SkinId, int dX, int dY) {
    double tmp_float = 0.f;
    if (PlayerManaMaxAddr && *(DWORD*)PlayerManaMaxAddr != 0) {
        tmp_float = ((double)(*(DWORD*)PlayerManaAddr)) / (*(DWORD*)PlayerManaMaxAddr);
    }
    char tmpBuff[8];
    sprintf_s(tmpBuff, sizeof(tmpBuff), "%d%%", static_cast<int32_t>(100 * tmp_float));
    if (DrawSkin) DrawSkin(nSurface, X, Y, W, H, SkinId, dX, dY);
    if (PrintText) PrintText(nSurface, X + 45, Y, 2, 180, 180, 180, tmpBuff, 1);

    if (InGameMarket::IsOpen()) {
        HWND hTibia = FindWindowA("TibiaClient", NULL);
        if (hTibia) {
            RECT rc;
            if (GetClientRect(hTibia, &rc)) {
                int sW = rc.right - rc.left;
                int sH = rc.bottom - rc.top;
                InGameMarket::Render(nSurface, sW, sH);
            }
        }
    }
}

static void __stdcall MyDrawHPBar(DWORD nSurface, DWORD X, DWORD Y, DWORD W, DWORD creaturePointer, DWORD nRed, DWORD nGreen, DWORD nBlue) {
    if (nRed == 0 && nGreen == 0 && nBlue == 0) return;
    if (creaturePointer < 0x10000 || IsBadReadPtr((void*)creaturePointer, sizeof(DWORD))) return;

    DWORD creatureId = *(DWORD*)creaturePointer;
    if (creatureId >= 0x80000000) return; // NPC

    uint32_t engineAddr = GetEngineAddr ? GetEngineAddr() : 0;
    if (!engineAddr) return;

    uint32_t drawRectAddr = *(DWORD*)(*(DWORD*)(engineAddr) + 0x14);
    ((void(__fastcall*)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(engineAddr, 0, nSurface, X - 1, Y - 1, 27, 4, 0, 0, 0);
    ((void(__fastcall*)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(engineAddr, 0, nSurface, X, Y, W, 2, nRed, nGreen, nBlue);

    if (creatureId == *(DWORD*)PlayerIDAddr && g_config.drawManaBar) {
        double tmp_float = 1.f;
        if (*(DWORD*)PlayerManaMaxAddr != 0) {
            tmp_float = ((double)(*(DWORD*)PlayerManaAddr)) / (*(DWORD*)PlayerManaMaxAddr);
        }
        ((void(__fastcall*)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(engineAddr, 0, nSurface, X - 1, Y + 4, 27, 4, 0, 0, 0);
        ((void(__fastcall*)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(engineAddr, 0, nSurface, X, Y + 5, static_cast<int32_t>(25 * tmp_float), 2, 0, 108, 255);
    }
}

static DWORD HPBarRenderHandle() {
    return (DWORD)&g_newRenderer;
}

static inline bool IsAttackAnimation(int type) {
    // Magic Effect IDs used by modern OpenTibia servers for weapon attack slashes & hits:
    // 304: Sword, 305: Axe, 306: Club, 307: Special Weapon, 309: Fist
    return (type == 304 || type == 305 || type == 306 || type == 307 || type == 309);
}

static int g_attackAnimPx = 2;
static int g_attackAnimPy = 2;

typedef int(__cdecl* t_GetMagicEffectSprite)(int type, int w, int h, int layer, int px, int py, int pz, int anim);

static int __cdecl Hooked_GetMagicEffectSprite(int type, int w, int h, int layer, int px, int py, int pz, int anim) {
    if (IsAttackAnimation(type)) {
        if (!g_config.showAttackAnimations) {
            return 0; // Don't draw weapon attack animation if disabled in config.ini
        }
        // Apply directional patterns (px, py) for 3x3 weapon swing effects
        px = g_attackAnimPx;
        py = g_attackAnimPy;
    }
    t_GetMagicEffectSprite origFunc = (t_GetMagicEffectSprite)(g_clientBaseAddr + 0x105540);
    return origFunc(type, w, h, layer, px, py, pz, anim);
}

static void __cdecl Hooked_AddMagicEffect(int posX, int posY, int posZ, int type) {
    if (IsAttackAnimation(type)) {
        if (!g_config.showAttackAnimations) {
            return; // Skip adding attack effect when disabled
        }

        // Calculate attack direction from player to target position
        int playerX = *(int*)(g_clientBaseAddr + 0x23FE6C);
        int playerY = *(int*)(g_clientBaseAddr + 0x23FE68);
        int dx = posX - playerX;
        int dy = posY - playerY;

        if (dx > 0 && dy == 0)      { g_attackAnimPx = 3; g_attackAnimPy = 2; } // East
        else if (dx < 0 && dy == 0) { g_attackAnimPx = 1; g_attackAnimPy = 2; } // West
        else if (dx == 0 && dy > 0) { g_attackAnimPx = 2; g_attackAnimPy = 3; } // South
        else if (dx == 0 && dy < 0) { g_attackAnimPx = 2; g_attackAnimPy = 1; } // North
        else if (dx > 0 && dy > 0)  { g_attackAnimPx = 3; g_attackAnimPy = 3; } // SouthEast
        else if (dx > 0 && dy < 0)  { g_attackAnimPx = 3; g_attackAnimPy = 1; } // NorthEast
        else if (dx < 0 && dy > 0)  { g_attackAnimPx = 1; g_attackAnimPy = 3; } // SouthWest
        else if (dx < 0 && dy < 0)  { g_attackAnimPx = 1; g_attackAnimPy = 1; } // NorthWest
        else {
            // On same tile: use player facing direction
            uint32_t playerDir = *(uint32_t*)(g_clientBaseAddr + 0x23FE20);
            switch (playerDir) {
                case 0: g_attackAnimPx = 2; g_attackAnimPy = 1; break; // North
                case 1: g_attackAnimPx = 3; g_attackAnimPy = 2; break; // East
                case 2: g_attackAnimPx = 2; g_attackAnimPy = 3; break; // South
                case 3: g_attackAnimPx = 1; g_attackAnimPy = 2; break; // West
                default: g_attackAnimPx = 2; g_attackAnimPy = 2; break;
            }
        }
    }

    typedef void(__cdecl* t_AddMagicEffect)(int, int, int, int);
    ((t_AddMagicEffect)(g_clientBaseAddr + 0xE52C0))(posX, posY, posZ, type);
}

// Real-Time Magic Effects Animation Timing (Matches TFC frame timing at ~75ms/frame)
static DWORD g_effectStartTime[200] = { 0 };
static int g_effectLastType[200] = { 0 };

static int __cdecl Hooked_GetMagicEffectFrame(int effectIndex) {
    if (effectIndex < 0 || effectIndex >= 200) {
        return 1;
    }

    uint8_t* slot = (uint8_t*)(g_clientBaseAddr + 0x24F610 + (effectIndex * 0x60));
    int state = *(int*)(slot + 0x00);
    if (state != 1) { // Not an active magic effect
        g_effectStartTime[effectIndex] = 0;
        return 1;
    }

    int effectType = *(int*)(slot + 0x04);
    DWORD now = timeGetTime();
    if (g_effectStartTime[effectIndex] == 0 || g_effectLastType[effectIndex] != effectType) {
        g_effectStartTime[effectIndex] = now;
        g_effectLastType[effectIndex] = effectType;
    }

    typedef int(__cdecl* t_GetEffectAnimCount)(int);
    int animCount = ((t_GetEffectAnimCount)(g_clientBaseAddr + 0x105390))(effectType);
    if (animCount <= 1) {
        return 1;
    }

    int frameDuration = g_config.effectSpeedMs > 0 ? g_config.effectSpeedMs : 75;
    DWORD elapsed = now - g_effectStartTime[effectIndex];
    int frame = (int)(elapsed / frameDuration) + 1; // 1-indexed

    if (frame > animCount) {
        typedef void(__cdecl* t_DestroyEffect)(int);
        ((t_DestroyEffect)(g_clientBaseAddr + 0xE5DB0))(effectIndex);
        g_effectStartTime[effectIndex] = 0;
        return animCount;
    }

    *(int*)(slot + 0x08) = frame;
    return frame;
}

static void __cdecl Hooked_UpdateEffects() {
    DWORD now = timeGetTime();
    int frameDuration = g_config.effectSpeedMs > 0 ? g_config.effectSpeedMs : 75;
    typedef int(__cdecl* t_GetEffectAnimCount)(int);
    typedef void(__cdecl* t_DestroyEffect)(int);
    t_GetEffectAnimCount getAnimCount = (t_GetEffectAnimCount)(g_clientBaseAddr + 0x105390);
    t_DestroyEffect destroyEffect = (t_DestroyEffect)(g_clientBaseAddr + 0xE5DB0);

    for (int i = 0; i < 200; ++i) {
        uint8_t* slot = (uint8_t*)(g_clientBaseAddr + 0x24F610 + (i * 0x60));
        int state = *(int*)(slot + 0x00);
        if (state == 1) { // Magic Effect
            int effectType = *(int*)(slot + 0x04);
            if (g_effectStartTime[i] == 0 || g_effectLastType[i] != effectType) {
                g_effectStartTime[i] = now;
                g_effectLastType[i] = effectType;
            }
            int animCount = getAnimCount(effectType);
            if (animCount > 1) {
                DWORD elapsed = now - g_effectStartTime[i];
                int frame = (int)(elapsed / frameDuration) + 1;
                if (frame > animCount) {
                    destroyEffect(i);
                    g_effectStartTime[i] = 0;
                } else {
                    *(int*)(slot + 0x08) = frame;
                }
            }
        } else if (state == 2) {
            // Distance Shoot / Projectile update
            int count = *(int*)(slot + 0x1C);
            count++;
            *(int*)(slot + 0x1C) = count;
            if (count > 10) {
                destroyEffect(i);
            }
        }
    }
}

// Creature Movement & Step Duration Hook (Matches server monster step duration)
class NativeCreatureHelper {
public:
    void __thiscall Hooked_Walk(int deltaX, int deltaY, int groundSpeed);
    int __thiscall Hooked_GetAnimationFrame(int animCount);
};

int __thiscall NativeCreatureHelper::Hooked_GetAnimationFrame(int animCount) {
    if (animCount <= 1) {
        return 1;
    }

    uint8_t* creature = (uint8_t*)this;
    if (!creature) {
        return 1;
    }

    int offsetX = *(int*)(creature + 0x30);
    int offsetY = *(int*)(creature + 0x34);
    uint32_t walkStartTime = *(uint32_t*)(creature + 0x38);

    typedef uint32_t(__cdecl* t_GetGameTick)();
    uint32_t currentTick = ((t_GetGameTick)(g_clientBaseAddr + 0x1414E0))();

    // If standing still (or step movement completed)
    if (offsetX == 0 && offsetY == 0 && currentTick > walkStartTime) {
        return 1; // Idle frame (Frame 1)
    }

    // Creature is currently walking: calculate frame proportional to walked pixels (0..31 pixels)
    int maxOffset = max(abs(offsetX), abs(offsetY));
    if (maxOffset > 32) maxOffset = 32;
    int walkedPixels = 32 - maxOffset; // 0 to 31 pixels traversed

    int movingFrames = animCount - 1; // Number of walking frames
    if (movingFrames <= 0) {
        return 1;
    }

    // Proportional frame calculation across the entire 32-pixel step
    int movingFrameIndex = (walkedPixels * movingFrames) / 32;
    if (movingFrameIndex >= movingFrames) {
        movingFrameIndex = movingFrames - 1;
    }
    if (movingFrameIndex < 0) {
        movingFrameIndex = 0;
    }

    // Frame 1 is Idle, Frames 2..N are Moving steps (1-indexed)
    return movingFrameIndex + 2;
}

void __thiscall NativeCreatureHelper::Hooked_Walk(int deltaX, int deltaY, int groundSpeed) {
    uint8_t* creature = (uint8_t*)this;
    if (!creature) return;

    // Determine direction from deltaX and deltaY
    if (deltaX > 0) {
        *(int*)(creature + 0x50) = 1; // East
    } else if (deltaX < 0) {
        *(int*)(creature + 0x50) = 3; // West
    } else if (deltaY < 0) {
        *(int*)(creature + 0x50) = 0; // North
    } else if (deltaY > 0) {
        *(int*)(creature + 0x50) = 2; // South
    }

    // Check prewalk for local player
    uint32_t localPlayerId = *(uint32_t*)(g_clientBaseAddr + 0x23FE98);
    uint32_t creatureId = *(uint32_t*)creature;

    if (groundSpeed != 0 && creatureId == localPlayerId) {
        int preWalkX = *(int*)(g_clientBaseAddr + 0x23FEE8);
        int preWalkY = *(int*)(g_clientBaseAddr + 0x23FEE4);
        bool isPreWalk = (preWalkX == deltaX && preWalkY == deltaY);
        *(int*)(g_clientBaseAddr + 0x23FEE8) = 0;
        *(int*)(g_clientBaseAddr + 0x23FEE4) = 0;
        if (isPreWalk) {
            return;
        }
    }

    *(int*)(g_clientBaseAddr + 0x23FEA0) += 1;
    *(int*)(creature + 0x54) = *(int*)(creature + 0x50); // oldDirection = direction

    int speed = *(int*)(creature + 0x8C);
    if (speed <= 0) speed = 150;
    if (groundSpeed <= 0) groundSpeed = 150;

    // Monsters (0x40000000..0x7FFFFFFF) and NPCs (0x80000000+) walk with server pacing (3.0x step duration)
    bool isMonsterOrNpc = (creatureId >= 0x40000000);
    double multiplier = isMonsterOrNpc ? 3.0 : 1.0;

    int stepDuration = (int)(((double)(groundSpeed * 1000) / speed) * multiplier);
    if (stepDuration < 1) stepDuration = 1000;

    int serverBeat = *(int*)(g_clientBaseAddr + 0x23FEEC);
    if (serverBeat <= 0) serverBeat = 50;
    stepDuration = ((stepDuration + serverBeat - 1) / serverBeat) * serverBeat;

    *(int*)(creature + 0x30) = -deltaX * 32;
    *(int*)(creature + 0x34) = -deltaY * 32;
    *(int*)(creature + 0x40) = abs(deltaX) * 32;
    *(int*)(creature + 0x44) = abs(deltaY) * 32;
    *(int*)(creature + 0x48) = stepDuration;

    typedef uint32_t(__cdecl* t_GetGameTick)();
    uint32_t currentTick = ((t_GetGameTick)(g_clientBaseAddr + 0x1414E0))();
    *(uint32_t*)(creature + 0x38) = currentTick + stepDuration;

    int diagDuration = stepDuration;
    if (deltaX != 0 && deltaY != 0) {
        diagDuration = (int)(((double)(groundSpeed * 3000) / speed) * multiplier);
        if (diagDuration < 1) diagDuration = 1000;
    }
    *(uint32_t*)(creature + 0x3C) = currentTick + diagDuration;
    *(uint8_t*)(creature + 0x4C) = 1; // isWalking = 1
}

// Standard OpenTibia RSA Public Key (1024-bit, 309 decimal digits)
static const char g_openTibiaRSAKey[] =
    "1091201329673994292788609605089955415282375029027981291234687579"
    "3726629149257644633073969600111060390723088861007265581882535850"
    "3429057592827629436413108566029093628212635953836686562675849720"
    "6207862794310902180176810615217550567108238764764442605581471797"
    "07119674283982419152118103759076030616683978566631413";

static void SafeInit() {
    LoadConfig("config.ini");
    InitDirectDrawProxy();

    HANDLE baseHandle = GetModuleHandle(NULL);
    g_clientBaseAddr = (uint32_t)baseHandle;
    if (!baseHandle) return;

    DWORD dwOldProtect;
    VirtualProtect((LPVOID)(g_clientBaseAddr + 0x1000), 0x238000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    // RSA Key Patch (allows connection to OpenTibia servers)
    const char* rsaToInject = g_config.customRSAKey.empty() ? g_openTibiaRSAKey : g_config.customRSAKey.c_str();
    size_t rsaLen = strlen(rsaToInject);
    if (rsaLen <= 309) {
        HookMemory(g_clientBaseAddr + 0x1B8980, rsaToInject, rsaLen + 1);
    }

    // Auto Login Server Redirection (if configured in config.ini)
    if (!g_config.serverIP.empty() && g_config.serverIP.length() < 20) {
        for (int i = 0; i < 10; ++i) {
            uint32_t hostAddr = g_clientBaseAddr + 0x1B88B4 + (i * 20);
            HookMemory(hostAddr, g_config.serverIP.c_str(), g_config.serverIP.length() + 1);

            uint32_t portAddr = g_clientBaseAddr + 0x1B8864 + (i * 8);
            OverWriteWord(portAddr, g_config.serverPort);
        }
    }

    // Tibia 8.60 Function and Data Addresses
    g_clientPointerTransPixels = g_clientBaseAddr + 0x24A9A8;
    GetFileName = (_GetFileName)(g_clientBaseAddr + 0x108870);
    GetEngineAddr = (_GetEngineAddr)(g_clientBaseAddr + 0x122A90);
    DrawSkin = (_DrawSkin)(g_clientBaseAddr + 0xB96E0);
    PrintText = (_PrintText)(g_clientBaseAddr + 0xB4DD0);

    PlayerHealthAddr = g_clientBaseAddr + 0x23FE94;
    PlayerHealthMaxAddr = g_clientBaseAddr + 0x23FE90;
    PlayerManaAddr = g_clientBaseAddr + 0x23FE78;
    PlayerManaMaxAddr = g_clientBaseAddr + 0x23FE74;
    PlayerIDAddr = g_clientBaseAddr + 0x23FE98;

    // Sprite loader hooks
    HookCall(g_clientBaseAddr + 0xABA36, (DWORD)&HookPointers);
    HookCall(g_clientBaseAddr + 0xAB9C9, (DWORD)&HookSignature);
    HookCall(g_clientBaseAddr + 0xAB9D9, (DWORD)&HookNumSprites);
    HookCall(g_clientBaseAddr + 0xADCC9, (DWORD)&HookLoadSprite);
    Nop(g_clientBaseAddr + 0xAB9DE, 3);

    // Health/Mana percentage text hooks
    HookCall(g_clientBaseAddr + 0x340C4, (DWORD)&HookDrawHealthBar);
    HookCall(g_clientBaseAddr + 0x34276, (DWORD)&HookDrawManaBar);

    // High resolution timer (sets 1ms precision via timeBeginPeriod)
    InitTimerHooks();

    // DirectDraw 7 Upgrade
    OverWrite(g_clientBaseAddr + 0x1D8840, 0x15E65EC0);
    OverWrite(g_clientBaseAddr + 0x1D8840 + 4, 0x11D23B9C);
    OverWrite(g_clientBaseAddr + 0x1D8840 + 8, 0x60002FB9);
    OverWrite(g_clientBaseAddr + 0x1D8840 + 12, 0x5BEA9797);
    OverWriteByte(g_clientBaseAddr + 0x1C1DB7, 0x37);

    // Limits Patches
    if (g_config.extendedMagicEffects) {
        // Opcode 131 (0x83) - Magic Effect (uint16)
        HookCall(g_clientBaseAddr + 0x104B4, g_clientBaseAddr + 0xF9C00);
        OverWriteByte(g_clientBaseAddr + 0x104BA, 0xB7);

        // Opcode 133 (0x85) - Distance Shoot / Missile (uint16)
        HookCall(g_clientBaseAddr + 0x108F6, g_clientBaseAddr + 0xF9C00);
        OverWriteByte(g_clientBaseAddr + 0x108FC, 0xB7);
    }

    // Weapon Attack Animation Hooks (Effects 304=Sword, 305=Axe, 306=Club, 307, 309=Fist)
    HookCall(g_clientBaseAddr + 0x104CC, (DWORD)&Hooked_AddMagicEffect);
    HookCall(g_clientBaseAddr + 0xF25C7, (DWORD)&Hooked_GetMagicEffectSprite);
    HookCall(g_clientBaseAddr + 0xF338A, (DWORD)&Hooked_GetMagicEffectSprite);

    g_newRenderer = (Render_NEW*)calloc(1, sizeof(*g_newRenderer));
    if (g_newRenderer) {
        if (g_config.drawManaBar) {
            g_newRenderer->DrawRectangle = MyDrawHPBar;
            HookCall(g_clientBaseAddr + 0xF5675, (DWORD)&HPBarRenderHandle);
            HookCall(g_clientBaseAddr + 0xF573E, (DWORD)&HPBarRenderHandle);
            HookCall(g_clientBaseAddr + 0xF5875, (DWORD)&HPBarRenderHandle);
            HookCall(g_clientBaseAddr + 0xF5922, (DWORD)&HPBarRenderHandle);
            OverWriteWord(g_clientBaseAddr + 0xF57D8, 0xFFD0);
            OverWriteWord(g_clientBaseAddr + 0xF59B0, 0xFFD0);
        }

        if (g_config.alphaTransparency) {
            g_newRenderer->LoadSprite = MyLoadSprite;

            CreateGlContext = g_clientBaseAddr + 0x139F50;
            DeleteGlContext = g_clientBaseAddr + 0x139CF0;
            CreateDX9Context = g_clientBaseAddr + 0x131FD0;
            DeleteDX9Context = g_clientBaseAddr + 0x131D30;
            CreateDX7Context = g_clientBaseAddr + 0x129A10;
            DeleteDX7Context = g_clientBaseAddr + 0x1297D0;
            SpriteContext = g_clientBaseAddr + 0xAF686;
            SpriteContextPos = 0x57;
            SpriteContextNeg = 0x51;

            HookCall(g_clientBaseAddr + 0xAF67A, (DWORD)&HookExtendedEngine);
            HookCall(g_clientBaseAddr + 0x1403AB, (DWORD)&HookCreateGLContext);
            HookCall(g_clientBaseAddr + 0x1400A6, (DWORD)&HookDeleteGLContext);
            HookCallN(g_clientBaseAddr + 0x13E948, (DWORD)&HookGLEnableAlpha);
            HookCallN(g_clientBaseAddr + 0x13EA44, (DWORD)&HookGLDisableAlpha);
            OverWrite(g_clientBaseAddr + 0x1D2214, (DWORD)&HookCreateDX9Context);
            OverWrite(g_clientBaseAddr + 0x1D2210, (DWORD)&HookDeleteDX9Context);
            OverWrite(g_clientBaseAddr + 0x1D1DF4, (DWORD)&HookCreateDX7Context);
            OverWrite(g_clientBaseAddr + 0x1D1DF0, (DWORD)&HookDeleteDX7Context);
        }
    }

    // Extended DAT and SPR file parser patches
    if (g_config.extendedSprites) {
        // Overwrite sprite reading in ThingType::getSpriteIndex (0x50080C) to 4-byte DWORDs
        OverWrite(g_clientBaseAddr + 0x10080C, 0xFC8A448B); // mov eax, dword ptr [edx + ecx*4 - 4]
        Nop(g_clientBaseAddr + 0x10080C + 4, 1);
    }

    // Initialize Extended DAT Reader (Loads modern 10.x/12.x/extended DATs seamlessly)
    InitDatReaderHooks();

    // Initialize Extended Modules
    InitCreatureHooks();
    InitNetworkHooks();
    InitMarketHooks();

    // Hook Frame Present (renders in-game Market every single frame without needing window resize)
    HookCall(g_clientBaseAddr + 0x5A34A, (DWORD)&HookEnginePresent);

    // Patch ContentWindows.cpp:2460 Experience Underflow crash [bug0000996]
    // In Tibia 8.60 (0x4377D0), "cmp dword ptr [edi], ebx; jge 0x43793B" (0F 8D 63 01 00 00, 6 bytes)
    // Characters with experience >= 2,147,483,648 (or 64-bit exp) trigger an underflow assertion crash.
    // Replace with "jmp 0x43793B; nop" (E9 64 01 00 00 90, exactly 6 bytes)
    const uint8_t jmpBytes[6] = { 0xE9, 0x64, 0x01, 0x00, 0x00, 0x90 };
    HookMemory(g_clientBaseAddr + 0x377D2, jmpBytes, 6);

    // Patch ContentWindows.cpp:2206 / Utils.cpp:659 (Experience / Number >= 2,147,483,648 crash)
    // In formatNumberWithCommas (0x559EF0), line 0x55A02C checks "cmp edi, 0; jge 0x55A140" (0F 8D 0E 01 00 00).
    // Large unsigned numbers (e.g. experience >= 2.14B) are treated as negative, failing assertion "Number >= 0".
    // Replace with "jmp 0x55A140; nop" (E9 0F 01 00 00 90, 6 bytes) and format with "%u" instead of "%d".
    static const char g_fmtUnsigned[] = "%u";
    const uint8_t jmpFmtBytes[6] = { 0xE9, 0x0F, 0x01, 0x00, 0x00, 0x90 };
    HookMemory(g_clientBaseAddr + 0x15A02C, jmpFmtBytes, 6);
    OverWrite(g_clientBaseAddr + 0x15A142, (DWORD)g_fmtUnsigned);

    // Patch Outfit Limit & Dialogs.cpp:1612 Assertion Crash [In(AvailableOutfits,1,AVAILABLE_OUTFITS)]
    // 1. Expand network packet allocation for outfit list:
    OverWrite(g_clientBaseAddr + 0x13D9C, 0x2000); // Struct buffer (was 0xE4 for 25 outfits -> 0x2000 for 512+)
    OverWrite(g_clientBaseAddr + 0x13DB0, 0x4000); // Names buffer (was 0x2EE for 25 outfits -> 0x4000 for 512+)

    // 2. Bypass network packet outfit count assertion at 0x413E1A:
    const uint8_t netAssertJmp[6] = { 0xE9, 0xA6, 0x00, 0x00, 0x00, 0x90 };
    HookMemory(g_clientBaseAddr + 0x13E1A, netAssertJmp, 6);

    // 3. Expand OutfitDialog object memory allocation size at 0x4A09FC:
    OverWrite(g_clientBaseAddr + 0xA09FC, 0x5000); // (was 0x644 for 25 outfits -> 0x5000 for 512+)

    // 4. Bypass Dialogs.cpp:1612 assertion at 0x498B92:
    const uint8_t dlgAssertJmp[6] = { 0xE9, 0xB1, 0x00, 0x00, 0x00, 0x90 };
    HookMemory(g_clientBaseAddr + 0x98B92, dlgAssertJmp, 6);

    // 5. Remap outfit_addons array displacement from 0x2EC (25 outfits) to 0xA88 (512 outfits):
    OverWrite(g_clientBaseAddr + 0x98C68, 0xA88);
    OverWrite(g_clientBaseAddr + 0x98C7F, 0xA88);
    OverWrite(g_clientBaseAddr + 0x98C9A, 0xA88);
    OverWrite(g_clientBaseAddr + 0x9920D, 0xA88);
    OverWrite(g_clientBaseAddr + 0x880BB, 0xA88);
    OverWrite(g_clientBaseAddr + 0x8818D, 0xA88);
    OverWrite(g_clientBaseAddr + 0x881B8, 0xA88);

    // 6. Remap outfit_names array displacement from 0x350 (25 outfits) to 0x1288 (512 outfits):
    OverWrite(g_clientBaseAddr + 0x98CB1, 0x1288);
    OverWrite(g_clientBaseAddr + 0x99274, 0x1288);
    OverWrite(g_clientBaseAddr + 0x8821F, 0x1288);

    // 7. Hook Creature::walk (0x45DCA0) for monster step duration synchronization
    union MethodPointer {
        void (NativeCreatureHelper::*pmfn)(int, int, int);
        DWORD pAddress;
    };
    MethodPointer mp;
    mp.pmfn = &NativeCreatureHelper::Hooked_Walk;
    HookJMP(g_clientBaseAddr + 0x5DCA0, mp.pAddress);

    // 8. Hook Creature::getAnimationFrame (0x45E130) for smooth multi-frame outfit animations
    union MethodPointerAnim {
        int (NativeCreatureHelper::*pmfn)(int);
        DWORD pAddress;
    };
    MethodPointerAnim mpa;
    mpa.pmfn = &NativeCreatureHelper::Hooked_GetAnimationFrame;
    HookJMP(g_clientBaseAddr + 0x5E130, mpa.pAddress);

    // 9. Hook Magic Effects frame calculation (0x4D9DD0) and update loop (0x4E62F0) for real-time 75ms timing
    HookJMP(g_clientBaseAddr + 0xD9DD0, (DWORD)&Hooked_GetMagicEffectFrame);
    HookJMP(g_clientBaseAddr + 0xE62F0, (DWORD)&Hooked_UpdateEffects);
}

static DWORD WINAPI InitThread(LPVOID lpParam) {
    try {
        SafeInit();
    } catch (...) {
        OutputDebugStringA("[ExtendedDLL] Error initializing hooks in main thread.\n");
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        HANDLE hThread = CreateThread(NULL, 0, InitThread, NULL, 0, NULL);
        if (hThread) {
            CloseHandle(hThread);
        }
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        ShutdownTimerHooks();
        FreeDirectDrawProxy();
    }
    return TRUE;
}
