/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "outfit_window.h"
#include "creature_manager.h"
#include "features.h"
#include "main.h"
#include "hook.h"
#include <algorithm>

// Static state
std::vector<MountDetail> NativeOutfitManager::s_mounts;
int32_t NativeOutfitManager::s_currentMountIndex = 0;
uint16_t NativeOutfitManager::s_selectedMountId = 0;
uint8_t NativeOutfitManager::s_mountColors[4] = { 0, 0, 0, 0 };

void* NativeOutfitManager::s_mountCreatureControl = nullptr;
void* NativeOutfitManager::s_mountLabelControl = nullptr;
void* NativeOutfitManager::s_currentDialog = nullptr;

const int EVENT_MOUNT_PREV = 711;
const int EVENT_MOUNT_NEXT = 712;

// Native engine client_malloc (0x5719E2)
static void* Call_ClientMalloc(size_t size) {
    if (!g_clientBaseAddr) return nullptr;
    typedef void* (__cdecl* t_ClientMalloc)(size_t);
    return ((t_ClientMalloc)(g_clientBaseAddr + 0x1719E2))(size);
}

// DialogContent::SetDimensions (0x4C09E0) - __thiscall with 2 DWORD args (ret 8)
static void Call_SetDimensions(void* pThis, uint32_t width, uint32_t height) {
    if (!g_clientBaseAddr || !pThis) return;
    uint32_t func = g_clientBaseAddr + 0xC09E0;
    __asm {
        push height
        push width
        mov ecx, pThis
        call func
    }
}

// DialogContent::AddChild (0x4C0C60) - __thiscall with 1 DWORD arg (ret 4)
static void Call_AddChild(void* pThis, void* child) {
    if (!g_clientBaseAddr || !pThis || !child) return;
    uint32_t func = g_clientBaseAddr + 0xC0C60;
    __asm {
        push child
        mov ecx, pThis
        call func
    }
}

// DialogContent::SetChildBounds (0x4C4110) - __thiscall with child + 4 DWORDs Rect (ret 0x14)
static void Call_SetChildBounds(void* pThis, void* child, uint32_t left, uint32_t top, uint32_t width, uint32_t height) {
    if (!g_clientBaseAddr || !pThis || !child) return;
    uint32_t func = g_clientBaseAddr + 0xC4110;
    __asm {
        push height
        push width
        push top
        push left
        push child
        mov ecx, pThis
        call func
    }
}

// CreatureControl::CreatureControl (0x47C130) - __thiscall with 5 DWORD args (ret 0x14)
static void* Call_CreatureControlCtor(void* pThis, uint32_t lookType, uint32_t dir, void* colorsPtr, uint32_t addons, void* sizePtr) {
    if (!g_clientBaseAddr || !pThis) return nullptr;
    uint32_t func = g_clientBaseAddr + 0x7C130;
    void* result = nullptr;
    __asm {
        push sizePtr
        push addons
        push colorsPtr
        push dir
        push lookType
        mov ecx, pThis
        call func
        mov result, eax
    }
    return result;
}

// CreatureControl::SetLookType (0x464EA0) - __thiscall with 1 DWORD arg (ret 4)
static void Call_CreatureSetLookType(void* creatureCtrl, uint32_t lookType) {
    if (!g_clientBaseAddr || !creatureCtrl) return;
    uint32_t func = g_clientBaseAddr + 0x64EA0;
    __asm {
        push lookType
        mov ecx, creatureCtrl
        call func
    }
    uintptr_t vtable = *(uintptr_t*)creatureCtrl;
    if (vtable) {
        typedef void(__thiscall* t_Redraw)(void*);
        ((t_Redraw)(*(uintptr_t*)(vtable + 0x48)))(creatureCtrl);
    }
}

