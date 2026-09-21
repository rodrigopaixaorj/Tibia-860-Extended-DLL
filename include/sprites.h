/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __SPRITES_H__
#define __SPRITES_H__

#include <cstdio>
#include <cstdint>
#include <windows.h>

class Sprites {
public:
    Sprites(const char* filename, const char* readType);
    virtual ~Sprites();

    void sprSeek(unsigned long position);
    void sprRead(void* buf, unsigned long size);
    unsigned char sprGetC();
    bool sprLoad() const { return m_loaded; }

private:
    char* m_data = nullptr;
    FILE* m_file = nullptr;
    unsigned long m_offset = 0;
    bool m_isCached = false;
    bool m_loaded = false;
};

extern Sprites* g_spritesFile;
extern bool g_sprHasAlpha;
extern uint32_t g_numSprites;

uint32_t HookPointers();
uint32_t HookSignature();
uint32_t HookNumSprites();
void HookLoadSprite(uint32_t sprite, unsigned char* pixels);
unsigned char* LoadSpriteAlpha(uint32_t sprite);

#endif // __SPRITES_H__
