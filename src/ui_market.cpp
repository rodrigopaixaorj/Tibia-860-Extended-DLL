/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#include "ui_market.h"
#include "main.h"
#include "config.h"

MarketSystem& MarketSystem::get() {
    static MarketSystem instance;
    return instance;
}

void MarketSystem::open() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isOpen = true;
}

void MarketSystem::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isOpen = false;
}

void MarketSystem::addBuyOffer(const MarketOffer& offer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buyOffers.push_back(offer);
}

void MarketSystem::addSellOffer(const MarketOffer& offer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sellOffers.push_back(offer);
}

void MarketSystem::clearOffers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buyOffers.clear();
    m_sellOffers.clear();
    m_myOffers.clear();
    m_myHistory.clear();
}

void MarketSystem::registerMarketItem(uint16_t tradeAs, const std::string& name, uint16_t category, uint16_t reqLvl, uint16_t reqVoc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    MarketItem item;
    item.thingId = tradeAs;
    item.name = name;
    item.count = 0;
    m_marketItems.push_back(item);
}

void MarketSystem::parseMarketData(const uint8_t* buffer, size_t size) {
    if (!buffer || size < 2) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    // Decode market protocol binary messages received from the server
}

void MarketSystem::render() {
    if (!m_isOpen) return;

    // Render Market window (Faithful layout to TFC: 650x450 dimensions, tabs, tables)
}

void InitMarketHooks() {
    // Register required hooks for Market GUI integration
}
