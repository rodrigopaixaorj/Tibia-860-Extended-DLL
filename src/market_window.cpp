/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "market_window.h"
#include "main.h"
#include "config.h"
#include "dat_reader.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <ctime>

// Static member definitions
bool InGameMarket::s_isOpen = false;
HWND InGameMarket::s_hTibiaWnd = NULL;
WNDPROC InGameMarket::s_oldWndProc = NULL;

InGameMarket::MarketTab InGameMarket::s_currentTab = InGameMarket::TAB_OFFERS;
int InGameMarket::s_selectedCategory = 0;
int InGameMarket::s_selectedItemIndex = -1;
int InGameMarket::s_selectedSellOfferIndex = -1;
int InGameMarket::s_selectedBuyOfferIndex = -1;
int InGameMarket::s_selectedMySellOfferIndex = -1;
int InGameMarket::s_selectedMyBuyOfferIndex = -1;

bool InGameMarket::s_filterLevel = false;
bool InGameMarket::s_filterVoc = false;
bool InGameMarket::s_filter1H = false;
bool InGameMarket::s_filter2H = false;
bool InGameMarket::s_showLockerOnly = false;

int InGameMarket::s_itemListScroll = 0;
int InGameMarket::s_categoryListScroll = 0;
int InGameMarket::s_sellOfferScroll = 0;
int InGameMarket::s_buyOfferScroll = 0;
int InGameMarket::s_mySellOfferScroll = 0;
int InGameMarket::s_myBuyOfferScroll = 0;
int InGameMarket::s_detailsScroll = 0;
int InGameMarket::s_statsScroll = 0;

uint8_t InGameMarket::s_createOfferType = 1; // 1 = Sell, 0 = Buy
uint32_t InGameMarket::s_createAmount = 1;
uint32_t InGameMarket::s_createMaxAmount = 100;
std::string InGameMarket::s_createPriceStr = "";
bool InGameMarket::s_isAnonymous = false;

InGameMarket::FocusedBox InGameMarket::s_focusedBox = InGameMarket::FOCUS_NONE;
std::string InGameMarket::s_searchText = "";

uint32_t InGameMarket::s_acceptSellAmount = 1;
uint32_t InGameMarket::s_acceptBuyAmount = 1;

std::vector<InGameMarket::DisplayItem> InGameMarket::s_displayItems;

static const char* g_marketCategories[23] = {
    "Armors", "Amulets", "Boots", "Containers",
    "Decoration", "Food", "Helmets and Hats", "Legs", "Others",
    "Potions", "Rings", "Runes", "Shields", "Tibia Coins", "Tools",
    "Valuables", "Weapons: Ammo", "Weapons: Axes", "Weapons: Clubs",
    "Weapons: Distance", "Weapons: Swords", "Weapons: Wands", "Weapons: All"
};

static inline void DrawRectNative(int nSurface, int x, int y, int w, int h, int r, int g, int b) {
    uint32_t engineAddr = GetEngineAddr ? GetEngineAddr() : 0;
    if (engineAddr) {
        uint32_t drawRectAddr = *(DWORD*)(*(DWORD*)(engineAddr) + 0x14);
        if (drawRectAddr) {
            ((void(__fastcall*)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))drawRectAddr)(
                engineAddr, 0, nSurface, x, y, w, h, r, g, b);
        }
    }
}

