cmake_minimum_required(VERSION 3.1)
project(flatjson-test)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=return-type -fno-exceptions -march=native -m32")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -D__FJ__ADDRESS_SANITIZER_ENABLED") #-fsanitize=address
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /EHsc /GR- /D_HAS_EXCEPTIONS=0")
endif()

add_definitions(
    -UNDEBUG
    #-DFJ_NO_TOPLEV_FJSON
    #-DFJ_NO_TOPLEV_IO
    #-DFJ_DONT_USE_SIMD
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
)
