cmake_minimum_required(VERSION 3.15)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")

project(YandexTransfers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.10.5
)

set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cpr)

add_executable(YandexTransfers
  bin/main.cpp
  lib/ArgParser.cpp
  lib/TimeFormat.cpp
  lib/RoutesGetter.cpp
)

target_link_libraries(YandexTransfers
    cpr::cpr
    nlohmann_json::nlohmann_json
)