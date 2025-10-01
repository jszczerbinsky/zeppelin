cmake_minimum_required(VERSION 4.0)

project(zeppelin)

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

# Choosing (cross) compiler 
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(TARGET_PLATFORM STREQUAL "Linux")
		set(CMAKE_C_COMPILER gcc)
	else()
		set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
		set(CMAKE_EXE_LINKER_FLAGS "-static")
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if(TARGET_PLATFORM STREQUAL "Windows")
		set(CMAKE_C_COMPILER gcc)
	else()
		message(FATAL_ERROR "Cross compiling to Linux is not supported on Windows yet")
	endif()
endif()

# Choosing compiler flags
set(CMAKE_C_FLAGS_DEBUG "-Wall -ggdb2 -pg -DDEBUG_INTERFACE -DTARGET_PLATFORM=${TARGET_PLATFORM}")
set(CMAKE_C_FLAGS_RELEASE "-Wall -O3 -DTARGET_PLATFORM=${TARGET_PLATFORM}")

# Building executable
add_executable(zeppelin ${SOURCE_FILES})

# Unit tests
find_package(Python3 REQUIRED)

enable_testing()

add_test(
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_tests.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zeppelin${CMAKE_EXECUTABLE_SUFFIX}
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
