cmake_minimum_required(VERSION 3.10)
project(pack-unpack)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=return-type -fno-exceptions -msse4.2")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /EHsc /GR- /D_HAS_EXCEPTIONS=0")
endif()

add_definitions(
    -UNDEBUG
)

include_directories(
    ../include
)

set(SOURCES
    ../include/flatjson/types.hpp
    ../include/flatjson/flatjson.hpp
    ../include/flatjson/state.hpp
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