void InGameMarket::FormatNumberWithCommas(uint64_t num, char* outBuf, size_t outSize) {
    char temp[64];
    sprintf_s(temp, sizeof(temp), "%llu", num);
    std::string s = temp;
    int insertPosition = (int)s.length() - 3;
    while (insertPosition > 0) {
        s.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    strncpy_s(outBuf, outSize, s.c_str(), outSize - 1);
}

void InGameMarket::DrawSunkenBox(int nSurface, int x, int y, int w, int h, int bgR, int bgG, int bgB) {
    DrawRectNative(nSurface, x, y, w, h, bgR, bgG, bgB);
    // Dark border (top/left)
    DrawRectNative(nSurface, x, y, w, 1, 28, 28, 28);
    DrawRectNative(nSurface, x, y, 1, h, 28, 28, 28);
    // Bright border (bottom/right)
    DrawRectNative(nSurface, x, y + h - 1, w, 1, 85, 85, 85);
    DrawRectNative(nSurface, x + w - 1, y, 1, h, 85, 85, 85);
}

void InGameMarket::DrawButton(int nSurface, int x, int y, int w, int h, const char* text, bool pressed, bool disabled) {
    if (disabled) {
        DrawRectNative(nSurface, x, y, w, h, 42, 42, 42);
        DrawRectNative(nSurface, x, y, w, 1, 55, 55, 55);
        DrawRectNative(nSurface, x, y, 1, h, 55, 55, 55);
        DrawRectNative(nSurface, x, y + h - 1, w, 1, 32, 32, 32);
        DrawRectNative(nSurface, x + w - 1, y, 1, h, 32, 32, 32);
        if (PrintText) PrintText(nSurface, x + w / 2, y + (h - 12) / 2, 2, 100, 100, 100, text, 2);
        return;
    }

    if (pressed) {
        DrawRectNative(nSurface, x, y, w, h, 46, 46, 46);
        DrawRectNative(nSurface, x, y, w, 1, 18, 18, 18);
        DrawRectNative(nSurface, x, y, 1, h, 18, 18, 18);
        DrawRectNative(nSurface, x, y + h - 1, w, 1, 95, 95, 95);
        DrawRectNative(nSurface, x + w - 1, y, 1, h, 95, 95, 95);
        if (PrintText) PrintText(nSurface, x + w / 2 + 1, y + (h - 12) / 2 + 1, 2, 255, 255, 255, text, 2);
    } else {
        DrawRectNative(nSurface, x, y, w, h, 64, 64, 64);
        DrawRectNative(nSurface, x, y, w, 1, 100, 100, 100);
        DrawRectNative(nSurface, x, y, 1, h, 100, 100, 100);
        DrawRectNative(nSurface, x, y + h - 1, w, 1, 22, 22, 22);
        DrawRectNative(nSurface, x + w - 1, y, 1, h, 22, 22, 22);
        if (PrintText) PrintText(nSurface, x + w / 2, y + (h - 12) / 2, 2, 225, 225, 225, text, 2);
    }
}

void InGameMarket::DrawRadioButton(int nSurface, int x, int y, int w, int h, const char* text, bool checked) {
    DrawButton(nSurface, x, y, w, h, text, checked, false);
}

void InGameMarket::DrawCheckBox(int nSurface, int x, int y, int, int h, const char* text, bool checked) {
    int boxSize = 12;
    int boxY = y + (h - boxSize) / 2;
    DrawSunkenBox(nSurface, x, boxY, boxSize, boxSize, 35, 35, 35);
    if (checked) {
        DrawRectNative(nSurface, x + 3, boxY + 3, boxSize - 6, boxSize - 6, 220, 220, 220);
    }
    if (PrintText) PrintText(nSurface, x + boxSize + 6, y + (h - 12) / 2, 2, 220, 220, 220, text, 1);
}

void InGameMarket::DrawScrollBar(int nSurface, int x, int y, int w, int h, int pos, int maxPos) {
    DrawSunkenBox(nSurface, x, y, w, h, 38, 38, 38);
    // Up arrow button
    DrawButton(nSurface, x, y, w, 12, "^", false);
    // Down arrow button
    DrawButton(nSurface, x, y + h - 12, w, 12, "v", false);
    // Thumb
    int trackH = h - 24;
    if (trackH > 10 && maxPos > 0) {
        int thumbH = max(10, trackH / (maxPos + 1));
        int thumbY = y + 12 + (trackH - thumbH) * pos / maxPos;
        DrawButton(nSurface, x, thumbY, w, thumbH, "", false);
    }
}

void InGameMarket::DrawHScrollBar(int nSurface, int x, int y, int w, int h, int pos, int maxPos) {
    DrawSunkenBox(nSurface, x, y, w, h, 38, 38, 38);
    // Left button
    DrawButton(nSurface, x, y, 12, h, "<", false);
    // Right button
    DrawButton(nSurface, x + w - 12, y, 12, h, ">", false);
    // Thumb
    int trackW = w - 24;
    if (trackW > 10 && maxPos > 0) {
        int thumbW = max(10, trackW / (maxPos + 1));
        int thumbX = x + 12 + (trackW - thumbW) * pos / maxPos;
        DrawButton(nSurface, thumbX, y, thumbW, h, "", false);
    }
}

void InGameMarket::RefreshDisplayItems() {
    s_displayItems.clear();
    const auto& depot = MarketSystem::get().getDepotItems();

    std::string searchLower = s_searchText;
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    for (const auto& kv : depot) {
        DisplayItem item;
        item.thingId = kv.first;
        item.count = kv.second;
        item.name = "Item #" + std::to_string(kv.first);

        if (!searchLower.empty()) {
            std::string nameLower = item.name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (nameLower.find(searchLower) == std::string::npos && std::to_string(item.thingId).find(searchLower) == std::string::npos) {
                continue;
            }
        }
        s_displayItems.push_back(item);
    }
}

void InGameMarket::Init() {
    if (!s_hTibiaWnd) {
        s_hTibiaWnd = FindWindowA("TibiaClient", NULL);
        if (s_hTibiaWnd) {
            s_oldWndProc = (WNDPROC)SetWindowLongPtrA(s_hTibiaWnd, GWLP_WNDPROC, (LONG_PTR)InGameMarket::SubclassProc);
        }
    }
}

void InGameMarket::Open() {
    Init();
    s_isOpen = true;
    s_currentTab = TAB_OFFERS;
    Refresh();
}

void InGameMarket::Close() {
    s_isOpen = false;
    MarketSystem::get().sendLeave();
}

bool InGameMarket::IsOpen() {
    return s_isOpen;
}

void InGameMarket::Refresh() {
    RefreshDisplayItems();
}

void InGameMarket::Render(int nSurface, int screenW, int screenH) {
    if (!s_isOpen) return;

    int winW = 750;
    int winH = 545;
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;

    if (winX < 0) winX = 0;
    if (winY < 0) winY = 0;

    // Window Base Background
    DrawSunkenBox(nSurface, winX, winY, winW, winH, 53, 53, 53);
    
    // Window Title Bar
    DrawRectNative(nSurface, winX + 2, winY + 2, winW - 4, 18, 40, 40, 40);
    DrawRectNative(nSurface, winX + 2, winY + 19, winW - 4, 1, 28, 28, 28);
    if (PrintText) {
        PrintText(nSurface, winX + 16, winY + 5, 2, 255, 255, 255, "Market", 1);
    }
    // Close button [X] at top-right
    DrawButton(nSurface, winX + winW - 22, winY + 3, 18, 14, "X", false);

    // =========================================================================
    // BOTTOM BAR (y = winY + 508)
    // =========================================================================
    // Separator line
    DrawRectNative(nSurface, winX + 14, winY + 504, winW - 28, 1, 28, 28, 28);
    DrawRectNative(nSurface, winX + 14, winY + 505, winW - 28, 1, 85, 85, 85);

    // Cash / Bank Gold Box
    DrawSunkenBox(nSurface, winX + 16, winY + 512, 106, 20, 38, 38, 38);
    char goldBuf[64];
    FormatNumberWithCommas(MarketSystem::get().getAccountBalance(), goldBuf, sizeof(goldBuf));
    if (PrintText) {
        PrintText(nSurface, winX + 116, winY + 516, 2, 255, 255, 255, goldBuf, 2);
    }

    // Tibia Coins Box
    DrawSunkenBox(nSurface, winX + 128, winY + 512, 106, 20, 38, 38, 38);
    char coinsBuf[64];
    FormatNumberWithCommas(MarketSystem::get().getCoins(), coinsBuf, sizeof(coinsBuf));
    if (PrintText) {
        PrintText(nSurface, winX + 228, winY + 516, 2, 255, 255, 255, coinsBuf, 2);
    }

    // [ Get Coins ] button
    DrawButton(nSurface, winX + 242, winY + 512, 84, 20, "Get Coins", false);

    // Navigation Buttons
    if (s_currentTab == TAB_OFFERS || s_currentTab == TAB_DETAILS) {
        DrawRadioButton(nSurface, winX + 360, winY + 512, 72, 20, "Offers", s_currentTab == TAB_OFFERS);
        DrawRadioButton(nSurface, winX + 436, winY + 512, 72, 20, "Details", s_currentTab == TAB_DETAILS);
        DrawRadioButton(nSurface, winX + 512, winY + 512, 80, 20, "My Offers", false);
        if (s_currentTab == TAB_OFFERS) {
            DrawButton(nSurface, winX + 652, winY + 512, 82, 20, "Close", false);
        } else {
            DrawButton(nSurface, winX + 652, winY + 512, 82, 20, "Market", false);
        }
    } else { // TAB_MY_OFFERS or TAB_OFFER_HISTORY
        DrawRadioButton(nSurface, winX + 440, winY + 512, 98, 20, "Current Offers", s_currentTab == TAB_MY_OFFERS);
        DrawRadioButton(nSurface, winX + 542, winY + 512, 98, 20, "Offer History", s_currentTab == TAB_OFFER_HISTORY);
        DrawButton(nSurface, winX + 652, winY + 512, 82, 20, "Market", false);
    }

    // =========================================================================
    // LEFT PANEL (x = winX + 16) - Rendered in TAB_OFFERS and TAB_DETAILS
    // =========================================================================
    if (s_currentTab == TAB_OFFERS || s_currentTab == TAB_DETAILS) {
        // Category List Box (x=winX+16, y=winY+28, w=160, h=76)
        DrawSunkenBox(nSurface, winX + 16, winY + 28, 160, 76, 45, 45, 45);
        for (int i = 0; i < 5 && (i + s_categoryListScroll) < 23; ++i) {
            int catIdx = i + s_categoryListScroll;
            int catY = winY + 30 + i * 15;
            if (catIdx == s_selectedCategory) {
                DrawRectNative(nSurface, winX + 18, catY, 142, 14, 90, 90, 90);
            }
            if (PrintText) {
                PrintText(nSurface, winX + 20, catY + 1, 1, 225, 225, 225, g_marketCategories[catIdx], 1);
            }
        }
        DrawScrollBar(nSurface, winX + 162, winY + 28, 14, 76, s_categoryListScroll, 18);

        // Filter Buttons (y=winY+110, h=20)
        DrawRadioButton(nSurface, winX + 16, winY + 110, 34, 20, "Level", s_filterLevel);
        DrawRadioButton(nSurface, winX + 58, winY + 110, 34, 20, "Voc.", s_filterVoc);
        DrawRadioButton(nSurface, winX + 100, winY + 110, 34, 20, "1H", s_filter1H);
        DrawRadioButton(nSurface, winX + 142, winY + 110, 34, 20, "2H", s_filter2H);

        // CheckBox "Show Locker only"
        DrawCheckBox(nSurface, winX + 16, winY + 134, 160, 18, "Show Locker only", s_showLockerOnly);

        // Label "Items:"
        if (PrintText) PrintText(nSurface, winX + 16, winY + 158, 2, 220, 220, 220, "Items:", 1);

        // Item List (x=winX+16, y=winY+172, w=160, h=276)
        DrawSunkenBox(nSurface, winX + 16, winY + 172, 160, 276, 45, 45, 45);
        int maxVisibleItems = 8;
        for (int i = 0; i < maxVisibleItems && (i + s_itemListScroll) < (int)s_displayItems.size(); ++i) {
            int itemIdx = i + s_itemListScroll;
            const auto& item = s_displayItems[itemIdx];
            int rowY = winY + 174 + i * 34;

            if (itemIdx == s_selectedItemIndex) {
                DrawRectNative(nSurface, winX + 18, rowY, 142, 34, 90, 90, 90);
            } else {
                DrawRectNative(nSurface, winX + 18, rowY, 142, 34, 52, 52, 52);
            }
            // Icon slot
            DrawSunkenBox(nSurface, winX + 20, rowY + 1, 32, 32, 36, 36, 36);

            // Item count & name
            char countBuf[16];
            sprintf_s(countBuf, sizeof(countBuf), "%u", item.count);
            if (PrintText) {
                PrintText(nSurface, winX + 50, rowY + 20, 1, 255, 255, 255, countBuf, 2);
                PrintText(nSurface, winX + 56, rowY + 10, 2, 220, 220, 220, item.name.c_str(), 1);
            }
        }
        int maxItemScroll = max(0, (int)s_displayItems.size() - maxVisibleItems);
        DrawScrollBar(nSurface, winX + 162, winY + 172, 14, 276, s_itemListScroll, maxItemScroll);

        // Preview slot of selected item (x=winX+16, y=winY+456, w=34, h=34)
        DrawSunkenBox(nSurface, winX + 16, winY + 456, 34, 34, 38, 38, 38);

        // Search Box (x=winX+56, y=winY+462, w=120, h=22)
        DrawSunkenBox(nSurface, winX + 56, winY + 462, 120, 22, 38, 38, 38);
        if (PrintText) {
            if (s_searchText.empty() && s_focusedBox != FOCUS_SEARCH) {
                PrintText(nSurface, winX + 62, winY + 467, 2, 120, 120, 120, "Type to search", 1);
            } else {
                PrintText(nSurface, winX + 62, winY + 467, 2, 255, 255, 255, s_searchText.c_str(), 1);
            }
        }
    }

    // =========================================================================
    // RIGHT PANEL - TAB_OFFERS
    // =========================================================================
    if (s_currentTab == TAB_OFFERS) {
        const auto& sellOffers = MarketSystem::get().getSellOffers();
        const auto& buyOffers = MarketSystem::get().getBuyOffers();

        // 1. Sell Offers Header
        if (PrintText) {
            PrintText(nSurface, winX + 186, winY + 28, 2, 220, 220, 220, "Sell Offers:", 1);
            PrintText(nSurface, winX + 270, winY + 28, 2, 220, 220, 220, "Amount:", 1);
            char amtStr[16];
            sprintf_s(amtStr, sizeof(amtStr), "%u", s_acceptSellAmount);
            PrintText(nSurface, winX + 345, winY + 28, 2, 255, 255, 255, amtStr, 2);
            PrintText(nSurface, winX + 580, winY + 28, 2, 220, 220, 220, "Total:", 1);
        }
        DrawHScrollBar(nSurface, winX + 355, winY + 28, 140, 12, s_acceptSellAmount, 100);
        DrawButton(nSurface, winX + 685, winY + 24, 48, 20, "Accept", false, sellOffers.empty());

        // Sell Offers Table (x=winX+186, y=winY+48, w=548, h=147)
        DrawSunkenBox(nSurface, winX + 186, winY + 48, 548, 147, 45, 45, 45);
        DrawRectNative(nSurface, winX + 188, winY + 50, 530, 16, 36, 36, 36);
        if (PrintText) {
            PrintText(nSurface, winX + 192, winY + 53, 1, 200, 200, 200, "Name", 1);
            PrintText(nSurface, winX + 340, winY + 53, 1, 200, 200, 200, "Amount", 2);
            PrintText(nSurface, winX + 420, winY + 53, 1, 200, 200, 200, "Piece Price", 2);
            PrintText(nSurface, winX + 510, winY + 53, 1, 200, 200, 200, "Total Price", 2);
            PrintText(nSurface, winX + 550, winY + 53, 1, 200, 200, 200, "Ends At", 1);
        }
        for (int i = 0; i < 7 && (i + s_sellOfferScroll) < (int)sellOffers.size(); ++i) {
            int offIdx = i + s_sellOfferScroll;
            const auto& off = sellOffers[offIdx];
            int rY = winY + 68 + i * 16;
            if (offIdx == s_selectedSellOfferIndex) {
                DrawRectNative(nSurface, winX + 188, rY, 530, 16, 90, 90, 90);
            }
            if (PrintText) {
                PrintText(nSurface, winX + 192, rY + 2, 1, 220, 220, 220, off.playerName.empty() ? "Anonymous" : off.playerName.c_str(), 1);
                char b[32];
                sprintf_s(b, sizeof(b), "%u", off.amount); PrintText(nSurface, winX + 340, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u gp", off.piecePrice); PrintText(nSurface, winX + 420, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u gp", off.amount * off.piecePrice); PrintText(nSurface, winX + 510, rY + 2, 1, 220, 220, 220, b, 2);
                PrintText(nSurface, winX + 550, rY + 2, 1, 220, 220, 220, "Active", 1);
            }
        }
        DrawScrollBar(nSurface, winX + 720, winY + 48, 14, 147, s_sellOfferScroll, max(0, (int)sellOffers.size() - 7));

        // 2. Buy Offers Header
        if (PrintText) {
            PrintText(nSurface, winX + 186, winY + 206, 2, 220, 220, 220, "Buy Offers:", 1);
            PrintText(nSurface, winX + 270, winY + 206, 2, 220, 220, 220, "Amount:", 1);
            char amtStr[16];
            sprintf_s(amtStr, sizeof(amtStr), "%u", s_acceptBuyAmount);
            PrintText(nSurface, winX + 345, winY + 206, 2, 255, 255, 255, amtStr, 2);
            PrintText(nSurface, winX + 580, winY + 206, 2, 220, 220, 220, "Total:", 1);
        }
        DrawHScrollBar(nSurface, winX + 355, winY + 206, 140, 12, s_acceptBuyAmount, 100);
        DrawButton(nSurface, winX + 685, winY + 202, 48, 20, "Accept", false, buyOffers.empty());

        // Buy Offers Table (x=winX+186, y=winY+226, w=548, h=147)
        DrawSunkenBox(nSurface, winX + 186, winY + 226, 548, 147, 45, 45, 45);
        DrawRectNative(nSurface, winX + 188, winY + 228, 530, 16, 36, 36, 36);
        if (PrintText) {
            PrintText(nSurface, winX + 192, winY + 231, 1, 200, 200, 200, "Name", 1);
            PrintText(nSurface, winX + 340, winY + 231, 1, 200, 200, 200, "Amount", 2);
            PrintText(nSurface, winX + 420, winY + 231, 1, 200, 200, 200, "Piece Price", 2);
            PrintText(nSurface, winX + 510, winY + 231, 1, 200, 200, 200, "Total Price", 2);
            PrintText(nSurface, winX + 550, winY + 231, 1, 200, 200, 200, "Ends At", 1);
        }
        for (int i = 0; i < 7 && (i + s_buyOfferScroll) < (int)buyOffers.size(); ++i) {
            int offIdx = i + s_buyOfferScroll;
            const auto& off = buyOffers[offIdx];
            int rY = winY + 246 + i * 16;
            if (offIdx == s_selectedBuyOfferIndex) {
                DrawRectNative(nSurface, winX + 188, rY, 530, 16, 90, 90, 90);
            }
            if (PrintText) {
                PrintText(nSurface, winX + 192, rY + 2, 1, 220, 220, 220, off.playerName.empty() ? "Anonymous" : off.playerName.c_str(), 1);
                char b[32];
                sprintf_s(b, sizeof(b), "%u", off.amount); PrintText(nSurface, winX + 340, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u gp", off.piecePrice); PrintText(nSurface, winX + 420, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u gp", off.amount * off.piecePrice); PrintText(nSurface, winX + 510, rY + 2, 1, 220, 220, 220, b, 2);
                PrintText(nSurface, winX + 550, rY + 2, 1, 220, 220, 220, "Active", 1);
            }
        }
        DrawScrollBar(nSurface, winX + 720, winY + 226, 14, 147, s_buyOfferScroll, max(0, (int)buyOffers.size() - 7));

        // 3. Create Offer Section
        if (PrintText) {
            PrintText(nSurface, winX + 186, winY + 380, 2, 220, 220, 220, "Create Offer:", 1);
        }
        DrawRadioButton(nSurface, winX + 186, winY + 398, 58, 20, "Sell", s_createOfferType == 1);
        DrawRadioButton(nSurface, winX + 186, winY + 422, 58, 20, "Buy", s_createOfferType == 0);

        char cAmtBuf[32];
        sprintf_s(cAmtBuf, sizeof(cAmtBuf), "Amount: %u", s_createAmount);
        if (PrintText) {
            PrintText(nSurface, winX + 260, winY + 400, 2, 220, 220, 220, cAmtBuf, 1);
            PrintText(nSurface, winX + 260, winY + 424, 2, 220, 220, 220, "Piece Price:", 1);
        }
        DrawHScrollBar(nSurface, winX + 355, winY + 402, 140, 12, s_createAmount, s_createMaxAmount);

        // Piece Price Box
        DrawSunkenBox(nSurface, winX + 355, winY + 422, 140, 20, 38, 38, 38);
        if (PrintText) {
            PrintText(nSurface, winX + 361, winY + 426, 2, 255, 255, 255, s_createPriceStr.c_str(), 1);
        }

        // Financial calculations
        uint64_t price = 0;
        if (!s_createPriceStr.empty()) price = _strtoui64(s_createPriceStr.c_str(), NULL, 10);
        uint64_t gross = price * s_createAmount;
        uint64_t fee = (gross * 2) / 100;
        if (gross > 0) {
            if (fee < 20) fee = 20;
            if (fee > 250000) fee = 250000;
        }
        uint64_t finalTotal = (s_createOfferType == 1) ? (gross >= fee ? gross - fee : 0) : (gross + fee);

        char numStr[64];
        if (PrintText) {
            PrintText(nSurface, winX + 520, winY + 400, 2, 220, 220, 220, (s_createOfferType == 1) ? "Gross Profit:" : "Total Price:", 1);
            FormatNumberWithCommas(gross, numStr, sizeof(numStr));
            PrintText(nSurface, winX + 710, winY + 400, 2, 255, 255, 255, numStr, 2);

            PrintText(nSurface, winX + 520, winY + 418, 2, 220, 220, 220, "Fee:", 1);
            FormatNumberWithCommas(fee, numStr, sizeof(numStr));
            PrintText(nSurface, winX + 710, winY + 418, 2, 255, 255, 255, numStr, 2);
        }
        // Separator
        DrawRectNative(nSurface, winX + 520, winY + 436, 204, 1, 28, 28, 28);
        DrawRectNative(nSurface, winX + 520, winY + 437, 204, 1, 85, 85, 85);

        if (PrintText) {
            PrintText(nSurface, winX + 520, winY + 442, 2, 220, 220, 220, (s_createOfferType == 1) ? "Total Profit:" : "Total Price:", 1);
            FormatNumberWithCommas(finalTotal, numStr, sizeof(numStr));
            PrintText(nSurface, winX + 710, winY + 442, 2, 255, 255, 255, numStr, 2);
        }

        DrawCheckBox(nSurface, winX + 560, winY + 470, 90, 18, "Anonymous", s_isAnonymous);
        DrawButton(nSurface, winX + 665, winY + 468, 68, 22, "Create", false);
    }

    // =========================================================================
    // RIGHT PANEL - TAB_DETAILS
    // =========================================================================
    else if (s_currentTab == TAB_DETAILS) {
        const auto& det = MarketSystem::get().getCurrentDetail();

        if (PrintText) {
            PrintText(nSurface, winX + 186, winY + 28, 2, 220, 220, 220, "Details:", 1);
        }
        DrawSunkenBox(nSurface, winX + 186, winY + 48, 548, 126, 45, 45, 45);
        if (PrintText) {
            int lineY = winY + 54;
            if (!det.description.empty()) { PrintText(nSurface, winX + 192, lineY, 1, 220, 220, 220, det.description.c_str(), 1); lineY += 14; }
            if (!det.attack.empty()) { std::string s = "Attack: " + det.attack; PrintText(nSurface, winX + 192, lineY, 1, 220, 220, 220, s.c_str(), 1); lineY += 14; }
            if (!det.defense.empty()) { std::string s = "Defence: " + det.defense; PrintText(nSurface, winX + 192, lineY, 1, 220, 220, 220, s.c_str(), 1); lineY += 14; }
            if (!det.armor.empty()) { std::string s = "Armor: " + det.armor; PrintText(nSurface, winX + 192, lineY, 1, 220, 220, 220, s.c_str(), 1); lineY += 14; }
            if (!det.weight.empty()) { std::string s = "Weight: " + det.weight + " oz"; PrintText(nSurface, winX + 192, lineY, 1, 220, 220, 220, s.c_str(), 1); lineY += 14; }
            if (!det.reqlvl.empty()) { std::string s = "Req. Level: " + det.reqlvl; PrintText(nSurface, winX + 192, lineY, 1, 220, 220, 220, s.c_str(), 1); lineY += 14; }
            if (!det.vocation.empty()) { std::string s = "Vocation: " + det.vocation; PrintText(nSurface, winX + 192, lineY, 1, 220, 220, 220, s.c_str(), 1); lineY += 14; }
        }

        if (PrintText) {
            PrintText(nSurface, winX + 186, winY + 186, 2, 220, 220, 220, "Statistics:", 1);
        }
        DrawSunkenBox(nSurface, winX + 186, winY + 204, 548, 276, 45, 45, 45);
        if (PrintText) {
            int sY = winY + 212;
            PrintText(nSurface, winX + 192, sY, 2, 255, 255, 255, "Buy Offers:", 1); sY += 16;
            if (det.hasBuyStats && det.buyStats.numTransactions > 0) {
                char b[64];
                sprintf_s(b, sizeof(b), "  Number of Transactions: %u", det.buyStats.numTransactions); PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, b, 1); sY += 14;
                sprintf_s(b, sizeof(b), "  Highest Price: %u gold", det.buyStats.highestPrice); PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, b, 1); sY += 14;
                sprintf_s(b, sizeof(b), "  Average Price: %u gold", det.buyStats.totalPrice / det.buyStats.numTransactions); PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, b, 1); sY += 14;
                sprintf_s(b, sizeof(b), "  Lowest Price: %u gold", det.buyStats.lowestPrice); PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, b, 1); sY += 14;
            } else {
                PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, "  No transaction data available.", 1); sY += 14;
            }
            sY += 10;
            PrintText(nSurface, winX + 192, sY, 2, 255, 255, 255, "Sell Offers:", 1); sY += 16;
            if (det.hasSellStats && det.sellStats.numTransactions > 0) {
                char b[64];
                sprintf_s(b, sizeof(b), "  Number of Transactions: %u", det.sellStats.numTransactions); PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, b, 1); sY += 14;
                sprintf_s(b, sizeof(b), "  Highest Price: %u gold", det.sellStats.highestPrice); PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, b, 1); sY += 14;
                sprintf_s(b, sizeof(b), "  Average Price: %u gold", det.sellStats.totalPrice / det.sellStats.numTransactions); PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, b, 1); sY += 14;
                sprintf_s(b, sizeof(b), "  Lowest Price: %u gold", det.sellStats.lowestPrice); PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, b, 1); sY += 14;
            } else {
                PrintText(nSurface, winX + 192, sY, 1, 220, 220, 220, "  No transaction data available.", 1); sY += 14;
            }
        }
    }

    // =========================================================================
    // TAB_MY_OFFERS & TAB_OFFER_HISTORY (Full width tables)
    // =========================================================================
    else if (s_currentTab == TAB_MY_OFFERS || s_currentTab == TAB_OFFER_HISTORY) {
        bool isHistory = (s_currentTab == TAB_OFFER_HISTORY);
        const auto& mySell = isHistory ? MarketSystem::get().getSellHistory() : MarketSystem::get().getMySellOffers();
        const auto& myBuy = isHistory ? MarketSystem::get().getBuyHistory() : MarketSystem::get().getMyBuyOffers();

        char sHeader[64], bHeader[64];
        sprintf_s(sHeader, sizeof(sHeader), "Sell Offers (%u):", (uint32_t)mySell.size());
        sprintf_s(bHeader, sizeof(bHeader), "Buy Offers (%u):", (uint32_t)myBuy.size());

        if (PrintText) {
            PrintText(nSurface, winX + 16, winY + 28, 2, 220, 220, 220, sHeader, 1);
        }
        if (!isHistory) {
            DrawButton(nSurface, winX + 648, winY + 24, 86, 20, "Cancel Offer", false, mySell.empty() || s_selectedMySellOfferIndex < 0);
        }

        // Sell Offers Table
        DrawSunkenBox(nSurface, winX + 16, winY + 48, 718, 180, 45, 45, 45);
        DrawRectNative(nSurface, winX + 18, winY + 50, 700, 16, 36, 36, 36);
        if (PrintText) {
            PrintText(nSurface, winX + 22, winY + 53, 1, 200, 200, 200, "Item", 1);
            PrintText(nSurface, winX + 250, winY + 53, 1, 200, 200, 200, "Amount", 2);
            PrintText(nSurface, winX + 350, winY + 53, 1, 200, 200, 200, "Piece Price", 2);
            PrintText(nSurface, winX + 460, winY + 53, 1, 200, 200, 200, "Total Price", 2);
            PrintText(nSurface, winX + 510, winY + 53, 1, 200, 200, 200, isHistory ? "Ended At" : "Ends At", 1);
            if (isHistory) PrintText(nSurface, winX + 640, winY + 53, 1, 200, 200, 200, "Status", 1);
        }
        for (int i = 0; i < 9 && (i + s_mySellOfferScroll) < (int)mySell.size(); ++i) {
            int idx = i + s_mySellOfferScroll;
            const auto& off = mySell[idx];
            int rY = winY + 68 + i * 16;
            if (idx == s_selectedMySellOfferIndex) {
                DrawRectNative(nSurface, winX + 18, rY, 700, 16, 90, 90, 90);
            }
            if (PrintText) {
                char b[64];
                sprintf_s(b, sizeof(b), "Item #%u", off.itemId); PrintText(nSurface, winX + 22, rY + 2, 1, 220, 220, 220, b, 1);
                sprintf_s(b, sizeof(b), "%u", off.amount); PrintText(nSurface, winX + 250, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u gp", off.piecePrice); PrintText(nSurface, winX + 350, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u gp", off.amount * off.piecePrice); PrintText(nSurface, winX + 460, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u ts", off.timestamp); PrintText(nSurface, winX + 510, rY + 2, 1, 220, 220, 220, b, 1);
                if (isHistory) {
                    const char* st = (off.state == 3) ? "sold" : ((off.state == 1) ? "cancelled" : "expired");
                    PrintText(nSurface, winX + 640, rY + 2, 1, 220, 220, 220, st, 1);
                }
            }
        }
        DrawScrollBar(nSurface, winX + 718, winY + 48, 14, 180, s_mySellOfferScroll, max(0, (int)mySell.size() - 9));

        if (PrintText) {
            PrintText(nSurface, winX + 16, winY + 236, 2, 220, 220, 220, bHeader, 1);
        }
        if (!isHistory) {
            DrawButton(nSurface, winX + 648, winY + 232, 86, 20, "Cancel Offer", false, myBuy.empty() || s_selectedMyBuyOfferIndex < 0);
        }

        // Buy Offers Table
        DrawSunkenBox(nSurface, winX + 16, winY + 256, 718, 224, 45, 45, 45);
        DrawRectNative(nSurface, winX + 18, winY + 258, 700, 16, 36, 36, 36);
        if (PrintText) {
            PrintText(nSurface, winX + 22, winY + 261, 1, 200, 200, 200, "Item", 1);
            PrintText(nSurface, winX + 250, winY + 261, 1, 200, 200, 200, "Amount", 2);
            PrintText(nSurface, winX + 350, winY + 261, 1, 200, 200, 200, "Piece Price", 2);
            PrintText(nSurface, winX + 460, winY + 261, 1, 200, 200, 200, "Total Price", 2);
            PrintText(nSurface, winX + 510, winY + 261, 1, 200, 200, 200, isHistory ? "Ended At" : "Ends At", 1);
            if (isHistory) PrintText(nSurface, winX + 640, winY + 261, 1, 200, 200, 200, "Status", 1);
        }
        for (int i = 0; i < 12 && (i + s_myBuyOfferScroll) < (int)myBuy.size(); ++i) {
            int idx = i + s_myBuyOfferScroll;
            const auto& off = myBuy[idx];
            int rY = winY + 276 + i * 16;
            if (idx == s_selectedMyBuyOfferIndex) {
                DrawRectNative(nSurface, winX + 18, rY, 700, 16, 90, 90, 90);
            }
            if (PrintText) {
                char b[64];
                sprintf_s(b, sizeof(b), "Item #%u", off.itemId); PrintText(nSurface, winX + 22, rY + 2, 1, 220, 220, 220, b, 1);
                sprintf_s(b, sizeof(b), "%u", off.amount); PrintText(nSurface, winX + 250, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u gp", off.piecePrice); PrintText(nSurface, winX + 350, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u gp", off.amount * off.piecePrice); PrintText(nSurface, winX + 460, rY + 2, 1, 220, 220, 220, b, 2);
                sprintf_s(b, sizeof(b), "%u ts", off.timestamp); PrintText(nSurface, winX + 510, rY + 2, 1, 220, 220, 220, b, 1);
                if (isHistory) {
                    const char* st = (off.state == 3) ? "bought" : ((off.state == 1) ? "cancelled" : "expired");
                    PrintText(nSurface, winX + 640, rY + 2, 1, 220, 220, 220, st, 1);
                }
            }
        }
        DrawScrollBar(nSurface, winX + 718, winY + 256, 14, 224, s_myBuyOfferScroll, max(0, (int)myBuy.size() - 12));
    }
}

