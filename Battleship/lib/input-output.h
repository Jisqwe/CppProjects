#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>

std::vector<std::string> getSeparatedLine();

std::vector<std::string> getSeparatedLine(std::ifstream &file);

std::string getWord(std::vector<std::string> &separatedLine);

bool setString(std::string curString);

bool dumpInFile(std::string str, std::string path);