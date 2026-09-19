/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "main.h"
#include "ext_engines.h"
#include "sprites.h"

ExtendedEngine* g_transEngine = nullptr;

void ExtendedEngineDX9::LoadSprite(int surface, int x, int y, int w, int h, void* data) {
    unsigned char* rgbaPixels = LoadSpriteAlpha((uint32_t)data);
    if (!rgbaPixels) return;

    RECT d3drect;
    d3drect.left = x;
    d3drect.right = x + w;
    d3drect.top = y;
    d3drect.bottom = y + h;

    uint32_t engineAddr = GetEngineAddr ? GetEngineAddr() : 0;
    if (!engineAddr) {
        free(rgbaPixels);
        return;
    }

    IDirect3DTexture9* texture = (IDirect3DTexture9*)(*(uint32_t*)(engineAddr + 0x200 + surface * 4));
    if (!texture) {
        free(rgbaPixels);
        return;
    }

    D3DLOCKED_RECT rect;
    HRESULT hr = texture->LockRect(0, &rect, &d3drect, D3DLOCK_NOSYSLOCK);
    if (FAILED(hr)) {
        free(rgbaPixels);
        return;
    }

    unsigned char* tBits = (unsigned char*)rect.pBits;
    uint32_t readData = 0, pixel = 0;

    for (int j = 0; j < h; ++j) {
        for (int k = 0; k < w; ++k) {
            pixel = j * rect.Pitch + k * 4;
            tBits[pixel + 0] = rgbaPixels[readData + 2]; // BGRA format
            tBits[pixel + 1] = rgbaPixels[readData + 1];
            tBits[pixel + 2] = rgbaPixels[readData + 0];
            tBits[pixel + 3] = rgbaPixels[readData + 3];
            readData += 4;
        }
    }

    texture->UnlockRect(0);
    free(rgbaPixels);
}

void ExtendedEngineOGL::LoadSprite(int surface, int x, int y, int w, int h, void* data) {
    unsigned char* rgbaPixels = LoadSpriteAlpha((uint32_t)data);
    if (!rgbaPixels) return;

    uint32_t engineAddr = GetEngineAddr ? GetEngineAddr() : 0;
    if (!engineAddr) {
        free(rgbaPixels);
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, *(GLuint*)(engineAddr + 0x1D4 + surface * 32));
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
    free(rgbaPixels);
}

void ExtendedEngineOGL::enableAlpha() {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

void ExtendedEngineOGL::disableAlpha() {
    glDisable(GL_BLEND);
}