// -----------------------------------------------------------------------------
// Mouse & Keyboard Input Handling
// -----------------------------------------------------------------------------
bool InGameMarket::OnLMouseDown(int mouseX, int mouseY) {
    if (!s_isOpen) return false;

    RECT rc;
    GetClientRect(s_hTibiaWnd, &rc);
    int screenW = rc.right - rc.left;
    int screenH = rc.bottom - rc.top;
    int winW = 750, winH = 545;
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;

    if (mouseX < winX || mouseX > winX + winW || mouseY < winY || mouseY > winY + winH) {
        return false; // Click outside window
    }

    auto inRect = [&](int rx, int ry, int rw, int rh) {
        return (mouseX >= rx && mouseX <= rx + rw && mouseY >= ry && mouseY <= ry + rh);
    };

    // Close button [X] at top-right
    if (inRect(winX + winW - 22, winY + 3, 18, 14)) {
        Close();
        return true;
    }

    // Bottom Bar Buttons
    if (inRect(winX + 242, winY + 512, 84, 20)) {
        ShellExecuteA(NULL, "open", "https://www.tibia.com/news/?subtopic=latestnews", NULL, NULL, SW_SHOWNORMAL);
        return true;
    }

    if (s_currentTab == TAB_OFFERS || s_currentTab == TAB_DETAILS) {
        if (inRect(winX + 360, winY + 512, 72, 20)) { s_currentTab = TAB_OFFERS; return true; }
        if (inRect(winX + 436, winY + 512, 72, 20)) { s_currentTab = TAB_DETAILS; return true; }
        if (inRect(winX + 512, winY + 512, 80, 20)) {
            s_currentTab = TAB_MY_OFFERS;
            MarketSystem::get().sendBrowseOwnOffers();
            return true;
        }
        if (s_currentTab == TAB_OFFERS && inRect(winX + 652, winY + 512, 82, 20)) { Close(); return true; }
        if (s_currentTab == TAB_DETAILS && inRect(winX + 652, winY + 512, 82, 20)) { s_currentTab = TAB_OFFERS; return true; }
    } else {
        if (inRect(winX + 440, winY + 512, 98, 20)) {
            s_currentTab = TAB_MY_OFFERS;
            MarketSystem::get().sendBrowseOwnOffers();
            return true;
        }
        if (inRect(winX + 542, winY + 512, 98, 20)) {
            s_currentTab = TAB_OFFER_HISTORY;
            MarketSystem::get().sendBrowseOwnHistory();
            return true;
        }
        if (inRect(winX + 652, winY + 512, 82, 20)) { s_currentTab = TAB_OFFERS; return true; }
    }

    // Left Panel clicks
    if (s_currentTab == TAB_OFFERS || s_currentTab == TAB_DETAILS) {
        // Categories
        if (inRect(winX + 16, winY + 28, 146, 76)) {
            int clickedRow = (mouseY - (winY + 28)) / 15;
            if (clickedRow >= 0 && clickedRow < 5) {
                int catIdx = clickedRow + s_categoryListScroll;
                if (catIdx >= 0 && catIdx < 23) {
                    s_selectedCategory = catIdx;
                }
            }
            return true;
        }

        // Filter buttons
        if (inRect(winX + 16, winY + 110, 34, 20)) { s_filterLevel = !s_filterLevel; return true; }
        if (inRect(winX + 58, winY + 110, 34, 20)) { s_filterVoc = !s_filterVoc; return true; }
        if (inRect(winX + 100, winY + 110, 34, 20)) { s_filter1H = !s_filter1H; return true; }
        if (inRect(winX + 142, winY + 110, 34, 20)) { s_filter2H = !s_filter2H; return true; }
        if (inRect(winX + 16, winY + 134, 160, 18)) { s_showLockerOnly = !s_showLockerOnly; return true; }

        // Item List clicks
        if (inRect(winX + 16, winY + 172, 146, 276)) {
            int clickedRow = (mouseY - (winY + 172)) / 34;
            int itemIdx = clickedRow + s_itemListScroll;
            if (itemIdx >= 0 && itemIdx < (int)s_displayItems.size()) {
                s_selectedItemIndex = itemIdx;
                uint16_t itemId = s_displayItems[itemIdx].thingId;
                MarketSystem::get().sendBrowse(itemId);
            }
            return true;
        }

        // Search Box focus
        if (inRect(winX + 56, winY + 462, 120, 22)) {
            s_focusedBox = FOCUS_SEARCH;
            return true;
        }
    }

    // Right Panel - TAB_OFFERS
    if (s_currentTab == TAB_OFFERS) {
        // Accept Sell Offer
        if (inRect(winX + 685, winY + 24, 48, 20)) {
            const auto& sellOffers = MarketSystem::get().getSellOffers();
            if (s_selectedSellOfferIndex >= 0 && s_selectedSellOfferIndex < (int)sellOffers.size()) {
                const auto& off = sellOffers[s_selectedSellOfferIndex];
                MarketSystem::get().sendAcceptOffer(off.timestamp, off.counter, (uint16_t)s_acceptSellAmount);
            }
            return true;
        }

        // Sell Offers Table selection
        if (inRect(winX + 186, winY + 68, 534, 127)) {
            int clickedRow = (mouseY - (winY + 68)) / 16;
            int offIdx = clickedRow + s_sellOfferScroll;
            const auto& sellOffers = MarketSystem::get().getSellOffers();
            if (offIdx >= 0 && offIdx < (int)sellOffers.size()) {
                s_selectedSellOfferIndex = offIdx;
                s_acceptSellAmount = min(sellOffers[offIdx].amount, 100u);
            }
            return true;
        }

        // Accept Buy Offer
        if (inRect(winX + 685, winY + 202, 48, 20)) {
            const auto& buyOffers = MarketSystem::get().getBuyOffers();
            if (s_selectedBuyOfferIndex >= 0 && s_selectedBuyOfferIndex < (int)buyOffers.size()) {
                const auto& off = buyOffers[s_selectedBuyOfferIndex];
                MarketSystem::get().sendAcceptOffer(off.timestamp, off.counter, (uint16_t)s_acceptBuyAmount);
            }
            return true;
        }

        // Buy Offers Table selection
        if (inRect(winX + 186, winY + 246, 534, 127)) {
            int clickedRow = (mouseY - (winY + 246)) / 16;
            int offIdx = clickedRow + s_buyOfferScroll;
            const auto& buyOffers = MarketSystem::get().getBuyOffers();
            if (offIdx >= 0 && offIdx < (int)buyOffers.size()) {
                s_selectedBuyOfferIndex = offIdx;
                s_acceptBuyAmount = min(buyOffers[offIdx].amount, 100u);
            }
            return true;
        }

        // Create Offer controls
        if (inRect(winX + 186, winY + 398, 58, 20)) { s_createOfferType = 1; return true; }
        if (inRect(winX + 186, winY + 422, 58, 20)) { s_createOfferType = 0; return true; }
        if (inRect(winX + 355, winY + 422, 140, 20)) { s_focusedBox = FOCUS_PRICE; return true; }
        if (inRect(winX + 560, winY + 470, 90, 18)) { s_isAnonymous = !s_isAnonymous; return true; }

        // [ Create ] Offer button
        if (inRect(winX + 665, winY + 468, 68, 22)) {
            if (s_selectedItemIndex >= 0 && s_selectedItemIndex < (int)s_displayItems.size()) {
                uint16_t thingId = s_displayItems[s_selectedItemIndex].thingId;
                uint64_t price = _strtoui64(s_createPriceStr.c_str(), NULL, 10);
                if (price > 0 && s_createAmount > 0) {
                    MarketSystem::get().sendCreateOffer(s_createOfferType, thingId, (uint16_t)s_createAmount, (uint32_t)price, s_isAnonymous);
                    s_createPriceStr = "";
                }
            }
            return true;
        }
    }

    // Cancel Offer buttons in My Offers
    if (s_currentTab == TAB_MY_OFFERS) {
        if (inRect(winX + 648, winY + 24, 86, 20)) {
            const auto& mySell = MarketSystem::get().getMySellOffers();
            if (s_selectedMySellOfferIndex >= 0 && s_selectedMySellOfferIndex < (int)mySell.size()) {
                MarketSystem::get().sendCancelOffer(mySell[s_selectedMySellOfferIndex].timestamp, mySell[s_selectedMySellOfferIndex].counter);
                s_selectedMySellOfferIndex = -1;
            }
            return true;
        }
        if (inRect(winX + 16, winY + 68, 700, 160)) {
            int clickedRow = (mouseY - (winY + 68)) / 16;
            int idx = clickedRow + s_mySellOfferScroll;
            if (idx >= 0 && idx < (int)MarketSystem::get().getMySellOffers().size()) {
                s_selectedMySellOfferIndex = idx;
            }
            return true;
        }

        if (inRect(winX + 648, winY + 232, 86, 20)) {
            const auto& myBuy = MarketSystem::get().getMyBuyOffers();
            if (s_selectedMyBuyOfferIndex >= 0 && s_selectedMyBuyOfferIndex < (int)myBuy.size()) {
                MarketSystem::get().sendCancelOffer(myBuy[s_selectedMyBuyOfferIndex].timestamp, myBuy[s_selectedMyBuyOfferIndex].counter);
                s_selectedMyBuyOfferIndex = -1;
            }
            return true;
        }
        if (inRect(winX + 16, winY + 276, 700, 200)) {
            int clickedRow = (mouseY - (winY + 276)) / 16;
            int idx = clickedRow + s_myBuyOfferScroll;
            if (idx >= 0 && idx < (int)MarketSystem::get().getMyBuyOffers().size()) {
                s_selectedMyBuyOfferIndex = idx;
            }
            return true;
        }
    }

    return true; // Click consumed inside Market window
}

