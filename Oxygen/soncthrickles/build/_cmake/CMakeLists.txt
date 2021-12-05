cmake_minimum_required(VERSION 3.10)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()


project(Sonic3AIR)


# Determine whether to support discord integration
set(USE_DISCORD true)
if (UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
	link_directories(/opt/vc/lib)	# Needed when building on Rasperry Pi
	set(USE_DISCORD false)
endif()
message(STATUS "USE_DISCORD = ${USE_DISCORD}")



set(CMAKE_CXX_STANDARD 17)

set(WORKSPACE_DIR ../../../..)

#add_definitions(-DDEBUG)


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

	# GCC only
	#set(CMAKE_CXX_FLAGS "-Wall")
	set(CMAKE_CXX_FLAGS_DEBUG "-g2")		# Include debug information
	set(CMAKE_CXX_FLAGS_RELEASE "-g0")		# Leave out debug information
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")		# Full optimization
	add_compile_options(-Wno-psabi)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

	# Clang only
	#set(CMAKE_C_COMPILER   "/usr/bin/clang")
	#set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
	#set(CMAKE_AR      "/usr/bin/llvm-ar" CACHE PATH "archive")
	#set(CMAKE_LINKER  "/usr/bin/llvm-ld" CACHE PATH "linker")
	#set(CMAKE_NM      "/usr/bin/llvm-nm" CACHE PATH "nm")
	#set(CMAKE_OBJDUMP "/usr/bin/llvm-objdump" CACHE PATH "objdump")
	#set(CMAKE_RANLIB  "/usr/bin/llvm-ranlib" CACHE PATH "ranlib")

endif()


find_package(OpenGL REQUIRED)


# SDL2 sources as virtual subdirectory "SDL"
set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(${WORKSPACE_DIR}/framework/external/sdl/SDL2 SDL)

# zlib sources as virtual subdirectory "zlib"
include_directories(${WORKSPACE_DIR}/framework/external/zlib/zlib)		# Included before add_subdirectory so that the examples will build
add_subdirectory(${WORKSPACE_DIR}/framework/external/zlib/zlib zlib)



include_directories(SDL/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libogg/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/include)
include_directories(${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/lib)
include_directories(build/zlib)		# Needed for zconf.h
include_directories(${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip)
include_directories(${WORKSPACE_DIR}/librmx/source)
include_directories(${WORKSPACE_DIR}/librmx/source/rmxmedia/glew)
include_directories(${WORKSPACE_DIR}/Oxygen/lemonscript/source)
include_directories(${WORKSPACE_DIR}/Oxygen/oxygenengine/source)
include_directories(${WORKSPACE_DIR}/Oxygen/soncthrickles/source)
include_directories(${WORKSPACE_DIR}/Oxygen/soncthrickles/source/external)



# ogg vorbis

# TODO: I tried creating config_types.h by calling "./configure" by a add_custom_command,
#       but that did not work; instead the solution is to simply add the generated file to versioning

file(GLOB OGGVORBIS_SOURCES ${WORKSPACE_DIR}/framework/external/ogg-vorbis/libogg/src/*.c
							${WORKSPACE_DIR}/framework/external/ogg-vorbis/libvorbis/lib/*.c)

add_library(oggvorbis ${OGGVORBIS_SOURCES})



# minizip

file(GLOB MINIZIP_SOURCES ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/ioapi.c
						  ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/unzip.c
						  ${WORKSPACE_DIR}/framework/external/zlib/zlib/contrib/minizip/zip.c)

add_library(minizip ${MINIZIP_SOURCES})

target_link_libraries(minizip zlibstatic)



# rmxbase

file(GLOB RMXBASE_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxbase/*.cpp
						  ${WORKSPACE_DIR}/librmx/source/rmxbase/jsoncpp/*.cpp)

add_library(rmxbase ${RMXBASE_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(rmxbase PRIVATE ${WORKSPACE_DIR}/librmx/source/rmxbase.h)
endif()

target_link_libraries(rmxbase stdc++fs)
target_link_libraries(rmxbase zlibstatic)



# rmxmedia

file(GLOB RMXMEDIA_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxmedia/*.cpp
						   ${WORKSPACE_DIR}/librmx/source/rmxmedia/glew/*.c)

add_library(rmxmedia ${RMXMEDIA_SOURCES})

target_link_libraries(rmxmedia rmxbase)
target_link_libraries(rmxmedia SDL2-static)
target_link_libraries(rmxmedia ${OPENGL_gl_LIBRARY})



# rmxext_oggvorbis

file(GLOB RMXEXT_OGGVORBIS_SOURCES ${WORKSPACE_DIR}/librmx/source/rmxext_oggvorbis/*.cpp)

add_library(rmxext_oggvorbis ${RMXEXT_OGGVORBIS_SOURCES})

target_link_libraries(rmxext_oggvorbis rmxmedia)
target_link_libraries(rmxext_oggvorbis oggvorbis)



# lemonscript

file(GLOB_RECURSE LEMONSCRIPT_SOURCES ${WORKSPACE_DIR}/Oxygen/lemonscript/source/lemon/*.cpp)

add_library(lemonscript ${LEMONSCRIPT_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(lemonscript PRIVATE ${WORKSPACE_DIR}/Oxygen/lemonscript/source/lemon/pch.h)
endif()

target_link_libraries(lemonscript rmxbase)



# Oxygen Core Library

file(GLOB_RECURSE OXYGEN_NETCORE_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/*.cpp)

add_library(oxygen_netcore ${OXYGEN_NETCORE_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(oxygen_netcore PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen_netcore/pch.h)
endif()

target_link_libraries(oxygen_netcore rmxbase)



# Oxygen Engine Library

file(GLOB_RECURSE OXYGEN_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen/*.cpp)

add_library(oxygen ${OXYGEN_SOURCES})

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(oxygen PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/oxygen/pch.h)
endif()

target_link_libraries(oxygen minizip)
target_link_libraries(oxygen rmxmedia)
target_link_libraries(oxygen rmxext_oggvorbis)
target_link_libraries(oxygen lemonscript)
target_link_libraries(oxygen oxygen_netcore)



# OxygenApp

file(GLOB_RECURSE ENGINEAPP_SOURCES ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/engineapp/*.cpp)

# TODO: CMake seems to use a different working directory for this, so "Oxygen" is intentionally missing here
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${WORKSPACE_DIR}/oxygenengine")

add_executable(OxygenApp ${ENGINEAPP_SOURCES})

if (UNIX AND NOT APPLE)
	# Different executable name on Linux
	set_target_properties(OxygenApp PROPERTIES OUTPUT_NAME "oxygenapp_linux")
endif ()

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(OxygenApp PRIVATE ${WORKSPACE_DIR}/Oxygen/oxygenengine/source/engineapp/pch.h)
endif()

target_link_libraries(OxygenApp oxygen)



# discord_game_sdk

if (USE_DISCORD)
	file(GLOB_RECURSE DISCORD_SOURCES ${WORKSPACE_DIR}/Oxygen/soncthrickles/source/external/discord_game_sdk/cpp/*.cpp)

	add_library(discord_game_sdk_source ${DISCORD_SOURCES})

	link_directories("${WORKSPACE_DIR}/Oxygen/soncthrickles/source/external/discord_game_sdk/lib/x86_64")

	target_link_libraries(discord_game_sdk_source discord_game_sdk)

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
endif()



# Sonic3AIR

file(GLOB_RECURSE SONIC3AIR_SOURCES ${WORKSPACE_DIR}/Oxygen/soncthrickles/source/sonic3air/*.cpp)

# TODO: CMake seems to use a different working directory for this, so "Oxygen" is intentionally missing here
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${WORKSPACE_DIR}/soncthrickles")

add_executable(Sonic3AIR ${SONIC3AIR_SOURCES})

if (UNIX AND NOT APPLE)
	# Different executable name on Linux
	set_target_properties(Sonic3AIR PROPERTIES OUTPUT_NAME "sonic3air_linux")
endif()

target_compile_definitions(Sonic3AIR PUBLIC ENDUSER)

if (NOT CMAKE_VERSION VERSION_LESS "3.16.0")
	target_precompile_headers(Sonic3AIR PRIVATE ${WORKSPACE_DIR}/Oxygen/soncthrickles/source/sonic3air/pch.h)
endif()

target_link_libraries(Sonic3AIR oxygen)
if (USE_DISCORD)
	target_link_libraries(Sonic3AIR discord_game_sdk_source)
endif()
