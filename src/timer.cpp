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

void InitTimerHooks() {
    if (g_config.highResolutionTimer && !g_timerInitialized) {
        // High resolution timer: set OS timer resolution to 1ms
        // Ensures smooth frame pacing and eliminates Windows 10/11 15.6ms scheduler stutter
        timeBeginPeriod(1);
        g_timerInitialized = true;
    }
}

void ShutdownTimerHooks() {
    if (g_timerInitialized) {
        timeEndPeriod(1);
        g_timerInitialized = false;
    }
}
