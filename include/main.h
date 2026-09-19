/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __MAIN_H__
#define __MAIN_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <d3d9.h>
#include <gl/GL.h>

#define PROJECT_NAME "Extended Tibia"
#define PROJECT_AUTHOR "Nottinghster (github.com/rodrigopaixaorj)"
#define PROJECT_VERSION "1.0.0"

typedef uint32_t (*_GetEngineAddr)();
extern _GetEngineAddr GetEngineAddr;

typedef void (*_DrawSkin)(int nSurface, int X, int Y, int W, int H, int SkinId, int dX, int dY);
extern _DrawSkin DrawSkin;

typedef void (*_PrintText)(int nSurface, int nX, int nY, int nFont, int nRed, int nGreen, int nBlue, const char* lpText, int nAlign);
extern _PrintText PrintText;

typedef const char* (*_GetFileName)(int fileId, bool backup);
extern _GetFileName GetFileName;

extern uint32_t g_clientBaseAddr;
extern uint32_t g_clientVersion;
extern uint32_t g_clientPointerTransPixels;

#endif // __MAIN_H__