bool InGameMarket::OnLMouseUp(int mouseX, int mouseY) {
    if (!s_isOpen) return false;
    return true;
}

bool InGameMarket::OnMouseMove(int mouseX, int mouseY) {
    if (!s_isOpen) return false;
    return true;
}

bool InGameMarket::OnWheel(int mouseX, int mouseY, bool wheelUp) {
    if (!s_isOpen) return false;

    RECT rc;
    GetClientRect(s_hTibiaWnd, &rc);
    int screenW = rc.right - rc.left;
    int screenH = rc.bottom - rc.top;
    int winW = 750, winH = 545;
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;

    if (mouseX >= winX + 16 && mouseX <= winX + 176 && mouseY >= winY + 172 && mouseY <= winY + 448) {
        if (wheelUp) s_itemListScroll = max(0, s_itemListScroll - 1);
        else s_itemListScroll = min(max(0, (int)s_displayItems.size() - 8), s_itemListScroll + 1);
        return true;
    }
    return true;
}

bool InGameMarket::OnKeyDown(int vk) {
    if (!s_isOpen) return false;

    if (vk == VK_ESCAPE) {
        Close();
        return true;
    }
    if (vk == VK_BACK) {
        if (s_focusedBox == FOCUS_SEARCH && !s_searchText.empty()) {
            s_searchText.pop_back();
            RefreshDisplayItems();
            return true;
        }
        if (s_focusedBox == FOCUS_PRICE && !s_createPriceStr.empty()) {
            s_createPriceStr.pop_back();
            return true;
        }
    }
    return true;
}

