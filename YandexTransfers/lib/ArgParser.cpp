#include "ArgParser.h"
#include <iostream>

using namespace ArgumentParser;

bool ArgParser::isCorrect(){
    for (auto arg : args){
        if (!arg->isValid()){
            return false;
        }
    }
    return true;
}

void ArgParser::setStored(){
    for (auto arg : args){
        arg->store();
    }
}

bool ArgParser::Parse(std::vector<std::string> words){
    StringArgument* curString = nullptr;
    IntArgument* curInt = nullptr;
    for (std::string word : words){
        if (curInt != nullptr){
            bool check = false;
            if (isNumber(word)){
                for (auto arg : args){
                    if (auto intArg = dynamic_cast<IntArgument*>(arg)){
                        if (curInt == intArg){
                            intArg->values.push_back(std::stoi(word));
                            curInt = nullptr;
                            check = true;
                            break;
                        }
                    }
                }
                if (check){
                    continue;
                } else {
                    return false;
                }
            }
        }
        if (curString != nullptr){
            bool checkStr = false;
            for (auto arg : args){
                if (auto strArg = dynamic_cast<StringArgument*>(arg)){
                    if (strArg == curString){
                        strArg->values.push_back(word);
                        curString = nullptr;
                        checkStr = true;
                        break;
                    }
                }
            }
            if (checkStr){
                continue;
            } else {
                return false;
            }
        }
        if (word.size() > 2 && word.substr(0, 2) == "--" || word.size() > 1 && word[0] == '-'){
            if (isHelp){
                if (word.find(helpName) != std::string::npos || word.find(longHelpName) != std::string::npos){
                    std::cout << HelpDescription();
                    isHelpUsed = true;
                    return true;
                }
            }
            if (word.find('=') != std::string::npos){
                std::string temp = word.substr(word.find('=') + 1);
                for (auto arg : args){
                    if (arg->wordFind(word)){
                        break;
                    }
                }
            } else {
                for (auto arg : args){
                    if (arg->wordConstFind(word)){
                        if (auto strArg = dynamic_cast<StringArgument*>(arg)) {
                            curString = strArg;
                            break;
                        } else if (auto intArg = dynamic_cast<IntArgument*>(arg)) {
                            curInt = intArg;
                            break;
                        } else if (auto flagArg = dynamic_cast<FlagArgument*>(arg)) {
                            flagArg->setValue(true);
                        }
                    }
                }
            }
        } else {
            for (auto arg : args){
                if (isNumber(word)){
                    if (auto intArg = dynamic_cast<IntArgument*>(arg)){
                        if (intArg->isPositional){
                            intArg->values.push_back(std::stoi(word));
                            break;
                        }
                    }
                } else {
                    if (auto strArg = dynamic_cast<StringArgument*>(arg)){
                        if (strArg->isPositional){
                            strArg->values.push_back(word);
                            break;
                        }
                    }
                }
            }
        }
    }
    setStored();
    return isCorrect();
}

bool ArgParser::Parse(int argc, char** argv){
    std::vector<std::string> temp;
    for (int i = 0; i < argc; ++i){
        temp.push_back(argv[i]);
    }
    return Parse(temp);
}

bool ArgParser::isNumber(const std::string& num){
    bool sign = false;
    if (num.size() == 0){
        return false;
    }
    if (num[0] == '-'){
        if (num.size() < 2){
            return false;
        }
        sign = true;
    }
    for (int i = sign; i < num.size(); ++i){
        if (!isdigit(num[i])){
            return false;
        }
    }
    return true;
}

ArgParser::StringArgument& ArgParser::AddStringArgument(const std::string& longName, const std::string& desc){
    StringArgument* stringArg = new StringArgument;
    stringArg->longName = longName;
    stringArg->description = desc;
    args.push_back(stringArg);
    return *stringArg;
}

ArgParser::StringArgument& ArgParser::AddStringArgument(const char name, const std::string& longName, const std::string& desc){
    StringArgument* stringArg = new StringArgument;
    stringArg->name = name;
    stringArg->longName = longName;
    stringArg->description = desc;
    args.push_back(stringArg);
    return *stringArg;
}

std::string ArgParser::GetStringValue(const std::string& argName, const int index) const{
    for (auto pure_arg : args){
        StringArgument* arg = static_cast<StringArgument*>(pure_arg);
        if (arg->name == argName || arg->longName == argName){
            if (!arg->values.empty()){
                if (index >= arg->values.size()){
                    std::cerr << "Index out of range of count string arguments\n";
                    std::exit(EXIT_FAILURE);
                }
                return arg->values[index];
            } else {
                if (!arg->defaultValue.empty()){
                    return arg->defaultValue;
                } else {
                    std::cerr << "Argument for " << argName << " not found\n";
                    std::exit(EXIT_FAILURE);
                }
            }
        }
    }
    return "";
}

