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
#include <map>
#include <mutex>

// ===================================================================
// Market System Structures
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

    // Data update methods (called by network_protocol.cpp)
    void clearOffers();
    void setAccountBalance(uint32_t balance) { m_accountBalance = balance; }
    uint32_t getAccountBalance() const { return m_accountBalance; }
    void setCoins(uint32_t coins) { m_coins = coins; }
    uint32_t getCoins() const { return m_coins; }

    void updateDepotItems(const std::map<uint16_t, uint32_t>& items);
    void updateMyOffers(const std::vector<MarketOffer>& buyOffers,
                        const std::vector<MarketOffer>& sellOffers);
    void updateHistory(const std::vector<MarketOffer>& buyOffers,
                       const std::vector<MarketOffer>& sellOffers);
    void updateOffers(uint16_t itemId,
                      const std::vector<MarketOffer>& buyOffers,
                      const std::vector<MarketOffer>& sellOffers);
    void updateDetail(uint16_t itemId,
                      const std::string strings[], int numStrings,
                      bool hasBuyStats, uint32_t buyTx, uint32_t buyHigh,
                      uint32_t buyTotal, uint32_t buyLow,
                      bool hasSellStats, uint32_t sellTx, uint32_t sellHigh,
                      uint32_t sellTotal, uint32_t sellLow);

    // Client->server send helpers
    void sendBrowse(uint16_t itemId) const;
    void sendBrowseOwnOffers() const;
    void sendBrowseOwnHistory() const;
    void sendCreateOffer(uint8_t type, uint16_t itemId, uint16_t amount,
                         uint32_t price, bool anonymous) const;
    void sendCancelOffer(uint32_t timestamp, uint16_t counter) const;
    void sendAcceptOffer(uint32_t timestamp, uint16_t counter, uint16_t amount) const;
    void sendLeave() const;

    // Market item registration (from DAT/item data)
    void registerMarketItem(uint16_t tradeAs, const std::string& name,
                            uint16_t category, uint16_t reqLvl, uint16_t reqVoc);

    // Render (called each frame when open)
    void render();

    // Getters for UI
    const std::vector<MarketOffer>& getBuyOffers() const  { return m_browseItemBuyOffers; }
    const std::vector<MarketOffer>& getSellOffers() const { return m_browseItemSellOffers; }
    const std::vector<MarketOffer>& getMyBuyOffers() const  { return m_myBuyOffers; }
    const std::vector<MarketOffer>& getMySellOffers() const { return m_mySellOffers; }
    const std::vector<MarketOffer>& getBuyHistory() const  { return m_historyBuy; }
    const std::vector<MarketOffer>& getSellHistory() const { return m_historySell; }
    const std::map<uint16_t, uint32_t>& getDepotItems() const { return m_depotItems; }
    const MarketDetail& getCurrentDetail() const { return m_currentDetail; }
    uint16_t getCurrentBrowseItem() const { return m_currentBrowseItem; }

private:
    MarketSystem() = default;
    ~MarketSystem() = default;

    bool m_isOpen = false;
    uint32_t m_accountBalance = 0;
    uint32_t m_coins = 0;
    uint16_t m_currentBrowseItem = 0;

    std::vector<MarketOffer> m_browseItemBuyOffers;
    std::vector<MarketOffer> m_browseItemSellOffers;
    std::vector<MarketOffer> m_myBuyOffers;
    std::vector<MarketOffer> m_mySellOffers;
    std::vector<MarketOffer> m_historyBuy;
    std::vector<MarketOffer> m_historySell;

    std::map<uint16_t, uint32_t> m_depotItems;
    std::vector<MarketItem>      m_marketItems;

    MarketDetail m_currentDetail;

    std::mutex m_mutex;
};

void InitMarketHooks();

#endif // __UI_MARKET_H__

