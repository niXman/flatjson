cmake_minimum_required(VERSION 3.10)
project(flatjson-test)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=return-type -fno-exceptions -march=native")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=leak")
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
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
    ../include/flatjson/version.hpp
    ../include/flatjson/stringview.hpp
    ../include/flatjson/error.hpp
    ../include/flatjson/types.hpp
    ../include/flatjson/chunks.hpp
    ../include/flatjson/args.hpp
    ../include/flatjson/conv.hpp
    ../include/flatjson/iterator.hpp
    ../include/flatjson/dump.hpp
    ../include/flatjson/parse.hpp
    ../include/flatjson/fjson.hpp
    ../include/flatjson/flatjson.hpp
    ../include/flatjson/compare.hpp
    ../include/flatjson/serialize.hpp
    ../include/flatjson/state.hpp
    main.cpp
    inst.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
	${PROJECT_NAME}
)
