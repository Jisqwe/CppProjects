#pragma once
#include <iostream>
#include <ctime>
#include "help.h"
#include "board.h"

class Player{
protected:
    Board board_;
    uint64_t enemyKilled_[kMaxSizeOfShip];
    std::vector<Ship> enemyKilledShips_;
    std::vector<std::vector<bool>> enemyBoard_;
    std::string strategy_ = "custom";
    Coordinates lastOrderedShot_{0, 0};
    Coordinates lastShot_{0, 0};
    bool isLoaded_;
public:
    Player(){};

    virtual bool setWidth(uint64_t w) = 0;

    virtual bool setHeight(uint64_t h) = 0;

    virtual bool setCount(uint x, uint64_t w) = 0;

    void setEnemyBoard();

    bool setStrategy(std::string strat);

    bool setResult(std::string res);

    std::string shot();

    std::string getShot(Coordinates coord);

    uint64_t getWidth() const;

    uint64_t getHeight() const;

    uint64_t getCount(uint x) const;

    void setShips();

    bool isFinished();

    bool isLost();

    bool isWin();

    bool dump(std::string path);

    bool load(std::string path);

    void clear();
};

class Master : public Player{
public:
    Master(){
        setWidth(10);
        setHeight(10);
        setCount(1, 4);
        setCount(2, 3);
        setCount(3, 2);
        setCount(4, 1);
    }

    bool setWidth(uint64_t w) override;

    bool setHeight(uint64_t h) override;

    bool setCount(uint x, uint64_t count) override;
};

class Slave : public Player{
public:
    Slave(){};

    bool setWidth(uint64_t w) override;

    bool setHeight(uint64_t h) override;

    bool setCount(uint x, uint64_t count) override;
};