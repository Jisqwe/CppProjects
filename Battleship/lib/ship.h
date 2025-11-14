#pragma once
#include <iostream>
#include <vector>
#include "help.h"

class Ship{
public:
    Ship(int size, char direction, Coordinates coordinates);

    ~Ship(){};

    bool isHitted(Coordinates hit);

    bool wasHit(Coordinates hit);

    bool isKilled();

    Coordinates getCoordinates();

    int getSize();

    char getDirection();

    std::string hitStatus(Coordinates hit);
private:
    const int size_;
    const char direction_;
    const Coordinates coordinates_;
    int hits_;
    std::vector<Coordinates> hitsCoordinates_;
};