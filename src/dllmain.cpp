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
