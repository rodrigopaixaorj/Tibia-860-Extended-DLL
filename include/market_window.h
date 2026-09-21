/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __MARKET_WINDOW_H__
#define __MARKET_WINDOW_H__

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include "ui_market.h"

class InGameMarket {
public:
    enum MarketTab {
        TAB_OFFERS = 0,
        TAB_DETAILS = 1,
        TAB_MY_OFFERS = 2,
        TAB_OFFER_HISTORY = 3
    };

    static void Init();
    static void Open();
    static void Close();
    static bool IsOpen();
    static void Refresh();

    // In-game Rendering
    static void Render(int nSurface, int screenW, int screenH);

    // In-game Input Handling (from TibiaClient Window Subclass)
    static bool OnLMouseDown(int x, int y);
    static bool OnLMouseUp(int x, int y);
    static bool OnMouseMove(int x, int y);
    static bool OnWheel(int x, int y, bool wheelUp);
    static bool OnKeyDown(int vk);
    static bool OnChar(char c);

    static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    static void DrawSunkenBox(int nSurface, int x, int y, int w, int h, int bgR = 48, int bgG = 48, int bgB = 48);
    static void DrawButton(int nSurface, int x, int y, int w, int h, const char* text, bool pressed = false, bool disabled = false);
    static void DrawRadioButton(int nSurface, int x, int y, int w, int h, const char* text, bool checked = false);
    static void DrawCheckBox(int nSurface, int x, int y, int w, int h, const char* text, bool checked = false);
    static void DrawScrollBar(int nSurface, int x, int y, int w, int h, int pos, int maxPos);
    static void DrawHScrollBar(int nSurface, int x, int y, int w, int h, int pos, int maxPos);

    static void FormatNumberWithCommas(uint64_t num, char* outBuf, size_t outSize);

    // State
    static bool s_isOpen;
    static HWND s_hTibiaWnd;
    static WNDPROC s_oldWndProc;

    static MarketTab s_currentTab;
    static int s_selectedCategory;
    static int s_selectedItemIndex;
    static int s_selectedSellOfferIndex;
    static int s_selectedBuyOfferIndex;
    static int s_selectedMySellOfferIndex;
    static int s_selectedMyBuyOfferIndex;

    // Filters
    static bool s_filterLevel;
    static bool s_filterVoc;
    static bool s_filter1H;
    static bool s_filter2H;
    static bool s_showLockerOnly;

    // Item list scroll
    static int s_itemListScroll;
    static int s_categoryListScroll;
    static int s_sellOfferScroll;
    static int s_buyOfferScroll;
    static int s_mySellOfferScroll;
    static int s_myBuyOfferScroll;
    static int s_detailsScroll;
    static int s_statsScroll;

    // Offer creation state
    static uint8_t s_createOfferType; // 1 = Sell, 0 = Buy
    static uint32_t s_createAmount;
    static uint32_t s_createMaxAmount;
    static std::string s_createPriceStr;
    static bool s_isAnonymous;

    // Textbox focus
    enum FocusedBox {
        FOCUS_NONE = 0,
        FOCUS_SEARCH = 1,
        FOCUS_PRICE = 2
    };
    static FocusedBox s_focusedBox;
    static std::string s_searchText;

    // Amount scrollbars in offer lists
    static uint32_t s_acceptSellAmount;
    static uint32_t s_acceptBuyAmount;

    // Cached item display list
    struct DisplayItem {
        uint16_t thingId;
        std::string name;
        uint32_t count;
    };
    static std::vector<DisplayItem> s_displayItems;
    static void RefreshDisplayItems();
};

#endif // __MARKET_WINDOW_H__
