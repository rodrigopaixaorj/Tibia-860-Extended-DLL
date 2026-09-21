/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "sprites.h"
#include "main.h"
#include "config.h"
#include <vector>
#include <cstdlib>
#include <cstring>

Sprites* g_spritesFile = nullptr;
bool g_sprHasAlpha = false;
uint32_t g_numSprites = 0;

Sprites::Sprites(const char* filename, const char* readType) {
    m_loaded = false;
    m_isCached = false;
    m_offset = 0;
    m_data = nullptr;

    fopen_s(&m_file, filename, readType);
    if (!m_file) return;

    fseek(m_file, 0, SEEK_END);
    unsigned long fileSize = ftell(m_file);
    rewind(m_file);

    // If cache is enabled and file size is under 250MB, cache in RAM
    if (g_config.cacheSprites && fileSize > 0 && fileSize < (250 * 1024 * 1024)) {
        m_data = (char*)malloc(fileSize);
        if (m_data) {
            if (fread(m_data, 1, fileSize, m_file) == fileSize) {
                m_isCached = true;
                fclose(m_file);
                m_file = nullptr;
            } else {
                free(m_data);
                m_data = nullptr;
            }
        }
    }

    m_loaded = true;
}

Sprites::~Sprites() {
    if (m_data) {
        free(m_data);
        m_data = nullptr;
    }
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
}

void Sprites::sprSeek(unsigned long position) {
    m_offset = position;
    if (!m_isCached && m_file) {
        fseek(m_file, position, SEEK_SET);
    }
}

void Sprites::sprRead(void* buf, unsigned long size) {
    if (m_isCached && m_data) {
        memcpy(buf, m_data + m_offset, size);
        m_offset += size;
    } else if (m_file) {
        fread(buf, 1, size, m_file);
        m_offset += size;
    }
}

unsigned char Sprites::sprGetC() {
    unsigned char v = 0;
    sprRead(&v, 1);
    return v;
}

uint32_t HookSignature() {
    const char* sprPath = GetFileName ? GetFileName(4, false) : "Tibia.spr";
    FILE* f = nullptr;
    fopen_s(&f, sprPath, "rb");
    if (f) {
        uint32_t sig = 0;
        fread(&sig, 4, 1, f);
        fclose(f);
        return sig;
    }
    return 0x44545845; // Fallback 'EXTD'
}

uint32_t HookPointers() {
    uint32_t v = 0;
    if (g_spritesFile) {
        g_spritesFile->sprRead(&v, 4);
    }
    return v;
}

static void DetectSprFormat(uint32_t numSprites) {
    if (!g_spritesFile || numSprites == 0) return;

    for (uint32_t s = 1; s <= numSprites && s <= 50; ++s) {
        g_spritesFile->sprSeek((g_config.extendedSprites ? 8 : 6) + (s - 1) * 4);
        uint32_t offset = 0;
        g_spritesFile->sprRead(&offset, 4);
        if (offset == 0) continue;

        g_spritesFile->sprSeek(offset + 3); // Skip RGB color key
        uint16_t sprSize = 0;
        g_spritesFile->sprRead(&sprSize, 2);
        if (sprSize == 0 || sprSize > 8192) continue;

        std::vector<uint8_t> sprData(sprSize);
        g_spritesFile->sprRead(sprData.data(), sprSize);

        // Test 3-byte RGB
        size_t p3 = 0;
        bool ok3 = true;
        while (p3 < sprSize) {
            if (p3 + 2 > sprSize) { ok3 = false; break; }
            p3 += 2; // transparent count
            if (p3 >= sprSize) break;
            if (p3 + 2 > sprSize) { ok3 = false; break; }
            uint16_t colored = *(uint16_t*)&sprData[p3];
            p3 += 2;
            if (p3 + (size_t)colored * 3 > sprSize) { ok3 = false; break; }
            p3 += (size_t)colored * 3;
        }
        if (ok3 && p3 == sprSize) {
            g_sprHasAlpha = false;
            return;
        }

        // Test 4-byte RGBA
        size_t p4 = 0;
        bool ok4 = true;
        while (p4 < sprSize) {
            if (p4 + 2 > sprSize) { ok4 = false; break; }
            p4 += 2; // transparent count
            if (p4 >= sprSize) break;
            if (p4 + 2 > sprSize) { ok4 = false; break; }
            uint16_t colored = *(uint16_t*)&sprData[p4];
            p4 += 2;
            if (p4 + (size_t)colored * 4 > sprSize) { ok4 = false; break; }
            p4 += (size_t)colored * 4;
        }
        if (ok4 && p4 == sprSize) {
            g_sprHasAlpha = true;
            return;
        }
    }
}

