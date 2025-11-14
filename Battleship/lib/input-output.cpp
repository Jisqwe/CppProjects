#include "input-output.h"

std::vector<std::string> getSeparatedLine(){
    std::string line;
    std::string word;
    std::vector<std::string> separatedLine;
    std::getline(std::cin, line);
    std::istringstream stream(line);
    while (stream >> word){
        separatedLine.push_back(word);
    }
    return separatedLine;
}

std::vector<std::string> getSeparatedLine(std::ifstream &file) {
    std::string line;
    if (!std::getline(file, line)) {
        return {};
    }
    
    std::vector<std::string> sepLine;
    std::istringstream stream(line);
    std::string word;
    while (stream >> word) {
        sepLine.push_back(word);
    }
    return sepLine;
}

std::string getWord(std::vector<std::string> &separatedLine){
    if (!separatedLine.empty()){
        std::string word = separatedLine[0];
        separatedLine.erase(separatedLine.begin());
        return word;
    }
    std::cout << "Nothing to get, vector is empty" << std::endl;
    exit(EXIT_FAILURE);
}

bool dumpInFile(std::string str, std::string path){
    std::ofstream file(path);
    if (file){
        file << str;
        return true;
    }
    return false;
}

bool setString(std::string curString){
    std::cout << curString + "\n";
    return true;
}