cmake_minimum_required(VERSION 3.5)
project(Shingles)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

configure_file(input.txt input.txt COPYONLY)

set(SOURCE_FILES main.cpp Dictionary.cpp Word.cpp Gram.cpp Parser.cpp)
add_executable(shingles ${SOURCE_FILES})