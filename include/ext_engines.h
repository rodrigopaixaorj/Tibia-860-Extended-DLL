/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __EXT_ENGINES_H__
#define __EXT_ENGINES_H__

#define OGL_RENDER 1
#define DX9_RENDER 2

class ExtendedEngine {
public:
    virtual ~ExtendedEngine() {}
    virtual int getRenderId() = 0;
    virtual void LoadSprite(int surface, int x, int y, int w, int h, void* data) = 0;
    virtual void enableAlpha() = 0;
    virtual void disableAlpha() = 0;

protected:
    ExtendedEngine() {}
};

class ExtendedEngineDX9 : public ExtendedEngine {
public:
    ExtendedEngineDX9() {}
    virtual ~ExtendedEngineDX9() {}

    virtual int getRenderId() override { return DX9_RENDER; }
    virtual void LoadSprite(int surface, int x, int y, int w, int h, void* data) override;
    virtual void enableAlpha() override {}
    virtual void disableAlpha() override {}
};

class ExtendedEngineOGL : public ExtendedEngine {
public:
    ExtendedEngineOGL() {}
    virtual ~ExtendedEngineOGL() {}

    virtual int getRenderId() override { return OGL_RENDER; }
    virtual void LoadSprite(int surface, int x, int y, int w, int h, void* data) override;
    virtual void enableAlpha() override;
    virtual void disableAlpha() override;
};

extern ExtendedEngine* g_transEngine;

#endif // __EXT_ENGINES_H__
