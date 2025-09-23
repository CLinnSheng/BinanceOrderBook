#include "OrderBookData.h"

OrderBookData::OrderBookData() : lastUpdateId_(0)
{
}

const BidsMap& OrderBookData::getBids() const
{
    return bids_;
}

const AsksMap& OrderBookData::getAsks() const
{
    return asks_;
}

long long OrderBookData::getLastUpdateId() const
{
    return lastUpdateId_;
}

BidsMap& OrderBookData::getBids()
{
    return bids_;
}

AsksMap& OrderBookData::getAsks()
{
    return asks_;
}

void OrderBookData::setLastUpdateId(long long id)
{
    lastUpdateId_ = id;
}

std::vector<OrderBookLevel> OrderBookData::getTopBids(int levels) const
{
    std::vector<OrderBookLevel> result;
    result.reserve(levels);

    auto it = bids_.begin();
    for (int i = 0; i < levels && it != bids_.end(); ++i, ++it)
    {
        result.emplace_back(it->first, it->second);
    }

    return result;
}

std::vector<OrderBookLevel> OrderBookData::getTopAsks(int levels) const
{
    std::vector<OrderBookLevel> result;
    result.reserve(levels);

    auto it = asks_.begin();
    for (int i = 0; i < levels && it != asks_.end(); ++i, ++it)
    {
        result.emplace_back(it->first, it->second);
    }

    return result;
}

void OrderBookData::clear()
{
    bids_.clear();
    asks_.clear();
    lastUpdateId_ = 0;
}