// CreatureControl::SetColors (0x46AFC0) - __thiscall with 1 DWORD arg (ret 4)
static void Call_CreatureSetColors(void* creatureCtrl, void* colorsPtr) {
    if (!g_clientBaseAddr || !creatureCtrl || !colorsPtr) return;
    uint32_t func = g_clientBaseAddr + 0x6AFC0;
    __asm {
        push colorsPtr
        mov ecx, creatureCtrl
        call func
    }
    uintptr_t vtable = *(uintptr_t*)creatureCtrl;
    if (vtable) {
        typedef void(__thiscall* t_Redraw)(void*);
        ((t_Redraw)(*(uintptr_t*)(vtable + 0x48)))(creatureCtrl);
    }
}

// BorderContainer (0x467CD0) - wraps child with native sunken frame border
static void* Call_CreateBorderContainer(void* childControl) {
    if (!g_clientBaseAddr || !childControl) return nullptr;
    void* pMem = Call_ClientMalloc(0x68);
    if (!pMem) return nullptr;
    uint32_t func = g_clientBaseAddr + 0x67CD0;
    void* result = nullptr;
    __asm {
        push 0
        push 0
        push 8
        push 0
        push 0xA
        push 0xB
        push 0
        push 9
        push 0
        push childControl
        mov ecx, pMem
        call func
        mov result, eax
    }
    return result;
}

// Arrow Button (0x47C310) - __thiscall button with native arrow icons
static void* Call_ArrowButtonCtor(void* pThis, uint32_t icon1, uint32_t icon2, uint32_t eventId, void* target) {
    if (!g_clientBaseAddr || !pThis) return nullptr;
    uint32_t func = g_clientBaseAddr + 0x7C310;
    void* result = nullptr;
    __asm {
        push 0          // arg 8
        push 0          // arg 7 (tooltip)
        push eventId    // arg 6 (eventId)
        push target     // arg 5 (target dialogContent)
        push icon2      // arg 4
        push icon1      // arg 3
        push 0          // arg 2
        push 0          // arg 1
        mov ecx, pThis  // this (malloc 0x48)
        call func
        mov result, eax
    }
    return result;
}

// TextDisplayControl (0x47B420) - native centered text display
static void* Call_CreateTextDisplay(const char* text) {
    if (!g_clientBaseAddr || !text) return nullptr;
    void* pMem = Call_ClientMalloc(0x4C);
    if (!pMem) return nullptr;
    uint32_t func = g_clientBaseAddr + 0x7B420;
    uint32_t fontData = g_clientBaseAddr + 0x24B774;
    void* result = nullptr;
    __asm {
        push text
        sub esp, 0x14
        mov ecx, esp
        mov edx, fontData
        mov eax, [edx]
        mov [ecx], eax
        mov eax, [edx + 4]
        mov [ecx + 4], eax
        mov eax, [edx + 8]
        mov [ecx + 8], eax
        mov eax, [edx + 12]
        mov [ecx + 12], eax
        mov eax, [edx + 16]
        mov [ecx + 16], eax
        mov ecx, pMem
        call func
        mov result, eax
    }
    return result;
}

// TextDisplayControl::SetText (vtable[33] offset +0x84)
static void Call_LabelSetText(void* labelControl, const char* text) {
    if (!labelControl || !text) return;
    uintptr_t vtable = *(uintptr_t*)labelControl;
    if (vtable) {
        typedef void(__thiscall* t_SetText)(void* pThis, const char* str, int flag);
        ((t_SetText)(*(uintptr_t*)(vtable + 0x84)))(labelControl, text, 0);

        typedef void(__thiscall* t_Redraw)(void*);
        ((t_Redraw)(*(uintptr_t*)(vtable + 0x48)))(labelControl);
    }
}

