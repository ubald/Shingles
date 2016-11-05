cmake_minimum_required(VERSION 3.5)

project(Shingles)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

include_directories(
    vendor/jsoncpp
    vendor/linenoise
    vendor/optionparser
)

set(VENDOR_SOURCES
    vendor/jsoncpp/jsoncpp.cpp
    vendor/linenoise/linenoise.c
)

set(SOURCE_FILES
    main.cpp
    Parser.cpp
    Dictionary.cpp
    Word.cpp
    Gram.cpp
    utils/color.cpp
    ${VENDOR_SOURCES})

add_executable(shingles ${SOURCE_FILES})