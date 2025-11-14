#include "../lib/ship.h"
#include "../lib/game.h"
#include "../lib/players.h"
#include "../lib/input-output.h"

#include <string>
#include <iostream>

int main() {
    Game game;
    while(true) {
        std::vector<std::string> sepLine;
        sepLine = getSeparatedLine();
        if (!game.runFunction(sepLine)){
            return EXIT_FAILURE;
        }
    }
    return 0;
}