void NativeOutfitManager::SetMountData(uint16_t currentMount,
                                       uint8_t mountHead, uint8_t mountBody, uint8_t mountLegs, uint8_t mountFeet,
                                       const std::vector<MountDetail>& mounts) {
    s_mountColors[0] = mountHead;
    s_mountColors[1] = mountBody;
    s_mountColors[2] = mountLegs;
    s_mountColors[3] = mountFeet;

    s_mounts.clear();
    // Always include a "None" mount option as index 0
    s_mounts.push_back({ 0, "None" });
    for (const auto& m : mounts) {
        if (m.mountId != 0) {
            s_mounts.push_back(m);
        }
    }

    s_selectedMountId = currentMount;
    s_currentMountIndex = 0;
    for (size_t i = 0; i < s_mounts.size(); ++i) {
        if (s_mounts[i].mountId == currentMount) {
            s_currentMountIndex = static_cast<int32_t>(i);
            break;
        }
    }
}

uint16_t NativeOutfitManager::GetSelectedMountId() {
    return s_selectedMountId;
}

void NativeOutfitManager::GetSelectedMountColors(uint8_t& head, uint8_t& body, uint8_t& legs, uint8_t& feet) {
    head = s_mountColors[0];
    body = s_mountColors[1];
    legs = s_mountColors[2];
    feet = s_mountColors[3];
}

bool NativeOutfitManager::IsLocalPlayerMounted() {
    if (!g_clientBaseAddr) return false;
    uint32_t localPlayerId = *(uint32_t*)(g_clientBaseAddr + 0x23FE98);
    if (!localPlayerId) return false;
    ExtendedCreature ext;
    if (CreatureManager::get().getExtendedData(localPlayerId, ext)) {
        return ext.mount.mountId != 0;
    }
    return false;
}

void NativeOutfitManager::SendToggleMount(bool mount) {
    if (!g_clientBaseAddr) return;
    uint32_t localPlayerId = *(uint32_t*)(g_clientBaseAddr + 0x23FE98);
    if (!localPlayerId) return;

    typedef void(__cdecl* t_StartPacket)(uint8_t opcode);
    typedef void(__cdecl* t_WriteByte)(uint8_t val);
    typedef void(__cdecl* t_SendPacket)(int flag);

    t_StartPacket startPacket = (t_StartPacket)(g_clientBaseAddr + 0xF8290);
    t_WriteByte   writeByte   = (t_WriteByte)(g_clientBaseAddr + 0xF8560);
    t_SendPacket  sendPacket  = (t_SendPacket)(g_clientBaseAddr + 0xF8E40);

    startPacket(0xD4); // opcode 0xD4 (toggleMount)
    writeByte(mount ? 1 : 0);
    sendPacket(1);
}

void NativeOutfitManager::ToggleMount() {
    if (!g_clientBaseAddr) return;
    uint32_t localPlayerId = *(uint32_t*)(g_clientBaseAddr + 0x23FE98);
    if (!localPlayerId) return;

    bool isMounted = IsLocalPlayerMounted();
    SendToggleMount(!isMounted);
}

