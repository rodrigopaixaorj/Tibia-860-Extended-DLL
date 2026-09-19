/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>

struct DLLConfig {
    bool highResolutionTimer = true;
    bool extendedSprites = true;
    bool alphaTransparency = true;
    bool cacheSprites = true;
    bool drawManaBar = true;
    bool enableMounts = true;
    bool enableMarket = true;
    bool extendedOpcode = true;
    bool extendedMagicEffects = true;
    bool extendedPlayerStats = true;
    bool extendedPlayerSkills = true;
};

extern DLLConfig g_config;

void LoadConfig(const std::string& filename = "config.ini");

#endif // __CONFIG_H__
