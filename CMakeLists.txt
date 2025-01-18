cmake_minimum_required(VERSION 3.3)

project(zeppelin)

set(SOURCE_FILES
	src/main.c
	src/movegen.c
	src/perft.c
	src/precomp.c
	src/uci.c
	src/game.c
	src/movegen.c
	src/dump.c
	src/search.c
	src/zobrist.c
	src/eval.c
	src/time.c
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(SOURCE_FILES
		${SOURCE_FILES}
		src/debug.c
	)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

set(CMAKE_C_FLAGS_DEBUG "-Wall -ggdb2 -DDEBUG_INTERFACE")
set(CMAKE_C_FLAGS_RELEASE "-Wall -O3")

add_executable(zeppelin ${SOURCE_FILES})


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_custom_target(test
		COMMAND python ${CMAKE_SOURCE_DIR}/test/test.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zeppelin${CMAKE_EXECUTABLE_SUFFIX}
	)
endif()
