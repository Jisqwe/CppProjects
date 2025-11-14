#pragma once

#include <iostream>
#include <string>
#include "board.h"
#include "players.h"

class Game{
private:
    Player* player_;
    bool isStarted_;
    bool isFinished_;
    bool isStopped_;
public:
    Game();

    void ping();

    bool exit();

    void start();

    void stop();

    std::string lose();

    std::string win();
    
    std::string finished();

    void create(std::string mode);
    
    bool setWidth(uint64_t w);

    bool setHeight(uint64_t w);

    bool setCount(uint size, uint64_t count);

    bool setStrategy(std::string strat);

    bool setResult(std::string res);
    
    bool dump(std::string path);

    bool load(std::string path);

    uint64_t getWidth() const;

    uint64_t getHeight() const;

    uint64_t getCount(uint x) const;
    
    bool runFunction(std::vector<std::string> sepLine);
};