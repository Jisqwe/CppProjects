#include "TimeFormat.h"
#include <stdexcept>
#include <string>
#include <cstdint>

static long long daysGone(int y, unsigned m, unsigned d) {
    y -= (m <= 2);
    int era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = static_cast<unsigned>(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<long long>(era) * 146097 + static_cast<long long>(doe) - 719468;
}

std::uint64_t calcTime(const std::string& s) {
    if (!((s.size() == 20 && s[19] == 'Z') || s.size() == 25)) {
        throw std::runtime_error("Неверный формат времени: " + s);
    }
    int y = std::stoi(s.substr(0, 4));
    unsigned mo = static_cast<unsigned>(std::stoi(s.substr(5, 2)));
    unsigned da = static_cast<unsigned>(std::stoi(s.substr(8, 2)));
    int hh = std::stoi(s.substr(11, 2));
    int mm = std::stoi(s.substr(14, 2));
    int ss = std::stoi(s.substr(17, 2));
    long long days = daysGone(y, mo, da);
    long long as_if_utc = days * 86400LL + hh * 3600LL + mm * 60LL + ss;
    int tz_sec = 0;
    if (s.size() == 25) {
        char sign = s[19];
        int off_h = std::stoi(s.substr(20, 2));
        int off_m = std::stoi(s.substr(23, 2));
        int off = off_h * 3600 + off_m * 60;
        tz_sec = (sign == '-') ? -off : off;
    }
    return static_cast<std::uint64_t>(as_if_utc - tz_sec);
}

std::string calcDuration(const std::string& dep, const std::string& arr) {
    std::uint64_t d = calcTime(dep);
    std::uint64_t a = calcTime(arr);
    if (a <= d) {
        return "0 ч. 0 мин.";
    }
    std::uint64_t dur = a - d;
    return std::to_string(dur / 3600) + " ч. " + std::to_string((dur % 3600) / 60) + " мин.";
}

std::string formatDate(const std::string& s) {
    if (s.size() == 20 && s.back() == 'Z') {
        return s.substr(0, 10) + " в " + s.substr(11, 8) + " +00:00 GMT";
    }
    if (s.size() == 25) {
        return s.substr(0, 10) + " в " + s.substr(11, 8) + " " + s.substr(19, 6) + " GMT";
    }
    return s;
}