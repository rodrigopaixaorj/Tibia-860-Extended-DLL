/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "ui_market.h"
#include "market_window.h"
#include "main.h"
#include "config.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

// -----------------------------------------------------------------------
// Native client write helpers (inline wrappers for output buffer)
// -----------------------------------------------------------------------
static inline void NativeSendBegin() {
    // 0x4F96E0 = beginSendPacket() -- clears the output buffer
    if (g_clientBaseAddr)
        ((void(__cdecl*)())(g_clientBaseAddr + 0xF96E0))();
}
static inline void NativeSendEnd() {
    // 0x4F9680 = sendPacket() -- flushes the output buffer
    if (g_clientBaseAddr)
        ((void(__cdecl*)())(g_clientBaseAddr + 0xF9680))();
}
static inline void NativeWriteByte(uint8_t v) {
    if (g_clientBaseAddr)
        ((void(__cdecl*)(uint8_t))(g_clientBaseAddr + 0xF8560))(v);
}
static inline void NativeWriteU16(uint16_t v) {
    if (g_clientBaseAddr)
        ((void(__cdecl*)(uint16_t))(g_clientBaseAddr + 0xF8700))(v);
}
static inline void NativeWriteU32(uint32_t v) {
    if (g_clientBaseAddr) {
        ((void(__cdecl*)(uint16_t))(g_clientBaseAddr + 0xF8700))(static_cast<uint16_t>(v & 0xFFFF));
        ((void(__cdecl*)(uint16_t))(g_clientBaseAddr + 0xF8700))(static_cast<uint16_t>(v >> 16));
    }
}
static inline void NativeWriteString(const std::string& s) {
    NativeWriteU16(static_cast<uint16_t>(s.size()));
    for (char c : s) NativeWriteByte(static_cast<uint8_t>(c));
}

// -----------------------------------------------------------------------
// Singleton
// -----------------------------------------------------------------------
MarketSystem& MarketSystem::get() {
    static MarketSystem instance;
    return instance;
}

// -----------------------------------------------------------------------
// State management
// -----------------------------------------------------------------------
void MarketSystem::open() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isOpen = true;
    }
    InGameMarket::Open();
}

void MarketSystem::close() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isOpen = false;
    }
    InGameMarket::Close();
}

void MarketSystem::clearOffers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_browseItemBuyOffers.clear();
    m_browseItemSellOffers.clear();
    m_myBuyOffers.clear();
    m_mySellOffers.clear();
    m_historyBuy.clear();
    m_historySell.clear();
    m_depotItems.clear();
    m_currentBrowseItem = 0;
    InGameMarket::Refresh();
}

// -----------------------------------------------------------------------
// Data update methods (called from network_protocol.cpp)
// -----------------------------------------------------------------------
void MarketSystem::updateDepotItems(const std::map<uint16_t, uint32_t>& items) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_depotItems = items;
    }
    InGameMarket::Refresh();
}

void MarketSystem::updateMyOffers(const std::vector<MarketOffer>& buyOffers,
                                   const std::vector<MarketOffer>& sellOffers) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myBuyOffers  = buyOffers;
        m_mySellOffers = sellOffers;
    }
    InGameMarket::Refresh();
}

void MarketSystem::updateHistory(const std::vector<MarketOffer>& buyOffers,
                                  const std::vector<MarketOffer>& sellOffers) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_historyBuy  = buyOffers;
        m_historySell = sellOffers;
    }
    InGameMarket::Refresh();
}

void MarketSystem::updateOffers(uint16_t itemId,
                                 const std::vector<MarketOffer>& buyOffers,
                                 const std::vector<MarketOffer>& sellOffers) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentBrowseItem   = itemId;
        m_browseItemBuyOffers  = buyOffers;
        m_browseItemSellOffers = sellOffers;
    }
    InGameMarket::Refresh();
}

