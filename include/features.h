/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __FEATURES_H__
#define __FEATURES_H__

#include <cstdint>
#include <string>

// ===================================================================
// General Engine & Graphics Features
// ===================================================================

// Enables high resolution timer for Windows 10/11 (fixes FPS stuttering)
#define FEATURE_HIGH_RESOLUTION_TIMER 1

// Enables extended sprite limit (> 65,535 sprites via uint32_t)
#define FEATURE_EXTENDED_SPRITES 1

// Enables support for 32-bit Alpha transparency (ARGB) in OpenGL/DirectX
#define FEATURE_ALPHA_TRANSPARENCY 0

// Enables in-memory sprite texture caching for maximum performance
#define FEATURE_CACHE_SPRITES 1

// ===================================================================
// Gameplay & Interface Features
// ===================================================================

// Enables mana bar drawn below the player
#define FEATURE_DRAW_MANA_BAR 1

// Enables mounts system and layer elevation (+3px)
#define FEATURE_ENABLE_MOUNTS 1

// Enables modern Market System graphical interface
#define FEATURE_ENABLE_MARKET 1

// Enables weapon attack directional slash animations (304, 305, 306, 307, 309)
#define FEATURE_SHOW_ATTACK_ANIMATIONS 1

// Enables extended server communication channel (Opcode 0x32 / 50)
#define FEATURE_EXTENDED_OPCODE 1

// ===================================================================
// Limits & Protocol Expansion
// ===================================================================

// Breaks magic effects limit (uint8_t 255 -> uint16_t 65535)
#define FEATURE_EXTENDED_MAGIC_EFFECTS 1

// Breaks player health and mana display limit (uint16_t -> int32_t)
#define FEATURE_EXTENDED_PLAYER_STATS 1

// Breaks player skills display limit (uint8_t -> uint16_t)
#define FEATURE_EXTENDED_PLAYER_SKILLS 0

// ===================================================================
// Animation & Movement Tuning
// ===================================================================

// Magic Effects animation speed: duration per frame in milliseconds (12.90+ standard = 50ms)
#define TUNING_EFFECT_SPEED_MS 50

// ===================================================================
// Network & Auto-Redirection
// ===================================================================

// Automatically redirect login servers to this IP and Port (no external IP Changer needed if set)
// Leave empty ("") if you prefer using an external IP Changer
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 7171

// Optional Custom RSA Key (1024-bit decimal string).
// If empty (""), standard OpenTibia 1024-bit RSA key is used automatically.
#define CUSTOM_RSA_KEY ""

#endif // __FEATURES_H__
