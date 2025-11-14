#include "game.h"
#include "players.h"
#include "ship.h"
#include "input-output.h"

Game::Game(){
    isStarted_ = false;
    isFinished_ = false;
    isStopped_ = false;
}

bool Game::runFunction(std::vector<std::string> sepLine){
    std::string cmd = getWord(sepLine);
    if (cmd == "exit"){
        return exit();
    } else if (cmd == "ping"){
        ping();
        return true;
    } else if (cmd == "start"){
        start();
        return true;
    } else if (cmd == "stop"){
        stop();
        return true;
    } else if (cmd == "lose"){
        setString(lose());
        return true;
    } else if (cmd == "win"){
        setString(win());
        return true;
    } else if (cmd == "finished"){
        setString(finished());
        return true;
    } else if (cmd == "create"){
        std::string mode;
        mode = getWord(sepLine);
        create(mode);
        return true;
    } else if (cmd == "get"){
        std::string parameter;
        parameter = getWord(sepLine);
        if (parameter == "count"){
            long long count;
            count = std::stoll(getWord(sepLine));
            setString(std::to_string(getCount(count)));
            return true;
        } else if (parameter == "width"){
            setString(std::to_string(getWidth()));
            return true;
        } else if (parameter == "height"){
            setString(std::to_string(getHeight()));
            return true;
        } else {
            setString("undefind command");
            return false;
        }
    } else if (cmd == "dump"){
        std::string path = getWord(sepLine);
        return dump(path);
    } else if (cmd == "load"){
        std::string path = getWord(sepLine);
        return load(path);
    }
    if (isStopped_){
        std::cout << "The game was stopped, illegal command" << std::endl;
        return false;
    }
    if (cmd == "set"){
        std::string parameter;
        parameter = getWord(sepLine);
        if (parameter == "width"){
            uint64_t w = std::stoull(getWord(sepLine));
            if (setWidth(w)){
                setString("ok");
                return true;
            } else {
                setString("faild");
                return false;
            }
        } else if (parameter == "height"){
            uint64_t h = std::stoull(getWord(sepLine));
            if (setHeight(h)){
                setString("ok");
                return true;
            } else {
                setString("faild");
                return false;
            }
        } else if (parameter == "count"){
            long long size = std::stoll(getWord(sepLine));
            uint64_t count = std::stoull(getWord(sepLine));
            if (setCount(size, count)){
                setString("ok");
                return true;
            } else {
                setString("faild");
                return false;
            }
        } else if (parameter == "result"){
            std::string res = getWord(sepLine);
            return setResult(res);
        } else if (parameter == "strategy"){
            std::string strat = getWord(sepLine);
            return setStrategy(strat);
        }
    } else if (cmd == "shot"){
        if (sepLine.empty()){
            setString(player_->shot());
            return true;
        }
        Coordinates coord;
        std::string tempX = getWord(sepLine);
        std::string tempY = getWord(sepLine);
        if (tempX[0] == '-' || tempY[0] == '-'){
            setString("Wrong format of coordinates, should be > 0");
            return false;
        }
        coord.xCoordinate = std::stoull(tempX);
        coord.yCoordinate = std::stoull(tempY);
        setString(player_->getShot(coord));
        return true;
    }
    setString("undefind command");
    return false;
}

uint64_t Game::getWidth() const{
    return player_->getWidth();
}

uint64_t Game::getHeight() const{
    return player_->getHeight();
}

uint64_t Game::getCount(uint x) const{
    return player_->getCount(x);
}

bool Game::setWidth(uint64_t w){
    return player_->setWidth(w);
}

bool Game::setHeight(uint64_t h){
    return player_->setHeight(h);
}

bool Game::setCount(uint size, uint64_t count){
    return player_->setCount(size, count);
}

bool Game::setStrategy(std::string strat){
    return player_->setStrategy(strat);
}

void Game::ping(){
    setString("pong");
}

bool Game::exit(){
    setString("ok");
    std::exit(EXIT_SUCCESS);
}

void Game::start(){
    isStarted_ = true;
    player_->setShips();
    player_->setEnemyBoard();
    setString("ok");
}

void Game::stop(){
    isStopped_ = true;
    setString("ok");
}

void Game::create(std::string mode){
    if (mode == "master"){
        player_ = new Master();
        setString("ok");
    } else if (mode == "slave"){
        player_ = new Slave();
        setString("ok");
    } else {
        setString("failed");
    }
}

bool Game::dump(std::string path){
    player_->dump(path);
    setString("ok");
    return true;
}

bool Game::load(std::string path){
    player_->load(path);
    setString("ok");
    return true;
}

std::string Game::lose(){
    if( player_->isLost()){
        return "yes";
    }
    return "no";
}

std::string Game::win(){
    if (player_->isWin()){
        return "yes";
    }
    return "no";
}

std::string Game::finished(){
    if (player_->isFinished()){
        return "yes";
    }
    return "no";
}

bool Game::setResult(std::string res){
    setString("ok");
    return player_->setResult(res);
}