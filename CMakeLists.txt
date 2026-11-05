cmake_minimum_required(VERSION 3.15)
project(lab2_warehouse CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lab2_warehouse main.cpp)

target_include_directories(lab2_warehouse PRIVATE "C:/msys64/ucrt64/include")
target_link_directories(lab2_warehouse PRIVATE "C:/msys64/ucrt64/lib")
target_link_libraries(lab2_warehouse PRIVATE fltk ole32 comctl32 ws2_32 winspool)
target_link_options(lab2_warehouse PRIVATE -mwindows)