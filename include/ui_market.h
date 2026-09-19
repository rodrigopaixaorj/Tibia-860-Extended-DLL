/*
  Tibia 860 - Extended Client DLL
  Copyright (C) 2026 Nottinghster (github.com/rodrigopaixaorj)

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.
*/

#ifndef __UI_MARKET_H__
#define __UI_MARKET_H__

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

// ===================================================================
// Market System Structures Faithful to The-Forgotten-Client (TFC)
// ===================================================================

struct MarketItem {
    uint16_t thingId = 0;
    std::string name = "";
    uint32_t count = 0;
};

struct MarketOffer {
    uint32_t timestamp = 0;
    uint16_t counter = 0;
    uint16_t itemId = 0;
    uint32_t amount = 0;
    uint32_t piecePrice = 0;
    std::string playerName = "";
    uint8_t state = 0; // 0 = Active, 1 = Cancelled, 2 = Expired, 3 = Accepted
};

struct MarketStatistics {
    uint32_t numTransactions = 0;
    uint32_t highestPrice = 0;
    uint32_t totalPrice = 0;
    uint32_t lowestPrice = 0;
};

struct MarketDetail {
    uint16_t itemId = 0;
    std::string armor;
    std::string attack;
    std::string container;
    std::string defense;
    std::string description;
    std::string decayTime;
    std::string absorb;
    std::string reqlvl;
    std::string reqmaglvl;
    std::string vocation;
    std::string runespellname;
    std::string stats;
    std::string charges;
    std::string weaponName;
    std::string weight;
    std::string imbuements;
    bool hasBuyStats = false;
    MarketStatistics buyStats;
    bool hasSellStats = false;
    MarketStatistics sellStats;
};

class MarketSystem {
public:
    static MarketSystem& get();

    void open();
    void close();
    bool isOpen() const { return m_isOpen; }

    void addBuyOffer(const MarketOffer& offer);
    void addSellOffer(const MarketOffer& offer);
    void clearOffers();

    void setAccountBalance(uint32_t balance) { m_accountBalance = balance; }
    uint32_t getAccountBalance() const { return m_accountBalance; }

    void registerMarketItem(uint16_t tradeAs, const std::string& name, uint16_t category, uint16_t reqLvl, uint16_t reqVoc);
    void parseMarketData(const uint8_t* buffer, size_t size);
    void render();

private:
    MarketSystem() = default;
    ~MarketSystem() = default;

    bool m_isOpen = false;
    uint32_t m_accountBalance = 0;

    std::vector<MarketOffer> m_buyOffers;
    std::vector<MarketOffer> m_sellOffers;
    std::vector<MarketOffer> m_myOffers;
    std::vector<MarketOffer> m_myHistory;
    std::vector<MarketItem>  m_marketItems;

    std::mutex m_mutex;
};

void InitMarketHooks();

#endif // __UI_MARKET_H__
