/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __TIMER_H__
#define __TIMER_H__

#include <windows.h>
#include <cstdint>

void InitTimerHooks();
DWORD WINAPI HookedTimeGetTime();

#endif // __TIMER_H__
