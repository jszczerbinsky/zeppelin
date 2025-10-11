cmake_minimum_required(VERSION 4.0)

project(zeppelin)

# Determine program version
find_package(Git)
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --dirty
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE PROGRAM_VERSION
	RESULT_VARIABLE ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
if(PROGRAM_VERSION STREQUAL "")
	set(PROGRAM_VERSION "v0.0.0-unknown")
endif()
message(STATUS "Determined version: ${PROGRAM_VERSION}")

# Setting target platform
set(TARGET_PLATFORM "Linux" CACHE STRING "Target platform: Linux / Windows")
set_property(CACHE TARGET_PLATFORM PROPERTY STRINGS "Linux" "Windows")
set(ALLOWED_TARGET_PLATFORMS "Linux" "Windows")
if(NOT TARGET_PLATFORM IN_LIST ALLOWED_TARGET_PLATFORMS)
	message(FATAL_ERROR "Unsupported target platform '${TARGET_PLATFORM}'")
endif()

# Setting build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type: Debug / Release" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
set(ALLOWED_CMAKE_BUILD_TYPE "Debug" "Release")
if(NOT CMAKE_BUILD_TYPE IN_LIST ALLOWED_CMAKE_BUILD_TYPE)
	message(FATAL_ERROR "Unsupported build type '${CMAKE_BUILD_TYPE}'")
endif()

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

# Embedding binary files
set(EMBEDDED_PRECOMPUTED_FILE precomputed.bin)
set(EMBEDDED_PRECOMPUTED_OBJ ${CMAKE_CURRENT_BINARY_DIR}/precomputed.o)
add_custom_command(
	OUTPUT ${EMBEDDED_PRECOMPUTED_OBJ}
	COMMAND ld -r -b binary ${EMBEDDED_PRECOMPUTED_FILE} -o ${EMBEDDED_PRECOMPUTED_OBJ}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res
)

set(EMBEDDED_WEIGHTS_FILE weights.bin)
set(EMBEDDED_WEIGHTS_OBJ ${CMAKE_CURRENT_BINARY_DIR}/weights.o)
add_custom_command(
	OUTPUT ${EMBEDDED_WEIGHTS_OBJ}
	COMMAND ld -r -b binary ${EMBEDDED_WEIGHTS_FILE} -o ${EMBEDDED_WEIGHTS_OBJ}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res
)


# Choosing (cross) compiler 
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(TARGET_PLATFORM STREQUAL "Linux")
		set(CMAKE_C_COMPILER gcc)
	else()
		set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
		set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if(TARGET_PLATFORM STREQUAL "Windows")
		set(CMAKE_C_COMPILER gcc)
	else()
		message(FATAL_ERROR "Cross compiling to Linux is not supported on Windows yet")
	endif()
endif()

# Defining strings
add_compile_definitions(TARGET_PLATFORM="${TARGET_PLATFORM}")
add_compile_definitions(PROGRAM_VERSION="${PROGRAM_VERSION}")

# Choosing compiler flags
set(CMAKE_C_FLAGS_DEBUG "-Wall -ggdb2 -pg -DDEBUG_INTERFACE")
set(CMAKE_C_FLAGS_RELEASE "-Wall -O3")

# Building executable
add_executable(zeppelin ${SOURCE_FILES} ${EMBEDDED_PRECOMPUTED_OBJ} ${EMBEDDED_WEIGHTS_OBJ})

# Unit tests
find_package(Python3 REQUIRED)

enable_testing()

add_test(
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/run_tests.py
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tools/
)
