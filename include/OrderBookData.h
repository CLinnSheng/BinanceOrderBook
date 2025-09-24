#pragma once

#include "OrderBookLevel.h"
#include <map>
#include <vector>

// Bids: highest to lowest (reverse order)
// Asks: lowest to highest (normal order)
using BidsMap = std::map<double, double, std::greater<double>>;
using AsksMap = std::map<double, double>;

class OrderBookData
{
  private:
    BidsMap bids_;
    AsksMap asks_;
    long long lastUpdateId_;

  public:
    OrderBookData();

    const BidsMap &getBids() const;
    const AsksMap &getAsks() const;
    long long getLastUpdateId() const;

    BidsMap &getBids();
    AsksMap &getAsks();
    void setLastUpdateId(long long id);

    std::vector<OrderBookLevel> getTopBids(int levels = 5) const;
    std::vector<OrderBookLevel> getTopAsks(int levels = 5) const;
    void clear();
};
