#pragma once

#include <iostream>

const uint kMaxSizeOfShip = 4;
const uint kCountOfSides = 4;

struct Coordinates{
    bool operator=(const Coordinates& other){
        return (xCoordinate == other.xCoordinate) && (yCoordinate == other.yCoordinate);
    }
    uint64_t xCoordinate;
    uint64_t yCoordinate;
};