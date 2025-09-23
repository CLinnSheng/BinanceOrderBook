#pragma once

class OrderBookLevel
{
private:
    double price_;
    double quantity_;

public:
    OrderBookLevel(double price, double quantity);

    // Getters
    double getPrice() const;
    double getQuantity() const;

    // Setters
    void setPrice(double price);
    void setQuantity(double quantity);
};