uint32_t HookNumSprites() {
    uint32_t numSprites = 0;

    const char* sprPath = GetFileName ? GetFileName(4, false) : "Tibia.spr";
    g_spritesFile = new Sprites(sprPath, "rb");

    if (g_spritesFile && g_spritesFile->sprLoad()) {
        g_spritesFile->sprSeek(4);
        if (g_config.extendedSprites) {
            g_spritesFile->sprRead(&numSprites, 4);
        } else {
            uint16_t u16Read = 0;
            g_spritesFile->sprRead(&u16Read, 2);
            numSprites = u16Read;
        }
        g_numSprites = numSprites;

        // Auto-detect whether sprites in Tibia.spr use 3-byte (RGB) or 4-byte (RGBA) pixels
        DetectSprFormat(numSprites);

        // Restore file pointer to the start of the sprite offset table
        g_spritesFile->sprSeek(g_config.extendedSprites ? 8 : 6);
    } else {
        MessageBoxA(NULL, "Cannot read client .spr file.", PROJECT_NAME, MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    return numSprites;
}

unsigned char* LoadSpriteAlpha(uint32_t sprite) {
    unsigned char* pixels = (unsigned char*)calloc(1, 4096);
    if (!pixels) return nullptr;

    if (sprite == 0 || !g_clientPointerTransPixels) {
        return pixels;
    }

    uint8_t* spriteTable = *(uint8_t**)g_clientPointerTransPixels;
    if (!spriteTable || !g_spritesFile) {
        return pixels;
    }

    if (g_numSprites > 0 && sprite > g_numSprites) {
        return pixels;
    }

    uint32_t* transPixels = (uint32_t*)(spriteTable + (sprite - 1) * 0x10);
    uint32_t pointer = transPixels[0];
    if (pointer == 0) {
        return pixels;
    }

    g_spritesFile->sprSeek(pointer);

    unsigned char ckR = g_spritesFile->sprGetC();
    unsigned char ckG = g_spritesFile->sprGetC();
    unsigned char ckB = g_spritesFile->sprGetC();

    uint16_t sprSize = 0;
    g_spritesFile->sprRead(&sprSize, 2);
    if (sprSize == 0) {
        return pixels;
    }

    uint32_t readData = 0;
    int pixelIndex = 0;

    while (readData < sprSize && pixelIndex < 1024) {
        if (readData + 2 > sprSize) break;
        uint16_t numTransparent = 0;
        g_spritesFile->sprRead(&numTransparent, 2);
        readData += 2;
        pixelIndex += numTransparent;

        if (readData >= sprSize || pixelIndex >= 1024) break;

        if (readData + 2 > sprSize) break;
        uint16_t numColored = 0;
        g_spritesFile->sprRead(&numColored, 2);
        readData += 2;

        for (int i = 0; i < numColored; ++i) {
            unsigned char r = g_spritesFile->sprGetC();
            unsigned char g = g_spritesFile->sprGetC();
            unsigned char b = g_spritesFile->sprGetC();
            readData += 3;
            unsigned char a = 0xFF;
            if (g_sprHasAlpha) {
                a = g_spritesFile->sprGetC();
                readData++;
            } else {
                if (r == ckR && g == ckG && b == ckB) {
                    a = 0x00;
                }
            }
            if (pixelIndex < 1024) {
                pixels[pixelIndex * 4 + 0] = r;
                pixels[pixelIndex * 4 + 1] = g;
                pixels[pixelIndex * 4 + 2] = b;
                pixels[pixelIndex * 4 + 3] = a;
            }
            pixelIndex++;
        }
    }

    return pixels;
}

void HookLoadSprite(uint32_t sprite, unsigned char* pixels) {
    if (sprite == 0 || !g_clientPointerTransPixels) {
        return;
    }

    uint8_t* spriteTable = *(uint8_t**)g_clientPointerTransPixels;
    if (!spriteTable) {
        return;
    }

    if (g_numSprites > 0 && sprite > g_numSprites) {
        return;
    }

    uint32_t* transPixels = (uint32_t*)(spriteTable + (sprite - 1) * 0x10);
    if (transPixels[0] == 0 || !g_spritesFile) {
        transPixels[1] = 0xFF;
        transPixels[2] = 0x00;
        transPixels[3] = 0xFF;
        for (int i = 0; i < 1024; ++i) {
            pixels[i * 3 + 0] = 0xFF;
            pixels[i * 3 + 1] = 0x00;
            pixels[i * 3 + 2] = 0xFF;
        }
        return;
    }

    g_spritesFile->sprSeek(transPixels[0]);

    unsigned char R = g_spritesFile->sprGetC();
    unsigned char G = g_spritesFile->sprGetC();
    unsigned char B = g_spritesFile->sprGetC();
    transPixels[1] = static_cast<uint32_t>(R);
    transPixels[2] = static_cast<uint32_t>(G);
    transPixels[3] = static_cast<uint32_t>(B);

    for (int i = 0; i < 1024; ++i) {
        pixels[i * 3 + 0] = R;
        pixels[i * 3 + 1] = G;
        pixels[i * 3 + 2] = B;
    }

    uint16_t sprSize = 0;
    g_spritesFile->sprRead(&sprSize, 2);
    if (sprSize == 0) return;

    uint32_t readData = 0;
    int pixelIndex = 0;

    while (readData < sprSize && pixelIndex < 1024) {
        if (readData + 2 > sprSize) break;
        uint16_t numTransparent = 0;
        g_spritesFile->sprRead(&numTransparent, 2);
        readData += 2;
        pixelIndex += numTransparent;

        if (readData >= sprSize || pixelIndex >= 1024) break;

        if (readData + 2 > sprSize) break;
        uint16_t numColored = 0;
        g_spritesFile->sprRead(&numColored, 2);
        readData += 2;

        for (int i = 0; i < numColored; ++i) {
            unsigned char r = g_spritesFile->sprGetC();
            unsigned char g = g_spritesFile->sprGetC();
            unsigned char b = g_spritesFile->sprGetC();
            readData += 3;
            if (g_sprHasAlpha) {
                g_spritesFile->sprGetC();
                readData++;
            }
            if (pixelIndex < 1024) {
                pixels[pixelIndex * 3 + 0] = r;
                pixels[pixelIndex * 3 + 1] = g;
                pixels[pixelIndex * 3 + 2] = b;
            }
            pixelIndex++;
        }
    }
}