bool InGameMarket::OnChar(char c) {
    if (!s_isOpen) return false;

    if (s_focusedBox == FOCUS_SEARCH) {
        if (c >= 32 && c <= 126 && s_searchText.length() < 30) {
            s_searchText.push_back(c);
            RefreshDisplayItems();
            return true;
        }
    } else if (s_focusedBox == FOCUS_PRICE) {
        if (c >= '0' && c <= '9' && s_createPriceStr.length() < 12) {
            s_createPriceStr.push_back(c);
            return true;
        }
    }
    return true;
}

LRESULT CALLBACK InGameMarket::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (s_isOpen) {
        switch (msg) {
        case WM_LBUTTONDOWN: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);
            if (OnLMouseDown(x, y)) return 0;
            break;
        }
        case WM_LBUTTONUP: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);
            if (OnLMouseUp(x, y)) return 0;
            break;
        }
        case WM_MOUSEMOVE: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);
            if (OnMouseMove(x, y)) return 0;
            break;
        }
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            ScreenToClient(hwnd, &pt);
            if (OnWheel(pt.x, pt.y, delta > 0)) return 0;
            break;
        }
        case WM_KEYDOWN: {
            if (OnKeyDown((int)wParam)) return 0;
            break;
        }
        case WM_CHAR: {
            if (OnChar((char)wParam)) return 0;
            break;
        }
        }
    }
    return CallWindowProcA(s_oldWndProc, hwnd, msg, wParam, lParam);
}