ArgParser::IntArgument& ArgParser::AddIntArgument(const std::string& longName, const std::string& desc){
    IntArgument* intArg = new IntArgument;
    intArg->longName = longName;
    intArg->description = desc;
    args.push_back(intArg);
    return *intArg;
}

ArgParser::IntArgument& ArgParser::AddIntArgument(const char name, const std::string& longName, const std::string& desc){
    IntArgument* intArg = new IntArgument;
    intArg->name = name;
    intArg->longName = longName;
    intArg->description = desc;
    args.push_back(intArg);
    return *intArg;
}

int ArgParser::GetIntValue(const std::string& argName, const int index) const{
    for (auto pure_arg : args){
        IntArgument* arg = static_cast<IntArgument*>(pure_arg);
        if (arg->name == argName || arg->longName == argName){
            if (!arg->values.empty()){
                if (index >= arg->values.size()){
                    std::cerr << "Index out of range of count string arguments\n";
                    std::exit(EXIT_FAILURE);
                }
                return arg->values[index];
            } else {
                if (arg->defaultValue){
                    return arg->defaultValue;
                } else {
                    std::cerr << "Argument for " << argName << " not found\n";
                    std::exit(EXIT_FAILURE);
                }
            }
        }
    }
    return 0;
}

ArgParser::FlagArgument& ArgParser::AddFlag(const std::string& longName, const std::string& desc){
    FlagArgument* flagArg = new FlagArgument;
    flagArg->longName = longName;
    flagArg->description = desc;
    args.push_back(flagArg);
    return *flagArg;
}

ArgParser::FlagArgument& ArgParser::AddFlag(const char name, const std::string& longName, const std::string& desc){
    FlagArgument* flagArg = new FlagArgument;
    flagArg->name = name;
    flagArg->longName = longName;
    flagArg->description = desc;
    args.push_back(flagArg);
    return *flagArg;
}

bool ArgParser::GetFlag(const std::string& argName) const{
    for (auto pure_arg : args){
        FlagArgument* arg = static_cast<FlagArgument*>(pure_arg);
        if (arg->name == argName || arg->longName == argName){
            if (arg->value){
                return arg->value;
            } else {
                if (arg->defaultValue){
                    return arg->defaultValue;
                } else {
                    std::cerr << "Argument for " << argName << " not found\n";
                    std::exit(EXIT_FAILURE);
                }
            }
        }
    }
    return false;
}

void ArgParser::AddHelp(const std::string& longName, const std::string& desc){
    longHelpName = longName;
    helpDescription = desc;
    isHelp = true;
}

void ArgParser::AddHelp(const char name, const std::string& longName, const std::string& desc){
    helpName = name;
    longHelpName = longName;
    helpDescription = desc;
    isHelp = true;
}

bool ArgParser::Help() const{
    return isHelpUsed;
}

std::string ArgParser::HelpDescription() const{
    std::string description;
    description += progName + '\n' + helpDescription + ": \n\n";
    for (auto arg : args){
        if (!arg->name.empty()){
            description += " -" + arg->name + ", ";
        } else {
            description += "\t ";
        }
        description += "--" + arg->longName + ", ";
        description += arg->description + " ";
        if (arg->isDefault || arg->isMultiValue || arg->isPositional){
            description += '[';
            if (arg->isDefault){
                description += "default = " + arg->defaultValue;
                if (arg->isMultiValue || arg->isPositional){
                    description += ", ";
                }
            }
            if (arg->isMultiValue){
                description += "repeated, min args = " + std::to_string(arg->minCount);
                if (arg->isPositional){
                    description += ", ";
                }
            }
            if (arg->isPositional){
                description += "positional";
            }
            description += ']';
        }
        description += '\n';
    }
    return description;
}

bool ArgumentParser::ArgParser::StringArgument::wordFind(std::string word){
    std::string temp = word.substr(word.find('=') + 1);
    if (!ArgParser{"name"}.isNumber(temp)){
        if (word.find(longName) != std::string::npos || word.find(name) != std::string::npos){
            values.push_back(temp);
            return true;
        }
    }
    return false;
}

bool ArgumentParser::ArgParser::StringArgument::wordConstFind(std::string word){
    return word.find(longName) != std::string::npos || word.find(name) != std::string::npos;
}

bool ArgumentParser::ArgParser::StringArgument::isValid() const{
    if (values.size() < minCount) {
        return false;
    }
    if (values.empty() && !isDefault) {
        return false;
    }
    return true;
}

void ArgumentParser::ArgParser::StringArgument::store(){
    if (isStored && !values.empty()) {
        if (storedValues != nullptr) {
            *storedValues = values;
        }
        if (storedValue != nullptr) {
            *storedValue = values[0];
        }
    }
}

