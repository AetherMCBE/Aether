cmake_minimum_required(VERSION 3.30)
project(Aether)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)

CPMAddPackage("gh:gabime/spdlog@1.15.2")

add_library(Aether SHARED dll/src/Main.cpp)

target_link_libraries(Aether PRIVATE spdlog::spdlog)