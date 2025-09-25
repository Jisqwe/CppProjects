#include "RoutesGetter.h"

using json = nlohmann::json;

std::string getApiKey() {
    const char* apiKey = std::getenv("YANDEX_KEY");
    if (apiKey) {
        return apiKey;
    } else {
        std::cerr << "Переменная окружения YANDEX_KEY не найдена" << std::endl;
        return "0";
    }
}

bool isCached(const std::string& filename) {
    std::ifstream cacheFile("cache/" + filename);
    return cacheFile.is_open();
}

std::string readFromCache(const std::string& filename) {
    std::ifstream cacheFile("cache/" + filename);
    std::stringstream buffer;
    buffer << cacheFile.rdbuf();
    return buffer.str();
}

void createCacheFile(const std::string& filename, const std::string& data) {
    if (!std::filesystem::exists("cache")) {
        std::filesystem::create_directory("cache");
    }
    std::ofstream cacheFile("cache/" + filename);
    cacheFile << data;
}

void writeData(const json& data){
    std::string fromStation = data["search"]["from"].value("title", "Нет данных");
    std::string toStation = data["search"]["to"].value("title", "Нет данных");
    for (const auto& segment : data["segments"]) {
        std::size_t temp = 1;
        if (segment.contains("details") && segment["details"].is_array()) {
            temp = 0;
            for (const auto& d : segment["details"]) {
                if (d.contains("arrival")) {
                    ++temp;
                }
            }
            if (temp == 0) {
                temp = 1;
            }
        }
        const std::size_t transfers = (temp > 0) ? (temp - 1) : 0;
        if (transfers > 1) {
            continue;
        }

        std::string departure = segment.value("departure", "Нет данных");
        std::string arrival = segment.value("arrival", "Нет данных");

        std::cout << "Маршрут: " << fromStation << " -> " << toStation << std::endl;
        std::cout << "Отправление: " << formatDate(departure) << std::endl;
        std::cout << "Прибытие: " << formatDate(arrival) << std::endl;
        std::string duration = calcDuration(departure, arrival);
        std::cout << "Время в пути: " << duration << std::endl;

        if (segment.contains("details") && segment["details"].is_array()) {
            std::cout << "Детали маршрута:" << std::endl;
            for (const auto& detail : segment["details"]) {
                if (!detail.contains("arrival")) {
                    continue;
                }

                std::string detailDeparture = detail.value("departure", "Нет данных");
                std::string detailArrival = detail.value("arrival", "Нет данных");
                std::string detailFrom = detail["from"].value("title", "Нет данных");
                std::string detailTo = detail["to"].value("title", "Нет данных");
                std::string transportType = detail["thread"].value("transport_type", "Нет данных");
                std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

                std::cout << "  Путь: " << detailFrom << " -> " << detailTo << std::endl;
                std::cout << "  Тип транспорта: " << transportType << std::endl;
                std::cout << "  Отправление: " << formatDate(detailDeparture) << std::endl;
                std::cout << "  Прибытие: " << formatDate(detailArrival) << std::endl;
                std::string segment_duration = calcDuration(detailDeparture, detailArrival);
                std::cout << "  Время в пути: " << segment_duration << std::endl;
            }
        } else {
            std::string transportType = segment["thread"].value("transport_type", "Нет данных");
            std::cout << "Тип транспорта: " << transportType << std::endl;
        }
        std::cout << "------------------------------------------" << std::endl;
    }
}

void getRoutes(const std::string& from, const std::string& to, const std::string& apiKey, const std::string& date) {
    std::string filename = from + to + date + ".json";
    if (isCached(filename)) {
        std::string cachedData = readFromCache(filename);
        json data = json::parse(cachedData);
        if (data.contains("segments")) {
            writeData(data);
        } else {
            std::cout << "Маршруты не найдены." << std::endl;
        }
        return;
    }
    std::string url = "https://api.rasp.yandex.net/v3.0/search/";
    cpr::Parameters params = {
        {"apikey", apiKey},
        {"from", from},
        {"to", to},
        {"lang", "ru_RU"},
        {"format", "json"},
        {"date", date},
        {"page", "1"},
        {"limit", "100"},
        {"transfers", "true"}
    };
    try {
        cpr::Response response = cpr::Get(cpr::Url{url}, params);
        if (response.status_code == 200) {
            createCacheFile(filename, response.text);
            json data = json::parse(response.text);
            if (data.contains("segments") && data["segments"].is_array()) {
                writeData(data);
            }
        } else {
            std::cout << "Ошибка при выполнении запроса: " << response.status_code << std::endl;
        }
    } catch (const std::exception& exception) {
        std::cerr << "Ошибка: " << exception.what() << std::endl;
    }
}