void NativeOutfitManager::OnOutfitDialogCreated(void* dialogContent) {
    s_currentDialog = dialogContent;
    s_mountCreatureControl = nullptr;
    s_mountLabelControl = nullptr;

#if FEATURE_ENABLE_MOUNTS
    if (!g_clientBaseAddr || !dialogContent) return;

    try {
        Call_SetDimensions(dialogContent, 490, 284);

        // Determine initial mount lookType
        uint32_t mountLookType = 0;
        if (s_currentMountIndex >= 0 && s_currentMountIndex < (int32_t)s_mounts.size()) {
            mountLookType = s_mounts[s_currentMountIndex].mountId;
        }

        // 1. Create native CreatureControl for mount preview (140x140)
        void* mountCtrl = Call_ClientMalloc(0x4C);
        if (mountCtrl) {
            void* sizePtr = (void*)(g_clientBaseAddr + 0x24B708); // standard 71x71 size array in .data
            void* colorsPtr = (void*)((uintptr_t)dialogContent + 0x270); // outfit current colors

            Call_CreatureControlCtor(mountCtrl, mountLookType, 2 /* SOUTH */, colorsPtr, 0, sizePtr);

            // Wrap in native Border Container (0x467CD0) for sunken frame border!
            void* mountBox = Call_CreateBorderContainer(mountCtrl);
            if (mountBox) {
                Call_AddChild(dialogContent, mountBox);
                Call_SetChildBounds(dialogContent, mountBox, 337, 112, 140, 140);
            } else {
                Call_AddChild(dialogContent, mountCtrl);
                Call_SetChildBounds(dialogContent, mountCtrl, 337, 112, 140, 140);
            }
            s_mountCreatureControl = mountCtrl;
        }

        // 2. Create Mount Previous button ("<", ID 711) with native arrow icon (0x46, 0x47)
        void* btnPrev = Call_ClientMalloc(0x48);
        if (btnPrev) {
            Call_ArrowButtonCtor(btnPrev, 0x46, 0x47, EVENT_MOUNT_PREV, dialogContent);
            Call_AddChild(dialogContent, btnPrev);
            Call_SetChildBounds(dialogContent, btnPrev, 318, 256, 20, 20);
        }

        // 3. Create Mount Name display inside a sunken border groove (W = 116px)
        const char* mountName = (s_currentMountIndex >= 0 && s_currentMountIndex < (int32_t)s_mounts.size())
                                ? s_mounts[s_currentMountIndex].name.c_str() : "None";
        void* lblMount = Call_CreateTextDisplay(mountName);
        if (lblMount) {
            void* lblBox = Call_CreateBorderContainer(lblMount);
            if (lblBox) {
                Call_AddChild(dialogContent, lblBox);
                Call_SetChildBounds(dialogContent, lblBox, 340, 256, 116, 20);
            } else {
                Call_AddChild(dialogContent, lblMount);
                Call_SetChildBounds(dialogContent, lblMount, 340, 256, 116, 20);
            }
            s_mountLabelControl = lblMount;
        }

        // 4. Create Mount Next button (">", ID 712) with native arrow icon (0x48, 0x49)
        void* btnNext = Call_ClientMalloc(0x48);
        if (btnNext) {
            Call_ArrowButtonCtor(btnNext, 0x48, 0x49, EVENT_MOUNT_NEXT, dialogContent);
            Call_AddChild(dialogContent, btnNext);
            Call_SetChildBounds(dialogContent, btnNext, 458, 256, 20, 20);
        }
    } catch (...) {
        // Fallback safely to prevent crashing dialog
    }
#endif
}

bool NativeOutfitManager::OnOutfitDialogEvent(void* dialogContent, int controlId) {
#if FEATURE_ENABLE_MOUNTS
    if (!dialogContent) return false;

    // Handle Mount Previous
    if (controlId == EVENT_MOUNT_PREV) {
        if (!s_mounts.empty()) {
            s_currentMountIndex--;
            if (s_currentMountIndex < 0) {
                s_currentMountIndex = static_cast<int32_t>(s_mounts.size() - 1);
            }
            s_selectedMountId = s_mounts[s_currentMountIndex].mountId;

            Call_CreatureSetLookType(s_mountCreatureControl, s_selectedMountId);
            Call_LabelSetText(s_mountLabelControl, s_mounts[s_currentMountIndex].name.c_str());
        }
        return true; // Handled
    }

    // Handle Mount Next
    if (controlId == EVENT_MOUNT_NEXT) {
        if (!s_mounts.empty()) {
            s_currentMountIndex++;
            if (s_currentMountIndex >= (int32_t)s_mounts.size()) {
                s_currentMountIndex = 0;
            }
            s_selectedMountId = s_mounts[s_currentMountIndex].mountId;

            Call_CreatureSetLookType(s_mountCreatureControl, s_selectedMountId);
            Call_LabelSetText(s_mountLabelControl, s_mounts[s_currentMountIndex].name.c_str());
        }
        return true; // Handled
    }

    // Handle Color Palette changes (control IDs 2000 to 2215): sync colors to mount preview
    if (controlId >= 2000 && controlId <= 2215) {
        if (s_mountCreatureControl && dialogContent) {
            void* pColors = (void*)((uintptr_t)dialogContent + 0x270);
            Call_CreatureSetColors(s_mountCreatureControl, pColors);
        }
    }

    // Handle OK button (ID 1)
    if (controlId == 1) {
        if (s_currentMountIndex >= 0 && s_currentMountIndex < (int32_t)s_mounts.size()) {
            s_selectedMountId = s_mounts[s_currentMountIndex].mountId;
        }
        if (dialogContent) {
            uint8_t* pColors = (uint8_t*)((uintptr_t)dialogContent + 0x270);
            s_mountColors[0] = pColors[0];
            s_mountColors[1] = pColors[1];
            s_mountColors[2] = pColors[2];
            s_mountColors[3] = pColors[3];
        }
    }
#endif

    return false; // Let native handler process default buttons (OK, Cancel, Outfits, Addons, Colors)
}

