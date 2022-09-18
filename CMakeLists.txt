cmake_minimum_required(VERSION 3.1)
project(flatjson)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -fsanitize=address") #-fsanitize=address

add_definitions(
	-UNDEBUG
)

set(SOURCES
	flatjson.hpp
	main.cpp
    inst.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
	${PROJECT_NAME}
	pthread
)