ArgumentParser::ArgParser::StringArgument& ArgumentParser::ArgParser::StringArgument::Default(const std::string& def){
    isDefault = true;
    defaultValue = def;
    return *this;
}

ArgumentParser::ArgParser::StringArgument& ArgumentParser::ArgParser::StringArgument::StoreValue(std::string& value){
    isStored = true;
    storedValue = &value;
    return *this;
}

ArgumentParser::ArgParser::StringArgument& ArgumentParser::ArgParser::StringArgument::StoreValues(std::vector<std::string>& values){
    isStored = true;
    storedValues = &values;
    return *this;
}

ArgumentParser::ArgParser::StringArgument& ArgumentParser::ArgParser::StringArgument::MultiValue() {
    return MultiValue(1);
}

ArgumentParser::ArgParser::StringArgument& ArgumentParser::ArgParser::StringArgument::MultiValue(const int& count) {
    isMultiValue = true;
    minCount = count;
    return *this;
}

ArgumentParser::ArgParser::StringArgument& ArgumentParser::ArgParser::StringArgument::Positional() {
    isPositional = true;
    return *this;
}

bool  ArgumentParser::ArgParser::IntArgument::wordFind(std::string word){
    std::string temp = word.substr(word.find('=') + 1);
    if (ArgParser{"name"}.isNumber(temp)){
        if (word.find(longName) != std::string::npos || word.find(name) != std::string::npos){
            values.push_back(std::stoi(temp));
            return true;
        }
    }
    return false;
}

bool ArgumentParser::ArgParser::IntArgument::wordConstFind(std::string word){
    return word.find(longName) != std::string::npos || word.find(name) != std::string::npos;
}

void ArgumentParser::ArgParser::IntArgument::store() {
    if (isStored && !values.empty()) {
        if (storedValues != nullptr) {
            *storedValues = values;
        }
        if (storedValue != nullptr) {
            *storedValue = values[0];
        }
    }
}
bool ArgumentParser::ArgParser::IntArgument::isValid() const {
    if (values.size() < minCount) {
        return false;
    }
    if (values.empty() && !isDefault) {
        return false;
    }
    return true;
}

ArgumentParser::ArgParser::IntArgument& ArgumentParser::ArgParser::IntArgument::Default(const int& def){
    isDefault = true;
    defaultValue = def;
    return *this;
}

ArgumentParser::ArgParser::IntArgument& ArgumentParser::ArgParser::IntArgument::StoreValue(int& value){
    isStored = true;
    storedValue = &value;
    return *this;
}

ArgumentParser::ArgParser::IntArgument& ArgumentParser::ArgParser::IntArgument::StoreValues(std::vector<int>& values){
    isStored = true;
    storedValues = &values;
    return *this;
}

ArgumentParser::ArgParser::IntArgument& ArgumentParser::ArgParser::IntArgument::MultiValue() {
    return MultiValue(1);
}

ArgumentParser::ArgParser::IntArgument& ArgumentParser::ArgParser::IntArgument::MultiValue(const int& count) {
    isMultiValue = true;
    minCount = count;
    return *this;
}

ArgumentParser::ArgParser::IntArgument& ArgumentParser::ArgParser::IntArgument::Positional() {
    isPositional = true;
    return *this;
}

bool ArgumentParser::ArgParser::FlagArgument::wordFind(std::string word) {
    return false;
}

bool ArgumentParser::ArgParser::FlagArgument::wordConstFind(std::string word) {
    return word.find(longName) != std::string::npos || word.find(name) != std::string::npos;
}

void ArgumentParser::ArgParser::FlagArgument::setValue(bool val){
    value = val;
}

void ArgumentParser::ArgParser::FlagArgument::store()  {
    if (isStored) {
        if (storedValue != nullptr) {
            *storedValue = value;
        }
    }
}

bool ArgumentParser::ArgParser::FlagArgument::isValid() const {
    return true;
}

ArgumentParser::ArgParser::FlagArgument& ArgumentParser::ArgParser::FlagArgument::Default(const bool& def){
    isDefault = true;
    defaultValue = def;
    return *this;
}

ArgumentParser::ArgParser::FlagArgument& ArgumentParser::ArgParser::FlagArgument::StoreValue(bool& value){
    isStored = true;
    storedValue = &value;
    return *this;
}

ArgumentParser::ArgParser::FlagArgument& ArgumentParser::ArgParser::FlagArgument::MultiValue() {
    return MultiValue(1);
}

ArgumentParser::ArgParser::FlagArgument& ArgumentParser::ArgParser::FlagArgument::MultiValue(const int& count) {
    isMultiValue = true;
    minCount = count;
    return *this;
}

ArgumentParser::ArgParser::FlagArgument& ArgumentParser::ArgParser::FlagArgument::Positional() {
    isPositional = true;
    return *this;
}