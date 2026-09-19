/*
  Tibia World RPG Client - Extended DLL
  Copyright (C) 2020-2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __HOOK_H__
#define __HOOK_H__

#include <windows.h>
#include <cstdint>

void HookJMP(uintptr_t dwAddress, uintptr_t dwFunction);
void HookCall(uintptr_t dwAddress, uintptr_t dwFunction);
void HookCallN(uintptr_t dwAddress, uintptr_t dwFunction);
void OverWriteByte(uintptr_t dwAddress, uint8_t byteValue);
void OverWriteWord(uintptr_t dwAddress, uint16_t wordValue);
void OverWrite(uintptr_t dwAddress, uint32_t dwordValue);
void Nop(uintptr_t dwAddress, size_t length);
void HookMemory(uintptr_t dwAddress, const void* pData, size_t length);

#endif // __HOOK_H__