// Helper called right after 0x4988F0 finishes construction
extern "C" void __cdecl C_OnOutfitDialogPostInit(void* dialogContent) {
    NativeOutfitManager::OnOutfitDialogCreated(dialogContent);
}

// Naked hook replacing call 0x4988F0 at 0x4A0A1D
__declspec(naked) static void Hook_CreateOutfitDialog_Naked() {
    __asm {
        // [esp + 0] = ret_to_0x4A0A22
        // [esp + 4] = outfitData
        // [esp + 8] = namesData
        // ecx = this (dialogContent)

        // Duplicate the 2 parameters on stack for 0x4988F0
        push dword ptr [esp + 8] // push namesData
        push dword ptr [esp + 8] // push outfitData
        // Now stack has:
        // [esp + 0] = outfitData
        // [esp + 4] = namesData
        // [esp + 8] = ret_to_0x4A0A22
        // [esp + 12] = outfitData (original)
        // [esp + 16] = namesData (original)

        mov eax, g_clientBaseAddr
        add eax, 0x988F0
        call eax
        // 0x4988F0 consumed the two pushed arguments with ret 8!
        // eax = constructed dialogContent
        // Stack now has:
        // [esp + 0] = ret_to_0x4A0A22
        // [esp + 4] = outfitData (original)
        // [esp + 8] = namesData (original)

        // Save registers across post-init C call
        push eax
        push ecx
        push edx

        push eax // argument for C_OnOutfitDialogPostInit
        call C_OnOutfitDialogPostInit
        add esp, 4

        pop edx
        pop ecx
        pop eax  // restore eax = constructed dialogContent

        // Return to 0x4A0A22 and clean caller's original outfitData and namesData
        ret 8
    }
}

// C-linkage helper for virtual OnEvent hook
extern "C" bool __cdecl C_OnOutfitDialogEvent(void* pThis, int controlId) {
    return NativeOutfitManager::OnOutfitDialogEvent(pThis, controlId);
}

// Naked hook on OutfitDialogContent::OnEvent (virtual method in vtable 0x5C1190 index 28)
__declspec(naked) static void Hook_OutfitDialogOnEvent_Naked() {
    __asm {
        push ecx                 // PRESERVE THIS (ecx)!
        push dword ptr [esp + 8] // controlId (offset +8 because we pushed ecx)
        push ecx                 // pThis
        call C_OnOutfitDialogEvent
        add esp, 8
        pop ecx                  // RESTORE THIS IN ECX!
        test al, al
        jnz handled
        // Jump to original 0x487F90 with ECX 100% intact!
        mov eax, g_clientBaseAddr
        add eax, 0x87F90
        jmp eax
handled:
        ret 4
    }
}

