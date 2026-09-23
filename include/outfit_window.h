/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __OUTFIT_WINDOW_H__
#define __OUTFIT_WINDOW_H__

#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

struct OutfitDetail {
    uint16_t lookType;
    std::string name;
    uint8_t addons;
};

struct MountDetail {
    uint16_t mountId;
    std::string name;
};

class NativeOutfitManager {
public:
    static void InitHooks();
    static void ApplyNativeLayoutPatch();

    // Called by network_protocol when opcode 200 is received
    static void SetMountData(uint16_t currentMount,
                             uint8_t mountHead, uint8_t mountBody, uint8_t mountLegs, uint8_t mountFeet,
                             const std::vector<MountDetail>& mounts);

    static uint16_t GetSelectedMountId();
    static void GetSelectedMountColors(uint8_t& head, uint8_t& body, uint8_t& legs, uint8_t& feet);

    // Mount / Dismount toggling
    static bool IsLocalPlayerMounted();
    static void ToggleMount();
    static void SendToggleMount(bool mount);

    static void OnOutfitDialogCreated(void* dialogContent);
    static bool OnOutfitDialogEvent(void* dialogContent, int controlId);

private:

    // Native hook trampolines
    static void* __cdecl Hook_CreateOutfitDialog(void* outfitData, void* namesData);
    static void __thiscall Hook_OutfitDialogOnEvent(void* pThis, int controlId);

    static std::vector<MountDetail> s_mounts;
    static int32_t s_currentMountIndex;
    static uint16_t s_selectedMountId;
    static uint8_t s_mountColors[4];

    // Pointers to native child controls added to the official OutfitDialog
    static void* s_mountCreatureControl;
    static void* s_mountLabelControl;
    static void* s_currentDialog;
};

#endif // __OUTFIT_WINDOW_H__