void MarketSystem::updateDetail(uint16_t itemId,
                                 const std::string strings[], int numStrings,
                                 bool hasBuyStats,  uint32_t buyTx,  uint32_t buyHigh,
                                 uint32_t buyTotal, uint32_t buyLow,
                                 bool hasSellStats, uint32_t sellTx, uint32_t sellHigh,
                                 uint32_t sellTotal,uint32_t sellLow) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentDetail = MarketDetail{};
        m_currentDetail.itemId = itemId;

        std::string* fields[] = {
            &m_currentDetail.armor, &m_currentDetail.attack, &m_currentDetail.container,
            &m_currentDetail.defense, &m_currentDetail.description, &m_currentDetail.decayTime,
            &m_currentDetail.absorb, &m_currentDetail.reqlvl, &m_currentDetail.reqmaglvl,
            &m_currentDetail.vocation, &m_currentDetail.runespellname, &m_currentDetail.stats,
            &m_currentDetail.charges, &m_currentDetail.weaponName, &m_currentDetail.weight,
            &m_currentDetail.imbuements
        };
        for (int i = 0; i < numStrings && i < 16; ++i)
            *fields[i] = strings[i];

        m_currentDetail.hasBuyStats = hasBuyStats;
        if (hasBuyStats) {
            m_currentDetail.buyStats.numTransactions = buyTx;
            m_currentDetail.buyStats.highestPrice    = buyHigh;
            m_currentDetail.buyStats.totalPrice      = buyTotal;
            m_currentDetail.buyStats.lowestPrice     = buyLow;
        }
        m_currentDetail.hasSellStats = hasSellStats;
        if (hasSellStats) {
            m_currentDetail.sellStats.numTransactions = sellTx;
            m_currentDetail.sellStats.highestPrice    = sellHigh;
            m_currentDetail.sellStats.totalPrice      = sellTotal;
            m_currentDetail.sellStats.lowestPrice     = sellLow;
        }
    }
    InGameMarket::Refresh();
}

// -----------------------------------------------------------------------
// Client->Server packet senders
// -----------------------------------------------------------------------
void MarketSystem::sendLeave() const {
    NativeSendBegin();
    NativeWriteByte(0xF4);
    NativeSendEnd();
}

void MarketSystem::sendBrowse(uint16_t itemId) const {
    NativeSendBegin();
    NativeWriteByte(0xF5);
    NativeWriteU16(itemId);
    NativeSendEnd();
}

void MarketSystem::sendBrowseOwnOffers() const {
    NativeSendBegin();
    NativeWriteByte(0xF5);
    NativeWriteU16(0xFFFF); // MARKETREQUEST_OWN_OFFERS
    NativeSendEnd();
}

void MarketSystem::sendBrowseOwnHistory() const {
    NativeSendBegin();
    NativeWriteByte(0xF5);
    NativeWriteU16(0xFFFE); // MARKETREQUEST_OWN_HISTORY
    NativeSendEnd();
}

void MarketSystem::sendCreateOffer(uint8_t type, uint16_t itemId, uint16_t amount,
                                    uint32_t price, bool anonymous) const {
    NativeSendBegin();
    NativeWriteByte(0xF6);
    NativeWriteByte(type);
    NativeWriteU16(itemId);
    NativeWriteU16(amount);
    NativeWriteU32(price);
    NativeWriteByte(anonymous ? 1 : 0);
    NativeSendEnd();
}

void MarketSystem::sendCancelOffer(uint32_t timestamp, uint16_t counter) const {
    NativeSendBegin();
    NativeWriteByte(0xF7);
    NativeWriteU32(timestamp);
    NativeWriteU16(counter);
    NativeSendEnd();
}

void MarketSystem::sendAcceptOffer(uint32_t timestamp, uint16_t counter, uint16_t amount) const {
    NativeSendBegin();
    NativeWriteByte(0xF8);
    NativeWriteU32(timestamp);
    NativeWriteU16(counter);
    NativeWriteU16(amount);
    NativeSendEnd();
}

// -----------------------------------------------------------------------
// Market item registration (from DAT reader)
// -----------------------------------------------------------------------
void MarketSystem::registerMarketItem(uint16_t tradeAs, const std::string& name,
                                       uint16_t /*category*/, uint16_t /*reqLvl*/,
                                       uint16_t /*reqVoc*/) {
    std::lock_guard<std::mutex> lock(m_mutex);
    MarketItem item;
    item.thingId = tradeAs;
    item.name    = name;
    item.count   = 0;
    m_marketItems.push_back(item);
}

// -----------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------
void MarketSystem::render() {
    // InGameMarket::Render is invoked in the client frame render hook
}

// -----------------------------------------------------------------------
// Hook registration (called from dllmain.cpp SafeInit)
// -----------------------------------------------------------------------
void InitMarketHooks() {
    InGameMarket::Init();
}