// C-linkage helper for virtual CreatureControl::Render hook
extern "C" bool __cdecl C_OnCreatureControlRender(void* pThis, void* surface, int x, int y) {
    if (!pThis) return false;
    uint32_t lookType = *(uint32_t*)((uintptr_t)pThis + 0x24);
    if (lookType == 0) {
        return true; // Handled: "None" mount, do not render creature sprite to avoid Tibia.dat index exception
    }
    uint32_t width = *(uint32_t*)((uintptr_t)pThis + 0x1C);
    uint32_t height = *(uint32_t*)((uintptr_t)pThis + 0x20);
    if (width != height) {
        // Enforce square dimensions if ever mismatched to prevent DialogElements.cpp:981 assertion
        *(uint32_t*)((uintptr_t)pThis + 0x20) = width;
    }
    return false; // Not handled, proceed to original 0x46ab50
}

// Naked hook on CreatureControl::Render (virtual method at 0x5BFF84)
__declspec(naked) static void Hook_CreatureControlRender_Naked() {
    __asm {
        push ecx                 // Preserve this in ecx
        // Stack after push ecx:
        // [esp + 0] = saved ecx
        // [esp + 4] = ret_addr
        // [esp + 8] = surface
        // [esp + 12] = x
        // [esp + 16] = y

        push dword ptr [esp + 16] // y
        push dword ptr [esp + 16] // x
        push dword ptr [esp + 16] // surface
        push ecx                  // pThis
        call C_OnCreatureControlRender
        add esp, 16

        pop ecx                  // Restore this in ecx

        test al, al
        jnz handled              // If handled (lookType == 0), skip native render

        // Jump to original 0x46ab50 with ECX, stack and all registers 100% intact!
        mov eax, g_clientBaseAddr
        add eax, 0x6AB50
        jmp eax

handled:
        ret 0xC                  // Clean caller's 3 stack arguments (surface, x, y)
    }
}

#define COMMAND_TOGGLE_MOUNT 0x27F0

// C-linkage helper for ContextMenu AddItem hook (0x453283)
extern "C" void __cdecl C_ContextMenu_AddOutfit(void* pContextMenu, uint32_t cmdId, const char* text, uint32_t flag) {
    if (!g_clientBaseAddr || !pContextMenu) return;
    typedef void(__thiscall* t_AddItem)(void* pMenu, uint32_t cmd, const char* str, uint32_t flg);
    t_AddItem nativeAddItem = (t_AddItem)(g_clientBaseAddr + 0x52320);

    // 1. Add "Set Outfit" as normal
    nativeAddItem(pContextMenu, cmdId, text, flag);

    // 2. Add "Mount" or "Dismount" right below "Set Outfit"!
    bool isMounted = NativeOutfitManager::IsLocalPlayerMounted();
    const char* mountText = isMounted ? "Dismount" : "Mount";
    nativeAddItem(pContextMenu, COMMAND_TOGGLE_MOUNT, mountText, 0);
}

// Naked hook replacing call 0x452320 at 0x453283
__declspec(naked) static void Hook_ContextMenu_AddOutfit_Naked() {
    __asm {
        push ebp
        mov ebp, esp

        // Stack layout:
        // [ebp + 0] = saved ebp
        // [ebp + 4] = ret_addr (0x453288)
        // [ebp + 8] = cmdId (0x2718)
        // [ebp + 12] = text ("Set Outfit")
        // [ebp + 16] = flag (0)
        // ecx = pContextMenu

        push dword ptr [ebp + 16] // flag
        push dword ptr [ebp + 12] // text
        push dword ptr [ebp + 8]  // cmdId
        push ecx                  // pContextMenu
        call C_ContextMenu_AddOutfit
        add esp, 16

        pop ebp
        ret 12                    // clean original 3 parameters
    }
}

// Naked hook on PlayerContextMenu::OnEvent (vtable 0x5BD950 index 28, offset +0x70)
__declspec(naked) static void Hook_ContextMenu_Execute_Naked() {
    __asm {
        // [esp + 0] = ret_addr
        // [esp + 4] = commandId
        // ecx = this (PlayerContextMenu)
        cmp dword ptr [esp + 4], 0x27F0
        jne call_original

        // Toggle mount action!
        call NativeOutfitManager::ToggleMount
        mov eax, 1
        ret 4

    call_original:
        mov eax, g_clientBaseAddr
        add eax, 0x4E960
        jmp eax
    }
}

