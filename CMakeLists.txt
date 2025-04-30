cmake_minimum_required(VERSION 3.30)
project(Aether)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(Aether SHARED dll/src/Main.cpp)