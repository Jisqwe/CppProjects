#include "players.h"
#include "input-output.h"

void Player::setEnemyBoard(){
    enemyBoard_.reserve(getHeight());
    for (int i = 0; i < getHeight(); ++i) {
        std::vector<bool> innerVector;
        innerVector.reserve(getWidth());
        innerVector.resize(getWidth(), false);
        enemyBoard_.push_back(innerVector);
    }
}

void Player::clear(){
    for (int i = 0; i < kMaxSizeOfShip; ++i){
        board_.setShipsCount(i, 0);
        board_.setShipsMaxCount(i, 0);
    }
    board_.clear();
}

bool Player::setWidth(uint64_t w){
    return board_.setWidth(w);
}

bool Player::setHeight(uint64_t h){
    return board_.setHeight(h);
}

bool Player::setCount(uint x, uint64_t count){
    return board_.setCount(x, count);
}

bool Player::setStrategy(std::string strat){
    if (strat == "custom" || strat == "ordered"){
        strategy_ = strat;
        setString("ok");
        return true;
    }
    setString("failed");
    return false;
}

uint64_t Player::getWidth() const{
    return board_.getWidth(); 
}

uint64_t Player::getHeight() const{
    return board_.getHeight(); 
}

uint64_t Player::getCount(uint x) const{
    return board_.getCount(x);
}

bool Master::setHeight(uint64_t h){
    if (board_.getHeight() == 0 || isLoaded_){
        return Player::setHeight(h);
    }
    return false;
}

bool Master::setWidth(uint64_t w){
    if (board_.getWidth() == 0 || isLoaded_){
        return Player::setWidth(w);
    }
    return false;
}

bool Master::setCount(uint x, uint64_t count){
    if (board_.getCount(x) == 0 || isLoaded_){
        return Player::setCount(x, count);
    }
    return false;
}

bool Slave::setHeight(uint64_t h){
    return Player::setHeight(h);
}

bool Slave::setWidth(uint64_t w){
    return Player::setWidth(w);
}

bool Slave::setCount(uint x, uint64_t count){
    return Player::setCount(x, count);
}

std::string Player::shot(){
    std::srand(std::time(0));
    if (strategy_ == "custom"){
        lastShot_.xCoordinate = (rand() * rand()) % board_.getWidth();
        lastShot_.yCoordinate = (rand() * rand()) % board_.getHeight();
        return std::to_string(lastShot_.xCoordinate) + " " + std::to_string(lastShot_.yCoordinate);
    }
    if (lastOrderedShot_.xCoordinate == board_.getWidth()){
        if (lastOrderedShot_.yCoordinate == board_.getHeight() - 1){
            setString("Game should be ended");
            exit(EXIT_FAILURE);
        }
        lastOrderedShot_.xCoordinate = 0;
        ++lastOrderedShot_.yCoordinate;
    }
    lastShot_.xCoordinate = lastOrderedShot_.xCoordinate;
    lastShot_.yCoordinate = lastOrderedShot_.yCoordinate;
    return std::to_string(lastOrderedShot_.xCoordinate++) + " " + std::to_string(lastOrderedShot_.yCoordinate);
}

std::string Player::getShot(Coordinates coord){
    for (auto& ship : board_.getShips()){
        if (ship.hitStatus(coord) == "hit" || ship.hitStatus(coord) == "kill"){
            return ship.hitStatus(coord);
        }
    }
    return "miss";
}

bool Player::dump(std::string path){
    std::string ans = std::to_string(board_.getWidth()) + " " + std::to_string(board_.getHeight()) + '\n';
    std::vector<Ship> ships = board_.getShips();
    for (int i = 0; i < ships.size() - 1; ++i){
        ans += std::to_string(ships[i].getSize()) + " " + ships[i].getDirection() + " "
                            + std::to_string(ships[i].getCoordinates().xCoordinate) + " "
                             + std::to_string(ships[i].getCoordinates().yCoordinate) + '\n';
    }
    ans += std::to_string(ships[ships.size() - 1].getSize()) + " " + ships[ships.size() - 1].getDirection() + " "
                            + std::to_string(ships[ships.size() - 1].getCoordinates().xCoordinate) + " "
                             + std::to_string(ships[ships.size() - 1].getCoordinates().yCoordinate);
    dumpInFile(ans, path);
    return true;
}

