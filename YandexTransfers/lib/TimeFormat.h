#pragma once
#include <string>
#include <cstdint>

std::string formatDate(const std::string& datetime);

uint64_t calcTime(const std::string& iso);

std::string calcDuration(const std::string& departure, const std::string& arrival);