// Naked hook on PlayerContextMenu2::OnEvent (vtable 0x5BD9F0 index 28, offset +0x70)
__declspec(naked) static void Hook_ContextMenu2_Execute_Naked() {
    __asm {
        // [esp + 0] = ret_addr
        // [esp + 4] = commandId
        // ecx = this (PlayerContextMenu2)
        cmp dword ptr [esp + 4], 0x27F0
        jne call_original

        // Toggle mount action!
        call NativeOutfitManager::ToggleMount
        mov eax, 1
        ret 4

    call_original:
        mov eax, g_clientBaseAddr
        add eax, 0x4F3E0
        jmp eax
    }
}

void NativeOutfitManager::ApplyNativeLayoutPatch() {
    if (!g_clientBaseAddr) return;

    DWORD oldProtect = 0;
    uintptr_t layoutData = g_clientBaseAddr + 0x24B6EC;
    VirtualProtect((void*)layoutData, 0x100, PAGE_READWRITE, &oldProtect);

    // 1. Overall DialogContent dimensions (Compact 490 x 284)
    *(uint32_t*)(g_clientBaseAddr + 0x24B6EC) = 490; // Dialog Content Width
    *(uint32_t*)(g_clientBaseAddr + 0x24B6F0) = 284; // Dialog Content Height

    // 2. Character Preview Box (Top Left: 10, 14, 140, 140)
    *(uint32_t*)(g_clientBaseAddr + 0x24B6F4) = 10;  // X
    *(uint32_t*)(g_clientBaseAddr + 0x24B6F8) = 14;  // Y
    *(uint32_t*)(g_clientBaseAddr + 0x24B6FC) = 140; // W
    *(uint32_t*)(g_clientBaseAddr + 0x24B700) = 140; // H

    // 3. Outfit Navigation Controls (Below Character Preview: Y = 158)
    *(uint32_t*)(g_clientBaseAddr + 0x24B714) = 10;  // Prev Button (<) X
    *(uint32_t*)(g_clientBaseAddr + 0x24B718) = 158; // Prev Button (<) Y
    *(uint32_t*)(g_clientBaseAddr + 0x24B71C) = 130; // Next Button (>) X
    *(uint32_t*)(g_clientBaseAddr + 0x24B720) = 158; // Next Button (>) Y

    *(uint32_t*)(g_clientBaseAddr + 0x24B764) = 33;  // Outfit Name Label X
    *(uint32_t*)(g_clientBaseAddr + 0x24B768) = 158; // Outfit Name Label Y
    *(uint32_t*)(g_clientBaseAddr + 0x24B76C) = 94;  // Outfit Name Label W
    *(uint32_t*)(g_clientBaseAddr + 0x24B770) = 20;  // Outfit Name Label H

    // 4. Color Mode Buttons (Head, Primary, Secondary, Detail: X = 165, Y = 14..86)
    *(uint32_t*)(g_clientBaseAddr + 0x24B724) = 165; // Head X
    *(uint32_t*)(g_clientBaseAddr + 0x24B728) = 14;  // Head Y
    *(uint32_t*)(g_clientBaseAddr + 0x24B72C) = 165; // Primary X
    *(uint32_t*)(g_clientBaseAddr + 0x24B730) = 38;  // Primary Y
    *(uint32_t*)(g_clientBaseAddr + 0x24B734) = 165; // Secondary X
    *(uint32_t*)(g_clientBaseAddr + 0x24B738) = 62;  // Secondary Y
    *(uint32_t*)(g_clientBaseAddr + 0x24B73C) = 165; // Detail X
    *(uint32_t*)(g_clientBaseAddr + 0x24B740) = 86;  // Detail Y

    // 5. Color Palette Matrix (Top Right: X = 230, Y = 14)
    *(uint32_t*)(g_clientBaseAddr + 0x24B744) = 230; // Color Matrix X
    *(uint32_t*)(g_clientBaseAddr + 0x24B748) = 14;  // Color Matrix Y

    // 6. Addon Checkboxes (Center, Below Color Mode Buttons: X = 165, Y = 115)
    *(uint32_t*)(g_clientBaseAddr + 0x24B754) = 165; // Addon X
    *(uint32_t*)(g_clientBaseAddr + 0x24B758) = 115; // Addon Y
    *(uint32_t*)(g_clientBaseAddr + 0x24B75C) = 140; // Addon W
    *(uint32_t*)(g_clientBaseAddr + 0x24B760) = 20;  // Addon H

    // 7. Description / Help Text (Bottom Left, Below Outfit Navigation: X = 10, Y = 188, W = 300, H = 88)
    *(uint32_t*)(g_clientBaseAddr + 0x24B788) = 10;  // Help Text X
    *(uint32_t*)(g_clientBaseAddr + 0x24B78C) = 188; // Help Text Y
    *(uint32_t*)(g_clientBaseAddr + 0x24B790) = 300; // Help Text Width
    *(uint32_t*)(g_clientBaseAddr + 0x24B794) = 88;  // Help Text Height

    VirtualProtect((void*)layoutData, 0x100, oldProtect, &oldProtect);
}