bool Player::load(std::string path){
    isLoaded_ = true;
    std::ifstream file(path);
    std::vector<std::string> line = getSeparatedLine(file);
    setWidth(std::stoull(line[0]));
    setHeight(std::stoull(line[1]));
    line = getSeparatedLine(file);
    clear();
    while (line.size()){
        setCount(std::stoull(line[0]), getCount(std::stoull(line[0])) + 1);
        Ship ship = Ship(std::stoull(line[0]), line[1][0], {std::stoull(line[2]), std::stoull(line[3])});
        board_.setShip(ship);
        line = getSeparatedLine(file);
    }
    isLoaded_ = false;
    return true;
}

bool Player::isLost(){
    int count = 0;
    for (auto& ship : board_.getShips()){
        if (ship.isKilled()){
            ++count;
        }
    }
    return (count == board_.getShips().size());
}

bool Player::isWin(){
    bool flag = true;
    for (int i = 0; i < kMaxSizeOfShip; ++i){
        if (enemyKilled_[i] != board_.getCount(i + 1)){
            flag = false;
        }
    }
    return flag;
}

bool Player::isFinished(){
    return isWin() || isLost();
}

void Player::setShips(){
    uint64_t x{};
    uint64_t y{};
    uint64_t diff;
    uint64_t s = 0;
    for (size_t i = 1; i <= kMaxSizeOfShip; ++i){
        s += board_.getCount(i) * i;
    }
    if (s > board_.getHeight() * board_.getWidth()){
        std::cerr << "Can't place all ships" << std::endl;
        exit(EXIT_FAILURE);
    }
    while (true){
        if (board_.getHeight() >= board_.getWidth()){
            bool flag = true;
            for (int i = 1; i <= kMaxSizeOfShip; ++i){
                if (board_.getCurCount(i) < board_.getCount(i)){
                    flag = false;
                    break;
                }
            }
            if (flag){
                return;
            }
            if (x >= board_.getWidth()){
                std::cerr << "Can't place all ships" << std::endl;
                exit(EXIT_FAILURE);
            }
            diff = board_.getHeight() - y;
            if (diff > 4){
                diff = 4;
            }
            if (diff >= 1){
                while (diff){
                    if (board_.getCurCount(diff) == board_.getCount(diff)){
                        if (!diff){
                            break;
                        }
                        --diff;
                    } else {
                        board_.setShip(Ship(diff, 'v', {x, y}));
                        y += diff + 1;
                        if (y >= board_.getHeight()){
                            y = 0;
                            x += 2;
                        }
                        break;
                    }
                }
            } else {
                x += 2;
                y = 0;
                continue;
            }
        } else if (board_.getHeight() < board_.getWidth()){
            bool flag = true;
            for (int i = 1; i <= kMaxSizeOfShip; ++i){
                if (board_.getCurCount(i) < board_.getCount(i)){
                    flag = false;
                    break;
                }
            }
            if (flag){
                return;
            }
            if (y >= board_.getHeight()){
                std::cerr << "Can't place all ships" << std::endl;
                exit(EXIT_FAILURE);
            }
            diff = board_.getWidth() - x;
            if (diff > 4){
                diff = 4;
            }
            if (diff >= 1){
                while (diff){
                    if (board_.getCurCount(diff - 1) == board_.getCount(diff - 1)){
                        if (!diff){
                            break;
                        }
                        --diff;
                    } else {
                        board_.setShip(Ship(diff, 'h', {x, y}));
                        x += diff + 1;
                        if (x >= board_.getWidth()){
                            x = 0;
                            y += 2;
                        }
                        break;
                    }
                }
            } else {
                y += 2;
                x = 0;
                continue;
            }
        }
    }
    return;
}

