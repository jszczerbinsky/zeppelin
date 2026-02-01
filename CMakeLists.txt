cmake_minimum_required(VERSION 3.15)

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

# Setting executable name
set(EXE_NAME "zeppelin" CACHE STRING "Executable name:")

# Setting target platform
set(TARGET_PLATFORM "Linux" CACHE STRING "Target platform: Linux / Windows")
set_property(CACHE TARGET_PLATFORM PROPERTY STRINGS "Linux" "Windows")
set(ALLOWED_TARGET_PLATFORMS "Linux" "Windows")
if(NOT TARGET_PLATFORM IN_LIST ALLOWED_TARGET_PLATFORMS)
	message(FATAL_ERROR "Unsupported target platform '${TARGET_PLATFORM}'")
endif()

# Setting target architecture 
set(TARGET_ARCH "x86_64" CACHE STRING "Target arch: x86 / ARM")
set_property(CACHE TARGET_ARCH PROPERTY STRINGS "x86_64" "aarch64")
set(ALLOWED_TARGET_ARCH "x86_64" "aarch64")
if(NOT TARGET_ARCH IN_LIST ALLOWED_TARGET_ARCH)
	message(FATAL_ERROR "Unsupported target arch '${TARGET_ARCH}'")
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

# Setting evaluation type
if(NOT EVAL_TYPE)
	set(EVAL_TYPE "EVAL_NNUE" CACHE STRING "Eval type: NNUE / EVAL_MATERIAL_ONLY" FORCE)
endif()
set_property(CACHE EVAL_TYPE PROPERTY STRINGS "EVAL_NNUE" "EVAL_MATERIAL_ONLY")
set(ALLOWED_EVAL_TYPE "EVAL_NNUE" "EVAL_MATERIAL_ONLY")
if(NOT EVAL_TYPE IN_LIST ALLOWED_EVAL_TYPE)
	message(FATAL_ERROR "Unsupported eval type '${EVAL_TYPE}'")
endif()

# Setting vectorization support
if(NOT VECTORIZATION_METHOD)
	set(VECTORIZATION_METHOD "VECT_AVX2" CACHE STRING "Vectorization type: VECT_NONE / VECT_AVX2 / VECT_NEON" FORCE)
endif()
set_property(CACHE VECTORIZATION_METHOD PROPERTY STRINGS "VECT_NONE" "VECT_AVX2" "VECT_NEON")
set(ALLOWED_VECTORIZATION_METHOD "VECT_NONE" "VECT_AVX2" "VECT_NEON")
if(NOT VECTORIZATION_METHOD IN_LIST ALLOWED_VECTORIZATION_METHOD)
	message(FATAL_ERROR "Unsupported vectorization method '${VECTORIZATION_METHOD}'")
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
	src/nnue.c
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(SOURCE_FILES
		${SOURCE_FILES}
		src/debug.c
	)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

set(LD ld)

# Detecting cross compilation
if(CMAKE_HOST_SYSTEM_NAME STREQUAL TARGET_PLATFORM AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL TARGET_ARCH)
	set(IS_CROSS_COMPILATION OFF)
else()
	set(IS_CROSS_COMPILATION ON)
endif()

# Choosing (cross) compiler 
if(TARGET_PLATFORM STREQUAL "Linux")
	set(CMAKE_EXE_LINKER_FLAGS "-pthread")
	if(TARGET_ARCH STREQUAL "x86_64")
		set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc)
		if(IS_CROSS_COMPILATION)
			set(LD x86_64-linux-gnu-ld)
		endif()
	else()
		set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
		if(IS_CROSS_COMPILATION)
			set(LD aarch64-linux-gnu-ld)
		endif()
	endif()
else()
	set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ -pthread")
	if(TARGET_ARCH STREQUAL "x86_64")
		set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
		if(IS_CROSS_COMPILATION)
			set(LD x86_64-w64-mingw32-ld)
		endif()
	else()
		message(FATAL_ERROR "Windows on ARM CPUs is not supported yet")
	endif()
