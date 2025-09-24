#include "BinanceAPI.h"
#include <algorithm>
#include <cctype>
#include <curl/curl.h>
#include <iostream>
#include <json/json.h>
#include <thread>

// Callback function to write HTTP response data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *data)
{
    size_t totalSize = size * nmemb;
    data->append((char *)contents, totalSize);
    return totalSize;
}

std::string BinanceAPI::buildSnapshotUrl(const std::string &symbol, int limit)
{
    // Convert symbol to uppercase for Binance API
    std::string upperSymbol = symbol;
    std::transform(upperSymbol.begin(), upperSymbol.end(), upperSymbol.begin(), ::toupper);
    return "https://api.binance.com/api/v3/depth?symbol=" + upperSymbol + "&limit=" + std::to_string(limit);
}

std::string BinanceAPI::makeHttpRequest(const std::string &url)
{
    CURL *curl;
    CURLcode res;
    std::string response;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 second timeout
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Skip SSL verification for simplicity

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return "";
        }
    }

    return response;
}

DepthSnapshot BinanceAPI::parseSnapshotResponse(const std::string &response)
{
    DepthSnapshot snapshot;

    try
    {
        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(response, root))
        {
            std::cerr << "Failed to parse JSON response" << std::endl;
            return snapshot;
        }

        // Check for API error
        if (root.isMember("code") && root.isMember("msg"))
        {
            std::cerr << "Binance API error: " << root["msg"].asString() << " (code: " << root["code"].asInt() << ")"
                      << std::endl;
            return snapshot;
        }

        // Parse lastUpdateId
        if (!root.isMember("lastUpdateId"))
        {
            std::cerr << "Missing lastUpdateId in response" << std::endl;
            return snapshot;
        }
        snapshot.lastUpdateId = root["lastUpdateId"].asInt64();

        // Parse bids
        if (root.isMember("bids"))
        {
            Json::Value bidsArray = root["bids"];
            for (const auto &bid : bidsArray)
            {
                if (bid.isArray() && bid.size() >= 2)
                {
                    double price = std::stod(bid[0].asString());
                    double quantity = std::stod(bid[1].asString());
                    snapshot.bids[price] = quantity;
                }
            }
        }

        // Parse asks
        if (root.isMember("asks"))
        {
            Json::Value asksArray = root["asks"];
            for (const auto &ask : asksArray)
            {
                if (ask.isArray() && ask.size() >= 2)
                {
                    double price = std::stod(ask[0].asString());
                    double quantity = std::stod(ask[1].asString());
                    snapshot.asks[price] = quantity;
                }
            }
        }

        snapshot.isValid = true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing snapshot: " << e.what() << std::endl;
        snapshot.isValid = false;
    }

    return snapshot;
}

DepthSnapshot BinanceAPI::getDepthSnapshot(const std::string &symbol, int limit)
{
    std::string url = buildSnapshotUrl(symbol, limit);

    std::string response = makeHttpRequest(url);
    if (response.empty())
    {
        std::cerr << "Empty response from Binance API" << std::endl;
        return DepthSnapshot{};
    }

    return parseSnapshotResponse(response);
}

std::future<DepthSnapshot> BinanceAPI::getDepthSnapshotAsync(const std::string &symbol, int limit)
{
    return std::async(std::launch::async, [symbol, limit]() { return getDepthSnapshot(symbol, limit); });
}
