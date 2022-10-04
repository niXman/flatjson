cmake_minimum_required(VERSION 3.1)
project(c-style)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -fsanitize=address -fsanitize=leak")

add_definitions(
    -UNDEBUG
)

include_directories(
    ../../include
)

set(SOURCES
    ../../include/flatjson/flatjson.hpp
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
    ${PROJECT_NAME}
    pthread
)
