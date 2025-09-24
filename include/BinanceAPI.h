#pragma once

#include "utils.h"
#include <future>
#include <string>

struct DepthSnapshot
{
    BidsMap bids;
    AsksMap asks;
    long long lastUpdateId = 0;
    bool isValid = false;
};

class BinanceAPI
{
  public:
    static std::future<DepthSnapshot> getDepthSnapshotAsync(const std::string &symbol, int limit = 5000);
    static DepthSnapshot getDepthSnapshot(const std::string &symbol, int limit = 5000);

  private:
    static DepthSnapshot parseSnapshotResponse(const std::string &response);
    static std::string makeHttpRequest(const std::string &url);
    static std::string buildSnapshotUrl(const std::string &symbol, int limit);
};
