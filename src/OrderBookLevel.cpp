#include "OrderBookLevel.h"

OrderBookLevel::OrderBookLevel(double price, double quantity)
    : price_(price), quantity_(quantity)
{
}

double OrderBookLevel::getPrice() const
{
    return price_;
}

double OrderBookLevel::getQuantity() const
{
    return quantity_;
}

void OrderBookLevel::setPrice(double price)
{
    price_ = price;
}

void OrderBookLevel::setQuantity(double quantity)
{
    quantity_ = quantity;
}