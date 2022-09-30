cmake_minimum_required(VERSION 3.19)
project(SpatialReadTest)
set(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(/Users/binma/CLionProjects/SpatialReadTest/inc)

set(CMAKE_CXX_STANDARD 11)

add_executable(SpatialReadTest main.cpp cache.cc llc_prefetcher.cc block.cc llc_replacement.cc base_replacement.cc)