void NativeOutfitManager::InitHooks() {
    if (g_clientBaseAddr) {
        // Apply the compact 8.70-style layout patch to native layout globals in .data
        ApplyNativeLayoutPatch();

        // Hook the call to OutfitDialogContent constructor in CreateWindow_0x11 at 0x4A0A1D
        HookCall(g_clientBaseAddr + 0xA0A1D, (uintptr_t)&Hook_CreateOutfitDialog_Naked);

        // Hook OutfitDialogContent::OnEvent in vtable 0x5C1190 index 28 (offset +0x70)
        DWORD oldProtect = 0;
        uintptr_t vtableEntry = g_clientBaseAddr + 0x1C1190 + (28 * 4);
        VirtualProtect((void*)vtableEntry, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
        *(uintptr_t*)vtableEntry = (uintptr_t)&Hook_OutfitDialogOnEvent_Naked;
        VirtualProtect((void*)vtableEntry, 4, oldProtect, &oldProtect);

        // Hook CreatureControl::Render in vtable 0x5BFF40 at 0x5BFF84
        uintptr_t vtableCreatureRender = g_clientBaseAddr + 0x1BFF84;
        VirtualProtect((void*)vtableCreatureRender, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
        *(uintptr_t*)vtableCreatureRender = (uintptr_t)&Hook_CreatureControlRender_Naked;
        VirtualProtect((void*)vtableCreatureRender, 4, oldProtect, &oldProtect);

        // Hook ContextMenu item addition at 0x453283 (where "Set Outfit" is added)
        HookCall(g_clientBaseAddr + 0x53283, (uintptr_t)&Hook_ContextMenu_AddOutfit_Naked);

        // Hook PlayerContextMenu::OnEvent in vtable 0x5BD950 index 28 (offset +0x70)
        uintptr_t vtablePlayerMenu = g_clientBaseAddr + 0x1BD950 + (28 * 4);
        VirtualProtect((void*)vtablePlayerMenu, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
        *(uintptr_t*)vtablePlayerMenu = (uintptr_t)&Hook_ContextMenu_Execute_Naked;
        VirtualProtect((void*)vtablePlayerMenu, 4, oldProtect, &oldProtect);

        // Hook PlayerContextMenu2::OnEvent in vtable 0x5BD9F0 index 28 (offset +0x70)
        uintptr_t vtablePlayerMenu2 = g_clientBaseAddr + 0x1BD9F0 + (28 * 4);
        VirtualProtect((void*)vtablePlayerMenu2, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
        *(uintptr_t*)vtablePlayerMenu2 = (uintptr_t)&Hook_ContextMenu2_Execute_Naked;
        VirtualProtect((void*)vtablePlayerMenu2, 4, oldProtect, &oldProtect);
    }
}
