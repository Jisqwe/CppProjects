#include <iostream>

#include "../lib/TimeFormat.h"
#include "../lib/ArgParser.h"
#include "../lib/RoutesGetter.h"

//Example of code usage

int main(int argc, char** argv) {
    ArgumentParser::ArgParser parser("My parser");
    parser.AddStringArgument("from");
    parser.AddStringArgument("to");
    parser.AddStringArgument("date");
    std::string apiKey = getApiKey();
    if (!parser.Parse(argc, argv)){
        std::cerr << "Wrong arguments of command line\nExample of command line: ./labwork6 --from=c2 --to=c62 --date=2025-02-28" << std::endl;
        return 1;
    }
    if (apiKey == "0"){
        std::cerr << "Api key was not founded" << std::endl;
        return 1;
    }
    std::cout << "\nМаршруты из первого города во второй:\n" << std::endl;
    getRoutes(parser.GetStringValue("from"), parser.GetStringValue("to"), apiKey, parser.GetStringValue("date"));
    std::cout << "\nМаршруты обратно из второго города в первый:\n" << std::endl;
    getRoutes(parser.GetStringValue("to"), parser.GetStringValue("from"), apiKey, parser.GetStringValue("date")); 
    return 0;
}