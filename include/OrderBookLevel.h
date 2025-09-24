#pragma once

class OrderBookLevel
{
  private:
    double price_;
    double quantity_;

  public:
    OrderBookLevel(double price, double quantity);

    double getPrice() const;
    double getQuantity() const;

    void setPrice(double price);
    void setQuantity(double quantity);
};