bool Player::setResult(std::string res){
    if (res == "miss"){
        return true;
    }
    if (res == "hit"){
        enemyBoard_[lastShot_.yCoordinate][lastShot_.xCoordinate] = true;
        return true;
    } else if (res == "kill"){ 
        for (auto ship : enemyKilledShips_){
            if (ship.isHitted({lastShot_.yCoordinate, lastShot_.xCoordinate})){
                return true;
            }
        }
        enemyBoard_[lastShot_.yCoordinate][lastShot_.xCoordinate] = true;
        int temp = 0;
        int counter = 0;
        bool flag = false;

        if (lastShot_.xCoordinate >= 1 && enemyBoard_[lastShot_.yCoordinate][lastShot_.xCoordinate - 1]){
            while (enemyBoard_[lastShot_.yCoordinate][lastShot_.xCoordinate - temp] == true){
                ++temp;
                if (lastShot_.xCoordinate < temp){
                    break;
                }
            }
            Coordinates first{lastShot_.yCoordinate,lastShot_.xCoordinate + 1 - temp};
            while (lastShot_.xCoordinate < getWidth() + temp + 1 && enemyBoard_[lastShot_.yCoordinate][lastShot_.xCoordinate - temp + 1] == true){
                ++counter;
                --temp;
            }
            Ship ship{counter, 'h', first};
            ++enemyKilled_[counter - 1];
            enemyKilledShips_.push_back(ship);
            return true;
        }

        if (lastShot_.yCoordinate >= 1 && enemyBoard_[lastShot_.yCoordinate - 1][lastShot_.xCoordinate]){
            while (enemyBoard_[lastShot_.yCoordinate - 1 - temp][lastShot_.xCoordinate] == true){
                ++temp;
                if (lastShot_.yCoordinate < temp + 1){
                    break;
                }
            }
            Coordinates first{lastShot_.yCoordinate - temp, lastShot_.xCoordinate};
            while (lastShot_.yCoordinate < getHeight() + temp && enemyBoard_[lastShot_.yCoordinate - temp][lastShot_.xCoordinate] == true){
                ++counter;
                --temp;
            }
            Ship ship{counter, 'v', first};
            ++enemyKilled_[counter - 1];
            enemyKilledShips_.push_back(ship);
            return true;
        }
        
        if (lastShot_.xCoordinate < getWidth() - 1 && enemyBoard_[lastShot_.yCoordinate][lastShot_.xCoordinate + 1]){
            int counter = 0;
            Coordinates first{lastShot_.yCoordinate, lastShot_.xCoordinate};
            while (lastShot_.xCoordinate + counter < getWidth() && enemyBoard_[lastShot_.yCoordinate][lastShot_.xCoordinate + counter]){
                ++counter;
            }
            Ship ship{counter, 'h', first};
            ++enemyKilled_[counter - 1];
            enemyKilledShips_.push_back(ship);
            return true;
        }

        if (lastShot_.yCoordinate < getHeight() - 1 && enemyBoard_[lastShot_.yCoordinate + 1][lastShot_.xCoordinate]){
            int counter = 0;
            Coordinates first{lastShot_.yCoordinate, lastShot_.xCoordinate};
            while (lastShot_.yCoordinate + counter < getHeight() && enemyBoard_[lastShot_.yCoordinate + counter][lastShot_.xCoordinate]){
                ++counter;
            }
            Ship ship{counter, 'v', first};
            ++enemyKilled_[counter - 1];
            enemyKilledShips_.push_back(ship);
            return true;
        }
        Ship ship = {1, 'v', {lastShot_.yCoordinate, lastShot_.xCoordinate}};
        ++enemyKilled_[0];
        enemyKilledShips_.push_back(ship);
        return true;
    }
    std::cerr << "Undefind result of shot" << std::endl;
    return false;
}