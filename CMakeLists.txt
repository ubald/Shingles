cmake_minimum_required(VERSION 3.5)
project(Shingles)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

configure_file(books/test.txt books/test.txt COPYONLY)
configure_file(books/alice.txt books/alice.txt COPYONLY)

set(SOURCE_FILES main.cpp Dictionary.cpp Word.cpp Gram.cpp)
add_executable(shingles ${SOURCE_FILES})