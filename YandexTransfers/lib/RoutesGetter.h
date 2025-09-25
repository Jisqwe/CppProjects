#pragma once

#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include "TimeFormat.h"

std::string getApiKey();

void getRoutes(const std::string& from, const std::string& to, const std::string& apiKey, const std::string& date);