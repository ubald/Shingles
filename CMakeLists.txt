cmake_minimum_required(VERSION 3.5)

project(Shingles)

# Prerequisites
#find_package(PkgConfig REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

#pkg_check_modules(JSONCPP REQUIRED jsoncpp)
set(JSONCPP_INCLUDE_DIRS vendor/jsoncpp)

include_directories(${JSONCPP_INCLUDE_DIRS})
#link_directories(${JSONCPP_LIBRARY_DIRS})

set(VENDOR_SOURCES
    vendor/jsoncpp/jsoncpp.cpp
)

set(SOURCE_FILES
    main.cpp
    Parser.cpp
    Dictionary.cpp
    Word.cpp
    Gram.cpp
    ${VENDOR_SOURCES}
)

add_executable(shingles ${SOURCE_FILES})
#target_link_libraries(shingles PRIVATE ${JSONCPP_LIBRARIES})

configure_file(input.txt input.txt COPYONLY)