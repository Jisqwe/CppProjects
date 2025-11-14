#pragma once
#include "ship.h"

class Board{
private:
    uint64_t width_;
    uint64_t height_;
    std::vector<Ship> ships_;
    uint64_t shipsMaxCount_[kMaxSizeOfShip];
    uint64_t shipsCount_[kMaxSizeOfShip];
public:

    void setShipsMaxCount(uint i, int64_t value);

    void setShipsCount(uint i, int64_t value);

    void clear();

    uint64_t getWidth() const;

    uint64_t getHeight() const;

    uint64_t getCount(uint x) const;

    uint64_t getCurCount(uint x) const;

    std::vector<Ship>& getShips();

    bool setWidth(uint64_t w);

    bool setHeight(uint64_t w);

    bool setCount(uint x, uint64_t count);

    bool setShip(Ship ship);
};