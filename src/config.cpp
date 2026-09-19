/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "config.h"
#include <windows.h>

DLLConfig g_config;

static bool ReadIniBool(const char* section, const char* key, bool defaultValue, const char* filename) {
    int val = GetPrivateProfileIntA(section, key, defaultValue ? 1 : 0, filename);
    return (val != 0);
}

static std::string ReadIniString(const char* section, const char* key, const std::string& defaultValue, const char* filename) {
    char buffer[512];
    DWORD len = GetPrivateProfileStringA(section, key, defaultValue.c_str(), buffer, sizeof(buffer), filename);
    return std::string(buffer, len);
}

void LoadConfig(const std::string& filename) {
    char fullPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, fullPath);
    strcat_s(fullPath, MAX_PATH, "\\");
    strcat_s(fullPath, MAX_PATH, filename.c_str());

    g_config.highResolutionTimer = ReadIniBool("General", "HighResolutionTimer", true, fullPath);
    g_config.extendedSprites     = ReadIniBool("General", "ExtendedSprites", true, fullPath);
    g_config.alphaTransparency   = ReadIniBool("General", "AlphaTransparency", true, fullPath);
    g_config.cacheSprites        = ReadIniBool("General", "CacheSprites", true, fullPath);

    g_config.drawManaBar         = ReadIniBool("Features", "DrawManaBar", true, fullPath);
    g_config.enableMounts        = ReadIniBool("Features", "EnableMounts", true, fullPath);
    g_config.enableMarket        = ReadIniBool("Features", "EnableMarket", true, fullPath);
    g_config.extendedOpcode      = ReadIniBool("Features", "ExtendedOpcode", true, fullPath);

    g_config.extendedMagicEffects = ReadIniBool("Limits", "ExtendedMagicEffects", true, fullPath);
    g_config.extendedPlayerStats  = ReadIniBool("Limits", "ExtendedPlayerStats", true, fullPath);
    g_config.extendedPlayerSkills = ReadIniBool("Limits", "ExtendedPlayerSkills", true, fullPath);

    g_config.serverIP      = ReadIniString("Network", "ServerIP", "", fullPath);
    g_config.serverPort    = static_cast<uint16_t>(GetPrivateProfileIntA("Network", "ServerPort", 7171, fullPath));
    g_config.customRSAKey  = ReadIniString("Network", "CustomRSAKey", "", fullPath);
}

