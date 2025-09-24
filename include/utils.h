#pragma once
#include <map>

// Bids: highest to lowest (reverse order)
// Asks: lowest to highest (normal order)
using BidsMap = std::map<double, double, std::greater<double>>;
using AsksMap = std::map<double, double>;

enum PriceChange
{
    CONSTANT,
    INCREASE,
    DECREASE
};

enum class SyncState
{
    INITIALIZING,      // Starting up
    BUFFERING,         // Buffering events, waiting for snapshot
    SNAPSHOT_RECEIVED, // Snapshot received, processing buffer
    SYNCHRONIZED,      // Fully synchronized, real-time updates
    ERROR_STATE        // Error occurred, need reset
};
