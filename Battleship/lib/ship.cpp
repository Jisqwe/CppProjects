#include "ship.h"

Ship::Ship(int size, char direction, Coordinates coordinates) : size_(size), direction_(direction), coordinates_(coordinates), hits_(0){
    if (size < 1 || size > 4){
        std::cerr << "Ship size should be 0 < size < 5" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (direction != 'h' && direction != 'v'){
        std::cerr << "Direction can be \'h\' or \'v\'" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

Coordinates Ship::getCoordinates(){
    return coordinates_;
}

int Ship::getSize(){
    return size_;
}

char Ship::getDirection(){
    return direction_;
}

bool Ship::isKilled(){
    return hits_ >= size_;
}

bool Ship::isHitted(Coordinates hit){
    if (direction_ == 'h'){
        return (hit.xCoordinate == coordinates_.xCoordinate && hit.yCoordinate >= coordinates_.yCoordinate && hit.yCoordinate < coordinates_.yCoordinate + size_);
    }
    if (direction_ == 'v'){
        return (hit.yCoordinate == coordinates_.yCoordinate && hit.xCoordinate >= coordinates_.xCoordinate && hit.xCoordinate < coordinates_.xCoordinate + size_);
    }
    return false;
}

bool Ship::wasHit(Coordinates hit){
    for (auto coor : hitsCoordinates_){
        if (coor.xCoordinate == hit.xCoordinate && coor.yCoordinate == hit.yCoordinate){
            return true;
        }
    }
    return false;
}

std::string Ship::hitStatus(Coordinates hit){
    if (wasHit(hit)){
        if (isKilled()){
            return "kill";
        } else {
            return "hit";
        }
    } else {
        if (isHitted(hit)){
            ++hits_;
            hitsCoordinates_.push_back(hit);
            if (isKilled()){
                return "kill";
            }
            return "hit";
        }
    }
    return "miss";
}