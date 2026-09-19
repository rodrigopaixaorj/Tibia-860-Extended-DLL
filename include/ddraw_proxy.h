/*
  Tibia World RPG Client - Extended DLL
  Copyright (C) 2020-2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __DDRAW_PROXY_H__
#define __DDRAW_PROXY_H__

#include <windows.h>
#include <ddraw.h>

bool InitDirectDrawProxy();
void FreeDirectDrawProxy();

// DirectDraw proxy exports declared with WINAPI convention
extern "C" {
    HRESULT WINAPI FakeDirectDrawCreate(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter);
    HRESULT WINAPI FakeDirectDrawCreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter);
    HRESULT WINAPI FakeDirectDrawCreateEx(GUID* lpGUID, LPVOID* lplpDD, REFIID iid, IUnknown* pUnkOuter);
    HRESULT WINAPI FakeDirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext);
    HRESULT WINAPI FakeDirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);
    HRESULT WINAPI FakeDirectDrawEnumerateExW(LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags);
    HRESULT WINAPI FakeDirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext);
    HRESULT WINAPI FakeDllCanUnloadNow();
    HRESULT WINAPI FakeDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
}

#endif // __DDRAW_PROXY_H__
