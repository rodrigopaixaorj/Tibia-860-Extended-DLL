/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "timer.h"
#include "config.h"
#include "hook.h"
#include "main.h"
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

static bool g_timerInitialized = false;

uint32_t __cdecl Hooked_GetAnimTick() {
    // Standard item animation interval: 75 ms per frame
    return static_cast<uint32_t>(timeGetTime() / 75);
}

void InitTimerHooks() {
    if (g_config.highResolutionTimer && !g_timerInitialized) {
        // High resolution timer: set OS timer resolution to 1ms
        timeBeginPeriod(1);
        g_timerInitialized = true;
    }
    if (g_clientBaseAddr) {
        // Hook 0x51D1B0 (GetAnimTick) to return real-time 75ms animation ticks
        HookJMP(g_clientBaseAddr + 0x11D1B0, (uintptr_t)&Hooked_GetAnimTick);
    }
}

void ShutdownTimerHooks() {
    if (g_timerInitialized) {
        timeEndPeriod(1);
        g_timerInitialized = false;
    }
}
