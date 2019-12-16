cmake_minimum_required(VERSION 2.8)
project(flatjson)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -fsanitize=address") # -fsanitize=address

add_definitions(
	-UNDEBUG
	-D__FLATJSON__SHOULD_CHECK_OVERFLOW
)

set(SOURCES
	flatjson.hpp
	main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
	${PROJECT_NAME}
	pthread
)
