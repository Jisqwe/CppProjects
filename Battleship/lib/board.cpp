#include "board.h"

void Board::setShipsMaxCount(uint i, int64_t value){
    shipsMaxCount_[i] = value;
}

void Board::setShipsCount(uint i, int64_t value){
    shipsCount_[i] = value;
}

void Board::clear(){
    ships_.clear();
}

bool Board::setWidth(uint64_t w){
    if (w > 0){
        width_ = w;
        return true;
    }
    return false;
}

bool Board::setHeight(uint64_t h){
    if (h > 0){
        height_ = h;
        return true;
    }
    return false;
}

bool Board::setCount(uint x, uint64_t count){
    if (x > kMaxSizeOfShip){
        return false;
    }
    shipsMaxCount_[x - 1] = count;
    return true;
}

uint64_t Board::getWidth() const{
    return width_;
}

uint64_t Board::getHeight() const{
    return height_;
}

uint64_t Board::getCount(uint x) const{
    if (x > 0 && x < 5){
        return shipsMaxCount_[x - 1];
    } else {
        std::cerr << "Size of ships is 0 < size < 5" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

uint64_t Board::getCurCount(uint x) const{
    if (x > 0 && x < 5){
        return shipsCount_[x - 1];
    } else {
        std::cerr << "Size of ships is 0 < size < 5" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

std::vector<Ship>& Board::getShips() {
    return ships_;
}

bool Board::setShip(Ship ship){
    if (shipsCount_[ship.getSize() - 1] < shipsMaxCount_[ship.getSize() - 1]){
        ships_.push_back(ship);
        ++shipsCount_[ship.getSize() - 1];
        return true;
    } else {
        return false;
    }
}