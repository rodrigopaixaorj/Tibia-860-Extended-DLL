/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "features.h"

struct DLLConfig {
    bool highResolutionTimer   = (FEATURE_HIGH_RESOLUTION_TIMER != 0);
    bool extendedSprites       = (FEATURE_EXTENDED_SPRITES != 0);
    bool alphaTransparency     = (FEATURE_ALPHA_TRANSPARENCY != 0);
    bool cacheSprites          = (FEATURE_CACHE_SPRITES != 0);
    bool drawManaBar           = (FEATURE_DRAW_MANA_BAR != 0);
    bool enableMounts          = (FEATURE_ENABLE_MOUNTS != 0);
    bool enableMarket          = (FEATURE_ENABLE_MARKET != 0);
    bool showAttackAnimations  = (FEATURE_SHOW_ATTACK_ANIMATIONS != 0);
    bool extendedOpcode        = (FEATURE_EXTENDED_OPCODE != 0);
    bool extendedMagicEffects  = (FEATURE_EXTENDED_MAGIC_EFFECTS != 0);
    bool extendedPlayerStats   = (FEATURE_EXTENDED_PLAYER_STATS != 0);
    bool extendedPlayerSkills  = (FEATURE_EXTENDED_PLAYER_SKILLS != 0);

    // Animation & Movement Tuning
    int effectSpeedMs          = TUNING_EFFECT_SPEED_MS;

    // Network & Server
    std::string serverIP       = SERVER_IP;
    uint16_t serverPort        = SERVER_PORT;
    std::string customRSAKey   = CUSTOM_RSA_KEY;
};

inline const DLLConfig g_config;

#endif // __CONFIG_H__
