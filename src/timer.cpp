/*
  Tibia World RPG Client - Extended DLL
  Copyright (C) 2020-2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "timer.h"
#include "config.h"

static LARGE_INTEGER g_frequency;
static LARGE_INTEGER g_startTime;
static bool g_timerInitialized = false;

DWORD WINAPI HookedTimeGetTime() {
    if (!g_timerInitialized) {
        QueryPerformanceFrequency(&g_frequency);
        QueryPerformanceCounter(&g_startTime);
        g_timerInitialized = true;
    }

    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    LONGLONG elapsed = currentTime.QuadPart - g_startTime.QuadPart;
    return static_cast<DWORD>((elapsed * 1000) / g_frequency.QuadPart);
}

void InitTimerHooks() {
    if (g_config.highResolutionTimer) {
        // Replace native timeGetTime if configured
        QueryPerformanceFrequency(&g_frequency);
        QueryPerformanceCounter(&g_startTime);
        g_timerInitialized = true;
    }
}
