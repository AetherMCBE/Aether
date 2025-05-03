cmake_minimum_required(VERSION 3.30)
project(Aether)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)

CPMAddPackage("gh:gabime/spdlog@1.15.2")

add_library(AetherIPC
    libs/ipc/src/IpcChannel.cpp)
target_include_directories(AetherIPC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/ipc/include)
target_link_libraries(AetherIPC PUBLIC spdlog::spdlog)

# TODO don't push this lellll
add_executable(ipc_test tests/test.cpp)
target_link_libraries(ipc_test PRIVATE AetherIPC)

add_library(Aether SHARED dll/src/Aether.cpp)

target_link_libraries(Aether PRIVATE spdlog::spdlog)