endif()

# Ensuring correct vertorization
if(VECTORIZATION_METHOD STREQUAL "VECT_AVX2" AND NOT TARGET_ARCH STREQUAL "x86_64")
	message(FATAL_ERROR "Trying to use AVX2 on non-x86 CPU")
elseif(VECTORIZATION_METHOD STREQUAL "VECT_NEON" AND NOT TARGET_ARCH STREQUAL "aarch64")
	message(FATAL_ERROR "Trying to use NEON on non-ARM CPU")
endif()

# Embedding binary files
set(EMBEDDED_PRECOMPUTED_FILE precomputed.bin)
set(EMBEDDED_PRECOMPUTED_OBJ ${CMAKE_CURRENT_BINARY_DIR}/precomputed.o)
add_custom_command(
	OUTPUT ${EMBEDDED_PRECOMPUTED_OBJ}
	COMMAND ${LD} -r -b binary ${EMBEDDED_PRECOMPUTED_FILE} -o ${EMBEDDED_PRECOMPUTED_OBJ}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res
)

set(EMBEDDED_WEIGHTS_FILE nnue_weights.bin)
set(EMBEDDED_WEIGHTS_OBJ ${CMAKE_CURRENT_BINARY_DIR}/nnue_weights.o)
add_custom_command(
	OUTPUT ${EMBEDDED_WEIGHTS_OBJ}
	COMMAND ${LD} -r -b binary ${EMBEDDED_WEIGHTS_FILE} -o ${EMBEDDED_WEIGHTS_OBJ}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res
)

set(EMBEDDED_BIAS_FILE nnue_bias.bin)
set(EMBEDDED_BIAS_OBJ ${CMAKE_CURRENT_BINARY_DIR}/nnue_bias.o)
add_custom_command(
	OUTPUT ${EMBEDDED_BIAS_OBJ}
	COMMAND ${LD} -r -b binary ${EMBEDDED_BIAS_FILE} -o ${EMBEDDED_BIAS_OBJ}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res
)

set(EMBEDDED_FILES ${EMBEDDED_PRECOMPUTED_OBJ} ${EMBEDDED_FILES} ${EMBEDDED_WEIGHTS_OBJ} ${EMBEDDED_BIAS_OBJ})

# Defining strings
add_compile_definitions(TARGET_PLATFORM="${TARGET_PLATFORM}")
add_compile_definitions(PROGRAM_VERSION="${PROGRAM_VERSION}")

# Choosing compiler flags
set(COMMON_FLAGS "-Wall -Wextra -Wformat -Wconversion -Wtype-limits -Woverflow -Wsign-conversion -Wsign-compare -Wfloat-conversion -Wsizeof-pointer-div -Wlogical-op -Wcast-qual -Wshadow -pedantic")
if(VECTORIZATION_METHOD STREQUAL "VECT_AVX2")
	set(COMMON_FLAGS "${COMMON_FLAGS} -mavx2")
elseif(VECTORIZATION_METHOD STREQUAL "VECT_NEON")

endif()
set(CMAKE_C_FLAGS_DEBUG "${COMMON_FLAGS} -ggdb2 -pg -DDEBUG_INTERFACE -D${EVAL_TYPE} -D${VECTORIZATION_METHOD}")
set(CMAKE_C_FLAGS_RELEASE "${COMMON_FLAGS} -O3 -D${EVAL_TYPE} -D${VECTORIZATION_METHOD}")

# Building executable
add_executable(zeppelin ${SOURCE_FILES} ${EMBEDDED_FILES})
set_target_properties(zeppelin PROPERTIES 
	OUTPUT_NAME ${EXE_NAME}
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
)
target_link_libraries(zeppelin m)

# Unit tests
#find_package(Python3 REQUIRED)

#enable_testing()

#add_test(
#	COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/run_tests.py
#	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tools/
#)
