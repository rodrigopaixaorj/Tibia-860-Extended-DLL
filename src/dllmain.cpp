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
#include "timer.h"
#include "dat_reader.h"

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
}

static void __stdcall MyDrawHPBar(DWORD nSurface, DWORD X, DWORD Y, DWORD W, DWORD creaturePointer, DWORD nRed, DWORD nGreen, DWORD nBlue) {
    if (nRed == 0 && nGreen == 0 && nBlue == 0) return;
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

// Standard OpenTibia RSA Public Key (1024-bit)
static const char g_openTibiaRSAKey[] =
    "1091201329673994292788609605089955415282375929027929290382885787"
    "8584533835932840908816697375827592878812892590351731203044105555"
    "5106373059410154950149632459400278786856426436669218021685472810"
    "6616118537087815205705633368305019752110325981061502496144570150"
    "284288240361060689914090400755288849721";

static void SafeInit() {
    LoadConfig("config.ini");
    InitDirectDrawProxy();

    HANDLE baseHandle = GetModuleHandle(NULL);
    g_clientBaseAddr = (uint32_t)baseHandle;
    if (!baseHandle) return;

    DWORD dwOldProtect;
    VirtualProtect((LPVOID)(g_clientBaseAddr + 0x1000), 0x238000, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    // RSA Key Patch (allows connection to OpenTibia servers)
    HookMemory(g_clientBaseAddr + 0x1B8980, g_openTibiaRSAKey, sizeof(g_openTibiaRSAKey) - 1);

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

    // High resolution timer hook
    if (g_config.highResolutionTimer) {
        OverWrite(g_clientBaseAddr + 0x1B85A0, (DWORD)&HookedTimeGetTime);
    }

    // DirectDraw 7 Upgrade
    OverWrite(g_clientBaseAddr + 0x1D8840, 0x15E65EC0);
    OverWrite(g_clientBaseAddr + 0x1D8840 + 4, 0x11D23B9C);
    OverWrite(g_clientBaseAddr + 0x1D8840 + 8, 0x60002FB9);
    OverWrite(g_clientBaseAddr + 0x1D8840 + 12, 0x5BEA9797);
    OverWriteByte(g_clientBaseAddr + 0x1C1DB7, 0x37);

    // Limits Patches
    if (g_config.extendedMagicEffects) {
        HookCall(g_clientBaseAddr + 0x104B4, g_clientBaseAddr + 0xF9C00);
        OverWriteByte(g_clientBaseAddr + 0x104BA, 0xB7);
    }

    if (g_config.extendedPlayerStats) {
        HookCall(g_clientBaseAddr + 0x11D2B, g_clientBaseAddr + 0xF9DA0);
        OverWrite(g_clientBaseAddr + 0x11D30, 0x8990F08B);
        HookCall(g_clientBaseAddr + 0x11D36, g_clientBaseAddr + 0xF9DA0);
        OverWrite(g_clientBaseAddr + 0x11D3B, 0x8990F88B);

        HookCall(g_clientBaseAddr + 0x11D69, g_clientBaseAddr + 0xF9DA0);
        OverWrite(g_clientBaseAddr + 0x11D6E, 0x8990D08B);
        HookCall(g_clientBaseAddr + 0x11D74, g_clientBaseAddr + 0xF9DA0);
        OverWrite(g_clientBaseAddr + 0x11D79, 0x89909090);
    }

    if (g_config.extendedPlayerSkills) {
        HookCall(g_clientBaseAddr + 0x11FA4, g_clientBaseAddr + 0xF9C00);
        OverWriteByte(g_clientBaseAddr + 0x11FAA, 0xB7);
    }

    g_newRenderer = (Render_NEW*)calloc(1, sizeof(*g_newRenderer));
    if (g_newRenderer) {
        if (g_config.drawManaBar) {
            g_newRenderer->DrawRectangle = MyDrawHPBar;
            HookCall(g_clientBaseAddr + 0xF5675, (DWORD)&HPBarRenderHandle);
            HookCall(g_clientBaseAddr + 0xF573E, (DWORD)&HPBarRenderHandle);
            HookCall(g_clientBaseAddr + 0xF5875, (DWORD)&HPBarRenderHandle);
            HookCall(g_clientBaseAddr + 0xF5922, (DWORD)&HPBarRenderHandle);
            OverWriteWord(g_clientBaseAddr + 0xF57D8, 0xFFD0);
            OverWriteWord(g_clientBaseAddr + 0xF5912, 0xFFD0);
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
        FreeDirectDrawProxy();
    }
    return TRUE;
}
