/*
  Tibia World RPG Client - Extended DLL
  Copyright (C) 2020-2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "ddraw_proxy.h"

static HMODULE g_hOriginalDDraw = NULL;

typedef HRESULT(WINAPI* t_DirectDrawCreate)(GUID*, LPDIRECTDRAW*, IUnknown*);
typedef HRESULT(WINAPI* t_DirectDrawCreateClipper)(DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*);
typedef HRESULT(WINAPI* t_DirectDrawCreateEx)(GUID*, LPVOID*, REFIID, IUnknown*);
typedef HRESULT(WINAPI* t_DirectDrawEnumerateA)(LPDDENUMCALLBACKA, LPVOID);
typedef HRESULT(WINAPI* t_DirectDrawEnumerateExA)(LPDDENUMCALLBACKEXA, LPVOID, DWORD);
typedef HRESULT(WINAPI* t_DirectDrawEnumerateExW)(LPDDENUMCALLBACKEXW, LPVOID, DWORD);
typedef HRESULT(WINAPI* t_DirectDrawEnumerateW)(LPDDENUMCALLBACKW, LPVOID);
typedef HRESULT(WINAPI* t_DllCanUnloadNow)();
typedef HRESULT(WINAPI* t_DllGetClassObject)(REFCLSID, REFIID, LPVOID*);

static t_DirectDrawCreate        o_DirectDrawCreate = NULL;
static t_DirectDrawCreateClipper o_DirectDrawCreateClipper = NULL;
static t_DirectDrawCreateEx      o_DirectDrawCreateEx = NULL;
static t_DirectDrawEnumerateA    o_DirectDrawEnumerateA = NULL;
static t_DirectDrawEnumerateExA  o_DirectDrawEnumerateExA = NULL;
static t_DirectDrawEnumerateExW  o_DirectDrawEnumerateExW = NULL;
static t_DirectDrawEnumerateW    o_DirectDrawEnumerateW = NULL;
static t_DllCanUnloadNow         o_DllCanUnloadNow = NULL;
static t_DllGetClassObject      o_DllGetClassObject = NULL;

bool InitDirectDrawProxy() {
    if (g_hOriginalDDraw) return true;

    char systemPath[MAX_PATH];
    GetSystemDirectoryA(systemPath, MAX_PATH);
    strcat_s(systemPath, MAX_PATH, "\\ddraw.dll");

    g_hOriginalDDraw = LoadLibraryA(systemPath);
    if (!g_hOriginalDDraw) {
        return false;
    }

    o_DirectDrawCreate        = (t_DirectDrawCreate)GetProcAddress(g_hOriginalDDraw, "DirectDrawCreate");
    o_DirectDrawCreateClipper = (t_DirectDrawCreateClipper)GetProcAddress(g_hOriginalDDraw, "DirectDrawCreateClipper");
    o_DirectDrawCreateEx      = (t_DirectDrawCreateEx)GetProcAddress(g_hOriginalDDraw, "DirectDrawCreateEx");
    o_DirectDrawEnumerateA    = (t_DirectDrawEnumerateA)GetProcAddress(g_hOriginalDDraw, "DirectDrawEnumerateA");
    o_DirectDrawEnumerateExA  = (t_DirectDrawEnumerateExA)GetProcAddress(g_hOriginalDDraw, "DirectDrawEnumerateExA");
    o_DirectDrawEnumerateExW  = (t_DirectDrawEnumerateExW)GetProcAddress(g_hOriginalDDraw, "DirectDrawEnumerateExW");
    o_DirectDrawEnumerateW    = (t_DirectDrawEnumerateW)GetProcAddress(g_hOriginalDDraw, "DirectDrawEnumerateW");
    o_DllCanUnloadNow         = (t_DllCanUnloadNow)GetProcAddress(g_hOriginalDDraw, "DllCanUnloadNow");
    o_DllGetClassObject      = (t_DllGetClassObject)GetProcAddress(g_hOriginalDDraw, "DllGetClassObject");

    return true;
}

void FreeDirectDrawProxy() {
    if (g_hOriginalDDraw) {
        FreeLibrary(g_hOriginalDDraw);
        g_hOriginalDDraw = NULL;
    }
}

extern "C" {
    HRESULT WINAPI FakeDirectDrawCreate(GUID* lpGUID, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter) {
        if (!o_DirectDrawCreate) InitDirectDrawProxy();
        return o_DirectDrawCreate ? o_DirectDrawCreate(lpGUID, lplpDD, pUnkOuter) : E_FAIL;
    }

    HRESULT WINAPI FakeDirectDrawCreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter) {
        if (!o_DirectDrawCreateClipper) InitDirectDrawProxy();
        return o_DirectDrawCreateClipper ? o_DirectDrawCreateClipper(dwFlags, lplpDDClipper, pUnkOuter) : E_FAIL;
    }

    HRESULT WINAPI FakeDirectDrawCreateEx(GUID* lpGUID, LPVOID* lplpDD, REFIID iid, IUnknown* pUnkOuter) {
        if (!o_DirectDrawCreateEx) InitDirectDrawProxy();
        return o_DirectDrawCreateEx ? o_DirectDrawCreateEx(lpGUID, lplpDD, iid, pUnkOuter) : E_FAIL;
    }

    HRESULT WINAPI FakeDirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext) {
        if (!o_DirectDrawEnumerateA) InitDirectDrawProxy();
        return o_DirectDrawEnumerateA ? o_DirectDrawEnumerateA(lpCallback, lpContext) : E_FAIL;
    }

    HRESULT WINAPI FakeDirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags) {
        if (!o_DirectDrawEnumerateExA) InitDirectDrawProxy();
        return o_DirectDrawEnumerateExA ? o_DirectDrawEnumerateExA(lpCallback, lpContext, dwFlags) : E_FAIL;
    }

    HRESULT WINAPI FakeDirectDrawEnumerateExW(LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags) {
        if (!o_DirectDrawEnumerateExW) InitDirectDrawProxy();
        return o_DirectDrawEnumerateExW ? o_DirectDrawEnumerateExW(lpCallback, lpContext, dwFlags) : E_FAIL;
    }

    HRESULT WINAPI FakeDirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext) {
        if (!o_DirectDrawEnumerateW) InitDirectDrawProxy();
        return o_DirectDrawEnumerateW ? o_DirectDrawEnumerateW(lpCallback, lpContext) : E_FAIL;
    }

    HRESULT WINAPI FakeDllCanUnloadNow() {
        if (!o_DllCanUnloadNow) InitDirectDrawProxy();
        return o_DllCanUnloadNow ? o_DllCanUnloadNow() : E_FAIL;
    }

    HRESULT WINAPI FakeDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
        if (!o_DllGetClassObject) InitDirectDrawProxy();
        return o_DllGetClassObject ? o_DllGetClassObject(rclsid, riid, ppv) : E_FAIL;
    }
}
