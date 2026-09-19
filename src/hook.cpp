/*
  Tibia World RPG Client - Extended DLL
  Copyright (C) 2020-2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "hook.h"

void HookJMP(uintptr_t dwAddress, uintptr_t dwFunction) {
    DWORD dwOldProtect;
    VirtualProtect((LPVOID)dwAddress, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    
    uintptr_t dwNewCall = dwFunction - dwAddress - 5;
    *(uint8_t*)dwAddress = 0xE9; // JMP opcode
    *(uint32_t*)(dwAddress + 1) = static_cast<uint32_t>(dwNewCall);
    
    VirtualProtect((LPVOID)dwAddress, 5, dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddress, 5);
}

void HookCall(uintptr_t dwAddress, uintptr_t dwFunction) {
    DWORD dwOldProtect;
    VirtualProtect((LPVOID)dwAddress, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    
    uintptr_t dwNewCall = dwFunction - dwAddress - 5;
    *(uint8_t*)dwAddress = 0xE8; // CALL opcode
    *(uint32_t*)(dwAddress + 1) = static_cast<uint32_t>(dwNewCall);
    
    VirtualProtect((LPVOID)dwAddress, 5, dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddress, 5);
}

void HookCallN(uintptr_t dwAddress, uintptr_t dwFunction) {
    DWORD dwOldProtect;
    VirtualProtect((LPVOID)dwAddress, 6, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    
    uintptr_t dwNewCall = dwFunction - dwAddress - 5;
    *(uint8_t*)dwAddress = 0xE8;
    *(uint32_t*)(dwAddress + 1) = static_cast<uint32_t>(dwNewCall);
    *(uint8_t*)(dwAddress + 5) = 0x90; // NOP trailing byte
    
    VirtualProtect((LPVOID)dwAddress, 6, dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddress, 6);
}

void OverWriteByte(uintptr_t dwAddress, uint8_t byteValue) {
    DWORD dwOldProtect;
    VirtualProtect((LPVOID)dwAddress, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    *(uint8_t*)dwAddress = byteValue;
    VirtualProtect((LPVOID)dwAddress, 1, dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddress, 1);
}

void OverWriteWord(uintptr_t dwAddress, uint16_t wordValue) {
    DWORD dwOldProtect;
    VirtualProtect((LPVOID)dwAddress, 2, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    *(uint16_t*)dwAddress = wordValue;
    VirtualProtect((LPVOID)dwAddress, 2, dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddress, 2);
}

void OverWrite(uintptr_t dwAddress, uint32_t dwordValue) {
    DWORD dwOldProtect;
    VirtualProtect((LPVOID)dwAddress, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    *(uint32_t*)dwAddress = dwordValue;
    VirtualProtect((LPVOID)dwAddress, 4, dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddress, 4);
}

void Nop(uintptr_t dwAddress, size_t length) {
    DWORD dwOldProtect;
    VirtualProtect((LPVOID)dwAddress, length, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    memset((void*)dwAddress, 0x90, length);
    VirtualProtect((LPVOID)dwAddress, length, dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddress, length);
}

void HookMemory(uintptr_t dwAddress, const void* pData, size_t length) {
    DWORD dwOldProtect;
    VirtualProtect((LPVOID)dwAddress, length, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    memcpy((void*)dwAddress, pData, length);
    VirtualProtect((LPVOID)dwAddress, length, dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID)dwAddress, length);
}
