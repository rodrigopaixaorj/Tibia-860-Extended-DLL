/*
  Tibia World RPG Client - Extended DLL
  Copyright (C) 2020-2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "sprites.h"
#include "main.h"
#include "config.h"

Sprites* g_spritesFile = nullptr;

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
    } else {
        MessageBoxA(NULL, "Cannot read client .spr file.", PROJECT_NAME, MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    return numSprites;
}

unsigned char* LoadSpriteAlpha(uint32_t sprite) {
    unsigned char* pixels = (unsigned char*)malloc(4096);
    if (!pixels) return nullptr;

    for (int i = 0; i < 1024; ++i) {
        pixels[i * 4 + 3] = 0x00;
    }

    uint32_t pointer = *(uint32_t*)(g_clientPointerTransPixels - 0x10 + sprite * 0x10);
    if (pointer == 0 || !g_spritesFile) {
        return pixels;
    }

    g_spritesFile->sprSeek(pointer);

    // Ignore color key
    g_spritesFile->sprGetC();
    g_spritesFile->sprGetC();
    g_spritesFile->sprGetC();

    uint16_t sprSize = 0;
    g_spritesFile->sprRead(&sprSize, 2);
    if (sprSize == 0) {
        return pixels;
    }

    uint32_t writeData = 0, readData = 0;
    uint16_t numPix = 0;
    bool state = false;

    while (readData < sprSize) {
        g_spritesFile->sprRead(&numPix, 2);
        readData += 2;
        if (state) {
            for (int i = 0; i < numPix && writeData + 3 < 4096; ++i) {
                pixels[writeData++] = g_spritesFile->sprGetC();
                pixels[writeData++] = g_spritesFile->sprGetC();
                pixels[writeData++] = g_spritesFile->sprGetC();
                readData += 3;
                if (g_config.alphaTransparency) {
                    pixels[writeData++] = g_spritesFile->sprGetC();
                    readData++;
                } else {
                    pixels[writeData++] = 0xFF;
                }
            }
            state = false;
        } else {
            writeData += numPix * 4;
            state = true;
        }
    }

    return pixels;
}

void HookLoadSprite(uint32_t sprite, unsigned char* pixels) {
    uint32_t* transPixels = (uint32_t*)(g_clientPointerTransPixels - 0x10 + sprite * 0x10);
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

    uint32_t writeData = 0, readData = 0;
    uint16_t numPix = 0;
    bool state = false;

    while (readData < sprSize && writeData < 3072) {
        g_spritesFile->sprRead(&numPix, 2);
        readData += 2;
        if (state) {
            for (int i = 0; i < numPix && writeData + 2 < 3072; ++i) {
                pixels[writeData++] = g_spritesFile->sprGetC();
                pixels[writeData++] = g_spritesFile->sprGetC();
                pixels[writeData++] = g_spritesFile->sprGetC();
                readData += 3;
                if (g_config.alphaTransparency) {
                    g_spritesFile->sprGetC();
                    readData++;
                }
            }
            state = false;
        } else {
            writeData += numPix * 3;
            state = true;
        }
    }
}
