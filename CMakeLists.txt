cmake_minimum_required(VERSION 3.30)
project(Aether)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)

CPMAddPackage("gh:gabime/spdlog@1.15.2")

set(Boost_OPTIONS
        "BOOST_USE_STATIC_LIBS ON"
        "BOOST_NO_WARN_NEW_VERSIONS ON"
        "BOOST_ENABLE_CMAKE ON"
        "BOOST_INCLUDE_LIBRARIES container")
CPMAddPackage(
        NAME Boost
        VERSION 1.87.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.xz
        URL_HASH SHA256=7da75f171837577a52bbf217e17f8ea576c7c246e4594d617bfde7fafd408be5
        OPTIONS ${Boost_OPTIONS}
)

add_library(AetherCore
        libs/core/src/Core.cpp)
target_include_directories(AetherCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/core/include)
target_link_libraries(AetherCore PUBLIC spdlog::spdlog Boost::container)

add_library(AetherIPC
    libs/ipc/src/IpcChannel.cpp)
target_include_directories(AetherIPC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/ipc/include)
target_link_libraries(AetherIPC PUBLIC AetherCore)

# TODO don't push this lellll
add_executable(ipc_test tests/test.cpp)
target_link_libraries(ipc_test PRIVATE AetherIPC)

add_library(Aether SHARED dll/src/Aether.cpp)

target_link_libraries(Aether PRIVATE spdlog::spdlog)