cmake_minimum_required(VERSION 3.1)
project(flatjson)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Werror=return-type -fsanitize=address")

add_definitions(
	-UNDEBUG
    -D__FJ__SUPPORTS_STDIO
    -D__FJ__DONT_PACK_TOKENS
)

include_directories(
    ../include
)

set(SOURCES
    ../include/flatjson/flatjson.hpp
	main.cpp
    inst.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
	${PROJECT_NAME}
	pthread
)
